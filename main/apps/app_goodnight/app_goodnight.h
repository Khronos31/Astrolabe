#pragma once
#include "../app_ui.h"

namespace MOONCAKE { namespace USER_APP {

namespace GOODNIGHT {
    enum State_t { IDLE, RUNNING, DONE };
    struct Data_t {
        State_t state = IDLE;
        uint32_t done_at_ms = 0;
    };
}

class AppGoodnight : public AppUI {
private:
    const char* _tag = "app_goodnight";
    GOODNIGHT::Data_t _data;
    void _render();
public:
    void onSetup() override;
    void onCreate() override;
    void onRunning() override;
    void onDestroy() override;
};

}} // namespace MOONCAKE::USER_APP
