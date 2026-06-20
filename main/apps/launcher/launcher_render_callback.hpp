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

#define ICON_NUM                    2


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
    0xB8860B,  // brass (dimmer)
    0x2A4898,  // deep blue (color temp)
};

/* Icon tag */
static std::array<std::string, ICON_NUM * 2> icon_tag_list = {
    "LIGHT", "STUDY",
    "COLOR", "TEMP",
};

/* Icon pic */
static std::array<const uint16_t*, ICON_NUM> icon_pic_list = {
    image_data_icon_lightbulb,
    image_data_icon_sunny,
};

/* Sprite to render icon with transparency */
static LGFX_Sprite icon_sprite_list[ICON_NUM];




struct LauncherRender_CB_t : public SMOOTH_MENU::SimpleMenuCallback_t
{
    private:
        LGFX_Sprite* _canvas;

        void _drawAstrolabeBackground()
        {
            _canvas->fillScreen(TFT_BLACK);
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
