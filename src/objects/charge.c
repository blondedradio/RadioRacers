#include "../doomdef.h"
#include "../info.h"
#include "../k_objects.h"
#include "../p_local.h"
#include "../k_kart.h"
#include "../k_powerup.h"
#include "../m_random.h"

#define CHARGEAURA_BURSTTIME (9)
#define CHARGEAURA_SPARKRADIUS (160)

// xval1: destruction timer
// xval2: master (spawns other visuals)
void Obj_ChargeAuraThink (mobj_t *aura)
{
    if (P_MobjWasRemoved(aura->target) || !aura->target->player || (aura->extravalue1 >= CHARGEAURA_BURSTTIME))
    {
        P_RemoveMobj(aura);
    }
    else
    {
        mobj_t *mo = aura->target;
        player_t *player = mo->player;

        // Follow player
        aura->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(aura, mo->x, mo->y, mo->z + mo->height/2);
		aura->flags |= MF_NOCLIPTHING;
        aura->color = mo->color;

        aura->renderflags &= ~RF_DONTDRAW;

        fixed_t baseScale = 12*mo->scale/10;

        if (aura->extravalue1 || !player->trickcharge)
        {
            aura->extravalue1++;
            baseScale += (mo->scale / 3) * aura->extravalue1;
            aura->renderflags &= ~RF_TRANSMASK;
            aura->renderflags |= (aura->extravalue1)<<RF_TRANSSHIFT;
            if (aura->extravalue1 % 2)
                aura->renderflags |= RF_DONTDRAW;
        }

        P_SetScale(aura, baseScale);

        // Twirl
        aura->angle = aura->angle - ANG1*(player->trickcharge/TICRATE + 4);
        // Visuals
        aura->renderflags |= RF_PAPERSPRITE|RF_ADD;

        // fuck
        boolean forceinvisible = !!!(leveltime % 8);
        if (aura->extravalue1 || !(player->driftcharge > K_GetKartDriftSparkValueForStage(player, 3)))
            forceinvisible = false;

        if (forceinvisible)
            aura->renderflags |= RF_DONTDRAW;

        if (aura->extravalue2)
        {
            if (player->driftcharge)
            {
                mobj_t *spark = P_SpawnMobjFromMobj(aura, 
                    mo->scale*P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS),
                    mo->scale*P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS),
                    mo->scale*P_RandomRange(PR_DECORATION, -1*CHARGEAURA_SPARKRADIUS, CHARGEAURA_SPARKRADIUS),
                    MT_CHARGESPARK);
                spark->frame = P_RandomRange(PR_DECORATION, 1, 5);
                P_SetTarget(&spark->target, aura);
                P_SetScale(spark, 15*aura->scale/10);
            }

            if (forceinvisible)
            {
                mobj_t *flicker = P_SpawnMobjFromMobj(aura, 0, 0, 0, MT_CHARGEFLICKER);
                P_SetTarget(&flicker->target, aura);
                P_SetScale(flicker, aura->scale);
            }
        }
    }
}

void Obj_ChargeFallThink (mobj_t *charge)
{
    if (P_MobjWasRemoved(charge->target) || !charge->target->player)
    {
        P_RemoveMobj(charge);
    }
    else
    {
        mobj_t *mo = charge->target;
        player_t *player = mo->player;

        // Follow player
        charge->flags &= ~(MF_NOCLIPTHING);
		P_MoveOrigin(charge, mo->x, mo->y, mo->z);
		charge->flags |= MF_NOCLIPTHING;
        charge->color = mo->color;
        charge->angle = mo->angle + ANGLE_45 + (ANGLE_90 * charge->extravalue1);

        if (!P_IsObjectOnGround(mo))
            charge->renderflags |= RF_DONTDRAW;
        else
            charge->renderflags &= ~RF_DONTDRAW;

        fixed_t baseScale = 12*mo->scale/10;
        P_SetScale(charge, baseScale);

        charge->renderflags &= ~RF_TRANSMASK;
        if (charge->tics < 10)
            charge->renderflags |= (9 - charge->tics)<<RF_TRANSSHIFT;

        // Visuals
        charge->renderflags |= RF_PAPERSPRITE|RF_ADD;
    }
}

void Obj_ChargeFlickerThink (mobj_t *flicker)
{
    // xd
}

void Obj_ChargeSparkThink (mobj_t *spark)
{
    // xd
    spark->renderflags |= RF_FULLBRIGHT|RF_ADD;
}

void Obj_ChargeReleaseThink (mobj_t *flicker)
{
    // xd
}

void Obj_ChargeExtraThink (mobj_t *flicker)
{
    // xd
}