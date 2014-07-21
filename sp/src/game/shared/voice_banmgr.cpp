//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <string.h>
#include <stdio.h>
#include "voice_banmgr.h"
#include "filesystem.h"
#include "cdll_client_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BANMGR_FILEVERSION	1
const char *g_pBanMgrFilename = "voice_ban.dt";



// Hash a player ID to a byte.
unsigned char HashPlayerID(char const playerID[SIGNED_GUID_LEN])
{
	unsigned char curHash = 0;

	for(int i=0; i < SIGNED_GUID_LEN; i++)
		curHash = (unsigned char)(curHash + playerID[i]);

	return curHash;
}



CVoiceBanMgr::CVoiceBanMgr()
{
	Clear();
}


CVoiceBanMgr::~CVoiceBanMgr()
{
	Term();
}


bool CVoiceBanMgr::Init(const char *pGameDir)
{
	Term();

	// Load in the squelch file.
	FileHandle_t fh = filesystem->Open(g_pBanMgrFilename, "rb");
	if (fh)
	{
		int version;
		filesystem->Read(&version, sizeof(version), fh);
		if(version == BANMGR_FILEVERSION && filesystem->Size(fh) > 4 )
		{
			int nIDs = ( filesystem->Size( fh ) - sizeof(version)) / SIGNED_GUID_LEN;

			for(int i=0; i < nIDs; i++)
			{
				char playerID[SIGNED_GUID_LEN];
				filesystem->Read(playerID, SIGNED_GUID_LEN, fh);
				AddBannedPlayer(playerID);
			}			
		}

		filesystem->Close(fh);
	}

	return true;
}


void CVoiceBanMgr::Term()
{
	// Free all the player structures.
	for(int i=0; i < 256; i++)
	{
		BannedPlayer *pListHead = &m_PlayerHash[i];
		BannedPlayer *pNext;
		for(BannedPlayer *pCur=pListHead->m_pNext; pCur != pListHead; pCur=pNext)
		{
			pNext = pCur->m_pNext;
			delete pCur;
		}
	}

	Clear();
}


void CVoiceBanMgr::SaveState(const char *pGameDir)
{
	// Save the file out.
	FileHandle_t fh = filesystem->Open(g_pBanMgrFilename, "wb");
	if(fh)
	{
		int version = BANMGR_FILEVERSION;
		filesystem->Write(&version, sizeof(version), fh);

		for(int i=0; i < 256; i++)
		{
			BannedPlayer *pListHead = &m_PlayerHash[i];
			for(BannedPlayer *pCur=pListHead->m_pNext; pCur != pListHead; pCur=pCur->m_pNext)
			{
				filesystem->Write(pCur->m_PlayerID, SIGNED_GUID_LEN, fh);
			}
		}

		filesystem->Close(fh);
	}
}


bool CVoiceBanMgr::GetPlayerBan(char const playerID[SIGNED_GUID_LEN])
{
	return !!InternalFindPlayerSquelch(playerID);
}


void CVoiceBanMgr::SetPlayerBan(char const playerID[SIGNED_GUID_LEN], bool bSquelch)
{
	if(bSquelch)
	{
		// Is this guy already squelched?
		if(GetPlayerBan(playerID))
			return;
	
		AddBannedPlayer(playerID);
	}
	else
	{
		BannedPlayer *pPlayer = InternalFindPlayerSquelch(playerID);
		if(pPlayer)
		{
			pPlayer->m_pPrev->m_pNext = pPlayer->m_pNext;
			pPlayer->m_pNext->m_pPrev = pPlayer->m_pPrev;
			delete pPlayer;
		}
	}
}


void CVoiceBanMgr::Clear()
{
	// Tie off the hash table entries.
	for(int i=0; i < 256; i++)
		m_PlayerHash[i].m_pNext = m_PlayerHash[i].m_pPrev = &m_PlayerHash[i];

	// TODO memory leaks ???
}


CVoiceBanMgr::BannedPlayer* CVoiceBanMgr::InternalFindPlayerSquelch(char const playerID[SIGNED_GUID_LEN])
{
	int index = HashPlayerID(playerID);
	BannedPlayer *pListHead = &m_PlayerHash[index];
	for(BannedPlayer *pCur=pListHead->m_pNext; pCur != pListHead; pCur=pCur->m_pNext)
	{
		if(memcmp(playerID, pCur->m_PlayerID, SIGNED_GUID_LEN) == 0)
			return pCur;
	}

	return NULL;
}


CVoiceBanMgr::BannedPlayer* CVoiceBanMgr::AddBannedPlayer(char const playerID[SIGNED_GUID_LEN])
{
	BannedPlayer *pNew = new BannedPlayer;
	if(!pNew)
		return NULL;

	int index = HashPlayerID(playerID);
	memcpy(pNew->m_PlayerID, playerID, SIGNED_GUID_LEN);
	pNew->m_pNext = &m_PlayerHash[index];
	pNew->m_pPrev = m_PlayerHash[index].m_pPrev;
	pNew->m_pPrev->m_pNext = pNew->m_pNext->m_pPrev = pNew;
	return pNew;
}

