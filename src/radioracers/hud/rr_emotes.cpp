// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/hud/rr_emotes.cpp
/// \brief Emote functionality in the game's HUD.

#include <iostream>
#include <regex>
#include <string>

#include "../rr_hud.h"
#include "../rr_video.h"
#include "../rr_setup.h"
#include "../../z_zone.h"
#include "../../v_video.h"
#include "../../hu_stuff.h"
#include "../../d_clisrv.h"
#include "../../deh_soc.h"
#include "../../s_sound.h"

#define EMOTE_MENU_QUERY_SIZE 26 // query + '_'
#define ROWS 5
#define COLUMNS 5

// For the chat window
std::string emote_menu_query;
size_t emote_menu_page = 1;
std::vector<emote_t*> menu_emotes;
std::vector<emote_t*> menu_search_emotes;
size_t emote_menu_selection = 0;
boolean is_emote_menu_on = false;
std::vector<emote_t*> all_the_emotes;

boolean is_emote_preview_on = false;
std::string emote_search_query;
size_t emote_search_query_select = 0; 
std::vector<emote_t*> emote_search_results;

// Track chat_log array from hu_stuff.c so we don't have to keep recalculating stuff.
std::vector<std::vector<patch_t*>> chat_log_emote_patches;
std::vector<std::vector<patch_t*>> chat_mini_log_emote_patches;
std::vector<std::vector<emote_t*>> chat_log_emotes;
std::vector<std::vector<emote_t*>> chat_mini_log_emotes;

typedef struct
{
    std::string message;
    boolean contains_text;
    std::vector<patch_t*> found_emote_patches;
    std::vector<emote_t*> found_emotes;
    std::vector<int> found_emote_atlases;
} parse_message_results_t;

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

static boolean isValid(const char* msg)
{
    for (int i = 0; msg[i] != '\0'; i++) {
        if (V_CharacterValidEx(&fontv[HU_FONT], msg[i] - fontv[HU_FONT].start))
            return true;
    }
    return false;  // If we reach here, all characters are valid
}

static parse_message_results_t ParseMessageForEmotes(const char* message, int offset)
{
    std::string message_std = std::string(message);

    // You can bypass the max length of a message by pasting in the chat input
    if (message_std.length() >= HU_MAXMSGLEN) {
        message_std = message_std.substr(0, HU_MAXMSGLEN-2);
    }
    
    parse_message_results_t result;
    result.contains_text = true;

    /**
     * Match any word between two colons (e.g. :joy:)
     * 
     * loop starts at the first iteration of a colon,
     * keeps iterating until it hits the end of the line,
     * "start" resets until it finds the next instance of a colon
     * 
     * Example:
     * 
     * Hello :wave:, how are you?
     *       ^    ^
     * 1. start is at first colon (6), and loop iterates until it hits the NEXT instance of a colon (11)
     * 2. on next iteration of loop, start = 11 (where the last colon was):
     * 
     * Hello :wave:, how are you?
     *            ^   
     * 3. however, there are no more iterations of the next colon.
     *  so, message_std.find will return std::string::npos.
     *  the loop will iterate as long as it doesn't hit std::string::npos.
     */
    for (
            size_t start = message_std.find(":"); 
            start != std::string::npos; 
            start = message_std.find(":", start + 1)
        ) {
        size_t end = message_std.find(":", start + 1);

        // Reached the end of the message?
        if (end == std::string::npos)
            break;
        
        std::string emote_name = message_std.substr(start + 1, end - start - 1);

        if (EMOTES[emote_name])
        {
            /**
             * Found an emote. Now replace it with a placeholder character 
             * ('\x01' + the hypothetical width of the emote)
             */

            emote_t* temp_emote = EMOTES[emote_name];
            if (temp_emote->atlas_id == -1) {
                patch_t* temp_patch = static_cast<patch_t*>(W_CachePatchNum(temp_emote->frames[0], PU_CACHE));
                result.found_emote_patches.push_back(temp_patch);
            } else {
                result.found_emote_patches.push_back(nullptr);
            }
            result.found_emote_atlases.push_back(temp_emote->atlas_id);
            result.found_emotes.push_back(temp_emote);

            message_std.replace(start, end - start + 1, "\x01");
        }
    }

    result.contains_text = isValid(message_std.c_str() + offset);

    // NOW, replace the \x01 placeholders, all these fucking loops
    size_t idx = 0;
    for (
        size_t start = message_std.find("\x01"); 
        start != std::string::npos; 
        start = message_std.find("\x01", start + 1)
    ) {
        emote_t* temp_emote = result.found_emotes[idx];
        
        INT32 width = -1;
        if (temp_emote->atlas_id != -1) {
            width = getAtlasEmotePatchWidth(temp_emote->atlas_id, result.contains_text);
        } else {
            patch_t* temp_patch = result.found_emote_patches[idx];
            width = getEmotePatchWidth(temp_patch, result.contains_text);
        }
        
        // Get number of blank spaces to add
        float blank_spaces = ceil(static_cast<float_t>(width) / 2.f);

        // if (start > 0 && message_std[start-1] != ' ')
        // {
        //     message_std.replace(start, 1, " \x01");
        //     start++;
        // }

        // Only add the padding if the player explicitly adds more than whitespace character after the emote
        // if (start + 1 < message_std.size() && message_std[start+1] == ' ') {
        //     blank_spaces -= 1.f;
        // }
        
        message_std.replace(start, 1, "\x01" + std::string(static_cast<size_t>(blank_spaces), ' '));
        idx++;
    }

    result.message = message_std;

    return result;
}

static word_wrap_results_t word_wrap(
    INT32 w, 
    fixed_t scale, 
    INT32 option, 
    const char* string,
    size_t log_index,
    int offset,
    std::vector<std::vector<patch_t*>> *patch_table,
    std::vector<std::vector<emote_t*>> *emote_table
)
{
    // Replace any emote identifiers (if there are any) with '\x01'. It'll act as a placeholder.
    parse_message_results_t result = ParseMessageForEmotes(string, offset);
    
    patch_t** found_emote_patches = (result.found_emote_patches.empty()) ? nullptr : result.found_emote_patches.data();
    emote_t** found_emotes = (result.found_emotes.empty()) ? nullptr : result.found_emotes.data();

    if (found_emotes != nullptr)
    {
        if ((*patch_table)[log_index].empty()) 
        {
            (*patch_table)[log_index] = result.found_emote_patches;
            (*emote_table)[log_index] = result.found_emotes;
        }
    }

    word_wrap_results_t results = V_RR_ScaledWordWrap(
        w << FRACBITS,
        scale, FRACUNIT, FRACUNIT,
        option,
        HU_FONT,
        strdup(result.message.c_str()),
        found_emote_patches,
        result.found_emote_atlases.data(),
        result.contains_text
    );
    
    return results;
}

/**
 * When drawing the mini chat, the original code wraps the text twice.
 * Need to perform the same functionality as word_wrap, but this is just being used for reference.
 */
static word_wrap_results_t just_word_wrap(
    INT32 w, 
    fixed_t scale, 
    INT32 option, 
    int offset,
    const char* string
)
{
    // Replace any emote identifiers (if there are any) with '\x01'. It'll act as a placeholder.
    parse_message_results_t result = ParseMessageForEmotes(string, offset);
    patch_t** found_emote_patches = (result.found_emote_patches.empty()) ? nullptr : result.found_emote_patches.data();

    word_wrap_results_t results = V_RR_ScaledWordWrap(
        w << FRACBITS,
        scale, FRACUNIT, FRACUNIT,
        option,
        HU_FONT,
        strdup(result.message.c_str()),
        found_emote_patches,
        result.found_emote_atlases.data(),
        result.contains_text
    );

    return results;
}

static word_wrap_results_t RR_CHAT_Just_Word_Wrap(INT32 w, fixed_t scale, INT32 option, const char *string, size_t chat_mini_log_index)
{
    return just_word_wrap(w, scale, option, chat_mini_log_offset[chat_mini_log_index], string);
}

static word_wrap_results_t RR_CHAT_Mini_WordWrap(INT32 w, fixed_t scale, INT32 option, const char *string, size_t chat_mini_log_index)
{
    return word_wrap(w, scale, option, string, chat_mini_log_index, chat_mini_log_offset[chat_mini_log_index], &chat_mini_log_emote_patches, &chat_mini_log_emotes);
}

static word_wrap_results_t RR_CHAT_WordWrap(INT32 w, fixed_t scale, INT32 option, const char *string, size_t chat_log_index)
{
    return word_wrap(w, scale, option, string, chat_log_index, chat_log_offset[chat_log_index], &chat_log_emote_patches, &chat_log_emotes);
}

void RR_UpdateEmoteChatLogs(UINT32 chat_mini_log, UINT32 chat_log)
{
    // CONS_Printf("[%d]: Initialize emote chat log\n", chat_log);

    if (chat_log >= chat_log_emotes.size())
        chat_log_emotes.resize(chat_log+1);
    
    if (chat_log >= chat_log_emote_patches.size())
        chat_log_emote_patches.resize(chat_log+1);
    
    chat_log_emotes[chat_log].clear();
    chat_log_emote_patches[chat_log].clear();

    // CONS_Printf("[%d]: Initialize emote MINI chat log\n", chat_mini_log);
    
    if (chat_mini_log >= chat_mini_log_emotes.size())
        chat_mini_log_emotes.resize(chat_mini_log+1);
    
    if (chat_mini_log >= chat_mini_log_emote_patches.size())
        chat_mini_log_emote_patches.resize(chat_mini_log+1);
    
    chat_mini_log_emotes[chat_mini_log].clear();
    chat_mini_log_emote_patches[chat_mini_log].clear();
}

void RR_RemoveEmoteChatLog(UINT32 chat_log)
{
    if (chat_log == 0)
        return;
    
    for (size_t i = 0; i < chat_log - 1; i++) {
        chat_log_emotes[i] = chat_log_emotes[i + 1]; // Shift emote logs
        chat_log_emote_patches[i] = chat_log_emote_patches[i + 1]; // Shift emote patches
    }

    chat_log_emotes[chat_log-1].clear();
    chat_log_emote_patches[chat_log-1].clear();
}

void RR_RemoveEmoteChatMiniLog(UINT32 chat_mini_log)
{
    if (chat_mini_log == 0)
        return;
    
    for (size_t i = 0; i < chat_mini_log - 1; i++) {
        chat_mini_log_emotes[i] = chat_mini_log_emotes[i + 1]; // Shift emote logs
        chat_mini_log_emote_patches[i] = chat_mini_log_emote_patches[i + 1]; // Shift emote patches
    }
    
    chat_mini_log_emotes[chat_mini_log-1].clear();
    chat_mini_log_emote_patches[chat_mini_log-1].clear();
}

emote_t* RR_GetEmoteFromChatLog(size_t chat_log_index, int emote_tracer_index) 
{
    if (chat_log_emotes[chat_log_index].empty()) {
        return nullptr;
    } else {
        return (chat_log_emotes[chat_log_index][emote_tracer_index]);
    }
}

emote_t* RR_GetEmoteFromChatMiniLog(size_t chat_log_mini_index, int emote_tracer_index)
{
    if (chat_mini_log_emotes[chat_log_mini_index].empty()) {
        return nullptr;
    } else {
        return (chat_mini_log_emotes[chat_log_mini_index][emote_tracer_index]);
    }
}

std::unordered_map<chat_log_type_t, std::function<emote_t*(size_t, int)>> emote_fetch_map = {
    {chat_log_type_t::MAIN, RR_GetEmoteFromChatLog},
    {chat_log_type_t::MINI, RR_GetEmoteFromChatMiniLog}
};

/** 
 * Parts of HU_drawChatLog and HU_drawMiniChat copied over from hu_stuff.c
*/
#define EMOTE_PADDING_CONST EMOTE_LINE_PADDING_INT + 1
#define EMOTE_PADDING_CONST_TEXT EMOTE_LINE_PADDING_TEXT_INT + 1

// Lots of edge-cases to check for
typedef struct 
{
    boolean line_has_emote = false;
    boolean line_checked_for_emote = false;
    boolean previous_line_had_emote = false;
    boolean previous_line_checked = false;
    boolean had_at_least_one_emote = false;
} emote_flags_t;

static INT32 Parse_ChatLog_Message(
    emote_flags_t &flags,
    INT32& linescount,
    INT32& dy,
    INT32& startj,
    INT32 charheight,
    INT32 y,
    INT32 chat_topy,
    INT32 chat_bottomy,
    char* msg,
    INT32 emote_padding
)
{
    INT32 j = 0;

    boolean& line_has_emote = flags.line_has_emote;
    boolean& line_checked_for_emote = flags.line_checked_for_emote;
    boolean& previous_line_had_emote = flags.previous_line_had_emote;
    boolean& previous_line_checked = flags.previous_line_checked;
    boolean& had_at_least_one_emote = flags.had_at_least_one_emote;

    INT32 emotelinescount = 0;
    for (; msg[j]; j++) // iterate through msg
    {
        if (msg[j] == '\x01')
        {				
            if (!line_checked_for_emote) {
                if(!line_has_emote) {
                    previous_line_had_emote = false;
                    line_has_emote = true;
                    line_checked_for_emote = true;

                    had_at_least_one_emote = true;

                    // for every line with an emote, add another line
                    if (!previous_line_checked)
                        linescount++;
                    emotelinescount++;
                }
            }
        } else {
            if (previous_line_had_emote) {
                linescount++;
                previous_line_had_emote = false;
                previous_line_checked = true;
            }
        }
        
        if (msg[j] != '\n') // get back down.
            continue;

        if (y + dy >= chat_bottomy)
            ;
        else if (line_has_emote)
        {
            previous_line_had_emote = line_has_emote;
            previous_line_checked = false;
            line_has_emote = false;
            line_checked_for_emote = false;	

            // emote_padding_y += emote_padding;
            
            // CONS_Printf("TOP_Y: %d // %d\n", chat_topy, y + dy + 2 + emote_padding);
            
            if(y + dy + emote_padding + charheight < chat_topy) {					
                if (y + dy + emote_padding + charheight >= chat_topy) 
                {
                    dy += charheight;
                    startj = j;
                }
            }
        }
        else if (y + dy + 2 + charheight < chat_topy)
        {
            dy += charheight;

            if (y + dy + 2 + charheight >= chat_topy)
            {
                startj = j;
            }

            continue;
        }

        linescount++;
    }

    return emotelinescount;
}

INT32 RR_Parse_ChatLog(chat_log_parameters_t parameters)
{
    INT32 boxw = parameters.boxw;
    INT32 charheight = parameters.charheight;
	INT32 x = parameters.x, y = parameters.y;
	INT32 chat_topy = parameters.chat_topy, chat_bottomy = parameters.chat_bottomy;
    UINT32 chat_nummsg_log = parameters.chat_nummsg_log;
    INT32 flags = parameters.flags;

    fixed_t scale = parameters.scale;    
    INT32 dy = 0;

    for (UINT32 i = 0; i < chat_nummsg_log; i++)
    {
        word_wrap_results_t results = RR_CHAT_WordWrap(
            boxw-4,
            scale,
            flags,
            parameters.chat_log[i],
            i
        );
        char *msg = results.msg;
        
        INT32 startj = 0;
        INT32 linescount = 1;
        emote_flags_t emote_flags; // Flags to check for when parsing the message

        INT32 emote_padding = results.contains_text ? EMOTE_PADDING_CONST_TEXT : EMOTE_PADDING_CONST;
        INT32 emotelines = Parse_ChatLog_Message(emote_flags, 
            linescount, dy,
            startj, charheight,
            y, chat_topy, chat_bottomy,
            msg, emote_padding
        );

        if (y + dy < chat_bottomy)
        {
            V_RR_DrawStringScaled(
                (x + 2) << FRACBITS,
				(y + dy + ((emote_flags.had_at_least_one_emote || emote_flags.line_has_emote) ? emote_padding : 2)) << FRACBITS,
				scale, FRACUNIT, FRACUNIT,
				flags,
				NULL,
				HU_FONT,
				msg+startj,
				static_cast<int>(i),
				false,
                results.contains_text,
                chat_topy,
                chat_bottomy
			);
        }

        dy += (charheight * linescount);
        if (emotelines > 1 && results.contains_text)
        {
            dy -= (charheight * 1);
        }

        if (msg)
            Z_Free(msg);
    }

    return dy;
}

INT32 RR_Parse_ChatMiniLog_For_Lines(
    size_t i, 
    fixed_t scale,
    INT32 boxw
)
{
    INT32 msglines = 0;
    size_t reverse_i = 0;
	for (; i > 0; i--)
	{
		// char *msg = CHAT_WordWrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_mini[i-1]);
        word_wrap_results_t results = RR_CHAT_Just_Word_Wrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_mini[i-1], i-1);
		char *msg = results.msg;
		size_t j = 0;
		INT32 linescount = 1;

		boolean line_has_emote = false;
		boolean line_checked_for_emote = false;

		for (; msg[j]; j++) // iterate through msg
		{
			if (msg[j] == '\x01')
			{				
				if (!line_checked_for_emote) {
					if(!line_has_emote) {
						line_has_emote = true;
						line_checked_for_emote = true;

						// for every line with an emote, add another line
						linescount++;
					}
				}
			}

			if (msg[j] != '\n') // get back down.
				continue;

			if (msg[j] == '\n')
			{
				line_has_emote = false;
				line_checked_for_emote = false;
			}

			linescount++;
		}

		msglines += linescount;

		if (msg)
			Z_Free(msg);

		reverse_i++;
	}

    return msglines;
}

void RR_Draw_ChatMiniLog(chat_mini_log_parameters_t parameters) {
    INT32 boxw = parameters.boxw;
    INT32 charheight = parameters.charheight;
	INT32 *x = parameters.x, *y = parameters.y;
    UINT32 chat_nummsg_min = parameters.chat_nummsg_min;

    fixed_t scale = parameters.scale;
    size_t i = 0;

    for (; i<=(chat_nummsg_min-1); i++) // iterate through our hot messages
	{
        INT32 timer = ((cv_chattime.value*TICRATE)-chat_timers[i]) - cv_chattime.value*TICRATE+9; // see below...
        INT32 transflag = (timer >= 0 && timer <= 9) ? (timer*V_10TRANS) : 0; // you can make bad jokes out of this one.
		size_t j = 0;
        
        word_wrap_results_t results = RR_CHAT_Mini_WordWrap(boxw-4, scale, V_SNAPTOBOTTOM|V_SNAPTOLEFT, chat_mini[i], i); // get the current message, and word wrap it.
		char *msg = results.msg;

		INT32 linescount = 1;

        // Yeah
		boolean line_has_emote = false;
		boolean line_checked_for_emote = false;
		boolean previous_line_had_emote = false;
		boolean previous_line_checked = false;
		boolean had_at_least_one_emote = false;

		int emote_lines = 0;
		for (; msg[j]; j++) // iterate through msg
		{
			if (msg[j] == '\x01')
			{				
				if (!line_checked_for_emote) {
					if(!line_has_emote) {
						previous_line_had_emote = false;
						line_has_emote = true;
						line_checked_for_emote = true;

						had_at_least_one_emote = true;

						// for every line with an emote, add another line
						if (!previous_line_checked)
							linescount++;

						emote_lines++;
					}
				}
			} else {				
				if (previous_line_had_emote) {
					// extra_y_padding += 4;
					linescount++;
					previous_line_had_emote = false;
					previous_line_checked = true;
				}
			}

			if (msg[j] != '\n') // get back down.
				continue;

			if (msg[j] == '\n')
			{
				previous_line_had_emote = line_has_emote;
				previous_line_checked = false;
				line_has_emote = false;
				line_checked_for_emote = false;	
			}

			linescount++;
		}

		if (cv_chatbacktint.value) // on request of wolfy
		{
			INT32 width = V_RR_StringWidth(msg, 0, i, results.contains_text);
			if (vid.width >= 640)
				width /= 2;		

            INT32 final_height = charheight * linescount;
			V_DrawFillConsoleMap(
				(*x)-2, (*y),
				width+4,
				final_height,
				159|V_SNAPTOBOTTOM|V_SNAPTOLEFT
			);
		}

        int y_padding = (results.contains_text) ? EMOTE_PADDING_CONST_TEXT+1 : EMOTE_PADDING_CONST;
		V_RR_DrawStringScaled(
            (*x) << FRACBITS,
			((*y) + ((had_at_least_one_emote || line_has_emote) ? (y_padding) : 0)) << FRACBITS,
			scale, FRACUNIT, FRACUNIT,
			V_SNAPTOBOTTOM|V_SNAPTOLEFT|transflag,
			NULL,
			HU_FONT,
			msg,
			i,
			true,
            results.contains_text,
            0,
            0
		);
        
		(*y) += charheight * linescount;

		if (msg)
			Z_Free(msg);
	}
}

/** Chat stuff */

static void search_for_emotes(void)
{
    // fuzzy search
    emote_search_results.clear();
    
    for (const auto& temp_emote : EMOTES) {
        if (emote_search_results.size() >= 15) {
            break;
        }
        if (temp_emote.first.find(emote_search_query) != std::string::npos) {
            if (temp_emote.second != nullptr) {
                emote_search_results.push_back(temp_emote.second);
            }
        }
    }

    emote_search_query_select = 0;
}

static void update_query_add(INT32 c, std::string *query)
{
    if (query->size() >= EMOTE_MENU_QUERY_SIZE)
        return;
    
    query->push_back(static_cast<char>(c));
}

static void update_query_delete(std::string *query)
{
    if (query->empty())
        return;
    
    query->pop_back();
}

static void update_emote_query_add(INT32 c)
{
    if (emote_search_query.size() >= EMOTE_MENU_QUERY_SIZE)
        return;
    
    emote_search_query.push_back(static_cast<char>(c));
}

static void update_emote_query_delete(void)
{
    if (emote_search_query.empty())
        return;
    
    emote_search_query.pop_back();
}

void RR_ResetEmoteSearchQuery(void)
{
    emote_search_query.clear();
    is_emote_preview_on = false;
    emote_search_query_select = 0;
    emote_search_results.clear();
}

void RR_ResetEmoteMenuInfo(void)
{
    emote_menu_query.clear();
    menu_emotes.clear();
    menu_search_emotes.clear();
    emote_menu_selection = 0;
    emote_menu_page = 1;
    is_emote_menu_on = false;
}

void RR_ResetAllEmoteChatInfo(void)
{
    RR_ResetEmoteSearchQuery();
    RR_ResetEmoteMenuInfo();
}

void RR_CheckChatDeleteForEmotePreview(void)
{
    if (is_emote_menu_on) {
        return;
    }
    if(is_emote_preview_on)
    {
        if(c_input < 0) {
            return;
        }
        if (c_input - 2 < 0 || strlen(w_chat) == 0) {
            return;
        }
        if (w_chat[c_input-2] == ':') {
            RR_ResetEmoteSearchQuery();
        } else {
            update_emote_query_delete();           
            search_for_emotes();
        }
    } 
}

void RR_CheckChatInputForEmotePreview(INT32 c) {
    if(is_emote_menu_on) {
        return;
    }
    if(!is_emote_preview_on) {
        if(c_input < 0 || strlen(w_chat) == 0) {
            return;
        }
        if (w_chat[c_input-1] == ':' && c != ':' && c != ' ' ) {
            is_emote_preview_on = true;
            update_emote_query_add(c);
            search_for_emotes();
        }
    } else {
        if(c_input < 0) {
            return;
        }
        if (c == ':' || c == ' ') {
            RR_ResetEmoteSearchQuery();
        } else {
            update_emote_query_add(c);
            search_for_emotes();
        }
    }
}

static void draw_atlas_emote(emote_t* emote, INT16 x, INT16 y, INT32 scale)
{
    float scale_f = static_cast<float>(scale);

    patch_t* emote_patch = static_cast<patch_t*>(W_CachePatchNum(
        getChatEmoteFrame(emote), PU_HUDGFX
    ));

    fixed_t final_scale = FloatToFixed(std::min(
        scale_f/EMOTE_ATLASES[emote->atlas_id]->width, scale_f/EMOTE_ATLASES[emote->atlas_id]->height
    ));
    
    INT16 scaled_width = FixedToFloat(final_scale) * EMOTE_ATLASES[emote->atlas_id]->width;
    INT16 scaled_height = FixedToFloat(final_scale) * EMOTE_ATLASES[emote->atlas_id]->height;

    INT32 offset = (scaled_height < scale) ? (scale - scaled_height)/2 : 0;

    emote_atlas_coordinates_t coords = getEmoteAtlasCoordinates(emote, FixedToFloat(final_scale)); 

    V_DrawCroppedPatch(
        (x) << FRACBITS,
        (y + offset) << FRACBITS,
        final_scale,
        V_SNAPTOBOTTOM | V_SNAPTOLEFT,
        emote_patch,
        coords.x,
        coords.y,
        scaled_width * FRACUNIT,
        scaled_height * FRACUNIT
    );
}

static void draw_normal_emote(emote_t* emote, INT16 x, INT16 y, INT32 scale)
{
    float scale_f = static_cast<float>(scale);

    patch_t* emote_patch = static_cast<patch_t*>(W_CachePatchNum(
        getChatEmoteFrame(emote), PU_HUDGFX
    ));

    fixed_t final_scale = FloatToFixed(std::min(
        scale_f/emote_patch->width, scale_f/emote_patch->height
    ));

    int final_patch_height = FixedToFloat(final_scale) * emote_patch->height;
    int final_patch_width = FixedToFloat(final_scale) * emote_patch->width;
    INT32 offset_h = (final_patch_height < scale) ? (scale - final_patch_height)/2 : 0;
    INT32 offset_w = (final_patch_width < scale) ? (scale - final_patch_width)/2 : 0;

    V_DrawStretchyFixedPatch(
        (x + offset_w) << FRACBITS,
        (y + offset_h) << FRACBITS,
        final_scale,
        final_scale,
        V_SNAPTOBOTTOM | V_SNAPTOLEFT,
        emote_patch,
        NULL
    );
}

void RR_UpdateEmoteQueryChoice_Left(void) {
    if (emote_search_results.empty())
        return;
    
    if (emote_search_query_select == 0) {
        emote_search_query_select = emote_search_results.size() - 1;
    } else {
        emote_search_query_select--;
    }
}

void RR_UpdateEmoteQueryChoice_Right(void) {    
    if (++emote_search_query_select >= emote_search_results.size()) {
        emote_search_query_select = 0;
    }
}

void RR_SelectEmoteFromPreview(void)
{
    if (emote_search_results.empty()) {
        return;
    }

    // Replace the whole query in the chat input with the emote we've found
    std::string chosen_emote_name = emote_search_results[emote_search_query_select]->name;
    std::string final_emote_name = chosen_emote_name.append(": ");
    const char* final_emote_name_c = final_emote_name.c_str();

    // Reset to just where the query started (which should be right after the colon)
    size_t start = c_input - emote_search_query.size();

    if (start < 0) {
        // CONS_Printf("wtf, you shouldn't be less than 0\n");
        RR_ResetEmoteSearchQuery();
        return;
    }

    memmove(&w_chat[start], &w_chat[c_input], strlen(&w_chat[c_input]) + 1);
    c_input = start;

    size_t chat_length = strlen(w_chat);
    
    size_t final_length = strlen(final_emote_name_c);

    if (chat_length + final_length > HU_MAXMSGLEN) {
        RR_ResetEmoteSearchQuery();
        return;
    }

    memmove(&w_chat[c_input + final_length], &w_chat[c_input], (chat_length - c_input) + 1);
    memcpy(&w_chat[c_input], final_emote_name_c, final_length);

    c_input += final_length;

    RR_ResetEmoteSearchQuery();
}

void RR_DrawChatEmotePreview(
    INT16 x,
    INT16 y,
    INT32 w
)
{
    if (!is_emote_preview_on)
        return;
    
    const INT32 PREVIEW_HEIGHT = 10;

    if (emote_search_results.empty()) {
        V_DrawFillConsoleMap(
            x,
            y,
            w,
            PREVIEW_HEIGHT - 3,
            V_SNAPTOBOTTOM | V_SNAPTOLEFT
        );

        V_DrawStringScaled(
            (x + 1) << FRACBITS,
            (y + 1) << FRACBITS,
            FRACUNIT/2,
            FRACUNIT,
            FRACUNIT,
            V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
            NULL,
            TINY_FONT,
            "* No emotes found."
        );
    } else {
        V_DrawFillConsoleMap(
            x,
            y,
            w,
            PREVIEW_HEIGHT,
            V_SNAPTOBOTTOM | V_SNAPTOLEFT
        );
        
        V_DrawFillConsoleMap(
            x,
            y + PREVIEW_HEIGHT + 1,
            w,
            5,
            V_SNAPTOBOTTOM | V_SNAPTOLEFT
        );
        
        INT16 temp_x = x;
        for (size_t i = 0; i < emote_search_results.size(); i++) {
            emote_t* preview_emote = emote_search_results[i];

            // This shouldn't even be empty?
            if(!preview_emote) {
                continue;
            }

            if (preview_emote->atlas_id != -1) {
                draw_atlas_emote(preview_emote, temp_x+1, y+1, 8);
            } else {
                draw_normal_emote(preview_emote, temp_x+1, y+1, 8);
            }
    
            if (i == emote_search_query_select) {
                V_DrawStringScaled(
                    (temp_x) << FRACBITS,
                    (y + 1) << FRACBITS,
                    FRACUNIT/2,
                    FRACUNIT,
                    FRACUNIT,
                    V_GREENMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
                    NULL,
                    TINY_FONT,
                    "*"
                );

                V_DrawStringScaled(
                    (x + 1) << FRACBITS,
                    (y + PREVIEW_HEIGHT + 1) << FRACBITS,
                    FRACUNIT/2,
                    FRACUNIT,
                    FRACUNIT,
                    V_GREENMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
                    NULL,
                    TINY_FONT,
                    preview_emote->name
                );
            }
            temp_x += 10;
        }
    }
}

/**
 * RADIO: Emote menu
 */

 static std::vector<emote_t*>* get_current_emote_list()
 {
    // If we're querying an emote AND there AREN'T any results ..
    if (menu_search_emotes.empty() && !emote_menu_query.empty()) {
        return nullptr;
    } else {
        // .. but if we're NOT searching for anything
        return (menu_search_emotes.empty()) ? &EMOTES_VECTOR : &menu_search_emotes;
    }
 }
 static size_t get_max_emotes_on_menu_page() {
    std::vector<emote_t*> *current_emotes = get_current_emote_list();

    if (!current_emotes)
        return 0;

    if(current_emotes->size() <= ROWS*COLUMNS)
        return current_emotes->size();

    return std::min(
        ROWS*COLUMNS, signed(current_emotes->size() - (emote_menu_page * ROWS*COLUMNS))
    );
}

static size_t get_max_pages_for_emote_menu() {
    std::vector<emote_t*> *current_emotes = get_current_emote_list();

    if (!current_emotes || current_emotes->size() < ROWS*COLUMNS)
        return 1;

    return std::ceil(current_emotes->size() / (ROWS * COLUMNS));
}

static void update_emote_menu_emotes()
{
    size_t total_emotes_on_page = get_max_emotes_on_menu_page();
    
    size_t start_index = (emote_menu_page - 1) * (ROWS*COLUMNS);
    size_t end_index = start_index + (total_emotes_on_page - 1);
    
    std::vector<emote_t*> *current_emotes = get_current_emote_list();
    
    if (!current_emotes) {
        return;
    }
    for (size_t idx = start_index; idx <= end_index; idx++) {
        menu_emotes.push_back((*current_emotes)[idx]);
    }
}

static void update_emote_menu_results()
{
    emote_menu_page = 1;
    emote_menu_selection = 0;

    if (EMOTES_VECTOR.empty()) {
        return;
    }

    menu_emotes.clear();
    menu_search_emotes.clear();
    
    if (emote_menu_query.empty()) {
        update_emote_menu_emotes();
        return;
    }
    
    // fuzzy search
    for (const auto& temp_emote : EMOTES_VECTOR) {
        if (std::string(temp_emote->name).find(emote_menu_query) != std::string::npos) {
            menu_search_emotes.push_back(temp_emote);
        }
    }
    
    update_emote_menu_emotes();
}


void RR_CheckChatInputForEmoteMenu(INT32 c)
{
    if(is_emote_preview_on) {
        return;
    }
    
    if (c == ' ' || c == ':')
        return;
    
    if (is_emote_menu_on) {
        update_query_add(c, &emote_menu_query);
        update_emote_menu_results();
    }
}

void RR_CheckChatDeleteForEmoteMenu(void)
{
    if (is_emote_preview_on) {
        return;
    }

    if (is_emote_menu_on) {
        if (emote_menu_query.empty())
            return;
        update_query_delete(&emote_menu_query);
        update_emote_menu_results();
    }
}

void RR_CheckChatEnterforEmoteMenu(void)
{
    if(menu_emotes.empty())
        return;
    
    // find the emote we're currently selecting
    std::string chosen_emote_name = menu_emotes[emote_menu_selection]->name;
    chosen_emote_name.insert(0, 1, ':');
    std::string final_emote_name = chosen_emote_name.append(":");
    
    const char* final_emote_name_c = final_emote_name.c_str();

    size_t chat_length = strlen(w_chat);
    size_t final_length = strlen(final_emote_name_c);

    if (chat_length + final_length > HU_MAXMSGLEN) {
        if (radio_ding_sound != sfx_None) {
            S_StartSoundAtVolume(NULL, radio_ding_sound, INT32_MAX);
        }
        return;
    }
    
    memmove(&w_chat[c_input + final_length], &w_chat[c_input], (chat_length - c_input) + 1);
    memcpy(&w_chat[c_input], final_emote_name_c, final_length);

    c_input += final_length;

    S_StartSoundAtVolume(NULL, sfx_cdfm09, 192);
}

void RR_CheckEmoteMenuMovement(INT32 direction)
{
    // there can only be (ROWS * COLUMNS) per page
    size_t total_emotes_on_page = get_max_emotes_on_menu_page();
    
    if(total_emotes_on_page <= 0) {
        return;
    }
    
    size_t row_start = ((emote_menu_selection) / COLUMNS) * COLUMNS;
    size_t row_end = std::min(row_start + (COLUMNS - 1), total_emotes_on_page - 1);
    size_t max_page = get_max_pages_for_emote_menu();
    
    switch (direction) {
        case KEY_UPARROW:
            if (signed(emote_menu_selection - ROWS) >= 0) {
                emote_menu_selection -= ROWS;
                S_StartSound(NULL, sfx_menu1);
            }
            break;
        case KEY_DOWNARROW:
            if (emote_menu_selection + ROWS <= total_emotes_on_page - 1) {
                emote_menu_selection += ROWS;
                S_StartSound(NULL, sfx_menu1);
            }
            break;
        case KEY_LEFTARROW:
            if (emote_menu_selection > row_start) {
                emote_menu_selection--;
            } else {
                emote_menu_page = (emote_menu_page - 1 < 1) ? max_page : emote_menu_page - 1;
                menu_emotes.clear();
                update_emote_menu_emotes();
                emote_menu_selection = 0;
            }
            S_StartSound(NULL, sfx_menu1);
            break;
        case KEY_RIGHTARROW:
            if (emote_menu_selection < row_end) {
                emote_menu_selection++;
            } else {
                emote_menu_page = (emote_menu_page + 1 > max_page) ? 1 : emote_menu_page + 1;
                menu_emotes.clear();
                update_emote_menu_emotes();
                emote_menu_selection = 0;
            }
            S_StartSound(NULL, sfx_menu1);
            break;
    }
}

void RR_ToggleEmoteMenu(void)
{
    if (is_emote_preview_on)
        return;
        
    if(is_emote_menu_on == true) {
        RR_ResetEmoteMenuInfo();    
    } else {
        emote_menu_query.clear();
        menu_emotes.clear();
        menu_search_emotes.clear();
        emote_menu_selection = 0;
        emote_menu_page = 1;
        
        update_emote_menu_emotes();

        is_emote_menu_on = true;
    }
    S_StartSound(NULL, sfx_menu1);
}

/**
 * Discord/Skype-esque menu
 */
void RR_DrawChatEmoteMenu(
    INT16 x, 
    INT16 y
) {
    if (!is_emote_menu_on)
        return;
    
    // If there are absolutely no emotes loaded, why are we even here?
    if (EMOTES.empty()) {
        return;
    }

    const INT32 MENU_WIDTH = 56;
    const INT32 MENU_HEIGHT = 72;
    const INT32 background_y = y - (MENU_HEIGHT);

    // Just scale the whole emote. It'll look squashed if it's wide, but, who cares? Wide emotes are NON-standard.
    const INT32 _WIDTH = 10;

    // background
    V_DrawFill(x, background_y, MENU_WIDTH, MENU_HEIGHT, 31 | V_60TRANS | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

    // text input
    const INT32 text_x = x + 1;
    const INT32 text_y = background_y - 5;
    const INT32 text_bg_y = background_y - 5;
    
    V_DrawFill(x, text_bg_y, MENU_WIDTH, 5, 19 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);
    V_DrawStringScaled(
        (text_x) << FRACBITS,
        (text_y) << FRACBITS,
        FRACUNIT/2,
        FRACUNIT,
        FRACUNIT,
        V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
        NULL,
        TINY_FONT,
        emote_menu_query.c_str()
    );

    fixed_t widthsofar = V_StringScaledWidth(
        FRACUNIT/2,
        FRACUNIT,
        FRACUNIT,
        V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
        TINY_FONT,
        emote_menu_query.c_str()
    );

    
    if (hu_tick < 4)
    {
        V_DrawCharacterScaled(
            ((text_x) << FRACBITS) + widthsofar,
            (text_y) << FRACBITS,
            FRACUNIT/2,
            V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
            TINY_FONT,
            '_',
            NULL
        );
    }

     if (menu_emotes.empty()) {
        const INT32 middle_x = x + (MENU_WIDTH/2);
        const fixed_t string_w = V_StringScaledWidth( FRACUNIT/2, FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM | V_SNAPTOLEFT, TINY_FONT, "NOTHING");
        const int string_w_int = string_w/FRACUNIT;

        const int new_middle_x = middle_x - (string_w_int/2);
        V_DrawStringScaled(
            (new_middle_x << FRACBITS),
            (background_y + (MENU_HEIGHT/2)) << FRACBITS,
            FRACUNIT/2,
            FRACUNIT,
            FRACUNIT,
            V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
            NULL,
            TINY_FONT,
            "NOTHING"
        );   

        return;
    }

    // page number
    /**
     * If there are 80 emotes loaded, that's a maximum of 25 emotes per page (5 rows, 5 columns)
     * 
     * 80 / (ROWS * COLUMNS)
     * 80 / (5 * 5)
     * 80 / 25 = 3.2
     * 3.2 (rounded up) = 4
     * 
     * So, that's about 4 pages max. 75 emotes for the first three pages, 5 emotes for the last page.
     */

    size_t max_page = get_max_pages_for_emote_menu();

    const INT32 page_y = y - 5;

    fixed_t page_width = V_StringScaledWidth( FRACUNIT/2, FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM | V_SNAPTOLEFT, TINY_FONT, va("Page %d of %d", emote_menu_page, max_page));
    fixed_t page_x = ((x + MENU_WIDTH) << FRACBITS) - page_width;
    
    V_DrawStringScaled(
        page_x,
        (page_y) << FRACBITS,
        FRACUNIT/2,
        FRACUNIT,
        FRACUNIT,
        V_SNAPTOBOTTOM | V_SNAPTOLEFT,
        NULL,
        TINY_FONT,
        va("Page %d of %d", emote_menu_page, max_page)
    );

    // drawing the actual emotes
    size_t row_y = background_y + 1;
    size_t row_x = x + 1;
    
    size_t ei = 0;
    for (size_t i = 0; i < ROWS; i++) {
        for (size_t j = 0; j < COLUMNS; j++) {
            if (ei >= menu_emotes.size()) {
                continue;
            }
            emote_t* m_emote = menu_emotes[ei];
            if (m_emote == nullptr) {
                break;
            }
            
            size_t current_row = (emote_menu_selection) / COLUMNS;
            size_t current_column = (emote_menu_selection) % COLUMNS;

            if (i == current_row && j == current_column) {
                V_DrawFill(x, page_y - 6, MENU_WIDTH, 5, 21 | V_SNAPTOBOTTOM | V_SNAPTOLEFT);

                const INT32 middle_x = x + (MENU_WIDTH/2);
                const fixed_t emote_width = V_StringScaledWidth( FRACUNIT/2, FRACUNIT, FRACUNIT, V_SNAPTOBOTTOM | V_SNAPTOLEFT, TINY_FONT, m_emote->name);
                const int emote_width_int = emote_width/FRACUNIT;

                const int new_middle_x = middle_x - (emote_width_int/2);
                V_DrawStringScaled(
                    (new_middle_x << FRACBITS),
                    ((page_y - 6) << FRACBITS),
                    FRACUNIT/2,
                    FRACUNIT,
                    FRACUNIT,
                    V_YELLOWMAP | V_SNAPTOBOTTOM | V_SNAPTOLEFT,
                    NULL,
                    TINY_FONT,
                    m_emote->name
                );

                V_DrawFill(
                    row_x - 1,
                    row_y - 1,
                    _WIDTH + 2,
                    _WIDTH + 2,
                    100 | V_SNAPTOBOTTOM | V_SNAPTOLEFT
                );
            }
            
            if(m_emote->atlas_id != -1) {
                draw_atlas_emote(m_emote, row_x, row_y, _WIDTH);
            } else {
                draw_normal_emote(m_emote, row_x, row_y, _WIDTH);
            }

            row_x += 11;
            ei++;
        }

        row_y += (10) + 2;
        row_x = x +1;
    }
}