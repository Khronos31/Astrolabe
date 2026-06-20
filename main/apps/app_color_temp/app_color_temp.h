#pragma once
#include "../app.h"
#include "../../hal/hal.h"

namespace MOONCAKE { namespace USER_APP {

namespace COLOR_TEMP {
    struct Data_t {
        HAL::HAL* hal             = nullptr;
        int       color_temp      = 4000;   // Kelvin, 2700-6500
        bool      state_received  = false;
        bool      publish_pending = false;
        uint32_t  last_publish_ms      = 0;
        uint32_t  last_local_change_ms = 0;
    };
}

class AppColorTemp : public APP_BASE {
public:
    COLOR_TEMP::Data_t _data;
    void onSetup()   override;
    void onCreate()  override;
    void onRunning() override;
    void onDestroy() override;
private:
    const char* _tag = "AppColorTemp";
    void _render();
    void _publish();
};

}} // MOONCAKE::USER_APP
