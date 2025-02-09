#include <SimpleFOC.h>
#include "motor.h"
#include "arduino.h"
#include "HX711.h"
#include "SPI.h"

MagneticSensorSPI sensor = MagneticSensorSPI(AS5047_SPI, HSPI_SS);
SPIClass SPI_2(HSPI);
BLDCMotor motor = BLDCMotor(7);
BLDCDriver6PWM driver = BLDCDriver6PWM(PIN_UH, PIN_UL, PIN_VH, PIN_VL, PIN_WH, PIN_WL);
int32_t motor_gui_position = 0;
/******************************************************************************/
static const float DEAD_ZONE_DETENT_PERCENT = 0.2;                // 死区制动百分率
static const float DEAD_ZONE_RAD = 1 * _PI / 180;                 // 死区RAD
static const float IDLE_VELOCITY_EWMA_ALPHA = 0.001;              // 怠速速度ewma alpha
static const float IDLE_VELOCITY_RAD_PER_SEC = 0.05;              // 怠速速度每秒钟弧度
static const uint32_t IDLE_CORRECTION_DELAY_MILLIS = 500;         // 怠速修正延迟millis
static const float IDLE_CORRECTION_MAX_ANGLE_RAD = 5 * _PI / 180; // 怠速校正最大角度rad
static const float IDLE_CORRECTION_RATE_ALPHA = 0.0005;           // 怠速修正率
/******************************************************************************/

PB_SmartKnobConfig config;

/******************************************************************************/
float current_detent_center = 0; // 当前位置
float idle_check_velocity_ewma = 0;
uint32_t last_idle_start = 0; // 上次空闲开始状态
/******************************************************************************/
float CLAMP(const float value, const float low, const float high)
{
  return value < low ? low : (value > high ? high : value);
}
/******************************************************************************/
// 移植来自 motor_task.cpp
// 切换模式后的参数配置
void knob_config(uint8_t mode)
{
  current_detent_center = motor.shaft_angle;
  config = configs[mode];
  Serial.println(config.text); // 串口打印当前的模式
}
float lv_encode_read()
{
  float currentangle = 0;
  currentangle = sensor.getAngle();

  return currentangle;
}
/******************************************************************************/
// 电机振动，按键时做为反馈  motor_task.cpp  第176行
void knob_haptic(float strength)
{
  motor.move(strength);
  for (uint8_t i = 0; i < 3; i++)
  {
    motor.loopFOC();
    vTaskDelay(1);
  }
  motor.move(-strength);
  for (uint8_t i = 0; i < 3; i++)
  {
    motor.loopFOC();
    vTaskDelay(1);
  }
  motor.move(0);
  motor.loopFOC();
}
/******************************************************************************/
// motor_task.cpp  第194行
void smart_knob(void)
{
  // If we are not moving and we're close to the center (but not exactly there), slowly adjust the centerpoint to match the current position
  idle_check_velocity_ewma = motor.shaft_velocity * IDLE_VELOCITY_EWMA_ALPHA + idle_check_velocity_ewma * (1 - IDLE_VELOCITY_EWMA_ALPHA);
  if (fabsf(idle_check_velocity_ewma) > IDLE_VELOCITY_RAD_PER_SEC)
  {
    last_idle_start = 0;
  }
  else
  {
    if (last_idle_start == 0)
    {
      last_idle_start = millis();
    }
  }
  if (last_idle_start > 0 && millis() - last_idle_start > IDLE_CORRECTION_DELAY_MILLIS && fabsf(motor.shaft_angle - current_detent_center) < IDLE_CORRECTION_MAX_ANGLE_RAD)
  {
    current_detent_center = motor.shaft_angle * IDLE_CORRECTION_RATE_ALPHA + current_detent_center * (1 - IDLE_CORRECTION_RATE_ALPHA);
  }

  // Check where we are relative to the current nearest detent; update our position if we've moved far enough to snap to another detent
  float angle_to_detent_center = motor.shaft_angle - current_detent_center;

  float snap_point_radians = config.position_width_radians * config.snap_point;
  float bias_radians = config.position_width_radians * config.snap_point_bias;
  float snap_point_radians_decrease = snap_point_radians + (config.position <= 0 ? bias_radians : -bias_radians);
  float snap_point_radians_increase = -snap_point_radians + (config.position >= 0 ? -bias_radians : bias_radians);

  int32_t num_positions = config.max_position - config.min_position + 1;
  if (angle_to_detent_center > snap_point_radians_decrease && (num_positions <= 0 || config.position > config.min_position))
  {
    current_detent_center += config.position_width_radians;
    angle_to_detent_center -= config.position_width_radians;
    config.position--;
  }
  else if (angle_to_detent_center < snap_point_radians_increase && (num_positions <= 0 || config.position < config.max_position))
  {
    current_detent_center -= config.position_width_radians;
    angle_to_detent_center += config.position_width_radians;
    config.position++;
  }
  motor_gui_position = config.position;

  float dead_zone_adjustment = CLAMP(
      angle_to_detent_center,
      fmaxf(-config.position_width_radians * DEAD_ZONE_DETENT_PERCENT, -DEAD_ZONE_RAD),
      fminf(config.position_width_radians * DEAD_ZONE_DETENT_PERCENT, DEAD_ZONE_RAD));

  bool out_of_bounds = num_positions > 0 && ((angle_to_detent_center > 0 && config.position == config.min_position) || (angle_to_detent_center < 0 && config.position == config.max_position));
  motor.PID_velocity.limit = 10; // out_of_bounds ? 10 : 3;
  motor.PID_velocity.P = out_of_bounds ? config.endstop_strength_unit * 4 : config.detent_strength_unit * 4;

  // Apply motor torque based on our angle to the nearest detent (detent strength, etc is handled by the PID_velocity parameters)
  if (fabsf(motor.shaft_velocity) > 60)
  {
    // Don't apply torque if velocity is too high (helps avoid positive feedback loop/runaway)
    motor.move(0);
  }
  else
  {
    float input = -angle_to_detent_center + dead_zone_adjustment;
    if (!out_of_bounds && config.detent_positions_count > 0)
    {
      bool in_detent = false;
      for (uint8_t i = 0; i < config.detent_positions_count; i++)
      {
        if (config.detent_positions[i] == config.position)
        {
          in_detent = true;
          break;
        }
      }
      if (!in_detent)
      {
        input = 0;
      }
    }

    float torque = motor.PID_velocity(input);
    motor.move(torque);
  }
  vTaskDelay(1);
}
/******************************************************************************/
void motor_task(void *pvParameters)
{
  SPI_2.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS); // SCLK, MISO, MOSI, SS
  // initialise magnetic sensor hardware
  sensor.init(&SPI_2);

  // link the motor to the sensor
  motor.linkSensor(&sensor);
  // driver config
  // power supply voltage [V]
  driver.voltage_power_supply = 4;
  driver.init();
  // link the motor and the driver
  motor.linkDriver(&driver);
  // choose FOC modulation (optional)
  // motor.foc_modulation = FOCModulationType::SpaceVectorPWM;
  // set motion control loop to be used
  motor.controller = MotionControlType::torque; // 必须选择力矩模式
  // Not actually using the velocity loop built into SimpleFOC; but I'm using those PID variables
  // to run PID for torque (and SimpleFOC studio supports updating them easily over serial for tuning)
  motor.PID_velocity.P = 1; // motor_task.cpp 第68行
  motor.PID_velocity.I = 0;
  motor.PID_velocity.D = 0.148;
  motor.PID_velocity.output_ramp = 5000;
  motor.PID_velocity.limit = 3;
  // maximal voltage to be set to the motor
  motor.voltage_limit = 3;          // 限制电压
  motor.voltage_sensor_align = 2.5; // 零点校准对齐电压，默认为3，大功率电机设置小比如0.5-1,小功率设置大比如2-3,
  motor.LPF_velocity.Tf = 0.01;     // to set it to 10ms
  // As above but for angle (does not override velocity Low Pass Filtering)
  motor.LPF_angle.Tf = 0.01;
  // velocity low pass filtering time constant
  // the lower the less filtered
  // motor.LPF_velocity.Tf = 0.0075; // Tf太小电机振动
  // angle P controller
  //  motor.P_angle.P = 20;
  // maximal velocity of the position control
  // motor.velocity_limit = 40;
  // use monitoring with serial
  // comment out if not needed
  // motor.useMonitoring(Serial);
  // initialize motor
  motor.init();
  // align sensor and start FOC
  motor.initFOC(); // 第一次编译使用这一句，上电后电机先零点校准，获得偏置角和方向，填入括号中，重新编译下载
  Serial.println(F("Motor ready."));
  Serial.println(F("Set the target angle using serial terminal:"));
  _delay(1000);
  knob_config(0);

  while (1)
  {
    static int count = 0;
    static double lastangle = 0;
    static int mode = 0;
    static int lastmode = 0;
    // static int last_stdby_instrument_state = 0;
    // display the angle and the angular velocity to the terminal
    static float currentangle_init, last_angle;
    static int instrument_init_flag = 0;
    // if ((last_stdby_instrument_state == 0) && (stdby_instrument_state == 1)) // 刚进入仪表旋转模式
    // {
    //   currentangle_init = sensor.getAngle();
    //   Serial.print("ok");
    // }
    // Serial.print(last_stdby_instrument_state);
    // Serial.print(stdby_instrument_state);
    if (stdby_instrument_state == 1)
    {
      if (instrument_init_flag == 0)
      {
        sensor.update();
        motor.loopFOC();
        motor.controller = MotionControlType::angle;
        motor.PID_velocity.P = 1;
        motor.PID_velocity.I = 1;
        motor.PID_velocity.D = 0;
        currentangle_init = sensor.getAngle();
        instrument_init_flag++;
        motor.move(currentangle_init);
        last_angle = currentangle_init;
      }

      // motor.PID_velocity.output_ramp = 100;
      sensor.update();
      motor.loopFOC();
      motor.controller = MotionControlType::angle;
      motor.PID_velocity.P = 0.5;
      motor.PID_velocity.I = 0;
      motor.PID_velocity.D = 0;
      // Serial.print("cpu_load_val=");
      // Serial.print(cpu_load_val);
      // Serial.print("   cpu_load_val * 0.02=");
      // Serial.print(cpu_load_val * 0.02);
      // Serial.print("   currentangle_init=");
      // Serial.print(currentangle_init);
      // Serial.print("   currentangle_init - cpu_load_val * 0.02=");
      // Serial.println(currentangle_init - cpu_load_val * 0.02);
      if (last_angle < (currentangle_init - cpu_load_val * 0.02))
      {
        last_angle += 0.01;
        motor.move(last_angle);
      }
      else if (last_angle > (currentangle_init - cpu_load_val * 0.02))
      {
        last_angle -= 0.01;
        motor.move(last_angle);
      }
      else
      {
        motor.move(last_angle);
      }

      vTaskDelay(1);
    }
    else
    {
      instrument_init_flag = 0;
      sensor.update();
      motor.loopFOC();
      motor.controller = MotionControlType::torque; // 必须选择力矩模式
      motor.PID_velocity.P = 1;                     // motor_task.cpp 第68行
      motor.PID_velocity.I = 0;
      motor.PID_velocity.D = 0.148;
      smart_knob();
    }

    if (my_hx711_motor == 1)
    {
      knob_haptic(5);
      my_hx711_motor = 0;
      //    Serial.println("knob_haptic5555555");
    }
    else if (my_hx711_motor == 2)
    {
      knob_haptic(1.5);
      //   Serial.println("knob_haptic1111111111");
      my_hx711_motor = 0;
      //   digitalWrite(45, LOW);
    }
    // last_stdby_instrument_state = stdby_instrument_state;
  }
}

void motor_init()
{
  xTaskCreatePinnedToCore(motor_task, "motor_task", 8 * 1024, NULL, 1, NULL, 0);
}