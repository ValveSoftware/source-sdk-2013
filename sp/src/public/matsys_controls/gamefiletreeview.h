//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef GAMEFILETREEVIEW_H
#define GAMEFILETREEVIEW_H

#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlstring.h"
#include "vgui_controls/TreeView.h"
#include "vgui_controls/ImageList.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class IScheme;
}


//-----------------------------------------------------------------------------
// Purpose: Handles file view for game files
//-----------------------------------------------------------------------------
class CGameFileTreeView : public vgui::TreeView
{
	DECLARE_CLASS_SIMPLE( CGameFileTreeView, vgui::TreeView );

public:
	CGameFileTreeView( vgui::Panel *parent, const char *name, const char *pRootFolderName, const char *pRootDir, const char *pExtension = NULL );

	// Inherited from base classes
	virtual void GenerateChildrenOfNode( int itemIndex );
	virtual void GenerateContextMenu( int itemIndex, int x, int y ); 
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	// Purpose: Refreshes the active file list
	void RefreshFileList();

	// Sets an item to be colored as if its a menu
	void SetItemColorForDirectories( int itemID );

	// Gets the number of root directories
	int GetRootDirectoryCount();

	// Gets the ith root directory
	const char *GetRootDirectory( int nIndex );

	// Selects the root folder
	void SelectRoot();

private:
	// Populate the root node (necessary since tree view can't have multiple roots)
	void PopulateRootNode( int itemIndex );

	// Populate the root node with directories
	void AddDirectoriesOfNode( int itemIndex, const char *pFilePath );

	// Populate the root node with directories
	bool DoesDirectoryHaveSubdirectories( const char *pFilePath );

	// Populate the root node with files
	void AddFilesOfNode( int itemIndex, const char *pFilePath, const char *pExt );

	CUtlString m_RootDir;
	CUtlString m_Ext;
	CUtlString m_RootFolderName;
	vgui::ImageList m_Images;
	bool m_bUseExt;		// To differentiate "" from NULL in m_Ext
};


#endif // GAMEFILETREEVIEW_H

