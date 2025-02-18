//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "hud_baseachievement_tracker.h"
#include "c_tf_player.h"
#include "iachievementmgr.h"
#include "achievementmgr.h"
#include "hud_vote.h"
#include "baseachievement.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar hud_achievement_count_engineer( "hud_achievement_count_engineer", "3", FCVAR_ARCHIVE, "Max number of achievements that can be shown on the HUD when you're an engineer" );

class CHudAchievementTracker : public CHudBaseAchievementTracker
{
	DECLARE_CLASS_SIMPLE( CHudAchievementTracker, CHudBaseAchievementTracker );

public:
	CHudAchievementTracker( const char *pElementName );
	virtual void OnThink();
	virtual void PerformLayout();
	virtual int  GetMaxAchievementsShown();
	virtual bool ShouldShowAchievement( IAchievement *pAchievement );
	virtual bool ShouldDraw();

private:
	int m_iPlayerClass;
	CPanelAnimationVarAliasType( int, m_iNormalY, "NormalY", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEngineerY, "EngineerY", "170", "proportional_int" );
};

DECLARE_HUDELEMENT( CHudAchievementTracker );


CHudAchievementTracker::CHudAchievementTracker( const char *pElementName ) : BaseClass( pElementName )
{
	m_iPlayerClass = -1;
}

// layout panel again if player class changes
void CHudAchievementTracker::OnThink()
{
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFPlayerClass *pClass = pPlayer->GetPlayerClass();
		if ( pClass && m_iPlayerClass != pClass->GetClassIndex() )
		{
			InvalidateLayout();
			m_iPlayerClass = pClass->GetClassIndex();
			m_flNextThink = gpGlobals->curtime - 0.1f;
		}
	}
	
	BaseClass::OnThink();
}

// Show less achievements on the HUD for the engineer
int CHudAchievementTracker::GetMaxAchievementsShown()
{
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		return hud_achievement_count_engineer.GetInt();
	}
	return BaseClass::GetMaxAchievementsShown();
}

// shift panel down for the engineer
void CHudAchievementTracker::PerformLayout()
{
	BaseClass::PerformLayout();

	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	int x, y;
	GetPos( x, y );
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{		
		SetPos( x, m_iEngineerY );
	}
	else
	{
		SetPos( x, m_iNormalY );
	}
}

bool CHudAchievementTracker::ShouldShowAchievement( IAchievement *pAchievement )
{
	if ( !BaseClass::ShouldShowAchievement( pAchievement ) )
		return false;
	
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	// filter out class specific achievements
	int id = pAchievement->GetAchievementID();
	if ( id >= ACHIEVEMENT_TF_MEDIC_START_RANGE && id <= ACHIEVEMENT_TF_MEDIC_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_PYRO_START_RANGE && id <= ACHIEVEMENT_TF_PYRO_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_HEAVY_START_RANGE && id <= ACHIEVEMENT_TF_HEAVY_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_SCOUT_START_RANGE && id <= ACHIEVEMENT_TF_SCOUT_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_SNIPER_START_RANGE && id <= ACHIEVEMENT_TF_SNIPER_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_SPY_START_RANGE && id <= ACHIEVEMENT_TF_SPY_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_SOLDIER_START_RANGE && id <= ACHIEVEMENT_TF_SOLDIER_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_DEMOMAN_START_RANGE && id <= ACHIEVEMENT_TF_DEMOMAN_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
			return false;
	}
	else if ( id >= ACHIEVEMENT_TF_ENGINEER_START_RANGE && id <= ACHIEVEMENT_TF_ENGINEER_END_RANGE )
	{
		if ( !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
			return false;
	}

	CBaseAchievement *pBaseAchievement = dynamic_cast< CBaseAchievement * >( pAchievement );
	if ( pBaseAchievement && pBaseAchievement->GetMapNameFilter() && pBaseAchievement->GetAchievementMgr() )
	{
		if ( Q_strcmp( pBaseAchievement->GetAchievementMgr()->GetMapName(), pBaseAchievement->GetMapNameFilter() ) != 0 )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudAchievementTracker::ShouldDraw()
{
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
		if ( pHudVote && pHudVote->ShouldDraw() )
			return false;
	}

	return BaseClass::ShouldDraw();
}