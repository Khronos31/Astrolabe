#include "app_clock.h"
#include <cstdio>
#include <ctime>

using namespace MOONCAKE::USER_APP;
using namespace MOONCAKE::USER_APP::UI;

static const char* _wday[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};


void AppClock::onSetup()
{
    setAllowBgRunning(false);
    CLOCK::Data_t d; _data = d;
    _bind_hal();
}


void AppClock::onCreate()
{
    _log("onCreate");
    _render();
}


void AppClock::onRunning()
{
    // Clock IS the idle screen — keep activity timestamp alive so _simple_app_manager never times out
    _mark_activity();

    hal->mqtt.poll();

    // Encoder rotate → go to Light
    if (hal->encoder.wasMoved(true)) {
        hal->buzz.tone(4000, 20);
        _data.wants_light = true;
        destroyApp();
        return;
    }

    // Button → back to Launcher
    if (_back_button_pressed()) {
        destroyApp();
        return;
    }

    // Center tap → go to Light
    if (_handle_center_touch() == Press::SHORT) {
        hal->buzz.tone(4000, 20);
        _data.wants_light = true;
        destroyApp();
        return;
    }

    if (millis() - _data.last_render_ms >= 500) {
        _render();
    }
}


void AppClock::_render()
{
    time_t now;
    struct tm t;
    time(&now);
    localtime_r(&now, &t);
    bool time_valid = (t.tm_year > 100);

    auto* canvas = hal->canvas;
    canvas->fillScreen((uint32_t)0x020408);
    canvas->setFont(GUI_FONT_CN_BIG);

    if (time_valid) {
        char hhmm[6];
        snprintf(hhmm, sizeof(hhmm), "%02d:%02d", t.tm_hour, t.tm_min);
        canvas->setTextSize(1.5f);
        canvas->setTextColor((uint32_t)0xA0C0FF);
        canvas->drawCenterString(hhmm, CX, 72);

        int year = t.tm_year + 1900;
        int mon  = t.tm_mon + 1;
        int mday = t.tm_mday;
        char date[20];
        date[0] = '0' + (year / 1000) % 10;
        date[1] = '0' + (year / 100)  % 10;
        date[2] = '0' + (year / 10)   % 10;
        date[3] = '0' + (year)        % 10;
        date[4] = '/';
        date[5] = '0' + mon / 10;
        date[6] = '0' + mon % 10;
        date[7] = '/';
        date[8] = '0' + mday / 10;
        date[9] = '0' + mday % 10;
        date[10] = '\0';
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x405060);
        canvas->drawCenterString(date, CX, 126);

        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x507080);
        canvas->drawCenterString(_wday[t.tm_wday], CX, 154);
    } else {
        canvas->setTextSize(1.5f);
        canvas->setTextColor((uint32_t)0x303030);
        canvas->drawCenterString("--:--", CX, 72);
        canvas->setTextSize(0.75f);
        canvas->setTextColor((uint32_t)0x203040);
        canvas->drawCenterString("syncing...", CX, 140);
    }

    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x181818);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
    _data.last_render_ms = millis();
}


void AppClock::onDestroy()
{
    _log("onDestroy");
}
