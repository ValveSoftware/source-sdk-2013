//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "entityoutput.h"
#include "ndebugoverlay.h"
#include "func_bulletshield.h"
#include "collisionutils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ent_debugkeys;
extern ConVar	showtriggers;



LINK_ENTITY_TO_CLASS( func_bulletshield, CFuncBulletShield );

BEGIN_DATADESC( CFuncBulletShield )

/*
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_KEYFIELD( m_iDisabled, FIELD_INTEGER, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iSolidity, FIELD_INTEGER, "Solidity" ),
	DEFINE_KEYFIELD( m_bSolidBsp, FIELD_BOOLEAN, "solidbsp" ),
	DEFINE_KEYFIELD( m_iszExcludedClass, FIELD_STRING, "excludednpc" ),
	DEFINE_KEYFIELD( m_bInvertExclusion, FIELD_BOOLEAN, "invert_exclusion" ),
*/

END_DATADESC()



void CFuncBulletShield::Spawn( void )
{
	BaseClass::Spawn();

	AddSolidFlags( FSOLID_CUSTOMRAYTEST );
	AddSolidFlags( FSOLID_CUSTOMBOXTEST );
	// SetSolid(SOLID_CUSTOM);

	VPhysicsDestroyObject();
}

/*
bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Vector &vecRayOrigin, const Vector &vecRayDelta, 
	const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Ray_t &ray, const Vector &vecBoxOrigin, const QAngle &angBoxRotation,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Ray_t &ray, const matrix3x4_t &matOBBToWorld,
	const Vector &vecOBBMins, const Vector &vecOBBMaxs, float flTolerance, CBaseTrace *pTrace );

bool IntersectRayWithOBB( const Vector &vecRayStart, const Vector &vecRayDelta, 
	const matrix3x4_t &matOBBToWorld, const Vector &vecOBBMins, const Vector &vecOBBMaxs, 
	float flTolerance, BoxTraceInfo_t *pTrace );
	*/

bool CFuncBulletShield::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	// ignore unless a shot
	if ((mask & MASK_SHOT)	 == MASK_SHOT)
	{
		// use obb collision
		ICollideable *pCol = GetCollideable();
		Assert(pCol);

		return IntersectRayWithOBB(ray,pCol->GetCollisionOrigin(),pCol->GetCollisionAngles(),
			pCol->OBBMins(),pCol->OBBMaxs(),1.0f,&trace);

		/*
		const model_t *pModel = this->GetCollisionModel();
		if ( pModel && pModel->type == mod_brush )
		{
			int nModelIndex = this->GetCollisionModelIndex();
			cmodel_t *pCModel = CM_InlineModelNumber( nModelIndex - 1 );
			int nHeadNode = pCModel->headnode;

			CM_TransformedBoxTrace( ray, nHeadNode, fMask, this->GetCollisionOrigin(), this->GetCollisionAngles(), *pTrace );
			return true;
		}
		return false;
		*/

		// return BaseClass::TestCollision( ray, mask, trace );
	}
	else
		return false;
}
