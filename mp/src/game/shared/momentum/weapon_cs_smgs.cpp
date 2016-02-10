#include "cbase.h"
#include "weapon_csbasegun.h"
#include "mom_player_shared.h"
#include "tier0/memdbgon.h"

#if defined( CLIENT_DLL )
#define CWeaponMAC10 C_WeaponMAC10
#define CWeaponMP5Navy C_WeaponMP5Navy
#define CWeaponP90 C_WeaponP90
#define CWeaponTMP C_WeaponTMP
#define CWeaponUMP45 C_WeaponUMP45
#endif

//MAC10
class CWeaponMAC10 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponMAC10, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponMAC10();

    virtual void Spawn();
    virtual void PrimaryAttack();
    virtual bool Deploy();
    virtual bool Reload();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_MAC10; }


private:

    void MAC10Fire(float flSpread);

    CWeaponMAC10(const CWeaponMAC10 &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMAC10, DT_WeaponMAC10)

BEGIN_NETWORK_TABLE(CWeaponMAC10, DT_WeaponMAC10)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponMAC10)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_mac10, CWeaponMAC10);
PRECACHE_WEAPON_REGISTER(weapon_mac10);



CWeaponMAC10::CWeaponMAC10()
{
}


void CWeaponMAC10::Spawn()
{
    BaseClass::Spawn();

    m_flAccuracy = 0.15;
}


bool CWeaponMAC10::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flAccuracy = 0.15;

    return ret;
}

bool CWeaponMAC10::Reload()
{
    bool ret = BaseClass::Reload();

    m_flAccuracy = 0.15;

    return ret;
}


void CWeaponMAC10::MAC10Fire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))	// jumping
        pPlayer->KickBack(1.3, 0.55, 0.4, 0.05, 4.75, 3.75, 5);
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)				// running
        pPlayer->KickBack(0.9, 0.45, 0.25, 0.035, 3.5, 2.75, 7);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))	// ducking
        pPlayer->KickBack(0.75, 0.4, 0.175, 0.03, 2.75, 2.5, 10);
    else														// standing
        pPlayer->KickBack(0.775, 0.425, 0.2, 0.03, 3, 2.75, 9);
}


void CWeaponMAC10::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        MAC10Fire(0.375f * m_flAccuracy);
    else
        MAC10Fire(0.03f * m_flAccuracy);
}

//MP5NAVY
class CWeaponMP5Navy : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponMP5Navy, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponMP5Navy();

    virtual void Spawn();
    virtual void PrimaryAttack();
    virtual bool Deploy();
    virtual bool Reload();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_MP5NAVY; }


private:

    CWeaponMP5Navy(const CWeaponMP5Navy &);

    void MP5NFire(float flSpread);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponMP5Navy, DT_WeaponMP5Navy)

BEGIN_NETWORK_TABLE(CWeaponMP5Navy, DT_WeaponMP5Navy)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponMP5Navy)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_mp5navy, CWeaponMP5Navy);
PRECACHE_WEAPON_REGISTER(weapon_mp5navy);



CWeaponMP5Navy::CWeaponMP5Navy()
{
}

void CWeaponMP5Navy::Spawn()
{
    BaseClass::Spawn();

    m_flAccuracy = 0.0;
}


bool CWeaponMP5Navy::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flAccuracy = 0.0;

    return ret;
}

bool CWeaponMP5Navy::Reload()
{
    bool ret = BaseClass::Reload();

    m_flAccuracy = 0.0;

    return ret;
}

void CWeaponMP5Navy::PrimaryAttack(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        MP5NFire(0.2f * m_flAccuracy);
    else
        MP5NFire(0.04f * m_flAccuracy);
}

void CWeaponMP5Navy::MP5NFire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    // Kick the gun based on the state of the player.
    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(0.9, 0.475, 0.35, 0.0425, 5, 3, 6);
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(0.5, 0.275, 0.2, 0.03, 3, 2, 10);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.225, 0.15, 0.1, 0.015, 2, 1, 10);
    else
        pPlayer->KickBack(0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10);
}


//P90
class CWeaponP90 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponP90, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponP90();

    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_P90; }


private:

    CWeaponP90(const CWeaponP90 &);

    void P90Fire(float flSpread);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponP90, DT_WeaponP90)

BEGIN_NETWORK_TABLE(CWeaponP90, DT_WeaponP90)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponP90)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_p90, CWeaponP90);
PRECACHE_WEAPON_REGISTER(weapon_p90);



CWeaponP90::CWeaponP90()
{

}

void CWeaponP90::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        P90Fire(0.3f * m_flAccuracy);
    else if (pPlayer->GetAbsVelocity().Length2D() > 170)
        P90Fire(0.115f * m_flAccuracy);
    else
        P90Fire(0.045f * m_flAccuracy);
}


void CWeaponP90::P90Fire(float flSpread)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    // Kick the gun based on the state of the player.
    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(0.9, 0.45, 0.35, 0.04, 5.25, 3.5, 4);
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(0.45, 0.3, 0.2, 0.0275, 4, 2.25, 7);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.275, 0.2, 0.125, 0.02, 3, 1, 9);
    else
        pPlayer->KickBack(0.3, 0.225, 0.125, 0.02, 3.25, 1.25, 8);
}


//TMP
class CWeaponTMP : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponTMP, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponTMP();

    virtual void PrimaryAttack();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_TMP; }

    virtual bool IsSilenced(void) const { return true; }

private:

    CWeaponTMP(const CWeaponTMP &);

    void TMPFire(float flSpread);
    void DoFireEffects(void);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponTMP, DT_WeaponTMP)

BEGIN_NETWORK_TABLE(CWeaponTMP, DT_WeaponTMP)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponTMP)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_tmp, CWeaponTMP);
PRECACHE_WEAPON_REGISTER(weapon_tmp);



CWeaponTMP::CWeaponTMP()
{
}


void CWeaponTMP::PrimaryAttack(void)
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        TMPFire(0.25f * m_flAccuracy);
    else
        TMPFire(0.03f * m_flAccuracy);
}

void CWeaponTMP::TMPFire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(1.1, 0.5, 0.35, 0.045, 4.5, 3.5, 6);
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(0.8, 0.4, 0.2, 0.03, 3, 2.5, 7);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.7, 0.35, 0.125, 0.025, 2.5, 2, 10);
    else
        pPlayer->KickBack(0.725, 0.375, 0.15, 0.025, 2.75, 2.25, 9);
}

void CWeaponTMP::DoFireEffects(void)
{
    // TMP is silenced, so do nothing
}

//UMP45
class CWeaponUMP45 : public CWeaponCSBaseGun
{
public:
    DECLARE_CLASS(CWeaponUMP45, CWeaponCSBaseGun);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponUMP45();

    virtual void Spawn();
    virtual void PrimaryAttack();
    virtual bool Deploy();
    virtual bool Reload();

    virtual CSWeaponID GetWeaponID(void) const { return WEAPON_UMP45; }


private:

    CWeaponUMP45(const CWeaponUMP45 &);

    void UMP45Fire(float flSpread);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponUMP45, DT_WeaponUMP45)

BEGIN_NETWORK_TABLE(CWeaponUMP45, DT_WeaponUMP45)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponUMP45)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_ump45, CWeaponUMP45);
PRECACHE_WEAPON_REGISTER(weapon_ump45);



CWeaponUMP45::CWeaponUMP45()
{
}


void CWeaponUMP45::Spawn()
{
    BaseClass::Spawn();

    m_flAccuracy = 0.0;
}


bool CWeaponUMP45::Deploy()
{
    bool ret = BaseClass::Deploy();

    m_flAccuracy = 0.0;

    return ret;
}

bool CWeaponUMP45::Reload()
{
    bool ret = BaseClass::Reload();

    m_flAccuracy = 0.0;

    return ret;
}

void CWeaponUMP45::PrimaryAttack()
{
    CMomentumPlayer *pPlayer = GetPlayerOwner();
    if (!pPlayer)
        return;

    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        UMP45Fire(0.24f * m_flAccuracy);
    else
        UMP45Fire(0.04f * m_flAccuracy);
}

void CWeaponUMP45::UMP45Fire(float flSpread)
{
    if (!CSBaseGunFire(flSpread, GetCSWpnData().m_flCycleTime, true))
        return;

    CMomentumPlayer *pPlayer = GetPlayerOwner();

    // CSBaseGunFire can kill us, forcing us to drop our weapon, if we shoot something that explodes
    if (!pPlayer)
        return;

    // Kick the gun based on the state of the player.
    if (!FBitSet(pPlayer->GetFlags(), FL_ONGROUND))
        pPlayer->KickBack(0.125, 0.65, 0.55, 0.0475, 5.5, 4, 10);
    else if (pPlayer->GetAbsVelocity().Length2D() > 5)
        pPlayer->KickBack(0.55, 0.3, 0.225, 0.03, 3.5, 2.5, 10);
    else if (FBitSet(pPlayer->GetFlags(), FL_DUCKING))
        pPlayer->KickBack(0.25, 0.175, 0.125, 0.02, 2.25, 1.25, 10);
    else
        pPlayer->KickBack(0.275, 0.2, 0.15, 0.0225, 2.5, 1.5, 10);
}