//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Entity Spawner
//
//=============================================================================

#include "cbase.h"
#include "tf_entity_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// CEntitySpawnManager
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CEntitySpawnManager )

	// Fields
	DEFINE_KEYFIELD( m_iszEntityName, FIELD_STRING, "entity_name" ),
	DEFINE_KEYFIELD( m_iEntityCount, FIELD_INTEGER, "entity_count" ),
	DEFINE_KEYFIELD( m_iRespawnTime, FIELD_INTEGER, "respawn_time" ),
	DEFINE_KEYFIELD( m_bDropToGround, FIELD_BOOLEAN, "drop_to_ground" ),
	DEFINE_KEYFIELD( m_bRandomRotation, FIELD_BOOLEAN, "random_rotation" ),

	// Outputs
	// ON RESPAWN ?

END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_spawn_manager, CEntitySpawnManager );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CEntitySpawnManager::Spawn( void )
{
	BaseClass::Spawn();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Stores related spawn points.
//-----------------------------------------------------------------------------
void CEntitySpawnManager::RegisterSpawnPoint( CEntitySpawnPoint* pNewPoint )
{
	m_SpawnPoints.AddToHead( pNewPoint );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CEntitySpawnManager::Activate( void )
{
	BaseClass::Activate();

//	AssertMsg1( 0, ("entity_spawn_manager active with %i points!"), m_SpawnPoints.Count() );

	// Don't spawn more objects than we have points.
	m_iMaxSpawnedEntities = MIN( m_SpawnPoints.Count(), m_iEntityCount );

	if ( !m_iszEntityName || (m_iMaxSpawnedEntities == 0) )
	{
		AssertMsg1( 0, ("entity_spawn_manager %s active with nothing to spawn!"), GetEntityName().ToCStr() );
		return;
	}

	// Perform initial spawn.
	SpawnAllEntities();
}

//-----------------------------------------------------------------------------
// Populates a random set of spawn points with objects, up to our max.
//-----------------------------------------------------------------------------
void CEntitySpawnManager::SpawnAllEntities()
{
	int iNumUsed = 0;
	for ( int i=0; i<m_SpawnPoints.Count(); ++i )
	{
		if ( m_SpawnPoints[i]->IsUsed() )
		{
			iNumUsed++;
		}
	}

	int iNumToSpawn = m_iMaxSpawnedEntities - iNumUsed;
	if ( iNumToSpawn <= 0 )
		return;

	while ( iNumToSpawn )
	{
		SpawnEntity();
		iNumToSpawn--;
	}
}

//-----------------------------------------------------------------------------
// Spawns a single entity at a random location.
//-----------------------------------------------------------------------------
bool CEntitySpawnManager::SpawnEntity()
{
	return SpawnEntityAt( GetRandomUnusedIndex() );
}

//-----------------------------------------------------------------------------
// Returns a random unused point.
//-----------------------------------------------------------------------------
int CEntitySpawnManager::GetRandomUnusedIndex()
{
	int iStartIndex = rand() % m_SpawnPoints.Count();

	for ( int i=0; i<m_SpawnPoints.Count(); ++i )
	{
		int index = i+iStartIndex;
		if ( index >= m_SpawnPoints.Count() )
			index -= m_SpawnPoints.Count();
		CEntitySpawnPoint *pPoint = m_SpawnPoints[index];
		if ( !pPoint || pPoint->IsUsed() )
			continue;
		else
			return index;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Creates the entity.
//-----------------------------------------------------------------------------
bool CEntitySpawnManager::SpawnEntityAt( int iIndex )
{
	if ( iIndex == -1 )
		return false;

	CEntitySpawnPoint *pPoint = m_SpawnPoints[iIndex];
	if ( !pPoint )
		return false;

	CBaseEntity *pEnt = CreateEntityByName( m_iszEntityName.ToCStr() );
	if ( !pEnt )
		return false;

	Vector origin = pPoint->GetAbsOrigin();
	Vector mins, maxs, mins_o, maxs_o;
	pEnt->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
	mins_o = origin + mins;
	maxs_o = origin + mins;
	if ( !UTIL_IsSpaceEmpty( pEnt, mins_o, maxs_o ) )
	{
		UTIL_Remove( pEnt );
		return false;
	}

	if ( m_bDropToGround )
	{
		trace_t trace;
		UTIL_TraceHull( origin, origin + Vector( 0, 0, -500 ), mins, maxs, MASK_SOLID, pEnt, COLLISION_GROUP_NONE, &trace );
		origin = trace.endpos;
	}

	pEnt->SetAbsOrigin( origin );
	DispatchSpawn( pEnt );

	if ( m_bRandomRotation )
	{
		pEnt->SetAbsAngles( QAngle( 0, random->RandomFloat( 0, 360 ), 0 ) );
	}
	else
	{
		pEnt->SetAbsAngles( pPoint->GetAbsAngles() );
	}

	pPoint->SetEntity( pEnt );

	return true;
}


//-----------------------------------------------------------------------------
// CEntitySpawnPoint
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CEntitySpawnPoint )

	// Fields
	DEFINE_KEYFIELD( m_iszSpawnManagerName, FIELD_STRING, "spawn_manager_name" ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( entity_spawn_point, CEntitySpawnPoint );

//-----------------------------------------------------------------------------
// Initializes the spawn point and registers it with its manager.
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::Spawn( void )
{
	BaseClass::Spawn();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );

	if ( !m_iszSpawnManagerName )
	{
		AssertMsg( 0, ("entity_spawn_point with no spawn_manager_name!") );
		return;
	}

	m_hSpawnManager = dynamic_cast<CEntitySpawnManager*>( gEntList.FindEntityByName( NULL, m_iszSpawnManagerName ) );
	if ( !m_hSpawnManager )
	{
		AssertMsg2( 0, ("entity_spawn_point %s unable to find spawn_manager_name %s!"), GetEntityName().ToCStr(), m_iszSpawnManagerName.ToCStr() );
		return;
	}

	m_hSpawnManager->RegisterSpawnPoint( this );

	gEntList.AddListenerEntity( this );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CEntitySpawnPoint::UpdateOnRemove(void)
{
	gEntList.RemoveListenerEntity( this );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// When our entity is deleted, we become responsible for indicating when a new one should spawn.
//-----------------------------------------------------------------------------

void CEntitySpawnPoint::OnEntityDeleted( CBaseEntity *pEntity )
{
	if ( !m_hSpawnManager )
		return;

	if ( pEntity == m_hMyEntity )
	{
		m_flNodeFree = gpGlobals->curtime + 10.f;
		m_hMyEntity = NULL;
		SetNextThink( gpGlobals->curtime + m_hSpawnManager->GetRespawnTime() );
		SetThink( &CEntitySpawnPoint::RespawnNotifyThink );
	}
}

void CEntitySpawnPoint::RespawnNotifyThink( void )
{
	if ( (gpGlobals->curtime > m_flNodeFree) && m_hSpawnManager->SpawnEntity() )
	{
		SetThink( NULL );
		SetNextThink( TICK_NEVER_THINK );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 5.f );
		SetThink( &CEntitySpawnPoint::RespawnNotifyThink );
	}
}
