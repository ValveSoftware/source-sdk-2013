#include "cbase.h"
#include "weapon_csbasegun.h"
#include "mom_player_shared.h"
#include "fx_cs_shared.h"

#ifdef CLIENT_DLL
#define CAK47 C_AK47
#define CWeaponAug C_WeaponAug
#define CWeaponAWP C_WeaponAWP
#define CWeaponFamas C_WeaponFamas
#define CWeaponG3SG1 C_WeaponG3SG1
#define CWeaponGalil C_WeaponGalil
#define CWeaponM4A1 C_WeaponM4A1
#define CWeaponScout C_WeaponScout
#define CWeaponSG550 C_WeaponSG550
#define CWeaponSG552 C_WeaponSG552
#else
#include "KeyValues.h"
#endif

#include "tier0/memdbgon.h"


/*****************
AK47
*****************/
class CAK47 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CAK47, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CAK47();

    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_AK47; }

private:

    void AK47Fire(float flSpread);

    CAK47(const CAK47 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(AK47, DT_WeaponAK47)

BEGIN_NETWORK_TABLE(CAK47, DT_WeaponAK47)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CAK47)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_ak47, CAK47);
PRECACHE_WEAPON_REGISTER(weapon_ak47);

// ---------------------------------------------------------------------------- //
// CAK47 implementation.
// ---------------------------------------------------------------------------- //

CAK47::CAK47()
{
}

void CAK47::AK47Fire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1.5, 0.45, 0.225, 0.05, 6.5, 2.5, 7);
    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(2, 1.0, 0.5, 0.35, 9, 6, 5);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.9, 0.35, 0.15, 0.025, 5.5, 1.5, 9);
    else
        pPlayer->KickBack(1, 0.375, 0.175, 0.0375, 5.75, 1.75, 8);
}


void CAK47::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        AK47Fire(0.04f + 0.4f * m_flAccuracy);
    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        AK47Fire(0.04f + 0.07f * m_flAccuracy);
    else
        AK47Fire(0.0275f * m_flAccuracy);
}




/*
AUG
*/
class CWeaponAug : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponAug, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponAug();

    virtual void SecondaryAttack();
    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_AUG; }

#ifdef CLIENT_DLL
    virtual bool	HideViewModelWhenZoomed(void) { return false; }
#endif

private:

    void AUGFire(float flSpread, bool bZoomed);

    CWeaponAug(const CWeaponAug &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponAug, DT_WeaponAug)

BEGIN_NETWORK_TABLE(CWeaponAug, DT_WeaponAug)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponAug)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_aug, CWeaponAug);
PRECACHE_WEAPON_REGISTER(weapon_aug);



CWeaponAug::CWeaponAug()
{
}

void CWeaponAug::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 55, 0.2f);
    }
    else if (pPlayer->GetFOV() == 55)
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.15f);
    }
    else
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV());
    }
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}


void CWeaponAug::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    bool bZoomed = pPlayer->GetFOV() < pPlayer->GetDefaultFOV();

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        AUGFire(0.035f + 0.4f * m_flAccuracy, bZoomed);

    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        AUGFire(0.035f + 0.07f * m_flAccuracy, bZoomed);
    else
        AUGFire(0.02f * m_flAccuracy, bZoomed);
}


void CWeaponAug::AUGFire(float flSpread, bool bZoomed)
{
    float flCycleTime = GetCSWpnData().m_flCycleTime;

    if (bZoomed)
        flCycleTime = 0.135f;

    if (!CSBaseGunFire(flSpread, flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1, 0.45, 0.275, 0.05, 4, 2.5, 7);

    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.25, 0.45, 0.22, 0.18, 5.5, 4, 5);

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.575, 0.325, 0.2, 0.011, 3.25, 2, 8);

    else
        pPlayer->KickBack(0.625, 0.375, 0.25, 0.0125, 3.5, 2.25, 8);
}




/*
AWP
*/
#define SNIPER_ZOOM_CONTEXT		"SniperRifleThink"
class CWeaponAWP : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponAWP, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
    DECLARE_DATADESC();
#endif

    CWeaponAWP();

    virtual void Spawn();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack();

    virtual void AWPFire(float flSpread);

    virtual float GetMaxSpeed() const;
    virtual bool IsAwp() const;

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_AWP; }

private:

#ifndef CLIENT_DLL
    void				UnzoomThink(void);
#endif

    CWeaponAWP(const CWeaponAWP &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponAWP, DT_WeaponAWP)

BEGIN_NETWORK_TABLE(CWeaponAWP, DT_WeaponAWP)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponAWP)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_awp, CWeaponAWP);
PRECACHE_WEAPON_REGISTER(weapon_awp);

#ifndef CLIENT_DLL

BEGIN_DATADESC(CWeaponAWP)
DEFINE_THINKFUNC(UnzoomThink),
END_DATADESC()

#endif

CWeaponAWP::CWeaponAWP()
{
}

void CWeaponAWP::Spawn()
{
    Precache();

    BaseClass::Spawn();
}


void CWeaponAWP::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 40, 0.15f);
    }
    else if (pPlayer->GetFOV() == 40)
    {
        pPlayer->SetFOV(pPlayer, 10, 0.08f);
    }
    else
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.1f);
    }

    //pPlayer->ResetMaxSpeed();

#endif

#ifndef CLIENT_DLL
    // If this isn't guarded, the sound will be emitted twice, once by the server and once by the client.
    // Let the server play it since if only the client plays it, it's liable to get played twice cause of
    // a prediction error. joy.
    EmitSound("Default.Zoom");

    // let the bots hear the rifle zoom
    IGameEvent * event = gameeventmanager->CreateEvent("weapon_zoom");
    if (event)
    {
        event->SetInt("userid", pPlayer->GetUserID());
        gameeventmanager->FireEvent(event);
    }
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
    m_zoomFullyActiveTime = gpGlobals->curtime + 0.15; // The worst zoom time from above.  

}


void CWeaponAWP::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        AWPFire(0.85);

    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        AWPFire(0.25);

    else if (pPlayer->GetAbsVelocity().Length2D() > 10)
        AWPFire(0.10);

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        AWPFire(0.0);

    else
        AWPFire(0.001);
}

void CWeaponAWP::AWPFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    // If we are not zoomed in, or we have very recently zoomed and are still transitioning, the bullet diverts more.
    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV() || (gpGlobals->curtime < m_zoomFullyActiveTime))
    {
        flSpread += 0.08;
    }

    if (pPlayer->GetFOV() != pPlayer->GetDefaultFOV())
    {
        pPlayer->m_iLastZoom = pPlayer->GetFOV();

#ifndef CLIENT_DLL
#ifdef AWP_UNZOOM
        SetContextThink(&CWeaponAWP::UnzoomThink, gpGlobals->curtime + sv_awpunzoomdelay.GetFloat(), SNIPER_ZOOM_CONTEXT);
#else
        pPlayer->m_bResumeZoom = true;
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.1f);
#endif
#endif
    }

    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= 2;
    pPlayer->SetPunchAngle(angle);
}

#ifndef CLIENT_DLL
void CWeaponAWP::UnzoomThink(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.1f);
}
#endif


float CWeaponAWP::GetMaxSpeed() const
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return BaseClass::GetMaxSpeed();
    }

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        return BaseClass::GetMaxSpeed();
    }
    else
    {
        // Slower speed when zoomed in.
        return 150;
    }
}


bool CWeaponAWP::IsAwp() const
{
    return true;
}



/*
FAMAS
*/
class CWeaponFamas : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponFamas, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponFamas();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack();

    virtual void ItemPostFrame();

    void FamasFire(float flSpread, bool bFireBurst);
    void FireRemaining(int &shotsFired, float &shootTime);

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_FAMAS; }

private:

    CWeaponFamas(const CWeaponFamas &);
    CNetworkVar(bool, m_bBurstMode);
    float	m_flFamasShoot;			// time to shoot the remaining bullets of the famas burst fire
    int		m_iFamasShotsFired;		// used to keep track of the shots fired during the Famas burst fire mode....
    float	m_fBurstSpread;			// used to keep track of current spread factor so that all bullets in spread use same spread
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFamas, DT_WeaponFamas)

BEGIN_NETWORK_TABLE(CWeaponFamas, DT_WeaponFamas)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bBurstMode))
#else
SendPropBool(SENDINFO(m_bBurstMode))
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponFamas)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_famas, CWeaponFamas);
PRECACHE_WEAPON_REGISTER(weapon_famas);



CWeaponFamas::CWeaponFamas()
{
    m_bBurstMode = false;
}

// Secondary attack could be three-round burst mode
void CWeaponFamas::SecondaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_bBurstMode)
    {
        ClientPrint(pPlayer, HUD_PRINTCENTER, "#Switch_To_FullAuto");
        m_bBurstMode = false;
    }
    else
    {
        ClientPrint(pPlayer, HUD_PRINTCENTER, "#Switch_To_BurstFire");
        m_bBurstMode = true;
    }
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponFamas::PrimaryAttack()
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

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))	// if player is in air
        FamasFire(0.03f + 0.3f * m_flAccuracy, m_bBurstMode);

    else if (pPlayer->GetAbsVelocity().Length2D() > 140)	// if player is moving
        FamasFire(0.03f + 0.07f * m_flAccuracy, m_bBurstMode);
    /* new code */
    else
        FamasFire(0.02f * m_flAccuracy, m_bBurstMode);
}


// GOOSEMAN : FireRemaining used by Glock18
void CWeaponFamas::FireRemaining(int &shotsFired, float &shootTime)
{
    float nexttime = 0.1;

    m_iClip1--;

    if (m_iClip1 < 0)
    {
        m_iClip1 = 0;
        shotsFired = 3;
        shootTime = 0.0f;
        return;
    }

    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        Error("!pPlayer");

    // Famas burst mode
    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Secondary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        m_fBurstSpread);


    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

    pPlayer->DoMuzzleFlash();
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    shotsFired++;

    if (shotsFired != 3)
        shootTime = gpGlobals->curtime + nexttime;
    else
        shootTime = 0.0;
}


void CWeaponFamas::ItemPostFrame()
{
    if (m_flFamasShoot != 0.0 && m_flFamasShoot < gpGlobals->curtime)
        FireRemaining(m_iFamasShotsFired, m_flFamasShoot);

    BaseClass::ItemPostFrame();
}


void CWeaponFamas::FamasFire(float flSpread, bool bFireBurst)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    float flCycleTime = GetCSWpnData().m_flCycleTime;

    // change a few things if we're in burst mode
    if (bFireBurst)
    {
        m_iFamasShotsFired = 0;
        flCycleTime = 0.55f;
    }
    else
    {
        flSpread += 0.01;
    }

    if (!CSBaseGunFire(flSpread, flCycleTime, true))
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1, 0.45, 0.275, 0.05, 4, 2.5, 7);

    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.25, 0.45, 0.22, 0.18, 5.5, 4, 5);

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.575, 0.325, 0.2, 0.011, 3.25, 2, 8);

    else
        pPlayer->KickBack(0.625, 0.375, 0.25, 0.0125, 3.5, 2.25, 8);

    if (bFireBurst)
    {
        // Fire off the next two rounds
        m_flFamasShoot = gpGlobals->curtime + 0.05;	// 0.1
        m_fBurstSpread = flSpread;
        m_iFamasShotsFired++;
    }
}


/*
G3SG1
*/
class CWeaponG3SG1 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponG3SG1, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponG3SG1();

    virtual void Spawn();
    virtual void SecondaryAttack();
    virtual void PrimaryAttack();
    virtual bool Reload();
    virtual bool Deploy();

    virtual float GetMaxSpeed();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_G3SG1; }

private:

    CWeaponG3SG1(const CWeaponG3SG1 &);

    void G3SG1Fire(float flSpread);


    float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponG3SG1, DT_WeaponG3SG1)

BEGIN_NETWORK_TABLE(CWeaponG3SG1, DT_WeaponG3SG1)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponG3SG1)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_g3sg1, CWeaponG3SG1);
PRECACHE_WEAPON_REGISTER(weapon_g3sg1);



CWeaponG3SG1::CWeaponG3SG1()
{
    m_flLastFire = gpGlobals->curtime;
}

void CWeaponG3SG1::Spawn()
{
    BaseClass::Spawn();
    m_flAccuracy = 0.98;
}


void CWeaponG3SG1::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 40, 0.3f);
    }
    else if (pPlayer->GetFOV() == 40)
    {
        pPlayer->SetFOV(pPlayer, 15, 0.05);
    }
    else if (pPlayer->GetFOV() == 15)
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.1f);
    }

    //pPlayer->ResetMaxSpeed();
#endif

#ifndef CLIENT_DLL
    // If this isn't guarded, the sound will be emitted twice, once by the server and once by the client.
    // Let the server play it since if only the client plays it, it's liable to get played twice cause of
    // a prediction error. joy.
    EmitSound("Default.Zoom"); // zoom sound

    // let the bots hear the rifle zoom
    IGameEvent * event = gameeventmanager->CreateEvent("weapon_zoom");
    if (event)
    {
        event->SetInt("userid", pPlayer->GetUserID());
        gameeventmanager->FireEvent(event);
    }
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
    m_zoomFullyActiveTime = gpGlobals->curtime + 0.3; // The worst zoom time from above.  
}

void CWeaponG3SG1::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        G3SG1Fire(0.45 * (1 - m_flAccuracy));
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        G3SG1Fire(0.15);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        G3SG1Fire(0.035 * (1 - m_flAccuracy));
    else
        G3SG1Fire(0.055 * (1 - m_flAccuracy));
}

void CWeaponG3SG1::G3SG1Fire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    // If we are not zoomed in, or we have very recently zoomed and are still transitioning, the bullet diverts more.
    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV() || (gpGlobals->curtime < m_zoomFullyActiveTime))
        flSpread += 0.025;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy = 0.55 + (0.3) * (gpGlobals->curtime - m_flLastFire);

    if (m_flAccuracy > 0.98)
        m_flAccuracy = 0.98;

    m_flLastFire = gpGlobals->curtime;

    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    // Adjust the punch angle.
    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= SharedRandomFloat("G3SG1PunchAngleX", 0.75, 1.75) + (angle.x / 4);
    angle.y += SharedRandomFloat("G3SG1PunchAngleY", -0.75, 0.75);
    pPlayer->SetPunchAngle(angle);
}


bool CWeaponG3SG1::Reload()
{
    bool ret = BaseClass::Reload();

    m_flAccuracy = 0.98;

    return ret;
}

bool CWeaponG3SG1::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flAccuracy = 0.98;

    return ret;
}

float CWeaponG3SG1::GetMaxSpeed()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (pPlayer && pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
        return BaseClass::GetMaxSpeed();
    else
        return 150; // zoomed in
}


/*
GALIL
*/
class CWeaponGalil : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponGalil, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponGalil();

    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_GALIL; }

private:

    CWeaponGalil(const CWeaponGalil &);

    void GalilFire(float flSpread);

};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGalil, DT_WeaponGalil)

BEGIN_NETWORK_TABLE(CWeaponGalil, DT_WeaponGalil)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponGalil)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_galil, CWeaponGalil);
PRECACHE_WEAPON_REGISTER(weapon_galil);



CWeaponGalil::CWeaponGalil()
{
}


void CWeaponGalil::PrimaryAttack()
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

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        GalilFire(0.04f + 0.3f * m_flAccuracy);
    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        GalilFire(0.04f + 0.07f * m_flAccuracy);
    else
        GalilFire(0.0375f * m_flAccuracy);
}

void CWeaponGalil::GalilFire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1.0, 0.45, 0.28, 0.045, 3.75, 3, 7);
    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.2, 0.5, 0.23, 0.15, 5.5, 3.5, 6);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.6, 0.3, 0.2, 0.0125, 3.25, 2, 7);
    else
        pPlayer->KickBack(0.65, 0.35, 0.25, 0.015, 3.5, 2.25, 7);
}



/*
M4A1
*/
class CWeaponM4A1 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponM4A1, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponM4A1();

    virtual void Spawn();
    virtual void Precache();

    virtual void SecondaryAttack();
    virtual void PrimaryAttack();
    virtual bool Deploy();
    virtual bool Reload();
    virtual void WeaponIdle();
    virtual bool Holster(CBaseCombatWeapon *pSwitchingTo);

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_M4A1; }

    // return true if this weapon has a silencer equipped
    virtual bool IsSilenced(void) const { return m_bSilencerOn; }

    virtual Activity GetDeployActivity(void);

#ifdef CLIENT_DLL
    virtual int GetMuzzleFlashStyle(void);
#endif

    virtual const char		*GetWorldModel(void) const;
    virtual int				GetWorldModelIndex(void);

private:

    CWeaponM4A1(const CWeaponM4A1 &);

    void M4A1Fire(float flSpread);
    void DoFireEffects();

    CNetworkVar(bool, m_bSilencerOn);
    CNetworkVar(float, m_flDoneSwitchingSilencer);	// soonest time switching the silencer will be complete

    int m_silencedModelIndex;
    bool m_inPrecache;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponM4A1, DT_WeaponM4A1)

BEGIN_NETWORK_TABLE(CWeaponM4A1, DT_WeaponM4A1)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bSilencerOn)),
RecvPropTime(RECVINFO(m_flDoneSwitchingSilencer)),
#else
SendPropBool(SENDINFO(m_bSilencerOn)),
SendPropTime(SENDINFO(m_flDoneSwitchingSilencer)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponM4A1)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_m4a1, CWeaponM4A1);
PRECACHE_WEAPON_REGISTER(weapon_m4a1);



CWeaponM4A1::CWeaponM4A1()
{
    m_bSilencerOn = false;
    m_flDoneSwitchingSilencer = 0.0f;
    m_inPrecache = false;
}


void CWeaponM4A1::Spawn()
{
    BaseClass::Spawn();

    m_bSilencerOn = false;
    m_flDoneSwitchingSilencer = 0.0f;
    m_bDelayFire = true;
}


void CWeaponM4A1::Precache()
{
    m_inPrecache = true;
    BaseClass::Precache();

    m_silencedModelIndex = CBaseEntity::PrecacheModel(GetCSWpnData().m_szSilencerModel);
    m_inPrecache = false;
}


int CWeaponM4A1::GetWorldModelIndex(void)
{
    if (!m_bSilencerOn || m_inPrecache)
    {
        return m_iWorldModelIndex;
    }
    else
    {
        return m_silencedModelIndex;
    }
}


const char * CWeaponM4A1::GetWorldModel(void) const
{
    if (!m_bSilencerOn || m_inPrecache)
    {
        return BaseClass::GetWorldModel();
    }
    else
    {
        return GetCSWpnData().m_szSilencerModel;
    }
}


bool CWeaponM4A1::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flDoneSwitchingSilencer = 0.0f;
    m_bDelayFire = true;

    return ret;
}

Activity CWeaponM4A1::GetDeployActivity(void)
{
    if (IsSilenced())
    {
        return ACT_VM_DRAW_SILENCED;
    }
    else
    {
        return ACT_VM_DRAW;
    }
}

bool CWeaponM4A1::Holster(CBaseCombatWeapon *pSwitchingTo)
{
    if (m_flDoneSwitchingSilencer > 0.0f && m_flDoneSwitchingSilencer > gpGlobals->curtime)
    {
        // still switching the silencer.  Cancel the switch.
        m_bSilencerOn = !m_bSilencerOn;
        SetWeaponModelIndex(GetWorldModel());
    }

    return BaseClass::Holster(pSwitchingTo);
}

void CWeaponM4A1::SecondaryAttack()
{
    if (m_bSilencerOn)
    {
        m_bSilencerOn = false;
        SendWeaponAnim(ACT_VM_DETACH_SILENCER);
    }
    else
    {
        m_bSilencerOn = true;
        SendWeaponAnim(ACT_VM_ATTACH_SILENCER);
    }
    m_flDoneSwitchingSilencer = gpGlobals->curtime + 2;

    m_flNextSecondaryAttack = gpGlobals->curtime + 2;
    m_flNextPrimaryAttack = gpGlobals->curtime + 2;
    SetWeaponIdleTime(gpGlobals->curtime + 2);

    SetWeaponModelIndex(GetWorldModel());
}

void CWeaponM4A1::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
    {
        M4A1Fire(0.035f + 0.4f * m_flAccuracy);
    }
    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
    {
        M4A1Fire(0.035f + 0.07f * m_flAccuracy);
    }
    else
    {
        if (m_bSilencerOn)
            M4A1Fire(0.025f * m_flAccuracy);
        else
            M4A1Fire(0.02f * m_flAccuracy);
    }
}


void CWeaponM4A1::M4A1Fire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, !m_bSilencerOn))
        return;

    if (m_bSilencerOn)
        SendWeaponAnim(ACT_VM_PRIMARYATTACK_SILENCED);

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1.0, 0.45, 0.28, 0.045, 3.75, 3, 7);
    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.2, 0.5, 0.23, 0.15, 5.5, 3.5, 6);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.6, 0.3, 0.2, 0.0125, 3.25, 2, 7);
    else
        pPlayer->KickBack(0.65, 0.35, 0.25, 0.015, 3.5, 2.25, 7);
}


void CWeaponM4A1::DoFireEffects()
{
    if (!m_bSilencerOn)
    {
        CMomentumPlayer *pPlayer = GetPlayerOwner();
        if (pPlayer)
        {
            pPlayer->DoMuzzleFlash();
        }
    }
}

bool CWeaponM4A1::Reload()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return false;

    if (pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0)
        return false;

    int iResult = 0;

    if (m_bSilencerOn)
        iResult = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_SILENCED);
    else
        iResult = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);

    if (!iResult)
        return false;

    pPlayer->SetAnimation(PLAYER_RELOAD);

#ifndef CLIENT_DLL
    if ((iResult) && (pPlayer->GetFOV() != pPlayer->GetDefaultFOV()))
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV());
    }
#endif

    m_flAccuracy = 0.2;
    pPlayer->m_iShotsFired = 0;
    m_bDelayFire = false;
    return true;
}


void CWeaponM4A1::WeaponIdle()
{
    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        SetWeaponIdleTime(gpGlobals->curtime + GetCSWpnData().m_flIdleInterval);
        if (m_bSilencerOn)
            SendWeaponAnim(ACT_VM_IDLE_SILENCED);
        else
            SendWeaponAnim(ACT_VM_IDLE);
    }
}


#ifdef CLIENT_DLL
int CWeaponM4A1::GetMuzzleFlashStyle(void)
{
    if (m_bSilencerOn)
    {
        return CS_MUZZLEFLASH_NONE;
    }
    else
    {
        return CS_MUZZLEFLASH_X;
    }
}
#endif


//SCOUT
class CWeaponScout : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponScout, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponScout();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack();

    virtual float GetMaxSpeed() const;

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_SCOUT; }


private:

    CWeaponScout(const CWeaponScout &);

    void SCOUTFire(float flSpread);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponScout, DT_WeaponScout)

BEGIN_NETWORK_TABLE(CWeaponScout, DT_WeaponScout)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponScout)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_scout, CWeaponScout);
PRECACHE_WEAPON_REGISTER(weapon_scout);



CWeaponScout::CWeaponScout()
{
}

void CWeaponScout::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 40, 0.15f);
    }
    else if (pPlayer->GetFOV() == 40)
    {
        pPlayer->SetFOV(pPlayer, 15, 0.05);
    }
    else if (pPlayer->GetFOV() == 15)
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.05f);
    }

    //pPlayer->ResetMaxSpeed();
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
    m_zoomFullyActiveTime = gpGlobals->curtime + 0.15; // The worst zoom time from above.  

#ifndef CLIENT_DLL
    // If this isn't guarded, the sound will be emitted twice, once by the server and once by the client.
    // Let the server play it since if only the client plays it, it's liable to get played twice cause of
    // a prediction error. joy.
    EmitSound("Default.Zoom"); // zoom sound

    // let the bots hear the rifle zoom
    IGameEvent * event = gameeventmanager->CreateEvent("weapon_zoom");
    if (event)
    {
        event->SetInt("userid", pPlayer->GetUserID());
        gameeventmanager->FireEvent(event);
    }
#endif
}

void CWeaponScout::PrimaryAttack(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        SCOUTFire(0.2);
    else if (pPlayer->GetAbsVelocity().Length2D() > 170)
        SCOUTFire(0.075);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        SCOUTFire(0.0);
    else
        SCOUTFire(0.007);
}

void CWeaponScout::SCOUTFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return;
    }

    // If we are not zoomed in, or we have very recently zoomed and are still transitioning, the bullet diverts more.
    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV() || (gpGlobals->curtime < m_zoomFullyActiveTime))
    {
        flSpread += 0.025;
    }

    if (pPlayer->GetFOV() != pPlayer->GetDefaultFOV())
    {
        pPlayer->m_bResumeZoom = true;
        pPlayer->m_iLastZoom = pPlayer->GetFOV();

#ifndef CLIENT_DLL
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.05f);
#endif
    }

    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= 2;
    pPlayer->SetPunchAngle(angle);
}

//MOM_TODO: Consider LJ gametype
float CWeaponScout::GetMaxSpeed() const
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (pPlayer == NULL)
    {
        Assert(pPlayer != NULL);
        return BaseClass::GetMaxSpeed();
    }

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
        return BaseClass::GetMaxSpeed();
    else
        return 220;	// zoomed in.
}



/*
SG550
*/
class CWeaponSG550 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponSG550, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponSG550();

    virtual void Spawn();
    virtual void SecondaryAttack();
    virtual void PrimaryAttack();
    virtual bool Reload();
    virtual bool Deploy();

    virtual float GetMaxSpeed() const;

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_SG550; }


private:

    CWeaponSG550(const CWeaponSG550 &);

    void SG550Fire(float flSpread);

    float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSG550, DT_WeaponSG550)

BEGIN_NETWORK_TABLE(CWeaponSG550, DT_WeaponSG550)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponSG550)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_sg550, CWeaponSG550);
PRECACHE_WEAPON_REGISTER(weapon_sg550);



CWeaponSG550::CWeaponSG550()
{
    m_flLastFire = gpGlobals->curtime;
}

void CWeaponSG550::Spawn()
{
    BaseClass::Spawn();
    m_flAccuracy = 0.98;
}


void CWeaponSG550::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 40, 0.3f);
    }
    else if (pPlayer->GetFOV() == 40)
    {
        pPlayer->SetFOV(pPlayer, 15, 0.05f);
    }
    else if (pPlayer->GetFOV() == 15)
    {
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV(), 0.1f);
    }

    //pPlayer->ResetMaxSpeed();
#endif

#ifndef CLIENT_DLL
    // If this isn't guarded, the sound will be emitted twice, once by the server and once by the client.
    // Let the server play it since if only the client plays it, it's liable to get played twice cause of
    // a prediction error. joy.
    EmitSound("Default.Zoom"); // zoom sound.

    // let the bots hear the rifle zoom
    IGameEvent * event = gameeventmanager->CreateEvent("weapon_zoom");
    if (event)
    {
        event->SetInt("userid", pPlayer->GetUserID());
        gameeventmanager->FireEvent(event);
    }
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
    m_zoomFullyActiveTime = gpGlobals->curtime + 0.3; // The worst zoom time from above.  
}

void CWeaponSG550::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        SG550Fire(0.45f * (1 - m_flAccuracy));
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        SG550Fire(0.15f);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        SG550Fire(0.04f * (1 - m_flAccuracy));
    else
        SG550Fire(0.05f * (1 - m_flAccuracy));
}

void CWeaponSG550::SG550Fire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    // If we are not zoomed in, or we have very recently zoomed and are still transitioning, the bullet diverts more.
    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV() || (gpGlobals->curtime < m_zoomFullyActiveTime))
        flSpread += 0.025;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy = 0.65 + (0.35) * (gpGlobals->curtime - m_flLastFire);

    if (m_flAccuracy > 0.98)
        m_flAccuracy = 0.98;

    m_flLastFire = gpGlobals->curtime;

    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= SharedRandomFloat("SG550PunchAngleX", 0.75, 1.25) + (angle.x / 4);
    angle.y += SharedRandomFloat("SG550PunchAngleY", -0.75, 0.75);
    pPlayer->SetPunchAngle(angle);
}

bool CWeaponSG550::Reload()
{
    bool ret = BaseClass::Reload();

    m_flAccuracy = 0.98;

    return ret;
}

bool CWeaponSG550::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flAccuracy = 0.98;

    return ret;
}

float CWeaponSG550::GetMaxSpeed() const
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (!pPlayer || pPlayer->GetFOV() == 90)
        return BaseClass::GetMaxSpeed();
    else
        return 150; // zoomed in
}


/*
SG552
*/
class CWeaponSG552 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponSG552, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponSG552();

    virtual void SecondaryAttack();
    virtual void PrimaryAttack();

    virtual float GetMaxSpeed() const;

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_SG552; }

#ifdef CLIENT_DLL
    virtual bool	HideViewModelWhenZoomed(void) { return false; }
#endif

private:

    CWeaponSG552(const CWeaponSG552 &);

    void SG552Fire(float flSpread, bool bZoomed);

};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSG552, DT_WeaponSG552)

BEGIN_NETWORK_TABLE(CWeaponSG552, DT_WeaponSG552)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponSG552)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_sg552, CWeaponSG552);
PRECACHE_WEAPON_REGISTER(weapon_sg552);



CWeaponSG552::CWeaponSG552()
{
}



void CWeaponSG552::SecondaryAttack()
{
#ifndef CLIENT_DLL
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
    {
        pPlayer->SetFOV(pPlayer, 55, 0.2f);
    }
    else if (pPlayer->GetFOV() == 55)
    {
        pPlayer->SetFOV(pPlayer, 0, 0.15f);
    }
    else
    {
        //FIXME: This seems wrong
        pPlayer->SetFOV(pPlayer, pPlayer->GetDefaultFOV());
    }
#endif

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponSG552::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    bool bZoomed = pPlayer->GetFOV() < pPlayer->GetDefaultFOV();

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        SG552Fire(0.035f + 0.45f * m_flAccuracy, bZoomed);
    else if (pPlayer->GetAbsVelocity().Length2D() > 140)
        SG552Fire(0.035f + 0.075f * m_flAccuracy, bZoomed);
    else
        SG552Fire(0.02f * m_flAccuracy, bZoomed);
}


void CWeaponSG552::SG552Fire(float flSpread, bool bZoomed)
{
    float flCycleTime = GetCSWpnData().m_flCycleTime;

    if (bZoomed)
        flCycleTime = 0.135f;

    if (!CSBaseGunFire(flSpread, flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(1, 0.45, 0.28, 0.04, 4.25, 2.5, 7);
    else if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.25, 0.45, 0.22, 0.18, 6, 4, 5);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.6, 0.35, 0.2, 0.0125, 3.7, 2, 10);
    else
        pPlayer->KickBack(0.625, 0.375, 0.25, 0.0125, 4, 2.25, 9);
}


float CWeaponSG552::GetMaxSpeed() const
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();

    if (!pPlayer || pPlayer->GetFOV() == pPlayer->GetDefaultFOV())
        return BaseClass::GetMaxSpeed();
    else
        return 200; // zoomed in.
}