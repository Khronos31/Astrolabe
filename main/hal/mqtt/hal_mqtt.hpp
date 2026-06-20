#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_client.h"
#include <functional>
#include <string>
#include <vector>

namespace HAL {

struct MQTTMessage_t {
    char topic[128];
    char payload[512];
    int  topic_len;
    int  payload_len;
};

using MQTTSubCallback_t = std::function<void(const char* topic, const char* payload, int payload_len)>;

class MQTTClient {
public:
    /**
     * Connect to broker and start MQTT client task.
     * Returns true immediately after starting (connection is async).
     */
    bool begin(const char* uri, const char* username = nullptr, const char* password = nullptr);

    bool isConnected() const { return _connected; }

    /** Publish to topic. Returns false if not connected or on error. */
    bool publish(const char* topic, const char* payload, int qos = 0, bool retain = false);

    /**
     * Register a subscription. If already connected, subscribes immediately.
     * Callback is invoked from the main task via poll().
     */
    void subscribe(const char* topic, MQTTSubCallback_t callback);

    /**
     * Drain the received-message queue and invoke callbacks.
     * Call this once per main loop iteration.
     */
    void poll();

private:
    static void _event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data);

    esp_mqtt_client_handle_t _client    = nullptr;
    volatile bool            _connected = false;
    QueueHandle_t            _msg_queue = nullptr;

    struct Sub_t {
        std::string       topic;
        MQTTSubCallback_t callback;
    };
    std::vector<Sub_t> _subs;
};

} // namespace HAL
