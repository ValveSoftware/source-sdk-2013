#include "cbase.h"
#include "mom_gamemovement.h"
#include "in_buttons.h"
#include <stdarg.h>
#include "movevars_shared.h"
#include "engine/IEngineTrace.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "decals.h"
#include "coordsize.h"

#include "tier0/memdbgon.h"

#define	STOP_EPSILON		0.1
#define	MAX_CLIP_PLANES		5

extern bool g_bMovementOptimizations;
// remove this eventually
ConVar sv_ramp_fix("sv_ramp_fix", "1");

#ifndef CLIENT_DLL
#include "env_player_surface_trigger.h"
static ConVar dispcoll_drawplane("dispcoll_drawplane", "0");
#endif

#ifndef player
#define player GetMomentumPlayer()

CMomentumGameMovement::CMomentumGameMovement()
{

}

float CMomentumGameMovement::LadderLateralMultiplier(void) const
{
    if (mv->m_nButtons & IN_DUCK)
    {
        return 1.0f;
    }
    else
    {
        return 0.5f;
    }
}

float CMomentumGameMovement::ClimbSpeed(void) const
{
    if (mv->m_nButtons & IN_DUCK)
    {
        return BaseClass::ClimbSpeed() * DuckSpeedMultiplier;
    }
    else
    {
        return BaseClass::ClimbSpeed();
    }
}

void CMomentumGameMovement::WalkMove()
{
    BaseClass::WalkMove();
    CheckForLadders(player->GetGroundEntity() != NULL);
}

void CMomentumGameMovement::CheckForLadders(bool wasOnGround)
{
    if (!wasOnGround)
    {
        // If we're higher than the last place we were on the ground, bail - obviously we're not dropping
        // past a ladder we might want to grab.
        if (mv->GetAbsOrigin().z > player->m_lastStandingPos.z)
            return;

        Vector dir = -player->m_lastStandingPos + mv->GetAbsOrigin();
        if (!dir.x && !dir.y)
        {
            // If we're dropping straight down, we don't know which way to look for a ladder.  Oh well.
            return;
        }

        dir.z = 0.0f;
        float dist = dir.NormalizeInPlace();
        if (dist > 64.0f)
        {
            // Don't grab ladders too far behind us.
            return;
        }

        trace_t trace;

        TracePlayerBBox(
            mv->GetAbsOrigin(),
            player->m_lastStandingPos - dir*(5 + dist),
            (PlayerSolidMask() & (~CONTENTS_PLAYERCLIP)), COLLISION_GROUP_PLAYER_MOVEMENT, trace);

        if (trace.fraction != 1.0f && OnLadder(trace) && trace.plane.normal.z != 1.0f)
        {
            if (player->CanGrabLadder(trace.endpos, trace.plane.normal))
            {
                player->SetMoveType(MOVETYPE_LADDER);
                player->SetMoveCollide(MOVECOLLIDE_DEFAULT);

                player->SetLadderNormal(trace.plane.normal);
                mv->m_vecVelocity.Init();

                // The ladder check ignored playerclips, to fix a bug exposed by de_train, where a clipbrush is
                // flush with a ladder.  This causes the above tracehull to fail unless we ignore playerclips.
                // However, we have to check for playerclips before we snap to that pos, so we don't warp a
                // player into a clipbrush.
                TracePlayerBBox(
                    mv->GetAbsOrigin(),
                    player->m_lastStandingPos - dir*(5 + dist),
                    PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace);

                mv->SetAbsOrigin(trace.endpos);
            }
        }
    }
    else
    {
        player->m_lastStandingPos = mv->GetAbsOrigin();
    }
}

bool CMomentumGameMovement::LadderMove(void)
{
    bool isOnLadder = BaseClass::LadderMove();
    if (isOnLadder && player)
    {
        player->SurpressLadderChecks(mv->GetAbsOrigin(), player->m_vecLadderNormal);
    }
    return isOnLadder;

}

bool CMomentumGameMovement::OnLadder(trace_t &trace)
{
    if (trace.plane.normal.z == 1.0f)
        return false;

    return BaseClass::OnLadder(trace);
}

void CMomentumGameMovement::HandleDuckingSpeedCrop()
{
    if (!m_iSpeedCropped && (player->GetFlags() & FL_DUCKING))
    {
        float frac = 0.34f;//0.33333333f;
        mv->m_flForwardMove *= frac;
        mv->m_flSideMove *= frac;
        mv->m_flUpMove *= frac;
        m_iSpeedCropped = true;
    }
}

bool CMomentumGameMovement::CanUnduck()
{
    trace_t trace;
    Vector newOrigin;

    VectorCopy(mv->GetAbsOrigin(), newOrigin);

    if (player->GetGroundEntity() != NULL)
    {
        newOrigin += VEC_DUCK_HULL_MIN - VEC_HULL_MIN;
    }
    else
    {
        // If in air an letting go of croush, make sure we can offset origin to make
        //  up for uncrouching
        Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
        Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;

        newOrigin += -0.5f * (hullSizeNormal - hullSizeCrouch);
    }

    UTIL_TraceHull(mv->GetAbsOrigin(), newOrigin, VEC_HULL_MIN, VEC_HULL_MAX, PlayerSolidMask(), player, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

    if (trace.startsolid || (trace.fraction != 1.0f))
        return false;

    return true;
}

void CMomentumGameMovement::Duck(void)
{
    int buttonsChanged = (mv->m_nOldButtons ^ mv->m_nButtons);	// These buttons have changed this frame
    int buttonsPressed = buttonsChanged & mv->m_nButtons;			// The changed ones still down are "pressed"
    int buttonsReleased = buttonsChanged & mv->m_nOldButtons;		// The changed ones which were previously down are "released"

    if (mv->m_nButtons & IN_DUCK)
    {
        mv->m_nOldButtons |= IN_DUCK;
    }
    else
    {
        mv->m_nOldButtons &= ~IN_DUCK;
    }

    if (IsDead())
    {
        // Unduck
        if (player->GetFlags() & FL_DUCKING)
        {
            FinishUnDuck();
        }
        return;
    }

    HandleDuckingSpeedCrop();

    // Holding duck, in process of ducking or fully ducked?
    if ((mv->m_nButtons & IN_DUCK) || (player->m_Local.m_bDucking) || (player->GetFlags() & FL_DUCKING))
    {
        if (mv->m_nButtons & IN_DUCK)
        {
            bool alreadyDucked = (player->GetFlags() & FL_DUCKING) ? true : false;

            if ((buttonsPressed & IN_DUCK) && !(player->GetFlags() & FL_DUCKING))
            {
                // Use 1 second so super long jump will work
                player->m_Local.m_flDucktime = 1000;
                player->m_Local.m_bDucking = true;
            }

            float duckmilliseconds = max(0.0f, 1000.0f - (float) player->m_Local.m_flDucktime);
            float duckseconds = duckmilliseconds / 1000.0f;

            //time = max( 0.0, ( 1.0 - (float)player->m_Local.m_flDucktime / 1000.0 ) );

            if (player->m_Local.m_bDucking)
            {
                // Finish ducking immediately if duck time is over or not on ground
                if ((duckseconds > TIME_TO_DUCK) ||
                    (player->GetGroundEntity() == NULL) ||
                    alreadyDucked)
                {
                    FinishDuck();
                }
                else
                {
                    // Calc parametric time
                    float duckFraction = SimpleSpline(duckseconds / TIME_TO_DUCK);
                    SetDuckedEyeOffset(duckFraction);
                }
            }
        }
        else
        {
            // Try to unduck unless automovement is not allowed
            // NOTE: When not onground, you can always unduck
            if (player->m_Local.m_bAllowAutoMovement || player->GetGroundEntity() == NULL)
            {
                if ((buttonsReleased & IN_DUCK) && (player->GetFlags() & FL_DUCKING))
                {
                    // Use 1 second so super long jump will work
                    player->m_Local.m_flDucktime = 1000;
                    player->m_Local.m_bDucking = true;  // or unducking
                }

                float duckmilliseconds = max(0.0f, 1000.0f - (float) player->m_Local.m_flDucktime);
                float duckseconds = duckmilliseconds / 1000.0f;

                if (CanUnduck())
                {
                    if (player->m_Local.m_bDucking ||
                        player->m_Local.m_bDucked) // or unducking
                    {
                        // Finish ducking immediately if duck time is over or not on ground
                        if ((duckseconds > TIME_TO_UNDUCK) ||
                            (player->GetGroundEntity() == NULL))
                        {
                            FinishUnDuck();
                        }
                        else
                        {
                            // Calc parametric time
                            float duckFraction = SimpleSpline(1.0f - (duckseconds / TIME_TO_UNDUCK));
                            SetDuckedEyeOffset(duckFraction);
                        }
                    }
                }
                else
                {
                    // Still under something where we can't unduck, so make sure we reset this timer so
                    //  that we'll unduck once we exit the tunnel, etc.
                    player->m_Local.m_flDucktime = 1000;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Stop ducking
//-----------------------------------------------------------------------------
void CMomentumGameMovement::FinishUnDuck(void)
{
    trace_t trace;
    Vector newOrigin;

    VectorCopy(mv->GetAbsOrigin(), newOrigin);

    if (player->GetGroundEntity() != NULL)
    {
        newOrigin += VEC_DUCK_HULL_MIN - VEC_HULL_MIN;
    }
    else
    {
        // If in air an letting go of croush, make sure we can offset origin to make
        //  up for uncrouching
        Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
        Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;

        Vector viewDelta = -0.5f * (hullSizeNormal - hullSizeCrouch);

        VectorAdd(newOrigin, viewDelta, newOrigin);
    }

    player->m_Local.m_bDucked = false;
    player->RemoveFlag(FL_DUCKING);
    player->m_Local.m_bDucking = false;
    player->SetViewOffset(GetPlayerViewOffset(false));
    player->m_Local.m_flDucktime = 0;

    mv->SetAbsOrigin(newOrigin);

    // Recategorize position since ducking can change origin
    CategorizePosition();
}

//-----------------------------------------------------------------------------
// Purpose: Finish ducking
//-----------------------------------------------------------------------------
void CMomentumGameMovement::FinishDuck(void)
{
    Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
    Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;

    Vector viewDelta = 0.5f * (hullSizeNormal - hullSizeCrouch);

    player->SetViewOffset(GetPlayerViewOffset(true));
    player->AddFlag(FL_DUCKING);
    player->m_Local.m_bDucking = false;

    if (!player->m_Local.m_bDucked)
    {

        Vector org = mv->GetAbsOrigin();

        if (player->GetGroundEntity() != NULL)
        {
            org -= VEC_DUCK_HULL_MIN - VEC_HULL_MIN;
        }
        else
        {
            org += viewDelta;
        }
        mv->SetAbsOrigin(org);

        player->m_Local.m_bDucked = true;
    }

    // See if we are stuck?
    FixPlayerCrouchStuck(true);

    // Recategorize position since ducking can change origin
    CategorizePosition();
}


void CMomentumGameMovement::PlayerMove()
{
    BaseClass::PlayerMove();

    if (player->IsAlive())
    {
        // Check if our eye height is too close to the ceiling and lower it.
        // This is needed because we have taller models with the old collision bounds.

        const float eyeClearance = 12.0f; // eye pos must be this far below the ceiling

        Vector offset = player->GetViewOffset();

        Vector vHullMin = GetPlayerMins(player->m_Local.m_bDucked);
        vHullMin.z = 0.0f;
        Vector vHullMax = GetPlayerMaxs(player->m_Local.m_bDucked);

        Vector start = player->GetAbsOrigin();
        start.z += vHullMax.z;
        Vector end = start;
        end.z += eyeClearance - vHullMax.z;
        end.z += player->m_Local.m_bDucked ? VEC_DUCK_VIEW.z : VEC_VIEW.z;

        vHullMax.z = 0.0f;

        Vector fudge(1, 1, 0);
        vHullMin += fudge;
        vHullMax -= fudge;

        trace_t trace;
        Ray_t ray;
        ray.Init(start, end, vHullMin, vHullMax);
        UTIL_TraceRay(ray, PlayerSolidMask(), mv->m_nPlayerHandle.Get(), COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

        if (trace.fraction < 1.0f)
        {
            float est = start.z + trace.fraction * (end.z - start.z) - player->GetAbsOrigin().z - eyeClearance;
            if ((player->GetFlags() & FL_DUCKING) == 0 && !player->m_Local.m_bDucking && !player->m_Local.m_bDucked)
            {
                offset.z = est;
            }
            else
            {
                offset.z = min(est, offset.z);
            }
            player->SetViewOffset(offset);
        }
        else
        {
            if ((player->GetFlags() & FL_DUCKING) == 0 && !player->m_Local.m_bDucking && !player->m_Local.m_bDucked)
            {
                player->SetViewOffset(VEC_VIEW);
            }
            else if (player->m_Local.m_bDucked && !player->m_Local.m_bDucking)
            {
                player->SetViewOffset(VEC_DUCK_VIEW);
            }
        }
    }
}


bool CMomentumGameMovement::CheckJumpButton()
{

    if (player->pl.deadflag)
    {
        mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
        return false;
    }

    // See if we are waterjumping.  If so, decrement count and return.
    if (player->m_flWaterJumpTime)
    {
        player->m_flWaterJumpTime -= gpGlobals->frametime;
        if (player->m_flWaterJumpTime < 0)
            player->m_flWaterJumpTime = 0;

        return false;
    }

    // If we are in the water most of the way...
    if (player->GetWaterLevel() >= 2)
    {
        // swimming, not jumping
        SetGroundEntity(NULL);

        if (player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
            mv->m_vecVelocity[2] = 100;
        else if (player->GetWaterType() == CONTENTS_SLIME)
            mv->m_vecVelocity[2] = 80;

        // play swiming sound
        if (player->m_flSwimSoundTime <= 0)
        {
            // Don't play sound again for 1 second
            player->m_flSwimSoundTime = 1000;
            PlaySwimSound();
        }

        return false;
    }

    // No more effect
    if (player->GetGroundEntity() == NULL)
    {
        mv->m_nOldButtons |= IN_JUMP;
        return false;		// in air, so no effect
    }

    //if (mv->m_nOldButtons & IN_JUMP)
    //	return false;		// don't pogo stick

    // In the air now.
    SetGroundEntity(NULL);

    player->PlayStepSound((Vector &) mv->GetAbsOrigin(), player->m_pSurfaceData, 1.0, true);

    //MoveHelper()->PlayerSetAnimation( PLAYER_JUMP );
    //player->DoAnimationEvent(PLAYERANIMEVENT_JUMP);

    float flGroundFactor = 1.0f;
    if (player->m_pSurfaceData)
    {
        flGroundFactor = player->m_pSurfaceData->game.jumpFactor;
    }

    // if we weren't ducking, bots and hostages do a crouchjump programatically
    if ((!player || player->IsBot()) && !(mv->m_nButtons & IN_DUCK))
    {
        //player->m_duckUntilOnGround = true;
        FinishDuck();
    }

    // Acclerate upward
    // If we are ducking...
    float startz = mv->m_vecVelocity[2];
    if ((player->m_Local.m_bDucking) || (player->GetFlags() & FL_DUCKING))
    {
        mv->m_vecVelocity[2] = flGroundFactor * sqrt(2 * 800 * 57.0);  // 2 * gravity * height
    }
    else
    {
        mv->m_vecVelocity[2] += flGroundFactor * sqrt(2 * 800 * 57.0);  // 2 * gravity * height
    }

    FinishGravity();

    mv->m_outWishVel.z += mv->m_vecVelocity[2] - startz;
    mv->m_outStepHeight += 0.1f;

    // Flag that we jumped.
    mv->m_nOldButtons |= IN_JUMP;	// don't jump again until released
    return true;

}

void CMomentumGameMovement::CategorizePosition()
{
    Vector point;
    trace_t pm;

    // Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
    player->m_surfaceFriction = 1.0f;

    // if the player hull point one unit down is solid, the player
    // is on ground

    // see if standing on something solid	

    // Doing this before we move may introduce a potential latency in water detection, but
    // doing it after can get us stuck on the bottom in water if the amount we move up
    // is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
    // this several times per frame, so we really need to avoid sticking to the bottom of
    // water on each call, and the converse case will correct itself if called twice.
    CheckWater();

    // observers don't have a ground entity
    if (player->IsObserver())
        return;

    float flOffset = 2.0f;

    point[0] = mv->GetAbsOrigin()[0];
    point[1] = mv->GetAbsOrigin()[1];
    point[2] = mv->GetAbsOrigin()[2] - flOffset;

    Vector bumpOrigin;
    bumpOrigin = mv->GetAbsOrigin();

    // Shooting up really fast.  Definitely not on ground.
    // On ladder moving up, so not on ground either
    // NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY 140.0f

    float zvel = mv->m_vecVelocity[2];
    bool bMovingUp = zvel > 0.0f;
    bool bMovingUpRapidly = zvel > NON_JUMP_VELOCITY;
    float flGroundEntityVelZ = 0.0f;
    if (bMovingUpRapidly)
    {
        // Tracker 73219, 75878:  ywb 8/2/07
        // After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
        //  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
        // We need to account for standing on a moving ground object in that case in order to determine if we really 
        //  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
        //  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
        CBaseEntity *ground = player->GetGroundEntity();
        if (ground)
        {
            flGroundEntityVelZ = ground->GetAbsVelocity().z;
            bMovingUpRapidly = (zvel - flGroundEntityVelZ) > NON_JUMP_VELOCITY;
        }
    }

    // Was on ground, but now suddenly am not
    if (bMovingUpRapidly ||
        (bMovingUp && player->GetMoveType() == MOVETYPE_LADDER))
    {
        SetGroundEntity(NULL);
    }
    else
    {
        // Try and move down.
        TryTouchGround(bumpOrigin, point, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);

        // Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
        if (!pm.m_pEnt || pm.plane.normal[2] < 0.7)
        {
            // Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
            TryTouchGroundInQuadrants(bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);

            if (!pm.m_pEnt || pm.plane.normal[2] < 0.7)
            {
                SetGroundEntity(NULL);
                // probably want to add a check for a +z velocity too!
                if ((mv->m_vecVelocity.z > 0.0f) &&
                    (player->GetMoveType() != MOVETYPE_NOCLIP))
                {
                    player->m_surfaceFriction = 0.25f;
                }
            }
            else
            {
                if (m_flReflectNormal == NO_REFL_NORMAL_CHANGE)
                {
                    DoLateReflect();
                    CategorizePosition();

                    return;
                }

                SetGroundEntity(&pm);
            }
        }
        else
        {
            if (m_flReflectNormal == NO_REFL_NORMAL_CHANGE)
            {
                DoLateReflect();
                CategorizePosition();

                return;
            }

            SetGroundEntity(&pm);  // Otherwise, point to index of ent under us.
        }

#ifndef CLIENT_DLL

        // If our gamematerial has changed, tell any player surface triggers that are watching
        IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
        surfacedata_t *pSurfaceProp = physprops->GetSurfaceData(pm.surface.surfaceProps);
        char cCurrGameMaterial = pSurfaceProp->game.material;
        if (!player->GetGroundEntity())
        {
            cCurrGameMaterial = 0;
        }

        // Changed?
        if (player->m_chPreviousTextureType != cCurrGameMaterial)
        {
            CEnvPlayerSurfaceTrigger::SetPlayerSurface(player, cCurrGameMaterial);
        }

        player->m_chPreviousTextureType = cCurrGameMaterial;
#endif
    }
}

void CMomentumGameMovement::FullWalkMove()
{
    if (!CheckWater())
    {
        StartGravity();
    }

    // If we are leaping out of the water, just update the counters.
    if (player->m_flWaterJumpTime)
    {
        WaterJump();
        TryPlayerMove();
        // See if we are still in water?
        CheckWater();
        return;
    }

    // If we are swimming in the water, see if we are nudging against a place we can jump up out
    //  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
    if (player->GetWaterLevel() >= WL_Waist)
    {
        if (player->GetWaterLevel() == WL_Waist)
        {
            CheckWaterJump();
        }

        // If we are falling again, then we must not trying to jump out of water any more.
        if (mv->m_vecVelocity[2] < 0 &&
            player->m_flWaterJumpTime)
        {
            player->m_flWaterJumpTime = 0;
        }

        // Was jump button pressed?
        if (mv->m_nButtons & IN_JUMP)
        {
            CheckJumpButton();
        }
        else
        {
            mv->m_nOldButtons &= ~IN_JUMP;
        }

        // Perform regular water movement
        WaterMove();

        // Redetermine position vars
        CategorizePosition();

        // If we are on ground, no downward velocity.
        if (player->GetGroundEntity() != NULL)
        {
            mv->m_vecVelocity[2] = 0;
        }
    }
    else
        // Not fully underwater
    {
        // Was jump button pressed?
        if (mv->m_nButtons & IN_JUMP)
        {
            CheckJumpButton();
        }
        else
        {
            mv->m_nOldButtons &= ~IN_JUMP;
        }

        // Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
        //  we don't slow when standing still, relative to the conveyor.
        if (player->GetGroundEntity() != NULL)
        {
            mv->m_vecVelocity[2] = 0.0;
            Friction();
        }

        // Make sure velocity is valid.
        CheckVelocity();

        // By default assume we did the reflect for WalkMove()
        m_flReflectNormal = 1.0f;

        if (player->GetGroundEntity() != NULL)
        {
            WalkMove();
        }
        else
        {
            AirMove();  // Take into account movement when in air.
        }

        // Set final flags.
        CategorizePosition();

        // Make sure velocity is valid.
        CheckVelocity();

        // Add any remaining gravitational component.
        if (!CheckWater())
        {
            FinishGravity();
        }

        // If we are on ground, no downward velocity.
        if (player->GetGroundEntity() != NULL)
        {
            mv->m_vecVelocity[2] = 0;
        }
        CheckFalling();
    }

    if ((m_nOldWaterLevel == WL_NotInWater && player->GetWaterLevel() != WL_NotInWater) ||
        (m_nOldWaterLevel != WL_NotInWater && player->GetWaterLevel() == WL_NotInWater))
    {
        PlaySwimSound();
#if !defined( CLIENT_DLL )
        player->Splash();
#endif
    }
}


void CMomentumGameMovement::AirMove(void)
{
    int			i;
    Vector		wishvel;
    float		fmove, smove;
    Vector		wishdir;
    float		wishspeed;
    Vector forward, right, up;

    AngleVectors(mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

    // Copy movement amounts
    fmove = mv->m_flForwardMove;
    smove = mv->m_flSideMove;

    // Zero out z components of movement vectors
    forward[2] = 0;
    right[2] = 0;
    VectorNormalize(forward);  // Normalize remainder of vectors
    VectorNormalize(right);    // 

    for (i = 0; i<2; i++)       // Determine x and y parts of velocity
        wishvel[i] = forward[i] * fmove + right[i] * smove;
    wishvel[2] = 0;             // Zero out z part of velocity

    VectorCopy(wishvel, wishdir);   // Determine maginitude of speed of move
    wishspeed = VectorNormalize(wishdir);

    //
    // clamp to server defined max speed
    //
    if (wishspeed != 0 && (wishspeed > mv->m_flMaxSpeed))
    {
        VectorScale(wishvel, mv->m_flMaxSpeed / wishspeed, wishvel);
        wishspeed = mv->m_flMaxSpeed;
    }

    AirAccelerate(wishdir, wishspeed, sv_airaccelerate.GetFloat());

    // Add in any base velocity to the current velocity.
    VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

    m_flReflectNormal = NO_REFL_NORMAL_CHANGE;
    TryPlayerMove(NULL, NULL);

    // Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
    VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

    CheckForLadders(false);
    //return bDidReflect;
}

void CMomentumGameMovement::DoLateReflect(void)
{
    // Don't attempt to reflect after this.
    // Return below was causing recursion.
    m_flReflectNormal = 1.0f;


    if (mv->m_vecVelocity.Length() == 0.0f || player->GetGroundEntity() != NULL)
        return;


    Vector prevpos = mv->m_vecAbsOrigin;
    Vector prevvel = mv->m_vecVelocity;


    VectorAdd(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

    // Since we're doing two moves in one frame, only apply changes if we did the reflect and we gained speed.
    TryPlayerMove();
    if (m_flReflectNormal == 1.0f || prevvel.Length2DSqr() > mv->m_vecVelocity.Length2DSqr())
    {
        VectorCopy(prevpos, mv->m_vecAbsOrigin);
        VectorCopy(prevvel, mv->m_vecVelocity);
    }
    else
    {
        VectorSubtract(mv->m_vecVelocity, player->GetBaseVelocity(), mv->m_vecVelocity);

        DevMsg("Successful late reflect! Normal: %.2f\n", m_flReflectNormal);
    }
}

int CMomentumGameMovement::TryPlayerMove(Vector *pFirstDest, trace_t *pFirstTrace)
{
    int			bumpcount, numbumps;
    Vector		dir;
    float		d;
    int			numplanes;
    Vector		planes[MAX_CLIP_PLANES];
    Vector		primal_velocity, original_velocity;
    Vector      new_velocity;
    int			i, j;
    trace_t	pm;
    Vector		end;
    float		time_left, allFraction;
    int			blocked;

    numbumps = 4;           // Bump up to four times

    blocked = 0;           // Assume not blocked
    numplanes = 0;           //  and not sliding along any planes

    VectorCopy(mv->m_vecVelocity, original_velocity);  // Store original velocity
    VectorCopy(mv->m_vecVelocity, primal_velocity);

    allFraction = 0;
    time_left = gpGlobals->frametime;   // Total time for this movement operation.

    new_velocity.Init();

    for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
    {
        if (mv->m_vecVelocity.Length() == 0.0)
            break;

        // Assume we can move all the way from the current origin to the
        //  end point.
        VectorMA(mv->GetAbsOrigin(), time_left, mv->m_vecVelocity, end);

        // See if we can make it from origin to end point.
        if (g_bMovementOptimizations)
        {
            // If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
            if (pFirstDest && end == *pFirstDest)
                pm = *pFirstTrace;
            else
            {
#if defined( PLAYER_GETTING_STUCK_TESTING )
                trace_t foo;
                TracePlayerBBox(mv->GetAbsOrigin(), mv->GetAbsOrigin(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, foo);
                if (foo.startsolid || foo.fraction != 1.0f)
                {
                    Msg("bah\n");
                }
#endif
                TracePlayerBBox(mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);
            }
        }
        else
        {
            TracePlayerBBox(mv->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm);
        }

        allFraction += pm.fraction;

        // If we started in a solid object, or we were in solid space
        //  the whole way, zero out our velocity and return that we
        //  are blocked by floor and wall.
        if (pm.allsolid)
        {
            // entity is trapped in another solid
            VectorCopy(vec3_origin, mv->m_vecVelocity);
            return 4;
        }

        // If we moved some portion of the total distance, then
        //  copy the end position into the pmove.origin and 
        //  zero the plane counter.
        if (pm.fraction > 0)
        {
            if (numbumps > 0 && pm.fraction == 1)
            {
                // There's a precision issue with terrain tracing that can cause a swept box to successfully trace
                // when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
                // case until the bug is fixed.
                // If we detect getting stuck, don't allow the movement
                trace_t stuck;
                TracePlayerBBox(pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, stuck);
                if (stuck.startsolid || stuck.fraction != 1.0f)
                {
                    Msg("Player will become stuck!!!\n");
                    VectorCopy(vec3_origin, mv->m_vecVelocity);
                    break;
                }
            }

#if defined( PLAYER_GETTING_STUCK_TESTING )
            trace_t foo;
            TracePlayerBBox(pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, foo);
            if (foo.startsolid || foo.fraction != 1.0f)
            {
                Msg("Player will become stuck!!!\n");
            }
#endif
            // actually covered some distance
            mv->SetAbsOrigin(pm.endpos);
            VectorCopy(mv->m_vecVelocity, original_velocity);
            numplanes = 0;
        }

        // If we covered the entire distance, we are done
        //  and can return.
        if (pm.fraction == 1)
        {
            break;		// moved the entire distance
        }

        // Save entity that blocked us (since fraction was < 1.0)
        //  for contact
        // Add it if it's not already in the list!!!
        MoveHelper()->AddToTouched(pm, mv->m_vecVelocity);

        // If the plane we hit has a high z component in the normal, then
        //  it's probably a floor
        if (pm.plane.normal[2] > 0.7)
        {
            blocked |= 1;		// floor
        }
        // If the plane has a zero z component in the normal, then it's a 
        //  step or wall
        if (!pm.plane.normal[2])
        {
            blocked |= 2;		// step / wall
        }

        // Reduce amount of m_flFrameTime left by total time left * fraction
        //  that we covered.
        time_left -= time_left * pm.fraction;

        // Did we run out of planes to clip against?
        if (numplanes >= MAX_CLIP_PLANES)
        {
            // this shouldn't really happen
            //  Stop our movement if so.
            VectorCopy(vec3_origin, mv->m_vecVelocity);
            //Con_DPrintf("Too many planes 4\n");

            break;
        }

        // Set up next clipping plane
        VectorCopy(pm.plane.normal, planes[numplanes]);
        numplanes++;

        // modify original_velocity so it parallels all of the clip planes
        //

        // reflect player velocity 
        // Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
        //  and pressing forward and nobody was really using this bounce/reflection feature anyway...
        if (numplanes == 1 &&
            player->GetMoveType() == MOVETYPE_WALK &&
            player->GetGroundEntity() == NULL)
        {
            //Vector cross = mv->m_vecVelocity.Cross(planes[0]);

            //if (cross[1] > 0)//Are we going up a slope?
            //    flReflectNormal = 1.0f;//Don't bother trying to do a LateReflect
            //else
            m_flReflectNormal = planes[0][2];//Determine in CategorizePosition
            

            for (i = 0; i < numplanes; i++)
            {
                if (planes[i][2] > 0.7)
                {
                    // floor or slope
                    ClipVelocity(original_velocity, planes[i], new_velocity, 1);
                    VectorCopy(new_velocity, original_velocity);
                }
                else
                {
                    ClipVelocity(original_velocity, planes[i], new_velocity, 1.0 + sv_bounce.GetFloat() * (1 - player->m_surfaceFriction));
                }
            }

            VectorCopy(new_velocity, mv->m_vecVelocity);
            VectorCopy(new_velocity, original_velocity);
        }
        else
        {
            for (i = 0; i < numplanes; i++)
            {
                ClipVelocity(
                    original_velocity,
                    planes[i],
                    mv->m_vecVelocity,
                    1);

                for (j = 0; j < numplanes; j++)
                    if (j != i)
                    {
                        // Are we now moving against this plane?
                        if (mv->m_vecVelocity.Dot(planes[j]) < 0)
                            break;	// not ok
                    }
                if (j == numplanes)  // Didn't have to clip, so we're ok
                    break;
            }

            // Did we go all the way through plane set
            if (i != numplanes)
            {	// go along this plane
                // pmove.velocity is set in clipping call, no need to set again.
                ;
            }
            else
            {	// go along the crease
                if (numplanes != 2)
                {
                    VectorCopy(vec3_origin, mv->m_vecVelocity);
                    break;
                }
                CrossProduct(planes[0], planes[1], dir);
                dir.NormalizeInPlace();
                d = dir.Dot(mv->m_vecVelocity);
                VectorScale(dir, d, mv->m_vecVelocity);
            }

            //
            // if original velocity is against the original velocity, stop dead
            // to avoid tiny occilations in sloping corners
            //
            d = mv->m_vecVelocity.Dot(primal_velocity);
            if (d <= 0)
            {
                //Con_DPrintf("Back\n");
                if (!sv_ramp_fix.GetBool())
                    VectorCopy(vec3_origin, mv->m_vecVelocity); // RAMPBUG FIX #2
                break;
            }
        }
    }

    if (allFraction == 0)
    {
        if (!sv_ramp_fix.GetBool())
            VectorCopy(vec3_origin, mv->m_vecVelocity); // RAMPBUG FIX #1
    }

    // Check if they slammed into a wall
    float fSlamVol = 0.0f;

    float fLateralStoppingAmount = primal_velocity.Length2D() - mv->m_vecVelocity.Length2D();
    if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f)
    {
        fSlamVol = 1.0f;
    }
    else if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED)
    {
        fSlamVol = 0.85f;
    }

    PlayerRoughLandingEffects(fSlamVol);

    return blocked;
}


// This was the virtual void, overriding it for snow friction
void CMomentumGameMovement::SetGroundEntity(trace_t *pm)
{
    //CMomentumPlayer *player = GetMomentumPlayer();

    CBaseEntity *newGround = pm ? pm->m_pEnt : NULL;

    CBaseEntity *oldGround = player->GetGroundEntity();
    Vector vecBaseVelocity = player->GetBaseVelocity();

    if (!oldGround && newGround)
    {
        // Subtract ground velocity at instant we hit ground jumping
        vecBaseVelocity -= newGround->GetAbsVelocity();
        vecBaseVelocity.z = newGround->GetAbsVelocity().z;
    }
    else if (oldGround && !newGround)
    {
        // Add in ground velocity at instant we started jumping
        vecBaseVelocity += oldGround->GetAbsVelocity();
        vecBaseVelocity.z = oldGround->GetAbsVelocity().z;
    }

    player->SetBaseVelocity(vecBaseVelocity);
    player->SetGroundEntity(newGround);

    // If we are on something...

    if (newGround)
    {
        CategorizeGroundSurface(*pm);//Snow friction override

        // Then we are not in water jump sequence
        player->m_flWaterJumpTime = 0;

        // Standing on an entity other than the world, so signal that we are touching something.
        if (!pm->DidHitWorld())
        {
            MoveHelper()->AddToTouched(*pm, mv->m_vecVelocity);
        }

        mv->m_vecVelocity.z = 0.0f;
    }
}

void CMomentumGameMovement::CategorizeGroundSurface(trace_t &pm)
{
    IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
    //CMomentumPlayer *player = GetMomentumPlayer();

    player->m_surfaceProps = pm.surface.surfaceProps;
    player->m_pSurfaceData = physprops->GetSurfaceData(player->m_surfaceProps);
    physprops->GetPhysicsProperties(player->m_surfaceProps, NULL, NULL, &player->m_surfaceFriction, NULL);

    // HACKHACK: Scale this to fudge the relationship between vphysics friction values and player friction values.
    // A value of 0.8f feels pretty normal for vphysics, whereas 1.0f is normal for players.
    // This scaling trivially makes them equivalent.  REVISIT if this affects low friction surfaces too much.
    player->m_surfaceFriction *= 1.25f;
    if (player->m_surfaceFriction > 1.0f || (player->m_pSurfaceData->game.material == 'D'
        && player->m_pSurfaceData->physics.friction == 0.35f))
        player->m_surfaceFriction = 1.0f;//fix for snow friction

    player->m_chTextureType = player->m_pSurfaceData->game.material;
}

// Expose our interface.
static CMomentumGameMovement g_GameMovement;
IGameMovement *g_pGameMovement = (IGameMovement *) &g_GameMovement;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT, g_GameMovement);

#endif