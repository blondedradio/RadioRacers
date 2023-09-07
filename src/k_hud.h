// SONIC ROBO BLAST 2 KART
//-----------------------------------------------------------------------------
// Copyright (C) 2018-2020 by Kart Krew
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  k_hud.h
/// \brief HUD drawing functions exclusive to Kart

#ifndef __K_HUD__
#define __K_HUD__

#include "doomtype.h"
#include "doomstat.h"
#include "hu_stuff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RINGANIM_NUMFRAMES 10
#define RINGANIM_DELAYMAX 5

#define POS_DELAY_TIME 10

struct trackingResult_t
{
	fixed_t x, y;
	fixed_t scale;
	boolean onScreen;
	INT32 angle, pitch;
	fixed_t fov;
};

void K_ObjectTracking(trackingResult_t *result, const vector3_t *point, boolean reverse);

tic_t K_TranslateTimer(tic_t drawtime, UINT8 mode, INT32 *return_jitter);

const char *K_GetItemPatch(UINT8 item, boolean tiny);
void K_LoadKartHUDGraphics(void);
void K_drawKartHUD(void);
void K_drawKartFreePlay(void);
void K_drawKartPowerUps(void);
void K_drawKartTimestamp(tic_t drawtime, INT32 TX, INT32 TY, INT32 splitflags, UINT8 mode);
void K_drawKart2PTimestamp(void);
void K_drawKart4PTimestamp(void);
void K_DrawMapThumbnail(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, UINT16 map, const UINT8 *colormap);
void K_DrawLikeMapThumbnail(fixed_t x, fixed_t y, fixed_t width, UINT32 flags, patch_t *patch, const UINT8 *colormap);
void K_drawTargetHUD(const vector3_t *origin, player_t *player);
void K_drawButton(fixed_t x, fixed_t y, INT32 flags, patch_t *button[2], boolean pressed);
void K_drawButtonAnim(INT32 x, INT32 y, INT32 flags, patch_t *button[2], tic_t animtic);
void K_DrawSticker(INT32 x, INT32 y, INT32 width, INT32 flags, boolean isSmall);

extern patch_t *kp_capsuletarget_arrow[2][2];
extern patch_t *kp_capsuletarget_icon[2];
extern patch_t *kp_capsuletarget_far[2][2];
extern patch_t *kp_capsuletarget_far_text[2];
extern patch_t *kp_capsuletarget_near[2][8];

extern patch_t *kp_superflickytarget[2][4];

extern patch_t *kp_spraycantarget_far[2][6];
extern patch_t *kp_spraycantarget_near[2][6];

extern patch_t *kp_autoroulette;

extern patch_t *kp_button_a[2][2];
extern patch_t *kp_button_b[2][2];
extern patch_t *kp_button_c[2][2];
extern patch_t *kp_button_x[2][2];
extern patch_t *kp_button_y[2][2];
extern patch_t *kp_button_z[2][2];
extern patch_t *kp_button_start[2];
extern patch_t *kp_button_l[2];
extern patch_t *kp_button_r[2];
extern patch_t *kp_button_up[2];
extern patch_t *kp_button_down[2];
extern patch_t *kp_button_right[2];
extern patch_t *kp_button_left[2];

extern patch_t *kp_eggnum[6];

#ifdef __cplusplus
} // extern "C"
#endif

#endif
