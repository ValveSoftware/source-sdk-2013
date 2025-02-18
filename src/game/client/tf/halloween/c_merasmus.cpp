//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_merasmus.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_Merasmus, DT_Merasmus, CMerasmus )
	RecvPropBool( RECVINFO( m_bRevealed ) ),
	RecvPropBool( RECVINFO( m_bIsDoingAOEAttack ) ),
	RecvPropBool( RECVINFO( m_bStunned ) ),
END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_Merasmus::C_Merasmus()
{
	m_ghostEffect = NULL;
	m_aoeEffect = NULL;
	m_stunEffect = NULL;
	m_bRevealed = false;
	m_bWasRevealed = false;
	m_bIsDoingAOEAttack = false;
	m_bStunned = false;
}


//-----------------------------------------------------------------------------
C_Merasmus::~C_Merasmus()
{
	if ( m_ghostEffect )
	{
		ParticleProp()->StopEmission( m_ghostEffect );
		m_ghostEffect = NULL;
	}

	if ( m_aoeEffect )
	{
		ParticleProp()->StopEmission( m_aoeEffect );
		m_aoeEffect = NULL;
	}

	if ( m_stunEffect )
	{
		ParticleProp()->StopEmission( m_stunEffect );
		m_stunEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_Merasmus::Spawn( void )
{
	BaseClass::Spawn();

	m_vecViewOffset = Vector( 0, 0, 100.0f );
}


//-----------------------------------------------------------------------------
// Return the origin for player observers tracking this target
Vector C_Merasmus::GetObserverCamOrigin( void ) 
{ 
	return EyePosition();
}


//-----------------------------------------------------------------------------
void C_Merasmus::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasRevealed = m_bRevealed;
}


//-----------------------------------------------------------------------------
void C_Merasmus::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bRevealed != m_bWasRevealed )
	{
		if ( m_bRevealed )
		{
			if ( !m_ghostEffect )
			{
				m_ghostEffect = ParticleProp()->Create( "merasmus_ambient_body", PATTACH_ABSORIGIN_FOLLOW );
			}
		}
		else
		{
			if ( m_ghostEffect )
			{
				ParticleProp()->StopEmission( m_ghostEffect );
				m_ghostEffect = NULL;
			}
		}
	}

	// book attack
	if ( m_bIsDoingAOEAttack )
	{
		if ( !m_aoeEffect )
		{
			m_aoeEffect = ParticleProp()->Create( "merasmus_book_attack", PATTACH_POINT_FOLLOW, LookupAttachment( "effect_hand_R" ), Vector( 16.f, 0.f, 0.f ) );
		}
	}
	else
	{
		if ( m_aoeEffect )
		{
			ParticleProp()->StopEmission( m_aoeEffect );
			m_aoeEffect = NULL;
		}
	}

	// stunned
	if ( m_bStunned )
	{
		if ( !m_stunEffect )
		{
			m_stunEffect = ParticleProp()->Create( "merasmus_dazed", PATTACH_POINT_FOLLOW, LookupAttachment( "head" ) );
		}
	}
	else
	{
		if ( m_stunEffect )
		{
			ParticleProp()->StopEmission( m_stunEffect );
			m_stunEffect = NULL;
		}
	}
}


int C_Merasmus::GetSkin()
{
	return ( m_bIsDoingAOEAttack || m_bStunned ) ? 0 : 1;
}
