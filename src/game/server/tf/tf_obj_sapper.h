//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Upgrade that damages the object over time
//
//=============================================================================//

#ifndef TF_OBJ_SAPPER_H
#define TF_OBJ_SAPPER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj_baseupgrade_shared.h"

#define SAPPER_MAX_HEALTH	100


enum SapperModel_t
{
	SAPPER_MODEL_PLACED,
	SAPPER_MODEL_PLACEMENT,
	SAPPER_MODEL_TOTAL
};


// ------------------------------------------------------------------------ //
// Sapper upgrade
// ------------------------------------------------------------------------ //
class CObjectSapper : public CBaseObjectUpgrade
{
	DECLARE_CLASS( CObjectSapper, CBaseObjectUpgrade );

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CObjectSapper();

	virtual void	Spawn();
	virtual void	Precache();
	void			Precache( const char *pchBaseModel );
	virtual bool	IsHostileUpgrade( void ) { return true; }
	virtual void	FinishedBuilding( void );
	virtual void	SetupAttachedVersion( void );
	virtual void	DetachObjectFromObject( void );
	virtual void	UpdateOnRemove( void );
	virtual void	OnGoActive( void );
	bool			IsParentValid( void );

	const char*		GetSapperModelName( SapperModel_t nModel, const char *pchModelName = NULL );
	const char*		GetSapperSoundName( void );

	virtual void	SapperThink( void );

	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void	Killed( const CTakeDamageInfo &info );

	virtual int		GetBaseHealth( void );

	void			ApplyRoboSapper( CTFPlayer *pTarget, float flDuration, int nRadius = 200 );
	bool			ApplyRoboSapperEffects( CTFPlayer *pTarget, float flDuration );
	bool			IsValidRoboSapperTarget( CTFPlayer *pTarget );

	float			GetReversesBuildingConstructionSpeed( void );

private:
	float m_flSapperDamageAccumulator;
	float m_flLastThinkTime;
	float m_flLastHealthLeachTime;
	
	float m_flSelfDestructTime;
	float m_flSapperStartTime;

	char m_szSapperModel[ _MAX_PATH ];
	char m_szPlacementModel[ _MAX_PATH ];
	char szSapperSound[ _MAX_PATH ];
};

#endif // TF_OBJ_SAPPER_H
