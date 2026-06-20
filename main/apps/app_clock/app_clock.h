#pragma once
#include "../../hal/hal.h"
#include "../app.h"

namespace MOONCAKE { namespace USER_APP {

namespace CLOCK {
    struct Data_t {
        HAL::HAL* hal = nullptr;
        bool wants_light = false;
        uint32_t last_render_ms = 0;
    };
}

class AppClock : public APP_BASE {
private:
    const char* _tag = "app_clock";
    CLOCK::Data_t _data;
    void _handle_touch();
    void _render();
public:
    void onSetup() override;
    void onCreate() override;
    void onRunning() override;
    void onDestroy() override;
    bool wantsLight() const { return _data.wants_light; }
};

}} // namespace MOONCAKE::USER_APP
