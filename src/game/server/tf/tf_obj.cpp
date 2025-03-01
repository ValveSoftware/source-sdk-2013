//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base Object built by players
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "tf_weaponbase.h"
#include "rope.h"
#include "rope_shared.h"
#include "bone_setup.h"
#include "ndebugoverlay.h"
#include "rope_helpers.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "tier1/strtools.h"
#include "basegrenade_shared.h"
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "tf_shareddefs.h"
#include "vguiscreen.h"
#include "hierarchy.h"
#include "func_no_build.h"
#include "func_respawnroom.h"
#include <KeyValues.h>
#include "ihasbuildpoints.h"
#include "utldict.h"
#include "filesystem.h"
#include "npcevent.h"
#include "tf_shareddefs.h"
#include "animation.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#include "tf_gamestats.h"
#include "tf_ammo_pack.h"
#include "tf_obj_sapper.h"
#include "particle_parse.h"
#include "tf_fx.h"
#include "trains.h"
#include "serverbenchmark_base.h"
#include "tf_weapon_wrench.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_builder.h"

#include "player_vs_environment/tf_population_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Control panels
#define SCREEN_OVERLAY_MATERIAL "vgui/screens/vgui_overlay"

#define ROPE_HANG_DIST	150
#define UPGRADE_LEVEL_HEALTH_MULTIPLIER 1.2f

#ifdef _DEBUG
//------------------------------------------------------------------------------
// Damage the player's buildings the specified amount
//------------------------------------------------------------------------------
void CC_HurtBuilding_f( const CCommand &args )
{
	if ( !sv_cheats->GetBool() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if ( !pPlayer )
		return;

	int iDamage = 10;
	if ( args.ArgC() >= 2 )
	{
		iDamage = atoi( args[ 1 ] );
	}

	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	if ( pTFPlayer )
	{
		for ( int i = pTFPlayer->GetObjectCount() - 1; i >= 0; i-- )
		{
			CBaseObject *obj = pTFPlayer->GetObject( i );
			Assert( obj );

			if ( obj )
			{
				obj->TakeDamage( CTakeDamageInfo( NULL, NULL, iDamage, DMG_GENERIC ) );
			}
		}
	}
}
static ConCommand hurtbuilding( "hurtbuilding", CC_HurtBuilding_f, "Hurt the player's buildings.\n\tArguments: <health to lose>", FCVAR_CHEAT );
#endif // _DEBUG

ConVar tf_obj_gib_velocity_min( "tf_obj_gib_velocity_min", "100", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_gib_velocity_max( "tf_obj_gib_velocity_max", "450", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_gib_maxspeed( "tf_obj_gib_maxspeed", "800", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_upgrade_per_hit( "tf_obj_upgrade_per_hit", "25", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

ConVar object_verbose( "object_verbose", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Debug object system." );
ConVar obj_damage_factor( "obj_damage_factor","0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Factor applied to all damage done to objects" );
ConVar obj_child_damage_factor( "obj_child_damage_factor","0.25", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Factor applied to damage done to objects that are built on a buildpoint" );
ConVar tf_fastbuild("tf_fastbuild", "0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_obj_ground_clearance( "tf_obj_ground_clearance", "32", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Object corners can be this high above the ground" );

ConVar tf_obj_damage_tank_achievement_amount( "tf_obj_damage_tank_achievement_amount", "2000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

extern short g_sModelIndexFireball;
extern ConVar tf_cheapobjects;

// Minimum distance between 2 objects to ensure player movement between them
#define MINIMUM_OBJECT_SAFE_DISTANCE		100

// Maximum number of a type of objects on a single resource zone
#define MAX_OBJECTS_PER_ZONE			1

// Time it takes a fully healed object to deteriorate
ConVar  object_deterioration_time( "object_deterioration_time", "30", 0, "Time it takes for a fully-healed object to deteriorate." );

// Time after taking damage that an object will still drop resources on death
#define MAX_DROP_TIME_AFTER_DAMAGE			5

#define OBJ_BASE_THINK_CONTEXT				"BaseObjectThink"

#define PLASMA_DISABLE_TIME					4

IMPLEMENT_AUTO_LIST( IBaseObjectAutoList );

BEGIN_DATADESC( CBaseObject )
	// keys 
	DEFINE_KEYFIELD_NOT_SAVED( m_SolidToPlayers,		FIELD_INTEGER, "SolidToPlayer" ),
	DEFINE_KEYFIELD( m_nDefaultUpgradeLevel, FIELD_INTEGER, "defaultupgrade" ),

	DEFINE_THINKFUNC( UpgradeThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetSolidToPlayer", InputSetSolidToPlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "SetBuilder", InputSetBuilder ),
	DEFINE_INPUTFUNC( FIELD_VOID,  "Show", InputShow ),
	DEFINE_INPUTFUNC( FIELD_VOID,  "Hide", InputHide ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	// Outputs
	DEFINE_OUTPUT( m_OnDestroyed, "OnDestroyed" ),
	DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
	DEFINE_OUTPUT( m_OnRepaired, "OnRepaired" ),
	DEFINE_OUTPUT( m_OnBecomingDisabled, "OnDisabled" ),
	DEFINE_OUTPUT( m_OnBecomingReenabled, "OnReenabled" ),
	DEFINE_OUTPUT( m_OnObjectHealthChanged, "OnObjectHealthChanged" )
END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CBaseObject, DT_BaseObject)
	SendPropInt(SENDINFO(m_iHealth), -1, SPROP_VARINT ),
	SendPropInt(SENDINFO(m_iMaxHealth), -1, SPROP_VARINT ),
	SendPropBool(SENDINFO(m_bHasSapper) ),
	SendPropInt(SENDINFO(m_iObjectType), Q_log2( OBJ_LAST ) + 1, SPROP_UNSIGNED ),
	SendPropBool(SENDINFO(m_bBuilding) ),
	SendPropBool(SENDINFO(m_bPlacing) ),
	SendPropBool(SENDINFO(m_bCarried) ),
	SendPropBool(SENDINFO(m_bCarryDeploy) ),
	SendPropBool(SENDINFO(m_bMiniBuilding) ),
	SendPropFloat(SENDINFO(m_flPercentageConstructed), 8, 0, 0.0, 1.0f ),
	SendPropInt(SENDINFO(m_fObjectFlags), OF_BIT_COUNT, SPROP_UNSIGNED ),
	SendPropEHandle(SENDINFO(m_hBuiltOnEntity)),
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropEHandle( SENDINFO( m_hBuilder ) ),
	SendPropVector( SENDINFO( m_vecBuildMaxs ), -1, SPROP_COORD ),
	SendPropVector( SENDINFO( m_vecBuildMins ), -1, SPROP_COORD ),
	SendPropInt( SENDINFO( m_iDesiredBuildRotations ), 2, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bServerOverridePlacement ) ),
	SendPropInt( SENDINFO(m_iUpgradeLevel), 3 ),
	SendPropInt( SENDINFO(m_iUpgradeMetal), 10 ),
	SendPropInt( SENDINFO(m_iUpgradeMetalRequired), 10 ),
	SendPropInt( SENDINFO(m_iHighestUpgradeLevel), 3 ),
	SendPropInt( SENDINFO(m_iObjectMode), 2, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bDisposableBuilding ) ),
	SendPropBool( SENDINFO( m_bWasMapPlaced ) ),
	SendPropBool( SENDINFO( m_bPlasmaDisable ) ),
END_SEND_TABLE();


bool PlayerIndexLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}

// This controls whether ropes attached to objects are transmitted or not. It's important that
// ropes aren't transmitted to guys who don't own them.
class CObjectRopeTransmitProxy : public CBaseTransmitProxy
{
public:
	CObjectRopeTransmitProxy( CBaseEntity *pRope ) : CBaseTransmitProxy( pRope )
	{
	}
	
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo, int nPrevShouldTransmitResult )
	{
		// Don't transmit the rope if it's not even visible.
		if ( !nPrevShouldTransmitResult )
			return FL_EDICT_DONTSEND;

		// This proxy only wants to be active while one of the two objects is being placed.
		// When they're done being placed, the proxy goes away and the rope draws like normal.
		bool bAnyObjectPlacing = (m_hObj1 && m_hObj1->IsPlacing()) || (m_hObj2 && m_hObj2->IsPlacing());
		if ( !bAnyObjectPlacing )
		{
			Release();
			return nPrevShouldTransmitResult;
		}

		// Give control to whichever object is being placed.
		if ( m_hObj1 && m_hObj1->IsPlacing() )
			return m_hObj1->ShouldTransmit( pInfo );
		
		else if ( m_hObj2 && m_hObj2->IsPlacing() )
			return m_hObj2->ShouldTransmit( pInfo );
		
		else
			return FL_EDICT_ALWAYS;
	}


	CHandle<CBaseObject> m_hObj1;
	CHandle<CBaseObject> m_hObj2;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject::CBaseObject()
{
	m_iHealth = m_iMaxHealth = m_flHealth = 0;
	m_flPercentageConstructed = 0;
	m_bPlacing = false;
	m_bBuilding = false;
	m_Activity = ACT_INVALID;
	m_bDisabled = false;
	m_SolidToPlayers = SOLID_TO_PLAYER_USE_DEFAULT;
	m_bPlacementOK = false;
	m_aGibs.Purge();
	m_iHighestUpgradeLevel = 1;
	m_bCarryDeploy = false;
	m_flCarryDeployTime = 0;
	m_iHealthOnPickup = 0;
	m_iLifetimeDamage = 0;
	m_bCannotDie = false;
	m_bMiniBuilding = false;
	m_flPlasmaDisableTime = 0;
	m_bPlasmaDisable = false;

	m_bDisposableBuilding = false;

	m_vecBuildForward = vec3_origin;
	m_flBuildDistance = 0.0f;

	m_bForceQuickBuild = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::UpdateOnRemove( void )
{
	m_bDying = true;

	// check for sapper crits
	CObjectSapper *pSapper = GetSapper();
	if ( pSapper )
	{
		// give an assist to the sapper's owner
		CTFPlayer *pSapperOwner = pSapper->GetOwner();
		if ( pSapperOwner )
		{
			pSapperOwner->m_Shared.IncrementRevengeCrits();
		}
	}

	DestroyObject();

	if ( GetTeam() )
	{
		((CTFTeam*)GetTeam())->RemoveObject( this );
	}

	DetachObjectFromObject();

	// Make sure the object isn't in either team's list of objects...
	//Assert( !GetGlobalTFTeam(1)->IsObjectOnTeam( this ) );
	//Assert( !GetGlobalTFTeam(2)->IsObjectOnTeam( this ) );

	// Chain at end to mimic destructor unwind order
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseObject::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseObject::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// Always transmit to owner
	if ( GetBuilder() && pInfo->m_pClientEnt == GetBuilder()->edict() )
		return FL_EDICT_ALWAYS;

	// Placement models only transmit to owners
	if ( IsPlacing() )
		return FL_EDICT_DONTSEND;

	if ( pInfo->m_pClientEnt )
	{
		CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );
		if ( pRecipientEntity && pRecipientEntity->ShouldForceTransmitsForTeam( GetTeamNumber() ) )
			return FL_EDICT_ALWAYS;
	}

	return BaseClass::ShouldTransmit( pInfo );
}


void CBaseObject::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( entindex() ) )
		return;

	BaseClass::SetTransmit( pInfo, bAlways );

	// Force our screens to be sent too.
	int nTeam = CBaseEntity::Instance( pInfo->m_pClientEnt )->GetTeamNumber();
	for ( int i=0; i < m_hScreens.Count(); i++ )
	{
		CVGuiScreen *pScreen = m_hScreens[i].Get();
		if ( pScreen && pScreen->IsVisibleToTeam( nTeam ) )
			pScreen->SetTransmit( pInfo, bAlways );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::Precache()
{
	PrecacheMaterial( SCREEN_OVERLAY_MATERIAL );

	PrecacheScriptSound( GetObjectInfo( ObjectType() )->m_pExplodeSound );
	PrecacheScriptSound( GetObjectInfo( ObjectType() )->m_pUpgradeSound );

	const char *pEffect = GetObjectInfo( ObjectType() )->m_pExplosionParticleEffect;

	if ( pEffect && pEffect[0] != '\0' )
	{
		PrecacheParticleSystem( pEffect );
	}

	PrecacheParticleSystem( "nutsnbolts_build" );
	PrecacheParticleSystem( "nutsnbolts_upgrade" );
	PrecacheParticleSystem( "nutsnbolts_repair" );

	PrecacheModel( "models/weapons/w_models/w_toolbox.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::Spawn( void )
{
	Precache();

	CollisionProp()->SetSurroundingBoundsType( USE_BEST_COLLISION_BOUNDS );
	SetSolidToPlayers( m_SolidToPlayers, true );

	m_bWasMapPlaced = false;
	m_bHasSapper = false;
	if ( HasSpawnFlags(SF_BASEOBJ_INVULN) )
	{
		m_takedamage = DAMAGE_NO;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}

	AddFlag( FL_OBJECT ); // So NPCs will notice it
	SetViewOffset( WorldSpaceCenter() - GetAbsOrigin() );

	m_iDesiredBuildRotations = 0;
	m_flCurrentBuildRotation = 0;

	if ( MustBeBuiltOnAttachmentPoint() )
	{
		AddEffects( EF_NODRAW );
	}

	// assume valid placement
	m_bServerOverridePlacement = true;

	m_iUpgradeLevel = 1;
	m_iUpgradeMetalRequired = GetUpgradeMetalRequired();

	if ( !IsCarried() )
	{
		FirstSpawn();
	}

	UpdateLastKnownArea();
}

//-----------------------------------------------------------------------------
// Purpose: Initialization that should only be done when the object is first created.
//-----------------------------------------------------------------------------
void CBaseObject::FirstSpawn()
{
	if ( !VPhysicsGetObject() )
		VPhysicsInitStatic();

	m_iUpgradeMetal = 0;
	m_iKills = 0;
	m_iAssists = 0;
	m_ConstructorList.SetLessFunc( PlayerIndexLessFunc );
	m_flHealth = m_iMaxHealth = m_iHealth;

	SetContextThink( &CBaseObject::BaseObjectThink, gpGlobals->curtime + 0.1, OBJ_BASE_THINK_CONTEXT );
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseObject::GetControlPanelInfo( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = NULL;
}

//-----------------------------------------------------------------------------
// Returns information about the various control panels
//-----------------------------------------------------------------------------
void CBaseObject::GetControlPanelClassName( int nPanelIndex, const char *&pPanelName )
{
	pPanelName = "vgui_screen";
}

//-----------------------------------------------------------------------------
// This is called by the base object when it's time to spawn the control panels
//-----------------------------------------------------------------------------
void CBaseObject::SpawnControlPanels()
{
	char buf[64];

	// FIXME: Deal with dynamically resizing control panels?

	// If we're attached to an entity, spawn control panels on it instead of use
	CBaseAnimating *pEntityToSpawnOn = this;
	const char *pOrgLL = "controlpanel%d_ll";
	const char *pOrgUR = "controlpanel%d_ur";
	const char *pAttachmentNameLL = pOrgLL;
	const char *pAttachmentNameUR = pOrgUR;
	if ( IsBuiltOnAttachment() )
	{
		pEntityToSpawnOn = dynamic_cast<CBaseAnimating*>((CBaseEntity*)m_hBuiltOnEntity.Get());
		if ( pEntityToSpawnOn )
		{
			char sBuildPointLL[64];
			char sBuildPointUR[64];
			Q_snprintf( sBuildPointLL, sizeof( sBuildPointLL ), "bp%d_controlpanel%%d_ll", m_iBuiltOnPoint );
			Q_snprintf( sBuildPointUR, sizeof( sBuildPointUR ), "bp%d_controlpanel%%d_ur", m_iBuiltOnPoint );
			pAttachmentNameLL = sBuildPointLL;
			pAttachmentNameUR = sBuildPointUR;
		}
		else
		{
			pEntityToSpawnOn = this;
		}
	}

	Assert( pEntityToSpawnOn );

	// Lookup the attachment point...
	int nPanel;
	for ( nPanel = 0; true; ++nPanel )
	{
		Q_snprintf( buf, sizeof( buf ), pAttachmentNameLL, nPanel );
		int nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nLLAttachmentIndex <= 0)
		{
			// Try and use my panels then
			pEntityToSpawnOn = this;
			Q_snprintf( buf, sizeof( buf ), pOrgLL, nPanel );
			nLLAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nLLAttachmentIndex <= 0)
				return;
		}

		Q_snprintf( buf, sizeof( buf ), pAttachmentNameUR, nPanel );
		int nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
		if (nURAttachmentIndex <= 0)
		{
			// Try and use my panels then
			Q_snprintf( buf, sizeof( buf ), pOrgUR, nPanel );
			nURAttachmentIndex = pEntityToSpawnOn->LookupAttachment(buf);
			if (nURAttachmentIndex <= 0)
				return;
		}

		const char *pScreenName = NULL;
		GetControlPanelInfo( nPanel, pScreenName );
		if (!pScreenName)
			continue;

		const char *pScreenClassname;
		GetControlPanelClassName( nPanel, pScreenClassname );
		if ( !pScreenClassname )
			continue;

		// Compute the screen size from the attachment points...
		matrix3x4_t	panelToWorld;
		pEntityToSpawnOn->GetAttachment( nLLAttachmentIndex, panelToWorld );

		matrix3x4_t	worldToPanel;
		MatrixInvert( panelToWorld, worldToPanel );

		// Now get the lower right position + transform into panel space
		Vector lr, lrlocal;
		pEntityToSpawnOn->GetAttachment( nURAttachmentIndex, panelToWorld );
		MatrixGetColumn( panelToWorld, 3, lr );
		VectorTransform( lr, worldToPanel, lrlocal );

		float flWidth = lrlocal.x;
		float flHeight = lrlocal.y;

		CVGuiScreen *pScreen = CreateVGuiScreen( pScreenClassname, pScreenName, pEntityToSpawnOn, this, nLLAttachmentIndex );
		pScreen->ChangeTeam( GetTeamNumber() );
		pScreen->SetActualSize( flWidth, flHeight );
		pScreen->SetActive( false );
		pScreen->MakeVisibleOnlyToTeammates( true );
		pScreen->SetOverlayMaterial( SCREEN_OVERLAY_MATERIAL );
		pScreen->SetTransparency( true );

		// for now, only input by the owning player
		pScreen->SetPlayerOwner( GetBuilder(), true );

		int nScreen = m_hScreens.AddToTail( );
		m_hScreens[nScreen].Set( pScreen );
	}
}

//-----------------------------------------------------------------------------
// Handle commands sent from vgui panels on the client 
//-----------------------------------------------------------------------------
bool CBaseObject::ClientCommand( CTFPlayer *pSender, const CCommand &args )
{
	//const char *pCmd = args[0];
	return false;
}

#define BASE_OBJECT_THINK_DELAY	0.1
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::BaseObjectThink( void )
{
	SetNextThink( gpGlobals->curtime + BASE_OBJECT_THINK_DELAY, OBJ_BASE_THINK_CONTEXT );

	// Make sure animation is up to date
	DetermineAnimation();

	DeterminePlaybackRate();

	if ( m_bPlasmaDisable )
	{
		if ( gpGlobals->curtime > (m_flPlasmaDisableTime ) )
		{
			m_bPlasmaDisable = false;
			UpdateDisabledState();
		}
	}

	// Do nothing while we're being placed
	if ( IsPlacing() )
	{
		if ( MustBeBuiltOnAttachmentPoint() )
		{
			UpdateAttachmentPlacement();	
			m_bServerOverridePlacement = true;
		}
		else
		{
			m_bServerOverridePlacement = IsPlacementPosValid();

			UpdateDesiredBuildRotation( BASE_OBJECT_THINK_DELAY );
		}

		return;
	}

	// If we're building, keep going
	if ( IsBuilding() )
	{
		BuildingThink();
		return;
	}

	if ( IsUpgrading() )
	{
		UpgradeThink();
	}
	else
	{
		if ( GetReversesBuildingConstructionSpeed() > 0.0f )
		{
			DoReverseBuild();
		}
		else
		{
			if ( GetUpgradeLevel() < GetHighestUpgradeLevel() )
			{
				// Keep moving up levels until we reach the level we were at before.
				StartUpgrading();
			}
			else
			{
				m_bCarryDeploy = false;
			}
		}
	}
}

void CBaseObject::ResetPlacement( void )
{
	m_bPlacementOK = false;

	// Clear out previous parent 
	if ( m_hBuiltOnEntity.Get() )
	{
		m_hBuiltOnEntity = NULL;
		m_iBuiltOnPoint = 0;
		SetParent( NULL );
	}

	// teleport to builder's origin
	CTFPlayer *pPlayer = GetOwner();

	if ( pPlayer )
	{
		Teleport( &pPlayer->WorldSpaceCenter(), &GetLocalAngles(), NULL );
	}
}

bool CBaseObject::UpdateAttachmentPlacement( CBaseObject *pObjectOverride )
{
	// See if we should snap to a build position
	// finding one implies it is a valid position
	if ( FindSnapToBuildPos( pObjectOverride ) )
	{
		m_bPlacementOK = true;

		Teleport( &m_vecBuildOrigin, &GetLocalAngles(), NULL );
	}
	else
	{
		ResetPlacement();
	}

	return m_bPlacementOK;
}

//-----------------------------------------------------------------------------
// Purpose: Cheap check to see if we are in any server-defined No-build areas.
//-----------------------------------------------------------------------------
bool CBaseObject::EstimateValidBuildPos( void )
{
	// Make sure CalculatePlacementPos() has been called to setup the member variables used below

	CTFPlayer *pPlayer = GetOwner();

	if ( !pPlayer )
		return false;

	// Cannot build inside a nobuild brush
	if ( PointInNoBuild( m_vecBuildOrigin, this ) )
		return false;

	if ( PointInNoBuild( m_vecBuildCenterOfMass, this ) )
		return false;

	// If we're receiving trigger hurt damage, don't allow building here.
	if ( IsTakingTriggerHurtDamageAtPoint( m_vecBuildOrigin ) )
		return false;

	if ( IsTakingTriggerHurtDamageAtPoint( m_vecBuildCenterOfMass ) )
		return false;

	if ( PointInRespawnRoom( NULL, m_vecBuildOrigin ) && !g_pServerBenchmark->IsBenchmarkRunning() )
		return false;

	if ( PointInRespawnRoom( NULL, m_vecBuildCenterOfMass ) && !g_pServerBenchmark->IsBenchmarkRunning() )
		return false;

	Vector vecBuildFarEdge = m_vecBuildOrigin + m_vecBuildForward * ( m_flBuildDistance + 8.0f );
	if ( PointsCrossRespawnRoomVisualizer( pPlayer->WorldSpaceCenter(), vecBuildFarEdge ) )
		return false;

	return true;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::DeterminePlaybackRate( void )
{
	float flReverseBuildingConstructionSpeed = GetReversesBuildingConstructionSpeed();
	if ( flReverseBuildingConstructionSpeed == 0.0f )
	{
		flReverseBuildingConstructionSpeed = 1.0f;
	}
	else
	{
		flReverseBuildingConstructionSpeed *= -1.0f;
	}

	// If a sapper was added or removed part way through construction we need to invert the time to completion
	bool bAdjustCompleteTime = ( flReverseBuildingConstructionSpeed > 0.0f && GetPlaybackRate() < 0.0f ) || 
							   ( flReverseBuildingConstructionSpeed < 0.0f && GetPlaybackRate() >= 0.0f );

	if ( IsBuilding() )
	{
		// Default half rate, author build anim as if one player is building
		// ConstructionMultiplier already contains the reverse
		SetPlaybackRate( GetConstructionMultiplier() * 0.5 );	
	}
	else
	{
		SetPlaybackRate( 1.0 * flReverseBuildingConstructionSpeed );
	}

	if ( bAdjustCompleteTime )
	{
		float fRelativeCycle = ( ( flReverseBuildingConstructionSpeed > 0.0f ) ? ( 1.0f - GetCycle() ) : ( GetCycle() ) );

		float flUpgradeTime = GetUpgradeDuration();
		flUpgradeTime /= ( ( flReverseBuildingConstructionSpeed < 0.0f ) ? ( flReverseBuildingConstructionSpeed * -1.0 ) : 1.0f );
		m_flUpgradeCompleteTime = gpGlobals->curtime + flUpgradeTime * fRelativeCycle;
		m_flTotalConstructionTime = m_flConstructionTimeLeft = GetTotalTime();
		
		float flNewConstructionTimeLeft = m_flConstructionTimeLeft * fRelativeCycle;
		m_flConstructionTimeLeft *= fRelativeCycle;

		m_flConstructionStartTime += m_flConstructionTimeLeft - flNewConstructionTimeLeft;
		m_flConstructionTimeLeft = flNewConstructionTimeLeft;
	}

	if ( !(m_fObjectFlags & OF_DOESNT_HAVE_A_MODEL) )
	{
		StudioFrameAdvance();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayer *CBaseObject::GetOwner()
{ 
	return m_hBuilder; 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::SetBuilder( CTFPlayer *pBuilder )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseObject::SetBuilder builder %s\n", gpGlobals->curtime, 
		pBuilder ? pBuilder->GetPlayerName() : "NULL" ) );

	m_hBuilder = pBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CBaseObject::ObjectType( ) const
{
	return m_iObjectType;
}

//-----------------------------------------------------------------------------
// Purpose: Destroys the object, gives a chance to spawn an explosion
//-----------------------------------------------------------------------------
void CBaseObject::DetonateObject( void )
{
	// Blow us up.
	CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), 0, DMG_GENERIC );
	Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Remove this object from it's team and mark for deletion
//-----------------------------------------------------------------------------
void CBaseObject::DestroyObject( void )
{
	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseObject::DestroyObject %p:%s\n", gpGlobals->curtime, this, GetClassname() ) );

	// If we are carried, uncarry us before destruction.
	if ( IsCarried() && GetBuilder() )
	{
		DropCarriedObject( GetBuilder() );

		CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder*>( GetBuilder()->Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );
		if ( pBuilder )
		{
			pBuilder->SwitchOwnersWeaponToLast();
		}
	}

	if ( GetBuilder() )
	{
		GetBuilder()->OwnedObjectDestroyed( this );
	}

	UTIL_Remove( this );

	DestroyScreens();
}

//-----------------------------------------------------------------------------
// Purpose: Remove any screens that are active on this object
//-----------------------------------------------------------------------------
void CBaseObject::DestroyScreens( void )
{
	// Kill the control panels
	int i;
	for ( i = m_hScreens.Count(); --i >= 0; )
	{
		DestroyVGuiScreen( m_hScreens[i].Get() );
	}
	m_hScreens.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Get the total time it will take to build this object
//-----------------------------------------------------------------------------
float CBaseObject::GetTotalTime( void )
{
	float flBuildTime = GetObjectInfo( ObjectType() )->m_flBuildTime;

	CTFPlayer *pTFBuilder = GetBuilder();
	if ( pTFBuilder )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFBuilder, flBuildTime, mod_build_rate );
	}

	if ( tf_fastbuild.GetInt() )
		return Min( flBuildTime, 2.f );

	if ( TFGameRules()->IsQuickBuildTime() )
		return Min( flBuildTime, 1.f );

	return flBuildTime;
}

//-----------------------------------------------------------------------------
// Purpose: Start placing the object
//-----------------------------------------------------------------------------
void CBaseObject::StartPlacement( CTFPlayer *pPlayer )
{
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_bPlacing = true;
	m_bBuilding = false;
	if ( pPlayer )
	{
		SetBuilder( pPlayer );
		ChangeTeam( pPlayer->GetTeamNumber() );
	}

	// needed?
	m_nRenderMode = kRenderNormal;
	
	// Set my build size
	CollisionProp()->WorldSpaceAABB( &m_vecBuildMins.GetForModify(), &m_vecBuildMaxs.GetForModify() );
	m_vecBuildMins -= Vector( 4,4,0 );
	m_vecBuildMaxs += Vector( 4,4,0 );
	m_vecBuildMins -= GetAbsOrigin();
	m_vecBuildMaxs -= GetAbsOrigin();

	// Set the skin
	m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: Stop placing the object
//-----------------------------------------------------------------------------
void CBaseObject::StopPlacement( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Find the nearest buildpoint on the specified entity
//-----------------------------------------------------------------------------
bool CBaseObject::FindNearestBuildPoint( CBaseEntity *pEntity, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint, bool bIgnoreChecks )
{
	bool bFoundPoint = false;

	IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>(pEntity);
	Assert( pBPInterface );

	// Any empty buildpoints?
	for ( int i = 0; i < pBPInterface->GetNumBuildPoints(); i++ )
	{
		// Can this object build on this point?
		if ( pBPInterface->CanBuildObjectOnBuildPoint( i, GetType() ) )
		{
			// Close to this point?
			Vector vecBPOrigin;
			QAngle vecBPAngles;
			if ( pBPInterface->GetBuildPoint(i, vecBPOrigin, vecBPAngles) )
			{
				if ( !bIgnoreChecks )
				{
					// ignore build points outside our view
					if ( !pBuilder->FInViewCone( vecBPOrigin ) )
						continue;

					// Do a trace to make sure we don't place attachments through things (players, world, etc...)
					Vector vecStart = pBuilder->EyePosition();
					trace_t trace;
					CTraceFilterNoNPCsOrPlayer ignorePlayersFilter( pBuilder, COLLISION_GROUP_NONE );
					UTIL_TraceLine( vecStart, vecBPOrigin, MASK_SOLID, &ignorePlayersFilter, &trace );
					if ( trace.m_pEnt != pEntity && trace.fraction != 1.0 )
						continue;
				}

				float flDist = (vecBPOrigin - pBuilder->GetAbsOrigin()).Length();

				// if this is closer, or is the first one in our view, check it out
				if ( bIgnoreChecks || ( flDist < Min( flNearestPoint, pBPInterface->GetMaxSnapDistance( i ) ) ) )
				{
					flNearestPoint = flDist;
					vecNearestBuildPoint = vecBPOrigin;
					m_hBuiltOnEntity = pEntity;
					m_iBuiltOnPoint = i;

					// Set our angles to the buildpoint's angles
					SetAbsAngles( vecBPAngles );

					bFoundPoint = true;
				}
			}
		}
	}

	return bFoundPoint;
}

//-----------------------------------------------------------------------------
// Purpose: Find a buildpoint on the specified player
//-----------------------------------------------------------------------------
bool CBaseObject::FindBuildPointOnPlayer( CTFPlayer *pTFPlayer, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint )
{
	bool bFoundPoint = false;

	if ( !pTFPlayer )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_SAPPED ) ) 
		return false;

	if ( pTFPlayer->m_Shared.IsInvulnerable() )
		return false;

	if ( pTFPlayer->m_Shared.InCond( TF_COND_PHASE ) )
		return false;

	Vector vecOrigin = pTFPlayer->GetAbsOrigin();
	QAngle vecAngles = pTFPlayer->GetAbsAngles();
	float flDist = ( vecOrigin - pBuilder->GetAbsOrigin() ).Length();
	if ( flDist <= 160.f )
	{
		flNearestPoint = flDist;
		vecNearestBuildPoint = vecOrigin;
		m_hBuiltOnEntity = (CBaseEntity *)pTFPlayer;

		// Set our angles to the buildpoint's angles
		SetAbsAngles( vecAngles );

		bFoundPoint = true;
	}

	return bFoundPoint;
}

/*
class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsPlayer() )
		{
			return false;
		}

		return true;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Test around this build position to make sure it does not block a path
//-----------------------------------------------------------------------------
bool CBaseObject::TestPositionForPlayerBlock( Vector vecBuildOrigin, CBasePlayer *pPlayer )
{
	// find out the status of the 8 regions around this position
	int i;
	bool bNodeVisited[8];
	bool bNodeClear[8];

	// The first zone that is clear of obstructions
	int iFirstClear = -1;

	Vector vHalfPlayerDims = (VEC_HULL_MAX - VEC_HULL_MIN) * 0.5f;

	Vector vBuildDims = m_vecBuildMaxs - m_vecBuildMins;
	Vector vHalfBuildDims = vBuildDims * 0.5;

	 
	// the locations of the 8 test positions
	// boxes are adjacent to the object box and are at least as large as 
	// a player to ensure that a player can pass this location

	//	0  1  2
	//	7  X  3
	//	6  5  4

	static int iPositions[8][2] = 
	{
		{ -1, -1 },
		{ 0, -1 },
		{ 1, -1 },
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ -1, 1 },
		{ -1, 0 }
	};

	CTraceFilterIgnorePlayers traceFilter( this, COLLISION_GROUP_NONE );

	for ( i=0;i<8;i++ )
	{
		// mark them all as unvisited
		bNodeVisited[i] = false;

		Vector vecTest = vecBuildOrigin;		
		vecTest.x += ( iPositions[i][0] * ( vHalfBuildDims.x + vHalfPlayerDims.x ) );
		vecTest.y += ( iPositions[i][1] * ( vHalfBuildDims.y + vHalfPlayerDims.y ) );

		trace_t trace;
		UTIL_TraceHull( vecTest, vecTest, VEC_HULL_MIN, VEC_HULL_MAX, MASK_SOLID_BRUSHONLY, &traceFilter, &trace );

		bNodeClear[i] = ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );

		// NDebugOverlay::Box( vecTest, VEC_HULL_MIN, VEC_HULL_MAX, bNodeClear[i] ? 0 : 255, bNodeClear[i] ? 255 : 0, 0, 20, 0.1 );

		// Store off the first clear location
		if ( iFirstClear < 0 && bNodeClear[i] )
		{
			iFirstClear = i;
		}
	}

	if ( iFirstClear < 0 )
	{
		// no clear space
		return false;
	}

	// visit all nodes that are adjacent
	RecursiveTestBuildSpace( iFirstClear, bNodeClear, bNodeVisited );

	// if we still have unvisited nodes, return false
	// unvisited nodes means that one or more nodes was unreachable from our start position
	// ie, two places the player might want to traverse but would not be able to if we built here
	for ( i=0;i<8;i++ )
	{
		if ( bNodeVisited[i] == false && bNodeClear[i] == true )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Test around the build position, one quadrant at a time
//-----------------------------------------------------------------------------
void CBaseObject::RecursiveTestBuildSpace( int iNode, bool *bNodeClear, bool *bNodeVisited )
{
	// if the node is visited already
	if ( bNodeVisited[iNode] == true )
		return;

	// if the test node is blocked
	if ( bNodeClear[iNode] == false )
		return;

	bNodeVisited[iNode] = true;

	int iLeftNode = iNode - 1;
	if ( iLeftNode < 0 )
		iLeftNode = 7;

	RecursiveTestBuildSpace( iLeftNode, bNodeClear, bNodeVisited );

	int iRightNode = ( iNode + 1 ) % 8;

	RecursiveTestBuildSpace( iRightNode, bNodeClear, bNodeVisited );
}
*/

//-----------------------------------------------------------------------------
// Purpose: Move the placement model to the current position. Return false if it's an invalid position
//-----------------------------------------------------------------------------
bool CBaseObject::UpdatePlacement( void )
{
	if ( MustBeBuiltOnAttachmentPoint() )
	{
		return UpdateAttachmentPlacement();
	}
	
	// Finds bsp-valid place for building to be built
	// Checks for validity, nearby to other entities, in line of sight
	m_bPlacementOK = IsPlacementPosValid();

	Teleport( &m_vecBuildOrigin, &GetLocalAngles(), NULL );

	return m_bPlacementOK;
}

//-----------------------------------------------------------------------------
// Purpose: See if we should be snapping to a build position
//-----------------------------------------------------------------------------
bool CBaseObject::FindSnapToBuildPos( CBaseObject *pObjectOverride )
{
	if ( !MustBeBuiltOnAttachmentPoint() )
		return false;

	CTFPlayer *pPlayer = GetOwner();

	if ( !pPlayer )
	{
		return false;
	}

	bool bSnappedToPoint = false;
	bool bShouldAttachToParent = false;

	Vector vecNearestBuildPoint = vec3_origin;

	// See if there are any nearby build positions to snap to
	float flNearestPoint = 9999;
	int i;

	bool bHostileAttachment = IsHostileUpgrade();
	int iMyTeam = GetTeamNumber();

	if ( !pObjectOverride )
	{
		int nTeamCount = TFTeamMgr()->GetTeamCount();
		for ( int iTeam = FIRST_GAME_TEAM; iTeam < nTeamCount; ++iTeam )
		{
			// Hostile attachments look for enemy objects only
			if ( bHostileAttachment ) 
			{
				if ( iTeam == iMyTeam )
				{
					continue;
				}
			}
			// Friendly attachments look for friendly objects only
			else if ( iTeam != iMyTeam )
			{
				continue;
			}

			CTFTeam *pTeam = ( CTFTeam * )GetGlobalTeam( iTeam );
			if ( !pTeam )
				continue;
			
			// See if we're allowed to build on Robots
			if ( TFGameRules() && TFGameRules()->GameModeUsesMiniBosses() && 
				 GetType() == OBJ_ATTACHMENT_SAPPER && !pPlayer->IsBot() )
			{
				CUtlVector< CTFPlayer * > playerVector;
				CollectPlayers( &playerVector, pPlayer->GetOpposingTFTeam()->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );
				FOR_EACH_VEC( playerVector, i )
				{
					if ( !playerVector[i]->IsBot() )
						continue;

					if ( FindBuildPointOnPlayer( playerVector[i], pPlayer, flNearestPoint, vecNearestBuildPoint ) )
					{
						bSnappedToPoint = true;
						bShouldAttachToParent = true;
					}
				}
			}

			// look for nearby buildpoints on other objects
			for ( i = 0; i < pTeam->GetNumObjects(); i++ )
			{
				CBaseObject *pObject = pTeam->GetObject(i);
				Assert( pObject );
				if ( pObject && !pObject->IsPlacing() )
				{
					if ( FindNearestBuildPoint( pObject, pPlayer, flNearestPoint, vecNearestBuildPoint ) )
					{
						bSnappedToPoint = true;
						bShouldAttachToParent = true;
					}
				}
			}
		}	
	}
	else
	{
		if ( !pObjectOverride->IsPlacing() )
		{
			if ( FindNearestBuildPoint( pObjectOverride, pPlayer, flNearestPoint, vecNearestBuildPoint, true ) )
			{
				bSnappedToPoint = true;
				bShouldAttachToParent = true;
			}
		}
	}

	if ( !bSnappedToPoint )
	{
		AddEffects( EF_NODRAW );
	}
	else
	{
		RemoveEffects( EF_NODRAW );

		if ( bShouldAttachToParent )
		{
			AttachObjectToObject( m_hBuiltOnEntity.Get(), m_iBuiltOnPoint, vecNearestBuildPoint );
		}

		m_vecBuildOrigin = vecNearestBuildPoint;
	}

	return bSnappedToPoint;
}

//-----------------------------------------------------------------------------
// Purpose: Are we currently in a buildable position
//-----------------------------------------------------------------------------
bool CBaseObject::IsValidPlacement( void ) const
{
	return m_bPlacementOK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CBaseObject::GetResponseRulesModifier( void )
{
	switch ( GetType() )
	{
	case OBJ_DISPENSER: return "objtype:dispenser"; break;
	case OBJ_TELEPORTER: return "objtype:teleporter"; break;
	case OBJ_SENTRYGUN: return "objtype:sentrygun"; break;
	case OBJ_ATTACHMENT_SAPPER: return "objtype:sapper"; break;
	default:
		break;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CBaseObject::StartBuilding( CBaseEntity *pBuilder )
{
	// Need to add the object to the team now...
	CTFTeam *pTFTeam = ( CTFTeam * )GetGlobalTeam( GetTeamNumber() );

	// Deduct the cost from the player
	if ( pBuilder && pBuilder->IsPlayer() )
	{
		/*
		if ( ((CTFPlayer*)pBuilder)->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			((CTFPlayer*)pBuilder)->HintMessage( HINT_ENGINEER_USE_WRENCH_ONOWN );
		}
		*/

		if ( !IsCarried() )
		{
			if ( !ShouldQuickBuild() )
			{
				int iAmountPlayerPaidForMe = ((CTFPlayer*)pBuilder)->StartedBuildingObject( m_iObjectType );
				if ( !iAmountPlayerPaidForMe )
				{
					// Player couldn't afford to pay for me, so abort
					ClientPrint( (CBasePlayer*)pBuilder, HUD_PRINTCENTER, "#TF_Not_Enough_Resources" );
					StopPlacement();
					return false;
				}
			}

			((CTFPlayer*)pBuilder)->SpeakConceptIfAllowed( MP_CONCEPT_BUILDING_OBJECT, GetResponseRulesModifier() );
		}
		else
		{
			m_bCarried = false;
			m_bCarryDeploy = true;
			m_flCarryDeployTime = gpGlobals->curtime;
			SetActivity( ACT_OBJ_ASSEMBLING );

			((CTFPlayer*)pBuilder)->m_flCommentOnCarrying = 0.f;
			((CTFPlayer*)pBuilder)->SpeakConceptIfAllowed( MP_CONCEPT_REDEPLOY_BUILDING, GetResponseRulesModifier() );
		}
	}
	
	// Check to see if we need to add this to a hierarchy.  We can just do a simple ray trace from the center as
	// the placement code has guarenteed we are in a valid position.
	trace_t trace;
	UTIL_TraceHull( GetAbsOrigin() + Vector( 0.0f, 0.0f, 2.0f ), GetAbsOrigin() - Vector( 0.0f, 0.0f, 2.0f ), vec3_origin, vec3_origin, MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
	if ( trace.m_pEnt && trace.m_pEnt->IsBSPModel() )
	{
		CFuncTrackTrain *pTrain = dynamic_cast<CFuncTrackTrain*>( trace.m_pEnt );
		if ( pTrain )
		{
			SetParent( pTrain );
		}
	}

	// Add this object to the team's list (because we couldn't add it during
	// placement mode)
	if ( pTFTeam && !pTFTeam->IsObjectOnTeam( this ) )
	{
		pTFTeam->AddObject( this );
	}

	m_bPlacing = false;
	m_bBuilding = true;
	if ( m_bCarryDeploy )
	{
		SetHealth( m_iHealthOnPickup );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_dropobject" );
		if ( event )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pBuilder );
			event->SetInt( "userid", pTFPlayer ? pTFPlayer->GetUserID() : 0 );
			event->SetInt( "object", GetType() );
			event->SetInt( "index", entindex() );	// object entity index

			gameeventmanager->FireEvent( event, true );	// don't send to clients
		}
	}
	else if ( IsMiniBuilding() )
	{
		int iHealth = GetMaxHealthForCurrentLevel();
		if ( !IsDisposableBuilding() )
		{
			iHealth /= 2.0f;
		}
		SetHealth( iHealth );
	}
	else
	{
		SetHealth( OBJECT_CONSTRUCTION_STARTINGHEALTH );
	}
	m_flPercentageConstructed = 0;

	m_nRenderMode = kRenderNormal; 
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	// NOTE: We must spawn the control panels now, instead of during
	// Spawn, because until placement is started, we don't actually know
	// the position of the control panel because we don't know what it's
	// been attached to (could be a vehicle which supplies a different
	// place for the control panel)
	// NOTE: We must also spawn it before FinishedBuilding can be called
	if ( !(m_fObjectFlags & OF_DOESNT_HAVE_A_MODEL) )
	{
		SpawnControlPanels();
	}

	// Tell the object we've been built on that we exist
	if ( IsBuiltOnAttachment() && !m_hBuiltOnEntity->IsPlayer() )
	{
		IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>((CBaseEntity*)m_hBuiltOnEntity.Get());
		Assert( pBPInterface );
		pBPInterface->SetObjectOnBuildPoint( m_iBuiltOnPoint, this );
	}

	// Start the build animations
	m_flTotalConstructionTime = m_flConstructionTimeLeft = GetTotalTime();
	m_flConstructionStartTime = gpGlobals->curtime;

	if ( TFGameRules() && TFGameRules()->IsPowerupMode() && m_bCarryDeploy )
	{
		m_flTotalConstructionTime = 1.0f;
	}

	if ( pBuilder && pBuilder->IsPlayer() )
	{
		CTFPlayer *pTFBuilder = ToTFPlayer( pBuilder );
		pTFBuilder->FinishedObject( this );
		IGameEvent * event = gameeventmanager->CreateEvent( "player_builtobject" );
		if ( event )
		{
			event->SetInt( "userid", pTFBuilder->GetUserID() );
			event->SetInt( "object", ObjectType() );
			event->SetInt( "index", entindex() );	// object entity index
			gameeventmanager->FireEvent( event, true );	// don't send to clients
		}

		CSingleUserRecipientFilter user( pTFBuilder );
		user.MakeReliable();
		UserMessageBegin( user, "BuiltObject" );
			WRITE_BYTE( ObjectType() );
			WRITE_BYTE( GetObjectMode() );
			WRITE_BYTE( entindex() );
		MessageEnd();
	}

	m_vecBuildOrigin = GetAbsOrigin();

	int contents = UTIL_PointContents( m_vecBuildOrigin );
	if ( contents & MASK_WATER )
	{
		SetWaterLevel( 3 );
	}

	// instantly play the build anim
	DetermineAnimation();

	if ( IsMiniBuilding() && ( GetType() != OBJ_DISPENSER ) )
	{
		// Set the skin after placement mode.
		m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 2 : 3;
	}

	if ( ShouldQuickBuild() )
	{
		DoQuickBuild();
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldBeMiniBuilding( CTFPlayer* pPlayer )
{
	if ( !pPlayer )
		return false;

	CTFWrench* pWrench = dynamic_cast<CTFWrench*>( pPlayer->Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
	if ( !pWrench )
		return false;

	if ( TFGameRules()->GameModeUsesUpgrades() )
	{
		if ( pPlayer->GetNumObjects( OBJ_SENTRYGUN ) && pPlayer->CanBuild( OBJ_SENTRYGUN ) == CB_CAN_BUILD && !IsCarried() )
			return true;	
	}

	if ( !pWrench->IsPDQ() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseObject::MakeMiniBuilding( CTFPlayer* pPlayer )
{
	if ( !ShouldBeMiniBuilding( pPlayer ) || IsMiniBuilding() )
		return;

	m_bMiniBuilding = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseObject::MakeDisposableBuilding( CTFPlayer *pPlayer )
{
	m_bDisposableBuilding = true;
}

//-----------------------------------------------------------------------------
// Purpose: Continue construction of this object
//-----------------------------------------------------------------------------
void CBaseObject::BuildingThink( void )
{
	// Continue construction
	Construct( (GetMaxHealth() - OBJECT_CONSTRUCTION_STARTINGHEALTH) / m_flTotalConstructionTime * OBJECT_CONSTRUCTION_INTERVAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::SetControlPanelsActive( bool bState )
{
	// Activate control panel screens
	for ( int i = m_hScreens.Count(); --i >= 0; )
	{
		if (m_hScreens[i].Get())
		{
			m_hScreens[i]->SetActive( bState );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::FinishedBuilding( void )
{
	SetControlPanelsActive( true );

	// Only make a shadow if the object doesn't use vphysics
	if (!VPhysicsGetObject())
	{
		VPhysicsInitStatic();
	}

	m_bBuilding = false;

	OnGoActive();

	// We're done building, add in the stat...
	////TFStats()->IncrementStat( (TFStatId_t)(TF_STAT_FIRST_OBJECT_BUILT + ObjectType()), 1 );

	// Spawn any objects on this one
	SpawnObjectPoints();

	if ( IsUsingReverseBuild() )
	{
		// if we don't have a sapper (but we should!) then set ourselves as the damager
		CObjectSapper *pSapper = GetSapper();
		CBaseEntity *pDamager = pSapper ? pSapper : this;
		int iCustomDamageType = pSapper ? TF_DMG_CUSTOM_SAPPER_RECORDER_DEATH : 0;

		CTakeDamageInfo info;
		info.SetInflictor( pDamager );
		info.SetAttacker( pDamager );
		info.SetDamageForce( vec3_origin );
		info.SetDamagePosition( GetAbsOrigin() );
		info.SetDamage( 0 );
		info.SetDamageType( DMG_CRUSH );
		info.SetDamageCustom( iCustomDamageType );
		Killed( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Objects store health in hacky ways
//-----------------------------------------------------------------------------
void CBaseObject::SetHealth( float flHealth )
{
	if ( m_bCarryDeploy && (flHealth>m_iHealthOnPickup) )
	{
		// If we are re-deploying after being carried we shouldn't gain more health than we had
		// on pickup until the deploy process is finished.
		flHealth = m_iHealthOnPickup;
	}

	bool changed = m_flHealth != flHealth;

	m_flHealth = flHealth;
	m_iHealth = ceil(m_flHealth);


	/*
	// If we a pose parameter, set the pose parameter to reflect our health
	if ( LookupPoseParameter( "object_health") >= 0 && GetMaxHealth() > 0 )
	{
		SetPoseParameter( "object_health", 100 * ( GetHealth() / (float)GetMaxHealth() ) );
	}
	*/

	if ( changed )
	{
		// Set value and fire output
		m_OnObjectHealthChanged.Set( m_flHealth, this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override base traceattack to prevent visible effects from team members shooting me
//-----------------------------------------------------------------------------
void CBaseObject::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Prevent team damage here so blood doesn't appear
	if ( inputInfo.GetAttacker() )
	{
		if ( InSameTeam(inputInfo.GetAttacker()) )
		{
			// Pass Damage to enemy attachments
			int iNumObjects = GetNumObjectsOnMe();
			for ( int iPoint=iNumObjects-1;iPoint >= 0; --iPoint )
			{
				CBaseObject *pObject = GetBuildPointObject( iPoint );

				if ( pObject && pObject->IsHostileUpgrade() )
				{
					pObject->TraceAttack(inputInfo, vecDir, ptr, pAccumulator );
				}
			}
			return;
		}
	}

	SpawnBlood( ptr->endpos, vecDir, BloodColor(), inputInfo.GetDamage() );

	AddMultiDamage( inputInfo, this );
}

//-----------------------------------------------------------------------------
// Purpose: Prevent Team Damage
//-----------------------------------------------------------------------------
ConVar object_show_damage( "obj_show_damage", "0", 0, "Show all damage taken by objects." );
ConVar object_capture_damage( "obj_capture_damage", "0", 0, "Captures all damage taken by objects for dumping later." );

CUtlDict<int,int> g_DamageMap;

void Cmd_DamageDump_f(void)
{	
	CUtlDict<bool,int> g_UniqueColumns;
	int idx;

	// Build the unique columns:
	for( idx = g_DamageMap.First(); idx != g_DamageMap.InvalidIndex(); idx = g_DamageMap.Next(idx) )
	{
		char* szColumnName = strchr(g_DamageMap.GetElementName(idx),',') + 1;

		int ColumnIdx = g_UniqueColumns.Find( szColumnName );

		if( ColumnIdx == g_UniqueColumns.InvalidIndex() ) 
		{
			g_UniqueColumns.Insert( szColumnName, false );
		}
	}

	// Dump the column names:
	FileHandle_t f = filesystem->Open("damage.txt","wt+");

	for( idx = g_UniqueColumns.First(); idx != g_UniqueColumns.InvalidIndex(); idx = g_UniqueColumns.Next(idx) )
	{
		filesystem->FPrintf(f,"\t%s",g_UniqueColumns.GetElementName(idx));
	}

	filesystem->FPrintf(f,"\n");

 
	CUtlDict<bool,int> g_CompletedRows;

	// Dump each row:
	bool bDidRow;

	do
	{
		bDidRow = false;

		for( idx = g_DamageMap.First(); idx != g_DamageMap.InvalidIndex(); idx = g_DamageMap.Next(idx) )
		{
			char szRowName[256];

			// Check the Row name of each entry to see if I've done this row yet.
			Q_strncpy(szRowName, g_DamageMap.GetElementName(idx), sizeof( szRowName ) );
			*strchr(szRowName,',') = '\0';

			char szRowNameComma[256];
			Q_snprintf( szRowNameComma, sizeof( szRowNameComma ), "%s,", szRowName );

			if( g_CompletedRows.Find(szRowName) == g_CompletedRows.InvalidIndex() )
			{
				bDidRow = true;
				g_CompletedRows.Insert(szRowName,false);


				// Output the row name:
				filesystem->FPrintf(f,szRowName);

				for( int ColumnIdx = g_UniqueColumns.First(); ColumnIdx != g_UniqueColumns.InvalidIndex(); ColumnIdx = g_UniqueColumns.Next( ColumnIdx ) )
				{
					char szRowNameCommaColumn[256];
					Q_strncpy( szRowNameCommaColumn, szRowNameComma, sizeof( szRowNameCommaColumn ) );					
					Q_strncat( szRowNameCommaColumn, g_UniqueColumns.GetElementName( ColumnIdx ), sizeof( szRowNameCommaColumn ), COPY_ALL_CHARACTERS );

					int nDamageAmount = 0;
					// Fine to reuse idx since we are going to break anyways.
					for( idx = g_DamageMap.First(); idx != g_DamageMap.InvalidIndex(); idx = g_DamageMap.Next(idx) )
					{
						if( !stricmp( g_DamageMap.GetElementName(idx), szRowNameCommaColumn ) )
						{
							nDamageAmount = g_DamageMap[idx];
							break;
						}
					}

					filesystem->FPrintf(f,"\t%i",nDamageAmount);

				}

				filesystem->FPrintf(f,"\n");
				break;
			}
		}
		// Grab the row name:

	} while(bDidRow);

	// close the file:
	filesystem->Close(f);
}

static ConCommand obj_dump_damage( "obj_dump_damage", Cmd_DamageDump_f );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ReportDamage( const char* szInflictor, const char* szVictim, float fAmount, int nCurrent, int nMax )
{
	int iAmount = (int)fAmount;

	if( object_show_damage.GetBool() && iAmount )
	{
		Msg( "ShowDamage: Object %s taking %0.1f damage from %s ( %i / %i )\n", szVictim, fAmount, szInflictor, nCurrent, nMax );
	}

	if( object_capture_damage.GetBool() )
	{
		char szMangledKey[256];

		Q_snprintf(szMangledKey,sizeof(szMangledKey)/sizeof(szMangledKey[0]),"%s,%s",szInflictor,szVictim);
		int idx = g_DamageMap.Find( szMangledKey );

		if( idx == g_DamageMap.InvalidIndex() )
		{
			g_DamageMap.Insert( szMangledKey, iAmount );

		} else
		{
			g_DamageMap[idx] += iAmount;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the first non-hostile object build on this object
//-----------------------------------------------------------------------------
CBaseEntity *CBaseObject::GetFirstFriendlyObjectOnMe( void )
{
	CBaseObject *pFirstObject = NULL;

	IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>(this);
	int iNumObjects = pBPInterface->GetNumObjectsOnMe();
	for ( int iPoint=0;iPoint<iNumObjects;iPoint++ )
	{
		CBaseObject *pObject = GetBuildPointObject( iPoint );

		if ( pObject && !pObject->IsHostileUpgrade() )
		{
			pFirstObject = pObject;
			break;
		}
	}

	return pFirstObject;
}

//-----------------------------------------------------------------------------
// Purpose: Pass the specified amount of damage through to any objects I have built on me
//-----------------------------------------------------------------------------
bool CBaseObject::PassDamageOntoChildren( const CTakeDamageInfo &info, float *flDamageLeftOver )
{
	float flDamage = info.GetDamage();

	// Double the amount of damage done (and get around the child damage modifier)
	flDamage *= 2;
	if ( obj_child_damage_factor.GetFloat() )
	{
		flDamage *= (1 / obj_child_damage_factor.GetFloat());
	}

	// Remove blast damage because child objects (well specifically upgrades)
	// want to ignore direct blast damage but still take damage from parent
	CTakeDamageInfo childInfo = info;
	childInfo.SetDamage( flDamage );
	childInfo.SetDamageType( info.GetDamageType() & (~DMG_BLAST) );

	CBaseEntity *pEntity = GetFirstFriendlyObjectOnMe();
	while ( pEntity )
	{
		Assert( pEntity->m_takedamage != DAMAGE_NO );
		// Do damage to the next object
		float flDamageTaken = pEntity->OnTakeDamage( childInfo );
		// If we didn't kill it, abort
		CBaseObject *pObject = dynamic_cast<CBaseObject*>(pEntity);
		if ( !pObject || !pObject->IsDying() )
		{
			const char* szInflictor = "unknown";
			if( info.GetInflictor() )
				szInflictor = (char*)info.GetInflictor()->GetClassname();

			ReportDamage( szInflictor, GetClassname(), flDamageTaken, GetHealth(), GetMaxHealth() );

			*flDamageLeftOver = flDamage;
			return true;
		}
		// Reduce the damage and move on to the next
		flDamage -= flDamageTaken;
		pEntity = GetFirstFriendlyObjectOnMe();
	}

	*flDamageLeftOver = flDamage;
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseObject::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !IsAlive() )
		return info.GetDamage();

	if ( m_takedamage == DAMAGE_NO )
		return 0;

	if ( IsPlacing() )
		return 0;

	// Check teams
	if ( info.GetAttacker() )
	{
		if ( InSameTeam(info.GetAttacker()) )
			return 0;

		if ( TFGameRules() && TFGameRules()->IsTruceActive() )
		{
			// players cannot damage buildings while a truce is active
			if ( info.GetAttacker()->IsPlayer() && info.GetAttacker()->IsTruceValidForEnt() && ( ( info.GetAttacker()->GetTeamNumber() == TF_TEAM_RED ) || ( info.GetAttacker()->GetTeamNumber() == TF_TEAM_BLUE ) ) )
				return 0;
		}
	}

	m_AchievementData.AddDamagerToHistory( info.GetAttacker() );
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
	{
		ToTFPlayer( info.GetAttacker() )->m_AchievementData.AddTargetToHistory( this );
	}

	IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>(this);

	float flDamage = info.GetDamage();

	// Buildings are resistant to plasma damage.
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA )
	{
		flDamage *= 0.2f;
	}

	// Charged plasma damage disables buildings for a short time.
	if ( info.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA_CHARGED )
	{
		flDamage *= 0.2f;
		m_flPlasmaDisableTime = gpGlobals->curtime + PLASMA_DISABLE_TIME;
		m_bPlasmaDisable = true;
		UpdateDisabledState();
	}

	// Objects build on other objects take less damage
	if ( !IsAnUpgrade() && GetParentObject() )
	{
		flDamage *= obj_child_damage_factor.GetFloat();
	}

	if (obj_damage_factor.GetFloat())
	{
		flDamage *= obj_damage_factor.GetFloat();
	}

	
	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>(info.GetWeapon());
	if ( pWeapon )
	{

		// Apply attributes that increase damage vs buildings
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flDamage, mult_dmg_vs_buildings );
		CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pAttacker )
		{
			pWeapon->ApplyOnHitAttributes( NULL, pAttacker, info );
		}
	}

	if ( TFGameRules()->IsPowerupMode() )
	{
		CTFPlayer *pAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pAttacker )
		{
			if ( pAttacker->m_Shared.GetCarryingRuneType() == RUNE_STRENGTH || pAttacker->m_Shared.InCond( TF_COND_RUNE_IMBALANCE ) )
			{
				flDamage *= 2.f;
			}

			if ( pAttacker->m_Shared.GetCarryingRuneType() == RUNE_KNOCKOUT )
			{
				flDamage *= 4.f;
			}

			if ( pAttacker->m_Shared.GetCarryingRuneType() == RUNE_VAMPIRE )
			{
				int iModHealthOnHit = flDamage;

				if ( iModHealthOnHit > 0 )
				{
					pAttacker->TakeHealth( iModHealthOnHit, DMG_GENERIC );
				}
			}
		}
	}

	bool bFriendlyObjectsAttached = false;
	int iNumObjects = pBPInterface->GetNumObjectsOnMe();
	for ( int iPoint=0;iPoint<iNumObjects;iPoint++ )
	{
		CBaseObject *pObject = GetBuildPointObject( iPoint );

		if ( !pObject || pObject->IsHostileUpgrade() )
		{
			continue;
		}

		bFriendlyObjectsAttached = true;
		break;
	}

	// Don't look, Tom Bui!
	static struct
	{
		bool operator()( const float flHealth, const float flDamage ) const
		{
			return ( ( flHealth - flDamage ) < 1 );
		}
	} IsDamageFatal;

	// Only track actual damage - not overkill
	m_AchievementData.AddDamageEventToHistory( info.GetAttacker(), ( IsDamageFatal( m_flHealth, flDamage ) ) ? m_flHealth : flDamage );

	// if we cannot die
	if ( m_bCannotDie && IsDamageFatal( m_flHealth, flDamage ) )
	{
		flDamage = m_flHealth - 1;
	}

	// If I have objects on me, I can't be destroyed until they're gone. Ditto if I can't be killed.
	bool bWillDieButCant = ( bFriendlyObjectsAttached ) && IsDamageFatal( m_flHealth, flDamage );
	if ( bWillDieButCant )
	{
		// Soak up the damage it would take to drop us to 1 health
		flDamage = flDamage - m_flHealth;
		SetHealth( 1 );

		// Pass leftover damage 
		if ( flDamage )
		{
			if ( PassDamageOntoChildren( info, &flDamage ) )
				return flDamage;
		}
	}

	if ( flDamage )
	{
		m_iLifetimeDamage += floor( Min( flDamage, m_flHealth ) );
		if ( m_iLifetimeDamage > tf_obj_damage_tank_achievement_amount.GetInt() && GetBuilder() )
		{
			GetBuilder()->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_TANK_DAMAGE );
		}

		// Recheck our death possibility, because our objects may have all been blown off us by now
		bWillDieButCant = ( bFriendlyObjectsAttached ) && IsDamageFatal( m_flHealth, flDamage );
		if ( !bWillDieButCant )
		{
			// Reduce health
			SetHealth( m_flHealth - flDamage );
		}
	}

	m_OnDamaged.FireOutput(info.GetAttacker(), this);

	if ( GetHealth() <= 0 )
	{
		if ( info.GetAttacker() )
		{
			//TFStats()->IncrementTeamStat( info.GetAttacker()->GetTeamNumber(), TF_TEAM_STAT_DESTROYED_OBJECT_COUNT, 1 );
			//TFStats()->IncrementPlayerStat( info.GetAttacker(), TF_PLAYER_STAT_DESTROYED_OBJECT_COUNT, 1 );
		}

		m_lifeState = LIFE_DEAD;
		Killed( info );

		// Tell our builder to speak about it
		if ( m_hBuilder )
		{
			m_hBuilder->SpeakConceptIfAllowed( MP_CONCEPT_LOST_OBJECT, GetResponseRulesModifier() );
		}
	}

	const char* szInflictor = "unknown";
	if( info.GetInflictor() )
		szInflictor = (char*)info.GetInflictor()->GetClassname();

	ReportDamage( szInflictor, GetClassname(), flDamage, GetHealth(), GetMaxHealth() );

	IGameEvent *event = gameeventmanager->CreateEvent( "npc_hurt" );
	if ( event )
	{
		event->SetInt( "entindex", entindex() );
		event->SetInt( "health", Max( 0, (int)GetHealth() ) );
		event->SetInt( "damageamount", flDamage );
		event->SetBool( "crit", ( info.GetDamageType() & DMG_CRITICAL ) ? true : false );

		CTFPlayer *pTFAttacker = ToTFPlayer( info.GetAttacker() );
		if ( pTFAttacker )
		{
			event->SetInt( "attacker_player", pTFAttacker->GetUserID() );

			if ( pTFAttacker->GetActiveTFWeapon() )
			{
				event->SetInt( "weaponid", pTFAttacker->GetActiveTFWeapon()->GetWeaponID() );
			}
			else
			{
				event->SetInt( "weaponid", 0 );
			}
		}
		else
		{
			// hurt by world
			event->SetInt( "attacker_player", 0 );
			event->SetInt( "weaponid", 0 );
		}

		gameeventmanager->FireEvent( event );
	}

	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Repair / Help-Construct this object the specified amount
//-----------------------------------------------------------------------------
bool CBaseObject::Construct( float flHealth )
{
	// Multiply it by the repair rate
	flHealth *= GetConstructionMultiplier();
	if ( !flHealth )
		return false;

	if ( IsBuilding() )
	{
		// Reduce the construction time by the correct amount for the health passed in
		float flConstructionTime = flHealth / ((GetMaxHealth() - OBJECT_CONSTRUCTION_STARTINGHEALTH) / m_flTotalConstructionTime);
		if ( flConstructionTime < 0.0f )
		{
			flConstructionTime *= -1.0f;
		}

		m_flConstructionTimeLeft = MAX( 0, m_flConstructionTimeLeft - flConstructionTime);
		m_flConstructionTimeLeft = clamp( m_flConstructionTimeLeft, 0.0f, m_flTotalConstructionTime );

		m_flPercentageConstructed = m_flConstructionTimeLeft / m_flTotalConstructionTime;

		if ( flHealth >= 0.0f )
		{
			// Only do this if we're not reversing construction
			m_flPercentageConstructed = 1.0f - m_flPercentageConstructed;
		}
		m_flPercentageConstructed = clamp( (float) m_flPercentageConstructed, 0.0f, 1.0f );

		// Increase health (unless it's a mini-building, which start at max health)
		// Minibuildings build health at a reduced rate
		// Staging_engy
		{
			SetHealth( Min( (float)GetMaxHealth(), m_flHealth + (IsMiniBuilding() ? (flHealth * 0.5f) : flHealth) ) );
		}

		// Return true if we're constructed now
		if ( m_flConstructionTimeLeft <= 0.0f )
		{
			FinishedBuilding();
			return true;
		}
	}
	else
	{
		// Return true if we're already fully healed
		if ( GetHealth() >= GetMaxHealth() )
			return true;

		// Increase health.
		SetHealth( Min( (float)GetMaxHealth(), Max( 1.f, m_flHealth + flHealth ) ) );

		m_OnRepaired.FireOutput( this, this);

		// Return true if we're fully healed now
		if ( GetHealth() == GetMaxHealth() )
			return true;
	}

	return false;
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CBaseObject::OnConstructionHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc )
{
	// Get the player index
	int iPlayerIndex = pPlayer->entindex();

	// The time the repair is going to expire
	float flRepairExpireTime = gpGlobals->curtime + 1.0;

	// Update or Add the expire time to the list
	int index = m_ConstructorList.Find( iPlayerIndex );
	if ( index == m_ConstructorList.InvalidIndex() )
	{
		index = m_ConstructorList.Insert( iPlayerIndex );
		m_ConstructorList[index].flValue = pWrench->GetConstructionValue();
	}

	m_ConstructorList[index].flHitTime = flRepairExpireTime;

	// Play a construction hit effect.
	CPVSFilter filter( hitLoc );
	TE_TFParticleEffect( filter, 0.0f, "nutsnbolts_build", hitLoc, QAngle(0,0,0) );
}

//----------------------------------------------------------------------------------------------------------------------------------------
float CBaseObject::GetConstructionMultiplier( void )
{
	if ( IsUsingReverseBuild() )
		return -1.0f;

	float flMultiplier = 1.0;

	// expire all the old 
	int i = m_ConstructorList.LastInorder();
	while ( i != m_ConstructorList.InvalidIndex() )
	{
		int iThis = i;
		i = m_ConstructorList.PrevInorder( i );
		if ( m_ConstructorList[iThis].flHitTime < gpGlobals->curtime )
		{
			m_ConstructorList.RemoveAt( iThis );
		}
		else
		{
			// STAGING_ENGY
			// each Player adds a fixed amount of speed boost
			// Carry deploy hits add more
			flMultiplier += ( m_ConstructorList[iThis].flValue );
		}
	}

	// See if we have any attributes that want to modify our build rate
	CTFPlayer* pBuilder = GetOwner();
	if( pBuilder )
	{
		flMultiplier += pBuilder->GetObjectBuildSpeedMultiplier( ObjectType(), m_bCarryDeploy );
	}

	return flMultiplier;
}

//-----------------------------------------------------------------------------
// Purpose: Object is exploding because it was killed or detonate
//-----------------------------------------------------------------------------
void CBaseObject::Explode( void )
{
	const char *pExplodeSound = GetObjectInfo( ObjectType() )->m_pExplodeSound;

	if ( pExplodeSound && Q_strlen(pExplodeSound) > 0 )
	{
		EmitSound( pExplodeSound );
	}

	const char *pExplodeEffect = GetObjectInfo( ObjectType() )->m_pExplosionParticleEffect;
	if ( pExplodeEffect && pExplodeEffect[0] != '\0' )
	{
		// Send to everyone - we're inside prediction for the engy who hit this off, but we 
		// don't predict that the hit will kill this object.
		CDisablePredictionFiltering disabler;

		Vector origin = GetAbsOrigin();
		QAngle up(-90,0,0);

		CPVSFilter filter( origin );
		TE_TFParticleEffect( filter, 0.0f, pExplodeEffect, origin, up );
	}

	// create some delicious, metal filled gibs
	CreateObjectGibs();
}

void CBaseObject::CreateObjectGibs( void )
{
	if ( m_aGibs.Count() <= 0 )
	{
		return;
	}

	const CObjectInfo *pObjectInfo = GetObjectInfo( ObjectType() );

	// grant some percentage of the cost to build if number of metal to drop is not specified
	const float flMetalCostPercentage = 0.5f;
	const int nTotalMetal = pObjectInfo->m_iMetalToDropInGibs == 0 ? pObjectInfo->m_Cost * flMetalCostPercentage : pObjectInfo->m_iMetalToDropInGibs;

	
	int nMetalPerGib = nTotalMetal / m_aGibs.Count();
	int nLeftOver = nTotalMetal % m_aGibs.Count();

	if ( IsMiniBuilding() )
	{
		// STAGING_ENGY
		nMetalPerGib = 0;
		nLeftOver = 0;
	}

	int i;
	for ( i=0; i<m_aGibs.Count(); i++ )
	{
		// make sure we drop all metal include left over from int math
		CreateAmmoPack( m_aGibs[i].modelName, i == 0 ? nMetalPerGib + nLeftOver : nMetalPerGib );
	}
}

CTFAmmoPack* CBaseObject::CreateAmmoPack( const char *pchModel, int nMetal )
{
	CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( GetAbsOrigin(), GetAbsAngles(), this, pchModel );
	Assert( pAmmoPack );
	if ( pAmmoPack )
	{
		pAmmoPack->ActivateWhenAtRest();

		// Fill up the ammo pack.
		pAmmoPack->GiveAmmo( nMetal, TF_AMMO_METAL );

		// Calculate the initial impulse on the weapon.
		Vector vecImpulse( random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( -0.5, 0.5 ), random->RandomFloat( 0.75, 1.25 ) );
		VectorNormalize( vecImpulse );
		vecImpulse *= random->RandomFloat( tf_obj_gib_velocity_min.GetFloat(), tf_obj_gib_velocity_max.GetFloat() );

		// Cap the impulse.
		float flSpeed = vecImpulse.Length();
		if ( flSpeed > tf_obj_gib_maxspeed.GetFloat() )
		{
			VectorScale( vecImpulse, tf_obj_gib_maxspeed.GetFloat() / flSpeed, vecImpulse );
		}

		if ( pAmmoPack->VPhysicsGetObject() )
		{
			// We can probably remove this when the mass on the weapons is correct!
			//pAmmoPack->VPhysicsGetObject()->SetMass( 25.0f );
			AngularImpulse angImpulse( 0, random->RandomFloat( 0, 100 ), 0 );
			pAmmoPack->VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
		}

		pAmmoPack->SetInitialVelocity( vecImpulse );

		pAmmoPack->m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;

		// Give the ammo pack some health, so that trains can destroy it.
		pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		pAmmoPack->m_takedamage = DAMAGE_YES;		
		pAmmoPack->SetHealth( 900 );
		pAmmoPack->m_bObjGib = true;

		if ( IsMiniBuilding() )
		{
			pAmmoPack->SetModelScale( 0.6f );
		}
	}

	return pAmmoPack;
}

//-----------------------------------------------------------------------------
// Purpose: Object has been blown up. Drop resource chunks up to the value of my max health.
//-----------------------------------------------------------------------------
void CBaseObject::Killed( const CTakeDamageInfo &info )
{
	m_bDying = true;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CTFPlayer *pScorer = ToTFPlayer( TFGameRules()->GetDeathScorer( pKiller, pInflictor, this ) );
	CTFPlayer *pAssister = NULL;

	m_OnDestroyed.FireOutput( pKiller, this );

	// if this object has a sapper on it, and was not killed by the sapper (killed by damage other than crush, since sapper does crushing damage),
	// award an assist to the owner of the sapper since it probably contributed to destroying this object
	CObjectSapper *pSapper = GetSapper();
	if ( pSapper && !( DMG_CRUSH & info.GetDamageType() ) && !m_bPlasmaDisable )
	{
		// give an assist to the sapper's owner
		pAssister = pSapper->GetOwner();
		if ( pAssister )
		{
			CTF_GameStats.Event_AssistDestroyBuilding( pAssister, this );

			// Also increment the SapBuildings grind achievement
			pAssister->AwardAchievement( ACHIEVEMENT_TF_SPY_SAPPER_GRIND );
		}
	}
	else if ( pScorer )
	{
		// If a player is healing the scorer, give that player credit for the assist
		CTFPlayer *pHealer = ToTFPlayer( static_cast<CBaseEntity *>( pScorer->m_Shared.GetFirstHealer() ) );
		// Must be a medic to receive a healing assist, otherwise engineers get credit for assists from dispensers doing healing.
		// Also don't give an assist for healing if the inflictor was a sentry gun, otherwise medics healing engineers get assists for the engineer's sentry kills.
		if ( pHealer && ( pHealer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC ) )
		{
			pAssister = pHealer;
		}
	}

	// Don't do anything if we were detonated or dismantled
	if ( pScorer && pInflictor != this )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "object_destroyed" );

		// Work out what killed the player, and send a message to all clients about it
		int iWeaponID;
		const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, NULL, &iWeaponID );
		const char *killer_weapon_log_name = killer_weapon_name;

		CTFPlayer *pTFPlayer = GetOwner();

		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >( pScorer->Weapon_OwnsThisID( iWeaponID ) );
		if ( pWeapon )
		{
			CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();

			if ( pItem )
			{
				if ( pItem->GetStaticData()->GetIconClassname() )
				{
					killer_weapon_name = pItem->GetStaticData()->GetIconClassname();
				}

				if ( pItem->GetStaticData()->GetLogClassname() )
				{
					killer_weapon_log_name = pItem->GetStaticData()->GetLogClassname();
				}
			}
		}

		if ( event )
		{
			if ( pTFPlayer )
			{
				event->SetInt( "userid", pTFPlayer->GetUserID() );
			}
			if ( pAssister && ( pAssister != pScorer ) )
			{
				event->SetInt( "assister", pAssister->GetUserID() );
			}
			
			event->SetInt( "attacker", pScorer->GetUserID() );	// attacker
			event->SetString( "weapon", killer_weapon_name );
			event->SetString( "weapon_logclassname", killer_weapon_log_name );
			event->SetInt( "weaponid", iWeaponID );
			event->SetInt( "priority", 6 );		// HLTV event priority, not transmitted
			event->SetInt( "objecttype", GetType() );
			event->SetInt( "index", entindex() );	// object entity index
			event->SetBool( "was_building", m_bBuilding );
			event->SetInt( "team", GetTeamNumber() );
			gameeventmanager->FireEvent( event );
		}

		CTF_GameStats.Event_PlayerDestroyedBuilding( pScorer, this );
		pScorer->Event_KilledOther(this, info);

		// Also track stats for strange sappers.
		if ( pSapper )
		{
			CTFPlayer *pSapperOwner = pSapper->GetOwner();
			Assert( pSapperOwner );

			if ( pSapperOwner )
			{
				EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( pSapperOwner->GetEntityForLoadoutSlot( LOADOUT_POSITION_BUILDING ) ),
												  pSapperOwner,
												  GetOwner(),
												  kKillEaterEvent_BuildingSapped );
			}
		}

		// Check for Demo achievement:
		// Kill an Engineer building that you can't see with a direct hit from a Grenade Launcher

		if ( pScorer && pScorer->IsPlayerClass( TF_CLASS_DEMOMAN) )
		{
			if ( pScorer->GetActiveTFWeapon() && ( pScorer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_GRENADELAUNCHER) )
			{
				if ( pInflictor && pInflictor->IsPlayer() == false )
				{
					CTFGrenadePipebombProjectile *pBaseGrenade = dynamic_cast< CTFGrenadePipebombProjectile* >( pInflictor );
					if ( pBaseGrenade && pBaseGrenade->m_bTouched == false )
					{
						if ( pScorer->FVisible( this ) == false )
						{
							pScorer->AwardAchievement( ACHIEVEMENT_TF_DEMOMAN_KILL_BUILDING_DIRECT_HIT );
						}
					}
				}
			}
		}
	}
	else
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "object_detonated" );

		if ( event )
		{
			CTFPlayer *pTFPlayer = GetOwner();
			if ( pTFPlayer )
			{
				event->SetInt( "userid", pTFPlayer->GetUserID() );
			}
			event->SetInt( "objecttype", GetType() ); // object type
			event->SetInt( "index", entindex() );	// object entity index

			gameeventmanager->FireEvent( event );
		}
	}

	// Don't create gibs if it reversed back to a toolbox
	if ( IsUsingReverseBuild() && ( DMG_CRUSH & info.GetDamageType() ) != 0 )
	{
		CTFAmmoPack *pAmmoPack = CreateAmmoPack( "models/weapons/w_models/w_toolbox.mdl", GetObjectInfo( ObjectType() )->m_iMetalToDropInGibs );
		if ( pAmmoPack )
		{
			pAmmoPack->SetBodygroup( 1, 1 );
		}

		CObjectSapper *pSapper = GetSapper();
		if ( pSapper )
		{
			pSapper->Explode();
		}
	}
	else
	{
		// Do an explosion.
		Explode();
	}

	// Stats tracking for strange items.
	if ( info.GetWeapon() )
	{
		EconEntity_OnOwnerKillEaterEvent( dynamic_cast<CEconEntity *>( info.GetWeapon() ),
										  pScorer,
										  GetOwner(),
										  kKillEaterEvent_BuildingDestroyed );
	}
	else if ( pScorer && GetOwner() )
	{
		//  we still want strange cosmetics to count buildings destroyed
		HatAndMiscEconEntities_OnOwnerKillEaterEvent( pScorer,
													  GetOwner(),
													  kKillEaterEvent_BuildingDestroyed );
	}

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: Indicates this NPC's place in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CBaseObject::Classify( void )
{
	return CLASS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Get the type of this object
//-----------------------------------------------------------------------------
int	CBaseObject::GetType() const
{
	return m_iObjectType;
}

//-----------------------------------------------------------------------------
// Purpose: Get the builder of this object
//-----------------------------------------------------------------------------
CTFPlayer *CBaseObject::GetBuilder( void ) const
{
	return m_hBuilder;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the Owning CTeam should clean this object up automatically
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldAutoRemove( void )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iTeamNum - 
//-----------------------------------------------------------------------------
void CBaseObject::ChangeTeam( int iTeamNum )
{
	CTFTeam *pTeam = ( CTFTeam * )GetGlobalTeam( iTeamNum );
	CTFTeam *pExisting = ( CTFTeam * )GetTeam();

	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseObject::ChangeTeam old %s new %s\n", gpGlobals->curtime, 
		pExisting ? pExisting->GetName() : "NULL",
		pTeam ? pTeam->GetName() : "NULL" ) );

	// Already on this team
	if ( GetTeamNumber() == iTeamNum )
		return;

	if ( pExisting )
	{
		// Remove it from current team ( if it's in one ) and give it to new team
		pExisting->RemoveObject( this );
	}
		
	// Change to new team
	BaseClass::ChangeTeam( iTeamNum );
	
	// Add this object to the team's list
	// But only if we're not placing it
	if ( pTeam && (!m_bPlacing) )
	{
		pTeam->AddObject( this );
	}

	// Setup for our new team's model
	CreateBuildPoints();
}

CObjectSapper* CBaseObject::GetSapper( void )
{
	if ( !HasSapper() )
		return NULL;

	return dynamic_cast< CObjectSapper* >( FirstMoveChild() );
}

//-----------------------------------------------------------------------------
// Purpose: Return true if I have at least 1 sapper on me
//-----------------------------------------------------------------------------
bool CBaseObject::HasSapper( void )
{
	return m_bHasSapper;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseObject::IsPlasmaDisabled( void )
{
	return m_bPlasmaDisable;
}

//-----------------------------------------------------------------------------
void CBaseObject::OnAddSapper( void )
{
	// Assume we can only build 1 sapper per object
	Assert( m_bHasSapper == false );

	m_bHasSapper = true;

	CTFPlayer *pPlayer = GetBuilder();

	if ( pPlayer )
	{
		//pPlayer->HintMessage( HINT_OBJECT_YOUR_OBJECT_SAPPED, true );
		pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_SPY_SAPPER, GetResponseRulesModifier() );
	}

	UpdateDisabledState();
}

//-----------------------------------------------------------------------------
void CBaseObject::OnRemoveSapper( void )
{
	m_bHasSapper = false;
	UpdateDisabledState();
}

//-----------------------------------------------------------------------------
int CBaseObject::GetUpgradeMetalRequired()
{
	return GetObjectInfo( GetType() )->m_UpgradeCost;
}

//-----------------------------------------------------------------------------
bool CBaseObject::ShowVGUIScreen( int panelIndex, bool bShow )
{
	Assert( panelIndex >= 0 && panelIndex < m_hScreens.Count() );
	if ( m_hScreens[panelIndex].Get() )
	{
		m_hScreens[panelIndex]->SetActive( bShow );
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the health of the object
//-----------------------------------------------------------------------------
void CBaseObject::InputSetHealth( inputdata_t &inputdata )
{
	m_iMaxHealth = inputdata.value.Int();
	SetHealth( m_iMaxHealth );
}

//-----------------------------------------------------------------------------
// Purpose: Add health to the object
//-----------------------------------------------------------------------------
void CBaseObject::InputAddHealth( inputdata_t &inputdata )
{
	int iHealth = inputdata.value.Int();
	SetHealth( Min( (float)GetMaxHealth(), m_flHealth + iHealth ) );
}

//-----------------------------------------------------------------------------
// Purpose: Remove health from the object
//-----------------------------------------------------------------------------
void CBaseObject::InputRemoveHealth( inputdata_t &inputdata )
{
	int iDamage = inputdata.value.Int();

	SetHealth( m_flHealth - iDamage );
	if ( GetHealth() <= 0 )
	{
		m_lifeState = LIFE_DEAD;

		CTakeDamageInfo info( inputdata.pCaller, inputdata.pActivator, vec3_origin, GetAbsOrigin(), iDamage, DMG_GENERIC );
		Killed( info );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseObject::InputSetSolidToPlayer( inputdata_t &inputdata )
{
	int ival = inputdata.value.Int();
	ival = clamp( ival, (int)SOLID_TO_PLAYER_USE_DEFAULT, (int)SOLID_TO_PLAYER_NO );
	OBJSOLIDTYPE stp = (OBJSOLIDTYPE)ival;
	SetSolidToPlayers( stp );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseObject::InputSetBuilder( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( inputdata.pActivator );
	if ( GetBuilder() == NULL && pPlayer != NULL )
	{
		SetBuilder( pPlayer );
		ChangeTeam( pPlayer->GetTeamNumber() );
		pPlayer->AddObject( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseObject::InputShow( inputdata_t &inputdata )
{
	RemoveEffects( EF_NODRAW );
	UpdateDisabledState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseObject::InputHide( inputdata_t &inputdata )
{
	AddEffects( EF_NODRAW );
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : 
// Output : did this wrench hit do any work on the object?
//-----------------------------------------------------------------------------
bool CBaseObject::InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc )
{
	Assert( pPlayer );
	if ( !pPlayer )
		return false;

	bool bDidWork = false;

	if ( HasSapper() )
	{
		// do damage to any attached buildings
		CTakeDamageInfo info( pPlayer, pPlayer, pWrench, WRENCH_DMG_VS_SAPPER, DMG_CLUB, TF_DMG_WRENCH_FIX );

		IHasBuildPoints *pBPInterface = dynamic_cast< IHasBuildPoints * >( this );
		int iNumObjects = pBPInterface->GetNumObjectsOnMe();
		for ( int iPoint=0;iPoint<iNumObjects;iPoint++ )
		{
			CBaseObject *pObject = GetBuildPointObject( iPoint );

			if ( pObject && pObject->IsHostileUpgrade() )
			{
				int iBeforeHealth = pObject->GetHealth();

				pObject->TakeDamage( info );

				// This should always be true
				if ( iBeforeHealth != pObject->GetHealth() )
				{
					bDidWork = true;
					Assert( bDidWork );
				}
			}
		}
	}
	else if ( IsUpgrading() )
	{
		bDidWork = OnWrenchHit( pPlayer, pWrench, hitLoc );
//		bDidWork = false;
	}
	else if ( IsBuilding() )
	{
		OnConstructionHit( pPlayer, pWrench, hitLoc );
		bDidWork = true;
	}
	else
	{
		// upgrade, refill, repair damage
		bDidWork = OnWrenchHit( pPlayer, pWrench, hitLoc );
	}

	if ( bDidWork )
	{
		pPlayer->m_AchievementData.AddTargetToHistory( this );
	}

	return bDidWork;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseObject::OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector hitLoc )
{
	bool bRepairHit = false;
	bool bUpgradeHit = false;

	bRepairHit = ( Command_Repair( pPlayer, pWrench->GetRepairAmount(), 1.f ) > 0 );

	if ( !bRepairHit )
	{
		bUpgradeHit = CheckUpgradeOnHit( pPlayer );
	}

	DoWrenchHitEffect( hitLoc, bRepairHit, bUpgradeHit );

	return bUpgradeHit || bRepairHit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::DoWrenchHitEffect( Vector hitLoc, bool bRepairHit, bool bUpgradeHit )
{
	if ( bRepairHit )
	{
		// Play a repair hit effect.
		CPVSFilter filter( hitLoc );
		TE_TFParticleEffect( filter, 0.0f, "nutsnbolts_repair", hitLoc, QAngle(0,0,0) );
	}
	else if ( bUpgradeHit )
	{
		// Play an upgrade hit effect.
		CPVSFilter filter( hitLoc );
		TE_TFParticleEffect( filter, 0.0f, "nutsnbolts_upgrade", hitLoc, QAngle(0,0,0) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseObject::CheckUpgradeOnHit( CTFPlayer *pPlayer )
{
	if ( !CanBeUpgraded() )
		return false;

	if ( m_bCarryDeploy )
		return false;

	if ( CanBeUpgraded( pPlayer ) )
	{
		int iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );
		int nMaxToAdd = GetUpgradeAmountPerHit();
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, nMaxToAdd, upgrade_rate_mod );
		int iAmountToAdd = Min( nMaxToAdd, iPlayerMetal );

		if ( iAmountToAdd > ( m_iUpgradeMetalRequired - m_iUpgradeMetal ) )
			iAmountToAdd = ( m_iUpgradeMetalRequired - m_iUpgradeMetal );

		if ( tf_cheapobjects.GetBool() == false && !ShouldQuickBuild() )
		{
			pPlayer->RemoveAmmo( iAmountToAdd, TF_AMMO_METAL );
		}

		// testing quick builds for engineers in Raid mode
		if ( TFGameRules() && !TFGameRules()->IsPVEModeControlled( pPlayer ) )
		{
#ifdef TF_RAID_MODE
			if ( TFGameRules()->IsRaidMode() )
			{
				iAmountToAdd = 200;
			}
#endif
			
			if ( TFGameRules()->GameModeUsesUpgrades() && TFGameRules()->IsQuickBuildTime() )
			{
				iAmountToAdd = 200;
			}
		}

		m_iUpgradeMetal += iAmountToAdd;

		bool bDidWork = false;
		if ( iAmountToAdd > 0 )
		{
			bDidWork = true;
		}

		if ( m_iUpgradeMetal >= m_iUpgradeMetalRequired )
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "player_upgradedobject" );
			if ( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetInt( "object", ObjectType() );
				event->SetInt( "index", entindex() );
				event->SetBool( "isbuilder", pPlayer == GetBuilder() );

				gameeventmanager->FireEvent( event );
			}

			StartUpgrading();
			m_iUpgradeMetal = 0;
		}

		return bDidWork;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CBaseObject::CanBeUpgraded( CTFPlayer *pPlayer )
{
	// Already upgrading
	if ( IsUpgrading() )
		return false;

	if ( IsMiniBuilding() || IsDisposableBuilding() )
		return false;

	// only engineers
	if ( !ClassCanBuild( pPlayer->GetPlayerClass()->GetClassIndex(), GetType() ) )
		return false;

	// max upgraded
	if ( m_iUpgradeLevel >= OBJ_MAX_UPGRADE_LEVEL )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Separated so it can be triggered by wrench hit or by vgui screen
//-----------------------------------------------------------------------------
int CBaseObject::Command_Repair( CTFPlayer *pActivator, float flAmount, float flRepairMod, float flRepairToMetalRatio /*= 3.f*/, bool bSendEvent /*= false*/ )
{
	if ( !CanBeRepaired() )
		return false;
	
	float flRepairAmountMax = flAmount * flRepairMod;
	int iRepairAmount = Min( RoundFloatToInt( flRepairAmountMax ), GetMaxHealth() - RoundFloatToInt( GetHealth() ) );
	int iRepairCost = ceil( (float)( iRepairAmount ) / flRepairToMetalRatio );
	if ( iRepairCost > pActivator->GetBuildResources() )
	{
		// What can we afford?
		iRepairCost = pActivator->GetBuildResources();
	}

	TRACE_OBJECT( UTIL_VarArgs( "%0.2f CBaseObject::Command_Repair ( %f / %d ) - cost = %d\n", gpGlobals->curtime, 
		GetHealth(),
		GetMaxHealth(),
		iRepairCost ) );

	if ( iRepairCost > 0 )
	{
		iRepairAmount = iRepairCost * flRepairToMetalRatio;
		float flNewHealth = Min( (float)GetMaxHealth(), m_flHealth + iRepairAmount );
		if ( m_bCarryDeploy && ( flNewHealth > m_iHealthOnPickup ) )
		{
			// If we are re-deploying after being carried we shouldn't gain more health than we had
			// on pickup until the deploy process is finished.
			flNewHealth = m_iHealthOnPickup;
			iRepairAmount = 0;
		}
		else
		{
			// remove the repair cost
			pActivator->RemoveBuildResources( iRepairCost );
		}
		SetHealth( flNewHealth );

		if ( iRepairAmount > 0 )
		{
			if ( pActivator != GetBuilder() )
			{
				pActivator->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_REPAIR_TEAM_GRIND, iRepairAmount );
			}

			// This will spawn a large "+" particle over the object
			if ( bSendEvent )
			{
				IGameEvent * pEvent = gameeventmanager->CreateEvent( "building_healed" );
				if ( pEvent )
				{
					pEvent->SetInt( "priority", 1 ); // HLTV event priority, not transmitted
					pEvent->SetInt( "building", entindex() );
					pEvent->SetInt( "healer", pActivator->entindex() );
					pEvent->SetInt( "amount", iRepairAmount );
					gameeventmanager->FireEvent( pEvent );
				}
			}
		}

		return iRepairAmount;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Upgrade this object a single level
//-----------------------------------------------------------------------------
void CBaseObject::StartUpgrading( void )
{
	// Increase level
	m_iUpgradeLevel++;

	//In Powerup mode, carried level 3 sentries skip the second level when deploying in order to get it done quicker
	if ( m_bCarryDeploy && TFGameRules() && TFGameRules()->IsPowerupMode() )
	{
		m_iUpgradeLevel = GetHighestUpgradeLevel();
	}

	if ( GetHighestUpgradeLevel() < m_iUpgradeLevel )
	{
		m_iHighestUpgradeLevel = m_iUpgradeLevel;
	}

	// more health
	if ( !m_bCarryDeploy && !IsUsingReverseBuild() )
	{
		int iMaxHealth = GetMaxHealthForCurrentLevel();
		SetMaxHealth( iMaxHealth );
		SetHealth( iMaxHealth );
	}

	const char *pUpgradeSound = GetObjectInfo( ObjectType() )->m_pUpgradeSound;
	if ( pUpgradeSound && *pUpgradeSound )
	{
		EmitSound( pUpgradeSound );
	}

	if ( ( !m_bWasMapPlaced || ( m_iUpgradeLevel > (m_nDefaultUpgradeLevel+1) ) ) )
	{
		SetActivity( ACT_OBJ_UPGRADING );

		float flConstructionTime = ( ShouldQuickBuild() ? 0 : GetObjectInfo( ObjectType() )->m_flUpgradeDuration );
		float flReverseBuildingConstructionSpeed = GetReversesBuildingConstructionSpeed();
		flConstructionTime /= ( flReverseBuildingConstructionSpeed == 0.0f ? 1.0f : flReverseBuildingConstructionSpeed );

		m_flUpgradeCompleteTime = gpGlobals->curtime + flConstructionTime;
	}
	else
	{
		m_flUpgradeCompleteTime = gpGlobals->curtime;	//asap
	}

	RemoveAllGestures();

	if ( TFGameRules() && TFGameRules()->IsInTraining() && 
		TFGameRules()->GetTrainingModeLogic() && 
		GetOwner() && GetOwner()->IsFakeClient() == false )
	{
		TFGameRules()->GetTrainingModeLogic()->OnPlayerUpgradedBuilding( GetOwner(), this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::FinishUpgrading( void )
{
	const char *pUpgradeSound = GetObjectInfo( ObjectType() )->m_pUpgradeSound;
	if ( pUpgradeSound && *pUpgradeSound )
	{
		EmitSound( pUpgradeSound );
	}

	if ( IsUsingReverseBuild() )
	{
		m_iUpgradeLevel--;
		DoReverseBuild();
	}
}

//-----------------------------------------------------------------------------
// Playing the upgrade animation
//-----------------------------------------------------------------------------
void CBaseObject::UpgradeThink( void )
{
	if ( gpGlobals->curtime > m_flUpgradeCompleteTime )
	{
		FinishUpgrading();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles health upgrade for objects we've already built
//-----------------------------------------------------------------------------
void CBaseObject::ApplyHealthUpgrade( void )
{
	CTFPlayer *pTFPlayer = GetOwner();
	if ( !pTFPlayer )
		return;

	int iHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iHealth );
	SetHealth( iHealth );

	//DevMsg( "%i\n", GetMaxHealth() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::PlayStartupAnimation( void )
{
	SetActivity( ACT_OBJ_STARTUP );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::DetermineAnimation( void )
{
	Activity desiredActivity = m_Activity;

	switch ( m_Activity )
	{
	default:
		{
			if ( IsUpgrading() )
			{
				desiredActivity = ACT_OBJ_UPGRADING;
			}
			else if ( IsPlacing() )
			{
				/*
				if (1 || m_bPlacementOK )
				{
					desiredActivity = ACT_OBJ_PLACING;
				}
				else
				{
					desiredActivity = ACT_OBJ_IDLE;
				}
				*/
			}
			else if ( IsBuilding() )
			{
				desiredActivity = ACT_OBJ_ASSEMBLING;
			}
			else
			{
				desiredActivity = ACT_OBJ_RUNNING;
			}
		}
		break;
	case ACT_OBJ_STARTUP:
		{
			if ( IsActivityFinished() )
			{
				desiredActivity = ACT_OBJ_RUNNING;
			}
		}
		break;
	}

	if ( desiredActivity == m_Activity )
		return;

	SetActivity( desiredActivity );
}

//-----------------------------------------------------------------------------
// Purpose: Attach this object to the specified object
//-----------------------------------------------------------------------------

void CBaseObject::AttachObjectToObject( CBaseEntity *pEntity, int iPoint, Vector &vecOrigin )
{
	m_hBuiltOnEntity = pEntity;
	m_iBuiltOnPoint = iPoint;

	int iAttachment = 0;

	if ( m_hBuiltOnEntity.Get() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
		if ( pTFPlayer )
		{
			iAttachment = pTFPlayer->LookupAttachment( "head" );
		}
		else
		{
			CBaseAnimating *pAnimate = dynamic_cast<CBaseAnimating*>( pEntity );
			if ( pAnimate )
			{
				iAttachment = pAnimate->LookupBone( "weapon_bone" );
				if ( iAttachment >= 1 )
				{
					FollowEntity( m_hBuiltOnEntity.Get() );
				}
			}
			
			// Parent ourselves to the object
			IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>( pEntity );
			Assert( pBPInterface );
			if ( pBPInterface )
			{
				iAttachment = pBPInterface->GetBuildPointAttachmentIndex( iPoint );

				// re-link to the build points if the sapper is already built
				if ( !( IsPlacing() || IsBuilding() ) )
				{
					pBPInterface->SetObjectOnBuildPoint( m_iBuiltOnPoint, this );
				}
			}
		}

		SetParent( m_hBuiltOnEntity.Get(), iAttachment );

		if ( iAttachment >= 1 )
		{
			// Stick right onto the attachment point.
			vecOrigin.Init();
			SetLocalOrigin( vecOrigin );
			SetLocalAngles( QAngle(0,0,0) );
		}
		else
		{
			SetAbsOrigin( vecOrigin );
			vecOrigin = GetLocalOrigin();
		}

		SetupAttachedVersion();
	}

	Assert( m_hBuiltOnEntity.Get() == GetMoveParent() );
}

//-----------------------------------------------------------------------------
// Purpose: Detach this object from its parent, if it has one
//-----------------------------------------------------------------------------
void CBaseObject::DetachObjectFromObject( void )
{
	if ( !GetParentObject() )
		return;

	// Clear the build point
	IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>(GetParentObject() );
	Assert( pBPInterface );
	pBPInterface->SetObjectOnBuildPoint( m_iBuiltOnPoint, NULL );

	SetParent( NULL );
	m_hBuiltOnEntity = NULL;
	m_iBuiltOnPoint = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn any objects specified inside the mdl
//-----------------------------------------------------------------------------
void CBaseObject::SpawnEntityOnBuildPoint( const char *pEntityName, int iAttachmentNumber )
{
	// Try and spawn the object
	CBaseEntity *pEntity = CreateEntityByName( pEntityName );
	if ( !pEntity )
		return;

	Vector vecOrigin;
	QAngle vecAngles;
	GetAttachment( iAttachmentNumber, vecOrigin, vecAngles );
	pEntity->SetAbsOrigin( vecOrigin );
	pEntity->SetAbsAngles( vecAngles );
	pEntity->Spawn();
	
	// If it's an object, finish setting it up
	CBaseObject *pObject = dynamic_cast<CBaseObject*>(pEntity);
	if ( !pObject )
		return;

	// Add a buildpoint here
	int iPoint = AddBuildPoint( iAttachmentNumber );
	AddValidObjectToBuildPoint( iPoint, pObject->GetType() );
	pObject->SetBuilder( GetBuilder() );
	pObject->ChangeTeam( GetTeamNumber() );
	if ( !(pObject->m_fObjectFlags & OF_DOESNT_HAVE_A_MODEL) )
	{
		pObject->SpawnControlPanels();
	}
	pObject->SetHealth( pObject->GetMaxHealth() );
	pObject->FinishedBuilding();
	pObject->AttachObjectToObject( this, iPoint, vecOrigin );
	//pObject->m_fObjectFlags |= OF_CANNOT_BE_DISMANTLED;

	IHasBuildPoints *pBPInterface = dynamic_cast<IHasBuildPoints*>(this);
	Assert( pBPInterface );
	pBPInterface->SetObjectOnBuildPoint( iPoint, pObject );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn any objects specified inside the mdl
//-----------------------------------------------------------------------------
void CBaseObject::SpawnObjectPoints( void )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( !modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		modelKeyValues->deleteThis();
		return;
	}

	// Do we have a build point section?
	KeyValues *pkvAllObjectPoints = modelKeyValues->FindKey("object_points");
	if ( !pkvAllObjectPoints )
	{
		modelKeyValues->deleteThis();
		return;
	}

	// Start grabbing the sounds and slotting them in
	KeyValues *pkvObjectPoint;
	for ( pkvObjectPoint = pkvAllObjectPoints->GetFirstSubKey(); pkvObjectPoint; pkvObjectPoint = pkvObjectPoint->GetNextKey() )
	{
		// Find the attachment first
		const char *sAttachment = pkvObjectPoint->GetName();
		int iAttachmentNumber = LookupAttachment( sAttachment );
		if ( iAttachmentNumber <= 0 )
		{
			Msg( "ERROR: Model %s specifies object point %s, but has no attachment named %s.\n", STRING(GetModelName()), pkvObjectPoint->GetString(), pkvObjectPoint->GetString() );
			continue;
		}

		// Now see what we're supposed to spawn there
		// The count check is because it seems wrong to emit multiple entities on the same point
		int nCount = 0;
		KeyValues *pkvObject;
		for ( pkvObject = pkvObjectPoint->GetFirstSubKey(); pkvObject; pkvObject = pkvObject->GetNextKey() )
		{
			SpawnEntityOnBuildPoint( pkvObject->GetName(), iAttachmentNumber );
			++nCount;
			Assert( nCount <= 1 );
		}
	}

	modelKeyValues->deleteThis();
}

bool CBaseObject::IsSolidToPlayers( void ) const
{
	switch ( m_SolidToPlayers )
	{
	default:
		break;
	case SOLID_TO_PLAYER_USE_DEFAULT:
		{
			if ( GetObjectInfo( ObjectType() ) )
			{
				return GetObjectInfo( ObjectType() )->m_bSolidToPlayerMovement;
			}
		}
		break;
	case SOLID_TO_PLAYER_YES:
		return true;
	case SOLID_TO_PLAYER_NO:
		return false;
	}

	return false;
}

void CBaseObject::SetSolidToPlayers( OBJSOLIDTYPE stp, bool force )
{
	bool changed = stp != m_SolidToPlayers;
	m_SolidToPlayers = stp;

	if ( changed || force )
	{
		SetCollisionGroup( 
			IsSolidToPlayers() ? 
				TFCOLLISION_GROUP_OBJECT_SOLIDTOPLAYERMOVEMENT : 
				TFCOLLISION_GROUP_OBJECT );
	}
}

int CBaseObject::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ),"Health: %f / %d ( %.1f )", GetHealth(), GetMaxHealth(), (float)GetHealth() / (float)GetMaxHealth() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		CTFPlayer *pBuilder = GetBuilder();

		Q_snprintf( tempstr, sizeof( tempstr ),"Built by: (%d) %s",
			pBuilder ? pBuilder->entindex() : -1,
			pBuilder ? pBuilder->GetPlayerName() : "invalid builder" );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( IsBuilding() )
		{
			Q_snprintf( tempstr, sizeof( tempstr ),"Build Rate: %.1f", GetConstructionMultiplier() );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}
	}
	return text_offset;

}

//-----------------------------------------------------------------------------
// Purpose: Change build orientation
//-----------------------------------------------------------------------------
void CBaseObject::RotateBuildAngles( void )
{
	// rotate the build angles by 90 degrees ( final angle calculated after we network this )
	m_iDesiredBuildRotations++;
	m_iDesiredBuildRotations = m_iDesiredBuildRotations % 4;
}

//-----------------------------------------------------------------------------
// Purpose: called on edge cases to see if we need to change our disabled state
//-----------------------------------------------------------------------------
void CBaseObject::UpdateDisabledState( void )
{
	const bool bShouldBeEnabled = !m_bHasSapper
							   && !m_bPlasmaDisable
							   && (!TFGameRules()->RoundHasBeenWon() || TFGameRules()->GetWinningTeam() == GetTeamNumber());

	SetDisabled( !bShouldBeEnabled );
}

//-----------------------------------------------------------------------------
// Purpose: called when our disabled state changes
//-----------------------------------------------------------------------------
void CBaseObject::SetDisabled( bool bDisabled )
{
	if ( bDisabled && !m_bDisabled )
	{
		OnStartDisabled();
	}
	else if ( !bDisabled && m_bDisabled )
	{
		OnEndDisabled();
	}

	m_bDisabled = bDisabled;
}

//-----------------------------------------------------------------------------
void CBaseObject::SetPlasmaDisabled( float flDuration )
{
	m_bPlasmaDisable = true;
	m_flPlasmaDisableTime = gpGlobals->curtime + flDuration;
	UpdateDisabledState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::OnStartDisabled( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::OnEndDisabled( void )
{
}


//-----------------------------------------------------------------------------
// Purpose: Called when the model changes, find new attachments for the children
//-----------------------------------------------------------------------------
void CBaseObject::ReattachChildren( void )
{
	// Go through and store the children one by one, then reattach them.  We need
	// to store them like this because if we didnt and instead went through and
	// reattached them as we iterated over them we could get into a state where we have
	// children A and B and A has B as a sibling and B has NULL has a sibling,
	//  but we reattach B first which set's B's sibling to A creating a infinite loop
	CUtlVector<CBaseEntity*> vecChildren;
	for (CBaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer())
	{
		if( vecChildren.Find( pChild ) != vecChildren.InvalidIndex() )
		{
			AssertMsg( 0, "Cyclic siblings found when reattaching children!" );
			break;
		}

		vecChildren.AddToTail( pChild );
	}

	int iNumBuildPoints = GetNumBuildPoints();
	FOR_EACH_VEC( vecChildren, i )
	{
		CBaseObject *pObject = dynamic_cast<CBaseObject *>( vecChildren[i] );

		if ( !pObject )
		{
			continue;
		}

		Assert( pObject->GetParent() == this );

		// get the type
		int iObjectType = pObject->GetType();

		bool bReattached = false;

		Vector vecDummy;

		for ( int j = 0; j < iNumBuildPoints && bReattached == false; j++ )
		{
			// Can this object build on this point?
			if ( CanBuildObjectOnBuildPoint( j, iObjectType ) )
			{
				pObject->AttachObjectToObject( this, j, vecDummy );
				bReattached = true;
			}
		}

		// if we can't find an attach for the child, remove it and print an error
		if ( bReattached == false )
		{
			if ( m_bCarried && ( pObject->GetType() == OBJ_ATTACHMENT_SAPPER ) )
			{
				pObject->ResetPlacement();
			}
			else
			{
				pObject->DestroyObject();
				Assert( !"Couldn't find attachment point on upgraded object for existing child.\n" );
			}
		}
	}
}

void CBaseObject::SetModel( const char *pModel )
{
	// Skip if we're already the proper model
	if ( V_strcmp( GetModelName().ToCStr(), pModel ) == 0 )
		return;

	BaseClass::SetModel( pModel );

	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	CObjectSapper *pSapper = GetSapper();
	if ( pSapper )
	{
		pSapper->OnGoActive();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseObject::Activate( void )
{
	BaseClass::Activate();

	InitializeMapPlacedObject();
}

//-----------------------------------------------------------------------------
// Purpose: Map placed objects need to setup here.
//-----------------------------------------------------------------------------
void CBaseObject::InitializeMapPlacedObject( void )
{
	m_bWasMapPlaced = true;
	//m_fObjectFlags |= OF_CANNOT_BE_DISMANTLED;

	// If a map-placed object spawns child objects with their own control
	// panels, all of this lovely code will already have been run
	if ( m_hBuiltOnEntity.Get() )
		return;

	SetBuilder( NULL );

	// NOTE: We must spawn the control panels now, instead of during
	// Spawn, because until placement is started, we don't actually know
	// the position of the control panel because we don't know what it's
	// been attached to (could be a vehicle which supplies a different
	// place for the control panel)

	if ( !(m_fObjectFlags & OF_DOESNT_HAVE_A_MODEL) )
	{
		SpawnControlPanels();
	}

	SetHealth( GetMaxHealth() );

	//AlignToGround( GetAbsOrigin() );
	FinishedBuilding();

	// Set the skin
	m_nSkin = ( GetTeamNumber() == TF_TEAM_RED ) ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: Turns the object into one carried by someone.
//-----------------------------------------------------------------------------
void CBaseObject::MakeCarriedObject( CTFPlayer *pCarrier )
{
	if ( pCarrier )
	{
		// Make the object inactive.
		m_bCarried = true;
		m_bCarryDeploy = false;
		pCarrier->m_Shared.SetCarriedObject( this );
		m_iHealthOnPickup = m_iHealth; // If we are damaged, we want to remember how much damage we had sustained.

		// Remove screens.
		DestroyScreens();

		// Mount it to the player.
		FollowEntity( pCarrier );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_carryobject" );
		if ( event )
		{
			event->SetInt( "userid", pCarrier->GetUserID() );
			event->SetInt( "object", GetType() );
			event->SetInt( "index", entindex() );	// object entity index

			gameeventmanager->FireEvent( event, true );	// don't send to clients
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turns the object into one carried by someone.
//-----------------------------------------------------------------------------
void CBaseObject::DropCarriedObject( CTFPlayer* pCarrier )
{
	m_bCarried = false;
	m_bCarryDeploy = false;

	if ( pCarrier )
	{
		pCarrier->m_Shared.SetCarriedObject( NULL );
	}

	StopFollowingEntity();
}

//-----------------------------------------------------------------------------
// Purpose: Instantly build and upgrade this object
//-----------------------------------------------------------------------------
void CBaseObject::DoQuickBuild( bool bForceMax /* = false */ )
{
	if ( IsBuilding() )
	{
		FinishedBuilding();
	}

	int iTargetLevel = ( ( ( TFGameRules() && TFGameRules()->IsQuickBuildTime() ) || bForceMax ) ? OBJ_MAX_UPGRADE_LEVEL : GetUpgradeLevel() );

	if ( CanBeUpgraded( GetOwner() ) )
	{
		for ( int i = GetUpgradeLevel(); i < iTargetLevel; i++ )
		{
			StartUpgrading();
		}
	}
	else
	{
		int iMaxHealth = GetMaxHealthForCurrentLevel();
		SetMaxHealth( iMaxHealth );
		SetHealth( iMaxHealth );
	}
}

//-----------------------------------------------------------------------------
// Builds instantly under certain conditions/modes
//-----------------------------------------------------------------------------
bool CBaseObject::ShouldQuickBuild( void )
{
	if ( TFGameRules() )
	{
		if ( GetType() == OBJ_ATTACHMENT_SAPPER )
			return false;


		if ( TFGameRules()->IsQuickBuildTime() )
			return true;

		if ( TFGameRules()->IsMannVsMachineMode() )
		{
			if ( GetTeamNumber() == TF_TEAM_PVE_INVADERS )
				// Engineer bots in MvM deploy pre-built sentries that build up at the normal rate
				return m_bForceQuickBuild;

			if ( m_bCarryDeploy || TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS )
				return true;
		}
	}

	return m_bForceQuickBuild;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
float CBaseObject::GetUpgradeDuration( void )
{
	if ( ShouldQuickBuild() )
		return 1.f;

	return GetObjectInfo( ObjectType() )->m_flUpgradeDuration;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CBaseObject::DoReverseBuild( void )
{
	m_iHighestUpgradeLevel = m_iUpgradeLevel;
	m_iUpgradeMetal = 0;

	int iMaxHealth = GetMaxHealthForCurrentLevel();
	SetMaxHealth( iMaxHealth );
	if ( GetHealth() > iMaxHealth )
	{
		SetHealth( iMaxHealth );
	}

	if ( m_iUpgradeLevel > 1 )
	{
		m_iUpgradeLevel--;
		StartUpgrading();
	}
	else
	{
		m_bBuilding = true;
		m_bCarryDeploy = false;
		m_flTotalConstructionTime = m_flConstructionTimeLeft = GetTotalTime();
		m_flConstructionStartTime = gpGlobals->curtime;
		SetStartBuildingModel();
		SetControlPanelsActive( false );
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
float CBaseObject::GetReversesBuildingConstructionSpeed( void )
{
	CObjectSapper *pSapper = GetSapper();
	if ( !pSapper )
		return 0.0f;

	return pSapper->GetReversesBuildingConstructionSpeed();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
int	CBaseObject::GetUpgradeAmountPerHit( void )
{
	int nAmount = tf_obj_upgrade_per_hit.GetInt();

	if ( TFGameRules()->InSetup() || TFGameRules()->IsPowerupMode() )
	{
		nAmount *= 2;
	}

	return nAmount;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseObject::InputEnable( inputdata_t &inputdata )
{
	if ( IsDisabled() )
	{
		UpdateDisabledState();
		if ( !IsDisabled() )
		{
			OnGoActive();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseObject::InputDisable( inputdata_t &inputdata )
{
	if ( !IsDisabled() )
	{
		SetDisabled( true );
		OnGoInactive();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CBaseObject::GetMaxHealthForCurrentLevel( void )
{
	int iMaxHealth = IsMiniBuilding() ? GetMiniBuildingStartingHealth() : GetBaseHealth();
	if ( GetOwner() && !m_bDisposableBuilding )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( GetOwner(), iMaxHealth, mult_engy_building_health );
	}
	
	if ( !IsMiniBuilding() && ( GetUpgradeLevel() > 1 ) )
	{
		float flMultiplier = pow( UPGRADE_LEVEL_HEALTH_MULTIPLIER, GetUpgradeLevel() - 1 );
		iMaxHealth = (int)( iMaxHealth * flMultiplier );
	}

	return iMaxHealth;
}
