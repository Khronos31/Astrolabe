#pragma once
#include "../app_ui.h"

namespace MOONCAKE { namespace USER_APP {

namespace MEDIA {
    struct Data_t {
        bool is_playing = false;
        float volume = 0.3f;
        char title[64]  = {0};
        char artist[48] = {0};
        bool state_received = false;
        bool publish_pending = false;
        uint32_t last_publish_ms = 0;
        uint32_t last_local_change_ms = 0;
    };
}

class AppMedia : public AppUI {
private:
    const char* _tag = "app_media";
    MEDIA::Data_t _data;
    void _handle_encoder();
    void _handle_touch();
    void _publish_volume();
    void _publish_action(const char* action);
    void _render();
public:
    void onSetup() override;
    void onCreate() override;
    void onRunning() override;
    void onDestroy() override;
};

}} // namespace MOONCAKE::USER_APP
