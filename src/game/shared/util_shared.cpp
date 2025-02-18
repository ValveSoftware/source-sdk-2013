//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "mathlib/mathlib.h"
#include "util_shared.h"
#include "model_types.h"
#include "convar.h"
#include "IEffects.h"
#include "vphysics/object_hash.h"
#include "mathlib/IceKey.H"
#include "checksum_crc.h"
#ifdef TF_CLIENT_DLL
#include "cdll_util.h"
#endif
#include "particle_parse.h"
#include "KeyValues.h"
#include "time.h"

#ifdef USES_ECON_ITEMS
	#include "econ_item_constants.h"
	#include "econ_holidays.h"
	#include "rtime.h"
#endif // USES_ECON_ITEMS

#ifdef CLIENT_DLL
	#include "c_te_effect_dispatch.h"
	#include <vgui/ILocalize.h>
	extern vgui::ILocalize *g_pVGuiLocalize;
#else
	#include "te_effect_dispatch.h"

bool NPC_CheckBrushExclude( CBaseEntity *pEntity, CBaseEntity *pBrush );
#endif

#include "steam/steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar r_visualizetraces( "r_visualizetraces", "0", FCVAR_CHEAT );
ConVar developer("developer", "0", 0, "Set developer message level" ); // developer mode

float UTIL_VecToYaw( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
		return 0;
	
	float yaw = atan2( vec.y, vec.x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const Vector &vec )
{
	if (vec.y == 0 && vec.x == 0)
	{
		if (vec.z < 0)
			return 180.0;
		else
			return -180.0;
	}

	float dist = vec.Length2D();
	float pitch = atan2( -vec.z, dist );

	pitch = RAD2DEG(pitch);

	return pitch;
}

float UTIL_VecToYaw( const matrix3x4_t &matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float y = matrix[0][1] * tmp.x + matrix[1][1] * tmp.y + matrix[2][1] * tmp.z;

	if (x == 0.0f && y == 0.0f)
		return 0.0f;
	
	float yaw = atan2( -y, x );

	yaw = RAD2DEG(yaw);

	if (yaw < 0)
		yaw += 360;

	return yaw;
}


float UTIL_VecToPitch( const matrix3x4_t &matrix, const Vector &vec )
{
	Vector tmp = vec;
	VectorNormalize( tmp );

	float x = matrix[0][0] * tmp.x + matrix[1][0] * tmp.y + matrix[2][0] * tmp.z;
	float z = matrix[0][2] * tmp.x + matrix[1][2] * tmp.y + matrix[2][2] * tmp.z;

	if (x == 0.0f && z == 0.0f)
		return 0.0f;
	
	float pitch = atan2( z, x );

	pitch = RAD2DEG(pitch);

	if (pitch < 0)
		pitch += 360;

	return pitch;
}

Vector UTIL_YawToVector( float yaw )
{
	Vector ret;
	
	ret.z = 0;
	float angle = DEG2RAD( yaw );
	SinCos( angle, &ret.y, &ret.x );

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function get get determinisitc random values for shared/prediction code
// Input  : seedvalue - 
//			*module - 
//			line - 
// Output : static int
//-----------------------------------------------------------------------------
static int SeedFileLineHash( int seedvalue, const char *sharedname, int additionalSeed )
{
	CRC32_t retval;

	CRC32_Init( &retval );

	CRC32_ProcessBuffer( &retval, (void *)&seedvalue, sizeof( int ) );
	CRC32_ProcessBuffer( &retval, (void *)&additionalSeed, sizeof( int ) );
	CRC32_ProcessBuffer( &retval, (void *)sharedname, Q_strlen( sharedname ) );
	
	CRC32_Final( &retval );

	return (int)( retval );
}

float SharedRandomFloat( const char *sharedname, float flMinVal, float flMaxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), sharedname, additionalSeed );
	RandomSeed( seed );
	return RandomFloat( flMinVal, flMaxVal );
}

int SharedRandomInt( const char *sharedname, int iMinVal, int iMaxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), sharedname, additionalSeed );
	RandomSeed( seed );
	return RandomInt( iMinVal, iMaxVal );
}

Vector SharedRandomVector( const char *sharedname, float minVal, float maxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), sharedname, additionalSeed );
	RandomSeed( seed );
	// HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
	// Get a random vector.
	Vector vRandom;
	vRandom.x = RandomFloat( minVal, maxVal );
	vRandom.y = RandomFloat( minVal, maxVal );
	vRandom.z = RandomFloat( minVal, maxVal );
	return vRandom;
}

QAngle SharedRandomAngle( const char *sharedname, float minVal, float maxVal, int additionalSeed /*=0*/ )
{
	Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

	int seed = SeedFileLineHash( CBaseEntity::GetPredictionRandomSeed(), sharedname, additionalSeed );
	RandomSeed( seed );

	// HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
	// Get a random vector.
	Vector vRandom;
	vRandom.x = RandomFloat( minVal, maxVal );
	vRandom.y = RandomFloat( minVal, maxVal );
	vRandom.z = RandomFloat( minVal, maxVal );
	return QAngle( vRandom.x, vRandom.y, vRandom.z );
}


//-----------------------------------------------------------------------------
//
// Shared client/server trace filter code
//
//-----------------------------------------------------------------------------
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass ) 
{
	if ( !pPass )
		return true;

	if ( pTouch == pPass )
		return false;

	const CBaseEntity *pEntTouch = EntityFromEntityHandle( pTouch );
	const CBaseEntity *pEntPass = EntityFromEntityHandle( pPass );
	if ( !pEntTouch || !pEntPass )
		return true;

	// don't clip against own missiles
	if ( pEntTouch->GetOwnerEntity() == pEntPass )
		return false;
	
	// don't clip against owner
	if ( pEntPass->GetOwnerEntity() == pEntTouch )
		return false;	


	return true;
}


//-----------------------------------------------------------------------------
// A standard filter to be applied to just about everything.
//-----------------------------------------------------------------------------
bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask )
{
	CBaseEntity *pCollide = EntityFromEntityHandle( pHandleEntity );

	// Static prop case...
	if ( !pCollide )
		return true;

	SolidType_t solid = pCollide->GetSolid();
	const model_t *pModel = pCollide->GetModel();

	if ( ( modelinfo->GetModelType( pModel ) != mod_brush ) || (solid != SOLID_BSP && solid != SOLID_VPHYSICS) )
	{
		if ( (fContentsMask & CONTENTS_MONSTER) == 0 )
			return false;
	}

	// This code is used to cull out tests against see-thru entities
	if ( !(fContentsMask & CONTENTS_WINDOW) && pCollide->IsTransparent() )
		return false;

	// FIXME: this is to skip BSP models that are entities that can be 
	// potentially moved/deleted, similar to a monster but doors don't seem to 
	// be flagged as monsters
	// FIXME: the FL_WORLDBRUSH looked promising, but it needs to be set on 
	// everything that's actually a worldbrush and it currently isn't
	if ( !(fContentsMask & CONTENTS_MOVEABLE) && (pCollide->GetMoveType() == MOVETYPE_PUSH))// !(touch->flags & FL_WORLDBRUSH) )
		return false;

	return true;
}


//-----------------------------------------------------------------------------
// Simple trace filter
//-----------------------------------------------------------------------------
CTraceFilterSimple::CTraceFilterSimple( const IHandleEntity *passedict, int collisionGroup,
									   ShouldHitFunc_t pExtraShouldHitFunc )
{
	m_pPassEnt = passedict;
	m_collisionGroup = collisionGroup;
	m_pExtraShouldHitCheckFunction = pExtraShouldHitFunc;
}

//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
bool CTraceFilterSimple::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
		return false;

	if ( m_pPassEnt )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
		{
			return false;
		}
	}

	// Don't test if the game code tells us we should ignore this collision...
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;
	if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
		return false;
	if ( pEntity && !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
		return false;
	if ( m_pExtraShouldHitCheckFunction &&
		(! ( m_pExtraShouldHitCheckFunction( pHandleEntity, contentsMask ) ) ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Trace filter that only hits NPCs and the player
//-----------------------------------------------------------------------------
bool CTraceFilterOnlyNPCsAndPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;

#ifdef CSTRIKE_DLL
#ifndef CLIENT_DLL
		if ( pEntity->Classify() == CLASS_PLAYER_ALLY )
			return true; // CS hostages are CLASS_PLAYER_ALLY but not IsNPC()
#endif // !CLIENT_DLL
#endif // CSTRIKE_DLL
		return (pEntity->IsNPC() || pEntity->IsPlayer());
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Trace filter that only hits anything but NPCs and the player
//-----------------------------------------------------------------------------
bool CTraceFilterNoNPCsOrPlayer::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask ) )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return NULL;
#ifndef CLIENT_DLL
		if ( pEntity->Classify() == CLASS_PLAYER_ALLY )
			return false; // CS hostages are CLASS_PLAYER_ALLY but not IsNPC()
#endif
		return (!pEntity->IsNPC() && !pEntity->IsPlayer());
	}
	return false;
}

//-----------------------------------------------------------------------------
// Trace filter that skips two entities
//-----------------------------------------------------------------------------
CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup ) :
	BaseClass( passentity, collisionGroup ), m_pPassEnt2(passentity2)
{
}

bool CTraceFilterSkipTwoEntities::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	Assert( pHandleEntity );
	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt2 ) )
		return false;

	return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Trace filter that can take a list of entities to ignore
//-----------------------------------------------------------------------------
CTraceFilterSimpleList::CTraceFilterSimpleList( int collisionGroup ) :
	CTraceFilterSimple( NULL, collisionGroup )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterSimpleList::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	if ( m_PassEntities.Find(pHandleEntity) != m_PassEntities.InvalidIndex() )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Purpose: Add an entity to my list of entities to ignore in the trace
//-----------------------------------------------------------------------------
void CTraceFilterSimpleList::AddEntityToIgnore( IHandleEntity *pEntity )
{
	m_PassEntities.AddToTail( pEntity );
}


//-----------------------------------------------------------------------------
// Purpose: Custom trace filter used for NPC LOS traces
//-----------------------------------------------------------------------------
CTraceFilterLOS::CTraceFilterLOS( IHandleEntity *pHandleEntity, int collisionGroup, IHandleEntity *pHandleEntity2 ) :
		CTraceFilterSkipTwoEntities( pHandleEntity, pHandleEntity2, collisionGroup )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterLOS::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

	if ( !pEntity->BlocksLOS() )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

//-----------------------------------------------------------------------------
// Trace filter that can take a classname to ignore
//-----------------------------------------------------------------------------
CTraceFilterSkipClassname::CTraceFilterSkipClassname( const IHandleEntity *passentity, const char *pchClassname, int collisionGroup ) :
CTraceFilterSimple( passentity, collisionGroup ), m_pchClassname( pchClassname )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterSkipClassname::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity || FClassnameIs( pEntity, m_pchClassname ) )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

//-----------------------------------------------------------------------------
// Trace filter that skips two classnames
//-----------------------------------------------------------------------------
CTraceFilterSkipTwoClassnames::CTraceFilterSkipTwoClassnames( const IHandleEntity *passentity, const char *pchClassname, const char *pchClassname2, int collisionGroup ) :
BaseClass( passentity, pchClassname, collisionGroup ), m_pchClassname2(pchClassname2)
{
}

bool CTraceFilterSkipTwoClassnames::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity || FClassnameIs( pEntity, m_pchClassname2 ) )
		return false;

	return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
}

//-----------------------------------------------------------------------------
// Trace filter that can take a list of entities to ignore
//-----------------------------------------------------------------------------
CTraceFilterSimpleClassnameList::CTraceFilterSimpleClassnameList( const IHandleEntity *passentity, int collisionGroup ) :
CTraceFilterSimple( passentity, collisionGroup )
{
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterSimpleClassnameList::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
	if ( !pEntity )
		return false;

	for ( int i = 0; i < m_PassClassnames.Count(); ++i )
	{
		if ( FClassnameIs( pEntity, m_PassClassnames[ i ] ) )
			return false;
	}

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}


//-----------------------------------------------------------------------------
// Purpose: Add an entity to my list of entities to ignore in the trace
//-----------------------------------------------------------------------------
void CTraceFilterSimpleClassnameList::AddClassnameToIgnore( const char *pchClassname )
{
	m_PassClassnames.AddToTail( pchClassname );
}

CTraceFilterChain::CTraceFilterChain( ITraceFilter *pTraceFilter1, ITraceFilter *pTraceFilter2 )
{
	m_pTraceFilter1 = pTraceFilter1;
	m_pTraceFilter2 = pTraceFilter2;
}

bool CTraceFilterChain::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	bool bResult1 = true;
	bool bResult2 = true;

	if ( m_pTraceFilter1 )
		bResult1 = m_pTraceFilter1->ShouldHitEntity( pHandleEntity, contentsMask );

	if ( m_pTraceFilter2 )
		bResult2 = m_pTraceFilter2->ShouldHitEntity( pHandleEntity, contentsMask );

	return ( bResult1 && bResult2 );
}

//-----------------------------------------------------------------------------
// Sweeps against a particular model, using collision rules 
//-----------------------------------------------------------------------------
void UTIL_TraceModel( const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
					  const Vector &hullMax, CBaseEntity *pentModel, int collisionGroup, trace_t *ptr )
{
	// Cull it....
	if ( pentModel && pentModel->ShouldCollide( collisionGroup, MASK_ALL ) )
	{
		Ray_t ray;
		ray.Init( vecStart, vecEnd, hullMin, hullMax );
		enginetrace->ClipRayToEntity( ray, MASK_ALL, pentModel, ptr ); 
	}
	else
	{
		memset( ptr, 0, sizeof(trace_t) );
		ptr->fraction = 1.0f;
	}
}

bool UTIL_EntityHasMatchingRootParent( CBaseEntity *pRootParent, CBaseEntity *pEntity )
{
	if ( pRootParent )
	{
		// NOTE: Don't let siblings/parents collide.
		if ( pRootParent == pEntity->GetRootMoveParent() )
			return true;
		if ( pEntity->GetOwnerEntity() && pRootParent == pEntity->GetOwnerEntity()->GetRootMoveParent() )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Sweep an entity from the starting to the ending position 
//-----------------------------------------------------------------------------
class CTraceFilterEntity : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterEntity, CTraceFilterSimple );

public:
	CTraceFilterEntity( CBaseEntity *pEntity, int nCollisionGroup ) 
		: CTraceFilterSimple( pEntity, nCollisionGroup )
	{
		m_pRootParent = pEntity->GetRootMoveParent();
		m_pEntity = pEntity;
		m_checkHash = g_EntityCollisionHash->IsObjectInHash(pEntity);
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( !pEntity )
			return false;

		// Check parents against each other
		// NOTE: Don't let siblings/parents collide.
		if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pEntity ) )
			return false;

		if ( m_checkHash )
		{
			if ( g_EntityCollisionHash->IsObjectPairInHash( m_pEntity, pEntity ) )
				return false;
		}

#ifndef CLIENT_DLL
		if ( m_pEntity->IsNPC() )
		{
			if ( NPC_CheckBrushExclude( m_pEntity, pEntity ) )
				 return false;

		}
#endif

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:

	CBaseEntity *m_pRootParent;
	CBaseEntity *m_pEntity;
	bool		m_checkHash;
};

class CTraceFilterEntityIgnoreOther : public CTraceFilterEntity
{
	DECLARE_CLASS( CTraceFilterEntityIgnoreOther, CTraceFilterEntity );
public:
	CTraceFilterEntityIgnoreOther( CBaseEntity *pEntity, const IHandleEntity *pIgnore, int nCollisionGroup ) : 
		CTraceFilterEntity( pEntity, nCollisionGroup ), m_pIgnoreOther( pIgnore )
	{
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( pHandleEntity == m_pIgnoreOther )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:
	const IHandleEntity *m_pIgnoreOther;
};

//-----------------------------------------------------------------------------
// Sweeps a particular entity through the world 
//-----------------------------------------------------------------------------
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
	ICollideable *pCollision = pEntity->GetCollideable();

	// Adding this assertion here so game code catches it, but really the assertion belongs in the engine
	// because one day, rotated collideables will work!
	Assert( pCollision->GetCollisionAngles() == vec3_angle );

	CTraceFilterEntity traceFilter( pEntity, pCollision->GetCollisionGroup() );

#ifdef PORTAL
	UTIL_Portal_TraceEntity( pEntity, vecAbsStart, vecAbsEnd, mask, &traceFilter, ptr );
#else
	enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
#endif
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr )
{
	ICollideable *pCollision;
	pCollision = pEntity->GetCollideable();

	// Adding this assertion here so game code catches it, but really the assertion belongs in the engine
	// because one day, rotated collideables will work!
	Assert( pCollision->GetCollisionAngles() == vec3_angle );

	CTraceFilterEntityIgnoreOther traceFilter( pEntity, pIgnore, nCollisionGroup );

#ifdef PORTAL
 	UTIL_Portal_TraceEntity( pEntity, vecAbsStart, vecAbsEnd, mask, &traceFilter, ptr );
#else
	enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
#endif
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
	ICollideable *pCollision;
	pCollision = pEntity->GetCollideable();

	// Adding this assertion here so game code catches it, but really the assertion belongs in the engine
	// because one day, rotated collideables will work!
	Assert( pCollision->GetCollisionAngles() == vec3_angle );

#ifdef PORTAL
	UTIL_Portal_TraceEntity( pEntity, vecAbsStart, vecAbsEnd, mask, pFilter, ptr );
#else
	enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, pFilter, ptr );
#endif
}

// ----
// This is basically a regular TraceLine that uses the FilterEntity filter.
void UTIL_TraceLineFilterEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					   unsigned int mask, int nCollisionGroup, trace_t *ptr )
{
	CTraceFilterEntity traceFilter( pEntity, nCollisionGroup );
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, mask, &traceFilter, ptr );
}

void UTIL_ClipTraceToPlayers( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, ITraceFilter *filter, trace_t *tr )
{
	trace_t playerTrace;
	Ray_t ray;
	float smallestFraction = tr->fraction;
	const float maxRange = 60.0f;

	ray.Init( vecAbsStart, vecAbsEnd );

	for ( int k = 1; k <= gpGlobals->maxClients; ++k )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( k );

		if ( !player || !player->IsAlive() )
			continue;

#ifdef CLIENT_DLL
		if ( player->IsDormant() )
			continue;
#endif // CLIENT_DLL

		if ( filter && filter->ShouldHitEntity( player, mask ) == false )
			continue;

		float range = DistanceToRay( player->WorldSpaceCenter(), vecAbsStart, vecAbsEnd );
		if ( range < 0.0f || range > maxRange )
			continue;

		enginetrace->ClipRayToEntity( ray, mask|CONTENTS_HITBOX, player, &playerTrace );
		if ( playerTrace.fraction < smallestFraction )
		{
			// we shortened the ray - save off the trace
			*tr = playerTrace;
			smallestFraction = playerTrace.fraction;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a tracer using a particle effect
//-----------------------------------------------------------------------------
void UTIL_ParticleTracer( const char *pszTracerEffectName, const Vector &vecStart, const Vector &vecEnd, 
				 int iEntIndex, int iAttachment, bool bWhiz )
{
	int iParticleIndex = GetParticleSystemIndex( pszTracerEffectName );
	UTIL_Tracer( vecStart, vecEnd, iEntIndex, iAttachment, 0, bWhiz, "ParticleTracer", iParticleIndex );
}

//-----------------------------------------------------------------------------
// Purpose: Make a tracer effect using the old, non-particle system, tracer effects.
//-----------------------------------------------------------------------------
void UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex, 
				 int iAttachment, float flVelocity, bool bWhiz, const char *pCustomTracerName, int iParticleID )
{
	CEffectData data;
	data.m_vStart = vecStart;
	data.m_vOrigin = vecEnd;
#ifdef CLIENT_DLL
	data.m_hEntity = ClientEntityList().EntIndexToHandle( iEntIndex );
#else
	data.m_nEntIndex = iEntIndex;
#endif
	data.m_flScale = flVelocity;
	data.m_nHitBox = iParticleID;

	// Flags
	if ( bWhiz )
	{
		data.m_fFlags |= TRACER_FLAG_WHIZ;
	}

	if ( iAttachment != TRACER_DONT_USE_ATTACHMENT )
	{
		data.m_fFlags |= TRACER_FLAG_USEATTACHMENT;
		data.m_nAttachmentIndex = iAttachment;
	}

	// Fire it off
	if ( pCustomTracerName )
	{
		DispatchEffect( pCustomTracerName, data );
	}
	else
	{
		DispatchEffect( "Tracer", data );
	}
}


void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
	if ( !UTIL_ShouldShowBlood( color ) )
		return;

	if ( color == DONT_BLEED || amount == 0 )
		return;

	if ( g_Language.GetInt() == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED )
		color = 0;

	if ( g_pGameRules->IsMultiplayer() )
	{
		// scale up blood effect in multiplayer for better visibility
		amount *= 5;
	}

	if ( amount > 255 )
		amount = 255;

	if (color == BLOOD_COLOR_MECH)
	{
		g_pEffects->Sparks(origin);
		if (random->RandomFloat(0, 2) >= 1)
		{
			UTIL_Smoke(origin, random->RandomInt(10, 15), 10);
		}
	}
	else
	{
		// Normal blood impact
		UTIL_BloodImpact( origin, direction, color, amount );
	}
}	

//-----------------------------------------------------------------------------
// Purpose: Returns low violence settings
//-----------------------------------------------------------------------------
static ConVar	violence_hblood( "violence_hblood","1", 0, "Draw human blood" );
static ConVar	violence_hgibs( "violence_hgibs","1", 0, "Show human gib entities" );
static ConVar	violence_ablood( "violence_ablood","1", 0, "Draw alien blood" );
static ConVar	violence_agibs( "violence_agibs","1", 0, "Show alien gib entities" );

bool UTIL_IsLowViolence( void )
{
	// These convars are no longer necessary -- the engine is the final arbiter of
	// violence settings -- but they're here for legacy support and for testing low
	// violence when the engine is in normal violence mode.
	if ( !violence_hblood.GetBool() || !violence_ablood.GetBool() || !violence_hgibs.GetBool() || !violence_agibs.GetBool() )
		return true;

#ifdef TF_CLIENT_DLL
	// Use low violence if the local player has an item that allows them to see it (Pyro Goggles)
	if ( IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_PYRO ) )
	{
		return true;
	}
#endif

	return engine->IsLowViolence();
}

bool UTIL_ShouldShowBlood( int color )
{
	if ( color != DONT_BLEED )
	{
		if ( color == BLOOD_COLOR_RED )
		{
			return violence_hblood.GetBool();
		}
		else
		{
			return violence_ablood.GetBool();
		}
	}
	return false;
}


//------------------------------------------------------------------------------
// Purpose : Use trace to pass a specific decal type to the entity being decaled
// Input   :
// Output  :
//------------------------------------------------------------------------------
void UTIL_DecalTrace( trace_t *pTrace, char const *decalName )
{
	if (pTrace->fraction == 1.0)
		return;

	CBaseEntity *pEntity = pTrace->m_pEnt;
	pEntity->DecalTrace( pTrace, decalName );
}


void UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor )
{
	if ( UTIL_ShouldShowBlood( bloodColor ) )
	{
		if ( bloodColor == BLOOD_COLOR_RED )
		{
			UTIL_DecalTrace( pTrace, "Blood" );
		}
		else
		{
			UTIL_DecalTrace( pTrace, "YellowBlood" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &pos - 
//			&dir - 
//			color - 
//			amount - 
//-----------------------------------------------------------------------------
void UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount )
{
	CEffectData	data;

	data.m_vOrigin = pos;
	data.m_vNormal = dir;
	data.m_flScale = (float)amount;
	data.m_nColor = (unsigned char)color;

	DispatchEffect( "bloodimpact", data );
}

bool UTIL_IsSpaceEmpty( CBaseEntity *pMainEnt, const Vector &vMin, const Vector &vMax )
{
	Vector vHalfDims = ( vMax - vMin ) * 0.5f;
	Vector vCenter = vMin + vHalfDims;

	trace_t trace;
	UTIL_TraceHull( vCenter, vCenter, -vHalfDims, vHalfDims, MASK_SOLID, pMainEnt, COLLISION_GROUP_NONE, &trace );

	bool bClear = ( trace.fraction == 1 && trace.allsolid != 1 && (trace.startsolid != 1) );
	return bClear;
}

void UTIL_StringToFloatArray( float *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atof( pfront );

		// skip any leading whitespace
		while ( *pstr && *pstr <= ' ' )
			pstr++;

		// skip to next whitespace
		while ( *pstr && *pstr > ' ' )
			pstr++;

		if (!*pstr)
			break;

		pstr++;
		pfront = pstr;
	}
	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToVector( float *pVector, const char *pString )
{
	UTIL_StringToFloatArray( pVector, 3, pString );
}

void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
	char *pstr, *pfront, tempString[128];
	int	j;

	Q_strncpy( tempString, pString, sizeof(tempString) );
	pstr = pfront = tempString;

	for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
	{
		pVector[j] = atoi( pfront );

		while ( *pstr && *pstr != ' ' )
			pstr++;
		if (!*pstr)
			break;
		pstr++;
		pfront = pstr;
	}

	for ( j++; j < count; j++ )
	{
		pVector[j] = 0;
	}
}

void UTIL_StringToColor32( color32 *color, const char *pString )
{
	int tmp[4];
	UTIL_StringToIntArray( tmp, 4, pString );
	color->r = tmp[0];
	color->g = tmp[1];
	color->b = tmp[2];
	color->a = tmp[3];
}

#ifndef _XBOX
void UTIL_DecodeICE( unsigned char * buffer, int size, const unsigned char *key)
{
	if ( !key )
		return;

	IceKey ice( 0 ); // level 0 = 64bit key
	ice.set( key ); // set key

	int blockSize = ice.blockSize();

	unsigned char *temp = (unsigned char *)_alloca( PAD_NUMBER( size, blockSize ) );
	unsigned char *p1 = buffer;
	unsigned char *p2 = temp;
				
	// encrypt data in 8 byte blocks
	int bytesLeft = size;
	while ( bytesLeft >= blockSize )
	{
		ice.decrypt( p1, p2 );
		bytesLeft -= blockSize;
		p1+=blockSize;
		p2+=blockSize;
	}

	// copy encrypted data back to original buffer
	Q_memcpy( buffer, temp, size-bytesLeft );
}
#endif

// work-around since client header doesn't like inlined gpGlobals->curtime
float IntervalTimer::Now( void ) const
{
	return gpGlobals->curtime;
}

// work-around since client header doesn't like inlined gpGlobals->curtime
float CountdownTimer::Now( void ) const
{
	return gpGlobals->curtime;
}


#ifdef CLIENT_DLL
	CBasePlayer *UTIL_PlayerByIndex( int entindex )
	{
		return ToBasePlayer( ClientEntityList().GetEnt( entindex ) );
	}
#endif


CBasePlayer *UTIL_PlayerBySteamID( const CSteamID &steamID )
{
	CSteamID steamIDPlayer;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		if ( !pPlayer->GetSteamID( &steamIDPlayer ) )
			continue;

		if ( steamIDPlayer == steamID )
			return pPlayer;
	}
	return NULL;
}

// Helper for use with console commands and the like.
// Returns NULL if not found or if the provided arg would match multiple players.
// Currently accepts, in descending priority:
//  - Formatted SteamID ([U:1:1234])
//  - SteamID64 (76561123412341234)
//  - Legacy SteamID (STEAM_0:1:1234)
//  - UserID preceded by a pound (#4)
//  - Partial name match (if unique)
//  - UserID not preceded by a pound*
//
// *Does not count as ambiguous with higher priority items
CBasePlayer* UTIL_PlayerByCommandArg( const char *arg )
{
	size_t nLength = V_strlen( arg );
	if ( nLength < 1 )
		{ return NULL; }

	// Is the argument numeric?
	bool bAllButFirstNumbers = true;
	for ( size_t idx = 1; bAllButFirstNumbers && idx < nLength; idx++ )
	{
		bAllButFirstNumbers = V_isdigit( arg[idx] );
	}
	bool bAllNumbers = V_isdigit( arg[0] ) && bAllButFirstNumbers;

	// Keep searching when we find a match to track ambiguous results
	CBasePlayer *pFound = NULL;

	// Assign pFound unless we already found a different player, in which case return NULL due to ambiguous
	// WTB Lambdas
#define UTIL_PLAYERBYCMDARG_CHECKMATCH( pEvalMatch ) \
	do                                               \
	{                                                \
		CBasePlayer *_pMacroMatch = (pEvalMatch);    \
		if ( _pMacroMatch )                          \
		{                                            \
			/* Ambiguity check */                    \
			if ( pFound && pFound != _pMacroMatch )  \
				{ return NULL; }                     \
			pFound = _pMacroMatch;                   \
		}                                            \
	} while ( false );

	// Formatted SteamID or SteamID64
	if ( bAllNumbers || ( arg[0] == '[' && arg[nLength-1] == ']' ) )
	{
		CSteamID steamID;
		bool bMatch = steamID.SetFromStringStrict( arg, GetUniverse() );
		UTIL_PLAYERBYCMDARG_CHECKMATCH( bMatch ? UTIL_PlayerBySteamID( steamID ) : NULL );
	}

	// Legacy SteamID?
	const char szPrefix[] = "STEAM_";
	if ( nLength >= V_ARRAYSIZE( szPrefix ) && V_strncmp( szPrefix, arg, V_ARRAYSIZE( szPrefix ) - 1 ) == 0 )
	{
		CSteamID steamID;
		bool bMatch = SteamIDFromSteam2String( arg, GetUniverse(), &steamID );
		UTIL_PLAYERBYCMDARG_CHECKMATCH( bMatch ? UTIL_PlayerBySteamID( steamID ) : NULL );
	}

	// UserID preceded by a pound (#4)
	if ( nLength > 1 && arg[0] == '#' && bAllButFirstNumbers )
	{
		UTIL_PLAYERBYCMDARG_CHECKMATCH( UTIL_PlayerByUserId( V_atoi( arg + 1 ) ) );
	}

	// Partial name match (if unique)
	UTIL_PLAYERBYCMDARG_CHECKMATCH( UTIL_PlayerByPartialName( arg ) );

	// UserID not preceded by a pound
	// *Does not count as ambiguous with higher priority items
	if ( bAllNumbers && !pFound )
	{
		UTIL_PLAYERBYCMDARG_CHECKMATCH( UTIL_PlayerByUserId( V_atoi( arg ) ) );
	}

	return pFound;

#undef UTIL_PLAYERBYCMDARG_CHECKMATCH
}

CBasePlayer* UTIL_PlayerByName( const char *name )
{
	if ( !name || !name[0] )
		return NULL;

	for (int i = 1; i<=gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

#ifndef CLIENT_DLL
		if ( !pPlayer->IsConnected() )
			continue;
#endif

		if ( Q_stricmp( pPlayer->GetPlayerName(), name ) == 0 )
		{
			return pPlayer;
		}
	}

	return NULL;
}

// Finds a player who has this non-ambiguous substring
CBasePlayer* UTIL_PlayerByPartialName( const char *name )
{
	if ( !name || !name[0] )
		return NULL;

	CBasePlayer *pFound = NULL;
	for (int i = 1; i<=gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

#ifndef CLIENT_DLL
		if ( !pPlayer->IsConnected() )
			continue;
#endif

		if ( Q_stristr( pPlayer->GetPlayerName(), name ) )
		{
			if ( pFound )
			{
				// Ambiguous
				return NULL;
			}
			pFound = pPlayer;
		}
	}

	return pFound;
}

CBasePlayer* UTIL_PlayerByUserId( int userID )
{
	for (int i = 1; i<=gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

#ifndef CLIENT_DLL
		if ( !pPlayer->IsConnected() )
			continue;
#endif

		if ( pPlayer->GetUserID()  == userID )
		{
			return pPlayer;
		}
	}

	return NULL;
}

#ifdef CLIENT_DLL
char *UTIL_GetFilteredPlayerName( int iPlayerIndex, char *pszName )
{
	CSteamID steamIDPlayer;
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
	if ( pPlayer )
	{
		pPlayer->GetSteamID( &steamIDPlayer );
	}
	return UTIL_GetFilteredPlayerName( steamIDPlayer, pszName );
}


char *UTIL_GetFilteredPlayerName( const CSteamID &steamID, char *pszName )
{
	if ( !pszName )
	{
		pszName = "";
	}

	if ( SteamUtils() )
	{
		SteamUtils()->FilterText( k_ETextFilteringContextName, steamID, pszName, pszName, MAX_PLAYER_NAME_LENGTH );
	}
	return pszName;
}


wchar_t *UTIL_GetFilteredPlayerNameAsWChar( int iPlayerIndex, const char *pszName, wchar_t *pwszName )
{
	CSteamID steamIDPlayer;
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
	if ( pPlayer )
	{
		pPlayer->GetSteamID( &steamIDPlayer );
	}
	return UTIL_GetFilteredPlayerNameAsWChar( steamIDPlayer, pszName, pwszName );
}


wchar_t *UTIL_GetFilteredPlayerNameAsWChar( const CSteamID &steamID, const char *pszName, wchar_t *pwszName )
{
	if ( !pszName )
	{
		pszName = "";
	}

	if ( SteamUtils() )
	{
		char szName[ MAX_PLAYER_NAME_LENGTH ];
		SteamUtils()->FilterText( k_ETextFilteringContextName, steamID, pszName, szName, sizeof( szName ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( szName, pwszName, MAX_PLAYER_NAME_LENGTH * sizeof( wchar_t ) );
	}
	else
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pszName, pwszName, MAX_PLAYER_NAME_LENGTH * sizeof( wchar_t ) );
	}
	return pwszName;
}


char *UTIL_GetFilteredChatText( int iPlayerIndex, char *pszText, int nTextBufferSize )
{
	if ( SteamUtils() )
	{
		CSteamID steamIDPlayer;
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerIndex );
		if ( pPlayer )
		{
			pPlayer->GetSteamID( &steamIDPlayer );
		}
		SteamUtils()->FilterText( k_ETextFilteringContextChat, steamIDPlayer, pszText, pszText, nTextBufferSize );
	}
	return pszText;
}
#endif // CLIENT_DLL

char* ReadAndAllocStringValue( KeyValues *pSub, const char *pName, const char *pFilename )
{
	const char *pValue = pSub->GetString( pName, NULL );
	if ( !pValue )
	{
		if ( pFilename )
		{
			DevWarning( "Can't get key value	'%s' from file '%s'.\n", pName, pFilename );
		}
		return "";
	}

	int len = Q_strlen( pValue ) + 1;
	char *pAlloced = new char[ len ];
	Assert( pAlloced );
	Q_strncpy( pAlloced, pValue, len );
	return pAlloced;
}

int UTIL_StringFieldToInt( const char *szValue, const char **pValueStrings, int iNumStrings )
{
	if ( !szValue || !szValue[0] )
		return -1;

	for ( int i = 0; i < iNumStrings; i++ )
	{
		if ( FStrEq(szValue, pValueStrings[i]) )
			return i;
	}

	Assert(0);
	return -1;
}


int find_day_of_week( struct tm& found_day, int day_of_week, int step )
{
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef USES_ECON_ITEMS
static bool					  s_HolidaysCalculated = false;
static CBitVec<kHolidayCount> s_HolidaysActive;

//-----------------------------------------------------------------------------
// Purpose: Used at level change and round start to re-calculate which holiday is active
//-----------------------------------------------------------------------------
void UTIL_CalculateHolidays()
{
	s_HolidaysActive.ClearAll();

	CRTime::UpdateRealTime();
	for ( int iHoliday = 0; iHoliday < kHolidayCount; iHoliday++ )
	{
		if ( EconHolidays_IsHolidayActive( iHoliday, CRTime::RTime32TimeCur() ) )
		{
			s_HolidaysActive.Set( iHoliday );
		}
	}

	s_HolidaysCalculated = true;
}
#endif // USES_ECON_ITEMS

bool UTIL_IsHolidayActive( /*EHoliday*/ int eHoliday )
{
#ifdef USES_ECON_ITEMS
	if ( IsX360() )
		return false;

	if ( !s_HolidaysCalculated )
	{
		UTIL_CalculateHolidays();
	}

	return s_HolidaysActive.IsBitSet( eHoliday );
#else
	return false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	UTIL_GetHolidayForString( const char* pszHolidayName )
{
#ifdef USES_ECON_ITEMS
	if ( !pszHolidayName )
		return kHoliday_None;

	return EconHolidays_GetHolidayForString( pszHolidayName );
#else
	return 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* UTIL_GetActiveHolidayString()
{
#ifdef USES_ECON_ITEMS
	return EconHolidays_GetActiveHolidayString();
#else
	return NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* UTIL_GetActiveOperationString()
{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	if ( GetItemSchema() )
	{
		FOR_EACH_DICT_FAST( GetItemSchema()->GetOperationDefinitions(), iOperation )
		{
			CEconOperationDefinition *pOperation = GetItemSchema()->GetOperationDefinitions()[iOperation];
			if ( !pOperation || !pOperation->IsActive() || !pOperation->IsCampaign() )
				continue;

			return pOperation->GetName();
		}
	}
#endif

	return NULL;
}

extern ISoundEmitterSystemBase *soundemitterbase;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *UTIL_GetRandomSoundFromEntry( const char* pszEntryName )
{
	Assert( pszEntryName );

	if ( pszEntryName )
	{
		int soundIndex = soundemitterbase->GetSoundIndex( pszEntryName );
		CSoundParametersInternal *internal = ( soundIndex != -1 ) ? soundemitterbase->InternalGetParametersForSound( soundIndex ) : NULL;
		// See if we need to pick a random one
		if ( internal )
		{
			int wave = RandomInt( 0, internal->NumSoundNames() - 1 );
			pszEntryName = soundemitterbase->GetWaveName( internal->GetSoundNames()[wave].symbol );
		}
	}

	return pszEntryName;
}

/// Clamp and round float vals to int.  The values are in the 0...255 range.
Color FloatRGBAToColor( float r, float g, float b, float a )
{
	return Color(
		(unsigned char)clamp(r + .5f, 0.0, 255.0f),
		(unsigned char)clamp(g + .5f, 0.0, 255.0f),
		(unsigned char)clamp(b + .5f, 0.0, 255.0f),
		(unsigned char)clamp(a + .5f, 0.0, 255.0f)
	);
}

float LerpFloat( float x0, float x1, float t )
{
	return x0 + (x1 - x0) * t;
}

Color LerpColor( const Color &c0, const Color &c1, float t )
{
	if ( t <= 0.0f ) return c0;
	if ( t >= 1.0f ) return c1;
	return FloatRGBAToColor(
		LerpFloat( (float)c0.r(), (float)c1.r(), t ),
		LerpFloat( (float)c0.g(), (float)c1.g(), t ),
		LerpFloat( (float)c0.b(), (float)c1.b(), t ),
		LerpFloat( (float)c0.a(), (float)c1.a(), t )
	);
}

ISteamUtils* GetSteamUtils()
{
#ifdef GAME_DLL
	// Use steamgameserver context if this isn't a client/listenserver.
	if ( engine->IsDedicatedServer() )
	{
		return steamgameserverapicontext ? steamgameserverapicontext->SteamGameServerUtils() : NULL;
	}
#endif
	return steamapicontext ? steamapicontext->SteamUtils() : NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
EUniverse GetUniverse()
{
	if ( !GetSteamUtils() )
		return k_EUniverseInvalid;

	static EUniverse steamUniverse = GetSteamUtils()->GetConnectedUniverse();
	return steamUniverse;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CSteamID SteamIDFromDecimalString( const char *pszUint64InDecimal )
{
	uint64 ulSteamID = 0;
	if ( sscanf( pszUint64InDecimal, "%llu", &ulSteamID ) )
	{
		return CSteamID( ulSteamID );
	}
	else
	{
		Assert( false );
		return CSteamID();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to parse an un-ambiguous steamID from a string
//
//  Accepts
//  - Formatted SteamID ([U:1:1234])
//  - SteamID64 (76561123412341234)
//  - Legacy SteamID (STEAM_0:1:1234) (if bAllowSteam2)
//-----------------------------------------------------------------------------
CSteamID UTIL_SteamIDFromProperString( const char *pszInput, bool bAllowSteam2 /* = true */ )
{
	// Formatted SteamID or SteamID64
	{
		CSteamID steamID;
		bool bMatch = steamID.SetFromStringStrict( pszInput, GetUniverse() );
		if ( bMatch && steamID.IsValid() )
			{ return steamID; }
	}

	// Legacy SteamID?
	const char szPrefix[] = "STEAM_";
	if ( bAllowSteam2 && V_strlen( pszInput ) >= (int)V_ARRAYSIZE( szPrefix ) &&
	     V_strncmp( szPrefix, pszInput, V_ARRAYSIZE( szPrefix ) - 1 ) == 0 )
	{
		CSteamID steamID;
		bool bMatch = SteamIDFromSteam2String( pszInput, GetUniverse(), &steamID );
		if ( bMatch && steamID.IsValid() )
			{ return steamID; }
	}

	return CSteamID();
}

//-----------------------------------------------------------------------------
// Purpose: Try to parse a string referring to a steam account to a CSteamID.
//
//   This is intended for fuzzy user input -- NOT guaranteed to find a unique
//   or un-ambiugous result
//-----------------------------------------------------------------------------
CSteamID UTIL_GuessSteamIDFromFuzzyInput( const char *pszInputRaw, bool bCurrentUniverse /* = true */ )
{
	if( !pszInputRaw )
	{
		return CSteamID();
	}

	EUniverse localUniverse = GetUniverse();

	CUtlString strInput( pszInputRaw );
	strInput.Trim();

	// Is this a proper string once trimmed?
	CSteamID steamID = UTIL_SteamIDFromProperString( strInput, true );
	if ( steamID.IsValid() && ( !bCurrentUniverse || steamID.GetEUniverse() == localUniverse ) )
		{ return steamID; }

	// Check for all digits representing a 32bit number
	//
	// SteamIDFromProperString would've checked for a 64bit staemID, but if it is 32bit we can assume account ID for
	// current universe
	bool bAllDigits = true;
	for ( int i = 0; bAllDigits && i < strInput.Length(); i++ )
		{ bAllDigits = bAllDigits && V_isdigit( strInput[i] ); }

	if ( bAllDigits )
	{
		uint64_t ullParsed = V_atoi64( strInput );
		if ( ullParsed > 0 && ullParsed < UINT32_MAX ) // 0 and ~0 are bogus accountID values
		{
			CSteamID steamID( (uint32_t)ullParsed, localUniverse, k_EAccountTypeIndividual );
			if ( steamID.IsValid() )
				{ return steamID; }
		}
	}

	// See if it's a profile link. If it is, clip the SteamID from it.
	if ( V_strncmp( strInput, "http://", 7 ) == 0 )
		{ strInput = strInput.Slice( 0, 7 ); }
	if ( V_strncmp( strInput, "https://", 8 ) == 0 )
		{ strInput = strInput.Slice( 0, 8 ); }
	if ( V_strncmp( strInput, "www.", 4 ) == 0 )
		{ strInput = strInput.Slice( 0, 4 ); }

	const char pszProfilePrepend[] = "steamcommunity.com/profiles/";
	const size_t lenProfilePrepend = V_ARRAYSIZE( pszProfilePrepend ) - 1;
	if ( strInput.Length() > (int)lenProfilePrepend &&
	     V_strncmp( pszProfilePrepend, strInput, lenProfilePrepend ) == 0 )
	{
		// Read up to ? or # or /
		const char *pEnd = strchr( strInput + lenProfilePrepend, '?' );
		const char *pPound = strchr( strInput + lenProfilePrepend, '#' );
		if ( pPound < pEnd ) { pEnd = pPound; }
		const char *pSlash = strchr( strInput + lenProfilePrepend, '/' );
		if ( pSlash < pEnd ) { pEnd = pSlash; }

		strInput = strInput.Slice( lenProfilePrepend, pEnd ? ( pEnd - strInput.Get() ) : strInput.Length() );

		// /profiles/[U:1:2] *does* work, but STEAM_BLAH does not
		CSteamID steamID = UTIL_SteamIDFromProperString( strInput.Get(), /* bAllowSteam2 */ false );
		if ( steamID.IsValid() && ( !bCurrentUniverse || steamID.GetEUniverse() == localUniverse ) )
			{ return steamID; }
	}

	return CSteamID();
}

#define WORKSHOP_PREFIX_1		"workshop/"
#define MAP_WORKSHOP_PREFIX_1	"maps/" WORKSHOP_PREFIX_1

#define WORKSHOP_PREFIX_2		"workshop\\"
#define MAP_WORKSHOP_PREFIX_2	"maps\\" WORKSHOP_PREFIX_2

const char *GetCleanMapName( const char *pszUnCleanMapName, char (&pszTmp)[256])
{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	bool bPrefixMaps = true;
	const char *pszMapAfterPrefix = StringAfterPrefixCaseSensitive( pszUnCleanMapName, MAP_WORKSHOP_PREFIX_1 );
	if ( !pszMapAfterPrefix )
		pszMapAfterPrefix = StringAfterPrefixCaseSensitive( pszUnCleanMapName, MAP_WORKSHOP_PREFIX_2 );

	if ( !pszMapAfterPrefix )
	{
		bPrefixMaps = false;
		pszMapAfterPrefix = StringAfterPrefixCaseSensitive( pszUnCleanMapName, WORKSHOP_PREFIX_1 );
		if ( !pszMapAfterPrefix )
			pszMapAfterPrefix = StringAfterPrefixCaseSensitive( pszUnCleanMapName, WORKSHOP_PREFIX_2 );
	}

	if ( pszMapAfterPrefix )
	{
		if ( bPrefixMaps )
		{
			V_strcpy_safe( pszTmp, "maps" CORRECT_PATH_SEPARATOR_S );
			V_strcat_safe( pszTmp, pszMapAfterPrefix );
		}
		else
		{
			V_strcpy_safe( pszTmp, pszMapAfterPrefix );
		}

		char *pszUGC = V_strstr( pszTmp, ".ugc" );
		if ( pszUGC )
			*pszUGC = '\0';

		return pszTmp;
	}
#endif

	return pszUnCleanMapName;
}
