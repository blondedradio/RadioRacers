// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_feed.hpp

#ifndef __RR_FEED_HPP__
#define __RR_FEED_HPP__

#include <algorithm>
#include <array>
#include <vector>
#include <deque>
#include <string>
#include <utility>

#include <fmt/format.h>
#include <iostream>
#include <memory>

#include "../rr_hud.h"
#include "../../doomtype.h"
#include "../../info.h"
#include "../../i_time.h"
#include "../../p_tick.h"
#include "../../m_easing.h"
#include "../../v_video.h"
#include "../../r_draw.h"
#include "../../z_zone.h"
#include "../../k_color.h"

#include "../../v_draw.hpp" // srb2:Draw

// CONSTANTS

const fixed_t FEED_UPDATE_DISPLAY_TIME = 3*TICRATE;
const fixed_t FEED_UPDATE_FADE_TIME = TICRATE;
const tic_t FEED_UPDATE_SHIFT_TIME = (2*TICRATE)/3;
const int FEED_UPDATE_FADE_IN_START = 9;
const int FEED_UPDATE_FADE_OUT_START = 1;
const int FEED_UPDATE_SPACING_Y = 11;
const int FEED_MAX_FEED_UPDATES = 4;
const float FEED_UPDATE_SCALE = .7f;
const float FEED_UPDATE_ITEM_SCALE = .4f;

const std::string FEED_UPDATE_BACKGROUND_PATCH = "RRHDFDBR";

// ENUMS

typedef enum
{
    UPDATE_NONE = -1, // Default
    UPDATE_WAITING_TO_ENTER, // Waiting to enter
    UPDATE_ENTERING, // Fading-in
    UPDATE_DISPLAYING, // Just displaying
    UPDATE_EXITING, // Fading-out
    UPDATE_EXITED // Done
} updatestate_t;

typedef enum
{
    FEED_DIRECTION_UP = 0,
    FEED_DIRECTION_DOWN
} feeddirection_t;

typedef enum
{
    FEED_POSITION_LEFT = 0,
    FEED_POSITION_RIGHT,
    FEED_POSITION_MIDDLE
} feedposition_t;

struct ItemAnimatedConfig {
    boolean animated = false; // Animated?
    boolean should_flash = false; // Flashing patch?
    int animated_frames = 0; // Number of frames to animate
    int tics_per_frame = 6; // Tics per frame (how long each frame should animate for)
    skincolornum_t flashing_colour = SKINCOLOR_RED; // The colour to flash
};

// Item patches for feed updates - doing same thing in rr_item_timers.cpp with ItemTimer struct
struct ItemConfigForFeedUpdate {
    std::string patch; // Graphic name
    int width; // Patch width
    int height; // Patch height
    ItemAnimatedConfig animation_config; // Animation-related variables
    std::optional<float> patch_scale; // Graphic scale
    std::optional<int> y_offset; // Shrink, Eggmark, etc
    skincolornum_t recolour = SKINCOLOR_NONE; // The colour to recolour the item too (usually droptarget)
    boolean rainbow = false; // Rainbow colormap
};

struct ProcessedItemConfig {
    std::string patch_name;
    float patch_scale;
    int width_scaled;
    int height_scaled;
    int x_offset;
    int y_offset;
    boolean should_flash;
    boolean rainbow;
    UINT8 *colormap;
    UINT8 *item_recolormap;
};

// Drawing configurations
struct FeedConfig {
    int start_x; // the STARTING X-position on HUD
    int start_y; // the STARTING Y-position on HUD;
    int shift_y; // How far to shift an update in the feed by when there's a new entry

    feedposition_t position; // Determines alignment-style for text
    feeddirection_t direction; // Determines direction feed will update in
    INT32 hudflags; // For centering correctly in the HUD
    int maxupdates = FEED_MAX_FEED_UPDATES; // Max updates to show in the feed at once
};

FeedConfig TopMiddleConfig {
    160, 
    6, 
    FEED_UPDATE_SPACING_Y, 
    FEED_POSITION_MIDDLE, 
    FEED_DIRECTION_DOWN,
    V_SNAPTOTOP,
    3
};
FeedConfig BottomMiddleConfig {
    160, 
    210, 
    FEED_UPDATE_SPACING_Y, 
    FEED_POSITION_MIDDLE, 
    FEED_DIRECTION_UP,
    V_SNAPTOBOTTOM,
    3
};
FeedConfig NoItemBoxConfig {
    5, 
    65, 
    FEED_UPDATE_SPACING_Y, 
    FEED_POSITION_LEFT, 
    FEED_DIRECTION_UP,
    V_SNAPTOLEFT|V_SNAPTOTOP
};
FeedConfig ItemBoxConfig {
    65, 
    65, 
    FEED_UPDATE_SPACING_Y, 
    FEED_POSITION_LEFT, 
    FEED_DIRECTION_UP,
    V_SNAPTOLEFT|V_SNAPTOTOP
};
FeedConfig TimestampConfig {
    BASEVIDWIDTH - 5, 
    65, 
    FEED_UPDATE_SPACING_Y, 
    FEED_POSITION_RIGHT, 
    FEED_DIRECTION_UP,
    V_SNAPTORIGHT|V_SNAPTOTOP
};

// CLASSES

// Forward declaration
class Hudfeed;

// Util functions
namespace FeedUpdateUtils {
    // Calculate variables such as scale, offsetting and padding
    ProcessedItemConfig processItemConfig (ItemConfigForFeedUpdate config) {

        tic_t current_time = I_GetTime();

        float item_scale = config.patch_scale.has_value() ? config.patch_scale.value() : FEED_UPDATE_ITEM_SCALE;
        int item_width_scaled = static_cast<int>(config.width * item_scale);
        int item_height_scaled = static_cast<int>(config.height * item_scale);
        std::string item_patch_name = config.patch;

        // For flashing items (Eggmark, SPB, mine)
        UINT8 *colormap = (config.animation_config.should_flash && (current_time & 1) ? R_GetTranslationColormap(TC_BLINK, config.animation_config.flashing_colour, GTC_CACHE) : NULL);
        UINT8 *item_recolormap = (config.recolour) ? R_GetTranslationColormap(TC_DEFAULT, config.recolour, GTC_CACHE) : NULL;

        if (config.animation_config.animated) {
            int frames = config.animation_config.animated_frames;
            int tics_per_frame = config.animation_config.tics_per_frame;
            item_patch_name = fmt::format("{0}{1}", config.patch, (((current_time % (frames*tics_per_frame)) / tics_per_frame) + 1));
        }

        int item_y_offset = 0;
        if (config.y_offset.has_value()) {
            item_y_offset = config.y_offset.value();
        } else {
            item_y_offset = -(item_height_scaled/4);
            if (item_height_scaled <= 10) { 
                item_y_offset = -(item_height_scaled/6);
            }
        }

        int item_x_offset = (item_width_scaled <= 10) ? std::min(10 - (item_width_scaled/2), 3) : (item_width_scaled / 4);

        return {
            item_patch_name, 
            item_scale, 
            item_width_scaled, 
            item_height_scaled, 
            item_x_offset, 
            item_y_offset, 
            config.animation_config.should_flash,
            config.rainbow,
            static_cast<UINT8*>(colormap),
            static_cast<UINT8*>(item_recolormap)
        };
    }

    int getNameWidth (std::string name) {
        return V_StringScaledWidth(
            FloatToFixed(FEED_UPDATE_SCALE),
            FRACUNIT,
            FRACUNIT,
            FRACUNIT,
            TINY_FONT,
            name.c_str()
        )/FRACUNIT;
    }
}

// Base update "class", each update in the feed share these properties/methods
using srb2::Draw;
class BaseFeedUpdate {
    protected:
        fixed_t x;
        fixed_t x_overwrite = -1; // For any updates that need to change the x-coordinate
        fixed_t y;
        fixed_t target_y; // When shifting, there should be a target coordinate
        fixed_t temp_y = 0;

        boolean should_shift = false; // Shifting in the feed

        // Internal timer for easing animations
        tic_t shifttimer = 0;

        Hudfeed* feed = nullptr; // Reference to the feed
        tic_t fadetimer = 0; // For entering and exiting the feed

        // a timer won't work for these, so use static ints
        int entering_trans = -1;
        int exiting_trans = -1;

        tic_t displaytimer = 0; // For how long the update should be in the feed for
        updatestate_t state = UPDATE_NONE; // State of update
        transnum_t updatetrans = static_cast<transnum_t>(0); // Translucency
        fixed_t temp_shift_y = 0; // Temporary variable to store the y-shift from the feed config

        // Is the update relating to the main player (stplyr)?
        boolean is_about_self = false;
    
        void reset_fade() {
            updatetrans = static_cast<transnum_t>(0);
        }

        UINT8* get_rainbow_colormap() {
            skincolornum_t rainbow = static_cast<skincolornum_t>(K_RainbowColor(I_GetTime()/2));
            return static_cast<UINT8*>(
                R_GetTranslationColormap(TC_DEFAULT, rainbow, GTC_CACHE)
            );
        }

        void set_timer() {
            shifttimer = I_GetTime();
        }

        INT32 clamp_translucency(INT32 trans = -1) {
            if (trans == -1)
                return updatetrans;
            return (updatetrans < trans) ? trans : updatetrans;
        }

    public:
        // copying EggTVGraphics.hpp here, ty Jartha xo
        struct PatchManager
        {
            using patch = const char*;

            struct 
            {
                patch start =       "RRFDBEG";
                patch mid =         "RRFDMID";
                patch end =         "RRFDEND";
            } 
            background;
        };

        static const PatchManager patches_;

        BaseFeedUpdate() {};

        // Draw background
        void draw_background(FeedConfig &c, float width) {
            constexpr int backgroundW = 4;
            const boolean reverse = (c.position == FEED_POSITION_RIGHT);
            const float x_offset = reverse ? 4.f : -2.f;

            INT32 translucency_level = (is_about_self) ? 6 : 8;

            INT32 feed_flags = c.hudflags;

            // Background is drawn at 60% translucency
            // Shouldn't be any higher than that
            if (is_entering() || is_exiting()) {                
                translucency_level = clamp_translucency(translucency_level);
            }

            // Start drawing
            feed_flags |= (translucency_level << V_ALPHASHIFT);
            Draw background = Draw(FixedToFloat(get_x()) + x_offset, FixedToFloat(y) - 2.f).flags(feed_flags);

            if (reverse) {
                background = background.x(-backgroundW);
                background.patch(patches_.background.end);
                background = background.x(-width);
                background.width(width).stretch(Draw::Stretch::kWidth).patch(patches_.background.mid);
                background.x(-backgroundW).patch(patches_.background.start);
            } else {
                background.patch(patches_.background.start);
                background = background.x(backgroundW);
                background.width(width).stretch(Draw::Stretch::kWidth).patch(patches_.background.mid);
                background.x(width).patch(patches_.background.end);
            }
        }

        // == Read-only
        fixed_t get_x() { 
            return (x_overwrite != -1) ? x_overwrite : x; 
        }
        fixed_t get_y() { return y; }
        fixed_t get_temp_y() { return temp_y; }
        fixed_t get_target_y() { return target_y; }
        Hudfeed& get_feed () { return *feed; }
        boolean has_x_been_overwritten() { return x_overwrite != -1; }
        boolean is_done() {
            return state == UPDATE_EXITED;
        }
        boolean can_draw() {
            // Can the update be drawn?
            if (is_entering()) {return (entering_trans <= FEED_UPDATE_FADE_IN_START-1);}
            if (is_exiting()) {return exiting_trans >= FEED_UPDATE_FADE_OUT_START; }

            return state != UPDATE_NONE && !is_done();
        }        
        tic_t get_timer() { return shifttimer; }
        boolean is_exiting() {return state == UPDATE_EXITING;}
        boolean is_entering() {return state == UPDATE_ENTERING;}
        boolean is_waiting_to_enter() {return state == UPDATE_WAITING_TO_ENTER;}
        boolean is_shifting() { return should_shift; }
        fixed_t get_temp_shift_y() {return temp_shift_y;}

        // == Write-methods

        // Switch feed states
        void switch_state(updatestate_t new_state) { state = new_state; } 

        // Assign a feed to the update
        void join_feed(Hudfeed* newFeed) { feed = newFeed;}

        // Set X,Y
        void set_temp_y() { temp_y = y; }
        void set_target_y(fixed_t ty) { target_y = ty; }
        void set_x(fixed_t new_x) { x = new_x; }
        void overwrite_x(fixed_t new_x) { x_overwrite = new_x; }
        void set_y(fixed_t new_y) { y = new_y; }

        // Set temp y-coordinate for shifting
        void set_temp_shift_y(fixed_t shift_y) { 
            set_timer();
            temp_y = y;
            temp_shift_y = shift_y; 
        }

        // Set fade level
        void set_fade_for_enter() { updatetrans = static_cast<transnum_t>(entering_trans); }
        void set_fade_for_exit() { updatetrans = static_cast<transnum_t>(exiting_trans); }

        // Start moving in the feed
        void shift() {
            set_timer();
            should_shift = true;
        }

        // Enter the feed (fade-in)
        void enter() {
            // //CONS_Printf("entering\n");

            fadetimer = FEED_UPDATE_FADE_TIME;
            entering_trans = FEED_UPDATE_FADE_IN_START;
            switch_state(UPDATE_ENTERING);

            reset_fade();

            // Entering the feed, needs to shift
            shift();
        }

        // Exit the feed (fade-out)
        void exit() {
            // //CONS_Printf("exiting\n");
            fadetimer = FEED_UPDATE_FADE_TIME;
            exiting_trans = FEED_UPDATE_FADE_OUT_START;
            switch_state(UPDATE_EXITING);

            reset_fade();
        }

        // Decrement the timer depending on the state
        void tick() {
            switch (state) 
            {
                case UPDATE_NONE:
                case UPDATE_EXITED:
                case UPDATE_WAITING_TO_ENTER:
                    // By default (or when the update is done), don't do anything
                    break;
                case UPDATE_ENTERING:
                    set_fade_for_enter();
                    entering_trans--;
                    if (entering_trans < 1) {
                        // Faded-in, change state;
                        displaytimer = FEED_UPDATE_DISPLAY_TIME;
                        switch_state(UPDATE_DISPLAYING);
                    }
                    break;
                case UPDATE_EXITING:
                    // Fade-in or fade-out
                    set_fade_for_exit();
                    exiting_trans++;
                    if (exiting_trans >= 9) {
                        // Faded out, change state
                        //CONS_Printf("update done\n");
                        switch_state(UPDATE_EXITED);
                    }
                    break;
                case UPDATE_DISPLAYING:
                    displaytimer--;
                    if (displaytimer <= 0) {
                        // Done displaying, fade out
                        exit();
                    }
                    break;
            }

            if (should_shift) {
                // Moving up/down in the feed (based on config)
                tic_t duration = M_DueFrac(get_timer(), FEED_UPDATE_SHIFT_TIME);
                fixed_t new_y = Easing_OutQuart(duration, get_temp_y(), get_target_y());
                set_y(new_y);

                if (get_y() == get_target_y()) {
                    //CONS_Printf("done shifting\n");
                    should_shift = false;
                }
            }
        }

        // Draw method (needs to be overriden)
        // https://www.w3schools.com/cpp/cpp_virtual_functions.asp
        virtual void draw() { return; }
};

/**
 * The "feed"
 * 
 * Will be updated as certain events occur in-game, which are: 
 * 1. When someone gets hit by the player's item
 * 2. When someone hits the player with their item
 * 3. "Global" events
 */
class Hudfeed {
    protected:
        FeedConfig feed_config;
        // Still don't quite get this, but since polymorphism is being used, need to use this
        // https://en.cppreference.com/w/cpp/memory/unique_ptr.html

        std::deque<std::unique_ptr<BaseFeedUpdate>> log; // The actual feed
        std::deque<std::unique_ptr<BaseFeedUpdate>> backlog; // Updates waiting to enter the feed

        fixed_t get_target_y_from_config(fixed_t base_y, fixed_t shift_y) {
            return get_config().direction == FEED_DIRECTION_DOWN ? 
                    base_y + shift_y :
                    base_y - shift_y;
        }

    public:
        Hudfeed(const FeedConfig &c): feed_config(c) {}

        const FeedConfig& get_config() { return feed_config; }
        void update_config(FeedConfig new_config) { feed_config = new_config; } 

        std::deque<std::unique_ptr<BaseFeedUpdate>>& get_feed() { return log; }
        boolean is_feed_full() { return log.size() == static_cast<size_t>(feed_config.maxupdates + 1); }
        boolean is_feed_empty() { return log.empty(); }

        void push(std::unique_ptr<BaseFeedUpdate> update) {
            // Assign the feed to the update and push it to the log
            update->join_feed(this);

            // Initializing the y coordinate, which should be
            // just below (or above) the config's starting y coord

            fixed_t update_y = ((get_config().start_y - get_config().shift_y) << FRACBITS);
            fixed_t update_shift_y = (get_config().shift_y << FRACBITS); 

            update->set_x(get_config().start_x << FRACBITS);
            update->set_y(update_y);
            update->set_temp_shift_y(update_shift_y);
            update->set_target_y(get_target_y_from_config(update_y, update_shift_y));
            update->switch_state(UPDATE_WAITING_TO_ENTER);

            // If the log isn't empty, check for a couple of scenarios
            if (!log.empty()) {
                // Is the feed full?
                if (is_feed_full()) {
                    //CONS_Printf("feed full\n");
                    backlog.push_back(std::move(update));
                    return;
                }

                // Is the update at the back of the feed still "entering"?
                if (log.back()->is_shifting()) {
                    //CONS_Printf("yeah\n");
                    backlog.push_back(std::move(update));
                    return;
                }
                
                // Otherwise, start shifting the feed
                log.push_back(std::move(update));
                shift_feed();
                //CONS_Printf("pushing\n");
            } else {
                // Immediately enter the feed
                log.push_back(std::move(update));
                log.front()->enter();
            }
        }

        // Clear the feed
        void clear() {
            if (is_feed_empty()) { return; }

            log.clear();
            backlog.clear();
        }

        // Tell every update in the feed to start shifting
        void shift_feed() {
            if (is_feed_empty()) { return; }

            // Have reached max capacity for the feed
            // Immediately make the oldest update exit
            if (is_feed_full()) {
                //CONS_Printf("mass capacity");
                log.front()->exit();
            }

            for (auto &update : log) {
                // Addressing updates pushed from the backlog
                if (update->is_waiting_to_enter()) {
                    //CONS_Printf("let me in\n");
                    update->enter();
                } else {
                    // Set current y-coordinate (for interp)
                    update->set_temp_y();
                    
                    // Set target y-coordinate (for interp)
                    fixed_t ty = get_target_y_from_config(update->get_temp_y(), update->get_temp_shift_y());
                    update->set_target_y(ty);

                    // Then shift
                    update->shift();
                }
            }
        }

        // Control each update's internal timer
        void tick_feed() {
            if (is_feed_empty()) { return; }

            // Check the update waiting to enter the feed
            if (!backlog.empty()) {
                if(!log.back()->is_shifting()) {
                    log.push_back(std::move(backlog.front()));
                    backlog.pop_front();
                    
                    // Shift the feed
                    shift_feed();
                    return;
                }
            }

            // Check if the oldest update in the feed is "done"
            if (log.front()->is_done()) {
                // If so, pop
                log.pop_front();
                //CONS_Printf("popped\n");
            }

            // Increment each update's timer
            for (auto &update : log) {
                update->tick();
            }
        }

        void animate_feed() {
            if (is_feed_empty()) { return; }

            // Animate the feed by updating all the .. updates
            for (auto &update : log) {
                update->draw();
            }
        }
};

#endif // __RR_FEED_HPP__