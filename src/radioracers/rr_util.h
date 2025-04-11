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

typedef struct
{
    UINT32 index;
    INT32 x;
    INT32 message_y;
    INT32 boxw;
    INT32 chat_topy;
    INT32 flags;
    INT32 chat_bottomy;
    fixed_t scale;

} chat_log_message_param_t;
extern void RR_DrawChatLogMessageNumbers(chat_log_message_param_t p);
#ifdef __cplusplus
} // extern "C"
#endif

#endif