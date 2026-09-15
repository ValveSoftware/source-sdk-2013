//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "matsys_controls/manipulator.h"

#include "materialsystem/imaterialsystem.h"

#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "vgui/ISystem.h"
#include "vgui/MouseCode.h"

#include "mathlib/vector.h"
#include "mathlib/vmatrix.h"
#include "mathlib/mathlib.h"

#include <float.h>

using namespace vgui;


//-----------------------------------------------------------------------------
// local helper functions
//-----------------------------------------------------------------------------
static float UpdateTime( float &flLastTime )
{
	float flTime = vgui::system()->GetFrameTime();
	float dt = flTime - flLastTime;
	flLastTime = flTime;
	return dt;
}


//-----------------------------------------------------------------------------
// Base class for manipulators which operate on transforms
//-----------------------------------------------------------------------------
CTransformManipulator::CTransformManipulator( matrix3x4_t *pTransform ) :
	m_pTransform( pTransform )
{
}

void CTransformManipulator::SetTransform( matrix3x4_t *pTransform )
{
	m_pTransform = pTransform;
}

matrix3x4_t *CTransformManipulator::GetTransform( void )
{
	return m_pTransform;
}


//-----------------------------------------------------------------------------
// CPotteryWheelManip - nendo-style camera manipulator
//-----------------------------------------------------------------------------
CPotteryWheelManip::CPotteryWheelManip( matrix3x4_t *pTransform ) :
	CTransformManipulator( pTransform ),
	m_lastx( -1 ), m_lasty( -1 ),
	m_zoom( 100.0f ), m_altitude( 0.0f ), m_azimuth( 0.0f ),
	m_prevZoom( 100.0f ), m_prevAltitude( 0.0f ), m_prevAzimuth( 0.0f ),
	m_flLastMouseTime( 0.0f ), m_flLastTickTime( 0.0f ),
	m_flSpin( 0.0f ), m_bSpin( false )
{
}

void CPotteryWheelManip::OnBeginManipulation( void )
{
	m_prevZoom = m_zoom;
	m_prevAltitude = m_altitude;
	m_prevAzimuth = m_azimuth;
	m_flLastMouseTime = m_flLastTickTime = vgui::system()->GetFrameTime();
	m_flSpin = 0.0f;
	m_bSpin = false;
}

// Sets the zoom level
void CPotteryWheelManip::SetZoom( float flZoom )
{
	m_prevZoom = m_zoom = flZoom;
}


void CPotteryWheelManip::OnAcceptManipulation( void )
{
	m_flSpin = 0.0f;
	m_bSpin = false;
}

void CPotteryWheelManip::OnCancelManipulation( void )
{
	m_zoom = m_prevZoom;
	m_altitude = m_prevAltitude;
	m_azimuth = m_prevAzimuth;
	m_flSpin = 0.0f;
	m_bSpin = false;
	UpdateTransform();
}


void CPotteryWheelManip::OnTick( void )
{
	float dt = UpdateTime( m_flLastTickTime );

	if ( m_bSpin )
	{
		m_azimuth += dt * m_flSpin;
		UpdateTransform();
	}
}

void CPotteryWheelManip::OnCursorMoved( int x, int y )
{
	float dt = UpdateTime( m_flLastMouseTime );

	if ( m_bSpin )
	{
		m_lastx = x;
		m_lasty = y;
		return;
	}

	if ( input()->IsMouseDown( MOUSE_MIDDLE ) )
	{
		int dy = y - m_lasty;
		int dx = x - m_lastx;

		if ( abs( dx ) < 2 * abs( dy ) )
		{
			UpdateZoom( 0.2f * dy );
		}
		else
		{
			m_flSpin = (dt != 0.0f) ? 0.002f * dx / dt : 0.0f;
			m_azimuth  += 0.002f * dx;
		}
	}
	else
	{
		m_azimuth  += 0.002f * ( x - m_lastx );
		m_altitude -= 0.002f * ( y - m_lasty );
		m_altitude = max( (float)-M_PI/2, min( (float)M_PI/2, m_altitude ) );
	}
	m_lastx = x;
	m_lasty = y;

	UpdateTransform();
}

void CPotteryWheelManip::OnMousePressed( vgui::MouseCode code, int x, int y )
{
	UpdateTime( m_flLastMouseTime );
	m_lastx = x;
	m_lasty = y;
	m_bSpin = false;
	m_flSpin = 0.0f;
}

void CPotteryWheelManip::OnMouseReleased( vgui::MouseCode code, int x, int y )
{
	UpdateTime( m_flLastMouseTime );

	if ( code == MOUSE_MIDDLE )
	{
		m_bSpin = ( fabs( m_flSpin ) > 1.0f );
	}

	m_lastx = x;
	m_lasty = y;
}

void CPotteryWheelManip::OnMouseWheeled( int delta )
{
	UpdateTime( m_flLastMouseTime );

	UpdateZoom( -10.0f * delta );
	UpdateTransform();
}

void CPotteryWheelManip::UpdateTransform()
{
	if ( !m_pTransform )
		return;

	float y = m_zoom * sin( m_altitude );
	float xz = m_zoom * cos( m_altitude );
	float x = xz * sin( m_azimuth );
	float z = xz * cos( m_azimuth );

	Vector position(x, y, z);
	AngleMatrix( RadianEuler( -m_altitude, m_azimuth, 0 ), position, *m_pTransform );
}


void CPotteryWheelManip::UpdateZoom( float delta )
{
	m_zoom *= pow( 1.01f, delta );
}
