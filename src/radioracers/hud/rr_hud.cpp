// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/menu/rr_hud.cpp

#include <math.h>
#include <deque>

#include "../../doomstat.h" // r_splitscreen
#include "../../doomdef.h" // SKINCOLOR_CHAOSEMERALD*
#include "../rr_hud.h"
#include "../rr_setup.h"
#include "../rr_cvar.h"
#include "../../k_hud.h" // trackingResult_t
#include "../../p_local.h" // player, stplyr, P_MobjFlip()
#include "../../m_fixed.h" // FixedToFloat(), FixedMul(), FixedDiv()
#include "../../console.h"
#include "../../d_player.h"
#include "../../screen.h" // BASEVIDHEIGHT, BASEVIDWIDTH
#include "../../v_video.h" // V_* flags and V_Draw* functions
#include "../../r_draw.h" // TC_DEFAULT, GTC_CACHE
#include "../../k_battle.h" // K_NumEmeralds
#include "../../k_color.h" // K_RainbowColor
#include "../../z_zone.h" // Z_Realloc
#include "../../i_time.h"
#include "../../d_netcmd.h"

#include "../../v_draw.hpp" // srb2:Draw

#define LAPS_X 9				
#define LAPS_Y (BASEVIDHEIGHT - 29)

int chat_log_offset[CHAT_BUFSIZE];
int chat_mini_log_offset[8];

/**
 * Battle HUD
 */

static void RR_drawCompactEmeraldHud(INT32 flags)
{
    static patch_t *kp_rankemerald = static_cast<patch_t*>(W_CachePatchName("K_EMERC", GTC_CACHE));
    static patch_t *kp_rankemeraldflash = static_cast<patch_t*>(W_CachePatchName("K_EMERW", GTC_CACHE));

    INT32 badge_y = (LAPS_Y-6);
    INT32 startx = (LAPS_X + 8);
    INT32 starty = badge_y-2;

    INT32 i = 0;
    INT32 emeraldGap = 0;
    INT32 emeraldGapAdd = 6;

    // Firstly, draw the (compact) sticker/badge
    K_DrawSticker(LAPS_X + 8, badge_y, 45, flags, true);

    // Secondly, loop over all the potential emeralds a player can get (i.e. seven.)
    for (i = 0; i < 7; i++)
	{
		UINT32 emeraldFlag = (1 << i);
		skincolornum_t emeraldColor = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + i);

        // Does the player have THIS specific emerald?
		if (stplyr->emeralds & emeraldFlag)
		{
			V_DrawMappedPatch(
				startx + emeraldGap, starty,
				V_HUDTRANS|flags,
				kp_rankemerald, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
			);

			if (leveltime & 1) {
				V_DrawMappedPatch(
					startx + emeraldGap, starty,
					V_HUDTRANS|V_ADD|flags,
					kp_rankemeraldflash, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
				);
			}
		} else {
            // If they don't have it, draw a placeholder.
			V_DrawMappedPatch(
				startx + emeraldGap, starty,
				V_HUDTRANS|flags,
				kp_rankemeraldflash, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
			);
		}
		emeraldGap += emeraldGapAdd;
	}
}

/**
 * Draw an expanded version of the emerald display HUD.
 */
static void RR_drawEmeraldHudFull(INT32 flags)
{
    static patch_t *kp_chaosemerald = static_cast<patch_t*>(W_CachePatchName("EMRCA0", GTC_CACHE));
    static patch_t *kp_chaosemeraldoverlay = static_cast<patch_t*>(W_CachePatchName("EMRCB0", GTC_CACHE));

    INT32 startx = LAPS_X + 19;
    INT32 starty = BASEVIDHEIGHT - 29;

    INT32 i = 0;
    INT32 emeraldGap = 0;
    INT32 emeraldGapAdd = 10;
    
    // Firstly, draw the sticker/badge
    using srb2::Draw;
	Draw(LAPS_X+12, starty-14).flags(flags).align(Draw::Align::kCenter).width(75).sticker();

    // Secondly, loop over all the potential emeralds a player can get (i.e. seven.)
    for (i = 0; i < 7; i++)
	{
		UINT32 emeraldFlag = (1 << i);
		skincolornum_t emeraldColor = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + i);

        // Does the player have THIS specific emerald?
		if (stplyr->emeralds & emeraldFlag)
		{
			V_DrawTinyMappedPatch(
				startx + emeraldGap, starty,
				V_HUDTRANS|flags,
				kp_chaosemerald, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
			);

			if (leveltime & 1) {
				V_DrawTinyMappedPatch(
					startx + emeraldGap, starty,
					V_HUDTRANS|V_ADD|flags,
					kp_chaosemeraldoverlay, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
				);
			}
		} else {
            // If they don't have it, draw a placeholder.
            V_DrawTinyMappedPatch(
                startx + emeraldGap, starty,
                V_HUDTRANS|flags,
                kp_chaosemeraldoverlay, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
            );
		}
		emeraldGap += emeraldGapAdd;
	}
}

/**
 * Draw a minimal version of the emerald display HUD, akin to the bumpers and score graphic.
 */
static void RR_drawEmeraldHudMinimal(INT32 flags)
{
    const uint8_t numEmeralds = K_NumEmeralds(stplyr);
    const boolean isAboutToWin = numEmeralds >= 6;
    static patch_t *kp_chaosemerald = static_cast<patch_t*>(W_CachePatchName("EMRCA0", GTC_CACHE));

    INT32 startx = LAPS_X + 19;
    INT32 starty = BASEVIDHEIGHT - 29;
    
    // Firstly, draw the sticker/badge
    K_DrawSticker(LAPS_X+12, starty-14, 38, flags, false);
    using srb2::Draw;
    Draw row = Draw(LAPS_X+36, starty-16)
        .flags(flags)
        .font(Draw::Font::kThinTimer);

    if (isAboutToWin && leveltime % 8 < 4)
    {
        row = row.colorize(SKINCOLOR_TANGERINE);
    }
    row.text("{:02}", numEmeralds);
    
    // Second, draw the emerald alongside the player's total count
    skincolornum_t emeraldColor2 = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1);
    V_DrawTinyMappedPatch(
        startx+5, starty-2,
        V_HUDTRANS|flags,
        kp_chaosemerald, R_GetTranslationColormap(TC_DEFAULT, emeraldColor2, GTC_CACHE)
    );

    if (isAboutToWin && leveltime & 1) {
        V_DrawTinyMappedPatch(
            startx+5, starty-2,
            V_HUDTRANS|V_ADD|flags,
            kp_chaosemerald, R_GetTranslationColormap(TC_DEFAULT, emeraldColor2, GTC_CACHE)
        );
    }
    
}

static void RR_drawEmeraldHud(INT32 flags)
{
    static patch_t *kp_chaosemerald = static_cast<patch_t*>(W_CachePatchName("EMRCA0", GTC_CACHE));
    static patch_t *kp_chaosemeraldoverlay = static_cast<patch_t*>(W_CachePatchName("EMRCB0", GTC_CACHE));

    INT32 startx = LAPS_X + 19;
    INT32 starty = BASEVIDHEIGHT - 29;

    INT32 i = 0;
    INT32 emeraldGap = 0;
    INT32 emeraldGapAdd = 10;


    const uint8_t numEmeralds = K_NumEmeralds(stplyr);
    INT32 sticker_width = 20 + (5 * numEmeralds);
    
    // Firstly, draw the sticker/badge
    K_DrawSticker(LAPS_X+12, starty-14, sticker_width, flags, false);
    using srb2::Draw;
	// Draw(LAPS_X+12, starty-14).flags(flags).align(Draw::Align::kCenter).width(20).sticker(); // 75
    Draw row = Draw(LAPS_X+14, starty-16).flags(flags).font(Draw::Font::kThinTimer);
    row.text("{:02}", numEmeralds);

    // Secondly, loop over all the potential emeralds a player can get (i.e. seven.)
    for (i = 0; i < 7; i++)
	{
		UINT32 emeraldFlag = (1 << i);
		skincolornum_t emeraldColor = static_cast<skincolornum_t>(SKINCOLOR_CHAOSEMERALD1 + i);

        // Does the player have THIS specific emerald?
		if (stplyr->emeralds & emeraldFlag)
		{
			V_DrawTinyMappedPatch(
				startx + emeraldGap, starty,
				V_HUDTRANS|flags,
				kp_chaosemerald, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
			);

			if (leveltime & 1) {
				V_DrawTinyMappedPatch(
					startx + emeraldGap, starty,
					V_HUDTRANS|V_ADD|flags,
					kp_chaosemeraldoverlay, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
				);
			}
		} else {
            // If they don't have it, draw a placeholder.
            if (i == numEmeralds+1) {
                V_DrawTinyMappedPatch(
                    startx + emeraldGap, starty,
                    V_HUDTRANS|flags,
                    kp_chaosemeraldoverlay, R_GetTranslationColormap(TC_DEFAULT, emeraldColor, GTC_CACHE)
                );
            }
		}
		emeraldGap += emeraldGapAdd;
	}
}

// Just two layouts for now
typedef enum {
    MINIMAL = 0,
    FULL,
    MAX_EMERALD_LAYOUTS
} CustomEmeraldDrawType;

static void (*drawEmeraldLayout[MAX_EMERALD_LAYOUTS])(INT32) = {RR_drawEmeraldHudMinimal, RR_drawEmeraldHudFull};
extern void RR_drawKartEmeralds(void)
{
    INT32 splitflags = V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTOLEFT;
    const boolean DRAW_SPHERES_ON_PLAYER = cv_spheremeteronplayer.value == 1;

    /**
     * If the player wants the blue sphere meter drawn on top of them,
     * there's a huge gap on the bottom-left of the HUD.
     * 
     * If so, draw the emeralds there.
     * If not, still draw the emeralds there, BUT, draw it slightly smaller.
     */

    if (DRAW_SPHERES_ON_PLAYER) {
        drawEmeraldLayout[cv_customemeraldhud.value - 1](splitflags);
    } else {
        RR_drawCompactEmeraldHud(splitflags);
    }

}

/**
 * Chat HUD
 */

/** Do Radio-related functions for the chatbox, in here. */
void RR_DoChatStuff(chat_box_parameters_t parameters) {
    INT32 boxw = cv_chatwidth.value;
    INT16 chatx = parameters.x, y = parameters.y;
    INT16 typelines = parameters.typelines;
    CONST INT32 charheight = parameters.charheight;

    // Is the player trying to quick-select an emote?
    if (is_emote_preview_on) {
		RR_DrawChatEmotePreview(chatx, (y-1) + (typelines*charheight), boxw);
	}

    // Is the player trying to select an emote from the menu?
	if (is_emote_menu_on) {
		RR_DrawChatEmoteMenu(chatx + boxw + 4, (y-1) + (typelines*charheight));
	} else {
        if(cv_chat_emotes.value && cv_chat_emotes_button.value) {
            const INT32 endkey_x = chatx + boxw + 4;
            const INT32 endkey_y = (y-10) + (typelines*charheight);

            if (radioracers_useendkey) {
                const UINT8 anim_duration = 32; // K_drawButtonAnim
                const boolean push = ((I_GetTime() % (anim_duration * 2)) < anim_duration);
                const int btn_index = push ? 1 : 0;
    
                V_DrawStretchyFixedPatch(
                    (endkey_x) << FRACBITS,
                    (endkey_y) << FRACBITS,
                    FloatToFixed(.7f),
                    FloatToFixed(.7f),
                    V_SNAPTOBOTTOM | V_SNAPTOLEFT,
                    end_key[btn_index],
                    NULL
                );
            } else {
                const fixed_t scaled_w = V_StringScaledWidth(
                    FloatToFixed(.7f), FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM | V_SNAPTOLEFT, HU_FONT, "END"
                );
                V_DrawFill(endkey_x-1, endkey_y, (scaled_w/FRACUNIT) + 2, 9, 16 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);
                V_DrawStringScaled(
                    (endkey_x) << FRACBITS, 
                    (endkey_y + 2) << FRACBITS,
                    FloatToFixed(.7f), 
                    FRACUNIT,
                    FRACUNIT,
                    V_SNAPTOBOTTOM | V_SNAPTOLEFT,
                    NULL,
                    HU_FONT,
                    "END"
                );
            }
        }
    }
}

/**
 * Race HUD
 */

struct PlayerFinishTicker {
    std::string position;
    boolean is_local_player;
    int x;
};

static std::deque<PlayerFinishTicker> playerFinishTickerQueue;
static boolean drawLapFlagAtStart = false;

#define offscreen_offset() \
    ((vid.width/vid.dupx) - BASEVIDWIDTH)/ 2
#define offscreen_right_offset() \
    BASEVIDWIDTH + (offscreen_offset()) + 2

void RR_addPlayerToFinshTicker(player_t *player)
{    
    // https://www.mathsisfun.com/numbers/cardinal-ordinal-chart.html
    auto position_string = [](UINT8 position) -> std::string {
        if (position % 10 == 1 && position % 100 != 11)
            return std::to_string(position) + "st";
        if (position % 10 == 2 && position % 100 != 12)
            return std::to_string(position) + "nd";
        if (position % 10 == 3 && position % 100 != 13)
            return std::to_string(position) + "rd";
        
        return std::to_string(position) + "th";
    };

    const char* player_name = player_names[player-players];
    if (IsPlayerMuted(player-players))
        player_name = "???";
    
    /**
     * TODO: 
     *  * Handle player ties
     *  * Rare spectate case (just check player flags)
     */
    playerFinishTickerQueue.push_back(
        {
            M_GetText(va("%s \x86%s", position_string(player->position).c_str(), player_name)),
            P_IsMachineLocalPlayer(player),
            offscreen_right_offset() // Start just off-screen to the right
        }
    );
}

void RR_ridersFinishTick(void)
{
    if (playerFinishTickerQueue.empty()) return;
    
    // Move over to the left
    playerFinishTickerQueue.front().x -= 2;

    auto string_width = [](std::string string) -> INT32 
    {
        return V_ThinStringWidth(string.c_str(), 0);
    };

    /**
     * Once the player at the front of the queue passes
     * the middle of the screen, start drawing the next player who finished.
     */
    for (size_t i = 1; i < playerFinishTickerQueue.size(); ++i) {
        const INT32 padding = string_width(playerFinishTickerQueue[i-1].position) + 25;

        if (playerFinishTickerQueue[i].x - playerFinishTickerQueue[i-1].x >= padding)
            playerFinishTickerQueue[i].x -= 2;
    }

    const INT32 OFFSCREEN_X = 0 - offscreen_offset() - string_width(
        playerFinishTickerQueue.front().position.c_str()
    );

    // Once the player at the front of the queue is offscreen to the left, pop them
    if (playerFinishTickerQueue.front().x <= OFFSCREEN_X) {
        playerFinishTickerQueue.pop_front();

        if (drawLapFlagAtStart)
            drawLapFlagAtStart = false;
    }
}

static const int FINISH_TICKER_Y = 45;
/**
 * Draw the finish line ticker, like in Sonic Riders.
 * You know...
 */
void RR_drawRidersFinishTicker(void)
{
    if (playerFinishTickerQueue.empty()) return;

    patch_t *lapFlag = static_cast<patch_t*>(W_CachePatchName("K_SPTLAP", GTC_CACHE));

    for (size_t i = 0; i < playerFinishTickerQueue.size(); ++i) {
        PlayerFinishTicker &player = playerFinishTickerQueue[i];

        INT32 flags = V_20TRANS;

        if (player.is_local_player)
        {
            flags = V_YELLOWMAP|V_10TRANS;
        }

        if (i == 0 && drawLapFlagAtStart)
        {
            V_DrawMappedPatch(
                player.x - 15,
                FINISH_TICKER_Y,
                0,
                lapFlag,
                NULL
            );
        }
        
        V_DrawThinString(player.x, FINISH_TICKER_Y, flags, player.position.c_str());
    }
}

/** 
 * Draw mini laps
 */
void RR_DrawKartLapsMini(void)
{
    if (r_splitscreen)
        return;
    
    const INT32 MINI_LAPS_Y = LAPS_Y + 5;
    const INT32 splitflags = V_HUDTRANS|V_SLIDEIN|V_SNAPTOBOTTOM|V_SNAPTOLEFT;
    using srb2::Draw;

    Draw(LAPS_X + 7, MINI_LAPS_Y)
        .flags(splitflags)
        .align(Draw::Align::kCenter)
        .width(40)
        .small_sticker();

    V_DrawScaledPatch(
        LAPS_X + 8, 
        MINI_LAPS_Y - 2, 
        splitflags, 
        static_cast<patch_t *>(W_CachePatchName("K_SPTLAP", PU_HUDGFX))
    );
        
    V_DrawThinTimerString(
        LAPS_X+24, 
        MINI_LAPS_Y - 4, 
        splitflags, 
        va("%d/%d", std::min(stplyr->laps, numlaps), numlaps)
    );
}
/**
 * Empty the queue!
 */
void RR_resetRidersFinishTicker(void)
{
    playerFinishTickerQueue.clear();
    drawLapFlagAtStart = true;
}

/**
 * Emote stuff
 */

lumpnum_t getEmoteAtlasFrame(int atlas_id) {
    return EMOTE_ATLASES[atlas_id]->atlas_lump;
}

emote_atlas_coordinates_t getEmoteAtlasCoordinates(emote_t* emote, float scale)
{
    const int atlas_id = emote->atlas_id;
    INT32 height = EMOTE_ATLASES[atlas_id]->height;
    INT32 width = EMOTE_ATLASES[atlas_id]->width;
    
    return {
        FloatToFixed(scale * ((emote->atlas_column) * width)),
        FloatToFixed(scale * ((emote->atlas_row) * height))
    };
}

lumpnum_t getEmoteFrame(emote_t* emote) {
    if (emote->atlas_id != -1) {
        return getEmoteAtlasFrame(emote->atlas_id);
    }
    size_t& frame = emoteFrameMap[emote];
    tic_t& lastUpdate = emoteLastUpdate[emote];

    if (paused)
        return emote->frames[frame];

    if ((INT32)(leveltime - lastUpdate) >= emote->frame_delay) {
        frame = (frame + 1) % emote->frame_count;
        lastUpdate = leveltime;
    }

    return emote->frames[frame];
}

lumpnum_t getChatEmoteFrame(emote_t* emote) {
    if (emote->atlas_id != -1) {
        return getEmoteAtlasFrame(emote->atlas_id);
    }
    size_t& frame = chatEmoteFrameMap[emote];
    tic_t& lastUpdate = chatEmoteLastUpdate[emote];

    if (!cv_chat_emotes_animated.value)
        return emote->frames[0];
    
    if (paused)
        return emote->frames[frame];

    if ((INT32)(gametic - lastUpdate) >= emote->frame_delay) {
        frame = (frame + 1) % emote->frame_count;
        lastUpdate = gametic;
    }

    return emote->frames[frame];
}