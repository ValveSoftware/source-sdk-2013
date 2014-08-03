//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Declaration of FileOpenDialog class, a generic open/save as file dialog
//
// $NoKeywords: $
//===========================================================================//

#ifndef FILEOPENDIALOG_H
#define FILEOPENDIALOG_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"

namespace vgui
{

class FileCompletionEdit;		// local
class InputDialog;

//-----------------------------------------------------------------------------
// Purpose: generic open/save as file dialog
//-----------------------------------------------------------------------------
enum FileOpenDialogType_t
{
	FOD_SAVE = 0,
	FOD_OPEN,
	FOD_SELECT_DIRECTORY,
};


class FileOpenDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( FileOpenDialog, Frame );

public:
	// NOTE: Backward compat constructor
	FileOpenDialog( Panel *parent, const char *title, bool bOpenFile, KeyValues *pContextKeyValues = 0 );

	// The context keyvalues are added to all messages sent by this dialog if they are specified
	FileOpenDialog( Panel *parent, const char *title, FileOpenDialogType_t type, KeyValues *pContextKeyValues = 0 );
	~FileOpenDialog();

	// Set the directory the file search starts in
	void SetStartDirectory(const char *dir);

	// Sets the start directory context (and resets the start directory in the process)
	// NOTE: If you specify a startdir context, then if you've already opened
	// a file with that same start dir context before, it will start in the
	// same directory it ended up in.
	void SetStartDirectoryContext( const char *pContext, const char *pDefaultDir );

	// Add filters for the drop down combo box
	// The filter info, if specified, gets sent back to the app in the FileSelected message
	void AddFilter( const char *filter, const char *filterName, bool bActive, const char *pFilterInfo = NULL );

	// Activate the dialog
	// NOTE: The argument is there for backward compat
	void DoModal( bool bUnused = false );

	// Get the directory this is currently in
	void GetCurrentDirectory( char *buf, int bufSize );

	// Get the last selected file name
	void GetSelectedFileName( char *buf, int bufSize );

	/*
		messages sent:
			"FileSelected"
				"fullpath"	// specifies the fullpath of the file
				"filterinfo"	// Returns the filter info associated with the active filter
			"FileSelectionCancelled"
	*/

protected:
	virtual void OnCommand( const char *command );
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void OnClose();
	virtual void OnKeyCodeTyped(KeyCode code);

	// handles the open button being pressed
	// checks on what has changed and acts accordingly
	MESSAGE_FUNC( OnOpen, "OnOpen" );
	MESSAGE_FUNC( OnSelectFolder, "SelectFolder" );
	MESSAGE_FUNC( OnFolderUp, "OnFolderUp" );
	MESSAGE_FUNC( OnNewFolder, "OnNewFolder" );
	MESSAGE_FUNC( OnOpenInExplorer, "OpenInExplorer" );

	MESSAGE_FUNC( PopulateFileList, "PopulateFileList" );
	MESSAGE_FUNC( PopulateDriveList, "PopulateDriveList" );
	MESSAGE_FUNC( PopulateFileNameCompletion, "PopulateFileNameCompletion" );

	// moves the directory structure up
	virtual void MoveUpFolder();

	// validates that the current path is valid
	virtual void ValidatePath();

	// handles an item in the list being selected
	MESSAGE_FUNC( OnItemSelected, "ItemSelected" );
	MESSAGE_FUNC( OnListItemSelected, "ListItemSelected" )
	{
		OnItemSelected();
	}

	// changes directories in response to selecting drives from the combo box
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", kv );

	MESSAGE_FUNC( OnInputCanceled, "InputCanceled" );
	MESSAGE_FUNC_PARAMS( OnInputCompleted, "InputCompleted", data );

private:
	// Necessary because we have 2 constructors
	void Init( const char *title, KeyValues *pContextKeyValues );

	// Does the specified extension match something in the filter list?
	bool ExtensionMatchesFilter( const char *pExt );

	// Choose the first non *.* filter in the filter list
	void ChooseExtension( char *pExt, int nBufLen );

	// Saves the file to the start dir context
	void SaveFileToStartDirContext( const char *pFullPath );

	// Posts a file selected message
	void PostFileSelectedMessage( const char *pFileName );

	// Creates a new folder
	void NewFolder( char const *folderName );

	vgui::ComboBox 		*m_pFullPathEdit;
	vgui::ListPanel		*m_pFileList;
	
	FileCompletionEdit 	*m_pFileNameEdit;

	vgui::ComboBox 		*m_pFileTypeCombo;
	vgui::Button 		*m_pOpenButton;
	vgui::Button 		*m_pCancelButton;
	vgui::Button 		*m_pFolderUpButton;
	vgui::Button		*m_pNewFolderButton;
	vgui::Button		*m_pOpenInExplorerButton;
	vgui::ImagePanel 	*m_pFolderIcon;
	vgui::Label			*m_pFileTypeLabel;

	KeyValues			*m_pContextKeyValues;

	char m_szLastPath[1024];
	unsigned short m_nStartDirContext;
	FileOpenDialogType_t m_DialogType;
	bool m_bFileSelected : 1;

	VPANEL				m_SaveModal;
	vgui::DHANDLE< vgui::InputDialog >	m_hInputDialog;
};

} // namespace vgui

#endif // FILEOPENDIALOG_H
