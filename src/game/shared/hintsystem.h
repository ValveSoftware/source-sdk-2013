//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A class embedded in players to provide hints to that player
//
//=============================================================================

#ifndef HINTSYSTEM_H
#define HINTSYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_baseplayer.h"
#else
	#include "player.h"
#endif

#include "bitvec.h"

class CHintMessageQueue;
class CHintMessageTimers;

typedef bool (*HintTimerCallback)( CBasePlayer *pOnPlayer );

//-----------------------------------------------------------------------------
// Purpose: A class embedded in players to provide hints to that player
//-----------------------------------------------------------------------------
class CHintSystem
{
	DECLARE_CLASS_NOBASE( CHintSystem );

public:
	CHintSystem();
	~CHintSystem();

	//-----------------------------------------------------
	// Call this from your player constructor
	void Init( CBasePlayer *pPlayer, int iMaxHintTypes, const char **pszHintStrings );

	//-----------------------------------------------------
	// CBasePlayer calls these for you, if you fall back to its 
	// versions of Spawn(), Event_Killed(), and PreThink().
	// Call this when your player respawns
	void				ResetHints( void );

	// Call this when your player dies
	void				ResetHintTimers( void );

	// Call this when in your player PreThink()
	void				Update( void );

	//-----------------------------------------------------
	// Hint addition
	// Call these to add a hint directly onscreen
	bool 				HintMessage( int hint, bool bForce = false, bool bOnlyIfClear = false );
	void 				HintMessage( const char *pMessage );

	// Call this to add a hint timer. It'll be reset for you automatically 
	// everytime ResetHintTimers() is called.
	void				RegisterHintTimer( int iHintID, float flTimerDuration, bool bOnlyIfClear = false, HintTimerCallback pfnCallback = NULL );

	// Call these to start & stop registered hint timers
	void				StartHintTimer( int iHintID );
	void				StopHintTimer( int iHintID );
	void				RemoveHintTimer( int iHintID );
	bool				TimerShouldFire( int iHintID );

	// Set whether a player should see any hints at all
	void				SetShowHints( bool bShowHints ) { m_bShowHints = bShowHints; }
	void				SetHintPlayed( int iHintID );
	bool				ShouldShowHints( void );

	// Returns true if the hint has been played already
	bool				HasPlayedHint( int iHintID );
	void				PlayedAHint( void );
	void				ClearHintHistory( void ) { m_HintHistory.ClearAll(); }

	// Not really an optimal solution, but saves us querying the hud element,
	// which wouldn't be easy with derived versions in different mods.
	bool				HintIsCurrentlyVisible( void ) { return (gpGlobals->curtime - m_flLastHintPlayedAt < 11 ); }

private:
	void				ReAddHintTimerIfNotDisplayed( int iHintID, float flTimerDuration );

private:
	CBasePlayer			*m_pPlayer;

	float				m_flLastHintPlayedAt;
	bool				m_bShowHints;
	CVarBitVec			m_HintHistory;
	const char			**m_pszHintMessages;
	CHintMessageQueue	*m_pHintMessageQueue;
	CHintMessageTimers	*m_pHintMessageTimers;

	struct onresethints_t
	{
		int					iHintID;
		float				flTimer;
		bool				bOnlyIfClear;
		HintTimerCallback	pfnCallback;
	};
	CUtlVector<onresethints_t>	m_RegisteredResetHints;
};

#ifdef CLIENT_DLL
// Derive from this if you have an entity that wants to display a hint
// when the player waves his target ID over it on the client.
abstract_class ITargetIDProvidesHint
{
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer ) = 0;
};
#endif

#endif // HINTSYSTEM_H
