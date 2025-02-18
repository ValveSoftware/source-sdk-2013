//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_timer.h"


static CUtlLinkedList<CTimer*,int> g_Timers;


// ------------------------------------------------------------------------------------------ //
// CTimer functions.
// ------------------------------------------------------------------------------------------ //

CTimer::CTimer()
{
	m_iTeamNumber = 0;
	m_flNextThink = 0;
}


int CTimer::GetTeamNumber() const
{
	return m_iTeamNumber;
}


// ------------------------------------------------------------------------------------------ //
// Global timer functions.
// ------------------------------------------------------------------------------------------ //

CTimer* Timer_FindTimer( CBaseEntity *pPlayer, TFTimer_t timerType )
{
	FOR_EACH_LL( g_Timers, i )
	{
		CTimer *pTimer = g_Timers[i];

		if ( pTimer->m_hOwner == pPlayer )
		{
			if ( timerType == TF_TIMER_ANY || pTimer->m_Type == timerType )
				return pTimer;
		}
	}
	return NULL;
}


CTimer* Timer_CreateTimer( CBaseEntity *pPlayer, TFTimer_t timerType )
{
	Assert( !Timer_FindTimer( pPlayer, timerType ) );

	CTimer *pTimer = new CTimer;
	pTimer->m_hOwner = pPlayer;
	pTimer->m_Type = timerType;
	pTimer->m_iListIndex = g_Timers.AddToTail( pTimer );
	
	// TFTODO: Register the think functions here..
	if ( pTimer->m_Type == TF_TIMER_ROTHEALTH )
	{
		pTimer->m_flNextThink = gpGlobals->curtime + 5;
	}
	else if ( pTimer->m_Type == TF_TIMER_INFECTION )
	{
		pTimer->m_flNextThink = gpGlobals->curtime + 2;
	}

	// TFTODO: hook up thinks...
	// TF_TIMER_RETURNITEM -> CBaseEntity::ReturnItem  -> <up to caller>
	// TF_TIMER_ENDROUND   -> CBaseEntity::EndRoundEnd -> <up to caller>

	return pTimer;
}


void Timer_Remove( CTimer *pTimer )
{
	g_Timers.Remove( pTimer->m_iListIndex );
	delete pTimer;
}


void Timer_UpdateAll()
{
	int iNext = 0;
	int i = g_Timers.Head();
	while ( i != g_Timers.InvalidIndex() )
	{
		iNext = g_Timers.Next( i );
		CTimer *pTimer = g_Timers[i];
		i = iNext;

		// Get rid of invalid timers.
		if ( pTimer->m_hOwner.Get() == NULL )
		{
			g_Timers.Remove( i );
			delete pTimer;
		}
		else
		{
			// Is it time to think for this timer?
			if ( gpGlobals->curtime >= pTimer->m_flNextThink )
			{
				// TFTODO: think here.
			}
		}
	}
}


void Timer_RemoveAll()
{
	g_Timers.PurgeAndDeleteElements();
}

