#pragma once
#include "../app_ui.h"

namespace MOONCAKE { namespace USER_APP {

namespace COVER {
    struct Data_t {
        int  position          = 0;     // 現在位置 0–100（HA から受信）
        int  target_position   = 0;     // 操作中のターゲット位置
        char state_str[16]     = {0};   // "open" / "closed" / "opening" etc.
        bool state_received    = false;
        bool publish_pending   = false;
        uint32_t last_publish_ms        = 0;
        uint32_t last_local_change_ms   = 0;
    };
}

class AppCover : public AppUI {
public:
    void onSetup()   override;
    void onCreate()  override;
    void onRunning() override;
    void onDestroy() override;

private:
    const char* _tag = "app_cover";
    COVER::Data_t _data;

    void _handle_encoder();
    void _handle_touch();
    void _publish_position();
    void _publish_action(const char* action);
    void _render();
};

}} // namespace
