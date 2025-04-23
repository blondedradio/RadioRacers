// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/video/rr_video.cpp
/// \brief Video-related functions copied over from v_video.h

#include "../rr_video.h"
#include "../../v_video.h"
#include "../../z_zone.h"
#include "../rr_hud.h"

/**
 * RADIO: I'd rather not mess with the core drawing functions in v_video.cpp.
 * So, copy them here. Also, less work involved with rebasing.
 */

static UINT8 V_GetButtonCodeWidth(UINT8 c)
{
	UINT8 x = 0;

	switch (c & 0x0F)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
		// arrows
		x = 12;
		break;

	case 0x07:
	case 0x08:
	case 0x09:
		// shoulders, start
		x = 14;
		break;

	case 0x04:
		// dpad
		x = 14;
		break;

	case 0x0A:
	case 0x0B:
	case 0x0C:
	case 0x0D:
	case 0x0E:
	case 0x0F:
		// faces
		x = 10;
		break;
	}

	return x;
}

typedef struct
{
	fixed_t chw;
	fixed_t spacew;
	fixed_t lfh;
	fixed_t (*dim_fn)(fixed_t, fixed_t, INT32, INT32, fixed_t*);
	UINT8 button_yofs;
} fontspec_t;

static inline fixed_t FixedCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) scale;
	(void) hchw;
	(void) dupx;
	(*cwp) = chw;
	return 0;
}

static inline fixed_t VariableCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul((*cwp) << FRACBITS, scale);
	return 0;
}

static inline fixed_t CenteredCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	INT32 cxoff;
	/*
	For example, center a 4 wide patch to 8 width:
	4/2   = 2
	8/2   = 4
	4 - 2 = 2 (our offset)
	2 + 4 = 6 = 8 - 2 (equal space on either side)
	*/
	cxoff = hchw - ((*cwp) >> 1);
	(*cwp) = chw;
	return FixedMul((cxoff * dupx) << FRACBITS, scale);
}

static inline fixed_t BunchedCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 1) << FRACBITS, scale);
	return 0;
}

static inline fixed_t MenuCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 2) << FRACBITS, scale);
	return 0;
}

static inline fixed_t GamemodeCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 2) << FRACBITS, scale);
	return 0;
}

static inline fixed_t FileCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 3) << FRACBITS, scale);
	return 0;
}

static inline fixed_t LSTitleCharacterDim(fixed_t scale, fixed_t chw, INT32 hchw, INT32 dupx, fixed_t* cwp)
{
	(void) chw;
	(void) hchw;
	(void) dupx;
	(*cwp) = FixedMul(std::max<INT32>(1, (*cwp) - 4) << FRACBITS, scale);
	return 0;
}

static boolean V_CharacterValid(font_t* font, int c)
{
	return (c >= 0 && c < font->size && font->font[c] != NULL);
}

boolean V_CharacterValidEx(font_t* font, int c)
{
	return V_CharacterValid(font, c);
}

static void V_GetFontSpecification(int fontno, INT32 flags, fontspec_t* result)
{
	/*
	Hardcoded until a better system can be implemented
	for determining how fonts space.
	*/

	// All other properties are guaranteed to be set
	result->chw = 0;
	result->button_yofs = 0;

	const INT32 spacing = (flags & V_SPACINGMASK);

	switch (fontno)
	{
	default:
	case HU_FONT:
	case MENU_FONT:
		result->spacew = 4;
		switch (spacing)
		{
		case V_MONOSPACE:
			result->spacew = 8;
			/* FALLTHRU */
		case V_OLDSPACING:
			result->chw = 8;
			break;
		case V_6WIDTHSPACE:
			result->spacew = 6;
			break;
		}
		break;
	case TINY_FONT:
	case TINYTIMER_FONT:
		result->spacew = 2;
		switch (spacing)
		{
		case V_MONOSPACE:
			result->spacew = 5;
			/* FALLTHRU */
		case V_OLDSPACING:
			result->chw = 5;
			break;
		case V_6WIDTHSPACE:
			result->spacew = 3;
			break;
		}
		break;
	case MED_FONT:
		result->chw = 6;
		result->spacew = 6;
		break;
	case LT_FONT:
		result->spacew = 12;
		break;
	case CRED_FONT:
		result->spacew = 16;
		break;
	case KART_FONT:
		result->spacew = 3;
		switch (spacing)
		{
		case V_MONOSPACE:
			result->spacew = 12;
			/* FALLTHRU */
		case V_OLDSPACING:
			result->chw = 12;
			break;
		case V_6WIDTHSPACE:
			result->spacew = 6;
		}
		break;
	case GM_FONT:
		result->spacew = 6;
		break;
	case FILE_FONT:
		result->spacew = 0;
		break;
	case LSHI_FONT:
	case LSLOW_FONT:
		result->spacew = 10;
		break;
	case OPPRF_FONT:
		result->spacew = 5;
		break;
	case PINGF_FONT:
		result->spacew = 3;
		break;
	case ROLNUM_FONT:
		result->spacew = 17;
		break;
	case RO4NUM_FONT:
		result->spacew = 9;
		break;
	}

	switch (fontno)
	{
	default:
	case HU_FONT:
	case MENU_FONT:
	case TINY_FONT:
	case TINYTIMER_FONT:
	case KART_FONT:
	case MED_FONT:
		result->lfh = 12;
		break;
	case LT_FONT:
	case CRED_FONT:
	case FILE_FONT:
		result->lfh = 12;
		break;
	case GM_FONT:
		result->lfh = 32;
		break;
	case LSHI_FONT:
		result->lfh = 56;
		break;
	case LSLOW_FONT:
		result->lfh = 38;
		break;
	case OPPRF_FONT:
	case PINGF_FONT:
		result->lfh = 10;
		break;
	case ROLNUM_FONT:
		result->lfh = 33;
		break;
	case RO4NUM_FONT:
		result->lfh = 15;
		break;
	}

	switch (fontno)
	{
	default:
		if (result->chw)
			result->dim_fn = CenteredCharacterDim;
		else
			result->dim_fn = VariableCharacterDim;
		break;
	case HU_FONT:
		if (result->chw)
			result->dim_fn = CenteredCharacterDim;
		else
			result->dim_fn = BunchedCharacterDim;
		break;
	case MENU_FONT:
		if (result->chw)
			result->dim_fn = CenteredCharacterDim;
		else
			result->dim_fn = MenuCharacterDim;
		break;
	case KART_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = BunchedCharacterDim;
		break;
	case TINY_FONT:
	case TINYTIMER_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = BunchedCharacterDim;
		break;
	case MED_FONT:
		result->dim_fn = FixedCharacterDim;
		break;
	case GM_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = GamemodeCharacterDim;
		break;
	case FILE_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = FileCharacterDim;
		break;
	case LSHI_FONT:
	case LSLOW_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = LSTitleCharacterDim;
		break;
	case OPPRF_FONT:
	case PINGF_FONT:
	case ROLNUM_FONT:
	case RO4NUM_FONT:
		if (result->chw)
			result->dim_fn = FixedCharacterDim;
		else
			result->dim_fn = BunchedCharacterDim;
		break;
	}

	switch (fontno)
	{
	case HU_FONT:
		result->button_yofs = 2;
		break;
	case MENU_FONT:
		result->button_yofs = 1;
		break;
	}
}

static INT32 getEmotePatchWidth(patch_t* emote_patch, boolean contains_text = true)
{
    float_t scale = GET_CHAT_EMOTE_SCALE(emote_patch->height, contains_text);
    float_t scaled_width = emote_patch->width * scale;

    return static_cast<int>(scaled_width + EMOTE_PADDING);
}

static INT32 getAtlasEmotePatchWidth(int atlas_id, boolean contains_text = true)
{
    INT32 height = EMOTE_ATLASES[atlas_id]->height;
    INT32 width = EMOTE_ATLASES[atlas_id]->width;

    float_t scale = GET_CHAT_EMOTE_SCALE(height, contains_text);
    float_t scaled_width = width * scale;

    return static_cast<int>(scaled_width + EMOTE_PADDING);
}

static INT32 getEmotePatchWidthDirectly(emote_t* emote, boolean contains_text = true)
{
    /**
     * Use the very first frame of an emote.
     * Emotes have a standard resolution and rarely stray away from that.
     * But if an emote has a non-standard configuration 
     *  (i.e. inconsistent frame widths in an animated emote),
     *  that's not our problem.
     */
	
	INT32 width = -1;
	if (emote->atlas_id == -1) {
		patch_t* emote_patch = static_cast<patch_t*>(W_CachePatchNum(emote->frames[0], PU_HUDGFX));
		width = getEmotePatchWidth(emote_patch, contains_text);
	} else {
		width = getAtlasEmotePatchWidth(emote->atlas_id, contains_text);
	}
	return width;
}

word_wrap_results_t V_RR_ScaledWordWrap(
	fixed_t w,
	fixed_t scale,
	fixed_t spacescale,
	fixed_t lfscale,
	INT32 flags,
	int fontno,
	const char* s,
	patch_t** found_emote_patches = nullptr,
    int* found_emote_atlases = nullptr,
	boolean contains_text = true
)
{
    /**
     * RADIO: Yeah, here.
     */
    size_t emote_tracker_idx = 0;

	INT32 hchw; /* half-width for centering */

	INT32 dupx;

	font_t* font;

	boolean uppercase;

	fixed_t cx;
	fixed_t right;

	fixed_t cw;

	fixed_t ew; // emote width
	fixed_t ex; // emote x

	int c;

	uppercase = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags &= ~(V_FLIP); /* These two (V_FORCEUPPERCASE) share a bit. */

	font = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw = fontspec.chw >> 1;

	fontspec.chw <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;

#define Mul(id, scale) (id = FixedMul(scale, id))
	Mul(fontspec.chw, scale);
	Mul(fontspec.spacew, scale);
	Mul(fontspec.lfh, scale);

	Mul(fontspec.spacew, spacescale);
	Mul(fontspec.lfh, lfscale);
#undef Mul

	if ((flags & V_NOSCALESTART))
	{
		dupx = vid.dupx;

		hchw *= dupx;

		fontspec.chw *= dupx;
		fontspec.spacew *= dupx;
		fontspec.lfh *= vid.dupy;
	}
	else
	{
		dupx = 1;
	}

	cx = 0;
	right = 0;

	ex = 0;

	size_t reader = 0, writer = 0, startwriter = 0;
	fixed_t cxatstart = 0;

	size_t len = strlen(s) + 1;
	size_t potentialnewlines = 8;
	size_t sparenewlines = potentialnewlines;

	char* newstring = static_cast<char*>(Z_Malloc(len + sparenewlines, PU_STATIC, NULL));

	// RADIO: Check if next line has an emote or not, useful to determine padding when drawing the message
	std::vector<int> lines_with_emotes;
	bool line_checked_for_emotes = false;
	size_t current_line = 1;

	for (; (c = s[reader]); ++reader, ++writer)
	{
		newstring[writer] = s[reader];

		right = 0;

		switch (c)
		{
		case '\n':
			cx = 0;
			ex = 0;
			cxatstart = 0;
			startwriter = 0;
			current_line++;
			line_checked_for_emotes = false;
			break;
        case '\x01':
            // RADIO: Emote logic
            if (found_emote_patches != nullptr || found_emote_atlases != nullptr)
            {
				INT32 emote_w = -1;

				if (found_emote_atlases != nullptr) {
					if (found_emote_atlases[emote_tracker_idx] != -1) 
					{
						emote_w = getAtlasEmotePatchWidth(found_emote_atlases[emote_tracker_idx], contains_text);
					}
				}

				if(emote_w == -1 && found_emote_patches != nullptr ) {
					if (found_emote_patches[emote_tracker_idx] != nullptr) {
						emote_w = getEmotePatchWidth(found_emote_patches[emote_tracker_idx], contains_text);
					}
				}
				
                ew = SHORT(emote_w) * dupx;
				
				// Is the emote width about to pass the chat box width?
				if (ex + (ew * FRACUNIT) > w) {
					right = ex + (ew * FRACUNIT);
					cxatstart = ex;
					startwriter = writer-1;

					if (!line_checked_for_emotes) {
						// CONS_Printf("emote found on line %d\n", current_line);
						lines_with_emotes.push_back(current_line + 1);
						line_checked_for_emotes = true;
					}
					break;
				}

				if (ex == 0) { // If we've reached the first emote in the line
					ex = cx + (ew * FRACUNIT);
				} else {
					ex += (ew * FRACUNIT);
				}

                emote_tracker_idx++;

				// Look ahead
				if (!line_checked_for_emotes) {
					// CONS_Printf("emote found on line %d\n", current_line);
					lines_with_emotes.push_back(current_line);
					line_checked_for_emotes = true;
				}
            }
            break;
		default:
			if ((c & 0xF0) == 0x80 || c == V_STRINGDANCE)
				;
			else if ((c & 0xB0) & 0x80) // button prompts
			{
				cw = V_GetButtonCodeWidth(c) * dupx;
				cx += cw * scale;
				right = cx;
			}
			else
			{
				if (uppercase)
				{
					c = toupper(c);
				}
				else if (V_CharacterValid(font, c - font->start) == false)
				{
					// Try the other case if it doesn't exist
					if (c >= 'A' && c <= 'Z')
					{
						c = tolower(c);
					}
					else if (c >= 'a' && c <= 'z')
					{
						c = toupper(c);
					}
				}

				c -= font->start;
				if (V_CharacterValid(font, c) == true)
				{
					cw = SHORT(font->font[c]->width) * dupx;

					// How bunched dims work is by incrementing cx slightly less than a full character width.
					// This causes the next character to be drawn overlapping the previous.
					// We need to count the full width to get the rightmost edge of the string though.
					right = cx + (cw * scale);

					(*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);
					cx += cw;
				}
				else
				{
					cx += fontspec.spacew;
					cxatstart = cx;
					startwriter = writer;
				}

				// Track the emote x variable whenever we're drawing normal font characters
				ex = cx;
			}
		}

		// Start trying to wrap if presumed length exceeds the space we have on-screen.
		if (right && right > w)
		{
			if (startwriter != 0)
			{
				newstring[startwriter] = '\n';
				cx -= cxatstart;
				cxatstart = 0;
				startwriter = 0;

				// Reset this here, otherwise it only gets caught in the '\n' case
				ex = 0;
				if (line_checked_for_emotes) {
					line_checked_for_emotes = false;
				}
			}
			else
			{
				if (sparenewlines == 0)
				{
					sparenewlines = (potentialnewlines *= 2);
					newstring = static_cast<char*>(Z_Realloc(newstring, len + sparenewlines, PU_STATIC, NULL));
				}

				sparenewlines--;
				len++;

				newstring[writer++] = '\n';	   // Over-write previous
				cx = cw;					   // Valid value in the only case right is currently set
				newstring[writer] = s[reader]; // Re-add
			}
			current_line++;
		}
	}

	newstring[writer] = '\0';

	word_wrap_results_t results;
	results.msg = newstring;
	results.contains_text = contains_text;
	results.lines_with_emotes = lines_with_emotes;

	return results;
}


static bool doesNextLineHaveEmote(size_t current_line, std::vector<int> lines_with_emotes) {
    if (lines_with_emotes.empty()) {
        return false;
    }
    return std::find(lines_with_emotes.begin(), lines_with_emotes.end(), current_line+1) != lines_with_emotes.end();
}

void V_RR_DrawStringScaled(
	fixed_t x,
	fixed_t y,
	fixed_t scale,
	fixed_t spacescale,
	fixed_t lfscale,
	INT32 flags,
	const UINT8* colormap,
	int fontno,
	const char* s,
    int chat_log_index,
	std::vector<int> lines_with_emotes,
	chat_log_type_t log_type,
	boolean contains_text = true,
	int top_offset = 0,
	int bottom_offset = 0
)
{
    /**
     * RADIO: Yeah, here.
     */
    size_t emote_tracker_idx = 0;
    boolean line_has_emote = false;
	boolean check_for_emotes = false;
	boolean previous_line_had_emote = false;
	boolean previous_line_checked = false;
	fixed_t __PADDING = (contains_text) ? EMOTE_LINE_PADDING_TEXT : EMOTE_LINE_PADDING;
    
	INT32 hchw; /* half-width for centering */

	INT32 dupx;
	INT32 dupy;

	fixed_t right;
	fixed_t bot;

	font_t* font;

	boolean uppercase;
	boolean notcolored;

	boolean dance;
	boolean nodanceoverride;
	INT32 dancecounter;

	fixed_t cx, cy;

	fixed_t cxoff, cyoff;
	fixed_t cw;

	fixed_t left;

	int c;

	uppercase = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags &= ~(V_FLIP); /* These two (V_FORCEUPPERCASE) share a bit. */

	dance = (flags & V_STRINGDANCE) != 0;
	nodanceoverride = !dance;
	dancecounter = 0;

	/* Some of these flags get overloaded in this function so
	   don't pass them on. */
	flags &= ~(V_PARAMMASK);

	if (colormap == NULL)
	{
		colormap = V_GetStringColormap((flags & V_CHARCOLORMASK));
	}

	notcolored = !colormap;

	font = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw = fontspec.chw >> 1;

	fontspec.chw <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;
	fontspec.lfh <<= FRACBITS;

#define Mul(id, scale) (id = FixedMul(scale, id))
	Mul(fontspec.chw, scale);
	Mul(fontspec.spacew, scale);
	Mul(fontspec.lfh, scale);

	Mul(fontspec.spacew, spacescale);
	Mul(fontspec.lfh, lfscale);
#undef Mul

	if ((flags & V_NOSCALESTART))
	{
		dupx = vid.dupx;
		dupy = vid.dupy;

		hchw *= dupx;

		fontspec.chw *= dupx;
		fontspec.spacew *= dupx;
		fontspec.lfh *= dupy;

		right = vid.width;
	}
	else
	{
		dupx = 1;
		dupy = 1;

		right = (vid.width / vid.dupx);
		if (!(flags & V_SNAPTOLEFT))
		{
			left = (right - BASEVIDWIDTH) / 2; /* left edge of drawable area */
			right -= left;
		}
	}

	right <<= FRACBITS;
	bot = vid.height << FRACBITS;

	cx = x;
	cy = y;
	cyoff = 0;

	int count = 1;
	for (; (c = *s); ++s, ++dancecounter)
	{
		switch (c)
		{
		case '\n':
			// CONS_Printf("break #%d\n", count);

			previous_line_had_emote = line_has_emote;
			previous_line_checked = false;
			// The padding for the first line is handled when drawing the chat log.
			if (!check_for_emotes) {
				check_for_emotes = true;
			} 
			
			if(check_for_emotes) {
				line_has_emote = false;
			}

			cy += fontspec.lfh;
			// Look-ahead
			if(doesNextLineHaveEmote(count, lines_with_emotes)) {
				// CONS_Printf("[D]: emote on next line %d\n", count+1);

				if (previous_line_had_emote) {
					cy += __PADDING + __PADDING;
				} else {
					cy += __PADDING;
				}
				line_has_emote = true;
			}

			count++;

			if (cy >= bot)
				return;
			cx = x;
			break;
        case '\x01':
            // Emote logic
            if (cx < right) {
				auto emote_fetch = emote_fetch_map[log_type]; 

                if (emote_fetch(chat_log_index, emote_tracker_idx) != nullptr) {
					// CONS_Printf("yo\n");

					if (check_for_emotes && line_has_emote == false)
					{
						line_has_emote = true; 
						cy += __PADDING + __PADDING;
					}

                    emote_t* emote = emote_fetch(chat_log_index, emote_tracker_idx);

					fixed_t emote_y = cy - __PADDING;

					if (emote->atlas_id != -1) {
						float scale = GET_CHAT_EMOTE_SCALE(
							EMOTE_ATLASES[emote->atlas_id]->height, contains_text
						);

						patch_t* atlas_patch = static_cast<patch_t*>(W_CachePatchNum(
							getChatEmoteFrame(emote), PU_HUDGFX
						));

						emote_atlas_coordinates_t coords = getEmoteAtlasCoordinates(emote, scale); 

						fixed_t scaled_width = FloatToFixed(scale * EMOTE_ATLASES[emote->atlas_id]->width);
						if (emote->atlas_column == EMOTE_ATLASES[emote->atlas_id]->columns-1) {
							scaled_width += (2*FRACUNIT);
						}
						fixed_t scaled_height = FloatToFixed(scale * EMOTE_ATLASES[emote->atlas_id]->height);
						if (bottom_offset != 0) {
							if ((emote_y/FRACUNIT) + scaled_height/FRACUNIT > bottom_offset) {
								int temp_height = scaled_height/FRACUNIT;
								int diff = (temp_height - (bottom_offset - emote_y/FRACUNIT));
								scaled_height -= (diff-2) * FRACUNIT;
							}
						}

						fixed_t y_offset = 0;
						if (top_offset != 0) {
							if ((emote_y/FRACUNIT) < top_offset) {
								// CONS_Printf("%d y, %d\n", emote_y/FRACUNIT, top_offset);
								int temp_height = scaled_height/FRACUNIT;
								fixed_t diff = (top_offset - emote_y/FRACUNIT) * FRACUNIT;
								
								if (diff < scaled_height) {
									y_offset = diff;
									scaled_height = (temp_height*FRACUNIT) - diff;
								} else {
									scaled_height = 0;
								}
							}
						}
						V_DrawCroppedEmotePatch(
							cx,
							emote_y,
							FloatToFixed(scale),
							V_SNAPTOBOTTOM | V_SNAPTOLEFT|flags,
							atlas_patch,
							coords.x,
							coords.y,
							scaled_width,
							scaled_height,
							y_offset
						);
					} else {
						patch_t* emote_patch = static_cast<patch_t*>(W_CachePatchNum(
							getChatEmoteFrame(emote), PU_HUDGFX
						));
	
						fixed_t scale = FloatToFixed(GET_CHAT_EMOTE_SCALE(emote_patch->height, contains_text));
						
						V_DrawStretchyFixedPatch(
							cx,
							emote_y,
							scale,
							scale,
							flags,
							emote_patch,
							NULL
						);
					}
                    
                    INT32 width = getEmotePatchWidthDirectly(emote, contains_text);
                    cw = SHORT(width) * dupx;

                    // CONS_Printf("[DRAW] font space width %d\n", fontspec.spacew/FRACUNIT);
                    cx += cw;

                    line_has_emote = true;
                    emote_tracker_idx++;
                }
            } else {
				CONS_Printf("uh oh\n");
			}
            break;
		default:
			if ((c & 0xF0) == 0x80)
			{
				if (notcolored)
				{
					colormap = V_GetStringColormap(((c & 0x7f) << V_CHARCOLORSHIFT) & V_CHARCOLORMASK);
				}
				if (nodanceoverride)
				{
					dance = false;
				}
			}
			else if (cx < right)
			{
				if (uppercase)
				{
					c = toupper(c);
				}
				else if (V_CharacterValid(font, c - font->start) == false)
				{
					// Try the other case if it doesn't exist
					if (c >= 'A' && c <= 'Z')
					{
						c = tolower(c);
					}
					else if (c >= 'a' && c <= 'z')
					{
						c = toupper(c);
					}
				}

				c -= font->start;

				if (check_for_emotes && line_has_emote == false)
				{
					check_for_emotes = false; 
					if (previous_line_had_emote) {

						cy += __PADDING + __PADDING;
						previous_line_had_emote = false;
						previous_line_checked = true;
					}
				}
				if (V_CharacterValid(font, c) == true)
				{
					// Remove offsets from patch
					fixed_t patchxofs = SHORT(font->font[c]->leftoffset) * dupx * scale;
					cw = SHORT(font->font[c]->width) * dupx;
					cxoff = (*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);
					V_DrawFixedPatch(cx + cxoff + patchxofs, cy + cyoff, scale, flags, font->font[c], colormap);
					cx += cw;
				}
				else
					cx += fontspec.spacew;
			}
		}
	}
}

fixed_t V_RR_StringScaledWidth(
		fixed_t      scale,
		fixed_t spacescale,
		fixed_t    lfscale,
		INT32      flags,
		int        fontno,
		const char *s,
		int chat_log_index,
		boolean contains_text = true
)
{
	/**
     * RADIO: Yeah, here.
     */
    size_t emote_tracker_idx = 0;

	INT32     hchw;/* half-width for centering */

	INT32     dupx;

	font_t   *font;

	boolean uppercase;

	fixed_t cx;
	fixed_t right;

	fixed_t cw;

	int c;

	fixed_t fullwidth = 0;

	uppercase  = ((flags & V_FORCEUPPERCASE) == V_FORCEUPPERCASE);
	flags	&= ~(V_FLIP);/* These two (V_FORCEUPPERCASE) share a bit. */

	font       = &fontv[fontno];

	fontspec_t fontspec;

	V_GetFontSpecification(fontno, flags, &fontspec);

	hchw     = fontspec.chw >> 1;

	fontspec.chw    <<= FRACBITS;
	fontspec.spacew <<= FRACBITS;

#define Mul( id, scale ) ( id = FixedMul (scale, id) )
	Mul    (fontspec.chw,      scale);
	Mul (fontspec.spacew,      scale);
	Mul    (fontspec.lfh,      scale);

	Mul (fontspec.spacew, spacescale);
	Mul    (fontspec.lfh,    lfscale);
#undef  Mul

	if (( flags & V_NOSCALESTART ))
	{
		dupx      = vid.dupx;

		hchw     *=     dupx;

		fontspec.chw      *=     dupx;
		fontspec.spacew   *=     dupx;
		fontspec.lfh      *= vid.dupy;
	}
	else
	{
		dupx      = 1;
	}

	cx = 0;
	right = 0;

	for (; ( c = *s ); ++s)
	{
		switch (c)
		{
			case '\n':
				cx  =   0;
				break;
			case '\x01':
				// Emote logic
				if (emote_fetch_map[chat_log_type_t::CLT_MINI](chat_log_index, emote_tracker_idx) != nullptr) {

					emote_t* emote = emote_fetch_map[chat_log_type_t::CLT_MINI](chat_log_index, emote_tracker_idx);

                    INT32 width = getEmotePatchWidthDirectly(emote, contains_text);
                    cw = SHORT(width) * dupx;

					cx += cw;

					emote_tracker_idx++;
				}
				break;
			default:
				if (( c & 0xF0 ) == 0x80 || c == V_STRINGDANCE)
					continue;

				if (( c & 0xB0 ) & 0x80)
				{
					cw = V_GetButtonCodeWidth(c) * dupx;
					cx += cw * scale;
					right = cx;
					break;
				}

				if (uppercase)
				{
					c = toupper(c);
				}
				else if (V_CharacterValid(font, c - font->start) == false)
				{
					// Try the other case if it doesn't exist
					if (c >= 'A' && c <= 'Z')
					{
						c = tolower(c);
					}
					else if (c >= 'a' && c <= 'z')
					{
						c = toupper(c);
					}
				}

				c -= font->start;
				if (V_CharacterValid(font, c) == true)
				{
					cw = SHORT (font->font[c]->width) * dupx;

					// How bunched dims work is by incrementing cx slightly less than a full character width.
					// This causes the next character to be drawn overlapping the previous.
					// We need to count the full width to get the rightmost edge of the string though.
					right = cx + (cw * scale);

					(*fontspec.dim_fn)(scale, fontspec.chw, hchw, dupx, &cw);
					cx += cw;
				}
				else
					cx += fontspec.spacew;
		}

		fullwidth = std::max(right, std::max(cx, fullwidth));
	}

	return fullwidth;
}