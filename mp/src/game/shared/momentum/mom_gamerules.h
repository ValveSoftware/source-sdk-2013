#ifndef MOM_GAMERULES_H
#define MOM_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "gamerules.h"
#include "singleplay_gamerules.h"
#include "mom_player_shared.h"

#ifdef CLIENT_DLL
#define CMomentum C_Momentum
#else
#include "momentum/tickset.h"
#endif


class CMomentum : public CSingleplayRules
{
public:
    DECLARE_CLASS(CMomentum, CSingleplayRules);

#ifdef CLIENT_DLL

    DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else
    DECLARE_SERVERCLASS_NOBASE();
    

    //virtual void			Think(void);

    virtual bool			ClientCommand(CBaseEntity *pEdict, const CCommand &args);
    virtual void			PlayerSpawn(CBasePlayer *pPlayer);
    virtual bool			IsSpawnPointValid(CBaseEntity *pSpot, CBasePlayer *pPlayer);
    virtual CBaseEntity*    GetPlayerSpawnSpot(CBasePlayer *pPlayer);
    virtual const char *GetGameDescription(void) { return "Momentum"; }

    // Ammo
    virtual void			PlayerThink(CBasePlayer *pPlayer) {}
   // virtual float			GetAmmoDamage(CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType);

private:

    //void AdjustPlayerDamageTaken(CTakeDamageInfo *pInfo);
    //float AdjustPlayerDamageInflicted(float damage);
    Vector DropToGround(
        CBaseEntity *pMainEnt,
        const Vector &vPos,
        const Vector &vMins,
        const Vector &vMaxs);
    int DefaultFOV(void) { return 90; }
#endif

public:

    CMomentum();
    ~CMomentum();

};

inline CMomentum *GetMomentumGamerules()
{
    return static_cast<CMomentum*>(g_pGameRules);
}

#endif// MOM_GAMERULES_H