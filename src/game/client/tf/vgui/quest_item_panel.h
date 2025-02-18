//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef QUEST_ITEM_PANEL_H
#define QUEST_ITEM_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#if 0

#include "econ_item_inventory.h"
#include "tf_controls.h"
#include "econ_quests.h"

using namespace vgui;

class CScrollableQuestList;
class CItemModelPanel;

//-----------------------------------------------------------------------------
// Simple tooltip class that looks into the moused-over panel's dialog variables
// for "tiptext" and uses that value as its string to present.
//-----------------------------------------------------------------------------
class CQuestTooltip : public CTFTextToolTip
{
	DECLARE_CLASS_SIMPLE( CQuestTooltip, CTFTextToolTip );
public:
	CQuestTooltip( vgui::Panel *parent, const char *text = NULL )
		: BaseClass( parent, text )
	{}

	virtual void ShowTooltip( Panel *pCurrentPanel ) OVERRIDE;
	virtual void PositionWindow( Panel *pTipPanel ) OVERRIDE;
private:
};

//-----------------------------------------------------------------------------
// Can pass various input events to other panels
//-----------------------------------------------------------------------------
class CInputProxyPanel : public EditablePanel
{
public:

	enum EInputTypes
	{
		INPUT_MOUSE_ENTER = 0,
		INPUT_MOUSE_EXIT,
		INPUT_MOUSE_PRESS,
		INPUT_MOUSE_DOUBLE_PRESS,
		INPUT_MOUSE_RELEASED,
		INPUT_MOUSE_WHEEL,
		INPUT_MOUSE_MOVE,
		NUM_INPUT_TYPES,
	};

	DECLARE_CLASS_SIMPLE( CInputProxyPanel, EditablePanel );
	CInputProxyPanel( Panel *parent, const char *pszPanelName );

	void AddPanelForCommand( EInputTypes eInputType, Panel* pPanel, const char* pszCommand );

	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);
	virtual void OnMouseReleased(MouseCode code);
	virtual void OnMouseWheeled(int delta);

private:

	struct CommandPair_t
	{
		Panel* m_pPanel;
		const char* m_pszCommand;
	};
	CUtlVector< CommandPair_t > m_vecRedirectPanels[NUM_INPUT_TYPES];
};

//-----------------------------------------------------------------------------
// Contains a panel that animates into place when it needs to show or hide
//-----------------------------------------------------------------------------
class CQuestStatusPanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CQuestStatusPanel, EditablePanel );
	CQuestStatusPanel( Panel *parent, const char *pszPanelName );

	void SetShow( bool bShow );
	virtual void OnThink() OVERRIDE;

private:
	EditablePanel* m_pMovingContainer;
	RealTimeCountdownTimer m_transitionTimer;
	bool m_bShouldBeVisible;

	CPanelAnimationVarAliasType( int, m_iVisibleY, "visible_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iHiddenY, "hidden_y", "40", "proportional_int" );
};

//-----------------------------------------------------------------------------
// An representation of a single quest
//-----------------------------------------------------------------------------
class CQuestItemPanel : public EditablePanel, CGameEventListener
{
public:
	enum EItemPanelState_t
	{
		STATE_NORMAL = 0,
		STATE_UNIDENTIFIED,
		STATE_IDENTIFYING,
		STATE_COMPLETED,
		STATE_TURNING_IN__WAITING_FOR_GC,
		STATE_TURNING_IN__GC_RESPONDED,
		STATE_SHOW_ACCEPTED,

		NUM_STATES,
	};

	DECLARE_CLASS_SIMPLE( CQuestItemPanel, EditablePanel );

	CQuestItemPanel( Panel *parent, const char *pszPanelName, CQuest* pQuestItem, CScrollableQuestList* pQuestList );
	virtual ~CQuestItemPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void OnSizeChanged(int wide, int tall) OVERRIDE {}
	virtual void OnMouseReleased(MouseCode code) OVERRIDE;

	const CQuest* GetItem() { return m_pLiveQuest; }
	void SetItem( CQuest* pItem );
	void QuestCompletedResponse();
	EItemPanelState_t GetState() const { return m_eState; }
	void SetSelected( bool bSelected, bool bImmediate );
	bool IsSelected() const { return !m_bCollapsed; }
	bool IsCursorOverMainContainer() const;

	MESSAGE_FUNC( OnCollapsedGlowStart, "CollapsedGlowStart" );
	MESSAGE_FUNC( OnCollapsedGlowEnd, "CollapsedGlowEnd" );
	MESSAGE_FUNC( OnDiscardQuest, "DiscardQuest" );
	MESSAGE_FUNC( OnEquipLoaners, "EquipLoaners" );
	void OnCompleteQuest();
	void OnConfirmDelete( bool bConfirm );
	void OnConfirmEquipLoaners( bool bConfirm );

protected:

	bool HasAllControls() const { return m_bHasAllControls; }

	void LoadResFileForCurrentItem();
	void SetupObjectivesPanels( bool bRecreate );
	void SetState( EItemPanelState_t eState );
	void CaptureAndEncodeStrings();
	const wchar_t* GetDecodedString( const char* pszKeyName, float flPercentDecoded );
	void UpdateInvalidReasons();

	EItemPanelState_t m_eState;
	CQuest m_quest;
	CQuest* m_pLiveQuest;

	EditablePanel	*m_pQuestPaperContainer;
	EditablePanel	*m_pFrontFolderContainer;
	ImagePanel		*m_pFrontFolderImage;
	EditablePanel	*m_pBackFolderContainer;
	ImagePanel		*m_pBackFolderImage;
	ImagePanel		*m_pEncodedImage;
	EditablePanel	*m_pMainContainer;

	CQuestStatusPanel					*m_pEncodedStatus;
	CQuestStatusPanel					*m_pInactiveStatus;
	CQuestStatusPanel					*m_pReadyToTurnInStatus;
	Label								*m_pFlavorText;
	Label								*m_pObjectiveExplanationLabel;
	Label								*m_pExpirationLabel;
	EditablePanel						*m_pTurnInContainer;
	EditablePanel						*m_pTurnInDimmer;
	Button								*m_pTurnInButton;
	EditablePanel						*m_pTurnInSpinnerContainer;
	CExButton							*m_pTitleButton;
	EditablePanel						*m_pIdentifyDimmer;
	EditablePanel						*m_pIdentifyContainer;
	CExButton							*m_pIdentifyButton;
	ImagePanel							*m_pPhotoStatic;
	ImagePanel							*m_pAcceptedImage;
	Label								*m_pTurningInLabel;
	class CExScrollingEditablePanel		*m_pFlavorScrollingContainer;
	CExButton							*m_pFindServerButton;
	
	// loaners
	EditablePanel						*m_pLoanerContainerPanel;
	CExButton							*m_pRequestLoanerItemsButton;
	CExButton							*m_pEquipLoanerItemsButton;
	CItemModelPanel						*m_pLoanerItemModelPanel[2];

	CExButton							*m_pDiscardButton;
	CExButton							*m_pCompleteButton;


	int									m_nPaperXPos;
	int									m_nPaperYPos;	
	int									m_nPaperXShakePos;
	int									m_nPaperYShakePos;
	bool								m_bHasAllControls;
	CUtlString							m_strItemTrackerResFile;
	
	CUtlString							m_strMatchmakingGroupName;
	CUtlString							m_strMatchmakingCategoryName;
	CUtlString							m_strMatchmakingMapName;

	// Sound effects
	CUtlString							m_strExpandSound;
	CUtlString							m_strCollapseSound;
	CUtlString							m_strTurnInSound;
	CUtlString							m_strTurnInSuccessSound;
	CUtlString							m_strDecodeSound;

	// Animation
	CUtlString							m_strReset;
	CUtlString							m_strAnimExpand;
	CUtlString							m_strAnimCollapse;
	CUtlString							m_strTurningIn;
	CUtlString							m_strHighlightOn;
	CUtlString							m_strHighlightOff;

	class CQuestProgressTrackerPanel *m_pItemTrackerPanel;

	CScrollableQuestList *m_pQuestList;

	RealTimeCountdownTimer	m_StateTimer;
	KeyValues *m_pKVItemTracker;

	struct FolderPair_t
	{
		CUtlString m_strFront;
		CUtlString m_strBack;
	};
	CUtlVector< FolderPair_t > m_vecFoldersImages;

	CUtlString m_strEncodedText;
	CUtlString m_strExpireText;
	const char *m_pszCompleteSound;
	bool m_bCollapsed;

	KeyValues *m_pKVCipherStrings;

	CPanelAnimationVarAliasType( int, m_iFrontPaperHideHeight, "front_paper_hide_height", "1000", "proportional_int" ); // Default to a large value so it wont be visible
	CPanelAnimationVarAliasType( int, m_iUnidentifiedHeight, "unidentified_height", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iObjectiveInset, "objective_inset", "200", "proportional_int" );
	//CPanelAnimationVarAliasType( int, m_iScrollingContainerHeight, "scrolling_container_height", "200", "proportional_int" );

	enum EDecodeStyle
	{
		DECODE_STYLE_CYPHER = 0,
		DECODE_STYLE_PANEL_FADE,
	};
	CPanelAnimationVarAliasType( EDecodeStyle, m_eDecodeStyle, "decode_style", "0", "int" );
};

#endif // QUEST_ITEM_PANEL_H

#endif
