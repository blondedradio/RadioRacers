// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_util.h
/// \brief Util functions


#ifndef __RR_UTIL__
#define __RR_UTIL__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BATTLE_WIN_POINTS = 0,
    BATTLE_WIN_EMERALDS
} battle_win_type_t;

extern int scaleInt(int value, fixed_t scale);
extern void RR_HandleBlueSphereRumble(player_t *player);
extern void RR_AnnounceBattleWinner(player_t *player, battle_win_type_t type);
extern void RR_PlayCountdownJingle(INT16 timer, player_t *player);
extern boolean RR_IsBattle(void);
extern boolean RR_ShouldGhostRing(mobj_t *mo);
extern boolean RR_ShouldGhostRingboxes(mobj_t *mo);
extern boolean RR_ShouldGhostItemCapsuleParts(mobj_t* mo);
extern boolean RR_ShouldGhostItemCapsuleNumbers(mobj_t *mo);
extern boolean RR_ShouldRecolorVoltage(mobj_t *mo); // Voltage
extern INT32 RR_FetchAlternateTripwire(INT32 original_textnum); // Tripwire
extern void RR_GetTrackingCoordinatesForPlayer(trackingResult_t *result, boolean playerHasMobj);

#define IS_BEING_CHASED_BY_SPB(p) (p->pflags & PF_RINGLOCK)

#ifdef __cplusplus
} // extern "C"
#endif

#endif