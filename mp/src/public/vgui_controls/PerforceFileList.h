//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains a list of files, determines their perforce status
//
// $NoKeywords: $
//===========================================================================//

#ifndef PERFORCEFILELIST_H
#define PERFORCEFILELIST_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstring.h"
#include "tier1/UtlStringMap.h"
#include "vgui_controls/ListPanel.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
struct P4File_t;

namespace vgui
{
	class ListPanel;
}


namespace vgui
{

//-----------------------------------------------------------------------------
// Contains a list of files, determines their perforce status
//-----------------------------------------------------------------------------
class PerforceFileList : public vgui::ListPanel
{
	DECLARE_CLASS_SIMPLE( PerforceFileList, ListPanel );

public:
	// The context keyvalues are added to all messages sent by this dialog if they are specified
	PerforceFileList( Panel *parent, const char *pPanelName );
	~PerforceFileList();

	// Add a file to the file list. Note that this file may exist on disk or not
	// and it may exist in perforce or not. It's specified as a full path on disk though.
	// In the case where a file doesn't exist on disk, but it does exist in perforce
	// specify where that file would appear on disk.
	// This function returns the itemID of the added file
	// If you already know the file exists or is a directory (or not), specify that in the call.
	// -1 means autodetect whether the file exists or is a directory
	int AddFile( const char *pFullPath, int nFileExists = -1, int nIsDirectory = -1 );

	// Is a file already in the list?
	bool IsFileInList( const char *pFullPath );

	// Find the item ID associated with a particular file
	int FindFile( const char *pFullPath );

	// Remove all files from the list
	void RemoveAllFiles();

	// Refresh perforce information
	void Refresh();

	// Refresh perforce information manually
	void RefreshPerforceState( int nItemID, bool bFileExists, P4File_t *pFileInfo );

	// Is a particular list item a directory?
	bool IsDirectoryItem( int nItemID );

	// Returns the file associated with a particular item ID
	const char *GetFile( int nItemID );

	// Toggle showing deleted files or not
	void ShowDeletedFiles( bool bShowDeletedFiles );

	// Inherited from vgui::EditablePanel
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnMouseDoublePressed( MouseCode code );

	/*
		messages sent:
			"ItemDoubleClicked"	// Called when an item is double-clicked
	*/

protected:
	struct DirectoryInfo_t
	{
		CUtlString m_ClientSpec;
		CUtlVector< int > m_ItemIDs;
	};

	// Add a file to the file list. 
	int AddFileToFileList( const char *pFullPath, bool bExistsOnDisk );

	// Add a directory to the file list. 
	int AddDirectoryToFileList( const char *pFullPath, bool bExistsOnDisk );

	// Add a directory to the directory list, returns client spec
	void AddItemToDirectoryList( const char *pFullPath, int nItemID, bool bIsDirectory );

	// Used to look up directories -> client specs
	CUtlStringMap< DirectoryInfo_t > m_Directories;

	// Show deleted files?
	bool m_bShowDeletedFiles;
};


} // namespace vgui

#endif // PERFORCEFILELIST_H
