//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npc_antlion.h"
#include "antlion_maker.h"
#include "saverestore_utlvector.h"
#include "mapentities.h"
#include "decals.h"
#include "iservervehicle.h"
#include "antlion_dust.h"
#include "smoke_trail.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CAntlionMakerManager g_AntlionMakerManager( "CAntlionMakerManager" );

static const char *s_pPoolThinkContext = "PoolThinkContext";
static const char *s_pBlockedEffectsThinkContext = "BlockedEffectsThinkContext";
static const char *s_pBlockedCheckContext = "BlockedCheckContext";

ConVar g_debug_antlionmaker( "g_debug_antlionmaker", "0", FCVAR_CHEAT );


#define ANTLION_MAKER_PLAYER_DETECT_RADIUS	512
#define ANTLION_MAKER_BLOCKED_MASS			250.0f		// half the weight of a car
#define ANTLION_MAKE_SPORE_SPAWNRATE		25.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFightGoal( const Vector &vFightGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			pMaker->SetFightTarget( vFightGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFightGoal( CBaseEntity *pFightGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			pMaker->SetFightTarget( pFightGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFollowGoal( CBaseEntity *pFollowGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			//pMaker->SetFightTarget( NULL );
			pMaker->SetFollowTarget( pFollowGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FOLLOW );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::GatherMakers( void )
{
	CBaseEntity				*pSearch = NULL;
	CAntlionTemplateMaker	*pMaker;

	m_Makers.Purge();

	// Find these all once
	while ( ( pSearch = gEntList.FindEntityByClassname( pSearch, "npc_antlion_template_maker" ) ) != NULL )
	{
		pMaker = static_cast<CAntlionTemplateMaker *>(pSearch);

		m_Makers.AddToTail( pMaker );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::LevelInitPostEntity( void )
{
	//Find all antlion makers
	GatherMakers();
}

//-----------------------------------------------------------------------------
// Antlion template maker
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_antlion_template_maker, CAntlionTemplateMaker );

//DT Definition
BEGIN_DATADESC( CAntlionTemplateMaker )

	DEFINE_KEYFIELD( m_strSpawnGroup,	FIELD_STRING,	"spawngroup" ),
	DEFINE_KEYFIELD( m_strSpawnTarget,	FIELD_STRING,	"spawntarget" ),
	DEFINE_KEYFIELD( m_flSpawnRadius,	FIELD_FLOAT,	"spawnradius" ),
	DEFINE_KEYFIELD( m_strFightTarget,	FIELD_STRING,	"fighttarget" ),
	DEFINE_KEYFIELD( m_strFollowTarget,	FIELD_STRING,	"followtarget" ),
	DEFINE_KEYFIELD( m_bIgnoreBugbait,	FIELD_BOOLEAN,	"ignorebugbait" ),
	DEFINE_KEYFIELD( m_flVehicleSpawnDistance,	FIELD_FLOAT,	"vehicledistance" ),
	DEFINE_KEYFIELD( m_flWorkerSpawnRate,	FIELD_FLOAT,	"workerspawnrate" ),

	DEFINE_FIELD( m_nChildMoveState,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hFightTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hProxyTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hFollowTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_iSkinCount,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flBlockedBumpTime,  FIELD_TIME ),
	DEFINE_FIELD( m_bBlocked,			FIELD_BOOLEAN ),

	DEFINE_UTLVECTOR( m_Children,		FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_iPool,			FIELD_INTEGER,	"pool_start" ),
	DEFINE_KEYFIELD( m_iMaxPool,		FIELD_INTEGER,	"pool_max" ),
	DEFINE_KEYFIELD( m_iPoolRegenAmount,FIELD_INTEGER,	"pool_regen_amount" ),
	DEFINE_KEYFIELD( m_flPoolRegenTime,	FIELD_FLOAT,	"pool_regen_time" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetFightTarget",		InputSetFightTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFollowTarget",		InputSetFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearFollowTarget",	InputClearFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearFightTarget",		InputClearFightTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetSpawnRadius",		InputSetSpawnRadius ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddToPool",			InputAddToPool ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxPool",			InputSetMaxPool ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetPoolRegenAmount",	InputSetPoolRegenAmount ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	 "SetPoolRegenTime",	InputSetPoolRegenTime ),
	DEFINE_INPUTFUNC( FIELD_STRING,	 "ChangeDestinationGroup",	InputChangeDestinationGroup ),
	DEFINE_OUTPUT( m_OnAllBlocked, "OnAllBlocked" ),

	DEFINE_KEYFIELD( m_bCreateSpores,			FIELD_BOOLEAN,	"createspores" ),

	DEFINE_THINKFUNC( PoolRegenThink ),
	DEFINE_THINKFUNC( FindNodesCloseToPlayer ),
	DEFINE_THINKFUNC( BlockedCheckFunc ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAntlionTemplateMaker::CAntlionTemplateMaker( void )
{
	m_hFightTarget = NULL;
	m_hProxyTarget = NULL;
	m_hFollowTarget = NULL;
	m_nChildMoveState = ANTLION_MOVE_FREE;
	m_iSkinCount = 0;
	m_flBlockedBumpTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAntlionTemplateMaker::~CAntlionTemplateMaker( void )
{
	DestroyProxyTarget();
	m_Children.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAnt - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::AddChild( CNPC_Antlion *pAnt )
{
	m_Children.AddToTail( pAnt );
	m_nLiveChildren = m_Children.Count();

	pAnt->SetOwnerEntity( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAnt - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::RemoveChild( CNPC_Antlion *pAnt )
{
	m_Children.FindAndRemove( pAnt );
	m_nLiveChildren = m_Children.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::FixupOrphans( void )
{
	CBaseEntity		*pSearch = NULL;
	CNPC_Antlion	*pAntlion = NULL;

	// Iterate through all antlions and see if there are any orphans
	while ( ( pSearch = gEntList.FindEntityByClassname( pSearch, "npc_antlion" ) ) != NULL )
	{
		pAntlion = dynamic_cast<CNPC_Antlion *>(pSearch);

		// See if it's a live orphan
		if ( pAntlion && pAntlion->GetOwnerEntity() == NULL && pAntlion->IsAlive() )
		{
			// See if its parent was named the same as we are
			if ( stricmp( pAntlion->GetParentSpawnerName(), STRING( GetEntityName() ) ) == 0 )
			{
				// Relink us to this antlion, he's come through a transition and was orphaned
				AddChild( pAntlion );
			}
		}
	}	
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PrecacheTemplateEntity( CBaseEntity *pEntity )
{
	BaseClass::PrecacheTemplateEntity( pEntity );

	// If we can spawn workers, precache the worker as well.			
	if ( m_flWorkerSpawnRate != 0 )
	{
		pEntity->AddSpawnFlags( SF_ANTLION_WORKER );
		pEntity->Precache();
	}
}	


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::Activate( void )
{
	FixupOrphans();

	BaseClass::Activate();

	// Are we using the pool behavior for coast?
	if ( m_iMaxPool )
	{
		if ( !m_flPoolRegenTime )
		{
			Msg("%s using pool behavior without a specified pool regen time.\n", GetClassname() );
			m_flPoolRegenTime = 0.1;
		}

		// Start up our think cycle unless we're reloading this map (which would reset it)
		if ( m_bDisabled == false && gpGlobals->eLoadType != MapLoad_LoadGame )
		{
			// Start our pool regeneration cycle
			SetContextThink( &CAntlionTemplateMaker::PoolRegenThink, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );

			// Start our blocked effects cycle
			if ( hl2_episodic.GetBool() == true && HasSpawnFlags( SF_ANTLIONMAKER_DO_BLOCKEDEFFECTS ) )
			{
				SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer, gpGlobals->curtime + 1.0f, s_pBlockedEffectsThinkContext );
			}
		}
	}

	ActivateAllSpores();
}

void CAntlionTemplateMaker::ActivateSpore( const char* sporename, Vector vOrigin )
{
	if ( m_bCreateSpores == false )
		return;

	char szName[64];
	Q_snprintf( szName, sizeof( szName ), "%s_spore", sporename );

	SporeExplosion *pSpore = (SporeExplosion*)gEntList.FindEntityByName( NULL, szName );

	//One already exists...
	if ( pSpore )
	{	
		if ( pSpore->m_bDisabled == true )
		{
			inputdata_t inputdata;
			pSpore->InputEnable( inputdata );
		}

		return;
	}

	CBaseEntity *pEnt = CreateEntityByName( "env_sporeexplosion" );

	if ( pEnt )
	{
		pSpore = dynamic_cast<SporeExplosion*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->SetAbsOrigin( vOrigin );
			pSpore->SetName( AllocPooledString( szName ) );
			pSpore->m_flSpawnRate = ANTLION_MAKE_SPORE_SPAWNRATE;
		}
	}
}

void CAntlionTemplateMaker::DisableSpore( const char* sporename )
{
	if ( m_bCreateSpores == false )
		return;

	char szName[64];
	Q_snprintf( szName, sizeof( szName ), "%s_spore", sporename );

	SporeExplosion *pSpore = (SporeExplosion*)gEntList.FindEntityByName( NULL, szName );

	if ( pSpore && pSpore->m_bDisabled == false )
	{	
		inputdata_t inputdata;
		pSpore->InputDisable( inputdata );
		return;
	}
}

void CAntlionTemplateMaker::ActivateAllSpores( void )
{
	if ( m_bDisabled == true )
		return;

	if ( m_bCreateSpores == false )
		return;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	CUtlVector<CAI_Hint *> hintList;
	CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );

	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CAI_Hint *pTestHint = hintList[i];

		if ( pTestHint )
		{
			bool bBlank;
			if ( !AllHintsFromClusterBlocked( pTestHint, bBlank ) )
			{
				ActivateSpore( STRING( pTestHint->GetEntityName() ), pTestHint->GetAbsOrigin() );
			}
		}
	}
}

void CAntlionTemplateMaker::DisableAllSpores( void )
{
	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	CUtlVector<CAI_Hint *> hintList;
	CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );

	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CAI_Hint *pTestHint = hintList[i];

		if ( pTestHint )
		{
			DisableSpore( STRING( pTestHint->GetEntityName() ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CAntlionTemplateMaker::GetFightTarget( void )
{
	if ( m_hFightTarget != NULL )
		return m_hFightTarget;

	return m_hProxyTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CAntlionTemplateMaker::GetFollowTarget( void )
{
	return m_hFollowTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::UpdateChildren( void )
{
	//Update all children
	CNPC_Antlion *pAntlion = NULL;

	// Move through our child list
	int i=0;
	for ( ; i < m_Children.Count(); i++ )
	{
		pAntlion = m_Children[i];
		
		//HACKHACK
		//Let's just fix this up.
		//This guy might have been killed in another level and we just came back.
		if ( pAntlion == NULL )
		{
			m_Children.Remove( i );
			i--;
			continue;
		}
		
		if ( pAntlion->m_lifeState != LIFE_ALIVE )
			 continue;

		pAntlion->SetFightTarget( GetFightTarget() );
		pAntlion->SetFollowTarget( GetFollowTarget() );
		pAntlion->SetMoveState( m_nChildMoveState );
	}

	m_nLiveChildren = i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( string_t strTarget, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if ( HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_FIGHT_TARGET ) )
	{
		CBaseEntity *pSearch = m_hFightTarget;

		for ( int i = random->RandomInt(1,5); i > 0; i-- )
			pSearch = gEntList.FindEntityByName( pSearch, strTarget, this, pActivator, pCaller );

		if ( pSearch != NULL )
		{
			SetFightTarget( pSearch );
		}
		else
		{
			SetFightTarget( gEntList.FindEntityByName( NULL, strTarget, this, pActivator, pCaller ) );
		}
	}
	else 
	{
		SetFightTarget( gEntList.FindEntityByName( NULL, strTarget, this, pActivator, pCaller ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( CBaseEntity *pEntity )
{
	m_hFightTarget = pEntity;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &position - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( const Vector &position )
{
	CreateProxyTarget( position );
	
	m_hFightTarget = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFollowTarget( CBaseEntity *pTarget )
{
	m_hFollowTarget = pTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFollowTarget( string_t strTarget, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *pSearch = gEntList.FindEntityByName( NULL, strTarget, NULL, pActivator, pCaller );

	if ( pSearch != NULL )
	{
		SetFollowTarget( pSearch );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetChildMoveState( AntlionMoveState_e state )
{
	m_nChildMoveState = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &position - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::CreateProxyTarget( const Vector &position )
{
	// Create if we don't have one
	if ( m_hProxyTarget == NULL )
	{
		m_hProxyTarget = CreateEntityByName( "info_target" );
	}

	// Update if we do
	if ( m_hProxyTarget != NULL )
	{
		m_hProxyTarget->SetAbsOrigin( position );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::DestroyProxyTarget( void )
{
	if ( m_hProxyTarget )
	{
		UTIL_Remove( m_hProxyTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bIgnoreSolidEntities - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::CanMakeNPC( bool bIgnoreSolidEntities )
{
	if ( m_nMaxLiveChildren == 0 )
		 return false;

	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( m_strSpawnGroup == NULL_STRING )
			 return BaseClass::CanMakeNPC( bIgnoreSolidEntities );
	}

	if ( m_nMaxLiveChildren > 0 && m_nLiveChildren >= m_nMaxLiveChildren )
		return false;

	// If we're spawning from a pool, ensure the pool has an antlion in it
	if ( m_iMaxPool && !m_iPool )
		return false;

	if ( (CAI_BaseNPC::m_nDebugBits & bits_debugDisableAI) == bits_debugDisableAI )
		return false;

	return true;
}

void CAntlionTemplateMaker::Enable( void )
{
	BaseClass::Enable();

	if ( m_iMaxPool )
	{
		SetContextThink( &CAntlionTemplateMaker::PoolRegenThink, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
	}

	if ( hl2_episodic.GetBool() == true && HasSpawnFlags( SF_ANTLIONMAKER_DO_BLOCKEDEFFECTS ) )
	{
		SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer, gpGlobals->curtime + 1.0f, s_pBlockedEffectsThinkContext );
	}

	ActivateAllSpores();
}

void CAntlionTemplateMaker::Disable( void )
{
	BaseClass::Disable();

	SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	SetContextThink( NULL, gpGlobals->curtime, s_pBlockedEffectsThinkContext );

	DisableAllSpores();
}


//-----------------------------------------------------------------------------
// Randomly turn it into an antlion worker if that is enabled for this maker.
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::ChildPreSpawn( CAI_BaseNPC *pChild )
{
	BaseClass::ChildPreSpawn( pChild );

	if ( ( m_flWorkerSpawnRate > 0 ) && ( random->RandomFloat( 0, 1 ) < m_flWorkerSpawnRate ) )
	{
		pChild->AddSpawnFlags( SF_ANTLION_WORKER );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::MakeNPC( void )
{
	// If we're not restricting to hint groups, spawn as normal
	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( m_strSpawnGroup == NULL_STRING )
		{
			BaseClass::MakeNPC();
			return;
		}
	}

	if ( CanMakeNPC( true ) == false )
		return;

	// Set our defaults
	Vector	targetOrigin = GetAbsOrigin();
	QAngle	targetAngles = GetAbsAngles();

	// Look for our target entity
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_strSpawnTarget, this );

	// Take its position if it exists
	if ( pTarget != NULL )
	{
		UTIL_PredictedPosition( pTarget, 1.5f, &targetOrigin );
	}

	Vector	spawnOrigin = vec3_origin;

	CAI_Hint *pNode = NULL;

	bool bRandom = HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_SPAWN_NODE );

	if ( HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( FindNearTargetSpawnPosition( spawnOrigin, m_flSpawnRadius, pTarget ) == false )
			return;
	}
	else
	{
		// If we can't find a spawn position, then we can't spawn this time
		if ( FindHintSpawnPosition( targetOrigin, m_flSpawnRadius, m_strSpawnGroup, &pNode, bRandom ) == false )
			return;

		pNode->GetPosition( HULL_MEDIUM, &spawnOrigin );
	}
	
	// Point at the current position of the enemy
	if ( pTarget != NULL )
	{
		targetOrigin = pTarget->GetAbsOrigin();
	}	
 	
	// Create the entity via a template
	CAI_BaseNPC	*pent = NULL;
	CBaseEntity *pEntity = NULL;
	MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	
	if ( pEntity != NULL )
	{
		pent = (CAI_BaseNPC *) pEntity;
	}

	if ( pent == NULL )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		// Lock this hint node
		pNode->Lock( pEntity );
		
		// Unlock it in two seconds, this forces subsequent antlions to 
		// reject this point as a spawn point to spread them out a bit
		pNode->Unlock( 2.0f );
	}

	m_OnSpawnNPC.Set( pEntity, pEntity, this );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	ChildPreSpawn( pent );

	// Put us at the desired location
	pent->SetLocalOrigin( spawnOrigin );

	QAngle	spawnAngles;

	if ( pTarget )
	{
		// Face our spawning direction
		Vector	spawnDir = ( targetOrigin - spawnOrigin );
		VectorNormalize( spawnDir );

		VectorAngles( spawnDir, spawnAngles );
		spawnAngles[PITCH] = 0.0f;
		spawnAngles[ROLL] = 0.0f;
	}
	else if ( pNode )
	{
		spawnAngles = QAngle( 0, pNode->Yaw(), 0 );
	}

	pent->SetLocalAngles( spawnAngles );	
	DispatchSpawn( pent );
	
	pent->Activate();

	m_iSkinCount = ( m_iSkinCount + 1 ) % ANTLION_SKIN_COUNT;
	pent->m_nSkin = m_iSkinCount; 

	ChildPostSpawn( pent );

	// Hold onto the child
	CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion *>(pent);

	AddChild( pAntlion );

	m_bBlocked = false;
	SetContextThink( NULL, -1, s_pBlockedCheckContext );

	pAntlion->ClearBurrowPoint( spawnOrigin );

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		if ( m_iMaxPool )
		{
			m_iPool--;

			if ( g_debug_antlionmaker.GetInt() == 2 )
			{
				Msg("SPAWNED: Pool: %d (max %d) (Regenerating %d every %f)\n", m_iPool, m_iMaxPool, m_iPoolRegenAmount, m_flPoolRegenTime );
			}
		}
		else
		{
			m_nMaxNumNPCs--;
		}

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

bool CAntlionTemplateMaker::FindPositionOnFoot( Vector &origin, float radius, CBaseEntity *pTarget )
{
	int iMaxTries = 10;
	Vector vSpawnOrigin = pTarget->GetAbsOrigin();

	while ( iMaxTries > 0 )
	{
		vSpawnOrigin.x += random->RandomFloat( -radius, radius );
		vSpawnOrigin.y += random->RandomFloat( -radius, radius );
		vSpawnOrigin.z += 96;

		if ( ValidateSpawnPosition( vSpawnOrigin, pTarget ) == false )
		{
			iMaxTries--;
			continue;
		}

		origin = vSpawnOrigin;
		return true;
	}

	return false;
}

bool CAntlionTemplateMaker::FindPositionOnVehicle( Vector &origin, float radius, CBaseEntity *pTarget )
{
	int iMaxTries = 10;
	Vector vSpawnOrigin = pTarget->GetAbsOrigin();
	vSpawnOrigin.z += 96;

	if ( pTarget == NULL )
		 return false;

	while ( iMaxTries > 0 )
	{
		Vector vForward, vRight;
		
		pTarget->GetVectors( &vForward, &vRight, NULL );

		float flSpeed = (pTarget->GetSmoothedVelocity().Length() * m_flVehicleSpawnDistance) * random->RandomFloat( 1.0f, 1.5f );
	
		vSpawnOrigin = vSpawnOrigin + (vForward * flSpeed) + vRight * random->RandomFloat( -radius, radius );

		if ( ValidateSpawnPosition( vSpawnOrigin, pTarget ) == false )
		{
			iMaxTries--;
			continue;
		}

		origin = vSpawnOrigin;
		return true;
	}

	return false;
}

bool CAntlionTemplateMaker::ValidateSpawnPosition( Vector &vOrigin, CBaseEntity *pTarget )
{
	trace_t	tr;
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, 1024 ), MASK_BLOCKLOS | CONTENTS_WATER, NULL, COLLISION_GROUP_NONE, &tr );

	if ( g_debug_antlionmaker.GetInt() == 1 )
		 NDebugOverlay::Line( vOrigin, tr.endpos, 0, 255, 0, false, 5 );
		
	// Make sure this point is clear 
	if ( tr.fraction != 1.0 )
	{
		if ( tr.contents & ( CONTENTS_WATER ) )
			 return false;

		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );

		if ( psurf )
		{
			if ( g_debug_antlionmaker.GetInt() == 1 )
			{
				char szText[16];

				Q_snprintf( szText, 16, "Material %c", psurf->game.material );
				NDebugOverlay::Text( vOrigin, szText, true, 5 );
			}

			if ( psurf->game.material != CHAR_TEX_SAND )
				return false;
		}

		if ( CAntlionRepellant::IsPositionRepellantFree( tr.endpos ) == false )
			 return false;
	
		trace_t trCheck;
		UTIL_TraceHull( tr.endpos, tr.endpos + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), MASK_NPCSOLID, NULL, COLLISION_GROUP_NONE, &trCheck );

		if ( trCheck.DidHit() == false )
		{
			if ( g_debug_antlionmaker.GetInt() == 1 )
				 NDebugOverlay::Box( tr.endpos + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 0, 255, 0, 128, 5 );
		
			if ( pTarget )
			{
				if ( pTarget->IsPlayer() )
				{
					CBaseEntity *pVehicle = NULL;
					CBasePlayer *pPlayer = dynamic_cast < CBasePlayer *> ( pTarget );

					if ( pPlayer && pPlayer->GetVehicle() )
						 pVehicle = ((CBasePlayer *)pTarget)->GetVehicle()->GetVehicleEnt();

					CTraceFilterSkipTwoEntities traceFilter( pPlayer, pVehicle, COLLISION_GROUP_NONE );

					trace_t trVerify;
					
					Vector vVerifyOrigin = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();
					float flZOffset = NAI_Hull::Maxs( HULL_MEDIUM ).z;
					UTIL_TraceLine( vVerifyOrigin, tr.endpos + Vector( 0, 0, flZOffset ), MASK_BLOCKLOS | CONTENTS_WATER, &traceFilter, &trVerify );

					if ( trVerify.fraction != 1.0f )
					{
						const surfacedata_t *psurf = physprops->GetSurfaceData( trVerify.surface.surfaceProps );

						if ( psurf )
						{
							if ( psurf->game.material == CHAR_TEX_DIRT )
							{
								if ( g_debug_antlionmaker.GetInt() == 1 )
								{
									NDebugOverlay::Line( vVerifyOrigin, trVerify.endpos, 255, 0, 0, false, 5 );
								}

								return false;
							}
						}
					}

					if ( g_debug_antlionmaker.GetInt() == 1 )
					{
						NDebugOverlay::Line( vVerifyOrigin, trVerify.endpos, 0, 255, 0, false, 5 );
					}
				}
			}

	
			vOrigin = trCheck.endpos + Vector(0,0,5);
			return true;
		}
		else
		{
			if ( g_debug_antlionmaker.GetInt() == 1 )
				 NDebugOverlay::Box( tr.endpos + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 255, 0, 0, 128, 5 );

			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Find a position near the player to spawn the new antlion at
// Input  : &origin - search origin
//			radius - search radius
//			*retOrigin - found origin (if any)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::FindNearTargetSpawnPosition( Vector &origin, float radius, CBaseEntity *pTarget )
{
	if ( pTarget )
	{
		CBaseEntity *pVehicle = NULL;

		if ( pTarget->IsPlayer() )
		{
			CBasePlayer *pPlayer = ((CBasePlayer *)pTarget);

			if ( pPlayer->GetVehicle() )
				 pVehicle = ((CBasePlayer *)pTarget)->GetVehicle()->GetVehicleEnt();
		}

		if ( pVehicle )
		     return FindPositionOnVehicle( origin, radius, pVehicle );
		else 
			 return FindPositionOnFoot( origin, radius, pTarget );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Find a hint position to spawn the new antlion at
// Input  : &origin - search origin
//			radius - search radius
//			hintGroupName - search hint group name
//			*retOrigin - found origin (if any)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::FindHintSpawnPosition( const Vector &origin, float radius, string_t hintGroupName, CAI_Hint **pHint, bool bRandom )
{
	CAI_Hint *pChosenHint = NULL;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( hintGroupName );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	if ( bRandom )
	{
		hintCriteria.SetFlag( bits_HINT_NODE_RANDOM );
	}
	else
	{
		hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
	}
	
	// If requested, deny nodes that can be seen by the player
	if ( m_spawnflags & SF_NPCMAKER_HIDEFROMPLAYER )
	{
		hintCriteria.SetFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER );
	}

	hintCriteria.AddIncludePosition( origin, radius );

	if ( bRandom == true )
	{
		pChosenHint = CAI_HintManager::FindHintRandom( NULL, origin, hintCriteria );
	}
	else
	{
		pChosenHint = CAI_HintManager::FindHint( origin, hintCriteria );
	}

	if ( pChosenHint != NULL )
	{
		bool bChosenHintBlocked = false;

		if ( AllHintsFromClusterBlocked( pChosenHint, bChosenHintBlocked ) )
		{
			if ( ( GetIndexForThinkContext( s_pBlockedCheckContext ) == NO_THINK_CONTEXT ) ||
				( GetNextThinkTick( s_pBlockedCheckContext ) == TICK_NEVER_THINK ) )
			{
				SetContextThink( &CAntlionTemplateMaker::BlockedCheckFunc, gpGlobals->curtime + 2.0f, s_pBlockedCheckContext );
			}

			return false;
		}
		
		if ( bChosenHintBlocked == true )
		{
			return false;
		}

		*pHint = pChosenHint;
		return true;
	}

	return false;
}

void CAntlionTemplateMaker::DoBlockedEffects( CBaseEntity *pBlocker, Vector vOrigin )
{
	// If the object blocking the hole is a physics object, wobble it a bit.
	if( pBlocker )
	{
		IPhysicsObject *pPhysObj = pBlocker->VPhysicsGetObject();

		if( pPhysObj && pPhysObj->IsAsleep() )
		{
			// Don't bonk the object unless it is at rest.
			float x = RandomFloat( -5000, 5000 );
			float y = RandomFloat( -5000, 5000 );

			Vector vecForce = Vector( x, y, RandomFloat(10000, 15000) );
			pPhysObj->ApplyForceCenter( vecForce );

			UTIL_CreateAntlionDust( vOrigin, vec3_angle, true );
			pBlocker->EmitSound( "NPC_Antlion.MeleeAttackSingle_Muffled" );
			pBlocker->EmitSound( "NPC_Antlion.TrappedMetal" );


			m_flBlockedBumpTime = gpGlobals->curtime + random->RandomFloat( 1.75, 2.75 );
		}
	}
}

CBaseEntity *CAntlionTemplateMaker::AllHintsFromClusterBlocked( CAI_Hint *pNode, bool &bChosenHintBlocked )
{
	// Only do this for episodic content!
	if ( hl2_episodic.GetBool() == false )
		return NULL;

	CBaseEntity *pBlocker = NULL;

	if ( pNode != NULL )
	{
		int iNumBlocked = 0;
		int iNumNodes = 0;

		CHintCriteria	hintCriteria;

		hintCriteria.SetGroup( m_strSpawnGroup );
		hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

		CUtlVector<CAI_Hint *> hintList;
		CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );
	
		for ( int i = 0; i < hintList.Count(); i++ )
		{
			CAI_Hint *pTestHint = hintList[i];

			if ( pTestHint )
			{
				if ( pTestHint->NameMatches( pNode->GetEntityName() ) )
				{
					bool bBlocked;

					iNumNodes++;

					Vector spawnOrigin;
					pTestHint->GetPosition( HULL_MEDIUM, &spawnOrigin );

					bBlocked = false;

					CBaseEntity*	pList[20];
				
					int count = UTIL_EntitiesInBox( pList, 20, spawnOrigin + NAI_Hull::Mins( HULL_MEDIUM ), spawnOrigin + NAI_Hull::Maxs( HULL_MEDIUM ), 0 );

					//Iterate over all the possible targets
					for ( int i = 0; i < count; i++ )
					{
						if ( pList[i]->GetMoveType() != MOVETYPE_VPHYSICS )
							continue;

						if ( PhysGetEntityMass( pList[i] ) > ANTLION_MAKER_BLOCKED_MASS )
						{
							bBlocked = true;
							iNumBlocked++;
							pBlocker = pList[i];

							if ( pTestHint == pNode )
							{
								bChosenHintBlocked = true;
							}

							break;
						}
					}

					if ( g_debug_antlionmaker.GetInt() == 1 )
					{
						if ( bBlocked ) 
						{
							NDebugOverlay::Box( spawnOrigin + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 255, 0, 0, 128, 0.25f );
						}
						else
						{
							NDebugOverlay::Box( spawnOrigin + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), 0, 255, 0, 128, 0.25f );
						}
					}
				}
			}
		}

		//All the nodes from this cluster are blocked so start playing the effects.
		if ( iNumNodes > 0 && iNumBlocked == iNumNodes )
		{
			return pBlocker;
		}
	}

	return NULL;
}

void CAntlionTemplateMaker::FindNodesCloseToPlayer( void )
{
	SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer, gpGlobals->curtime + random->RandomFloat( 0.75, 1.75 ), s_pBlockedEffectsThinkContext );

	CBasePlayer *pPlayer = AI_GetSinglePlayer();

	if ( pPlayer == NULL )
		 return;

	CHintCriteria hintCriteria;

	float flRadius = ANTLION_MAKER_PLAYER_DETECT_RADIUS;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
	hintCriteria.AddIncludePosition( pPlayer->GetAbsOrigin(), ANTLION_MAKER_PLAYER_DETECT_RADIUS );

	CUtlVector<CAI_Hint *> hintList;

	if ( CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList ) <= 0 )
		return;

	CUtlVector<string_t> m_BlockedNames;

	//----
	//What we do here is find all hints of the same name (cluster name) and figure out if all of them are blocked.
	//If they are then we only need to play the blocked effects once
	//---
	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CAI_Hint *pNode = hintList[i];

		if ( pNode && pNode->HintMatchesCriteria( NULL, hintCriteria, pPlayer->GetAbsOrigin(), &flRadius ) )
		{
			bool bClusterAlreadyBlocked = false;

			//Have one of the nodes from this cluster been checked for blockage? If so then there's no need to do block checks again for this cluster.
			for ( int iStringCount = 0; iStringCount < m_BlockedNames.Count(); iStringCount++ )
			{
				if ( pNode->NameMatches( m_BlockedNames[iStringCount] ) )
				{
					bClusterAlreadyBlocked = true;
					break;
				}
			}

			if ( bClusterAlreadyBlocked == true )
				continue;

			Vector vHintPos;
			pNode->GetPosition( HULL_MEDIUM, &vHintPos );
		
			bool bBlank;
			if ( CBaseEntity *pBlocker = AllHintsFromClusterBlocked( pNode, bBlank ) )
			{
				DisableSpore( STRING( pNode->GetEntityName() ) );
				DoBlockedEffects( pBlocker, vHintPos );
				m_BlockedNames.AddToTail( pNode->GetEntityName() );
			}
			else
			{
				ActivateSpore( STRING( pNode->GetEntityName() ), pNode->GetAbsOrigin() );
			}
		}
	}
}

void CAntlionTemplateMaker::BlockedCheckFunc( void )
{
	SetContextThink( &CAntlionTemplateMaker::BlockedCheckFunc, -1, s_pBlockedCheckContext );

	if ( m_bBlocked == true )
		 return;

	CUtlVector<CAI_Hint *> hintList;
	int iBlocked = 0;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	if ( CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList ) > 0 )
	{
		for ( int i = 0; i < hintList.Count(); i++ )
		{
			CAI_Hint *pNode = hintList[i];

			if ( pNode )
			{
				Vector vHintPos;
				pNode->GetPosition( AI_GetSinglePlayer(), &vHintPos );

				CBaseEntity*	pList[20];
				int count = UTIL_EntitiesInBox( pList, 20, vHintPos + NAI_Hull::Mins( HULL_MEDIUM ), vHintPos + NAI_Hull::Maxs( HULL_MEDIUM ), 0 );

				//Iterate over all the possible targets
				for ( int i = 0; i < count; i++ )
				{
					if ( pList[i]->GetMoveType() != MOVETYPE_VPHYSICS )
						continue;

					if ( PhysGetEntityMass( pList[i] ) > ANTLION_MAKER_BLOCKED_MASS )
					{
						iBlocked++;
						break;
					}
				}
			}
		}
	}

	if ( iBlocked > 0 && hintList.Count() == iBlocked )
	{
		m_bBlocked = true;
		m_OnAllBlocked.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes the antlion immediatley unburrow if it started burrowed
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::ChildPostSpawn( CAI_BaseNPC *pChild )
{
	CNPC_Antlion *pAntlion = static_cast<CNPC_Antlion*>(pChild);

	if ( pAntlion == NULL )
		return;

	// Unburrow the spawned antlion immediately
	if ( pAntlion->m_bStartBurrowed )
	{
		pAntlion->BurrowUse( this, this, USE_ON, 0.0f );
	}

	// Set us to a follow target, if we have one
	if ( GetFollowTarget() )
	{
		pAntlion->SetFollowTarget( GetFollowTarget() );
	}
	else if ( ( m_strFollowTarget != NULL_STRING ) )
	{
		// If we don't already have a fight target, set it up
		SetFollowTarget( m_strFollowTarget );

		if ( GetFightTarget() == NULL )
		{
			SetChildMoveState( ANTLION_MOVE_FOLLOW );

			// If it's valid, fight there
			if ( GetFollowTarget() != NULL )
			{
				pAntlion->SetFollowTarget( GetFollowTarget() );
			}
		}
	}
	// See if we need to send them on their way to a fight goal
	if ( GetFightTarget() && !HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_FIGHT_TARGET ) )
	{
		pAntlion->SetFightTarget( GetFightTarget() );
	}
	else if ( m_strFightTarget != NULL_STRING )
	{
		// If we don't already have a fight target, set it up
		SetFightTarget( m_strFightTarget );	
		SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );

		// If it's valid, fight there
		if ( GetFightTarget() != NULL )
		{
			pAntlion->SetFightTarget( GetFightTarget() );
		}
	}

	// Set us to the desired movement state
	pAntlion->SetMoveState( m_nChildMoveState );

	// Save our name for level transitions
	pAntlion->SetParentSpawnerName( STRING( GetEntityName() ) );

	if ( m_hIgnoreEntity != NULL )
	{
		pChild->SetOwnerEntity( m_hIgnoreEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetFightTarget( inputdata_t &inputdata )
{
	// Set our new goal
	m_strFightTarget = MAKE_STRING( inputdata.value.String() );

	SetFightTarget( m_strFightTarget, inputdata.pActivator, inputdata.pCaller );
	SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
	
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetFollowTarget( inputdata_t &inputdata )
{
	// Set our new goal
	m_strFollowTarget = MAKE_STRING( inputdata.value.String() );

	SetFollowTarget( m_strFollowTarget, inputdata.pActivator, inputdata.pCaller );
	SetChildMoveState( ANTLION_MOVE_FOLLOW );
	
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputClearFightTarget( inputdata_t &inputdata )
{
	SetFightTarget( NULL );
	SetChildMoveState( ANTLION_MOVE_FOLLOW );

	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputClearFollowTarget( inputdata_t &inputdata )
{
	SetFollowTarget( NULL );
	SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );

	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetSpawnRadius( inputdata_t &inputdata )
{
	m_flSpawnRadius = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputAddToPool( inputdata_t &inputdata )
{
	PoolAdd( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetMaxPool( inputdata_t &inputdata )
{
	m_iMaxPool = inputdata.value.Int();
	if ( m_iPool > m_iMaxPool )
	{
		m_iPool = m_iMaxPool;
	}

	// Stop regenerating if we're supposed to stop using the pool
	if ( !m_iMaxPool )
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetPoolRegenAmount( inputdata_t &inputdata )
{
	m_iPoolRegenAmount = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetPoolRegenTime( inputdata_t &inputdata )
{
	m_flPoolRegenTime = inputdata.value.Float();

	if ( m_flPoolRegenTime != 0.0f )
	{
		SetContextThink( &CAntlionTemplateMaker::PoolRegenThink, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pool behavior for coast
// Input  : iNumToAdd - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PoolAdd( int iNumToAdd )
{
	m_iPool = clamp( m_iPool + iNumToAdd, 0, m_iMaxPool );
}

//-----------------------------------------------------------------------------
// Purpose: Regenerate the pool
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PoolRegenThink( void )
{
	if ( m_iPool < m_iMaxPool )
	{
		m_iPool = clamp( m_iPool + m_iPoolRegenAmount, 0, m_iMaxPool );

		if ( g_debug_antlionmaker.GetInt() == 2 )
		{
			Msg("REGENERATED: Pool: %d (max %d) (Regenerating %d every %f)\n", m_iPool, m_iMaxPool, m_iPoolRegenAmount, m_flPoolRegenTime );
		}
	}

	SetContextThink( &CAntlionTemplateMaker::PoolRegenThink, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::DeathNotice( CBaseEntity *pVictim )
{
	CNPC_Antlion *pAnt = dynamic_cast<CNPC_Antlion *>(pVictim);
	if ( pAnt == NULL )
		return;

	// Take it out of our list
	RemoveChild( pAnt );

	// Check if all live children are now dead
	if ( m_nLiveChildren <= 0 )
	{
		// Fire the output for this case
		m_OnAllLiveChildrenDead.FireOutput( this, this );

		bool bPoolDepleted = ( m_iMaxPool != 0 && m_iPool == 0 );
		if ( bPoolDepleted || IsDepleted() )
		{
			// Signal that all our children have been spawned and are now dead
			m_OnAllSpawnedDead.FireOutput( this, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: If this had a finite number of children, return true if they've all
//			been created.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::IsDepleted( void )
{
	// If we're running pool behavior, we're never depleted
	if ( m_iMaxPool )
		return false;

	return BaseClass::IsDepleted();
}

//-----------------------------------------------------------------------------
// Purpose: Change the spawn group the maker is using
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputChangeDestinationGroup( inputdata_t &inputdata )
{
	// FIXME: This function is redundant to the base class version, remove the m_strSpawnGroup
	m_strSpawnGroup = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
// Purpose: Draw debugging text for the spawner
//-----------------------------------------------------------------------------
int CAntlionTemplateMaker::DrawDebugTextOverlays( void )
{
	// We don't want the base class info, it's not useful to us
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[255];
		
		// Print the state of the spawner
		if ( m_bDisabled )
		{
			Q_strncpy( tempstr, "State: Disabled\n", sizeof(tempstr) );
		}
		else
		{
			Q_strncpy( tempstr, "State: Enabled\n", sizeof(tempstr) );
		}

		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print follow information
		if ( m_strFollowTarget != NULL_STRING )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Follow Target: %s\n", STRING( m_strFollowTarget ) );
		}
		else
		{
			Q_strncpy( tempstr, "Follow Target : NONE\n", sizeof(tempstr) );
		}

		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print fight information
		if ( m_strFightTarget != NULL_STRING )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Fight Target: %s\n", STRING( m_strFightTarget ) );
		}
		else
		{
			Q_strncpy( tempstr, "Fight Target : NONE\n", sizeof(tempstr) );
		}

		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print spawning criteria information
		if ( m_strSpawnTarget != NULL_STRING )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Spawn Target: %s\n", STRING( m_strSpawnTarget ) );
		}
		else
		{
			Q_strncpy( tempstr, "Spawn Target : NONE\n", sizeof(tempstr) );
		}

		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print the chilrens' state
		Q_snprintf( tempstr, sizeof(tempstr), "Spawn Frequency: %f\n", m_flSpawnFrequency );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print the spawn radius
		Q_snprintf( tempstr, sizeof(tempstr), "Spawn Radius: %.02f units\n", m_flSpawnRadius );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print the spawn group we're using
		if ( m_strSpawnGroup != NULL_STRING )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Spawn Group: %s\n", STRING( m_strSpawnGroup ) );
			EntityText( text_offset, tempstr, 0 );
			text_offset++;
		}

		// Print the chilrens' state
		Q_snprintf( tempstr, sizeof(tempstr), "Live Children: (%d/%d)\n", m_nLiveChildren, m_nMaxLiveChildren );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		// Print pool information
		if ( m_iMaxPool )
		{
			// Print the pool's state
			Q_snprintf( tempstr, sizeof(tempstr), "Pool: (%d/%d) (%d per regen)\n", m_iPool, m_iMaxPool, m_iPoolRegenAmount );
			EntityText( text_offset, tempstr, 0 );
			text_offset++;

			float flTimeRemaining = GetNextThink( s_pPoolThinkContext ) - gpGlobals->curtime;

			if ( flTimeRemaining < 0.0f )
			{
				flTimeRemaining = 0.0f;
			}

			// Print the pool's regeneration state
			Q_snprintf( tempstr, sizeof(tempstr), "Pool Regen Time: %.02f sec. (%.02f remaining)\n", m_flPoolRegenTime, flTimeRemaining );
			EntityText( text_offset, tempstr, 0 );
			text_offset++;
		}
	}

	return text_offset;	
}

//-----------------------------------------------------------------------------
// Purpose: Draw debugging overlays for the spawner
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		float r, g, b;

		// Color by active state
		if ( m_bDisabled )
		{
			r = 255.0f;
			g = 0.0f;
			b = 0.0f;
		}
		else
		{
			r = 0.0f;
			g = 255.0f;
			b = 0.0f;
		}

		// Draw ourself
		NDebugOverlay::Box( GetAbsOrigin(), -Vector(8,8,8), Vector(8,8,8), r, g, b, true, 0.05f );

		// Draw lines to our spawngroup hints
		if ( m_strSpawnGroup != NULL_STRING )
		{
			// Draw lines to our active hint groups
			AIHintIter_t iter;
			CAI_Hint *pHint = CAI_HintManager::GetFirstHint( &iter );
			while ( pHint != NULL )
			{
				// Must be of the hint group we care about
				if ( pHint->GetGroup() != m_strSpawnGroup )
				{
					pHint = CAI_HintManager::GetNextHint( &iter );
					continue;
				}

				// Draw an arrow to the spot
				NDebugOverlay::VertArrow( GetAbsOrigin(), pHint->GetAbsOrigin() + Vector( 0, 0, 32 ), 8.0f, r, g, b, 0, true, 0.05f );
				
				// Draw a box to represent where it's sitting
				Vector vecForward;
				AngleVectors( pHint->GetAbsAngles(), &vecForward );
				NDebugOverlay::BoxDirection( pHint->GetAbsOrigin(), -Vector(32,32,0), Vector(32,32,16), vecForward, r, g, b, true, 0.05f );
				
				// Move to the next
				pHint = CAI_HintManager::GetNextHint( &iter );
			}
		}

		// Draw a line to the spawn target (if it exists)
		if ( m_strSpawnTarget != NULL_STRING )
		{
			// Find all the possible targets
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_strSpawnTarget );
			if ( pTarget != NULL )
			{
				NDebugOverlay::VertArrow( GetAbsOrigin(), pTarget->WorldSpaceCenter(), 4.0f, 255, 255, 255, 0, true, 0.05f );
			}
		}

		// Draw a line to the follow target (if it exists)
		if ( m_strFollowTarget != NULL_STRING )
		{
			// Find all the possible targets
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_strFollowTarget );
			if ( pTarget != NULL )
			{
				NDebugOverlay::VertArrow( GetAbsOrigin(), pTarget->WorldSpaceCenter(), 4.0f, 255, 255, 0, 0, true, 0.05f );
			}
		}

		// Draw a line to the fight target (if it exists)
		if ( m_strFightTarget != NULL_STRING )
		{
			// Find all the possible targets
			CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, m_strFightTarget );
			if ( pTarget != NULL )
			{
				NDebugOverlay::VertArrow( GetAbsOrigin(), pTarget->WorldSpaceCenter(), 4.0f, 255, 0, 0, 0, true, 0.05f );
			}
		}
	}
}
