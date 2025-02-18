//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base Object built by a player
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_H
#define TF_OBJ_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "ihasbuildpoints.h"
#include "iscorer.h"
#include "baseobject_shared.h"
#include "utlmap.h"
#include "props_shared.h"
#include "tf_achievementdata.h"
//#include "tf_player.h"

class CTFPlayer;
class CTFTeam;
class CRopeKeyframe;
class CVGuiScreen;
class KeyValues;
class CTFWrench;
class CTFAmmoPack;
class CObjectSapper;
struct animevent_t;

#define WRENCH_DMG_VS_SAPPER	65
#define OBJ_MAX_UPGRADE_LEVEL	3
#define OBJECT_REPAIR_RATE		10			// Health healed per second while repairing

// Construction
#define OBJECT_CONSTRUCTION_INTERVAL			0.1
#define OBJECT_CONSTRUCTION_STARTINGHEALTH		0.1

extern ConVar object_verbose;
extern ConVar obj_child_range_factor;

#if defined( _DEBUG )
#define TRACE_OBJECT( str )										\
if ( object_verbose.GetInt() )									\
{																\
	Msg( "%s", str );					\
}																
#else
#define TRACE_OBJECT( string )
#endif

#define SF_BASEOBJ_INVULN	(1<<1)
#define LAST_SF_BASEOBJ		SF_BASEOBJ_INVULN

DECLARE_AUTO_LIST( IBaseObjectAutoList );

enum
{
	SHIELD_NONE = 0,
	SHIELD_NORMAL,	// 33% damage taken, no tracking
	SHIELD_MAX,		// 10% damage taken, tracking
};

#define SHIELD_NORMAL_VALUE		0.33f
#define SHIELD_MAX_VALUE		0.10f

// ------------------------------------------------------------------------ //
// Resupply object that's built by the player
// ------------------------------------------------------------------------ //
class CBaseObject : public CBaseCombatCharacter, public IHasBuildPoints, public IScorer, public IBaseObjectAutoList
{
	DECLARE_CLASS( CBaseObject, CBaseCombatCharacter );
public:
	CBaseObject();

	virtual void	UpdateOnRemove( void );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual bool	IsBaseObject( void ) const { return true; }

	virtual void	BaseObjectThink( void );
	//virtual void	LostPowerThink( void );
	virtual CTFPlayer *GetOwner( void );

	// Creation
	virtual void	Precache();
	virtual void	Spawn( void );
	virtual void	FirstSpawn( void );

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual void	SetBuilder( CTFPlayer *pBuilder );
	virtual void	SetType( int iObjectType );
	int				ObjectType( ) const;

	virtual int		BloodColor( void ) { return BLOOD_COLOR_MECH; }

	// Building
	virtual float	GetTotalTime( void );
	virtual void	StartPlacement( CTFPlayer *pPlayer );
	void			StopPlacement( void );
	bool			FindNearestBuildPoint( CBaseEntity *pEntity, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint, bool bIgnoreChecks = false );
	bool			FindBuildPointOnPlayer( CTFPlayer *pTFPlayer, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint );
	bool			VerifyCorner( const Vector &vBottomCenter, float xOffset, float yOffset );
	virtual float	GetNearbyObjectCheckRadius( void ) { return 30.0; }
	bool			UpdatePlacement( void );
	bool			UpdateAttachmentPlacement( CBaseObject *pObjectOverride = NULL );
	bool			IsValidPlacement( void ) const;
	
	virtual bool	IsPlacementPosValid( void );
	bool			FindSnapToBuildPos( CBaseObject *pObjectOverride = NULL );

	void			ReattachChildren( void );
	
	// I've finished building the specified object on the specified build point
	virtual int		FindObjectOnBuildPoint( CBaseObject *pObject );
	
	virtual bool	StartBuilding( CBaseEntity *pPlayer );
	virtual void	SetStartBuildingModel( void ) {}
	void			BuildingThink( void );
	void			SetControlPanelsActive( bool bState );
	virtual void	FinishedBuilding( void );
	bool			IsBuilding( void ) { return m_bBuilding; }
	bool			IsPlacing( void ) { return m_bPlacing; }
	virtual bool	IsUpgrading( void ) const { return false; }
	bool			MustBeBuiltOnAttachmentPoint( void ) const;
	bool			IsCarried( void ) const { return m_bCarried; }
	virtual void	OnEndBeingCarried( CBaseEntity *pBuilder ) {}

	// Returns information about the various control panels
	virtual void 	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual void	GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );

	// Client commands sent by clicking on various panels....
	// NOTE: pArg->Argv(0) == pCmd, pArg->Argv(1) == the first argument
	virtual bool	ClientCommand( CTFPlayer *pSender, const CCommand &args );

	// Damage
	void			SetHealth( float flHealth );
	float			GetHealth() { return m_flHealth; }
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	bool			PassDamageOntoChildren( const CTakeDamageInfo &info, float *flDamageLeftOver );
	virtual bool	Construct( float flHealth );

	void			OnConstructionHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );
	virtual float	GetConstructionMultiplier( void );

	// Destruction
	virtual void	DetonateObject( void );
	virtual bool	ShouldAutoRemove( void );
	virtual void	Explode( void );
	virtual void	Killed( const CTakeDamageInfo &info );
	virtual void	DestroyObject( void );		// Silent cleanup
	virtual bool	IsDying( void ) { return m_bDying; }
	void DestroyScreens( void );

	// Data
	virtual Class_T	Classify( void );
	virtual int		GetType( void ) const;
	virtual CTFPlayer *GetBuilder( void ) const;
	CTFTeam			*GetTFTeam( void ) { return (CTFTeam*)GetTeam(); };
	
	// ID functions
	virtual bool	IsAnUpgrade( void )			{ return false; }
	virtual bool	IsHostileUpgrade( void )	{ return false; }	// Attaches to enemy buildings

	// Inputs
	void			InputSetHealth( inputdata_t &inputdata );
	void			InputAddHealth( inputdata_t &inputdata );
	void			InputRemoveHealth( inputdata_t &inputdata );
	void			InputSetSolidToPlayer( inputdata_t &inputdata );
	void            InputSetBuilder( inputdata_t &inputdata );
	void			InputShow( inputdata_t &inputdata );
	void			InputHide( inputdata_t &inputdata );
	virtual void	InputEnable( inputdata_t &inputdata );
	virtual void	InputDisable( inputdata_t &inputdata );

	// Wrench hits
	virtual bool	InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );
	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc );
	virtual bool	CheckUpgradeOnHit( CTFPlayer *pPlayer );
	virtual int		Command_Repair( CTFPlayer *pActivator, float flAmount, float flRepairMod, float flRepairToMetalRatio = 3.f, bool bSendEvent = true );
	virtual void	DoWrenchHitEffect( Vector hitLoc, bool bRepairHit, bool bUpgradeHit );

	virtual bool	ShouldBeMiniBuilding( CTFPlayer* pPlayer );
	virtual void	MakeMiniBuilding( CTFPlayer* pPlayer );
	virtual void	MakeDisposableBuilding( CTFPlayer *pPlayer );

	virtual void	ChangeTeam( int iTeamNum ) OVERRIDE;			// Assign this entity to a team.

	// Handling object inactive
	virtual bool	ShouldBeActive( void );

	// Sappers
	CObjectSapper*	GetSapper( void );
	bool			HasSapper( void );
	bool			IsPlasmaDisabled( void );
	void			OnAddSapper( void );
	void			OnRemoveSapper( void );

	// Returns the object flags
	int				GetObjectFlags() const { return m_fObjectFlags; }
	void			SetObjectFlags( int flags ) { m_fObjectFlags = flags; }

	virtual void	OnGoActive( void );
	virtual void	OnGoInactive( void );

	// Disabling
	bool			IsDisabled( void ) { return m_bDisabled || m_bCarried; }
	virtual void	UpdateDisabledState( void );
	void			SetDisabled( bool bDisabled );
	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );
	void			SetPlasmaDisabled( float flDuration );

	// Animation
	virtual void	PlayStartupAnimation( void );

	Activity		GetActivity( ) const;
	void			SetActivity( Activity act );
	void			SetObjectSequence( int sequence );
	
	// Object points
	void			SpawnObjectPoints( void );

	// Derive to customize an object's attached version
	virtual	void	SetupAttachedVersion( void ) { return; }

	virtual int		DrawDebugTextOverlays( void );

	void			RotateBuildAngles( void );

	virtual bool	ShouldPlayersAvoid( void );

	void			IncrementKills( void ) { m_iKills++; }
	int				GetKills() { return m_iKills; }

	void			IncrementAssists( void ) { m_iAssists++; }
	int				GetAssists() { return m_iAssists; }

	void			CreateObjectGibs( void );
	CTFAmmoPack*	CreateAmmoPack( const char *pchModel, int nMetal );
	virtual void	SetModel( const char *pModel );

	const char		*GetResponseRulesModifier( void );

	// Map Placed
	virtual void	Activate( void );
	virtual void	InitializeMapPlacedObject( void );

	bool			CannotDie() const { return m_bCannotDie; }
	void			SetCannotDie( bool bCannotDie ) { m_bCannotDie = bCannotDie; }

	// Upgrading
	virtual bool	CanBeUpgraded( CTFPlayer *pPlayer );
	virtual void	StartUpgrading( void );
	virtual void	FinishUpgrading( void );
	void			UpgradeThink( void );
	virtual int		GetUpgradeLevel( void ) const { return m_iUpgradeLevel; }
	int				GetUpgradeMetal( void ) const { return m_iUpgradeMetal; }
	int				GetHighestUpgradeLevel( void ) { return Min( (int)m_iHighestUpgradeLevel, GetMaxUpgradeLevel() ); }
	void			SetHighestUpgradeLevel( int nLevel ) { m_iHighestUpgradeLevel = Min( nLevel, GetMaxUpgradeLevel() ); }
	void			ApplyHealthUpgrade( void );
	virtual int		GetBaseHealth( void ) = 0;
	void			DoQuickBuild( bool bForceMax = false );
	bool			ShouldQuickBuild( void );
	float			GetUpgradeDuration( void );
	void			DoReverseBuild( void );
	float			GetReversesBuildingConstructionSpeed( void );
	virtual int		GetMaxUpgradeLevel( void ) { return OBJ_MAX_UPGRADE_LEVEL; }
	int				GetUpgradeAmountPerHit( void );

	// Carrying
	virtual void	MakeCarriedObject( CTFPlayer *pCarrier );
	virtual void	DropCarriedObject( CTFPlayer* pCarrier );

	void			ForceQuickBuild() { m_bForceQuickBuild = true; }

	virtual int		GetMiniBuildingStartingHealth( void ) { return 100; }

	virtual int		GetMaxHealthForCurrentLevel( void );
	bool			IsUsingReverseBuild( void ){ return ( GetReversesBuildingConstructionSpeed() != 0.0f ); }
	void			ResetPlacement( void );

	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

	// IScorer interface
	virtual CBasePlayer *GetScorer( void ) { return ( CBasePlayer *)GetBuilder(); }
	virtual CBasePlayer *GetAssistant( void ) { return NULL; }

	// Achievement tracking
	// Achievement data storage
	CAchievementData	m_AchievementData;

	// Client/Server shared build point code
	void				CreateBuildPoints( void );
	void				AddAndParseBuildPoint( int iAttachmentNumber, KeyValues *pkvBuildPoint );
	virtual int			AddBuildPoint( int iAttachmentNum );
	virtual void		AddValidObjectToBuildPoint( int iPoint, int iObjectType );
	virtual CBaseObject *GetBuildPointObject( int iPoint );
	bool				IsBuiltOnAttachment( void ) { return (m_hBuiltOnEntity.Get() != NULL); }
	CBaseEntity			*GetBuiltOnObject( void ){ return m_hBuiltOnEntity.Get(); }
	void				AttachObjectToObject( CBaseEntity *pEntity, int iPoint, Vector &vecOrigin );
	virtual void		DetachObjectFromObject( void );
	CBaseObject			*GetParentObject( void );
	CBaseEntity			*GetParentEntity( void );

	virtual void		SetObjectMode( int iVal ) { m_iObjectMode = iVal; }
	int					GetObjectMode( void ) const { return m_iObjectMode; }

	float				GetConstructionStartTime( void ) { return m_flConstructionStartTime; }

// IHasBuildPoints
	virtual int			GetNumBuildPoints( void ) const;
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles );
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const;
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType );
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject );
	virtual float		GetMaxSnapDistance( int iBuildPoint );
	virtual bool		ShouldCheckForMovement( void ) { return true; }
	virtual int			GetNumObjectsOnMe();
	virtual CBaseEntity	*GetFirstFriendlyObjectOnMe( void );
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType );
	virtual void		RemoveAllObjects( void );

// IServerNetworkable.
	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void	SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	
	bool			IsMiniBuilding( void ) const { return m_bMiniBuilding; }
	bool			IsDisposableBuilding( void ) const { return m_bDisposableBuilding; }

	virtual bool ShouldBlockNav() const OVERRIDE { return false; }

	bool			IsMapPlaced( void ){ return m_bWasMapPlaced; }

	virtual int	GetShieldLevel() { return SHIELD_NONE; }

	virtual bool CanBeRepaired() const { return !IsDisposableBuilding(); }

	Vector GetBuildOrigin() { return m_vecBuildOrigin; }
	Vector GetBuildCenterOfMass() { return m_vecBuildCenterOfMass; }
protected:

	virtual bool CanBeUpgraded() const { return !( IsDisposableBuilding() || IsMiniBuilding() ); }
	
	virtual int  GetUpgradeMetalRequired();

	// Show/hide vgui screens.
	bool ShowVGUIScreen( int panelIndex, bool bShow );

	// Spawns the various control panels
	void SpawnControlPanels();

	virtual void DetermineAnimation( void );
	virtual void DeterminePlaybackRate( void );

	void UpdateDesiredBuildRotation( float flFrameTime );

	bool CalculatePlacementPos( void );
	bool EstimateValidBuildPos( void );

private:
	// Purpose: Spawn any objects specified inside the mdl
	void SpawnEntityOnBuildPoint( const char *pEntityName, int iAttachmentNumber );

	//bool TestPositionForPlayerBlock( Vector vecBuildOrigin, CBasePlayer *pPlayer );
	//void RecursiveTestBuildSpace( int iNode, bool *bNodeClear, bool *bNodeVisited );

protected:
	enum OBJSOLIDTYPE
	{
		SOLID_TO_PLAYER_USE_DEFAULT = 0,
		SOLID_TO_PLAYER_YES,
		SOLID_TO_PLAYER_NO,
	};

	bool		IsSolidToPlayers( void ) const;

	// object flags....
	CNetworkVar( int, m_fObjectFlags );
	CNetworkHandle( CTFPlayer,	m_hBuilder );

	// Placement
	Vector			m_vecBuildOrigin;
	Vector			m_vecBuildCenterOfMass;
	CNetworkVector( m_vecBuildMaxs );
	CNetworkVector( m_vecBuildMins );
	CNetworkHandle( CBaseEntity, m_hBuiltOnEntity );
	int				m_iBuiltOnPoint;

	bool	m_bDying;

	// Outputs
	COutputEvent m_OnDestroyed;
	COutputEvent m_OnDamaged;
	COutputEvent m_OnRepaired;

	COutputEvent m_OnBecomingDisabled;
	COutputEvent m_OnBecomingReenabled;

	COutputFloat m_OnObjectHealthChanged;

	// Control panel
	typedef CHandle<CVGuiScreen>	ScreenHandle_t;
	CUtlVector<ScreenHandle_t>	m_hScreens;

	// Map placed objects
	CNetworkVar( bool, m_bWasMapPlaced );

	float			GetCarryDeployTime() { return m_flCarryDeployTime; }

public:
	// Upgrade Level ( 1, 2, 3 )
	CNetworkVar( int, m_iUpgradeLevel );
	CNetworkVar( int, m_iUpgradeMetal );
	CNetworkVar( int, m_iUpgradeMetalRequired );
	CNetworkVar( int, m_iHighestUpgradeLevel );
	int m_nDefaultUpgradeLevel;
	float m_flUpgradeCompleteTime;			// Time when the upgrade animation will complete

private:
	// Make sure we pick up changes to these.
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_takedamage );

	Activity	m_Activity;

	CNetworkVar( int, m_iObjectType );


	// True if players shouldn't do collision avoidance, but should just collide exactly with the object.
	OBJSOLIDTYPE	m_SolidToPlayers;
	void		SetSolidToPlayers( OBJSOLIDTYPE stp, bool force = false );

	CNetworkVar( int , m_iObjectMode );

	// Disabled
	CNetworkVar( bool, m_bDisabled );

	// Building
	CNetworkVar( bool, m_bPlacing );					// True while the object's being placed
	CNetworkVar( bool, m_bBuilding );				// True while the object's still constructing itself

protected:
	float	m_flConstructionTimeLeft;	// Current time left in construction
	float	m_flTotalConstructionTime;	// Total construction time (the value of GetTotalTime() at the time construction 
										// started, ie, incase you teleport out of a construction yard)
	float   m_flConstructionStartTime;

protected:
	// Carried
	CNetworkVar( bool, m_bCarried );
	CNetworkVar( bool, m_bCarryDeploy );

	CNetworkVar( bool, m_bMiniBuilding );
	CNetworkVar( bool, m_bDisposableBuilding );

	float m_flPlasmaDisableTime;
	CNetworkVar( bool, m_bPlasmaDisable );

private:
	int			m_iHealthOnPickup;
	float	m_flCarryDeployTime;

	CNetworkVar( float, m_flPercentageConstructed );	// Used to send to client
	float	m_flHealth;					// Health during construction. Needed a float due to small increases in health.

	// Sapper on me
	CNetworkVar( bool, m_bHasSapper );

	// Build points
	CUtlVector<BuildPoint_t>	m_BuildPoints;

	// Maps player ent index to repair expire time
	struct constructor_t
	{
		float flHitTime; // Time this constructor last hit me
		float flValue;	 // Speed value of this constructor. Defaults to 1.0, but some constructors are worth more.
	};
	CUtlMap<int, constructor_t>	m_ConstructorList;

	// Result of last placement test
	bool		m_bPlacementOK;				// last placement state

	CNetworkVar( int, m_iKills );
	CNetworkVar( int, m_iAssists );

	CNetworkVar( int, m_iDesiredBuildRotations );		// Number of times we've rotated, used to calc final rotation
	float m_flCurrentBuildRotation;

	// Gibs.
	CUtlVector<breakmodel_t>	m_aGibs;

	CNetworkVar( bool, m_bServerOverridePlacement );

	// for ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE
	int		m_iLifetimeDamage;

	bool	m_bCannotDie;
	bool	m_bQuickBuild;

	// used when calculating the placement position
	Vector	m_vecBuildForward;
	float	m_flBuildDistance;

	bool	m_bForceQuickBuild;
};

extern short g_sModelIndexFireball;		// holds the index for the fireball

#endif // TF_OBJ_H
