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
// Purpose: Dispatches decal tempentity
//-----------------------------------------------------------------------------
class CTEPlayerDecal : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerDecal, CBaseTempEntity );

					CTEPlayerDecal( const char *name );
	virtual			~CTEPlayerDecal( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	DECLARE_SERVERCLASS();

public:
	CNetworkVar( int, m_nPlayer );
	CNetworkVector( m_vecOrigin );
	CNetworkVar( int, m_nEntity );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEPlayerDecal::CTEPlayerDecal( const char *name ) :
	CBaseTempEntity( name )
{
	m_nPlayer = 0;
	m_vecOrigin.Init();
	m_nEntity = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEPlayerDecal::~CTEPlayerDecal( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEPlayerDecal::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nPlayer = 1;
	m_nEntity = 0;
	m_vecOrigin = current_origin;

	Vector vecEnd;
	
	Vector forward;

	m_vecOrigin.GetForModify()[2] += 24;

	AngleVectors( current_angles, &forward );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 1024.0, forward, vecEnd );

	trace_t tr;

	UTIL_TraceLine( m_vecOrigin, vecEnd, MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &tr );

	m_vecOrigin = tr.endpos;

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEPlayerDecal, DT_TEPlayerDecal)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropInt( SENDINFO(m_nEntity), MAX_EDICT_BITS, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nPlayer), Q_log2( MAX_PLAYERS ), SPROP_UNSIGNED ),
END_SEND_TABLE()


// Singleton to fire TEPlayerDecal objects
static CTEPlayerDecal g_TEPlayerDecal( "Player Decal" );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			player - 
//			entity - 
//			index - 
//-----------------------------------------------------------------------------
void TE_PlayerDecal( IRecipientFilter& filter, float delay,
	const Vector* pos, int player, int entity )
{
	g_TEPlayerDecal.m_vecOrigin		= *pos;
	g_TEPlayerDecal.m_nPlayer		= player;
	g_TEPlayerDecal.m_nEntity		= entity;	

	// Send it over the wire
	g_TEPlayerDecal.Create( filter, delay );
}