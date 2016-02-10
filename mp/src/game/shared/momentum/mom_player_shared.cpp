#include "cbase.h"
#include "mom_player_shared.h"


#include "effect_dispatch_data.h"
#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#else
#include "c_te_effect_dispatch.h"
#endif
#include "weapon_csbase.h"
#include "engine/ivdebugoverlay.h"
#include "decals.h"

#include "tier0/memdbgon.h"


ConVar sv_showimpacts("sv_showimpacts", "0", FCVAR_REPLICATED, "Shows client (red) and server (blue) bullet impact point (1=both, 2=client-only, 3=server-only)");

void CMomentumPlayer::GetBulletTypeParameters(
    int iBulletType,
    float &fPenetrationPower,
    float &flPenetrationDistance)
{

    //MIKETODO: make ammo types come from a script file.
    if (IsAmmoType(iBulletType, BULLET_PLAYER_50AE))
    {
        fPenetrationPower = 30;
        flPenetrationDistance = 1000.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_762MM))
    {
        fPenetrationPower = 39;
        flPenetrationDistance = 5000.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_556MM) ||
        IsAmmoType(iBulletType, BULLET_PLAYER_556MM_BOX))
    {
        fPenetrationPower = 35;
        flPenetrationDistance = 4000.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_338MAG))
    {
        fPenetrationPower = 45;
        flPenetrationDistance = 8000.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_9MM))
    {
        fPenetrationPower = 21;
        flPenetrationDistance = 800.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_BUCKSHOT))
    {
        fPenetrationPower = 0;
        flPenetrationDistance = 0.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_45ACP))
    {
        fPenetrationPower = 15;
        flPenetrationDistance = 500.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_357SIG))
    {
        fPenetrationPower = 25;
        flPenetrationDistance = 800.0;
    }
    else if (IsAmmoType(iBulletType, BULLET_PLAYER_57MM))
    {
        fPenetrationPower = 30;
        flPenetrationDistance = 2000.0;
    }
    else
    {
        // What kind of ammo is this?
        Assert(false);
        fPenetrationPower = 0;
        flPenetrationDistance = 0.0;
    }
}

static bool TraceToExit(Vector &start, Vector &dir, Vector &end, float flStepSize, float flMaxDistance)
{
    float flDistance = 0;
    Vector last = start;

    while (flDistance <= flMaxDistance)
    {
        flDistance += flStepSize;

        end = start + flDistance *dir;

        if ((UTIL_PointContents(end) & MASK_SOLID) == 0)
        {
            // found first free point
            return true;
        }
    }

    return false;
}

inline void UTIL_TraceLineIgnoreTwoEntities(const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask,
    const IHandleEntity *ignore, const IHandleEntity *ignore2, int collisionGroup, trace_t *ptr)
{
    Ray_t ray;
    ray.Init(vecAbsStart, vecAbsEnd);
    CTraceFilterSkipTwoEntities traceFilter(ignore, ignore2, collisionGroup);
    enginetrace->TraceRay(ray, mask, &traceFilter, ptr);
    if (r_visualizetraces.GetBool())
    {
        DebugDrawLine(ptr->startpos, ptr->endpos, 255, 0, 0, true, -1.0f);
    }
}


void CMomentumPlayer::FireBullet(
    Vector vecSrc,	// shooting postion
    const QAngle &shootAngles,  //shooting angle
    float vecSpread, // spread vector
    float flDistance, // max distance 
    int iPenetration, // how many obstacles can be penetrated
    int iBulletType, // ammo type
    int iDamage, // base damage
    float flRangeModifier, // damage range modifier
    CBaseEntity *pevAttacker, // shooter
    bool bDoEffects,
    float x,
    float y
    )
{
    float fCurrentDamage = iDamage;   // damage of the bullet at it's current trajectory
    float flCurrentDistance = 0.0;  //distance that the bullet has traveled so far

    Vector vecDirShooting, vecRight, vecUp;
    AngleVectors(shootAngles, &vecDirShooting, &vecRight, &vecUp);

    // MIKETODO: put all the ammo parameters into a script file and allow for CS-specific params.
    float flPenetrationPower = 0;		// thickness of a wall that this bullet can penetrate
    float flPenetrationDistance = 0;	// distance at which the bullet is capable of penetrating a wall
    float flDamageModifier = 0.5;		// default modification of bullets power after they go through a wall.
    float flPenetrationModifier = 1.f;

    GetBulletTypeParameters(iBulletType, flPenetrationPower, flPenetrationDistance);


    if (!pevAttacker)
        pevAttacker = this;  // the default attacker is ourselves

    // add the spray 
    Vector vecDir = vecDirShooting +
        x * vecSpread * vecRight +
        y * vecSpread * vecUp;

    VectorNormalize(vecDir);

    bool bFirstHit = true;

    CBasePlayer *lastPlayerHit = NULL;

    //MDLCACHE_CRITICAL_SECTION();
    while (fCurrentDamage > 0)
    {
        Vector vecEnd = vecSrc + vecDir * flDistance;

        trace_t tr; // main enter bullet trace

        UTIL_TraceLineIgnoreTwoEntities(vecSrc, vecEnd, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, this, lastPlayerHit, COLLISION_GROUP_NONE, &tr);
        {
            CTraceFilterSkipTwoEntities filter(this, lastPlayerHit, COLLISION_GROUP_NONE);

            // Check for player hitboxes extending outside their collision bounds
            const float rayExtension = 40.0f;
            UTIL_ClipTraceToPlayers(vecSrc, vecEnd + vecDir * rayExtension, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, &filter, &tr);
        }

        lastPlayerHit = ToBasePlayer(tr.m_pEnt);

        if (tr.fraction == 1.0f)
            break; // we didn't hit anything, stop tracing shoot


        bFirstHit = false;

#ifndef CLIENT_DLL
        //
        // Propogate a bullet impact event
        // @todo Add this for shotgun pellets (which dont go thru here)
        //
        IGameEvent * event = gameeventmanager->CreateEvent("bullet_impact");
        if (event)
        {
            event->SetInt("userid", GetUserID());
            event->SetFloat("x", tr.endpos.x);
            event->SetFloat("y", tr.endpos.y);
            event->SetFloat("z", tr.endpos.z);
            gameeventmanager->FireEvent(event);
        }
#endif

        /************* MATERIAL DETECTION ***********/
        surfacedata_t *pSurfaceData = physprops->GetSurfaceData(tr.surface.surfaceProps);
        int iEnterMaterial = pSurfaceData->game.material;

        //GetMaterialParameters(iEnterMaterial, flPenetrationModifier, flDamageModifier);

        bool hitGrate = ((tr.contents & CONTENTS_GRATE) == 1);

        // since some railings in de_inferno are CONTENTS_GRATE but CHAR_TEX_CONCRETE, we'll trust the
        // CONTENTS_GRATE and use a high damage modifier.
        if (hitGrate)
        {
            // If we're a concrete grate (TOOLS/TOOLSINVISIBLE texture) allow more penetrating power.
            flPenetrationModifier = 1.0f;
            flDamageModifier = 0.99f;
        }

#ifdef CLIENT_DLL
        if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 2)
        {
            // draw red client impact markers
            debugoverlay->AddBoxOverlay(tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), QAngle(0, 0, 0), 255, 0, 0, 127, 4);

            if (tr.m_pEnt && tr.m_pEnt->IsPlayer())
            {
                C_BasePlayer *player = ToBasePlayer(tr.m_pEnt);
                player->DrawClientHitboxes(4, true);
            }
        }
#else
        if (sv_showimpacts.GetInt() == 1 || sv_showimpacts.GetInt() == 3)
        {
            // draw blue server impact markers
            NDebugOverlay::Box(tr.endpos, Vector(-2, -2, -2), Vector(2, 2, 2), 0, 0, 255, 127, 4);

            if (tr.m_pEnt && tr.m_pEnt->IsPlayer())
            {
                CBasePlayer *player = ToBasePlayer(tr.m_pEnt);
                player->DrawServerHitboxes(4, true);
            }
        }
#endif

        //calculate the damage based on the distance the bullet travelled.
        flCurrentDistance += tr.fraction * flDistance;
        fCurrentDamage *= pow(flRangeModifier, (flCurrentDistance / 500));

        // check if we reach penetration distance, no more penetrations after that
        if (flCurrentDistance > flPenetrationDistance && iPenetration > 0)
            iPenetration = 0;

        int iDamageType = DMG_BULLET | DMG_NEVERGIB;

        if (bDoEffects)
        {
            // See if the bullet ended up underwater + started out of the water
            if (enginetrace->GetPointContents(tr.endpos) & (CONTENTS_WATER | CONTENTS_SLIME))
            {
                trace_t waterTrace;
                UTIL_TraceLine(vecSrc, tr.endpos, (MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace);

                if (waterTrace.allsolid != 1)
                {
                    CEffectData	data;
                    data.m_vOrigin = waterTrace.endpos;
                    data.m_vNormal = waterTrace.plane.normal;
                    data.m_flScale = random->RandomFloat(8, 12);

                    if (waterTrace.contents & CONTENTS_SLIME)
                    {
                        data.m_fFlags |= FX_WATER_IN_SLIME;
                    }

                    DispatchEffect("gunshotsplash", data);
                }
            }
            else
            {
                //Do Regular hit effects

                // Don't decal nodraw surfaces
                if (!(tr.surface.flags & (SURF_SKY | SURF_NODRAW | SURF_HINT | SURF_SKIP)))
                {
                    CBaseEntity *pEntity = tr.m_pEnt;
                    //MOM_TODO: question the 
                    if (pEntity)
                        UTIL_ImpactTrace(&tr, iDamageType);
                    //if (!(pEntity && pEntity->GetTeamNumber() == GetTeamNumber()))
                    //{
                    //    UTIL_ImpactTrace(&tr, iDamageType);
                    //}
                }
            }
        } // bDoEffects

        // add damage to entity that we hit

#ifndef CLIENT_DLL
        ClearMultiDamage();

        CTakeDamageInfo info(pevAttacker, pevAttacker, fCurrentDamage, iDamageType);
        CalculateBulletDamageForce(&info, iBulletType, vecDir, tr.endpos);
        tr.m_pEnt->DispatchTraceAttack(info, vecDir, &tr);

        TraceAttackToTriggers(info, tr.startpos, tr.endpos, vecDir);

        ApplyMultiDamage();
#endif

        // check if bullet can penetarte another entity
        if (iPenetration == 0 && !hitGrate)
            break; // no, stop

        // If we hit a grate with iPenetration == 0, stop on the next thing we hit
        if (iPenetration < 0)
            break;

        Vector penetrationEnd;

        // try to penetrate object, maximum penetration is 128 inch
        if (!TraceToExit(tr.endpos, vecDir, penetrationEnd, 24, 128))
            break;

        // find exact penetration exit
        trace_t exitTr;
        UTIL_TraceLine(penetrationEnd, tr.endpos, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, NULL, &exitTr);

        if (exitTr.m_pEnt != tr.m_pEnt && exitTr.m_pEnt != NULL)
        {
            // something was blocking, trace again
            UTIL_TraceLine(penetrationEnd, tr.endpos, MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_HITBOX, exitTr.m_pEnt, COLLISION_GROUP_NONE, &exitTr);
        }

        // get material at exit point
        pSurfaceData = physprops->GetSurfaceData(exitTr.surface.surfaceProps);
        int iExitMaterial = pSurfaceData->game.material;

        hitGrate = hitGrate && (exitTr.contents & CONTENTS_GRATE);

        // if enter & exit point is wood or metal we assume this is 
        // a hollow crate or barrel and give a penetration bonus
        if (iEnterMaterial == iExitMaterial)
        {
            if (iExitMaterial == CHAR_TEX_WOOD ||
                iExitMaterial == CHAR_TEX_METAL)
            {
                flPenetrationModifier *= 2;
            }
        }

        float flTraceDistance = VectorLength(exitTr.endpos - tr.endpos);

        // check if bullet has enough power to penetrate this distance for this material
        if (flTraceDistance > (flPenetrationPower * flPenetrationModifier))
            break; // bullet hasn't enough power to penetrate this distance

        // penetration was successful

        // bullet did penetrate object, exit Decal
        if (bDoEffects)
        {
            UTIL_ImpactTrace(&exitTr, iDamageType);
        }

        //setup new start end parameters for successive trace

        flPenetrationPower -= flTraceDistance / flPenetrationModifier;
        flCurrentDistance += flTraceDistance;

        // NDebugOverlay::Box( exitTr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,255,0,127, 8 );

        vecSrc = exitTr.endpos;
        flDistance = (flDistance - flCurrentDistance) * 0.5;

        // reduce damage power each time we hit something other than a grate
        fCurrentDamage *= flDamageModifier;

        // reduce penetration counter
        iPenetration--;
    }

}

void CMomentumPlayer::KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change)
{
    float flKickUp;
    float flKickLateral;

    if (m_iShotsFired == 1) // This is the first round fired
    {
        flKickUp = up_base;
        flKickLateral = lateral_base;
    }
    else
    {
        flKickUp = up_base + m_iShotsFired*up_modifier;
        flKickLateral = lateral_base + m_iShotsFired*lateral_modifier;
    }


    QAngle angle = GetPunchAngle();

    angle.x -= flKickUp;
    if (angle.x < -1 * up_max)
        angle.x = -1 * up_max;

    if (m_iDirection == 1)
    {
        angle.y += flKickLateral;
        if (angle.y > lateral_max)
            angle.y = lateral_max;
    }
    else
    {
        angle.y -= flKickLateral;
        if (angle.y < -1 * lateral_max)
            angle.y = -1 * lateral_max;
    }

    if (!SharedRandomInt("KickBack", 0, direction_change))
        m_iDirection = 1 - m_iDirection;

    SetPunchAngle(angle);
}