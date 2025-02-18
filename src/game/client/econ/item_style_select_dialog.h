//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_STYLE_SELECT_DIALOG_H
#define ITEM_STYLE_SELECT_DIALOG_H

#ifdef _WIN32
#pragma once
#endif

class CItemModelPanel;

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ComboBox.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CComboBoxBackpackOverlayDialogBase : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CComboBoxBackpackOverlayDialogBase, vgui::EditablePanel );

protected:
	CComboBoxBackpackOverlayDialogBase( vgui::Panel *pParent, CEconItemView *pItem );

public:
	virtual ~CComboBoxBackpackOverlayDialogBase() { }

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
	void			Show();

protected:
	CItemModelPanel *GetPreviewModelPanel() { return m_pPreviewModelPanel; }
	vgui::ComboBox *GetComboBox() { return m_pComboBox; }

	CEconItemView			*m_pItem;
private:
	virtual void PopulateComboBoxOptions() = 0;
	virtual void OnComboBoxApplication() = 0;
	virtual void OnComboBoxChanged( int iNewSelection ) { }
	virtual const char *GetTitleLabelLocalizationToken() const = 0;

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	CItemModelPanel			*m_pPreviewModelPanel;
	vgui::ComboBox			*m_pComboBox;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CStyleSelectDialog : public CComboBoxBackpackOverlayDialogBase
{
	DECLARE_CLASS_SIMPLE( CStyleSelectDialog, CComboBoxBackpackOverlayDialogBase );

public:
	CStyleSelectDialog( vgui::Panel *pParent, CEconItemView *pItem ) : CComboBoxBackpackOverlayDialogBase( pParent, pItem ) { }

protected:
	virtual void PopulateComboBoxOptions();
	virtual void OnComboBoxApplication();
	virtual void OnComboBoxChanged( int iNewSelection );
	virtual const char *GetTitleLabelLocalizationToken() const { return "#TF_Item_SelectStyle"; }
};

#endif // ITEM_STYLE_SELECT_DIALOG_H
