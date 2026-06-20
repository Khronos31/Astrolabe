#include "app_goodnight.h"
#include <cstring>

#define TOPIC_RUN   "astrolabe/script/goodnight/run"
#define DONE_DISPLAY_MS  2000

using namespace MOONCAKE::USER_APP;
using namespace MOONCAKE::USER_APP::UI;


void AppGoodnight::onSetup()
{
    setAllowBgRunning(false);
    GOODNIGHT::Data_t d; _data = d;
    _bind_hal();
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
    if (_back_button_pressed()) {
        destroyApp();
        return;
    }

    /* Center touch → run script */
    if (_handle_center_touch() == Press::SHORT) {
        hal->buzz.tone(2000, 60);
        hal->mqtt.publish(TOPIC_RUN, "{}");
        _mark_activity();
        _data.state = GOODNIGHT::DONE;
        _data.done_at_ms = millis();
        _render();
    }
}


void AppGoodnight::_render()
{
    auto* canvas = hal->canvas;
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
