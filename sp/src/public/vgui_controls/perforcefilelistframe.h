//========= Copyright Valve Corporation, All rights reserved. ============//
//
// List of perforce files and operations
//
//=============================================================================

#ifndef PERFORCEFILELISTFRAME_H
#define PERFORCEFILELISTFRAME_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/Frame.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"
#include "p4lib/ip4.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Enumeration of operation dialog ids
//-----------------------------------------------------------------------------
enum
{
	OPERATION_DIALOG_ID_PERFORCE = 0,

	OPERATION_DIALOG_STANDARD_ID_COUNT,
	OPERATION_DIALOG_STANDARD_ID_MAX = OPERATION_DIALOG_STANDARD_ID_COUNT - 1,
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for a list of files + an operation to perform
//-----------------------------------------------------------------------------
class COperationFileListFrame : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( COperationFileListFrame, vgui::Frame );

public:
	// NOTE: The dialog ID is used to allow dialogs to have different configurations saved 
	COperationFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, bool bShowDescription, bool bShowOkOnly = false, int nDialogID = 1 );
	virtual ~COperationFileListFrame();

	// Command handler
	virtual void OnCommand( const char *pCommand );
	virtual void PerformLayout();

	// Adds files to the frame
	void ClearAllOperations();
	void AddOperation( const char *pOperation, const char *pFileName );
	void AddOperation( const char *pOperation, const char *pFileName, const Color& clr );

	// Resizes the operation column to fit the operation text
	void ResizeOperationColumnToContents();

	// Sets the column header for the 'operation' column
	void SetOperationColumnHeaderText( const char *pText );

	// Shows the panel
	void DoModal( KeyValues *pContextKeyValues = NULL, const char *pMessage = NULL );

	// Retrieves the number of files, the file names, and operations
	int GetOperationCount();
	const char *GetFileName( int i );
	const char *GetOperation( int i );

	// Retreives the description (only if it was shown)
	const char *GetDescription();

private:
	virtual bool PerformOperation() { return true; }
	const char *CompletionMessage();
	void CleanUpMessage();

	vgui::ListPanel *m_pFileBrowser;
	vgui::Splitter *m_pSplitter;
	vgui::TextEntry *m_pDescription;
	vgui::Button *m_pYesButton;
	vgui::Button *m_pNoButton;
	KeyValues *m_pContextKeyValues;
	CUtlString m_MessageName;
	char *m_pText;
};


//-----------------------------------------------------------------------------
// Purpose: Modal dialog for picker
//-----------------------------------------------------------------------------
enum PerforceAction_t
{
	PERFORCE_ACTION_NONE = -1,
	PERFORCE_ACTION_FILE_ADD = 0,
	PERFORCE_ACTION_FILE_EDIT,
	PERFORCE_ACTION_FILE_DELETE,
	PERFORCE_ACTION_FILE_REVERT,
	PERFORCE_ACTION_FILE_SUBMIT,
};

	
//-----------------------------------------------------------------------------
// Purpose: Modal dialog for picker
//-----------------------------------------------------------------------------
class CPerforceFileListFrame : public COperationFileListFrame
{
	DECLARE_CLASS_SIMPLE( CPerforceFileListFrame, COperationFileListFrame );

public:
	CPerforceFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, PerforceAction_t action );
	virtual ~CPerforceFileListFrame();

	// Adds files to the frame
	void ClearAllFiles();
	void AddFile( const char *pFullPath );
	void AddFile( const char *pRelativePath, const char *pPathId );

	void DoModal( KeyValues *pContextKeys = NULL, const char *pMessage = NULL );

private:
	virtual bool PerformOperation();

	// Adds files for open, submit
	void AddFileForOpen( const char *pFullPath );
	void AddFileForSubmit( const char *pFullPath, P4FileState_t state );

	// Does the perforce operation
	void PerformPerforceAction( );

	PerforceAction_t m_Action;
	CUtlVector< P4File_t > m_OpenedFiles;
	CUtlString m_LastOpenedFilePathId;
};


//-----------------------------------------------------------------------------
// Show the perforce query dialog
// The specified keyvalues message will be sent either
//		1) If you open the file for add/edit
//		2) If you indicate to not add a file for add but don't hit cancel
// If a specific perforce action is specified, then the dialog will only
// be displayed if that action is appropriate
//-----------------------------------------------------------------------------
void ShowPerforceQuery( vgui::Panel *pParent, const char *pFileName, vgui::Panel *pActionSignalTarget, KeyValues *pKeyValues, PerforceAction_t actionFilter = PERFORCE_ACTION_NONE );


#endif // PERFORCEFILELISTFRAME_H

