#include "my_gui.h"
#include "arduino.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "demos\lv_demos.h"
extern int32_t motor_gui_position;
extern int my_hx711_gui;

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf_1[screenWidth * screenHeight];
static lv_color_t buf_2[screenWidth * screenHeight];
// static lv_color_t buf_2[ screenWidth * screenHeight ];
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */
void lv_input_init(void);
void lv_menu_container_init(void);
void lv_con_container_init(void);
void event_cw_cb(lv_event_t);
void lv_led_contanier_init(void);
void lv_stdby_contanier_init(void);
void lv_stdby_instrument_contanier_init(void);
void lv_stdby_tkr_contanier_init(void);
void lv_dial_contanier_init(void);
void lv_au_contanier_init(void);
static lv_obj_t *menu_container = NULL;             // 定义容器1
static lv_obj_t *con_contanier = NULL;              // 定义容器2
static lv_obj_t *led_contanier = NULL;              // 定义容器3
static lv_obj_t *colorwheel_contanier = NULL;       // 定义容器4
static lv_obj_t *stdby_contanier = NULL;            // 定义容器5
static lv_obj_t *stdby_instrument_contanier = NULL; // 定义容器5
static lv_obj_t *stdby_astronaut_contanier = NULL;  // 定义容器5
static lv_obj_t *dial_contanier = NULL;             // 定义容器5
static lv_obj_t *stdby_instrument_arc_dram = NULL;
static lv_obj_t *stdby_instrument_arc_cpu = NULL;
static lv_obj_t *stdby_instrument_arc_gpu = NULL;
static lv_obj_t *au_contanier = NULL;
lv_obj_t *main_con, *main_led, *main_au, *main_scr, *main_dial;
lv_obj_t *label_cpu_val, *label_gpu, *label_dram;
lv_style_t cw_style_s, cw_style_v;
lv_obj_t *au_switch;
lv_group_t *default_g, *menu_g;
lv_color_hsv_t hsv_temp = {
    0x00,
    0xFF,
    0xBF,
};

lv_obj_t *cw_h, *cw_s, *cw_v;
u_int8_t cw_state = 0;
float cpu_load_val = 0;
int stdby_instrument_state = 0;
int stdby_astronaut_state = 0;
int dial_state = 0;
int au_state = 0;
static void button_event_cb(lv_event_t *event);
extern float lv_encode_read(void);
extern void knob_config(uint8_t);
LV_IMG_DECLARE(alpha_con_ble_pic);
LV_IMG_DECLARE(alpha_con_espnow_pic);
LV_IMG_DECLARE(alpha_con_usb_pic);
LV_IMG_DECLARE(alpha_con_wifi_pic);
LV_IMG_DECLARE(alpha_instrument_pic);
LV_IMG_DECLARE(alpha_led_breathing_pic);
LV_IMG_DECLARE(alpha_led_on_pic);
LV_IMG_DECLARE(alpha_led_rhythm_pic);
LV_IMG_DECLARE(alpha_menu_au_pic);
LV_IMG_DECLARE(alpha_menu_aucfg_pic);
LV_IMG_DECLARE(alpha_menu_con_pic);
LV_IMG_DECLARE(alpha_menu_led_pic);
LV_IMG_DECLARE(alpha_menu_standby_pic);
LV_IMG_DECLARE(alpha_menu_wificfg_pic);
LV_IMG_DECLARE(alpha_stdby_astronaut_pic);
LV_IMG_DECLARE(alpha_stdby_instrument_pic);
LV_IMG_DECLARE(alpha_menu_dial_pic128);
LV_IMG_DECLARE(alpha_menu_dial_pic_normal);
LV_IMG_DECLARE(tkr01);
LV_IMG_DECLARE(tkr02);
LV_IMG_DECLARE(tkr03);
LV_IMG_DECLARE(tkr04);
LV_IMG_DECLARE(tkr05);
LV_IMG_DECLARE(tkr06);
LV_IMG_DECLARE(tkr07);
LV_IMG_DECLARE(tkr08);
LV_IMG_DECLARE(tkr09);
LV_IMG_DECLARE(tkr10);
LV_IMG_DECLARE(tkr11);
LV_IMG_DECLARE(tkr12);
LV_IMG_DECLARE(tkr13);

static lv_indev_t *indev_encoder;
typedef struct _application_info_t
{
    char *name;

    char *img_src;

    // lv_obj_t* (*entry_point)(void* user_date);

    void (*release_resource_cb)(lv_event_t *event);

} application_info_t;

static void application_reg(application_info_t *application_info);

/* Display flushing */
void my_disp_flush(_lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

void my_gui_task(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler(); /* let the GUI do its work */
        vTaskDelay(5);
    }
}

void my_gui_init()
{
    lv_init();
    tft.begin();        /* TFT init */
    tft.setRotation(0); /* Landscape orientation, flipped */
    lv_disp_draw_buf_init(&draw_buf, buf_1, buf_2, screenWidth * screenHeight);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    lv_input_init();
    lv_menu_container_init();
    lv_con_container_init();
    lv_led_contanier_init();
    lv_stdby_contanier_init();
    lv_stdby_instrument_contanier_init();
    lv_stdby_tkr_contanier_init();
    lv_dial_contanier_init();
    lv_au_contanier_init();
    lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(dial_contanier, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(au_contanier, LV_OBJ_FLAG_HIDDEN);

    //  lv_demo_benchmark();          // OK
    /*
    xTaskCreatePinnedToCore() 函数有 7 个参数：
    1.实现任务的函数名（task1）
    2.任务的任何名称（“task1”等）
    3.以字为单位分配给任务的堆栈大小（1 个字=2 字节）
    4.任务输入参数（可以为NULL）
    5.任务的优先级（0为最低优先级）
    6.任务句柄（可以为 NULL）
    7.任务将运行的核心 ID（0 或 1）
    */
    xTaskCreatePinnedToCore(my_gui_task, "my_gui_task", 20480, NULL, 2, NULL, 1);
}

void my_knob_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) // 传递键值给LVGL
{
    static int32_t last_motor_gui_position = 0;
    static u_int8_t hsv_flag = 0, last_hsv_flag = 0;
    byte rec[5] = {0};
    if (cw_state == 1) // 色环控制模式
    {
        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            hsv_flag++;
        }
        if (hsv_flag == 0) // h
        {
            // lv_obj_remove_style(cw_s, &cw_style_s, LV_PART_KNOB);
            // lv_obj_remove_style(cw_v, &cw_style_v, LV_PART_KNOB);
            // lv_obj_add_style(cw_s, &cw_style, LV_PART_KNOB);
            if (motor_gui_position > 359)
            {
                motor_gui_position = 359;
            }
            else if (motor_gui_position < 0)
            {
                motor_gui_position = 0;
            }

            hsv_temp.h = 360 - motor_gui_position; // 0-359

            lv_colorwheel_set_hsv(cw_h, hsv_temp);
            lv_colorwheel_set_hsv(cw_s, hsv_temp);
            lv_colorwheel_set_hsv(cw_v, hsv_temp);
            // Serial.print("motor_gui_position");
            // Serial.print(motor_gui_position);
            // Serial.print("  hsv_temp.h");
            // Serial.println(hsv_temp.h);
        }
        else if (hsv_flag == 1) // s
        {
            if (last_hsv_flag != hsv_flag)
            {
                if (hsv_flag == 3)
                {
                    knob_config(0);
                }
                else
                {
                    knob_config(hsv_flag + 1);
                    lv_obj_remove_style(cw_h, NULL, LV_PART_KNOB);
                    lv_obj_del(cw_s);
                    cw_s = lv_colorwheel_create(colorwheel_contanier, true);
                    lv_obj_set_size(cw_s, 150, 150);
                    lv_obj_set_style_arc_width(cw_s, 10, LV_PART_MAIN);
                    lv_colorwheel_set_hsv(cw_s, hsv_temp);
                    //   lv_obj_add_event_cb(cw_s, event_cw_cb, LV_EVENT_VALUE_CHANGED, NULL);
                    lv_colorwheel_set_mode(cw_s, LV_COLORWHEEL_MODE_SATURATION); // LV_COLORWHEEL_MODE_HUE/SATURATION/VALUE
                    lv_obj_center(cw_s);
                    lv_colorwheel_set_mode_fixed(cw_s, true);
                }
            }
            if (motor_gui_position > 99)
            {
                motor_gui_position = 99;
            }
            else if (motor_gui_position < 0)
            {
                motor_gui_position = 0;
            }
            hsv_temp.s = 99 - motor_gui_position; // 0-99
            lv_colorwheel_set_hsv(cw_s, hsv_temp);
            lv_colorwheel_set_hsv(cw_v, hsv_temp);
            // Serial.print("motor_gui_position");
            // Serial.print(motor_gui_position);
            // Serial.print("  hsv_temp.s");
            // Serial.println(hsv_temp.s);
        }
        else if (hsv_flag == 2) // v
        {
            if (last_hsv_flag != hsv_flag)
            {
                if (hsv_flag == 3)
                {
                    knob_config(0);
                }
                else
                {
                    if (hsv_flag == 3)
                    {
                        knob_config(0);
                    }
                    else
                    {
                        knob_config(hsv_flag + 1);
                        lv_obj_remove_style(cw_s, NULL, LV_PART_KNOB);
                        lv_obj_del(cw_v);
                        cw_v = lv_colorwheel_create(colorwheel_contanier, true);
                        lv_obj_set_size(cw_v, 100, 100);
                        lv_obj_set_style_arc_width(cw_v, 10, LV_PART_MAIN);
                        lv_colorwheel_set_hsv(cw_v, hsv_temp);
                        //  lv_obj_add_event_cb(cw_v, event_cw_cb, LV_EVENT_VALUE_CHANGED, NULL);
                        lv_colorwheel_set_mode(cw_v, LV_COLORWHEEL_MODE_VALUE); // LV_COLORWHEEL_MODE_HUE/SATURATION/VALUE
                        lv_obj_center(cw_v);
                        lv_colorwheel_set_mode_fixed(cw_v, true);
                    }
                }
            }
            if (motor_gui_position > 99)
            {
                motor_gui_position = 99;
            }
            else if (motor_gui_position < 0)
            {
                motor_gui_position = 0;
            }
            hsv_temp.v = 99 - motor_gui_position; // 0-99
            lv_colorwheel_set_hsv(cw_v, hsv_temp);
            // Serial.print("motor_gui_position");
            // Serial.print(motor_gui_position);
            // Serial.print("  hsv_temp.v");
            // Serial.println(hsv_temp.v);
        }

        else
        {
            cw_state = 0;
            last_hsv_flag = 0;
            hsv_flag = 0;
            // exit
            knob_config(0);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
        }
        uint8_t send_h = hsv_temp.h / 1.4;
        uint8_t send_s = hsv_temp.s * 2.5;
        uint8_t send_v = hsv_temp.v * 2.5;
        Serial.write(0xaa);
        Serial.write(0x55);
        Serial.write(send_h);
        Serial.write(send_s);
        Serial.write(send_v);
        Serial.write(0x55);
        Serial.write(0xaa);
        // if (last_hsv_flag != hsv_flag)
        // {
        //     if (hsv_flag == 3)
        //     {
        //         knob_config(0);
        //     }
        //     else
        //     {
        //         knob_config(hsv_flag + 1);
        //     }
        // }
        last_hsv_flag = hsv_flag;
    }
    else if (stdby_instrument_state == 1)
    {

        if (Serial.available() > 0)
        {
            byte serialData[2] = {0};
            serialData[0] = Serial.read();
            serialData[1] = Serial.read();
            if (serialData[0] == 0xAA)
            {
                // serialData[1] = Serial.read();
                if (serialData[1] == 0x55)
                {
                    for (int i = 0; i < 5; i++)
                    {
                        rec[i] = Serial.read();
                    }
                }
            }
        }
        if ((rec[3] == 0x55) && (rec[4] == 0xaa))
        {
            // lv_arc_set_value(stdby_instrument_arc_cpu, map(rec[0], 0, 100, 20, 80));
            // lv_arc_set_value(stdby_instrument_arc_gpu, map(rec[1], 0, 100, 20, 80));
            // lv_arc_set_value(stdby_instrument_arc_dram, map(rec[2], 0, 100, 20, 80));
            static uint16_t get_cpu_val = 0, get_gpu_val = 0, get_dram_val = 0;
            // get_cpu_val = lv_arc_get_value(stdby_instrument_arc_cpu);
            // get_gpu_val = lv_arc_get_value(stdby_instrument_arc_gpu);
            // get_dram_val = lv_arc_get_value(stdby_instrument_arc_dram);
            if (get_cpu_val > (int)(rec[0] * 0.6 + 20))
            {
                get_cpu_val--;
                lv_arc_set_value(stdby_instrument_arc_cpu, get_cpu_val);
            }
            else if (get_cpu_val < (int)(rec[0] * 0.6 + 20))
            {
                get_cpu_val++;
                lv_arc_set_value(stdby_instrument_arc_cpu, get_cpu_val);
            }
            else
            {
            }
            if (get_gpu_val > (int)(rec[1] * 0.6 + 20))
            {
                get_gpu_val--;
                lv_arc_set_value(stdby_instrument_arc_gpu, get_gpu_val);
            }
            else if (get_gpu_val < (int)(rec[1] * 0.6 + 20))
            {
                get_gpu_val++;
                lv_arc_set_value(stdby_instrument_arc_gpu, get_gpu_val);
            }
            else
            {
            }
            if (get_dram_val > (int)(rec[2] * 0.6 + 20))
            {
                get_dram_val--;
                lv_arc_set_value(stdby_instrument_arc_dram, get_dram_val);
            }
            else if (get_dram_val < (int)(rec[2] * 0.6 + 20))
            {
                get_dram_val++;
                lv_arc_set_value(stdby_instrument_arc_dram, get_dram_val);
            }
            else
            {
            }
            cpu_load_val = rec[0];
            // lv_arc_set_value(stdby_instrument_arc_cpu, (int)(rec[0] * 0.6 + 20));
            // lv_arc_set_value(stdby_instrument_arc_gpu, (int)(rec[1] * 0.6 + 20));
            // lv_arc_set_value(stdby_instrument_arc_dram, (int)(rec[2] * 0.6 + 20));
            lv_label_set_text_fmt(label_cpu_val, "%d%%", rec[0]);
            lv_label_set_text_fmt(label_gpu, "GPU:%d%%", rec[1]);
            lv_label_set_text_fmt(label_dram, "RAM:%d%%", rec[2]);
            // Serial.print(rec[0]);
            // Serial.print(rec[1]);
            // Serial.println(rec[2]);
        }

        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            stdby_instrument_state = 0;
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (stdby_astronaut_state == 1)
    {
        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            stdby_astronaut_state = 0;
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (dial_state == 1)
    {
        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            dial_state = 0;
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(dial_contanier, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (au_state == 1)
    {
        if (last_motor_gui_position != motor_gui_position)
        {
            if (last_motor_gui_position < motor_gui_position)
            {
                // data->state = LV_INDEV_STATE_PR;
                // data->key = LV_KEY_NEXT;
                // Serial.print("LV_KEY_NEXT");
                lv_obj_add_state(au_switch, LV_STATE_CHECKED);
            }
            else
            {
                lv_obj_clear_state(au_switch, LV_STATE_CHECKED);
                // data->state = LV_INDEV_STATE_PR;
                // data->key = LV_KEY_PREV;
                // Serial.print("LV_KEY_PREV");
            }
        }
        else
        {
            // data->state = LV_INDEV_STATE_REL;
        }
        last_motor_gui_position = motor_gui_position;
        // Serial.print("config.position = ");
        // Serial.println(motor_gui_position);
        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            // data->key = LV_KEY_ENTER;
            // data->state = LV_INDEV_STATE_PRESSED;
            // Serial.println("LV_KEY_ENTER");
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(au_contanier, LV_OBJ_FLAG_HIDDEN);
            au_state = 0;
        }
        else if (my_hx711_gui == 2)
        {
            my_hx711_gui = 0;
            // data->key = LV_KEY_ENTER;
            // data->state = LV_INDEV_STATE_RELEASED;
        }
    }

    else // 普通模式
    {
        if (last_motor_gui_position != motor_gui_position)
        {
            if (last_motor_gui_position < motor_gui_position)
            {
                data->state = LV_INDEV_STATE_PR;
                data->key = LV_KEY_NEXT;
                //   Serial.print("LV_KEY_NEXT");
            }
            else
            {
                data->state = LV_INDEV_STATE_PR;
                data->key = LV_KEY_PREV;
                //   Serial.print("LV_KEY_PREV");
            }
        }
        else
        {
            data->state = LV_INDEV_STATE_REL;
        }
        last_motor_gui_position = motor_gui_position;
        // Serial.print("config.position = ");
        // Serial.println(motor_gui_position);
        if (my_hx711_gui == 1)
        {
            my_hx711_gui = 0;
            data->key = LV_KEY_ENTER;
            data->state = LV_INDEV_STATE_PRESSED;
            // Serial.println("LV_KEY_ENTER");
        }
        else if (my_hx711_gui == 2)
        {
            my_hx711_gui = 0;
            data->key = LV_KEY_ENTER;
            data->state = LV_INDEV_STATE_RELEASED;
        }
    }
}
void lv_input_init(void)
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD; // LV_INDEV_TYPE_ENCODER
    indev_drv.read_cb = my_knob_read;
    indev_encoder = lv_indev_drv_register(&indev_drv);

    //   menu_g = lv_group_create();                   // 创建组
    default_g = lv_group_create();                // 创建组
    lv_group_set_default(default_g);              // 设置组(group)为默认模式,后面的部件创建时会自动加入组(group)
                                                  //  lv_indev_set_group(indev_encoder, menu_g);    // 将输入设备(indev)和组(group)关联起来
    lv_indev_set_group(indev_encoder, default_g); // 将输入设备(indev)和组(group)关联起来
                                                  //   lv_group_set_wrap(menu_g,1);
}

/**
 * @brief 处理按钮事件的回调函数
 * @param event
 */
static void application_button_event_cb(lv_event_t *event)
{

    lv_obj_t *current_btn = lv_event_get_current_target(event);
    // Serial.println("LV_EVENT_FOCUSED");
    if (event->code == LV_EVENT_FOCUSED)
    {
        //  printf("LV_EVENT_FOCUSED");
        uint32_t current_btn_index = lv_obj_get_index(current_btn);
        uint32_t mid_btn_index = (lv_obj_get_child_cnt(menu_container) - 1) / 2;

        if (current_btn_index > mid_btn_index)
        {
            lv_obj_scroll_to_view(lv_obj_get_child(menu_container, mid_btn_index - 1), LV_ANIM_OFF);
            lv_obj_scroll_to_view(lv_obj_get_child(menu_container, mid_btn_index), LV_ANIM_ON);
            lv_obj_move_to_index(lv_obj_get_child(menu_container, 0), -1);
        }
        else if (current_btn_index < mid_btn_index)
        {
            lv_obj_scroll_to_view(lv_obj_get_child(menu_container, mid_btn_index + 1), LV_ANIM_OFF);
            lv_obj_scroll_to_view(lv_obj_get_child(menu_container, mid_btn_index), LV_ANIM_ON);
            lv_obj_move_to_index(lv_obj_get_child(menu_container, -1), 0);
        }

        for (uint8_t i = 0; i < 3; i++)
        {
            lv_obj_set_size(lv_obj_get_child(menu_container, mid_btn_index - i), 100 - i * 50, 100 - i * 50);
            lv_obj_set_size(lv_obj_get_child(menu_container, mid_btn_index + i), 100 - i * 50, 100 - i * 50);
            //   lv_obj_set_style_bg_opa(lv_obj_get_child(menu_container, mid_btn_index - i), 255 - 50 * i, LV_PART_MAIN);
            //    lv_obj_set_style_bg_opa(lv_obj_get_child(menu_container, mid_btn_index + i), 255 - 50 * i, LV_PART_MAIN);
        }
    }
    else if (event->code == LV_EVENT_SIZE_CHANGED)
    {
        //   Serial.println("LV_EVENT_SIZE_CHANGED");

        /*缩放图标*/
        lv_obj_t *img = lv_obj_get_child(current_btn, 0);

        if (lv_obj_is_valid(img))
        {
            lv_img_set_zoom(img, (uint16_t)(lv_obj_get_width(current_btn) * 0.9 / 128 * LV_IMG_ZOOM_NONE));
        }
    }

    else if (event->code == LV_EVENT_SCROLL)
    {
        LV_LOG_ERROR("H");
    }
    else if (event->code == LV_EVENT_PRESSED)
    {
        lv_obj_t *obj = lv_event_get_target(event);
        void *user_data = lv_event_get_user_data(event);
        if (user_data == (void *)1)
        {
            // 这是控件1的操作
            //     Serial.print("LV_EVENT_PRESSED1111111111");
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_group_remove_obj(main_dial);
            lv_group_remove_obj(main_scr);
            lv_group_remove_obj(main_au);
            lv_group_remove_obj(main_led);
            lv_group_remove_obj(main_con);
        }
        else if (user_data == (void *)2)
        {
            // 这是控件2的操作
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);

            //     Serial.print("LV_EVENT_PRESSED222222222222");
        }
        else if (user_data == (void *)4)
        {
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(au_contanier, LV_OBJ_FLAG_HIDDEN);
            au_state = 1;
        }
        else if (user_data == (void *)5)
        {
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
        }
        else if (user_data == (void *)6)
        {
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(dial_contanier, LV_OBJ_FLAG_HIDDEN);
            dial_state = 1;
        }
        else if ((user_data == (void *)11) || (user_data == (void *)12) || (user_data == (void *)13) || (user_data == (void *)14))
        {
            lv_obj_clear_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_group_add_obj(default_g, main_dial);
            lv_group_add_obj(default_g, main_scr);
            lv_group_add_obj(default_g, main_au);
            lv_group_add_obj(default_g, main_led);
            lv_group_add_obj(default_g, main_con);
        }
        else if (user_data == (void *)21)
        {
            // 这是控件2的操作
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);

            cw_state = 1;
            knob_config(1);
            //       Serial.print("LV_EVENT_PRESSED222222222222");
        }
        else if (user_data == (void *)31)
        {
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            stdby_astronaut_state = 1;
        }
        else if (user_data == (void *)32)
        {
            // 这是控件2的操作
            lv_obj_add_flag(menu_container, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(led_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(colorwheel_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(con_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(stdby_instrument_contanier, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(stdby_astronaut_contanier, LV_OBJ_FLAG_HIDDEN);
            stdby_instrument_state = 1;
        }
    }
}
void lv_menu_container_init()
{
    // contanier1容器里装载滚动图标
    menu_container = lv_obj_create(lv_scr_act());
    // lv_obj_set_size(container, LV_PCT(240), LV_PCT(240));
    lv_obj_set_size(menu_container, 240, 240);
    lv_obj_set_scrollbar_mode(menu_container, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(menu_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(menu_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_snap_x(menu_container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_style_bg_opa(menu_container, LV_OPA_100, LV_PART_MAIN); // LV_OPA_100
    lv_obj_set_style_bg_color(menu_container, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(menu_container, 10, LV_PART_MAIN); // 图标之间的间隙
    lv_obj_center(menu_container);

    // 创建主菜单中的con图片按钮
    main_con = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(main_con, 128, 128);
    lv_obj_add_event_cb(main_con, application_button_event_cb, LV_EVENT_FOCUSED, (void *)1);      // 添加事件回调函数
    lv_obj_add_event_cb(main_con, application_button_event_cb, LV_EVENT_PRESSED, (void *)1);      // 添加事件回调函数
    lv_obj_add_event_cb(main_con, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)1); // 添加事件回调函数
    lv_obj_remove_style_all(main_con);
    lv_obj_set_style_bg_opa(main_con, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    lv_obj_t *mian_con_btn_img = lv_img_create(main_con);      // 在按钮上创建一个图片
    lv_img_set_src(mian_con_btn_img, &alpha_menu_con_pic);     // 设置图片源
    lv_obj_center(mian_con_btn_img);                           // 图片居中

    // 创建主菜单中的led图片按钮
    main_led = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(main_led, 128, 128);
    lv_obj_add_event_cb(main_led, application_button_event_cb, LV_EVENT_FOCUSED, (void *)2);      // 添加事件回调函数
    lv_obj_add_event_cb(main_led, application_button_event_cb, LV_EVENT_PRESSED, (void *)2);      // 添加事件回调函数
    lv_obj_add_event_cb(main_led, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)2); // 添加事件回调函数
    lv_obj_remove_style_all(main_led);
    lv_obj_set_style_bg_opa(main_led, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    lv_obj_t *main_led_btn_img = lv_img_create(main_led);      // 在按钮上创建一个图片
    lv_img_set_src(main_led_btn_img, &alpha_menu_led_pic);     // 设置图片源
    lv_obj_center(main_led_btn_img);                           // 图片居中

    // 创建主菜单中的wifi图片按钮
    // lv_obj_t *main_wifi = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    // lv_obj_set_size(main_wifi, 128, 128);
    // lv_obj_add_event_cb(main_wifi, application_button_event_cb, LV_EVENT_FOCUSED, (void *)3);      // 添加事件回调函数
    // lv_obj_add_event_cb(main_wifi, application_button_event_cb, LV_EVENT_PRESSED, (void *)3);      // 添加事件回调函数
    // lv_obj_add_event_cb(main_wifi, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)3); // 添加事件回调函数
    // lv_obj_remove_style_all(main_wifi);
    // lv_obj_set_style_bg_opa(main_wifi, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    // lv_obj_t *main_wifi_btn_img = lv_img_create(main_wifi);     // 在按钮上创建一个图片
    // lv_img_set_src(main_wifi_btn_img, &alpha_menu_wificfg_pic); // 设置图片源
    // lv_obj_center(main_wifi_btn_img);                           // 图片居中
    // 创建主菜单中的au图片按钮
    main_au = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(main_au, 128, 128);
    lv_obj_add_event_cb(main_au, application_button_event_cb, LV_EVENT_FOCUSED, (void *)4);      // 添加事件回调函数
    lv_obj_add_event_cb(main_au, application_button_event_cb, LV_EVENT_PRESSED, (void *)4);      // 添加事件回调函数
    lv_obj_add_event_cb(main_au, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)4); // 添加事件回调函数
    lv_obj_remove_style_all(main_au);
    lv_obj_set_style_bg_opa(main_au, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    lv_obj_t *main_au_btn_img = lv_img_create(main_au);       // 在按钮上创建一个图片
    lv_img_set_src(main_au_btn_img, &alpha_menu_aucfg_pic);   // 设置图片源
    lv_obj_center(main_au_btn_img);                           // 图片居中

    // 创建主菜单中的scr图片按钮
    main_scr = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(main_scr, 128, 128);
    lv_obj_add_event_cb(main_scr, application_button_event_cb, LV_EVENT_FOCUSED, (void *)5);      // 添加事件回调函数
    lv_obj_add_event_cb(main_scr, application_button_event_cb, LV_EVENT_PRESSED, (void *)5);      // 添加事件回调函数
    lv_obj_add_event_cb(main_scr, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)5); // 添加事件回调函数
    lv_obj_remove_style_all(main_scr);
    lv_obj_set_style_bg_opa(main_scr, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    lv_obj_t *main_scr_btn_img = lv_img_create(main_scr);      // 在按钮上创建一个图片
    lv_img_set_src(main_scr_btn_img, &alpha_menu_standby_pic); // 设置图片源
    lv_obj_center(main_scr_btn_img);                           // 图片居中

    // 创建主菜单中的surface dial图片按钮
    main_dial = lv_btn_create(menu_container); // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(main_dial, 128, 128);
    lv_obj_add_event_cb(main_dial, application_button_event_cb, LV_EVENT_FOCUSED, (void *)6);      // 添加事件回调函数
    lv_obj_add_event_cb(main_dial, application_button_event_cb, LV_EVENT_PRESSED, (void *)6);      // 添加事件回调函数
    lv_obj_add_event_cb(main_dial, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)6); // 添加事件回调函数
    lv_obj_remove_style_all(main_dial);
    lv_obj_set_style_bg_opa(main_dial, LV_OPA_0, LV_PART_MAIN); // 设置背景色透明
    lv_obj_t *main_dial_btn_img = lv_img_create(main_dial);     // 在按钮上创建一个图片
    lv_img_set_src(main_dial_btn_img, &alpha_menu_dial_pic128); // 设置图片源
    lv_obj_center(main_dial_btn_img);                           // 图片居中

    // uint32_t mid_btn_index = (lv_obj_get_child_cnt(menu_container) - 1) / 2;
    // printf("%d\n",mid_btn_index);
    // for (uint32_t i = 0; i < mid_btn_index; i++)
    // {
    //     lv_obj_move_to_index(lv_obj_get_child(menu_container, -1), 0);
    // }
    // /*当按钮数为偶数时，确保按钮居中*/
    // lv_obj_scroll_to_view(lv_obj_get_child(menu_container, mid_btn_index), LV_ANIM_OFF);
}
void lv_con_container_init(void)
{
    con_contanier = lv_obj_create(lv_scr_act());
    lv_obj_set_size(con_contanier, 240, 240);
    lv_obj_set_style_radius(con_contanier, 0, 100);
    lv_obj_set_style_bg_opa(con_contanier, LV_OPA_100, LV_PART_MAIN); // LV_OPA_100
    lv_obj_set_style_bg_color(con_contanier, lv_color_black(), LV_PART_MAIN);
    // con_contanier容器里装载一个usb按钮
    lv_obj_t *con_usb_btn = lv_btn_create(con_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(con_usb_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(con_usb_btn, LV_ALIGN_TOP_MID, 0, 0);                                                // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(con_usb_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)11);      // 添加事件回调函数
    lv_obj_add_event_cb(con_usb_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)11);      // 添加事件回调函数
    lv_obj_add_event_cb(con_usb_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)11); // 添加事件回调函数
    lv_obj_set_style_bg_opa(con_usb_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *usb_btn_img = lv_img_create(con_usb_btn);                                               // 在按钮上创建一个图片
    lv_img_set_src(usb_btn_img, &alpha_con_usb_pic);                                                  // 设置图片源
    lv_obj_center(usb_btn_img);                                                                       // 图片居中

    // con_contanier容器里装载一个蓝牙按钮
    lv_obj_t *con_ble_btn = lv_btn_create(con_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(con_ble_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(con_ble_btn, LV_ALIGN_RIGHT_MID, 0, 0);                                              // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(con_ble_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)12);      // 添加事件回调函数
    lv_obj_add_event_cb(con_ble_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)12);      // 添加事件回调函数
    lv_obj_add_event_cb(con_ble_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)12); // 添加事件回调函数
    lv_obj_set_style_bg_opa(con_ble_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *ble_btn_img = lv_img_create(con_ble_btn);                                               // 在按钮上创建一个图片
    lv_img_set_src(ble_btn_img, &alpha_con_ble_pic);                                                  // 设置图片源
    lv_obj_center(ble_btn_img);                                                                       // 图片居中

    // con_contanier容器里装载一个wifi按钮
    lv_obj_t *con_wifi_btn = lv_btn_create(con_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(con_wifi_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(con_wifi_btn, LV_ALIGN_BOTTOM_MID, 0, 0);                                             // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(con_wifi_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)13);      // 添加事件回调函数
    lv_obj_add_event_cb(con_wifi_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)13);      // 添加事件回调函数
    lv_obj_add_event_cb(con_wifi_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)13); // 添加事件回调函数
    lv_obj_set_style_bg_opa(con_wifi_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *wifi_btn_img = lv_img_create(con_wifi_btn);                                              // 在按钮上创建一个图片
    lv_img_set_src(wifi_btn_img, &alpha_con_wifi_pic);                                                 // 设置图片源
    lv_obj_center(wifi_btn_img);                                                                       // 图片居中

    // con_contanier容器里装载一个2.4G按钮
    lv_obj_t *con_res_btn = lv_btn_create(con_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(con_res_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(con_res_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(con_res_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)14);      // 添加事件回调函数
    lv_obj_add_event_cb(con_res_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)14);      // 添加事件回调函数
    lv_obj_add_event_cb(con_res_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)14); // 添加事件回调函数
    lv_obj_set_style_bg_opa(con_res_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *res_btn_img = lv_img_create(con_res_btn);                                               // 在按钮上创建一个图片
    lv_img_set_src(res_btn_img, &alpha_con_espnow_pic);                                               // 设置图片源
    lv_obj_center(res_btn_img);                                                                       // 图片居中
}
void lv_led_contanier_init(void)
{
    led_contanier = lv_obj_create(lv_scr_act());
    lv_obj_set_size(led_contanier, 240, 240);
    lv_obj_set_style_radius(led_contanier, 0, 100);
    // lv_obj_set_size(led_contanier, LV_PCT(240), LV_PCT(240));
    lv_obj_set_scrollbar_mode(led_contanier, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(led_contanier, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(led_contanier, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_snap_x(led_contanier, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_style_bg_opa(led_contanier, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(led_contanier, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(led_contanier, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(led_contanier, 10, LV_PART_MAIN); // 图标之间的间隙
    lv_obj_center(led_contanier);

    // 生成演示按钮
    lv_obj_t *led_on_btn = lv_btn_create(led_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(led_on_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(led_on_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(led_on_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)21);      // 添加事件回调函数
    lv_obj_add_event_cb(led_on_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)21);      // 添加事件回调函数
    lv_obj_add_event_cb(led_on_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)21); // 添加事件回调函数
    lv_obj_set_style_bg_opa(led_on_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *led_on_img = lv_img_create(led_on_btn);                                                // 在按钮上创建一个图片
    lv_img_set_src(led_on_img, &alpha_led_on_pic);                                                   // 设置图片源
    lv_obj_center(led_on_img);                                                                       // 图片居中

    lv_obj_t *led_breathing_btn = lv_btn_create(led_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(led_breathing_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(led_breathing_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(led_breathing_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)22);      // 添加事件回调函数
    lv_obj_add_event_cb(led_breathing_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)22);      // 添加事件回调函数
    lv_obj_add_event_cb(led_breathing_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)22); // 添加事件回调函数
    lv_obj_set_style_bg_opa(led_breathing_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *led_breathing_img = lv_img_create(led_breathing_btn);                                         // 在按钮上创建一个图片
    lv_img_set_src(led_breathing_img, &alpha_led_breathing_pic);                                            // 设置图片源
    lv_obj_center(led_breathing_img);                                                                       // 图片居中

    lv_obj_t *led_rhythm_btn = lv_btn_create(led_contanier);                                             // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(led_rhythm_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(led_rhythm_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(led_rhythm_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)23);      // 添加事件回调函数
    lv_obj_add_event_cb(led_rhythm_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)23);      // 添加事件回调函数
    lv_obj_add_event_cb(led_rhythm_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)23); // 添加事件回调函数
    lv_obj_set_style_bg_opa(led_rhythm_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *led_rhythm_img = lv_img_create(led_rhythm_btn);                                            // 在按钮上创建一个图片
    lv_img_set_src(led_rhythm_img, &alpha_led_rhythm_pic);                                               // 设置图片源
    lv_obj_center(led_rhythm_img);                                                                       // 图片居中

    uint32_t mid_btn_index = (lv_obj_get_child_cnt(led_contanier) - 1) / 2;
    printf("%d\n", mid_btn_index);
    for (uint32_t i = 0; i < mid_btn_index; i++)
    {
        lv_obj_move_to_index(lv_obj_get_child(led_contanier, -1), 0);
    }
    lv_obj_scroll_to_view(lv_obj_get_child(led_contanier, mid_btn_index), LV_ANIM_OFF);

    colorwheel_contanier = lv_obj_create(lv_scr_act());
    lv_obj_set_size(colorwheel_contanier, 240, 240);

    cw_h = lv_colorwheel_create(colorwheel_contanier, true);
    lv_obj_set_size(cw_h, 200, 200);
    lv_obj_set_style_arc_width(cw_h, 10, LV_PART_MAIN);
    lv_colorwheel_set_hsv(cw_h, hsv_temp);
    // lv_obj_add_event_cb(cw_h, event_cw_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_colorwheel_set_mode(cw_h, LV_COLORWHEEL_MODE_HUE); // LV_COLORWHEEL_MODE_HUE/SATURATION/VALUE
    lv_obj_center(cw_h);
    lv_colorwheel_set_mode_fixed(cw_h, true);
    lv_obj_set_style_bg_opa(cw_h, LV_OPA_0, LV_PART_MAIN);

    //  lv_style_init(&cw_style_s);
    cw_s = lv_colorwheel_create(colorwheel_contanier, true);
    lv_obj_set_size(cw_s, 150, 150);
    lv_obj_set_style_arc_width(cw_s, 10, LV_PART_MAIN);
    lv_colorwheel_set_hsv(cw_s, hsv_temp);
    //   lv_obj_add_event_cb(cw_s, event_cw_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_colorwheel_set_mode(cw_s, LV_COLORWHEEL_MODE_SATURATION); // LV_COLORWHEEL_MODE_HUE/SATURATION/VALUE
    lv_obj_center(cw_s);
    lv_colorwheel_set_mode_fixed(cw_s, true);
    // lv_obj_remove_style(cw_s, &cw_style_s, LV_PART_KNOB);
    lv_obj_remove_style(cw_s, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(cw_s, LV_OPA_0, LV_PART_MAIN);

    // lv_style_init(&cw_style_v);
    cw_v = lv_colorwheel_create(colorwheel_contanier, true);
    lv_obj_set_size(cw_v, 100, 100);
    lv_obj_set_style_arc_width(cw_v, 10, LV_PART_MAIN);
    lv_colorwheel_set_hsv(cw_v, hsv_temp);
    //  lv_obj_add_event_cb(cw_v, event_cw_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_colorwheel_set_mode(cw_v, LV_COLORWHEEL_MODE_VALUE); // LV_COLORWHEEL_MODE_HUE/SATURATION/VALUE
    lv_obj_center(cw_v);
    lv_colorwheel_set_mode_fixed(cw_v, true);
    lv_obj_remove_style(cw_v, NULL, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(cw_v, LV_OPA_0, LV_PART_MAIN);

    //    lv_obj_remove_style(cw_v, NULL, LV_PART_KNOB);
}
void event_cw_cb(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    //  lv_color_t i = lv_colorwheel_get_rgb(target);
    //   Serial.println(i.);
}

void lv_stdby_contanier_init(void)
{
    lv_obj_t *obj;
    stdby_contanier = lv_obj_create(lv_scr_act());
    lv_obj_set_size(stdby_contanier, 240, 240);
    lv_obj_set_style_radius(stdby_contanier, 0, 100);
    // lv_obj_set_style_bg_opa(stdby_contanier, LV_OPA_100, LV_PART_MAIN); // LV_OPA_100
    // lv_obj_set_style_bg_color(stdby_contanier, lv_color_black(), LV_PART_MAIN);

    lv_obj_set_size(stdby_contanier, LV_PCT(240), LV_PCT(240));
    lv_obj_set_scrollbar_mode(stdby_contanier, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(stdby_contanier, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(stdby_contanier, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_scroll_snap_x(stdby_contanier, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_style_bg_opa(stdby_contanier, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_color(stdby_contanier, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(stdby_contanier, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(stdby_contanier, 20, LV_PART_MAIN); // 图标之间的间隙
    lv_obj_center(stdby_contanier);

    // 生成演示按钮
    lv_obj_t *stdby_astronaut_btn = lv_btn_create(stdby_contanier);                                           // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(stdby_astronaut_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(stdby_astronaut_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(stdby_astronaut_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)31);      // 添加事件回调函数
    lv_obj_add_event_cb(stdby_astronaut_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)31);      // 添加事件回调函数
    lv_obj_add_event_cb(stdby_astronaut_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)31); // 添加事件回调函数
    lv_obj_set_style_bg_opa(stdby_astronaut_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *stdby_astronaut_img = lv_img_create(stdby_astronaut_btn);                                       // 在按钮上创建一个图片
    lv_img_set_src(stdby_astronaut_img, &alpha_stdby_astronaut_pic);                                          // 设置图片源
    lv_obj_center(stdby_astronaut_img);                                                                       // 图片居中

    lv_obj_t *stdby_instrument_btn = lv_btn_create(stdby_contanier);                                           // 在con_contanier画布上创建一个按钮
    lv_obj_set_size(stdby_instrument_btn, 70, 70);                                                             // 设置按钮尺寸
    lv_obj_align(stdby_instrument_btn, LV_ALIGN_LEFT_MID, 0, 0);                                               // 设置按钮顶部居中对齐
    lv_obj_add_event_cb(stdby_instrument_btn, application_button_event_cb, LV_EVENT_FOCUSED, (void *)32);      // 添加事件回调函数
    lv_obj_add_event_cb(stdby_instrument_btn, application_button_event_cb, LV_EVENT_PRESSED, (void *)32);      // 添加事件回调函数
    lv_obj_add_event_cb(stdby_instrument_btn, application_button_event_cb, LV_EVENT_SIZE_CHANGED, (void *)32); // 添加事件回调函数
    lv_obj_set_style_bg_opa(stdby_instrument_btn, LV_OPA_0, LV_PART_MAIN);                                     // 设置背景色透明
    lv_obj_t *stdby_instrument_img = lv_img_create(stdby_instrument_btn);                                      // 在按钮上创建一个图片
    lv_img_set_src(stdby_instrument_img, &alpha_stdby_instrument_pic);                                         // 设置图片源
    lv_obj_center(stdby_instrument_img);                                                                       // 图片居中
}

void lv_stdby_instrument_contanier_init(void)
{
    // stdby_instrument_contanier = lv_obj_create(lv_scr_act());
    // lv_obj_set_size(stdby_instrument_contanier, 240, 240);
    // lv_obj_set_style_radius(stdby_instrument_contanier, 0, 100);

    static lv_style_t style;        // 创建样式
    lv_style_init(&style);          // 初始化样式
    lv_style_set_radius(&style, 5); // 设置圆角半径

    //  LV_IMG_DECLARE(animimg001);                                           // 加载图片声明 .c文件的图片
    lv_style_set_bg_img_src(&style, &alpha_instrument_pic); // 设置背景图片
    lv_style_set_bg_img_opa(&style, LV_OPA_100);            // 设置背景图片透明度
    // lv_style_set_bg_img_recolor(&style, lv_palette_main(LV_PALETTE_RED)); // 设置背景图片重着色
    // lv_style_set_bg_img_recolor_opa(&style, LV_OPA_80);                   // 设置背景图片重着色透明度
    // lv_style_set_bg_img_tiled(&style, false);                             // 设置背景图片平铺

    /*Create an object with the new style*/
    stdby_instrument_contanier = lv_obj_create(lv_scr_act()); // 创建对象
    lv_obj_add_style(stdby_instrument_contanier, &style, 0);  // 设置对象的样式
    lv_obj_set_size(stdby_instrument_contanier, 240, 240);    // 设置对象的尺寸
    lv_obj_center(stdby_instrument_contanier);                // 居中对象
    lv_obj_set_style_bg_opa(stdby_instrument_contanier, LV_OPA_0, LV_PART_MAIN);

    stdby_instrument_arc_cpu = lv_arc_create(stdby_instrument_contanier);
    lv_obj_align(stdby_instrument_arc_cpu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(stdby_instrument_arc_cpu, 185, 185); // 设置对象的尺寸
    lv_obj_set_style_bg_opa(stdby_instrument_arc_cpu, LV_OPA_0, LV_PART_MAIN);
    lv_obj_remove_style(stdby_instrument_arc_cpu, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(stdby_instrument_arc_cpu, 0, LV_PART_MAIN);       // 设置背景弧宽度
    lv_obj_set_style_arc_width(stdby_instrument_arc_cpu, 12, LV_PART_INDICATOR); // 设置前景弧宽度
    lv_obj_set_style_arc_color(stdby_instrument_arc_cpu, lv_color_hex(0xff8C00), LV_PART_INDICATOR);
    lv_arc_set_change_rate(stdby_instrument_arc_cpu, 90);
    lv_arc_set_range(stdby_instrument_arc_cpu, 20, 80);
    lv_arc_set_value(stdby_instrument_arc_cpu, 60);

    stdby_instrument_arc_gpu = lv_arc_create(stdby_instrument_contanier);
    lv_obj_align(stdby_instrument_arc_gpu, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(stdby_instrument_arc_gpu, 163, 163); // 设置对象的尺寸
    lv_obj_set_style_bg_opa(stdby_instrument_arc_gpu, LV_OPA_0, LV_PART_MAIN);
    lv_obj_remove_style(stdby_instrument_arc_gpu, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(stdby_instrument_arc_gpu, 0, LV_PART_MAIN);       // 设置背景弧宽度
    lv_obj_set_style_arc_width(stdby_instrument_arc_gpu, 12, LV_PART_INDICATOR); // 设置前景弧宽度
    lv_obj_set_style_arc_color(stdby_instrument_arc_gpu, lv_color_hex(0x00BFFF), LV_PART_INDICATOR);
    lv_arc_set_change_rate(stdby_instrument_arc_gpu, 90);
    lv_arc_set_range(stdby_instrument_arc_gpu, 20, 80);
    lv_arc_set_value(stdby_instrument_arc_gpu, 60);

    stdby_instrument_arc_dram = lv_arc_create(stdby_instrument_contanier);
    lv_obj_align(stdby_instrument_arc_dram, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(stdby_instrument_arc_dram, 141, 141); // 设置对象的尺寸
    lv_obj_set_style_bg_opa(stdby_instrument_arc_dram, LV_OPA_0, LV_PART_MAIN);
    lv_obj_remove_style(stdby_instrument_arc_dram, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(stdby_instrument_arc_dram, 0, LV_PART_MAIN);       // 设置背景弧宽度
    lv_obj_set_style_arc_width(stdby_instrument_arc_dram, 12, LV_PART_INDICATOR); // 设置前景弧宽度
    lv_obj_set_style_arc_color(stdby_instrument_arc_dram, lv_color_hex(0x9400D3), LV_PART_INDICATOR);
    lv_arc_set_change_rate(stdby_instrument_arc_dram, 90);
    lv_arc_set_range(stdby_instrument_arc_dram, 20, 80);
    lv_arc_set_value(stdby_instrument_arc_dram, 60);

    label_cpu_val = lv_label_create(stdby_instrument_contanier);
    lv_obj_set_width(label_cpu_val, LV_SIZE_CONTENT);  // 设置宽度
    lv_obj_set_height(label_cpu_val, LV_SIZE_CONTENT); // 设置高度
    lv_label_set_text_fmt(label_cpu_val, " %d%%", 0);
    lv_obj_align(label_cpu_val, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(label_cpu_val, LV_TEXT_ALIGN_CENTER, 0); // 设置对象字体样式居中
    lv_obj_set_style_text_font(label_cpu_val, &lv_font_montserrat_34, LV_STATE_DEFAULT);

    lv_obj_t *label_cpu = lv_label_create(stdby_instrument_contanier);
    lv_label_set_text_fmt(label_cpu, "CPU");
    lv_obj_align(label_cpu, LV_ALIGN_CENTER, 0, 25);
    lv_obj_set_style_text_align(label_cpu, LV_TEXT_ALIGN_CENTER, 0); // 设置对象字体样式居中
    lv_obj_set_width(label_cpu, LV_SIZE_CONTENT);                    // 设置宽度
    lv_obj_set_height(label_cpu, LV_SIZE_CONTENT);                   // 设置高度
    label_gpu = lv_label_create(stdby_instrument_contanier);
    lv_label_set_text_fmt(label_gpu, "GPU:%d%%", 0);
    lv_obj_align(label_gpu, LV_ALIGN_CENTER, 0, 55);
    lv_obj_set_style_text_align(label_gpu, LV_TEXT_ALIGN_CENTER, 0); // 设置对象字体样式居中
    lv_obj_set_width(label_gpu, LV_SIZE_CONTENT);                    // 设置宽度
    lv_obj_set_height(label_gpu, LV_SIZE_CONTENT);                   // 设置高度
    label_dram = lv_label_create(stdby_instrument_contanier);
    lv_label_set_text_fmt(label_dram, "RAM:%d%%", 0);
    lv_obj_align(label_dram, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_text_align(label_dram, LV_TEXT_ALIGN_CENTER, 0); // 设置对象字体样式居中
    lv_obj_set_width(label_dram, LV_SIZE_CONTENT);                    // 设置宽度
    lv_obj_set_height(label_dram, LV_SIZE_CONTENT);                   // 设置高度

    //  lv_obj_set_size(label_cpu, lv_pct(50), lv_pct(50));
    // lv_obj_set_style_bg_color(stdby_instrument_arc, lv_color_hex(0xFF0000), LV_PART_MAIN);

    // lv_obj_set_style_bg_opa(stdby_instrument_arc, LV_OPA_100, LV_PART_INDICATOR);
    // lv_obj_set_style_bg_color(stdby_instrument_arc, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    // lv_obj_set_style_pad_column(stdby_instrument_arc, 40, LV_PART_INDICATOR);
    // lv_obj_set_style_pad_row(stdby_instrument_arc, 40, LV_PART_INDICATOR);
}
void lv_stdby_tkr_contanier_init(void)
{
    static const void *astronauts_anim_imgs[13] = {
        &tkr01,
        &tkr02,
        &tkr03,
        &tkr04,
        &tkr05,
        &tkr06,
        &tkr07,
        &tkr08,
        &tkr09,
        &tkr10,
        &tkr11,
        &tkr12,
        &tkr13,
    };
    stdby_astronaut_contanier = lv_animimg_create(lv_scr_act());                     // 创建动画对象
    lv_obj_center(stdby_astronaut_contanier);                                        // 将对象置于屏幕中央
    lv_animimg_set_src(stdby_astronaut_contanier, astronauts_anim_imgs, 13);         // lv_animimg_set_src(animimg1, (lv_img_dsc_t **)astronauts_anim_imgs, 3); // 加载动画资源
    lv_animimg_set_duration(stdby_astronaut_contanier, 1000);                        // 创建动画时间
    lv_animimg_set_repeat_count(stdby_astronaut_contanier, LV_ANIM_REPEAT_INFINITE); // 设置一直重复时间
    lv_animimg_start(stdby_astronaut_contanier);
}
void lv_dial_contanier_init(void)
{
    static lv_style_t style;        // 创建样式
    lv_style_init(&style);          // 初始化样式
    lv_style_set_radius(&style, 5); // 设置圆角半径

    //  LV_IMG_DECLARE(animimg001);                                           // 加载图片声明 .c文件的图片
    lv_style_set_bg_img_src(&style, &alpha_menu_dial_pic_normal); // 设置背景图片
    lv_style_set_bg_img_opa(&style, LV_OPA_100);                  // 设置背景图片透明度

    // lv_style_set_bg_img_recolor(&style, lv_palette_main(LV_PALETTE_RED)); // 设置背景图片重着色
    // lv_style_set_bg_img_recolor_opa(&style, LV_OPA_80);                   // 设置背景图片重着色透明度
    // lv_style_set_bg_img_tiled(&style, false);                             // 设置背景图片平铺
    /*Create an object with the new style*/
    dial_contanier = lv_obj_create(lv_scr_act()); // 创建对象
    lv_obj_add_style(dial_contanier, &style, 0);  // 设置对象的样式
    lv_obj_set_size(dial_contanier, 240, 240);    // 设置对象的尺寸
    lv_obj_center(dial_contanier);                // 居中对象
    lv_obj_set_style_bg_color(dial_contanier, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dial_contanier, LV_OPA_0, LV_PART_MAIN);
}
void lv_au_contanier_init(void)
{
    au_contanier = lv_obj_create(lv_scr_act());
    lv_obj_set_size(au_contanier, 240, 240);
    lv_obj_set_style_radius(au_contanier, 0, 100);
    lv_obj_set_style_bg_opa(au_contanier, LV_OPA_100, LV_PART_MAIN); // LV_OPA_100
    lv_obj_set_style_bg_color(au_contanier, lv_color_black(), LV_PART_MAIN);

    au_switch = lv_switch_create(au_contanier);
    lv_obj_set_style_bg_opa(au_switch, LV_OPA_100, LV_PART_MAIN); // LV_OPA_100
    lv_obj_set_align(au_switch, LV_ALIGN_CENTER);
    lv_obj_set_size(au_switch, 100, 50);
}