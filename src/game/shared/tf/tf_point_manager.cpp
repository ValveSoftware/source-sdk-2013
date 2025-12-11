//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_point_manager.h"

#ifdef CLIENT_DLL
#include "prediction.h"
#endif // CLIENT_DLL

#ifdef GAME_DLL
#include "tf_pumpkin_bomb.h"
#include "tf_generic_bomb.h"
#include "halloween/merasmus/merasmus_trick_or_treat_prop.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFPointManager, DT_TFPointManager );


BEGIN_NETWORK_TABLE( CTFPointManager, DT_TFPointManager )
#ifdef GAME_DLL
	SendPropInt( SENDINFO( m_nRandomSeed ) ),
	SendPropInt( SENDINFO( m_unNextPointIndex ), -1, SPROP_UNSIGNED | SPROP_VARINT ),
	SendPropArray3( SENDINFO_ARRAY3( m_nSpawnTime ), SendPropInt( SENDINFO_ARRAY( m_nSpawnTime ), -1, SPROP_VARINT ) ),
#else
	RecvPropInt( RECVINFO( m_nRandomSeed ) ),
	RecvPropInt( RECVINFO( m_unNextPointIndex ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_nSpawnTime ), RecvPropInt( RECVINFO( m_nSpawnTime[0] ) ) ),
#endif
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFPointManager )
END_DATADESC()

CTFPointManager::CTFPointManager()
{
	m_unNextPointIndex = 0;
	m_flLastUpdateTime = gpGlobals->curtime;
}

void CTFPointManager::Spawn( void )
{
	BaseClass::Spawn();

#ifdef GAME_DLL
	m_takedamage = DAMAGE_NO;

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY );
	SetSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	m_nRandomSeed = RandomInt( 0, 9999 );

	SetThink( &CTFPointManager::PointThink );
	SetNextThink( gpGlobals->curtime );
#endif // GAME_DLL
}


void CTFPointManager::UpdateOnRemove( void )
{
	// make sure we clean this up before parent gets deleted
	ClearPoints();

	BaseClass::UpdateOnRemove();
}

void CTFPointManager::InitializePoint( tf_point_t *pPoint, int nPointIndex )
{
#ifdef CLIENT_DLL
	Assert( !prediction->InPrediction() );
#endif// CLIENT_DLL

	int nSeed = m_nSpawnTime[ nPointIndex ] + m_nRandomSeed + nPointIndex;
	m_randomStream.SetSeed( nSeed );

	pPoint->m_vecPosition = GetInitialPosition();
	pPoint->m_vecPrevPosition = pPoint->m_vecPosition;
	pPoint->m_vecVelocity = GetInitialVelocity();
	pPoint->m_flSpawnTime = gpGlobals->curtime;
	pPoint->m_flLifeTime = GetLifeTime();
	pPoint->m_nPointIndex = nPointIndex;

}


tf_point_t* CTFPointManager::AddPointInternal( int nPointIndex )
{
	tf_point_t *pNewPoint = AllocatePoint();
	if ( pNewPoint )
	{
		InitializePoint( pNewPoint, nPointIndex );
		m_vecPoints.AddToTail( pNewPoint );

		return pNewPoint;
	}

	return NULL;
}


#ifdef GAME_DLL
void CTFPointManager::Touch( CBaseEntity *pOther )
{
	if ( !ShouldCollide( pOther ) )
		return;

	// find the first point that collide with this ent
	FOR_EACH_VEC( m_vecPoints, iPoint )
	{
		tf_point_t *pPoint = m_vecPoints[iPoint];

		float flRadius = GetRadius( pPoint );
		Vector vMins = flRadius * Vector( -1, -1, -1 );
		Vector vMaxs = flRadius * Vector( 1, 1, 1 );

		Ray_t ray;
		ray.Init( pPoint->m_vecPrevPosition, pPoint->m_vecPosition, vMins, vMaxs );

		trace_t trEnt;
		enginetrace->ClipRayToEntity( ray, MASK_SOLID | CONTENTS_HITBOX, pOther, &trEnt );
		if ( trEnt.DidHit() )
		{
			OnCollide( pOther, iPoint );

			// found the first ray that hit this entity, stop checking against other rays
			break;
		}
	}
}

int CTFPointManager::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_PVSCHECK ); 
}

unsigned int CTFPointManager::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
}

bool CTFPointManager::AddPoint( int iCurrentTick )
{
	if ( !CanAddPoint() )
		return false;

	if ( AddPointInternal( m_unNextPointIndex ) != NULL )
	{
		// network tickcount to make sure client spawns on the same frame
		m_nSpawnTime.Set( m_unNextPointIndex, iCurrentTick );

		m_unNextPointIndex = ( m_unNextPointIndex + 1 ) % MAX_POINT_MANAGER_POINTS;
		return true;
	}

	return false;
}

void CTFPointManager::PointThink()
{
	Update();

	SetNextThink( gpGlobals->curtime );
}

#else // GAME_DLL

void CTFPointManager::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged(updateType);

	// Simulate every frame.
	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}	
}

void CTFPointManager::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	int nSpawnTime = TIME_TO_TICKS( gpGlobals->curtime );
	// try to catch up with the server's last spawn point
	while ( m_unClientNextPointIndex != m_unNextPointIndex )
	{
		int nServerSpawnTime = m_nSpawnTime.Get( m_unClientNextPointIndex );

		// skip invalid
		if ( nServerSpawnTime == 0 )
		{
			m_unClientNextPointIndex = ( m_unClientNextPointIndex + 1 ) % MAX_POINT_MANAGER_POINTS;
			continue;
		}
		 
		// wait until we can spawn
		if ( nSpawnTime < nServerSpawnTime )
			break;
		
		tf_point_t *pNewestPoint = AddPointInternal( m_unClientNextPointIndex );
		if ( pNewestPoint )
		{
			if ( nSpawnTime > nServerSpawnTime )
			{
				// need to update new point to catch up to current time
				float flDT = m_flLastUpdateTime - TICKS_TO_TIME( nServerSpawnTime );
				UpdatePoint( pNewestPoint, m_vecPoints.Count() - 1, flDT );
			}
			OnClientPointAdded( pNewestPoint );
		}
		else
		{
			// we failed to create for some reason, just stop and try again next update
			Assert( !"Failed to add new point" );
		}

		m_unClientNextPointIndex = ( m_unClientNextPointIndex + 1 ) % MAX_POINT_MANAGER_POINTS;
	}
}

void CTFPointManager::ClientThink()
{
	if ( !prediction->InPrediction() )
	{
		Update();
	}
}

#endif // CLIENT_DLL

void CTFPointManager::Update()
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 )

	float flDT = gpGlobals->curtime - m_flLastUpdateTime;
	if ( flDT <= 0.f )
		return;

	m_flLastUpdateTime = gpGlobals->curtime;

#ifdef GAME_DLL
	bool bUpdatePoints = m_vecPoints.Count() > 0;
	Vector vHullMin( MAX_COORD_FLOAT, MAX_COORD_FLOAT, MAX_COORD_FLOAT );
	Vector vHullMax( MIN_COORD_FLOAT, MIN_COORD_FLOAT, MIN_COORD_FLOAT );
#endif // GAME_DLL

	// update point pos
	FOR_EACH_VEC_BACK( m_vecPoints, i )
	{
		tf_point_t *pPoint = m_vecPoints[i];

		// reset random seed
		m_randomStream.SetSeed( m_nSpawnTime[ pPoint->m_nPointIndex ] + entindex() );

		bool bShouldRemove = false;
		// expired
		if ( gpGlobals->curtime > pPoint->m_flSpawnTime + pPoint->m_flLifeTime )
		{
			bShouldRemove = true;
		}

		// in water?
		int nContents = UTIL_PointContents( pPoint->m_vecPosition );
		if ( (nContents & MASK_WATER) )
		{	
			bShouldRemove = true;
		}

		if ( bShouldRemove )
		{
			RemovePoint( i );
			continue;
		}
		
		Vector vecNewPos, vecMins, vecMaxs;
		if ( !UpdatePoint( pPoint, i, flDT, &vecNewPos, &vecMins, &vecMaxs ) )
		{
			RemovePoint( i );
			continue;
		}

#ifdef GAME_DLL
		VectorMin( vecNewPos + vecMins, vHullMin, vHullMin );
		VectorMax( vecNewPos + vecMaxs, vHullMax, vHullMax );
#endif // GAME_DLL
	}

#ifdef GAME_DLL
	if ( bUpdatePoints )
	{
		if ( m_vecPoints.Count() == 0 )
		{
			UTIL_Remove( this );
		}
		else
		{
			Vector vExtent = 0.5f * ( vHullMax - vHullMin );
			Vector vOrigin = vHullMin + vExtent;
			SetAbsOrigin( vOrigin );
			UTIL_SetSize( this, -vExtent, vExtent );
		}
	}
#endif // GAME_DLL
}

// return false if this point should be removed
bool CTFPointManager::UpdatePoint( tf_point_t *pPoint, int nIndex, float flDT, Vector *pVecNewPos /*= NULL*/, Vector *pVecMins /*= NULL*/, Vector *pVecMaxs /*= NULL*/ )
{
	if ( flDT <= 0.f )
		return true;

	float flRadius = GetRadius( pPoint );
	Vector vecMins = flRadius * Vector( -1, -1, -1 );
	Vector vecMaxs = flRadius * Vector( 1, 1, 1 );

	Vector vecGravity = Vector( 0, 0, GetGravity() ) * flDT;
	Vector vecNewVelocity = pPoint->m_vecVelocity + vecGravity + GetAdditionalVelocity( pPoint );
	Vector vecNewPos = pPoint->m_vecPosition + flDT * vecNewVelocity;

	// Create a ray for point to trace
	Ray_t rayWorld;
	rayWorld.Init( pPoint->m_vecPosition, vecNewPos, vecMins, vecMaxs );

	// check against world first for point movement
	trace_t trWorld;
	UTIL_TraceRay( rayWorld, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );

	// start in a wall, just remove this
	if ( !ShouldIgnoreStartSolid() && trWorld.startsolid )
	{
		return false;
	}
	// hit world? change direction to move along wall
	else if ( trWorld.fraction < 1.f )
	{
		// increment number of time this point has touched world
		pPoint->m_nHitWall++;

#ifdef GAME_DLL
		// Some things we collide with but don't touch.  Here we're going to make sure we collide.
		if ( trWorld.m_pEnt && ShouldCollide( trWorld.m_pEnt ) )
		{
			// Sigh...
			bool bSpecialMagicCollide = dynamic_cast<CTFPumpkinBomb*>( trWorld.m_pEnt ) ||
										dynamic_cast<CTFGenericBomb*>( trWorld.m_pEnt ) ||
										dynamic_cast<CTFMerasmusTrickOrTreatProp*>( trWorld.m_pEnt );
	
			if ( bSpecialMagicCollide )
			{
				OnCollide( trWorld.m_pEnt, nIndex );
			}
		}
#endif

		if ( OnPointHitWall( pPoint, vecNewPos, vecNewVelocity, trWorld, flDT ) )
		{
			return false;
		}
	}
	else
	{
		// vecNewVelocity only used to compute vecNewPos
		// if we hit nothing, set back to original velocity so we can apply drag
		vecNewVelocity = pPoint->m_vecVelocity;
	}

	// apply drag
	float flDrag = GetDrag();
	float flSpeed = vecNewVelocity.NormalizeInPlace();
	flSpeed = Clamp( flSpeed - flDT * flDrag * flSpeed, 0.f, flSpeed );
	pPoint->m_vecVelocity = flSpeed * vecNewVelocity + vecGravity;

	ModifyAdditionalMovementInfo( pPoint, flDT );

	pPoint->m_vecPrevPosition = pPoint->m_vecPosition;

	pPoint->m_vecPosition = vecNewPos;

	if ( pVecMins )
	{
		*pVecMins = vecMins;
	}
	if ( pVecMaxs )
	{
		*pVecMaxs = vecMaxs;
	}
	if ( pVecNewPos )
	{
		*pVecNewPos = vecNewPos;
	}

	return true;
}

bool CTFPointManager::OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT )
{
	// default behavior is to stop point on collision
	vecNewPos = tr.endpos + GetRadius( pPoint ) * tr.plane.normal;
	vecNewVelocity = vec3_origin;

	return false;
}

void CTFPointManager::RemovePoint( int nPointIndex )
{
	// free up spawn time for this slot
	m_nSpawnTime.Set( m_vecPoints[ nPointIndex ]->m_nPointIndex, 0 );

	delete m_vecPoints[ nPointIndex ];
	m_vecPoints.Remove( nPointIndex );
}

void CTFPointManager::ClearPoints( void )
{
	m_vecPoints.PurgeAndDeleteElements();
}