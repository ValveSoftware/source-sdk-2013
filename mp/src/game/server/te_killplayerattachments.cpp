//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches blood stream tempentity
//-----------------------------------------------------------------------------
class CTEKillPlayerAttachments : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEKillPlayerAttachments, CBaseTempEntity );

					CTEKillPlayerAttachments( const char *name );
	virtual			~CTEKillPlayerAttachments( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nPlayer );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEKillPlayerAttachments::CTEKillPlayerAttachments( const char *name ) :
	CBaseTempEntity( name )
{
	m_nPlayer = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEKillPlayerAttachments::~CTEKillPlayerAttachments( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEKillPlayerAttachments::Test( const Vector& current_origin, const QAngle& current_angles )
{
	m_nPlayer = 1;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}


IMPLEMENT_SERVERCLASS_ST(CTEKillPlayerAttachments, DT_TEKillPlayerAttachments)
	SendPropInt( SENDINFO(m_nPlayer), 5, SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEKillPlayerAttachments objects
static CTEKillPlayerAttachments g_TEKillPlayerAttachments( "KillPlayerAttachments" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			player - 
//-----------------------------------------------------------------------------
void TE_KillPlayerAttachments( IRecipientFilter& filter, float delay,
	int player )
{
	g_TEKillPlayerAttachments.m_nPlayer = player;

	// Send it over the wire
	g_TEKillPlayerAttachments.Create( filter, delay );
}