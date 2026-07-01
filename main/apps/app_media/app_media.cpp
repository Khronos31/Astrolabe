#include "app_media.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define TOPIC_SET   "astrolabe/media/spotify/set"
#define TOPIC_STATE "astrolabe/media/spotify/state"
#define PUBLISH_INTERVAL_MS  150
#define VOL_STEP   0.05f

#define SPOTIFY_GREEN 0x1DB954

using namespace MOONCAKE::USER_APP;
using namespace MOONCAKE::USER_APP::UI;


/* "key":"value" の value を out に取り出す簡易JSON文字列パーサ */
static void _json_str(const char* payload, const char* key, char* out, size_t out_sz)
{
    const char* p = strstr(payload, key);
    if (!p) { out[0] = '\0'; return; }
    p += strlen(key);
    size_t i = 0;
    while (*p && *p != '"' && i < out_sz - 1) {
        if (*p == '\\' && *(p + 1)) p++;   // エスケープを1つスキップ
        out[i++] = *p++;
    }
    out[i] = '\0';
}


void AppMedia::onSetup()
{
    setAllowBgRunning(false);
    MEDIA::Data_t d; _data = d;
    _bind_hal();
}


void AppMedia::onCreate()
{
    _log("onCreate");

    _mqtt_subscribe(TOPIC_STATE,
        [this](const char* /*topic*/, const char* payload, int /*len*/) {
            if (millis() - _data.last_local_change_ms < 2000) return;

            _data.is_playing = (strstr(payload, "\"state\":\"playing\"") != nullptr);

            const char* v = strstr(payload, "\"volume\":");
            if (v) sscanf(v + 9, "%f", &_data.volume);
            _data.volume = std::max(0.0f, std::min(1.0f, _data.volume));

            _json_str(payload, "\"title\":\"",  _data.title,  sizeof(_data.title));
            _json_str(payload, "\"artist\":\"", _data.artist, sizeof(_data.artist));

            _data.state_received = true;
            _log("rcv play=%d vol=%.2f title=%s", _data.is_playing, _data.volume, _data.title);
            _render();
        });

    _render();
}


void AppMedia::_handle_encoder()
{
    if (!hal->encoder.wasMoved(true)) return;

    int dir = hal->encoder.getDirection();
    _data.volume += (dir < 1) ? VOL_STEP : -VOL_STEP;
    _data.volume = std::max(0.0f, std::min(1.0f, _data.volume));

    _data.last_local_change_ms = millis();
    _mark_activity();
    _data.publish_pending = true;
    _render();
}


void AppMedia::_handle_touch()
{
    /* Long press: 次の曲. Short press: 再生/一時停止トグル. */
    Press p = _handle_center_touch(LONGPRESS_MS, [&] {
        hal->buzz.tone(3000, 40);
        _mark_activity();
        _publish_action("next");
    });

    if (p != Press::SHORT) return;

    _data.is_playing = !_data.is_playing;   // 楽観的更新
    hal->buzz.tone(4000, 20);
    _data.last_local_change_ms = millis();
    _mark_activity();
    _publish_action("play_pause");
    _render();
}


void AppMedia::onRunning()
{
    hal->mqtt.poll();

    _handle_encoder();

    if (_data.publish_pending &&
        millis() - _data.last_publish_ms >= PUBLISH_INTERVAL_MS)
    {
        _publish_volume();
        _data.publish_pending = false;
    }

    if (_back_button_pressed()) {
        destroyApp();
        return;
    }

    _handle_touch();
}


void AppMedia::_publish_volume()
{
    char buf[40];
    snprintf(buf, sizeof(buf), "{\"volume\":%.2f}", _data.volume);
    hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppMedia::_publish_action(const char* action)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "{\"action\":\"%s\"}", action);
    hal->mqtt.publish(TOPIC_SET, buf);
    _data.last_publish_ms = millis();
    _log("pub %s", buf);
}


void AppMedia::_render()
{
    auto* canvas = hal->canvas;
    canvas->fillScreen((uint32_t)0x04100A);

    /* Volume ring */
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, ARC_START + ARC_SWEEP, (uint32_t)0x0C2818);
    float angle = ARC_START + _data.volume * ARC_SWEEP;
    canvas->fillArc(CX, CY, R_OUT, R_IN, ARC_START, angle, (uint32_t)SPOTIFY_GREEN);

    /* Title (上部・小フォント) */
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(0.6f);
    canvas->setTextColor((uint32_t)0x70C090);
    if (_data.title[0])
        canvas->drawCenterString(_data.title, CX, 60);

    /* Volume % (中央) */
    int vol_pct = (int)(_data.volume * 100 + 0.5f);
    char vol_str[8];
    snprintf(vol_str, sizeof(vol_str), "%d%%", vol_pct);
    canvas->setFont(GUI_FONT_CN_BIG);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x40E080);
    canvas->drawCenterString(vol_str, CX, CY - 12);

    /* Play / Pause 状態 */
    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.2f);
    canvas->setTextColor(_data.is_playing ? (uint32_t)0x1DB954 : (uint32_t)0x406050);
    canvas->drawCenterString(_data.is_playing ? "> PLAYING" : "|| PAUSED", CX, CY + 24);

    if (!_data.state_received) {
        canvas->setFont(&fonts::Font2);
        canvas->setTextSize(1.0f);
        canvas->setTextColor((uint32_t)0x2A4A38);
        canvas->drawCenterString("connecting...", CX, 178);
    }

    canvas->setFont(&fonts::Font2);
    canvas->setTextSize(1.0f);
    canvas->setTextColor((uint32_t)0x222222);
    canvas->drawCenterString("press: back", CX, 210);

    canvas->pushSprite(0, 0);
}


void AppMedia::onDestroy()
{
    _clear_mqtt_subscriptions();
    _log("onDestroy");
}
