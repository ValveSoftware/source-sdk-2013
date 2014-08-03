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
#include "te_basebeam.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBaseBeam::CTEBaseBeam( const char *name ) :
  CBaseTempEntity( name )
{
	m_nModelIndex	= 0;
	m_nHaloIndex	= 0;
	m_nStartFrame	= 0;
	m_nFrameRate	= 0;
	m_fLife			= 0.0;
	m_fWidth		= 0;
	m_fEndWidth		= 0;
	m_nFadeLength	= 0;
	m_fAmplitude	= 0;
	r = g = b = a = 0;
	m_nSpeed		= 0;
	m_nFlags		= 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBaseBeam::~CTEBaseBeam( void )
{
}



IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEBaseBeam, DT_BaseBeam )
	SendPropModelIndex( SENDINFO(m_nModelIndex) ),
	SendPropModelIndex( SENDINFO(m_nHaloIndex) ),
	SendPropInt( SENDINFO(m_nStartFrame), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nFrameRate), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fLife), 8, 0, 0.0, 25.6 ),
	SendPropFloat( SENDINFO(m_fWidth), 10, 0, 0.0, 128.0 ),
	SendPropFloat( SENDINFO(m_fEndWidth), 10, 0, 0.0, 128.0 ),
	SendPropInt( SENDINFO(m_nFadeLength), 8, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_fAmplitude), 8, 0, 0.0, 64.0 ),
	SendPropInt( SENDINFO(m_nSpeed), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(r), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(g), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(b), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(a), 8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_nFlags), 32, SPROP_UNSIGNED ),
END_SEND_TABLE()

