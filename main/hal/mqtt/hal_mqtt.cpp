#include "hal_mqtt.hpp"
#include "esp_log.h"
#include <algorithm>
#include <cstring>

static const char* TAG = "MQTTClient";
#define MSG_QUEUE_DEPTH  16

namespace HAL {

void MQTTClient::_event_handler(void* arg, esp_event_base_t /*event_base*/,
                                int32_t event_id, void* event_data)
{
    MQTTClient* self = static_cast<MQTTClient*>(arg);
    esp_mqtt_event_handle_t ev = static_cast<esp_mqtt_event_handle_t>(event_data);

    switch ((esp_mqtt_event_id_t)event_id) {

        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "connected");
            self->_connected = true;
            for (auto& s : self->_subs) {
                esp_mqtt_client_subscribe(self->_client, s.topic.c_str(), 0);
                ESP_LOGI(TAG, "subscribed: %s", s.topic.c_str());
            }
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "disconnected (auto-reconnect by esp-mqtt)");
            self->_connected = false;
            break;

        case MQTT_EVENT_DATA: {
            if (!self->_msg_queue || ev->data_len <= 0) break;
            MQTTMessage_t msg = {};
            msg.topic_len   = std::min(ev->topic_len,  (int)sizeof(msg.topic)   - 1);
            msg.payload_len = std::min(ev->data_len,   (int)sizeof(msg.payload) - 1);
            memcpy(msg.topic,   ev->topic, msg.topic_len);
            memcpy(msg.payload, ev->data,  msg.payload_len);
            msg.topic[msg.topic_len]     = '\0';
            msg.payload[msg.payload_len] = '\0';
            /* This is called from the MQTT client task, not an ISR. */
            if (xQueueSend(self->_msg_queue, &msg, 0) != pdTRUE) {
                ESP_LOGW(TAG, "msg queue full, dropping: %s", msg.topic);
            }
            break;
        }

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            break;
    }
}

bool MQTTClient::begin(const char* uri, const char* username, const char* password)
{
    _msg_queue = xQueueCreate(MSG_QUEUE_DEPTH, sizeof(MQTTMessage_t));
    if (!_msg_queue) {
        ESP_LOGE(TAG, "queue create failed");
        return false;
    }

    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = uri;
    if (username && username[0]) cfg.credentials.username = username;
    if (password && password[0]) cfg.credentials.authentication.password = password;

    _client = esp_mqtt_client_init(&cfg);
    if (!_client) {
        ESP_LOGE(TAG, "client init failed");
        return false;
    }

    ESP_ERROR_CHECK(esp_mqtt_client_register_event(
        _client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, _event_handler, this));
    ESP_ERROR_CHECK(esp_mqtt_client_start(_client));

    ESP_LOGI(TAG, "started → %s", uri);
    return true;
}

bool MQTTClient::publish(const char* topic, const char* payload, int qos, bool retain)
{
    if (!_client || !_connected) return false;
    int id = esp_mqtt_client_publish(_client, topic, payload, 0, qos, retain ? 1 : 0);
    return id != -1;
}

void MQTTClient::subscribe(const char* topic, MQTTSubCallback_t callback)
{
    _subs.push_back({topic, callback});
    if (_connected && _client) {
        esp_mqtt_client_subscribe(_client, topic, 0);
        ESP_LOGI(TAG, "subscribed: %s", topic);
    }
}

void MQTTClient::poll()
{
    if (!_msg_queue) return;
    MQTTMessage_t msg;
    while (xQueueReceive(_msg_queue, &msg, 0) == pdTRUE) {
        for (auto& s : _subs) {
            if (s.topic == msg.topic) {
                s.callback(msg.topic, msg.payload, msg.payload_len);
            }
        }
    }
}

} // namespace HAL
