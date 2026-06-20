#pragma once
#include "../app_ui.h"

namespace MOONCAKE { namespace USER_APP {

namespace LIGHT {
    enum Mode_t { MODE_DIMMER = 0, MODE_COLOR_TEMP = 1 };
    struct Data_t {
        bool is_on = true;
        int brightness = 128;
        int color_temp = 4000;
        Mode_t mode = MODE_DIMMER;
        bool state_received = false;
        bool publish_pending = false;
        uint32_t last_publish_ms = 0;
        uint32_t last_local_change_ms = 0;
    };
}

class AppLight : public AppUI {
private:
    const char* _tag = "app_light";
    LIGHT::Data_t _data;
    void _handle_encoder();
    void _handle_touch();
    void _publish_brightness();
    void _publish_color_temp();
    void _publish_color_temp_with_state();
    void _render();
public:
    void onSetup() override;
    void onCreate() override;
    void onRunning() override;
    void onDestroy() override;
};

}} // namespace MOONCAKE::USER_APP
