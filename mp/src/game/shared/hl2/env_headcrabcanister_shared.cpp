//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "env_headcrabcanister_shared.h"
#include "mapdata_shared.h"
#include "sharedInterface.h"
#include "mathlib/vmatrix.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ROTATION_SPEED 90.0f


BEGIN_SIMPLE_DATADESC( CEnvHeadcrabCanisterShared )
	DEFINE_FIELD( m_vecStartPosition,			FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecEnterWorldPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecDirection,				FIELD_VECTOR ),
	DEFINE_FIELD( m_vecStartAngles,				FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_flLaunchHeight,			FIELD_FLOAT,	"StartingHeight" ),
	DEFINE_KEYFIELD( m_flFlightSpeed,			FIELD_FLOAT,	"FlightSpeed" ),
	DEFINE_KEYFIELD( m_flFlightTime,			FIELD_FLOAT,	"FlightTime" ),
	DEFINE_FIELD( m_flLaunchTime,				FIELD_TIME ),
	DEFINE_FIELD( m_flWorldEnterTime,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flInitialZSpeed,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flZAcceleration,			FIELD_FLOAT ),
	DEFINE_FIELD( m_flHorizSpeed,				FIELD_FLOAT ),
	DEFINE_FIELD( m_bLaunchedFromWithinWorld,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vecSkyboxOrigin,			FIELD_VECTOR ),
	DEFINE_FIELD( m_vecParabolaDirection,		FIELD_VECTOR ),
	DEFINE_FIELD( m_flSkyboxScale,				FIELD_FLOAT ),
	DEFINE_FIELD( m_bInSkybox,					FIELD_BOOLEAN ),
END_DATADESC()


BEGIN_NETWORK_TABLE_NOBASE( CEnvHeadcrabCanisterShared, DT_EnvHeadcrabCanisterShared )

#if !defined( CLIENT_DLL )
	SendPropFloat	( SENDINFO( m_flFlightSpeed ),			0, SPROP_NOSCALE ),	
	SendPropTime	( SENDINFO( m_flLaunchTime ) ),
	SendPropVector	( SENDINFO( m_vecParabolaDirection ),	0, SPROP_NOSCALE ),	

	SendPropFloat	( SENDINFO( m_flFlightTime ),			0, SPROP_NOSCALE ),
	SendPropFloat	( SENDINFO( m_flWorldEnterTime ),		0, SPROP_NOSCALE ),

	SendPropFloat	( SENDINFO( m_flInitialZSpeed ),		0, SPROP_NOSCALE ),
	SendPropFloat	( SENDINFO( m_flZAcceleration ),		0, SPROP_NOSCALE ),
	SendPropFloat	( SENDINFO( m_flHorizSpeed ),			0, SPROP_NOSCALE ),
	SendPropBool	( SENDINFO( m_bLaunchedFromWithinWorld ) ),
	
	SendPropVector	( SENDINFO( m_vecStartPosition ),       0, SPROP_NOSCALE ),	
	SendPropVector	( SENDINFO( m_vecEnterWorldPosition ),  0, SPROP_NOSCALE ),	
	SendPropVector	( SENDINFO( m_vecDirection ),			0, SPROP_NOSCALE ),	
	SendPropVector	( SENDINFO( m_vecStartAngles ),			0, SPROP_NOSCALE ),	

	SendPropVector	( SENDINFO( m_vecSkyboxOrigin ),		0, SPROP_NOSCALE ),	
	SendPropFloat	( SENDINFO( m_flSkyboxScale ),			0, SPROP_NOSCALE ),
	SendPropBool	( SENDINFO( m_bInSkybox ) ),
#else
	RecvPropFloat	( RECVINFO( m_flFlightSpeed ) ),	
	RecvPropTime	( RECVINFO( m_flLaunchTime ) ),
	RecvPropVector	( RECVINFO( m_vecParabolaDirection ) ),	

	RecvPropFloat	( RECVINFO( m_flFlightTime ) ),	
	RecvPropFloat	( RECVINFO( m_flWorldEnterTime ) ),	

	RecvPropFloat	( RECVINFO( m_flInitialZSpeed ) ),	
	RecvPropFloat	( RECVINFO( m_flZAcceleration ) ),	
	RecvPropFloat	( RECVINFO( m_flHorizSpeed ) ),	
	RecvPropBool	( RECVINFO( m_bLaunchedFromWithinWorld ) ),	

	RecvPropVector	( RECVINFO( m_vecStartPosition ) ),	
	RecvPropVector	( RECVINFO( m_vecEnterWorldPosition ) ),	
	RecvPropVector	( RECVINFO( m_vecDirection ) ),	
	RecvPropVector	( RECVINFO( m_vecStartAngles ) ),	

	RecvPropVector	( RECVINFO( m_vecSkyboxOrigin ) ),	
	RecvPropFloat	( RECVINFO( m_flSkyboxScale ) ),	
	RecvPropBool	( RECVINFO( m_bInSkybox ) ),	
#endif

END_NETWORK_TABLE()



//=============================================================================
//
// HeadcrabCanister Functions.
//

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CEnvHeadcrabCanisterShared::CEnvHeadcrabCanisterShared()
{
	m_vecStartPosition.Init();
	m_vecDirection.Init();
	m_flFlightSpeed = 0.0f;

	// This tells the client DLL to not draw trails, etc.
	m_flLaunchTime = -1.0f;

	m_flWorldEnterTime = 0.0f;
	m_flFlightTime = 0.0f;
	m_bInSkybox = false;
}


//-----------------------------------------------------------------------------
// Creates a headcrab canister in the world
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanisterShared::InitInWorld( float flLaunchTime, 
	const Vector &vecStartPosition, const QAngle &vecStartAngles, 
	const Vector &vecDirection, const Vector &vecImpactPosition, bool bLaunchedFromWithinWorld )
{
	Vector vecActualStartPosition = vecStartPosition;
	if ( !bLaunchedFromWithinWorld )
	{
		// Move the start position inward if it's too close
		Vector vecDelta;
		VectorSubtract( vecStartPosition, vecImpactPosition, vecDelta );
		VectorNormalize( vecDelta );

		VectorMA( vecImpactPosition, m_flFlightTime * m_flFlightSpeed, vecDelta, vecActualStartPosition );
	}
 
	// Setup initial parametric state.
	m_flLaunchTime = flLaunchTime;
	m_vecStartPosition = vecActualStartPosition;
	m_vecEnterWorldPosition = vecActualStartPosition;
	m_vecDirection = vecDirection;
	m_vecStartAngles = vecStartAngles;
	m_flWorldEnterTime = 0.0f;
	m_bInSkybox = false;
	m_bLaunchedFromWithinWorld = bLaunchedFromWithinWorld;
 
	if ( m_bLaunchedFromWithinWorld )
	{
		m_flSkyboxScale = 1;
		m_vecSkyboxOrigin = vec3_origin;

		float flLength = m_vecDirection.Get().AsVector2D().Length();
		VectorSubtract(vecImpactPosition, vecStartPosition, m_vecParabolaDirection.GetForModify());
		m_vecParabolaDirection.GetForModify().z = 0;
		float flTotalDistance = VectorNormalize( m_vecParabolaDirection.GetForModify() );
		m_vecDirection.GetForModify().x = flLength * m_vecParabolaDirection.Get().x;
		m_vecDirection.GetForModify().y = flLength * m_vecParabolaDirection.Get().y;
 
		m_flHorizSpeed = flTotalDistance / m_flFlightTime;
		m_flWorldEnterTime = 0;
 
		float flFinalZSpeed = m_vecDirection.Get().z * m_flHorizSpeed;
		m_flFlightSpeed = sqrt( m_flHorizSpeed * m_flHorizSpeed + flFinalZSpeed * flFinalZSpeed );
		m_flInitialZSpeed = (2.0f * ( vecImpactPosition.z - vecStartPosition.z ) - flFinalZSpeed * m_flFlightTime) / m_flFlightTime;
		m_flZAcceleration = (flFinalZSpeed - m_flInitialZSpeed) / m_flFlightTime;
	}
}


//-----------------------------------------------------------------------------
// Creates a headcrab canister in the skybox
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanisterShared::InitInSkybox( float flLaunchTime, 
	const Vector &vecStartPosition, const QAngle &vecStartAngles, const Vector &vecDirection,
	const Vector &vecImpactPosition, const Vector &vecSkyboxOrigin, float flSkyboxScale )
{
	// Compute a horizontal speed (constant)
	m_vecParabolaDirection.Init( vecDirection.x, vecDirection.y, 0.0f );
	float flLength = VectorNormalize( m_vecParabolaDirection.GetForModify() ); 
	m_flHorizSpeed = flLength * m_flFlightSpeed;

	// compute total distance to travel
	float flTotalDistance = m_flFlightTime * m_flHorizSpeed;
	flTotalDistance -= vecStartPosition.AsVector2D().DistTo( vecImpactPosition.AsVector2D() );
	if ( flTotalDistance <= 0.0f )
	{
		InitInWorld( flLaunchTime, vecStartPosition, vecStartAngles, vecDirection, vecImpactPosition );
		return;
	}

	// Setup initial parametric state.
	m_flLaunchTime = flLaunchTime;
	m_flWorldEnterTime = flTotalDistance / m_flHorizSpeed;
	m_vecSkyboxOrigin = vecSkyboxOrigin;
	m_flSkyboxScale = flSkyboxScale;

	m_vecEnterWorldPosition = vecStartPosition;
	m_vecDirection = vecDirection;
	m_vecStartAngles = vecStartAngles;
	m_bInSkybox = true;
	m_bLaunchedFromWithinWorld = false;

	// Compute parabolic course
	// Assume the x velocity remains constant.
	// Z moves ballistically, as if under gravity
	// zf + lh = zo
	// vf = vo + a*t
	// zf = zo + vo*t + 0.5 * a * t*t
	// a*t = vf - vo
	// zf = zo + vo*t + 0.5f * (vf - vo) * t
	// zf - zo = 0.5f *vo*t + 0.5f * vf * t
	// -lh - 0.5f * vf * t = 0.5f * vo * t
	// vo = -2.0f * lh / t - vf
	// a = (vf - vo) / t
	m_flHorizSpeed /= flSkyboxScale;

	VectorMA( vecSkyboxOrigin, 1.0f / m_flSkyboxScale, vecStartPosition, m_vecStartPosition.GetForModify() );
	VectorMA( m_vecStartPosition.Get(), -m_flHorizSpeed * m_flWorldEnterTime, m_vecParabolaDirection, m_vecStartPosition.GetForModify() );

	float flLaunchHeight = m_flLaunchHeight / flSkyboxScale;
	float flFinalZSpeed = m_vecDirection.Get().z * m_flFlightSpeed / flSkyboxScale;
	m_vecStartPosition.GetForModify().z += flLaunchHeight;
	m_flZAcceleration = 2.0f * ( flLaunchHeight + flFinalZSpeed * m_flWorldEnterTime ) / ( m_flWorldEnterTime * m_flWorldEnterTime );
	m_flInitialZSpeed = flFinalZSpeed - m_flZAcceleration * m_flWorldEnterTime;
}


//-----------------------------------------------------------------------------
// Convert from skybox to world
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanisterShared::ConvertFromSkyboxToWorld()
{
	Assert( m_bInSkybox );
	m_bInSkybox = false;
}


//-----------------------------------------------------------------------------
// Returns the time at which it enters the world
//-----------------------------------------------------------------------------
float CEnvHeadcrabCanisterShared::GetEnterWorldTime() const
{
	return m_flWorldEnterTime;
}


//-----------------------------------------------------------------------------
// Did we impact?
//-----------------------------------------------------------------------------
bool CEnvHeadcrabCanisterShared::DidImpact( float flTime ) const
{
	return (flTime - m_flLaunchTime) >= m_flFlightTime;
}


//-----------------------------------------------------------------------------
// Computes the position of the canister
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanisterShared::GetPositionAtTime( float flTime, Vector &vecPosition, QAngle &vecAngles )
{
	float flDeltaTime = flTime - m_flLaunchTime;
	if ( flDeltaTime > m_flFlightTime )
	{
		flDeltaTime = m_flFlightTime;
	}

	VMatrix initToWorld;
	if ( m_bLaunchedFromWithinWorld || m_bInSkybox )
	{
		VectorMA( m_vecStartPosition, flDeltaTime * m_flHorizSpeed, m_vecParabolaDirection, vecPosition );
		vecPosition.z += m_flInitialZSpeed * flDeltaTime + 0.5f * m_flZAcceleration * flDeltaTime * flDeltaTime;

		Vector vecLeft;
		CrossProduct( m_vecParabolaDirection, Vector( 0, 0, 1 ), vecLeft );

		Vector vecForward;
		VectorMultiply( m_vecParabolaDirection, -1.0f, vecForward );
		vecForward.z = -(m_flInitialZSpeed + m_flZAcceleration * flDeltaTime) / m_flHorizSpeed;	// This is -dz/dx.
		VectorNormalize( vecForward );

		Vector vecUp;
		CrossProduct( vecForward, vecLeft, vecUp );
 
		initToWorld.SetBasisVectors( vecForward, vecLeft, vecUp );
	}
	else
	{
		flDeltaTime -= m_flWorldEnterTime;
		Vector vecVelocity;
		VectorMultiply( m_vecDirection, m_flFlightSpeed, vecVelocity );
		VectorMA( m_vecEnterWorldPosition, flDeltaTime, vecVelocity, vecPosition );

		MatrixFromAngles( m_vecStartAngles.Get(), initToWorld );
	}

	VMatrix rotation;
	MatrixBuildRotationAboutAxis( rotation, Vector( 1, 0, 0 ), flDeltaTime * ROTATION_SPEED );

	VMatrix newAngles;
	MatrixMultiply( initToWorld, rotation, newAngles );
	MatrixToAngles( newAngles, vecAngles );
}


//-----------------------------------------------------------------------------
// Are we in the skybox?
//-----------------------------------------------------------------------------
bool CEnvHeadcrabCanisterShared::IsInSkybox( )
{
	// Check to see if we are always in the world!
	return m_bInSkybox;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CEnvHeadcrabCanisterShared::CalcEnterTime( const Vector &vecTriggerMins, 
											  const Vector &vecTriggerMaxs )
{
	/*
#define HEADCRABCANISTER_TRIGGER_EPSILON	0.001f

	// Initialize the enter/exit fractions.
	float flEnterFrac = 0.0f;
	float flExitFrac = 1.0f;

	// Create an arbitrarily large end position.
	Vector vecEndPosition;
	VectorMA( m_vecStartPosition, 32000.0f, m_vecDirection, vecEndPosition );

	float flFrac, flDistStart, flDistEnd;
	for( int iAxis = 0; iAxis < 3; iAxis++ )
	{
		// Negative Axis
		flDistStart = -m_vecStartPosition[iAxis] + vecTriggerMins[iAxis];
		flDistEnd = -vecEndPosition[iAxis] + vecTriggerMins[iAxis];

		if ( ( flDistStart > 0.0f ) && ( flDistEnd < 0.0f ) ) 
		{ 
			flFrac = ( flDistStart - HEADCRABCANISTER_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if ( flFrac > flEnterFrac ) { flEnterFrac = flFrac; }
		}

		if ( ( flDistStart < 0.0f ) && ( flDistEnd > 0.0f ) ) 
		{ 
			flFrac = ( flDistStart + HEADCRABCANISTER_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if( flFrac < flExitFrac ) { flExitFrac = flFrac; }
		}

		if ( ( flDistStart > 0.0f ) && ( flDistEnd > 0.0f ) )
			return;

		// Positive Axis
		flDistStart = m_vecStartPosition[iAxis] - vecTriggerMaxs[iAxis];
		flDistEnd = vecEndPosition[iAxis] - vecTriggerMaxs[iAxis];

		if ( ( flDistStart > 0.0f ) && ( flDistEnd < 0.0f ) ) 
		{ 
			flFrac = ( flDistStart - HEADCRABCANISTER_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if ( flFrac > flEnterFrac ) { flEnterFrac = flFrac; }
		}

		if ( ( flDistStart < 0.0f ) && ( flDistEnd > 0.0f ) ) 
		{ 
			flFrac = ( flDistStart + HEADCRABCANISTER_TRIGGER_EPSILON ) / ( flDistStart - flDistEnd );
			if( flFrac < flExitFrac ) { flExitFrac = flFrac; }
		}

		if ( ( flDistStart > 0.0f ) && ( flDistEnd > 0.0f ) )
			return;
	}

	// Check for intersection.
	if ( flExitFrac >= flEnterFrac )
	{
		// Check to see if we start in the world or the skybox!
		if ( flEnterFrac == 0.0f )
		{
			m_nLocation = HEADCRABCANISTER_LOCATION_WORLD;
		}
		else
		{
			m_nLocation = HEADCRABCANISTER_LOCATION_SKYBOX;
		}

		// Calculate the enter/exit times.
		Vector vecEnterPoint, vecExitPoint, vecDeltaPosition;
		VectorSubtract( vecEndPosition, m_vecStartPosition, vecDeltaPosition );
		VectorScale( vecDeltaPosition, flEnterFrac, vecEnterPoint );
		VectorScale( vecDeltaPosition, flExitFrac, vecExitPoint );

		m_flWorldEnterTime = vecEnterPoint.Length() / m_flFlightSpeed;
		m_flWorldEnterTime += m_flLaunchTime;
	}
	*/

#undef HEADCRABCANISTER_TRIGGER_EPSILON
}

