//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "c_entity_currencypack.h"
#include "c_tf_player.h"

IMPLEMENT_CLIENTCLASS_DT( C_CurrencyPack, DT_CurrencyPack, CCurrencyPack )
	RecvPropBool( RECVINFO( m_bDistributed ) ),
END_RECV_TABLE()

C_CurrencyPack::C_CurrencyPack()
{
	m_bDistributed = false;

	m_pGlowEffect = NULL;
	m_bShouldGlowForLocalPlayer = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_CurrencyPack::~C_CurrencyPack()
{
	DestroyGlowEffect();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::ClientThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect if we have a cart
	if ( m_bShouldGlowForLocalPlayer )
	{
		Vector color = m_bDistributed ? Vector( 150, 0, 0 ) : Vector( 0, 150, 0 );
		m_pGlowEffect = new CGlowObject( this, color, 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_CurrencyPack::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
