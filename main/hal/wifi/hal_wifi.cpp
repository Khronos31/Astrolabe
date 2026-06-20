#include "hal_wifi.hpp"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/ip4_addr.h"
#include <cstring>

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1
#define WIFI_MAX_RETRY      5

static const char* TAG = "WiFiSTA";

namespace HAL {

EventGroupHandle_t WiFiSTA::_event_group = nullptr;
int  WiFiSTA::_retry_count = 0;
bool WiFiSTA::_connected   = false;

void WiFiSTA::_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        _connected = false;
        if (_retry_count < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            _retry_count++;
            ESP_LOGW(TAG, "reconnect attempt %d/%d", _retry_count, WIFI_MAX_RETRY);
        } else if (_event_group) {
            xEventGroupSetBits(_event_group, WIFI_FAIL_BIT);
        }

    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* e = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&e->ip_info.ip));
        _retry_count = 0;
        _connected   = true;
        if (_event_group) {
            xEventGroupSetBits(_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

bool WiFiSTA::begin(const char* ssid, const char* password, uint32_t timeout_ms)
{
    /* NVS — required by WiFi driver */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    _event_group = xEventGroupCreate();
    _retry_count = 0;

    esp_event_handler_instance_t h_wifi, h_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &_event_handler, nullptr, &h_wifi));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &_event_handler, nullptr, &h_ip));

    wifi_config_t wifi_cfg = {};
    strncpy((char*)wifi_cfg.sta.ssid,     ssid,     sizeof(wifi_cfg.sta.ssid));
    strncpy((char*)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "connecting to %s ...", ssid);

    EventBits_t bits = xEventGroupWaitBits(
        _event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(timeout_ms)
    );

    bool ok = (bits & WIFI_CONNECTED_BIT) != 0;

    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, h_wifi);
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, h_ip);
    vEventGroupDelete(_event_group);
    _event_group = nullptr;

    if (ok) {
        /* Re-register disconnect handler for auto-reconnect after begin() returns */
        esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                   &_event_handler, nullptr);
        esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP,
                                   &_event_handler, nullptr);
        ESP_LOGI(TAG, "connected");
    } else {
        ESP_LOGE(TAG, "failed to connect to %s", ssid);
    }
    return ok;
}

} // namespace HAL
