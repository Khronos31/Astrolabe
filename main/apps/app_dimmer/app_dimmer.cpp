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
// Arc: 225° (7:30) → 225+270=495° (4:30), sweep = 270°
#define ARC_START  225.0f
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
            // Payload: {"state":"ON","brightness":200}
            _data.is_on = (strstr(payload, "\"ON\"") != nullptr);
            const char* b = strstr(payload, "\"brightness\":");
            if (b) sscanf(b + 13, "%d", &_data.brightness);
            _data.state_received = true;
            _log("rcv state=%s br=%d", _data.is_on ? "ON" : "OFF", _data.brightness);
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

    /* Button */
    if (!_data.hal->encoder.btn.read())
    {
        uint32_t t0 = millis();
        while (!_data.hal->encoder.btn.read())
        {
            delay(10);
            if (millis() - t0 > 800) {       // long press → exit
                while (!_data.hal->encoder.btn.read()) delay(10);
                destroyApp();
                return;
            }
        }
        /* Short press → toggle */
        _data.is_on = !_data.is_on;
        if (_data.is_on && _data.brightness == 0) _data.brightness = 128;
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

    /* Arc tip dot */
    {
        float tip_angle = ARC_START;
        if (_data.is_on && _data.brightness > 0)
            tip_angle += (_data.brightness / 255.0f) * ARC_SWEEP;
        float rad = tip_angle * (float)M_PI / 180.0f;
        int r_mid = (R_OUT + R_IN) / 2;
        int tx = CX + (int)(r_mid * sinf(rad));   // LovyanGFX: 0°=top
        int ty = CY - (int)(r_mid * cosf(rad));
        canvas->fillSmoothCircle(tx, ty, 5, (uint32_t)0xDAA520);
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
    canvas->drawCenterString("hold: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppDimmer::onDestroy()
{
    _log("onDestroy");
}
