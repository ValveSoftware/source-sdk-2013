//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#define PROTECTED_THINGS_DISABLE

#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/DirectorySelectDialog.h>
#include <vgui_controls/TreeView.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MessageBox.h>
#include <vgui/Cursor.h>
#include <KeyValues.h>
#include <vgui/IInput.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <filesystem.h>

#ifdef WIN32
#include <direct.h>
#include <stdio.h>
#include <io.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DirectoryTreeView::DirectoryTreeView(DirectorySelectDialog *parent, const char *name) : TreeView(parent, name)
{
	m_pParent = parent;
}

void DirectoryTreeView::GenerateChildrenOfNode(int itemIndex)
{
	m_pParent->GenerateChildrenOfDirectoryNode(itemIndex);
}

//-----------------------------------------------------------------------------
// Purpose: Used to prompt the user to create a directory
//-----------------------------------------------------------------------------
class CreateDirectoryDialog : public Frame
{
	DECLARE_CLASS_SIMPLE(CreateDirectoryDialog, Frame);

public:
	CreateDirectoryDialog(Panel *parent, const char *defaultCreateDirName) : BaseClass(parent, NULL)
	{
		SetSize(320, 100);
		SetSizeable(false);
		SetTitle("Choose directory name", false);
		MoveToCenterOfScreen();

		m_pOKButton = new Button(this, "OKButton", "#vgui_ok");
		m_pCancelButton = new Button(this, "OKButton", "#vgui_cancel");
		m_pNameEntry = new TextEntry(this, "NameEntry");

		m_pOKButton->SetCommand("OK");
		m_pCancelButton->SetCommand("Close");
		m_pNameEntry->SetText(defaultCreateDirName);
		m_pNameEntry->RequestFocus();
		m_pNameEntry->SelectAllText(true);
	
		// If some other window was hogging the input focus, then we have to hog it or else we'll never get input.
		m_PrevAppFocusPanel = vgui::input()->GetAppModalSurface();
		if ( m_PrevAppFocusPanel )
			vgui::input()->SetAppModalSurface( GetVPanel() );
	}

	~CreateDirectoryDialog()
	{
		if ( m_PrevAppFocusPanel )
			vgui::input()->SetAppModalSurface( m_PrevAppFocusPanel );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		m_pNameEntry->SetBounds(24, 32, GetWide() - 48, 24);
		m_pOKButton->SetBounds(GetWide() - 176, 64, 72, 24);
		m_pCancelButton->SetBounds(GetWide() - 94, 64, 72, 24);
	}

	virtual void OnCommand(const char *command)
	{
		if (!stricmp(command, "OK"))
		{
			PostActionSignal(new KeyValues("CreateDirectory", "dir", GetControlString("NameEntry")));
			Close();
		}
		else
		{
			BaseClass::OnCommand(command);
		}
	}

	virtual void OnClose()
	{
		BaseClass::OnClose();
		MarkForDeletion();
	}

private:
	vgui::Button *m_pOKButton;
	vgui::Button *m_pCancelButton;
	vgui::TextEntry *m_pNameEntry;
	vgui::VPANEL m_PrevAppFocusPanel;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
DirectorySelectDialog::DirectorySelectDialog(vgui::Panel *parent, const char *title) : Frame(parent, NULL)
{
	SetTitle(title, true);
	SetSize(320, 360);
	SetMinimumSize(300, 240);
	m_szCurrentDir[0] = 0;
	m_szDefaultCreateDirName[0] = 0;

	m_pDirTree = new DirectoryTreeView(this, "DirTree");
	m_pDriveCombo = new ComboBox(this, "DriveCombo", 6, false);
	m_pCancelButton = new Button(this, "CancelButton", "#VGui_Cancel");
	m_pSelectButton = new Button(this, "SelectButton", "#VGui_Select");
	m_pCreateButton = new Button(this, "CreateButton", "#VGui_CreateFolder");
	m_pCancelButton->SetCommand("Cancel");
	m_pSelectButton->SetCommand("Select");
	m_pCreateButton->SetCommand("Create");
}

//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void DirectorySelectDialog::PerformLayout()
{
	BaseClass::PerformLayout();

	// lay out all the controls
	m_pDriveCombo->SetBounds(24, 30, GetWide() - 48, 24);
	m_pDirTree->SetBounds(24, 64, GetWide() - 48, GetTall() - 128);

	m_pCreateButton->SetBounds(24, GetTall() - 48, 104, 24);
	m_pSelectButton->SetBounds(GetWide() - 172, GetTall() - 48, 72, 24);
	m_pCancelButton->SetBounds(GetWide() - 96, GetTall() - 48, 72, 24);
}

//-----------------------------------------------------------------------------
// Purpose: lays out controls
//-----------------------------------------------------------------------------
void DirectorySelectDialog::ApplySchemeSettings(IScheme *pScheme)
{
	ImageList *imageList = new ImageList(false);
	imageList->AddImage(scheme()->GetImage("Resource/icon_folder", false));
	imageList->AddImage(scheme()->GetImage("Resource/icon_folder_selected", false));
	m_pDirTree->SetImageList(imageList, true);

	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: Move the start string forward until we hit a slash and return the
//			the first character past the trailing slash
//-----------------------------------------------------------------------------
inline const char *MoveToNextSubDir( const char *pStart, int *nCount )
{
	int nMoved = 0;

	// Move past pre-pended slash
	if ( pStart[nMoved] == '\\' )
	{
		nMoved++;
	}

	// Move past the current block of text until we've hit the next path seperator (or end)
	while ( pStart[nMoved] != '\\' && pStart[nMoved] != '\0' )
	{
		nMoved++;
	}

	// Move past trailing slash
	if ( pStart[nMoved] == '\\' )
	{
		nMoved++;
	}

	// Give back a count if they've supplied a pointer
	if ( nCount != NULL )
	{
		*nCount = nMoved;
	}
	
	// The beginning of the next string, past slash
	return (pStart+nMoved);
}

//-----------------------------------------------------------------------------
// Purpose: Walk through our directory structure given a path as our guide, while expanding
//			and populating the nodes of the tree view to match
// Input  : *path - path (with drive letter) to show
//-----------------------------------------------------------------------------
void DirectorySelectDialog::ExpandTreeToPath( const char *lpszPath, bool bSelectFinalDirectory /*= true*/ )
{
	// Make sure our slashes are correct!
	char workPath[MAX_PATH];
	Q_strncpy( workPath, lpszPath, sizeof(workPath) );
	Q_FixSlashes( workPath );
	
	// Set us to the work drive
	SetStartDirectory( workPath );

	// Check that the path is valid
	if ( workPath[0] == '\0' || DoesDirectoryHaveSubdirectories( m_szCurrentDrive, "" ) == false )
	{
		// Failing, start in C:
		SetStartDirectory( "C:\\" );
	}

	// Start at the root of our tree
	int nItemIndex = m_pDirTree->GetRootItemIndex();
	
	// Move past the drive letter to the first subdir
	int nPathPos = 0;
	const char *lpszSubDirName = MoveToNextSubDir( workPath, &nPathPos ); 
	const char *lpszLastSubDirName = NULL;
	int nPathIncr = 0;
	char subDirName[MAX_PATH];

	// While there are subdirectory names present, expand and populate the tree with their subdirectories
	while ( lpszSubDirName[0] != '\0' )
	{
		// Move our string pointer forward while keeping where our last subdir started off
		lpszLastSubDirName = lpszSubDirName;
		lpszSubDirName = MoveToNextSubDir( lpszSubDirName, &nPathIncr );

		// Get the span between the last subdir and the new one
		Q_StrLeft( lpszLastSubDirName, nPathIncr, subDirName, sizeof(subDirName) );
		Q_StripTrailingSlash( subDirName );

		// Increment where we are in the string for use later
		nPathPos += nPathIncr;

		// Run through the list and expand to our currently selected directory
		for ( int i = 0; i < m_pDirTree->GetNumChildren( nItemIndex ); i++ )
		{
			// Get the child and data for it
			int nChild = m_pDirTree->GetChild( nItemIndex, i );
			KeyValues *pValues = m_pDirTree->GetItemData( nChild );

			// See if this matches
			if ( Q_stricmp( pValues->GetString( "Text" ), subDirName ) == 0 )
			{
				// This is the new root item
				nItemIndex = nChild;

				// Get the full path (starting from the drive letter) up to our current subdir
				Q_strncpy( subDirName, workPath, nPathPos );
				Q_AppendSlash( subDirName, sizeof(subDirName) );

				// Expand the tree node and populate its subdirs for our next iteration
				ExpandTreeNode( subDirName, nItemIndex );
				break;
			}
		}
	}

	// Select our last directory if we've been asked to (and it's valid)
	if ( bSelectFinalDirectory && m_pDirTree->IsItemIDValid( nItemIndex ) )
	{
		// If we don't call this once before selecting an item, the tree will not be properly expanded
		// before it calculates how to show the selected item in the view
		PerformLayout();

		// Select that item
		m_pDirTree->AddSelectedItem( nItemIndex, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets where it should start searching
//-----------------------------------------------------------------------------
void DirectorySelectDialog::SetStartDirectory(const char *path)
{
	V_strncpy(m_szCurrentDir, path, sizeof(m_szCurrentDir));
	V_strncpy(m_szCurrentDrive, path, sizeof(m_szCurrentDrive));
	m_szCurrentDrive[sizeof(m_szCurrentDrive) - 1] = 0;
	char *firstSlash = strstr(m_szCurrentDrive, "\\");
	if (firstSlash)
	{
		firstSlash[1] = 0;
	}

	BuildDirTree();
	BuildDriveChoices();

	// update state of create directory button
	int selectedIndex = m_pDirTree->GetFirstSelectedItem();
	if (m_pDirTree->IsItemIDValid(selectedIndex))
	{
		m_pCreateButton->SetEnabled(true);
	}
	else
	{
		m_pCreateButton->SetEnabled(false);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets what name should show up by default in the create directory dialog
//-----------------------------------------------------------------------------
void DirectorySelectDialog::SetDefaultCreateDirectoryName(const char *defaultCreateDirName)
{
	strncpy(m_szDefaultCreateDirName, defaultCreateDirName, sizeof(m_szDefaultCreateDirName));
	m_szDefaultCreateDirName[sizeof(m_szDefaultCreateDirName) - 1] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: opens the dialog
//-----------------------------------------------------------------------------
void DirectorySelectDialog::DoModal()
{
	input()->SetAppModalSurface(GetVPanel());
	BaseClass::Activate();
	MoveToCenterOfScreen();
}

//-----------------------------------------------------------------------------
// Purpose: Builds drive choices
//-----------------------------------------------------------------------------
void DirectorySelectDialog::BuildDriveChoices()
{
	m_pDriveCombo->DeleteAllItems();

	char drives[256] = { 0 };
	int len = system()->GetAvailableDrives(drives, sizeof(drives));
	char *pBuf = drives;
	KeyValues *kv = new KeyValues("drive");
	for (int i = 0; i < len / 4; i++)
	{
		kv->SetString("drive", pBuf);
		int itemID = m_pDriveCombo->AddItem(pBuf, kv);
		if (!stricmp(pBuf, m_szCurrentDrive))
		{
			m_pDriveCombo->ActivateItem(itemID);
		}

		pBuf += 4;
	}
	kv->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: Builds the base tree directory
//-----------------------------------------------------------------------------
void DirectorySelectDialog::BuildDirTree()
{
	// clear current tree
	m_pDirTree->RemoveAll();

	// add in a root
	int rootIndex = m_pDirTree->AddItem(new KeyValues("root", "Text", m_szCurrentDrive), -1);

	// build first level of the tree
	ExpandTreeNode(m_szCurrentDrive, rootIndex);
	
	// start the root expanded
	m_pDirTree->ExpandItem(rootIndex, true);
}

//-----------------------------------------------------------------------------
// Purpose: expands a path
//-----------------------------------------------------------------------------
void DirectorySelectDialog::ExpandTreeNode(const char *path, int parentNodeIndex)
{
	// set the small wait cursor
	surface()->SetCursor(dc_waitarrow);

	// get all the subfolders of the current drive
	char searchString[512];
	sprintf(searchString, "%s*.*", path);

	FileFindHandle_t h;
	const char *pFileName = g_pFullFileSystem->FindFirstEx( searchString, NULL, &h );
	for ( ; pFileName; pFileName = g_pFullFileSystem->FindNext( h ) )
	{
		if ( !Q_stricmp( pFileName, ".." ) || !Q_stricmp( pFileName, "." ) )
			continue;

		KeyValues *kv = new KeyValues("item");
		kv->SetString("Text", pFileName);
		// set the folder image
		kv->SetInt("Image", 1);
		kv->SetInt("SelectedImage", 1);
		kv->SetInt("Expand", DoesDirectoryHaveSubdirectories(path, pFileName));	
		m_pDirTree->AddItem(kv, parentNodeIndex);
	}
	g_pFullFileSystem->FindClose( h );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool DirectorySelectDialog::DoesDirectoryHaveSubdirectories(const char *path, const char *dir)
{
	char searchString[512];
	sprintf(searchString, "%s%s\\*.*", path, dir);

	FileFindHandle_t h;
	const char *pFileName = g_pFullFileSystem->FindFirstEx( searchString, NULL, &h );
	for ( ; pFileName; pFileName = g_pFullFileSystem->FindNext( h ) )
	{
		char szFullPath[ MAX_PATH ];
		Q_snprintf( szFullPath, sizeof(szFullPath), "%s\\%s", path, pFileName );
		Q_FixSlashes( szFullPath ); 
		if ( g_pFullFileSystem->IsDirectory( szFullPath ) )
		{
			g_pFullFileSystem->FindClose( h );
			return true;
		}
	}
	g_pFullFileSystem->FindClose( h );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Generates the children for the specified node
//-----------------------------------------------------------------------------
void DirectorySelectDialog::GenerateChildrenOfDirectoryNode(int nodeIndex)
{
	// generate path
	char path[512];
	GenerateFullPathForNode(nodeIndex, path, sizeof(path));

	// expand out
	ExpandTreeNode(path, nodeIndex);
}

//-----------------------------------------------------------------------------
// Purpose: creates the full path for a node
//-----------------------------------------------------------------------------
void DirectorySelectDialog::GenerateFullPathForNode(int nodeIndex, char *path, int pathBufferSize)
{
	// get all the nodes
	CUtlLinkedList<int, int> nodes;
	nodes.AddToTail(nodeIndex);
	int parentIndex = nodeIndex;
	while (1)
	{
		parentIndex = m_pDirTree->GetItemParent(parentIndex);
		if (parentIndex == -1)
			break;
		nodes.AddToHead(parentIndex);
	}

	// walk the nodes, adding to the path
	path[0] = 0;
	bool bFirst = true;
	FOR_EACH_LL( nodes, i )
	{
		KeyValues *kv = m_pDirTree->GetItemData( nodes[i] );
		strcat(path, kv->GetString("Text"));

		if (!bFirst)
		{
			strcat(path, "\\");
		}
		bFirst = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles combo box changes
//-----------------------------------------------------------------------------
void DirectorySelectDialog::OnTextChanged()
{
	KeyValues *kv = m_pDriveCombo->GetActiveItemUserData();
	if (!kv)
		return;
	const char *newDrive = kv->GetString("drive");
	if (stricmp(newDrive, m_szCurrentDrive))
	{
		// drive changed, reset
		SetStartDirectory(newDrive);
	}
}

//-----------------------------------------------------------------------------
// Purpose: creates a directory
//-----------------------------------------------------------------------------
void DirectorySelectDialog::OnCreateDirectory(const char *dir)
{
	int selectedIndex = m_pDirTree->GetFirstSelectedItem();
	if (m_pDirTree->IsItemIDValid(selectedIndex))
	{
		char fullPath[512];
		GenerateFullPathForNode(selectedIndex, fullPath, sizeof(fullPath));

		// create the new directory underneath
		strcat(fullPath, dir);
		if (_mkdir(fullPath) == 0)
		{
			// add new path to tree view
			KeyValues *kv = new KeyValues("item");
			kv->SetString("Text", dir);
			// set the folder image
			kv->SetInt("Image", 1);
			kv->SetInt("SelectedImage", 1);
			int itemID = m_pDirTree->AddItem(kv, selectedIndex);

			// select the item
			m_pDirTree->AddSelectedItem( itemID, true );
		}
		else
		{
			// print error message
			MessageBox *box = new MessageBox("#vgui_CreateDirectoryFail_Title", "#vgui_CreateDirectoryFail_Info");
			box->DoModal(this);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: dialog closes
//-----------------------------------------------------------------------------
void DirectorySelectDialog::OnClose()
{
	BaseClass::OnClose();
	MarkForDeletion();
}

//-----------------------------------------------------------------------------
// Purpose: handles button commands
//-----------------------------------------------------------------------------
void DirectorySelectDialog::OnCommand(const char *command)
{
	if (!stricmp(command, "Cancel"))
	{
		Close();
	}
	else if (!stricmp(command, "Select"))
	{
		// path selected
		int selectedIndex = m_pDirTree->GetFirstSelectedItem();
		if (m_pDirTree->IsItemIDValid(selectedIndex))
		{
			char fullPath[512];
			GenerateFullPathForNode(selectedIndex, fullPath, sizeof(fullPath));
			PostActionSignal(new KeyValues("DirectorySelected", "dir", fullPath));
			Close();
		}
	}
	else if (!stricmp(command, "Create"))
	{
		int selectedIndex = m_pDirTree->GetFirstSelectedItem();
		if (m_pDirTree->IsItemIDValid(selectedIndex))
		{
			CreateDirectoryDialog *dlg = new CreateDirectoryDialog(this, m_szDefaultCreateDirName);
			dlg->AddActionSignalTarget(this);
			dlg->Activate();
		}
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the text in the combo
//-----------------------------------------------------------------------------
void DirectorySelectDialog::OnTreeViewItemSelected()
{
	int selectedIndex = m_pDirTree->GetFirstSelectedItem();
	if (!m_pDirTree->IsItemIDValid(selectedIndex))
	{
		m_pCreateButton->SetEnabled(false);
		return;
	}
	m_pCreateButton->SetEnabled(true);

	// build the string
	char fullPath[512];
	GenerateFullPathForNode(selectedIndex, fullPath, sizeof(fullPath));

	int itemID = m_pDriveCombo->GetActiveItem();
	m_pDriveCombo->UpdateItem(itemID, fullPath, NULL);
	m_pDriveCombo->SetText(fullPath);
}