//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef QUEST_LOG_PANEL_H
#define QUEST_LOG_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#if 0
#include "vgui_controls/EditablePanel.h"
#include <game/client/iviewport.h>
#include "quest_item_panel.h"
#include "local_steam_shared_object_listener.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Creates the quest log if it doesnt exists, and gives you a pointer to it
//-----------------------------------------------------------------------------
class CQuestLogPanel *GetQuestLog();

//-----------------------------------------------------------------------------
// A scrollable list of quest items
//-----------------------------------------------------------------------------
class CScrollableQuestList : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CScrollableQuestList, EditablePanel );
public:
	CScrollableQuestList( Panel *parent, const char *pszPanelName );
	virtual ~CScrollableQuestList();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	void DirtyQuestLayout() { m_bQuestsLayoutDirty = true; }
	void PopulateQuestLists();
	void QuestCompletedResponse();
	bool AnyQuestItemPanelsInState( CQuestItemPanel::EItemPanelState_t eState ) const;
	void PositionQuestItemPanels();

	void SetSelected( CQuestItemPanel *pItem, bool bImmediately );
	void SetCompletingPanel( const CQuestItemPanel *pItem ) { m_pCompletingPanel = pItem; }
	const CQuestItemPanel *GetCompletingPanel() const { return m_pCompletingPanel; }
	void UpdateEmptyMessage();

protected:

	bool m_bQuestsLayoutDirty;
	EditablePanel *m_pContainer;
	CUtlVector< CQuestItemPanel* > m_vecQuestItemPanels;
	CQuestItemPanel* m_spCompletingPanel;
	const CQuestItemPanel* m_pCompletingPanel; 

	CUtlString m_pszNoQuests;
	CUtlString m_pszNeedAPass;
	CUtlString m_pszNotPossible;

	CPanelAnimationVarAliasType( int, m_iEntryStep, "entry_step", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEntryStartingX, "entry_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEntryStartingY, "entry_y", "0", "proportional_int" );
};

//-----------------------------------------------------------------------------
// The default quest log panel
//-----------------------------------------------------------------------------
class CQuestLogPanel 
	: public EditablePanel
	, public IViewPortPanel
	, public CGameEventListener
	, public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CQuestLogPanel, EditablePanel );
public:
	CQuestLogPanel( IViewPort *pViewPort );
	virtual ~CQuestLogPanel();

	void AttachToGameUI();
	virtual const char *GetName( void ) OVERRIDE;
	virtual void SetData( KeyValues *data ) OVERRIDE {}
	virtual void Reset() OVERRIDE { Update(); SetVisible( true ); }
	virtual void Update() OVERRIDE { return; }
	virtual bool NeedsUpdate( void ) OVERRIDE { return false; }
	virtual bool HasInputElements( void ) OVERRIDE { return true; }
	virtual void ShowPanel( bool bShow ) OVERRIDE;

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible() OVERRIDE { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) OVERRIDE { BaseClass::SetParent( parent ); }

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *pCommand ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void SetVisible( bool bState ) OVERRIDE;
	virtual void OnKeyCodePressed( KeyCode code ) OVERRIDE;
	virtual void OnKeyCodeTyped(KeyCode code) OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

	void QuestCompletedResponse();
	void UpdateQuestsItemPanels();
	void MarkQuestsDirty();

	void UpdateBadgeProgressPanels();

	bool AnyQuestItemPanelsInState( CQuestItemPanel::EItemPanelState_t eState ) const;

	MESSAGE_FUNC( OnCompleteQuest, "CompleteQuest" );

private:

	void SOEvent( const GCSDK::CSharedObject *pObject );

	CScrollableQuestList *m_pQuestList;

	class CItemModelPanel			*m_pMouseOverItemPanel;
	class CItemModelPanelToolTip	*m_pMouseOverTooltip;

	EditablePanel					*m_pProgressPanel;

	class CQuestTooltip					*m_pToolTip;
	EditablePanel				*m_pToolTipEmbeddedPanel;

	ButtonCode_t	m_iQuestLogKey;
	bool			m_bWaitingForComplete;
	bool			m_bInventoryDirty;

	Button			*m_pDebugButton;
};

#endif // QUEST_LOG_PANEL_H

#endif
