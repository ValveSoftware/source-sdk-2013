//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#ifndef IACHIEVEMENTMGR_H
#define IACHIEVEMENTMGR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlmap.h"
#include "vgui_controls/Panel.h"

class CBaseAchievement;

abstract_class IAchievement
{
public:
	virtual int GetAchievementID() = 0;
	virtual const char *GetName() = 0;
	virtual int GetFlags() = 0;
	virtual int GetGoal() = 0;
	virtual int GetCount() = 0;
	virtual bool IsAchieved() = 0;
	virtual int GetPointValue() = 0;
	virtual bool ShouldSaveWithGame() = 0;
	virtual bool ShouldHideUntilAchieved() = 0;
	virtual bool ShouldShowOnHUD() = 0;
	virtual void SetShowOnHUD( bool bShow ) = 0;
};


abstract_class IAchievementMgr
{
public:
	virtual IAchievement* GetAchievementByIndex( int index ) = 0;
	virtual CBaseAchievement* GetAchievementByID ( int id ) = 0;
	virtual int GetAchievementCount() = 0;
	virtual void InitializeAchievements() = 0;
	virtual void AwardAchievement( int iAchievementID ) = 0;
	virtual void OnMapEvent( const char *pchEventName ) = 0;
	virtual void DownloadUserData() = 0;
	virtual void EnsureGlobalStateLoaded() = 0;
	virtual void SaveGlobalStateIfDirty( bool bAsync ) = 0;
	virtual bool HasAchieved( const char *pchName ) = 0;
	virtual bool WereCheatsEverOn() = 0;
};

// flags for IAchievement::GetFlags

#define ACH_LISTEN_KILL_EVENTS				0x0001
#define ACH_LISTEN_MAP_EVENTS				0x0002
#define ACH_LISTEN_COMPONENT_EVENTS			0x0004
#define ACH_HAS_COMPONENTS					0x0020
#define ACH_SAVE_WITH_GAME					0x0040
#define ACH_SAVE_GLOBAL						0x0080
#define ACH_FILTER_ATTACKER_IS_PLAYER		0x0100
#define ACH_FILTER_VICTIM_IS_PLAYER_ENEMY	0x0200
#define ACH_FILTER_FULL_ROUND_ONLY			0x0400

#define ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS		ACH_LISTEN_KILL_EVENTS | ACH_FILTER_ATTACKER_IS_PLAYER | ACH_FILTER_VICTIM_IS_PLAYER_ENEMY
#define ACH_LISTEN_KILL_ENEMY_EVENTS		ACH_LISTEN_KILL_EVENTS | ACH_FILTER_VICTIM_IS_PLAYER_ENEMY

// Update this for changes in either abstract class in this file
#define ACHIEVEMENTMGR_INTERFACE_VERSION "ACHIEVEMENTMGR_INTERFACE_VERSION001"

#define ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( name ) \
	( g_pVGuiLocalize->Find( CFmtStr( "#%s_NAME", name ) ) )

#define ACHIEVEMENT_LOCALIZED_NAME( pAchievement ) \
	( ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pAchievement->GetName() ) )

#define ACHIEVEMENT_LOCALIZED_DESC_FROM_STR( name ) \
	( g_pVGuiLocalize->Find( CFmtStr( "#%s_DESC", name ) ) )

#define ACHIEVEMENT_LOCALIZED_DESC( pAchievement ) \
	( ACHIEVEMENT_LOCALIZED_DESC_FROM_STR( pAchievement->GetName() ) )

#endif // IACHIEVEMENTMGR_H
