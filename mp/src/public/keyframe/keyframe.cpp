//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>

typedef unsigned char byte;
#pragma warning(disable:4244)

#include "tier0/dbg.h"
#include "mathlib/vector.h"
#include "keyframe.h"
#include "mathlib/mathlib.h"
#include "rope_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
//  Implementation of keyframe.h interface
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//  Key Frames
//-----------------------------------------------------------------------------
#define HIGHEST_KEYFRAME	3
#define LOWEST_KEYFRAME		-3

#define TOTAL_KEYFRAMES		(HIGHEST_KEYFRAME - LOWEST_KEYFRAME + 1)

//

struct KeyFrame_t
{
	Vector vPos;
	Quaternion qRot;
};


KeyFrame_t g_KeyFrames[ TOTAL_KEYFRAMES ];
KeyFrame_t *g_KeyFramePtr = &g_KeyFrames[ -LOWEST_KEYFRAME ];	// points to the middle keyframe, keyframe 0

bool Motion_SetKeyAngles( int keyNum, Quaternion &quatAngles )
{
	if ( keyNum > HIGHEST_KEYFRAME || keyNum < LOWEST_KEYFRAME )
		return false;

	g_KeyFramePtr[keyNum].qRot = quatAngles;
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Time Modifier function enumeration & implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef float (*TimeModifierFunc_t)(float);

typedef struct 
{
	const char *szName;
	TimeModifierFunc_t pFunc;

} TimeModifier_t;

float TimeModifierFunc_Linear( float time )
{
	return time;
}

float TimeModifierFunc_Cosine( float time )
{
	return ( cos((time+1) * M_PI) * 0.5 ) + 0.5;
}

float TimeModifierFunc_TimeSquared( float time )
{
	return (time * time);
}

TimeModifier_t g_TimeModifiers[] =
{
	{ "Linear", TimeModifierFunc_Linear },
	{ "Accel/Deaccel (cosine)", TimeModifierFunc_Cosine },
	{ "Accel (time*time)", TimeModifierFunc_TimeSquared },
};

int Motion_GetNumberOfTimeModifiers( void )
{
	return ARRAYSIZE(g_TimeModifiers);
}

bool Motion_GetTimeModifierDetails( int timeInterpNum, const char **outName )
{
	if ( timeInterpNum < 0 || timeInterpNum >= Motion_GetNumberOfTimeModifiers() )
	{
		return false;
	}

	if ( !g_TimeModifiers[0].szName || !g_TimeModifiers[0].pFunc )
	{
		return false;
	}

	if ( outName )
		*outName = g_TimeModifiers[0].szName;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//			timeModifierFuncNum - 
//			*outNewTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Motion_CalculateModifiedTime( float time, int timeModifierFuncNum, float *outNewTime )
{
	*outNewTime = g_TimeModifiers[timeModifierFuncNum].pFunc( time );
	return true;
}




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Position interpolator function enumeration & implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// ------------------------------------------------------------------------------------ //
// Linear position interpolator.
// ------------------------------------------------------------------------------------ //

class CPositionInterpolator_Linear : public IPositionInterpolator
{
public:
	virtual void		Release();
	virtual void		GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq );
	virtual void		SetKeyPosition( int keyNum, Vector const &vPos );
	virtual void		InterpolatePosition( float time, Vector &vOut );
	virtual bool		ProcessKey( char const *pName, char const *pValue ) { return false; }
};

CPositionInterpolator_Linear g_LinearInterpolator;

IPositionInterpolator* GetLinearInterpolator()
{
	return &g_LinearInterpolator;
}

void CPositionInterpolator_Linear::Release()
{
}

void CPositionInterpolator_Linear::GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq )
{
	*outName = "Linear";
	*outMinKeyReq = 0;
	*outMaxKeyReq = 1;
}

void CPositionInterpolator_Linear::SetKeyPosition( int keyNum, Vector const &vPos )
{
	Assert ( keyNum <= HIGHEST_KEYFRAME && keyNum >= LOWEST_KEYFRAME );
	VectorCopy( vPos, g_KeyFramePtr[keyNum].vPos );
}

void CPositionInterpolator_Linear::InterpolatePosition( float time, Vector &vOut )
{
	VectorLerp( g_KeyFramePtr[0].vPos, g_KeyFramePtr[1].vPos, time, vOut );
}





// ------------------------------------------------------------------------------------ //
// Catmull-Rom position interpolator.
// ------------------------------------------------------------------------------------ //

class CPositionInterpolator_CatmullRom : public IPositionInterpolator
{
public:
	virtual void		Release();
	virtual void		GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq );
	virtual void		SetKeyPosition( int keyNum, Vector const &vPos );
	virtual void		InterpolatePosition( float time, Vector &vOut );
	virtual bool		ProcessKey( char const *pName, char const *pValue ) { return false; }
};

CPositionInterpolator_CatmullRom g_CatmullRomInterpolator;

IPositionInterpolator* GetCatmullRomInterpolator()
{
	return &g_CatmullRomInterpolator;
}

void CPositionInterpolator_CatmullRom::Release()
{
}

void CPositionInterpolator_CatmullRom::GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq )
{
	*outName = "Catmull-Rom Spline";
	*outMinKeyReq = -1;
	*outMaxKeyReq = 2;
}

void CPositionInterpolator_CatmullRom::SetKeyPosition( int keyNum, Vector const &vPos )
{
	Assert ( keyNum <= HIGHEST_KEYFRAME && keyNum >= LOWEST_KEYFRAME );
	VectorCopy( vPos, g_KeyFramePtr[keyNum].vPos );
}

void CPositionInterpolator_CatmullRom::InterpolatePosition( float time, Vector &vOut )
{
	Catmull_Rom_Spline( 
		g_KeyFramePtr[-1].vPos,
		g_KeyFramePtr[0].vPos,
		g_KeyFramePtr[1].vPos,
		g_KeyFramePtr[2].vPos,
		time,
		vOut );
}



// ------------------------------------------------------------------------------------ //
// Rope interpolator.
// ------------------------------------------------------------------------------------ //
#include "rope_physics.h"

class CRopeDelegate : public CSimplePhysics::IHelper
{
public:
	virtual void	GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel );
	virtual void	ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes );


public:
	Vector			m_CurEndPoints[2];
};

void CRopeDelegate::GetNodeForces( CSimplePhysics::CNode *pNodes, int iNode, Vector *pAccel )
{
	// Gravity.
	pAccel->Init( 0, 0, -1500 );
}

void CRopeDelegate::ApplyConstraints( CSimplePhysics::CNode *pNodes, int nNodes )
{
	if( nNodes >= 2 )
	{
		pNodes[0].m_vPos        = m_CurEndPoints[0];
		pNodes[nNodes-1].m_vPos = m_CurEndPoints[1];
	}
}


class CPositionInterpolator_Rope : public IPositionInterpolator
{
public:
						CPositionInterpolator_Rope();

	virtual void		Release();
	virtual void		GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq );
	virtual void		SetKeyPosition( int keyNum, Vector const &vPos );
	virtual void		InterpolatePosition( float time, Vector &vOut );
	virtual bool		ProcessKey( char const *pName, char const *pValue );


private:
	CRopePhysics<10>	m_RopePhysics;
	CRopeDelegate		m_Delegate;

	float				m_flSlack;	// Extra length of rope.

	bool				m_bChange;
	int					m_nSegments;
};

IPositionInterpolator* GetRopeInterpolator()
{
	return new CPositionInterpolator_Rope;
}


CPositionInterpolator_Rope::CPositionInterpolator_Rope()
{
	m_flSlack = 0;
	m_bChange = false;
	m_nSegments = 5;

	for( int i=0; i < 2; i++ )
		m_Delegate.m_CurEndPoints[i] = Vector( 1e24, 1e24, 1e24 );
}

void CPositionInterpolator_Rope::Release()
{
	delete this;
}

void CPositionInterpolator_Rope::GetDetails( char **outName, int *outMinKeyReq, int *outMaxKeyReq )
{
	*outName = "Rope";
	*outMinKeyReq = 0;
	*outMinKeyReq = 1;
}

void CPositionInterpolator_Rope::SetKeyPosition( int keyNum, Vector const &vPos )
{
	if( keyNum == 0 || keyNum == 1 )
	{
		if( vPos != m_Delegate.m_CurEndPoints[keyNum] )
			m_bChange = true;

		m_Delegate.m_CurEndPoints[keyNum] = vPos;
	}
}

void CPositionInterpolator_Rope::InterpolatePosition( float time, Vector &vOut )
{
	// Check if we need to resimulate..
	if( m_bChange )
	{
		m_RopePhysics.SetNumNodes( m_nSegments );

		// Init all the nodes.
		for( int i=0; i < m_RopePhysics.NumNodes(); i++ )
			m_RopePhysics.GetNode(i)->m_vPos = m_RopePhysics.GetNode(i)->m_vPrevPos = m_Delegate.m_CurEndPoints[0];

		float flDist = (m_Delegate.m_CurEndPoints[0] - m_Delegate.m_CurEndPoints[1]).Length();
		flDist += m_flSlack;

		m_RopePhysics.Restart();
		m_RopePhysics.SetupSimulation( flDist / (m_RopePhysics.NumNodes() - 1), &m_Delegate );

		// Run the simulation for a while to let the rope settle down..
		m_RopePhysics.Simulate( 5 );
	
		m_bChange = false;
	}

	// Ok, now we have all the nodes setup..
	float flNode = time * (m_RopePhysics.NumNodes()-1);
	int iNode = (int)( flNode );
	VectorLerp( 
		m_RopePhysics.GetNode(iNode)->m_vPredicted,
		m_RopePhysics.GetNode(iNode+1)->m_vPredicted,
		flNode - iNode,
		vOut );
}

bool CPositionInterpolator_Rope::ProcessKey( char const *pName, char const *pValue )
{
	if( stricmp( pName, "Slack" ) == 0 )
	{
		m_flSlack = atof( pValue ) + ROPESLACK_FUDGEFACTOR;
		m_bChange = true;
		return true;
	}
	else if( stricmp( pName, "Type" ) == 0 )
	{
		int iType = atoi( pValue );
		if( iType == 0 )
			m_nSegments = ROPE_MAX_SEGMENTS;
		else if( iType == 1 )
			m_nSegments = ROPE_TYPE1_NUMSEGMENTS;
		else
			m_nSegments = ROPE_TYPE2_NUMSEGMENTS;

		m_bChange = true;
		return true;
	}

	return false;
}



// ------------------------------------------------------------------------------------ //
// The global table of all the position interpolators.
// ------------------------------------------------------------------------------------ //

typedef IPositionInterpolator* (*PositionInterpolatorCreateFn)();
PositionInterpolatorCreateFn g_PositionInterpolatorCreateFns[] =
{
	GetLinearInterpolator,
	GetCatmullRomInterpolator,
	GetRopeInterpolator
};

int Motion_GetNumberOfPositionInterpolators( void )
{
	return ARRAYSIZE(g_PositionInterpolatorCreateFns);
}


IPositionInterpolator* Motion_GetPositionInterpolator( int interpNum )
{
	Assert( interpNum >= 0 && interpNum < Motion_GetNumberOfPositionInterpolators() );
	return g_PositionInterpolatorCreateFns[clamp( interpNum, 0, Motion_GetNumberOfPositionInterpolators() - 1 )]();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//  Rotation interpolator function enumeration & implementation
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
typedef void (*RotationInterpolatorFunc_t)(float time, Quaternion &outRot);

typedef struct 
{
	char *szName;
	RotationInterpolatorFunc_t pFunc;

	// defines the range of keys this interpolator needs to function
	int iMinReqKeyFrame;
	int iMaxReqKeyFrame;

} RotationInterpolator_t;

void RotationInterpolatorFunc_Linear( float time, Quaternion &outRot )
{
	// basic 4D spherical linear interpolation
	QuaternionSlerp( g_KeyFramePtr[0].qRot, g_KeyFramePtr[1].qRot, time, outRot );
}

RotationInterpolator_t g_RotationInterpolators[] =
{
	{ "Linear", RotationInterpolatorFunc_Linear, 0, 1 },
};

int Motion_GetNumberOfRotationInterpolators( void )
{
	return ARRAYSIZE(g_RotationInterpolators);
}

bool Motion_GetRotationInterpolatorDetails( int rotInterpNum, char **outName, int *outMinKeyReq, int *outMaxKeyReq )
{
	if ( rotInterpNum < 0 || rotInterpNum >= Motion_GetNumberOfRotationInterpolators() )
	{
		return false;
	}

	if ( !g_RotationInterpolators[rotInterpNum].szName || !g_RotationInterpolators[rotInterpNum].pFunc )
	{
		return false;
	}

	if ( outName )
		*outName = g_RotationInterpolators[rotInterpNum].szName;

	if ( outMinKeyReq )
		*outMinKeyReq = g_RotationInterpolators[rotInterpNum].iMinReqKeyFrame;

	if ( outMaxKeyReq )
		*outMaxKeyReq = g_RotationInterpolators[rotInterpNum].iMaxReqKeyFrame;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Interpolates a rotation
//			Time is assumed to have already been modified by the TimeModifyFunc (above)
//			Requires the keyframes be already set
// Input  : time - value from 0..1
//			interpFuncNum - 
//			*outQuatRotation - result in quaternion form
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Motion_InterpolateRotation( float time, int interpFuncNum, Quaternion &outQuatRotation )
{
	if ( time < 0.0f || time > 1.0f )
		return false;

	g_RotationInterpolators[interpFuncNum].pFunc( time, outQuatRotation );
	return true;
}
