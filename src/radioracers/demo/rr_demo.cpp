// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/demo/rr_demo.cpp
/// \brief Feature demos

#include "../rr_demo.h"
#include "../../command.h"
#include "../../d_clisrv.h"
#include "../../g_game.h"
#include "../../p_local.h"
#include "../../d_main.h"
#include "../../z_zone.h"
#include "../../i_time.h"
#include "../../hu_stuff.h"
#include "../../music.h"

#include <iostream>
#include <fstream>
#include <map>

static tic_t _millisecondstoTics(INT32 milliseconds) {
    return(tic_t) ((milliseconds * TICRATE)/ 1000);
}

static tic_t _timeToTics(std::string time) {
    // convert MM:SS.ss to milliseconds first
    size_t delimiter_pos = time.find(":");
    int minutes = std::stoi(time.substr(0, delimiter_pos));

    size_t dot_pos = time.find(".", delimiter_pos);
    int seconds = std::stoi(time.substr(delimiter_pos + 1, dot_pos - delimiter_pos - 1));

    int milliseconds = 0;
    if (dot_pos != std::string::npos) {
        milliseconds = std::stoi(time.substr(dot_pos + 1));
    }

    int total_milliseconds = (minutes * 60 + seconds) * 1000 + milliseconds;

    // then to tics
    return _millisecondstoTics(total_milliseconds);
}

typedef enum
{
    DEMO_NONE,
    DEMO_PLAYING,
    DEMO_DONE
} demo_state_e;

struct emote_demo_t
{
    std::map<tic_t, std::string> lyric;
    tic_t start_time;
};

static demo_state_e demo_state = DEMO_NONE;
static emote_demo_t emote_demo;

static void Command_DemoEmotes(void)
{
    if (demo_state == DEMO_PLAYING) {
        CONS_Printf("Demo already playing!\n");
        return;
    }

    size_t argc = COM_Argc();

    if (argc < 2) {
        CONS_Printf(M_GetText("emotedemo <demo-file number>: Load a demo-file.\n"));
        return;
    }

    if (!Playing()) {
        CONS_Printf("You need to be in-game in order to run a demo.\n");
        return;
    }

    if (netgame && !P_IsDisplayPlayer(&players[serverplayer])) {
        CONS_Printf("Not running demo, not your server!\n");
        return;
    }

    // Find the emotedemo_[n].txt
    const char* demo_n = COM_Argv(1);

    char *demo_file;
    char *demo_path;

    demo_file = Z_StrDup(va("emotedemo_%s.txt", demo_n));
    demo_path = Z_StrDup(va("%s" PATHSEP "%s", srb2home, demo_file));

    std::ifstream file(demo_path);
    if (!file)
        return;

    if (!W_LumpExists(va("O_DEMO%s", demo_n)))
    {
        CONS_Printf("Demo music lump missing! (%s)\n", va("DEMO%s", demo_n));
        return;
    }

    std::string line;

    while (std::getline(file, line)) {
        size_t delimiter_pos = line.find(";");
        std::string time = line.substr(0, delimiter_pos);
        std::string lyric = line.substr(line.find(";") + 1); 
        emote_demo.lyric.emplace(_timeToTics(time), lyric);
    }

    file.close();
    
    // and initialize the demo
    const char* actual_demo_song = Z_StrDup(va("DEMO%s", demo_n));
    Music_Remap("stereo", actual_demo_song);
    Music_Play("stereo");
    emote_demo.start_time = I_GetTime();
    demo_state = DEMO_PLAYING;
}

void RR_DemoTicker(void) {
    if(demo_state != DEMO_PLAYING)
        return;
    
    if(emote_demo.lyric.empty()) {
        demo_state = DEMO_DONE;
        return;
    }
    
    // and now we tick
    int current_timer = I_GetTime() - emote_demo.start_time;

    if (emote_demo.lyric.find(current_timer) != emote_demo.lyric.end()) {
        COM_BufInsertText(va("say \"%s\"\n", emote_demo.lyric.at(current_timer).c_str()));
        emote_demo.lyric.erase(current_timer);
    }
}

void RR_InitDemoCommands(void)
{
#ifndef ENABLE_RADIO_DEMOS
    COM_AddCommand("emotedemo", Command_DemoEmotes);
#endif
}

