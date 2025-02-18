//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Achievement Zone.
//
//=============================================================================//
#ifndef FUNC_ACHIEVEMENT_ZONE_H
#define FUNC_ACHIEVEMENT_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//=============================================================================
//
// CTF Achievement Zone class.
//
class CAchievementZone : public CBaseTrigger
{
	DECLARE_CLASS( CAchievementZone, CBaseTrigger );

public:
	DECLARE_DATADESC();

	CAchievementZone();

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

	int		GetZoneID( void ){ return m_iZoneID; }

private:
	bool	m_bDisabled;
	int		m_iZoneID;
};

// Return true if the specified entity is in an Achievement zone
CAchievementZone *InAchievementZone( CBaseEntity *pEntity );

#endif // FUNC_ACHIEVEMENT_ZONE_H












