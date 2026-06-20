#pragma once
#include "../app.h"
#include "../../hal/hal.h"

namespace MOONCAKE { namespace USER_APP {

namespace DIMMER {
    struct Data_t {
        HAL::HAL* hal            = nullptr;
        int       brightness     = 128;   // 0-255
        bool      is_on          = false;
        bool      state_received = false; // got first MQTT state
        bool      publish_pending = false;
        uint32_t  last_publish_ms = 0;
    };
}

class AppDimmer : public APP_BASE {
public:
    DIMMER::Data_t _data;

    void onSetup()   override;
    void onCreate()  override;
    void onRunning() override;
    void onDestroy() override;

private:
    const char* _tag = "AppDimmer";
    void _render();
    void _publish();
};

}} // MOONCAKE::USER_APP
