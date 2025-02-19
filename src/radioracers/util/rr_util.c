// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/util/rr_util.c
/// \brief Util functions

#include "../../doomstat.h" // encoremode, localencore
#include "../rr_util.h" // encoremode, localencore
#include "../rr_cvar.h"
#include "../rr_controller.h"
#include "../../p_local.h"
#include "../../hu_stuff.h"
#include "../../g_game.h"

boolean shouldApplyEncore(void)
{
    // Check the encoremode flag first, THEN the clientside flag.
    return (encoremode || localencore) || hakimode;
}

boolean shouldUseHaki(void)
{
    // Prefer to use a function rather then just checking for 'hakimode' everywhere.
    return hakimode;
}

void RR_HandleBlueSphereRumble(player_t *player)
{
    if (!cv_rr_rumble_spheres.value)
        return;

    if (P_IsMachineLocalPlayer(player)) {
        localPlayerPickupSpheresDelay = 0; // reset the delay everytime a player picks up a sphere

        if (localPlayerPickupSpheresDelay < 4)
            localPlayerPickupSpheres++;
    }
}

static const char* BATTLE_WIN_MESSAGES [] = {
    "\x82%s\\FUCKING WINS!", // Points
    "\x82%s\\GOT ALL OF THE FUCKING EMERALDS!", // Emeralds  
};

/**
 * Just do a cecho!
 */
void RR_AnnounceBattleWinner(player_t *player, battle_win_type_t type)
{
    if (!cv_battle_toggle_winner_announcement.value)
        return;

    HU_SetCEchoDuration(4);
    HU_DoCEcho(va(M_GetText(BATTLE_WIN_MESSAGES[type]), player_names[player-players]));
}

int scaleInt(int value, fixed_t scale)
{
    return value - (value - (int)(round(value * FixedToFloat(scale))));
}