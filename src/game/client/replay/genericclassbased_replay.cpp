//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "genericclassbased_replay.h"
#include "clientmode_shared.h"
#include "replay/ireplaymoviemanager.h"
#include "replay/ireplayfactory.h"
#include "replay/ireplayscreenshotmanager.h"
#include "replay/screenshot.h"
#include "replay/gamedefs.h"
#include <time.h>

//----------------------------------------------------------------------------------------

extern IReplayScreenshotManager *g_pReplayScreenshotManager;

//----------------------------------------------------------------------------------------

CGenericClassBasedReplay::CGenericClassBasedReplay()
:	m_nPlayerTeam( 0 )
{
	m_szKillerName[ 0 ] = 0;

	m_nPlayerClass = REPLAY_CLASS_UNDEFINED;
	m_nKillerClass = REPLAY_CLASS_UNDEFINED;
}

CGenericClassBasedReplay::~CGenericClassBasedReplay()
{
	OnEndRecording();

	m_vecKills.PurgeAndDeleteElements();
	m_vecDominations.PurgeAndDeleteElements();
	m_vecAssisterDominations.PurgeAndDeleteElements();
	m_vecRevenges.PurgeAndDeleteElements();
	m_vecAssisterRevenges.PurgeAndDeleteElements();
}

void CGenericClassBasedReplay::OnBeginRecording()
{
	BaseClass::OnBeginRecording();

	Assert( gameeventmanager );
	ListenForGameEvent( "player_death" );
}

void CGenericClassBasedReplay::OnEndRecording()
{
	if ( gameeventmanager )
	{
		gameeventmanager->RemoveListener( this );
	}

	BaseClass::OnEndRecording();
}

void CGenericClassBasedReplay::OnComplete()
{
	BaseClass::OnComplete();
}

bool CGenericClassBasedReplay::ShouldAllowDelete() const
{
	return g_pClientReplayContext->GetMovieManager()->GetNumMoviesDependentOnReplay( this ) == 0;
}

void CGenericClassBasedReplay::OnDelete()
{
	BaseClass::OnDelete();
}

void CGenericClassBasedReplay::FireGameEvent( IGameEvent *pEvent )
{
}

void CGenericClassBasedReplay::Update()
{
	BaseClass::Update();

	// Record any new stats
	RecordUpdatedStats();

	// Setup next update
	m_flNextUpdateTime = engine->Time() + .1f;
}

float CGenericClassBasedReplay::GetKillScreenshotDelay()
{
	ConVarRef replay_screenshotkilldelay( "replay_screenshotkilldelay" );
	return replay_screenshotkilldelay.IsValid() ? replay_screenshotkilldelay.GetFloat() : 0.5f;
}

void CGenericClassBasedReplay::RecordUpdatedStats()
{
	// Get current stats
	static RoundStats_t s_curStats;
	if ( !GetCurrentStats( s_curStats ) )
		return;

	// Go through each stat and see if it's changed
	for ( int i = 0; i < REPLAY_GAMESTATS_MAX; ++i )
	{
		const int nCurStat = s_curStats.Get( i );
		const int nRefStat = m_refStats.Get( i );

		if ( nCurStat != nRefStat )
		{
			// Calculate new stat based on reference
			const int nLifeStat = nCurStat - nRefStat;

			if ( nLifeStat != m_lifeStats.Get( i ) )
			{
				ConVarRef replay_debug( "replay_debug" );
				if ( replay_debug.IsValid() && replay_debug.GetBool() )
				{
					Msg( "REPLAY: Player stat \"%s\" changed from %i to %i.\n", GetStatString( i ), nRefStat, nCurStat );
				}

				// Set the new stat
				m_lifeStats.Set( i, nLifeStat );
			}
		}
	}
}

bool CGenericClassBasedReplay::Read( KeyValues *pIn )
{
	if ( !BaseClass::Read( pIn ) )
		return false;

	// Read player class
	m_nPlayerClass = pIn->GetInt( "player_class" );		Assert( IsValidClass( m_nPlayerClass ) );
	
	// Read player team
	m_nPlayerTeam = pIn->GetInt( "player_team" );		Assert( IsValidTeam( m_nPlayerTeam ) );

	// Read killer info
	m_nKillerClass = pIn->GetInt( "killer_class" );
	V_strcpy_safe( m_szKillerName, pIn->GetString( "killer_name" ) );

	// Make sure vector is clear
	Assert( GetKillCount() == 0 );

	// Read all kill data and add the kills vector
	KeyValues *pKills = pIn->FindKey( "kills" );
	if ( pKills )
	{
		FOR_EACH_TRUE_SUBKEY( pKills, pKill )
		{
			// Create the kill data
			AddKill(
				pKill->GetString( "victim_name" ),
				pKill->GetInt( "victim_class" )
			);
		}
	}

	AddKillStats( m_vecDominations        , pIn, "dominations", REPLAY_GAMESTATS_DOMINATIONS );
	AddKillStats( m_vecAssisterDominations, pIn, "assister_dominations", REPLAY_GAMESTATS_UNDEFINED );
	AddKillStats( m_vecRevenges           , pIn, "revenges", REPLAY_GAMESTATS_REVENGE );
	AddKillStats( m_vecAssisterRevenges   , pIn, "assister_revenges", REPLAY_GAMESTATS_UNDEFINED );

	// Read stats by index
	KeyValues *pStats = pIn->FindKey( "stats" );
	if ( pStats )
	{
		for ( int i = 0; i < REPLAY_GAMESTATS_MAX; ++i )
		{
			char szStatKey[ 16 ];
			V_snprintf( szStatKey, sizeof( szStatKey ), "%i", i );
			m_lifeStats.Set( i, pStats->GetInt( szStatKey ) );
		}
	}

	return true;
}

void CGenericClassBasedReplay::AddKillStats( CUtlVector< GenericStatInfo_t * > &vecKillStats, KeyValues *pIn, const char *pSubKeyName, int iStatIndex )
{
	Assert( vecKillStats.Count() == 0 );
	KeyValues *pSubKey = pIn->FindKey( pSubKeyName );
	if ( pSubKey )
	{
		FOR_EACH_TRUE_SUBKEY( pSubKey, pCurKillStat )
		{
			GenericStatInfo_t *pNewKillStat = new GenericStatInfo_t;
			pNewKillStat->m_nVictimFriendId = pCurKillStat->GetInt( "victim_friend_id" );
			pNewKillStat->m_nAssisterFriendId = pCurKillStat->GetInt( "assister_friend_id" );
			vecKillStats.AddToTail( pNewKillStat );
		}
	}

	// Duplicate the data in the life stats
	if ( iStatIndex > m_nStatUndefined )
	{
		m_lifeStats.Set( iStatIndex, vecKillStats.Count() );
	}
}

void CGenericClassBasedReplay::Write( KeyValues *pOut )
{
	BaseClass::Write( pOut );

	// Write player class
	pOut->SetInt( "player_class", m_nPlayerClass );

	// Write player team
	pOut->SetInt( "player_team", m_nPlayerTeam );

	// Write killer info
	pOut->SetInt( "killer_class", m_nKillerClass );
	pOut->SetString( "killer_name", m_szKillerName );

	// Write kills
	KeyValues *pKills = new KeyValues( "kills" );
	pOut->AddSubKey( pKills );

	for ( int i = 0; i < GetKillCount(); ++i )
	{
		KillData_t *pCurKill = m_vecKills[ i ];

		KeyValues *pKillOut = new KeyValues( "kill" );
		pKills->AddSubKey( pKillOut );

		// Write kill data
		pKillOut->SetString( "victim_name", pCurKill->m_szPlayerName );
		pKillOut->SetInt( "victim_class", pCurKill->m_nPlayerClass );
	}

	WriteKillStatVector( m_vecDominations        , "dominations"         , "domination"         , pOut, 1 );
	WriteKillStatVector( m_vecAssisterDominations, "assister_dominations", "assister_domination", pOut, 2 );
	WriteKillStatVector( m_vecRevenges           , "revenges"            , "revenge"            , pOut, 1 );
	WriteKillStatVector( m_vecAssisterRevenges   , "assister_revenges"   , "assister_revenge"   , pOut, 2 );

	// Write non-zero stats by index
	KeyValues *pStats = new KeyValues( "stats" );
	pOut->AddSubKey( pStats );

	for ( int i = 0; i < REPLAY_GAMESTATS_MAX; ++i )
	{
		const int nCurStat = m_lifeStats.Get( i );
		if ( nCurStat )
		{
			char szStatKey[ 16 ];
			V_snprintf( szStatKey, sizeof( szStatKey ), "%i", i );
			pStats->SetInt( szStatKey, nCurStat );
		}
	}
}

void CGenericClassBasedReplay::WriteKillStatVector( CUtlVector< CGenericClassBasedReplay::GenericStatInfo_t * > const &vec, const char *pSubKeyName,
													const char *pElementKeyName, KeyValues *pRootKey, int nNumMembersToWrite ) const
{
	Assert( nNumMembersToWrite >= 1 );

	// Write dominations
	KeyValues *pSubKey = new KeyValues( pSubKeyName );
	pRootKey->AddSubKey( pSubKey );

	for ( int i = 0; i < vec.Count(); ++i )
	{
		GenericStatInfo_t *pSrcData = vec[ i ];

		KeyValues *pCurSubKey = new KeyValues( pElementKeyName );
		pSubKey->AddSubKey( pCurSubKey );

		// Always write
		pCurSubKey->SetInt( "victim_friend_id", pSrcData->m_nVictimFriendId );

		if ( nNumMembersToWrite > 1 )
		{
			pCurSubKey->SetInt( "assister_friend_id", pSrcData->m_nAssisterFriendId );
		}
	}
}

void CGenericClassBasedReplay::AddKill( const char *pPlayerName, int nPlayerClass )
{
	KillData_t *pNewKillData = new KillData_t;

	V_strcpy_safe( pNewKillData->m_szPlayerName , pPlayerName );
	pNewKillData->m_nPlayerClass = nPlayerClass;

	ConVarRef replay_debug( "replay_debug" );
	if ( replay_debug.IsValid() && replay_debug.GetBool() )
	{
		DevMsg( "\n\nRecorded kill: name=%s, class=%s (this=%p)\n\n", pPlayerName, GetPlayerClass( nPlayerClass ), this );
	}

	m_vecKills.AddToTail( pNewKillData );
}

const char *CGenericClassBasedReplay::GetPlayerClass() const
{
	return GetPlayerClass( m_nPlayerClass );
}

void CGenericClassBasedReplay::AddDomination( int nVictimID )
{
	AddKillStatFromUserIds( m_vecDominations, nVictimID );
}

void CGenericClassBasedReplay::AddAssisterDomination( int nVictimID, int nAssiterID )
{
	AddKillStatFromUserIds( m_vecAssisterDominations, nVictimID, nAssiterID );
}

void CGenericClassBasedReplay::AddRevenge( int nVictimID )
{
	AddKillStatFromUserIds( m_vecRevenges, nVictimID );
}

void CGenericClassBasedReplay::AddAssisterRevenge( int nVictimID, int nAssiterID )
{
	AddKillStatFromUserIds( m_vecAssisterRevenges, nVictimID, nAssiterID );
}

void CGenericClassBasedReplay::AddKillStatFromUserIds( CUtlVector< GenericStatInfo_t * > &vec, int nVictimId, int nAssisterId/*=0*/ )
{
	uint32 nVictimFriendId;
	if ( !GetFriendIdFromUserId( engine->GetPlayerForUserID( nVictimId ), nVictimFriendId ) )
		return;

	uint32 nAssisterFriendId = 0;
	if ( nAssisterId && !GetFriendIdFromUserId( engine->GetPlayerForUserID( nAssisterId ), nAssisterFriendId ) )
		return;

	AddKillStatFromFriendIds( vec, nVictimFriendId, nAssisterFriendId );
}

void CGenericClassBasedReplay::AddKillStatFromFriendIds( CUtlVector< GenericStatInfo_t * > &vec, uint32 nVictimFriendId, uint32 nAssisterFriendId/*=0*/ )
{
	GenericStatInfo_t *pNewKillStat = new GenericStatInfo_t;
	pNewKillStat->m_nVictimFriendId = nVictimFriendId;
	pNewKillStat->m_nAssisterFriendId = nAssisterFriendId;
	vec.AddToTail( pNewKillStat );
}

bool CGenericClassBasedReplay::GetFriendIdFromUserId( int nPlayerIndex, uint32 &nFriendIdOut ) const
{
	player_info_t pi;
	if ( !steamapicontext->SteamFriends() ||
		 !steamapicontext->SteamUtils() ||
		 !engine->GetPlayerInfo( nPlayerIndex, &pi ) )
	{
		AssertMsg( 0, "REPLAY: Failed to add domination" );
		nFriendIdOut = 0;
		return false;
	}

	nFriendIdOut = pi.friendsID;

	return true;
}

const char *CGenericClassBasedReplay::GetMaterialFriendlyPlayerClass() const
{
	return GetPlayerClass();
}

const char *CGenericClassBasedReplay::GetKillerName() const
{
	Assert( WasKilled() );
	return m_szKillerName;
}

const char *CGenericClassBasedReplay::GetKillerClass() const
{
	Assert( WasKilled() );
	return GetPlayerClass( m_nKillerClass );
}

void CGenericClassBasedReplay::DumpGameSpecificData() const
{
	DevMsg( "  class: %s\n", GetPlayerClass() );
	
	// Print kills
	DevMsg( "  %i kills:\n", GetKillCount() );
	for ( int i = 0; i < GetKillCount(); ++i )
	{
		KillData_t *pCurKill = m_vecKills[ i ];
		Msg( "    kill %i: name=%s class=%s\n", i, pCurKill->m_szPlayerName, GetPlayerClass( pCurKill->m_nPlayerClass ) );
	}

	if ( !WasKilled() )
	{
		Msg( "  No killer.\n" );
		return;
	}

	// Print killer info
	Msg( "  killer: name=%s class=%s\n", m_szKillerName, GetPlayerClass( m_nKillerClass ) );
}

void CGenericClassBasedReplay::SetPlayerClass( int nPlayerClass )
{
	//Assert( IsValidClass( nPlayerClass ) );
	m_nPlayerClass = nPlayerClass;

	// Setup reference stats if this is a valid class
	if ( IsValidClass( nPlayerClass ) )
	{
		GetCurrentStats( m_refStats );
	}
}

void CGenericClassBasedReplay::SetPlayerTeam( int nPlayerTeam )
{
	m_nPlayerTeam = nPlayerTeam;
}

void CGenericClassBasedReplay::RecordPlayerDeath( const char *pKillerName, int nKillerClass )
{
	V_strcpy_safe( m_szKillerName, pKillerName );
	m_nKillerClass = nKillerClass;
}


//----------------------------------------------------------------------------------------

#endif
