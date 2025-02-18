//========= Copyright Valve Corporation, All rights reserved. ============//
// c_boss_alpha.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_boss_alpha.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_BossAlpha, DT_BossAlpha, CBossAlpha )

	RecvPropBool( RECVINFO( m_isNuking ) ),

END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_BossAlpha::C_BossAlpha()
{
}


//-----------------------------------------------------------------------------
C_BossAlpha::~C_BossAlpha()
{
	if ( m_nukeEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_nukeEffect );
		m_nukeEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_BossAlpha::Spawn( void )
{
	BaseClass::Spawn();

	m_vecViewOffset = Vector( 0, 0, 180.0f );

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


//-----------------------------------------------------------------------------
void C_BossAlpha::ClientThink( void )
{
	if ( m_isNuking )
	{
		if ( !m_nukeEffect )
		{
			m_nukeEffect = ParticleProp()->Create( "charge_up", PATTACH_POINT_FOLLOW, LookupAttachment( "head" ) );
		}
	}
	else
	{
		if ( m_nukeEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_nukeEffect );
			m_nukeEffect = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Return the origin for player observers tracking this target
Vector C_BossAlpha::GetObserverCamOrigin( void ) 
{ 
	return EyePosition();
}


//-----------------------------------------------------------------------------
void C_BossAlpha::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		EmitSound( "RobotBoss.Footstep" );
	}
}
