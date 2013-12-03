//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains a list of files, determines their perforce status
//
// $NoKeywords: $
//===========================================================================//

#include <vgui_controls/PerforceFileList.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImageList.h>
#include "tier1/KeyValues.h"
#include <vgui/ISurface.h>
#include "filesystem.h"
#include "p4lib/ip4.h"
#include "tier2/tier2.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


using namespace vgui;


static int ListFileNameSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	NOTE_UNUSED( pPanel );

	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if ( dir1 != dir2 )
	{
		return dir1 ? -1 : 1;
	}

	const char *string1 = item1.kv->GetString("text");
	const char *string2 = item2.kv->GetString("text");

	// YWB:  Mimic windows behavior where filenames starting with numbers are sorted based on numeric part
	int num1 = Q_atoi( string1 );
	int num2 = Q_atoi( string2 );

	if ( num1 != 0 && 
		 num2 != 0 )
	{
		if ( num1 < num2 )
			return -1;
		else if ( num1 > num2 )
			return 1;
	}

	// Push numbers before everything else
	if ( num1 != 0 )
	{
		return -1;
	}
	
	// Push numbers before everything else
	if ( num2 != 0 )
	{
		return 1;
	}

	return Q_stricmp( string1, string2 );
}

static int ListBaseStringSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2, char const *fieldName )
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	const char *string1 = item1.kv->GetString(fieldName);
	const char *string2 = item2.kv->GetString(fieldName);
	int cval = Q_stricmp(string1, string2);
	if ( cval == 0 )
	{
		// Use filename to break ties
		return ListFileNameSortFunc( pPanel, item1, item2 );
	}

	return cval;
}

static int ListBaseIntegerSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2, char const *fieldName )
{
	bool dir1 = item1.kv->GetInt("directory") == 1;
	bool dir2 = item2.kv->GetInt("directory") == 1;

	// if they're both not directories of files, return if dir1 is a directory (before files)
	if (dir1 != dir2)
	{
		return -1;
	}

	int i1 = item1.kv->GetInt(fieldName);
	int i2 = item2.kv->GetInt(fieldName);
	if ( i1 == i2 )
	{
		// Use filename to break ties
		return ListFileNameSortFunc( pPanel, item1, item2 );
	}

	return ( i1 < i2 ) ? -1 : 1;
}

static int ListFileSizeSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseIntegerSortFunc( pPanel, item1, item2, "filesizeint" );
}

static int ListFileAttributesSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseStringSortFunc( pPanel, item1, item2, "attributes" );
}

static int ListFileTypeSortFunc(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2 )
{
	return ListBaseStringSortFunc( pPanel, item1, item2, "type" );
}


//-----------------------------------------------------------------------------
// Dictionary of start dir contexts 
//-----------------------------------------------------------------------------
struct ColumnInfo_t
{
	char const	*columnName;
	char const	*columnText;
	int			startingWidth;
	int			minWidth;
	int			maxWidth;
	int			flags;
	SortFunc	*pfnSort;
	Label::Alignment alignment;
};

static ColumnInfo_t g_ColInfo[] =
{
	{	"text",				"#PerforceFileList_Col_Name",			175,	20, 10000, ListPanel::COLUMN_UNHIDABLE,		&ListFileNameSortFunc			, Label::a_west },
	{	"type",				"#PerforceFileList_Col_Type",			150,	20, 10000, 0,								&ListFileTypeSortFunc			, Label::a_west },
	{	"in_perforce",		"#PerforceFileList_Col_InPerforce",		50,		20, 10000, ListPanel::COLUMN_UNHIDABLE,		&ListFileAttributesSortFunc		, Label::a_west },
	{	"synched",			"#PerforceFileList_Col_Synched",		50,		20, 10000, ListPanel::COLUMN_UNHIDABLE,		&ListFileAttributesSortFunc		, Label::a_west },
	{	"checked_out",		"#PerforceFileList_Col_Checked_Out",	50,		20, 10000, ListPanel::COLUMN_UNHIDABLE,		&ListFileAttributesSortFunc		, Label::a_west },
	{	"attributes",		"#PerforceFileList_Col_Attributes",		50,		20, 10000, ListPanel::COLUMN_HIDDEN,		&ListFileAttributesSortFunc		, Label::a_west },
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
PerforceFileList::PerforceFileList( Panel *pParent, const char *pPanelName ) : 
	BaseClass( pParent, pPanelName )
{
	SetMultiselectEnabled( false );
	m_bShowDeletedFiles = false;

	// list panel
	for ( int i = 0; i < ARRAYSIZE( g_ColInfo ); ++i )
	{
		const ColumnInfo_t& info = g_ColInfo[ i ];

		AddColumnHeader( i, info.columnName, info.columnText, info.startingWidth, info.minWidth, info.maxWidth, info.flags );
		SetSortFunc( i, info.pfnSort );
		SetColumnTextAlignment( i, info.alignment );
	}

	SetSortColumn( 0 );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
PerforceFileList::~PerforceFileList()
{
}


//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void PerforceFileList::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	ImageList *pImageList = new ImageList( false );
	pImageList->AddImage( scheme()->GetImage( "resource/icon_file", false ) );
	pImageList->AddImage( scheme()->GetImage( "resource/icon_folder", false ) );
	pImageList->AddImage( scheme()->GetImage( "resource/icon_folder_selected", false ) );

	SetImageList( pImageList, true );
}


//-----------------------------------------------------------------------------
// Toggle showing deleted files or not
//-----------------------------------------------------------------------------
void PerforceFileList::ShowDeletedFiles( bool bShowDeletedFiles )
{
	if ( m_bShowDeletedFiles != bShowDeletedFiles )
	{
		m_bShowDeletedFiles = bShowDeletedFiles;

		for ( int i = FirstItem(); i != InvalidItemID(); i = NextItem( i ) )
		{
			KeyValues *pKeyValues = GetItem( i ); 
			if ( !pKeyValues->GetInt( "deleted", 0 ) )
				continue;

			SetItemVisible( i, m_bShowDeletedFiles );
		}
	}
}


//-----------------------------------------------------------------------------
// Add a directory to the directory list, returns client spec
//-----------------------------------------------------------------------------
void PerforceFileList::AddItemToDirectoryList( const char *pFullPath, int nItemID, bool bIsDirectory )
{
	char pDirectoryBuf[MAX_PATH];
	Q_ExtractFilePath( pFullPath, pDirectoryBuf, sizeof(pDirectoryBuf) );
	Q_StripTrailingSlash( pDirectoryBuf ); 
	pFullPath = pDirectoryBuf;

	DirectoryInfo_t *pInfo;
	UtlSymId_t i = m_Directories.Find( pFullPath );
	if ( i != m_Directories.InvalidIndex() )
	{
		pInfo = &m_Directories[i];
	}
	else
	{
		char pClientSpec[MAX_PATH];
		if ( !p4->GetClientSpecForDirectory( pFullPath, pClientSpec, sizeof(pClientSpec) ) )
		{
			pClientSpec[0] = 0;
		}

		pInfo = &m_Directories[ pFullPath ];
		pInfo->m_ClientSpec = pClientSpec;
	}

	pInfo->m_ItemIDs.AddToTail( nItemID );
}

	
//-----------------------------------------------------------------------------
// Add a file to the file list. 
//-----------------------------------------------------------------------------
int PerforceFileList::AddFileToFileList( const char *pFullPath, bool bExistsOnDisk )
{
	bool bIsFileWriteable = bExistsOnDisk ? g_pFullFileSystem->IsFileWritable( pFullPath, NULL ) : true;

	// add the file to the list
	KeyValues *kv = new KeyValues("item");

	const char *pRelativePath = Q_UnqualifiedFileName( pFullPath );
	kv->SetString( "text", pRelativePath );
	kv->SetString( "fullpath", pFullPath );
	kv->SetInt( "image", 1 );

	IImage *pImage = surface()->GetIconImageForFullPath( pFullPath );
	if ( pImage )
	{
		kv->SetPtr( "iconImage", (void *)pImage );
	}

	kv->SetInt( "imageSelected", 1 );
	kv->SetInt( "directory", 0 );

	// These are computed by Refresh
	kv->SetInt( "in_perforce", 0 );
	kv->SetInt( "synched", 0 );
	kv->SetInt( "checked_out", 0 );
	kv->SetInt( "deleted", 0 );

	wchar_t pFileType[ 80 ];
	g_pFullFileSystem->GetFileTypeForFullPath( pFullPath, pFileType, sizeof( pFileType ) );

	kv->SetWString( "type", pFileType );
	kv->SetString( "attributes", bIsFileWriteable ? "" : "R" );

	int nItemID = AddItem( kv, 0, false, false );
	kv->deleteThis();

	AddItemToDirectoryList( pFullPath, nItemID, false );
	return nItemID;
}


//-----------------------------------------------------------------------------
// Add a directory to the file list. 
//-----------------------------------------------------------------------------
int PerforceFileList::AddDirectoryToFileList( const char *pFullPath, bool bExistsOnDisk )
{
	KeyValues *kv = new KeyValues("item");

	const char *pRelativePath = Q_UnqualifiedFileName( pFullPath );
	kv->SetString( "text", pRelativePath );
	kv->SetString( "fullpath", pFullPath );
	kv->SetPtr( "iconImage", (void *)NULL );
	kv->SetInt( "image", 2 );
	kv->SetInt( "imageSelected", 3 );
	kv->SetInt( "directory", 1 );

	// These are computed by Refresh
	kv->SetInt( "in_perforce", 0 );
	kv->SetInt( "synched", 0 );
	kv->SetInt( "checked_out", 0 );
	kv->SetInt( "deleted", 0 );

	kv->SetString( "type", "#PerforceFileList_FileType_Folder" );
	kv->SetString( "attributes", "D" );

	int nItemID = AddItem(kv, 0, false, false);
	kv->deleteThis();

	AddItemToDirectoryList( pFullPath, nItemID, true );
	return nItemID;
}


//-----------------------------------------------------------------------------
// Add a file or directory to the file list. 
//-----------------------------------------------------------------------------
int PerforceFileList::AddFile( const char *pFullPath, int nFileExists, int nIsDirectory )
{
	if ( !pFullPath )
		return InvalidItemID();

	if ( !Q_IsAbsolutePath( pFullPath ) )
	{
		Warning( "Absolute paths required for PerforceFileList::AddFile!\n"
			"\"%s\" is not an abolute path", pFullPath );
		return InvalidItemID();
	}

	char pFixedPath[MAX_PATH];
	Q_strncpy( pFixedPath, pFullPath, sizeof(pFixedPath) );
	Q_FixSlashes( pFixedPath );

	// Check to see if the file is on disk
	int nItemID = -1;
	bool bFileExists, bIsDirectory;
	if ( nFileExists < 0 )
	{
		bFileExists = g_pFullFileSystem->FileExists( pFixedPath ) ;
	}
	else
	{
		bFileExists = ( nFileExists != 0 ); 
	}

	if ( nIsDirectory < 0 )
	{
		if ( bFileExists )
		{
			bIsDirectory = g_pFullFileSystem->IsDirectory( pFixedPath );
		}
		else
		{
			int nLen = Q_strlen( pFixedPath );
			bIsDirectory = ( pFixedPath[nLen-1] == CORRECT_PATH_SEPARATOR );
		}
	}
	else
	{
		bIsDirectory = ( nIsDirectory != 0 ); 
	}

	if ( bIsDirectory )
	{
		nItemID = AddDirectoryToFileList( pFixedPath, bFileExists );
	}
	else
	{
		nItemID = AddFileToFileList( pFixedPath, bFileExists );
	}

	return nItemID;
}


//-----------------------------------------------------------------------------
// Remove all files from the list
//-----------------------------------------------------------------------------
void PerforceFileList::RemoveAllFiles()
{
	RemoveAll();
	m_Directories.Clear();
}


//-----------------------------------------------------------------------------
// Finds a file in the p4 list
//-----------------------------------------------------------------------------
static P4File_t *FindFileInPerforceList( const char *pFileName, CUtlVector<P4File_t> &fileList, bool *pFound )
{
	int nCount = fileList.Count();
	for ( int i = 0; i < nCount; ++i )
	{
		if ( pFound[i] )
			continue;

		const char *pPerforceFileName = p4->String( fileList[i].m_sLocalFile );
		if ( !Q_stricmp( pPerforceFileName, pFileName ) )
		{
			pFound[i] = true;
			return &fileList[i];
		}
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Refresh perforce information
//-----------------------------------------------------------------------------
void PerforceFileList::RefreshPerforceState( int nItemID, bool bFileExists, P4File_t *pFileInfo )
{
	KeyValues *kv = GetItem( nItemID );

	bool bIsSynched = false;
	bool bIsFileInPerforce = (pFileInfo != NULL);
	if ( bIsFileInPerforce )
	{
		if ( pFileInfo->m_bDeleted != bFileExists )
		{
			bIsSynched = ( pFileInfo->m_bDeleted || ( pFileInfo->m_iHeadRevision == pFileInfo->m_iHaveRevision ) );
		}
	}
	else
	{
		bIsSynched = !bFileExists;
	}

	bool bIsDeleted = bIsFileInPerforce && !bFileExists && pFileInfo->m_bDeleted;

	kv->SetInt( "in_perforce", bIsFileInPerforce );
	kv->SetInt( "synched", bIsSynched );
	kv->SetInt( "checked_out", bIsFileInPerforce && ( pFileInfo->m_eOpenState != P4FILE_UNOPENED ) );
	kv->SetInt( "deleted", bIsDeleted ); 

	if ( bIsDeleted )
	{
		SetItemVisible( nItemID, m_bShowDeletedFiles );
	}
}


//-----------------------------------------------------------------------------
// Refresh perforce information
//-----------------------------------------------------------------------------
void PerforceFileList::Refresh()
{
	/*
	// Slow method.. does too many perforce operations
	for ( int i = FirstItem(); i != InvalidItemID(); i = NextItem( i ) )
	{
		const char *pFile = GetFile( i );

		P4File_t fileInfo;
		bool bIsFileInPerforce = p4->GetFileInfo( pFile, &fileInfo );
		bool bFileExists = g_pFullFileSystem->FileExists( pFile );
		RefreshPerforceState( i, bFileExists, bIsFileInPerforce ? &fileInfo : NULL );
	}
	*/

	// NOTE: Reducing the # of perforce calls is important for performance
	int nCount = m_Directories.GetNumStrings();
	for ( int i = 0; i < nCount; ++i )
	{
		const char *pDirectory = m_Directories.String(i);
		DirectoryInfo_t *pInfo = &m_Directories[i];

		// Retrives files, uses faster method to avoid finding clientspec
		CUtlVector<P4File_t> &fileList = p4->GetFileListUsingClientSpec( pDirectory, pInfo->m_ClientSpec );
		int nFileCount = fileList.Count();
		bool *pFound = (bool*)_alloca( nFileCount * sizeof(bool) );
		memset( pFound, 0, nFileCount * sizeof(bool) );

		int nItemCount = pInfo->m_ItemIDs.Count();
		for ( int j = 0; j < nItemCount; ++j )
		{
			int nItemID = pInfo->m_ItemIDs[j];
			const char *pFileName = GetFile( nItemID );
			bool bFileExists = g_pFullFileSystem->FileExists( pFileName );
			P4File_t *pFileInfo = FindFileInPerforceList( pFileName, fileList, pFound );
			RefreshPerforceState( nItemID, bFileExists, pFileInfo );
		}
	}
}


//-----------------------------------------------------------------------------
// Is a particular list item a directory?
//-----------------------------------------------------------------------------
bool PerforceFileList::IsDirectoryItem( int nItemID )
{
	KeyValues *kv = GetItem( nItemID ); 
	return kv->GetInt( "directory", 0 ) != 0;
}

	
//-----------------------------------------------------------------------------
// Returns the file associated with a particular item ID
//-----------------------------------------------------------------------------
const char *PerforceFileList::GetFile( int nItemID )
{
	KeyValues *kv = GetItem( nItemID ); 
	Assert( kv );
	return kv->GetString( "fullpath", "<no file>" );
}

	
//-----------------------------------------------------------------------------
// Find the item ID associated with a particular file
//-----------------------------------------------------------------------------
int PerforceFileList::FindFile( const char *pFullPath )
{
	for ( int i = FirstItem(); i != InvalidItemID(); i = NextItem( i ) )
	{
		const char *pFile = GetFile( i ); 
		if ( !Q_stricmp( pFile, pFullPath ) )
			return i;
	}
	return InvalidItemID();
}


//-----------------------------------------------------------------------------
// Is a file already in the list?
//-----------------------------------------------------------------------------
bool PerforceFileList::IsFileInList( const char *pFullPath )
{
	return ( FindFile( pFullPath ) != InvalidItemID() );
}


//-----------------------------------------------------------------------------
// Purpose: Double-click: expand folders
//-----------------------------------------------------------------------------
void PerforceFileList::OnMouseDoublePressed( MouseCode code )
{
	if ( code == MOUSE_LEFT )
	{
		// select the item
		OnMousePressed(code);

		// post a special message
		if ( GetSelectedItemsCount() > 0 )
		{
			PostActionSignal( new KeyValues("ItemDoubleClicked" ) );
		}
		return;
	}

	BaseClass::OnMouseDoublePressed( code );
}


