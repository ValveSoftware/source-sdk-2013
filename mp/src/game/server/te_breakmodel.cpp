//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "basetempentity.h"
#include "vstdlib/random.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches model smash pieces
//-----------------------------------------------------------------------------
class CTEBreakModel : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEBreakModel, CBaseTempEntity );

					CTEBreakModel( const char *name );
	virtual			~CTEBreakModel( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles );
	
	virtual void	Precache( void );

	DECLARE_SERVERCLASS();

public:
	CNetworkVector( m_vecOrigin );
	CNetworkVector( m_vecSize );
	CNetworkVector( m_vecVelocity );
	CNetworkQAngle( m_angRotation );
	CNetworkVar( int, m_nRandomization );
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nCount );
	CNetworkVar( float, m_fTime );
	CNetworkVar( int, m_nFlags );
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBreakModel::CTEBreakModel( const char *name ) :
	CBaseTempEntity( name )
{
	m_vecOrigin.Init();
	m_vecSize.Init();
	m_vecVelocity.Init();
	m_angRotation.Init();
	m_nModelIndex		= 0;
	m_nRandomization	= 0;
	m_nCount			= 0;
	m_fTime				= 0.0;
	m_nFlags			= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBreakModel::~CTEBreakModel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTEBreakModel::Precache( void )
{
	CBaseEntity::PrecacheModel( "models/gibs/hgibs.mdl" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBreakModel::Test( const Vector& current_origin, const QAngle& current_angles )
{
	// Fill in data
	m_nModelIndex = CBaseEntity::PrecacheModel( "models/gibs/hgibs.mdl" );
	m_vecOrigin = current_origin;
	m_angRotation = current_angles;
	m_vecSize.Init( 16, 16, 16 );

	m_vecVelocity.Init( random->RandomFloat( -10, 10 ), random->RandomFloat( -10, 10 ), random->RandomFloat( 0, 20 ) );

	m_nRandomization = 100;
	m_nCount = 10;
	m_fTime = 5.0;
	m_nFlags = 0;
	
	Vector forward, right;

	m_vecOrigin += Vector( 0, 0, 24 );

	AngleVectors( current_angles, &forward, &right, 0 );
	forward[2] = 0.0;
	VectorNormalize( forward );

	VectorMA( m_vecOrigin, 50.0, forward, m_vecOrigin.GetForModify() );
	VectorMA( m_vecOrigin, 25.0, right, m_vecOrigin.GetForModify() );

	CBroadcastRecipientFilter filter;
	Create( filter, 0.0 );
}

IMPLEMENT_SERVERCLASS_ST(CTEBreakModel, DT_TEBreakModel)
	SendPropVector( SENDINFO(m_vecOrigin), -1, SPROP_COORD),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 13 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 13 ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 13 ),
	SendPropVector( SENDINFO(m_vecSize), -1, SPROP_COORD),
	SendPropVector( SENDINFO(m_vecVelocity), -1, SPROP_COORD),
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropInt( SENDINFO(m_nRandomization), 9, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nCount), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fTime), 10, 0, 0, 102.4 ),
	SendPropInt( SENDINFO(m_nFlags), 8, SPROP_UNSIGNED ),
END_SEND_TABLE()

// Singleton to fire TEBreakModel objects
static CTEBreakModel g_TEBreakModel( "breakmodel" );

void TE_BreakModel( IRecipientFilter& filter, float delay,
	const Vector& pos, const QAngle& angles, const Vector& size, const Vector& vel, int modelindex, int randomization,
	int count, float time, int flags )
{
	g_TEBreakModel.m_vecOrigin		= pos;
	g_TEBreakModel.m_angRotation	= angles;
	g_TEBreakModel.m_vecSize		= size;
	g_TEBreakModel.m_vecVelocity	= vel;
	g_TEBreakModel.m_nModelIndex	= modelindex;	
	g_TEBreakModel.m_nRandomization	= randomization;
	g_TEBreakModel.m_nCount			= count;
	g_TEBreakModel.m_fTime			= time;
	g_TEBreakModel.m_nFlags			= flags;

	// Send it over the wire
	g_TEBreakModel.Create( filter, delay );
}