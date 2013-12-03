//========= Copyright Valve Corporation, All rights reserved. ============//
//
// This is a helper class designed to help with the chains of modal dialogs
// encountered when trying to open or save a particular file
//
//=============================================================================

#include "vgui_controls/FileOpenStateMachine.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/MessageBox.h"
#include "vgui_controls/perforcefilelistframe.h"
#include "vgui_controls/savedocumentquery.h"
#include "filesystem.h"
#include "p4lib/ip4.h"
#include "tier2/tier2.h"
#include "tier0/icommandline.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
FileOpenStateMachine::FileOpenStateMachine( vgui::Panel *pParent, IFileOpenStateMachineClient *pClient ) : BaseClass( pParent, "FileOpenStateMachine" )
{
	m_pClient = pClient;
	m_CompletionState = SUCCESSFUL;
	m_CurrentState = STATE_NONE;
	m_pContextKeyValues = NULL;
	SetVisible( false );
}

FileOpenStateMachine::~FileOpenStateMachine()
{
	CleanUpContextKeyValues();
}


//-----------------------------------------------------------------------------
// Cleans up keyvalues
//-----------------------------------------------------------------------------
void FileOpenStateMachine::CleanUpContextKeyValues()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Returns the state machine completion state
//-----------------------------------------------------------------------------
FileOpenStateMachine::CompletionState_t FileOpenStateMachine::GetCompletionState()
{
	return m_CompletionState;
}


//-----------------------------------------------------------------------------
// Utility to set the completion state
//-----------------------------------------------------------------------------
void FileOpenStateMachine::SetCompletionState( FileOpenStateMachine::CompletionState_t state )
{
	m_CompletionState = state;
	if ( m_CompletionState == IN_PROGRESS )
		return;

	m_CurrentState = STATE_NONE;

	KeyValues *kv = new KeyValues( "FileStateMachineFinished" );
	kv->SetInt( "completionState", m_CompletionState );
	kv->SetInt( "wroteFile", m_bWroteFile );
	kv->SetString( "fullPath", m_FileName.Get() );
	kv->SetString( "fileType", m_bIsOpeningFile ? m_OpenFileType.Get() : m_SaveFileType.Get() );
	if ( m_pContextKeyValues )
	{
		kv->AddSubKey( m_pContextKeyValues );
		m_pContextKeyValues = NULL;
	}
	PostActionSignal( kv );
}


//-----------------------------------------------------------------------------
// Called by the message box in OverwriteFileDialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnOverwriteFile( )
{
	CheckOutDialog( );
}

void FileOpenStateMachine::OnCancelOverwriteFile( )
{
	SetCompletionState( FILE_NOT_OVERWRITTEN );
}


//-----------------------------------------------------------------------------
// Shows the overwrite existing file dialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OverwriteFileDialog( )
{
	if ( !g_pFullFileSystem->FileExists( m_FileName ) )
	{
		CheckOutDialog( );
		return;
	}

	m_CurrentState = STATE_SHOWING_OVERWRITE_DIALOG;

	char pBuf[1024];
	Q_snprintf( pBuf, sizeof(pBuf), "File already exists. Overwrite it?\n\n\"%s\"\n", m_FileName.Get() ); 
	vgui::MessageBox *pMessageBox = new vgui::MessageBox( "Overwrite Existing File?", pBuf, GetParent() );
	pMessageBox->AddActionSignalTarget( this );
	pMessageBox->SetOKButtonVisible( true );
	pMessageBox->SetOKButtonText( "Yes" );
	pMessageBox->SetCancelButtonVisible( true );
	pMessageBox->SetCancelButtonText( "No" );
	pMessageBox->SetCloseButtonVisible( false ); 
	pMessageBox->SetCommand( new KeyValues( "OverwriteFile" ) );
	pMessageBox->SetCancelCommand( new KeyValues( "CancelOverwriteFile" ) );
	pMessageBox->DoModal();
}


//-----------------------------------------------------------------------------
// Used to open a particular file in perforce, and deal with all the lovely dialogs
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnFileSelectionCancelled()
{
	if ( m_CurrentState == STATE_SHOWING_SAVE_DIALOG )
	{
		SetCompletionState( FILE_SAVE_NAME_NOT_SPECIFIED );
		return;
	}

	if ( m_CurrentState == STATE_SHOWING_OPEN_DIALOG )
	{
		SetCompletionState( FILE_OPEN_NAME_NOT_SPECIFIED );
		return;
	}

	Assert(0);
}


//-----------------------------------------------------------------------------
// Used to open a particular file in perforce, and deal with all the lovely dialogs
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnFileSelected( KeyValues *pKeyValues )
{
	if ( m_CurrentState == STATE_SHOWING_SAVE_DIALOG )
	{
		m_FileName = pKeyValues->GetString( "fullpath" );
		const char *pFilterInfo = pKeyValues->GetString( "filterinfo" );
		if ( pFilterInfo )
		{
			m_SaveFileType = pFilterInfo;
		}
		OverwriteFileDialog();
		return;
	}

	if ( m_CurrentState == STATE_SHOWING_OPEN_DIALOG )
	{
		m_FileName = pKeyValues->GetString( "fullpath" );
		const char *pFilterInfo = pKeyValues->GetString( "filterinfo" );
		if ( pFilterInfo )
		{
			m_OpenFileType = pFilterInfo;
		}
		ReadFile( );
		return;
	}

	Assert(0);
}


//-----------------------------------------------------------------------------
// Writes the file out
//-----------------------------------------------------------------------------
void FileOpenStateMachine::WriteFile()
{
	m_CurrentState = STATE_WRITING_FILE;
	if ( !m_pClient->OnWriteFileToDisk( m_FileName, m_SaveFileType, m_pContextKeyValues ) )
	{
		SetCompletionState( ERROR_WRITING_FILE );
		return;
	}

	m_bWroteFile = true;
	if ( m_bShowPerforceDialogs )
	{
		m_CurrentState = STATE_SHOWING_PERFORCE_ADD_DIALOG;
		ShowPerforceQuery( GetParent(), m_FileName, this, NULL, PERFORCE_ACTION_FILE_ADD );
		return;
	}

	if ( !m_bIsOpeningFile )
	{
		SetCompletionState( SUCCESSFUL );
		return;
	}

	OpenFileDialog();
}


//-----------------------------------------------------------------------------
// Called by the message box in MakeFileWriteableDialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnMakeFileWriteable( )
{
	if ( !g_pFullFileSystem->SetFileWritable( m_FileName, true ) )
	{
		SetCompletionState( ERROR_MAKING_FILE_WRITEABLE );
		return;
	}

	WriteFile();
}

void FileOpenStateMachine::OnCancelMakeFileWriteable( )
{
	SetCompletionState( FILE_NOT_MADE_WRITEABLE );
}


//-----------------------------------------------------------------------------
// Shows the make file writeable dialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::MakeFileWriteableDialog( )
{
	// If the file is writeable, write it!
	if ( !g_pFullFileSystem->FileExists( m_FileName ) || g_pFullFileSystem->IsFileWritable( m_FileName ) )
	{
		WriteFile();
		return;
	}

	// If it's in perforce, and not checked out, then we must abort.
	bool bIsInPerforce = p4->IsFileInPerforce( m_FileName );
	bool bIsOpened = ( p4->GetFileState( m_FileName ) != P4FILE_UNOPENED );
	if ( bIsInPerforce && !bIsOpened )
	{
		SetCompletionState( FILE_NOT_CHECKED_OUT );
		return;
	}

	m_CurrentState = STATE_SHOWING_MAKE_FILE_WRITEABLE_DIALOG;

	char pBuf[1024];
	Q_snprintf( pBuf, sizeof(pBuf), "Encountered read-only file. Should it be made writeable?\n\n\"%s\"\n", m_FileName.Get() ); 
	vgui::MessageBox *pMessageBox = new vgui::MessageBox( "Make File Writeable?", pBuf, GetParent() );
	pMessageBox->AddActionSignalTarget( this );
	pMessageBox->SetOKButtonVisible( true );
	pMessageBox->SetOKButtonText( "Yes" );
	pMessageBox->SetCancelButtonVisible( true );
	pMessageBox->SetCancelButtonText( "No" );
	pMessageBox->SetCloseButtonVisible( false ); 
	pMessageBox->SetCommand( new KeyValues( "MakeFileWriteable" ) );
	pMessageBox->SetCancelCommand( new KeyValues( "CancelMakeFileWriteable" ) );
	pMessageBox->DoModal();
}


//-----------------------------------------------------------------------------
// Called when ShowPerforceQuery completes
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnPerforceQueryCompleted( KeyValues *pKeyValues )
{
	if ( m_CurrentState == STATE_SHOWING_CHECK_OUT_DIALOG )
	{
		if ( pKeyValues->GetInt( "operationPerformed" ) == 0 )
		{
			SetCompletionState( FILE_NOT_CHECKED_OUT );
			return;
		}

		MakeFileWriteableDialog();
		return;
	}

	if ( m_CurrentState == STATE_SHOWING_PERFORCE_ADD_DIALOG )
	{
		if ( !m_bIsOpeningFile )
		{
			SetCompletionState( SUCCESSFUL );
			return;
		}

		OpenFileDialog();
		return;
	}

	Assert(0);
}


//-----------------------------------------------------------------------------
// Used to open a particular file in perforce, and deal with all the lovely dialogs
//-----------------------------------------------------------------------------
void FileOpenStateMachine::CheckOutDialog( )
{
	if ( m_bShowPerforceDialogs )
	{
		m_CurrentState = STATE_SHOWING_CHECK_OUT_DIALOG;
		ShowPerforceQuery( GetParent(), m_FileName, this, NULL, PERFORCE_ACTION_FILE_EDIT );
		return;
	}

	WriteFile();
}


//-----------------------------------------------------------------------------
// These 3 messages come from the savedocumentquery dialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OnSaveFile()
{
	if ( !m_FileName[0] || !Q_IsAbsolutePath( m_FileName ) )
	{
		m_CurrentState = STATE_SHOWING_SAVE_DIALOG;

		FileOpenDialog *pDialog = new FileOpenDialog( GetParent(), "Save As", false );
		m_pClient->SetupFileOpenDialog( pDialog, false, m_SaveFileType, m_pContextKeyValues );
		pDialog->SetDeleteSelfOnClose( true );
		pDialog->AddActionSignalTarget( this );
		pDialog->DoModal( );
		return;
	}

	CheckOutDialog( );
}

void FileOpenStateMachine::OnMarkNotDirty()
{
	if ( !m_bIsOpeningFile )
	{
		SetCompletionState( SUCCESSFUL );
		return;
	}

	// Jump right to opening the file
	OpenFileDialog( );
}

void FileOpenStateMachine::OnCancelSaveDocument()
{
	SetCompletionState( FILE_SAVE_CANCELLED );
}


//-----------------------------------------------------------------------------
// Show the save document query dialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::ShowSaveQuery( )
{
	m_CurrentState = STATE_SHOWING_SAVE_DIRTY_FILE_DIALOG;
	ShowSaveDocumentQuery( GetParent(), m_FileName, m_SaveFileType, 0, this, NULL );
}


//-----------------------------------------------------------------------------
// Used to save a specified file, and deal with all the lovely dialogs
//-----------------------------------------------------------------------------
void FileOpenStateMachine::SaveFile( KeyValues *pContextKeyValues, const char *pFileName, const char *pFileType, int nFlags )
{
	CleanUpContextKeyValues();
	SetCompletionState( IN_PROGRESS );
	m_pContextKeyValues = pContextKeyValues;
	m_FileName = pFileName;
	m_SaveFileType = pFileType;
	m_OpenFileType = NULL;
	m_OpenFileName = NULL;

	// Clear the P4 dialog flag for SDK users and licensees without Perforce
	if ( CommandLine()->FindParm( "-nop4" ) )
	{
		nFlags &= ~FOSM_SHOW_PERFORCE_DIALOGS;
	}

	m_bShowPerforceDialogs = ( nFlags & FOSM_SHOW_PERFORCE_DIALOGS ) != 0;
	m_bShowSaveQuery = ( nFlags & FOSM_SHOW_SAVE_QUERY ) != 0;
	m_bIsOpeningFile = false;
	m_bWroteFile = false;

	if ( m_bShowSaveQuery )
	{
		ShowSaveQuery();
		return;
	}

	OnSaveFile();
}


//-----------------------------------------------------------------------------
// Reads the file in
//-----------------------------------------------------------------------------
void FileOpenStateMachine::ReadFile()
{
	m_CurrentState = STATE_READING_FILE;
	if ( !m_pClient->OnReadFileFromDisk( m_FileName, m_OpenFileType, m_pContextKeyValues ) )
	{
		SetCompletionState( ERROR_READING_FILE );
		return;
	}

	SetCompletionState( SUCCESSFUL );
}


//-----------------------------------------------------------------------------
// Shows the open file dialog
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OpenFileDialog( )
{
	m_CurrentState = STATE_SHOWING_OPEN_DIALOG;

	if ( m_OpenFileName.IsEmpty() )
	{
		FileOpenDialog *pDialog = new FileOpenDialog( GetParent(), "Open", true );
		m_pClient->SetupFileOpenDialog( pDialog, true, m_OpenFileType, m_pContextKeyValues );
		pDialog->SetDeleteSelfOnClose( true );
		pDialog->AddActionSignalTarget( this );
		pDialog->DoModal( );
	}
	else
	{
		m_FileName = m_OpenFileName;
		ReadFile();
	}
}


//-----------------------------------------------------------------------------
// Opens a file, saves an existing one if necessary
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OpenFile( const char *pOpenFileType, KeyValues *pContextKeyValues, const char *pSaveFileName, const char *pSaveFileType, int nFlags )
{
	CleanUpContextKeyValues();
	SetCompletionState( IN_PROGRESS );
	m_pContextKeyValues = pContextKeyValues;
	m_FileName = pSaveFileName;
	m_SaveFileType = pSaveFileType;
	m_OpenFileType = pOpenFileType;
	m_OpenFileName = NULL;
	m_bShowPerforceDialogs = ( nFlags & FOSM_SHOW_PERFORCE_DIALOGS ) != 0;
	m_bShowSaveQuery = ( nFlags & FOSM_SHOW_SAVE_QUERY ) != 0;
	m_bIsOpeningFile = true;
	m_bWroteFile = false;

	if ( m_bShowSaveQuery )
	{
		ShowSaveQuery();
		return;
	}

	OpenFileDialog();
}


//-----------------------------------------------------------------------------
// Version of OpenFile that skips browsing for a particular file to open
//-----------------------------------------------------------------------------
void FileOpenStateMachine::OpenFile( const char *pOpenFileName, const char *pOpenFileType, KeyValues *pContextKeyValues, const char *pSaveFileName, const char *pSaveFileType, int nFlags )
{
	CleanUpContextKeyValues();
	SetCompletionState( IN_PROGRESS );
	m_pContextKeyValues = pContextKeyValues;
	m_FileName = pSaveFileName;
	m_SaveFileType = pSaveFileType;
	m_OpenFileType = pOpenFileType;
	m_bShowPerforceDialogs = ( nFlags & FOSM_SHOW_PERFORCE_DIALOGS ) != 0;
	m_bShowSaveQuery = ( nFlags & FOSM_SHOW_SAVE_QUERY ) != 0;
	m_bIsOpeningFile = true;
	m_bWroteFile = false;
	m_OpenFileName = pOpenFileName;
	if ( m_bShowSaveQuery )
	{
		ShowSaveQuery();
		return;
	}

	OpenFileDialog();
}


