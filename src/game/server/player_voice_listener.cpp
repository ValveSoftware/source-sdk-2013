//========== Copyright � Valve Corporation, All rights reserved. ============
//
// Purpose: Maintain data about voice communications from various players
//			so that the server can react to it.
//			
//===========================================================================

#include "cbase.h"
#include "vscript_server.h"

#include "player_voice_listener.h"

ConVar voice_player_speaking_delay_threshold( "voice_player_speaking_delay_threshold", "0.5f", FCVAR_CHEAT );

BEGIN_SCRIPTDESC_ROOT_NAMED( CPlayerVoiceListener, "CPlayerVoiceListener", SCRIPT_SINGLETON "Player voice listeners" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsPlayerSpeaking, "IsPlayerSpeaking", "Returns whether the player specified is speaking." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetPlayerSpeechDuration, "GetPlayerSpeechDuration", "Returns the number of seconds the player has been continuously speaking." )
END_SCRIPTDESC()

inline bool IsPlayerIndexValid( int nPlayerIndex )
{
	if ( nPlayerIndex < 1 || nPlayerIndex > MAX_PLAYERS )
		return false;

	return true;
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
CPlayerVoiceListener::CPlayerVoiceListener( void )
{
	InitData();
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
bool CPlayerVoiceListener::IsPlayerSpeaking( int nPlayerIndex )
{
	// Validate the player index
	if ( IsPlayerIndexValid( nPlayerIndex ) == false )
		return false;

	// See if they last spoke within our time threshold
	if ( ( m_flLastPlayerSpeechTime[nPlayerIndex] + voice_player_speaking_delay_threshold.GetFloat() ) >= gpGlobals->curtime )
		return true;

	// Silent too long
	return false;
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
bool CPlayerVoiceListener::IsPlayerSpeaking( CBasePlayer *pPlayer )
{
	// Validate the player
	Assert( pPlayer );
	if ( pPlayer == NULL )
		return false;

	return IsPlayerSpeaking( pPlayer->entindex() );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
bool CPlayerVoiceListener::ScriptIsPlayerSpeaking( int nPlayerIndex )
{
	return IsPlayerSpeaking( nPlayerIndex );
}


#define SPEECH_DURATION_HACK	1.0f	// This is a total hack to guess a duration of a voice packet, this has no real value

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
void CPlayerVoiceListener::AddPlayerSpeakTime( int nPlayerIndex )
{
	// Validate the player index
	if ( IsPlayerIndexValid( nPlayerIndex ) == false )
		return;

	// Maintain our current speech time
	if ( IsPlayerSpeaking( nPlayerIndex ) )
	{
		// We're continuing the speech
		m_flPlayerSpeechDuration[nPlayerIndex] += ( gpGlobals->curtime - m_flLastPlayerSpeechTime[nPlayerIndex] );
	}
	else
	{
		// We've just begun to speak
		m_flPlayerSpeechDuration[nPlayerIndex] = 0.0f;
	}

	// Take the time update
	m_flLastPlayerSpeechTime[nPlayerIndex] = gpGlobals->curtime;

	// Msg("Player %d last spoke at %0.2f for %0.2f seconds\n", nPlayerIndex, gpGlobals->curtime, m_flPlayerSpeechDuration[nPlayerIndex] );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
void CPlayerVoiceListener::AddPlayerSpeakTime( CBasePlayer *pPlayer )
{
	// Validate the player
	Assert( pPlayer );
	if ( pPlayer == NULL )
		return;

	return AddPlayerSpeakTime( pPlayer->entindex() );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerLastSpeechTime( int nPlayerIndex )
{
	if ( IsPlayerIndexValid( nPlayerIndex ) == false )
		return -1.0f;

	return m_flLastPlayerSpeechTime[ nPlayerIndex ];
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerLastSpeechTime( CBasePlayer *pPlayer )
{
	// Validate the player
	Assert( pPlayer );
	if ( pPlayer == NULL )
		return -1.0f;

	return GetPlayerLastSpeechTime( pPlayer->entindex() );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerSilenceDuration( int nPlayerIndex )
{
	if ( IsPlayerIndexValid( nPlayerIndex ) == false )
		return -1.0f;

	return gpGlobals->curtime - m_flLastPlayerSpeechTime[ nPlayerIndex ];
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerSilenceDuration( CBasePlayer *pPlayer )
{
	// Validate the player
	Assert( pPlayer );
	if ( pPlayer == NULL )
		return -1.0f;

	return GetPlayerSilenceDuration( pPlayer->entindex() );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerSpeechDuration( int nPlayerIndex )
{
	if ( IsPlayerIndexValid( nPlayerIndex ) == false )
		return -1.0f;

	return m_flPlayerSpeechDuration[nPlayerIndex];
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::GetPlayerSpeechDuration( CBasePlayer *pPlayer )
{
	// Validate the player
	Assert( pPlayer );
	if ( pPlayer == NULL )
		return -1.0f;

	return GetPlayerSpeechDuration( pPlayer->entindex() );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
float CPlayerVoiceListener::ScriptGetPlayerSpeechDuration( int nPlayerIndex )
{
	return GetPlayerSpeechDuration( nPlayerIndex );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
void CPlayerVoiceListener::InitData( void )
{
	// Clear our tracking data
	memset( m_flLastPlayerSpeechTime, 0.0f, sizeof( m_flLastPlayerSpeechTime ) );
	memset( m_flPlayerSpeechDuration, 0.0f, sizeof( m_flPlayerSpeechDuration ) );
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
void CPlayerVoiceListener::LevelInitPreEntity( void )
{
	InitData();
}

//---------------------------------------------------------------------------
// Purpose:
//---------------------------------------------------------------------------
void CPlayerVoiceListener::LevelShutdownPreEntity( void )
{
	InitData();
}

// Singleton
CPlayerVoiceListener g_PlayerVoiceListener;
CPlayerVoiceListener &PlayerVoiceListener( void ) { return g_PlayerVoiceListener; }