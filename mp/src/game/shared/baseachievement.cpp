//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "achievementmgr.h"
#include "icommandline.h"
#ifdef CLIENT_DLL
#include "tier3/tier3.h"
#include "vgui/ILocalize.h"
#include "achievement_notification_panel.h"
#include "fmtstr.h"
#include "gamestats.h"
#endif // CLIENT_DLL

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for sorting achievements by award time
//=============================================================================

#include <vgui/ISystem.h>
#include "vgui_controls/Controls.h"

//=============================================================================
// HPE_END
//=============================================================================

CBaseAchievementHelper *CBaseAchievementHelper::s_pFirst = NULL;

BEGIN_DATADESC_NO_BASE( CBaseAchievement )
DEFINE_FIELD( m_iCount,						FIELD_INTEGER ),
END_DATADESC()

BEGIN_DATADESC( CFailableAchievement )
DEFINE_FIELD( m_bActivated,					FIELD_BOOLEAN ),
DEFINE_FIELD( m_bFailed,					FIELD_BOOLEAN ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CBaseAchievement::CBaseAchievement()
{
	m_iFlags = 0;
	m_iGoal = 0;
	m_iProgressMsgIncrement = 0;
	m_iProgressMsgMinimum = 0;
	m_iAchievementID = 0;
	m_iPointValue = 0;
	m_bHideUntilAchieved = false;
	m_bStoreProgressInSteam = false;
	m_pVictimClassNameFilter = NULL;
	m_pAttackerClassNameFilter = NULL;
	m_pInflictorClassNameFilter = NULL;
	m_pInflictorEntityNameFilter = NULL;
	m_pMapNameFilter = NULL;
	m_pGameDirFilter = NULL;
	m_pszComponentNames = NULL;
	m_pszComponentPrefix = NULL;
	m_iNumComponents = 0;
	m_iComponentPrefixLen = 0;
	m_iComponentBits = 0;
	m_iCount = 0;
	m_iProgressShown = 0;
	m_bAchieved = false;
	m_uUnlockTime = 0;
	m_pAchievementMgr = NULL;
	m_bShowOnHUD = false;
	m_pszStat = NULL;
}

CBaseAchievement::~CBaseAchievement()
{
}

//-----------------------------------------------------------------------------
// Purpose: sets flags
//-----------------------------------------------------------------------------
void CBaseAchievement::SetFlags( int iFlags ) 
{ 
	// must always specify a save method
	Assert( iFlags & ( ACH_SAVE_WITH_GAME | ACH_SAVE_GLOBAL ) );

	m_iFlags = iFlags; 
}

//-----------------------------------------------------------------------------
// Purpose: called when a game event being listened for is dispatched
//-----------------------------------------------------------------------------
void CBaseAchievement::FireGameEvent( IGameEvent *event )
{
	// perform common filtering to make it simpler to write achievements
	if ( !IsActive() )
		return;

	// if the achievement only applies to a specific map, and it's not the current map, skip it
	if ( m_pMapNameFilter && ( 0 != Q_strcmp( m_pAchievementMgr->GetMapName(), m_pMapNameFilter ) ) )
		return;

	const char *name = event->GetName();
	if ( 0 == Q_strcmp( name, "teamplay_round_win" ) )
	{
		// if this is a round win and the achievement wants full round events only, filter this out
		// if this is not the end of a full round
		if ( ( m_iFlags & ACH_FILTER_FULL_ROUND_ONLY ) && ( false == event->GetBool( "full_round" ) ) )
			return;
	}

	// let the achievement handle the event
	FireGameEvent_Internal( event );
}


//-----------------------------------------------------------------------------
// Purpose: sets victim class to filter with
//-----------------------------------------------------------------------------
void CBaseAchievement::SetVictimFilter( const char *pClassName )
{
	m_pVictimClassNameFilter = pClassName;
}

//-----------------------------------------------------------------------------
// Purpose: sets attacker class to filter with
//-----------------------------------------------------------------------------
void CBaseAchievement::SetAttackerFilter( const char *pClassName )
{
	m_pAttackerClassNameFilter = pClassName;
}

//-----------------------------------------------------------------------------
// Purpose: sets inflictor class to filter with
//-----------------------------------------------------------------------------
void CBaseAchievement::SetInflictorFilter( const char *pClassName )
{
	m_pInflictorClassNameFilter = pClassName;
}

//-----------------------------------------------------------------------------
// Purpose: sets inflictor entity name to filter with
//-----------------------------------------------------------------------------
void CBaseAchievement::SetInflictorEntityNameFilter( const char *pEntityName )
{
	m_pInflictorEntityNameFilter = pEntityName;
}

//-----------------------------------------------------------------------------
// Purpose: sets map name to filter with
//-----------------------------------------------------------------------------
void CBaseAchievement::SetMapNameFilter( const char *pMapName )
{
	m_pMapNameFilter = pMapName;
}

//-----------------------------------------------------------------------------
// Purpose: sets game dir to filter with.  Note: in general, achievements should
//			only be compiled into products they pertain to.  But if there are
//			any game-specific achievements which need to be in a binary shared
//			across products (e.g. Ep1 & Ep2), use the game dir as a runtime
//			filter.
//-----------------------------------------------------------------------------
void CBaseAchievement::SetGameDirFilter( const char *pGameDir )
{
	m_pGameDirFilter = pGameDir;
}

//-----------------------------------------------------------------------------
// Purpose: sets prefix to look for in map event string to identify a component
//			for this achievement
//-----------------------------------------------------------------------------
void CBaseAchievement::SetComponentPrefix( const char *pPrefix )
{
	m_pszComponentPrefix = pPrefix;
	m_iComponentPrefixLen = Q_strlen( pPrefix );
}

//-----------------------------------------------------------------------------
// Purpose: called when a kill that passes filter critera occurs.  This
//			is the default implementation, achievements can override to
//			do special handling
//-----------------------------------------------------------------------------
void CBaseAchievement::Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
{
	// extra paranoid check: should only get here if registered as a kill event listener
	Assert( GetFlags() & ACH_LISTEN_KILL_EVENTS );
	if ( !( GetFlags() & ACH_LISTEN_KILL_EVENTS ) )
		return;

	// default implementation is just to increase count when filter criteria pass
	// Msg( "Base achievement incremented on kill event.\n" );
	IncrementCount();
}

//-----------------------------------------------------------------------------
// Purpose: called when an event that counts toward an achievement occurs
//-----------------------------------------------------------------------------
void CBaseAchievement::IncrementCount( int iOptIncrement )
{
	if ( !IsAchieved() && LocalPlayerCanEarn() )
	{
		if ( !AlwaysEnabled() && !m_pAchievementMgr->CheckAchievementsEnabled() )
		{
			Msg( "Achievements disabled, ignoring achievement progress for %s\n", GetName() );
			return;
		}

		// on client, where the count is kept, increment count
		if ( iOptIncrement > 0 )
		{
			// user specified that we want to increase by more than one.
			m_iCount += iOptIncrement;
			if ( m_iCount > m_iGoal )
			{
				m_iCount = m_iGoal;
			}
		}
		else
		{
			m_iCount++;
		}

		// if this achievement gets saved w/global state, flag our global state as dirty
		if ( GetFlags() & ACH_SAVE_GLOBAL )
		{
			m_pAchievementMgr->SetDirty( true );
		}

		if ( cc_achievement_debug.GetInt() )
		{
			Msg( "Achievement count increased for %s: %d/%d\n", GetName(), m_iCount, m_iGoal );
		}

#ifndef NO_STEAM
		// if this achievement's progress should be stored in Steam, set the steam stat for it
		if ( StoreProgressInSteam() && steamapicontext->SteamUserStats() )
		{
			// Set the Steam stat with the same name as the achievement.  Only cached locally until we upload it.
			char pszProgressName[1024];
			Q_snprintf( pszProgressName, 1024, "%s_STAT", GetStat() );
			bool bRet = steamapicontext->SteamUserStats()->SetStat( pszProgressName, m_iCount );
			if ( !bRet )
			{
				DevMsg( "ISteamUserStats::GetStat failed to set progress value in Steam for achievement %s\n", pszProgressName );
			}

			m_pAchievementMgr->SetDirty( true );
		}
#endif
		// if we've hit goal, award the achievement
		if ( m_iGoal > 0 )
		{
			if ( m_iCount >= m_iGoal )
			{
				AwardAchievement();
			}
			else
			{	
				HandleProgressUpdate();
			}
		}

	}
}

void CBaseAchievement::SetShowOnHUD( bool bShow )
{
 	if ( m_bShowOnHUD != bShow )
	{
 		m_pAchievementMgr->SetDirty( true );
	}

	m_bShowOnHUD = bShow;
}

void CBaseAchievement::HandleProgressUpdate()
{
	// if we've hit the right # of progress steps to show a progress notification, show it
	if ( ( m_iProgressMsgIncrement > 0 ) && m_iCount >= m_iProgressMsgMinimum && ( 0 == ( m_iCount % m_iProgressMsgIncrement ) ) )
	{
		// which notification is this
		int iProgress = m_iCount / m_iProgressMsgIncrement;
		// if we haven't already shown this progress step, show it
		if ( iProgress > m_iProgressShown )
		{
			ShowProgressNotification();
			// remember progress step shown so we don't show it again if the player loads an earlier save game
			// and gets past this point again
			m_iProgressShown = iProgress;
			m_pAchievementMgr->SetDirty( true );
		}					
	}
}

//-----------------------------------------------------------------------------
// Purpose: calculates at how many steps we should show a progress notification
//-----------------------------------------------------------------------------
void CBaseAchievement::CalcProgressMsgIncrement()
{
	// by default, show progress at every 25%
	m_iProgressMsgIncrement = m_iGoal / 4;
	// if goal is not evenly divisible by 4, try some other values
	if ( 0 != ( m_iGoal % 4 ) )
	{
		if ( 0 == ( m_iGoal % 3 ) )
		{
			// if evenly divisible by 3, use that
			m_iProgressMsgIncrement = m_iGoal / 3;
		}
		else if ( 0 == ( m_iGoal % 5 ) )
		{
			// if evenly divisible by 5, use that
			m_iProgressMsgIncrement = m_iGoal / 5;
		}
		// otherwise stick with divided by 4, rounded off
	}

	// don't show progress notifications for less than 5 things
	if ( m_iProgressMsgIncrement < 5 )
	{
		m_iProgressMsgIncrement = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAchievement::SetNextThink( float flThinkTime ) 
{ 
	m_pAchievementMgr->SetAchievementThink( this, flThinkTime ); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseAchievement::ClearThink( void ) 
{ 
	m_pAchievementMgr->SetAchievementThink( this, THINK_CLEAR ); 
}

//-----------------------------------------------------------------------------
// Purpose: see if we should award an achievement based on what just happened
//-----------------------------------------------------------------------------
void CBaseAchievement::EvaluateNewAchievement()
{
	if ( !IsAchieved() && m_iGoal > 0 && m_iCount >= m_iGoal )
	{
		AwardAchievement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: determine if we should set this achievement to be achieved based
//			on other state.  Used at init time.
//-----------------------------------------------------------------------------
void CBaseAchievement::EvaluateIsAlreadyAchieved()
{
	if ( !IsAchieved() && m_iGoal > 0 && m_iCount >= m_iGoal )
	{
		m_bAchieved = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: called a map event for this achievement occurs
//-----------------------------------------------------------------------------
void CBaseAchievement::OnMapEvent( const char *pEventName )
{
	Assert( m_iFlags & ACH_LISTEN_MAP_EVENTS );

	if ( 0 == Q_stricmp( pEventName, GetName() ) )
	{
		IncrementCount();
	}
}

//-----------------------------------------------------------------------------
// Purpose: called when an achievement is awarded
//-----------------------------------------------------------------------------
void CBaseAchievement::AwardAchievement()
{
	Assert( !IsAchieved() );
	if ( IsAchieved() )
		return;

	m_pAchievementMgr->AwardAchievement( m_iAchievementID );
}

//-----------------------------------------------------------------------------
// Purpose: called when a component of a multi-component event is found
//-----------------------------------------------------------------------------
void CBaseAchievement::OnComponentEvent( const char *pchComponentName )
{
	// find the component name in our list
	for ( int i = 0; i < m_iNumComponents; i++ )
	{
		if ( 0 == Q_strcmp( pchComponentName, m_pszComponentNames[i] ) )
		{
			EnsureComponentBitSetAndEvaluate( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the specified component bit # if it is not already.
//			If it does get set, evaluate if this satisfies an achievement
//-----------------------------------------------------------------------------
void CBaseAchievement::EnsureComponentBitSetAndEvaluate( int iBitNumber )
{
	Assert( iBitNumber < 64 );	// this is bit #, not a bit mask

	if ( IsAchieved() )
		return;

	// calculate which bit this component corresponds to
	uint64 iBitMask = ( (uint64) 1 ) << iBitNumber;

	// see if we already have gotten this component
	if ( 0 == ( iBitMask & m_iComponentBits ) )
	{				
		if ( !AlwaysEnabled() && !m_pAchievementMgr->CheckAchievementsEnabled() )
		{
			Msg( "Achievements disabled, ignoring achievement component for %s\n", GetName() );
			return;
		}

		// new component, set the bit and increment the count
		SetComponentBits( m_iComponentBits | iBitMask );
		if ( m_iCount != m_iGoal )
		{
			// save our state at the next good opportunity
			m_pAchievementMgr->SetDirty( true );		

			if ( cc_achievement_debug.GetInt() )
			{
				Msg( "Component %d for achievement %s found\n", iBitNumber, GetName() );
			}

			ShowProgressNotification();
		}				
	}
	else
	{
		if ( cc_achievement_debug.GetInt() )
		{
			Msg( "Component %d for achievement %s found, but already had that component\n", iBitNumber, GetName() );
		}
	}

	// Check to see if we've achieved our goal even if the bit is already set 
	// (this fixes some older achievements that are stuck in the 9/9 state and could never be evaluated)
	Assert( m_iCount <= m_iGoal );
	if ( m_iCount == m_iGoal )
	{
		// all components found, award the achievement (and save state)
		AwardAchievement();
	}
}

//-----------------------------------------------------------------------------
// Purpose: displays achievement progress notification in the HUD
//-----------------------------------------------------------------------------
void CBaseAchievement::ShowProgressNotification()
{
	if ( !ShouldShowProgressNotification() )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "achievement_event" );
	if ( event )
	{
		event->SetString( "achievement_name", GetName() );
		event->SetInt( "cur_val", m_iCount );
		event->SetInt( "max_val", m_iGoal );
#ifdef GAME_DLL
		gameeventmanager->FireEvent( event );
#else
		gameeventmanager->FireEventClientSide( event );
#endif
	}	
}

//-----------------------------------------------------------------------------
// Purpose: clears dynamic state for this achievement
//-----------------------------------------------------------------------------
void CBaseAchievement::PreRestoreSavedGame()
{
	// if this achievement gets saved with the game, clear its state
	if ( m_iFlags & ACH_SAVE_WITH_GAME )
	{
		m_iCount = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: called after the data in this achievement has been restored from saved game
//-----------------------------------------------------------------------------
void CBaseAchievement::PostRestoreSavedGame()
{
	EvaluateIsAlreadyAchieved();
}

//-----------------------------------------------------------------------------
// Purpose: sets component bits for this achievement
//-----------------------------------------------------------------------------
void CBaseAchievement::SetComponentBits( uint64 iComponentBits ) 
{ 
	Assert( m_iFlags & ACH_HAS_COMPONENTS );
	// set the bit field
	m_iComponentBits = iComponentBits; 
	// count how many bits are set and save that as the count
	int iNumBitsSet = 0;
	while ( iComponentBits > 0 )
	{
		if ( iComponentBits & 1 )
		{
			iNumBitsSet++;
		}
		iComponentBits >>= 1;
	}
	m_iCount = iNumBitsSet;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether we should save this achievement with a save game
//-----------------------------------------------------------------------------
bool CBaseAchievement::ShouldSaveWithGame() 
{ 
	// save if we should get saved with the game, have a non-zero count, and have not
	// been achieved (at which point the achievement state gets saved globally)
	return ( ( m_iFlags & ACH_SAVE_WITH_GAME ) > 0 && ( GetCount() > 0 ) && !IsAchieved() );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether we should save this achievement to the global file
//-----------------------------------------------------------------------------
bool CBaseAchievement::ShouldSaveGlobal() 
{ 
	// save if we should get saved globally and have a non-zero count, or if we have been achieved, or if the player has pinned this achievement to the HUD
	return ( ( ( m_iFlags & ACH_SAVE_GLOBAL ) > 0 && ( GetCount() > 0 ) ) || IsAchieved() || ( m_iProgressShown > 0 ) || ShouldShowOnHUD() );
}

//-----------------------------------------------------------------------------
// Purpose: returns whether this achievement is active
//-----------------------------------------------------------------------------
bool CBaseAchievement::IsActive()
{
	// we're not active if already achieved
	if ( IsAchieved() )	
		return false;
	
	// if there's a map filter and we're not on the specified map, we're not active
	if ( ( m_pMapNameFilter ) && ( 0 != Q_strcmp( m_pAchievementMgr->GetMapName(), m_pMapNameFilter ) ) )
		return false;

	return true;
}

//=============================================================================
// HPE_BEGIN:
// [pfreese] Moved serialization from AchievementMgr to here so that derived
// classes can persist additional data
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Serialize our data to the KeyValues node
//-----------------------------------------------------------------------------
void CBaseAchievement::GetSettings( KeyValues *pNodeOut )
{
	pNodeOut->SetInt( "value", IsAchieved() ? 1 : 0 );

	if ( HasComponents() )
	{
		pNodeOut->SetUint64( "data", m_iComponentBits );
	}
	else
	{
		if ( !IsAchieved() )
		{
			pNodeOut->SetInt( "data", m_iCount );
		}
	}
	pNodeOut->SetInt( "hud", ShouldShowOnHUD() ? 1 : 0 );
	pNodeOut->SetInt( "msg", m_iProgressShown );
}

//-----------------------------------------------------------------------------
// Purpose: Unserialize our data from the KeyValues node
//-----------------------------------------------------------------------------
void CBaseAchievement::ApplySettings( KeyValues *pNodeIn )
{
	// set the count
	if ( pNodeIn->GetInt( "value" ) > 0 )
	{
		m_iCount = m_iGoal;
		m_bAchieved = true;
	}
	else if ( !HasComponents() )
	{
		m_iCount = pNodeIn->GetInt( "data" );
	}

	// if this achievement has components, set the component bits
	if ( HasComponents() )
	{
		int64 iComponentBits = pNodeIn->GetUint64( "data" );
		SetComponentBits( iComponentBits );
	}
	SetShowOnHUD( !!pNodeIn->GetInt( "hud" ) );
	m_iProgressShown = pNodeIn->GetInt( "msg" );
}

//=============================================================================
// HPE_END
//=============================================================================

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CFailableAchievement::CFailableAchievement() : CBaseAchievement()
{
	m_bFailed = false;
	m_bActivated = false;
}

//-----------------------------------------------------------------------------
// Purpose: returns whether we should save this achievement with a save game
//-----------------------------------------------------------------------------
bool CFailableAchievement::ShouldSaveWithGame() 
{ 
	// save if we should get saved with the game, and are active or have failed
	return ( ( ( m_iFlags & ACH_SAVE_WITH_GAME ) > 0 ) && ( m_bActivated || m_bFailed ) );
}

//-----------------------------------------------------------------------------
// Purpose: clears dynamic state for this achievement
//-----------------------------------------------------------------------------
void CFailableAchievement::PreRestoreSavedGame() 
{ 
	m_bFailed = false;
	m_bActivated = false;

	BaseClass::PreRestoreSavedGame();
}


//-----------------------------------------------------------------------------
// Purpose: called after the data in this achievement has been restored from saved game
//-----------------------------------------------------------------------------
void CFailableAchievement::PostRestoreSavedGame()
{
	// if there is no activation event set for this achievement, it is always active, activate it now
	if ( !m_bFailed && !GetActivationEventName()[0] )
	{
		m_bActivated = true;
	}

	if ( m_bActivated )
	{
		Activate();
	}

	BaseClass::PostRestoreSavedGame();
}

//-----------------------------------------------------------------------------
// Purpose: called when a map event occurs
//-----------------------------------------------------------------------------
void CFailableAchievement::OnMapEvent( const char *pEventName )
{
	// if we're not activated and we got the activation event, activate
	if ( !m_bActivated && ( 0 == Q_stricmp( pEventName, GetActivationEventName() ) ) )
	{
		OnActivationEvent();
	}
	// if this is the evaluation event, see if we've failed or not
	else if ( m_bActivated && 0 == Q_stricmp( pEventName, GetEvaluationEventName() ) )
	{
		OnEvaluationEvent();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when this failable achievement is activated
//-----------------------------------------------------------------------------
void CFailableAchievement::Activate()
{
	m_bActivated = true;
	ListenForEvents();
	if ( cc_achievement_debug.GetInt() )
	{
		Msg( "Failable achievement %s now active\n", GetName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when this failable achievement should be evaluated
//-----------------------------------------------------------------------------
void CFailableAchievement::OnEvaluationEvent()
{
	if ( !m_bFailed )
	{
		// we haven't failed and we reached the evaluation point, we've succeeded
		IncrementCount();
	}
	
	if ( cc_achievement_debug.GetInt() )
	{
		Msg( "Failable achievement %s has been evaluated (%s), now inactive\n", GetName(), m_bFailed ? "FAILED" : "AWARDED" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Sets this achievement to failed
//-----------------------------------------------------------------------------
void CFailableAchievement::SetFailed()
{
	if ( !m_bFailed )
	{
		m_bFailed = true;

		if ( cc_achievement_debug.GetInt() )
		{
			Msg( "Achievement failed: %s (%s)\n", GetName(), GetName() );
		}	
	}	
}

//===========================================

void CAchievement_AchievedCount::Init() 
{
	SetFlags( ACH_SAVE_GLOBAL );
	SetGoal( 1 );
	SetAchievementsRequired( 0, 0, 0 );
}

// Count how many achievements have been earned in our range
void CAchievement_AchievedCount::OnSteamUserStatsStored( void )
{
	// DO NO CALL. REPLACED BY CHECKMETAACHIEVEMENTS!
	Assert( 0 );
}

void CAchievement_AchievedCount::SetAchievementsRequired( int iNumRequired, int iLowRange, int iHighRange )
{
	m_iNumRequired = iNumRequired;
	m_iLowRange = iLowRange;
	m_iHighRange = iHighRange;
}