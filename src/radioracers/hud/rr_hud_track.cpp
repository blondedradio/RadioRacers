// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_hud_track.cpp

#include <math.h>
#include <fmt/format.h>

#include "../../doomstat.h" // r_splitscreen
#include "../../doomdef.h" // SKINCOLOR_CHAOSEMERALD*
#include "../rr_hud.h"
#include "../rr_setup.h"
#include "../rr_cvar.h"
#include "../../k_roulette.h" // ROULETTE_SPACING, SLOT_SPACING
#include "../../k_kart.h"
#include "../../k_podium.h"
#include "../../r_skins.h"
#include "../../r_main.h"
#include "../../k_hud.h" // trackingResult_t
#include "../../p_local.h" // player, stplyr, P_MobjFlip()
#include "../../m_fixed.h" // FixedToFloat(), FixedMul(), FixedDiv()
#include "../../r_fps.h" // R_InterpolateFixed()
#include "../../console.h"
#include "../../st_stuff.h"
#include "../../d_player.h"
#include "../../screen.h" // BASEVIDHEIGHT, BASEVIDWIDTH
#include "../../v_video.h" // V_* flags and V_Draw* functions
#include "../../r_draw.h" // TC_DEFAULT, GTC_CACHE
#include "../../k_battle.h" // K_NumEmeralds
#include "../../k_color.h" // K_RainbowColor
#include "../../z_zone.h" // Z_Realloc
#include "../../i_time.h"
#include "../../tables.h"

#define ITEM_BOX_WIDTH 46 // 50 - 4
#define ITEM_BOX_HEIGHT 50

#define RING_BOX_WIDTH 55 // 56 - 1
#define RING_BOX_HEIGHT 48

/**
 * =================
 * ROULETTE TRACKING
 * =================
*/

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
 * =================
 * ROULETTE TRACKING
 * =================
*/
static boolean areWeInDanger(player_t* dangerousPlayer) {
    boolean is_charging = dangerousPlayer->instaWhipCharge >= INSTAWHIP_CHARGETIME;
    boolean is_invincible = dangerousPlayer->itemtype == KITEM_INVINCIBILITY || dangerousPlayer->invincibilitytimer;
    boolean is_fat = dangerousPlayer->itemtype == KITEM_GROW || dangerousPlayer->growshrinktimer > 0;

    return (is_charging || is_invincible || is_fat);
}

// Following code is copied (and tweaked) straight from K_DrawTargetTracking (see: k_hud_track.cpp)
// Thanks TehRealSalt
static void _drawOffscreenCheck(trackingResult_t *result, player_t* dangerousPlayer) {

    uint8_t* colormap = R_GetTranslationColormap(TC_DEFAULT, static_cast<skincolornum_t>(dangerousPlayer->mo->color), GTC_CACHE);
    
    int32_t scrVal = 240;
    vector2_t screenSize = {};

    int32_t borderSize = 7;
    vector2_t borderWin = {};
    vector2_t borderDir = {};
    fixed_t borderLen = FRACUNIT;

    vector2_t arrowDir = {};

    vector2_t arrowPos = {};
    patch_t* arrowPatch = kp_capsuletarget_arrow[1][((leveltime / 3)) & 1];
    int32_t arrowFlags = 0;

    vector2_t targetPos = {};
    patch_t* targetPatch = faceprefix[dangerousPlayer->skin][FACE_MINIMAP];

    screenSize.x = vid.width / vid.dupx;
    screenSize.y = vid.height / vid.dupy;

    scrVal = std::max(screenSize.x, screenSize.y) - 80;

    borderWin.x = screenSize.x - borderSize;
    borderWin.y = screenSize.y - borderSize;

    arrowDir.x = 0;
    arrowDir.y = P_MobjFlip(dangerousPlayer->mo) * FRACUNIT;

    // Simply pointing towards the result doesn't work, so inaccurate hack...
    borderDir.x = FixedMul(
        FixedMul(
            FINESINE((result->angle >> ANGLETOFINESHIFT) & FINEMASK),
            FINECOSINE((-result->pitch >> ANGLETOFINESHIFT) & FINEMASK)
        ),
        result->fov
    );

    borderDir.y = FixedMul(FINESINE((-result->pitch >> ANGLETOFINESHIFT) & FINEMASK), result->fov);

    borderLen = R_PointToDist2(0, 0, borderDir.x, borderDir.y);

    if (borderLen > 0)
    {
        borderDir.x = FixedDiv(borderDir.x, borderLen);
        borderDir.y = FixedDiv(borderDir.y, borderLen);
    } else {
        // Interception, don't draw at all
        return;
    }

    if (abs(borderDir.x) > abs(borderDir.y))
    {
        // Horizontal arrow
        arrowPatch = kp_capsuletarget_arrow[1][((leveltime / 3)) & 1];
        arrowDir.y = 0;

        if (borderDir.x < 0)
        {
            // LEFT
            arrowDir.x = -FRACUNIT;
        }
        else
        {
            // RIGHT
            arrowDir.x = FRACUNIT;
        }
    } else {
        // Interception, don't draw at all if on top or bottom
        return;
    }

    arrowPos.x = (screenSize.x >> 1) + FixedMul(scrVal, borderDir.x);
    arrowPos.y = (screenSize.y >> 1) + FixedMul(scrVal, borderDir.y);

    arrowPos.x = std::clamp(arrowPos.x, borderSize, borderWin.x) * FRACUNIT;
    arrowPos.y = std::clamp(arrowPos.y, borderSize, borderWin.y) * FRACUNIT;

    {
        targetPos.x = arrowPos.x - (arrowDir.x * 12);
        targetPos.y = arrowPos.y - (arrowDir.y * 12);

        targetPos.x -= (targetPatch->width << FRACBITS) >> 1;
        targetPos.y -= (targetPatch->height << FRACBITS) >> 1;
    }

    arrowPos.x -= (arrowPatch->width << FRACBITS) >> 1;
    arrowPos.y -= (arrowPatch->height << FRACBITS) >> 1;

    if (arrowDir.x < 0)
    {
        arrowPos.x += arrowPatch->width << FRACBITS;
        arrowFlags |= V_FLIP;
    }

    if (arrowDir.y < 0)
    {
        // Don't draw at all
        return;
    }

    if (targetPatch)
    {
        // For now, draw the generic ringsting graphic to denote danger
        const std::vector ringsting_ids = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
        const std::string ringsting_id = fmt::format("MXCL{0:c}0", ringsting_ids[
            leveltime % ringsting_ids.size()
        ]);
        patch_t* ringstring_patch = static_cast<patch_t*>(W_CachePatchName(ringsting_id.c_str(), GTC_CACHE));

        const fixed_t ringsting_x = (arrowDir.x < 0) ? targetPos.x + (20*FRACUNIT) : targetPos.x - (10*FRACUNIT);

        // Draw the ring sting graphic
        V_DrawFixedPatch(ringsting_x, targetPos.y + (13*FRACUNIT), FRACUNIT/3, V_SPLITSCREEN, ringstring_patch, colormap);

        // And then the target patch (which is the minimap graphic)
        V_DrawFixedPatch(targetPos.x, targetPos.y, FRACUNIT, V_SPLITSCREEN, targetPatch, colormap);
    }

    // Then the arrow!
    V_DrawFixedPatch(arrowPos.x, arrowPos.y, FRACUNIT, V_SPLITSCREEN | arrowFlags, arrowPatch, colormap);
    // V_DrawFixedPatch((BASEVIDWIDTH - 20) * FRACUNIT, result->y, FRACUNIT, V_HUDTRANS|V_SPLITSCREEN|splitflags, faceprefix[dangerousPlayer->skin][FACE_MINIMAP], colormap);
}

// Draw dangerous players (ready to instawhip, invincible, grow, etc) to the side of the HUD
void RR_DrawDangerousPlayerCheck(player_t *dangerousPlayer, fixed_t distance, trackingResult_t* result)
{
    /**
     * Not sure if this function is even useful. Players can come at you hard and fast.
     * You wouldn't even have enough time to parse the danger check before getting hit.
     * To compensate, *could* make the distance check a lot more generous.
     * Math could be better too.
     */
    // see K_GlanceAtPlayers
    const boolean podiumspecial = (
        K_PodiumSequence() == true && 
        stplyr->nextwaypoint == NULL && 
        stplyr->speed == 0
    );

    // The danger should be *relatively* close
    const fixed_t maxdangerdistance = FixedMul(3200 * mapobjectscale, K_GetKartGameSpeedScalar(gamespeed));

    // Don't draw these checks if we're on the podium
    if (podiumspecial) {
        return;
    }

    // V_DrawString(60, 60, V_BLUEMAP, va("%d", maxdistance/FRACUNIT));
    // V_DrawString(60, 80, V_GREENMAP, va("%d", distance/FRACUNIT));
    // V_DrawString(60, 100, V_BLUEMAP, va("%d", maxdangerdistance/FRACUNIT));

    // Don't draw these checks if the dangerous players are too far
    if (distance > maxdangerdistance) {
        return;
    }

    // Any danger?
    if (!areWeInDanger(dangerousPlayer)) {
        return;
    }

    // Now draw
    _drawOffscreenCheck(result, dangerousPlayer);
}
 
 

