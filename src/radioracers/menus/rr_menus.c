// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/menu/rr_menus_hud.c

#include "../rr_menu.h"
#include "../rr_cvar.h"
#include "../rr_setup.h"

#include "../../d_main.h"
#include "../../v_video.h"

// HUD Options - Race
static menuitem_t OPTIONS_RadioRacersHudRace[] =
{
	{IT_STRING | IT_CVAR, "Ring Counter Position", "Toggle the RING COUNTER's HUD position.",
		NULL, {.cvar = &cv_ringsonplayer}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Hide HUD Elements", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Countdown", "Hide the countdown graphics at the beginning of a race.",
		NULL, {.cvar = &cv_hud_hidecountdown}, 0, 0},   
            
	{IT_STRING | IT_CVAR, "POSITION!!!", "Hide the POSITION!!! graphics at the beginning of a race.",
		NULL, {.cvar = &cv_hud_hideposition}, 0, 0},

	{IT_STRING | IT_CVAR, "Lap Emblem", "Hide the Lap Emblem when you begin a new lap.",
		NULL, {.cvar = &cv_hud_hidelapemblem}, 0, 0}
};

static menu_t OPTIONS_RadioRacersHudRaceDef = 
{
	sizeof (OPTIONS_RadioRacersHudRace) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersHudDef,
	0,
	OPTIONS_RadioRacersHudRace,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

// HUD Options - Battle
static menuitem_t OPTIONS_RadioRacersHudBattle[] =
{
	{IT_STRING | IT_CVAR, "Announce Winner", "Show the winner at the end of a Battle round.",
		NULL, {.cvar = &cv_battle_toggle_winner_announcement}, 0, 0},

	{IT_STRING | IT_CVAR, "Sphere Gauge Position", "Toggle the Sphere Gauge's HUD position.",
		NULL, {.cvar = &cv_spheremeteronplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "Emerald HUD", "Toggle an alternate take on the Emerald display in the HUD.",
		NULL, {.cvar = &cv_customemeraldhud}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Toggle HUD Elements", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Emeralds", "Show emerald positions in the minimap?",
		NULL, {.cvar = &cv_battle_toggle_emerald_on_minimap}, 0, 0},

	{IT_STRING | IT_CVAR, "Combat UFO Timer", "Show where and how long until the next Combat UFO spawns?",
		NULL, {.cvar = &cv_battle_toggle_ufo_timer_on_minimap}, 0, 0},

	{IT_STRING | IT_CVAR, "Track Players", "Display TARGET markers on the HUD to track players?",
		NULL, {.cvar = &cv_targetrackplayers}, 0, 0},
};


static menu_t OPTIONS_RadioRacersHudBattleDef =
{
	sizeof (OPTIONS_RadioRacersHudBattle) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersHudDef,
	0,
	OPTIONS_RadioRacersHudBattle,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL,
};

// HUD
menuitem_t OPTIONS_RadioRacersHud[] =
{	
	{IT_STRING | IT_SUBMENU, "Race..", "Extended HUD options for Races.",
		NULL, {.submenu = &OPTIONS_RadioRacersHudRaceDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Battle..", "Extended HUD options for Battle Mode.",
		NULL, {.submenu = &OPTIONS_RadioRacersHudBattleDef}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "General Options", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Hold Rankings Button", "Press and hold the rankings button to view the rankings, just like in SRB2Kart.",
		NULL, {.cvar = &cv_holdbuttonforscoreboard}, 0, 0},
	
	{IT_STRING | IT_CVAR, "Use Higher Resolution Portraits", "Draw higher resolution portraits in the minirankings.",
		NULL, {.cvar = &cv_hud_usehighresportraits}, 0, 0},

	{IT_HEADER, "Roulette Options", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Roulette Layout", "Change the HUD layout for drawing the item/ring roluette.",
		NULL, {.cvar = &cv_rouletteonplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "Ring Roulette Scale", "Choose a scale to draw the RING ROULETTE at.",
		NULL, {.cvar = &cv_ringbox_roulette_player_scale}, 0, 0},

	{IT_STRING | IT_CVAR, "Ring Roulette Position", "Choose where the RING ROULETTE should be positioned.",
		NULL, {.cvar = &cv_ringbox_roulette_player_position}, 0, 0},

	{IT_STRING | IT_CVAR, "Item Roulette Scale", "Choose a scale to draw the ITEM ROULETTE at.",
		NULL, {.cvar = &cv_item_roulette_player_scale}, 0, 0},

	{IT_STRING | IT_CVAR, "Item Roulette Position", "Choose where the ITEM ROULETTE should be positioned.",
		NULL, {.cvar = &cv_item_roulette_player_position}, 0, 0},
};

menu_t OPTIONS_RadioRacersHudDef = {
	sizeof (OPTIONS_RadioRacersHud) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersMenuDef,
	0,
	OPTIONS_RadioRacersHud,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	Roulette_OnChange,
	NULL,
	NULL,
};

// Gameplay
static menuitem_t OPTIONS_RadioRacersGameplay[] =
{			
	{IT_STRING | IT_CVAR, "Extended Controller Rumbles", "Toggle the extended controller rumble events.",
		NULL, {.cvar = &cv_morerumbleevents}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Rings", "Toggle controller rumble when you pickup and use rings.",
		NULL, {.cvar = &cv_rr_rumble_rings}, 0, 0},

	{IT_STRING | IT_CVAR, "Spheres", "Toggle controller rumble when you pickup any blue spheres.",
		NULL, {.cvar = &cv_rr_rumble_spheres}, 0, 0},

	{IT_STRING | IT_CVAR, "Drift", "Toggle controller rumble when a new drift spark starts.",
		NULL, {.cvar = &cv_rr_rumble_drift}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Spindash", "Toggle controller rumble when spindashing.",
		NULL, {.cvar = &cv_rr_rumble_spindash}, 0, 0},

	{IT_STRING | IT_CVAR, "Wall Bump", "Toggle controller rumble when you bump into a wall.",
		NULL, {.cvar = &cv_rr_rumble_wall_bump}, 0, 0},

	{IT_STRING | IT_CVAR, "Fastfall Bounce", "Toggle controller rumble when you bounce after a fastfall.",
		NULL, {.cvar = &cv_rr_rumble_fastfall_bounce}, 0, 0},

	{IT_STRING | IT_CVAR, "Tailwhip", "Toggle controller rumble when you charge a tailwhip.",
		NULL, {.cvar = &cv_rr_rumble_tailwhip}, 0, 0},

	{IT_STRING | IT_CVAR, "Wavedash", "Toggle controller rumble when your wavedash boost starts.",
		NULL, {.cvar = &cv_rr_rumble_wavedash}, 0, 0},
};

static menu_t OPTIONS_RadioRacersGameplayDef = {
	sizeof (OPTIONS_RadioRacersGameplay) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersMenuDef,
	0,
	OPTIONS_RadioRacersGameplay,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	RumbleEvents_OnChange,
	NULL,
	NULL,
};

// Fun features
static menuitem_t OPTIONS_RadioRacersFun[] =
{
	{IT_STRING | IT_CVAR, "Enable Encore Palettes", "Toggle encore palettes clientside for levels, if available.",
		NULL, {.cvar = &cv_applylocalencore}, 0, 0},

	{IT_STRING | IT_CVAR, "Observation Haki", "Apply a grayscale filter to the level, keeping important elements in colour.",
		NULL, {.cvar = &cv_applyhaki}, 0, 0},

	{IT_STRING | IT_CVAR, "Riders Finish Line Ticker", "Show a finish line ticker, like in Sonic Riders!",
		NULL, {.cvar = &cv_show_riders_finish_ticker}, 0, 0},
};

void RadioFunMenu_Init(void)
{
	if (!radioracers_usehakiencore) {
		OPTIONS_RadioRacersFun[1].status = IT_GRAYEDOUT;	
	}
}

static menu_t OPTIONS_RadioRacersFunDef = {
	sizeof (OPTIONS_RadioRacersFun) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersMenuDef,
	0,
	OPTIONS_RadioRacersFun,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	RadioFunMenu_Init,
	NULL,
	NULL,
};

// Main
menuitem_t OPTIONS_RadioRacersMenu[] =
{
	{IT_STRING | IT_SUBMENU, "HUD..", "Extended options for the HUD.",
		NULL, {.submenu = &OPTIONS_RadioRacersHudDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "Gameplay..", "Gameplay-enhancing options.",
		NULL, {.submenu = &OPTIONS_RadioRacersGameplayDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "\\(^_^)/", "Fun stuff!",
		NULL, {.submenu = &OPTIONS_RadioRacersFunDef}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},
		
	{IT_HEADER, "Chat", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Emotes", "Show the funny emotes in the chatbox.",
		NULL, {.cvar = &cv_chat_emotes}, 0, 0},

	{IT_STRING | IT_CVAR, "Animated Emotes", "Toggle animated emotes.",
		NULL, {.cvar = &cv_chat_emotes_animated}, 0, 0},

	{IT_STRING | IT_CVAR, "Menu Button", "Toggle the END key on the side of the chatbox.",
		NULL, {.cvar = &cv_chat_emotes_button}, 0, 0},
	
	{IT_HEADER, "Netplay", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Vote Snitch", "Show players who initiated and cast votes in the chatbox.",
		NULL, {.cvar = &cv_votesnitch}, 0, 0},
};

void RumbleEvents_OnChange(void)
{
	if (con_startup) return;

	UINT16 newstatus = (cv_morerumbleevents.value) ? IT_STRING | IT_CVAR : IT_GRAYEDOUT;

	for (int i = 2; i < 10; i++) {
		OPTIONS_RadioRacersGameplay[i].status = newstatus;
	}

	if (!cv_morerumbleevents.value)
	{
		if (localPlayerWavedashClickTimer > 0)
			localPlayerWavedashClickTimer = 0;

		if (localPlayerJustBootyBounced)
			localPlayerJustBootyBounced = false;

		if (localPlayerPickupSpheresDelay >= 4)
			localPlayerPickupSpheresDelay = 0;

		if (localPlayerPickupSpheres > 0)
			localPlayerPickupSpheres = 0;
	}
}

void Roulette_OnChange(void)
{
	if (con_startup) return;

	UINT16 newstatus = (cv_rouletteonplayer.value) ? IT_STRING | IT_CVAR : IT_GRAYEDOUT;

	for (int i = 8; i < 12; i++) {
		OPTIONS_RadioRacersHud[i].status = newstatus;
	}
}

menu_t OPTIONS_RadioRacersMenuDef = {
	sizeof (OPTIONS_RadioRacersMenu) / sizeof (menuitem_t),
	&OPTIONS_MainDef,
	0,
	OPTIONS_RadioRacersMenu,
	48, 80,
	SKINCOLOR_BANANA, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	NULL,
	NULL,
	NULL
};