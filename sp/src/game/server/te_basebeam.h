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

//-----------------------------------------------------------------------------
// Purpose: Dispatches a beam ring between two entities
//-----------------------------------------------------------------------------
#if !defined( TE_BASEBEAM_H )
#define TE_BASEBEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "basetempentity.h"

abstract_class CTEBaseBeam : public CBaseTempEntity
{
public:

	DECLARE_CLASS( CTEBaseBeam, CBaseTempEntity );
	DECLARE_SERVERCLASS();


public:
					CTEBaseBeam( const char *name );
	virtual			~CTEBaseBeam( void );

	virtual void	Test( const Vector& current_origin, const QAngle& current_angles ) = 0;
	
public:
	CNetworkVar( int, m_nModelIndex );
	CNetworkVar( int, m_nHaloIndex );
	CNetworkVar( int, m_nStartFrame );
	CNetworkVar( int, m_nFrameRate );
	CNetworkVar( float, m_fLife );
	CNetworkVar( float, m_fWidth );
	CNetworkVar( float, m_fEndWidth );
	CNetworkVar( int, m_nFadeLength );
	CNetworkVar( float, m_fAmplitude );
	CNetworkVar( int, r );
	CNetworkVar( int, g );
	CNetworkVar( int, b );
	CNetworkVar( int, a );
	CNetworkVar( int, m_nSpeed );
	CNetworkVar( int, m_nFlags );
};

EXTERN_SEND_TABLE(DT_BaseBeam);

#endif // TE_BASEBEAM_H