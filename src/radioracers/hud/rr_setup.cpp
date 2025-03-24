// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_setup.cpp
/// \brief HUD setup stuff

#include <vector>
#include <string>
#include <unordered_map>

#include "../rr_hud.h"
#include "../rr_setup.h"
#include "../../d_main.h"
#include "../../w_wad.h"
#include "../../z_zone.h"
#include "../../r_defs.h"
#include "../../r_picformats.h"

static const char* EMOTE_FRAME_NAME = "FRAME";
static const char* EMOTE_ATLAS_FRAME_NAME = "EMOATLAS";

boolean found_radioracers = false;
boolean found_radioracers_plus = false;
boolean radioracers_usemuteicons = false;
boolean radioracers_usehakiencore = false;
boolean radioracers_useendkey = false;
patch_t* end_key[2];

sfxenum_t radio_ding_sound;

/**
 * 300 frames is really generous. 
 * Some animated emotes are really cheeky with how long they are.
 * 
 * (see: most 7TV emotes)
 * */
#define MAX_EMOTE_FRAMES 300

// Global trackers
std::unordered_map<emote_t*, size_t> emoteFrameMap;
std::unordered_map<emote_t*, size_t> emoteLastUpdate;

std::unordered_map<emote_t*, size_t> chatEmoteFrameMap;
std::unordered_map<emote_t*, size_t> chatEmoteLastUpdate;

// Emotes
std::unordered_map<std::string, emote_t*> EMOTES;
// Checks if the emote is already IN the vector
std::unordered_map<emote_t*, bool> EMOTES_INDEX; 
std::vector<emote_t*> EMOTES_VECTOR;
std::unordered_map<int, emote_atlas_t*> EMOTE_ATLASES;

static int ATLAS_ID = -1;

// Grade Emotes
std::vector<emote_t> E_RANK_EMOTES;
std::vector<emote_t> D_RANK_EMOTES;
std::vector<emote_t> C_RANK_EMOTES;
std::vector<emote_t> B_RANK_EMOTES;
std::vector<emote_t> A_RANK_EMOTES;

struct RankLookup {
    char rank;
    std::vector<emote_t>* array;
};

RankLookup rankLookupTable[] = {
    {'E', &E_RANK_EMOTES},
    {'D', &D_RANK_EMOTES},
    {'C', &C_RANK_EMOTES},
    {'B', &B_RANK_EMOTES},
    {'A', &A_RANK_EMOTES},
};

// Rolls right off the tongue
using GradeEmoteTokens = std::vector<
    std::pair<std::string, std::string>
>;
using AtlasEmoteTokens = std::vector<
    std::pair<std::string, std::string>
>;

typedef struct
{
    AtlasEmoteTokens t;
    std::vector<std::string> emote_names;
} atlas_emote_config_t;
 
/** EMOTE TOKENS */
static constexpr const char* NAME       = "Name";
static constexpr const char* RANK       = "Rank";
static constexpr const char* ANIMATED   = "Animated";
static constexpr const char* TICS       = "Tics";

/** ATLAS TOKENS */
static constexpr const char* ROWS       = "Rows";
static constexpr const char* COLUMNS    = "Columns";
static constexpr const char* WIDTH      = "Width";
static constexpr const char* HEIGHT     = "Height";
static constexpr const char* EMOTE      = "Emote";

static constexpr const char* GRADE_EMOTE_CONFIG_LUMP = "EMOTEDEF";
static constexpr const char* EMOTE_ATLAS_CONFIG_LUMP = "ATLASDEF";

static std::vector<emote_t>* getRankEmoteArray(char rank) {
    for (size_t i = 0; i < sizeof(rankLookupTable) / sizeof(RankLookup); ++i) {
        if (rankLookupTable[i].rank == rank) {
            // CONS_Printf("Adding to %c array\n", rank);
            return rankLookupTable[i].array;
        }
    }
    return nullptr;
}

static int getNextAtlasId()
{
    return ++ATLAS_ID;
}

static boolean hasRankToken(GradeEmoteTokens tokens) {
    boolean found = false; 
    for (const auto& token: tokens) {
        found = token.first == RANK;
        if (found) 
            return found;
    }
    return found;
}

static boolean is_emote_already_loaded(std::string name) {
    return (!EMOTES.empty() && EMOTES.find(name) != EMOTES.end());
}

static boolean has_punctuation(const std::string& str) {
    for (char ch: str) {
        if (std::ispunct(static_cast<unsigned char>(ch)) && ch != '_') {
            return true;
        }
    }
    return false;
}

static boolean is_emote_good(std::string name) {
    // Is the name length < EMOTE_NAME_SIZE?
    if(name.length() > EMOTE_NAME_SIZE) {
        CONS_Printf("Emote '%s' is TOOOOO long, skipping.\n", name.c_str());
        return false;
    }

    // Does the name have any punctuation?
    if (has_punctuation(name)) {
        CONS_Printf("Emote '%s' has some invalid characters, skipping.\n", name.c_str());
        return false;
    }

    // Is it already loaded?
    if (is_emote_already_loaded(name)) {
        CONS_Printf("Emote '%s' already exists, skipping.\n", name.c_str());
        return false;
    }

    return true;
}

static void initDefaultGradeEmote(emote_t *rank_emote)
{
    memset(rank_emote, 0, sizeof (emote_t));

    rank_emote->frames = static_cast<lumpnum_t*>(malloc(sizeof(lumpnum_t) * MAX_EMOTE_FRAMES));
    rank_emote->frame_count = 0;
    
    rank_emote->atlas_id = -1;
    rank_emote->atlas_row = -1;
    rank_emote->atlas_column = -1;

    rank_emote->animated = false;
    rank_emote->frame_delay = 0; 
    strcpy(rank_emote->rank, "E");
    strcpy(rank_emote->name, "DefaultEmote");
}

static void initDefaultAtlasEmote(emote_atlas_t *atlas)
{
    memset(atlas, 0, sizeof (emote_atlas_t));

    atlas->rows = -1;
    atlas->columns = -1;
    atlas->width = -1;
    atlas->height = -1;
}

static std::pair<std::string, std::string> tokenize_loop(
    const std::string& lines,
    size_t &start,
    size_t &end
)
{
    const char* delimiters = "\r\n";

    end = lines.find_first_of("= ", start);

    if (end == std::string::npos)
        return {};
    
    std::string key = lines.substr(start, end - start);

    start = lines.find_first_not_of("= ", end);
    end = lines.find_first_of(delimiters, start);

    std::string value = lines.substr(start, end - start);

    return {key, value};
}

/**
 * Loop over the EMOTEDEF lump, line by line.
 * Find each token, its value, and return a list of them.
 * 
 * Basically, what already happens in R_AddSkins.
 */
GradeEmoteTokens TokenizeGradeEmotes(
    const std::string& lines
) {
    GradeEmoteTokens tokens;
    size_t start = 0, end = 0;

    const char* delimiters = "\r\n";
    
    // Example line, 'Rank = A'
    // Firstly, skip over newlines and/or spaces.
    /**
     *    Rank = A
     * ^^^
     */
    while ((start = lines.find_first_not_of(delimiters, end)) != std::string::npos)
    {
        // Ignore comments
        if (lines[start] == '#') {
            end = lines.find_first_of(delimiters, start);
            continue;
        }

        end = lines.find_first_of("= ", start);

        // No token value?
        if (end == std::string::npos)
            break;
        
        // Rank = A
        // ^^^^
        std::string key = lines.substr(start, end - start);

        start = lines.find_first_not_of("= ", end);
        end = lines.find_first_of(delimiters, start);

        // Rank = A
        //        ^
        std::string value = lines.substr(start, end - start);

        // kick, push. kick, push.
        tokens.emplace_back(key, value);
    }
    return tokens;
}

atlas_emote_config_t TokenizeAtlasEmotes(
    const std::string& lines
) {
    AtlasEmoteTokens tokens;
    std::vector<std::string> emote_names;

    size_t start = 0, end = 0;
    const char* delimiters = "\r\n";

    while ((start = lines.find_first_not_of(delimiters, end)) != std::string::npos)
    {
        auto [key, value] = tokenize_loop(lines, start, end);

        if (key.find(EMOTE) == 0) {
            emote_names.push_back(value);
        } else {
            tokens.emplace_back(key, value);
        }
    }
    
    return {tokens, emote_names};
}

// R_LoadSkinSprites
static void LoadEmoteLumps(UINT16 wadnum, UINT16 *lump, UINT16 *lastlump, emote_t *rank_emote)
{
    UINT16 newlastlump;
    lumpinfo_t *lumpinfo;
    // softwarepatch_t patch;
    
    lumpinfo = wadfiles[wadnum]->lumpinfo;

    *lump += 1;
    *lastlump = W_FindNextEmptyInPwad(wadnum, *lump);
    
    newlastlump = W_CheckNumForNamePwad(GRADE_EMOTE_CONFIG_LUMP, wadnum, *lump);
    if (newlastlump < *lastlump)
        *lastlump = newlastlump;

    if (*lastlump > wadfiles[wadnum]->numlumps)
        *lastlump = wadfiles[wadnum]->numlumps;
    
    for (UINT16 i = *lump; i < *lastlump; i++) {

        // Only process lumps that match FRAME(xxx).lmp
        if(memcmp(lumpinfo[i].name, EMOTE_FRAME_NAME, 5))
            continue;

        // Fuck off!! I don't care how funny you THINK your emote is, it doesn't need to be THAT long!
        if(rank_emote->frame_count + 1 > MAX_EMOTE_FRAMES)
        {
            CONS_Printf("Reached max frame limit for emote. How embarassing.\n");
            break;
        }

        lumpnum_t wadlump = wadnum;
        
        wadlump <<=16;
        wadlump += i;

        // INT32 width, height;
        // INT16 topoffset, leftoffset;

        // W_ReadLumpHeaderPwad(wadnum, i, &patch, PNG_HEADER_SIZE, 0);

        // width = (INT32)(SHORT(patch.width));
        // height = (INT32)(SHORT(patch.height));
        // topoffset = (INT16)(SHORT(patch.topoffset));
        // leftoffset = (INT16)(SHORT(patch.leftoffset));

        // CONS_Printf("width %d, height %d, topoffset %d, leftoffset, %d\n", width, height, topoffset, leftoffset);

        // CONS_Printf("adding %s\n", lumpinfo[i].name);
        // CONS_Printf("size %d\n", lumpinfo[i].size);

        rank_emote->frames[rank_emote->frame_count] = wadlump;
        rank_emote->frame_count++;

        // CONS_Printf("frame count %d\n", rank_emote->frame_count);
    }
}

static void LoadAtlasEmoteLump(UINT16 wadnum, UINT16 *lump, UINT16 *lastlump, emote_atlas_t *atlas)
{
    UINT16 newlastlump;
    lumpinfo_t *lumpinfo;

    lumpinfo = wadfiles[wadnum]->lumpinfo;

    *lump += 1;
    *lastlump = W_FindNextEmptyInPwad(wadnum, *lump);
    
    newlastlump = W_CheckNumForNamePwad(EMOTE_ATLAS_CONFIG_LUMP, wadnum, *lump);
    if (newlastlump < *lastlump)
        *lastlump = newlastlump;

    if (*lastlump > wadfiles[wadnum]->numlumps)
        *lastlump = wadfiles[wadnum]->numlumps;

    for (UINT16 i = *lump; i < *lastlump; i++) {

        // Only process lumps that match EMOATLAS
        if(memcmp(lumpinfo[i].name, EMOTE_ATLAS_FRAME_NAME, 8))
            continue;
        
        lumpnum_t wadlump = wadnum;
        
        wadlump <<=16;
        wadlump += i;

        // Got the atlas lump, escape the loop immediately
        atlas->atlas_lump = wadlump;
        break;
    }
}

static void add_quick_emote(const char* lump_name, const char* emote_name)
{
    if (W_LumpExists(lump_name)) {
        emote_t* emote = static_cast<emote_t*>(malloc(sizeof(emote_t)));
        initDefaultGradeEmote(emote);

        emote->frame_count = 1;
        emote->frames[0] = W_GetNumForLongName(lump_name);
        emote->animated = false;
        strcpy(emote->name, emote_name);

        EMOTES.emplace(std::string(emote->name), new emote_t(*emote));
    }
}

static void AddInGameEmotes(void)
{
    /**
     * When this function is called, the gfx.pk3 should already be loaded.
     * So let's just set up some default emotes using the in-game graphics.
     */

    add_quick_emote("K_ITSHRK", "shrink"); // Shrink
    add_quick_emote("K_ITGBOM", "gachabomb"); // Gachabomb
    add_quick_emote("K_ITDTRG", "bumper"); // Burger
    add_quick_emote("K_ITRING", "superring"); // Super Ring
    add_quick_emote("K_ITTHNS", "lightningshield"); // Lightning Shield

    // font
    add_quick_emote("THIFN063", "?");
    add_quick_emote("THIFN033", "!");
    add_quick_emote("THIFN048", "0");
    add_quick_emote("THIFN049", "1");
    add_quick_emote("THIFN050", "2");
    add_quick_emote("THIFN051", "3");
    add_quick_emote("THIFN052", "4");
    add_quick_emote("THIFN053", "5");
    add_quick_emote("THIFN054", "6");
    add_quick_emote("THIFN055", "7");
    add_quick_emote("THIFN056", "8");
    add_quick_emote("THIFN057", "9");
}

static void UpdateEmotesVector(void)
{
    for (const std::pair<const std::string, emote_t*>& entry : EMOTES) {
        emote_t* em = entry.second;

        if (!EMOTES_INDEX[em]) {
            EMOTES_VECTOR.push_back(em);
            EMOTES_INDEX[em] = true;
        }
    }

    auto emote_sort = [](const emote_t* a, const emote_t* b) {
        // convert the emote names to lowercase so they appear in the right order in the menus
        std::string lc_a = std::string(a->name);
        std::string lc_b = std::string(b->name);

        std::transform(lc_a.begin(), lc_a.end(), lc_a.begin(), ::tolower);
        std::transform(lc_b.begin(), lc_b.end(), lc_b.begin(), ::tolower);

        return lc_a < lc_b;
    };

    std::sort(EMOTES_VECTOR.begin(), EMOTES_VECTOR.end(), emote_sort);
}

void RR_AddAtlasEmotes(UINT16 wadnum)
{
    UINT16 lump, lastlump = 0;

    size_t lump_size;
    emote_atlas_t* atlas = static_cast<emote_atlas_t*>(malloc(sizeof(emote_atlas_t)));

    while((lump = W_CheckNumForNamePwad(EMOTE_ATLAS_CONFIG_LUMP, wadnum, lastlump)) != INT16_MAX)
    {
        lastlump = lump + 1;
        char *buffer = static_cast<char*>(W_CacheLumpNumPwad(wadnum, lump, PU_CACHE));
        lump_size = W_LumpLengthPwad(wadnum, lump);

        std::string bufferStr(buffer, lump_size);

        // Tokenise, again, but with a twist.
        atlas_emote_config_t config = TokenizeAtlasEmotes(bufferStr);

        // First of all, is this atlas configured with ANY emotes?
        if (config.emote_names.empty())
        {
            free(atlas);
            continue;
        }

        // Initialize the atlas and load the lump
        initDefaultAtlasEmote(atlas);

        for (size_t i = 0; i < config.t.size(); i++) {
            const std::string &key = config.t[i].first;
            const std::string &value = config.t[i].second;

            if (!stricmp(key.c_str(), ROWS)) {
                atlas->rows = std::stoi(value);
            } else if (!stricmp(key.c_str(), COLUMNS)) {
                atlas->columns = std::stoi(value);
            } else if (!stricmp(key.c_str(), WIDTH)) {
                atlas->width = std::stoi(value);
            } else if (!stricmp(key.c_str(), HEIGHT)) {
                atlas->height = std::stoi(value);
            }
        }

        // We need ALL of these
        if (
            atlas->height == -1 || atlas->width == -1 || atlas->rows == -1 || atlas->columns == -1
        ) {
            free(atlas);
            continue;
        }
        LoadAtlasEmoteLump(wadnum, &lump, &lastlump, atlas);

        if (atlas->atlas_lump == LUMPERROR)
        {
            CONS_Printf("Something hilarious just happened, abandoning atlas configuration.\n");
            free(atlas);
            continue;
        }

        const int atlas_id = getNextAtlasId();
        EMOTE_ATLASES.emplace(atlas_id, new emote_atlas_t(*atlas));
        CONS_Printf("Added atlas at ID #%d\n", atlas_id);

        
        // Now initialize the emotes we found
        int row = 0;
        int column = 0;

        for (size_t i = 0; i < config.emote_names.size(); i++) {
            emote_t* atlas_emote = static_cast<emote_t*>(malloc(sizeof(emote_t)));

            initDefaultGradeEmote(atlas_emote);
        
            // copy the name (case-sensitive)
            std::string temp_name = config.emote_names[i];

            if (!is_emote_good(temp_name)) {
                free(atlas_emote);
                continue;
            }
            
            STRBUFCPY(atlas_emote->name, temp_name.c_str());

            // assign the atlas ID and row/column
            atlas_emote->atlas_id = atlas_id;
            atlas_emote->atlas_row = row;
            atlas_emote->atlas_column = column;

            EMOTES.emplace(std::string(atlas_emote->name), new emote_t(*atlas_emote));

            CONS_Printf("Added atlas emote '%s' for atlas #%d at (%d, %d)\n", temp_name.c_str(), atlas_id, row, column);

            if (column + 1 > atlas->columns-1) {
                column = 0;
                row++;
            } else {
                column++;
            }            
        }
    }
}
/**
 * Copying R_AddSkins behaviour.
 */
void RR_AddEmotes(UINT16 wadnum)
{
    UINT16 lump, lastlump = 0;

    boolean eligible_emote = false;
    boolean eligible_rank_emote = false;
    size_t lump_size;
    emote_t* rank_emote = static_cast<emote_t*>(malloc(sizeof(emote_t)));

    // As long as there's an EMOTEDEF lump
    while((lump = W_CheckNumForNamePwad(GRADE_EMOTE_CONFIG_LUMP, wadnum, lastlump)) != INT16_MAX)
    {
        eligible_emote = false;
        eligible_rank_emote = false;
        lastlump = lump + 1;

        // CONS_Printf("Loading lump data\n");

        // Load in the lump data into the PU_CACHE
        char *buffer = static_cast<char*>(W_CacheLumpNumPwad(wadnum, lump, PU_CACHE));
        lump_size = W_LumpLengthPwad(wadnum, lump);

        // CONS_Printf("Tokenizing\n");

        // Tokenize.. the EMOTEDEF lump
        std::string bufferStr(buffer, lump_size);
        GradeEmoteTokens tokens = TokenizeGradeEmotes(bufferStr);

        // Set default values
        initDefaultGradeEmote(rank_emote);

        for (size_t i = 0; i < tokens.size(); i++) {
            const std::string &key = tokens[i].first;
            const std::string &value = tokens[i].second;

            // CONS_Printf("%s: %s\n", key.c_str(), value.c_str());

            // TODO: Add support for comments?
            if (!stricmp(key.c_str(), RANK)) {
                STRBUFCPY(rank_emote->rank, value.c_str());

                eligible_rank_emote = true;
            } else if (!stricmp(key.c_str(), TICS)) {
                rank_emote->frame_delay = atoi(value.c_str());
            } else if (!stricmp(key.c_str(), NAME)) {
                std::string temp_name = value;
                // std::transform(temp_name.begin(), temp_name.end(), temp_name.begin(), ::tolower);
                STRBUFCPY(rank_emote->name, value.c_str());

                // Needs a name to be used for the chat.
                eligible_emote = true;
            }
        }

        if(!is_emote_good(std::string(rank_emote->name))) {
            free(rank_emote);
            continue;
        }

        // And then (somehow) load the lumps..
        LoadEmoteLumps(wadnum, &lump, &lastlump, rank_emote);

        // You've added the configuration. But you don't have any frames?
        if (rank_emote->frame_count == 0)
        {
            free(rank_emote);
            continue;
        }

        // Yeah
        rank_emote->animated = (rank_emote->frame_count > 1);
    
        // Then push to the appropriate array
        if (eligible_rank_emote)
        {
            std::vector<emote_t>* rankArray = getRankEmoteArray(rank_emote->rank[0]);
            if (rankArray != nullptr) {
                // CONS_Printf("frame count %d\n", rank_emote->frame_count);
                rankArray->push_back(*rank_emote);
            }
            CONS_Printf("Added RANK %s emote '%s'\n", rank_emote->rank, rank_emote->name);
        }
        
        if (eligible_emote) {
            EMOTES.emplace(std::string(rank_emote->name), new emote_t(*rank_emote));
            CONS_Printf("Added GENERIC emote '%s'\n", rank_emote->name);
        }
    }
}

void RR_AddAllEmotes(UINT16 wadnum)
{
    RR_AddEmotes(wadnum);
    RR_AddAtlasEmotes(wadnum);

    if (!EMOTES.empty()) {
        UpdateEmotesVector();
    }
}

void RR_InitEmotes(void) {
    AddInGameEmotes();
    
    // C++ is so...
    for (std::size_t wadnum = 0; wadnum < numwadfiles; ++wadnum) {
        RR_AddAtlasEmotes(static_cast<UINT16>(wadnum));
        RR_AddEmotes(static_cast<UINT16>(wadnum));        
    }

    if (!EMOTES.empty()) {
        {
            // initialize the vector copy of the EMOTES unordered map for misc. references (i.e. chatbox)
            UpdateEmotesVector();
            CONS_Printf("Initialized EMOTES vector with %d emotes\n", EMOTES.size());
        } 
    }
}

// Anytime the player is NOT in the game, the tracker arrays should be emptied.
void RR_CleanupEmoteFrames(void) {
    if (emoteFrameMap.empty() && emoteLastUpdate.empty()) {
        return;
    }
    
    for (auto &frame: emoteFrameMap) {
        delete frame.first;
    }
    for (auto& timer: emoteLastUpdate) {
        delete timer.first;
    }

    // then clear
    emoteFrameMap.clear();
    emoteLastUpdate.clear();
}

/** Initialize anything relating to RadioRacers */
void RR_Init(void) {
    if (found_radioracers) {
        // Yeah
        radio_ding_sound = S_AddSoundFx("emenup", false, 0, false);
    
        // Mute icon for Pause Menu
        if (W_LumpExists("M_ICOMUT") && W_LumpExists("M_ICOMU2")) {
            radioracers_usemuteicons = true;
        }
    
        // The haki mode thing - this is just Sky Sanctuary's encore palette
        if(W_LumpExists("GRAYENCR")) {
            radioracers_usehakiencore = true;
        }

        if (W_LumpExists("EMENU_A") && W_LumpExists("EMENU_AB")) {
            HU_UpdatePatch(&end_key[0], "EMENU_A");
            HU_UpdatePatch(&end_key[1], "EMENU_AB");
            radioracers_useendkey = true;
        }
    }

    // Any emotes?
    CONS_Printf("RADIO: Setting up emotes.\n");
    RR_InitEmotes();
}