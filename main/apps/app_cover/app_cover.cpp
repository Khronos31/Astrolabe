#include "app_cover.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

#define TOPIC_SET   "astrolabe/cover/study/set"
#define TOPIC_STATE "astrolabe/cover/study/state"
#define PUBLISH_INTERVAL_MS 200
#define POS_STEP 5

using namespace MOONCAKE::USER_APP;
using namespace MOONCAKE::USER_APP::UI;


void AppCover::onSetup()
{
    setAllowBgRunning(false);
    COVER::Data_t d; _data = d;
    _bind_hal();
}


void AppCover::onCreate()
{
    _log("onCreate");

    _mqtt_subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            if (millis() - _data.last_local_change_ms < 2000) return;

            const char* p = strstr(payload, "\"position\":");
            if (p) {
                sscanf(p + 11, "%d", &_data.position);
                _data.position = std::max(0, std::min(100, _data.position));
                _data.target_position = _data.position;
            }

            const char* s = strstr(payload, "\"state\":\"");
            if (s) {
                s += 9;
                int i = 0;
                while (*s && *s != '"' && i < (int)sizeof(_data.state_str) - 1)
                    _data.state_str[i++] = *s++;
                _data.state_str[i] = '\0';
            }

            _data.state_received = true;
            _log("rcv pos=%d state=%s", _data.position, _data.state_str);
            _render();
        });

    _render();
}


void AppCover::_handle_encoder()
{
    if (!hal->encoder.wasMoved(true)) return;

    int dir = hal->encoder.getDirection();
    _data.target_position += (dir < 1) ? POS_STEP : -POS_STEP;
    _data.target_position = std::max(0, std::min(100, _data.target_position));

    _data.last_local_change_ms = millis();
    _mark_activity();
    _data.publish_pending = true;
    _render();
}


void AppCover::_handle_touch()
{
    /* Long press: stop. Short press: toggle open/close. */
    Press p = _handle_center_touch(LONGPRESS_MS, [&] {
        hal->buzz.tone(2000, 50);
        _mark_activity();
        _publish_action("stop");
    });

    if (p != Press::SHORT) return;

    hal->buzz.tone(4000, 20);
    _data.last_local_change_ms = millis();
    _mark_activity();

    /* 50%より開いていれば閉じる、それ以外なら開く */
    if (_data.position >= 50) {
        _data.target_position = 0;
        _publish_action("close");
    } else {
        _data.target_position = 100;
        _publish_action("open");
    }
    _render();
}


void AppCover::onRunning()
{
    hal->mqtt.poll();

    _handle_encoder();

    if (_data.publish_pending &&
        millis() - _data.last_publish_ms >= PUBLISH_INTERVAL_MS)
    {
        _publish_position();
        _data.publish_pending = false;
    }

    if (_back_button_pressed()) {
        destroyApp();
        return;
    }

    _handle_touch();
}


void AppCover::_publish_position()
{
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"position\":%d}", _data.target_position);
    hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppCover::_publish_action(const char* action)
{
    char buf[40];
    snprintf(buf, sizeof(buf), "{\"action\":\"%s\"}", action);
    hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppCover::_render()
{
    auto* canvas = hal->canvas;
    canvas->fillScreen((uint32_t)0x040810);

    /* Position ring */
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP, (uint32_t)0x0A1828);
    if (_data.target_position > 0) {
        float angle = ARC_START + (_data.target_position / 100.0f) * ARC_SWEEP;
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, (uint32_t)0x2A7EC0);
    }

    /* Position % (中央) */
    char pos_str[8];
    snprintf(pos_str, sizeof(pos_str), "%d%%", _data.target_position);
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x70B0F0);
    canvas->drawCenterString(pos_str, CX, CY - 14);

    /* State text */
    if (_data.state_received) {
        const char* label = _data.state_str;
        if      (strcmp(label, "open")    == 0) label = "OPEN";
        else if (strcmp(label, "closed")  == 0) label = "CLOSED";
        else if (strcmp(label, "opening") == 0) label = "OPENING...";
        else if (strcmp(label, "closing") == 0) label = "CLOSING...";
        else if (strcmp(label, "stopped") == 0) label = "STOPPED";

        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.2f);
        canvas->setTextColor((uint32_t)0x4080C0);
        canvas->drawCenterString(label, CX, CY + 20);
    } else {
        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x1E3050);
        canvas->drawCenterString("connecting...", CX, CY + 20);
    }

    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x1A2840);
    canvas->drawCenterString("hold: stop  btn: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppCover::onDestroy()
{
    _clear_mqtt_subscriptions();
    _log("onDestroy");
}
