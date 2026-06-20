#include "app_ac.h"
#include "../common_define.h"
#include <cstdio>
#include <cstring>

#define TOPIC_SET   "astrolabe/climate/study/set"
#define TOPIC_STATE "astrolabe/climate/study/state"
#define PUBLISH_INTERVAL_MS  200
#define TEMP_STEP   1.0f
#define TEMP_MIN   18.0f
#define TEMP_MAX   30.0f

#define CX  120
#define CY  120
#define ARC_START  135.0f
#define ARC_SWEEP  270.0f
#define R_OUT       98
#define R_IN        78

#define GHOST_MS     70
#define LONGPRESS_MS 500

using namespace MOONCAKE::USER_APP;


void AppAC::onSetup()
{
    setAllowBgRunning(false);
    AC::Data_t d; _data = d;
    _data.hal = (HAL::HAL*)getUserData();
}


void AppAC::onCreate()
{
    _log("onCreate");

    _data.hal->mqtt.subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            if (millis() - _data.last_local_change_ms < 2000) return;

            _data.is_on = (strstr(payload, "\"OFF\"") == nullptr);

            const char* t = strstr(payload, "\"temperature\":");
            if (t) sscanf(t + 14, "%f", &_data.temperature);
            if (_data.temperature < TEMP_MIN) _data.temperature = TEMP_MIN;
            if (_data.temperature > TEMP_MAX) _data.temperature = TEMP_MAX;

            // Only update mode if not "off"
            if (strstr(payload, "\"hvac_mode\":\"cool\"")) _data.mode = AC::MODE_COOL;
            if (strstr(payload, "\"hvac_mode\":\"heat\"")) _data.mode = AC::MODE_HEAT;

            _data.state_received = true;
            _log("rcv on=%d temp=%.1f mode=%s", _data.is_on, _data.temperature,
                 _data.mode == AC::MODE_COOL ? "cool" : "heat");
            _render();
        });

    _render();
}


void AppAC::_handle_encoder()
{
    if (!_data.hal->encoder.wasMoved(true)) return;

    int dir = _data.hal->encoder.getDirection();
    _data.temperature += (dir < 1) ? TEMP_STEP : -TEMP_STEP;
    if (_data.temperature > TEMP_MAX) _data.temperature = TEMP_MAX;
    if (_data.temperature < TEMP_MIN) _data.temperature = TEMP_MIN;

    _data.last_local_change_ms = millis();
    _data.hal->last_activity_ms = millis();
    _data.publish_pending = true;
    _render();
}


void AppAC::_handle_touch()
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
            // Switch mode immediately, turn on if off
            _data.mode = (_data.mode == AC::MODE_COOL) ? AC::MODE_HEAT : AC::MODE_COOL;
            _data.is_on = true;
            _data.hal->buzz.tone(3000, 40);
            _data.last_local_change_ms = millis();
    _data.hal->last_activity_ms = millis();
            _publish_hvac_mode();
            _render();
            while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
            return;
        }
    }

    uint32_t duration = millis() - press_start;
    if (duration < GHOST_MS) return;

    // Short press: toggle ON/OFF
    _data.is_on = !_data.is_on;
    _data.hal->buzz.tone(4000, 20);
    _data.last_local_change_ms = millis();
    _data.hal->last_activity_ms = millis();
    if (_data.is_on)
        _publish_hvac_mode();
    else
        _publish_off();
    _render();
}


void AppAC::onRunning()
{
    _data.hal->mqtt.poll();

    _handle_encoder();

    if (_data.publish_pending &&
        millis() - _data.last_publish_ms >= PUBLISH_INTERVAL_MS)
    {
        _publish_temperature();
        _data.publish_pending = false;
    }

    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read()) delay(10);
        destroyApp();
        return;
    }

    _handle_touch();
}


void AppAC::_publish_hvac_mode()
{
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"hvac_mode\":\"%s\"}",
             _data.mode == AC::MODE_COOL ? "cool" : "heat");
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppAC::_publish_off()
{
    _data.hal->mqtt.publish(TOPIC_SET, "{\"state\":\"OFF\"}");
    _data.last_publish_ms = millis();
    _log("pub OFF");
}


void AppAC::_publish_temperature()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "{\"temperature\":%.1f}", _data.temperature);
    _data.hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppAC::_render()
{
    auto* canvas = _data.hal->canvas;

    // Mode-dependent colors
    uint32_t bg, bg_arc, arc_col, text_col, dim_col;
    if (_data.mode == AC::MODE_COOL) {
        bg      = 0x040810;
        bg_arc  = 0x0C1830;
        arc_col = 0x4090FF;
        text_col = 0x80C0FF;
        dim_col  = 0x203060;
    } else {
        bg      = 0x100804;
        bg_arc  = 0x301808;
        arc_col = 0xE06020;
        text_col = 0xFF9040;
        dim_col  = 0x603010;
    }

    canvas->fillScreen(bg);
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP, bg_arc);

    if (_data.is_on) {
        float t = (_data.temperature - TEMP_MIN) / (TEMP_MAX - TEMP_MIN);
        float angle = ARC_START + t * ARC_SWEEP;
        canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, arc_col);
    }

    // Temperature in center
    char temp_str[12];
    snprintf(temp_str, sizeof(temp_str), "%.0f\xc2\xb0\x43", _data.temperature); // °C in UTF-8
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);
    canvas->setTextColor(_data.is_on ? text_col : dim_col);
    canvas->drawCenterString(temp_str, CX, CY - 16);

    // ON/OFF
    canvas->setTextSize(0.75f);
    canvas->setTextColor(_data.is_on ? arc_col : dim_col);
    canvas->drawCenterString(_data.is_on ? "ON" : "OFF", CX, CY + 14);

    // Mode indicator
    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor(_data.mode == AC::MODE_COOL ? (uint32_t)0x80C0FF : (uint32_t)0x0C1830);
    canvas->drawCenterString("COOL", CX - 24, 183);
    canvas->setTextColor((uint32_t)0x3A3A3A);
    canvas->drawCenterString("|", CX, 183);
    canvas->setTextColor(_data.mode == AC::MODE_HEAT ? (uint32_t)0xFF9040 : (uint32_t)0x301808);
    canvas->drawCenterString("HEAT", CX + 24, 183);

    if (!_data.state_received) {
        canvas->setTextColor(dim_col);
        canvas->drawCenterString("connecting...", CX, 197);
    }

    canvas->setTextColor((uint32_t)0x222222);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppAC::onDestroy()
{
    _log("onDestroy");
}
