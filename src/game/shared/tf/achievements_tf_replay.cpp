//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievementmgr.h"
#include "baseachievement.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_gamerules.h"
#include "achievements_tf.h"

//======================================================================================================================================
// REPLAY ACHIEVEMENTS
//======================================================================================================================================

class CReplayAchievement : public CBaseTFAchievement
{
public:
	virtual bool AlwaysListen() { return true; }
	virtual bool LocalPlayerCanEarn() { return true; }
	virtual bool AlwaysEnabled() { return true; }
};

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFReplay_SaveReplay : public CReplayAchievement
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		ListenForGameEvent( "replay_saved" );
		SetGoal( 1 );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "replay_saved" ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_SaveReplay, ACHIEVEMENT_TF_REPLAY_SAVE_REPLAY, "TF_REPLAY_SAVE_REPLAY", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFReplay_PerformanceMode : public CReplayAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		ListenForGameEvent( "entered_performance_mode" );
		SetGoal( 1 ); 
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "entered_performance_mode" ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_PerformanceMode, ACHIEVEMENT_TF_REPLAY_PERFORMANCE_MODE, "TF_REPLAY_PERFORMANCE_MODE", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFReplay_BrowseReplays : public CReplayAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		ListenForGameEvent( "browse_replays" );
		SetGoal( 1 );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "browse_replays" ) )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_BrowseReplays, ACHIEVEMENT_TF_REPLAY_BROWSE_REPLAYS, "TF_REPLAY_BROWSE_REPLAYS", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTFReplay_EditTime : public CReplayAchievement
{
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_EditTime, ACHIEVEMENT_TF_REPLAY_EDIT_TIME, "TF_REPLAY_EDIT_TIME", 5 );


//----------------------------------------------------------------------------------------------------------------

class CAchievementTFReplay_YouTube_Views_Tier  : public CReplayAchievement
{
public:
	void Init() 
	{
		SetFlags( ACH_SAVE_GLOBAL );
		ListenForGameEvent( "replay_youtube_stats" );
		SetStoreProgressInSteam( true );
		SetStat( "TF_REPLAY_YOUTUBE_VIEWS" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( FStrEq( event->GetName(), "replay_youtube_stats" ) )
		{
			int iCurrentCount = GetCount();
			int iNewCount = event->GetInt( "views" );
			if ( iNewCount > iCurrentCount )
			{
				IncrementCount( iNewCount - iCurrentCount );
			}			
		}
	}

	virtual bool ShouldShowProgressNotification() { return false; }
};

class CAchievementTFReplay_YouTube_Views_Tier1 : public CAchievementTFReplay_YouTube_Views_Tier
{
	DECLARE_CLASS( CAchievementTFReplay_YouTube_Views_Tier1, CAchievementTFReplay_YouTube_Views_Tier );
public:
	void Init() 
	{
		BaseClass::Init();

		SetGoal( 100 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_YouTube_Views_Tier1, ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER1, "TF_REPLAY_YOUTUBE_VIEWS_TIER1", 5 );

class CAchievementTFReplay_YouTube_Views_Tier2 : public CAchievementTFReplay_YouTube_Views_Tier
{
	DECLARE_CLASS( CAchievementTFReplay_YouTube_Views_Tier1, CAchievementTFReplay_YouTube_Views_Tier );
public:
	void Init() 
	{
		BaseClass::Init();
		SetGoal( 1000 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_YouTube_Views_Tier2, ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER2, "TF_REPLAY_YOUTUBE_VIEWS_TIER2", 5 );

class CAchievementTFReplay_YouTube_Views_Tier3 : public CAchievementTFReplay_YouTube_Views_Tier
{
	DECLARE_CLASS( CAchievementTFReplay_YouTube_Views_Tier1, CAchievementTFReplay_YouTube_Views_Tier );
public:
	void Init() 
	{
		BaseClass::Init();
		SetGoal( 10000 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_YouTube_Views_Tier3, ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_TIER3, "TF_REPLAY_YOUTUBE_VIEWS_TIER3", 5 );

class CAchievementTFReplay_YouTube_Views_Highest : public CAchievementTFReplay_YouTube_Views_Tier
{
	DECLARE_CLASS( CAchievementTFReplay_YouTube_Views_Tier1, CAchievementTFReplay_YouTube_Views_Tier );
public:
	void Init() 
	{
		BaseClass::Init();
		SetGoal( 100000 );
	}
};
DECLARE_ACHIEVEMENT( CAchievementTFReplay_YouTube_Views_Highest, ACHIEVEMENT_TF_REPLAY_YOUTUBE_VIEWS_HIGHEST, "TF_REPLAY_YOUTUBE_VIEWS_HIGHEST", 5 );

//----------------------------------------------------------------------------------------------------------------

#endif // CLIENT_DLL



