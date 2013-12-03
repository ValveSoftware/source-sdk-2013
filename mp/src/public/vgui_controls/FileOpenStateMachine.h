//========= Copyright Valve Corporation, All rights reserved. ============//
//
// This is a helper class designed to help with the chains of modal dialogs
// encountered when trying to open or save a particular file
//
//=============================================================================

#ifndef FILEOPENSTATEMACHINE_H
#define FILEOPENSTATEMACHINE_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Panel.h"
#include "tier1/utlstring.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------


namespace vgui
{

//-----------------------------------------------------------------------------
// Interface for things using the file open state machine
//-----------------------------------------------------------------------------
abstract_class IFileOpenStateMachineClient
{
public:
	// Called by to allow clients to set up the save dialog
	virtual void SetupFileOpenDialog( vgui::FileOpenDialog *pDialog, bool bOpenFile, const char *pFileFormat, KeyValues *pContextKeyValues ) = 0;

	// Called by to allow clients to actually read the file in
	virtual bool OnReadFileFromDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues ) = 0;

	// Called by to allow clients to actually write the file out
	virtual bool OnWriteFileToDisk( const char *pFileName, const char *pFileFormat, KeyValues *pContextKeyValues ) = 0;
};


//-----------------------------------------------------------------------------
// This is a helper class designed to help with chains of modal dialogs
//-----------------------------------------------------------------------------
enum FileOpenStateMachineFlags_t
{
	FOSM_SHOW_PERFORCE_DIALOGS	= 0x1,
	FOSM_SHOW_SAVE_QUERY		= 0x2,
};

class FileOpenStateMachine : public Panel
{
	DECLARE_CLASS_SIMPLE( FileOpenStateMachine, Panel );

public:
	enum CompletionState_t
	{
		IN_PROGRESS = 0,				// Still not finished, not successful or error
		SUCCESSFUL,						// Operation finished successfully
		FILE_SAVE_CANCELLED,			// The user chose 'cancel' in the dialog asking if he wanted to save
		FILE_SAVE_NAME_NOT_SPECIFIED,	// User hit cancel in the SaveAs dialog
		FILE_NOT_OVERWRITTEN,			// Operation aborted; existed file and user chose to not write over it
		FILE_NOT_CHECKED_OUT,			// Operation aborted; file wasn't checked out so couldn't be written over
		ERROR_WRITING_FILE,				// Error occurred writing the file out
		ERROR_MAKING_FILE_WRITEABLE,	// Error occurred when making the file writeable
		FILE_NOT_MADE_WRITEABLE,		// User chose to not make the file be writeable
		FILE_OPEN_NAME_NOT_SPECIFIED,	// User hit cancel in the Open dialog
		ERROR_READING_FILE,				// Error occurred reading the file in
	};

	FileOpenStateMachine( vgui::Panel *pParent, IFileOpenStateMachineClient *pClient );
	virtual ~FileOpenStateMachine();

	// Opens a file, saves an existing one if necessary
	void OpenFile( const char *pOpenFileType, KeyValues *pContextKeyValues, const char *pSaveFileName = NULL, const char *pSaveFileType = NULL, int nFlags = 0 );

	// Version of OpenFile that skips browsing for a particular file to open
	void OpenFile( const char *pOpenFileName, const char *pOpenFileType, KeyValues *pContextKeyValues, const char *pSaveFileName = NULL, const char *pSaveFileType = NULL, int nFlags = 0 );

	// Used to save a specified file, and deal with all the lovely dialogs
	// Pass in NULL to get a dialog to choose a filename to save
	void SaveFile( KeyValues *pContextKeyValues, const char *pFileName, const char *pFileType, int nFlags = FOSM_SHOW_PERFORCE_DIALOGS );

	// Returns the state machine completion state
	CompletionState_t GetCompletionState();

	/* MESSAGES SENT
		"FileStateMachineFinished" - Called when we exit the state machine for any reason
			"completionState" - See the CompletionState_t enum above
			"wroteFile" - Indicates whether a file was written or not
			"fullPath" - Indicates the full path of the file read for OpenFile or written for SaveFile
			"fileType" - Indicates the file type of the file read for OpenFile or written for SaveFile
			Use GetFirstTrueSubKey() to get the context passed into the OpenFile/SaveFile methods
	*/									  

private:
	enum FOSMState_t
	{
		STATE_NONE = -1,
		STATE_SHOWING_SAVE_DIRTY_FILE_DIALOG = 0,
		STATE_SHOWING_SAVE_DIALOG,
		STATE_SHOWING_OVERWRITE_DIALOG,
		STATE_SHOWING_CHECK_OUT_DIALOG,
		STATE_SHOWING_MAKE_FILE_WRITEABLE_DIALOG,
		STATE_WRITING_FILE,
		STATE_SHOWING_PERFORCE_ADD_DIALOG,
		STATE_SHOWING_OPEN_DIALOG,
		STATE_READING_FILE,
	};

	MESSAGE_FUNC_PARAMS( OnFileSelected, "FileSelected", pKeyValues );
	MESSAGE_FUNC( OnFileSelectionCancelled, "FileSelectionCancelled" );
	MESSAGE_FUNC_PARAMS( OnPerforceQueryCompleted, "PerforceQueryCompleted", pKeyValues );
	MESSAGE_FUNC( OnMakeFileWriteable, "MakeFileWriteable" );
	MESSAGE_FUNC( OnCancelMakeFileWriteable, "CancelMakeFileWriteable" );

	// These messages are related to the dialog in OverwriteFileDialog
	MESSAGE_FUNC( OnOverwriteFile, "OverwriteFile" );
	MESSAGE_FUNC( OnCancelOverwriteFile, "CancelOverwriteFile" );

	// These messages come from the savedocumentquery dialog
	MESSAGE_FUNC( OnSaveFile, "OnSaveFile" );
	MESSAGE_FUNC( OnMarkNotDirty, "OnMarkNotDirty" );
	MESSAGE_FUNC( OnCancelSaveDocument, "OnCancelSaveDocument" );

	// Cleans up keyvalues
	void CleanUpContextKeyValues();

	// Utility to set the completion state
	void SetCompletionState( CompletionState_t state );

	// Show the save document query dialog
	void ShowSaveQuery( );

	// Shows the overwrite existing file dialog
	void OverwriteFileDialog( );

	// Shows the open file for edit dialog
	void CheckOutDialog( );

	// Shows the make file writeable dialog
	void MakeFileWriteableDialog( );

	// Writes the file out
	void WriteFile();

	// Shows the open file dialog
	void OpenFileDialog( );

	// Reads the file in
	void ReadFile();

	IFileOpenStateMachineClient *m_pClient;
	KeyValues *m_pContextKeyValues;
	FOSMState_t m_CurrentState;
	CompletionState_t m_CompletionState;
	CUtlString m_FileName;
	CUtlString m_SaveFileType;
	CUtlString m_OpenFileType;
	CUtlString m_OpenFileName;
	bool m_bShowPerforceDialogs : 1;
	bool m_bShowSaveQuery : 1;
	bool m_bIsOpeningFile : 1;
	bool m_bWroteFile : 1;
};

} // end namespace vgui



#endif // FILEOPENSTATEMACHINE_H
