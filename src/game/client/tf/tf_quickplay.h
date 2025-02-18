//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Quickplay client UI
//
// $NoKeywords: $
//=============================================================================//

#ifndef _INCLUDED_TF_QUICKPLAY_UI_H
#define _INCLUDED_TF_QUICKPLAY_UI_H
#ifdef _WIN32
#pragma once
#endif

namespace vgui
{
	class Button;
	class ComboBox;
	class RadioButton;
}

#include "vgui_controls/EditablePanel.h"
#include "tf_quickplay_shared.h"

struct QuickplayItem
{
	const char *pTitle;
	const char *pDescription;
	const char *pMoreInfo;
	const char *pComplexity;
	const char *pImage;
	const char *pBetaImage;
	EGameCategory gameType;
};

//-----------------------------------------------------------------------------
// Purpose: Quickplay Dialog
//-----------------------------------------------------------------------------
extern ConVar tf_quickplay_lastviewedmode;

class CQuickplayPanelBase : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQuickplayPanelBase, vgui::EditablePanel );
public:
	CQuickplayPanelBase( vgui::Panel *parent, const char *name );
	virtual ~CQuickplayPanelBase();

	virtual void ApplySettings( KeyValues *pInResourceData ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *pCommand );

	void ShowItemByGameType( EGameCategory gameType );
	void ShowItemByIndex( int iItem );
	int GetSelectedItemIndex() const { return m_iCurrentItem; }

	void SetPageScrollButtonsVisible( bool bFlag );

protected:
	void SetupActionTarget( const char *pPanelName );
	void AddItem( EGameCategory gameType, const char *pTitle, const char *pDescription, const char *pMoreInfo, const char *pComplexity, const char *pImage, const char *pBetaImage );
	virtual void UpdateSelectableItems();
	void SaveSettings();
	void SetupMoreOptions();
	void ReadOptionCombos();
	void WriteOptionCombosAndSummary();
	virtual void GetOptionsAndSummaryText( wchar_t *pwszSummary );
	void ShowSimplifiedOrAdvancedOptions();
	virtual const char *GetItemImage( const QuickplayItem& item ) const;
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );

	virtual void UserSelectItemByIndex( int iNewItem );

protected:

	vgui::EditablePanel *m_pContainer;
	vgui::EditablePanel *m_pSimplifiedOptionsContainer;
	vgui::EditablePanel *m_pAdvOptionsContainer;
	vgui::EditablePanel *m_pGameModeInfoContainer;
	vgui::Button *m_pPrevPageButton;
	vgui::Button *m_pNextPageButton;
	vgui::Button *m_pMoreOptionsButton;
	vgui::EditablePanel *m_pMoreInfoContainer;
	vgui::ComboBox *m_pGameModeCombo;
	vgui::Label *m_pOptionsSummaryLabel;

	enum EAdvOption
	{
		kEAdvOption_ServerHost,
		kEAdvOption_MaxPlayers,
		kEAdvOption_Respawn,
		kEAdvOption_RandomCrits,
		kEAdvOption_DamageSpread
	};

	struct AdvOption {
		const char *m_pszContainerName;
		ConVar *m_pConvar;
		CUtlVector<const char *> m_vecOptionNames;
		CUtlVector<vgui::RadioButton *> m_vecRadioButtons;
		CUtlVector<const char *> m_vecOptionSummaryNames;
		int m_nChoice;
	};
	CUtlVector<AdvOption> m_vecAdvOptions;

	//vgui::CheckButton *m_pFavoritesCheckButton;
	//vgui::Button *m_pRefreshButton;
	CUtlVector< QuickplayItem > m_vecItems;
	CUtlVector< QuickplayItem > m_vecAllItems;
	int m_iCurrentItem;
	bool m_bSetInitialSelection;
	bool m_bShowRandomOption;

	char	m_szEvent247Image[MAX_PATH];
	char	m_szCommunityUpdateImage[MAX_PATH];
};

#endif // #ifndef _INCLUDED_TF_QUICKPLAY_UI_H
