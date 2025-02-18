//========= Copyright Valve Corporation, All rights reserved. ============//
//
// List of perforce files and operations
//
//=============================================================================

#include "vgui_controls/perforcefilelistframe.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls/Splitter.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/MessageBox.h"
#include "tier2/tier2.h"
#include "p4lib/ip4.h"
#include "filesystem.h"
#include "vgui/IVGui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Sort by asset name
//-----------------------------------------------------------------------------
static int __cdecl OperationSortFunc( vgui::ListPanel *pPanel, const vgui::ListPanelItem &item1, const vgui::ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("operation");
	const char *string2 = item2.kv->GetString("operation");
	int nRetVal = Q_stricmp( string1, string2 );
	if ( nRetVal != 0 )
		return nRetVal;

	string1 = item1.kv->GetString("filename");
	string2 = item2.kv->GetString("filename");
	return Q_stricmp( string1, string2 );
}

static int __cdecl FileBrowserSortFunc( vgui::ListPanel *pPanel, const vgui::ListPanelItem &item1, const vgui::ListPanelItem &item2 )
{
	const char *string1 = item1.kv->GetString("filename");
	const char *string2 = item2.kv->GetString("filename");
	return Q_stricmp( string1, string2 );
}

			 
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
COperationFileListFrame::COperationFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, bool bShowDescription, bool bShowOkOnly, int nDialogID ) :
	BaseClass( pParent, "PerforceFileList" )
{
	m_pText = NULL;
	vgui::Panel *pBrowserParent = this;

	m_pDescription = NULL;
	m_pSplitter = NULL;
	if ( bShowDescription )
	{
		m_pSplitter = new vgui::Splitter( this, "Splitter", vgui::SPLITTER_MODE_HORIZONTAL, 1 );

		pBrowserParent = m_pSplitter->GetChild( 0 );
		vgui::Panel *pDescParent = m_pSplitter->GetChild( 1 );

		m_pDescription = new vgui::TextEntry( pDescParent, "Description" );
		m_pDescription->SetMultiline( true );
		m_pDescription->SetCatchEnterKey( true );
		m_pDescription->SetText( "<enter description here>" );
	}

	// FIXME: Might be nice to have checkboxes per row
	m_pFileBrowser = new vgui::ListPanel( pBrowserParent, "Browser" );
 	m_pFileBrowser->AddColumnHeader( 0, "operation", "Operation", 52, 0 );
	m_pFileBrowser->AddColumnHeader( 1, "filename", pColumnHeader, 128, vgui::ListPanel::COLUMN_RESIZEWITHWINDOW );
    m_pFileBrowser->SetSelectIndividualCells( false );
	m_pFileBrowser->SetMultiselectEnabled( false );
	m_pFileBrowser->SetEmptyListText( "No Perforce Operations" );
 	m_pFileBrowser->SetDragEnabled( true );
 	m_pFileBrowser->AddActionSignalTarget( this );
	m_pFileBrowser->SetSortFunc( 0, OperationSortFunc );
	m_pFileBrowser->SetSortFunc( 1, FileBrowserSortFunc );
	m_pFileBrowser->SetSortColumn( 0 );

	m_pYesButton = new vgui::Button( this, "YesButton", "Yes", this, "Yes" );
	m_pNoButton = new vgui::Button( this, "NoButton", "No", this, "No" );

	SetBlockDragChaining( true );
	SetDeleteSelfOnClose( true );

	if ( bShowDescription )
	{
		LoadControlSettingsAndUserConfig( "resource/perforcefilelistdescription.res", nDialogID );
	}
	else
	{
		LoadControlSettingsAndUserConfig( "resource/perforcefilelist.res", nDialogID );
	}

	if ( bShowOkOnly )
	{
		m_pYesButton->SetText( "#MessageBox_OK" );
		m_pNoButton->SetVisible( false );
	}

	m_pContextKeyValues = NULL;

	SetTitle( pTitle, false );
}

COperationFileListFrame::~COperationFileListFrame()
{
	SaveUserConfig();
	CleanUpMessage();
	if ( m_pText )
	{
		delete[] m_pText;
	}
}


//-----------------------------------------------------------------------------
// Deletes the message
//-----------------------------------------------------------------------------
void COperationFileListFrame::CleanUpMessage()
{
	if ( m_pContextKeyValues )
	{
		m_pContextKeyValues->deleteThis();
		m_pContextKeyValues = NULL;
	}
}


//-----------------------------------------------------------------------------
// Performs layout
//-----------------------------------------------------------------------------
void COperationFileListFrame::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( m_pSplitter )
	{
		int x, y, w, h;
		GetClientArea( x, y, w, h );
		y += 6;
		h -= 36;
		m_pSplitter->SetBounds( x, y, w, h );
	}
}


//-----------------------------------------------------------------------------
// Adds files to the frame
//-----------------------------------------------------------------------------
void COperationFileListFrame::ClearAllOperations()
{
	m_pFileBrowser->RemoveAll();	
}


//-----------------------------------------------------------------------------
// Adds the strings to the list panel
//-----------------------------------------------------------------------------
void COperationFileListFrame::AddOperation( const char *pOperation, const char *pFileName )
{
	KeyValues *kv = new KeyValues( "node", "filename", pFileName );
	kv->SetString( "operation", pOperation );
	m_pFileBrowser->AddItem( kv, 0, false, false );
}

void COperationFileListFrame::AddOperation( const char *pOperation, const char *pFileName, const Color& clr )
{
	KeyValues *kv = new KeyValues( "node", "filename", pFileName );
	kv->SetString( "operation", pOperation );
	kv->SetColor( "cellcolor", clr );
	m_pFileBrowser->AddItem( kv, 0, false, false );
}


//-----------------------------------------------------------------------------
// Resizes the operation column to fit the operation text
//-----------------------------------------------------------------------------
void COperationFileListFrame::ResizeOperationColumnToContents()
{
	m_pFileBrowser->ResizeColumnToContents( 0 );
}


//-----------------------------------------------------------------------------
// Sets the column header for the 'operation' column
//-----------------------------------------------------------------------------
void COperationFileListFrame::SetOperationColumnHeaderText( const char *pText )
{
	m_pFileBrowser->SetColumnHeaderText( 0, pText );
}


//-----------------------------------------------------------------------------
// Adds the strings to the list panel
//-----------------------------------------------------------------------------
void COperationFileListFrame::DoModal( KeyValues *pContextKeyValues, const char *pMessage )
{
	m_MessageName = pMessage ? pMessage : "OperationConfirmed";
	CleanUpMessage();
	m_pContextKeyValues = pContextKeyValues;
	m_pFileBrowser->SortList();
	if ( m_pNoButton->IsVisible() )
	{
		m_pYesButton->SetEnabled( m_pFileBrowser->GetItemCount() != 0 );
	}
	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// Retrieves the number of files, the file names, and operations
//-----------------------------------------------------------------------------
int COperationFileListFrame::GetOperationCount()
{
	return m_pFileBrowser->GetItemCount();
}

const char *COperationFileListFrame::GetFileName( int i )
{
	int nItemId = m_pFileBrowser->GetItemIDFromRow( i );
	KeyValues *pKeyValues = m_pFileBrowser->GetItem( nItemId );
	return pKeyValues->GetString( "filename" );
}

const char *COperationFileListFrame::GetOperation( int i )
{
	int nItemId = m_pFileBrowser->GetItemIDFromRow( i );
	KeyValues *pKeyValues = m_pFileBrowser->GetItem( nItemId );
	return pKeyValues->GetString( "operation" );
}


//-----------------------------------------------------------------------------
// Retreives the description (only if it was shown)
//-----------------------------------------------------------------------------
const char *COperationFileListFrame::GetDescription()
{
	return m_pText; 
}

	
//-----------------------------------------------------------------------------
// Returns the message name
//-----------------------------------------------------------------------------
const char *COperationFileListFrame::CompletionMessage() 
{ 
	return m_MessageName; 
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void COperationFileListFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Yes" ) )
	{
		if ( m_pDescription )
		{
			int nLen = m_pDescription->GetTextLength() + 1;
			m_pText = new char[ nLen ];
			m_pDescription->GetText( m_pText, nLen );
		}
		
		KeyValues *pActionKeys;
		if ( PerformOperation() )
		{
			pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 1 );
		}
		else
		{
			pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 0 );
		}

		if ( m_pContextKeyValues )
		{
			pActionKeys->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	if ( !Q_stricmp( pCommand, "No" ) )
	{
		KeyValues *pActionKeys = new KeyValues( CompletionMessage(), "operationPerformed", 0 );
		if ( m_pContextKeyValues )
		{
			pActionKeys->AddSubKey( m_pContextKeyValues );
			m_pContextKeyValues = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
//
// Version that does the work of perforce actions
//
//-----------------------------------------------------------------------------
CPerforceFileListFrame::CPerforceFileListFrame( vgui::Panel *pParent, const char *pTitle, const char *pColumnHeader, PerforceAction_t action ) :
	BaseClass( pParent, pTitle, pColumnHeader, (action == PERFORCE_ACTION_FILE_SUBMIT), false, OPERATION_DIALOG_ID_PERFORCE ) 
{
	m_Action = action;
}

CPerforceFileListFrame::~CPerforceFileListFrame()
{
}


//-----------------------------------------------------------------------------
// Activates the modal dialog
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::DoModal( KeyValues *pContextKeys, const char *pMessage )
{
	BaseClass::DoModal( pContextKeys, pMessage ? pMessage : "PerforceActionConfirmed" );
}

	
//-----------------------------------------------------------------------------
// Adds a file for open
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFileForOpen( const char *pFullPath )
{
	if ( !p4 )
		return;

	bool bIsInPerforce = p4->IsFileInPerforce( pFullPath );
	bool bIsOpened = ( p4->GetFileState( pFullPath ) != P4FILE_UNOPENED );
	switch( m_Action )
	{
	case PERFORCE_ACTION_FILE_ADD:
		if ( !bIsInPerforce && !bIsOpened )
		{
			AddOperation( "Add", pFullPath );
		}
		break;

	case PERFORCE_ACTION_FILE_EDIT:
		if ( bIsInPerforce && !bIsOpened )
		{
			AddOperation( "Edit", pFullPath );
		}
		break;

	case PERFORCE_ACTION_FILE_DELETE:
		if ( bIsInPerforce && !bIsOpened )
		{
			AddOperation( "Delete", pFullPath );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Add files to dialog for submit/revert dialogs
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFileForSubmit( const char *pFullPath, P4FileState_t state )
{
	if ( state == P4FILE_UNOPENED )
		return;

	char pBuf[128];
	const char *pPrefix = (m_Action == PERFORCE_ACTION_FILE_REVERT) ? "Revert" : "Submit";
	switch( state )
	{
	case P4FILE_OPENED_FOR_ADD:
		Q_snprintf( pBuf, sizeof(pBuf), "%s Add", pPrefix );
		AddOperation( pBuf, pFullPath );
		break;

	case P4FILE_OPENED_FOR_EDIT:
		Q_snprintf( pBuf, sizeof(pBuf), "%s Edit", pPrefix );
		AddOperation( pBuf, pFullPath );
		break;

	case P4FILE_OPENED_FOR_DELETE:
		Q_snprintf( pBuf, sizeof(pBuf), "%s Delete", pPrefix );
		AddOperation( pBuf, pFullPath );
		break;

	case P4FILE_OPENED_FOR_INTEGRATE:
		Q_snprintf( pBuf, sizeof(pBuf), "%s Integrate", pPrefix );
		AddOperation( pBuf, pFullPath );
		break;
	}
}


//-----------------------------------------------------------------------------
// Version of AddFile that accepts full paths
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFile( const char *pFullPath )
{
	if ( !p4 )
		return;

	if ( m_Action < PERFORCE_ACTION_FILE_REVERT )
	{
		// If the file wasn't found on the disk, then abort
		if ( g_pFullFileSystem->FileExists( pFullPath, NULL ) )
		{
			AddFileForOpen( pFullPath );
		}
		return;
	}

	// Deal with submit, revert
	bool bFileExists = g_pFullFileSystem->FileExists( pFullPath, NULL );
	P4FileState_t state = p4->GetFileState( pFullPath );
	if ( bFileExists || (state == P4FILE_OPENED_FOR_DELETE) )
	{
		AddFileForSubmit( pFullPath, state );
	}
}


//-----------------------------------------------------------------------------
// Version of AddFile that accepts relative paths + search path ids
//-----------------------------------------------------------------------------
void CPerforceFileListFrame::AddFile( const char *pRelativePath, const char *pPathId )
{
	if ( !p4 )
		return;

	// Deal with add, open, edit
	if ( m_Action < PERFORCE_ACTION_FILE_REVERT )
	{
		// If the file wasn't found on the disk, then abort
		if ( g_pFullFileSystem->FileExists( pRelativePath, pPathId ) )
		{
			char pFullPath[MAX_PATH];
			g_pFullFileSystem->RelativePathToFullPath( pRelativePath, pPathId, pFullPath, sizeof( pFullPath ) );
			AddFileForOpen( pFullPath );
		}
		return;
	}

	// Deal with submit, revert

	// First, handle the case where the file exists on the drive
	char pFullPath[MAX_PATH];
	if ( g_pFullFileSystem->FileExists( pRelativePath, pPathId ) )
	{
		g_pFullFileSystem->RelativePathToFullPath( pRelativePath, pPathId, pFullPath, sizeof( pFullPath ) );
		P4FileState_t state = p4->GetFileState( pFullPath );
		AddFileForSubmit( pFullPath, state );
		return;
	}

	// Get the list of opened files, cache it off so we aren't continually reasking
	if ( Q_stricmp( pPathId, m_LastOpenedFilePathId ) )
	{
		p4->GetOpenedFileListInPath( pPathId, m_OpenedFiles );
		m_LastOpenedFilePathId = pPathId;
	}

	// If the file doesn't exist, it was opened for delete. 
	// Using the client spec of the path, we need to piece together 
	// the full path; the full path unfortunately is usually ambiguous: 
	// you can never exactly know which mod it came from.
	char pTemp[MAX_PATH];
	char pSearchString[MAX_PATH];
	Q_strncpy( pSearchString, pRelativePath, sizeof(pSearchString) );
	Q_FixSlashes( pSearchString );

	int k;
	int nOpenedFileCount = m_OpenedFiles.Count();
	for ( k = 0; k < nOpenedFileCount; ++k )
	{
		if ( m_OpenedFiles[k].m_eOpenState != P4FILE_OPENED_FOR_DELETE )
			continue;

		// Check to see if the end of the local file matches the file
		const char *pLocalFile = p4->String( m_OpenedFiles[k].m_sLocalFile );

		// This ensures the full path lies under the search path
		if ( !g_pFullFileSystem->FullPathToRelativePathEx( pLocalFile, pPathId, pTemp, sizeof(pTemp) ) )
			continue;

		// The relative paths had better be the same
		if ( Q_stricmp( pTemp, pSearchString ) )
			continue;

		AddFileForSubmit( pLocalFile, m_OpenedFiles[k].m_eOpenState );
		break;
	}
}


//-----------------------------------------------------------------------------
// Does the perforce operation
//-----------------------------------------------------------------------------
bool CPerforceFileListFrame::PerformOperation( )
{
	if ( !p4 )
		return false;

	int nFileCount = GetOperationCount();
	const char **ppFileNames = (const char**)_alloca( nFileCount * sizeof(char*) );
	for ( int i = 0; i < nFileCount; ++i )
	{
		ppFileNames[i] = GetFileName( i );
	}

	bool bSuccess = false;

	switch ( m_Action )
	{
	case PERFORCE_ACTION_FILE_ADD:
		bSuccess = p4->OpenFilesForAdd( nFileCount, ppFileNames );
		break;

	case PERFORCE_ACTION_FILE_EDIT:
		bSuccess = p4->OpenFilesForEdit( nFileCount, ppFileNames );
		break;

	case PERFORCE_ACTION_FILE_DELETE:
		bSuccess = p4->OpenFilesForDelete( nFileCount, ppFileNames );
		break;

	case PERFORCE_ACTION_FILE_REVERT:
		bSuccess = p4->RevertFiles( nFileCount, ppFileNames );
		break;

	case PERFORCE_ACTION_FILE_SUBMIT:
		{ 
			// Ensure a description was added
			const char *pDescription = GetDescription();
			if ( !pDescription[0] || !Q_stricmp( pDescription, "<enter description here>" ) )
			{
				vgui::MessageBox *pError = new vgui::MessageBox( "Submission Error!", "Description required for submission.", GetParent() );
				pError->SetSmallCaption( true );
				pError->DoModal();
				return false;
			}
			else
			{
				bSuccess = p4->SubmitFiles( nFileCount, ppFileNames, pDescription );
			}
		}
		break;
	}

	const char *pErrorString = p4->GetLastError();
	if ( !bSuccess )
	{
		vgui::MessageBox *pError = new vgui::MessageBox( "Perforce Error!", pErrorString, GetParent() );
		pError->SetSmallCaption( true );
		pError->DoModal();
	}
#if 0
	if ( *pErrorString )
	{
		if ( V_strstr( pErrorString, "opened for add" ) )
			return bSuccess;
		if ( V_strstr( pErrorString, "opened for edit" ) )
			return bSuccess;
		// TODO - figure out the rest of these...

		const char *pPrefix =	"Perforce has generated the following message which may or may not be an error.\n"
								"Please email joe with the text of the message, whether you think it was an error, and what perforce operation you where performing.\n"
								"To copy the message, hit ~ to enter the console, where you will find the message reprinted.\n"
								"Select the lines of text in the message, right click, select Copy, and then paste into an email message.\n\n";
		static int nPrefixLen = V_strlen( pPrefix );
		int nErrorStringLength = V_strlen( pErrorString );
		char *pMsg = (char*)_alloca( nPrefixLen + nErrorStringLength + 1 );
		V_strcpy( pMsg, pPrefix );
		V_strcpy( pMsg + nPrefixLen, pErrorString );

		vgui::MessageBox *pError = new vgui::MessageBox( "Dubious Perforce Message", pMsg, GetParent() );
		pError->SetSmallCaption( true );
		pError->DoModal();
	}
#endif
	return bSuccess;
}


//-----------------------------------------------------------------------------
// Show the perforce query dialog
//-----------------------------------------------------------------------------
void ShowPerforceQuery( vgui::Panel *pParent, const char *pFileName, vgui::Panel *pActionSignalTarget, KeyValues *pKeyValues, PerforceAction_t actionFilter )
{
	if ( !p4 )
	{
		KeyValues *pSpoofKeys = new KeyValues( "PerforceQueryCompleted", "operationPerformed", 1 );
		if ( pKeyValues )
		{
			pSpoofKeys->AddSubKey( pKeyValues );
		}
		vgui::ivgui()->PostMessage( pActionSignalTarget->GetVPanel(), pSpoofKeys, 0 );

		return;
	}

	// Refresh the current perforce settings
	p4->RefreshActiveClient();

	PerforceAction_t action = PERFORCE_ACTION_NONE;
	const char *pTitle = NULL;
	if ( !p4->IsFileInPerforce( pFileName )	)
	{
		// If the file isn't in perforce, ask to add it
		action = PERFORCE_ACTION_FILE_ADD;
		pTitle = "Add File to Perforce?";
	}
	else if ( p4->GetFileState( pFileName ) == P4FILE_UNOPENED )
	{
		// If the file isn't checked out yet, ask to check it out
		action = PERFORCE_ACTION_FILE_EDIT;
		pTitle = "Check Out File from Perforce?";
	}

	if ( ( action == PERFORCE_ACTION_NONE ) || ( ( actionFilter != PERFORCE_ACTION_NONE ) && ( actionFilter != action ) ) )
	{
		// Spoof a completion event
		KeyValues *pSpoofKeys = new KeyValues( "PerforceQueryCompleted", "operationPerformed", 1 );
		if ( pKeyValues )
		{
			pSpoofKeys->AddSubKey( pKeyValues );
		}
		vgui::ivgui()->PostMessage( pActionSignalTarget->GetVPanel(), pSpoofKeys, 0 );
		return;
	}

	CPerforceFileListFrame *pQuery = new CPerforceFileListFrame( pParent, pTitle, "File", action );
	pQuery->AddFile( pFileName );
	if ( pActionSignalTarget )
	{
		pQuery->AddActionSignalTarget( pActionSignalTarget );
	}
	pQuery->DoModal( pKeyValues, "PerforceQueryCompleted" );
}
