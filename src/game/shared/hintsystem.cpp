//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "hintsystem.h"
#include "hintmessage.h"

#ifdef GAME_DLL
#else
	#include <igameevents.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
ConVar cl_showhelp( "cl_showhelp", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "Set to 0 to not show on-screen help" );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHintSystem::CHintSystem( void )
{
	Init( NULL, 0, NULL );
	m_pHintMessageQueue = NULL;
	m_pHintMessageTimers = NULL;
	m_flLastHintPlayedAt = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHintSystem::~CHintSystem( void )
{
	if ( m_pHintMessageTimers )
	{
		delete m_pHintMessageTimers;
		m_pHintMessageTimers = NULL;
	}

	if ( m_pHintMessageQueue )
	{
		delete m_pHintMessageQueue;
		m_pHintMessageQueue = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::Init( CBasePlayer *pPlayer, int iMaxHintTypes, const char **pszHintStrings )
{
	m_pPlayer = pPlayer;	
	m_bShowHints = true;

	m_HintHistory.Resize( iMaxHintTypes );
	m_HintHistory.ClearAll();

	m_pszHintMessages = pszHintStrings;

	if ( m_pPlayer )
	{
		m_pHintMessageQueue = new CHintMessageQueue( m_pPlayer );
		m_pHintMessageTimers = new CHintMessageTimers( this, m_pHintMessageQueue );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::Update( void )
{
	if ( m_pHintMessageQueue )
	{
		m_pHintMessageQueue->Update();
	}
	if ( m_pHintMessageTimers )
	{
		m_pHintMessageTimers->Update();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Displays a hint message to the player
// Input  : hint - enum'd hint to show
//			bForce - always play this hint even if they have seen it before
//-----------------------------------------------------------------------------
bool CHintSystem::HintMessage( int hint, bool bForce /* = false */, bool bOnlyIfClear /* = false */ )
{
	Assert( m_pPlayer );
	Assert( hint < m_HintHistory.GetNumBits() );

	// Not really an optimal solution, but saves us querying the hud element,
	// which wouldn't be easy with derived versions in different mods.
	if ( bOnlyIfClear && (gpGlobals->curtime - m_flLastHintPlayedAt < 11 ) )
		return false;

	if ( bForce || !HasPlayedHint(hint) )
	{
		PlayedAHint();
		HintMessage( m_pszHintMessages[hint] );
		m_HintHistory.Set(hint);
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Displays a hint message to the player
// Input  : *pMessage - 
//-----------------------------------------------------------------------------
void CHintSystem::HintMessage( const char *pMessage )
{
	Assert( m_pPlayer );

#ifdef GAME_DLL
	// On the server, we send it down to the queue who sends it to the client
	if ( !m_pPlayer->IsNetClient() || !m_pHintMessageQueue )
		return;

	if ( !m_bShowHints )
		return;

	m_pHintMessageQueue->AddMessage( pMessage );
#else
	// On the client, we just send it straight to the hint hud element
	if ( cl_showhelp.GetBool() )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_hintmessage" );
		if ( event )
		{
			event->SetString( "hintmessage", pMessage );
			gameeventmanager->FireEventClientSide( event );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Clear out the existing timers, and register new ones for any
//			hints that haven't been displayed yet.
//-----------------------------------------------------------------------------
void CHintSystem::ResetHints( void )
{
	if ( !m_pHintMessageTimers )
		return;

	m_pHintMessageTimers->Reset();

	// Readd registered hints
	for (int i = 0; i < m_RegisteredResetHints.Count(); i++ )
	{
		ReAddHintTimerIfNotDisplayed( m_RegisteredResetHints[i].iHintID, m_RegisteredResetHints[i].flTimer );
	}

	// Reset our queue
	if ( m_pHintMessageQueue )
	{
		m_pHintMessageQueue->Reset();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Call this to add a hint message that should be re-added  
//			everytime we're reset, if it hasn't been displayed yet.
//-----------------------------------------------------------------------------
void CHintSystem::RegisterHintTimer( int iHintID, float flTimerDuration, bool bOnlyIfClear /* = false */, HintTimerCallback pfnCallback )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );

	onresethints_t newHint;
	newHint.iHintID = iHintID;
	newHint.flTimer = flTimerDuration;
	newHint.bOnlyIfClear = bOnlyIfClear;
	newHint.pfnCallback = pfnCallback;
	m_RegisteredResetHints.AddToTail( newHint );
}

//-----------------------------------------------------------------------------
// Purpose: If the hint hasn't been displayed, start a timer for it
//-----------------------------------------------------------------------------
void CHintSystem::ReAddHintTimerIfNotDisplayed( int iHintID, float flTimerDuration )
{
	Assert( iHintID < m_HintHistory.GetNumBits() );
	if ( m_HintHistory[iHintID] == 0 )
	{
		m_pHintMessageTimers->AddTimer( iHintID, flTimerDuration );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::StartHintTimer( int iHintID )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );
	Assert(m_pHintMessageTimers);
	m_pHintMessageTimers->StartTimer( iHintID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::StopHintTimer( int iHintID )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );
	Assert(m_pHintMessageTimers);
	m_pHintMessageTimers->StopTimer( iHintID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::ResetHintTimers( void )
{
	Assert( m_pPlayer );
	Assert(m_pHintMessageTimers);
	m_pHintMessageTimers->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::RemoveHintTimer( int iHintID )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );
	Assert(m_pHintMessageTimers);
	m_pHintMessageTimers->RemoveTimer( iHintID );

	// Mark us as having heard this hint
	m_HintHistory.Set(iHintID);
}

//-----------------------------------------------------------------------------
// Purpose: See if there's a callback registered for the specified hint.
//			If so, see if it wants to allow the hint to fire.
//-----------------------------------------------------------------------------
bool CHintSystem::TimerShouldFire( int iHintID )
{
	for (int i = 0; i < m_RegisteredResetHints.Count(); i++ )
	{
		if ( m_RegisteredResetHints[i].iHintID != iHintID )
			continue;
		
		if ( m_RegisteredResetHints[i].bOnlyIfClear && HintIsCurrentlyVisible() )
			return false;

		if ( m_RegisteredResetHints[i].pfnCallback )
			return m_RegisteredResetHints[i].pfnCallback( m_pPlayer );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHintSystem::ShouldShowHints( void ) 
{ 
#ifdef GAME_DLL
	return m_bShowHints; 
#else
	return cl_showhelp.GetBool();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::PlayedAHint( void )
{
	m_flLastHintPlayedAt = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHintSystem::HasPlayedHint( int iHintID )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );
	return ( m_HintHistory[iHintID] > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHintSystem::SetHintPlayed( int iHintID )
{
	Assert( m_pPlayer );
	Assert( iHintID < m_HintHistory.GetNumBits() );
	m_HintHistory.Set(iHintID);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void HintClear( void )
{
#ifdef CLIENT_DLL
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
#else
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
#endif
	if ( pPlayer && pPlayer->Hints() )
	{
		pPlayer->Hints()->ClearHintHistory();
	}
}
#ifdef CLIENT_DLL
ConCommand cl_clearhinthistory( "cl_clearhinthistory", HintClear, "Clear memory of client side hints displayed to the player." );
#else
ConCommand sv_clearhinthistory( "sv_clearhinthistory", HintClear, "Clear memory of server side hints displayed to the player." );
#endif
