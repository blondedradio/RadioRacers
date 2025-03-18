// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_tally.cpp
/// \brief Emote nonsense (for grades)

#include <algorithm>

#include "../../doomstat.h" // gp_rank_e
#include "../../hu_stuff.h"
#include "../../v_video.h"
#include "../../r_draw.h"
#include "../../m_random.h" // M_RandomRange
#include "../../z_zone.h" // PU_CACHE
#include "../../p_tick.h"
#include "../../g_demo.h" // demo.playback
#include "../../g_game.h" // netgame
#include "../../p_local.h" //stplyr
#include "../rr_hud.h"

#define GRADE_GRAPHIC_HEIGHT 46
#define GRADE_GRAPHIC_HEIGHT_F 46.f
#define GRADE_GRAPHIC_WIDTH 53
#define GRADE_GRAPHIC_WIDTH_HALF 26.5f

typedef enum
{
    FLIPPING_NONE,
    FLIPPING_NOT_DOING,
    FLIPPING_GRADE,
    FLIPPING_EMOTE,
    FLIPPING_DONE
} flipping_state_t;

typedef struct 
{
    fixed_t horz_scale;
    fixed_t offset;
} flipped_graphic_t;

struct GPRankLookup {
    gp_rank_e grade;
    std::vector<emote_t>* array;
};

std::vector<emote_t>* RANK_EMOTES_LOOKUP[] = {
    &E_RANK_EMOTES, //GRADE_E
    &D_RANK_EMOTES,
    &C_RANK_EMOTES,
    &B_RANK_EMOTES,
    &A_RANK_EMOTES
};

static const int GRADE_FLIP_TIMER_START = 100;
static const int EMOTE_FLIP_TIMER_START = 140; // Slightly slower reveal

static const int FASTER_FLIP_TIMER = 30;

static int GRADE_FLIP_TIMER = GRADE_FLIP_TIMER_START;
static int EMOTE_FLIP_TIMER = EMOTE_FLIP_TIMER_START;

static int timer = GRADE_FLIP_TIMER_START;

// If the player is spectating someone else, choose one for the appropriate grade.
static emote_t* CHOICE_EMOTE[5] = {nullptr};
static flipping_state_t GRADE_FLIPPING_STATE[5] = {FLIPPING_NONE};


static flipping_state_t flipping_state = FLIPPING_NONE;

static emote_t* getRandomGradeEmote(gp_rank_e grade) {
    if (grade <= GRADE_INVALID || grade > GRADE_S)
        return nullptr;

    std::vector<emote_t>* array = RANK_EMOTES_LOOKUP[static_cast<int>(grade)];

    if (array->empty())
        return nullptr;

    return &(*array)[M_RandomRange(0, static_cast<int>(array->size()-1))];
}

static void initRandomEmotes(void) {
    for (int i = GRADE_E; i <= GRADE_S; i++) {
        CHOICE_EMOTE[i] = getRandomGradeEmote(static_cast<gp_rank_e>(i));
    }
}

static flipped_graphic_t getFlippedGraphic(float progress, int width)
{
	angle_t fakeang = progress * 90 * ANG1;
	fixed_t scalex = FINESINE(fakeang >> ANGLETOFINESHIFT);
	scalex = abs(scalex);
	fixed_t offs = ((FRACUNIT - scalex) * width)/2;

    return {scalex, offs};
}

static void flipGrade(fixed_t x, fixed_t y, patch_t *patch, skincolornum_t grade_color)
{
    if (flipping_state != FLIPPING_GRADE) return;

	float progress = ((float) GRADE_FLIP_TIMER / GRADE_FLIP_TIMER_START);

	// CONS_Printf(M_GetText("progress %.2f\n"), progress);

    flipped_graphic_t flipped_graphic = getFlippedGraphic(progress, patch->width);

	V_DrawStretchyFixedPatch(
		x+flipped_graphic.offset,
		y,
		FixedMul(abs(flipped_graphic.horz_scale), FRACUNIT),
		FRACUNIT,
		0,
		patch,
        R_GetTranslationColormap(TC_DEFAULT, grade_color, GTC_CACHE)
	);
}

static void flipEmote(fixed_t x, fixed_t y, emote_t* grade_emote)
{
    if (flipping_state != FLIPPING_EMOTE && flipping_state != FLIPPING_DONE) return;

    patch_t* emote = static_cast<patch_t*>(W_CachePatchNum(getEmoteFrame(grade_emote), PU_CACHE));

    float progress = 1.0f - ((float) EMOTE_FLIP_TIMER / EMOTE_FLIP_TIMER_START);
    fixed_t scale = FRACUNIT;
    float_t emote_width = static_cast<float_t>(emote->width);

    // Scale down the emote in-case it's massive or too small
    // Looking at you, :widepeepo:
    scale = FloatToFixed(GRADE_GRAPHIC_HEIGHT_F/static_cast<float_t>(emote->height));
    
    /**
     * If the emote is wider than the RANK graphic (53px), then it should be drawn
     * more to the left. So it looks like it's "flipping" from the middle.
     */
    if (emote->width > GRADE_GRAPHIC_WIDTH) {
        // Offset the x-coordinate by half the width of the emote and half the width of the RANK graphic
        // (EMOTE-WIDTH / 2) - (RANK GRAPHIC WIDTH / 2)
        float_t emote_half_width = (emote_width * FixedToFloat(scale)) / 2.f;

        x -= FloatToFixed(emote_half_width - GRADE_GRAPHIC_WIDTH_HALF);

        // Magic number
        x-= FloatToFixed(2.0f);
    }
    
    flipped_graphic_t flipped_graphic = getFlippedGraphic(
        progress, 
        static_cast<int>((emote_width * FixedToFloat(scale)))
    );

    fixed_t main_x = x+flipped_graphic.offset;
    fixed_t main_y = y;

    fixed_t pscale = FixedMul(abs(flipped_graphic.horz_scale), scale);
    fixed_t vscale = scale;

    // Draw a black background behind the emote to give it some padding
    patch_t* background = static_cast<patch_t*>(W_CachePatchName("RGRADEBG", PU_CACHE));

    float_t final_emote_width = emote_width * FixedToFloat(scale);
    float_t final_emote_height = GRADE_GRAPHIC_HEIGHT_F;

    // For slight padding
    float_t target_width = final_emote_width + 2.f;
    float_t target_height = final_emote_height + 2.f;

    // I hate HUD math
    float_t height_scale_factor = target_height / static_cast<float_t>(background->height);
    float_t width_scale_factor = target_width / static_cast<float_t>(background->width);

    fixed_t bg_w = FixedMul(FloatToFixed(width_scale_factor), FRACUNIT);
    fixed_t bg_h = FixedMul(FloatToFixed(height_scale_factor), FRACUNIT);

    // Move the background 1px to the left, and 1px upwards
    V_DrawStretchyFixedPatch(
        main_x - FloatToFixed(1.f),
        main_y - FloatToFixed(1.f),
        FixedMul(abs(flipped_graphic.horz_scale), bg_w),
        bg_h,
        0,
        background,
        NULL
    );

    // Then draw the emote.
	V_DrawStretchyFixedPatch(
	    main_x,
		main_y,
		pscale,
		vscale,
		0,
		emote,
		NULL
	);
}

void RR_DrawGradeEmote(player_grade_info_t grade_info)
{
    // Huh?
    if (!stplyr->tally.active)
        return;
    
    // This is already checked in RR_IsEmoteTallyPossible, but check again in case.
    if (GRADE_FLIPPING_STATE[static_cast<int>(grade_info.grade)] == FLIPPING_NOT_DOING)
        return;
    
    if (flipping_state == FLIPPING_NONE)
    {
        // We got random emotes now, epic. Are there any for the grade the player just got?
        initRandomEmotes();

        // There aren't any? Damn! Guess we can't draw for that grade.
        if (CHOICE_EMOTE[static_cast<int>(grade_info.grade)] == nullptr)
        {
            GRADE_FLIPPING_STATE[static_cast<int>(grade_info.grade)] = FLIPPING_NOT_DOING;
            return;
        }
        // If the player has fast forwarded the tally, flip everything faster
        if (
            stplyr->tally.active &&
            stplyr->tally.delay <= TICRATE &&
            !netgame
        ) {
            timer = FASTER_FLIP_TIMER;
            flipping_state = FLIPPING_EMOTE;
        } else {
            flipping_state = FLIPPING_GRADE;
        }
    }

    if (flipping_state == FLIPPING_GRADE) {
        flipGrade(grade_info.x, grade_info.y, grade_info.grade_patch, grade_info.grade_color);
    }

    if (flipping_state == FLIPPING_EMOTE || flipping_state == FLIPPING_DONE) {
        flipEmote(grade_info.x, grade_info.y, CHOICE_EMOTE[grade_info.grade]);
    }
}

boolean RR_ShouldDrawEmoteTally(void)
{
    return flipping_state != FLIPPING_NOT_DOING;
}

boolean RR_IsEmoteTallyPossible(gp_rank_e grade)
{
    return false;
    if (GRADE_FLIPPING_STATE[grade] == FLIPPING_NONE) {    
        std::vector<emote_t>* array = RANK_EMOTES_LOOKUP[grade];
        boolean is_possible = !array->empty();
    
        if (!is_possible)
            GRADE_FLIPPING_STATE[grade] = FLIPPING_NOT_DOING;
    
        return is_possible;
    } else {
        return (GRADE_FLIPPING_STATE[grade] != FLIPPING_NOT_DOING);
    }
}

void RR_EmoteTally_Tick(void)
{
    if (timer >= 0)
    {
        timer--;
    }

    switch(flipping_state)
    {
        case FLIPPING_GRADE:
        {
            if (timer == 0) 
            {
                flipping_state = FLIPPING_EMOTE;
                timer = EMOTE_FLIP_TIMER_START;
            }
            break;
        }
        case FLIPPING_EMOTE:
        {
            if (timer == 0)
            {
                flipping_state = FLIPPING_DONE;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

void RR_InitGradeEmoteTally(void)
{
    // Reset all the timers
    // GRADE_FLIP_TIMER = GRADE_FLIP_TIMER_START;
    // EMOTE_FLIP_TIMER = EMOTE_FLIP_TIMER_START;

    timer = GRADE_FLIP_TIMER_START;

    // Reset the state
    flipping_state = FLIPPING_NONE;

    // And reset the choice emote array
    std::fill(
        std::begin(CHOICE_EMOTE), std::end(CHOICE_EMOTE), nullptr
    );

    // And the flipping state one too.
    std::fill(
        std::begin(GRADE_FLIPPING_STATE), std::end(GRADE_FLIPPING_STATE), FLIPPING_NONE
    );
}