#pragma once
#include "../app_ui.h"

namespace MOONCAKE { namespace USER_APP {

namespace AC {
    enum Mode_t { MODE_COOL = 0, MODE_HEAT = 1 };
    struct Data_t {
        bool is_on = false;
        float temperature = 26.0f;
        Mode_t mode = MODE_COOL;
        bool state_received = false;
        bool publish_pending = false;
        uint32_t last_publish_ms = 0;
        uint32_t last_local_change_ms = 0;
    };
}

class AppAC : public AppUI {
private:
    const char* _tag = "app_ac";
    AC::Data_t _data;
    void _handle_encoder();
    void _handle_touch();
    void _publish_hvac_mode();
    void _publish_off();
    void _publish_temperature();
    void _render();
public:
    void onSetup() override;
    void onCreate() override;
    void onRunning() override;
    void onDestroy() override;
};

}} // namespace MOONCAKE::USER_APP
