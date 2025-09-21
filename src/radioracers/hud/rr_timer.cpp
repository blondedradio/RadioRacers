// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/rr_timer.cpp

#include <algorithm>

#include "../rr_cvar.h"
#include "../rr_video.h"
#include "../rr_hud.h"

#include "../../doomstat.h"
#include "../../g_game.h"
#include "../../k_hud.h"
#include "../../p_local.h"
#include "../../r_fps.h"
#include "../../z_zone.h"
#include "../../w_wad.h"
#include "../../st_stuff.h"
#include "../../v_draw.hpp"

// This isn't exposed in k_hud.h, so just copying this here.
tic_t player_timer(const player_t* player)
{
	if (player->realtime == UINT32_MAX)
	{
		return 0;
	}

	return K_TranslateTimer(player->realtime, 0, nullptr);
}

/** Unfortunately, this is tied to the original K_drawKartTimestamp function. So, just copy it here.*/
static void DrawSPBAttack_Bar(void)
{
    if (modeattacking & ATTACKING_SPB && stplyr->SPBdistance > 0)
	{
		UINT8 *colormap = R_GetTranslationColormap(stplyr->skin, static_cast<skincolornum_t>(stplyr->skincolor), GTC_CACHE);
		INT32 ybar = 180;
		INT32 widthbar = 120, xbar = 160 - widthbar/2, currentx;
		INT32 barflags = V_SNAPTOBOTTOM|V_SLIDEIN;
		INT32 transflags = ((6)<<FF_TRANSSHIFT);

		V_DrawScaledPatch(xbar, ybar - 2, barflags|transflags, 
            static_cast<patch_t*>(W_CachePatchName("MINIPROG", PU_HUDGFX))
        );

		V_DrawMappedPatch(160 + widthbar/2 - 7, ybar - 7, barflags, faceprefix[stplyr->skin][FACE_MINIMAP], colormap);

		// vibes-based math
		currentx = (stplyr->SPBdistance/mapobjectscale - mobjinfo[MT_SPB].radius/FRACUNIT - mobjinfo[MT_PLAYER].radius/FRACUNIT) * 6;
		if (currentx > 0)
		{
			currentx = sqrt(currentx);
			if (currentx > widthbar)
				currentx = widthbar;
		}
		else
		{
			currentx = 0;
		}
		V_DrawScaledPatch(160 + widthbar/2 - currentx - 5, ybar - 7, barflags, 
            static_cast<patch_t*>(W_CachePatchName("SPBMMAP", PU_HUDGFX))
        );
	}
}

void RR_DrawKartMiniTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT32 splitflags, UINT8 mode)
{
    if (r_splitscreen)
        return;
    
    // Mostly copying the logic from RR_DrawKartTimestamp, just tweaking it to accomodate the thin-timer font
    TX += 80;

    INT32 jitter = 0;
    drawtime = K_TranslateTimer(drawtime, mode, &jitter);

    using srb2::Draw;
    Draw(TX-12, TY+7)
        .flags(splitflags)
        .align(Draw::Align::kCenter)
        .width(65)
        .small_sticker();

    V_DrawScaledPatch(TX-10, TY+4, splitflags, static_cast<patch_t *>(W_CachePatchName("K_STTIMS", PU_HUDGFX)));

    TX += 4;

    if (drawtime == UINT32_MAX)
        ;
    else if (mode && !drawtime)
    {
        // apostrophe location     _'__ __
        V_DrawThinTimerString(TX+12, TY+3, splitflags, va("'"));

        // quotation mark location    _ __"__
        V_DrawThinTimerString(TX+30, TY+3, splitflags, va("\""));
    }
    else
    {
        tic_t worktime = drawtime/(60*TICRATE);

        if (worktime >= 100)
        {
            jitter = (drawtime & 1 ? 1 : -1);
            worktime = 99;
            drawtime = (100*(60*TICRATE))-1;
        }

        // minutes time      00 __ __
        V_DrawThinTimerString(TX,    TY+3+jitter, splitflags, va("%d", worktime/10));
        V_DrawThinTimerString(TX+6, TY+3-jitter, splitflags, va("%d", worktime%10));

        // apostrophe location     _'__ __
        V_DrawThinTimerString(TX+12, TY+3, splitflags, va("'"));

        worktime = (drawtime/TICRATE % 60);

        // seconds time       _ 00 __
        V_DrawThinTimerString(TX+18, TY+3+jitter, splitflags, va("%d", worktime/10));
        V_DrawThinTimerString(TX+24, TY+3-jitter, splitflags, va("%d", worktime%10));

        // quotation mark location    _ __"__
        V_DrawThinTimerString(TX+30, TY+3, splitflags, va("\""));

        worktime = G_TicsToCentiseconds(drawtime);

        // tics               _ __ 00
        V_DrawThinTimerString(TX+36, TY+3+jitter, splitflags, va("%d", worktime/10));
        V_DrawThinTimerString(TX+42, TY+3-jitter, splitflags, va("%d", worktime%10));
    }

    TX-= 84;
    TX+= 33;

    // Medal data!
	if ((modeattacking || (mode == 1))
    && !demo.playback)
    {
        INT32 workx = TX + 96, worky = TY+18;
        UINT8 i = stickermedalinfo.visiblecount;

        if (stickermedalinfo.targettext[0] != '\0')
        {
            if (!mode)
            {
                if (stickermedalinfo.jitter)
                {
                    jitter = stickermedalinfo.jitter+3;
                    if (jitter & 2)
                        workx += jitter/4;
                    else
                        workx -= jitter/4;
                }

                if (stickermedalinfo.norecord == true)
                {
                    splitflags = (splitflags &~ V_HUDTRANS)|V_HUDTRANSHALF;
                }
            }

            workx -= K_drawKartMicroTime(stickermedalinfo.targettext, workx, worky, splitflags);
        }

        workx -= (((1 + i - stickermedalinfo.platinumcount)*6) - 1);

        if (!mode)
            splitflags = (splitflags &~ V_HUDTRANSHALF)|V_HUDTRANS;
        while (i > 0)
        {
            i--;

            if (gamedata->collected[(stickermedalinfo.emblems[i]-emblemlocations)])
            {
                V_DrawMappedPatch(workx, worky, splitflags,
                    static_cast<patch_t*>(W_CachePatchName(M_GetEmblemPatch(stickermedalinfo.emblems[i], false), PU_CACHE)),
                    R_GetTranslationColormap(TC_DEFAULT, M_GetEmblemColor(stickermedalinfo.emblems[i]), GTC_CACHE)
                );

                workx += 6;
            }
            else if (
                stickermedalinfo.emblems[i]->type != ET_TIME
                || stickermedalinfo.emblems[i]->tag != AUTOMEDAL_PLATINUM
            )
            {
                V_DrawMappedPatch(workx, worky, splitflags,
                    static_cast<patch_t*>(W_CachePatchName("NEEDIT", PU_CACHE)),
                    NULL
                );

                workx += 6;
            }
        }
    }

    // SPB Attack
    DrawSPBAttack_Bar();
}

void RR_DrawMiniTimestamp(tic_t time, INT32 x, INT32 y, INT32 flags, float sc)
{
    srb2::Draw mini_timer = srb2::Draw(x, y).flags(flags).font(srb2::Draw::Font::kZVote).scale(sc);
    
    mini_timer.patch("K_STTIMS");
    mini_timer
        .xy(6, 1)
        .align(srb2::Draw::Align::kLeft)
        .text(time == UINT32_MAX ?
        "--'--\"--" : va(
        "%i'%02i\"%02i",
        G_TicsToMinutes(time, true),
        G_TicsToSeconds(time),
        G_TicsToCentiseconds(time)
    ));
}