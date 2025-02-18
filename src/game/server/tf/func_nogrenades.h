//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF NoGrenades Zone.
//
//=============================================================================//
#ifndef FUNC_NOGRENADES_ZONE_H
#define FUNC_NOGRENADES_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

#define NOGRENADE_SPRITE "sprites/light_glow02_noz.vmt"

//=============================================================================
//
// CTF NoGrenades Zone class.
//
class CNoGrenadesZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CNoGrenadesZone, CBaseTrigger );

	CNoGrenadesZone();

	void	Spawn( void );
	void	Precache( void );
	
	// Return true if the specified entity is touching this zone
	bool	IsTouching( const CBaseEntity *pEntity ) const OVERRIDE;

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

private:
	bool	m_bDisabled;
};

// Return true if the specified entity is in a NoGrenades zone
bool InNoGrenadeZone( CBaseEntity *pEntity );

#endif // FUNC_NOGRENADES_ZONE_H












