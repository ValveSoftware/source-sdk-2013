//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "mp_shareddefs.h"
#include "basemultiplayerplayer.h"

// Minimum interval between rate-limited commands that players can run.
#define COMMAND_MAX_RATE  0.3

CBaseMultiplayerPlayer::CBaseMultiplayerPlayer()
{
	m_iCurrentConcept = MP_CONCEPT_NONE;
	m_flLastForcedChangeTeamTime = -1;
	m_iBalanceScore = 0;
	m_flConnectionTime = gpGlobals->curtime;

	// per life achievement counters
	m_pAchievementKV = new KeyValues( "achievement_counts" );

	m_flAreaCaptureScoreAccumulator = 0.0f;
}

CBaseMultiplayerPlayer::~CBaseMultiplayerPlayer()
{
	m_pAchievementKV->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Expresser *CBaseMultiplayerPlayer::CreateExpresser( void )
{
	m_pExpresser = new CMultiplayer_Expresser(this);
	if ( !m_pExpresser)
		return NULL;

	m_pExpresser->Connect(this);
	return m_pExpresser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::PostConstructor( const char *szClassname )
{
	BaseClass::PostConstructor( szClassname );
	CreateExpresser();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	BaseClass::ModifyOrAppendCriteria( criteriaSet );

	ModifyOrAppendPlayerCriteria( criteriaSet );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::SpeakIfAllowed( AIConcept_t concept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter ) 
{ 
	if ( !IsAlive() )
		return false;

	//if ( IsAllowedToSpeak( concept, bRespondingToPlayer ) )
	return Speak( concept, modifiers, pszOutResponseChosen, bufsize, filter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IResponseSystem *CBaseMultiplayerPlayer::GetResponseSystem()
{
	return BaseClass::GetResponseSystem();
	// NOTE: This is where you would hook your custom responses.
//	return <*>GameRules()->m_ResponseRules[iIndex].m_ResponseSystems[m_iCurrentConcept];
}

//-----------------------------------------------------------------------------
// Purpose: Doesn't actually speak the concept. Just finds a response in the system. You then have to play it yourself.
//-----------------------------------------------------------------------------
AI_Response *CBaseMultiplayerPlayer::SpeakConcept( int iConcept )
{
	m_iCurrentConcept = iConcept;
	return SpeakFindResponse( g_pszMPConcepts[iConcept] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::SpeakConceptIfAllowed( int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter )
{
	// Save the current concept.
	m_iCurrentConcept = iConcept;
	return SpeakIfAllowed( g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return m_iIgnoreGlobalChat != CHAT_IGNORE_ALL;

	// check if we're ignoring all chat
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_ALL )
		return false;

	// check if we're ignoring all but teammates
	if ( m_iIgnoreGlobalChat == CHAT_IGNORE_TEAM && g_pGameRules->PlayerRelationship( this, pPlayer ) != GR_TEAMMATE )
		return false;

	// can't hear dead players if we're alive
	if ( pPlayer->m_lifeState != LIFE_ALIVE && m_lifeState == LIFE_ALIVE )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand( const CCommand &args )
{
	return ShouldRunRateLimitedCommand( args[0] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand( const char *pszCommand )
{
	const char *pcmd = pszCommand;

	int i = m_RateLimitLastCommandTimes.Find( pcmd );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( pcmd, gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBaseMultiplayerPlayer::ClientCommand( const CCommand &args )
{
	const char *pcmd = args[0];

	if ( FStrEq( pcmd, "ignoremsg" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			m_iIgnoreGlobalChat = (m_iIgnoreGlobalChat + 1) % 3;
			switch( m_iIgnoreGlobalChat )
			{
			case CHAT_IGNORE_NONE:
				ClientPrint( this, HUD_PRINTTALK, "#Accept_All_Messages" );
				break;
			case CHAT_IGNORE_ALL:
				ClientPrint( this, HUD_PRINTTALK, "#Ignore_Broadcast_Messages" );
				break;
			case CHAT_IGNORE_TEAM:
				ClientPrint( this, HUD_PRINTTALK, "#Ignore_Broadcast_Team_Messages" );
				break;
			default:
				break;
			}
		}
		return true;
	}

	return BaseClass::ClientCommand( args );
}

bool CBaseMultiplayerPlayer::ShouldShowVoiceSubtitleToEnemy( void )
{
	return false;
}

//-----------------------------------------------------------------------------
// calculate a score for this player. higher is more likely to be switched
//-----------------------------------------------------------------------------
int	CBaseMultiplayerPlayer::CalculateTeamBalanceScore( void )
{
	// base score is 0 - ( seconds on server )
	float flTimeConnected = gpGlobals->curtime - m_flConnectionTime;
	int iScore = 0 - (int)flTimeConnected;

	// if we were switched recently, score us way down
	float flLastSwitchedTime = GetLastForcedChangeTeamTime();
	if ( flLastSwitchedTime > 0 && ( gpGlobals->curtime - flLastSwitchedTime ) < 300 )
	{
		iScore -= 10000;
	}
	return iScore;
}

void CBaseMultiplayerPlayer::Spawn( void )
{
	ResetPerLifeCounters();

	StopScoringEscortPoints();

	BaseClass::Spawn();
}

void CBaseMultiplayerPlayer::AwardAchievement( int iAchievement, int iCount )
{
	Assert( iAchievement >= 0 && iAchievement < 0xFFFF );		// must fit in short

	CSingleUserRecipientFilter filter( this );

	UserMessageBegin( filter, "AchievementEvent" );
		WRITE_SHORT( iAchievement );
		WRITE_SHORT( iCount );
	MessageEnd();
}

#ifdef _DEBUG

	#include "utlbuffer.h"

	void DumpAchievementCounters( const CCommand &args )
	{
		int iPlayerIndex = 1;

		if ( args.ArgC() >= 2 )
		{
			iPlayerIndex = atoi( args[1] );
		}

		CBaseMultiplayerPlayer *pPlayer = ToBaseMultiplayerPlayer( UTIL_PlayerByIndex( iPlayerIndex ) );
		if ( pPlayer && pPlayer->GetPerLifeCounterKeys() )
		{
			CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );
			pPlayer->GetPerLifeCounterKeys()->RecursiveSaveToFile( buf, 0 );

			char szBuf[1024];

			// probably not the best way to print out a CUtlBuffer
			int pos = 0;
			while ( buf.PeekStringLength() )
			{
				szBuf[pos] = buf.GetChar();
				pos++;
			}
			szBuf[pos] = '\0';

			Msg( "%s\n", szBuf );
		}
	}
	ConCommand dump_achievement_counters( "dump_achievement_counters", DumpAchievementCounters, "Spew the per-life achievement counters for multiplayer players", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

#endif // _DEBUG

int	CBaseMultiplayerPlayer::GetPerLifeCounterKV( const char *name )
{
	return m_pAchievementKV->GetInt( name, 0 );
}

void CBaseMultiplayerPlayer::SetPerLifeCounterKV( const char *name, int value )
{
	m_pAchievementKV->SetInt( name, value );
}

void CBaseMultiplayerPlayer::ResetPerLifeCounters( void )
{
	m_pAchievementKV->Clear();
}


ConVar tf_escort_score_rate( "tf_escort_score_rate", "1", FCVAR_CHEAT, "Score for escorting the train, in points per second" );

#define ESCORT_SCORE_CONTEXT		"AreaScoreContext"
#define ESCORT_SCORE_INTERVAL		0.1

//-----------------------------------------------------------------------------
// Purpose: think to accumulate and award points for escorting the train
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::EscortScoringThink( void )
{
	m_flAreaCaptureScoreAccumulator += ESCORT_SCORE_INTERVAL;

	if ( m_flCapPointScoreRate > 0 )
	{
		float flTimeForOnePoint = 1.0f / m_flCapPointScoreRate; 

		int iPoints = 0;

		while ( m_flAreaCaptureScoreAccumulator >= flTimeForOnePoint )
		{
			m_flAreaCaptureScoreAccumulator -= flTimeForOnePoint;
			iPoints++;
		}

		if ( iPoints > 0 )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "player_escort_score" );
			if ( event )
			{
				event->SetInt( "player", entindex() );
				event->SetInt( "points", iPoints );
				gameeventmanager->FireEvent( event, true /* only to server */ );
			}
		}
	}

	SetContextThink( &CBaseMultiplayerPlayer::EscortScoringThink, gpGlobals->curtime + ESCORT_SCORE_INTERVAL, ESCORT_SCORE_CONTEXT );	
}

//-----------------------------------------------------------------------------
// Purpose: We're escorting the train, start giving points
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::StartScoringEscortPoints( float flRate )
{
	Assert( flRate > 0.0f );
	m_flCapPointScoreRate = flRate;
	SetContextThink( &CBaseMultiplayerPlayer::EscortScoringThink, gpGlobals->curtime + ESCORT_SCORE_INTERVAL, ESCORT_SCORE_CONTEXT );
}

//-----------------------------------------------------------------------------
// Purpose: Stopped escorting the train
//-----------------------------------------------------------------------------
void CBaseMultiplayerPlayer::StopScoringEscortPoints( void )
{
	SetContextThink( NULL, 0, ESCORT_SCORE_CONTEXT );
}

