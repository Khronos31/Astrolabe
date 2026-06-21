#include "app_embodied.h"

#define TOPIC_TRIGGER    "astrolabe/embodied/trigger"
#define DONE_DISPLAY_MS  2000

using namespace MOONCAKE::USER_APP;
using namespace MOONCAKE::USER_APP::UI;


void AppEmbodied::onSetup()
{
    setAllowBgRunning(false);
    EMBODIED::Data_t d; _data = d;
    _bind_hal();
}


void AppEmbodied::onCreate()
{
    _log("onCreate");
    _render();
}


void AppEmbodied::onRunning()
{
    if (_data.state == EMBODIED::DONE) {
        if (millis() - _data.done_at_ms >= DONE_DISPLAY_MS)
            destroyApp();
        return;
    }

    if (_back_button_pressed()) {
        destroyApp();
        return;
    }

    if (_handle_center_touch() == Press::SHORT) {
        hal->buzz.tone(1800, 60);
        hal->mqtt.publish(TOPIC_TRIGGER, "手動実行");
        _mark_activity();
        _data.state      = EMBODIED::DONE;
        _data.done_at_ms = millis();
        _render();
    }
}


void AppEmbodied::_render()
{
    auto* canvas = hal->canvas;
    canvas->fillScreen((uint32_t)0x050212);

    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);

    if (_data.state == EMBODIED::DONE) {
        canvas->setTextColor((uint32_t)0x00B898);
        canvas->drawCenterString("CALLED", CX, CY - 12);
    } else {
        canvas->setTextColor((uint32_t)0x006050);
        canvas->drawCenterString("EMBODIED", CX, CY - 28);
        canvas->drawCenterString("HA", CX, CY - 4);

        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x003028);
        canvas->drawCenterString("tap to call", CX, CY + 28);
        canvas->drawCenterString("press: back", CX, 210);
    }

    canvas->pushSprite(0, 0);
}


void AppEmbodied::onDestroy()
{
    _log("onDestroy");
}
