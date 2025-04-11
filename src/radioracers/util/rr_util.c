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
#include "../../v_video.h"

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
 * Just do a cecho
 */
void RR_AnnounceBattleWinner(player_t *player, battle_win_type_t type)
{
    if (!cv_battle_toggle_winner_announcement.value)
        return;

    HU_SetCEchoFlags(0);
    HU_SetCEchoDuration(4);

    // This used to crash in netgames but just to be safe..
    if (type < 0 || type >= sizeof(BATTLE_WIN_MESSAGES) / sizeof(BATTLE_WIN_MESSAGES[0])) {
        return;
    }

    // va causes crashes sometimes but ONLY semi-rarely, weird.
    HU_DoCEcho(va(BATTLE_WIN_MESSAGES[type], player_names[player-players]));    
}

/** Draw the index of each message in the chat log when viewing the chat log */
void RR_DrawChatLogMessageNumbers(chat_log_message_param_t p)
{
    const char* chat_log_index_str = va("%d", p.index);
    const fixed_t index_x = ((p.x - 4) << FRACBITS) -  V_StringScaledWidth(p.scale, FRACUNIT, FRACUNIT, 0, HU_FONT, chat_log_index_str);

    cliprect_t clip;
    V_SaveClipRect(&clip);
    
    V_SetClipRect(
        index_x, (p.chat_topy) << FRACBITS,
        index_x + ((p.boxw) << FRACBITS), (p.chat_bottomy) <<FRACBITS,
        V_SNAPTOBOTTOM|V_SNAPTOLEFT
    );

    V_DrawStringScaled(
        index_x,
        (p.message_y) << FRACBITS,
        p.scale, FRACUNIT, FRACUNIT,
        p.flags | V_TRANSLUCENT,
        NULL,
        HU_FONT,
        chat_log_index_str
    );

    V_RestoreClipRect(&clip);
}

int scaleInt(int value, fixed_t scale)
{
    return value - (value - (int)(round(value * FixedToFloat(scale))));
}