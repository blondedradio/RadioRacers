// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_video.h
/// \brief Video related functions, usually copies of functions from v_video.h

#ifndef __RR_VIDEO__
#define __RR_VIDEO__

#include "../m_cond.h"
#include "../command.h"
#include "../console.h"
#include "../v_video.h"

#include "rr_cvar.h"
#include "rr_controller.h"

#ifdef __cplusplus

#include <vector>
/**
 * RADIO: Copied straight from v_video.cpp
 * Instead of manipulating the original function, just copy it here.
 */
extern boolean V_CharacterValidEx(font_t* font, int c);

typedef struct
{
    char* msg;
    std::vector<int> lines_with_emotes;
    boolean contains_text;
} word_wrap_results_t;

typedef enum  {
    CLT_MAIN = 0,
    CLT_MINI,
    CLT_INPUT
} chat_log_type_t;

word_wrap_results_t V_RR_ScaledWordWrap(
	fixed_t w,
	fixed_t scale,
	fixed_t spacescale,
	fixed_t lfscale,
	INT32 flags,
	int fontno,
	const char* s,
    patch_t** found_emote_patches,
    int* found_emote_atlases,
    boolean contains_text
);

void V_RR_DrawStringScaled(
    fixed_t     x,
    fixed_t     y,
    fixed_t           scale,
    fixed_t     space_scale,
    fixed_t  linefeed_scale,
    INT32       flags,
    const UINT8 *colormap,
    int         font,
    const char *text,
    int chat_log_index,
    std::vector<int> lines_with_emotes,
    chat_log_type_t log_type,
    boolean contains_text,
    int top_offset,
    int bottom_offset
);

#define V_RR_StringWidth( string,option, log_index, contains_text ) \
	(V_RR_StringScaledWidth ( FRACUNIT, FRACUNIT, FRACUNIT, option, HU_FONT, string, log_index, contains_text) / FRACUNIT)

fixed_t V_RR_StringScaledWidth(
    fixed_t      scale,
    fixed_t spacescale,
    fixed_t    lfscale,
    INT32      flags,
    int        fontno,
    const char *s,
    int chat_log_index,
    boolean contains_text
);

extern "C" {
#endif

#define MAX_HEIGHT 10.f
#define MAX_HEIGHT_WITH_TEXT 8.0f

#define EMOTE_LINE_PADDING_INT 3
#define EMOTE_LINE_PADDING FloatToFixed(3.f)

#define EMOTE_LINE_PADDING_TEXT_INT 2
#define EMOTE_LINE_PADDING_TEXT FloatToFixed(2.f)

#define EMOTE_PADDING 1
#define GET_CHAT_EMOTE_SCALE(h, has_text) (((has_text) ? MAX_HEIGHT_WITH_TEXT : MAX_HEIGHT)/static_cast<float_t>(h))

#define V_DrawThinTimerString( x,y,option,string ) \
	V__DrawDupxString (x,y,FRACUNIT,option,NULL,TINYTIMER_FONT,string)

#define V_DrawThinTimerStringScaled( x,y,option,string, scale ) \
	V__DrawDupxString (x,y,scale,option,NULL,TINYTIMER_FONT,string)

    // Input display
extern boolean isPingDrawn;
extern boolean isDrawingInput;

#ifdef __cplusplus
} // extern "C"
#endif

#endif
