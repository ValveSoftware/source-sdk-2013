//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_TIMER_H
#define TF_TIMER_H
#ifdef _WIN32
#pragma once
#endif


#include "ehandle.h"
#include "tf_shareddefs.h"


class CTFPlayer;


class CTimer
{
public:
	CTimer();
	
	int GetTeamNumber() const;

public:
	EHANDLE m_hOwner;
	EHANDLE m_hEnemy;
	TFTimer_t m_Type; // One of the TF_TIMER_ defines.
	int m_iTeamNumber;
	float m_flNextThink;
	int weapon; // GI_RET_ define.
	
	// For g_Timers.
	int m_iListIndex;
};


// This stuff replaces the functions like CBaseEntity::FindTimer, CBaseEntity::CreateTimer,
// and all the timer handlers in TF.

// Find an active timer on the specified entity.
CTimer* Timer_FindTimer( CBaseEntity *pPlayer, TFTimer_t timerType );

// Create a new timer.
CTimer* Timer_CreateTimer( CBaseEntity *pPlayer, TFTimer_t timerType );

// Get rid of a timer.
void Timer_Remove( CTimer *pTimer );

// Update all timers.
void Timer_UpdateAll();

// Call at round restart.
void Timer_RemoveAll();


#endif // TF_TIMER_H
