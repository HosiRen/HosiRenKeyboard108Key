#include "arduino.h"
#include "my_gui.h"
#include "motor.h"
#include "my_hx711.h"

void setup()
{
  pinMode(45, OUTPUT);
  digitalWrite(45, LOW);


  Serial.begin(115200);

  motor_init();
  my_gui_init();
  hx711_init();
  delay(50);
}

void loop()
{
  vTaskDelay(5);
}
