//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TESTITEM_DIALOG_H
#define TESTITEM_DIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_controls.h"

enum testitem_entrysteps_t
{
	TI_STEP_MODELNAME,
	TI_STEP_WPN_ITEMREPLACED,
	TI_STEP_NONWPN_BODYGROUPS,
	TI_STEP_OTHER_OPTIONS,
	TI_STEP_CUSTOMIZATION,

	TI_STEP_FINISHED,
};

enum testitem_bodygroups_to_hide_t
{
	TI_HIDEBG_HAT,
	TI_HIDEBG_HEADPHONES,
	TI_HIDEBG_MEDALS,
	TI_HIDEBG_GRENADES,
	TI_HIDEBG_BULLETS,
	TI_HIDEBG_ARROWS,
	TI_HIDEBG_RIGHTARM,
	TI_HIDEBG_SHOES_SOCKS,

	TI_HIDEBG_COUNT,
};

//-----------------------------------------------------------------------------
// A dialog that handles adding or modifying an item we're testing
//-----------------------------------------------------------------------------
class CTestItemDialog : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTestItemDialog, vgui::EditablePanel );
public:
	CTestItemDialog( vgui::Panel *parent, testitem_itemtypes_t iItemType, int iClassUsage, KeyValues *pExistingKVs );
	~CTestItemDialog( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void FireGameEvent( IGameEvent *event );

	void	Close( void );
	void	CloseAndUpdateItem( void );

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

private:
	void	InitializeFromExistingKVs( KeyValues *pExistingKVs );
	void	SetEntryStep( testitem_entrysteps_t iStep );
	void	OpenSelectModelDialog( void );
	void	SetupItemComboBox( vgui::ComboBox *pComboBox );
	void	SetupPaintColorComboBox( void );
	void	SetupUnusualEffectComboBox( void );
	void	HandleClassCheckbuttonChecked( vgui::Panel *pPanel );

private:
	testitem_entrysteps_t	m_iEntryStep;
	testitem_itemtypes_t	m_iItemType;
	int						m_iClassUsage;

	vgui::FileOpenDialog	*m_hImportModelDialog;
	char					m_szRelativePath[MAX_PATH];

	CExLabel				*m_pModelLabel;
	CExLabel				*m_pSelectModelLabel;
	CExLabel				*m_pNoItemsToReplaceLabel;
	CExButton				*m_pSelectModelButton;
	CExButton				*m_pOkButton;
	vgui::ComboBox			*m_pItemReplacedComboBox;
	vgui::EditablePanel		*m_pBodygroupPanel;
	vgui::EditablePanel		*m_pItemReplacedPanel;
	vgui::CheckButton		*m_pBodygroupCheckButtons[TI_HIDEBG_COUNT];

	vgui::EditablePanel		*m_pCustomizationsPanel;
	vgui::ComboBox			*m_pPaintColorComboBox;
	vgui::ComboBox			*m_pUnusualEffectComboBox;

	vgui::EditablePanel		*m_pExistingItemToTestPanel;
	vgui::ComboBox			*m_pExistingItemComboBox;
};

#endif // TESTITEM_DIALOG_H
