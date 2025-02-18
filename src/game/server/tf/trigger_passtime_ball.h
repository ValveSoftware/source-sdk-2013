//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TRIGGER_PASSTIME_BALL_H
#define TRIGGER_PASSTIME_BALL_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"

//-----------------------------------------------------------------------------
class CTriggerPasstimeBall : public CBaseEntity
{
public:
	DECLARE_CLASS( CTriggerPasstimeBall, CBaseEntity );
	DECLARE_DATADESC();
	virtual void Spawn() OVERRIDE;

private:
	void Update();
	bool BTouching( CBaseEntity *pEnt );

	bool m_bPresent;
	COutputEvent m_onBallEnter;
	COutputEvent m_onBallExit;
};

#endif // TRIGGER_PASSTIME_BALL_H 
