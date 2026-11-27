#ifndef TF_STREAMER_MODE_H
#define TF_STREAMER_MODE_H
#ifdef _WIN32
#pragma once
#endif

class CSteamID;

enum ETFStreamerNameStyle
{
	TF_STREAMER_NAME_PLAYER_NUMBERS = 0,
	TF_STREAMER_NAME_BOT_NAMES,

	TF_STREAMER_NAME_STYLE_COUNT
};

bool TF_IsStreamerModeEnabled( void );

const char *TF_GetPlayerDisplayName( int nPlayerIndex );

const char *TF_GetPlayerDisplayName( const CSteamID &steamID, const char *pszFallbackRealName );

const char *TF_GetLocalStreamerDisplayName( void );

const char *TF_GetPartyMemberDisplayName( const CSteamID &steamID, const char *pszFallbackRealName );

bool TF_ShouldAnonymisePlayerNames( void );
bool TF_ShouldUseDefaultAvatar( void );
bool TF_ShouldHideChat( void );
bool TF_ShouldHidePartyChat( void );
bool TF_ShouldHideCallouts( void );
bool TF_ShouldDisableVoice( void );
bool TF_ShouldHideServerInformation( void );
bool TF_ShouldDisableDirectJoinPresence( void );
bool TF_ShouldHideCustomItemText( uint32 unAccountID );
bool TF_ShouldHidePlayerCreatedImages( int nPlayerIndex );
bool TF_IsLocalAccountID( uint32 unAccountID );
bool TF_ShouldHideFriendsList( void );

void TF_StreamerMode_ResetSession( void );

#endif // TF_STREAMER_MODE_H
