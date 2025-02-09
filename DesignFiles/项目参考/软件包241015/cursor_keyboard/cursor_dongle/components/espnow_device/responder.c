/* Solution Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"

#include "sdkconfig.h"

#include "esp_mac.h"

#include "espnow.h"
#include "espnow_storage.h"
#include "espnow_utils.h"

#ifdef CONFIG_APP_ESPNOW_PROVISION
#include "espnow_prov.h"
#endif

#ifdef CONFIG_APP_ESPNOW_SECURITY
#include "espnow_security.h"
#include "espnow_security_handshake.h"
#endif

static const char *TAG = "espnow_responder";

static void app_espnow_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != ESP_EVENT_ESPNOW)
    {
        return;
    }

    switch (id)
    {
    default:
        break;
    }
}

/*
 * Connect to route by debug command or espnow provisioning
 */
#if defined(CONFIG_APP_ESPNOW_PROVISION)

#define EXAMPLE_ESP_MAXIMUM_RETRY 5

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void app_wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

#endif

#ifdef CONFIG_APP_ESPNOW_PROVISION

static TaskHandle_t s_prov_task;

/// @brief 收到配网信息回调函数
/// @param src_addr 
/// @param data 
/// @param size 
/// @param rx_ctrl 
/// @return 
static esp_err_t app_espnow_prov_initiator_recv_cb(uint8_t *src_addr, void *data,
                                                   size_t size, wifi_pkt_rx_ctrl_t *rx_ctrl)
{
    ESP_PARAM_CHECK(src_addr);
    ESP_PARAM_CHECK(data);
    ESP_PARAM_CHECK(size);
    ESP_PARAM_CHECK(rx_ctrl);

    espnow_prov_wifi_t *wifi_config = (espnow_prov_wifi_t *)data;

    ESP_LOGI(TAG, "MAC: " MACSTR ", Channel: %d, RSSI: %d, wifi_mode: %d, ssid: %s, password: %s, token: %s",
             MAC2STR(src_addr), rx_ctrl->channel, rx_ctrl->rssi,
             wifi_config->mode, wifi_config->sta.ssid, wifi_config->sta.password, wifi_config->token);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, (wifi_config_t *)&wifi_config->sta));
    ESP_ERROR_CHECK(esp_wifi_connect());
    return ESP_OK;
}

/// @brief ESPNOW配网任务
/// @param arg 
static void app_espnow_prov_initiator_init(void *arg)
{
    esp_err_t ret = ESP_OK;
    wifi_pkt_rx_ctrl_t rx_ctrl = {0};
    espnow_prov_initiator_t initiator_info = {
        .product_id = "initiator_test",
    };
    espnow_addr_t responder_addr = {0};
    espnow_prov_responder_t responder_info = {0};

    for (;;)
    {
#ifdef CONFIG_APP_ESPNOW_SECURITY
        uint8_t key_info[APP_KEY_LEN];
        /* Security key is not ready */
        if (espnow_get_key(key_info) != ESP_OK)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "waiting for security key ready");
            continue;
        }
#endif
        // 扫描配网信息
        ret = espnow_prov_initiator_scan(responder_addr, &responder_info, &rx_ctrl, pdMS_TO_TICKS(3 * 1000));
        ESP_ERROR_CONTINUE(ret != ESP_OK, "");
        // 输出接收到的配网信息
        ESP_LOGI(TAG, "MAC: " MACSTR ", Channel: %d, RSSI: %d, Product_id: %s, Device Name: %s",
                 MAC2STR(responder_addr), rx_ctrl.channel, rx_ctrl.rssi,
                 responder_info.product_id, responder_info.device_name);
        // 发送本机信息
        // 在 app_espnow_prov_initiator_recv_cb 中联网
        ret = espnow_prov_initiator_send(responder_addr, &initiator_info, app_espnow_prov_initiator_recv_cb, pdMS_TO_TICKS(3 * 1000));
        ESP_ERROR_CONTINUE(ret != ESP_OK, "<%s> espnow_prov_responder_add", esp_err_to_name(ret));

        break;
    }

    ESP_LOGI(TAG, "provisioning initiator exit");
    vTaskDelete(NULL);
    s_prov_task = NULL;
}

esp_err_t app_espnow_prov_responder_start(void)
{
    if (!s_prov_task)
    {
        xTaskCreate(app_espnow_prov_initiator_init, "PROV_init", 3072, NULL, tskIDLE_PRIORITY + 1, &s_prov_task);
    }
    return ESP_OK;
}

#endif

void app_espnow_responder_register()
{
    esp_event_handler_register(ESP_EVENT_ESPNOW, ESP_EVENT_ANY_ID, app_espnow_event_handler, NULL);
#if defined(CONFIG_APP_ESPNOW_PROVISION)
    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,    app_wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP, app_wifi_event_handler, NULL);
#endif
}

void app_espnow_responder()
{
#ifdef CONFIG_APP_ESPNOW_SECURITY
    const char *pop_data = CONFIG_APP_ESPNOW_SESSION_POP;
    uint8_t key_info[APP_KEY_LEN];

    /* If espnow_set_key succeed, sending and receiving will be in security mode */
    if (espnow_get_key(key_info) == ESP_OK)
    {
        espnow_set_key(key_info);
        espnow_set_dec_key(key_info);
    }

    /* If responder handshake with initiator succeed, espnow_set_key will be executed again. */
    espnow_sec_responder_start(pop_data);
#endif
}