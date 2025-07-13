// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_item_timers.cpp
/// \brief Backporting a debugging script I wrote back in 2020

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "../rr_hud.h"
#include "../rr_cvar.h"
#include "../../doomstat.h"
#include "../../p_local.h"
#include "../../g_game.h"
#include "../../d_netcmd.h"
#include "../../k_color.h"
#include "../../k_powerup.h"
#include "../../v_draw.hpp"
#include "../../core/static_vec.hpp"

using srb2::Draw;

constexpr const int HARD_Y = 180;
constexpr const int HARD_HIGHER_Y = 160; // For SPB Attacks/Sealed Stars
constexpr const int HARD_X = 160;
constexpr const int SHIFT_X = 17; // To the left, to the left

const std::vector start_boost_patches = {"DBOSA5", "DBOSB5", "DBOSC5"};
const std::vector drift_patches = {"DRIFC3C7", "DRIFD3D7", "DRIFA3A7"};
const std::vector voltage_patches = {"TRC3B0", "TRC3C0", "TRC3D0", "TRC3E0", "TRC3F0"};

// STRUCTS

struct ItemTimer
{
    INT32 time; // Timer.. time
    std::string patch; // Item graphic
    std::optional<float> patch_scale; // Item scale
    std::pair<int, int> offsets; // Patch offset (x, y)
    std::optional<UINT16> colormap; // Colormap

    // Straight from Icon struct in powerup.cpp, thank you Jartha xoxoxo
    bool operator<(const ItemTimer& b) const { return time < b.time; }
    bool operator>(const ItemTimer& b) const { return time > b.time; }
};

// STRUCTS


static UINT16 getDriftSparkColor(void) {
    if (stplyr->driftboost > 125) { // Rainbow sparks
        return K_RainbowColor(leveltime);
    } else if (stplyr->driftboost > 85) { // Blue sparks
        return SKINCOLOR_BLUE;
    } else if (stplyr->driftboost > 50) { // Red sparks
        return SKINCOLOR_KETCHUP;
    } else if (stplyr-> driftboost > 20) { // Yellow sparks
        return SKINCOLOR_GOLD;
    }
    return SKINCOLOR_GREY; // Gray sparks
}

std::vector<ItemTimer> getTimers(void) {
    std::vector<ItemTimer> timers;

    // Invincibility
    if (!(gametyperules & GTR_POINTLIMIT)) {
        const std::string invincibility_patch = fmt::format("K_ISINV{0}", (((leveltime % (6*3)) / 3) + 1));
        timers.push_back({stplyr->invincibilitytimer, invincibility_patch});
    }

    // Grow
    if (stplyr->growshrinktimer > 0)
        timers.push_back({stplyr->growshrinktimer, "K_ISGROW"});

    // Shrink
    if (stplyr->growshrinktimer < 0) 
        timers.push_back({abs(stplyr->growshrinktimer), "K_ISSHRK"});

    // Ring boost
    timers.push_back({stplyr->ringboost, "K_ISRING"});
    
    // Boost
    timers.push_back({stplyr->sneakertimer, "K_ISSHOE"});

    // Hyuu
    timers.push_back({stplyr->hyudorotimer, "K_ISHYUD"});

    // Drift charge
    
    std::pair<int, int> drift_patches_offsets = {12, 17};
    
    int drift_index = (leveltime % drift_patches.size());
    float drift_patch_scale = 0.3;

    // If the player's drift charge is less than stage two, draw the first two patches only.
    // And draw at a slightly smaller scale.
    // Conveys less intensity.
    if (stplyr-> driftboost <= 20) {
        drift_index = ((leveltime % ((drift_patches.size()-1)*2)) / 2);
        drift_patch_scale = 0.2;
    }

    const std::string drift_patch = drift_patches[drift_index];

    timers.push_back({
        stplyr->driftboost, 
        drift_patch, 
        drift_patch_scale, 
        drift_patches_offsets,
        getDriftSparkColor()
    });

    // Start boost
    timers.push_back({
        stplyr->startboost, 
        start_boost_patches[leveltime % start_boost_patches.size()], 
        0.2, 
        {14, 17}, 
        K_RainbowColor(leveltime)
    });

    // Voltage timer (trick boost)
    timers.push_back({
        stplyr->trickcharge, 
        voltage_patches[leveltime % voltage_patches.size()],
        0.3,
        {14, 15}
    });

    // Wavedash
    timers.push_back({
        stplyr->wavedashboost,
        "SLPTHLHR",
        0.3,
        {14, 15}
    });

    // FAULT!
    if ((stplyr->pflags & PF_VOID || stplyr->pflags & PF_FAULT)) {
        if (stplyr->mo)
            timers.push_back({stplyr->mo->hitlag, "K_NOBLNS", {}, {7, 6}});
    }

    // Battle powerups
    if (gametyperules & GTR_POINTLIMIT) {
        for (int k = FIRSTPOWERUP; k < ENDOFPOWERUPS; ++k)
        {
            const kartitems_t powerup = static_cast<kartitems_t>(k);
            const int letter = 'A' + (powerup - FIRSTPOWERUP); 

            timers.push_back({
                static_cast<INT32>(K_PowerUpRemaining(stplyr, powerup)),
                fmt::format("PWRS{0:c}L{0:c}R", letter),
                {},
                {15, 18},
                stplyr->skincolor
            });
        }
    }

    // And, sort.
    std::stable_sort(timers.begin(), timers.end(), std::greater<ItemTimer>());

    return timers;
}

/**
 * Draw item timers for the stplyr (you) in the bottom-middle of the screen.
 */
void RR_DrawItemTimers(void)
{   
    /**
     * Not supporting splitscreen, never ever
     * Don't bother drawing these in a race.. space is occupied by the battle timers 
     */
    if (r_splitscreen > 0 || (P_MobjWasRemoved(stplyr->mo) || stplyr->mo == NULL || stplyr->exiting))
        return;

    std::vector<ItemTimer> timers = getTimers();

    INT32 start_x = HARD_X;
    INT32 x_offset = -SHIFT_X;
    for (const ItemTimer& t : timers) {
        if (t.time <= 0)
            continue;
        x_offset += SHIFT_X;
    }
    start_x -= (x_offset / 2);

    const INT32 draw_flags = V_SNAPTOBOTTOM;
    const boolean usingProgressionBar = (gametype == GT_SPECIAL || (modeattacking & ATTACKING_SPB));
    INT32 start_y = ((HARD_Y * FRACUNIT) + (cv_gingeritemtimersoffset.value)) >> FRACBITS;
    
    // Enforcing a higher position for the timers during attacking modes
    // So players don't have to keep hotswapping their timer positions between modes
    if (usingProgressionBar)
        start_y = HARD_HIGHER_Y;
    
    Draw timer_row = Draw(start_x, start_y)
        .font(Draw::Font::kThin)
        .align(Draw::Align::kCenter).flags(draw_flags);
    
    for (const ItemTimer& t : timers) {
        if (t.time <= 0) continue;

        const INT32 centiseconds = G_TicsToCentiseconds(t.time);
        const INT32 seconds = G_TicsToSeconds(t.time);
        INT32 extra_flags = draw_flags;

        // Slow fade out
        if (seconds == 0 && centiseconds <= 9) {                
            extra_flags |= (static_cast<transnum_t>(abs(centiseconds - 10)) << V_ALPHASHIFT);
        }

        // First the graphic
        int graphic_x = -15;
        int graphic_y = -10;

        // If a patch doesn't have offsets, the pair will default to {0, 0}
        graphic_x += t.offsets.first;
        graphic_y += t.offsets.second;

        timer_row
            .xy(graphic_x, graphic_y)
            .scale(t.patch_scale.has_value() ? t.patch_scale.value() : 0.6)
            .colormap(t.colormap.has_value()? t.colormap.value(): SKINCOLOR_NONE)
            .flags(extra_flags)
            .patch(t.patch);

        // Then the time
        const INT32 timer_text_y = (cv_gingeritemtimersbiggertext.value) ? 5 : 10;
        const float timer_text_scale = (cv_gingeritemtimersbiggertext.value) ? 0.9 : 0.7;
        timer_row
            .y(timer_text_y)
            .scale(timer_text_scale)
            .flags(extra_flags)
            .text("{}.{}", seconds, centiseconds);

        // And shift
        timer_row = timer_row.x(SHIFT_X);
    }
    
}