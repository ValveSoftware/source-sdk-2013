//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Dynamic light at the end of a spotlight 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	SPOTLIGHTEND_H
#define	SPOTLIGHTEND_H

#ifdef _WIN32
#pragma once
#endif


#include "baseentity.h"

class CSpotlightEnd : public CBaseEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CSpotlightEnd, CBaseEntity );

	void				Spawn( void );

	int					ObjectCaps( void )
	{
		// Don't save and don't go across transitions
		return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; 
	}

	DECLARE_SERVERCLASS();

public:
	CNetworkVar( float, m_flLightScale );
	CNetworkVar( float, m_Radius );
//	CNetworkVector( m_vSpotlightDir );
//	CNetworkVector( m_vSpotlightOrg );
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightOrg;
};

#endif	//SPOTLIGHTEND_H


