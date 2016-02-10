//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_basecsgrenade.h"
#include "in_buttons.h"	
#include "datacache/imdlcache.h"
#include "mom_player_shared.h"

#ifndef CLIENT_DLL

#include "items.h"

#endif


#define GRENADE_TIMER	1.5f //Seconds


IMPLEMENT_NETWORKCLASS_ALIASED(BaseCSGrenade, DT_BaseCSGrenade)

BEGIN_NETWORK_TABLE(CBaseCSGrenade, DT_BaseCSGrenade)

#ifndef CLIENT_DLL
SendPropBool(SENDINFO(m_bRedraw)),
SendPropBool(SENDINFO(m_bPinPulled)),
SendPropFloat(SENDINFO(m_fThrowTime), 0, SPROP_NOSCALE),
#else
RecvPropBool( RECVINFO(m_bRedraw) ),
RecvPropBool( RECVINFO(m_bPinPulled) ),
RecvPropFloat( RECVINFO(m_fThrowTime) ),
#endif

END_NETWORK_TABLE()

#if defined CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseCSGrenade )
DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_basecsgrenade, CBaseCSGrenade);


CBaseCSGrenade::CBaseCSGrenade()
{
    m_bRedraw = false;
    m_bPinPulled = false;
    m_fThrowTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCSGrenade::Precache()
{
    BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseCSGrenade::Deploy()
{
    m_bRedraw = false;
    m_bPinPulled = false;
    m_fThrowTime = 0;

#ifndef CLIENT_DLL
    // if we're officially out of grenades, ditch this weapon
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        pPlayer->Weapon_Drop(this, NULL, NULL);
        UTIL_Remove(this);
        return false;
    }
#endif

    return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCSGrenade::Holster(CBaseCombatWeapon *pSwitchingTo)
{
    m_bRedraw = false;
    m_bPinPulled = false; // when this is holstered make sure the pin isn’t pulled.
    m_fThrowTime = 0;

#ifndef CLIENT_DLL
    // If they attempt to switch weapons before the throw animation is done, 
    // allow it, but kill the weapon if we have to.
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        CBaseCombatCharacter *pOwner = (CBaseCombatCharacter *) pPlayer;
        pOwner->Weapon_Drop(this);
        UTIL_Remove(this);
    }
#endif

    return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCSGrenade::PrimaryAttack()
{
    if (m_bRedraw || m_bPinPulled || m_fThrowTime > 0.0f)
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer || pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
        return;

    // The pull pin animation has to finish, then we wait until they aren't holding the primary
    // attack button, then throw the grenade.
    SendWeaponAnim(ACT_VM_PULLPIN);
    m_bPinPulled = true;

    // Don't let weapon idle interfere in the middle of a throw!
    MDLCACHE_CRITICAL_SECTION();
    SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

    m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCSGrenade::SecondaryAttack()
{
    if (m_bRedraw)
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer == NULL)
        return;

    //See if we're ducking
    if (pPlayer->GetFlags() & FL_DUCKING)
    {
        //Send the weapon animation
        SendWeaponAnim(ACT_VM_SECONDARYATTACK);
    }
    else
    {
        //Send the weapon animation
        SendWeaponAnim(ACT_VM_HAULBACK);
    }

    // Don't let weapon idle interfere in the middle of a throw!
    SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

    m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseCSGrenade::Reload()
{
    if ((m_bRedraw) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
    {
        //Redraw the weapon
        SendWeaponAnim(ACT_VM_DRAW);

        //Update our times
        m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
        m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

        SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

        //Mark this as done
        //	m_bRedraw = false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseCSGrenade::ItemPostFrame()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    CBaseViewModel *vm = pPlayer->GetViewModel(m_nViewModelIndex);
    if (!vm)
        return;

    // If they let go of the fire button, they want to throw the grenade.
    if (m_bPinPulled && !(pPlayer->m_nButtons & IN_ATTACK))
    {
        //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_THROW_GRENADE );

        StartGrenadeThrow();

        MDLCACHE_CRITICAL_SECTION();
        m_bPinPulled = false;
        SendWeaponAnim(ACT_VM_THROW);
        SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration());

        m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration(); // we're still throwing, so reset our next primary attack

#ifndef CLIENT_DLL
        IGameEvent * event = gameeventmanager->CreateEvent("weapon_fire");
        if (event)
        {
            const char *weaponName = STRING(m_iClassname);
            if (strncmp(weaponName, "weapon_", 7) == 0)
            {
                weaponName += 7;
            }

            event->SetInt("userid", pPlayer->GetUserID());
            event->SetString("weapon", weaponName);
            gameeventmanager->FireEvent(event);
        }
#endif
    }
    else if ((m_fThrowTime > 0) && (m_fThrowTime < gpGlobals->curtime))
    {
        // only decrement our ammo when we actually create the projectile
        DecrementAmmo(pPlayer);

        ThrowGrenade();
    }
    else if (m_bRedraw)
    {
        // Has the throw animation finished playing
        if (m_flTimeWeaponIdle < gpGlobals->curtime)
        {
#ifdef GAME_DLL
            // if we're officially out of grenades, ditch this weapon
            if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
            {
                pPlayer->Weapon_Drop(this, NULL, NULL);
                UTIL_Remove(this);
            }
            else
            {
                pPlayer->SwitchToNextBestWeapon(this);
            }
#endif
            return;	//don't animate this grenade any more!
        }
    }
    else if (!m_bRedraw)
    {
        BaseClass::ItemPostFrame();
    }
}



#ifdef CLIENT_DLL

void CBaseCSGrenade::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
}

void CBaseCSGrenade::DropGrenade()
{
    m_bRedraw = true;
    m_fThrowTime = 0.0f;
}

void CBaseCSGrenade::ThrowGrenade()
{
    m_bRedraw = true;
    m_fThrowTime = 0.0f;
}

void CBaseCSGrenade::StartGrenadeThrow()
{
    m_fThrowTime = gpGlobals->curtime + 0.1f;
}

#else

BEGIN_DATADESC(CBaseCSGrenade)
DEFINE_FIELD(m_bRedraw, FIELD_BOOLEAN),
END_DATADESC()

int CBaseCSGrenade::CapabilitiesGet()
{
    return bits_CAP_WEAPON_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CBaseCSGrenade::DecrementAmmo(CBaseCombatCharacter *pOwner)
{
    pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

void CBaseCSGrenade::StartGrenadeThrow()
{
    m_fThrowTime = gpGlobals->curtime + 0.1f;
}

void CBaseCSGrenade::ThrowGrenade()
{
    CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
    {
        Assert(false);
        return;
    }

    QAngle angThrow = pPlayer->LocalEyeAngles();

    Vector vForward, vRight, vUp;

    if (angThrow.x < 90)
        angThrow.x = -10 + angThrow.x * ((90 + 10) / 90.0);
    else
    {
        angThrow.x = 360.0f - angThrow.x;
        angThrow.x = -10 + angThrow.x * -((90 - 10) / 90.0);
    }

    float flVel = (90 - angThrow.x) * 6;

    if (flVel > 750)
        flVel = 750;

    AngleVectors(angThrow, &vForward, &vRight, &vUp);

    Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();

    // We want to throw the grenade from 16 units out.  But that can cause problems if we're facing
    // a thin wall.  Do a hull trace to be safe.
    trace_t trace;
    Vector mins(-2, -2, -2);
    Vector maxs(2, 2, 2);
    UTIL_TraceHull(vecSrc, vecSrc + vForward * 16, mins, maxs, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace);
    vecSrc = trace.endpos;

    Vector vecThrow = vForward * flVel + pPlayer->GetAbsVelocity();

    EmitGrenade(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer);

    m_bRedraw = true;
    m_fThrowTime = 0.0f;

    //CMomentumPlayer *pCSPlayer = ToCMOMPlayer( pPlayer );

    //if( pCSPlayer )
    //	pCSPlayer->Radio( "Radio.FireInTheHole",   "#Cstrike_TitlesTXT_Fire_in_the_hole" );
}

void CBaseCSGrenade::DropGrenade()
{
    CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
    {
        Assert(false);
        return;
    }

    Vector vForward;
    pPlayer->EyeVectors(&vForward);
    Vector vecSrc = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset() + vForward * 16;

    Vector vecVel = pPlayer->GetAbsVelocity();

    EmitGrenade(vecSrc, vec3_angle, vecVel, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer);

    m_bRedraw = true;
    m_fThrowTime = 0.0f;
}

void CBaseCSGrenade::EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer)
{
    Assert(0 && "CBaseCSGrenade::EmitGrenade should not be called. Make sure to implement this in your subclass!\n");
}

bool CBaseCSGrenade::AllowsAutoSwitchFrom(void) const
{
    return !m_bPinPulled;
}

#endif

