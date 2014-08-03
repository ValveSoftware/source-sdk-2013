//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_ai_basenpc.h"
#include "engine/ivmodelinfo.h"
#include "rope_physics.h"
#include "materialsystem/imaterialsystem.h"
#include "fx_line.h"
#include "engine/ivdebugoverlay.h"
#include "bone_setup.h"
#include "model_types.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BARNACLE_TONGUE_POINTS		7

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_NPC_Barnacle : public C_AI_BaseNPC
{
public:

	DECLARE_CLASS( C_NPC_Barnacle, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_NPC_Barnacle( void );

	virtual void GetRenderBounds( Vector &theMins, Vector &theMaxs )
	{
		BaseClass::GetRenderBounds( theMins, theMaxs );

		// Extend our bounding box downwards the length of the tongue
   		theMins -= Vector( 0, 0, m_flAltitude );
	}

	// Purpose: Initialize absmin & absmax to the appropriate box
	virtual void ComputeWorldSpaceSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
	{
		// Extend our bounding box downwards the length of the tongue
		CollisionProp()->WorldSpaceAABB( pVecWorldMins, pVecWorldMaxs );

		// We really care about the tongue tip. The altitude is not really relevant.
		VectorMin( *pVecWorldMins, m_vecTip, *pVecWorldMins );
		VectorMax( *pVecWorldMaxs, m_vecTip, *pVecWorldMaxs );

//		pVecWorldMins->z -= m_flAltitude;
	}

	void	OnDataChanged( DataUpdateType_t updateType );
	void	InitTonguePhysics( void );
	void	ClientThink( void );
	void	StandardBlendingRules( CStudioHdr *pStudioHdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );

	void	SetVecTip( const float *pPosition );
	void	SetAltitude( float flAltitude );

	// Purpose: 
	void	ComputeVisualTipPoint( Vector *pTip );

protected:
	Vector	m_vecTipPrevious;
	Vector	m_vecRoot;
	Vector	m_vecTip;
	Vector  m_vecTipDrawOffset;

private:
	// Tongue points
	float	m_flAltitude;
	Vector	m_vecTonguePoints[BARNACLE_TONGUE_POINTS];
	CRopePhysics<BARNACLE_TONGUE_POINTS>	m_TonguePhysics;

	// Tongue physics delegate
	class CBarnaclePhysicsDelegate : public CSimplePhysics::IHelper
	{
	public:
		virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
		virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );
	
		C_NPC_Barnacle	*m_pBarnacle;
	};
	friend class CBarnaclePhysicsDelegate;
	CBarnaclePhysicsDelegate	m_PhysicsDelegate;

private:
	C_NPC_Barnacle( const C_NPC_Barnacle & ); // not defined, not accessible
};

static void RecvProxy_VecTip( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	((C_NPC_Barnacle*)pStruct)->SetVecTip( pData->m_Value.m_Vector );
}

IMPLEMENT_CLIENTCLASS_DT( C_NPC_Barnacle, DT_Barnacle, CNPC_Barnacle )
	RecvPropFloat( RECVINFO( m_flAltitude ) ),
	RecvPropVector( RECVINFO( m_vecRoot ) ),
	RecvPropVector( RECVINFO( m_vecTip ), 0, RecvProxy_VecTip ),
	RecvPropVector( RECVINFO( m_vecTipDrawOffset ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_NPC_Barnacle::C_NPC_Barnacle( void )
{
	m_PhysicsDelegate.m_pBarnacle = this;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		InitTonguePhysics();

		// We want to think every frame.
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
}


//-----------------------------------------------------------------------------
// Sets the tongue altitude
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::SetAltitude( float flAltitude )
{
	m_flAltitude = flAltitude;
}

void C_NPC_Barnacle::SetVecTip( const float *pPosition )
{
	Vector vecNewTip;
	vecNewTip.Init( pPosition[0], pPosition[1], pPosition[2] );
	if ( vecNewTip != m_vecTip )
	{
		m_vecTip = vecNewTip;
		CollisionProp()->MarkSurroundingBoundsDirty();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::InitTonguePhysics( void )
{
	// Init tongue spline
	// First point is at the top
	m_TonguePhysics.SetupSimulation( m_flAltitude / (BARNACLE_TONGUE_POINTS-1), &m_PhysicsDelegate );
	m_TonguePhysics.Restart();

	// Initialize the positions of the nodes.
	m_TonguePhysics.GetFirstNode()->m_vPos = m_vecRoot;
	m_TonguePhysics.GetFirstNode()->m_vPrevPos = m_TonguePhysics.GetFirstNode()->m_vPos;
	float flAltitude = m_flAltitude;
	for( int i = 1; i < m_TonguePhysics.NumNodes(); i++ )
	{
		flAltitude *= 0.5;
		CSimplePhysics::CNode *pNode = m_TonguePhysics.GetNode( i );
		pNode->m_vPos = m_TonguePhysics.GetNode(i-1)->m_vPos - Vector(0,0,flAltitude);
		pNode->m_vPrevPos = pNode->m_vPos;

		// Set the length of the node's spring
		//m_TonguePhysics.ResetNodeSpringLength( i-1, flAltitude );
	}

	m_vecTipPrevious = m_vecTip;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::ClientThink( void )
{
	m_TonguePhysics.Simulate( gpGlobals->frametime );

	// Set the spring's length to that of the tongue's extension
	m_TonguePhysics.ResetSpringLength( m_flAltitude / (BARNACLE_TONGUE_POINTS-1) );

	// Necessary because ComputeVisualTipPoint depends on m_vecTipPrevious
	Vector vecTemp;
	ComputeVisualTipPoint( &vecTemp );
	m_vecTipPrevious = vecTemp; 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	if ( !hdr )
		return;

	int firstBone = Studio_BoneIndexByName( hdr, "Barnacle.tongue1" );

	Vector vecPrevRight;
	GetVectors( NULL, &vecPrevRight, NULL );

	Vector vecPrev = pos[Studio_BoneIndexByName( hdr, "Barnacle.base" )];
	Vector vecCurr = vec3_origin;
	Vector vecForward;
	for ( int i = 0; i <= BARNACLE_TONGUE_POINTS; i++ )
	{
		// We double up the bones at the last node.
		if ( i == BARNACLE_TONGUE_POINTS )
		{
			vecCurr = m_TonguePhysics.GetLastNode()->m_vPos;
		}
		else
		{
			vecCurr = m_TonguePhysics.GetNode(i)->m_vPos;
		}

		//debugoverlay->AddBoxOverlay( vecCurr, -Vector(2,2,2), Vector(2,2,2), vec3_angle, 0,255,0, 128, 0.1 );

		// Fill out the positions in local space
		VectorITransform( vecCurr, EntityToWorldTransform(), pos[firstBone+i] );
		vecCurr = pos[firstBone+i];

		// Disallow twist in the tongue visually
		// Forward vector has to follow the tongue, right + up have to minimize twist from
		// the previous bone

		// Fill out the angles
		if ( i != BARNACLE_TONGUE_POINTS )
		{
			vecForward = (vecCurr - vecPrev);
			if ( VectorNormalize( vecForward ) < 1e-3 )
			{
				vecForward.Init( 0, 0, 1 );
			}
		}

		// Project the previous vecRight into a plane perpendicular to vecForward
		// that's the vector closest to what we want...
		Vector vecRight, vecUp;
		VectorMA( vecPrevRight, -DotProduct( vecPrevRight, vecForward ), vecForward, vecRight );
		VectorNormalize( vecRight );
		CrossProduct( vecForward, vecRight, vecUp );

		BasisToQuaternion( vecForward, vecRight, vecUp, q[firstBone+i] );

		vecPrev = vecCurr;
		vecPrevRight = vecRight;
	}
}

//===============================================================================================================================
// BARNACLE TONGUE PHYSICS
//===============================================================================================================================
#define TONGUE_GRAVITY			0, 0, -1000
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::CBarnaclePhysicsDelegate::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	// Gravity.
	pAccel->Init( TONGUE_GRAVITY );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#define TIP_SNAP_FACTOR 200
// Todo: this really ought to be SIMD.
void C_NPC_Barnacle::ComputeVisualTipPoint( Vector *pTip )
{
	float flTipMove = TIP_SNAP_FACTOR * gpGlobals->frametime;
	Vector tipIdeal;
	VectorAdd(m_vecTip, m_vecTipDrawOffset, tipIdeal);
	if ( tipIdeal.DistToSqr( m_vecTipPrevious ) > (flTipMove * flTipMove) )
	{
		// Inch the visual tip toward the actual tip
		VectorSubtract( tipIdeal, m_vecTipPrevious, *pTip );
		VectorNormalize( *pTip );
		*pTip *= flTipMove;
		*pTip += m_vecTipPrevious;
	}
	else
	{
		*pTip = tipIdeal;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_NPC_Barnacle::CBarnaclePhysicsDelegate::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	// Startpoint always stays at the root
	pNodes[0].m_vPos = m_pBarnacle->m_vecRoot;

	// Endpoint always stays at the tip
	m_pBarnacle->ComputeVisualTipPoint( &pNodes[nNodes-1].m_vPos );
}
