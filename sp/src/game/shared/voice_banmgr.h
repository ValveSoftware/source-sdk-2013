//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VOICE_BANMGR_H
#define VOICE_BANMGR_H
#ifdef _WIN32
#pragma once
#endif


// This class manages the (persistent) list of squelched players.
class CVoiceBanMgr
{
public:

				CVoiceBanMgr();
				~CVoiceBanMgr();	

	// Init loads the list of squelched players from disk.
	bool		Init(const char *pGameDir);
	void		Term();

	// Saves the state into voice_squelch.dt.
	void		SaveState(const char *pGameDir);

	bool		GetPlayerBan(char const playerID[SIGNED_GUID_LEN]);
	void		SetPlayerBan(char const playerID[SIGNED_GUID_LEN], bool bSquelch);


protected:

	class BannedPlayer
	{
	public:
		char			m_PlayerID[SIGNED_GUID_LEN];
		BannedPlayer	*m_pPrev, *m_pNext;
	};

	void			Clear();
	BannedPlayer*	InternalFindPlayerSquelch(char const playerID[SIGNED_GUID_LEN]);
	BannedPlayer*	AddBannedPlayer(char const playerID[SIGNED_GUID_LEN]);


protected:

	BannedPlayer	m_PlayerHash[256];
};


#endif // VOICE_BANMGR_H
