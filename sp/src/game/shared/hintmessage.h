//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef HINTMESSAGE_H
#define HINTMESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "simtimer.h"

#ifdef GAME_DLL
	#include "player.h"
#else
	#include "c_baseplayer.h"
#endif

class CHintSystem;

//--------------------------------------------------------------------------------------------------------------
class CHintMessage
{
public:
	CHintMessage( const char * hintString, CUtlVector< const char * > * args, float duration );
	~CHintMessage();

	float GetDuration() const { return m_duration; }
	void Send( CBasePlayer *client );

	bool IsEquivalent( const char *hintString, CUtlVector< const char * > * args ) const;

private:
	const char * m_hintString;					///< hintString is a pointer to a string that should never be deleted.
	CUtlVector< char * > m_args;				///< list of arguments.  The memory for these strings is internal to the CHintMessage.
	float m_duration;							///< time until the next message can be displayed
};


//--------------------------------------------------------------------------------------------------------------
class CHintMessageQueue
{
public:
	CHintMessageQueue( CBasePlayer *pPlayer );
	void		Reset();
	void		Update();
	bool		AddMessage( const char* message, float duration = 6.0f, CUtlVector< const char * > * args = NULL );
	inline bool IsEmpty() { return m_messages.Count() == 0; }

private:
	float		m_tmMessageEnd;
	CUtlVector< CHintMessage * > m_messages;
	CBasePlayer *m_pPlayer;
};

//--------------------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------------------
// Timers that manage hint messages that should be displayed after some time.
class CHintMessageTimers
{
public:
	CHintMessageTimers( void );
	CHintMessageTimers( CHintSystem *pSystem, CHintMessageQueue *pQueue );

	void		Reset();
	void		Update();

	// Add / Register timers that will be started/stopped during play
	void		AddTimer( int iHintID, float timer_duration, float message_duration = 6.0f, CUtlVector< const char * > * args = NULL );
	void		RemoveTimer( int iHintID );

	// Start / Stop timers that were previously registered via AddTimer()
	void		StartTimer( int iHintID );
	void		StopTimer( int iHintID );

private:
	int			GetTimerIndex( int iHintID );

private:
	struct hintmessagetime_t
	{
		hintmessagetime_t( float flTimerDuration ) :
			timer(flTimerDuration)
		{
			iHintID = 0;
			flMessageDuration = 6.0;
		}

		~hintmessagetime_t()
		{
			for ( int i=0; i<args.Count(); ++i )
			{
				delete[] args[i];
			}
			args.RemoveAll();
		}

		int						iHintID;
		CStopwatch				timer;
		float					flMessageDuration;
		CUtlVector< char * >	args;
	};

	CUtlVector< hintmessagetime_t* >  m_Timers;
	CHintMessageQueue				  *m_pQueue;
	CHintSystem						  *m_pHintSystem;
};

#endif // HINTMESSAGE_H
