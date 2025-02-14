// RadioRacers
//-----------------------------------------------------------------------------
// Copyright (C) 2025 by $HOME
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file radioracers/cvar/rr_cvar.c

#include "../rr_cvar.h"
#include "../../doomstat.h"

void KartLocalEncore_OnChange(void)
{
    localencore = (boolean)cv_applylocalencore.value;
    CONS_Printf(M_GetText("Encore Mode palettes will be \x82%s\x80 next round.\n"), cv_applylocalencore.string);
}

void KartHaki_OnChange(void)
{
    hakimode = (boolean)cv_applyhaki.value;
    // TODO: Play dramatic sound.
    CONS_Printf(M_GetText("Your observation haki will be turned \x82%s\x80 next round.\n"), cv_applyhaki.string);
}