// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2024 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_hud.h
/// \brief RadioRacers HUD Functionality

#ifndef __RR_HUD__
#define __RR_HUD__

#include "../k_menu.h"
#include "../m_cond.h"
#include "../command.h"
#include "../console.h"
#include "../d_player.h"
#include "../doomstat.h"
#include "../m_fixed.h"
#include "../../hu_stuff.h"

#include "rr_cvar.h"
#include "rr_video.h"

#ifdef __cplusplus

#include <string>
#include <vector>
#include <unordered_map>

#define EMOTE_NAME_SIZE 80 // Even THIS seems generous

typedef struct 
{
    int atlas_id;
    int atlas_row;
    int atlas_column;

    char name[EMOTE_NAME_SIZE+1];
    boolean animated;
    INT32 frame_delay; 
    lumpnum_t* frames;
    size_t frame_count;
    char rank[2];       // A? B? C? D? E? ... S?
} emote_t;

typedef struct
{
    int rows;
    int columns;
    int width;
    int height;
    lumpnum_t atlas_lump;
} emote_atlas_t;

typedef struct
{
    fixed_t x;
    fixed_t y;
} emote_atlas_coordinates_t;

extern std::unordered_map<std::string, emote_t*> EMOTES;
extern std::vector<emote_t*> EMOTES_VECTOR;
extern std::vector<emote_t*> EMOTES_VECTOR_SORTED;
extern std::vector<emote_t*> EMOTES_VECTOR_FAVOURITES;
extern std::unordered_map<std::string, emote_t*> EMOTES_FAVOURITE_MAP;
extern std::unordered_map<int, emote_atlas_t*> EMOTE_ATLASES;
extern std::unordered_map<std::string, int> EMOTE_USAGE;

extern std::vector<emote_t> E_RANK_EMOTES;
extern std::vector<emote_t> D_RANK_EMOTES;
extern std::vector<emote_t> C_RANK_EMOTES;
extern std::vector<emote_t> B_RANK_EMOTES;
extern std::vector<emote_t> A_RANK_EMOTES;

extern std::unordered_map<emote_t*, size_t> emoteFrameMap;
extern std::unordered_map<emote_t*, size_t> emoteLastUpdate;

extern std::unordered_map<emote_t*, size_t> chatEmoteFrameMap;
extern std::unordered_map<emote_t*, size_t> chatEmoteLastUpdate;

extern std::vector<std::vector<patch_t*>> chat_log_emote_patches;
extern std::vector<std::vector<emote_t*>> chat_log_emotes;

emote_atlas_coordinates_t getEmoteAtlasCoordinates(emote_t* emote, float scale);
extern lumpnum_t getEmoteAtlasFrame(int atlas_id);
extern lumpnum_t getEmoteFrame(emote_t* emote);
extern lumpnum_t getChatEmoteFrame(emote_t* emote);

emote_t* RR_GetEmoteFromChatLog(size_t chat_log_index, int emote_tracer_index);
emote_t* RR_GetEmoteFromChatMiniLog(size_t chat_log_mini_index, int emote_tracer_index);
extern std::unordered_map<chat_log_type_t, std::function<emote_t*(size_t, int)>> emote_fetch_map;


extern "C" {
#endif

/**
 * Emotes
 */

// Copying savedip logic here since it's sensible

#define EMOTE_NAME_SIZE 80 // Even this seems generous
#define NUMLOGEMOTES 50 // Even THIS seems generous
#define EMOTE_MOST_USED_FILE "rr_mostusedemotes.txt" // accidentially named this fortnite.txt
#define EMOTE_FAVOURITES_FILE "rr_favemotes.txt" // accidentially named this fortnite.txt

// For the chat
extern boolean is_emote_preview_on;
extern boolean is_emote_menu_on;

typedef struct
{
    UINT32 chat_nummsg_log;

    INT32 charheight;
    INT32 x;
    INT32 y;
    INT32 chat_topy;
    INT32 chat_bottomy;

    INT32 boxw;
    fixed_t scale;
    INT32 flags;
    char (*chat_log)[255]; // Yuck
} chat_log_parameters_t;

typedef struct
{
    INT32 charheight;
    INT32 *x;
    INT32 *y;

    INT32 boxw;
    fixed_t scale;
    UINT32 chat_nummsg_min;
} chat_mini_log_parameters_t;

typedef struct
{
    INT16 x;
    INT16 y;
    INT16 typelines;
    INT32 charheight;
} chat_box_parameters_t;

typedef struct
{
    INT32 boxw;
    fixed_t scale;
    INT32 flags;
    INT32 y;
    INT32 chatx;
    INT32 charheight;
    const char* talk;
} chat_input_parameters_t;

extern void RR_UpdateEmoteChatLogs(UINT32 chat_mini_log, UINT32 chat_log);
extern void RR_RemoveEmoteChatMiniLog(UINT32 chat_mini_log);
extern void RR_RemoveEmoteChatLog(UINT32 chat_log);
extern void RR_UpdateEmoteChatInputLog(void);
extern void RR_RemoveEmoteChatInputLog(void);
extern INT32 RR_Parse_ChatLog(chat_log_parameters_t parameters);
extern void RR_Draw_ChatMiniLog(chat_mini_log_parameters_t parameters);
extern INT16 RR_DrawChatInput(chat_input_parameters_t p, INT32 *y);
extern boolean RR_CheckChatDeleteEmoteForInput(void);
extern INT32 RR_Parse_ChatMiniLog_For_Lines(
    size_t i, 
    fixed_t scale,
    INT32 boxw
);
extern void RR_CheckChatInputForEmotePreview(INT32 c);
extern void RR_CheckChatDeleteForEmotePreview(void);
extern void RR_CheckChatInputForEmoteMenu(INT32 c);
extern void RR_CheckChatDeleteForEmoteMenu(void);
extern void RR_CheckChatEnterforEmoteMenu(void);
extern void RR_ToggleEmoteMenu(void);
extern void RR_UpdateEmoteQueryChoice_Left(void);
extern void RR_UpdateEmoteQueryChoice_Right(void);
extern void RR_ResetEmoteSearchQuery(void);
extern void RR_ResetEmoteMenuInfo(void);
extern void RR_ResetAllEmoteChatInfo(void);
extern void RR_SelectEmoteFromPreview(void);
extern void RR_DrawChatEmotePreview(
    INT16 x,
    INT16 y,
    INT32 w
);
extern void RR_DrawChatEmoteMenu(
    INT16 x,
    INT16 y
);
extern void RR_CheckEmoteMenuMovement(INT32 c);
extern void RR_EmoteUsageCheckOnSend(const char* msg);
extern void RR_UpdateFavouriteEmotes(void);

/**
 * Ringbox / Itembox
 */
typedef enum {
    LEFT = 0,
    ABOVE,
    RIGHT
} itemboxposition_e;

typedef struct
{
    fixed_t x;
    fixed_t y;
    boolean valid_coords;
} itembox_tracking_coordinates_t;

typedef struct 
{
    fixed_t space;
    fixed_t offset;
} roulette_offset_spacing_t;


// Emote grade tally
extern void RR_EmoteTally_Tick(void);

/**
 * Item box graphic is 50 x 50. (42 x 42 excluding any empty space).
 * Ring box graphic is 56 x 48.
 * 
 * If we're drawing the graphics with tracking without doing any extra work to the coordinates, they would be drawn right on top of the player.
 * So, the basis for any positioning (depending on what the player has chosen) is the width .. and multiplying that by the scale cvar.
 */
extern itembox_tracking_coordinates_t RR_getRouletteCoordinatesForKartItem(void);
extern itembox_tracking_coordinates_t RR_getRouletteCoordinatesForRingBox(void);

extern fixed_t RR_getItemBoxHudScale(void);
extern float RR_getItemBoxHudScaleFloat(void);
extern fixed_t RR_getRingBoxHudScale(void);
extern float RR_getRingBoxHudScaleFloat(void);

extern vector2_t RR_getRouletteCroppingForKartItem(vector2_t rouletteCrop);
extern vector2_t RR_getRouletteCroppingForRingBox(vector2_t rouletteCrop);

extern roulette_offset_spacing_t RR_getRouletteSpacingOffsetForKartItem(fixed_t offset);
extern roulette_offset_spacing_t RR_getRouletteSpacingOffsetForRingBox(fixed_t offset);

/**
 * Battle HUD 
 */

/**
 * Alternate take on the emerald display during Battle Mode.
 * Draws the Emeralds on the bottom-left of the HUD.
 */
extern void RR_drawKartEmeralds(void);

/**
 * Tally
 */
extern void RR_InitGradeEmoteTally(void);
extern boolean RR_ShouldDrawEmoteTally(void);
extern boolean RR_IsEmoteTallyPossible(gp_rank_e grade);

/**
 * Race HUD
 */
extern void RR_addPlayerToFinshTicker(player_t *player);
extern void RR_drawRidersFinishTicker(void);
extern void RR_ridersFinishTick(void);
extern void RR_resetRidersFinishTicker(void);

typedef struct
{
    gp_rank_e grade;
    patch_t* grade_patch;
    skincolornum_t grade_color;
    fixed_t x;
    fixed_t y;
} player_grade_info_t;

/**
 * Chat HUD
 */
extern int chat_log_offset[CHAT_BUFSIZE];
extern int chat_mini_log_offset[8];
extern int hu_radio_tick;
extern void RR_DoChatStuff(chat_box_parameters_t parameters);
// Tally
void RR_DrawGradeEmote(player_grade_info_t grade_info);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
