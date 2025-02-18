//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Clients CBaseObject
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASEOBJECT_H
#define C_BASEOBJECT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseobject_shared.h"
#include <vgui_controls/Panel.h>
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_basecombatcharacter.h"
#include "ihasbuildpoints.h"
#include <vgui/ILocalize.h>

class C_TFPlayer;

// Max Length of ID Strings
#define MAX_ID_STRING		256

extern mstudioevent_t *GetEventIndexForSequence( mstudioseqdesc_t &seqdesc );

DECLARE_AUTO_LIST( IBaseObjectAutoList );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_BaseObject : public C_BaseCombatCharacter, public IHasBuildPoints, public ITargetIDProvidesHint, public IBaseObjectAutoList
{
	DECLARE_CLASS( C_BaseObject, C_BaseCombatCharacter );
public:
	DECLARE_CLIENTCLASS();

	C_BaseObject();
	~C_BaseObject( void );

	virtual void	Spawn( void );

	virtual bool	IsBaseObject( void ) const { return true; }
	virtual bool	IsAnUpgrade(void ) const { return false; }

	virtual void	SetType( int iObjectType );

	virtual void	AddEntity();
	virtual void	Select( void );

	void			SetActivity( Activity act );
	Activity		GetActivity( ) const;
	void			SetObjectSequence( int sequence );
	virtual void	ResetClientsideFrame( void );

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual int		GetHealth() const { return m_iHealth; }
	void			SetHealth( int health ) { m_iHealth = health; }
	virtual int		GetMaxHealth() const { return m_iMaxHealth; }
	int				GetObjectFlags( void ) { return m_fObjectFlags; }
	void			SetObjectFlags( int flags ) { m_fObjectFlags = flags; }

	// Derive to customize an object's attached version
	virtual	void	SetupAttachedVersion( void ) { return; }

	virtual const char	*GetTargetDescription( void ) const;
	virtual const char	*GetIDString( void );
	virtual bool	IsValidIDTarget( void );

	virtual void	GetTargetIDString( OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sIDString, int iMaxLenInBytes, bool bSpectator );
	virtual void	GetTargetIDDataString( OUT_Z_BYTECAP(iMaxLenInBytes) wchar_t *sDataString, int iMaxLenInBytes );

	virtual bool	ShouldBeActive( void );
	virtual void	OnGoActive( void );
	virtual void	OnGoInactive( void );

	virtual void	UpdateOnRemove( void );

	C_TFPlayer		*GetBuilder( void ) { return m_hBuilder; }

	virtual void	SetDormant( bool bDormant );

	void			SendClientCommand( const char *pCmd );

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	// Builder preview...
	void			ActivateYawPreview( bool enable );
	void			PreviewYaw( float yaw );
	bool			IsPreviewingYaw() const;
	
	virtual void	RecalculateIDString( void );

	int GetType() const { return m_iObjectType; }
	bool IsOwnedByLocalPlayer() const;
	C_TFPlayer *GetOwner();

	virtual void	Simulate();

	virtual int		DrawModel( int flags );

	float			GetPercentageConstructed( void ) { return m_flPercentageConstructed; }

	bool			IsPlacing( void ) const { return m_bPlacing; }
	bool			IsBuilding( void ) const { return m_bBuilding; }
	virtual bool	IsUpgrading( void ) const { return false; }
	bool			IsCarried( void ) const { return m_bCarried; }

	float			GetReversesBuildingConstructionSpeed( void );

	virtual void	FinishedBuilding( void ) { return; }

	virtual const char* GetStatusName() const;

	// Object Previews
	void			HighlightBuildPoints( int flags );

	bool			HasSapper( void );

	bool			IsPlasmaDisabled( void );

	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;
	virtual bool	ShouldPlayersAvoid( void );

	bool			MustBeBuiltOnAttachmentPoint( void ) const;

	virtual bool	IsHostileUpgrade( void ) { return false; }

	// For ordering in hud building status
	virtual int		GetDisplayPriority( void );

	virtual const char *GetHudStatusIcon( void );

	virtual BuildingHudAlert_t GetBuildingAlertLevel( void );

	// Upgrading
	virtual int		GetUpgradeLevel( void ) { return m_iUpgradeLevel; }
	int				GetUpgradeMetal( void ) { return m_iUpgradeMetal; }
	virtual int		GetUpgradeMetalRequired( void ) { return m_iUpgradeMetalRequired; }
	virtual void	UpgradeLevelChanged() { return; }
	int				GetHighestUpgradeLevel( void ) { return m_iHighestUpgradeLevel; }

	int				GetObjectMode( void ) const { return m_iObjectMode; }

	// Shadows
	virtual ShadowType_t ShadowCastType( void ) OVERRIDE;

	// Stealth
	virtual float	GetInvisibilityLevel( void );
	virtual void	SetInvisibilityLevel( float flValue );
	bool			IsEnteringOrExitingFullyInvisible( float flValue ) { return ( ( m_flInvisibilityPercent != 1.f && flValue == 1.f ) || ( m_flInvisibilityPercent == 1.f && flValue != 1.f ) ); }

private:
	void StopAnimGeneratedSounds( void );

public:
	// Client/Server shared build point code
	void				CreateBuildPoints( void );
	void				AddAndParseBuildPoint( int iAttachmentNumber, KeyValues *pkvBuildPoint );
	virtual int			AddBuildPoint( int iAttachmentNum );
	virtual void		AddValidObjectToBuildPoint( int iPoint, int iObjectType );
	virtual CBaseObject *GetBuildPointObject( int iPoint );
	bool				IsBuiltOnAttachment( void ) { return m_hBuiltOnEntity.IsValid(); }
	void				AttachObjectToObject( CBaseEntity *pEntity, int iPoint, Vector &vecOrigin );
	CBaseObject			*GetParentObject( void );
	CBaseEntity			*GetParentEntity( void );
	void				SetBuildPointPassenger( int iPoint, int iPassenger );

	// Build points
	CUtlVector<BuildPoint_t>	m_BuildPoints;

	bool				IsDisabled( void ) { return m_bDisabled || m_bCarried; }

	// Shared placement
	bool 				VerifyCorner( const Vector &vBottomCenter, float xOffset, float yOffset );
	virtual bool		IsPlacementPosValid( void );
	virtual float		GetNearbyObjectCheckRadius( void ) { return 30.0; }

	virtual void		OnPlacementStateChanged( bool bValidPlacement );

	bool				ServerValidPlacement( void );		// allow server to trump our placement state

	bool				WasLastPlacementPosValid( void );	// query if we're in a valid place, when we last tried to calculate it

// IHasBuildPoints
public:
	virtual int			GetNumBuildPoints( void ) const;
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles );
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const;
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType );
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject );
	virtual float		GetMaxSnapDistance( int iBuildPoint );
	virtual bool		ShouldCheckForMovement( void ) { return true; }
	virtual int			GetNumObjectsOnMe( void );
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType );
	virtual void		RemoveAllObjects( void );
	virtual int			FindObjectOnBuildPoint( CBaseObject *pObject );

	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	bool				IsMiniBuilding() { return m_bMiniBuilding; }
	bool				IsDisposableBuilding( void ) const { return m_bDisposableBuilding; }

// ITargetIDProvidesHint
public:
	virtual void		DisplayHintTo( C_BasePlayer *pPlayer );

	virtual void		GetGlowEffectColor( float *r, float *g, float *b );

	bool				IsMapPlaced( void ){ return m_bWasMapPlaced; }

protected:
	virtual void		UpdateDamageEffects( BuildingDamageLevel_t damageLevel ) {}	// default is no effects

	void				UpdateDesiredBuildRotation( float flFrameTime );

	bool				CalculatePlacementPos( void );

protected:

	BuildingDamageLevel_t CalculateDamageLevel( void );

	char			m_szIDString[ MAX_ID_STRING ];

	BuildingDamageLevel_t m_damageLevel;

	Vector m_vecBuildOrigin;
	Vector m_vecBuildCenterOfMass;

	// Upgrading
	int				m_iUpgradeLevel;
	int				m_iOldUpgradeLevel;
	int				m_iUpgradeMetal;
	int				m_iHighestUpgradeLevel;
	int				m_iUpgradeMetalRequired;

	HPARTICLEFFECT	m_hDamageEffects;

private:
	enum
	{
		YAW_PREVIEW_OFF	= 0,
		YAW_PREVIEW_ON,
		YAW_PREVIEW_WAITING_FOR_UPDATE
	};

	Activity		m_Activity;

	int				m_fObjectFlags;
	float			m_fYawPreview;
	char			m_YawPreviewState;
	CHandle< C_TFPlayer > m_hOldOwner;
	CHandle< C_TFPlayer > m_hBuilder;
	bool			m_bWasActive;
	int				m_iOldHealth;
	bool			m_bHasSapper;
	bool			m_bOldSapper;
	int				m_iObjectType;
	int				m_iHealth;
	int				m_iMaxHealth;
	bool			m_bWasBuilding;
	bool			m_bBuilding;
	bool			m_bWasPlacing;
	bool			m_bPlacing;
	bool			m_bDisabled;
	bool			m_bOldDisabled;
	bool			m_bCarried;
	bool			m_bCarryDeploy;
	bool			m_bOldCarryDeploy;
	bool			m_bMiniBuilding;
	bool			m_bDisposableBuilding;
	float			m_flPercentageConstructed;
	EHANDLE			m_hBuiltOnEntity;
	int				m_iObjectMode;
	bool			m_bPlasmaDisable;

	CNetworkVector( m_vecBuildMaxs );
	CNetworkVector( m_vecBuildMins );

	CNetworkVar( int, m_iDesiredBuildRotations );
	float m_flCurrentBuildRotation;

	int m_iLastPlacementPosValid;	// -1 - init, 0 - invalid, 1 - valid

	CNetworkVar( bool, m_bServerOverridePlacement );

	int m_nObjectOldSequence;

	// used when calculating the placement position
	Vector	m_vecBuildForward;
	float	m_flBuildDistance;

	// Stealth
	float			m_flInvisibilityPercent;
	float			m_flPrevInvisibilityPercent;

	CNetworkVar( bool, m_bWasMapPlaced );

private:
	C_BaseObject( const C_BaseObject & ); // not defined, not accessible
};

#endif // C_BASEOBJECT_H
