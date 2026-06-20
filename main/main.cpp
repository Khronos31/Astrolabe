/**
 * @file va_meter_firmware.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-07-07
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "hal/hal.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "apps/app.h"
#include "apps/launcher/launcher.h"

#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))

extern "C" void app_main(void)
{
    HAL::HAL hal;

    /* Hardware init */
    hal.init();
    // HAL::encoder_test(hal);
    // HAL::tp_test(hal);
    // HAL::rtc_test(hal);

/* Start launcher */
    MOONCAKE::USER_APP::Launcher app_launcher;
    app_launcher.setUserData((void*)&hal);
    app_launcher.onSetup();
    app_launcher.onCreate();
    while (1)
    {
        app_launcher.onRunning();
    }
}
