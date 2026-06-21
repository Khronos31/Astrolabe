#include "hal_http.hpp"
#include <esp_http_client.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cstdlib>

namespace HAL
{
    static const char* HTTP_TAG = "hal_http";

    bool http_get(const char* url, uint8_t** out_buf, size_t* out_len, size_t max_len)
    {
        *out_buf = nullptr;
        *out_len = 0;

        esp_http_client_config_t cfg = {};
        cfg.url = url;
        cfg.timeout_ms = 8000;

        esp_http_client_handle_t client = esp_http_client_init(&cfg);
        if (!client) return false;

        esp_err_t err = esp_http_client_open(client, 0);
        if (err != ESP_OK) {
            ESP_LOGE(HTTP_TAG, "open failed: %s", esp_err_to_name(err));
            esp_http_client_cleanup(client);
            return false;
        }

        int content_len = esp_http_client_fetch_headers(client);
        int status = esp_http_client_get_status_code(client);
        if (status != 200) {
            ESP_LOGE(HTTP_TAG, "http status %d", status);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return false;
        }

        size_t cap = (content_len > 0 && (size_t)content_len <= max_len)
                     ? (size_t)content_len : max_len;

        uint8_t* buf = (uint8_t*)heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
        if (!buf) buf = (uint8_t*)malloc(cap);
        if (!buf) {
            ESP_LOGE(HTTP_TAG, "alloc %d failed", (int)cap);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return false;
        }

        size_t total = 0;
        while (total < cap) {
            int r = esp_http_client_read(client, (char*)buf + total, cap - total);
            if (r < 0) break;
            if (r == 0) break;
            total += r;
        }

        esp_http_client_close(client);
        esp_http_client_cleanup(client);

        if (total == 0) {
            free(buf);
            return false;
        }

        *out_buf = buf;
        *out_len = total;
        ESP_LOGI(HTTP_TAG, "got %d bytes", (int)total);
        return true;
    }
}
