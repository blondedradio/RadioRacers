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
#include "../rr_setup.h"
#include "../rr_controller.h"
#include "../../p_local.h"
#include "../../hu_stuff.h"
#include "../../g_game.h"
#include "../../s_sound.h"

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

void RR_PlayCountdownJingle(INT16 timer, player_t *player) {
    if (!cv_powersound.value)
        return;

    if (stplyr != player)
        return;

    if (timer > 0 && timer <= 3 * TICRATE)
    {
        if (timer % TICRATE == 0)
        {
            S_StartSound(NULL, sfx_s242);
            
            // (skypegiggle)
            if (found_radioracers && cv_powersoundjoke.value && radio_last_powerup_jingle_sound != sfx_None && timer == TICRATE) {
                S_StartSoundAtVolume(NULL, radio_last_powerup_jingle_sound, 255/4);
            }
        }
    }
}
	
static boolean isRing(mobj_t* mo)
{
    return (mo->type == MT_RING || mo->type == MT_FLINGRING);
}
static boolean isRingBox(mobj_t* mo)
{
    statenum_t specialstate = mo->state - states;
    return (mo->type == MT_RANDOMITEM) && (specialstate >= S_RINGBOX1 && specialstate <= S_RINGBOX12);
}

static boolean canGhost(void)
{
    return cv_accessibility_rings_hide.value && r_splitscreen == 0;
}

boolean RR_ShouldGhostRing(mobj_t *mo)
{
    return canGhost() &&
    isRing(mo) && 
    !(!P_MobjWasRemoved(mo->target) && mo->target->type == MT_PLAYER) && 
    (IS_BEING_CHASED_BY_SPB(stplyr) || RINGTOTAL(stplyr) >= 20);
}

boolean RR_ShouldGhostRingboxes(mobj_t *mo)
{
    return canGhost() &&
    isRingBox(mo) && 
    (IS_BEING_CHASED_BY_SPB(stplyr));
}

boolean RR_ShouldGhostItemCapsuleParts(mobj_t *mo)
{
    return canGhost() &&
    (
        (mo->type == MT_ITEMCAPSULE_PART && (mo->sprite == SPR_ITEM && mo->frame & KITEM_SUPERRING)) ||
        (mo->type == MT_ITEMCAPSULE)
    ) &&
    IS_BEING_CHASED_BY_SPB(stplyr);
}

boolean RR_ShouldGhostItemCapsuleNumbers(mobj_t *mo)
{
    // isSuperRingItemNumber is only set to true in p_mobj.c in P_RefreshItemCapsuleParts
    return canGhost() && 
    mo->isSuperRingItemNumber &&
    IS_BEING_CHASED_BY_SPB(stplyr);
}

int scaleInt(int value, fixed_t scale)
{
    return value - (value - (int)(round(value * FixedToFloat(scale))));
}