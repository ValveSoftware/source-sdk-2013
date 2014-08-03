//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "entityblocker.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( entity_blocker, CEntityBlocker );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&mins - 
//			&maxs - 
//			NULL - 
// Output : CEntityBlocker
//-----------------------------------------------------------------------------
CEntityBlocker *CEntityBlocker::Create( const Vector &origin, const Vector &mins, const Vector &maxs, CBaseEntity *pOwner, bool bBlockPhysics )
{
	CEntityBlocker *pBlocker = (CEntityBlocker *) CBaseEntity::Create( "entity_blocker", origin, vec3_angle, pOwner );
	
	if ( pBlocker != NULL )
	{
		pBlocker->SetSize( mins, maxs );
		if ( bBlockPhysics )
		{
			pBlocker->VPhysicsInitStatic();
		}
	}

	return pBlocker;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEntityBlocker::Spawn( void )
{
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST );
}

//-----------------------------------------------------------------------------
// Purpose: Entity blockers don't block tracelines so they don't screw up weapon fire, etc
//-----------------------------------------------------------------------------
bool CEntityBlocker::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	return false;
}

//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CC_Test_Entity_Blocker( void )
{
	CBasePlayer *pPlayer = UTIL_GetCommandClient();
	Vector vecForward;
	pPlayer->GetVectors( &vecForward, NULL, NULL );

	trace_t tr;
	Vector vecOrigin = pPlayer->GetAbsOrigin() + (vecForward * 256);
	UTIL_TraceHull( vecOrigin + Vector(0,0,256), vecOrigin - Vector(0,0,256), VEC_HULL_MIN_SCALED( pPlayer ), VEC_HULL_MAX_SCALED( pPlayer ), MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( !tr.allsolid && !tr.startsolid )
	{
		CEntityBlocker::Create( tr.endpos, VEC_HULL_MIN_SCALED( pPlayer ), VEC_HULL_MAX_SCALED( pPlayer ), NULL, true );
		NDebugOverlay::Box( tr.endpos, VEC_HULL_MIN_SCALED( pPlayer ), VEC_HULL_MAX_SCALED( pPlayer ), 0, 255, 0, 64, 1000.0 );
	}
}
static ConCommand test_entity_blocker("test_entity_blocker", CC_Test_Entity_Blocker, "Test command that drops an entity blocker out in front of the player.", FCVAR_CHEAT );