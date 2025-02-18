//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_QUEST_MAP_NODE_VIEW_PANEL_H
#define TF_QUEST_MAP_NODE_VIEW_PANEL_H


#include "vgui_controls/EditablePanel.h"
#include "local_steam_shared_object_listener.h"
#include "tf_gcmessages.h"
#include "tf_controls.h"

using namespace vgui;
using namespace GCSDK;

class CItemModelPanelToolTip;
class CQuestProgressTrackerPanel;
class CItemModelPanel;
class CQuestMapNodePanel;
class CQuestViewSubPanel;

class CQuestObjectivePanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQuestObjectivePanel, EditablePanel );
public:
	CQuestObjectivePanel( Panel* pParent, const char* pszPanelname );

	virtual void PerformLayout() OVERRIDE;

	void SetQuestData( const CQuest* pQuest, const CQuestDefinition* pQuestDef );

private:

	CQuestProgressTrackerPanel*	m_pItemTrackerPanel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CQuestObjectiveTooltip : public vgui::BaseTooltip 
{
	DECLARE_CLASS_SIMPLE( CQuestObjectiveTooltip, vgui::BaseTooltip );
public:
	CQuestObjectiveTooltip(vgui::Panel *parent, const char *text = NULL);

	void SetText(const char *text) { return; }
	const char *GetText() { return NULL; }

	virtual void PerformLayout();
	virtual void ShowTooltip( vgui::Panel *currentPanel );
	virtual void HideTooltip();

	void SetObjectivePanel( CQuestObjectivePanel* pObjectivePanel ) { m_pObjectivePanel = pObjectivePanel; }

private:

	CQuestObjectivePanel	*m_pObjectivePanel;	// This is the tooltip panel we make visible. Must be a CQuestObjectivePanel.
	vgui::DHANDLE<CQuestViewSubPanel> m_hCurrentPanel;
};


class CQuestViewSubPanel : public CExpandablePanel
						 , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CQuestViewSubPanel, CExpandablePanel );
public:
	CQuestViewSubPanel( Panel* pParent,
						const char* pszPanelName,
						const CQuest* pQuest,
						const CQuestDefinition* pQuestDef,
						const CSOQuestMapNode& msgNodeData );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnSizeChanged( int nWide, int nTall ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	const CQuestDefinition* GetQuestDefindex() const { return m_pQuestDef; }
	void SetQuestData( const CQuest* pQuest, const CQuestDefinition* pQuestDef, bool bShowObjectives );
	void SetNodeData( const CSOQuestMapNode& msgNodeData );
	void SetTextTooltip( CTFTextToolTip* pTextTooltip ) { m_pToolTip = pTextTooltip; }
	void SetObjectiveTooltip( CQuestObjectiveTooltip* pObjectiveTooltip ) { m_pObjectiveTooltip = pObjectiveTooltip; }

	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	void OnConfirmSelectQuest();

	MESSAGE_FUNC( OnShowTurnInSuccess, "ShowTurnInSuccess" );

private:
	void HideSelectQuestInfo();
	void BeginTurnInAnimation();

	EditablePanel* m_pAcceptTooltipHack;
	CExButton* m_pActivateButton;
	CTFTextToolTip* m_pToolTip;
	ImagePanel* m_pBGImage;
	ImagePanel* m_pInfo;

	// Turn in
	EditablePanel* m_pTurnInContainer;

	bool m_bTurningIn = false;
	bool m_bShowObjectives = false;
	CSOQuestMapNode m_msgNodeData;
	const CQuest* m_pQuest;
	const CQuestDefinition* m_pQuestDef;
	CQuestObjectivePanel* m_pObjectivePanel;
	CQuestObjectiveTooltip* m_pObjectiveTooltip;
};


//-----------------------------------------------------------------------------
// Purpose: The view of a quest node with inspected
//-----------------------------------------------------------------------------
class CQuestNodeViewPanel : public CExpandablePanel
						  , public CLocalSteamSharedObjectListener
						  , public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CQuestNodeViewPanel, CExpandablePanel );
	CQuestNodeViewPanel( Panel *pParent, const char *pszPanelname );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *pCommand ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void Paint() OVERRIDE;
	virtual void SetVisible( bool bVisible ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	void SetItemModelPanelTooltip( CItemModelPanelToolTip* pItemTooltip ) { m_pItemToolTip = pItemTooltip; }
	void SetTextTooltip( CTFTextToolTip* pTooltip );
	void SetObjectiveTooltip( CQuestObjectiveTooltip* pObjectiveTooltip );
	void SetData( const CSOQuestMapNode& msgNodeData );
	const CSOQuestMapNode GetData() const { return m_msgNodeData; }

	void UpdateQuestViewPanelForNode( CQuestMapNodePanel* pNodePanel );

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE;

	MESSAGE_FUNC_PARAMS( QuestClicked, "QuestClicked", pParams );
	MESSAGE_FUNC( TurnInComplete, "TurnInComplete" );
private:

	void UpdatePosition();
	void UpdateQuestSubPanels();
	void UpdateHeights();

	//
	// Animation things
	//
	void SelectedQuestArrived();

	CSOQuestMapNode m_msgPreTurnInSoData;
	CSOQuestMapNode m_msgNodeData;
	
	Panel*							m_pNodeStateBorder;
	Label*							m_pTitleLabel;
	EditablePanel*					m_pContentContainer;
	EditablePanel*					m_pRewardsContainer;
	CUtlVector< CQuestViewSubPanel*	> m_vecQuestSubPanels;
	CItemModelPanelToolTip*			m_pItemToolTip;
	CItemModelPanel*				m_pRewardItemPanel;
	CTFTextToolTip*					m_pToolTip;
	ImagePanel*						m_pContractsInfo;
	ImagePanel*						m_pRewardsInfo;
	bool m_bQuestCreated;
	bool m_bHasItemsToShow = false;
	bool m_bTurningIn = false;

	int m_nNodePanelX;
	int m_nNodePanelY;
	int m_nNodePanelWide;
	int m_nNodePanelTall;

};

#endif //TF_QUEST_MAP_NODE_VIEW_PANEL_H
