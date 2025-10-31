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
#include "../rr_hud.h"

#include "../../d_main.h"
#include "../../v_video.h"
#include "../../k_kart.h"
#include "../../r_main.h"

// HUD Options - Race
static menuitem_t OPTIONS_RadioRacersHudRace[] =
{
	{IT_HEADER, "Toggle HUD Elements", NULL,
		NULL, {NULL}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Nametags", "Toggle in-game nametags.",
		NULL, {.cvar = &cv_toggle_nametags}, 0, 0},   
		
	{IT_STRING | IT_CVAR, "EXP on Split", "Show/hide EXP gained during checkpoint splits.",
		NULL, {.cvar = &cv_showexponsplit}, 0, 0},   
		
	{IT_STRING | IT_CVAR, "Position Number", "Toggle position number on the bottom-right.",
		NULL, {.cvar = &cv_toggle_position_number}, 0, 0},   

	{IT_STRING | IT_CVAR, "Minimap", "Hide the minimap.",
		NULL, {.cvar = &cv_toggle_race_minimap}, 0, 0},   

	{IT_STRING | IT_CVAR, "Player Standings", "Show/hide the player standings.",
		NULL, {.cvar = &cv_toggle_race_standings}, 0, 0},   

	{IT_STRING | IT_CVAR, "Trick Text", "Show/hide the 'COOL' text when performing a trick.",
		NULL, {.cvar = &cv_toggle_trick_cool}, 0, 0},   

	{IT_STRING | IT_CVAR, "Countdown", "Show/hide the countdown graphics at the beginning of a race.",
		NULL, {.cvar = &cv_hud_hidecountdown}, 0, 0},   
            
	{IT_STRING | IT_CVAR, "POSITION!!!", "Show/hide the POSITION!!! graphics at the beginning of a race.",
		NULL, {.cvar = &cv_hud_hideposition}, 0, 0},

	{IT_STRING | IT_CVAR, "Lap Emblem", "Show/hide the Lap Emblem when you begin a new lap.",
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

// HUD Options - Hudfeed
static menuitem_t OPTIONS_RadioRacersHudfeed[] = 
{
	{IT_STRING | IT_CVAR, "Enabled?", "Show the feed.",
		NULL, {.cvar = &cv_hudfeed_enabled}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Options", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Position", "Feed position on HUD. (Changing this will clear the feed.)",
		NULL, {.cvar = &cv_hudfeed_position}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Faults?", "Show player faults in the feed?",
		NULL, {.cvar = &cv_hudfeed_show_faults}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Grades?", "Show player's grades/ranks in the feed?",
		NULL, {.cvar = &cv_hudfeed_show_grades}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Snipes?", "Show any snipes (SEGA!) in the feed?",
		NULL, {.cvar = &cv_hudfeed_show_snipes}, 0, 0},

	{IT_STRING | IT_CVAR, "Show Amps?", "Show amps gained in the feed?",
		NULL, {.cvar = &cv_hudfeed_show_amps}, 0, 0}
};

void RadioHudfeedMenu_Init(void)
{
	if (!found_radioracers || !radioracers_usehudfeed) {
		OPTIONS_RadioRacersHudfeed[3].status = IT_GRAYEDOUT;
		OPTIONS_RadioRacersHudfeed[4].status = IT_GRAYEDOUT;
		OPTIONS_RadioRacersHudfeed[5].status = IT_GRAYEDOUT;
		OPTIONS_RadioRacersHudfeed[6].status = IT_GRAYEDOUT;
		OPTIONS_RadioRacersHudfeed[7].status = IT_GRAYEDOUT;
	}
}

static menu_t OPTIONS_RadioRacersHudfeedDef =
{
	sizeof (OPTIONS_RadioRacersHudfeed) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersHudDef,
	0,
	OPTIONS_RadioRacersHudfeed,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	RadioHudfeedMenu_Init,
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

	{IT_STRING | IT_SUBMENU, "Hudfeed..", "An in-game feed of player events during races.",
		NULL, {.submenu = &OPTIONS_RadioRacersHudfeedDef}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Ring Counter Options", NULL,
		NULL, {NULL}, 0, 0},
		
	{IT_STRING | IT_CVAR, "Ring Counter Position", "Toggle the RING COUNTER's HUD position.",
		NULL, {.cvar = &cv_ringsonplayer}, 0, 0},
			
	{IT_STRING | IT_CVAR, "Speedometer Position", "Toggle the SPEEDOMETER's HUD position.",
		NULL, {.cvar = &cv_speedometeronplayer}, 0, 0},

	{IT_STRING | IT_CVAR, "EXP Display Position", "Toggle the EXP HUD position.",
		NULL, {.cvar = &cv_exponplayer}, 0, 0},

	{IT_HEADER, "General Options", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Input Display Type", "Toggle between the DIGITAL and ANALOG controller types in the input display.",
		NULL, {.cvar = &cv_inputdisplaytoggle}, 0, 0},

	{IT_STRING | IT_CVAR, "Input Display Size", "Toggle the size of the input display size",
		NULL, {.cvar = &cv_inputdisplaytogglesize}, 0, 0},

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

// Accessibility
static menuitem_t OPTIONS_RadioRacersAccessibility[] =
{
	{IT_STRING | IT_CVAR, "Observation Haki", "Apply a grayscale filter to the level, keeping important elements in colour.",
		NULL, {.cvar = &cv_applyhaki}, 0, 0},

	{IT_STRING | IT_CVAR, "Obvious Voltage", "Re-colour your voltage aura depending on your driftcharge.",
		NULL, {.cvar = &cv_obvious_voltage}, 0, 0},

	{IT_STRING | IT_CVAR, "Precise Countdown", "Show a more precise countdown, accompanied with a little bar.",
		NULL, {.cvar = &cv_precise_countdown}, 0, 0},

	{IT_STRING | IT_CVAR, "Ghost Rings", "Ghost rings and ringboxes when you're unable to collect any rings.",
		NULL, {.cvar = &cv_accessibility_rings_hide}, 0, 0},

	{IT_STRING | IT_CVAR, "Dangerous Player Checks", "Draw warning symbols to the side of the HUD for any incoming danger.",
		NULL, {.cvar = &cv_show_dangerous_player_check}, 0, 0},

	{IT_STRING | IT_CVAR, "Obvious Tripwires", "Draw color-coded tripwires to indicate whether you can pass through or not.",
		NULL, {.cvar = &cv_obvious_tripwire}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},

	{IT_HEADER, "Item Timers", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Enabled?", "Show item timers in the middle-bottom of your HUD.",
		NULL, {.cvar = &cv_gingeritemtimers}, 0, 0},

	{IT_STRING | IT_CVAR, "Font Size", "Switch the font size for the timer text.",
		NULL, {.cvar = &cv_gingeritemtimersbiggertext}, 0, 0},

	{IT_STRING | IT_CVAR, "Vertical Offset", "Vertically offset the timers from their default position.",
		NULL, {.cvar = &cv_gingeritemtimersoffset}, 0, 0},
	
	{IT_HEADER, "Powerups", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Powerup Countdown Jingle", "Play an audible countdown when your invincibility/grow powerup is about to end.",
		NULL, {.cvar = &cv_powersound}, 0, 0},

	{IT_STRING | IT_CVAR, "Extra Countdown Jingle", "Play an extra sound on the last second the coutdown jingle.",
		NULL, {.cvar = &cv_powersoundjoke}, 0, 0},
};

void RadioAccessibilityMenu_Init(void)
{
	if (!found_radioracers || radio_last_powerup_jingle_sound == sfx_None) {
		OPTIONS_RadioRacersAccessibility[13].status = IT_GRAYEDOUT;	
	}

	// Can't use these options, they rely on custom graphics
	// Haki mode and voltage
	if (!found_radioracers) {
		OPTIONS_RadioRacersAccessibility[0].status = IT_GRAYEDOUT;
		OPTIONS_RadioRacersAccessibility[1].status = IT_GRAYEDOUT;
	}
}

static menu_t OPTIONS_RadioRacersAccessibilityDef = {
	sizeof (OPTIONS_RadioRacersAccessibility) / sizeof (menuitem_t),
	&OPTIONS_RadioRacersMenuDef,
	0,
	OPTIONS_RadioRacersAccessibility,
	48, 80,
	SKINCOLOR_SUNSLAM, 0,
	MBF_DRAWBGWHILEPLAYING,
	NULL,
	2, 5,
	M_DrawGenericOptions,
	M_DrawOptionsCogs,
	M_OptionsTick,
	RadioAccessibilityMenu_Init,
	NULL,
	NULL,
};

// Gameplay
static menuitem_t OPTIONS_RadioRacersGameplay[] =
{			
	{IT_STRING | IT_CVAR, "Drift Spark Pulse Size", "Tweak the scale of the drift spark whilst drifting.",
		NULL, {.cvar = &cv_driftsparkrate_size}, 0, 0},

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
	{IT_STRING | IT_CVAR, "Enable Encore Palettes (Clientside)", "Toggle encore palettes clientside for levels, if available.",
		NULL, {.cvar = &cv_applylocalencore}, 0, 0},

	{IT_STRING | IT_CVAR, "Riders Finish Line Ticker", "Show a finish line ticker, like in Sonic Riders!",
		NULL, {.cvar = &cv_show_riders_finish_ticker}, 0, 0},

	{IT_STRING | IT_CVAR, "Show 'S' ranks", "Show S ranks in the player tally and standings (purely cosmetic).",
		NULL, {.cvar = &cv_show_s_ranks}, 0, 0},
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

	{IT_STRING | IT_SUBMENU, "Accessibility..", "Accessibility options.",
		NULL, {.submenu = &OPTIONS_RadioRacersAccessibilityDef}, 0, 0},

	{IT_STRING | IT_SUBMENU, "\\(^_^)/", "Fun stuff!",
		NULL, {.submenu = &OPTIONS_RadioRacersFunDef}, 0, 0},

	{IT_SPACE | IT_NOTHING, NULL,  NULL,
		NULL, {NULL}, 0, 0},
		
	{IT_HEADER, "Chat", NULL,
		NULL, {NULL}, 0, 0},

	{IT_STRING | IT_CVAR, "Emotes", "Show the funny emotes in the chatbox.",
		NULL, {.cvar = &cv_chat_emotes}, 0, 0},

	{IT_STRING | IT_CVAR, "In-Line Previews", "Toggle in-line emote previews.",
		NULL, {.cvar = &cv_chat_emotes_preview}, 0, 0},

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

	for (int i = 3; i < 11; i++) {
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

	for (int i = 15; i < 19; i++) {
		OPTIONS_RadioRacersHud[i].status = newstatus;
	}

	// Updating this cvar shifts the Hudfeed to the left
	if (cv_hudfeed_enabled.value && cv_hudfeed_position.value == 0) {
		RR_UpdateHudFeedConfig();
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