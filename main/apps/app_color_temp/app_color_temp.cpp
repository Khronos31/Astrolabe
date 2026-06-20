#include "app_color_temp.h"
#include "../common_define.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

#define TOPIC_SET   "astrolabe/light/study/set"
#define TOPIC_STATE "astrolabe/light/study/state"
#define PUBLISH_INTERVAL_MS  120
#define COLOR_TEMP_STEP  200
#define COLOR_TEMP_MIN   2700
#define COLOR_TEMP_MAX   6500

#define CX  120
#define CY  120
#define ARC_START  135.0f
#define ARC_SWEEP  270.0f
#define R_OUT       98
#define R_IN        78

using namespace MOONCAKE::USER_APP;


void AppColorTemp::onSetup()
{
    setAllowBgRunning(false);
    COLOR_TEMP::Data_t d; _data = d;
    _data.hal = (HAL::HAL*)getUserData();
}


void AppColorTemp::onCreate()
{
    _log("onCreate");

    _data.hal->mqtt.subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            if (millis() - _data.last_local_change_ms < 2000) return;
            const char* ct = strstr(payload, "\"color_temp_kelvin\":");
            if (ct) sscanf(ct + 20, "%d", &_data.color_temp);
            _data.color_temp = std::max(COLOR_TEMP_MIN, std::min(COLOR_TEMP_MAX, _data.color_temp));
            _data.state_received = true;
            _log("rcv color_temp=%dK", _data.color_temp);
            _render();
        });

    _render();
}


void AppColorTemp::onRunning()
{
    _data.hal->mqtt.poll();

    /* Encoder */
    if (_data.hal->encoder.wasMoved(true))
    {
        int dir = _data.hal->encoder.getDirection();
        _data.color_temp += (dir < 1) ? COLOR_TEMP_STEP : -COLOR_TEMP_STEP;
        if (_data.color_temp > COLOR_TEMP_MAX) _data.color_temp = COLOR_TEMP_MAX;
        if (_data.color_temp < COLOR_TEMP_MIN) _data.color_temp = COLOR_TEMP_MIN;

        _data.last_local_change_ms = millis();
        _data.publish_pending = true;
        _render();
    }

    /* Rate-limited publish while turning */
    if (_data.publish_pending &&
        millis() - _data.last_publish_ms >= PUBLISH_INTERVAL_MS)
    {
        _publish();
        _data.publish_pending = false;
    }

    /* Button short press → back to launcher */
    if (!_data.hal->encoder.btn.read())
    {
        while (!_data.hal->encoder.btn.read()) delay(10);
        destroyApp();
        return;
    }
}


void AppColorTemp::_publish()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "{\"color_temp_kelvin\":%d}", _data.color_temp);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppColorTemp::_render()
{
    auto* canvas = _data.hal->canvas;

    canvas->fillScreen((uint32_t)0x08090D);

    /* Background arc */
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP,
                    (uint32_t)0x181A22);

    /* Arc fill color: warm amber → cool blue-white */
    float t = (float)(_data.color_temp - COLOR_TEMP_MIN) / (COLOR_TEMP_MAX - COLOR_TEMP_MIN);
    uint8_t r = (uint8_t)(0xFF - (int)(0x37 * t));
    uint8_t g = (uint8_t)(0xA0 + (int)(0x40 * t));
    uint8_t b = (uint8_t)(0x40 + (int)(0xBF * t));
    uint32_t col = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    float angle = ARC_START + t * ARC_SWEEP;
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, col);

    /* Kelvin value in center */
    char k_str[12];
    snprintf(k_str, sizeof(k_str), "%dK", _data.color_temp);
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);
    canvas->setTextColor(col);
    canvas->drawCenterString(k_str, CX, CY - 16);

    /* WARM / COOL label */
    canvas->setTextSize(0.75f);
    uint32_t dim = ((uint32_t)(r / 3) << 16) | ((uint32_t)(g / 3) << 8) | (b / 3);
    canvas->setTextColor(dim);
    canvas->drawCenterString(t < 0.5f ? "WARM" : "COOL", CX, CY + 14);

    /* Connecting indicator */
    if (!_data.state_received) {
        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x2A2A3A);
        canvas->drawCenterString("connecting...", CX, 195);
    }

    /* Hint */
    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x181A22);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppColorTemp::onDestroy()
{
    _log("onDestroy");
}
