//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_FLAG_ALERT_H
#define FUNC_FLAG_ALERT_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "tf_shareddefs.h"

//-----------------------------------------------------------------------------
// Purpose: Designates an area that triggers an alert when a player carrying a flag starts touching the area
//-----------------------------------------------------------------------------
class CFuncFlagAlertZone : public CBaseTrigger
{
	DECLARE_CLASS( CFuncFlagAlertZone, CBaseTrigger );

public:
	DECLARE_DATADESC();

	CFuncFlagAlertZone();

	void	Spawn( void );
	void	StartTouch( CBaseEntity *pOther );

private:
	float			m_flNextAlertTime[TF_TEAM_COUNT];
	bool			m_bPlaySound;
	int				m_nAlertDelay;
	COutputEvent	m_OnTriggeredByTeam1;
	COutputEvent	m_OnTriggeredByTeam2;
};

#endif // FUNC_FLAG_ALERT_H
