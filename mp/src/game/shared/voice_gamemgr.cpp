//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "voice_gamemgr.h"
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "player.h"
#include "ivoiceserver.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define UPDATE_INTERVAL	0.3


// These are stored off as CVoiceGameMgr is created and deleted.
CPlayerBitVec	g_PlayerModEnable;		// Set to 1 for each player if the player wants to use voice in this mod.
										// (If it's zero, then the server reports that the game rules are saying the
										// player can't hear anyone).

CPlayerBitVec	g_BanMasks[VOICE_MAX_PLAYERS];	// Tells which players don't want to hear each other.
												// These are indexed as clients and each bit represents a client
												// (so player entity is bit+1).

CPlayerBitVec	g_SentGameRulesMasks[VOICE_MAX_PLAYERS];	// These store the masks we last sent to each client so we can determine if
CPlayerBitVec	g_SentBanMasks[VOICE_MAX_PLAYERS];			// we need to resend them.
CPlayerBitVec	g_bWantModEnable;

ConVar voice_serverdebug( "voice_serverdebug", "0" );

// Set game rules to allow all clients to talk to each other.
// Muted players still can't talk to each other.
ConVar sv_alltalk( "sv_alltalk", "0", FCVAR_NOTIFY, "Players can hear all other players, no team restrictions" );


CVoiceGameMgr g_VoiceGameMgr;


// ------------------------------------------------------------------------ //
// Static helpers.
// ------------------------------------------------------------------------ //

// Find a player with a case-insensitive name search.
static CBasePlayer* FindPlayerByName(const char *pTestName)
{
	for(int i=1; i <= gpGlobals->maxClients; i++)
	{
		edict_t *pEdict = engine->PEntityOfEntIndex(i);
		if(pEdict)
		{
			CBaseEntity *pEnt = CBaseEntity::Instance(pEdict);
			if(pEnt && pEnt->IsPlayer())
			{			
				const char *pNetName = STRING(pEnt->GetEntityName());
				if(stricmp(pNetName, pTestName) == 0)
				{
					return (CBasePlayer*)pEnt;
				}
			}
		}
	}

	return NULL;
}

static void VoiceServerDebug( const char *pFmt, ... )
{
	char msg[4096];
	va_list marker;

	if( !voice_serverdebug.GetInt() )
		return;

	va_start( marker, pFmt );
	_vsnprintf( msg, sizeof(msg), pFmt, marker );
	va_end( marker );

	Msg( "%s", msg );
}


CVoiceGameMgr* GetVoiceGameMgr()
{
	return &g_VoiceGameMgr;
}



// ------------------------------------------------------------------------ //
// CVoiceGameMgr.
// ------------------------------------------------------------------------ //

CVoiceGameMgr::CVoiceGameMgr()
{
	m_UpdateInterval = 0;
	m_nMaxPlayers = 0;
	m_iProximityDistance = -1;
}


CVoiceGameMgr::~CVoiceGameMgr()
{
}

bool CVoiceGameMgr::Init(
	IVoiceGameMgrHelper *pHelper,
	int maxClients)
{		  
	m_pHelper = pHelper;
	m_nMaxPlayers = VOICE_MAX_PLAYERS < maxClients ? VOICE_MAX_PLAYERS : maxClients;

	return true;
}


void CVoiceGameMgr::SetHelper(IVoiceGameMgrHelper *pHelper)
{
	m_pHelper = pHelper;
}


void CVoiceGameMgr::Update(double frametime)
{
	// Only update periodically.
	m_UpdateInterval += frametime;
	if(m_UpdateInterval < UPDATE_INTERVAL)
		return;

	UpdateMasks();
}


void CVoiceGameMgr::ClientConnected(struct edict_t *pEdict)
{
	int index = ENTINDEX(pEdict) - 1;
	
	// Clear out everything we use for deltas on this guy.
	g_bWantModEnable[index] = true;
	g_SentGameRulesMasks[index].Init(0);
	g_SentBanMasks[index].Init(0);
}


bool CVoiceGameMgr::ClientCommand( CBasePlayer *pPlayer, const CCommand &args )
{
	int playerClientIndex = pPlayer->entindex() - 1;
	if(playerClientIndex < 0 || playerClientIndex >= m_nMaxPlayers)
	{
		VoiceServerDebug( "CVoiceGameMgr::ClientCommand: cmd %s from invalid client (%d)\n", args[0], playerClientIndex );
		return true;
	}

	bool bBan = stricmp( args[0], "vban" ) == 0;
	if( bBan && args.ArgC() >= 2 )
	{
		for(int i=1; i < args.ArgC(); i++)
		{
			uint32 mask = 0;
			sscanf( args[i], "%x", &mask);

			if( i <= VOICE_MAX_PLAYERS_DW )
			{
				VoiceServerDebug( "CVoiceGameMgr::ClientCommand: vban (0x%x) from %d\n", mask, playerClientIndex );
				g_BanMasks[playerClientIndex].SetDWord(i-1, mask);
			}
			else
			{
				VoiceServerDebug( "CVoiceGameMgr::ClientCommand: invalid index (%d)\n", i );
			}
		}

		// Force it to update the masks now.
		//UpdateMasks();		
		return true;
	}
	else if(stricmp( args[0], "VModEnable") == 0 && args.ArgC() >= 2)
	{
		VoiceServerDebug( "CVoiceGameMgr::ClientCommand: VModEnable (%d)\n", !!atoi( args[1] ) );
		g_PlayerModEnable[playerClientIndex] = !!atoi( args[1] );
		g_bWantModEnable[playerClientIndex] = false;
		//UpdateMasks();		
		return true;
	}
	else
	{
		return false;
	}
}


void CVoiceGameMgr::UpdateMasks()
{
	m_UpdateInterval = 0;

	bool bAllTalk = !!sv_alltalk.GetInt();

	for(int iClient=0; iClient < m_nMaxPlayers; iClient++)
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex(iClient+1);
		if(!pEnt || !pEnt->IsPlayer())
			continue;

		CBasePlayer *pPlayer = (CBasePlayer*)pEnt;

		CSingleUserRecipientFilter user( pPlayer );

		// Request the state of their "VModEnable" cvar.
		if(g_bWantModEnable[iClient])
		{

			UserMessageBegin( user, "RequestState" );
			MessageEnd();
			// Since this is reliable, only send it once
			g_bWantModEnable[iClient] = false;
		}

		CPlayerBitVec gameRulesMask;
		CPlayerBitVec ProximityMask;
		bool		bProximity = false;
		if( g_PlayerModEnable[iClient] )
		{
			// Build a mask of who they can hear based on the game rules.
			for(int iOtherClient=0; iOtherClient < m_nMaxPlayers; iOtherClient++)
			{
				CBaseEntity *pEnt = UTIL_PlayerByIndex(iOtherClient+1);
				if(pEnt && pEnt->IsPlayer() && 
					(bAllTalk || m_pHelper->CanPlayerHearPlayer(pPlayer, (CBasePlayer*)pEnt, bProximity )) )
				{
					gameRulesMask[iOtherClient] = true;
					ProximityMask[iOtherClient] = bProximity;
				}
			}
		}

		// If this is different from what the client has, send an update. 
		if(gameRulesMask != g_SentGameRulesMasks[iClient] || 
			g_BanMasks[iClient] != g_SentBanMasks[iClient])
		{
			g_SentGameRulesMasks[iClient] = gameRulesMask;
			g_SentBanMasks[iClient] = g_BanMasks[iClient];

			UserMessageBegin( user, "VoiceMask" );
				int dw;
				for(dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
				{
					WRITE_LONG(gameRulesMask.GetDWord(dw));
					WRITE_LONG(g_BanMasks[iClient].GetDWord(dw));
				}
				WRITE_BYTE( !!g_PlayerModEnable[iClient] );
			MessageEnd();
		}

		// Tell the engine.
		for(int iOtherClient=0; iOtherClient < m_nMaxPlayers; iOtherClient++)
		{
			bool bCanHear = gameRulesMask[iOtherClient] && !g_BanMasks[iClient][iOtherClient];
			g_pVoiceServer->SetClientListening( iClient+1, iOtherClient+1, bCanHear );

			if ( bCanHear )
			{
				g_pVoiceServer->SetClientProximity( iClient+1, iOtherClient+1, !!ProximityMask[iOtherClient] );
			}
		}
	}
}

bool CVoiceGameMgr::IsPlayerIgnoringPlayer( int iTalker, int iListener )
{
	return !!g_BanMasks[iListener-1][iTalker-1];
}

void CVoiceGameMgr::SetProximityDistance( int iDistance )
{
	m_iProximityDistance = iDistance;
}

bool CVoiceGameMgr::CheckProximity( int iDistance )
{
	if ( m_iProximityDistance >= iDistance )
		return true;

	return false;
}