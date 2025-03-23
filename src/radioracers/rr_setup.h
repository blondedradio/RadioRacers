// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_setup.h

#ifndef __RR_SETUP__
#define __RR_SETUP__

#ifdef __cplusplus
extern "C" {
#endif

extern boolean found_radioracers;
extern boolean found_radioracers_plus;
extern boolean radioracers_usemuteicons;
extern boolean radioracers_usehakiencore;
extern sfxenum_t radio_ding_sound;
extern void RR_Init(void);
extern void RR_AddAllEmotes(UINT16 wadnum);
extern void RR_CleanupEmoteFrames(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
