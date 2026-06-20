#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

namespace HAL {

class WiFiSTA {
public:
    /**
     * Connect to AP in station mode. Blocks until connected or timeout.
     * Handles NVS, netif, event loop init internally.
     * Returns true on success.
     */
    bool begin(const char* ssid, const char* password, uint32_t timeout_ms = 12000);

    bool isConnected() const { return _connected; }

private:
    static void _event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

    static EventGroupHandle_t _event_group;
    static int  _retry_count;
    static bool _connected;
};

} // namespace HAL
