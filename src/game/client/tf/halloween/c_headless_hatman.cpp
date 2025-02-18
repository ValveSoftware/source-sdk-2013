//========= Copyright Valve Corporation, All rights reserved. ============//
// C_HeadlessHatman.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_headless_hatman.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_HeadlessHatman, DT_HeadlessHatman, CHeadlessHatman )
END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_HeadlessHatman::C_HeadlessHatman()
{
	m_ghostEffect = NULL;
	m_leftEyeEffect = NULL;
	m_rightEyeEffect = NULL;
}


//-----------------------------------------------------------------------------
C_HeadlessHatman::~C_HeadlessHatman()
{
	if ( m_ghostEffect )
	{
		ParticleProp()->StopEmission( m_ghostEffect );
		m_ghostEffect = NULL;
	}

	if ( m_leftEyeEffect )
	{
		ParticleProp()->StopEmission( m_leftEyeEffect );
		m_leftEyeEffect = NULL;
	}

	if ( m_rightEyeEffect )
	{
		ParticleProp()->StopEmission( m_rightEyeEffect );
		m_rightEyeEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_HeadlessHatman::Spawn( void )
{
	BaseClass::Spawn();

	m_vecViewOffset = Vector( 0, 0, 100.0f );

	SetNextClientThink( gpGlobals->curtime + 1.0f );
}


//-----------------------------------------------------------------------------
void C_HeadlessHatman::ClientThink( void )
{
	if ( !m_leftEyeEffect )
	{
		m_leftEyeEffect = ParticleProp()->Create( "halloween_boss_eye_glow", PATTACH_POINT_FOLLOW, "lefteye" );
	}

	if ( !m_rightEyeEffect )
	{
		m_rightEyeEffect = ParticleProp()->Create( "halloween_boss_eye_glow", PATTACH_POINT_FOLLOW, "righteye" );
	}

	if ( !m_ghostEffect )
	{
		m_ghostEffect = ParticleProp()->Create( "ghost_pumpkin", PATTACH_ABSORIGIN_FOLLOW );
	}

	SetNextClientThink( CLIENT_THINK_NEVER );
}


//-----------------------------------------------------------------------------
// Return the origin for player observers tracking this target
Vector C_HeadlessHatman::GetObserverCamOrigin( void ) 
{ 
	return EyePosition();
}


//-----------------------------------------------------------------------------
void C_HeadlessHatman::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		// footstep event
		EmitSound( "Halloween.HeadlessBossFootfalls" );

		ParticleProp()->Create( "halloween_boss_foot_impact", PATTACH_ABSORIGIN, 0 );
	}
}
