/**
 * @file launcher_render_callback.hpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2023-07-25
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once
#include "launcher_icons/launcher_icons.h"
#include <iostream>
#include <array>
#include <cmath>


/* Astrolabe brass palette */
#define BRASS_BRIGHT    (uint32_t)0xB8860B
#define BRASS_MID       (uint32_t)0x7A5C1E
#define BRASS_DARK      (uint32_t)0x3A2A08
#define BRASS_SUBTLE    (uint32_t)0x1E1606

/* Render setting */
#define THEME_COLOR_FG              BRASS_BRIGHT
#define THEME_COLOR_BG              (uint32_t)0x0D0A04
#define SELECTOR_COLOR              (uint32_t)0xDAA520
#define SELECTOR_RADIUS             5
#define ICON_RADIUS                 22
#define ICON_SELECTED_R_OFFSET      3
#define ICON_TAG_COLOR              (uint32_t)0xD4AA50
#define ICON_TAG_UP_OFFSET          -24
#define ICON_TAG_DOWN_OFFSET        0

#define ICON_NUM                    8


/* Struct to hold icon value */
struct Icon_t
{
    std::string tag_up;
    std::string tag_down;
    void* icon_pic;
    uint32_t color;
    int x;
    int y;
};

/* Icon list */
static std::array<Icon_t, ICON_NUM> icon_list;

/* Icon color — muted, rich tones consistent with astrolabe palette */
static std::array<uint32_t, ICON_NUM> icon_color_list = {
    0xB04030,  // deep red
    0x2A4898,  // deep blue
    0x1A6838,  // deep green
    0x0A5850,  // deep teal
    0x9A4E20,  // deep amber
    0x0A5840,  // deep green-teal
    0x004E9A,  // deep cobalt
    0x2A3E5A   // slate blue
};

/* Icon tag */
static std::array<std::string, ICON_NUM * 2> icon_tag_list = {
    "LIGHT", "STUDY",
    "RTC", "TIME",
    "RFID", "TEST",
    "BRIGHTNESS", "SET",
    "WIFI", "SCAN",
    "BLE", "SERVER",
    "TEMP CTRL", "DEMO",
    "MORE", ""
};

/* Icon pic */
static std::array<const uint16_t*, ICON_NUM> icon_pic_list = {
    image_data_icon_lcd,
    image_data_icon_rtc,
    image_data_icon_rfid,
    image_data_icon_brigntness,
    image_data_icon_wifi,
    image_data_icon_ble,
    image_data_icon_temp,
    image_data_icon_more
};

/* Sprite to render icon with transparency */
static LGFX_Sprite icon_sprite_list[ICON_NUM];




struct LauncherRender_CB_t : public SMOOTH_MENU::SimpleMenuCallback_t
{
    private:
        LGFX_Sprite* _canvas;

        void _drawAstrolabeBackground()
        {
            /* Dark base */
            _canvas->fillScreen(THEME_COLOR_BG);

            /* Outer limb — double ring for depth */
            _canvas->drawCircle(120, 120, 116, BRASS_DARK);
            _canvas->drawCircle(120, 120, 115, BRASS_MID);
            _canvas->drawCircle(120, 120, 112, BRASS_DARK);

            /* Graduation ticks on limb — major every 30° (icon positions), minor every 10°, small every 5° */
            for (int deg = 0; deg < 360; deg += 5)
            {
                float rad = deg * (float)M_PI / 180.0f;
                float c = cosf(rad), s = sinf(rad);
                bool major  = (deg % 30 == 0);
                bool medium = (deg % 10 == 0);
                uint32_t col = major ? BRASS_BRIGHT : (medium ? BRASS_MID : BRASS_DARK);
                int r_out = 114;
                int r_in  = major ? 101 : (medium ? 107 : 111);
                _canvas->drawLine(
                    (int)(120 + r_out * c), (int)(120 + r_out * s),
                    (int)(120 + r_in  * c), (int)(120 + r_in  * s),
                    col
                );
            }

            /* Icon orbit track (subtle) */
            _canvas->drawCircle(120, 120, 95, BRASS_SUBTLE);

            /* Selector orbit track */
            _canvas->drawCircle(120, 120, 60, BRASS_DARK);

            /* Inner decorative rings */
            _canvas->drawCircle(120, 120, 35, BRASS_DARK);
            _canvas->drawCircle(120, 120, 33, BRASS_SUBTLE);

            /* Center pivot */
            _canvas->fillSmoothCircle(120, 120, 4, BRASS_MID);
            _canvas->fillSmoothCircle(120, 120, 2, BRASS_BRIGHT);
        }

    public:
        inline void setCanvas(LGFX_Sprite* canvasPtr) { _canvas = canvasPtr; }

        /* Override render callback */
        void renderCallback(
            const std::vector<SMOOTH_MENU::Item_t*>& menuItemList,
            const SMOOTH_MENU::RenderAttribute_t& selector,
            const SMOOTH_MENU::RenderAttribute_t& camera
        ) override
        {
            /* Draw astrolabe background */
            _drawAstrolabeBackground();

            /* Draw selector */
            _canvas->fillSmoothCircle(selector.x, selector.y, SELECTOR_RADIUS, SELECTOR_COLOR);

            /* Draw icons that move with menu items */
            int icon_r = 190;
            int x;
            int y;
            for (int i = 0; i < (int)icon_list.size(); i++)
            {
                x = (menuItemList[i]->x - 120) * icon_r / 120 + 120;
                y = (menuItemList[i]->y - 120) * icon_r / 120 + 120;

                if (i == selector.targetItem)
                {
                    _canvas->fillSmoothCircle(x, y, selector.width + ICON_SELECTED_R_OFFSET, icon_list[i].color);
                    icon_sprite_list[i].pushRotateZoom(_canvas, x, y, 0, 1.1, 1.1, TFT_BLACK);
                }
                else
                {
                    _canvas->fillSmoothCircle(x, y, ICON_RADIUS, icon_list[i].color);
                    icon_sprite_list[i].pushRotateZoom(_canvas, x, y, 0, 1, 1, TFT_BLACK);
                }
            }

            /* Draw icon tag at center */
            int new_r = 20;
            int icon_tag_x = (selector.x - 120) * new_r / 120 + 120;
            int icon_tag_y = (selector.y - 120) * new_r / 120 + 120;

            if (selector.targetItem < (int)icon_list.size())
            {
                _canvas->setFont(GUI_FONT_CN_BIG);
                _canvas->setTextColor(ICON_TAG_COLOR);

                _canvas->drawCenterString(
                    icon_list[selector.targetItem].tag_up.c_str(),
                    icon_tag_x,
                    icon_tag_y + ICON_TAG_UP_OFFSET
                );
                _canvas->drawCenterString(
                    icon_list[selector.targetItem].tag_down.c_str(),
                    icon_tag_x,
                    icon_tag_y + ICON_TAG_DOWN_OFFSET
                );
            }
            else
            {
                _canvas->drawCenterString(
                    "-. -",
                    icon_tag_x,
                    icon_tag_y + ICON_TAG_UP_OFFSET
                );
            }
        }
};
