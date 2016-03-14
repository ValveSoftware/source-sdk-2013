#include "cbase.h"
#include "weapon_csbasegun.h"
#include "fx_cs_shared.h"
#include "mom_player_shared.h"

#if defined( CLIENT_DLL )
#define CWeaponM249 C_WeaponM249
#define CWeaponM3 C_WeaponM3
#define CWeaponXM1014 C_WeaponXM1014
#else
#include "momentum/te_shotgun_shot.h"
#endif

#include "tier0/memdbgon.h"

//M3

class CWeaponM3 : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponM3, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponM3();

    virtual void PrimaryAttack();
    virtual bool Reload();
    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_M3; }


private:

    CWeaponM3(const CWeaponM3 &);

    float m_flPumpTime;
    CNetworkVar(int, m_fInSpecialReload);

};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM3, DT_WeaponM3)

BEGIN_NETWORK_TABLE(CWeaponM3, DT_WeaponM3)

#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_fInSpecialReload))
#else
SendPropInt(SENDINFO(m_fInSpecialReload), 2, SPROP_UNSIGNED)
#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponM3)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_m3, CWeaponM3);
PRECACHE_WEAPON_REGISTER(weapon_m3);



CWeaponM3::CWeaponM3()
{
    m_flPumpTime = 0;
}

void CWeaponM3::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    // don't fire underwater
    if (pPlayer->GetWaterLevel() == 3)
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
        return;
    }

    // Out of ammo?
    if (m_iClip1 <= 0)
    {
        Reload();
        if (m_iClip1 == 0)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

    m_iClip1--;
    pPlayer->DoMuzzleFlash();

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    // Dispatch the FX right away with full accuracy.
    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255, // wrap it for network traffic so it's the same between client and server
        0.0675);

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    if (m_iClip1 != 0)
        m_flPumpTime = gpGlobals->curtime + 0.5;

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.875;
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.875;
    if (m_iClip1 != 0)
        SetWeaponIdleTime(gpGlobals->curtime + 2.5);
    else
        SetWeaponIdleTime(gpGlobals->curtime + 0.875);
    m_fInSpecialReload = 0;

    // Update punch angles.
    QAngle angle = pPlayer->GetPunchAngle();

    if (pPlayer->GetFlags() & FL_ONGROUND)
    {
        angle.x -= SharedRandomInt("M3PunchAngleGround", 4, 6);
    }
    else
    {
        angle.x -= SharedRandomInt("M3PunchAngleAir", 8, 11);
    }

    pPlayer->SetPunchAngle(angle);
}


bool CWeaponM3::Reload()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 || m_iClip1 == GetMaxClip1())
        return true;

    // don't reload until recoil is done
    if (m_flNextPrimaryAttack > gpGlobals->curtime)
        return true;

    // check to see if we're ready to reload
    if (m_fInSpecialReload == 0)
    {
        pPlayer->SetAnimation(PLAYER_RELOAD);

        SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
        m_fInSpecialReload = 1;
        pPlayer->m_flNextAttack = gpGlobals->curtime + 0.5;
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
        m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
        SetWeaponIdleTime(gpGlobals->curtime + 0.5);

#ifdef GAME_DLL
        //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_START );
#endif

        return true;
    }
    else if (m_fInSpecialReload == 1)
    {
        if (m_flTimeWeaponIdle > gpGlobals->curtime)
            return true;
        // was waiting for gun to move to side
        m_fInSpecialReload = 2;

        SendWeaponAnim(ACT_VM_RELOAD);
        SetWeaponIdleTime(gpGlobals->curtime + 0.5);
#ifdef GAME_DLL
        if (m_iClip1 == 7)
        {
            //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );
        }
        else
        {
            //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
        }
#endif
    }
    else
    {
        // Add them to the clip
        m_iClip1 += 1;

#ifdef GAME_DLL
        SendReloadEvents();
#endif

        CMomentumPlayer *pPlayer = GetPlayerOwner();

        if (pPlayer)
            pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);

        m_fInSpecialReload = 1;
    }

    return true;
}


void CWeaponM3::WeaponIdle()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
    {
        // play pumping sound
        m_flPumpTime = 0;
    }

    if (m_flTimeWeaponIdle < gpGlobals->curtime)
    {
        if (m_iClip1 == 0 && m_fInSpecialReload == 0 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
        {
            Reload();
        }
        else if (m_fInSpecialReload != 0)
        {
            if (m_iClip1 != 8 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
            {
                Reload();
            }
            else
            {
                // reload debounce has timed out
                //MIKETODO: shotgun anims
                SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);

                // play cocking sound
                m_fInSpecialReload = 0;
                SetWeaponIdleTime(gpGlobals->curtime + 1.5);
            }
        }
        else
        {
            SendWeaponAnim(ACT_VM_IDLE);
        }
    }
}

//M249

class CWeaponM249 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponM249, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponM249();

    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_M249; }


private:

    CWeaponM249(const CWeaponM249 &);

    void M249Fire(float flSpread);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM249, DT_WeaponM249)

BEGIN_NETWORK_TABLE(CWeaponM249, DT_WeaponM249)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponM249)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_m249, CWeaponM249);
PRECACHE_WEAPON_REGISTER(weapon_m249);



CWeaponM249::CWeaponM249()
{
}


void CWeaponM249::PrimaryAttack(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        M249Fire(0.045f + 0.5f * m_flAccuracy);
    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        M249Fire(0.045f + 0.095f * m_flAccuracy);
    else
        M249Fire(0.03f * m_flAccuracy);
}

void CWeaponM249::M249Fire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.8, 0.65, 0.45, 0.125, 5, 3.5, 8);

    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1.1, 0.5, 0.3, 0.06, 4, 3, 8);

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.75, 0.325, 0.25, 0.025, 3.5, 2.5, 9);

    else
        pPlayer->KickBack(0.8, 0.35, 0.3, 0.03, 3.75, 3, 9);
}

//XM1014
class CWeaponXM1014 : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponXM1014, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponXM1014();

    virtual void Spawn();
    virtual void PrimaryAttack();
    virtual bool Reload();
    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_XM1014; }


private:

    CWeaponXM1014(const CWeaponXM1014 &);

    float m_flPumpTime;
    CNetworkVar(int, m_fInSpecialReload);

};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponXM1014, DT_WeaponXM1014)

BEGIN_NETWORK_TABLE(CWeaponXM1014, DT_WeaponXM1014)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_fInSpecialReload))
#else
SendPropInt(SENDINFO(m_fInSpecialReload), 2, SPROP_UNSIGNED)
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponXM1014)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_xm1014, CWeaponXM1014);
PRECACHE_WEAPON_REGISTER(weapon_xm1014);



CWeaponXM1014::CWeaponXM1014()
{
    m_flPumpTime = 0;
}

void CWeaponXM1014::Spawn()
{
    //m_iDefaultAmmo = M3_DEFAULT_GIVE;
    //FallInit();// get ready to fall
    BaseClass::Spawn();
}


void CWeaponXM1014::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    // don't fire underwater
    if (pPlayer->GetWaterLevel() == 3)
    {
        PlayEmptySound();
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
        return;
    }

    if (m_iClip1 <= 0)
    {
        Reload();

        if (m_iClip1 == 0)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
        }

        return;
    }

    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

    //pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
    //pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

    m_iClip1--;
    pPlayer->DoMuzzleFlash();

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    // Dispatch the FX right away with full accuracy.
    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255, // wrap it for network traffic so it's the same between client and server
        0.0725 // flSpread
        );

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    if (m_iClip1 != 0)
        m_flPumpTime = gpGlobals->curtime + 0.5;

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.25;
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.25;
    if (m_iClip1 != 0)
        SetWeaponIdleTime(gpGlobals->curtime + 2.5);
    else
        SetWeaponIdleTime(gpGlobals->curtime + 0.25);
    m_fInSpecialReload = 0;

    // Update punch angles.
    QAngle angle = pPlayer->GetPunchAngle();

    if (pPlayer->GetFlags() & FL_ONGROUND)
    {
        angle.x -= SharedRandomInt("XM1014PunchAngleGround", 3, 5);
    }
    else
    {
        angle.x -= SharedRandomInt("XM1014PunchAngleAir", 7, 10);
    }

    pPlayer->SetPunchAngle(angle);
}


bool CWeaponXM1014::Reload()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0 || m_iClip1 == GetMaxClip1())
        return true;

    // don't reload until recoil is done
    if (m_flNextPrimaryAttack > gpGlobals->curtime)
        return true;

    //MIKETODO: shotgun reloading (wait until we get content)

    // check to see if we're ready to reload
    if (m_fInSpecialReload == 0)
    {
        pPlayer->SetAnimation(PLAYER_RELOAD);

        SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
        m_fInSpecialReload = 1;
        pPlayer->m_flNextAttack = gpGlobals->curtime + 0.5;
        SetWeaponIdleTime(gpGlobals->curtime + 0.5);
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
        m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;

#ifdef GAME_DLL
        //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_START );
#endif

        return true;
    }
    else if (m_fInSpecialReload == 1)
    {
        if (m_flTimeWeaponIdle > gpGlobals->curtime)
            return true;
        // was waiting for gun to move to side
        m_fInSpecialReload = 2;

        SendWeaponAnim(ACT_VM_RELOAD);
        SetWeaponIdleTime(gpGlobals->curtime + 0.5);
#ifdef GAME_DLL
        if (m_iClip1 == 6)
        {
            //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );
        }
        else
        {
            //pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
        }
#endif
    }
    else
    {
        // Add them to the clip
        m_iClip1 += 1;

#ifdef GAME_DLL
        SendReloadEvents();
#endif

        CMomentumPlayer *pPlayer = GetPlayerOwner();

        if (pPlayer)
            pPlayer->RemoveAmmo(1, m_iPrimaryAmmoType);

        m_fInSpecialReload = 1;
    }


    return true;
}


void CWeaponXM1014::WeaponIdle()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
    {
        // play pumping sound
        m_flPumpTime = 0;
    }

    if (m_flTimeWeaponIdle < gpGlobals->curtime)
    {
        if (m_iClip1 == 0 && m_fInSpecialReload == 0 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
        {
            Reload();
        }
        else if (m_fInSpecialReload != 0)
        {
            if (m_iClip1 != 7 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType))
            {
                Reload();
            }
            else
            {
                // reload debounce has timed out
                //MIKETODO: shotgun anims
                SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);

                // play cocking sound
                m_fInSpecialReload = 0;
                SetWeaponIdleTime(gpGlobals->curtime + 1.5);
            }
        }
        else
        {
            SendWeaponAnim(ACT_VM_IDLE);
        }
    }
}