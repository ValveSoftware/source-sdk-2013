//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hintmessage.h"
#include "hintsystem.h"

#ifdef GAME_DLL
	#include "util.h"
#endif

//--------------------------------------------------------------------------------------------------------
/**
* Simple utility function to allocate memory and duplicate a string
*/
inline char *CloneString( const char *str )
{
	char *cloneStr = new char [ strlen(str)+1 ];
	strcpy( cloneStr, str );
	return cloneStr;
}

extern int gmsgHudText;

enum { HMQ_SIZE = 8 };	// Maximum number of messages queue can hold
						// If the limit is reached, no more can be
						// added.

//--------------------------------------------------------------------------------------------------------------
CHintMessage::CHintMessage( const char * hintString, CUtlVector< const char * > * args, float duration )
{
	m_hintString = hintString;
	m_duration = duration;

	if ( args )
	{
		for ( int i=0; i<args->Count(); ++i )
		{
			m_args.AddToTail( CloneString( (*args)[i] ) );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
CHintMessage::~CHintMessage()
{
	for ( int i=0; i<m_args.Count(); ++i )
	{
		delete[] m_args[i];
	}
	m_args.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
bool CHintMessage::IsEquivalent( const char *hintString, CUtlVector< const char * > * args ) const
{
	if ( FStrEq( hintString, m_hintString ) )
	{
		if ( !args && !m_args.Count() )
		{
			return true;
		}

		if ( !args )
			return false;

		if ( args->Count() != m_args.Count() )
			return false;

		for ( int i=0; i<args->Count(); ++i )
		{
			if ( !FStrEq( (*args)[i], m_args[i] ) )
			{
				return false;
			}
		}
	}
	return false;
}

//--------------------------------------------------------------------------------------------------------------
void CHintMessage::Send( CBasePlayer * client )
{
	if ( !client )
		return;

#ifdef GAME_DLL
	// Custom hint text sending to allow for arguments.  This is OK because the client has a custom
	// message parser for hint text that can read the arguments.
	CSingleUserRecipientFilter user( (CBasePlayer *)client );
	user.MakeReliable();

	// client can handle 1 string only
	UserMessageBegin( user, "HintText" );
		WRITE_STRING( m_hintString );
	MessageEnd();
#endif
}

//--------------------------------------------------------------------------------------------------------------
CHintMessageQueue::CHintMessageQueue( CBasePlayer *pPlayer )
{
	m_pPlayer = pPlayer;
}

//--------------------------------------------------------------------------------------------------------------
void CHintMessageQueue::Reset()
{
	m_tmMessageEnd = 0;
	for ( int i=0; i<m_messages.Count(); ++i )
	{
		delete m_messages[i];
	}
	m_messages.RemoveAll();
}

//--------------------------------------------------------------------------------------------------------------
void CHintMessageQueue::Update()
{
	if ( !m_pPlayer )
		return;

	// test this - send the message as soon as it is ready, 
	// just stomp the old message
	//if ( gpGlobals->curtime > m_tmMessageEnd )
	{
		if ( m_messages.Count() )
		{
			CHintMessage *msg = m_messages[0];
			m_tmMessageEnd = gpGlobals->curtime + msg->GetDuration();
			msg->Send( m_pPlayer );
			delete msg;
			m_messages.Remove( 0 );
		}
	}
}

//--------------------------------------------------------------------------------------------------------------
bool CHintMessageQueue::AddMessage( const char* message, float duration, CUtlVector< const char * > * args )
{
	if ( !m_pPlayer )
		return false;

	for ( int i=0; i<m_messages.Count(); ++i )
	{
		// weed out duplicates
		if ( m_messages[i]->IsEquivalent( message, args ) )
		{
			return true;
		}
	}

	// 'message' is not copied, so the pointer must remain valid forever
	CHintMessage *msg = new CHintMessage( message, args, duration );
	m_messages.AddToTail( msg );
	return true;
}

//--------------------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
CHintMessageTimers::CHintMessageTimers( CHintSystem *pSystem, CHintMessageQueue *pQueue )
{
	m_pHintSystem = pSystem;
	m_pQueue = pQueue;
}

//-----------------------------------------------------------------------------
// Purpose: Clear out all registered timers
//-----------------------------------------------------------------------------
void CHintMessageTimers::Reset()
{
	for ( int i=0; i<m_Timers.Count(); ++i )
	{
		delete m_Timers[i];
	}
	m_Timers.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Check & fire any timers that should fire based on their duration.
//-----------------------------------------------------------------------------
void CHintMessageTimers::Update()
{
	if ( !m_pHintSystem )
		return;

	for ( int i = 0; i < m_Timers.Count(); i++ )
	{
		if ( m_Timers[i]->timer.Expired() )
		{
			if ( m_pHintSystem->TimerShouldFire( m_Timers[i]->iHintID ) )
			{
				//Warning("TIMER FIRED: %s\n", m_pszHintMessages[m_Timers[i]->iHintID] );

				m_pHintSystem->HintMessage( m_Timers[i]->iHintID );

				// Remove and return. No reason to bring up multiple hints.
				RemoveTimer( m_Timers[i]->iHintID );
				return;
			}
			else
			{
				// Push the timer out again
				m_Timers[i]->timer.Start();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Register a new timer that the system should should keep track of.
// Input  : iHintID - The ID of the hint message
//			timer_duration - the total time the timer should run for until it fires the hint message
//			message_duration - the duration passed into the hint message system when the hint fires
//			args - the arguments passed into the hint message system when the hint fires
//-----------------------------------------------------------------------------
void CHintMessageTimers::AddTimer( int iHintID, float timer_duration, float message_duration, CUtlVector< const char * > * args )
{
	if ( GetTimerIndex(iHintID) != m_Timers.InvalidIndex() )
		return;

	// 'message' is not copied, so the pointer must remain valid forever
	hintmessagetime_t *newTimer = new hintmessagetime_t( timer_duration );
	newTimer->iHintID = iHintID;
	newTimer->flMessageDuration = message_duration;
	if ( args )
	{
		for ( int i=0; i<args->Count(); ++i )
		{
			newTimer->args.AddToTail( CloneString( (*args)[i] ) );
		}
	}
	m_Timers.AddToTail( newTimer );

	//Warning("TIMER ADDED: %s\n", m_pszHintMessages[iHintID] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintMessageTimers::RemoveTimer( int iHintID )
{
	int iIndex = GetTimerIndex(iHintID);
	if ( iIndex != m_Timers.InvalidIndex() )
	{
		//Warning("TIMER REMOVED: %s\n", m_pszHintMessages[iHintID] );
		m_Timers.Remove( iIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintMessageTimers::StartTimer( int iHintID )
{
	int iIndex = GetTimerIndex(iHintID);
	if ( iIndex != m_Timers.InvalidIndex() )
	{
		//Warning("TIMER STARTED: %s\n", m_pszHintMessages[iHintID] );
		m_Timers[iIndex]->timer.Start();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintMessageTimers::StopTimer( int iHintID )
{
	int iIndex = GetTimerIndex(iHintID);
	if ( iIndex != m_Timers.InvalidIndex() )
	{
		//Warning("TIMER STOPPED: %s\n", m_pszHintMessages[iHintID] );
		m_Timers[iIndex]->timer.Stop();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return the index of the hint message in the timer list, if any
//-----------------------------------------------------------------------------
int CHintMessageTimers::GetTimerIndex( int iHintID )
{
	for ( int i = 0; i < m_Timers.Count(); i++ )
	{
		if ( m_Timers[i]->iHintID == iHintID )
			return i;
	}

	return m_Timers.InvalidIndex();
}
