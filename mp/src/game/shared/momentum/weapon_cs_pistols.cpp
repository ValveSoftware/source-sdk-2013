#include "cbase.h" 
#include "decals.h" 
#include "shake.h" 
#include "weapon_csbase.h"
#include "fx_cs_shared.h"
#include "mom_player_shared.h"


#ifdef CLIENT_DLL
#include "c_te_effect_dispatch.h"

#define CDEagle C_DEagle
#define CDEagleOverride C_DEagleOverride
#define CWeaponElite C_WeaponElite
#define CWeaponFiveSeven C_WeaponFiveSeven
#define CWeaponGlock C_WeaponGlock
#define CWeaponP228 C_WeaponP228
#define CWeaponUSP C_WeaponUSP
#else
#include "hierarchy.h"
#endif


#include "tier0/memdbgon.h"


//DEAGLE

#define DEAGLE_WEIGHT   7
#define DEAGLE_MAX_CLIP 7

enum deagle_e
{
    DEAGLE_IDLE1 = 0,
    DEAGLE_SHOOT1,
    DEAGLE_SHOOT2,
    DEAGLE_SHOOT_EMPTY,
    DEAGLE_RELOAD,
    DEAGLE_DRAW,
};

class CDEagle : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CDEagle, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CDEagle();

    void Spawn();

    void PrimaryAttack();
    void DEAGLEFire(float flSpread);
    virtual bool Deploy();
    bool Reload();
    void WeaponIdle();
    void MakeBeam();
    void BeamUpdate();
    virtual bool UseDecrement() { return true; };

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_DEAGLE; }

public:

    float m_flLastFire;


private:
    CDEagle(const CDEagle &);
};

//This is an example of an override class
//We're going to want to point the entity 
class CDEagleOverride : public CBaseCombatWeapon
{
public:
    DECLARE_CLASS(CDEagleOverride, CBaseCombatWeapon);
    DECLARE_NETWORKCLASS();

    CDEagleOverride();

    void DefaultTouch(CBaseEntity* pActivator)
    {
#ifdef GAME_DLL
        if (pActivator->IsPlayer())
        {
            CMomentumPlayer *pPlayer = static_cast<CMomentumPlayer*>(pActivator);
            CUtlVector<CBaseEntity *> children;
            if (GetAllChildren(this, children) > 0)
            {
                FOR_EACH_VEC(children, i)
                {
                    if (children[i])
                    {
                        Vector offset = children[i]->GetLocalOrigin();
                        //QAngle offsetAng = children[i]->GetLocalAngles();
                        //offset.z -= pActivator->GetViewOffset().z;

                        children[i]->SetParent(pPlayer->GetActiveWeapon());
                        children[i]->SetLocalOrigin(offset);
                        //children[i]->SetLocalAngles(offsetAng);
                    }
                }
                //CMomentumPlayer *pPlay = dynamic_cast<CMomentumPlayer*>(pActivator);
                //if (pPlay)
                //    SetParent(pPlay);
            }
        }
#endif
    };

    void OnPickedUp(CBaseCombatCharacter* pNewOwner)
    {
        //Use(pNewOwner, NULL, , NULL);
    }

    void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value) {};

private:
    CDEagleOverride(const CDEagleOverride&);
};

CDEagleOverride::CDEagleOverride()
{
    SetSolid(SOLID_NONE);
}

IMPLEMENT_NETWORKCLASS_ALIASED(DEagle, DT_WeaponDEagle)

IMPLEMENT_NETWORKCLASS_ALIASED(DEagleOverride, DT_DEagleOverride)

BEGIN_NETWORK_TABLE(CDEagle, DT_WeaponDEagle)
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE(CDEagleOverride, DT_DEagleOverride)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CDEagle)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_deagle, CDEagleOverride);
PRECACHE_WEAPON_REGISTER(weapon_deagle);



CDEagle::CDEagle()
{
    m_flLastFire = gpGlobals->curtime;
}


void CDEagle::Spawn()
{
    BaseClass::Spawn();
    m_flAccuracy = 0.9;
}


bool CDEagle::Deploy()
{
    m_flAccuracy = 0.9;
    return BaseClass::Deploy();
}


void CDEagle::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        DEAGLEFire(1.5f * (1 - m_flAccuracy));

    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        DEAGLEFire(0.25f * (1 - m_flAccuracy));

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        DEAGLEFire(0.115f * (1 - m_flAccuracy));

    else
        DEAGLEFire(0.13f * (1 - m_flAccuracy));
}


void CDEagle::DEAGLEFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    pPlayer->m_iShotsFired++;

    if (pPlayer->m_iShotsFired > 1)
        return;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.35)*(0.4 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.9)
        m_flAccuracy = 0.9;
    else if (m_flAccuracy < 0.55)
        m_flAccuracy = 0.55;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_iClip1--;

    pPlayer->DoMuzzleFlash();

    if (m_iClip1 > 0)
        SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    else
        SendWeaponAnim(ACT_VM_DRYFIRE);

    //SetPlayerShieldAnim();

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    //pPlayer->m_iWeaponVolume = BIG_EXPLOSION_VOLUME;
    //pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        flSpread);	// bullets

    m_flNextPrimaryAttack = gpGlobals->curtime + GetCSWpnData().m_flCycleTime;

    if (!m_iClip1 && pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 1.8);

    QAngle punchAngle = pPlayer->GetPunchAngle();
    punchAngle.x -= 2;
    pPlayer->SetPunchAngle(punchAngle);

    //ResetPlayerShieldAnim();
}


bool CDEagle::Reload()
{
    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.9;
    return true;
}

void CDEagle::WeaponIdle()
{
    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    SetWeaponIdleTime(gpGlobals->curtime + 20);

    if (m_iClip1 != 0)
    {
        SendWeaponAnim(ACT_VM_IDLE);
    }

    //if ( FBitSet(m_iWeaponState, WPNSTATE_SHIELD_DRAWN) )
    //	 SendWeaponAnim( SHIELDGUN_DRAWN_IDLE, UseDecrement() ? 1:0 );
}


/*
ELITES
*/
class CWeaponElite : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponElite, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponElite();

    virtual void Spawn();
    virtual void Precache();

    virtual void PrimaryAttack();
    virtual bool Deploy();

    void ELITEFire(float flSpread);

    virtual bool Reload();

    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_ELITE; }

#ifdef CLIENT_DLL
    virtual int		GetMuzzleAttachment(void);
    virtual bool OnFireEvent(C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options);
#endif

    virtual const char		*GetWorldModel(void) const;
    virtual int				GetWorldModelIndex(void);

private:

    CWeaponElite(const CWeaponElite &);
    float		m_flLastFire;
    CNetworkVar(bool, m_bFireRight);

    int m_droppedModelIndex;
    bool m_inPrecache;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponElite, DT_WeaponElite)

BEGIN_NETWORK_TABLE(CWeaponElite, DT_WeaponElite)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bFireRight)),
#else
SendPropBool(SENDINFO(m_bFireRight)),
#endif
END_NETWORK_TABLE()

#if defined CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponElite)
DEFINE_PRED_FIELD(m_bFireRight, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_elite, CWeaponElite);
PRECACHE_WEAPON_REGISTER(weapon_elite);

CWeaponElite::CWeaponElite()
{
    m_flLastFire = gpGlobals->curtime;
    m_inPrecache = false;
}


void CWeaponElite::Spawn()
{
    m_flAccuracy = 0.88;
    m_bFireRight = false;

    BaseClass::Spawn();
}


void CWeaponElite::Precache()
{
    m_inPrecache = true;
    BaseClass::Precache();

    PrecacheModel("models/weapons/w_eq_eholster_elite.mdl");
    PrecacheModel("models/weapons/w_eq_eholster.mdl");
    PrecacheModel("models/weapons/w_pist_elite_single.mdl");
    m_droppedModelIndex = CBaseEntity::PrecacheModel(GetCSWpnData().m_szDroppedModel);
    m_inPrecache = false;
}

bool CWeaponElite::Deploy()
{
    m_flAccuracy = 0.88;
    return BaseClass::Deploy();
}

int CWeaponElite::GetWorldModelIndex(void)
{
    if (GetOwner() || m_inPrecache)
    {
        return m_iWorldModelIndex;
    }
    else
    {
        return m_droppedModelIndex;
    }
}

const char * CWeaponElite::GetWorldModel(void) const
{
    if (GetOwner() || m_inPrecache)
    {
        return BaseClass::GetWorldModel();
    }
    else
    {
        return GetCSWpnData().m_szDroppedModel;
    }
}

void CWeaponElite::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        ELITEFire(1.3f * (1 - m_flAccuracy));

    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        ELITEFire(0.175f * (1 - m_flAccuracy));

    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        ELITEFire(0.08f * (1 - m_flAccuracy));

    else
        ELITEFire(0.1f * (1 - m_flAccuracy));
}

void CWeaponElite::ELITEFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    pPlayer->m_iShotsFired++;

    if (pPlayer->m_iShotsFired > 1)
        return;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.275)*(0.325 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.88)
        m_flAccuracy = 0.88;
    else if (m_flAccuracy < 0.55)
        m_flAccuracy = 0.55;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_iClip1--;
    m_bFireRight = !m_bFireRight; // flip side

    pPlayer->DoMuzzleFlash();

    //SetPlayerShieldAnim();

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        m_bFireRight ? Primary_Mode : Secondary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        flSpread);

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetCSWpnData().m_flCycleTime;

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 2.5);

    if (m_bFireRight)  //even number
    {
        if (m_iClip1 > 1)
            SendWeaponAnim(ACT_VM_PRIMARYATTACK);
        else
            SendWeaponAnim(ACT_VM_DRYFIRE_LEFT);
    }
    else
    {
        if (m_iClip1 > 0)
            SendWeaponAnim(ACT_VM_SECONDARYATTACK);
        else
            SendWeaponAnim(ACT_VM_DRYFIRE);
    }

    QAngle punchAngle = pPlayer->GetPunchAngle();
    punchAngle.x -= 2;
    pPlayer->SetPunchAngle(punchAngle);

    //ResetPlayerShieldAnim();
}


bool CWeaponElite::Reload()
{
    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.88;
    m_bFireRight = false;
    return true;
}

void CWeaponElite::WeaponIdle()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;
    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        if (m_iClip1 == 1)
            SendWeaponAnim(ACT_VM_IDLE_EMPTY_LEFT);
        else
            SendWeaponAnim(ACT_VM_IDLE);
    }

}

#ifdef CLIENT_DLL

bool CWeaponElite::OnFireEvent(C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options)
{
    if (event == 5001)
    {
        CMomentumPlayer *pPlayer = GetPlayerOwner();
        if (pPlayer && pPlayer->GetFOV() < pPlayer->GetDefaultFOV() && HideViewModelWhenZoomed())
            return true;

        CEffectData data;
        data.m_fFlags = 0;
        data.m_hEntity = pViewModel->GetRefEHandle();
        data.m_nAttachmentIndex = m_bFireRight ? 2 : 1; // toggle muzzle flash
        data.m_flScale = GetCSWpnData().m_flMuzzleScale;

        DispatchEffect("CS_MuzzleFlash", data);

        return true;
    }

    return BaseClass::OnFireEvent(pViewModel, origin, angles, event, options);
}

int CWeaponElite::GetMuzzleAttachment(void)
{
    if (m_bFireRight)  //even number
    {
        return LookupAttachment("muzzle_flash_r");
    }
    else
    {
        return LookupAttachment("muzzle_flash_l");
    }
}

#endif


// FIVESEVEN
class CWeaponFiveSeven : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponFiveSeven, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponFiveSeven();

    virtual void Spawn();


    virtual void PrimaryAttack();
    virtual void SecondaryAttack();
    virtual bool Deploy();

    virtual bool Reload();

    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_FIVESEVEN; }

private:

    CWeaponFiveSeven(const CWeaponFiveSeven &);

    void FiveSevenFire(float flSpread);

    float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFiveSeven, DT_WeaponFiveSeven)

BEGIN_NETWORK_TABLE(CWeaponFiveSeven, DT_WeaponFiveSeven)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponFiveSeven)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_fiveseven, CWeaponFiveSeven);
PRECACHE_WEAPON_REGISTER(weapon_fiveseven);



CWeaponFiveSeven::CWeaponFiveSeven()
{
    m_flLastFire = gpGlobals->curtime;
}


void CWeaponFiveSeven::Spawn()
{
    BaseClass::Spawn();

    m_flAccuracy = 0.92;
}

bool CWeaponFiveSeven::Deploy()
{
    m_flAccuracy = 0.92;
    return BaseClass::Deploy();
}

void CWeaponFiveSeven::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        FiveSevenFire(1.5f * (1 - m_flAccuracy));
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        FiveSevenFire(0.255f * (1 - m_flAccuracy));
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        FiveSevenFire(0.075f * (1 - m_flAccuracy));
    else
        FiveSevenFire(0.15f * (1 - m_flAccuracy));
}

void CWeaponFiveSeven::SecondaryAttack()
{
}

void CWeaponFiveSeven::FiveSevenFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    pPlayer->m_iShotsFired++;

    if (pPlayer->m_iShotsFired > 1)
        return;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.25)*(0.275 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.92)
        m_flAccuracy = 0.92;
    else if (m_flAccuracy < 0.725)
        m_flAccuracy = 0.725;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_iClip1--;
    pPlayer->DoMuzzleFlash();

    SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);


    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        flSpread);

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetCSWpnData().m_flCycleTime;

    if (!m_iClip1 && pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 2);

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= 2;
    pPlayer->SetPunchAngle(angle);
}


bool CWeaponFiveSeven::Reload()
{
    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.92;
    return true;
}

void CWeaponFiveSeven::WeaponIdle()
{
    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        SetWeaponIdleTime(gpGlobals->curtime + 4);
        SendWeaponAnim(ACT_VM_IDLE);
    }
}


//GLOCK
class CWeaponGlock : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponGlock, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponGlock();

    virtual void Spawn();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack();
    virtual bool Deploy();

    virtual void ItemPostFrame();

    void GlockFire(float flSpread, bool bFireBurst);
    void FireRemaining(int &shotsFired, float &shootTime);

    virtual bool Reload();

    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_GLOCK; }

private:

    CWeaponGlock(const CWeaponGlock &);

    CNetworkVar(bool, m_bBurstMode);

    int		m_iGlock18ShotsFired;	// used to keep track of the shots fired during the Glock18 burst fire mode.
    float	m_flGlock18Shoot;		// time to shoot the remaining bullets of the glock18 burst fire
    float	m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGlock, DT_WeaponGlock)

BEGIN_NETWORK_TABLE(CWeaponGlock, DT_WeaponGlock)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bBurstMode))
#else
SendPropBool(SENDINFO(m_bBurstMode))
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponGlock)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_glock, CWeaponGlock);
PRECACHE_WEAPON_REGISTER(weapon_glock);



CWeaponGlock::CWeaponGlock()
{
    m_flLastFire = gpGlobals->curtime;
}


void CWeaponGlock::Spawn()
{
    BaseClass::Spawn();

    m_bBurstMode = false;
    m_iGlock18ShotsFired = 0;
    m_flGlock18Shoot = 0.0;
    m_flAccuracy = 0.9;
}

bool CWeaponGlock::Deploy()
{
    m_iGlock18ShotsFired = 0;
    m_flGlock18Shoot = 0.0;
    m_flAccuracy = 0.9;

    return BaseClass::Deploy();
}

void CWeaponGlock::SecondaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_bBurstMode)
    {
        ClientPrint(pPlayer, HUD_PRINTCENTER, "#Switch_To_SemiAuto");
        m_bBurstMode = false;
    }
    else
    {
        ClientPrint(pPlayer, HUD_PRINTCENTER, "#Switch_To_BurstFire");
        m_bBurstMode = true;
    }

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
}

void CWeaponGlock::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_bBurstMode)
    {
        if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
            GlockFire(1.2f * (1 - m_flAccuracy), true);

        else if (pPlayer->GetAbsVelocity().Length2D() > 5)
            GlockFire(0.185f * (1 - m_flAccuracy), true);

        else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
            GlockFire(0.095f * (1 - m_flAccuracy), true);

        else
            GlockFire(0.3f * (1 - m_flAccuracy), true);
    }
    else
    {
        if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
            GlockFire(1.0f * (1 - m_flAccuracy), false);

        else if (pPlayer->GetAbsVelocity().Length2D() > 5)
            GlockFire(0.165f * (1 - m_flAccuracy), false);

        else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
            GlockFire(0.075f * (1 - m_flAccuracy), false);

        else
            GlockFire(0.1f * (1 - m_flAccuracy), false);
    }
}


// GOOSEMAN : FireRemaining used by Glock18
void CWeaponGlock::FireRemaining(int &shotsFired, float &shootTime)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        Error("!pPlayer");

    float nexttime = 0.1;
    m_iClip1--;
    if (m_iClip1 < 0)
    {
        m_iClip1 = 0;
        shotsFired = 3;
        shootTime = 0.0f;
        return;
    }

    // TODO FIXME damage = 18, rangemode 0.9

    // Dispatch the FX right away with full accuracy.
    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Secondary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255, // wrap it for network traffic so it's the same between client and server
        0.05);

    /*
    #if defined( CLIENT_WEAPONS )
    int flag = FEV_NOTHOST;
    #else
    int flag = 0;
    #endif

    PLAYBACK_EVENT_FULL( flag, m_pPlayer->edict(), m_usFireGlock18,
    0.0, (float *)&g_vecZero, (float *)&g_vecZero,
    vecDir.x,
    vecDir.y,
    m_pPlayer->pev->punchangle.x * 10000,
    m_pPlayer->pev->punchangle.y * 10000,
    m_iClip1 ? 0 : 1, 0 );
    */

    //pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    shotsFired++;

    if (shotsFired != 3)
        shootTime = gpGlobals->curtime + nexttime;
    else
        shootTime = 0.0;
}


void CWeaponGlock::ItemPostFrame()
{
    if (m_flGlock18Shoot != 0.0)
        FireRemaining(m_iGlock18ShotsFired, m_flGlock18Shoot);

    BaseClass::ItemPostFrame();
}


void CWeaponGlock::GlockFire(float flSpread, bool bFireBurst)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    float flCycleTime = GetCSWpnData().m_flCycleTime;

    if (bFireBurst)
    {
        m_iGlock18ShotsFired = 0;
        flCycleTime = 0.5f;
    }
    else
    {
        pPlayer->m_iShotsFired++;

        if (pPlayer->m_iShotsFired > 1)
            return;
    }

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.275)*(0.325 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.9)
        m_flAccuracy = 0.9;
    else if (m_flAccuracy < 0.6)
        m_flAccuracy = 0.6;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_iClip1--;

    pPlayer->DoMuzzleFlash();

    //SetPlayerShieldAnim();

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    // non-silenced
    //pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
    //pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255, // wrap it for network traffic so it's the same between client and server
        flSpread);

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 2.5);

    if (bFireBurst)
    {
        // Fire off the next two rounds
        m_flGlock18Shoot = gpGlobals->curtime + 0.1;
        m_iGlock18ShotsFired++;

        SendWeaponAnim(ACT_VM_SECONDARYATTACK);
    }
    else
    {
        SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    }

    //ResetPlayerShieldAnim();
}


bool CWeaponGlock::Reload()
{
    if (m_flGlock18Shoot != 0)
        return true;

    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.9;
    return true;
}

void CWeaponGlock::WeaponIdle()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        SendWeaponAnim(ACT_VM_IDLE);
    }
}


//P228
class CWeaponP228 : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponP228, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponP228();

    virtual void Spawn();

    virtual void PrimaryAttack();
    virtual bool Deploy();

    virtual bool Reload();
    virtual void WeaponIdle();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_P228; }


private:

    CWeaponP228(const CWeaponP228 &);
    void P228Fire(float flSpread);

    float m_flLastFire;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponP228, DT_WeaponP228)

BEGIN_NETWORK_TABLE(CWeaponP228, DT_WeaponP228)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponP228)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_p228, CWeaponP228);
PRECACHE_WEAPON_REGISTER(weapon_p228);



CWeaponP228::CWeaponP228()
{
    m_flLastFire = gpGlobals->curtime;
}


void CWeaponP228::Spawn()
{
    m_flAccuracy = 0.9;

    BaseClass::Spawn();
}


bool CWeaponP228::Deploy()
{
    m_flAccuracy = 0.9;

    return BaseClass::Deploy();
}

void CWeaponP228::PrimaryAttack(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        P228Fire(1.5f * (1 - m_flAccuracy));
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        P228Fire(0.255f * (1 - m_flAccuracy));
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        P228Fire(0.075f * (1 - m_flAccuracy));
    else
        P228Fire(0.15f * (1 - m_flAccuracy));
}

void CWeaponP228::P228Fire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    pPlayer->m_iShotsFired++;

    if (pPlayer->m_iShotsFired > 1)
        return;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.3)*(0.325 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.9)
        m_flAccuracy = 0.9;
    else if (m_flAccuracy < 0.6)
        m_flAccuracy = 0.6;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_iClip1--;

    pPlayer->DoMuzzleFlash();
    //SetPlayerShieldAnim();

    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);

    // Aiming
    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        flSpread);

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetCSWpnData().m_flCycleTime;

    if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 2);

    //ResetPlayerShieldAnim();

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= 2;
    pPlayer->SetPunchAngle(angle);
}


bool CWeaponP228::Reload()
{
    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.9;
    return true;
}

void CWeaponP228::WeaponIdle()
{
    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        SetWeaponIdleTime(gpGlobals->curtime + 3.0);
        SendWeaponAnim(ACT_VM_IDLE);
    }
}


//USP
class CWeaponUSP : public CWeaponCSBase
{
public:
    DECLARE_CLASS(CWeaponUSP, CWeaponCSBase);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponUSP();

    virtual void Spawn();
    virtual void Precache();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack();
    virtual bool Deploy();
    virtual bool Holster(CBaseCombatWeapon *pSwitchingTo);

    virtual bool Reload();
    virtual void WeaponIdle();

    // We overload this so we can translate all weapon activities to silenced versions.
    virtual bool SendWeaponAnim(int iActivity);

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_USP; }

    // return true if this weapon has a silencer equipped
    virtual bool IsSilenced(void) const { return m_bSilencerOn; }

    virtual Activity GetDeployActivity(void);

#ifdef CLIENT_DLL
    virtual int GetMuzzleFlashStyle(void);
#endif

    virtual const char		*GetWorldModel(void) const;
    virtual int				GetWorldModelIndex(void);

private:

    CWeaponUSP(const CWeaponUSP &);
    void USPFire(float flSpread);

    CNetworkVar(bool, m_bSilencerOn);
    CNetworkVar(float, m_flDoneSwitchingSilencer);	// soonest time switching the silencer will be complete
    float m_flLastFire;

    int m_silencedModelIndex;
    bool m_inPrecache;
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponUSP, DT_WeaponUSP)

BEGIN_NETWORK_TABLE(CWeaponUSP, DT_WeaponUSP)
#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bSilencerOn)),
RecvPropTime(RECVINFO(m_flDoneSwitchingSilencer)),
#else
SendPropBool(SENDINFO(m_bSilencerOn)),
SendPropTime(SENDINFO(m_flDoneSwitchingSilencer)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponUSP)
DEFINE_PRED_FIELD(m_bSilencerOn, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_usp, CWeaponUSP);
PRECACHE_WEAPON_REGISTER(weapon_usp);


Activity g_SilencedTranslations[][2] =
{
    { ACT_VM_RELOAD, ACT_VM_RELOAD_SILENCED },
    { ACT_VM_PRIMARYATTACK, ACT_VM_PRIMARYATTACK_SILENCED },
    { ACT_VM_DRAW, ACT_VM_DRAW_SILENCED },
};



CWeaponUSP::CWeaponUSP()
{
    m_flLastFire = gpGlobals->curtime;
    m_bSilencerOn = false;
    m_flDoneSwitchingSilencer = 0.0f;
    m_inPrecache = false;
}


void CWeaponUSP::Spawn()
{
    //m_iDefaultAmmo = 12;
    m_flAccuracy = 0.92;
    m_bSilencerOn = false;
    m_flDoneSwitchingSilencer = 0.0f;

    //FallInit();// get ready to fall down.
    BaseClass::Spawn();
}


void CWeaponUSP::Precache()
{
    m_inPrecache = true;
    BaseClass::Precache();

    m_silencedModelIndex = CBaseEntity::PrecacheModel(GetCSWpnData().m_szSilencerModel);
    m_inPrecache = false;
}


int CWeaponUSP::GetWorldModelIndex(void)
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


const char * CWeaponUSP::GetWorldModel(void) const
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


bool CWeaponUSP::Deploy()
{
    m_flAccuracy = 0.92;
    m_flDoneSwitchingSilencer = 0.0f;

    return BaseClass::Deploy();
}

bool CWeaponUSP::Holster(CBaseCombatWeapon *pSwitchingTo)
{
    if (m_flDoneSwitchingSilencer > 0.0f && m_flDoneSwitchingSilencer > gpGlobals->curtime)
    {
        // still switching the silencer.  Cancel the switch.
        m_bSilencerOn = !m_bSilencerOn;
        SetWeaponModelIndex(GetWorldModel());
    }

    return BaseClass::Holster(pSwitchingTo);
}

Activity CWeaponUSP::GetDeployActivity(void)
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

void CWeaponUSP::SecondaryAttack()
{
    if (m_bSilencerOn)
    {
        SendWeaponAnim(ACT_VM_DETACH_SILENCER);
    }
    else
    {
        SendWeaponAnim(ACT_VM_ATTACH_SILENCER);
    }
    m_bSilencerOn = !m_bSilencerOn;
    m_flDoneSwitchingSilencer = gpGlobals->curtime + 3;

    m_flNextSecondaryAttack = gpGlobals->curtime + 3;
    m_flNextPrimaryAttack = gpGlobals->curtime + 3;
    SetWeaponIdleTime(gpGlobals->curtime + 3);

    SetWeaponModelIndex(GetWorldModel());
}


void CWeaponUSP::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (m_bSilencerOn)
    {
        if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
            USPFire(1.3f * (1 - m_flAccuracy));
        else if (pPlayer->GetAbsVelocity().Length2D() > 5)
            USPFire(0.25f * (1 - m_flAccuracy));
        else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
            USPFire(0.125f * (1 - m_flAccuracy));
        else
            USPFire(0.15f * (1 - m_flAccuracy));
    }
    else
    {
        if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
            USPFire(1.2f * (1 - m_flAccuracy));
        else if (pPlayer->GetAbsVelocity().Length2D() > 5)
            USPFire(0.225f * (1 - m_flAccuracy));
        else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
            USPFire(0.08f * (1 - m_flAccuracy));
        else
            USPFire(0.1f * (1 - m_flAccuracy));
    }

}

void CWeaponUSP::USPFire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    float flCycleTime = GetCSWpnData().m_flCycleTime;

    pPlayer->m_iShotsFired++;

    if (pPlayer->m_iShotsFired > 1)
        return;

    // Mark the time of this shot and determine the accuracy modifier based on the last shot fired...
    m_flAccuracy -= (0.275)*(0.3 - (gpGlobals->curtime - m_flLastFire));

    if (m_flAccuracy > 0.92)
        m_flAccuracy = 0.92;
    else if (m_flAccuracy < 0.6)
        m_flAccuracy = 0.6;

    m_flLastFire = gpGlobals->curtime;

    if (m_iClip1 <= 0)
    {
        if (m_bFireOnEmpty)
        {
            PlayEmptySound();
            m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
        }

        return;
    }

    m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + flCycleTime;

    m_iClip1--;

    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

    // player "shoot" animation
    pPlayer->SetAnimation(PLAYER_ATTACK1);


    if (!m_bSilencerOn)
    {
        pPlayer->DoMuzzleFlash();
    }

    FX_FireBullets(
        pPlayer->entindex(),
        pPlayer->Weapon_ShootPosition(),
        pPlayer->EyeAngles() + 2.0f * pPlayer->GetPunchAngle(),
        GetWeaponID(),
        m_bSilencerOn ? Secondary_Mode : Primary_Mode,
        CBaseEntity::GetPredictionRandomSeed() & 255,
        flSpread);

    if (!m_iClip1 && pPlayer->GetAmmoCount(GetPrimaryAmmoType()) <= 0)
    {
        // HEV suit - indicate out of ammo condition
        pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
    }

    SetWeaponIdleTime(gpGlobals->curtime + 2);

    QAngle angle = pPlayer->GetPunchAngle();
    angle.x -= 2;
    pPlayer->SetPunchAngle(angle);
}


bool CWeaponUSP::Reload()
{
    if (!DefaultPistolReload())
        return false;

    m_flAccuracy = 0.92;
    return true;
}

void CWeaponUSP::WeaponIdle()
{
    if (m_flTimeWeaponIdle > gpGlobals->curtime)
        return;

    // only idle if the slid isn't back
    if (m_iClip1 != 0)
    {
        SetWeaponIdleTime(gpGlobals->curtime + 6.0);
    }
}

bool CWeaponUSP::SendWeaponAnim(int iActivity)
{
    // Translate the activity?
    if (m_bSilencerOn)
    {
        for (int i = 0; i < ARRAYSIZE(g_SilencedTranslations); i++)
        {
            if (g_SilencedTranslations[i][0] == iActivity)
            {
                iActivity = g_SilencedTranslations[i][1];
                break;
            }
        }
    }

    return BaseClass::SendWeaponAnim(iActivity);
}


#ifdef CLIENT_DLL
int CWeaponUSP::GetMuzzleFlashStyle(void)
{
    if (m_bSilencerOn)
    {
        return CS_MUZZLEFLASH_NONE;
    }
    else
    {
        return CS_MUZZLEFLASH_NORM;
    }
}
#endif