//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HUD_ITEM_PROGRESS_TRACKER_H
#define TF_HUD_ITEM_PROGRESS_TRACKER_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "hud_baseachievement_tracker.h"
#include "iachievementmgr.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "gcsdk/gcclient_sharedobjectcache.h"
#include "econ_item_inventory.h"
#include "econ_quests.h"
#include <functional>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace GCSDK;

class CQuest;
class CQuestDefinition;

class CQuestObjectiveTextPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQuestObjectiveTextPanel, vgui::EditablePanel );
public:
	CQuestObjectiveTextPanel( vgui::Panel* pParent, const char *pElementName, const QuestObjectiveInstance_t& objective, const char* pszResFileName );
	virtual ~CQuestObjectiveTextPanel() {}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	void SetProgress( Color glowColor );
	int GetContentTall() const;
	EQuestPoints GetPointsType() const { return m_objective.GetPointsType(); }
	int GetPoints() const { return m_objective.GetPoints(); }

	void SetIsValid( bool bIsValid );

	void SetDefinitions( const QuestObjectiveInstance_t& objective, const CQuestDefinition* pQuestDef );
	uint32 GetDefIndex() const { return m_objective.GetObjectiveDef()->GetDefIndex(); }
	const CQuestObjectiveDefinition* GetObjective() const { return m_objective.GetObjectiveDef(); }

	MESSAGE_FUNC( HighlightCompletion, "HighlightCompletion" );
private:

	void UpdateText();

	Label *m_pAttribDesc;
	Label *m_pAttribGlow;
	Label *m_pAttribBlur;

	Color m_enabledTextColor;
	Color m_disabledTextColor;

	QuestObjectiveInstance_t m_objective;
	const CQuestDefinition* m_pQuestDef = NULL;

	CUtlString m_strResFileName;
	bool m_bMapView = false;
};

class CQuestProgressTrackerPanel : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CQuestProgressTrackerPanel, vgui::EditablePanel );
public:

	CQuestProgressTrackerPanel( vgui::Panel* pParent,
								const char *pElementName,
								const CQuest* pQuest,
								const CQuestDefinition* pQuestDef,
								const char* pszResFile = "resource/ui/quests/QuestItemTrackerPanel_Base.res" );
	virtual ~CQuestProgressTrackerPanel();

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual int GetContentTall() const;

	virtual void OnThink() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	bool ArePointsCompleted( uint32 nIndex ) const;
	bool IsDoneProgressing() const
	{
		if ( !m_PointsBars.BIsDoneProgressing() )
			return false;

		return true;
	}

	void SetQuest( const CQuest* pQuest );
	void SetQuestDef( const CQuestDefinition* pQuestDef );
	const CQuestDefinition* GetQuestDef() const { return m_pQuestDef; }
	bool IsValidForLocalPlayer() const;

	const CUtlVector< CQuestObjectiveTextPanel* >& GetAttributePanels() const { return m_vecObjectivePanels; }

protected:
	MESSAGE_FUNC_PARAMS( UpdateStar, "UpdateStar", pParams );

	void UpdateStars();
	void CaptureProgress();
	void UpdateBars();
	void UpdateObjectives();
	bool BIsTurningIn() const { return m_bTurningIn; };

	

	vgui::Label *m_pItemName;


	const CQuest* m_pQuest;
	const CQuestDefinition* m_pQuestDef;
	CUtlVector< CQuestObjectiveTextPanel* > m_vecObjectivePanels;

	struct PointsView_t
	{
		PointsView_t();
		bool BIsDoneProgressing() const { return m_flCurrentProgress == m_flTargetProgress; }

		EditablePanel		*m_pBarBG;
		EditablePanel		*m_pBarCommitted;
		EditablePanel		*m_pBarUncommitted;
		EditablePanel		*m_pBarJustEarned;

		float m_flCurrentProgress;
		float m_flTargetProgress;
		float m_flUpdateTime;
		uint32 m_nMaxPoints;
		CUtlString m_strObjectiveTick;
		CUtlString m_strPointsComplete;
	};

	PointsView_t m_PointsBars;
	ImagePanel* m_arStarImages[ EQuestPoints_ARRAYSIZE ];
	Label* m_pPrimaryObjectiveLabel = NULL;
	Label* m_pBonusObjectiveLabel = NULL;

	float m_flLastThink;

	const char* m_pszSoundToPlay;
	int			m_nQueuedSoundPriority;
	float		m_flCurrentJustEarnedProgress;

	int m_nContentTall;

	CUtlString m_strResFile;
	CUtlString m_strItemAttributeResFile;

	CUtlVector< std::pair< float, vgui::Panel* > >m_vecScorerLabels;

	CPanelAnimationVarAliasType( int, m_nAttribYStartOffset, "attrib_y_start_offset", "5", "proportional_int");
	CPanelAnimationVarAliasType( int, m_nAttribYStep, "attrib_y_step", "0", "proportional_int");
	CPanelAnimationVarAliasType( int, m_nAttribXOffset, "attrib_x_offset", "5", "proportional_int");

	// No effects.  Meaning, don't glow, dont show call to action panels ( ie. "Press F2 to turn in!")
	CPanelAnimationVar( bool, m_bMapView, "map_view", "0" );
	CPanelAnimationVar( bool, m_bShowItemName, "show_item_name", "1" );


	CPanelAnimationVar( bool, m_bGroupBarsWithObjectives, "group_bars_with_objectives", "0" );

	CPanelAnimationVarAliasType( int, m_nBarGap, "bar_gap", "0", "proportional_int");

	CPanelAnimationVar( Color, m_clrStandardHighlight, "standard_glow_color", "QuestMap_ActiveOrange" );
	bool m_bTurningIn;
	bool m_bSuppressStarChanges = false;
};

class CHudItemAttributeTracker : public CHudElement, public EditablePanel, public ISharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CHudItemAttributeTracker, EditablePanel );
public:

	CHudItemAttributeTracker( const char *pElementName );
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
	virtual bool ShouldDraw();
	virtual void OnThink() OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE
	{
		HandleSOEvent( steamIDOwner, pObject );
	}
	virtual void PreSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE
	{
		HandleSOEvent( steamIDOwner, pObject );
	}
	virtual void PostSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE
	{
		HandleSOEvent( steamIDOwner, pObject );
	}
	virtual void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	virtual void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};

	virtual void LevelInit( void ) OVERRIDE;
	virtual void LevelShutdown( void ) OVERRIDE;

private:

	void HandleSOEvent( const CSteamID & steamIDOwner, const CSharedObject *pObject );
	bool FindTrackerForItem( const CQuest* pItem, CQuestProgressTrackerPanel** ppTracker, bool bCreateIfNotFound );

	CUtlMap< uint64, CQuestProgressTrackerPanel* > m_mapTrackers;
	EditablePanel *m_pStatusContainer;
	Label *m_pCallToActionLabel;
	Label *m_pStatusHeaderLabel;
	CPanelAnimationVarAliasType( int, m_nStatusBufferWidth, "stats_buffer_width", "0", "proportional_int");
};

int QuestSort_PointsAscending( CQuestObjectiveTextPanel* const* p1, CQuestObjectiveTextPanel* const* p2 );

#endif // TF_HUD_ITEM_PROGRESS_TRACKER_H
