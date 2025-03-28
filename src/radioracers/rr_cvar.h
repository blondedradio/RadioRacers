// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_cvar.h
/// \brief RadioRacers CVARs

// Separating custom functionality into new header and source code files is so much cleaner.    

#ifndef __RR_CVAR__
#define __RR_CVAR__

// consvar_t
#include "../command.h"

#ifdef __cplusplus
extern "C" {
#endif

// Player (Clientside)
extern consvar_t cv_votesnitch;         // Vote Snitch
extern consvar_t cv_ringsonplayer;      // Rings drawn on player
extern consvar_t cv_rouletteonplayer;   // Item/Ring Roulette drawn on player
extern consvar_t cv_applylocalencore;    // Clientside encore palettes
extern consvar_t cv_applyhaki;           // Observation Haki mode

// Battle
extern consvar_t cv_customemeraldhud;    // Alternate Emerald display for Battle Mode
extern consvar_t cv_spheremeteronplayer; // Blue Sphere meter drawn on player

// Battle - HUD Tracking
extern consvar_t cv_targetrackplayers;  // Toggle the TARGET HUD graphics for other players

void KartLocalEncore_OnChange(void);
void KartHaki_OnChange(void);
void KartFinishLineTicker_OnChange(void);
void RR_ChatEmotes_OnChange(void);
/**
 * Checks if either the encoremode flag or the clientside flag is on
 * \sa cv_applylocalencore encoremode
 */
extern boolean shouldApplyEncore(void);
extern boolean shouldUseHaki(void);

// Extra customization
extern consvar_t cv_ringbox_roulette_player_scale;
extern consvar_t cv_ringbox_roulette_player_position;
extern consvar_t cv_item_roulette_player_scale;
extern consvar_t cv_item_roulette_player_position;

// Controller Rumble Toggles
extern consvar_t cv_morerumbleevents;           // Extra gameplay events considered for controller rumble
extern consvar_t cv_rr_rumble_wall_bump;        // Wall Bump
extern consvar_t cv_rr_rumble_fastfall_bounce;  // Fastfall Bounce
extern consvar_t cv_rr_rumble_drift;            // Drift
extern consvar_t cv_rr_rumble_spindash;         // Spindash
extern consvar_t cv_rr_rumble_tailwhip;         // Tailwhip
extern consvar_t cv_rr_rumble_rings;            // Rings
extern consvar_t cv_rr_rumble_spheres;          // Blue Spheres
extern consvar_t cv_rr_rumble_wavedash;         // Wavedash

// HUD
extern consvar_t cv_translucenthud;    // Self-explanatory
extern consvar_t cv_hud_hidecountdown; // Hide the bigass letters at the start of the race
extern consvar_t cv_hud_hideposition;  // Hide the bigass position bulbs at the start of the race
extern consvar_t cv_hud_hidelapemblem; // Hide the bigass lap emblem when you start a new lap
extern consvar_t cv_hud_usehighresportraits; // Draw higher-res portraits in the minirankings
extern consvar_t cv_holdbuttonforscoreboard; // Restore SRB2Kart behaviour when viewing in-game scoreboards
extern consvar_t cv_show_riders_finish_ticker; // Show the Sonic Riders :tm: finish line ticker
extern consvar_t cv_chat_emotes;                    // Self-explanatory
extern consvar_t cv_chat_emotes_animated;           // Should emotes animate?
extern consvar_t cv_chat_emotes_button;             // Toggle the END key button
extern consvar_t cv_chat_emotes_preview;            // Toggle emotes previewing in the chat input

// HUD -- Battle
extern consvar_t cv_battle_toggle_emerald_on_minimap; 
extern consvar_t cv_battle_toggle_ufo_timer_on_minimap;
extern consvar_t cv_battle_toggle_winner_announcement;

void RumbleEvents_OnChange(void);
void Roulette_OnChange(void);
#ifdef __cplusplus
} // extern "C"
#endif

#endif