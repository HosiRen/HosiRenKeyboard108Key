#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_mac.h"
#include "espnow.h"
#include "espnow_utils.h"
#include "espnow_ctrl.h"
#include "iot_button.h"
#include <wifi_provisioning/manager.h>
#include "responder.h"
#include "app_tusb_hid.h"
#include "app_udp_server.h"
#include "settings.h"

static const char *TAG = "app";

#define SSID     "douzhi1"
#define PASSWORD "douzhi123"

#define LED_STRIP_GPIO     GPIO_NUM_48
static led_strip_handle_t g_strip_handle = NULL;

#define WIFI_PROV_KEY_GPIO GPIO_NUM_0
button_handle_t button_handle;

typedef enum
{
    APP_WIFI_PROV_INIT,
    APP_WIFI_PROV_START,
    APP_WIFI_PROV_SUCCESS,
    APP_WIFI_PROV_MAX
} app_wifi_prov_status_t;
static app_wifi_prov_status_t s_wifi_prov_status = APP_WIFI_PROV_INIT;

static uint32_t wifi_disconnected_color = 0;

void app_led_set_color(uint32_t color)
{
    uint8_t red = (color >> 16) & 0xff;
    uint8_t green = (color >> 8) & 0xff;
    uint8_t blue = color & 0xff;
    led_strip_set_pixel(g_strip_handle, 0, red, green, blue);
    led_strip_refresh(g_strip_handle);
}

static void app_led_init(void)
{
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &g_strip_handle));
    /* Set all LED off to clear all pixels */
    led_strip_clear(g_strip_handle);
}

/// @brief 十六进制字符串转数组
/// @param pbDest 数组
/// @param pbSrc 字符串
/// @param nLen 
int hexString_to_hexArray(char *pbDest, char *pbSrc, int nLen)
{
	char h1, h2;
	char s1, s2;
	int  i;

	uint8_t len = nLen / 2;
	for (i = 0; i < len; i++)
	{
		h1 = pbSrc[2 * i];
		h2 = pbSrc[2 * i + 1];

		s1 = toupper(h1) - 0x30;
		if (s1 > 9)
			s1 -= 7;
		s2 = toupper(h2) - 0x30;
		if (s2 > 9)
			s2 -= 7;

		pbDest[i] = s1 * 16 + s2;
	}

    return len;
}

/// @brief WiFi连接事件处理函数
/// @param arg 
/// @param event_base 
/// @param event_id 
/// @param event_data 
static void app_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Wi-Fi disconnected");
        s_wifi_prov_status = APP_WIFI_PROV_INIT;
        app_led_set_color(wifi_disconnected_color);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        s_wifi_prov_status = APP_WIFI_PROV_SUCCESS;
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Wi-Fi connected");
        app_led_set_color((wifi_disconnected_color * 10));
    }
}

static char *bind_error_to_string(espnow_ctrl_bind_error_t bind_error)
{
    switch (bind_error)
    {
    case ESPNOW_BIND_ERROR_NONE:
    {
        return "No error";
        break;
    }

    case ESPNOW_BIND_ERROR_TIMEOUT:
    {
        return "bind timeout";
        break;
    }

    case ESPNOW_BIND_ERROR_RSSI:
    {
        return "bind packet RSSI below expected threshold";
        break;
    }

    case ESPNOW_BIND_ERROR_LIST_FULL:
    {
        return "bindlist is full";
        break;
    }

    default:
    {
        return "unknown error";
        break;
    }
    }
}

static void app_espnow_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != ESP_EVENT_ESPNOW)
    {
        return;
    }

    printf("app_espnow_event_handler: %lx\r\n", id);

    switch (id)
    {
    case ESP_EVENT_ESPNOW_CTRL_BIND:
    {
        // 绑定成功
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(TAG, "bind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        break;
    }

    case ESP_EVENT_ESPNOW_CTRL_UNBIND:
    {
        // 解绑成功
        espnow_ctrl_bind_info_t *info = (espnow_ctrl_bind_info_t *)event_data;
        ESP_LOGI(TAG, "unbind, uuid: " MACSTR ", initiator_type: %d", MAC2STR(info->mac), info->initiator_attribute);
        break;
    }

    case ESP_EVENT_ESPNOW_CTRL_BIND_ERROR:
    {
        // 绑定失败
        espnow_ctrl_bind_error_t *bind_error = (espnow_ctrl_bind_error_t *)event_data;
        ESP_LOGI(TAG, "bind error: %s", bind_error_to_string(*bind_error));
        break;
    }

    default:
        break;
    }
}

/// @brief 接收数据
/// @param src_addr 
/// @param data 
/// @param rx_ctrl 
static void espnow_ctrl_recv_data_cb(espnow_addr_t src_addr, espnow_ctrl_data_t *data, wifi_pkt_rx_ctrl_t *rx_ctrl)
{
    espnow_ctrl_data_t *recv_data = (espnow_ctrl_data_t *)data;

    printf("espnow_ctrl_recv_data_cb: %d\r\n", recv_data->initiator_attribute);
    printf("espnow_ctrl_recv_data_cb: %d\r\n", recv_data->responder_attribute);
    printf("espnow_ctrl_recv_data_cb: %d\r\n", recv_data->responder_value_s_flag);
    printf("espnow_ctrl_recv_data_cb: %d\r\n", recv_data->responder_value_s_size);
    printf("espnow_ctrl_recv_data_cb: %s\r\n", recv_data->responder_value_s);// 字符串以'\0'结尾
    printf("\r\n");

    char hex_array[8] = {0};
    if (((recv_data->responder_value_s_size - 1) / 2) < (sizeof(hex_array) / sizeof(hex_array[0])))
        return;
    hexString_to_hexArray(hex_array, recv_data->responder_value_s, (sizeof(hex_array) / sizeof(hex_array[0])) * 2);
    app_tusb_hid_send_key((uint8_t *)hex_array, 8);
}

static void app_control_responder_init(void)
{
    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ESPNOW_CTRL_BIND,       app_espnow_event_handler, NULL);
    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ESPNOW_CTRL_UNBIND,     app_espnow_event_handler, NULL);
    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ESPNOW_CTRL_BIND_ERROR, app_espnow_event_handler, NULL);
    // 设置绑定超时时间为30s, 接收信号强度为-55dBm
    ESP_ERROR_CHECK(espnow_ctrl_responder_bind(30 * 1000, -55, NULL));
    ESP_ERROR_CHECK(espnow_ctrl_recv(espnow_ctrl_recv_data_cb));
}

/// @brief 双击进入配网模式: 通过ESP-NOW扫描配网信息
/// @param arg 
/// @param usr_data 
static void app_wifi_prov_start_press_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_DOUBLE_CLICK == iot_button_get_event(arg)));
    if (s_wifi_prov_status == APP_WIFI_PROV_INIT)
    {
        ESP_LOGI(TAG, "Start WiFi provisioning over ESP-NOW on responder");
        // 启动配网模式
        // 创建一个task
        // 在task中扫描配网信息
        // 收到配网信息后, 返回确认信息
        // 连接到AP并保存配置
        // 删除task
        // 备注: 这个任务没有超时退出的机制, 直到收到配网信息才退出
        app_espnow_prov_responder_start();
        s_wifi_prov_status = APP_WIFI_PROV_START;
    }
    else if (s_wifi_prov_status == APP_WIFI_PROV_START)
    {
        ESP_LOGI(TAG, "WiFi provisioning is started");
    }
    else
    {
        ESP_LOGI(TAG, "WiFi is already provisioned");
    }
}

/// @brief 重置配网信息并重启
/// @param arg 
/// @param usr_data 
static void app_wifi_prov_reset_press_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_LONG_PRESS_START == iot_button_get_event(arg)));
    ESP_LOGI(TAG, "Reset WiFi provisioning information and restart");
    sys_param_t *sys_param = settings_get_parameter();
    sys_param->mode_hid = MODE_HID_ESPNOW;
    settings_write_parameter_to_nvs();
    espnow_ctrl_responder_clear_bindlist();
    wifi_prov_mgr_reset_provisioning();
    esp_wifi_disconnect();
    esp_restart();
}

/// @brief 配网按钮初始化
/// @param  
static void app_wifi_prov_button_init(void)
{
    iot_button_register_cb(button_handle, BUTTON_DOUBLE_CLICK,     app_wifi_prov_start_press_cb, NULL);
    iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_START, app_wifi_prov_reset_press_cb, NULL);
}

static void app_wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    app_wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, app_wifi_event_handler, NULL);
}

static void app_wifi_connect(void)
{
    wifi_config_t wifi_config = {0};
    esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Get wifi config failed, %d", ret);
        return;
    }

    if (strlen((const char *)wifi_config.sta.ssid) == 0)
    {
        ESP_LOGW(TAG, "WiFi not configured");
        memcpy(wifi_config.sta.ssid, (char *)SSID, strlen((char *)SSID));
        memcpy(wifi_config.sta.password, (char *)PASSWORD, strlen((char *)PASSWORD));
    }

    ESP_LOGI(TAG, "WiFi SSID: %s", wifi_config.sta.ssid);
    ESP_LOGI(TAG, "WiFi Password: %s", wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    settings_read_parameter_from_nvs();
    sys_param_t *sys_param = settings_get_parameter();

    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config = {
            .gpio_num = WIFI_PROV_KEY_GPIO,
            .active_level = 0,
        },
    };
    button_handle = iot_button_create(&button_config);
    
    app_led_init();
    int led_blink_count = 0;
    while (1)
    {
        uint8_t hid_mode_key_state = gpio_get_level(WIFI_PROV_KEY_GPIO);
        if (!hid_mode_key_state)
        {
            sys_param->mode_hid = (sys_param->mode_hid + 1) % MODE_HID_MAX;
            settings_write_parameter_to_nvs();
            break;
        }
        app_led_set_color(0x050505);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        app_led_set_color(0x000000);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_blink_count++;
        if (led_blink_count >= 20)
            break;
    }

    app_wifi_init();

    if (sys_param->mode_hid == MODE_HID_ESPNOW)
    {
        ESP_LOGI(TAG, "++++++++++HID mode: ESPNOW");
        wifi_disconnected_color = 0x000300;
        espnow_config_t espnow_config = ESPNOW_INIT_CONFIG_DEFAULT();
        espnow_config.qsize = CONFIG_APP_ESPNOW_QUEUE_SIZE;
        espnow_config.sec_enable = 1;
        espnow_init(&espnow_config);
        app_wifi_prov_button_init();
        app_control_responder_init();
        app_espnow_responder();
    }
    else
    {
        ESP_LOGI(TAG, "++++++++++HID mode: UDP");
        wifi_disconnected_color = 0x000003;
        app_udp_server_start();
    }
    app_led_set_color(wifi_disconnected_color);
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    app_tusb_hid_init();
    app_wifi_connect();
}
