//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF ChangeClass Zone.
//
//=============================================================================//
#ifndef FUNC_CHANGECLASS_ZONE_H
#define FUNC_CHANGECLASS_ZONE_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

//=============================================================================
//
// CTF Regenerate Zone class.
//
class CChangeClassZone : public CBaseTrigger
{
public:
	DECLARE_CLASS( CChangeClassZone, CBaseTrigger );

	CChangeClassZone();

	void	Spawn( void );
	void	Precache( void );
	void	Touch( CBaseEntity *pOther );
	void	EndTouch( CBaseEntity *pOther );

	bool	IsDisabled( void );
	void	SetDisabled( bool bDisabled );

	// Input handlers
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

private:
	bool	m_bDisabled;
};

#endif // FUNC_CHANGECLASS_ZONE_H












