//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TESTITEM_ROOT_H
#define TESTITEM_ROOT_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_controls.h"
#include "testitem_dialog.h"


//-----------------------------------------------------------------------------
// A panel that handles the overall item testing process
//-----------------------------------------------------------------------------
class CTestItemBotControls : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTestItemBotControls, vgui::EditablePanel );
public:
	CTestItemBotControls( vgui::Panel *parent );
	~CTestItemBotControls( void );

	void			SetupComboBoxes( void );
	virtual void	FireGameEvent( IGameEvent *event );
	void			ImportTestSetup( KeyValues *pKV );
	void			Close( void );
	void			SetEmbedded( bool bEmbedded ) { m_bEmbedded = bEmbedded; InvalidateLayout(); }

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	void			UpdateBots( void );
	void			CommitSettingsToKV( void );

private:
	vgui::ComboBox			*m_pBotAnimationComboBox;
	vgui::Slider			*m_pBotAnimationSpeedSlider;
	vgui::CheckButton		*m_pBotForceFireCheckBox;
	vgui::CheckButton		*m_pBotTurntableCheckBox;
	vgui::CheckButton		*m_pBotViewScanCheckBox;
	bool					m_bEmbedded;
};


//-----------------------------------------------------------------------------
// A panel that handles the overall item testing process
//-----------------------------------------------------------------------------
class CTestItemRoot : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTestItemRoot, vgui::EditablePanel );
public:
	CTestItemRoot( vgui::Panel *parent );
	~CTestItemRoot( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void FireGameEvent( IGameEvent *event );

	void	Close( void );
	void	CloseAndTestItem( void );
	void	UpdateTestItems( void );
	int		FindReplaceableItemsForSelectedClass( CUtlVector<item_definition_index_t> *pItemDefs = NULL, bool bWeapons = false );
	void	ExportTestSetup( const char *pFilename );
	void	ImportTestSetup( const char *pFilename );
	void	ImportTestSetup( KeyValues *pKV );
	void	CommitSettingsToKV( void );

	MESSAGE_FUNC_PARAMS( OnSetTestItemKVs, "SetTestItemKVs", pKV );
	MESSAGE_FUNC_PARAMS( OnButtonChecked, "CheckButtonChecked", pData );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

private:
	void	SetupComboBoxes( void );

private:
	int						m_iClassUsage;

	vgui::EditablePanel		*m_pClassUsagePanel;
	vgui::EditablePanel		*m_pTestingPanel;
	vgui::EditablePanel		*m_pBotAdditionPanel;
	CTestItemBotControls	*m_pBotControlPanel;

	// Testing panel
	CExButton				*m_pItemTestButtons[TI_TYPE_COUNT];
	CExButton				*m_pItemRemoveButtons[TI_TYPE_COUNT];
	CExLabel				*m_pItemTestLabels[TI_TYPE_COUNT];
	vgui::CheckButton		*m_pClassCheckButtons[TF_LAST_NORMAL_CLASS];
	KeyValues				*m_pItemTestKVs[TI_TYPE_COUNT];

	// Bot addition panel
	vgui::ComboBox			*m_pBotSelectionComboBox;
	vgui::CheckButton		*m_pAutoAddBotsCheckBox;
	vgui::CheckButton		*m_pBotsOnBlueTeamCheckBox;
	CExButton				*m_pAddBotButton;

	vgui::DHANDLE<CTestItemDialog> m_hEditItemDialog;
	vgui::FileOpenDialog	*m_hImportExportDialog;
	bool					m_bExporting;
};

#endif // TESTITEM_ROOT_H
