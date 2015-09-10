//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The base class from which all game entities are derived.
//
//===========================================================================//

#include "cbase.h"
#include "globalstate.h"
#include "isaverestore.h"
#include "client.h"
#include "decals.h"
#include "gamerules.h"
#include "entityapi.h"
#include "entitylist.h"
#include "eventqueue.h"
#include "hierarchy.h"
#include "basecombatweapon.h"
#include "const.h"
#include "player.h"		// For debug draw sending
#include "ndebugoverlay.h"
#include "physics.h"
#include "model_types.h"
#include "team.h"
#include "sendproxy.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "baseentity.h"
#include "collisionutils.h"
#include "coordsize.h"
#include "animation.h"
#include "tier1/strtools.h"
#include "engine/IEngineSound.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "bone_setup.h"
#include "vcollide_parse.h"
#include "filters.h"
#include "te_effect_dispatch.h"
#include "AI_Criteria.h"
#include "AI_ResponseSystem.h"
#include "world.h"
#include "globals.h"
#include "saverestoretypes.h"
#include "SkyCamera.h"
#include "sceneentity.h"
#include "game.h"
#include "tier0/vprof.h"
#include "ai_basenpc.h"
#include "iservervehicle.h"
#include "eventlist.h"
#include "scriptevent.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "UtlCachedFileData.h"
#include "utlbuffer.h"
#include "positionwatcher.h"
#include "movetype_push.h"
#include "tier0/icommandline.h"
#include "vphysics/friction.h"
#include <ctype.h>
#include "datacache/imdlcache.h"
#include "ModelSoundsCache.h"
#include "env_debughistory.h"
#include "tier1/utlstring.h"
#include "utlhashtable.h"

#if defined( TF_DLL )
#include "tf_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bTestMoveTypeStepSimulation;
extern ConVar sv_vehicle_autoaim_scale;

// Init static class variables
bool CBaseEntity::m_bInDebugSelect = false;	// Used for selection in debug overlays
int CBaseEntity::m_nDebugPlayer = -1;		// Player doing the selection

// This can be set before creating an entity to force it to use a particular edict.
edict_t *g_pForceAttachEdict = NULL;

bool CBaseEntity::m_bDebugPause = false;		// Whether entity i/o is paused.
int CBaseEntity::m_nDebugSteps = 1;				// Number of entity outputs to fire before pausing again.
bool CBaseEntity::sm_bDisableTouchFuncs = false;	// Disables PhysicsTouch and PhysicsStartTouch function calls
bool CBaseEntity::sm_bAccurateTriggerBboxChecks = true;	// set to false for legacy behavior in ep1

int CBaseEntity::m_nPredictionRandomSeed = -1;
int CBaseEntity::m_nPredictionRandomSeedServer = -1;
CBasePlayer *CBaseEntity::m_pPredictionPlayer = NULL;

// Used to make sure nobody calls UpdateTransmitState directly.
int g_nInsideDispatchUpdateTransmitState = 0;

// When this is false, throw an assert in debug when GetAbsAnything is called. Used when hierachy is incomplete/invalid.
bool CBaseEntity::s_bAbsQueriesValid = true;


ConVar sv_netvisdist( "sv_netvisdist", "10000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Test networking visibility distance" );

// This table encodes edict data.
void SendProxy_AnimTime( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;

#if defined( _DEBUG )
	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();
	Assert( pAnimating );

	if ( pAnimating )
	{
		Assert( !pAnimating->IsUsingClientSideAnimation() );
	}
#endif
	
	int ticknumber = TIME_TO_TICKS( pEntity->m_flAnimTime );
	// Tickbase is current tick rounded down to closes 100 ticks
	int tickbase = gpGlobals->GetNetworkBase( gpGlobals->tickcount, pEntity->entindex() );
	int addt = 0;
	// If it's within the last tick interval through the current one, then we can encode it
	if ( ticknumber >= ( tickbase - 100 ) )
	{
		addt = ( ticknumber - tickbase ) & 0xFF;
	}

	pOut->m_Int = addt;
}

// This table encodes edict data.
void SendProxy_SimulationTime( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;

	int ticknumber = TIME_TO_TICKS( pEntity->m_flSimulationTime );
	// tickbase is current tick rounded down to closest 100 ticks
	int tickbase = gpGlobals->GetNetworkBase( gpGlobals->tickcount, pEntity->entindex() );
	int addt = 0;
	if ( ticknumber >= tickbase )
	{
		addt = ( ticknumber - tickbase ) & 0xff;
	}

	pOut->m_Int = addt;
}

void* SendProxy_ClientSideAnimation( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;
	CBaseAnimating *pAnimating = pEntity->GetBaseAnimating();

	if ( pAnimating && !pAnimating->IsUsingClientSideAnimation() )
		return (void*)pVarData;
	else
		return NULL;	// Don't send animtime unless the client needs it.
}	
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_ClientSideAnimation );


BEGIN_SEND_TABLE_NOBASE( CBaseEntity, DT_AnimTimeMustBeFirst )
	// NOTE:  Animtime must be sent before origin and angles ( from pev ) because it has a 
	//  proxy on the client that stores off the old values before writing in the new values and
	//  if it is sent after the new values, then it will only have the new origin and studio model, etc.
	//  interpolation will be busted
	SendPropInt	(SENDINFO(m_flAnimTime), 8, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN|SPROP_ENCODED_AGAINST_TICKCOUNT, SendProxy_AnimTime),
END_SEND_TABLE()

#if !defined( NO_ENTITY_PREDICTION )
BEGIN_SEND_TABLE_NOBASE( CBaseEntity, DT_PredictableId )
	SendPropPredictableId( SENDINFO( m_PredictableID ) ),
	SendPropInt( SENDINFO( m_bIsPlayerSimulated ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()


static void* SendProxy_SendPredictableId( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	CBaseEntity *pEntity = (CBaseEntity *)pStruct;
	if ( !pEntity || !pEntity->m_PredictableID->IsActive() )
		return NULL;

	int id_player_index = pEntity->m_PredictableID->GetPlayer();
	pRecipients->SetOnly( id_player_index );
	
	return ( void * )pVarData;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendPredictableId );
#endif

void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *entity = (CBaseEntity*)pStruct;
	Assert( entity );

	const Vector *v;

	if ( !entity->UseStepSimulationNetworkOrigin( &v ) )
	{
		v = &entity->GetLocalOrigin();
	}

	pOut->m_Vector[ 0 ] = v->x;
	pOut->m_Vector[ 1 ] = v->y;
	pOut->m_Vector[ 2 ] = v->z;
}

//--------------------------------------------------------------------------------------------------------
// Used when breaking up origin, note we still have to deal with StepSimulation
//--------------------------------------------------------------------------------------------------------
void SendProxy_OriginXY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *entity = (CBaseEntity*)pStruct;
	Assert( entity );

	const Vector *v;

	if ( !entity->UseStepSimulationNetworkOrigin( &v ) )
	{
		v = &entity->GetLocalOrigin();
	}

	pOut->m_Vector[ 0 ] = v->x;
	pOut->m_Vector[ 1 ] = v->y;
}

//--------------------------------------------------------------------------------------------------------
// Used when breaking up origin, note we still have to deal with StepSimulation
//--------------------------------------------------------------------------------------------------------
void SendProxy_OriginZ( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *entity = (CBaseEntity*)pStruct;
	Assert( entity );

	const Vector *v;

	if ( !entity->UseStepSimulationNetworkOrigin( &v ) )
	{
		v = &entity->GetLocalOrigin();
	}

	pOut->m_Float = v->z;
}


void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CBaseEntity *entity = (CBaseEntity*)pStruct;
	Assert( entity );

	const QAngle *a;

	if ( !entity->UseStepSimulationNetworkAngles( &a ) )
	{
		a = &entity->GetLocalAngles();
	}

	pOut->m_Vector[ 0 ] = anglemod( a->x );
	pOut->m_Vector[ 1 ] = anglemod( a->y );
	pOut->m_Vector[ 2 ] = anglemod( a->z );
}

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE( CBaseEntity, DT_BaseEntity )
	SendPropDataTable( "AnimTimeMustBeFirst", 0, &REFERENCE_SEND_TABLE(DT_AnimTimeMustBeFirst), SendProxy_ClientSideAnimation ),
	SendPropInt			(SENDINFO(m_flSimulationTime),	SIMULATION_TIME_WINDOW_BITS, SPROP_UNSIGNED|SPROP_CHANGES_OFTEN|SPROP_ENCODED_AGAINST_TICKCOUNT, SendProxy_SimulationTime),

#if PREDICTION_ERROR_CHECK_LEVEL > 1 
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
#else
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
#endif

	SendPropInt		(SENDINFO( m_ubInterpolationFrame ), NOINTERP_PARITY_MAX_BITS, SPROP_UNSIGNED ),
	SendPropModelIndex(SENDINFO(m_nModelIndex)),
	SendPropDataTable( SENDINFO_DT( m_Collision ), &REFERENCE_SEND_TABLE(DT_CollisionProperty) ),
	SendPropInt		(SENDINFO(m_nRenderFX),		8, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_nRenderMode),	8, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_fEffects),		EF_MAX_BITS, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_clrRender),	32, SPROP_UNSIGNED),
	SendPropInt		(SENDINFO(m_iTeamNum),		TEAMNUM_NUM_BITS, 0),
	SendPropInt		(SENDINFO(m_CollisionGroup), 5, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flElasticity), 0, SPROP_COORD),
	SendPropFloat	(SENDINFO(m_flShadowCastDistance), 12, SPROP_UNSIGNED ),
	SendPropEHandle (SENDINFO(m_hOwnerEntity)),
	SendPropEHandle (SENDINFO(m_hEffectEntity)),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
	SendPropInt		(SENDINFO(m_iParentAttachment), NUM_PARENTATTACHMENT_BITS, SPROP_UNSIGNED),

	SendPropInt		(SENDINFO_NAME( m_MoveType, movetype ), MOVETYPE_MAX_BITS, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO_NAME( m_MoveCollide, movecollide ), MOVECOLLIDE_MAX_BITS, SPROP_UNSIGNED ),
#if PREDICTION_ERROR_CHECK_LEVEL > 1 
	SendPropVector	(SENDINFO(m_angRotation), -1, SPROP_NOSCALE|SPROP_CHANGES_OFTEN, 0, HIGH_DEFAULT, SendProxy_Angles ),
#else
	SendPropQAngles	(SENDINFO(m_angRotation), 13, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
#endif

	SendPropInt		( SENDINFO( m_iTextureFrameIndex ),		8, SPROP_UNSIGNED ),

#if !defined( NO_ENTITY_PREDICTION )
	SendPropDataTable( "predictable_id", 0, &REFERENCE_SEND_TABLE( DT_PredictableId ), SendProxy_SendPredictableId ),
#endif

	// FIXME: Collapse into another flag field?
	SendPropInt		(SENDINFO(m_bSimulatedEveryTick),		1, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_bAnimatedEveryTick),		1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bAlternateSorting )),

#ifdef TF_DLL
	SendPropArray3( SENDINFO_ARRAY3(m_nModelIndexOverrides), SendPropInt( SENDINFO_ARRAY(m_nModelIndexOverrides), SP_MODEL_INDEX_BITS, 0 ) ),
#endif

END_SEND_TABLE()


// dynamic models
class CBaseEntityModelLoadProxy
{
protected:
	class Handler : public IModelLoadCallback
	{
	public:
		explicit Handler( CBaseEntity *pEntity ) : m_pEntity(pEntity) { }
		virtual void OnModelLoadComplete( const model_t *pModel );
		CBaseEntity* m_pEntity;
	};
	Handler* m_pHandler;

public:
	explicit CBaseEntityModelLoadProxy( CBaseEntity *pEntity ) : m_pHandler( new Handler( pEntity ) ) { }
	~CBaseEntityModelLoadProxy() { delete m_pHandler; }
	void Register( int nModelIndex ) const { modelinfo->RegisterModelLoadCallback( nModelIndex, m_pHandler ); }
	operator CBaseEntity * () const { return m_pHandler->m_pEntity; }

private:
	CBaseEntityModelLoadProxy( const CBaseEntityModelLoadProxy& );
	CBaseEntityModelLoadProxy& operator=( const CBaseEntityModelLoadProxy& );
};

static CUtlHashtable< CBaseEntityModelLoadProxy, empty_t, PointerHashFunctor, PointerEqualFunctor, CBaseEntity * > sg_DynamicLoadHandlers;

void CBaseEntityModelLoadProxy::Handler::OnModelLoadComplete( const model_t *pModel )
{
	m_pEntity->OnModelLoadComplete( pModel );
	sg_DynamicLoadHandlers.Remove( m_pEntity ); // NOTE: destroys *this!
}


CBaseEntity::CBaseEntity( bool bServerOnly )
{
	m_pAttributes = NULL;

	COMPILE_TIME_ASSERT( MOVETYPE_LAST < (1 << MOVETYPE_MAX_BITS) );
	COMPILE_TIME_ASSERT( MOVECOLLIDE_COUNT < (1 << MOVECOLLIDE_MAX_BITS) );

#ifdef _DEBUG
	// necessary since in debug, we initialize vectors to NAN for debugging
	m_vecAngVelocity.Init();
//	m_vecAbsAngVelocity.Init();
	m_vecViewOffset.Init();
	m_vecBaseVelocity.GetForModify().Init();
	m_vecVelocity.Init();
	m_vecAbsVelocity.Init();
#endif

	m_bAlternateSorting = false;
	m_CollisionGroup = COLLISION_GROUP_NONE;
	m_iParentAttachment = 0;
	CollisionProp()->Init( this );
	NetworkProp()->Init( this );

	// NOTE: THIS MUST APPEAR BEFORE ANY SetMoveType() or SetNextThink() calls
	AddEFlags( EFL_NO_THINK_FUNCTION | EFL_NO_GAME_PHYSICS_SIMULATION | EFL_USE_PARTITION_WHEN_NOT_SOLID );

	// clear debug overlays
	m_debugOverlays  = 0;
	m_pTimedOverlay  = NULL;
	m_pPhysicsObject = NULL;
	m_flElasticity   = 1.0f;
	m_flShadowCastDistance = m_flDesiredShadowCastDistance = 0;
	SetRenderColor( 255, 255, 255, 255 );
	m_iTeamNum = m_iInitialTeamNum = TEAM_UNASSIGNED;
	m_nLastThinkTick = gpGlobals->tickcount;
	m_nSimulationTick = -1;
	SetIdentityMatrix( m_rgflCoordinateFrame );
	m_pBlocker = NULL;
#if _DEBUG
	m_iCurrentThinkContext = NO_THINK_CONTEXT;
#endif
	m_nWaterTouch = m_nSlimeTouch = 0;

	SetSolid( SOLID_NONE );
	ClearSolidFlags();

	m_nModelIndex = 0;
	m_bDynamicModelAllowed = false;
	m_bDynamicModelPending = false;
	m_bDynamicModelSetBounds = false;

	SetMoveType( MOVETYPE_NONE );
	SetOwnerEntity( NULL );
	SetCheckUntouch( false );
	SetModelIndex( 0 );
	SetModelName( NULL_STRING );
	m_nTransmitStateOwnedCounter = 0;

	SetCollisionBounds( vec3_origin, vec3_origin );
	ClearFlags();

	SetFriction( 1.0f );

	if ( bServerOnly )
	{
		AddEFlags( EFL_SERVER_ONLY );
	}
	NetworkProp()->MarkPVSInformationDirty();

#ifndef _XBOX
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Scale up our physics hull and test against the new one
// Input  : *pNewCollide - New collision hull
//-----------------------------------------------------------------------------
void CBaseEntity::SetScaledPhysics( IPhysicsObject *pNewObject )
{
	if ( pNewObject )
	{
		AddSolidFlags( FSOLID_CUSTOMBOXTEST | FSOLID_CUSTOMRAYTEST );
	}
	else
	{
		RemoveSolidFlags( FSOLID_CUSTOMBOXTEST | FSOLID_CUSTOMRAYTEST );
	}
}

extern bool g_bDisableEhandleAccess;

//-----------------------------------------------------------------------------
// Purpose: See note below
//-----------------------------------------------------------------------------
CBaseEntity::~CBaseEntity( )
{
	// FIXME: This can't be called from UpdateOnRemove! There's at least one
	// case where friction sounds are added between the call to UpdateOnRemove + ~CBaseEntity
	PhysCleanupFrictionSounds( this );

	Assert( !IsDynamicModelIndex( m_nModelIndex ) );
	Verify( !sg_DynamicLoadHandlers.Remove( this ) );

	// In debug make sure that we don't call delete on an entity without setting
	//  the disable flag first!
	// EHANDLE accessors will check, in debug, for access to entities during destruction of
	//  another entity.
	// That kind of operation should only occur in UpdateOnRemove calls
	// Deletion should only occur via UTIL_Remove(Immediate) calls, not via naked delete calls
	Assert( g_bDisableEhandleAccess );

	VPhysicsDestroyObject();

	// Need to remove references to this entity before EHANDLES go null
	{
		g_bDisableEhandleAccess = false;
		CBaseEntity::PhysicsRemoveTouchedList( this );
		CBaseEntity::PhysicsRemoveGroundList( this );
		SetGroundEntity( NULL ); // remove us from the ground entity if we are on it
		DestroyAllDataObjects();
		g_bDisableEhandleAccess = true;

		// Remove this entity from the ent list (NOTE:  This Makes EHANDLES go NULL)
		gEntList.RemoveEntity( GetRefEHandle() );
	}
}

void CBaseEntity::PostConstructor( const char *szClassname )
{
	if ( szClassname )
	{
		SetClassname(szClassname);
	}

	Assert( m_iClassname != NULL_STRING && STRING(m_iClassname) != NULL );

	// Possibly get an edict, and add self to global list of entites.
	if ( IsEFlagSet( EFL_SERVER_ONLY ) )
	{
		gEntList.AddNonNetworkableEntity( this );
	}
	else
	{
		// Certain entities set up their edicts in the constructor
		if ( !IsEFlagSet( EFL_NO_AUTO_EDICT_ATTACH ) )
		{
			NetworkProp()->AttachEdict( g_pForceAttachEdict );
			g_pForceAttachEdict = NULL;
		}
		
		// Some ents like the player override the AttachEdict function and do it at a different time.
		// While precaching, they don't ever have an edict, so we don't need to add them to
		// the entity list in that case.
		if ( edict() )
		{
			gEntList.AddNetworkableEntity( this, entindex() );
			
			// Cache our IServerNetworkable pointer for the engine for fast access.
			if ( edict() )
				edict()->m_pNetworkable = NetworkProp();
		}
	}

	CheckHasThinkFunction( false );
	CheckHasGamePhysicsSimulation();
}

//-----------------------------------------------------------------------------
// Purpose: Called after player becomes active in the game
//-----------------------------------------------------------------------------
void CBaseEntity::PostClientActive( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Verifies that this entity's data description is valid in debug builds.
//-----------------------------------------------------------------------------
#ifdef _DEBUG
typedef CUtlVector< const char * >	KeyValueNameList_t;

static void AddDataMapFieldNamesToList( KeyValueNameList_t &list, datamap_t *pDataMap )
{
	while (pDataMap != NULL)
	{
		for (int i = 0; i < pDataMap->dataNumFields; i++)
		{
			typedescription_t *pField = &pDataMap->dataDesc[i];

			if (pField->fieldType == FIELD_EMBEDDED)
			{
				AddDataMapFieldNamesToList( list, pField->td );
				continue;
			}

			if (pField->flags & FTYPEDESC_KEY)
			{
				list.AddToTail( pField->externalName );
			}
		}
	
		pDataMap = pDataMap->baseMap;
	}
}

void CBaseEntity::ValidateDataDescription(void)
{
	// Multiple key fields that have the same name are not allowed - it creates an
	// ambiguity when trying to parse keyvalues and outputs.
	datamap_t *pDataMap = GetDataDescMap();
	if ((pDataMap == NULL) || pDataMap->bValidityChecked)
		return;

	pDataMap->bValidityChecked = true;

	// Let's generate a list of all keyvalue strings in the entire hierarchy...
	KeyValueNameList_t	names(128);
	AddDataMapFieldNamesToList( names, pDataMap );

	for (int i = names.Count(); --i > 0; )
	{
		for (int j = i - 1; --j >= 0; )
		{
			if (!Q_stricmp(names[i], names[j]))
			{
				DevMsg( "%s has multiple data description entries for \"%s\"\n", STRING(m_iClassname), names[i]);
				break;
			}
		}
	}
}
#endif // _DEBUG


//-----------------------------------------------------------------------------
// Sets the collision bounds + the size
//-----------------------------------------------------------------------------
void CBaseEntity::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	m_Collision.SetCollisionBounds( mins, maxs );
}


void CBaseEntity::StopFollowingEntity( )
{
	if( !IsFollowingEntity() )
	{
//		Assert( IsEffectActive( EF_BONEMERGE ) == 0 );
		return;
	}

	SetParent( NULL );
	RemoveEffects( EF_BONEMERGE );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	CollisionRulesChanged();
}

bool CBaseEntity::IsFollowingEntity()
{
	return IsEffectActive( EF_BONEMERGE ) && (GetMoveType() == MOVETYPE_NONE) && GetMoveParent();
}

CBaseEntity *CBaseEntity::GetFollowedEntity()
{
	if (!IsFollowingEntity())
		return NULL;
	return GetMoveParent();
}

void CBaseEntity::SetClassname( const char *className )
{
	m_iClassname = AllocPooledString( className );
}

void CBaseEntity::SetModelIndex( int index )
{
	if ( IsDynamicModelIndex( index ) && !(GetBaseAnimating() && m_bDynamicModelAllowed) )
	{
		AssertMsg( false, "dynamic model support not enabled on server entity" );
		index = -1;
	}

	if ( index != m_nModelIndex )
	{
		if ( m_bDynamicModelPending )
		{
			sg_DynamicLoadHandlers.Remove( this );
		}
		
		modelinfo->ReleaseDynamicModel( m_nModelIndex );
		modelinfo->AddRefDynamicModel( index );
		m_nModelIndex = index;
		
		m_bDynamicModelSetBounds = false;

		if ( IsDynamicModelIndex( index ) )
		{
			m_bDynamicModelPending = true;
			sg_DynamicLoadHandlers[ sg_DynamicLoadHandlers.Insert( this ) ].Register( index );
		}
		else
		{
			m_bDynamicModelPending = false;
			OnNewModel();
		}
	}
	DispatchUpdateTransmitState();
}

void CBaseEntity::ClearModelIndexOverrides( void )
{
#ifdef TF_DLL
	for ( int index = 0 ; index < MAX_VISION_MODES ; index++ )
	{
		m_nModelIndexOverrides.Set( index, 0 );
	}
#endif
}

void CBaseEntity::SetModelIndexOverride( int index, int nValue )
{
#ifdef TF_DLL
	if ( ( index >= VISION_MODE_NONE ) && ( index < MAX_VISION_MODES ) )
	{
		if ( nValue != m_nModelIndexOverrides[index] )
		{
			m_nModelIndexOverrides.Set( index, nValue );
		}	
	}
#endif
}
	  
// position to shoot at
Vector CBaseEntity::BodyTarget( const Vector &posSrc, bool bNoisy) 
{ 
	return WorldSpaceCenter( ); 
}

// return the position of my head. someone's trying to attack it.
Vector CBaseEntity::HeadTarget( const Vector &posSrc )
{
	return EyePosition();
}


struct TimedOverlay_t
{
	char 			*msg;
	int				msgEndTime;
	int				msgStartTime;
	TimedOverlay_t	*pNextTimedOverlay; 
};

//-----------------------------------------------------------------------------
// Purpose: Display an error message on the entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::AddTimedOverlay( const char *msg, int endTime )
{
	TimedOverlay_t *pNewTO = new TimedOverlay_t;
	int len = strlen(msg);
	pNewTO->msg = new char[len + 1];
	Q_strncpy(pNewTO->msg,msg, len+1);
	pNewTO->msgEndTime = gpGlobals->curtime + endTime;
	pNewTO->msgStartTime = gpGlobals->curtime;
	pNewTO->pNextTimedOverlay = m_pTimedOverlay;
	m_pTimedOverlay = pNewTO;
}

//-----------------------------------------------------------------------------
// Purpose: Send debug overlay box to the client
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CBaseEntity::DrawBBoxOverlay( float flDuration )
{
	if (edict())
	{
		NDebugOverlay::EntityBounds(this, 255, 100, 0, 0, flDuration );

		if ( CollisionProp()->IsSolidFlagSet( FSOLID_USE_TRIGGER_BOUNDS ) )
		{
			Vector vecTriggerMins, vecTriggerMaxs;
			CollisionProp()->WorldSpaceTriggerBounds( &vecTriggerMins, &vecTriggerMaxs );
			Vector center = 0.5f * (vecTriggerMins + vecTriggerMaxs);
			Vector extents = vecTriggerMaxs - center;
			NDebugOverlay::Box(center, -extents, extents, 0, 255, 255, 0, flDuration );
		}
	}
}


void CBaseEntity::DrawAbsBoxOverlay()
{
	int red = 0;
	int green = 200;

	if ( VPhysicsGetObject() && VPhysicsGetObject()->IsAsleep() )
	{
		red = 90;
		green = 120;
	}

	if (edict())
	{
		// Surrounding boxes are axially aligned, so ignore angles
		Vector vecSurroundMins, vecSurroundMaxs;
		CollisionProp()->WorldSpaceSurroundingBounds( &vecSurroundMins, &vecSurroundMaxs );
		Vector center = 0.5f * (vecSurroundMins + vecSurroundMaxs);
		Vector extents = vecSurroundMaxs - center;
		NDebugOverlay::Box(center, -extents, extents, red, green, 0, 0 ,0);
	}
}

void CBaseEntity::DrawRBoxOverlay()
{	

}

//-----------------------------------------------------------------------------
// Purpose: Draws an axis overlay at the origin and angles of the entity
//-----------------------------------------------------------------------------
void CBaseEntity::SendDebugPivotOverlay( void )
{
	if ( edict() )
	{
		NDebugOverlay::Axis( GetAbsOrigin(), GetAbsAngles(), 20, true, 0 );
	}
}

//------------------------------------------------------------------------------
// Purpose : Add new entity positioned overlay text
// Input   : How many lines to offset text from origin
//			 The text to print
//			 How long to display text
//			 The color of the text
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::EntityText( int text_offset, const char *text, float duration, int r, int g, int b, int a )
{
	Vector origin;
	Vector vecLocalCenter;

	VectorAdd( m_Collision.OBBMins(), m_Collision.OBBMaxs(), vecLocalCenter );
	vecLocalCenter *= 0.5f;

	if ( ( m_Collision.GetCollisionAngles() == vec3_angle ) || ( vecLocalCenter == vec3_origin ) )
	{
		VectorAdd( vecLocalCenter, m_Collision.GetCollisionOrigin(), origin );
	}
	else
	{
		VectorTransform( vecLocalCenter, m_Collision.CollisionToWorldTransform(), origin );
	}

	NDebugOverlay::EntityTextAtPosition( origin, text_offset, text, duration, r, g, b, a );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawTimedOverlays(void)
{
	// Draw name first if I have an overlay or am in message mode
	if ((m_debugOverlays & OVERLAY_MESSAGE_BIT))
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof( tempstr ), "[%s]", GetDebugName() );
		EntityText(0,tempstr, 0);
	}
	
	// Now draw overlays
	TimedOverlay_t* pTO		= m_pTimedOverlay;
	TimedOverlay_t* pNextTO = NULL;
	TimedOverlay_t* pLastTO = NULL;
	int				nCount	= 1;	// Offset by one
	while (pTO)
	{
		pNextTO = pTO->pNextTimedOverlay;

		// Remove old messages unless messages are paused
		if ((!CBaseEntity::Debug_IsPaused() && gpGlobals->curtime > pTO->msgEndTime) ||
			(nCount > 10))
		{
			if (pLastTO)
			{
				pLastTO->pNextTimedOverlay = pNextTO;
			}
			else
			{
				m_pTimedOverlay = pNextTO;
			}

			delete pTO->msg;
			delete pTO;
		}
		else
		{
			int nAlpha = 0;
			
			// If messages aren't paused fade out
			if (!CBaseEntity::Debug_IsPaused())
			{
				nAlpha = 255*((gpGlobals->curtime - pTO->msgStartTime)/(pTO->msgEndTime - pTO->msgStartTime));
			}
			int r = 185;
			int g = 145;
			int b = 145;

			// Brighter when new message
			if (nAlpha < 50)
			{
				r = 255;
				g = 205;
				b = 205;
			}
			if (nAlpha < 0) nAlpha = 0;
			EntityText(nCount,pTO->msg, 0.0, r, g, b, 255-nAlpha);
			nCount++;

			pLastTO = pTO;
		}
		pTO	= pNextTO;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw all overlays (should be implemented by subclass to add
//			any additional non-text overlays)
// Input  :
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
void CBaseEntity::DrawDebugGeometryOverlays(void) 
{
	DrawTimedOverlays();
	DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_NAME_BIT) 
	{	
		EntityText(0,GetDebugName(), 0);
	}
	if (m_debugOverlays & OVERLAY_BBOX_BIT) 
	{	
		DrawBBoxOverlay();
	}
	if (m_debugOverlays & OVERLAY_ABSBOX_BIT )
	{
		DrawAbsBoxOverlay();
	}
	if (m_debugOverlays & OVERLAY_PIVOT_BIT) 
	{	
		SendDebugPivotOverlay();
	}
	if( m_debugOverlays & OVERLAY_RBOX_BIT )
	{
		DrawRBoxOverlay();
	}
	if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT) )
	{
		// draw mass center
		if ( VPhysicsGetObject() )
		{
			Vector massCenter = VPhysicsGetObject()->GetMassCenterLocalSpace();
			Vector worldPos;
			VPhysicsGetObject()->LocalToWorld( &worldPos, massCenter );
			NDebugOverlay::Cross3D( worldPos, 12, 255, 0, 0, false, 0 );
			DebugDrawContactPoints(VPhysicsGetObject());
			if ( GetMoveType() != MOVETYPE_VPHYSICS )
			{
				Vector pos;
				QAngle angles;
				VPhysicsGetObject()->GetPosition( &pos, &angles );
				float dist = (pos - GetAbsOrigin()).Length();

				Vector axis;
				float deltaAngle;
				RotationDeltaAxisAngle( angles, GetAbsAngles(), axis, deltaAngle );
				if ( dist > 2 || fabsf(deltaAngle) > 2 )
				{
					Vector mins, maxs;
					physcollision->CollideGetAABB( &mins, &maxs, VPhysicsGetObject()->GetCollide(), vec3_origin, vec3_angle );
					NDebugOverlay::BoxAngles( pos, mins, maxs, angles, 255, 255, 0, 16, 0 );
				}
			}
		}
	}
	if ( m_debugOverlays & OVERLAY_SHOW_BLOCKSLOS )
	{
		if ( BlocksLOS() )
		{
			NDebugOverlay::EntityBounds(this, 255, 255, 255, 0, 0 );
		}
	}
	if ( m_debugOverlays & OVERLAY_AUTOAIM_BIT && (GetFlags()&FL_AIMTARGET) && AI_GetSinglePlayer() != NULL )
	{
		// Crude, but it gets the point across.
		Vector vecCenter = GetAutoAimCenter();
		Vector vecRight, vecUp, vecDiag;
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		float radius = GetAutoAimRadius();

		QAngle angles = pPlayer->EyeAngles();
		AngleVectors( angles, NULL, &vecRight, &vecUp );

		int r,g,b;

		if( ((int)gpGlobals->curtime) % 2 == 1 )
		{
			r = 255; 
			g = 255;
			b = 255;

			if( pPlayer->GetActiveWeapon() != NULL )
				radius *= pPlayer->GetActiveWeapon()->WeaponAutoAimScale();

		}
		else
		{
			r = 255;g=0;b=0;

			if( !ShouldAttractAutoAim(pPlayer) )
			{
				g = 255;
			}
		}

		if( pPlayer->IsInAVehicle() )
		{
			radius *= sv_vehicle_autoaim_scale.GetFloat();
		}

		NDebugOverlay::Line( vecCenter, vecCenter + vecRight * radius, r, g, b, true, 0.1 );
		NDebugOverlay::Line( vecCenter, vecCenter - vecRight * radius, r, g, b, true, 0.1 );
		NDebugOverlay::Line( vecCenter, vecCenter + vecUp * radius, r, g, b, true, 0.1 );
		NDebugOverlay::Line( vecCenter, vecCenter - vecUp * radius, r, g, b, true, 0.1 );

		vecDiag = vecRight + vecUp;
		VectorNormalize( vecDiag );
		NDebugOverlay::Line( vecCenter - vecDiag * radius, vecCenter + vecDiag * radius, r, g, b, true, 0.1 );

		vecDiag = vecRight - vecUp;
		VectorNormalize( vecDiag );
		NDebugOverlay::Line( vecCenter - vecDiag * radius, vecCenter + vecDiag * radius, r, g, b, true, 0.1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays (override in subclass to add additional text)
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CBaseEntity::DrawDebugTextOverlays(void) 
{
	int offset = 1;
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr, sizeof(tempstr), "(%d) Name: %s (%s)", entindex(), GetDebugName(), GetClassname() );
		EntityText(offset,tempstr, 0);
		offset++;

		if( m_iGlobalname != NULL_STRING )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "GLOBALNAME: %s", STRING(m_iGlobalname) );
			EntityText(offset,tempstr, 0);
			offset++;
		}

		Vector vecOrigin = GetAbsOrigin();
		Q_snprintf( tempstr, sizeof(tempstr), "Position: %0.1f, %0.1f, %0.1f\n", vecOrigin.x, vecOrigin.y, vecOrigin.z );
		EntityText( offset, tempstr, 0 );
		offset++;

		if( GetModelName() != NULL_STRING || GetBaseAnimating() )
		{
			Q_snprintf(tempstr, sizeof(tempstr), "Model:%s", STRING(GetModelName()) );
			EntityText(offset,tempstr,0);
			offset++;
		}

		if( m_hDamageFilter.Get() != NULL )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "DAMAGE FILTER:%s", m_hDamageFilter->GetDebugName() );
			EntityText( offset,tempstr,0 );
			offset++;
		}
	}

	if (m_debugOverlays & OVERLAY_VIEWOFFSET)
	{	
		NDebugOverlay::Cross3D( EyePosition(), 16, 255, 0, 0, true, 0.05f );
	}

	return offset;
}


void CBaseEntity::SetParent( string_t newParent, CBaseEntity *pActivator, int iAttachment )
{
	// find and notify the new parent
	CBaseEntity *pParent = gEntList.FindEntityByName( NULL, newParent, NULL, pActivator );

	// debug check
	if ( newParent != NULL_STRING && pParent == NULL )
	{
		Msg( "Entity %s(%s) has bad parent %s\n", STRING(m_iClassname), GetDebugName(), STRING(newParent) );
	}
	else
	{
		// make sure there isn't any ambiguity
		if ( gEntList.FindEntityByName( pParent, newParent, NULL, pActivator ) )
		{
			Msg( "Entity %s(%s) has ambigious parent %s\n", STRING(m_iClassname), GetDebugName(), STRING(newParent) );
		}
		SetParent( pParent, iAttachment );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move our points from parent to worldspace
// Input  : *pParent - Parent to use as reference
//-----------------------------------------------------------------------------
void CBaseEntity::TransformStepData_ParentToWorld( CBaseEntity *pParent )
{
	// Fix up our step simulation points to be in the proper local space
	StepSimulationData *step = (StepSimulationData *) GetDataObject( STEPSIMULATION );
	if ( step != NULL )
	{
		// Convert our positions
		UTIL_ParentToWorldSpace( pParent, step->m_Previous2.vecOrigin, step->m_Previous2.qRotation );
		UTIL_ParentToWorldSpace( pParent, step->m_Previous.vecOrigin, step->m_Previous.qRotation );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move step data between two parent-spaces
// Input  : *pOldParent - parent we were attached to
//			*pNewParent - parent we're now attached to
//-----------------------------------------------------------------------------
void CBaseEntity::TransformStepData_ParentToParent( CBaseEntity *pOldParent, CBaseEntity *pNewParent )
{
	// Fix up our step simulation points to be in the proper local space
	StepSimulationData *step = (StepSimulationData *) GetDataObject( STEPSIMULATION );
	if ( step != NULL )
	{
		// Convert our positions
		UTIL_ParentToWorldSpace( pOldParent, step->m_Previous2.vecOrigin, step->m_Previous2.qRotation );
		UTIL_WorldToParentSpace( pNewParent, step->m_Previous2.vecOrigin, step->m_Previous2.qRotation );
		
		UTIL_ParentToWorldSpace( pOldParent, step->m_Previous.vecOrigin, step->m_Previous.qRotation );
		UTIL_WorldToParentSpace( pNewParent, step->m_Previous.vecOrigin, step->m_Previous.qRotation );
	}
}

//-----------------------------------------------------------------------------
// Purpose: After parenting to an object, we need to also correctly translate our
//			step stimulation positions and angles into that parent space.  Otherwise
//			we end up splining between two different world spaces.
//-----------------------------------------------------------------------------
void CBaseEntity::TransformStepData_WorldToParent( CBaseEntity *pParent )
{
	// Fix up our step simulation points to be in the proper local space
	StepSimulationData *step = (StepSimulationData *) GetDataObject( STEPSIMULATION );
	if ( step != NULL )
	{
		// Convert our positions
		UTIL_WorldToParentSpace( pParent, step->m_Previous2.vecOrigin, step->m_Previous2.qRotation );
		UTIL_WorldToParentSpace( pParent, step->m_Previous.vecOrigin, step->m_Previous.qRotation );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the movement parent of this entity. This entity will be moved
//			to a local coordinate calculated from its current absolute offset
//			from the parent entity and will then follow the parent entity.
// Input  : pParentEntity - This entity's new parent in the movement hierarchy.
//-----------------------------------------------------------------------------
void CBaseEntity::SetParent( CBaseEntity *pParentEntity, int iAttachment )
{
	// If they didn't specify an attachment, use our current
	if ( iAttachment == -1 )
	{
		iAttachment = m_iParentAttachment;
	}

	bool bWasNotParented = ( GetParent() == NULL );
	CBaseEntity *pOldParent = m_pParent;

	// notify the old parent of the loss
	UnlinkFromParent( this );

	// set the new name
	m_pParent = pParentEntity;

	if ( m_pParent == this )
	{
		// should never set parent to 'this' - makes no sense
		Assert(0);
		m_pParent = NULL;
	}
	
	if ( m_pParent == NULL )
	{
		m_iParent = NULL_STRING;

		// Transform step data from parent to worldspace
		TransformStepData_ParentToWorld( pOldParent );
		return;
	}

	m_iParent = m_pParent->m_iName;

	RemoveSolidFlags( FSOLID_ROOT_PARENT_ALIGNED );
	if ( pParentEntity )
	{
		if ( const_cast<CBaseEntity *>(pParentEntity)->GetRootMoveParent()->GetSolid() == SOLID_BSP )
		{
			AddSolidFlags( FSOLID_ROOT_PARENT_ALIGNED );
		}
		else
		{
			if ( GetSolid() == SOLID_BSP )
			{
				// Must be SOLID_VPHYSICS because parent might rotate
				SetSolid( SOLID_VPHYSICS );
			}
		}
	}
	// set the move parent if we have one
	if ( edict() )
	{
		// add ourselves to the list
		LinkChild( m_pParent, this );

		m_iParentAttachment = (char)iAttachment;

		EntityMatrix matrix, childMatrix;
		matrix.InitFromEntity( const_cast<CBaseEntity *>(pParentEntity), m_iParentAttachment ); // parent->world
		childMatrix.InitFromEntityLocal( this ); // child->world
		Vector localOrigin = matrix.WorldToLocal( GetLocalOrigin() );
		
		// I have the axes of local space in world space. (childMatrix)
		// I want to compute those world space axes in the parent's local space
		// and set that transform (as angles) on the child's object so the net
		// result is that the child is now in parent space, but still oriented the same way
		VMatrix tmp = matrix.Transpose(); // world->parent
		tmp.MatrixMul( childMatrix, matrix ); // child->parent
		QAngle angles;
		MatrixToAngles( matrix, angles );
		SetLocalAngles( angles );
		UTIL_SetOrigin( this, localOrigin );

		// Move our step data into the correct space
		if ( bWasNotParented )
		{
			// Transform step data from world to parent-space
			TransformStepData_WorldToParent( this );
		}
		else
		{
			// Transform step data between parent-spaces
			TransformStepData_ParentToParent( pOldParent, this );
		}
	}
	if ( VPhysicsGetObject() )
	{
		if ( VPhysicsGetObject()->IsStatic())
		{
			if ( VPhysicsGetObject()->IsAttachedToConstraint(false) )
			{
				Warning("SetParent on static object, all constraints attached to %s (%s)will now be broken!\n", GetDebugName(), GetClassname() );
			}
			VPhysicsDestroyObject();
			VPhysicsInitShadow(false, false);
		}
	}
	CollisionRulesChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::ValidateEntityConnections()
{
	if ( m_target == NULL_STRING )
		return;

	if ( ClassMatches( "scripted_*" )			||
		 ClassMatches( "trigger_relay" )		||
		 ClassMatches( "trigger_auto" )			||
		 ClassMatches( "path_*" )				||
		 ClassMatches( "monster_*" )			||
		 ClassMatches( "trigger_teleport" )		||
		 ClassMatches( "func_train" )			||
		 ClassMatches( "func_tracktrain" )		||
		 ClassMatches( "func_plat*" )			||
		 ClassMatches( "npc_*" )				||
		 ClassMatches( "info_big*" )			||
		 ClassMatches( "env_texturetoggle" )	||
		 ClassMatches( "env_render" )			||
		 ClassMatches( "func_areaportalwindow")	||
		 ClassMatches( "point_view*")			||
		 ClassMatches( "func_traincontrols" )	||
		 ClassMatches( "multisource" )			||
		 ClassMatches( "xen_plant*" ) )
		return;

	datamap_t *dmap = GetDataDescMap();
	while ( dmap )
	{
		int fields = dmap->dataNumFields;
		for ( int i = 0; i < fields; i++ )
		{
			typedescription_t *dataDesc = &dmap->dataDesc[i];
			if ( ( dataDesc->fieldType == FIELD_CUSTOM ) && ( dataDesc->flags & FTYPEDESC_OUTPUT ) )
			{
				CBaseEntityOutput *pOutput = (CBaseEntityOutput *)((int)this + (int)dataDesc->fieldOffset[0]);
				if ( pOutput->NumberOfElements() )
					return;
			}
		}

		dmap = dmap->baseMap;
	}

	Vector vecLoc = WorldSpaceCenter();
	Warning("---------------------------------\n");
	Warning( "Entity %s - (%s) has a target and NO OUTPUTS\n", GetDebugName(), GetClassname() );
	Warning( "Location %f %f %f\n", vecLoc.x, vecLoc.y, vecLoc.z );
	Warning("---------------------------------\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::FireNamedOutput( const char *pszOutput, variant_t variant, CBaseEntity *pActivator, CBaseEntity *pCaller, float flDelay )
{
	if ( pszOutput == NULL )
		return;

	datamap_t *dmap = GetDataDescMap();
	while ( dmap )
	{
		int fields = dmap->dataNumFields;
		for ( int i = 0; i < fields; i++ )
		{
			typedescription_t *dataDesc = &dmap->dataDesc[i];
			if ( ( dataDesc->fieldType == FIELD_CUSTOM ) && ( dataDesc->flags & FTYPEDESC_OUTPUT ) )
			{
				CBaseEntityOutput *pOutput = ( CBaseEntityOutput * )( ( int )this + ( int )dataDesc->fieldOffset[0] );
				if ( !Q_stricmp( dataDesc->externalName, pszOutput ) )
				{
					pOutput->FireOutput( variant, pActivator, pCaller, flDelay );
					return;
				}
			}
		}

		dmap = dmap->baseMap;
	}
}

void CBaseEntity::Activate( void )
{
#ifdef DEBUG
	extern bool g_bCheckForChainedActivate;
	extern bool g_bReceivedChainedActivate;

	if ( g_bCheckForChainedActivate && g_bReceivedChainedActivate )
	{
		Assert( !"Multiple calls to base class Activate()\n" );
	}
	g_bReceivedChainedActivate = true;
#endif

	// NOTE: This forces a team change so that stuff in the level
	// that starts out on a team correctly changes team
	if (m_iInitialTeamNum)
	{
		ChangeTeam( m_iInitialTeamNum );
	}	

	// Get a handle to my damage filter entity if there is one.
	if ( m_iszDamageFilterName != NULL_STRING )
	{
		m_hDamageFilter = gEntList.FindEntityByName( NULL, m_iszDamageFilterName );
	}

	// Add any non-null context strings to our context vector
	if ( m_iszResponseContext != NULL_STRING ) 
	{
		AddContext( m_iszResponseContext.ToCStr() );
	}

#ifdef HL1_DLL
	ValidateEntityConnections();
#endif //HL1_DLL
}

////////////////////////////  old CBaseEntity stuff ///////////////////////////////////


// give health. 
// Returns the amount of health actually taken.
int CBaseEntity::TakeHealth( float flHealth, int bitsDamageType )
{
	if ( !edict() || m_takedamage < DAMAGE_YES )
		return 0;

	int iMax = GetMaxHealth();

// heal
	if ( m_iHealth >= iMax )
		return 0;

	const int oldHealth = m_iHealth;

	m_iHealth += flHealth;

	if (m_iHealth > iMax)
		m_iHealth = iMax;

	return m_iHealth - oldHealth;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

int CBaseEntity::OnTakeDamage( const CTakeDamageInfo &info )
{
	Vector			vecTemp;

	if ( !edict() || !m_takedamage )
		return 0;

	if ( info.GetInflictor() )
	{
		vecTemp = info.GetInflictor()->WorldSpaceCenter() - ( WorldSpaceCenter() );
	}
	else
	{
		vecTemp.Init( 1, 0, 0 );
	}

	// this global is still used for glass and other non-NPC killables, along with decals.
	g_vecAttackDir = vecTemp;
	VectorNormalize(g_vecAttackDir);
		
	// save damage based on the target's armor level

	// figure momentum add (don't let hurt brushes or other triggers move player)

	// physics objects have their own calcs for this: (don't let fire move things around!)
	if ( !IsEFlagSet( EFL_NO_DAMAGE_FORCES ) )
	{
		if ( ( GetMoveType() == MOVETYPE_VPHYSICS ) )
		{
			VPhysicsTakeDamage( info );
		}
		else
		{
			if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK || GetMoveType() == MOVETYPE_STEP) && 
				!info.GetAttacker()->IsSolidFlagSet(FSOLID_TRIGGER) )
			{
				Vector vecDir, vecInflictorCentroid;
				vecDir = WorldSpaceCenter( );
				vecInflictorCentroid = info.GetInflictor()->WorldSpaceCenter( );
				vecDir -= vecInflictorCentroid;
				VectorNormalize( vecDir );

				float flForce = info.GetDamage() * ((32 * 32 * 72.0) / (WorldAlignSize().x * WorldAlignSize().y * WorldAlignSize().z)) * 5;
				
				if (flForce > 1000.0) 
					flForce = 1000.0;
				ApplyAbsVelocityImpulse( vecDir * flForce );
			}
		}
	}

	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
	// do the damage
		m_iHealth -= info.GetDamage();
		if (m_iHealth <= 0)
		{
			Event_Killed( info );
			return 0;
		}
	}

	return 1;
}

//-----------------------------------------------------------------------------
// Purpose: Scale damage done and call OnTakeDamage
//-----------------------------------------------------------------------------
int CBaseEntity::TakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( !g_pGameRules )
		return 0;

	bool bHasPhysicsForceDamage = !g_pGameRules->Damage_NoPhysicsForce( inputInfo.GetDamageType() );
	if ( bHasPhysicsForceDamage && inputInfo.GetDamageType() != DMG_GENERIC )
	{
		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.

		if ( inputInfo.GetDamageForce() == vec3_origin || inputInfo.GetDamagePosition() == vec3_origin )
		{
			static int warningCount = 0;
			if ( ++warningCount < 10 )
			{
				if ( inputInfo.GetDamageForce() == vec3_origin )
				{
					DevWarning( "CBaseEntity::TakeDamage:  with inputInfo.GetDamageForce() == vec3_origin\n" );
				}
				if ( inputInfo.GetDamagePosition() == vec3_origin )
				{
					DevWarning( "CBaseEntity::TakeDamage:  with inputInfo.GetDamagePosition() == vec3_origin\n" );
				}
			}
		}
	}

	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( inputInfo ))
	{
		return 0;
	}

	if( !g_pGameRules->AllowDamage(this, inputInfo) )
	{
		return 0;
	}

	if ( PhysIsInCallback() )
	{
		PhysCallbackDamage( this, inputInfo );
	}
	else
	{
		CTakeDamageInfo info = inputInfo;
		
		// Scale the damage by the attacker's modifier.
		if ( info.GetAttacker() )
		{
			info.ScaleDamage( info.GetAttacker()->GetAttackDamageScale( this ) );
		}

		// Scale the damage by my own modifiers
		info.ScaleDamage( GetReceivedDamageScale( info.GetAttacker() ) );

		//Msg("%s took %.2f Damage, at %.2f\n", GetClassname(), info.GetDamage(), gpGlobals->curtime );

		return OnTakeDamage( info );
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done by this entity.
//-----------------------------------------------------------------------------
float CBaseEntity::GetAttackDamageScale( CBaseEntity *pVictim )
{
	float flScale = 1;
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( !m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
	return flScale;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a value that scales all damage done to this entity
//-----------------------------------------------------------------------------
float CBaseEntity::GetReceivedDamageScale( CBaseEntity *pAttacker )
{
	float flScale = 1;
	FOR_EACH_LL( m_DamageModifiers, i )
	{
		if ( m_DamageModifiers[i]->IsDamageDoneToMe() )
		{
			flScale *= m_DamageModifiers[i]->GetModifier();
		}
	}
	return flScale;
}


//-----------------------------------------------------------------------------
// Purpose: Applies forces to our physics object in response to damage.
//-----------------------------------------------------------------------------
int CBaseEntity::VPhysicsTakeDamage( const CTakeDamageInfo &info )
{
	// don't let physics impacts or fire cause objects to move (again)
	bool bNoPhysicsForceDamage = g_pGameRules->Damage_NoPhysicsForce( info.GetDamageType() );
	if ( bNoPhysicsForceDamage || info.GetDamageType() == DMG_GENERIC )
		return 1;

	Assert(VPhysicsGetObject() != NULL);
	if ( VPhysicsGetObject() )
	{
		Vector force = info.GetDamageForce();
		Vector offset = info.GetDamagePosition();

		// If you hit this assert, you've called TakeDamage with a damage type that requires a physics damage
		// force & position without specifying one or both of them. Decide whether your damage that's causing 
		// this is something you believe should impart physics force on the receiver. If it is, you need to 
		// setup the damage force & position inside the CTakeDamageInfo (Utility functions for this are in
		// takedamageinfo.cpp. If you think the damage shouldn't cause force (unlikely!) then you can set the 
		// damage type to DMG_GENERIC, or | DMG_CRUSH if you need to preserve the damage type for purposes of HUD display.
#if !defined( TF_DLL )
		Assert( force != vec3_origin && offset != vec3_origin );
#else
		// this was spamming the console for Payload maps in TF (trigger_hurt entity on the front of the cart)
		if ( !TFGameRules() || TFGameRules()->GetGameType() != TF_GAMETYPE_ESCORT )
		{
			Assert( force != vec3_origin && offset != vec3_origin );
		}
#endif

		unsigned short gameFlags = VPhysicsGetObject()->GetGameFlags();
		if ( gameFlags & FVPHYSICS_PLAYER_HELD )
		{
			// if the player is holding the object, use it's real mass (player holding reduced the mass)
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				float mass = pPlayer->GetHeldObjectMass( VPhysicsGetObject() );
				if ( mass != 0.0f )
				{
					float ratio = VPhysicsGetObject()->GetMass() / mass;
					force *= ratio;
				}
			}
		}
		else if ( (gameFlags & FVPHYSICS_PART_OF_RAGDOLL) && (gameFlags & FVPHYSICS_CONSTRAINT_STATIC) )
		{
			IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
			int count = VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
			for ( int i = 0; i < count; i++ )
			{
				if ( !(pList[i]->GetGameFlags() & FVPHYSICS_CONSTRAINT_STATIC) )
				{
					pList[i]->ApplyForceOffset( force, offset );
					return 1;
				}
			}

		}
		VPhysicsGetObject()->ApplyForceOffset( force, offset );
	}

	return 1;
}

	// Character killed (only fired once)
void CBaseEntity::Event_Killed( const CTakeDamageInfo &info )
{
	if( info.GetAttacker() )
	{
		info.GetAttacker()->Event_KilledOther(this, info);
	}

	m_takedamage = DAMAGE_NO;
	m_lifeState = LIFE_DEAD;
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: helper method to send a game event when this entity is killed.  Note:
//			gets called specifically for particular entities (mostly NPC), this
//			does not get called for every entity
//-----------------------------------------------------------------------------
void CBaseEntity::SendOnKilledGameEvent( const CTakeDamageInfo &info )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "entity_killed" );
	if ( event )
	{
		event->SetInt( "entindex_killed", entindex() );
		if ( info.GetAttacker())
		{
			event->SetInt( "entindex_attacker", info.GetAttacker()->entindex() );
		}
		if ( info.GetInflictor())
		{
			event->SetInt( "entindex_inflictor", info.GetInflictor()->entindex() );
		}		
		event->SetInt( "damagebits", info.GetDamageType() );
		gameeventmanager->FireEvent( event );
	}
}


bool CBaseEntity::HasTarget( string_t targetname )
{
	if( targetname != NULL_STRING && m_target != NULL_STRING )
		return FStrEq(STRING(targetname), STRING(m_target) );
	else
		return false;
}


CBaseEntity *CBaseEntity::GetNextTarget( void )
{
	if ( !m_target )
		return NULL;
	return gEntList.FindEntityByName( NULL, m_target );
}

class CThinkContextsSaveDataOps : public CDefSaveRestoreOps
{
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CThinkContextsSaveDataOps does not support arrays");

		// Write out the vector
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		SaveUtlVector( pSave, pUtlVector, FIELD_EMBEDDED );

		// Get our owner
		CBaseEntity *pOwner = (CBaseEntity*)fieldInfo.pOwner;

		pSave->StartBlock();
		// Now write out all the functions
		for ( int i = 0; i < pUtlVector->Size(); i++ )
		{
#ifdef WIN32
			void **ppV = (void**)&((*pUtlVector)[i].m_pfnThink);
#else
			BASEPTR *ppV = &((*pUtlVector)[i].m_pfnThink);
#endif
			bool bHasFunc = (*ppV != NULL);
			pSave->WriteBool( &bHasFunc, 1 );
			if ( bHasFunc )
			{
				pSave->WriteFunction( pOwner->GetDataDescMap(), "m_pfnThink", (inputfunc_t **)ppV, 1 );
			}
		}
		pSave->EndBlock();
	}

	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CThinkContextsSaveDataOps does not support arrays");

		// Read in the vector
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		RestoreUtlVector( pRestore, pUtlVector, FIELD_EMBEDDED );

		// Get our owner
		CBaseEntity *pOwner = (CBaseEntity*)fieldInfo.pOwner;

		pRestore->StartBlock();
		// Now read in all the functions
		for ( int i = 0; i < pUtlVector->Size(); i++ )
		{
			bool bHasFunc;
			pRestore->ReadBool( &bHasFunc, 1 );
#ifdef WIN32
			void **ppV = (void**)&((*pUtlVector)[i].m_pfnThink);
#else
			BASEPTR *ppV = &((*pUtlVector)[i].m_pfnThink);
			Q_memset( (void *)ppV, 0x0, sizeof(inputfunc_t) );
#endif
			if ( bHasFunc )
			{
				SaveRestoreRecordHeader_t header;
				pRestore->ReadHeader( &header );
				pRestore->ReadFunction( pOwner->GetDataDescMap(), (inputfunc_t **)ppV, 1, header.size );
			}
			else
			{
				*ppV = NULL;
			}
		}
		pRestore->EndBlock();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlVector< thinkfunc_t > *pUtlVector = (CUtlVector< thinkfunc_t > *)fieldInfo.pField;
		return ( pUtlVector->Count() == 0 );
	}

	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		BASEPTR pFunc = *((BASEPTR*)fieldInfo.pField);
		pFunc = NULL;
	}
};
CThinkContextsSaveDataOps g_ThinkContextsSaveDataOps;
ISaveRestoreOps *thinkcontextFuncs = &g_ThinkContextsSaveDataOps;

BEGIN_SIMPLE_DATADESC( thinkfunc_t )

	DEFINE_FIELD( m_iszContext,	FIELD_STRING ),
	// DEFINE_FIELD( m_pfnThink,		FIELD_FUNCTION ),		// Manually written
	DEFINE_FIELD( m_nNextThinkTick,	FIELD_TICK	),
	DEFINE_FIELD( m_nLastThinkTick,	FIELD_TICK	),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( ResponseContext_t )

	DEFINE_FIELD( m_iszName,			FIELD_STRING ),
	DEFINE_FIELD( m_iszValue,			FIELD_STRING ),
	DEFINE_FIELD( m_fExpirationTime,	FIELD_TIME ),

END_DATADESC()

BEGIN_DATADESC_NO_BASE( CBaseEntity )

	DEFINE_KEYFIELD( m_iClassname, FIELD_STRING, "classname" ),
	DEFINE_GLOBAL_KEYFIELD( m_iGlobalname, FIELD_STRING, "globalname" ),
	DEFINE_KEYFIELD( m_iParent, FIELD_STRING, "parentname" ),

	DEFINE_KEYFIELD( m_iHammerID, FIELD_INTEGER, "hammerid" ), // save ID numbers so that entities can be tracked between save/restore and vmf

	DEFINE_KEYFIELD( m_flSpeed, FIELD_FLOAT, "speed" ),
	DEFINE_KEYFIELD( m_nRenderFX, FIELD_CHARACTER, "renderfx" ),
	DEFINE_KEYFIELD( m_nRenderMode, FIELD_CHARACTER, "rendermode" ),

	// Consider moving to CBaseAnimating?
	DEFINE_FIELD( m_flPrevAnimTime, FIELD_TIME ),
	DEFINE_FIELD( m_flAnimTime, FIELD_TIME ),
	DEFINE_FIELD( m_flSimulationTime, FIELD_TIME ),
	DEFINE_FIELD( m_nLastThinkTick, FIELD_TICK ),

	DEFINE_KEYFIELD( m_nNextThinkTick, FIELD_TICK, "nextthink" ),
	DEFINE_KEYFIELD( m_fEffects, FIELD_INTEGER, "effects" ),
	DEFINE_KEYFIELD( m_clrRender, FIELD_COLOR32, "rendercolor" ),
	DEFINE_GLOBAL_KEYFIELD( m_nModelIndex, FIELD_SHORT, "modelindex" ),
#if !defined( NO_ENTITY_PREDICTION )
	// DEFINE_FIELD( m_PredictableID, CPredictableId ),
#endif
	DEFINE_FIELD( touchStamp, FIELD_INTEGER ),
	DEFINE_CUSTOM_FIELD( m_aThinkFunctions, thinkcontextFuncs ),
	//								m_iCurrentThinkContext (not saved, debug field only, and think transient to boot)

	DEFINE_UTLVECTOR(m_ResponseContexts,		FIELD_EMBEDDED),
	DEFINE_KEYFIELD( m_iszResponseContext, FIELD_STRING, "ResponseContext" ),

	DEFINE_FIELD( m_pfnThink, FIELD_FUNCTION ),
	DEFINE_FIELD( m_pfnTouch, FIELD_FUNCTION ),
	DEFINE_FIELD( m_pfnUse, FIELD_FUNCTION ),
	DEFINE_FIELD( m_pfnBlocked, FIELD_FUNCTION ),
	DEFINE_FIELD( m_pfnMoveDone, FIELD_FUNCTION ),

	DEFINE_FIELD( m_lifeState, FIELD_CHARACTER ),
	DEFINE_FIELD( m_takedamage, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_iMaxHealth, FIELD_INTEGER, "max_health" ),
	DEFINE_KEYFIELD( m_iHealth, FIELD_INTEGER, "health" ),
	// DEFINE_FIELD( m_pLink, FIELD_CLASSPTR ),
	DEFINE_KEYFIELD( m_target, FIELD_STRING, "target" ),

	DEFINE_KEYFIELD( m_iszDamageFilterName, FIELD_STRING, "damagefilter" ),
	DEFINE_FIELD( m_hDamageFilter, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_debugOverlays, FIELD_INTEGER ),

	DEFINE_GLOBAL_FIELD( m_pParent, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iParentAttachment, FIELD_CHARACTER ),
	DEFINE_GLOBAL_FIELD( m_hMoveParent, FIELD_EHANDLE ),
	DEFINE_GLOBAL_FIELD( m_hMoveChild, FIELD_EHANDLE ),
	DEFINE_GLOBAL_FIELD( m_hMovePeer, FIELD_EHANDLE ),
	
	DEFINE_FIELD( m_iEFlags, FIELD_INTEGER ),

	DEFINE_FIELD( m_iName, FIELD_STRING ),
	DEFINE_EMBEDDED( m_Collision ),
	DEFINE_EMBEDDED( m_Network ),

	DEFINE_FIELD( m_MoveType, FIELD_CHARACTER ),
	DEFINE_FIELD( m_MoveCollide, FIELD_CHARACTER ),
	DEFINE_FIELD( m_hOwnerEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_CollisionGroup, FIELD_INTEGER ),
	DEFINE_PHYSPTR( m_pPhysicsObject),
	DEFINE_FIELD( m_flElasticity, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_flShadowCastDistance, FIELD_FLOAT, "shadowcastdist" ),
	DEFINE_FIELD( m_flDesiredShadowCastDistance, FIELD_FLOAT ),

	DEFINE_INPUT( m_iInitialTeamNum, FIELD_INTEGER, "TeamNum" ),
	DEFINE_FIELD( m_iTeamNum, FIELD_INTEGER ),

//	DEFINE_FIELD( m_bSentLastFrame, FIELD_INTEGER ),

	DEFINE_FIELD( m_hGroundEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flGroundChangeTime, FIELD_TIME ),
	DEFINE_GLOBAL_KEYFIELD( m_ModelName, FIELD_MODELNAME, "model" ),
	
	DEFINE_KEYFIELD( m_vecBaseVelocity, FIELD_VECTOR, "basevelocity" ),
	DEFINE_FIELD( m_vecAbsVelocity, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_vecAngVelocity, FIELD_VECTOR, "avelocity" ),
//	DEFINE_FIELD( m_vecAbsAngVelocity, FIELD_VECTOR ),
	DEFINE_ARRAY( m_rgflCoordinateFrame, FIELD_FLOAT, 12 ), // NOTE: MUST BE IN LOCAL SPACE, NOT POSITION_VECTOR!!! (see CBaseEntity::Restore)

	DEFINE_KEYFIELD( m_nWaterLevel, FIELD_CHARACTER, "waterlevel" ),
	DEFINE_FIELD( m_nWaterType, FIELD_CHARACTER ),
	DEFINE_FIELD( m_pBlocker, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_flGravity, FIELD_FLOAT, "gravity" ),
	DEFINE_KEYFIELD( m_flFriction, FIELD_FLOAT, "friction" ),

	// Local time is local to each object.  It doesn't need to be re-based if the clock
	// changes.  Therefore it is saved as a FIELD_FLOAT, not a FIELD_TIME
	DEFINE_KEYFIELD( m_flLocalTime, FIELD_FLOAT, "ltime" ),
	DEFINE_FIELD( m_flVPhysicsUpdateLocalTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMoveDoneTime, FIELD_FLOAT ),

//	DEFINE_FIELD( m_nPushEnumCount, FIELD_INTEGER ),

	DEFINE_FIELD( m_vecAbsOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( m_vecVelocity, FIELD_VECTOR, "velocity" ),
	DEFINE_KEYFIELD( m_iTextureFrameIndex, FIELD_CHARACTER, "texframeindex" ),
	DEFINE_FIELD( m_bSimulatedEveryTick, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAnimatedEveryTick, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAlternateSorting, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_spawnflags, FIELD_INTEGER, "spawnflags" ),
	DEFINE_FIELD( m_nTransmitStateOwnedCounter, FIELD_CHARACTER ),
	DEFINE_FIELD( m_angAbsRotation, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecOrigin, FIELD_VECTOR ),			// NOTE: MUST BE IN LOCAL SPACE, NOT POSITION_VECTOR!!! (see CBaseEntity::Restore)
	DEFINE_FIELD( m_angRotation, FIELD_VECTOR ),

	DEFINE_KEYFIELD( m_vecViewOffset, FIELD_VECTOR, "view_ofs" ),

	DEFINE_FIELD( m_fFlags, FIELD_INTEGER ),
#if !defined( NO_ENTITY_PREDICTION )
//	DEFINE_FIELD( m_bIsPlayerSimulated, FIELD_INTEGER ),
//	DEFINE_FIELD( m_hPlayerSimulationOwner, FIELD_EHANDLE ),
#endif
	// DEFINE_FIELD( m_pTimedOverlay, TimedOverlay_t* ),
	DEFINE_FIELD( m_nSimulationTick, FIELD_TICK ),
	// DEFINE_FIELD( m_RefEHandle, CBaseHandle ),

//	DEFINE_FIELD( m_nWaterTouch,		FIELD_INTEGER ),
//	DEFINE_FIELD( m_nSlimeTouch,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flNavIgnoreUntilTime,	FIELD_TIME ),

//	DEFINE_FIELD( m_bToolRecording,		FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_ToolHandle,		FIELD_INTEGER ),

	// NOTE: This is tricky. TeamNum must be saved, but we can't directly
	// read it in, because we can only set it after the team entity has been read in,
	// which may or may not actually occur before the entity is parsed.
	// Therefore, we set the TeamNum from the InitialTeamNum in Activate
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTeam", InputSetTeam ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KillHierarchy", InputKillHierarchy ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Use", InputUse ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "Alpha", InputAlpha ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "AlternativeSorting", InputAlternativeSorting ),
	DEFINE_INPUTFUNC( FIELD_COLOR32, "Color", InputColor ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetParent", InputSetParent ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetParentAttachment", InputSetParentAttachment ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetParentAttachmentMaintainOffset", InputSetParentAttachmentMaintainOffset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearParent", InputClearParent ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetDamageFilter", InputSetDamageFilter ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableDamageForces", InputEnableDamageForces ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableDamageForces", InputDisableDamageForces ),

	DEFINE_INPUTFUNC( FIELD_STRING, "DispatchEffect", InputDispatchEffect ),
	DEFINE_INPUTFUNC( FIELD_STRING, "DispatchResponse", InputDispatchResponse ),

	// Entity I/O methods to alter context
	DEFINE_INPUTFUNC( FIELD_STRING, "AddContext", InputAddContext ),
	DEFINE_INPUTFUNC( FIELD_STRING, "RemoveContext", InputRemoveContext ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearContext", InputClearContext ),

	DEFINE_INPUTFUNC( FIELD_VOID, "DisableShadow", InputDisableShadow ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableShadow", InputEnableShadow ),

	DEFINE_INPUTFUNC( FIELD_STRING, "AddOutput", InputAddOutput ),

	DEFINE_INPUTFUNC( FIELD_STRING, "FireUser1", InputFireUser1 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FireUser2", InputFireUser2 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FireUser3", InputFireUser3 ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FireUser4", InputFireUser4 ),

	DEFINE_OUTPUT( m_OnUser1, "OnUser1" ),
	DEFINE_OUTPUT( m_OnUser2, "OnUser2" ),
	DEFINE_OUTPUT( m_OnUser3, "OnUser3" ),
	DEFINE_OUTPUT( m_OnUser4, "OnUser4" ),

	// Function Pointers
	DEFINE_FUNCTION( SUB_Remove ),
	DEFINE_FUNCTION( SUB_DoNothing ),
	DEFINE_FUNCTION( SUB_StartFadeOut ),
	DEFINE_FUNCTION( SUB_StartFadeOutInstant ),
	DEFINE_FUNCTION( SUB_FadeOut ),
	DEFINE_FUNCTION( SUB_Vanish ),
	DEFINE_FUNCTION( SUB_CallUseToggle ),
	DEFINE_THINKFUNC( ShadowCastDistThink ),

	DEFINE_FIELD( m_hEffectEntity, FIELD_EHANDLE ),

	//DEFINE_FIELD( m_DamageModifiers, FIELD_?? ), // can't save?
	// DEFINE_FIELD( m_fDataObjectTypes, FIELD_INTEGER ),

#ifdef TF_DLL
	DEFINE_ARRAY( m_nModelIndexOverrides, FIELD_INTEGER, MAX_VISION_MODES ),
#endif

END_DATADESC()

// For code error checking
extern bool g_bReceivedChainedUpdateOnRemove;

//-----------------------------------------------------------------------------
// Purpose: Called just prior to object destruction
//  Entities that need to unlink themselves from other entities should do the unlinking
//  here rather than in their destructor.  The reason why is that when the global entity list
//  is told to Clear(), it first takes a pass through all active entities and calls UTIL_Remove
//  on each such entity.  Then it calls the delete function on each deleted entity in the list.
// In the old code, the objects were simply destroyed in order and there was no guarantee that the
//  destructor of one object would not try to access another object that might already have been
//  destructed (especially since the entity list order is more or less random!).
// NOTE:  You should never call delete directly on an entity (there's an assert now), see note
//  at CBaseEntity::~CBaseEntity for more information.
// 
// NOTE:  You should chain to BaseClass::UpdateOnRemove after doing your own cleanup code, e.g.:
// 
// void CDerived::UpdateOnRemove( void )
// {
//		... cleanup code
//		...
//
//		BaseClass::UpdateOnRemove();
// }
//
// In general, this function updates global tables that need to know about entities being removed
//-----------------------------------------------------------------------------
void CBaseEntity::UpdateOnRemove( void )
{
	g_bReceivedChainedUpdateOnRemove = true;

	// Virtual call to shut down any looping sounds.
	StopLoopingSounds();

	// Notifies entity listeners, etc
	gEntList.NotifyRemoveEntity( GetRefEHandle() );

	if ( edict() )
	{
		AddFlag( FL_KILLME );
		if ( GetFlags() & FL_GRAPHED )
		{
			/*	<<TODO>>
			// this entity was a LinkEnt in the world node graph, so we must remove it from
			// the graph since we are removing it from the world.
			for ( int i = 0 ; i < WorldGraph.m_cLinks ; i++ )
			{
				if ( WorldGraph.m_pLinkPool [ i ].m_pLinkEnt == pev )
				{
					// if this link has a link ent which is the same ent that is removing itself, remove it!
					WorldGraph.m_pLinkPool [ i ].m_pLinkEnt = NULL;
				}
			}
			*/
		}
	}

	if ( m_iGlobalname != NULL_STRING )
	{
		// NOTE: During level shutdown the global list will suppress this
		// it assumes your changing levels or the game will end
		// causing the whole list to be flushed
		GlobalEntity_SetState( m_iGlobalname, GLOBAL_DEAD );
	}

	VPhysicsDestroyObject();

	// This is only here to allow the MOVETYPE_NONE to be set without the
	// assertion triggering. Why do we bother setting the MOVETYPE to none here?
	RemoveEffects( EF_BONEMERGE );
	SetMoveType(MOVETYPE_NONE);

	// If we have a parent, unlink from it.
	UnlinkFromParent( this );

	// Any children still connected are orphans, mark all for delete
	CUtlVector<CBaseEntity *> childrenList;
	GetAllChildren( this, childrenList );
	if ( childrenList.Count() )
	{
		DevMsg( 2, "Warning: Deleting orphaned children of %s\n", GetClassname() );
		for ( int i = childrenList.Count()-1; i >= 0; --i )
		{
			UTIL_Remove( childrenList[i] );
		}
	}

	SetGroundEntity( NULL );

	if ( m_bDynamicModelPending )
	{
		sg_DynamicLoadHandlers.Remove( this );
	}
	
	if ( IsDynamicModelIndex( m_nModelIndex ) )
	{
		modelinfo->ReleaseDynamicModel( m_nModelIndex ); // no-op if not dynamic
		m_nModelIndex = -1;
	}
}

//-----------------------------------------------------------------------------
// capabilities
//-----------------------------------------------------------------------------
int CBaseEntity::ObjectCaps( void ) 
{
#if 1
	model_t *pModel = GetModel();
	bool bIsBrush = ( pModel && modelinfo->GetModelType( pModel ) == mod_brush );

	// We inherit our parent's use capabilities so that we can forward use commands
	// to our parent.
	CBaseEntity *pParent = GetParent();
	if ( pParent )
	{
		int caps = pParent->ObjectCaps();

		if ( !bIsBrush )
			caps &= ( FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE );
		else
			caps &= ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE );

		if ( pParent->IsPlayer() )
			caps |= FCAP_ACROSS_TRANSITION;

		return caps;
	}
	else if ( !bIsBrush ) 
	{
		return FCAP_ACROSS_TRANSITION;
	}

	return 0;
#else
	// We inherit our parent's use capabilities so that we can forward use commands
	// to our parent.
	int parentCaps = 0;
	if (GetParent())
	{
		parentCaps = GetParent()->ObjectCaps();
		parentCaps &= ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE );
	}	

	model_t *pModel = GetModel();
	if ( pModel && modelinfo->GetModelType( pModel ) == mod_brush )
		return parentCaps;

	return FCAP_ACROSS_TRANSITION | parentCaps;
#endif
}

void CBaseEntity::StartTouch( CBaseEntity *pOther )
{
	// notify parent
	if ( m_pParent != NULL )
		m_pParent->StartTouch( pOther );
}

void CBaseEntity::Touch( CBaseEntity *pOther )
{ 
	if ( m_pfnTouch ) 
		(this->*m_pfnTouch)( pOther );

	// notify parent of touch
	if ( m_pParent != NULL )
		m_pParent->Touch( pOther );
}

void CBaseEntity::EndTouch( CBaseEntity *pOther )
{
	// notify parent
	if ( m_pParent != NULL )
	{
		m_pParent->EndTouch( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dispatches blocked events to this entity's blocked handler, set via SetBlocked.
// Input  : pOther - The entity that is blocking us.
//-----------------------------------------------------------------------------
void CBaseEntity::Blocked( CBaseEntity *pOther )
{ 
	if ( m_pfnBlocked )
	{
		(this->*m_pfnBlocked)( pOther );
	}

	//
	// Forward the blocked event to our parent, if any.
	//
	if ( m_pParent != NULL )
	{
		m_pParent->Blocked( pOther );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Dispatches use events to this entity's use handler, set via SetUse.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBaseEntity::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) 
{
	if ( m_pfnUse != NULL ) 
	{
		(this->*m_pfnUse)( pActivator, pCaller, useType, value );
	}
	else
	{
		//
		// We don't handle use events. Forward to our parent, if any.
		//
		if ( m_pParent != NULL )
		{
			m_pParent->Use( pActivator, pCaller, useType, value );
		}
	}
}

static CBaseEntity *FindPhysicsBlocker( IPhysicsObject *pPhysics, physicspushlist_t &list, const Vector &pushVel )
{
	IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();
	CBaseEntity *pBlocker = NULL;
	float maxForce = 0;
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
		bool inList = false;
		for ( int i = 0; i < list.pushedCount; i++ )
		{
			if ( pOtherEntity == list.pushedEnts[i] )
			{
				inList = true;
				break;
			}
		}

		Vector normal;
		pSnapshot->GetSurfaceNormal(normal);
		float dot = DotProduct( pushVel, pSnapshot->GetNormalForce() * normal );
		if ( !pBlocker || (!inList && dot > maxForce) )
		{
			pBlocker = pOtherEntity;
			if ( !inList )
			{
				maxForce = dot;
			}
		}

		pSnapshot->NextFrictionData();
	}
	pPhysics->DestroyFrictionSnapshot( pSnapshot );

	return pBlocker;
}


struct pushblock_t
{
	physicspushlist_t *pList;
	CBaseEntity *pRootParent;
	CBaseEntity *pBlockedEntity;
	float		moveBackFraction;
	float		movetime;
};

static void ComputePushStartMatrix( matrix3x4_t &start, CBaseEntity *pEntity, const pushblock_t &params )
{
	Vector localOrigin;
	QAngle localAngles;
	if ( params.pList )
	{
		localOrigin = params.pList->localOrigin;
		localAngles = params.pList->localAngles;
	}
	else
	{
		localOrigin = params.pRootParent->GetAbsOrigin() - params.pRootParent->GetAbsVelocity() * params.movetime;
		localAngles = params.pRootParent->GetAbsAngles() - params.pRootParent->GetLocalAngularVelocity() * params.movetime;
	}
	matrix3x4_t xform, delta;
	AngleMatrix( localAngles, localOrigin, xform );

	matrix3x4_t srcInv;
	// xform = src(-1) * dest
	MatrixInvert( params.pRootParent->EntityToWorldTransform(), srcInv );
	ConcatTransforms( xform, srcInv, delta );
	ConcatTransforms( delta, pEntity->EntityToWorldTransform(), start );
}

#define DEBUG_PUSH_MESSAGES 0
static void CheckPushedEntity( CBaseEntity *pEntity, pushblock_t &params )
{
	IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
	if ( !pPhysics )
		return;
	// somehow we've got a static or motion disabled physics object in hierarchy!
	// This is not allowed!  Don't test blocking in that case.
	Assert(pPhysics->IsMoveable());
	if ( !pPhysics->IsMoveable() || !pPhysics->GetShadowController() )
	{
#if DEBUG_PUSH_MESSAGES
		Msg("Blocking %s, not moveable!\n", pEntity->GetClassname());
#endif
		return;
	}

	bool checkrot = true;
	bool checkmove = true;
	Vector origin;
	QAngle angles;
	pPhysics->GetShadowPosition( &origin, &angles );
	float fraction = -1.0f;

	matrix3x4_t parentDelta;
	if ( pEntity == params.pRootParent )
	{
		if ( pEntity->GetLocalAngularVelocity() == vec3_angle )
			checkrot = false;
		if ( pEntity->GetLocalVelocity() == vec3_origin)
			checkmove = false;
	}
	else
	{
#if DEBUG_PUSH_MESSAGES
		if ( pPhysics->IsAttachedToConstraint(false))
		{
			Msg("Warning, hierarchical entity is attached to a constraint %s\n", pEntity->GetClassname());
		}
#endif
	}

	if ( checkmove )
	{
		// project error onto the axis of movement
		Vector dir = pEntity->GetAbsVelocity();
		float speed = VectorNormalize(dir);
		Vector targetPos;
		pPhysics->GetShadowController()->GetTargetPosition( &targetPos, NULL );
		float targetAmount = DotProduct(targetPos, dir);
		float currentAmount = DotProduct(origin, dir);
		float entityAmount = DotProduct(pEntity->GetAbsOrigin(), dir);

		// if target and entity origin are not in sync, then the position of the entity was updated
		// by something outside of push physics
		if ( (targetAmount - entityAmount) > 1 )
		{
			pEntity->UpdatePhysicsShadowToCurrentPosition(0);
#if DEBUG_PUSH_MESSAGES
			Warning("Someone slammed the position of a %s\n", pEntity->GetClassname() );
#endif
		}
		else
		{
			float dist = targetAmount - currentAmount;
			if ( dist > 1 )
			{
	#if DEBUG_PUSH_MESSAGES
				const char *pName = pEntity->GetClassname();
				Msg( "%s blocked by %.2f units\n", pName, dist );
	#endif
				float movementAmount = targetAmount - (speed * params.movetime);
				if ( pEntity == params.pRootParent )
				{
					if ( params.pList )
					{
						Vector localVel = pEntity->GetLocalVelocity();
						VectorNormalize(localVel);
						float localTargetAmt = DotProduct(pEntity->GetLocalOrigin(), localVel);
						movementAmount = targetAmount + DotProduct(params.pList->localOrigin, localVel) - localTargetAmt;
					}
				}
				else
				{
					matrix3x4_t start;
					ComputePushStartMatrix( start, pEntity, params );
					Vector startPos;
					MatrixPosition( start, startPos );
					movementAmount = DotProduct(startPos, dir);
				}
				float expectedDist = targetAmount - movementAmount;
				// compute the fraction to move back the AI to match the physics
				if ( expectedDist <= 0 )
				{
					fraction = 1;
				}
				else
				{
					fraction = dist / expectedDist;
					fraction = clamp(fraction, 0.f, 1.f);
				}
			}
		}
	}

	if ( checkrot )
	{
		Vector axis;
		float deltaAngle;
		RotationDeltaAxisAngle( angles, pEntity->GetAbsAngles(), axis, deltaAngle );
		if ( fabsf(deltaAngle) > 0.5f )
		{
			Vector targetAxis;
			QAngle targetRot;
			float deltaTargetAngle;
			pPhysics->GetShadowController()->GetTargetPosition( NULL, &targetRot );
			RotationDeltaAxisAngle( angles, targetRot, targetAxis, deltaTargetAngle );
			if ( fabsf(deltaTargetAngle) > 0.01f )
			{
				float expectedDist = deltaAngle;
#if DEBUG_PUSH_MESSAGES
				const char *pName = pEntity->GetClassname();
				Msg( "%s blocked by %.2f degrees\n", pName, deltaAngle );
				if ( pPhysics->IsAsleep() )
				{
					Msg("Asleep while blocked?\n");
				}
				if ( pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING )
				{
					Msg("Blocking for penetration!\n");
				}
#endif
				if ( pEntity == params.pRootParent )
				{
					expectedDist = pEntity->GetLocalAngularVelocity().Length() * params.movetime;
				}
				else
				{
					matrix3x4_t start;
					ComputePushStartMatrix( start, pEntity, params );
					Vector startAxis;
					float startAngle;
					Vector startPos;
					QAngle startAngles;
					MatrixAngles( start, startAngles, startPos );
					RotationDeltaAxisAngle( startAngles, pEntity->GetAbsAngles(), startAxis, startAngle );
					expectedDist = startAngle * DotProduct( startAxis, axis );
				}

				float t = expectedDist != 0.0f ? fabsf(deltaAngle / expectedDist) : 1.0f;
				t = clamp(t,0.f,1.f);
				fraction = MAX(fraction, t);
			}
			else
			{
				pEntity->UpdatePhysicsShadowToCurrentPosition(0);
#if DEBUG_PUSH_MESSAGES
				Warning("Someone slammed the position of a %s\n", pEntity->GetClassname() );
#endif
			}
		}
	}
	if ( fraction >= params.moveBackFraction )
	{
		params.moveBackFraction = fraction;
		params.pBlockedEntity = pEntity;
	}
}

void CBaseEntity::VPhysicsUpdatePusher( IPhysicsObject *pPhysics )
{
	float movetime = m_flLocalTime - m_flVPhysicsUpdateLocalTime;
	if (movetime <= 0)
		return;

	// only reconcile pushers on the final vphysics tick
	if ( !PhysIsFinalTick() )
		return;

	Vector origin;
	QAngle angles;

	// physics updated the shadow, so check to see if I got blocked
	// NOTE: SOLID_BSP cannont compute consistent collisions wrt vphysics, so 
	// don't allow vphysics to block.  Assume game physics has handled it.
	if ( GetSolid() != SOLID_BSP && pPhysics->GetShadowPosition( &origin, &angles ) )
	{
		CUtlVector<CBaseEntity *> list;
		GetAllInHierarchy( this, list );
		//NDebugOverlay::BoxAngles( origin, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), angles, 255,0,0,0, gpGlobals->frametime);

		physicspushlist_t *pList = NULL;
		if ( HasDataObjectType(PHYSICSPUSHLIST) )
		{
			pList = (physicspushlist_t *)GetDataObject( PHYSICSPUSHLIST );
			Assert(pList);
		}
		bool checkrot = (GetLocalAngularVelocity() != vec3_angle) ? true : false;
		bool checkmove = (GetLocalVelocity() != vec3_origin) ? true : false;

		pushblock_t params;
		params.pRootParent = this;
		params.pList = pList;
		params.pBlockedEntity = NULL;
		params.moveBackFraction = 0.0f;
		params.movetime = movetime;
		for ( int i = 0; i < list.Count(); i++ )
		{
			if ( list[i]->IsSolid() )
			{
				CheckPushedEntity( list[i], params );
			}
		}

		float physLocalTime = m_flLocalTime;
		if ( params.pBlockedEntity )
		{
			float moveback = movetime * params.moveBackFraction;
			if ( moveback > 0 )
			{
				physLocalTime = m_flLocalTime - moveback;
				// add 1% noise for bouncing in collision.
				if ( physLocalTime <= (m_flVPhysicsUpdateLocalTime + movetime * 0.99f) )
				{
					CBaseEntity *pBlocked = NULL;
					IPhysicsObject *pOther;
					if ( params.pBlockedEntity->VPhysicsGetObject()->GetContactPoint( NULL, &pOther ) )
					{
						pBlocked = static_cast<CBaseEntity *>(pOther->GetGameData());
					}
					// UNDONE: Need to traverse hierarchy here?  Shouldn't.
					if ( pList )
					{
						SetLocalOrigin( pList->localOrigin );
						SetLocalAngles( pList->localAngles );
						physLocalTime = pList->localMoveTime;
						for ( int i = 0; i < pList->pushedCount; i++ )
						{
							CBaseEntity *pEntity = pList->pushedEnts[i];
							if ( !pEntity )
								continue;

							pEntity->SetAbsOrigin( pEntity->GetAbsOrigin() - pList->pushVec[i] );
						}
						CBaseEntity *pPhysicsBlocker = FindPhysicsBlocker( VPhysicsGetObject(), *pList, pList->pushVec[0] );
						if ( pPhysicsBlocker )
						{
							pBlocked = pPhysicsBlocker;
						}
					}
					else
					{
						Vector origin = GetLocalOrigin();
						QAngle angles = GetLocalAngles();

						if ( checkmove )
						{
							origin -= GetLocalVelocity() * moveback;
						}
						if ( checkrot )
						{
							// BUGBUG: This is pretty hack-tastic!
							angles -= GetLocalAngularVelocity() * moveback;
						}

						SetLocalOrigin( origin );
						SetLocalAngles( angles );
					}

					if ( pBlocked )
					{
						Blocked( pBlocked );
					}
					m_flLocalTime = physLocalTime;
				}
			}
		}
	}

	// this data is no longer useful, free the memory
	if ( HasDataObjectType(PHYSICSPUSHLIST) )
	{
		DestroyDataObject( PHYSICSPUSHLIST );
	}

	m_flVPhysicsUpdateLocalTime = m_flLocalTime;
	if ( m_flMoveDoneTime <= m_flLocalTime && m_flMoveDoneTime > 0 )
	{
		SetMoveDoneTime( -1 );
		MoveDone();
	}
}


void CBaseEntity::SetMoveDoneTime( float flDelay )
{
	if (flDelay >= 0)
	{
		m_flMoveDoneTime = GetLocalTime() + flDelay;
	}
	else
	{
		m_flMoveDoneTime = -1;
	}
	CheckHasGamePhysicsSimulation();
}

//-----------------------------------------------------------------------------
// Purpose: Relinks all of a parents children into the collision tree
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsRelinkChildren( float dt )
{
	CBaseEntity *child;

	// iterate through all children
	for ( child = FirstMoveChild(); child != NULL; child = child->NextMovePeer() )
	{
		if ( child->IsSolid() || child->IsSolidFlagSet(FSOLID_TRIGGER) )
		{
			child->PhysicsTouchTriggers();
		}

		//
		// Update their physics shadows. We should never have any children of
		// movetype VPHYSICS.
		//
		if ( child->GetMoveType() != MOVETYPE_VPHYSICS )
		{
			child->UpdatePhysicsShadowToCurrentPosition( dt );
		}
		else if ( child->GetOwnerEntity() != this )
		{
			// the only case where this is valid is if this entity is an attached ragdoll.
			// So assert here to catch the non-ragdoll case.
			Assert( 0 );
		}

		if ( child->FirstMoveChild() )
		{
			child->PhysicsRelinkChildren(dt);
		}
	}
}

void CBaseEntity::PhysicsTouchTriggers( const Vector *pPrevAbsOrigin )
{
	edict_t *pEdict = edict();
	if ( pEdict && !IsWorld() )
	{
		Assert(CollisionProp());
		bool isTriggerCheckSolids = IsSolidFlagSet( FSOLID_TRIGGER );
		bool isSolidCheckTriggers = IsSolid() && !isTriggerCheckSolids;		// NOTE: Moving triggers (items, ammo etc) are not 
																			// checked against other triggers to reduce the number of touchlinks created
		if ( !(isSolidCheckTriggers || isTriggerCheckSolids) )
			return;

		if ( GetSolid() == SOLID_BSP ) 
		{
			if ( !GetModel() && Q_strlen( STRING( GetModelName() ) ) == 0 ) 
			{
				Warning( "Inserted %s with no model\n", GetClassname() );
				return;
			}
		}

		SetCheckUntouch( true );
		if ( isSolidCheckTriggers )
		{
			engine->SolidMoved( pEdict, CollisionProp(), pPrevAbsOrigin, sm_bAccurateTriggerBboxChecks );
		}
		if ( isTriggerCheckSolids )
		{
			engine->TriggerMoved( pEdict, sm_bAccurateTriggerBboxChecks );
		}
	}
}

void CBaseEntity::VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent )
{
}



void CBaseEntity::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// filter out ragdoll props hitting other parts of itself too often
	// UNDONE: Store a sound time for this entity (not just this pair of objects)
	// and filter repeats on that?
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];

	// Don't make sounds / effects if neither entity is MOVETYPE_VPHYSICS.  The game
	// physics should have done so.
	if ( GetMoveType() != MOVETYPE_VPHYSICS && pHitEntity->GetMoveType() != MOVETYPE_VPHYSICS )
		return;

	if ( pEvent->deltaCollisionTime < 0.5 && (pHitEntity == this) )
		return;

	// don't make noise for hidden/invisible/sky materials
	surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	const surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[index] );
	if ( phit->game.material == 'X' || pprops->game.material == 'X' )
		return;

	if ( pHitEntity == this )
	{
		PhysCollisionSound( this, pEvent->pObjects[index], CHAN_BODY, pEvent->surfaceProps[index], pEvent->surfaceProps[otherIndex], pEvent->deltaCollisionTime, pEvent->collisionSpeed );
	}
	else
	{
		PhysCollisionSound( this, pEvent->pObjects[index], CHAN_STATIC, pEvent->surfaceProps[index], pEvent->surfaceProps[otherIndex], pEvent->deltaCollisionTime, pEvent->collisionSpeed );
	}
	PhysCollisionScreenShake( pEvent, index );

#if HL2_EPISODIC
	// episodic does something different for when advisor shields are struck
	if ( phit->game.material == 'Z' || pprops->game.material == 'Z')
	{
		PhysCollisionWarpEffect( pEvent, phit );
	}
	else
	{
		PhysCollisionDust( pEvent, phit );
	}
#else
	PhysCollisionDust( pEvent, phit );
#endif
}

void CBaseEntity::VPhysicsFriction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit )
{
	PhysFrictionSound( this, pObject, energy, surfaceProps, surfacePropsHit );
}


void CBaseEntity::VPhysicsSwapObject( IPhysicsObject *pSwap )
{
	if ( !pSwap )
	{
		PhysRemoveShadow(this);
	}

	if ( !m_pPhysicsObject )
	{
		Warning( "Bad vphysics swap for %s\n", STRING(m_iClassname) );
	}
	m_pPhysicsObject = pSwap;
}


// Tells the physics shadow to update it's target to the current position
void CBaseEntity::UpdatePhysicsShadowToCurrentPosition( float deltaTime )
{
	if ( GetMoveType() != MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhys = VPhysicsGetObject();
		if ( pPhys )
		{
			pPhys->UpdateShadow( GetAbsOrigin(), GetAbsAngles(), false, deltaTime );
		}
	}
}

int CBaseEntity::VPhysicsGetObjectList( IPhysicsObject **pList, int listMax )
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if ( pPhys )
	{
		// multi-object entities must implement this function
		Assert( !(pPhys->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY) );
		if ( listMax > 0 )
		{
			pList[0] = pPhys;
			return 1;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CBaseEntity::VPhysicsIsFlesh( void )
{
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		int material = pList[i]->GetMaterialIndex();
		const surfacedata_t *pSurfaceData = physprops->GetSurfaceData( material );
		// Is flesh ?, don't allow pickup
		if ( pSurfaceData->game.material == CHAR_TEX_ANTLION || pSurfaceData->game.material == CHAR_TEX_FLESH || pSurfaceData->game.material == CHAR_TEX_BLOODYFLESH || pSurfaceData->game.material == CHAR_TEX_ALIENFLESH )
			return true;
	}
	return false;
}

bool CBaseEntity::Intersects( CBaseEntity *pOther )
{
	if ( !edict() || !pOther->edict() )
		return false;

	CCollisionProperty *pMyProp = CollisionProp();
	CCollisionProperty *pOtherProp = pOther->CollisionProp();

	return IsOBBIntersectingOBB( 
		pMyProp->GetCollisionOrigin(), pMyProp->GetCollisionAngles(), pMyProp->OBBMins(), pMyProp->OBBMaxs(),
		pOtherProp->GetCollisionOrigin(), pOtherProp->GetCollisionAngles(), pOtherProp->OBBMins(), pOtherProp->OBBMaxs() );
}

extern ConVar ai_LOS_mode;

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the target
//=========================================================
bool CBaseEntity::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	VPROF( "CBaseEntity::FVisible" );

	if ( pEntity->GetFlags() & FL_NOTARGET )
		return false;

#if HL1_DLL
	// FIXME: only block LOS through opaque water
	// don't look through water
	if ((m_nWaterLevel != 3 && pEntity->m_nWaterLevel == 3) 
		|| (m_nWaterLevel == 3 && pEntity->m_nWaterLevel == 0))
		return false;
#endif

	Vector vecLookerOrigin = EyePosition();//look through the caller's 'eyes'
	Vector vecTargetOrigin = pEntity->EyePosition();

	trace_t tr;
	if ( !IsXbox() && ai_LOS_mode.GetBool() )
	{
		UTIL_TraceLine(vecLookerOrigin, vecTargetOrigin, traceMask, this, COLLISION_GROUP_NONE, &tr);
	}
	else
	{
		// If we're doing an LOS search, include NPCs.
		if ( traceMask == MASK_BLOCKLOS )
		{
			traceMask = MASK_BLOCKLOS_AND_NPCS;
		}

		// Player sees through nodraw
		if ( IsPlayer() )
		{
			traceMask &= ~CONTENTS_BLOCKLOS;
		}

		// Use the custom LOS trace filter
		CTraceFilterLOS traceFilter( this, COLLISION_GROUP_NONE, pEntity );
		UTIL_TraceLine( vecLookerOrigin, vecTargetOrigin, traceMask, &traceFilter, &tr );
	}
	
	if (tr.fraction != 1.0 || tr.startsolid )
	{
		// If we hit the entity we're looking for, it's visible
		if ( tr.m_pEnt == pEntity )
			return true;

		// Got line of sight on the vehicle the player is driving!
		if ( pEntity && pEntity->IsPlayer() )
		{
			CBasePlayer *pPlayer = assert_cast<CBasePlayer*>( pEntity );
			if ( tr.m_pEnt == pPlayer->GetVehicleEntity() )
				return true;
		}

		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}

		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}

//=========================================================
// FVisible - returns true if a line can be traced from
// the caller's eyes to the wished position.
//=========================================================
bool CBaseEntity::FVisible( const Vector &vecTarget, int traceMask, CBaseEntity **ppBlocker )
{
#if HL1_DLL
	
	// don't look through water
	// FIXME: only block LOS through opaque water
	bool inWater = ( UTIL_PointContents( vecTarget ) & (CONTENTS_SLIME|CONTENTS_WATER) ) ? true : false;

	// Don't allow it if we're straddling two areas
	if ( ( m_nWaterLevel == 3 && !inWater ) || ( m_nWaterLevel != 3 && inWater ) )
		return false;

#endif 

	trace_t tr;
	Vector vecLookerOrigin = EyePosition();// look through the caller's 'eyes'

	if ( ai_LOS_mode.GetBool() )
	{
		UTIL_TraceLine( vecLookerOrigin, vecTarget, traceMask, this, COLLISION_GROUP_NONE, &tr);
	}
	else
	{
		// If we're doing an LOS search, include NPCs.
		if ( traceMask == MASK_BLOCKLOS )
		{
			traceMask = MASK_BLOCKLOS_AND_NPCS;
		}

		// Player sees through nodraw and blocklos
		if ( IsPlayer() )
		{
			traceMask |= CONTENTS_IGNORE_NODRAW_OPAQUE;
			traceMask &= ~CONTENTS_BLOCKLOS;
		}

		// Use the custom LOS trace filter
		CTraceFilterLOS traceFilter( this, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecLookerOrigin, vecTarget, traceMask, &traceFilter, &tr );
	}

	if (tr.fraction != 1.0)
	{
		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}
		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}

extern ConVar ai_debug_los;
//-----------------------------------------------------------------------------
// Purpose: Turn on prop LOS debugging mode
//-----------------------------------------------------------------------------
void CC_AI_LOS_Debug( IConVar *var, const char *pOldString, float flOldValue )
{
	int iLOSMode = ai_debug_los.GetInt();
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		if ( iLOSMode == 1 && pEntity->IsSolid() )
		{
			pEntity->m_debugOverlays |= OVERLAY_SHOW_BLOCKSLOS;
		}
		else if ( iLOSMode == 2 )
		{
			pEntity->m_debugOverlays |= OVERLAY_SHOW_BLOCKSLOS;
		}
		else
		{
			pEntity->m_debugOverlays &= ~OVERLAY_SHOW_BLOCKSLOS;
		}
	}
}
ConVar ai_debug_los("ai_debug_los", "0", FCVAR_CHEAT, "NPC Line-Of-Sight debug mode. If 1, solid entities that block NPC LOC will be highlighted with white bounding boxes. If 2, it'll show non-solid entities that would do it if they were solid.", CC_AI_LOS_Debug );


Class_T CBaseEntity::Classify ( void )
{ 
	return CLASS_NONE;
}

float CBaseEntity::GetAutoAimRadius()
{
	if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
		return 48.0f;
	else
		return 24.0f;
}

//-----------------------------------------------------------------------------
// Changes the shadow cast distance over time
//-----------------------------------------------------------------------------
void CBaseEntity::ShadowCastDistThink( )
{
	SetShadowCastDistance( m_flDesiredShadowCastDistance );
	SetContextThink( NULL, gpGlobals->curtime, "ShadowCastDistThink" );
}

void CBaseEntity::SetShadowCastDistance( float flDesiredDistance, float flDelay )
{
	m_flDesiredShadowCastDistance = flDesiredDistance;
	if ( m_flDesiredShadowCastDistance != m_flShadowCastDistance )
	{
		SetContextThink( &CBaseEntity::ShadowCastDistThink, gpGlobals->curtime + flDelay, "ShadowCastDistThink" );
	}
}


/*
================
TraceAttack
================
*/

//-----------------------------------------------------------------------------
// Purpose: Returns whether a damage info can damage this entity.
//-----------------------------------------------------------------------------
bool CBaseEntity::PassesDamageFilter( const CTakeDamageInfo &info )
{
	if (m_hDamageFilter)
	{
		CBaseFilter *pFilter = (CBaseFilter *)(m_hDamageFilter.Get());
		return pFilter->PassesDamageFilter(info);
	}

	return true;
}

FORCEINLINE bool NamesMatch( const char *pszQuery, string_t nameToMatch )
{
	if ( nameToMatch == NULL_STRING )
		return (!pszQuery || *pszQuery == 0 || *pszQuery == '*');

	const char *pszNameToMatch = STRING(nameToMatch);

	// If the pointers are identical, we're identical
	if ( pszNameToMatch == pszQuery )
		return true;

	while ( *pszNameToMatch && *pszQuery )
	{
		unsigned char cName = *pszNameToMatch;
		unsigned char cQuery = *pszQuery;
		// simple ascii case conversion
		if ( cName == cQuery )
			;
		else if ( cName - 'A' <= (unsigned char)'Z' - 'A' && cName - 'A' + 'a' == cQuery )
			;
		else if ( cName - 'a' <= (unsigned char)'z' - 'a' && cName - 'a' + 'A' == cQuery )
			;
		else
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *pszNameToMatch == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' )
		return true;

	return false;
}

bool CBaseEntity::NameMatchesComplex( const char *pszNameOrWildcard )
{
	if ( !Q_stricmp( "!player", pszNameOrWildcard) )
		return IsPlayer();

	return NamesMatch( pszNameOrWildcard, m_iName );
}

bool CBaseEntity::ClassMatchesComplex( const char *pszClassOrWildcard )
{
	return NamesMatch( pszClassOrWildcard, m_iClassname );
}

void CBaseEntity::MakeDormant( void )
{
	AddEFlags( EFL_DORMANT );

	// disable thinking for dormant entities
	SetThink( NULL );

	if ( !edict() )
		return;

	SETBITS( m_iEFlags, EFL_DORMANT );
	
	// Don't touch
	AddSolidFlags( FSOLID_NOT_SOLID );
	// Don't move
	SetMoveType( MOVETYPE_NONE );
	// Don't draw
	AddEffects( EF_NODRAW );
	// Don't think
	SetNextThink( TICK_NEVER_THINK );
}

int CBaseEntity::IsDormant( void )
{
	return IsEFlagSet( EFL_DORMANT );
}


bool CBaseEntity::IsInWorld( void ) const
{  
	if ( !edict() )
		return true;

	// position 
	if (GetAbsOrigin().x >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().y >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().z >= MAX_COORD_INTEGER) return false;
	if (GetAbsOrigin().x <= MIN_COORD_INTEGER) return false;
	if (GetAbsOrigin().y <= MIN_COORD_INTEGER) return false;
	if (GetAbsOrigin().z <= MIN_COORD_INTEGER) return false;
	// speed
	if (GetAbsVelocity().x >= 2000) return false;
	if (GetAbsVelocity().y >= 2000) return false;
	if (GetAbsVelocity().z >= 2000) return false;
	if (GetAbsVelocity().x <= -2000) return false;
	if (GetAbsVelocity().y <= -2000) return false;
	if (GetAbsVelocity().z <= -2000) return false;

	return true;
}


bool CBaseEntity::IsViewable( void )
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		return false;
	}

	if (IsBSPModel())
	{
		if (GetMoveType() != MOVETYPE_NONE)
		{
			return true;
		}
	}
	else if (GetModelIndex() != 0)
	{
		// check for total transparency???
		return true;
	}
	return false;
}


int CBaseEntity::ShouldToggle( USE_TYPE useType, int currentState )
{
	if ( useType != USE_TOGGLE && useType != USE_SET )
	{
		if ( (currentState && useType == USE_ON) || (!currentState && useType == USE_OFF) )
			return 0;
	}
	return 1;
}


// NOTE: szName must be a pointer to constant memory, e.g. "NPC_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity *CBaseEntity::Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CBaseEntity *pEntity = CreateNoSpawn( szName, vecOrigin, vecAngles, pOwner );

	DispatchSpawn( pEntity );
	return pEntity;
}



// NOTE: szName must be a pointer to constant memory, e.g. "NPC_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity * CBaseEntity::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CBaseEntity *pEntity = CreateEntityByName( szName );
	if ( !pEntity )
	{
		Assert( !"CreateNoSpawn: only works for CBaseEntities" );
		return NULL;
	}

	pEntity->SetLocalOrigin( vecOrigin );
	pEntity->SetLocalAngles( vecAngles );
	pEntity->SetOwnerEntity( pOwner );

	gEntList.NotifyCreateEntity( pEntity );

	return pEntity;
}

Vector CBaseEntity::GetSoundEmissionOrigin() const
{
	return WorldSpaceCenter();
}


//-----------------------------------------------------------------------------
// Purpose: Saves the current object out to disk, by iterating through the objects
//			data description hierarchy
// Input  : &save - save buffer which the class data is written to
// Output : int	- 0 if the save failed, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::Save( ISave &save )
{
	// loop through the data description list, saving each data desc block
	int status = SaveDataDescBlock( save, GetDataDescMap() );

	return status;
}

//-----------------------------------------------------------------------------
// Purpose: Recursively saves all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::SaveDataDescBlock( ISave &save, datamap_t *dmap )
{
	return save.WriteAll( this, dmap );
}

//-----------------------------------------------------------------------------
// Purpose: Restores the current object from disk, by iterating through the objects
//			data description hierarchy
// Input  : &restore - restore buffer which the class data is read from
// Output : int	- 0 if the restore failed, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::Restore( IRestore &restore )
{
	// This is essential to getting the spatial partition info correct
	CollisionProp()->DestroyPartitionHandle();

	// loops through the data description list, restoring each data desc block in order
	int status = RestoreDataDescBlock( restore, GetDataDescMap() );

	// ---------------------------------------------------------------
	// HACKHACK: We don't know the space of these vectors until now
	// if they are worldspace, fix them up.
	// ---------------------------------------------------------------
	{
		CGameSaveRestoreInfo *pGameInfo = restore.GetGameSaveRestoreInfo();
		Vector parentSpaceOffset = pGameInfo->modelSpaceOffset;
		if ( !GetParent() )
		{
			// parent is the world, so parent space is worldspace
			// so update with the worldspace leveltransition transform
			parentSpaceOffset += pGameInfo->GetLandmark();
		}
		
		// NOTE: Do *not* use GetAbsOrigin() here because it will
		// try to recompute m_rgflCoordinateFrame!
		MatrixSetColumn( m_vecAbsOrigin, 3, m_rgflCoordinateFrame );

		m_vecOrigin += parentSpaceOffset;
	}

	// Gotta do this after the coordframe is set up as it depends on it.

	// By definition, the surrounding bounds are dirty
	// Also, twiddling with the flags here ensures it gets added to the KD tree dirty list
	// (We don't want to use the saved version of this flag)
	RemoveEFlags( EFL_DIRTY_SPATIAL_PARTITION );
	CollisionProp()->MarkSurroundingBoundsDirty();

	if ( edict() && GetModelIndex() != 0 && GetModelName() != NULL_STRING && restore.GetPrecacheMode() )
	{
		PrecacheModel( STRING( GetModelName() ) );

		//Adrian: We should only need to do this after we precache. No point in setting the model again.
		SetModelIndex( modelinfo->GetModelIndex( STRING(GetModelName() ) ) );
	}

	// Restablish ground entity
	if ( m_hGroundEntity != NULL )
	{
		m_hGroundEntity->AddEntityToGroundList( this );
	}

	return status;
}


//-----------------------------------------------------------------------------
// handler to do stuff before you are saved
//-----------------------------------------------------------------------------
void CBaseEntity::OnSave( IEntitySaveUtils *pUtils )
{
	// Here, we must force recomputation of all abs data so it gets saved correctly
	// We can't leave the dirty bits set because the loader can't cope with it.
	CalcAbsolutePosition();
	CalcAbsoluteVelocity();
}

//-----------------------------------------------------------------------------
// handler to do stuff after you are restored
//-----------------------------------------------------------------------------
void CBaseEntity::OnRestore()
{
#if defined( PORTAL ) || defined( HL2_EPISODIC ) || defined ( HL2_DLL ) || defined( HL2_LOSTCOAST )
	// We had a short period during the 2013 beta where the FL_* flags had a bogus value near the top, so detect
	// these bad saves and just give up. Only saves from the short beta period should have been effected.
	if ( GetFlags() & FL_FAKECLIENT )
	{
		char szMsg[256];
		V_snprintf( szMsg, sizeof(szMsg), "\nInvalid save, unable to load. Please run \"map %s\" to restart this level manually\n\n", gpGlobals->mapname.ToCStr() );
		Msg( "%s", szMsg );
		
		engine->ServerCommand("wait;wait;disconnect;showconsole\n");
	}
#endif

	SimThink_EntityChanged( this );

	// touchlinks get recomputed
	if ( IsEFlagSet( EFL_CHECK_UNTOUCH ) )
	{
		RemoveEFlags( EFL_CHECK_UNTOUCH );
		SetCheckUntouch( true );
	}

	// disable touch functions while we recreate the touch links between entities
	// NOTE: We don't do this on transitions, because we'd miss the OnStartTouch call!
#if !defined(HL2_DLL) || ( defined(HL2_DLL) && defined(HL2_EPISODIC) )
	CBaseEntity::sm_bDisableTouchFuncs = ( gpGlobals->eLoadType != MapLoad_Transition );
	PhysicsTouchTriggers();
	CBaseEntity::sm_bDisableTouchFuncs = false;
#endif // HL2_EPISODIC

	//Adrian: If I'm restoring with these fields it means I've become a client side ragdoll.
	//Don't create another one, just wait until is my time of being removed.
	if ( GetFlags() & FL_TRANSRAGDOLL )
	{
		m_nRenderFX = kRenderFxNone;
		AddEffects( EF_NODRAW );
		RemoveFlag( FL_DISSOLVING | FL_ONFIRE );
	}

	if ( m_pParent )
	{
		CBaseEntity *pChild = m_pParent->FirstMoveChild();
		while ( pChild )
		{
			if ( pChild == this )
				break;
			pChild = pChild->NextMovePeer();
		}
		if ( pChild != this )
		{
#if _DEBUG
			// generally this means you've got something marked FCAP_DONT_SAVE
			// in a hierarchy.  That's probably ok given this fixup, but the hierarhcy
			// linked list is just saved/loaded in-place
			Warning("Fixing up parent on %s\n", GetClassname() );
#endif
			// We only need to be back in the parent's list because we're already in the right place and with the right data
			LinkChild( m_pParent, this );
		}
	}

	// We're not save/loading the PVS dirty state. Assume everything is dirty after a restore
	NetworkProp()->MarkPVSInformationDirty();
}


//-----------------------------------------------------------------------------
// Purpose: Recursively restores all the classes in an object, in reverse order (top down)
// Output : int 0 on failure, 1 on success
//-----------------------------------------------------------------------------
int CBaseEntity::RestoreDataDescBlock( IRestore &restore, datamap_t *dmap )
{
	return restore.ReadAll( this, dmap );
}

//-----------------------------------------------------------------------------

bool CBaseEntity::ShouldSavePhysics()
{
	return true;
}

//-----------------------------------------------------------------------------

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// CBaseEntity new/delete
// allocates and frees memory for itself from the engine->
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *CBaseEntity::operator new( size_t stAllocateBlock )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
};

void *CBaseEntity::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
}

void CBaseEntity::operator delete( void *pMem )
{
	// get the engine to free the memory
	engine->FreeEntPrivateData( pMem );
}

#include "tier0/memdbgon.h"


#ifdef _DEBUG
void CBaseEntity::FunctionCheck( void *pFunction, const char *name )
{ 
#ifdef USES_SAVERESTORE
	// Note, if you crash here and your class is using multiple inheritance, it is
	// probably the case that CBaseEntity (or a descendant) is not the first
	// class in your list of ancestors, which it must be.
	if (pFunction && !UTIL_FunctionToName( GetDataDescMap(), (inputfunc_t *)pFunction ) )
	{
		Warning( "FUNCTION NOT IN TABLE!: %s:%s (%08lx)\n", STRING(m_iClassname), name, (unsigned long)pFunction );
		Assert(0);
	}
#endif
}
#endif


bool CBaseEntity::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	return false;
}

//-----------------------------------------------------------------------------
// Perform hitbox test, returns true *if hitboxes were tested at all*!!
//-----------------------------------------------------------------------------
bool CBaseEntity::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	return false;
}


void CBaseEntity::SetOwnerEntity( CBaseEntity* pOwner )
{
	if ( m_hOwnerEntity.Get() != pOwner )
	{
		m_hOwnerEntity = pOwner;

		CollisionRulesChanged();
	}
}

void CBaseEntity::SetMoveType( MoveType_t val, MoveCollide_t moveCollide )
{
#ifdef _DEBUG
	// Make sure the move type + move collide are compatible...
	if ((val != MOVETYPE_FLY) && (val != MOVETYPE_FLYGRAVITY))
	{
		Assert( moveCollide == MOVECOLLIDE_DEFAULT );
	}

	if ( m_MoveType == MOVETYPE_VPHYSICS && val != m_MoveType )
	{
		if ( VPhysicsGetObject() && val != MOVETYPE_NONE )
		{
			// What am I supposed to do with the physics object if
			// you're changing away from MOVETYPE_VPHYSICS without making the object 
			// shadow?  This isn't likely to work, assert.
			// You probably meant to call VPhysicsInitShadow() instead of VPhysicsInitNormal()!
			Assert( VPhysicsGetObject()->GetShadowController() );
		}
	}
#endif

	if ( m_MoveType == val )
	{
		m_MoveCollide = moveCollide;
		return;
	}

	// This is needed to the removal of MOVETYPE_FOLLOW:
	// We can't transition from follow to a different movetype directly
	// or the leaf code will break.
	Assert( !IsEffectActive( EF_BONEMERGE ) );
	m_MoveType = val;
	m_MoveCollide = moveCollide;

	CollisionRulesChanged();

	switch( m_MoveType )
	{
	case MOVETYPE_WALK:
		{
			SetSimulatedEveryTick( true );
			SetAnimatedEveryTick( true );
		}
		break;
	case MOVETYPE_STEP:
		{
			// This will probably go away once I remove the cvar that controls the test code
			SetSimulatedEveryTick( g_bTestMoveTypeStepSimulation ? true : false );
			SetAnimatedEveryTick( false );
		}
		break;
	case MOVETYPE_FLY:
	case MOVETYPE_FLYGRAVITY:
		{
			// Initialize our water state, because these movetypes care about transitions in/out of water
			UpdateWaterState();
		}
		break;
	default:
		{
			SetSimulatedEveryTick( true );
			SetAnimatedEveryTick( false );
		}
	}

	// This will probably go away or be handled in a better way once I remove the cvar that controls the test code
	CheckStepSimulationChanged();
	CheckHasGamePhysicsSimulation();
}

void CBaseEntity::Spawn( void ) 
{
}


CBaseEntity* CBaseEntity::Instance( const CBaseHandle &hEnt )
{
	return gEntList.GetBaseEntity( hEnt );
}

int CBaseEntity::GetTransmitState( void )
{
	edict_t *ed = edict();

	if ( !ed )
		return 0;

	return ed->m_fStateFlags;
}

int	CBaseEntity::SetTransmitState( int nFlag)
{
	edict_t *ed = edict();

	if ( !ed )
		return 0;

	// clear current flags = check ShouldTransmit()
	ed->ClearTransmitState();	
	
	int oldFlags = ed->m_fStateFlags;
	ed->m_fStateFlags |= nFlag;
	
	// Tell the engine (used for a network backdoor optimization).
	if ( (oldFlags & FL_EDICT_DONTSEND) != (ed->m_fStateFlags & FL_EDICT_DONTSEND) )
		engine->NotifyEdictFlagsChange( entindex() );

	return ed->m_fStateFlags;
}

int CBaseEntity::UpdateTransmitState()
{
	// If you get this assert, you should be calling DispatchUpdateTransmitState
	// instead of UpdateTransmitState.
	Assert( g_nInsideDispatchUpdateTransmitState > 0 );
	
	// If an object is the moveparent of something else, don't skip it just because it's marked EF_NODRAW or else
	//  the client won't have a proper origin for the child since the hierarchy won't be correctly transmitted down
	if ( IsEffectActive( EF_NODRAW ) && 
		!m_hMoveChild.Get() )
	{
		return SetTransmitState( FL_EDICT_DONTSEND );
	}

	if ( !IsEFlagSet( EFL_FORCE_CHECK_TRANSMIT ) )
	{
		if ( !GetModelIndex() || !GetModelName() )
		{
			return SetTransmitState( FL_EDICT_DONTSEND );
		}
	}

	// Always send the world
	if ( GetModelIndex() == 1 )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	if ( IsEFlagSet( EFL_IN_SKYBOX ) )
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	// by default cull against PVS
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

int CBaseEntity::DispatchUpdateTransmitState()
{
	edict_t *ed = edict();
	if ( m_nTransmitStateOwnedCounter != 0 )
		return ed ? ed->m_fStateFlags : 0;
	
	g_nInsideDispatchUpdateTransmitState++;
	int ret = UpdateTransmitState();
	g_nInsideDispatchUpdateTransmitState--;
	
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Note, an entity can override the send table ( e.g., to send less data or to send minimal data for
//  objects ( prob. players ) that are not in the pvs.
// Input  : **ppSendTable - 
//			*recipient - 
//			*pvs - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
int CBaseEntity::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	int fFlags = DispatchUpdateTransmitState();

	if ( fFlags & FL_EDICT_PVSCHECK )
	{
		return FL_EDICT_PVSCHECK;
	}
	else if ( fFlags & FL_EDICT_ALWAYS )
	{
		return FL_EDICT_ALWAYS;
	}
	else if ( fFlags & FL_EDICT_DONTSEND )
	{
		return FL_EDICT_DONTSEND;
	}

//	if ( IsToolRecording() )
//	{
//		return FL_EDICT_ALWAYS;
//	}

	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	Assert( pRecipientEntity->IsPlayer() );
	
	CBasePlayer *pRecipientPlayer = static_cast<CBasePlayer*>( pRecipientEntity );


	// FIXME: Refactor once notion of "team" is moved into HL2 code
	// Team rules may tell us that we should
	if ( pRecipientPlayer->GetTeam() ) 
	{
		if ( pRecipientPlayer->GetTeam()->ShouldTransmitToPlayer( pRecipientPlayer, this ))
			return FL_EDICT_ALWAYS;
	}
	

/*#ifdef INVASION_DLL
	// Check test network vis distance stuff. Eventually network LOD will do this.
	float flTestDistSqr = pRecipientEntity->GetAbsOrigin().DistToSqr( WorldSpaceCenter() );
	if ( flTestDistSqr > sv_netvisdist.GetFloat() * sv_netvisdist.GetFloat() )
		return TRANSMIT_NO;	// TODO doesn't work with HLTV
#endif*/

	// by default do a PVS check

	return FL_EDICT_PVSCHECK;
}


//-----------------------------------------------------------------------------
// Rules about which entities need to transmit along with me
//-----------------------------------------------------------------------------
void CBaseEntity::SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways )
{
	int index = entindex();

	// Are we already marked for transmission?
	if ( pInfo->m_pTransmitEdict->Get( index ) )
		return;

	CServerNetworkProperty *pNetworkParent = NetworkProp()->GetNetworkParent();

	pInfo->m_pTransmitEdict->Set( index );

	// HLTV/Replay need to know if this entity is culled by PVS limits
	if ( pInfo->m_pTransmitAlways )
	{
		// in HLTV/Replay mode always transmit entitys with move-parents
		// HLTV/Replay can't resolve the mode-parents relationships 
		if ( bAlways || pNetworkParent )
		{
			// tell HLTV/Replay that this entity is always transmitted
			pInfo->m_pTransmitAlways->Set( index );
		}
		else 
		{
			// HLTV/Replay will PVS cull this entity, so update the 
			// node/cluster infos if necessary
			m_Network.RecomputePVSInformation();
		}
	}

	// Force our aiment and move parent to be sent.
	if ( pNetworkParent )
	{
		CBaseEntity *pMoveParent = pNetworkParent->GetBaseEntity();
		pMoveParent->SetTransmit( pInfo, bAlways );
	}
}


//-----------------------------------------------------------------------------
// Returns which skybox the entity is in
//-----------------------------------------------------------------------------
CSkyCamera *CBaseEntity::GetEntitySkybox()
{
	int area = engine->GetArea( WorldSpaceCenter() );

	CSkyCamera *pCur = GetSkyCameraList();
	while ( pCur )
	{
		if ( engine->CheckAreasConnected( area, pCur->m_skyboxData.area ) )
			return pCur;

		pCur = pCur->m_pNext;
	}

	return NULL;
}

bool CBaseEntity::DetectInSkybox()
{
	if ( GetEntitySkybox() != NULL )
	{
		AddEFlags( EFL_IN_SKYBOX );
		return true;
	}

	RemoveEFlags( EFL_IN_SKYBOX );
	return false;
}


//------------------------------------------------------------------------------
// Computes a world-aligned bounding box that surrounds everything in the entity
//------------------------------------------------------------------------------
void CBaseEntity::ComputeWorldSpaceSurroundingBox( Vector *pMins, Vector *pMaxs )
{
	// Should never get here.. only use USE_GAME_CODE with bounding boxes
	// if you have an implementation for this method
	Assert( 0 );
}


//------------------------------------------------------------------------------
// Purpose : If name exists returns name, otherwise returns classname
// Input   :
// Output  :
//------------------------------------------------------------------------------
const char *CBaseEntity::GetDebugName(void)
{
	if ( this == NULL )
		return "<<null>>";

	if ( m_iName != NULL_STRING ) 
	{
		return STRING(m_iName);
	}
	else
	{
		return STRING(m_iClassname);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawInputOverlay(const char *szInputName, CBaseEntity *pCaller, variant_t Value)
{
	char bigstring[1024];
	if ( Value.FieldType() == FIELD_INTEGER )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s,%d) <-- (%s)\n", gpGlobals->curtime, szInputName, Value.Int(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else if ( Value.FieldType() == FIELD_STRING )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s,%s) <-- (%s)\n", gpGlobals->curtime, szInputName, Value.String(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) <-- (%s)\n", gpGlobals->curtime, szInputName, pCaller ? pCaller->GetDebugName() : NULL);
	}
	AddTimedOverlay(bigstring, 10.0);

	if ( Value.FieldType() == FIELD_INTEGER )
	{
		DevMsg( 2, "input: (%s,%d) -> (%s,%s), from (%s)\n", szInputName, Value.Int(), STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else if ( Value.FieldType() == FIELD_STRING )
	{
		DevMsg( 2, "input: (%s,%s) -> (%s,%s), from (%s)\n", szInputName, Value.String(), STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
	}
	else
		DevMsg( 2, "input: (%s) -> (%s,%s), from (%s)\n", szInputName, STRING(m_iClassname), GetDebugName(), pCaller ? pCaller->GetDebugName() : NULL);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::DrawOutputOverlay(CEventAction *ev)
{
	// Print to entity
	char bigstring[1024];
	if ( ev->m_flDelay )
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) --> (%s),%.1f) \n", gpGlobals->curtime, STRING(ev->m_iTargetInput), STRING(ev->m_iTarget), ev->m_flDelay);
	}
	else
	{
		Q_snprintf( bigstring,sizeof(bigstring), "%3.1f  (%s) --> (%s)\n", gpGlobals->curtime,  STRING(ev->m_iTargetInput), STRING(ev->m_iTarget));
	}
	AddTimedOverlay(bigstring, 10.0);

	// Now print to the console
	if ( ev->m_flDelay )
	{
		DevMsg( 2, "output: (%s,%s) -> (%s,%s,%.1f)\n", STRING(m_iClassname), GetDebugName(), STRING(ev->m_iTarget), STRING(ev->m_iTargetInput), ev->m_flDelay );
	}
	else
	{
		DevMsg( 2, "output: (%s,%s) -> (%s,%s)\n", STRING(m_iClassname), GetDebugName(), STRING(ev->m_iTarget), STRING(ev->m_iTargetInput) );
	}
}


//-----------------------------------------------------------------------------
// Entity events... these are events targetted to a particular entity
// Each event defines its own well-defined event data structure
//-----------------------------------------------------------------------------
void CBaseEntity::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	switch( event )
	{
	case ENTITY_EVENT_WATER_TOUCH:
		{
			int nContents = (int)pEventData;
			if ( !nContents || (nContents & CONTENTS_WATER) )
			{
				++m_nWaterTouch;
			}
			if ( nContents & CONTENTS_SLIME )
			{
				++m_nSlimeTouch;
			}
		}
		break;

	case ENTITY_EVENT_WATER_UNTOUCH:
		{
			int nContents = (int)pEventData;
			if ( !nContents || (nContents & CONTENTS_WATER) )
			{
				--m_nWaterTouch;
			}
			if ( nContents & CONTENTS_SLIME )
			{
				--m_nSlimeTouch;
			}
		}
		break;

	default:
		return;
	}

	// Only do this for vphysics objects
	if ( GetMoveType() != MOVETYPE_VPHYSICS )
		return;

	int nNewContents = 0;
	if ( m_nWaterTouch > 0 )
	{
		nNewContents |= CONTENTS_WATER;
	}

	if ( m_nSlimeTouch > 0 )
	{
 		nNewContents |= CONTENTS_SLIME;
	}

	if (( nNewContents & MASK_WATER ) == 0)
	{
		SetWaterLevel( 0 );
		SetWaterType( CONTENTS_EMPTY );
		return;
	}

	SetWaterLevel( 1 );
	SetWaterType( nNewContents );
}


ConVar ent_messages_draw( "ent_messages_draw", "0", FCVAR_CHEAT, "Visualizes all entity input/output activity." );


//-----------------------------------------------------------------------------
// Purpose: calls the appropriate message mapped function in the entity according
//			to the fired action.
// Input  : char *szInputName - input destination
//			*pActivator - entity which initiated this sequence of actions
//			*pCaller - entity from which this event is sent
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
	if ( ent_messages_draw.GetBool() )
	{
		if ( pCaller != NULL )
		{
			NDebugOverlay::Line( pCaller->GetAbsOrigin(), GetAbsOrigin(), 255, 255, 255, false, 3 );
			NDebugOverlay::Box( pCaller->GetAbsOrigin(), Vector(-4, -4, -4), Vector(4, 4, 4), 255, 0, 0, 0, 3 );
		}

		NDebugOverlay::Text( GetAbsOrigin(), szInputName, false, 3 );	
		NDebugOverlay::Box( GetAbsOrigin(), Vector(-4, -4, -4), Vector(4, 4, 4), 0, 255, 0, 0, 3 );
	}

	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, szInputName) )
				{
					// found a match

					char szBuffer[256];
					// mapper debug message
					if (pCaller != NULL)
					{
						Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input %s: %s.%s(%s)\n", gpGlobals->curtime, STRING(pCaller->m_iName), GetDebugName(), szInputName, Value.String() );
					}
					else
					{
						Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input <NULL>: %s.%s(%s)\n", gpGlobals->curtime, GetDebugName(), szInputName, Value.String() );
					}
					DevMsg( 2, "%s", szBuffer );
					ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );

					if (m_debugOverlays & OVERLAY_MESSAGE_BIT)
					{
						DrawInputOverlay(szInputName,pCaller,Value);
					}

					// convert the value if necessary
					if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
					{
						if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
						{
							if ( !Value.Convert( (fieldtype_t)dmap->dataDesc[i].fieldType ) )
							{
								// bad conversion
								Warning( "!! ERROR: bad input/output link:\n!! %s(%s,%s) doesn't match type from %s(%s)\n", 
									STRING(m_iClassname), GetDebugName(), szInputName, 
									( pCaller != NULL ) ? STRING(pCaller->m_iClassname) : "<null>",
									( pCaller != NULL ) ? STRING(pCaller->m_iName) : "<null>" );
								return false;
							}
						}
					}

					// call the input handler, or if there is none just set the value
					inputfunc_t pfnInput = dmap->dataDesc[i].inputFunc;

					if ( pfnInput )
					{ 
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator;
						data.pCaller = pCaller;
						data.value = Value;
						data.nOutputID = outputID;

						(this->*pfnInput)( data );
					}
					else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
					{
						// set the value directly
						Value.SetOther( ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
					
						// TODO: if this becomes evil and causes too many full entity updates, then we should make
						// a macro like this:
						//
						// define MAKE_INPUTVAR(x) void Note##x##Modified() { x.GetForModify(); }
						//
						// Then the datadesc points at that function and we call it here. The only pain is to add
						// that function for all the DEFINE_INPUT calls.
						NetworkStateChanged();
					}

					return true;
				}
			}
		}
	}

	DevMsg( 2, "unhandled input: (%s) -> (%s,%s)\n", szInputName, STRING(m_iClassname), GetDebugName()/*,", from (%s,%s)" STRING(pCaller->m_iClassname), STRING(pCaller->m_iName)*/ );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for the entity alpha.
// Input  : nAlpha - Alpha value (0 - 255).
//-----------------------------------------------------------------------------
void CBaseEntity::InputAlpha( inputdata_t &inputdata )
{
	SetRenderColorA( clamp( inputdata.value.Int(), 0, 255 ) );
}


//-----------------------------------------------------------------------------
// Activate alternative sorting
//-----------------------------------------------------------------------------
void CBaseEntity::InputAlternativeSorting( inputdata_t &inputdata )
{
	m_bAlternateSorting = inputdata.value.Bool();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for the entity color. Ignores alpha since that is handled
//			by a separate input handler.
// Input  : Color32 new value for color (alpha is ignored).
//-----------------------------------------------------------------------------
void CBaseEntity::InputColor( inputdata_t &inputdata )
{
	color32 clr = inputdata.value.Color32();

	SetRenderColor( clr.r, clr.g, clr.b );
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever the entity is 'Used'.  This can be when a player hits
//			use, or when an entity targets it without an output name (legacy entities)
//-----------------------------------------------------------------------------
void CBaseEntity::InputUse( inputdata_t &inputdata )
{
	Use( inputdata.pActivator, inputdata.pCaller, (USE_TYPE)inputdata.nOutputID, 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Reads an output variable, by string name, from an entity
// Input  : char *varName - the string name of the variable
//			variant_t *var - the value is stored here
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseEntity::ReadKeyField( const char *varName, variant_t *var )
{
	if ( !varName )
		return false;

	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the readable fields in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & (FTYPEDESC_OUTPUT | FTYPEDESC_KEY) )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, varName) )
				{
					var->Set( dmap->dataDesc[i].fieldType, ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] );
					return true;
				}
			}
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the damage filter on the object
//-----------------------------------------------------------------------------
void CBaseEntity::InputEnableDamageForces( inputdata_t &inputdata )
{
	RemoveEFlags( EFL_NO_DAMAGE_FORCES );
}

void CBaseEntity::InputDisableDamageForces( inputdata_t &inputdata )
{
	AddEFlags( EFL_NO_DAMAGE_FORCES );
}

	
//-----------------------------------------------------------------------------
// Purpose: Sets the damage filter on the object
//-----------------------------------------------------------------------------
void CBaseEntity::InputSetDamageFilter( inputdata_t &inputdata )
{
	// Get a handle to my damage filter entity if there is one.
	m_iszDamageFilterName = inputdata.value.StringID();
	if ( m_iszDamageFilterName != NULL_STRING )
	{
		m_hDamageFilter = gEntList.FindEntityByName( NULL, m_iszDamageFilterName );
	}
	else
	{
		m_hDamageFilter = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dispatch effects on this entity
//-----------------------------------------------------------------------------
void CBaseEntity::InputDispatchEffect( inputdata_t &inputdata )
{
	const char *sEffect = inputdata.value.String();
	if ( sEffect && sEffect[0] )
	{
		CEffectData data;
		GetInputDispatchEffectPosition( sEffect, data.m_vOrigin, data.m_vAngles );
		AngleVectors( data.m_vAngles, &data.m_vNormal );
		data.m_vStart = data.m_vOrigin;
		data.m_nEntIndex = entindex();

		// Clip off leading attachment point numbers
		while ( sEffect[0] >= '0' && sEffect[0] <= '9' )
		{
			sEffect++;
		}
		DispatchEffect( sEffect, data );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the origin at which to play an inputted dispatcheffect 
//-----------------------------------------------------------------------------
void CBaseEntity::GetInputDispatchEffectPosition( const char *sInputString, Vector &pOrigin, QAngle &pAngles )
{
	pOrigin = GetAbsOrigin();
	pAngles = GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Marks the entity for deletion
//-----------------------------------------------------------------------------
void CBaseEntity::InputKill( inputdata_t &inputdata )
{
	// tell owner ( if any ) that we're dead.This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	UTIL_Remove( this );
}

void CBaseEntity::InputKillHierarchy( inputdata_t &inputdata )
{
	CBaseEntity *pChild, *pNext;
	for ( pChild = FirstMoveChild(); pChild; pChild = pNext )
	{
		pNext = pChild->NextMovePeer();
		pChild->InputKillHierarchy( inputdata );
	}

	// tell owner ( if any ) that we're dead. This is mostly for NPCMaker functionality.
	CBaseEntity *pOwner = GetOwnerEntity();
	if ( pOwner )
	{
		pOwner->DeathNotice( this );
		SetOwnerEntity( NULL );
	}

	UTIL_Remove( this );
}

//------------------------------------------------------------------------------
// Purpose: Input handler for changing this entity's movement parent.
//------------------------------------------------------------------------------
void CBaseEntity::InputSetParent( inputdata_t &inputdata )
{
	// If we had a parent attachment, clear it, because it's no longer valid.
	if ( m_iParentAttachment )
	{
		m_iParentAttachment = 0;
	}

	SetParent( inputdata.value.StringID(), inputdata.pActivator );
}

//------------------------------------------------------------------------------
// Purpose: 
//------------------------------------------------------------------------------
void CBaseEntity::SetParentAttachment( const char *szInputName, const char *szAttachment, bool bMaintainOffset )
{
	// Must have a parent
	if ( !m_pParent )
	{
		Warning("ERROR: Tried to %s for entity %s (%s), but it has no parent.\n", szInputName, GetClassname(), GetDebugName() );
		return;
	}

	// Valid only on CBaseAnimating
	CBaseAnimating *pAnimating = m_pParent->GetBaseAnimating();
	if ( !pAnimating )
	{
		Warning("ERROR: Tried to %s for entity %s (%s), but its parent has no model.\n", szInputName, GetClassname(), GetDebugName() );
		return;
	}

	// Lookup the attachment
	int iAttachment = pAnimating->LookupAttachment( szAttachment );
	if ( iAttachment <= 0 )
	{
		Warning("ERROR: Tried to %s for entity %s (%s), but it has no attachment named %s.\n", szInputName, GetClassname(), GetDebugName(), szAttachment );
		return;
	}

	m_iParentAttachment = iAttachment;
	SetParent( m_pParent, m_iParentAttachment );

	// Now move myself directly onto the attachment point
	SetMoveType( MOVETYPE_NONE );

	if ( !bMaintainOffset )
	{
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for changing this entity's movement parent's attachment point
//-----------------------------------------------------------------------------
void CBaseEntity::InputSetParentAttachment( inputdata_t &inputdata )
{
	SetParentAttachment( "SetParentAttachment", inputdata.value.String(), false );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for changing this entity's movement parent's attachment point
//-----------------------------------------------------------------------------
void CBaseEntity::InputSetParentAttachmentMaintainOffset( inputdata_t &inputdata )
{
	SetParentAttachment( "SetParentAttachmentMaintainOffset", inputdata.value.String(), true );
}

//------------------------------------------------------------------------------
// Purpose: Input handler for clearing this entity's movement parent.
//------------------------------------------------------------------------------
void CBaseEntity::InputClearParent( inputdata_t &inputdata )
{
	SetParent( NULL );
}


//------------------------------------------------------------------------------
// Purpose : Returns velcocity of base entity.  If physically simulated gets
//			 velocity from physics object
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CBaseEntity::GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity)
{
	if (GetMoveType()==MOVETYPE_VPHYSICS && m_pPhysicsObject)
	{
		m_pPhysicsObject->GetVelocity(vVelocity,vAngVelocity);
	}
	else
	{
		if (vVelocity != NULL)
		{
			*vVelocity = GetAbsVelocity();
		}
		if (vAngVelocity != NULL)
		{
			QAngle tmp = GetLocalAngularVelocity();
			QAngleToAngularImpulse( tmp, *vAngVelocity );
		}
	}
}

bool CBaseEntity::IsMoving()
{ 
	Vector velocity;
	GetVelocity( &velocity, NULL );
	return velocity != vec3_origin; 
}

//-----------------------------------------------------------------------------
// Purpose: Retrieves the coordinate frame for this entity.
// Input  : forward - Receives the entity's forward vector.
//			right - Receives the entity's right vector.
//			up - Receives the entity's up vector.
//-----------------------------------------------------------------------------
void CBaseEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	// This call is necessary to cause m_rgflCoordinateFrame to be recomputed
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pRight ); 
		*pRight *= -1.0f;
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets the model, validates that it's of the appropriate type
// Input  : *szModelName - 
//-----------------------------------------------------------------------------
void CBaseEntity::SetModel( const char *szModelName )
{
	int modelIndex = modelinfo->GetModelIndex( szModelName );
	const model_t *model = modelinfo->GetModel( modelIndex );
	if ( model && modelinfo->GetModelType( model ) != mod_brush )
	{
		Msg( "Setting CBaseEntity to non-brush model %s\n", szModelName );
	}
	UTIL_SetModel( this, szModelName );
}

//------------------------------------------------------------------------------

CStudioHdr *CBaseEntity::OnNewModel()
{
	// Do nothing.
	return NULL;
}


//================================================================================
// TEAM HANDLING
//================================================================================
void CBaseEntity::InputSetTeam( inputdata_t &inputdata )
{
	ChangeTeam( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: Put the entity in the specified team
//-----------------------------------------------------------------------------
void CBaseEntity::ChangeTeam( int iTeamNum )
{
	m_iTeamNum = iTeamNum;
}

//-----------------------------------------------------------------------------
// Get the Team this entity is on
//-----------------------------------------------------------------------------
CTeam *CBaseEntity::GetTeam( void ) const
{
	return GetGlobalTeam( m_iTeamNum );
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if these players are both in at least one team together
//-----------------------------------------------------------------------------
bool CBaseEntity::InSameTeam( CBaseEntity *pEntity ) const
{
	if ( !pEntity )
		return false;

	return ( pEntity->GetTeam() == GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the string name of the players team
//-----------------------------------------------------------------------------
const char *CBaseEntity::TeamID( void ) const
{
	if ( GetTeam() == NULL )
		return "";

	return GetTeam()->GetName();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if the player is on the same team
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInTeam( CTeam *pTeam ) const
{
	return ( GetTeam() == pTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CBaseEntity::GetTeamNumber( void ) const
{
	return m_iTeamNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseEntity::IsInAnyTeam( void ) const
{
	return ( GetTeam() != NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the type of damage that this entity inflicts.
//-----------------------------------------------------------------------------
int CBaseEntity::GetDamageType() const
{
	return DMG_GENERIC;
}


//-----------------------------------------------------------------------------
// process notification
//-----------------------------------------------------------------------------

void CBaseEntity::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
}


//-----------------------------------------------------------------------------
// Purpose: Holds an entity's previous abs origin and angles at the time of
//			teleportation. Used for child & constrained entity fixup to prevent
//			lazy updates of abs origins and angles from messing things up.
//-----------------------------------------------------------------------------
struct TeleportListEntry_t
{
	CBaseEntity *pEntity;
	Vector prevAbsOrigin;
	QAngle prevAbsAngles;
};


static void TeleportEntity( CBaseEntity *pSourceEntity, TeleportListEntry_t &entry, const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	CBaseEntity *pTeleport = entry.pEntity;
	Vector prevOrigin = entry.prevAbsOrigin;
	QAngle prevAngles = entry.prevAbsAngles;

	int nSolidFlags = pTeleport->GetSolidFlags();
	pTeleport->AddSolidFlags( FSOLID_NOT_SOLID );

	// I'm teleporting myself
	if ( pSourceEntity == pTeleport )
	{
		if ( newAngles )
		{
			pTeleport->SetLocalAngles( *newAngles );
			if ( pTeleport->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pTeleport;
				pPlayer->SnapEyeAngles( *newAngles );
			}
		}

		if ( newVelocity )
		{
			pTeleport->SetAbsVelocity( *newVelocity );
			pTeleport->SetBaseVelocity( vec3_origin );
		}

		if ( newPosition )
		{
			pTeleport->IncrementInterpolationFrame();
			UTIL_SetOrigin( pTeleport, *newPosition );
		}
	}
	else
	{
		// My parent is teleporting, just update my position & physics
		pTeleport->CalcAbsolutePosition();
	}
	IPhysicsObject *pPhys = pTeleport->VPhysicsGetObject();
	bool rotatePhysics = false;

	// handle physics objects / shadows
	if ( pPhys )
	{
		if ( newVelocity )
		{
			pPhys->SetVelocity( newVelocity, NULL );
		}
		const QAngle *rotAngles = &pTeleport->GetAbsAngles();
		// don't rotate physics on players or bbox entities
		if (pTeleport->IsPlayer() || pTeleport->GetSolid() == SOLID_BBOX )
		{
			rotAngles = &vec3_angle;
		}
		else
		{
			rotatePhysics = true;
		}

		pPhys->SetPosition( pTeleport->GetAbsOrigin(), *rotAngles, true );
	}

	g_pNotify->ReportTeleportEvent( pTeleport, prevOrigin, prevAngles, rotatePhysics );

	pTeleport->SetSolidFlags( nSolidFlags );
}


//-----------------------------------------------------------------------------
// Purpose: Recurses an entity hierarchy and fills out a list of all entities
//			in the hierarchy with their current origins and angles.
//
//			This list is necessary to keep lazy updates of abs origins and angles
//			from messing up our child/constrained entity fixup.
//-----------------------------------------------------------------------------
static void BuildTeleportList_r( CBaseEntity *pTeleport, CUtlVector<TeleportListEntry_t> &teleportList )
{
	TeleportListEntry_t entry;
	
	entry.pEntity = pTeleport;
	entry.prevAbsOrigin = pTeleport->GetAbsOrigin();
	entry.prevAbsAngles = pTeleport->GetAbsAngles();

	teleportList.AddToTail( entry );

	CBaseEntity *pList = pTeleport->FirstMoveChild();
	while ( pList )
	{
		BuildTeleportList_r( pList, teleportList );
		pList = pList->NextMovePeer();
	}
}


static CUtlVector<CBaseEntity *> g_TeleportStack;
void CBaseEntity::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	if ( g_TeleportStack.Find( this ) >= 0 )
		return;
	int index = g_TeleportStack.AddToTail( this );

	CUtlVector<TeleportListEntry_t> teleportList;
	BuildTeleportList_r( this, teleportList );

	int i;
	for ( i = 0; i < teleportList.Count(); i++)
	{
		TeleportEntity( this, teleportList[i], newPosition, newAngles, newVelocity );
	}

	for (i = 0; i < teleportList.Count(); i++)
	{
		teleportList[i].pEntity->CollisionRulesChanged();
	}

	if ( IsPlayer() )
	{
		// Tell the client being teleported
		IGameEvent *event = gameeventmanager->CreateEvent( "base_player_teleported" );
		if ( event )
		{
			event->SetInt( "entindex", entindex() );
			gameeventmanager->FireEventClientSide( event );
		}
	}

	Assert( g_TeleportStack[index] == this );
	g_TeleportStack.FastRemove( index );

	// FIXME: add an initializer function to StepSimulationData
	StepSimulationData *step = ( StepSimulationData * )GetDataObject( STEPSIMULATION );
	if (step)
	{
		Q_memset( step, 0, sizeof( *step ) );
	}
}

// Stuff implemented for weapon prediction code
void CBaseEntity::SetSize( const Vector &vecMin, const Vector &vecMax )
{
	UTIL_SetSize( this, vecMin, vecMax );
}

CStudioHdr *ModelSoundsCache_LoadModel( const char *filename )
{
	// Load the file
	int idx = engine->PrecacheModel( filename, true );
	if ( idx != -1 )
	{
		model_t *mdl = (model_t *)modelinfo->GetModel( idx );
		if ( mdl )
		{
			CStudioHdr *studioHdr = new CStudioHdr( modelinfo->GetStudiomodel( mdl ), mdlcache ); 
			if ( studioHdr->IsValid() )
			{
				return studioHdr;
			}
		}
	}
	return NULL;
}

void ModelSoundsCache_FinishModel( CStudioHdr *hdr )
{
	Assert( hdr );
	delete hdr;
}

void ModelSoundsCache_PrecacheScriptSound( const char *soundname )
{
	CBaseEntity::PrecacheScriptSound( soundname );
}

static CUtlCachedFileData< CModelSoundsCache > g_ModelSoundsCache( "modelsounds.cache", MODELSOUNDSCACHE_VERSION, 0, UTL_CACHED_FILE_USE_FILESIZE, false );																  

void ClearModelSoundsCache()
{
	if ( IsX360() )
	{
		return;
	}

	g_ModelSoundsCache.Reload();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool ModelSoundsCacheInit()
{
	if ( IsX360() )
	{
		return true;
	}

	return g_ModelSoundsCache.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ModelSoundsCacheShutdown()
{
	if ( IsX360() )
	{
		return;
	}

	g_ModelSoundsCache.Shutdown();
}

static CUtlSymbolTable g_ModelSoundsSymbolHelper( 0, 32, true );
class CModelSoundsCacheSaver: public CAutoGameSystem
{
public:
	CModelSoundsCacheSaver( const char *name ) : CAutoGameSystem( name )
	{
	}
	virtual void LevelInitPostEntity()
	{
		if ( IsX360() )
		{
			return;
		}

		if ( g_ModelSoundsCache.IsDirty() )
		{
			g_ModelSoundsCache.Save();
		}
	}
	virtual void LevelShutdownPostEntity()
	{
		if ( IsX360() )
		{
			// Unforunate that this table must persist through duration of level.
			// It is the common case that PrecacheModel() still gets called (and needs this table),
			// after LevelInitPostEntity, as PrecacheModel() redundantly precaches.
			g_ModelSoundsSymbolHelper.RemoveAll();
			return;
		}

		if ( g_ModelSoundsCache.IsDirty() )
		{
			g_ModelSoundsCache.Save();
		}
	}
};

static CModelSoundsCacheSaver g_ModelSoundsCacheSaver( "CModelSoundsCacheSaver" );

//#define WATCHACCESS
#if defined( WATCHACCESS )

static bool g_bWatching = true;

void ModelLogFunc( const char *fileName, const char *accessType )
{
	if ( g_bWatching && !CBaseEntity::IsPrecacheAllowed() )
	{
		if ( Q_stristr( fileName, ".vcd" ) )
		{
			Msg( "%s\n", fileName );
		}
	}
}

class CWatchForModelAccess: public CAutoGameSystem
{
public:
	virtual bool Init()
	{
		filesystem->AddLoggingFunc(&ModelLogFunc);
		return true;
	}

	virtual void Shutdown()
	{
		filesystem->RemoveLoggingFunc(&ModelLogFunc);
	}

};

static CWatchForModelAccess g_WatchForModels;

#endif

// HACK:  This must match the #define in cl_animevent.h in the client .dll code!!!
#define CL_EVENT_SOUND				5004
#define CL_EVENT_FOOTSTEP_LEFT		6004
#define CL_EVENT_FOOTSTEP_RIGHT		6005
#define CL_EVENT_MFOOTSTEP_LEFT		6006
#define CL_EVENT_MFOOTSTEP_RIGHT	6007

//-----------------------------------------------------------------------------
// Precache model sound. Requires a local symbol table to prevent
// a very expensive call to PrecacheScriptSound().
//-----------------------------------------------------------------------------
void CBaseEntity::PrecacheSoundHelper( const char *pName )
{
	if ( !IsX360() )
	{
		// 360 only
		Assert( 0 );
		return;
	}

	if ( !pName || !pName[0] )
	{
		return;
	}

	if ( UTL_INVAL_SYMBOL == g_ModelSoundsSymbolHelper.Find( pName ) )
	{
		g_ModelSoundsSymbolHelper.AddString( pName );

		// very expensive, only call when required
		PrecacheScriptSound( pName );
	}
}

//-----------------------------------------------------------------------------
// Precache model components
//-----------------------------------------------------------------------------
void CBaseEntity::PrecacheModelComponents( int nModelIndex )
{

	model_t *pModel = (model_t *)modelinfo->GetModel( nModelIndex );
	if ( !pModel || modelinfo->GetModelType( pModel ) != mod_studio )
	{
		return;
	}

	// sounds
	if ( IsPC() )
	{
		const char *name = modelinfo->GetModelName( pModel );
		if ( !g_ModelSoundsCache.EntryExists( name ) )
		{
			char extension[ 8 ];
			Q_ExtractFileExtension( name, extension, sizeof( extension ) );

			if ( Q_stristr( extension, "mdl" ) )
			{
				DevMsg( 2, "Late precache of %s, need to rebuild modelsounds.cache\n", name );
			}
			else
			{
				if ( !extension[ 0 ] )
				{
					Warning( "Precache of %s ambigious (no extension specified)\n", name );
				}
				else
				{
					Warning( "Late precache of %s (file missing?)\n", name );
				}
				return;
			}
		}

		CModelSoundsCache *entry = g_ModelSoundsCache.Get( name );
		Assert( entry );
		if ( entry )
		{
			entry->PrecacheSoundList();
		}
	}

	// particles
	{
		// Check keyvalues for auto-emitting particles
		KeyValues *pModelKeyValues = new KeyValues("");
		KeyValues::AutoDelete autodelete_pModelKeyValues( pModelKeyValues );
		if ( pModelKeyValues->LoadFromBuffer( modelinfo->GetModelName( pModel ), modelinfo->GetModelKeyValueText( pModel ) ) )
		{
			KeyValues *pParticleEffects = pModelKeyValues->FindKey("Particles");
			if ( pParticleEffects )
			{						   
				// Start grabbing the sounds and slotting them in
				for ( KeyValues *pSingleEffect = pParticleEffects->GetFirstSubKey(); pSingleEffect; pSingleEffect = pSingleEffect->GetNextKey() )
				{
					const char *pParticleEffectName = pSingleEffect->GetString( "name", "" );
					PrecacheParticleSystem( pParticleEffectName );
				}
			}
		}
	}

	// model anim event owned components
	{
		// Check animevents for particle events
		CStudioHdr studioHdr( modelinfo->GetStudiomodel( pModel ), mdlcache ); 
		if ( studioHdr.IsValid() )
		{
			// force animation event resolution!!!
			VerifySequenceIndex( &studioHdr );

			int nSeqCount = studioHdr.GetNumSeq();
			for ( int i = 0; i < nSeqCount; ++i )
			{
				mstudioseqdesc_t &seq = studioHdr.pSeqdesc( i );
				int nEventCount = seq.numevents;
				for ( int j = 0; j < nEventCount; ++j )
				{
					mstudioevent_t *pEvent = seq.pEvent( j );

					if ( !( pEvent->type & AE_TYPE_NEWEVENTSYSTEM ) || ( pEvent->type & AE_TYPE_CLIENT ) )
					{
						if ( pEvent->event == AE_CL_CREATE_PARTICLE_EFFECT )
						{
							char token[256];
							const char *pOptions = pEvent->pszOptions();
							nexttoken( token, pOptions, ' ' );
							if ( token[0] ) 
							{
								PrecacheParticleSystem( token );
							}
							continue;
						}
					}

					// 360 precaches the model sounds now at init time, the cost is now ~250 msecs worst case.
					// The disk based solution was not needed. Now at runtime partly due to already crawling the sequences
					// for the particles and the expensive part was redundant PrecacheScriptSound(), which is now prevented
					// by a local symbol table.
					if ( IsX360() )
					{
						switch ( pEvent->event )
						{
						default:
							{
								if ( ( pEvent->type & AE_TYPE_NEWEVENTSYSTEM ) && ( pEvent->event == AE_SV_PLAYSOUND ) )
								{
									PrecacheSoundHelper( pEvent->pszOptions() );
								}
							}
							break;
						case CL_EVENT_FOOTSTEP_LEFT:
						case CL_EVENT_FOOTSTEP_RIGHT:
							{
								char soundname[256];
								char const *options = pEvent->pszOptions();
								if ( !options || !options[0] )
								{
									options = "NPC_CombineS";
								}

								Q_snprintf( soundname, sizeof( soundname ), "%s.RunFootstepLeft", options );
								PrecacheSoundHelper( soundname );
								Q_snprintf( soundname, sizeof( soundname ), "%s.RunFootstepRight", options );
								PrecacheSoundHelper( soundname );
								Q_snprintf( soundname, sizeof( soundname ), "%s.FootstepLeft", options );
								PrecacheSoundHelper( soundname );
								Q_snprintf( soundname, sizeof( soundname ), "%s.FootstepRight", options );
								PrecacheSoundHelper( soundname );
							}
							break;
						case AE_CL_PLAYSOUND:
							{
								if ( !( pEvent->type & AE_TYPE_CLIENT ) )
									break;

								if ( pEvent->pszOptions()[0] )
								{
									PrecacheSoundHelper( pEvent->pszOptions() );
								}
								else
								{
									Warning( "-- Error --:  empty soundname, .qc error on AE_CL_PLAYSOUND in model %s, sequence %s, animevent # %i\n", 
										studioHdr.GetRenderHdr()->pszName(), seq.pszLabel(), j+1 );
								}
							}
							break;
						case CL_EVENT_SOUND:
						case SCRIPT_EVENT_SOUND:
						case SCRIPT_EVENT_SOUND_VOICE:
							{
								PrecacheSoundHelper( pEvent->pszOptions() );
							}
							break;
						}
					}
				}
			}
		}
	}
}


			
//-----------------------------------------------------------------------------
// Purpose: Add model to level precache list
// Input  : *name - model name
// Output : int -- model index for model
//-----------------------------------------------------------------------------
int CBaseEntity::PrecacheModel( const char *name, bool bPreload )
{
	if ( !name || !*name )
	{
#ifdef STAGING_ONLY
		Msg( "Attempting to precache model, but model name is NULL\n");
#endif
		return -1;
	}

	// Warn on out of order precache
	if ( !CBaseEntity::IsPrecacheAllowed() )
	{
		if ( !engine->IsModelPrecached( name ) )
		{
			DevMsg( "Late precache of %s -- not necessarily a bug now that we allow ~everything to be dynamically loaded.\n", name );
		}
	}
#if defined( WATCHACCESS )
	else
	{
		g_bWatching = false;
	}
#endif

	int idx = engine->PrecacheModel( name, bPreload );
	if ( idx != -1 )
	{
		PrecacheModelComponents( idx );
	}

#if defined( WATCHACCESS )
	g_bWatching = true;
#endif

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::Remove( )
{
	UTIL_Remove( this );
}

//   Entity degugging console commands
extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
extern void			SetDebugBits( CBasePlayer* pPlayer, const char *name, int bit );
extern CBaseEntity *GetNextCommandEntity( CBasePlayer *pPlayer, const char *name, CBaseEntity *ent );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void ConsoleFireTargets( CBasePlayer *pPlayer, const char *name)
{
	// If no name was given use the picker
	if (FStrEq(name,"")) 
	{
		CBaseEntity *pEntity = FindPickerEntity( pPlayer );
		if ( pEntity && !pEntity->IsMarkedForDeletion())
		{
			Msg( "[%03d] Found: %s, firing\n", gpGlobals->tickcount%1000, pEntity->GetDebugName());
			pEntity->Use( pPlayer, pPlayer, USE_TOGGLE, 0 );
			return;
		}
	}
	// Otherwise use name or classname
	FireTargets( name, pPlayer, pPlayer, USE_TOGGLE, 0 );
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Name( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_NAME_BIT);
}
static ConCommand ent_name("ent_name", CC_Ent_Name, 0, FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_Text( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_TEXT_BIT);
}
static ConCommand ent_text("ent_text", CC_Ent_Text, "Displays text debugging information about the given entity(ies) on top of the entity (See Overlay Text)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_BBox( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_BBOX_BIT);
}
static ConCommand ent_bbox("ent_bbox", CC_Ent_BBox, "Displays the movement bounding box for the given entity(ies) in orange.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


//------------------------------------------------------------------------------
void CC_Ent_AbsBox( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_ABSBOX_BIT);
}
static ConCommand ent_absbox("ent_absbox", CC_Ent_AbsBox, "Displays the total bounding box for the given entity(s) in green.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);


//------------------------------------------------------------------------------
void CC_Ent_RBox( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_RBOX_BIT);
}
static ConCommand ent_rbox("ent_rbox", CC_Ent_RBox, "Displays the total bounding box for the given entity(s) in green.  Some entites will also display entity specific overlays.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_AttachmentPoints( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_ATTACHMENTS_BIT);
}
static ConCommand ent_attachments("ent_attachments", CC_Ent_AttachmentPoints, "Displays the attachment points on an entity.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_ViewOffset( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_VIEWOFFSET);
}
static ConCommand ent_viewoffset("ent_viewoffset", CC_Ent_ViewOffset, "Displays the eye position for the given entity(ies) in red.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_Remove( const CCommand& args )
{
	CBaseEntity *pEntity = NULL;

	// If no name was given set bits based on the picked
	if ( FStrEq( args[1],"") ) 
	{
		pEntity = FindPickerEntity( UTIL_GetCommandClient() );
	}
	else 
	{
		int index = atoi( args[1] );
		if ( index )
		{
			pEntity = CBaseEntity::Instance( index );
		}
		else
		{
			// Otherwise set bits based on name or classname
			CBaseEntity *ent = NULL;
			while ( (ent = gEntList.NextEnt(ent)) != NULL )
			{
				if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(args[1], STRING(ent->GetEntityName())))	|| 
					(ent->m_iClassname != NULL_STRING	&& FStrEq(args[1], STRING(ent->m_iClassname))) ||
					(ent->GetClassname()!=NULL && FStrEq(args[1], ent->GetClassname())))
				{
					pEntity = ent;
					break;
				}
			}
		}
	}

	// Found one?
	if ( pEntity )
	{
		Msg( "Removed %s(%s)\n", STRING(pEntity->m_iClassname), pEntity->GetDebugName() );
		UTIL_Remove( pEntity );
	}
}
static ConCommand ent_remove("ent_remove", CC_Ent_Remove, "Removes the given entity(s)\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_RemoveAll( const CCommand& args )
{
	// If no name was given remove based on the picked
	if ( args.ArgC() < 2 )
	{
		Msg( "Removes all entities of the specified type\n\tArguments:   	{entity_name} / {class_name}\n" );
	}
	else 
	{
		// Otherwise remove based on name or classname
		int iCount = 0;
		CBaseEntity *ent = NULL;
		while ( (ent = gEntList.NextEnt(ent)) != NULL )
		{
			if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(args[1], STRING(ent->GetEntityName())))	|| 
				  (ent->m_iClassname != NULL_STRING	&& FStrEq(args[1], STRING(ent->m_iClassname))) ||
				  (ent->GetClassname()!=NULL && FStrEq(args[1], ent->GetClassname())))
			{
				UTIL_Remove( ent );
				iCount++;
			}
		}

		if ( iCount )
		{
			Msg( "Removed %d %s's\n", iCount, args[1] );
		}
		else
		{
			Msg( "No %s found.\n", args[1] );
		}
	}
}
static ConCommand ent_remove_all("ent_remove_all", CC_Ent_RemoveAll, "Removes all entities of the specified type\n\tArguments:   	{entity_name} / {class_name} ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Ent_SetName( const CCommand& args )
{
	CBaseEntity *pEntity = NULL;

	if ( args.ArgC() < 1 )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
		if (!pPlayer)
			return;

		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_setname <new name> <entity name>\n" );
	}
	else
	{
		// If no name was given set bits based on the picked
		if ( FStrEq( args[2],"") ) 
		{
			pEntity = FindPickerEntity( UTIL_GetCommandClient() );
		}
		else 
		{
			// Otherwise set bits based on name or classname
			CBaseEntity *ent = NULL;
			while ( (ent = gEntList.NextEnt(ent)) != NULL )
			{
				if (  (ent->GetEntityName() != NULL_STRING	&& FStrEq(args[1], STRING(ent->GetEntityName())))	|| 
					  (ent->m_iClassname != NULL_STRING	&& FStrEq(args[1], STRING(ent->m_iClassname))) ||
					  (ent->GetClassname()!=NULL && FStrEq(args[1], ent->GetClassname())))
				{
					pEntity = ent;
					break;
				}
			}
		}

		// Found one?
		if ( pEntity )
		{
			Msg( "Set the name of %s to %s\n", STRING(pEntity->m_iClassname), args[1] );
			pEntity->SetName( AllocPooledString( args[1] ) );
		}
	}
}
static ConCommand ent_setname("ent_setname", CC_Ent_SetName, "Sets the targetname of the given entity(s)\n\tArguments:   	{new entity name} {entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Find_Ent( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Total entities: %d (%d edicts)\n", gEntList.NumberOfEntities(), gEntList.NumberOfEdicts() );
		Msg( "Format: find_ent <substring>\n" );
		return;
	}

	int iCount = 0;
 	const char *pszSubString = args[1];
	Msg("Searching for entities with class/target name containing substring: '%s'\n", pszSubString );

	CBaseEntity *ent = NULL;
	while ( (ent = gEntList.NextEnt(ent)) != NULL )
	{
		const char *pszClassname = ent->GetClassname();
		const char *pszTargetname = STRING(ent->GetEntityName());

		bool bMatches = false;
		if ( pszClassname && pszClassname[0] )
		{
			if ( Q_stristr( pszClassname, pszSubString ) )
			{
				bMatches = true;
			}
		}

		if ( !bMatches && pszTargetname && pszTargetname[0] )
		{
			if ( Q_stristr( pszTargetname, pszSubString ) )
			{
				bMatches = true;
			}
		}

		if ( bMatches )
		{
 			iCount++;
			Msg("   '%s' : '%s' (entindex %d) \n", ent->GetClassname(), ent->GetEntityName().ToCStr(), ent->entindex() );
		}
	}

	Msg("Found %d matches.\n", iCount);
}
static ConCommand find_ent("find_ent", CC_Find_Ent, "Find and list all entities with classnames or targetnames that contain the specified substring.\nFormat: find_ent <substring>\n", FCVAR_CHEAT);

//------------------------------------------------------------------------------
void CC_Find_Ent_Index( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Format: find_ent_index <index>\n" );
		return;
	}

	int iIndex = atoi(args[1]);
	CBaseEntity	*pEnt = UTIL_EntityByIndex( iIndex );
	if ( pEnt )
	{
		Msg("   '%s' : '%s' (entindex %d) \n", pEnt->GetClassname(), pEnt->GetEntityName().ToCStr(), iIndex );
	}
	else
	{
		Msg("Found no entity at %d.\n", iIndex);
	}
}
static ConCommand find_ent_index("find_ent_index", CC_Find_Ent_Index, "Display data for entity matching specified index.\nFormat: find_ent_index <index>\n", FCVAR_CHEAT);

// Purpose : 
//------------------------------------------------------------------------------
void CC_Ent_Dump( const CCommand& args )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
	{
		return;
	}

	if ( args.ArgC() < 2 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_dump <entity name>\n" );
	}
	else
	{
		// iterate through all the ents of this name, printing out their details
		CBaseEntity *ent = NULL;
		bool bFound = false;
		while ( ( ent = gEntList.FindEntityByName(ent, args[1] ) ) != NULL )
		{
			bFound = true;
			for ( datamap_t *dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					variant_t var;
					if ( ent->ReadKeyField( dmap->dataDesc[i].externalName, &var) )
					{
						char buf[256];
						buf[0] = 0;
						switch( var.FieldType() )
						{
						case FIELD_STRING:
							Q_strncpy( buf, var.String() ,sizeof(buf));
							break;
						case FIELD_INTEGER:
							if ( var.Int() )
								Q_snprintf( buf,sizeof(buf), "%d", var.Int() );
							break;
						case FIELD_FLOAT:
							if ( var.Float() )
								Q_snprintf( buf,sizeof(buf), "%.2f", var.Float() );
							break;
						case FIELD_EHANDLE:
							{
								// get the entities name
								if ( var.Entity() )
								{
									Q_snprintf( buf,sizeof(buf), "%s", STRING(var.Entity()->GetEntityName()) );
								}
							}
							break;
						}

						// don't print out the duplicate keys
						if ( !Q_stricmp("parentname",dmap->dataDesc[i].externalName) || !Q_stricmp("targetname",dmap->dataDesc[i].externalName) )
							continue;

						// don't print out empty keys
						if ( buf[0] )
						{
							ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  %s: %s\n", dmap->dataDesc[i].externalName, buf) );
						}
					}
				}
			}
		}

		if ( !bFound )
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "ent_dump: no such entity" );
		}
	}
}
static ConCommand ent_dump("ent_dump", CC_Ent_Dump, "Usage:\n   ent_dump <entity name>\n", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_FireTarget( const CCommand& args )
{
	ConsoleFireTargets(UTIL_GetCommandClient(),args[1]);
}
static ConCommand firetarget("firetarget", CC_Ent_FireTarget, 0, FCVAR_CHEAT);

class CEntFireAutoCompletionFunctor : public ICommandCallback, public ICommandCompletionCallback
{
public:
	virtual void CommandCallback( const CCommand &command )
	{
		CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
		if (!pPlayer)
		{
			return;
		}

		// fires a command from the console
		if ( command.ArgC() < 2 )
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_fire <target> [action] [value] [delay]\n" );
		}
		else
		{
			const char *target = "", *action = "Use";
			variant_t value;
			int delay = 0;

			target = STRING( AllocPooledString(command.Arg( 1 ) ) );

			// Don't allow them to run anything on a point_servercommand unless they're the host player. Otherwise they can ent_fire
			// and run any command on the server. Admittedly, they can only do the ent_fire if sv_cheats is on, but 
			// people complained about users resetting the rcon password if the server briefly turned on cheats like this:
			//    give point_servercommand
			//    ent_fire point_servercommand command "rcon_password mynewpassword"
			//
			// Robin: Unfortunately, they get around point_servercommand checks with this:
			//	  ent_create point_servercommand; ent_setname mine; ent_fire mine command "rcon_password mynewpassword"
			// So, I'm removing the ability for anyone to execute ent_fires on dedicated servers (we can't check to see if
			// this command is going to connect with a point_servercommand entity here, because they could delay the event and create it later).
			if ( engine->IsDedicatedServer() )
			{
				// We allow people with disabled autokick to do it, because they already have rcon.
				if ( pPlayer->IsAutoKickDisabled() == false )
					return;
			}
			else if ( gpGlobals->maxClients > 1 )
			{
				// On listen servers with more than 1 player, only allow the host to issue ent_fires.
				CBasePlayer *pHostPlayer = UTIL_GetListenServerHost();
				if ( pPlayer != pHostPlayer )
					return;
			}

			if ( command.ArgC() >= 3 )
			{
				action = STRING( AllocPooledString(command.Arg( 2 )) );
			}
			if ( command.ArgC() >= 4 )
			{
				value.SetString( AllocPooledString(command.Arg( 3 )) );
			}
			if ( command.ArgC() >= 5 )
			{
				delay = atoi( command.Arg( 4 ) );
			}

			g_EventQueue.AddEvent( target, action, value, delay, pPlayer, pPlayer );
		}
	}

	virtual int CommandCompletionCallback( const char *partial, CUtlVector< CUtlString > &commands )
	{
		if ( !g_pGameRules )
		{
			return 0;
		}

		const char *cmdname = "ent_fire";

		char *substring = (char *)partial;
		if ( Q_strstr( partial, cmdname ) )
		{
			substring = (char *)partial + strlen( cmdname ) + 1;
		}

		int checklen = 0;
		char *space = Q_strstr( substring, " " );
		if ( space )
		{
			return EntFire_AutoCompleteInput( partial, commands );;
		}
		else
		{
			checklen = Q_strlen( substring );
		}

		CUtlRBTree< CUtlString > symbols( 0, 0, UtlStringLessFunc );

		CBaseEntity *pos = NULL;
		while ( ( pos = gEntList.NextEnt( pos ) ) != NULL )
		{
			// Check target name against partial string
			if ( pos->GetEntityName() == NULL_STRING )
				continue;

			if ( Q_strnicmp( STRING( pos->GetEntityName() ), substring, checklen ) )
				continue;

			CUtlString sym = STRING( pos->GetEntityName() );
			int idx = symbols.Find( sym );
			if ( idx == symbols.InvalidIndex() )
			{
				symbols.Insert( sym );
			}

			// Too many
			if ( symbols.Count() >= COMMAND_COMPLETION_MAXITEMS )
				break;
		}

		// Now fill in the results
		for ( int i = symbols.FirstInorder(); i != symbols.InvalidIndex(); i = symbols.NextInorder( i ) )
		{
			const char *name = symbols[ i ].String();

			char buf[ 512 ];
			Q_strncpy( buf, name, sizeof( buf ) );
			Q_strlower( buf );

			CUtlString command;
			command = CFmtStr( "%s %s", cmdname, buf );
			commands.AddToTail( command );
		}

		return symbols.Count();
	}
private:
	int EntFire_AutoCompleteInput( const char *partial, CUtlVector< CUtlString > &commands )
	{
		const char *cmdname = "ent_fire";

		char *substring = (char *)partial;
		if ( Q_strstr( partial, cmdname ) )
		{
			substring = (char *)partial + strlen( cmdname ) + 1;
		}

		int checklen = 0;
		char *space = Q_strstr( substring, " " );
		if ( !space )
		{
			Assert( !"CC_EntFireAutoCompleteInputFunc is broken\n" );
			return 0;
		}

		checklen = Q_strlen( substring );

		char targetEntity[ 256 ];
		targetEntity[0] = 0;
		int nEntityNameLength = (space-substring);
		Q_strncat( targetEntity, substring, sizeof( targetEntity ), nEntityNameLength );

		// Find the target entity by name
		CBaseEntity *target = gEntList.FindEntityByName( NULL, targetEntity );
		if ( target == NULL )
			return 0;

		CUtlRBTree< CUtlString > symbols( 0, 0, UtlStringLessFunc );

		// Find the next portion of the text chain, if any (removing space)
		int nInputNameLength = (checklen-nEntityNameLength-1);

		// Starting past the last space, this is the remainder of the string
		char *inputPartial = ( checklen > nEntityNameLength ) ? (space+1) : NULL;

		for ( datamap_t *dmap = target->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			// Make sure we don't keep adding things in if the satisfied the limit
			if ( symbols.Count() >= COMMAND_COMPLETION_MAXITEMS )
				break;

			int c = dmap->dataNumFields;
			for ( int i = 0; i < c; i++ )
			{
				typedescription_t *field = &dmap->dataDesc[ i ];

				// Only want inputs
				if ( !( field->flags & FTYPEDESC_INPUT ) )
					continue;

				// Only want input functions
				if ( field->flags & FTYPEDESC_SAVE )
					continue;

				// See if we've got a partial string for the input name already
				if ( inputPartial != NULL )
				{
					if ( Q_strnicmp( inputPartial, field->externalName, nInputNameLength ) )
						continue;
				}

				CUtlString sym = field->externalName;

				int idx = symbols.Find( sym );
				if ( idx == symbols.InvalidIndex() )
				{
					symbols.Insert( sym );
				}

				// Too many items have been added
				if ( symbols.Count() >= COMMAND_COMPLETION_MAXITEMS )
					break;
			}
		}

		// Now fill in the results
		for ( int i = symbols.FirstInorder(); i != symbols.InvalidIndex(); i = symbols.NextInorder( i ) )
		{
			const char *name = symbols[ i ].String();

			char buf[ 512 ];
			Q_strncpy( buf, name, sizeof( buf ) );
			Q_strlower( buf );

			CUtlString command;
			command = CFmtStr( "%s %s %s", cmdname, targetEntity, buf );
			commands.AddToTail( command );
		}

		return symbols.Count();
	}
};

static CEntFireAutoCompletionFunctor g_EntFireAutoComplete;
static ConCommand ent_fire("ent_fire", &g_EntFireAutoComplete, "Usage:\n   ent_fire <target> [action] [value] [delay]\n", FCVAR_CHEAT, &g_EntFireAutoComplete );

void CC_Ent_CancelPendingEntFires( const CCommand& args )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;

	g_EventQueue.CancelEvents( pPlayer );
}
static ConCommand ent_cancelpendingentfires("ent_cancelpendingentfires", CC_Ent_CancelPendingEntFires, "Cancels all ent_fire created outputs that are currently waiting for their delay to expire." );

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Info( const CCommand& args )
{
	CBasePlayer *pPlayer = ToBasePlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
	{
		return;
	}
	
	if ( args.ArgC() < 2 )
	{
		ClientPrint( pPlayer, HUD_PRINTCONSOLE, "Usage:\n   ent_info <class name>\n" );
	}
	else
	{
		// iterate through all the ents printing out their details
		CBaseEntity *ent = CreateEntityByName( args[1] );

		if ( ent )
		{
			datamap_t *dmap;
			for ( dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					if ( dmap->dataDesc[i].flags & FTYPEDESC_OUTPUT )
					{
						ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  output: %s\n", dmap->dataDesc[i].externalName) );
					}
				}
			}

			for ( dmap = ent->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
			{
				// search through all the actions in the data description, printing out details
				for ( int i = 0; i < dmap->dataNumFields; i++ )
				{
					if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
					{
						ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("  input: %s\n", dmap->dataDesc[i].externalName) );
					}
				}
			}

			delete ent;
		}
		else
		{
			ClientPrint( pPlayer, HUD_PRINTCONSOLE, UTIL_VarArgs("no such entity %s\n", args[1]) );
		}
	}
}
static ConCommand ent_info("ent_info", CC_Ent_Info, "Usage:\n   ent_info <class name>\n", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Messages( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_MESSAGE_BIT);
}
static ConCommand ent_messages("ent_messages", CC_Ent_Messages ,"Toggles input/output message display for the selected entity(ies).  The name of the entity will be displayed as well as any messages that it sends or receives.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Pause( void )
{
	if (CBaseEntity::Debug_IsPaused())
	{
		Msg( "Resuming entity I/O events\n" );
		CBaseEntity::Debug_Pause(false);
	}
	else
	{
		Msg( "Pausing entity I/O events\n" );
		CBaseEntity::Debug_Pause(true);
	}
}
static ConCommand ent_pause("ent_pause", CC_Ent_Pause, "Toggles pausing of input/output message processing for entities.  When turned on processing of all message will stop.  Any messages displayed with 'ent_messages' will stop fading and be displayed indefinitely. To step through the messages one by one use 'ent_step'.", FCVAR_CHEAT);


//------------------------------------------------------------------------------
// Purpose : Enables the entity picker, revelaing debug information about the 
//           entity under the crosshair.
// Input   : an optional command line argument "full" enables all debug info.
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Picker( void )
{
	CBaseEntity::m_bInDebugSelect = CBaseEntity::m_bInDebugSelect ? false : true;

	// Remember the player that's making this request
	CBaseEntity::m_nDebugPlayer = UTIL_GetCommandClientIndex();
}
static ConCommand picker("picker", CC_Ent_Picker, "Toggles 'picker' mode.  When picker is on, the bounding box, pivot and debugging text is displayed for whatever entity the player is looking at.\n\tArguments:	full - enables all debug information", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Pivot( const CCommand& args )
{
	SetDebugBits(UTIL_GetCommandClient(),args[1],OVERLAY_PIVOT_BIT);
}
static ConCommand ent_pivot("ent_pivot", CC_Ent_Pivot, "Displays the pivot for the given entity(ies).\n\t(y=up=green, z=forward=blue, x=left=red). \n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Ent_Step( const CCommand& args )
{
	int nSteps = atoi(args[1]);
	if (nSteps <= 0)
	{
		nSteps = 1;
	}
	CBaseEntity::Debug_SetSteps(nSteps);
}
static ConCommand ent_step("ent_step", CC_Ent_Step, "When 'ent_pause' is set this will step through one waiting input / output message at a time.", FCVAR_CHEAT);

void CBaseEntity::SetCheckUntouch( bool check )
{
	// Invalidate touchstamp
	if ( check )
	{
		touchStamp++;
		if ( !IsEFlagSet( EFL_CHECK_UNTOUCH ) )
		{
			AddEFlags( EFL_CHECK_UNTOUCH );
			EntityTouch_Add( this );
		}
	}
	else
	{
		RemoveEFlags( EFL_CHECK_UNTOUCH );
	}
}

model_t *CBaseEntity::GetModel( void )
{
	return (model_t *)modelinfo->GetModel( GetModelIndex() );
}


//-----------------------------------------------------------------------------
// Purpose: Calculates the absolute position of an edict in the world
//			assumes the parent's absolute origin has already been calculated
//-----------------------------------------------------------------------------
void CBaseEntity::CalcAbsolutePosition( void )
{
	if (!IsEFlagSet( EFL_DIRTY_ABSTRANSFORM ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	// Plop the entity->parent matrix into m_rgflCoordinateFrame
	AngleMatrix( m_angRotation, m_vecOrigin, m_rgflCoordinateFrame );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		// no move parent, so just copy existing values
		m_vecAbsOrigin = m_vecOrigin;
		m_angAbsRotation = m_angRotation;
		if ( HasDataObjectType( POSITIONWATCHER ) )
		{
			ReportPositionChanged( this );
		}
		return;
	}

	// concatenate with our parent's transform
	matrix3x4_t tmpMatrix, scratchSpace;
	ConcatTransforms( GetParentToWorldTransform( scratchSpace ), m_rgflCoordinateFrame, tmpMatrix );
	MatrixCopy( tmpMatrix, m_rgflCoordinateFrame );

	// pull our absolute position out of the matrix
	MatrixGetColumn( m_rgflCoordinateFrame, 3, m_vecAbsOrigin ); 

	// if we have any angles, we have to extract our absolute angles from our matrix
	if (( m_angRotation == vec3_angle ) && ( m_iParentAttachment == 0 ))
	{
		// just copy our parent's absolute angles
		VectorCopy( pMoveParent->GetAbsAngles(), m_angAbsRotation );
	}
	else
	{
		MatrixAngles( m_rgflCoordinateFrame, m_angAbsRotation );
	}
	if ( HasDataObjectType( POSITIONWATCHER ) )
	{
		ReportPositionChanged( this );
	}
}

void CBaseEntity::CalcAbsoluteVelocity()
{
	if (!IsEFlagSet( EFL_DIRTY_ABSVELOCITY ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSVELOCITY );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsVelocity = m_vecVelocity;
		return;
	}

	// This transforms the local velocity into world space
	VectorRotate( m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity );

	// Now add in the parent abs velocity
	m_vecAbsVelocity += pMoveParent->GetAbsVelocity();
}

// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
/*
void CBaseEntity::CalcAbsoluteAngularVelocity()
{
	if (!IsEFlagSet( EFL_DIRTY_ABSANGVELOCITY ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSANGVELOCITY );

	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		m_vecAbsAngVelocity = m_vecAngVelocity;
		return;
	}

	// This transforms the local ang velocity into world space
	matrix3x4_t angVelToParent, angVelToWorld;
	AngleMatrix( m_vecAngVelocity, angVelToParent );
	ConcatTransforms( pMoveParent->EntityToWorldTransform(), angVelToParent, angVelToWorld );
	MatrixAngles( angVelToWorld, m_vecAbsAngVelocity );
}
*/

//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsPosition = vecLocalPosition;
	}
	else
	{
		VectorTransform( vecLocalPosition, pMoveParent->EntityToWorldTransform(), *pAbsPosition );
	}
}


//-----------------------------------------------------------------------------
// Computes the abs position of a point specified in local space
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeAbsDirection( const Vector &vecLocalDirection, Vector *pAbsDirection )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsDirection = vecLocalDirection;
	}
	else
	{
		VectorRotate( vecLocalDirection, pMoveParent->EntityToWorldTransform(), *pAbsDirection );
	}
}


matrix3x4_t& CBaseEntity::GetParentToWorldTransform( matrix3x4_t &tempMatrix )
{
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		Assert( false );
		SetIdentityMatrix( tempMatrix );
		return tempMatrix;
	}

	if ( m_iParentAttachment != 0 )
	{
		MDLCACHE_CRITICAL_SECTION();

		CBaseAnimating *pAnimating = pMoveParent->GetBaseAnimating();
		if ( pAnimating && pAnimating->GetAttachment( m_iParentAttachment, tempMatrix ) )
		{
			return tempMatrix;
		}
	}
	
	// If we fall through to here, then just use the move parent's abs origin and angles.
	return pMoveParent->EntityToWorldTransform();
}


//-----------------------------------------------------------------------------
// These methods recompute local versions as well as set abs versions
//-----------------------------------------------------------------------------
void CBaseEntity::SetAbsOrigin( const Vector& absOrigin )
{
	AssertMsg( absOrigin.IsValid(), "Invalid origin set" );
	
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	if ( m_vecAbsOrigin == absOrigin )
		return;

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( POSITION_CHANGED );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_vecAbsOrigin = absOrigin;
		
	MatrixSetColumn( absOrigin, 3, m_rgflCoordinateFrame ); 

	Vector vecNewOrigin;
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		vecNewOrigin = absOrigin;
	}
	else
	{
		matrix3x4_t tempMat;
		matrix3x4_t &parentTransform = GetParentToWorldTransform( tempMat );

		// Moveparent case: transform the abs position into local space
		VectorITransform( absOrigin, parentTransform, vecNewOrigin );
	}

	if (m_vecOrigin != vecNewOrigin)
	{
		m_vecOrigin = vecNewOrigin;
		SetSimulationTime( gpGlobals->curtime );
	}
}

void CBaseEntity::SetAbsAngles( const QAngle& absAngles )
{
	// This is necessary to get the other fields of m_rgflCoordinateFrame ok
	CalcAbsolutePosition();

	// FIXME: The normalize caused problems in server code like momentary_rot_button that isn't
	//        handling things like +/-180 degrees properly. This should be revisited.
	//QAngle angleNormalize( AngleNormalize( absAngles.x ), AngleNormalize( absAngles.y ), AngleNormalize( absAngles.z ) );

	if ( m_angAbsRotation == absAngles )
		return;

	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( ANGLES_CHANGED );
	RemoveEFlags( EFL_DIRTY_ABSTRANSFORM );

	m_angAbsRotation = absAngles;
	AngleMatrix( absAngles, m_rgflCoordinateFrame );
	MatrixSetColumn( m_vecAbsOrigin, 3, m_rgflCoordinateFrame ); 

	QAngle angNewRotation;
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		angNewRotation = absAngles;
	}
	else
	{
		if ( m_angAbsRotation == pMoveParent->GetAbsAngles() )
		{
			angNewRotation.Init( );
		}
		else
		{
			// Moveparent case: transform the abs transform into local space
			matrix3x4_t worldToParent, localMatrix;
			MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
			ConcatTransforms( worldToParent, m_rgflCoordinateFrame, localMatrix );
			MatrixAngles( localMatrix, angNewRotation );
		}
	}

	if (m_angRotation != angNewRotation)
	{
		m_angRotation = angNewRotation;
		SetSimulationTime( gpGlobals->curtime );
	}
}

void CBaseEntity::SetAbsVelocity( const Vector &vecAbsVelocity )
{
	if ( m_vecAbsVelocity == vecAbsVelocity )
		return;

	// The abs velocity won't be dirty since we're setting it here
	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( VELOCITY_CHANGED );
	RemoveEFlags( EFL_DIRTY_ABSVELOCITY );

	m_vecAbsVelocity = vecAbsVelocity;

	// NOTE: Do *not* do a network state change in this case.
	// m_vecVelocity is only networked for the player, which is not manual mode
	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecVelocity = vecAbsVelocity;
		return;
	}

	// First subtract out the parent's abs velocity to get a relative
	// velocity measured in world space
	Vector relVelocity;
	VectorSubtract( vecAbsVelocity, pMoveParent->GetAbsVelocity(), relVelocity );

	// Transform relative velocity into parent space
	Vector vNew;
	VectorIRotate( relVelocity, pMoveParent->EntityToWorldTransform(), vNew );
	m_vecVelocity = vNew;
}

// FIXME: While we're using (dPitch, dYaw, dRoll) as our local angular velocity
// representation, we can't actually solve this problem
/*
void CBaseEntity::SetAbsAngularVelocity( const QAngle &vecAbsAngVelocity )
{
	// The abs velocity won't be dirty since we're setting it here
	// All children are invalid, but we are not
	InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
	RemoveEFlags( EFL_DIRTY_ABSANGVELOCITY );

	m_vecAbsAngVelocity = vecAbsAngVelocity;

	CBaseEntity *pMoveParent = GetMoveParent();
	if (!pMoveParent)
	{
		m_vecAngVelocity = vecAbsAngVelocity;
		return;
	}

	// NOTE: We *can't* subtract out parent ang velocity, it's nonsensical
	matrix3x4_t entityToWorld;
	AngleMatrix( vecAbsAngVelocity, entityToWorld );

	// Moveparent case: transform the abs relative angular vel into local space
	matrix3x4_t worldToParent, localMatrix;
	MatrixInvert( pMoveParent->EntityToWorldTransform(), worldToParent );
	ConcatTransforms( worldToParent, entityToWorld, localMatrix );
	MatrixAngles( localMatrix, m_vecAngVelocity );
}
*/

//-----------------------------------------------------------------------------
// Methods that modify local physics state, and let us know to compute abs state later
//-----------------------------------------------------------------------------
void CBaseEntity::SetLocalOrigin( const Vector& origin )
{
	// Safety check against NaN's or really huge numbers
	if ( !IsEntityPositionReasonable( origin ) )
	{
		if ( CheckEmitReasonablePhysicsSpew() )
		{
			Warning( "Bad SetLocalOrigin(%f,%f,%f) on %s\n", origin.x, origin.y, origin.z, GetDebugName() );
		}
		Assert( false );
		return;
	}

//	if ( !origin.IsValid() )
//	{
//		AssertMsg( 0, "Bad origin set" );
//		return;
//	}

	if (m_vecOrigin != origin)
	{
		// Sanity check to make sure the origin is valid.
#ifdef _DEBUG
		float largeVal = 1024 * 128;
		Assert( origin.x >= -largeVal && origin.x <= largeVal );
		Assert( origin.y >= -largeVal && origin.y <= largeVal );
		Assert( origin.z >= -largeVal && origin.z <= largeVal );
#endif
		
		InvalidatePhysicsRecursive( POSITION_CHANGED );
		m_vecOrigin = origin;
		SetSimulationTime( gpGlobals->curtime );
	}
}

void CBaseEntity::SetLocalAngles( const QAngle& angles )
{
	// NOTE: The angle normalize is a little expensive, but we can save
	// a bunch of time in interpolation if we don't have to invalidate everything
	// and sometimes it's off by a normalization amount

	// FIXME: The normalize caused problems in server code like momentary_rot_button that isn't
	//        handling things like +/-180 degrees properly. This should be revisited.
	//QAngle angleNormalize( AngleNormalize( angles.x ), AngleNormalize( angles.y ), AngleNormalize( angles.z ) );

	// Safety check against NaN's or really huge numbers
	if ( !IsEntityQAngleReasonable( angles ) )
	{
		if ( CheckEmitReasonablePhysicsSpew() )
		{
			Warning( "Bad SetLocalAngles(%f,%f,%f) on %s\n", angles.x, angles.y, angles.z, GetDebugName() );
		}
		AssertMsg( false, "Bad SetLocalAngles(%f,%f,%f) on %s\n", angles.x, angles.y, angles.z, GetDebugName() );
		return;
	}

	if (m_angRotation != angles)
	{
		InvalidatePhysicsRecursive( ANGLES_CHANGED );
		m_angRotation = angles;
		SetSimulationTime( gpGlobals->curtime );
	}
}

void CBaseEntity::SetLocalVelocity( const Vector &inVecVelocity )
{
	Vector vecVelocity = inVecVelocity;

	// Safety check against receive a huge impulse, which can explode physics
	switch ( CheckEntityVelocity( vecVelocity ) )
	{
		case -1:
			Warning( "Discarding SetLocalVelocity(%f,%f,%f) on %s\n", vecVelocity.x, vecVelocity.y, vecVelocity.z, GetDebugName() );
			Assert( false );
			return;
		case 0:
			if ( CheckEmitReasonablePhysicsSpew() )
			{
				Warning( "Clamping SetLocalVelocity(%f,%f,%f) on %s\n", inVecVelocity.x, inVecVelocity.y, inVecVelocity.z, GetDebugName() );
			}
			break;
	}

	if (m_vecVelocity != vecVelocity)
	{
		InvalidatePhysicsRecursive( VELOCITY_CHANGED );
		m_vecVelocity = vecVelocity;
	}
}

void CBaseEntity::SetLocalAngularVelocity( const QAngle &vecAngVelocity )
{
	// Safety check against NaN's or really huge numbers
	if ( !IsEntityQAngleVelReasonable( vecAngVelocity ) )
	{
		if ( CheckEmitReasonablePhysicsSpew() )
		{
			Warning( "Bad SetLocalAngularVelocity(%f,%f,%f) on %s\n", vecAngVelocity.x, vecAngVelocity.y, vecAngVelocity.z, GetDebugName() );
		}
		Assert( false );
		return;
	}

	if (m_vecAngVelocity != vecAngVelocity)
	{
//		InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
		m_vecAngVelocity = vecAngVelocity;
	}
}


//-----------------------------------------------------------------------------
// Sets the local position from a transform
//-----------------------------------------------------------------------------
void CBaseEntity::SetLocalTransform( const matrix3x4_t &localTransform )
{
	// FIXME: Should angles go away? Should we just use transforms?
	Vector vecLocalOrigin;
	QAngle vecLocalAngles;
	MatrixGetColumn( localTransform, 3, vecLocalOrigin );
	MatrixAngles( localTransform, vecLocalAngles );
	SetLocalOrigin( vecLocalOrigin );
	SetLocalAngles( vecLocalAngles );
}


//-----------------------------------------------------------------------------
// Is the entity floating?
//-----------------------------------------------------------------------------
bool CBaseEntity::IsFloating()
{
	if ( !IsEFlagSet(EFL_TOUCHING_FLUID) )
		return false;

	IPhysicsObject *pObject = VPhysicsGetObject();
	if ( !pObject )
		return false;

	int nMaterialIndex = pObject->GetMaterialIndex();

	float flDensity;
	float flThickness;
	float flFriction;
	float flElasticity;
	physprops->GetPhysicsProperties( nMaterialIndex, &flDensity,
		&flThickness, &flFriction, &flElasticity );

	// FIXME: This really only works for water at the moment..
	// Owing the check for density == 1000
	return (flDensity < 1000.0f);
}


//-----------------------------------------------------------------------------
// Purpose: Created predictable and sets up Id.  Note that persist is ignored on the server.
// Input  : *classname - 
//			*module - 
//			line - 
//			persist - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CBaseEntity::CreatePredictedEntityByName( const char *classname, const char *module, int line, bool persist /* = false */ )
{
#if !defined( NO_ENTITY_PREDICTION )
	CBasePlayer *player = CBaseEntity::GetPredictionPlayer();
	Assert( player );

	CBaseEntity *ent = NULL;

	int command_number = player->CurrentCommandNumber();
	int player_index = player->entindex() - 1;

	CPredictableId testId;
	testId.Init( player_index, command_number, classname, module, line );

	ent = CreateEntityByName( classname );
	// No factory???
	if ( !ent )
		return NULL;

	ent->SetPredictionEligible( true );

	// Set up "shared" id number
	ent->m_PredictableID.GetForModify().SetRaw( testId.GetRaw() );

	return ent;
#else
	return NULL;
#endif

}

void CBaseEntity::SetPredictionEligible( bool canpredict )
{
// Nothing in game code	m_bPredictionEligible = canpredict;
}

//-----------------------------------------------------------------------------
// These could be virtual, but only the player is overriding them
// NOTE: If you make any of these virtual, remove this implementation!!!
//-----------------------------------------------------------------------------
void CBaseEntity::AddPoints( int score, bool bAllowNegativeScore )
{
	CBasePlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CBasePlayer::AddPoints( score, bAllowNegativeScore );
	}
}

void CBaseEntity::AddPointsToTeam( int score, bool bAllowNegativeScore )
{
	CBasePlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CBasePlayer::AddPointsToTeam( score, bAllowNegativeScore );
	}
}

void CBaseEntity::ViewPunch( const QAngle &angleOffset )
{
	CBasePlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CBasePlayer::ViewPunch( angleOffset );
	}
}

void CBaseEntity::VelocityPunch( const Vector &vecForce )
{
	CBasePlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CBasePlayer::VelocityPunch( vecForce );
	}
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Purpose: Tell clients to remove all decals from this entity
//-----------------------------------------------------------------------------
void CBaseEntity::RemoveAllDecals( void )
{
	EntityMessageBegin( this );
		WRITE_BYTE( BASEENTITY_MSG_REMOVE_DECALS );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//-----------------------------------------------------------------------------
void CBaseEntity::ModifyOrAppendCriteria( AI_CriteriaSet& set )
{
	// TODO
	// Append chapter/day?

	set.AppendCriteria( "randomnum", UTIL_VarArgs("%d", RandomInt(0,100)) );
	// Append map name
	set.AppendCriteria( "map", gpGlobals->mapname.ToCStr() );
	// Append our classname and game name
	set.AppendCriteria( "classname", GetClassname() );
	set.AppendCriteria( "name", GetEntityName().ToCStr() );

	// Append our health
	set.AppendCriteria( "health", UTIL_VarArgs( "%i", GetHealth() ) );

	float healthfrac = 0.0f;
	if ( GetMaxHealth() > 0 )
	{
		healthfrac = (float)GetHealth() / (float)GetMaxHealth();
	}

	set.AppendCriteria( "healthfrac", UTIL_VarArgs( "%.3f", healthfrac ) );

	// Go through all the global states and append them

	for ( int i = 0; i < GlobalEntity_GetNumGlobals(); i++ ) 
	{
		const char *szGlobalName = GlobalEntity_GetName(i);
		int iGlobalState = (int)GlobalEntity_GetStateByIndex(i);
		set.AppendCriteria( szGlobalName, UTIL_VarArgs( "%i", iGlobalState ) );
	}

	// Append anything from I/O or keyvalues pairs
	AppendContextToCriteria( set );

	if( hl2_episodic.GetBool() )
	{
		set.AppendCriteria( "episodic", "1" );
	}

	// Append anything from world I/O/keyvalues with "world" as prefix
	CWorld *world = dynamic_cast< CWorld * >( CBaseEntity::Instance( engine->PEntityOfEntIndex( 0 ) ) );
	if ( world )
	{
		world->AppendContextToCriteria( set, "world" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : set - 
//			"" - 
//-----------------------------------------------------------------------------
void CBaseEntity::AppendContextToCriteria( AI_CriteriaSet& set, const char *prefix /*= ""*/ )
{
	RemoveExpiredConcepts();

	int c = GetContextCount();
	int i;

	char sz[ 128 ];
	for ( i = 0; i < c; i++ )
	{
		const char *name = GetContextName( i );
		const char *value = GetContextValue( i );

		Q_snprintf( sz, sizeof( sz ), "%s%s", prefix, name );

		set.AppendCriteria( sz, value );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Removes expired concepts from list
// Output : 
//-----------------------------------------------------------------------------
void CBaseEntity::RemoveExpiredConcepts( void )
{
	int c = GetContextCount();
	int i;

	for ( i = 0; i < c; i++ )
	{
		if ( ContextExpired( i ) )
		{
			m_ResponseContexts.Remove( i );
			c--;
			i--;
			continue;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get current context count
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::GetContextCount() const
{
	return m_ResponseContexts.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CBaseEntity::GetContextName( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.Count() )
	{
		Assert( 0 );
		return "";
	}

	return  m_ResponseContexts[ index ].m_iszName.ToCStr();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : const char
//-----------------------------------------------------------------------------
const char *CBaseEntity::GetContextValue( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.Count() )
	{
		Assert( 0 );
		return "";
	}

	return  m_ResponseContexts[ index ].m_iszValue.ToCStr();
}

//-----------------------------------------------------------------------------
// Purpose: Check if context has expired
// Input  : index - 
// Output : bool
//-----------------------------------------------------------------------------
bool CBaseEntity::ContextExpired( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.Count() )
	{
		Assert( 0 );
		return true;
	}

	if ( !m_ResponseContexts[ index ].m_fExpirationTime )
	{
		return false;
	}

	return ( m_ResponseContexts[ index ].m_fExpirationTime <= gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Search for index of named context string
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CBaseEntity::FindContextByName( const char *name ) const
{
	int c = m_ResponseContexts.Count();
	for ( int i = 0; i < c; i++ )
	{
		if ( FStrEq( name, GetContextName( i ) ) )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputAddContext( inputdata_t& inputdata )
{
	const char *contextName = inputdata.value.String();
	AddContext( contextName );
}


//-----------------------------------------------------------------------------
// Purpose: User inputs. These fire the corresponding user outputs, and are
//			a means of forwarding messages through !activator to a target known
//			known by !activator but not by the targetting entity.
//
//			For example, say you have three identical trains, following the same
//			path. Each train has a sprite in hierarchy with it that needs to
//			toggle on/off as it passes each path_track. You would hook each train's
//			OnUser1 output to it's sprite's Toggle input, then connect each path_track's
//			OnPass output to !activator's FireUser1 input.
//-----------------------------------------------------------------------------
void CBaseEntity::InputFireUser1( inputdata_t& inputdata )
{
	m_OnUser1.FireOutput( inputdata.pActivator, this );
}


void CBaseEntity::InputFireUser2( inputdata_t& inputdata )
{
	m_OnUser2.FireOutput( inputdata.pActivator, this );
}


void CBaseEntity::InputFireUser3( inputdata_t& inputdata )
{
	m_OnUser3.FireOutput( inputdata.pActivator, this );
}


void CBaseEntity::InputFireUser4( inputdata_t& inputdata )
{
	m_OnUser4.FireOutput( inputdata.pActivator, this );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *contextName - 
//-----------------------------------------------------------------------------
void CBaseEntity::AddContext( const char *contextName )
{
	char key[ 128 ];
	char value[ 128 ];
	float duration;

	const char *p = contextName;
	while ( p )
	{
		duration = 0.0f;
		p = SplitContext( p, key, sizeof( key ), value, sizeof( value ), &duration );
		if ( duration )
		{
			duration += gpGlobals->curtime;
		}

		int iIndex = FindContextByName( key );
		if ( iIndex != -1 )
		{
			// Set the existing context to the new value
			m_ResponseContexts[iIndex].m_iszValue = AllocPooledString( value );
			m_ResponseContexts[iIndex].m_fExpirationTime = duration;
			continue;
		}

		ResponseContext_t newContext;
		newContext.m_iszName = AllocPooledString( key );
		newContext.m_iszValue = AllocPooledString( value );
		newContext.m_fExpirationTime = duration;

		m_ResponseContexts.AddToTail( newContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputRemoveContext( inputdata_t& inputdata )
{
	const char *contextName = inputdata.value.String();
	int idx = FindContextByName( contextName );
	if ( idx == -1 )
		return;

	m_ResponseContexts.Remove( idx );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputClearContext( inputdata_t& inputdata )
{
	m_ResponseContexts.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : IResponseSystem
//-----------------------------------------------------------------------------
IResponseSystem *CBaseEntity::GetResponseSystem()
{
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputDispatchResponse( inputdata_t& inputdata )
{
	DispatchResponse( inputdata.value.String() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseEntity::InputDisableShadow( inputdata_t &inputdata )
{
	AddEffects( EF_NOSHADOW );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBaseEntity::InputEnableShadow( inputdata_t &inputdata )
{
	RemoveEffects( EF_NOSHADOW );
}

//-----------------------------------------------------------------------------
// Purpose: An input to add a new connection from this entity
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CBaseEntity::InputAddOutput( inputdata_t &inputdata )
{
	char sOutputName[MAX_PATH];
	Q_strncpy( sOutputName, inputdata.value.String(), sizeof(sOutputName) );
	char *sChar = strchr( sOutputName, ' ' );
	if ( sChar )
	{
		*sChar = '\0';
		// Now replace all the :'s in the string with ,'s.
		// Has to be done this way because Hammer doesn't allow ,'s inside parameters.
		char *sColon = strchr( sChar+1, ':' );
		while ( sColon )
		{
			*sColon = ',';
			sColon = strchr( sChar+1, ':' );
		}
		KeyValue( sOutputName, sChar+1 );
	}
	else
	{
		Warning("AddOutput input fired with bad string. Format: <output name> <targetname>,<inputname>,<parameter>,<delay>,<max times to fire (-1 == infinite)>\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *conceptName - 
//-----------------------------------------------------------------------------
void CBaseEntity::DispatchResponse( const char *conceptName )
{
	IResponseSystem *rs = GetResponseSystem();
	if ( !rs )
		return;

	AI_CriteriaSet set;
	// Always include the concept name
	set.AppendCriteria( "concept", conceptName, CONCEPT_WEIGHT );
	// Let NPC fill in most match criteria
	ModifyOrAppendCriteria( set );

	// Append local player criteria to set,too
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if( pPlayer )
		pPlayer->ModifyOrAppendPlayerCriteria( set );

	// Now that we have a criteria set, ask for a suitable response
	AI_Response result;
	bool found = rs->FindBestResponse( set, result );
	if ( !found )
		return;

	// Handle the response here...
	const char *szResponse = result.GetResponsePtr();
	switch ( result.GetType() )
	{
	case RESPONSE_SPEAK:
		EmitSound( szResponse );
		break;

	case RESPONSE_SENTENCE:
		{
			int sentenceIndex = SENTENCEG_Lookup( szResponse );
			if( sentenceIndex == -1 )
			{
				// sentence not found
				break;
			}

			// FIXME:  Get pitch from npc?
			CPASAttenuationFilter filter( this );
			CBaseEntity::EmitSentenceByIndex( filter, entindex(), CHAN_VOICE, sentenceIndex, 1, result.GetSoundLevel(), 0, PITCH_NORM );
		}
		break;

	case RESPONSE_SCENE:
		// Try to fire scene w/o an actor
		InstancedScriptedScene( NULL, szResponse );
		break;

	case RESPONSE_PRINT:
		break;
	default:
		// Don't know how to handle .vcds!!!
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseEntity::DumpResponseCriteria( void )
{
	Msg("----------------------------------------------\n");
	Msg("RESPONSE CRITERIA FOR: %s (%s)\n", GetClassname(), GetDebugName() );

	AI_CriteriaSet set;
	// Let NPC fill in most match criteria
	ModifyOrAppendCriteria( set );

	// Append local player criteria to set,too
	CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
	if ( pPlayer )
	{
		pPlayer->ModifyOrAppendPlayerCriteria( set );
	}

	// Now dump it all to console
	set.Describe();
}

//------------------------------------------------------------------------------
void CC_Ent_Show_Response_Criteria( const CCommand& args )
{
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = GetNextCommandEntity( UTIL_GetCommandClient(), args[1], pEntity )) != NULL )
	{
		pEntity->DumpResponseCriteria();
	}
}
static ConCommand ent_show_response_criteria("ent_show_response_criteria", CC_Ent_Show_Response_Criteria, "Print, to the console, an entity's current criteria set used to select responses.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at ", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Show an entity's autoaim radius
//------------------------------------------------------------------------------
void CC_Ent_Autoaim( const CCommand& args )
{
	SetDebugBits( UTIL_GetCommandClient(),args[1], OVERLAY_AUTOAIM_BIT );
}
static ConCommand ent_autoaim("ent_autoaim", CC_Ent_Autoaim, "Displays the entity's autoaim radius.\n\tArguments:   	{entity_name} / {class_name} / no argument picks what player is looking at", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_BaseNPC	*CBaseEntity::MyNPCPointer( void ) 
{ 
	if ( IsNPC() ) 
		return assert_cast<CAI_BaseNPC *>(this);

	return NULL;
}

ConVar step_spline( "step_spline", "0" );

//-----------------------------------------------------------------------------
// Purpose: Run one tick's worth of faked simulation
// Input  : *step - 
//-----------------------------------------------------------------------------
void CBaseEntity::ComputeStepSimulationNetwork( StepSimulationData *step )
{
	if ( !step )
	{
		Assert( !"ComputeStepSimulationNetworkOriginAndAngles with NULL step\n" );
		return;
	}

	// Don't run again if we've already calculated this tick
	if ( step->m_nLastProcessTickCount == gpGlobals->tickcount )
	{
		return;
	}

	step->m_nLastProcessTickCount = gpGlobals->tickcount;

	// Origin
	// It's inactive
	if ( step->m_bOriginActive )
	{
		// First see if any external code moved the entity
		if ( GetStepOrigin() != step->m_Next.vecOrigin )
		{
			step->m_bOriginActive = false;
		}
		else
		{
			// Compute interpolated info based on tick interval
			float frac = 1.0f;
			int tickdelta = step->m_Next.nTickCount - step->m_Previous.nTickCount;
			if ( tickdelta > 0 )
			{
				frac = (float)( gpGlobals->tickcount - step->m_Previous.nTickCount ) / (float) tickdelta;
				frac = clamp( frac, 0.0f, 1.0f );
			}
		
			if (step->m_Previous2.nTickCount == 0 || step->m_Previous2.nTickCount >= step->m_Previous.nTickCount)
			{
				Vector delta = step->m_Next.vecOrigin - step->m_Previous.vecOrigin;
				VectorMA( step->m_Previous.vecOrigin, frac, delta, step->m_vecNetworkOrigin );
			}
			else if (!step_spline.GetBool())
			{
				StepSimulationStep *pOlder = &step->m_Previous;
				StepSimulationStep *pNewer = &step->m_Next;
		
				if (step->m_Discontinuity.nTickCount > step->m_Previous.nTickCount)
				{
					if (gpGlobals->tickcount > step->m_Discontinuity.nTickCount)
					{
						pOlder = &step->m_Discontinuity;
					}
					else
					{
						pNewer = &step->m_Discontinuity;
					}
		
					tickdelta = pNewer->nTickCount - pOlder->nTickCount;
					if ( tickdelta > 0 )
					{
						frac = (float)( gpGlobals->tickcount - pOlder->nTickCount ) / (float) tickdelta;
						frac = clamp( frac, 0.0f, 1.0f );
					}
				}
		
				Vector delta = pNewer->vecOrigin - pOlder->vecOrigin;
				VectorMA( pOlder->vecOrigin, frac, delta, step->m_vecNetworkOrigin );
			}
			else
			{
				Hermite_Spline( step->m_Previous2.vecOrigin, step->m_Previous.vecOrigin, step->m_Next.vecOrigin, frac, step->m_vecNetworkOrigin );
			}
		}
	}

	// Angles
	if ( step->m_bAnglesActive )
	{
		// See if external code changed the orientation of the entity
		if ( GetStepAngles() != step->m_angNextRotation )
		{
			step->m_bAnglesActive = false;
		}
		else
		{
			// Compute interpolated info based on tick interval
			float frac = 1.0f;
			int tickdelta = step->m_Next.nTickCount - step->m_Previous.nTickCount;
			if ( tickdelta > 0 )
			{
				frac = (float)( gpGlobals->tickcount - step->m_Previous.nTickCount ) / (float) tickdelta;
				frac = clamp( frac, 0.0f, 1.0f );
			}
		
			if (step->m_Previous2.nTickCount == 0 || step->m_Previous2.nTickCount >= step->m_Previous.nTickCount)
			{
				// Pure blend between start/end orientations
				Quaternion outangles;
				QuaternionBlend( step->m_Previous.qRotation, step->m_Next.qRotation, frac, outangles );
				QuaternionAngles( outangles, step->m_angNetworkAngles );
			}
			else if (!step_spline.GetBool())
			{
				StepSimulationStep *pOlder = &step->m_Previous;
				StepSimulationStep *pNewer = &step->m_Next;
		
				if (step->m_Discontinuity.nTickCount > step->m_Previous.nTickCount)
				{
					if (gpGlobals->tickcount > step->m_Discontinuity.nTickCount)
					{
						pOlder = &step->m_Discontinuity;
					}
					else
					{
						pNewer = &step->m_Discontinuity;
					}
		
					tickdelta = pNewer->nTickCount - pOlder->nTickCount;
					if ( tickdelta > 0 )
					{
						frac = (float)( gpGlobals->tickcount - pOlder->nTickCount ) / (float) tickdelta;
						frac = clamp( frac, 0.0f, 1.0f );
					}
				}
		
				// Pure blend between start/end orientations
				Quaternion outangles;
				QuaternionBlend( pOlder->qRotation, pNewer->qRotation, frac, outangles );
				QuaternionAngles( outangles, step->m_angNetworkAngles );
			}
			else
			{
				// FIXME: enable spline interpolation when turning is debounced.
				Quaternion outangles;
				Hermite_Spline( step->m_Previous2.qRotation, step->m_Previous.qRotation, step->m_Next.qRotation, frac, outangles );
				QuaternionAngles( outangles, step->m_angNetworkAngles );
			}
		}
	}

}


//-----------------------------------------------------------------------------
bool CBaseEntity::UseStepSimulationNetworkOrigin( const Vector **out_v )
{
	Assert( out_v );


	if ( g_bTestMoveTypeStepSimulation &&
		GetMoveType() == MOVETYPE_STEP &&
		HasDataObjectType( STEPSIMULATION ) )
	{
		StepSimulationData *step = ( StepSimulationData * )GetDataObject( STEPSIMULATION );
		ComputeStepSimulationNetwork( step );
		*out_v = &step->m_vecNetworkOrigin;

		return step->m_bOriginActive;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool CBaseEntity::UseStepSimulationNetworkAngles( const QAngle **out_a )
{
	Assert( out_a );

	if ( g_bTestMoveTypeStepSimulation &&
		GetMoveType() == MOVETYPE_STEP &&
		HasDataObjectType( STEPSIMULATION ) )
	{
		StepSimulationData *step = ( StepSimulationData * )GetDataObject( STEPSIMULATION );
		ComputeStepSimulationNetwork( step );
		*out_a = &step->m_angNetworkAngles;
		return step->m_bAnglesActive;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

bool CBaseEntity::AddStepDiscontinuity( float flTime, const Vector &vecOrigin, const QAngle &vecAngles )
{
	if ((GetMoveType() != MOVETYPE_STEP ) || !HasDataObjectType( STEPSIMULATION ) )
	{
		return false;
	}

	StepSimulationData *step = ( StepSimulationData * )GetDataObject( STEPSIMULATION );

	if (!step)
	{
		Assert( 0 );
		return false;
	}

	step->m_Discontinuity.nTickCount = TIME_TO_TICKS( flTime );
	step->m_Discontinuity.vecOrigin = vecOrigin;
	AngleQuaternion( vecAngles, step->m_Discontinuity.qRotation );

	return true;
}


Vector CBaseEntity::GetStepOrigin( void ) const 
{ 
	return GetLocalOrigin();
}

QAngle CBaseEntity::GetStepAngles( void ) const
{
	return GetLocalAngles();
}

//-----------------------------------------------------------------------------
// Purpose: For each client who appears to be a valid recipient, checks the client has disabled CC and if so, removes them from 
//  the recipient list.
// Input  : filter - 
//-----------------------------------------------------------------------------
void CBaseEntity::RemoveRecipientsIfNotCloseCaptioning( CRecipientFilter& filter )
{
	int c = filter.GetRecipientCount();
	for ( int i = c - 1; i >= 0; --i )
	{
		int playerIndex = filter.GetRecipientIndex( i );

		CBasePlayer *player = static_cast< CBasePlayer * >( CBaseEntity::Instance( playerIndex ) );
		if ( !player )
			continue;
#if !defined( _XBOX )
		const char *cvarvalue = engine->GetClientConVarValue( playerIndex, "closecaption" );
		Assert( cvarvalue );
		if ( !cvarvalue[ 0 ] )
			continue;

		int value = atoi( cvarvalue );
#else
		static ConVar *s_pCloseCaption = NULL;
		if ( !s_pCloseCaption )
		{
			s_pCloseCaption = cvar->FindVar( "closecaption" );
			if ( !s_pCloseCaption )
			{
				Error( "XBOX couldn't find closecaption convar!!!" );
			}
		}

		int value = s_pCloseCaption->GetInt();
#endif
		// No close captions?
		if ( value == 0 )
		{
			filter.RemoveRecipient( player );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper to emit a sentence and also a close caption token for the sentence as appropriate.
// Input  : filter - 
//			iEntIndex - 
//			iChannel - 
//			iSentenceIndex - 
//			flVolume - 
//			iSoundlevel - 
//			iFlags - 
//			iPitch - 
//			bUpdatePositions - 
//			soundtime - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
	float flVolume, soundlevel_t iSoundlevel, int iFlags /*= 0*/, int iPitch /*=PITCH_NORM*/,
	const Vector *pOrigin /*=NULL*/, const Vector *pDirection /*=NULL*/, 
	bool bUpdatePositions /*=true*/, float soundtime /*=0.0f*/ )
{
	CUtlVector< Vector > dummy;
	enginesound->EmitSentenceByIndex( filter, iEntIndex, iChannel, iSentenceIndex, 
		flVolume, iSoundlevel, iFlags, iPitch, 0, pOrigin, pDirection, &dummy, bUpdatePositions, soundtime );
}


void CBaseEntity::SetRefEHandle( const CBaseHandle &handle )
{
	m_RefEHandle = handle;
	if ( edict() )
	{
		COMPILE_TIME_ASSERT( NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS <= 8*sizeof( edict()->m_NetworkSerialNumber ) );
		edict()->m_NetworkSerialNumber = m_RefEHandle.GetSerialNumber() & ( (1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS) - 1 );
	}
}


bool CPointEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "mins" ) || FStrEq( szKeyName, "maxs" ) )
	{
		Warning("Warning! Can't specify mins/maxs for point entities! (%s)\n", GetClassname() );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}						 

bool CServerOnlyPointEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "mins" ) || FStrEq( szKeyName, "maxs" ) )
	{
		Warning("Warning! Can't specify mins/maxs for point entities! (%s)\n", GetClassname() );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

bool CLogicalEntity::KeyValue( const char *szKeyName, const char *szValue ) 
{
	if ( FStrEq( szKeyName, "mins" ) || FStrEq( szKeyName, "maxs" ) )
	{
		Warning("Warning! Can't specify mins/maxs for point entities! (%s)\n", GetClassname() );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}


//-----------------------------------------------------------------------------
// Purpose: Sets the entity invisible, and makes it remove itself on the next frame
//-----------------------------------------------------------------------------
void CBaseEntity::RemoveDeferred( void )
{
	// Set our next think to remove us
	SetThink( &CBaseEntity::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Hide us completely
	AddEffects( EF_NODRAW );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
}

#define MIN_CORPSE_FADE_TIME		10.0
#define	MIN_CORPSE_FADE_DIST		256.0
#define	MAX_CORPSE_FADE_DIST		1500.0

//
// fade out - slowly fades a entity out, then removes it.
//
// DON'T USE ME FOR GIBS AND STUFF IN MULTIPLAYER!
// SET A FUTURE THINK AND A RENDERMODE!!
void CBaseEntity::SUB_StartFadeOut( float delay, bool notSolid )
{
	SetThink( &CBaseEntity::SUB_FadeOut );
	SetNextThink( gpGlobals->curtime + delay );
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;

	if ( notSolid )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalAngularVelocity( vec3_angle );
	}
}

void CBaseEntity::SUB_StartFadeOutInstant()
{
	SUB_StartFadeOut( 0, true );
}

//-----------------------------------------------------------------------------
// Purpose: Vanish when players aren't looking
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_Vanish( void )
{
	//Always think again next frame
	SetNextThink( gpGlobals->curtime + 0.1f );

	CBasePlayer *pPlayer;

	//Get all players
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		//Get the next client
		if ( ( pPlayer = UTIL_PlayerByIndex( i ) ) != NULL )
		{
			Vector corpseDir = (GetAbsOrigin() - pPlayer->WorldSpaceCenter() );

			float flDistSqr = corpseDir.LengthSqr();
			//If the player is close enough, don't fade out
			if ( flDistSqr < (MIN_CORPSE_FADE_DIST*MIN_CORPSE_FADE_DIST) )
				return;

			// If the player's far enough away, we don't care about looking at it
			if ( flDistSqr < (MAX_CORPSE_FADE_DIST*MAX_CORPSE_FADE_DIST) )
			{
				VectorNormalize( corpseDir );

				Vector	plForward;
				pPlayer->EyeVectors( &plForward );

				float dot = plForward.Dot( corpseDir );

				if ( dot > 0.0f )
					return;
			}
		}
	}

	//If we're here, then we can vanish safely
	m_iHealth = 0;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CBaseEntity::SUB_PerformFadeOut( void )
{
	float dt = gpGlobals->frametime;
	if ( dt > 0.1f )
	{
		dt = 0.1f;
	}
	m_nRenderMode = kRenderTransTexture;
	int speed = MAX(1,256*dt); // fade out over 1 second
	SetRenderColorA( UTIL_Approach( 0, m_clrRender->a, speed ) );
}

bool CBaseEntity::SUB_AllowedToFade( void )
{
	if( VPhysicsGetObject() )
	{
		if( VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD || GetEFlags() & EFL_IS_BEING_LIFTED_BY_BARNACLE )
			return false;
	}

	// on Xbox, allow these to fade out
#ifndef _XBOX
	CBasePlayer *pPlayer = ( AI_IsSinglePlayer() ) ? UTIL_GetLocalPlayer() : NULL;

	if ( pPlayer && pPlayer->FInViewCone( this ) )
		return false;
#endif

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Fade out slowly
//-----------------------------------------------------------------------------
void CBaseEntity::SUB_FadeOut( void  )
{
	if ( SUB_AllowedToFade() == false )
	{
		SetNextThink( gpGlobals->curtime + 1 );
		SetRenderColorA( 255 );
		return;
	}
    
	SUB_PerformFadeOut();

	if ( m_clrRender->a == 0 )
	{
		UTIL_Remove(this);
	}
	else
	{
		SetNextThink( gpGlobals->curtime );
	}
}


inline bool AnyPlayersInHierarchy_R( CBaseEntity *pEnt )
{
	if ( pEnt->IsPlayer() )
		return true;

	for ( CBaseEntity *pCur = pEnt->FirstMoveChild(); pCur; pCur=pCur->NextMovePeer() )
	{
		if ( AnyPlayersInHierarchy_R( pCur ) )
			return true;
	}
	
	return false;	
}


void CBaseEntity::RecalcHasPlayerChildBit()
{
	if ( AnyPlayersInHierarchy_R( this ) )
		AddEFlags( EFL_HAS_PLAYER_CHILD );
	else
		RemoveEFlags( EFL_HAS_PLAYER_CHILD );
}


bool CBaseEntity::DoesHavePlayerChild()
{
	return IsEFlagSet( EFL_HAS_PLAYER_CHILD );
}


//------------------------------------------------------------------------------
void CBaseEntity::IncrementInterpolationFrame()
{
	m_ubInterpolationFrame = (m_ubInterpolationFrame + 1) % NOINTERP_PARITY_MAX;
}

//------------------------------------------------------------------------------

void CBaseEntity::OnModelLoadComplete( const model_t* model )
{
	Assert( m_bDynamicModelPending && IsDynamicModelIndex( m_nModelIndex ) );
	Assert( model == modelinfo->GetModel( m_nModelIndex ) );
	
	m_bDynamicModelPending = false;
	
	if ( m_bDynamicModelSetBounds )
	{
		m_bDynamicModelSetBounds = false;
		SetCollisionBoundsFromModel();
	}

	OnNewModel();
}

//------------------------------------------------------------------------------

void CBaseEntity::SetCollisionBoundsFromModel()
{
	if ( IsDynamicModelLoading() )
	{
		m_bDynamicModelSetBounds = true;
		return;
	}

	if ( const model_t *pModel = GetModel() )
	{
		Vector mns, mxs;
		modelinfo->GetModelBounds( pModel, mns, mxs );
		UTIL_SetSize( this, mns, mxs );
	}
}


//------------------------------------------------------------------------------
// Purpose: Create an NPC of the given type
//------------------------------------------------------------------------------
void CC_Ent_Create( const CCommand& args )
{
	MDLCACHE_CRITICAL_SECTION();

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if (!pPlayer)
	{
		return;
	}

	// Don't allow regular users to create point_servercommand entities for the same reason as blocking ent_fire
	if ( !Q_stricmp( args[1], "point_servercommand" ) )
	{
		if ( engine->IsDedicatedServer() )
		{
			// We allow people with disabled autokick to do it, because they already have rcon.
			if ( pPlayer->IsAutoKickDisabled() == false )
				return;
		}
		else if ( gpGlobals->maxClients > 1 )
		{
			// On listen servers with more than 1 player, only allow the host to create point_servercommand.
			CBasePlayer *pHostPlayer = UTIL_GetListenServerHost();
			if ( pPlayer != pHostPlayer )
				return;
		}
	}

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CBaseEntity *entity = dynamic_cast< CBaseEntity * >( CreateEntityByName(args[1]) );
	if (entity)
	{
		entity->Precache();

		// Pass in any additional parameters.
		for ( int i = 2; i + 1 < args.ArgC(); i += 2 )
		{
			const char *pKeyName = args[i];
			const char *pValue = args[i+1];
			entity->KeyValue( pKeyName, pValue );
		}

		DispatchSpawn(entity);

		// Now attempt to drop into the world
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_SOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0 )
		{
			// Raise the end position a little up off the floor, place the npc and drop him down
			tr.endpos.z += 12;
			entity->Teleport( &tr.endpos, NULL, NULL );
			UTIL_DropToFloor( entity, MASK_SOLID );
		}

		entity->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
static ConCommand ent_create("ent_create", CC_Ent_Create, "Creates an entity of the given type where the player is looking.  Additional parameters can be passed in in the form: ent_create <entity name> <param 1 name> <param 1> <param 2 name> <param 2>...<param N name> <param N>", FCVAR_GAMEDLL | FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Teleport a specified entity to where the player is looking
//------------------------------------------------------------------------------
bool CC_GetCommandEnt( const CCommand& args, CBaseEntity **ent, Vector *vecTargetPoint, QAngle *vecPlayerAngle )
{
	// Find the entity
	*ent = NULL;
	// First try using it as an entindex
	int iEntIndex = atoi( args[1] );
	if ( iEntIndex )
	{
		*ent = CBaseEntity::Instance( iEntIndex );
	}
	else
	{
		// Try finding it by name
		*ent = gEntList.FindEntityByName( NULL, args[1] );

		if ( !*ent )
		{
			// Finally, try finding it by classname
			*ent = gEntList.FindEntityByClassname( NULL, args[1] );
		}
	}

	if ( !*ent )
	{
		Msg( "Couldn't find any entity named '%s'\n", args[1] );
		return false;
	}

	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	if ( vecTargetPoint )
	{
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
		{
			*vecTargetPoint = tr.endpos;
		}
	}

	if ( vecPlayerAngle )
	{
		*vecPlayerAngle = pPlayer->EyeAngles();
	}

	return true;
}

//------------------------------------------------------------------------------
// Purpose: Teleport a specified entity to where the player is looking
//------------------------------------------------------------------------------
void CC_Ent_Teleport( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Format: ent_teleport <entity name>\n" );
		return;
	}

	CBaseEntity *pEnt;
	Vector vecTargetPoint;
	if ( CC_GetCommandEnt( args, &pEnt, &vecTargetPoint, NULL ) )
	{
		pEnt->Teleport( &vecTargetPoint, NULL, NULL );
	}
}

static ConCommand ent_teleport("ent_teleport", CC_Ent_Teleport, "Teleport the specified entity to where the player is looking.\n\tFormat: ent_teleport <entity name>", FCVAR_CHEAT);

//------------------------------------------------------------------------------
// Purpose: Orient a specified entity to match the player's angles
//------------------------------------------------------------------------------
void CC_Ent_Orient( const CCommand& args )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "Format: ent_orient <entity name> <optional: allangles>\n" );
		return;
	}

	CBaseEntity *pEnt;
	QAngle vecPlayerAngles;
	if ( CC_GetCommandEnt( args, &pEnt, NULL, &vecPlayerAngles ) )
	{
		QAngle vecEntAngles = pEnt->GetAbsAngles();
		if ( args.ArgC() == 3 && !Q_strncmp( args[2], "allangles", 9 ) )
		{
			vecEntAngles = vecPlayerAngles;
		}
		else
		{
			vecEntAngles[YAW] = vecPlayerAngles[YAW];
		}

		pEnt->SetAbsAngles( vecEntAngles );
	}
}

static ConCommand ent_orient("ent_orient", CC_Ent_Orient, "Orient the specified entity to match the player's angles. By default, only orients target entity's YAW. Use the 'allangles' option to orient on all axis.\n\tFormat: ent_orient <entity name> <optional: allangles>", FCVAR_CHEAT);
