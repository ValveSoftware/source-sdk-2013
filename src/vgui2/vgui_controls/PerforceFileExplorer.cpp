//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains a list of files, determines their perforce status
//
// $NoKeywords: $
//===========================================================================//

#include <vgui_controls/PerforceFileExplorer.h>
#include <vgui_controls/PerforceFileList.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Tooltip.h>
#include "tier1/KeyValues.h"
#include "vgui/ISystem.h"
#include "filesystem.h"
#include <ctype.h>
#include "p4lib/ip4.h"
#include "tier2/tier2.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PerforceFileExplorer::PerforceFileExplorer( Panel *pParent, const char *pPanelName ) : 
	BaseClass( pParent, pPanelName )
{
	m_pFileList = new PerforceFileList( this, "PerforceFileList" );

	// Get the list of available drives and put them in a menu here.
	// Start with the directory we are in.
	m_pFullPathCombo = new ComboBox( this, "FullPathCombo", 8, false );
	m_pFullPathCombo->GetTooltip()->SetTooltipFormatToSingleLine();

	char pFullPath[MAX_PATH];
	g_pFullFileSystem->GetCurrentDirectory( pFullPath, sizeof(pFullPath) );
	SetCurrentDirectory( pFullPath );

	m_pFullPathCombo->AddActionSignalTarget( this );

	m_pFolderUpButton = new Button(this, "FolderUpButton", "", this);
	m_pFolderUpButton->GetTooltip()->SetText( "#FileOpenDialog_ToolTip_Up" );
	m_pFolderUpButton->SetCommand( new KeyValues( "FolderUp" ) );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PerforceFileExplorer::~PerforceFileExplorer()
{
}


//-----------------------------------------------------------------------------
// Inherited from Frame
//-----------------------------------------------------------------------------
void PerforceFileExplorer::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_pFolderUpButton->AddImage( scheme()->GetImage( "resource/icon_folderup", false), -3 );
}

	
//-----------------------------------------------------------------------------
// Inherited from Frame
//-----------------------------------------------------------------------------
void PerforceFileExplorer::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, h;
	GetClientArea( x, y, w, h );

	m_pFullPathCombo->SetBounds( x, y + 6, w - 30, 24 ); 
	m_pFolderUpButton->SetBounds( x + w - 24, y + 6, 24, 24 );

	m_pFileList->SetBounds( x, y + 36, w, h - 36 );
}

	
//-----------------------------------------------------------------------------
// Sets the current directory
//-----------------------------------------------------------------------------
void PerforceFileExplorer::SetCurrentDirectory( const char *pFullPath )
{
	if ( !pFullPath )
		return;

	while ( isspace( *pFullPath ) )
	{
		++pFullPath;
	}

	if ( !pFullPath[0] )
		return;

	m_CurrentDirectory = pFullPath;
	m_CurrentDirectory.StripTrailingSlash();
    m_CurrentDirectory.FixSlashes();

	PopulateFileList();
	PopulateDriveList();

	char pCurrentDirectory[ MAX_PATH ];
	m_pFullPathCombo->GetText( pCurrentDirectory, sizeof(pCurrentDirectory) );
	if ( Q_stricmp( m_CurrentDirectory.Get(), pCurrentDirectory ) )
	{
		char pNewDirectory[ MAX_PATH ];
		Q_snprintf( pNewDirectory, sizeof(pNewDirectory), "%s\\", m_CurrentDirectory.Get() );
		m_pFullPathCombo->SetText( pNewDirectory );
		m_pFullPathCombo->GetTooltip()->SetText( pNewDirectory );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PerforceFileExplorer::PopulateDriveList()
{
	char pFullPath[MAX_PATH * 4];
	char pSubDirPath[MAX_PATH * 4];
	Q_strncpy( pFullPath, m_CurrentDirectory.Get(), sizeof( pFullPath ) );
	Q_strncpy( pSubDirPath, m_CurrentDirectory.Get(), sizeof( pSubDirPath ) );

	m_pFullPathCombo->DeleteAllItems();

	// populate the drive list
	char buf[512];
	int len = system()->GetAvailableDrives(buf, 512);
	char *pBuf = buf;
	for (int i=0; i < len / 4; i++)
	{
		m_pFullPathCombo->AddItem(pBuf, NULL);

		// is this our drive - add all subdirectories
		if ( !_strnicmp( pBuf, pFullPath, 2 ) )
		{
			int indent = 0;
			char *pData = pFullPath;
			while (*pData)
			{
				if (*pData == '\\')
				{
					if (indent > 0)
					{
						memset(pSubDirPath, ' ', indent);
						memcpy(pSubDirPath+indent, pFullPath, pData-pFullPath+1);
						pSubDirPath[indent+pData-pFullPath+1] = 0;

						m_pFullPathCombo->AddItem( pSubDirPath, NULL );
					}
					indent += 2;
				}
				pData++;
			}
		}
		pBuf += 4;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Fill the filelist with the names of all the files in the current directory
//-----------------------------------------------------------------------------
void PerforceFileExplorer::PopulateFileList()
{
	// clear the current list
	m_pFileList->RemoveAllFiles();
	
	// Create filter string
	char pFullFoundPath[MAX_PATH];
	char pFilter[MAX_PATH+3];
	Q_snprintf( pFilter, sizeof(pFilter), "%s\\*.*", m_CurrentDirectory.Get() );

	// Find all files on disk
	FileFindHandle_t h;
	const char *pFileName = g_pFullFileSystem->FindFirstEx( pFilter, NULL, &h );
	for ( ; pFileName; pFileName = g_pFullFileSystem->FindNext( h ) )
	{
		if ( !Q_stricmp( pFileName, ".." ) || !Q_stricmp( pFileName, "." ) )
			continue;

		if ( !Q_IsAbsolutePath( pFileName ) )
		{
			Q_snprintf( pFullFoundPath, sizeof(pFullFoundPath), "%s\\%s", m_CurrentDirectory.Get(), pFileName );
			pFileName = pFullFoundPath;
		}

		int nItemID = m_pFileList->AddFile( pFileName, true );
		m_pFileList->RefreshPerforceState( nItemID, true, NULL );
	}
	g_pFullFileSystem->FindClose( h );

	// Now find all files in perforce
	CUtlVector<P4File_t> &fileList = p4->GetFileList( m_CurrentDirectory );
	int nCount = fileList.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		pFileName = p4->String( fileList[i].m_sLocalFile );
		if ( !pFileName[0] )
			continue;

		int nItemID = m_pFileList->FindFile( pFileName );
		bool bFileExists = true;
		if ( nItemID == m_pFileList->InvalidItemID() )
		{
			// If it didn't find it, the file must not exist 
			// since it already would have added it above
			bFileExists = false;
			nItemID = m_pFileList->AddFile( pFileName, false, fileList[i].m_bDir );
		}
		m_pFileList->RefreshPerforceState( nItemID, bFileExists, &fileList[i] );
	}

	m_pFileList->SortList();
}


//-----------------------------------------------------------------------------
// Purpose: Handle an item in the Drive combo box being selected
//-----------------------------------------------------------------------------
void PerforceFileExplorer::OnTextChanged( KeyValues *kv )
{
	Panel *pPanel = (Panel *)kv->GetPtr( "panel", NULL );

	// first check which control had its text changed!
	if ( pPanel == m_pFullPathCombo )
	{
		char pCurrentDirectory[ MAX_PATH ];
		m_pFullPathCombo->GetText( pCurrentDirectory, sizeof(pCurrentDirectory) );
		SetCurrentDirectory( pCurrentDirectory );
		return;
	}
}


//-----------------------------------------------------------------------------
// Called when the file list was doubleclicked
//-----------------------------------------------------------------------------
void PerforceFileExplorer::OnItemDoubleClicked()
{
	if ( m_pFileList->GetSelectedItemsCount() != 1 )
		return;

	int nItemID = m_pFileList->GetSelectedItem( 0 );
	if ( m_pFileList->IsDirectoryItem( nItemID ) )
	{
		const char *pDirectoryName = m_pFileList->GetFile( nItemID );
		SetCurrentDirectory( pDirectoryName );
	}
}


//-----------------------------------------------------------------------------
// Called when the folder up button was hit
//-----------------------------------------------------------------------------
void PerforceFileExplorer::OnFolderUp()
{
	char pUpDirectory[MAX_PATH];
	Q_strncpy( pUpDirectory, m_CurrentDirectory.Get(), sizeof(pUpDirectory) );
	Q_StripLastDir( pUpDirectory, sizeof(pUpDirectory) );
	Q_StripTrailingSlash( pUpDirectory );

	// This occurs at the root directory
	if ( !Q_stricmp( pUpDirectory, "." ) )
		return;
	SetCurrentDirectory( pUpDirectory ); 
}



