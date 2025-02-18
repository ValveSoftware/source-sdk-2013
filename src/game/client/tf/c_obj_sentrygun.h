//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_SENTRYGUN_H
#define C_OBJ_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"
#include "c_tf_projectile_rocket.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "c_tf_player.h"

class C_MuzzleFlashModel;

enum
{
	SHIELD_NONE = 0,
	SHIELD_NORMAL,	// 33% damage taken
	SHIELD_MAX,		// 10% damage taken, no inactive period
};

//-----------------------------------------------------------------------------
// Purpose: Sentry object
//-----------------------------------------------------------------------------
class C_ObjectSentrygun : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectSentrygun, C_BaseObject );
public:
	DECLARE_CLIENTCLASS();

	C_ObjectSentrygun();

	virtual void UpdateOnRemove( void );

	void GetAmmoCount( int &iShells, int &iMaxShells, int &iRockets, int & iMaxRockets );

	virtual BuildingHudAlert_t GetBuildingAlertLevel( void );

	virtual const char *GetHudStatusIcon( void );

	int GetKills( void ) { return m_iKills; }
	int GetAssists( void ) { return m_iAssists; }

	virtual void GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );

	virtual CStudioHdr *OnNewModel( void );
	virtual void UpdateDamageEffects( BuildingDamageLevel_t damageLevel );

	virtual void OnPlacementStateChanged( bool bValidPlacement );

	void DebugDamageParticles();

	virtual const char* GetStatusName() const;

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual bool	IsUpgrading( void ) const { return ( m_iState == SENTRY_STATE_UPGRADING ); }

	void			CreateLaserBeam( void );
	void			DestroyLaserBeam( void );

	virtual void	SetDormant( bool bDormant );
	void			CreateShield( void );
	void			DestroyShield( void );

	void			CreateSiren( void );
	void			DestroySiren( void );

	virtual void	OnGoActive( void );
	virtual void	OnGoInactive( void );
	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	virtual void	ClientThink( void );

	void			CheckNearMiss( Vector vecStart, Vector vecEnd );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );

	virtual void	BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

private:

	virtual void UpgradeLevelChanged();

private:
	int m_iState;

	int m_iAmmoShells;
	int m_iMaxAmmoShells;
	int m_iAmmoRockets;

	int m_iKills;
	int m_iAssists;

	int m_iPlacementBodygroup;
	int m_iPlacementBodygroup_Mini;

	int m_iOldBodygroups;

	bool m_bPlayerControlled;
	bool m_bOldPlayerControlled;
	uint32 m_nShieldLevel;
	uint32 m_nOldShieldLevel;
	bool m_bOldCarried;

	bool m_bPDQSentry;

	int m_iOldModelIndex;

	bool m_bNearMiss;
	bool m_bRecreateShield;
	bool m_bRecreateLaserBeam;
	float m_flNextNearMissCheck;

	C_LocalTempEntity *m_pTempShield;

	HPARTICLEFFECT  m_hSirenEffect;
	HPARTICLEFFECT  m_hShieldEffect;
	HPARTICLEFFECT	m_hLaserBeamEffect;
	CNetworkHandle( CBaseEntity, m_hEnemy );
	CNetworkHandle( C_TFPlayer, m_hAutoAimTarget );

	Vector	m_vecLaserBeamPos;

private:
	C_ObjectSentrygun( const C_ObjectSentrygun & ); // not defined, not accessible
};

class C_TFProjectile_SentryRocket : public C_TFProjectile_Rocket
{
	DECLARE_CLASS( C_TFProjectile_SentryRocket, C_TFProjectile_Rocket );
public:
	DECLARE_CLIENTCLASS();

	virtual void CreateRocketTrails( void ) {}
};

#endif	//C_OBJ_SENTRYGUN_H
