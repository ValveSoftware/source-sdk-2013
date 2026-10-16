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

#define BANMGR_FLAG_VOICE (1<<0)
#define BANMGR_FLAG_CHAT (1<<1)

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

	int		GetPlayerBan(char const playerID[SIGNED_GUID_LEN]);
	void	SetPlayerBan(char const playerID[SIGNED_GUID_LEN], int iBanFlag, bool bSquelch);


protected:

	class BannedPlayer
	{
	public:
		char			m_PlayerID[SIGNED_GUID_LEN];
		BannedPlayer	*m_pPrev, *m_pNext;
		int				m_iBanFlags;
	};

	void			Clear();
	BannedPlayer*	InternalFindPlayerSquelch(char const playerID[SIGNED_GUID_LEN]);
	BannedPlayer*	AddBannedPlayer(char const playerID[SIGNED_GUID_LEN], int iBanFlags);


protected:

	BannedPlayer	m_PlayerHash[256];
};


#endif // VOICE_BANMGR_H
