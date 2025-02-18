//========== Copyright � Valve Corporation, All rights reserved. ============
//
// Purpose: 
//
//===========================================================================

#ifndef PLAYER_VOICE_LISTENER_H
#define PLAYER_VOICE_LISTENER_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

class CPlayerVoiceListener : public CAutoGameSystem
{
public:
	CPlayerVoiceListener( void );

	// Auto-game cleanup
	virtual void LevelInitPreEntity( void );
	virtual void LevelShutdownPreEntity( void );

	bool IsPlayerSpeaking( int nPlayerIndex );
	bool IsPlayerSpeaking( CBasePlayer *pPlayer );
	bool ScriptIsPlayerSpeaking( int nPlayerIndex );

	void AddPlayerSpeakTime( int nPlayerIndex );
	void AddPlayerSpeakTime( CBasePlayer *pPlayer );

	float GetPlayerLastSpeechTime( int nPlayerIndex );
	float GetPlayerLastSpeechTime( CBasePlayer *pPlayer );

	float GetPlayerSilenceDuration( int nPlayerIndex );
	float GetPlayerSilenceDuration( CBasePlayer *pPlayer );

	float GetPlayerSpeechDuration( int nPlayerIndex );
	float GetPlayerSpeechDuration( CBasePlayer *pPlayer );
	float ScriptGetPlayerSpeechDuration( int nPlayerIndex );

private:
	void	InitData( void );

	float	m_flLastPlayerSpeechTime[MAX_PLAYERS_ARRAY_SAFE];
	float	m_flPlayerSpeechDuration[MAX_PLAYERS_ARRAY_SAFE];
};

extern CPlayerVoiceListener &PlayerVoiceListener( void );

#endif // PLAYER_VOICE_LISTENER_H
