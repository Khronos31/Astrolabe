/**
 * @file app_ui.h
 * @brief Astrolabe共通UI基底クラス。中心円タッチ・エンコーダボタン・共通定数を集約。
 *
 * 各アプリは APP_BASE ではなく AppUI を継承し、_bind_hal() で hal を取得する。
 * タッチ/ボタンの定型処理はここのヘルパーを呼ぶ。
 */
#pragma once
#include "../hal/hal.h"
#include "app.h"
#include "common_define.h"
#include <vector>

namespace MOONCAKE { namespace USER_APP {

namespace UI {
    /* レイアウト */
    constexpr int CX        = 120;
    constexpr int CY        = 120;
    constexpr int CENTER_R  = 70;     // 中心タッチ有効半径

    /* タッチタイミング */
    constexpr uint32_t GHOST_MS     = 70;    // これ未満はゴーストタッチ
    constexpr uint32_t LONGPRESS_MS = 500;   // これ以上は長押し

    /* リング描画（調光・AC共通） */
    constexpr float ARC_START = 135.0f;
    constexpr float ARC_SWEEP = 270.0f;
    constexpr int   R_OUT     = 98;
    constexpr int   R_IN      = 78;

    /* 中心タッチの判定結果 */
    enum class Press { NONE, SHORT, LONG };
}

class AppUI : public APP_BASE {
protected:
    HAL::HAL* hal = nullptr;
    std::vector<HAL::MQTTSubscriptionId_t> _mqtt_subscription_ids;

    /* getUserData() から HAL を取得（onSetupで呼ぶ） */
    void _bind_hal() { hal = (HAL::HAL*)getUserData(); }

    HAL::MQTTSubscriptionId_t _mqtt_subscribe(const char* topic, HAL::MQTTSubCallback_t callback) {
        if (!hal) return 0;
        auto id = hal->mqtt.subscribe(topic, callback);
        if (id != 0) {
            _mqtt_subscription_ids.push_back(id);
        }
        return id;
    }

    void _clear_mqtt_subscriptions() {
        if (!hal) {
            _mqtt_subscription_ids.clear();
            return;
        }
        for (auto id : _mqtt_subscription_ids) {
            hal->mqtt.unsubscribe(id);
        }
        _mqtt_subscription_ids.clear();
    }

    /* 最終操作時刻を更新（アイドルタイムアウト抑止） */
    void _mark_activity() { hal->last_activity_ms = millis(); }

    /* 指が離れるまで空読み */
    void _drain_touch() {
        while (hal->tp.isTouched()) { delay(10); hal->tp.update(); }
    }

    /* エンコーダボタン押下を検出。押されていれば離れるまで待って true。
       呼び元で destroyApp() するかを判断する。 */
    bool _back_button_pressed() {
        if (hal->encoder.btn.read()) return false;
        while (!hal->encoder.btn.read()) delay(10);
        return true;
    }

    /**
     * 中心円タッチを処理し Press を返す。
     * - 中心円(CENTER_R)外: 空読みして NONE
     * - long_ms>0 かつ閾値到達: on_long() を呼び、空読みして LONG
     * - 離した時 duration>=GHOST_MS: SHORT / 未満: NONE
     *
     * long_ms=0 を渡すと長押し判定なし（SHORT/NONEのみ）。
     */
    template <typename F>
    UI::Press _handle_center_touch(uint32_t long_ms, F on_long) {
        if (!hal->tp.isTouched()) return UI::Press::NONE;
        hal->tp.update();

        int tx = hal->tp.getTouchPointBuffer().x - UI::CX;
        int ty = hal->tp.getTouchPointBuffer().y - UI::CY;
        if (tx * tx + ty * ty > UI::CENTER_R * UI::CENTER_R) {
            _drain_touch();
            return UI::Press::NONE;
        }

        uint32_t press_start = millis();
        while (hal->tp.isTouched()) {
            delay(10);
            hal->tp.update();
            if (long_ms && millis() - press_start >= long_ms) {
                on_long();
                _drain_touch();
                return UI::Press::LONG;
            }
        }

        if (millis() - press_start < UI::GHOST_MS) return UI::Press::NONE;
        return UI::Press::SHORT;
    }

    /* 長押し判定なしの簡易版 */
    UI::Press _handle_center_touch() {
        return _handle_center_touch(0, [] {});
    }

public:
    ~AppUI() override {
        _clear_mqtt_subscriptions();
    }
};

}} // namespace MOONCAKE::USER_APP
