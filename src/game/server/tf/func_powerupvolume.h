//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Powerup Volume.
//
//=============================================================================//
#ifndef FUNC_POWERUP_VOLUME_H
#define FUNC_POWERUP_VOLUME_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

DECLARE_AUTO_LIST( IFuncPowerupVolumeAutoList );

//=============================================================================
//
// CTF Powerup Volume class.
//
class CPowerupVolume : public CTriggerMultiple, public IFuncPowerupVolumeAutoList
{
public:
	DECLARE_CLASS( CPowerupVolume, CTriggerMultiple );

	CPowerupVolume();

	void	Spawn( void );
	void	Precache( void );
	void	Touch( CBaseEntity *pOther );

 	bool	IsDisabled( void );
 	void	SetDisabled( bool bDisabled );
	int		GetNumTimesUsed( void ) { return m_nNumberOfTimesUsed; }
	void	SetNumTimesUsed( int NumberOfTimesUsed ) { m_nNumberOfTimesUsed = NumberOfTimesUsed; }

	int		m_nNumberOfTimesUsed;

private:
	bool	m_bDisabled;
};
#endif // FUNC_POWERUP_VOLUME_H












