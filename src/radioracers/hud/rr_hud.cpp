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
#include "../rr_cvar.h"
#include "../../k_roulette.h" // ROULETTE_SPACING, SLOT_SPACING
#include "../../k_hud.h" // trackingResult_t
#include "../../p_local.h" // player, stplyr, P_MobjFlip()
#include "../../m_fixed.h" // FixedToFloat(), FixedMul(), FixedDiv()
#include "../../r_fps.h" // R_InterpolateFixed()
#include "../../console.h"
#include "../../d_player.h"
#include "../../screen.h" // BASEVIDHEIGHT, BASEVIDWIDTH
#include "../../v_video.h" // V_* flags and V_Draw* functions
#include "../../r_draw.h" // TC_DEFAULT, GTC_CACHE
#include "../../k_battle.h" // K_NumEmeralds
#include "../../k_color.h" // K_RainbowColor
#include "../../z_zone.h" // Z_Realloc

#include "../../v_draw.hpp" // srb2:Draw

#define ITEM_BOX_WIDTH 46 // 50 - 4
#define ITEM_BOX_HEIGHT 50

#define RING_BOX_WIDTH 55 // 56 - 1
#define RING_BOX_HEIGHT 48

#define LAPS_X 9				
#define LAPS_Y (BASEVIDHEIGHT - 29)

int chat_log_offset[CHAT_BUFSIZE];
int chat_mini_log_offset[8];

typedef struct 
{
    fixed_t scale; // HUD scale for either Ring Box cvar or Item Roulette cvar
    float scale_f; // HUD scale (float)

    int GRAPHIC_WIDTH; // Width of item/ringbox graphic
    int GRAPHIC_HEIGHT; // Height of item/ringbox graphic 

    itemboxposition_e position; // Preferred position of item/ringbox graphic (cvar)
} roulette_type_info_t;


// ================ HELPER FUNCTIONS ===================
static trackingResult_t _getBaseRouletteCoordinatesForTrackingPlayer(void)
{
	trackingResult_t result;

	// No player object? Not bothering.
	const boolean doesPlayerHaveMo = !((stplyr->mo == NULL || P_MobjWasRemoved(stplyr->mo)));
	if (!doesPlayerHaveMo)
		return result;

    // Functionality copied from k_hud_track.cpp
	vector3_t v = {
		R_InterpolateFixed(stplyr->mo->old_x, stplyr->mo->x) + stplyr->mo->sprxoff,
		R_InterpolateFixed(stplyr->mo->old_y, stplyr->mo->y) + stplyr->mo->spryoff,
		R_InterpolateFixed(stplyr->mo->old_z, stplyr->mo->z) + stplyr->mo->sprzoff + 
        (stplyr->mo->height >> 1),
	};

	vector3_t vertical_offset_vector = {
		0, 
		0, 
		64 * stplyr->mo->scale * P_MobjFlip(stplyr->mo)
	};

	FV3_Add(&v, &vertical_offset_vector);
	
	K_ObjectTracking(&result, &v, false);

	return result;
}

static itembox_tracking_coordinates_t _getBaseRouletteCoordinates(int width, int height)
{
    // Initialize the coordinates..
    itembox_tracking_coordinates_t coords = { .x=0, .y=0 };

    // Firstly, get the tracking coordinates
    const trackingResult_t _tracking_coords = _getBaseRouletteCoordinatesForTrackingPlayer();

    // If we're out of range, no point in drawing anything, set valid_coords to false and return
    if(_tracking_coords.x == 0 && _tracking_coords.y == 0)
    {
        coords.valid_coords = false;
        return coords;
    }

    // Otherwise, let's start calculating.

    // Set some base variables to work off of.
    int16_t x = (_tracking_coords.x / FRACUNIT);
    int16_t y = (_tracking_coords.y / FRACUNIT);

    /**
     * Firstly, we need to center the graphic so that it's smack in the middle of the player.
     * Then, from there, we have three directions to choose from. LEFT, UP, RIGHT.
     * 
     * This is where the position CVARs will come into play.
     */
    x -= ((width) / 2);
    y += ((height) / 2);
    
    coords.x = x;
    coords.y = y;
    coords.valid_coords = true;
    
    return coords;
}

static int _clampRouletteCoordinates(fixed_t clamp_scale, int base)
{
    int NEW_BASE = (int)(base * FixedToFloat(clamp_scale));
    return (NEW_BASE);
}

static vector2_t _getRouletteCropping(vector2_t rouletteCrop, float hud_scale)
{
    const int base_x = rouletteCrop.x;
    const int base_y = rouletteCrop.y;
    const float base_x_scale_f = round(base_x * hud_scale);
    const float base_y_scale_f = round(base_y * hud_scale);


    /**
     * 100%
     *  7 - (7 - (7*1.0))
     *  7 - (7 - 7)
     *  7 - (0)
     *  7
     * 
     * 90%
     *  7 - (7 - (7*0.9))
     *  7 - (7 - 6)
     *  7 - (1)
     *  6
     * 
     * etc...
     */
    rouletteCrop.x = base_x - (base_x - (int)(base_x_scale_f));
    rouletteCrop.y = base_y - (base_y - (int)(base_y_scale_f));

    return rouletteCrop;
}

static roulette_offset_spacing_t _getRouletteSpacingOffset(fixed_t space, fixed_t offset, float hud_scale_float)
{
    const int base_roulette_spacing = (space) >> FRACBITS;
    
    const int new_roulette_spacing = ((int)((base_roulette_spacing * hud_scale_float)) << FRACBITS);
    const int new_roulette_offset = FixedMul(offset, FixedDiv(new_roulette_spacing, space));

    const roulette_offset_spacing_t roulette_offset_spacing = {
        .space=new_roulette_spacing, 
        .offset=new_roulette_offset
    };
    return roulette_offset_spacing;
}

static itembox_tracking_coordinates_t _getRouletteCoordinates(roulette_type_info_t info)
{
    const float _scale = info.scale_f;
    const fixed_t _HUD_SCALE = info.scale;
    const int _GRAPHIC_WIDTH = info.GRAPHIC_WIDTH;
    const int _GRAPHIC_HEIGHT = info.GRAPHIC_HEIGHT;

    const float width_f = round(_GRAPHIC_WIDTH * _scale);
    const int width = (int) (width_f);
    const int height = (int) (_GRAPHIC_HEIGHT - (int)(_GRAPHIC_HEIGHT * _scale));

    itembox_tracking_coordinates_t _base_coords = _getBaseRouletteCoordinates(width, height);
    const itemboxposition_e _position = info.position;

    if(!_base_coords.valid_coords)
        return _base_coords;

    // Used for calculating LEFT and RIGHT coordinates.
    const int BASE_WIDTH_OFFSET = _GRAPHIC_WIDTH;
    const int BASE_ITEM_BOX_WIDTH = (int)(BASE_WIDTH_OFFSET * _scale);

    // Used for calculating ABOVE coordinates.
    const int BASE_HEIGHT_OFFSET = _GRAPHIC_HEIGHT;
    const int BASE_ITEM_BOX_HEIGHT = (int)(BASE_HEIGHT_OFFSET * _scale);

    /**
     * Assume X = 0
     * So, what these calculations are doing are how much to subtract X by depending on the HUD scale.
     * 
     * For example, if drawing the graphics at 100% scale:
     * Subtract X by the width of the graphic. 
     * 
     * Drawing graphics at 100% scale:
     * X = X - (100% of the graphic width)
     * 
     * 90% scale:
     * X = X - (100% of the graphic width - 90% of the graphic width).
     * 
     * 80% scale:
     * X = X - (100% of the graphic width - 80% of the graphic width).
     * 
     * And so on. The idea behind this is that the bigger the scale, the further out the item roulette is to the side.
     * And as that scale decreases, it gets closer to the player, so it's easier to keep a track of.
     * 
     * However, past a certain scale, it can get a bit too close to the player and cause clipping.
     * So, there's a conditional that at 60% scale and below, draw the roulette graphic in the same
     * position where it'd be drawn at 60%. This prevents the clipping. 
     * 
     * I think it's called 'clamping'?
     * 
     * Same logic applies to ABOVE positioning, just happening vertically.
     */
    const fixed_t MAX_SCALE_X = (3*FRACUNIT)/5;  // 60%
    int x_offset = BASE_WIDTH_OFFSET - (BASE_WIDTH_OFFSET - BASE_ITEM_BOX_WIDTH);
    if (_HUD_SCALE <= MAX_SCALE_X) {
        x_offset = _clampRouletteCoordinates(MAX_SCALE_X, BASE_WIDTH_OFFSET);
    }

    // ABOVE
    const fixed_t MAX_SCALE_Y = (3*FRACUNIT)/5;  // 60%
    int y_offset = BASE_HEIGHT_OFFSET - (BASE_HEIGHT_OFFSET - BASE_ITEM_BOX_HEIGHT);
    if (_HUD_SCALE <= MAX_SCALE_Y) {
        y_offset = _clampRouletteCoordinates(MAX_SCALE_Y, BASE_HEIGHT_OFFSET);
    }

    const boolean UPSIDE_DOWN = (stplyr->mo->eflags & MFE_VERTICALFLIP);

    switch(_position)
    {
        case LEFT: // To the left of the player.
            _base_coords.x -= x_offset;
            break;
        case ABOVE: // Directly above the player.
            _base_coords.y = (UPSIDE_DOWN) ? 
                    _base_coords.y + y_offset : 
                    _base_coords.y - y_offset;
            break;
        case RIGHT: // To the right of the player.
            _base_coords.x += x_offset;
            break;
    }
    return _base_coords;
}
// ================ HELPER FUNCTIONS ===================

// ================ CVARS ===================
fixed_t RR_getItemBoxHudScale(void)
{
    return cv_item_roulette_player_scale.value;
}

fixed_t RR_getRingBoxHudScale(void)
{
    return cv_ringbox_roulette_player_scale.value;
}

static itemboxposition_e _getItemBoxHudPosition(void)
{
    return static_cast<itemboxposition_e>(cv_item_roulette_player_position.value);
}

static itemboxposition_e _getRingBoxHudPosition(void)
{
    return static_cast<itemboxposition_e>(cv_ringbox_roulette_player_position.value);
}

float RR_getItemBoxHudScaleFloat(void)
{
    return FixedToFloat(RR_getItemBoxHudScale());
}

float RR_getRingBoxHudScaleFloat(void)
{
    return FixedToFloat(RR_getRingBoxHudScale());
}
// ================ CVARS ===================


// ================ MAIN ===================
itembox_tracking_coordinates_t RR_getRouletteCoordinatesForKartItem(void)
{
    const roulette_type_info_t info = {
        .scale = RR_getItemBoxHudScale(),
        .scale_f = RR_getItemBoxHudScaleFloat(),
        .GRAPHIC_WIDTH = ITEM_BOX_WIDTH,
        .GRAPHIC_HEIGHT = ITEM_BOX_HEIGHT,
        .position = _getItemBoxHudPosition()
    };

    return _getRouletteCoordinates(info);
}

itembox_tracking_coordinates_t RR_getRouletteCoordinatesForRingBox(void)
{
    const roulette_type_info_t info = {
        .scale = RR_getRingBoxHudScale(),
        .scale_f = RR_getRingBoxHudScaleFloat(),
        .GRAPHIC_WIDTH = RING_BOX_WIDTH,
        .GRAPHIC_HEIGHT = RING_BOX_HEIGHT,
        .position = _getRingBoxHudPosition()
    };

    return _getRouletteCoordinates(info);
}

vector2_t RR_getRouletteCroppingForKartItem(vector2_t rouletteCrop)
{
    // Usually is {7, 7}
    return _getRouletteCropping(rouletteCrop, RR_getItemBoxHudScaleFloat());
}

vector2_t RR_getRouletteCroppingForRingBox(vector2_t rouletteCrop)
{
    // Usually is {10, 10}
    return _getRouletteCropping(rouletteCrop, RR_getRingBoxHudScaleFloat());
}

roulette_offset_spacing_t RR_getRouletteSpacingOffsetForKartItem(fixed_t offset)
{
    return _getRouletteSpacingOffset(
        ROULETTE_SPACING, // Usually is 36 << FRACBITS
        offset,
        RR_getItemBoxHudScaleFloat()
    );
}

roulette_offset_spacing_t RR_getRouletteSpacingOffsetForRingBox(fixed_t offset)
{
    return _getRouletteSpacingOffset(
        SLOT_SPACING, // Usually is 40 << FRACBITS
        offset,
        RR_getRingBoxHudScaleFloat()
    );
}

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
        V_DrawStretchyFixedPatch(
            (chatx + boxw + 4) << FRACBITS,
            ((y-10) + (typelines*charheight)) << FRACBITS,
            FRACUNIT/3,
            FRACUNIT/3,
            V_SNAPTOBOTTOM | V_SNAPTOLEFT,
            static_cast<patch_t*>(W_CachePatchName(
                "EMENUEND", PU_HUDGFX
            )),
            NULL
        );
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

    /**
     * TODO: 
     *  * Handle player ties
     *  * Rare spectate case (just check player flags)
     */
    playerFinishTickerQueue.push_back(
        {
            M_GetText(va("%s \x86%s", position_string(player->position).c_str(), player_names[player-players])),
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

    if (paused)
        return emote->frames[frame];

    if ((INT32)(gametic - lastUpdate) >= emote->frame_delay) {
        frame = (frame + 1) % emote->frame_count;
        lastUpdate = gametic;
    }

    return emote->frames[frame];
}