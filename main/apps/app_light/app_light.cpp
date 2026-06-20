#include "app_light.h"
#include "../common_define.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

#define TOPIC_SET   "astrolabe/light/study/set"
#define TOPIC_STATE "astrolabe/light/study/state"
#define PUBLISH_INTERVAL_MS  120
#define BRIGHTNESS_STEP        8
#define COLOR_TEMP_STEP      200
#define COLOR_TEMP_MIN      2700
#define COLOR_TEMP_MAX      6500

#define CX  120
#define CY  120
#define ARC_START  135.0f
#define ARC_SWEEP  270.0f
#define R_OUT       98
#define R_IN        78

#define GHOST_MS     70   // below this = ghost touch
#define LONGPRESS_MS 500  // above this = mode switch

using namespace MOONCAKE::USER_APP;


void AppLight::onSetup()
{
    setAllowBgRunning(false);
    LIGHT::Data_t d; _data = d;
    _data.hal = (HAL::HAL*)getUserData();
}


void AppLight::onCreate()
{
    _log("onCreate");

    _data.hal->mqtt.subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            if (millis() - _data.last_local_change_ms < 2000) return;

            _data.is_on = (strstr(payload, "\"ON\"") != nullptr);

            const char* b = strstr(payload, "\"brightness\":");
            if (b) sscanf(b + 13, "%d", &_data.brightness);

            const char* ct = strstr(payload, "\"color_temp_kelvin\":");
            if (ct) sscanf(ct + 20, "%d", &_data.color_temp);
            _data.color_temp = std::max(COLOR_TEMP_MIN, std::min(COLOR_TEMP_MAX, _data.color_temp));

            _data.state_received = true;
            _log("rcv on=%d br=%d ct=%d", _data.is_on, _data.brightness, _data.color_temp);
            _render();
        });

    _render();
}


void AppLight::_handle_encoder()
{
    if (!_data.hal->encoder.wasMoved(true)) return;

    int dir = _data.hal->encoder.getDirection();

    if (_data.mode == LIGHT::MODE_DIMMER) {
        _data.brightness += (dir < 1) ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP;
        if (_data.brightness > 255) _data.brightness = 255;
        if (_data.brightness < 0)   _data.brightness = 0;
        if (!_data.is_on && _data.brightness > 0) _data.is_on = true;
    } else {
        _data.color_temp += (dir < 1) ? COLOR_TEMP_STEP : -COLOR_TEMP_STEP;
        if (_data.color_temp > COLOR_TEMP_MAX) _data.color_temp = COLOR_TEMP_MAX;
        if (_data.color_temp < COLOR_TEMP_MIN) _data.color_temp = COLOR_TEMP_MIN;
    }

    _data.last_local_change_ms = millis();
    _data.hal->last_activity_ms = millis();
    _data.publish_pending = true;
    _render();
}


void AppLight::_handle_touch()
{
    if (!_data.hal->tp.isTouched()) return;
    _data.hal->tp.update();

    int tx = _data.hal->tp.getTouchPointBuffer().x - CX;
    int ty = _data.hal->tp.getTouchPointBuffer().y - CY;
    if (tx * tx + ty * ty > 70 * 70) {
        while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
        return;
    }

    uint32_t press_start = millis();
    while (_data.hal->tp.isTouched()) {
        delay(10);
        _data.hal->tp.update();
        if (millis() - press_start >= LONGPRESS_MS) {
            // Switch mode immediately on threshold, then drain
            _data.mode = (_data.mode == LIGHT::MODE_DIMMER)
                         ? LIGHT::MODE_COLOR_TEMP : LIGHT::MODE_DIMMER;
            _data.hal->buzz.tone(3000, 40);
            _render();
            while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
            return;
        }
    }

    uint32_t duration = millis() - press_start;
    if (duration < GHOST_MS) return;

    // Short press: toggle ON/OFF
    _data.is_on = !_data.is_on;
    if (_data.is_on && _data.brightness == 0) _data.brightness = 128;
    _data.hal->buzz.tone(4000, 20);
    _data.last_local_change_ms = millis();
    _data.hal->last_activity_ms = millis();
    if (_data.mode == LIGHT::MODE_DIMMER)
        _publish_brightness();
    else
        _publish_color_temp_with_state();
    _render();
}


void AppLight::onRunning()
{
    _data.hal->mqtt.poll();

    _handle_encoder();

    if (_data.publish_pending &&
        millis() - _data.last_publish_ms >= PUBLISH_INTERVAL_MS)
    {
        if (_data.mode == LIGHT::MODE_DIMMER)
            _publish_brightness();
        else
            _publish_color_temp();
        _data.publish_pending = false;
    }

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read()) delay(10);
        destroyApp();
        return;
    }

    _handle_touch();
}


void AppLight::_publish_brightness()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"state\":\"%s\",\"brightness\":%d}",
             _data.is_on ? "ON" : "OFF", _data.brightness);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppLight::_publish_color_temp()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "{\"color_temp_kelvin\":%d}", _data.color_temp);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppLight::_publish_color_temp_with_state()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"state\":\"%s\",\"color_temp_kelvin\":%d}",
             _data.is_on ? "ON" : "OFF", _data.color_temp);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppLight::_render()
{
    auto* canvas = _data.hal->canvas;

    if (_data.mode == LIGHT::MODE_DIMMER) {
        canvas->fillScreen((uint32_t)0x0D0A04);
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP,
                        (uint32_t)0x2A1E06);

        if (_data.is_on && _data.brightness > 0) {
            float angle = ARC_START + (_data.brightness / 255.0f) * ARC_SWEEP;
            uint32_t col = (_data.brightness > 200) ? 0xDAA520 :
                           (_data.brightness > 120) ? 0xB8860B : 0x7A5C1E;
            canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, col);
        }

        int pct = (_data.brightness * 100 + 127) / 255;
        char pct_str[12];
        snprintf(pct_str, sizeof(pct_str), "%d%%", pct);
        canvas->setFont(GUI_FONT_CN_BIG);
        canvas->setTextSize(1.0f);
        canvas->setTextColor(_data.is_on ? (uint32_t)0xDAA520 : (uint32_t)0x4A3810);
        canvas->drawCenterString(pct_str, CX, CY - 16);

        canvas->setTextSize(0.75f);
        canvas->setTextColor(_data.is_on ? (uint32_t)0xB8860B : (uint32_t)0x3A2A08);
        canvas->drawCenterString(_data.is_on ? "ON" : "OFF", CX, CY + 14);

    } else {
        canvas->fillScreen((uint32_t)0x08090D);
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP,
                        (uint32_t)0x181A22);

        float t = (float)(_data.color_temp - COLOR_TEMP_MIN) / (COLOR_TEMP_MAX - COLOR_TEMP_MIN);
        uint8_t r = (uint8_t)(0xFF - (int)(0x37 * t));
        uint8_t g = (uint8_t)(0xA0 + (int)(0x40 * t));
        uint8_t b = (uint8_t)(0x40 + (int)(0xBF * t));
        uint32_t col = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

        float angle = ARC_START + t * ARC_SWEEP;
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, col);

        char k_str[12];
        snprintf(k_str, sizeof(k_str), "%dK", _data.color_temp);
        canvas->setFont(GUI_FONT_CN_BIG);
        canvas->setTextSize(1.0f);
        canvas->setTextColor(col);
        canvas->drawCenterString(k_str, CX, CY - 16);

        canvas->setTextSize(0.75f);
        uint32_t dim_col = ((uint32_t)(r / 3) << 16) | ((uint32_t)(g / 3) << 8) | (b / 3);
        canvas->setTextColor(dim_col);
        canvas->drawCenterString(t < 0.5f ? "WARM" : "COOL", CX, CY + 14);
    }

    /* Mode indicator: DIM | CLR — active one is bright */
    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor(_data.mode == LIGHT::MODE_DIMMER
                         ? (uint32_t)0xDAA520 : (uint32_t)0x2A1E06);
    canvas->drawCenterString("DIM", CX - 22, 183);
    canvas->setTextColor((uint32_t)0x3A3A3A);
    canvas->drawCenterString("|", CX, 183);
    canvas->setTextColor(_data.mode == LIGHT::MODE_COLOR_TEMP
                         ? (uint32_t)0xA0C8FF : (uint32_t)0x1A2030);
    canvas->drawCenterString("CLR", CX + 22, 183);

    if (!_data.state_received) {
        canvas->setTextColor(_data.mode == LIGHT::MODE_DIMMER
                             ? (uint32_t)0x4A3810 : (uint32_t)0x2A2A3A);
        canvas->drawCenterString("connecting...", CX, 197);
    }

    canvas->setTextColor((uint32_t)0x222222);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppLight::onDestroy()
{
    _log("onDestroy");
}
