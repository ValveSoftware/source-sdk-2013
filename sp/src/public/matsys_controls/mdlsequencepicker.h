//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef MDLSEQUENCEPICKER_H
#define MDLSEQUENCEPICKER_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ImageList.h"
#include "vgui_controls/Frame.h"
#include "datacache/imdlcache.h"
#include "matsys_controls/mdlpanel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class Splitter;
	class Button;
}

class CGameFileTreeView;


//-----------------------------------------------------------------------------
// Purpose: Main app window
//-----------------------------------------------------------------------------
class CMDLSequencePicker : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMDLSequencePicker, vgui::EditablePanel );
public:
	CMDLSequencePicker( vgui::Panel *pParent );
	virtual ~CMDLSequencePicker();

	// overridden frame functions
	virtual void Activate();
	virtual void OnClose();
	virtual void PerformLayout();
	virtual void OnTick();

	char const *GetModelName();
	char const *GetSequenceName();
	int	GetSequenceNumber();

private:
	void SelectMDL( const char *pMDLName );
	void RefreshFileList();
	void RefreshActivitiesAndSequencesList();

	// Plays the selected activity
	void PlaySelectedActivity( );

	// Plays the selected sequence
	void PlaySelectedSequence( );

	MESSAGE_FUNC( OnFileSelected, "TreeViewItemSelected" );
	MESSAGE_FUNC_PTR_CHARPTR( OnTextChanged, "TextChanged", Panel, text );
	MESSAGE_FUNC_PARAMS( OnItemSelected, "ItemSelected", kv );
	MESSAGE_FUNC( OnPageChanged, "PageChanged" );

	// changes
//	MESSAGE_FUNC_INT( CloakFolder, "CloakFolder", item );
//	MESSAGE_FUNC_INT( OpenFileForEdit, "EditFile", item );
//	MESSAGE_FUNC_INT( OpenFileForDelete, "DeleteFile", item );

	CMDLPanel *m_pMDLPreview;
	vgui::ComboBox *m_pFilterList;
	CGameFileTreeView *m_pFileTree;
	vgui::ImageList m_Images;
	vgui::Splitter* m_pMDLSplitter;
	vgui::Splitter* m_pSequenceSplitter;
	vgui::PropertySheet *m_pViewsSheet;
	vgui::PropertyPage *m_pSequencesPage;
	vgui::PropertyPage *m_pActivitiesPage;

	vgui::ListPanel *m_pSequencesList;
	vgui::ListPanel *m_pActivitiesList;

	MDLHandle_t m_hSelectedMDL;

	friend class CMDLSequencePickerFrame;
};

//-----------------------------------------------------------------------------
// Model sequence picker frame
//-----------------------------------------------------------------------------
class CMDLSequencePickerFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CMDLSequencePickerFrame, vgui::Frame );
public:
	CMDLSequencePickerFrame( vgui::Panel *parent, char const *title );
	virtual ~CMDLSequencePickerFrame();

	virtual void PerformLayout();

protected:

	virtual void OnTick();

	MESSAGE_FUNC( OnOK, "OnOK" );
	MESSAGE_FUNC( OnCancel, "OnCancel" );

private:
	CMDLSequencePicker *m_pMDLSequencePicker;

	vgui::Button		*m_pOK;
	vgui::Button		*m_pCancel;
};

#endif // MDLSEQUENCEPICKER_H
