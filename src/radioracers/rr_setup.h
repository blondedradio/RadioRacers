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
extern boolean radioracers_useendkey;
extern boolean radioracers_usealternatetripwire;

/**
 * Textures in this game are stored by ID, not name, which is sensible.
 * To avoid having to look up the ID for custom textures, find it ONCE during texture setup, grab the ID and assign it.
 */

// Base texture IDs
extern INT32 RADIO_BADWIRE_TEX_ID;
extern INT32 RADIO_BADWIRE_4X_TEX_ID;
extern INT32 RADIO_BADWIRE_VERTICAL_TEX_ID;
extern INT32 RADIO_GOODWIRE_TEX_ID;
extern INT32 RADIO_GOODWIRE_4X_TEX_ID;
extern INT32 RADIO_GOODWIRE_VERTICAL_TEX_ID;

// Texture name
extern const char* RADIO_BADWIRE_TEX_NAME;
extern const char* RADIO_BADWIRE_4X_TEX_NAME;
extern const char* RADIO_BADWIRE_VERTICAL_TEX_NAME;
extern const char* RADIO_GOODWIRE_TEX_NAME;
extern const char* RADIO_GOODWIRE_4X_TEX_NAME;
extern const char* RADIO_GOODWIRE_VERTICAL_TEX_NAME;

extern patch_t *end_key[2];
extern sfxenum_t radio_ding_sound;
extern sfxenum_t radio_last_powerup_jingle_sound;
extern void RR_Init(void);
extern void RR_AddAllEmotes(UINT16 wadnum);
extern void RR_CleanupEmoteFrames(void);
extern void RR_SaveEmoteUsage(void);
extern void RR_SaveFavouriteEmotes(void);
extern void RR_UpdateEmoteUsageVector(void);
extern void RR_FavouriteEmote(char* name);
extern void RR_UnfavouriteEmote(char* name);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
