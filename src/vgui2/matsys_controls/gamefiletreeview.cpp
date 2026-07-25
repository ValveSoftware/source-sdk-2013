//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#if defined(WIN32) && !defined( _X360 )
#include <windows.h>
#endif
#undef PropertySheet

#include "matsys_controls/gamefiletreeview.h"
#include "filesystem.h"
#include "tier1/KeyValues.h"
#include "vgui/ISurface.h"
#include "vgui/Cursor.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;


//-----------------------------------------------------------------------------
// list of all tree view icons
//-----------------------------------------------------------------------------
enum
{
	IMAGE_FOLDER = 1,
	IMAGE_OPENFOLDER,
	IMAGE_FILE,
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CGameFileTreeView::CGameFileTreeView( Panel *parent, const char *name, const char *pRootFolderName, const char *pRootDir, const char *pExtension ) : BaseClass(parent, name), m_Images( false )
{
	m_RootDir = pRootDir;
	
	m_Ext = pExtension;
	m_bUseExt = ( pExtension != NULL );
	
	m_RootFolderName = pRootFolderName;

	// build our list of images
	m_Images.AddImage( scheme()->GetImage( "resource/icon_folder", false ) );
	m_Images.AddImage( scheme()->GetImage( "resource/icon_folder_selected", false ) );
	m_Images.AddImage( scheme()->GetImage( "resource/icon_file", false ) );
	SetImageList( &m_Images, false );
}


//-----------------------------------------------------------------------------
// Purpose: Refreshes the active file list
//-----------------------------------------------------------------------------
void CGameFileTreeView::RefreshFileList()
{
	RemoveAll();
	SetFgColor(Color(216, 222, 211, 255));

	// add the base node
	KeyValues *pkv = new KeyValues( "root" );
	pkv->SetString( "text", m_RootFolderName );
	pkv->SetInt( "root", 1 );
	pkv->SetInt( "expand", 1 );
	int iRoot = AddItem( pkv, GetRootItemIndex() );
	pkv->deleteThis();
	ExpandItem( iRoot, true );
}

	
//-----------------------------------------------------------------------------
// Selects the root folder
//-----------------------------------------------------------------------------
void CGameFileTreeView::SelectRoot()
{
	AddSelectedItem( GetRootItemIndex(), true );
}


//-----------------------------------------------------------------------------
// Gets the number of root directories
//-----------------------------------------------------------------------------
int CGameFileTreeView::GetRootDirectoryCount()
{
	return GetNumChildren( GetRootItemIndex() );
}


//-----------------------------------------------------------------------------
// Gets the ith root directory
//-----------------------------------------------------------------------------
const char *CGameFileTreeView::GetRootDirectory( int nIndex )
{
	int nItemIndex = GetChild( GetRootItemIndex(), nIndex );
	KeyValues *kv = GetItemData( nItemIndex );
	if ( !kv )
		return NULL;
	return kv->GetString( "path", NULL );
}


//-----------------------------------------------------------------------------
// Populate the root node (necessary since tree view can't have multiple roots)
//-----------------------------------------------------------------------------
void CGameFileTreeView::PopulateRootNode( int itemIndex )
{
	AddDirectoriesOfNode( itemIndex, m_RootDir );

	if ( m_bUseExt )
	{
		AddFilesOfNode( itemIndex, m_RootDir, m_Ext );
	}
}


//-----------------------------------------------------------------------------
// Populate the root node with directories
//-----------------------------------------------------------------------------
bool CGameFileTreeView::DoesDirectoryHaveSubdirectories( const char *pFilePath )
{
	char pSearchString[MAX_PATH];
	Q_snprintf( pSearchString, MAX_PATH, "%s\\*", pFilePath );

	// get the list of files
	FileFindHandle_t findHandle;

	// generate children
	// add all the items
	const char *pszFileName = g_pFullFileSystem->FindFirstEx( pSearchString, "GAME", &findHandle );
	while ( pszFileName )
	{
		bool bIsDirectory = g_pFullFileSystem->FindIsDirectory( findHandle );
		if ( bIsDirectory && Q_strnicmp( pszFileName, ".", 2 ) && Q_strnicmp( pszFileName, "..", 3 ) )
			return true;

		pszFileName = g_pFullFileSystem->FindNext( findHandle );
	}

	g_pFullFileSystem->FindClose( findHandle );
	return false;
}

	
//-----------------------------------------------------------------------------
// Populate the root node with directories
//-----------------------------------------------------------------------------
void CGameFileTreeView::AddDirectoriesOfNode( int itemIndex, const char *pFilePath )
{
	char pSearchString[MAX_PATH];
	Q_snprintf( pSearchString, MAX_PATH, "%s\\*", pFilePath );

	// get the list of files
	FileFindHandle_t findHandle;

	// generate children
	// add all the items
	const char *pszFileName = g_pFullFileSystem->FindFirstEx( pSearchString, "GAME", &findHandle );
	while ( pszFileName )
	{
		bool bIsDirectory = g_pFullFileSystem->FindIsDirectory( findHandle );
		if ( bIsDirectory && Q_strnicmp( pszFileName, ".", 2 ) && Q_strnicmp( pszFileName, "..", 3 ) )
		{
			KeyValues *kv = new KeyValues( "node", "text", pszFileName );
			 
			char pFullPath[MAX_PATH];
			Q_snprintf( pFullPath, sizeof(pFullPath), "%s/%s", pFilePath, pszFileName );
			Q_FixSlashes( pFullPath );
			Q_strlower( pFullPath );
			bool bHasSubdirectories = DoesDirectoryHaveSubdirectories( pFullPath );
			kv->SetString( "path", pFullPath );

			kv->SetInt( "expand", bHasSubdirectories );
			kv->SetInt( "dir", 1 );
			kv->SetInt( "image", IMAGE_FOLDER );

			int itemID = AddItem(kv, itemIndex);
 			kv->deleteThis();

			// mark directories in orange
			SetItemColorForDirectories( itemID );
		}

		pszFileName = g_pFullFileSystem->FindNext( findHandle );
	}

	g_pFullFileSystem->FindClose( findHandle );
}


//-----------------------------------------------------------------------------
// Populate the root node with files
//-----------------------------------------------------------------------------
void CGameFileTreeView::AddFilesOfNode( int itemIndex, const char *pFilePath, const char *pExt )
{
	char pSearchString[MAX_PATH];
	Q_snprintf( pSearchString, MAX_PATH, "%s\\*.%s", pFilePath, pExt );

	// get the list of files
	FileFindHandle_t findHandle;

	// generate children
	// add all the items
	const char *pszFileName = g_pFullFileSystem->FindFirst( pSearchString, &findHandle );
	while ( pszFileName )
	{
		if ( !g_pFullFileSystem->FindIsDirectory( findHandle ) )
		{
			KeyValues *kv = new KeyValues( "node", "text", pszFileName );

			char pFullPath[MAX_PATH];
			Q_snprintf( pFullPath, MAX_PATH, "%s\\%s", pFilePath, pszFileName );
			kv->SetString( "path", pFullPath );
			kv->SetInt( "image", IMAGE_FILE );

			AddItem(kv, itemIndex);
			kv->deleteThis();
		}

		pszFileName = g_pFullFileSystem->FindNext( findHandle );
	}

	g_pFullFileSystem->FindClose( findHandle );
}


//-----------------------------------------------------------------------------
// override to incremental request and show p4 directories
//-----------------------------------------------------------------------------
void CGameFileTreeView::GenerateChildrenOfNode(int itemIndex)
{
	KeyValues *pkv = GetItemData(itemIndex);
	if ( pkv->GetInt("root") )
	{
		PopulateRootNode( itemIndex );
		return;
	}

	if (!pkv->GetInt("dir"))
		return;

	const char *pFilePath = pkv->GetString("path", "");
	if (!pFilePath[0])
		return;

	surface()->SetCursor(dc_waitarrow);

	AddDirectoriesOfNode( itemIndex, pFilePath );

	if ( m_bUseExt )
	{
		AddFilesOfNode( itemIndex, pFilePath, m_Ext );
	}
}


//-----------------------------------------------------------------------------
// setup a context menu whenever a directory is clicked on
//-----------------------------------------------------------------------------
void CGameFileTreeView::GenerateContextMenu( int itemIndex, int x, int y ) 
{
	return;

	/*
	KeyValues *pkv = GetItemData(itemIndex);
	const char *pFilePath = pkv->GetString("path", "");
	if (!pFilePath[0])
		return;

	Menu *pContext = new Menu(this, "FileContext");
	pContext->AddMenuItem("Cloak folder", new KeyValues("CloakFolder", "item", itemIndex), GetParent(), NULL);

	// show the context menu
	pContext->SetPos(x, y);
	pContext->SetVisible(true);
	*/
}


//-----------------------------------------------------------------------------
// Sets an item to be colored as if its a menu
//-----------------------------------------------------------------------------
void CGameFileTreeView::SetItemColorForDirectories( int itemID )
{
	// mark directories in orange
	SetItemFgColor( itemID, Color(224, 192, 0, 255) );
}


//-----------------------------------------------------------------------------
// setup a smaller font
//-----------------------------------------------------------------------------
void CGameFileTreeView::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetFont( pScheme->GetFont("DefaultSmall") );
}

