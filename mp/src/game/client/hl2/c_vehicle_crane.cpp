//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "c_vehicle_crane.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int ScreenTransform( const Vector& point, Vector& screen );

IMPLEMENT_CLIENTCLASS_DT(C_PropCrane, DT_PropCrane, CPropCrane)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropBool( RECVINFO(m_bMagnetOn) ),
	RecvPropBool( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropBool( RECVINFO( m_bExitAnimOn ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropCrane )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()


#define ROLL_CURVE_ZERO		5		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	45		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between

#define CRANE_FOV	75

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_PropCrane::C_PropCrane( void )
{
	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );
	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.flFOV = CRANE_FOV;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropCrane::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropCrane::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	// Store off the old shadow direction
	if ( m_hPlayer && !m_hPrevPlayer )
	{
		m_vecOldShadowDir = g_pClientShadowMgr->GetShadowDirection();
		//Vector vecDown = m_vecOldShadowDir - Vector(0,0,0.5);
		//VectorNormalize( vecDown );
		Vector vecDown = Vector(0,0,-1);
		g_pClientShadowMgr->SetShadowDirection( vecDown );
	}
	else if ( !m_hPlayer && m_hPrevPlayer )
	{
		g_pClientShadowMgr->SetShadowDirection( m_vecOldShadowDir );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropCrane::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropCrane::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropCrane::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
{
	SharedVehicleViewSmoothing( m_hPlayer, 
								pAbsOrigin, pAbsAngles, 
								m_bEnterAnimOn, m_bExitAnimOn, 
								m_vecEyeExitEndpoint, 
								&m_ViewSmoothingData, 
								pFOV );
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropCrane::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------
void C_PropCrane::DrawHudElements( )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : theMins - 
//			theMaxs - 
//-----------------------------------------------------------------------------
void C_PropCrane::GetRenderBounds( Vector &theMins, Vector &theMaxs )
{
	// This is kind of hacky:( Add 660.0 to the y coordinate of the bounding box to
	// allow for the full extension of the crane arm.
	BaseClass::GetRenderBounds( theMins, theMaxs );
	theMaxs.y += 660.0f;
}

