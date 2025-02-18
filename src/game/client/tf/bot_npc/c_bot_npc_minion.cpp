//========= Copyright Valve Corporation, All rights reserved. ============//
// c_bot_npc_minion.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_bot_npc_minion.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_BotNPCMinion, DT_BotNPCMinion, CBotNPCMinion )

	RecvPropEHandle( RECVINFO( m_stunTarget ) ),

END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_BotNPCMinion::C_BotNPCMinion()
{
	m_stunEffect = NULL;
	m_stunBeamEffect = NULL;
	m_scanEffect = NULL;
}


//-----------------------------------------------------------------------------
C_BotNPCMinion::~C_BotNPCMinion()
{
	if ( m_stunEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_stunEffect );
		m_stunEffect = NULL;
	}

	if ( m_stunBeamEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_stunBeamEffect );
		m_stunBeamEffect = NULL;
	}

	if ( m_scanEffect )
	{
		ParticleProp()->StopEmissionAndDestroyImmediately( m_scanEffect );
		m_scanEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_BotNPCMinion::Spawn( void )
{
	BaseClass::Spawn();

	SetNextClientThink( CLIENT_THINK_ALWAYS );

	//m_scanEffect = ParticleProp()->Create( "minion_scan", PATTACH_ABSORIGIN_FOLLOW );
}


//-----------------------------------------------------------------------------
void C_BotNPCMinion::ClientThink( void )
{
	if ( m_stunTarget )
	{
		if ( !m_stunEffect )
		{
			m_stunEffect = ParticleProp()->Create( "cart_flashinglight", PATTACH_ABSORIGIN_FOLLOW );
		}

		if ( !m_stunBeamEffect )
		{
			m_stunBeamEffect = ParticleProp()->Create( "laser_sight_beam", PATTACH_ABSORIGIN_FOLLOW );
		}

		if ( m_stunBeamEffect )
		{
			m_stunBeamEffect->SetSortOrigin( m_stunBeamEffect->GetRenderOrigin() );
			m_stunBeamEffect->SetControlPoint( 2, Vector( 255, 0, 255 ) );
			m_stunBeamEffect->SetControlPoint( 1, m_stunTarget->WorldSpaceCenter() );
		}
	}
	else
	{
		// shut off the effect
		if ( m_stunEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_stunEffect );
			m_stunEffect = NULL;
		}

		if ( m_stunBeamEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_stunBeamEffect );
			m_stunBeamEffect = NULL;
		}
	}
}


//-----------------------------------------------------------------------------
// Return the origin for player observers tracking this target
Vector C_BotNPCMinion::GetObserverCamOrigin( void ) 
{ 
	return GetAbsOrigin();
}


//-----------------------------------------------------------------------------
void C_BotNPCMinion::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
}
