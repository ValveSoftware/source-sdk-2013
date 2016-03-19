//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEPROJECTILE_H
#define BASEPROJECTILE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

#ifdef GAME_DLL
#include "baseanimating.h"
#else
#include "c_baseanimating.h"
#endif

#ifdef CLIENT_DLL
#define CBaseProjectile C_BaseProjectile
#endif // CLIENT_DLL

//=============================================================================
//
// Base Projectile.
//
//=============================================================================
#ifdef CLIENT_DLL
class CBaseProjectile : public CBaseAnimating
#else // CLIENT_DLL
DECLARE_AUTO_LIST( IBaseProjectileAutoList );
class CBaseProjectile : public CBaseAnimating, public IBaseProjectileAutoList
#endif // !CLIENT_DLL
{
public:
	DECLARE_CLASS( CBaseProjectile, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CBaseProjectile();

	virtual void Spawn();

#ifdef GAME_DLL
	virtual int GetBaseProjectileType() const { return -1; } // no base
	virtual int GetProjectileType() const { return -1; } // no type
	virtual int GetDestroyableHitCount( void ) const { return m_iDestroyableHitCount; }
	void IncrementDestroyableHitCount( void ) { ++m_iDestroyableHitCount; }

	virtual bool CanCollideWithTeammates() const { return m_bCanCollideWithTeammates; }
	virtual float GetCollideWithTeammatesDelay() const { return 0.25f; }
#endif // GAME_DLL

	virtual bool IsDestroyable( void ) { return false; }
	virtual void Destroy( bool bBlinkOut = true, bool bBreakRocket = false ) {}
	virtual void SetLauncher( CBaseEntity *pLauncher );
	CBaseEntity *GetOriginalLauncher() const { return m_hOriginalLauncher; }

protected:
#ifdef GAME_DLL
	void CollideWithTeammatesThink();

	int m_iDestroyableHitCount;
#endif // GAME_DLL

private:

#ifdef GAME_DLL
	void	ResetCollideWithTeammates();

	bool					m_bCanCollideWithTeammates;
#endif // GAME_DLL

	CNetworkHandle( CBaseEntity, m_hOriginalLauncher );
};

#endif // BASEPROJECTILE_H
