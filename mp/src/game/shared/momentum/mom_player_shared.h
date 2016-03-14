#ifndef MOM_PLAYER_SHARED_H
#define MOM_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#ifdef CLIENT_DLL
#include "momentum/c_mom_player.h"
#define CMomentumPlayer C_MomentumPlayer
#else
#include "momentum/mom_player.h"
#endif


inline CMomentumPlayer *ToCMOMPlayer(CBaseEntity *pEntity)
{
    if (!pEntity || !pEntity->IsPlayer())
        return NULL;
    return dynamic_cast<CMomentumPlayer*>(pEntity);
}

#endif // MOM_PLAYER_SHARED_H