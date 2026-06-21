/**
 * @file launcher.h
 * @author Forairaaaaa
 * @brief 
 * @version 0.1
 * @date 2023-07-25
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include "../app.h"
#include "../../hal/hal.h"
#include "../utilities/smooth_menu/src/simple_menu/simple_menu.h"
#include "launcher_render_callback.hpp"
#include "../utilities/gui_base/gui_base.h"

#include "../app_light/app_light.h"
#include "../app_ac/app_ac.h"
#include "../app_goodnight/app_goodnight.h"
#include "../app_clock/app_clock.h"
#include "../app_media/app_media.h"
#include "../app_cover/app_cover.h"
#include "../app_embodied/app_embodied.h"



namespace MOONCAKE
{
    namespace USER_APP
    {
        namespace LAUNCHER
        {
            struct Data_t
            {
                HAL::HAL* hal = nullptr;
                SMOOTH_MENU::Simple_Menu* menu;
                LauncherRender_CB_t* menu_render_cb;
            };
        }

        class Launcher : public APP_BASE
        {
            private:
                const char* _tag = "launcher";

                LAUNCHER::Data_t _data;
                void _menu_init();
                void _icon_list_init();
                void _launcher_init();
                void _launcher_loop();
                void _app_open_callback(uint8_t selectedNum);
                bool _simple_app_manager(MOONCAKE::APP_BASE* app);
                void _idle_loop(bool start_in_light = false);

            public:
                Launcher() = default;
                ~Launcher();

            
                /**
                 * @brief Lifecycle callbacks for derived to override
                 * 
                 */
                /* Setup App configs, called when App "install()" */
                void onSetup();

                /* Life cycle */
                void onCreate();
                // void onResume();
                void onRunning();
                // void onRunningBG();
                // void onPause();
                // void onDestroy();
        };

    }
}

