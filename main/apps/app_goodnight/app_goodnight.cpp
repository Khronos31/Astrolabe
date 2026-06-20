#include "app_goodnight.h"
#include "../common_define.h"
#include <cstring>

#define TOPIC_RUN   "astrolabe/script/goodnight/run"
#define DONE_DISPLAY_MS  2000

#define CX  120
#define CY  120
#define GHOST_MS  70

using namespace MOONCAKE::USER_APP;


void AppGoodnight::onSetup()
{
    setAllowBgRunning(false);
    GOODNIGHT::Data_t d; _data = d;
    _data.hal = (HAL::HAL*)getUserData();
}


void AppGoodnight::onCreate()
{
    _log("onCreate");
    _render();
}


void AppGoodnight::onRunning()
{
    /* After running: show DONE briefly then return */
    if (_data.state == GOODNIGHT::DONE) {
        if (millis() - _data.done_at_ms >= DONE_DISPLAY_MS) {
            destroyApp();
        }
        return;
    }

    /* Encoder button → back to launcher */
    if (!_data.hal->encoder.btn.read()) {
        while (!_data.hal->encoder.btn.read()) delay(10);
        destroyApp();
        return;
    }

    /* Center touch → run script */
    if (_data.hal->tp.isTouched()) {
        _data.hal->tp.update();
        int tx = _data.hal->tp.getTouchPointBuffer().x - CX;
        int ty = _data.hal->tp.getTouchPointBuffer().y - CY;
        if (tx * tx + ty * ty > 70 * 70) {
            while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
            return;
        }

        uint32_t press_start = millis();
        while (_data.hal->tp.isTouched()) { delay(10); _data.hal->tp.update(); }
        if (millis() - press_start < GHOST_MS) return;

        _data.hal->buzz.tone(2000, 60);
        _data.hal->mqtt.publish(TOPIC_RUN, "{}");
        _data.hal->last_activity_ms = millis();
        _data.state = GOODNIGHT::DONE;
        _data.done_at_ms = millis();
        _render();
    }
}


void AppGoodnight::_render()
{
    auto* canvas = _data.hal->canvas;
    canvas->fillScreen((uint32_t)0x04020A);

    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);

    if (_data.state == GOODNIGHT::DONE) {
        canvas->setTextColor((uint32_t)0x6040A0);
        canvas->drawCenterString("DONE", CX, CY - 12);
    } else {
        canvas->setTextColor((uint32_t)0x3A2860);
        canvas->drawCenterString("GOOD", CX, CY - 28);
        canvas->drawCenterString("NIGHT", CX, CY - 4);

        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x1A1230);
        canvas->drawCenterString("tap to run", CX, CY + 28);
        canvas->drawCenterString("press: back", CX, 210);
    }

    canvas->pushSprite(0, 0);
}


void AppGoodnight::onDestroy()
{
    _log("onDestroy");
}
