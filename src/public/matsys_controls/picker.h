//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An arbitrary picker
//
//=============================================================================

#ifndef PICKER_H
#define PICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Frame.h"
#include "tier1/utlstring.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class Panel;
}


//-----------------------------------------------------------------------------
// List of strings to appear in the picker
//-----------------------------------------------------------------------------
enum PickerChoiceType_t
{
	PICKER_CHOICE_STRING = 0,
	PICKER_CHOICE_PTR,
};

struct PickerInfo_t
{
	const char *m_pChoiceString;	// This is what displays in the dialog
	union
	{
		const char *m_pChoiceValue;
		void *m_pChoiceValuePtr;
	};
};

struct PickerList_t
{
	PickerList_t() : m_Type( PICKER_CHOICE_STRING ) {}
	PickerList_t( int nGrowSize, int nInitSize ) : m_Choices( nGrowSize, nInitSize ), m_Type( PICKER_CHOICE_STRING ) {}

	int Count() const { return m_Choices.Count(); }
	PickerInfo_t& operator[]( int i ) { return m_Choices[i]; }
	const PickerInfo_t& operator[]( int i ) const { return m_Choices[i]; }
	int AddToTail() { return m_Choices.AddToTail(); }
	void RemoveAll() { return m_Choices.RemoveAll(); }

	PickerChoiceType_t m_Type;
	CUtlVector< PickerInfo_t > m_Choices;
};


//-----------------------------------------------------------------------------
// Purpose: Base class for choosing raw assets
//-----------------------------------------------------------------------------
class CPicker : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CPicker, vgui::EditablePanel );

public:
	CPicker( vgui::Panel *pParent, const char *pColumnHeader, const char *pTextType );
	~CPicker();

	// Sets the list of strings to display
	void SetStringList( const PickerList_t &list ); 

	// Purpose: 
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	// Returns the selected string
	PickerChoiceType_t GetSelectionType() const;
	const char *GetSelectedString( ) const;
	void *GetSelectedPtr( ) const;

	// Returns the index of the selected string
	int GetSelectedIndex();

private:
	void RefreshChoiceList( );
	MESSAGE_FUNC( OnTextChanged, "TextChanged" );

	vgui::TextEntry *m_pFilterList;
	vgui::ListPanel *m_pPickerBrowser;
	CUtlString m_Filter;
	const char *m_pPickerType;
	const char *m_pPickerTextType;
	const char *m_pPickerExt;
	const char *m_pPickerSubDir;
	PickerChoiceType_t m_Type;

	friend class CPickerFrame;
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for picker
//-----------------------------------------------------------------------------
class CPickerFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CPickerFrame, vgui::Frame );

public:
	CPickerFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, const char *pTextType );
	~CPickerFrame();

	// Inherited from Frame
	virtual void OnCommand( const char *pCommand );

	// Purpose: Activate the dialog
	// The message "Picked" will be sent if something is picked.
	// You can pass in keyvalues to get added to the message also.
	void DoModal( const PickerList_t &list, KeyValues *pContextKeyValues = NULL );

private:
	void CleanUpMessage();

	CPicker *m_pPicker;
	vgui::Button *m_pOpenButton;
	vgui::Button *m_pCancelButton;
	KeyValues *m_pContextKeyValues;
};


#endif // PICKER_H
