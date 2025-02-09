#include "my_hx711.h"
#include "HX711.h"

HX711 scale;
int my_hx711_motor = 0;
int my_hx711_gui = 0;
void my_hx711_task(void *param)
{
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    while (1)
    {

        static int press_flag = 0;
        long reading = 0;

        if (scale.is_ready())
        {
            reading = scale.read();
            Serial.print("HX711 press: ");
            Serial.println(reading);
            if (reading > STRAIN_MAX)
            {
                if (press_flag == 0)
                {
                    // 触发press，flag=1,发送队列
                    press_flag = 1;
                    my_hx711_motor = 1;
                    my_hx711_gui = 1;
                    Serial.print("HX711 press: ");
                    Serial.println(reading);
                }
            }
            if (reading < (STRAIN_MIN + (STRAIN_MAX - STRAIN_MIN) * 0.3))
            {
                if (press_flag == 1)
                {
                    // 触发repress，flag=0,发送队列
                    press_flag = 0;
                    my_hx711_motor = 2;
                    my_hx711_gui = 2;
                    Serial.print("HX711 repress: ");
                    Serial.println(reading);
                }
            }
        }
        else
        {
            Serial.println("HX711 not found.");
        }
        vTaskDelay(50);
    }
}
void hx711_init(void)
{
    xTaskCreatePinnedToCore(my_hx711_task, "my_hx711_task", 6 * 1024, NULL, 1, NULL, 1);
}