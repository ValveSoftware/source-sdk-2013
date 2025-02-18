//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYBOARDEDITORDIALOG_H
#define KEYBOARDEDITORDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/PropertyPage.h"

class VControlsListPanel;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorPage : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorPage, EditablePanel );

public:
	CKeyBoardEditorPage( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );
	~CKeyBoardEditorPage();

	void	SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	virtual void	OnKeyCodeTyped(vgui::KeyCode code);

	virtual void	ApplySchemeSettings( IScheme *scheme );

	void			OnSaveChanges();
	void			OnRevert();
	void			OnUseDefaults();

protected:

	virtual void	OnPageHide();

	virtual void	OnCommand( char const *cmd );

	void			PopulateList();

	void			GetMappingList( Panel *panel, CUtlVector< PanelKeyBindingMap * >& maps );
	int				GetMappingCount( Panel *panel );

	void			BindKey( vgui::KeyCode code );

		// Trap row selection message
	MESSAGE_FUNC( ItemSelected, "ItemSelected" );
	MESSAGE_FUNC_INT( OnClearBinding, "ClearBinding", item );

	void			SaveMappings();
	void			UpdateCurrentMappings();
	void			RestoreMappings();
	void			ApplyMappings();

protected:
	void					AnsiText( char const *token, char *out, int nBuflen );

	Panel			*m_pPanel;
	KeyBindingContextHandle_t m_Handle;

	VControlsListPanel	*m_pList;

	struct SaveMapping_t
	{
		SaveMapping_t();
		SaveMapping_t( const SaveMapping_t& src );

		PanelKeyBindingMap		*map;
		CUtlVector< BoundKey_t > current;
		CUtlVector< BoundKey_t > original;
	};

	CUtlVector< SaveMapping_t * > m_Save;
};


//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorSheet : public PropertySheet
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorSheet, PropertySheet );

public:
	CKeyBoardEditorSheet( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );

	void	SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	void			OnSaveChanges();
	void			OnRevert();
	void			OnUseDefaults();

protected:

	vgui::PHandle			m_hPanel;
	KeyBindingContextHandle_t m_Handle;
	bool					m_bSaveToExternalFile;
	CUtlSymbol				m_SaveFileName;
	CUtlSymbol				m_SaveFilePathID;
	Color					m_clrAlteredItem;
};

//-----------------------------------------------------------------------------
// Purpose: Dialog for use in editing keybindings
//-----------------------------------------------------------------------------
class CKeyBoardEditorDialog : public Frame
{
	DECLARE_CLASS_SIMPLE( CKeyBoardEditorDialog, Frame );

public:
	CKeyBoardEditorDialog( Panel *parent, Panel *panelToEdit, KeyBindingContextHandle_t handle );

	void			SetKeybindingsSaveFile( char const *filename, char const *pathID = 0 );

	virtual void	OnCommand( char const *cmd );

private:
	CKeyBoardEditorSheet		*m_pKBEditor;

	Button						*m_pSave;
	Button						*m_pCancel;
	Button						*m_pRevert;
	Button						*m_pUseDefaults;
};

}

#endif // KEYBOARDEDITORDIALOG_H
