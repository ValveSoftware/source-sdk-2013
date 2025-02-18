//========= Copyright Valve Corporation, All rights reserved. ============//
// c_bot_npc.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_bot_npc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_BotNPC, DT_BotNPC, CBotNPC )

	RecvPropEHandle( RECVINFO( m_laserTarget ) ),
	RecvPropBool( RECVINFO( m_isNuking ) ),

END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_BotNPC::C_BotNPC()
{
}


//-----------------------------------------------------------------------------
C_BotNPC::~C_BotNPC()
{
	if ( m_laserBeamEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_laserBeamEffect );
		m_laserBeamEffect = NULL;
	}

	if ( m_nukeEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_nukeEffect );
		m_nukeEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_BotNPC::Spawn( void )
{
	BaseClass::Spawn();

	m_vecViewOffset = Vector( 0, 0, 180.0f );

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


//-----------------------------------------------------------------------------
void C_BotNPC::ClientThink( void )
{
	if ( m_laserTarget )
	{
		if ( !m_laserBeamEffect )
		{
			m_laserBeamEffect = ParticleProp()->Create( "laser_sight_beam", PATTACH_POINT_FOLLOW, LookupAttachment( "head" ) );
		}

		if ( m_laserBeamEffect )
		{
			m_laserBeamEffect->SetSortOrigin( m_laserBeamEffect->GetRenderOrigin() );
			m_laserBeamEffect->SetControlPoint( 2, Vector( 0, 255, 0 ) );
			m_laserBeamEffect->SetControlPoint( 1, m_laserTarget->WorldSpaceCenter() );
		}
	}
	else
	{
		// shut off the laser
		if ( m_laserBeamEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_laserBeamEffect );
			m_laserBeamEffect = NULL;
		}
	}

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
Vector C_BotNPC::GetObserverCamOrigin( void ) 
{ 
	return EyePosition();
}


//-----------------------------------------------------------------------------
void C_BotNPC::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( event == 7001 )
	{
		EmitSound( "RobotBoss.Footstep" );

/*
		ParticleProp()->Create( "halloween_boss_foot_impact", PATTACH_ABSORIGIN, 0 );
*/
	}
}
