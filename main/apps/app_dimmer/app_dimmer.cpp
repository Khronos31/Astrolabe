#include "app_dimmer.h"
#include "../common_define.h"
#include <cstdio>
#include <cstring>

#define TOPIC_SET   "astrolabe/light/study/set"
#define TOPIC_STATE "astrolabe/light/study/state"
#define PUBLISH_INTERVAL_MS  120
#define BRIGHTNESS_STEP        8

#define CX  120
#define CY  120
// Arc: 135° (4:30) → 135+270=405° (7:30), gap at bottom (6 o'clock)
#define ARC_START  135.0f
#define ARC_SWEEP  270.0f
#define R_OUT       98
#define R_IN        78

using namespace MOONCAKE::USER_APP;


void AppDimmer::onSetup()
{
    setAllowBgRunning(false);
    DIMMER::Data_t d; _data = d;
    _data.hal = (HAL::HAL*)getUserData();
}


void AppDimmer::onCreate()
{
    _log("onCreate");

    _data.hal->mqtt.subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            // Ignore HA feedback while user is actively controlling (2s cooldown)
            if (millis() - _data.last_local_change_ms < 2000) return;

            // Payload: {"state":"ON","brightness":200}
            _data.is_on = (strstr(payload, "\"ON\"") != nullptr);
            const char* b = strstr(payload, "\"brightness\":");
            if (b) sscanf(b + 13, "%d", &_data.brightness);
            _data.state_received = true;
            _log("rcv state=%s br=%d", _data.is_on ? "ON" : "OFF", _data.brightness);
            _render();
        });

    _render();
}


void AppDimmer::onRunning()
{
    _data.hal->mqtt.poll();

    /* Encoder */
    if (_data.hal->encoder.wasMoved(true))
    {
        int dir = _data.hal->encoder.getDirection();
        _data.brightness += (dir < 1) ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP;
        if (_data.brightness > 255) _data.brightness = 255;
        if (_data.brightness < 0)   _data.brightness = 0;

        if (!_data.is_on && _data.brightness > 0) _data.is_on = true;

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

    /* Long press (300ms) in center circle (r≤70) → toggle ON/OFF */
    if (_data.hal->tp.isTouched())
    {
        _data.hal->tp.update();
        int tx = _data.hal->tp.getTouchPointBuffer().x - CX;
        int ty = _data.hal->tp.getTouchPointBuffer().y - CY;
        if (tx * tx + ty * ty > 70 * 70) {
            while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
            return;
        }
        uint32_t press_start = millis();
        bool long_press = false;
        while (_data.hal->tp.isTouched()) {
            delay(10);
            _data.hal->tp.update();
            if (millis() - press_start >= 70) { long_press = true; break; }
        }
        while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
        if (!long_press) return;
        _data.hal->buzz.tone(4000, 20);
        _data.is_on = !_data.is_on;
        if (_data.is_on && _data.brightness == 0) _data.brightness = 128;
        _data.last_local_change_ms = millis();
        _publish();
        _render();
    }
}


void AppDimmer::_publish()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"state\":\"%s\",\"brightness\":%d}",
             _data.is_on ? "ON" : "OFF", _data.brightness);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppDimmer::_render()
{
    auto* canvas = _data.hal->canvas;
    canvas->fillScreen((uint32_t)0x0D0A04);

    /* Background arc */
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP,
                    (uint32_t)0x2A1E06);

    /* Filled arc — color shifts warm→bright as brightness increases */
    if (_data.is_on && _data.brightness > 0) {
        float angle = ARC_START + (_data.brightness / 255.0f) * ARC_SWEEP;
        uint32_t col = (_data.brightness > 200) ? 0xDAA520 :
                       (_data.brightness > 120) ? 0xB8860B : 0x7A5C1E;
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, col);
    }

    /* Brightness % in center */
    int pct = (_data.brightness * 100 + 127) / 255;
    char pct_str[12];
    snprintf(pct_str, sizeof(pct_str), "%d%%", pct);
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);
    canvas->setTextColor(_data.is_on ? (uint32_t)0xDAA520 : (uint32_t)0x4A3810);
    canvas->drawCenterString(pct_str, CX, CY - 16);

    /* ON / OFF label */
    canvas->setTextSize(0.75f);
    canvas->setTextColor(_data.is_on ? (uint32_t)0xB8860B : (uint32_t)0x3A2A08);
    canvas->drawCenterString(_data.is_on ? "ON" : "OFF", CX, CY + 14);

    /* Connecting indicator — show if no state received yet */
    if (!_data.state_received) {
        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x4A3810);
        canvas->drawCenterString("connecting...", CX, 195);
    }

    /* Hint */
    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x2A1E06);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppDimmer::onDestroy()
{
    _log("onDestroy");
}
