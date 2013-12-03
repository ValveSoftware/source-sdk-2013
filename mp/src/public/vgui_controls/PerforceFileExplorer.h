//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Allows you to browse a directory structure, showing perforce files
//
// $NoKeywords: $
//===========================================================================//

#ifndef PERFORCEFILEEXPLORER_H
#define PERFORCEFILEEXPLORER_H

#ifdef _WIN32
#pragma once
#endif


#include "tier1/utlstring.h"
#include "vgui_controls/Frame.h"


namespace vgui
{

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class PerforceFileList;
class ComboBox;
class Button;


//-----------------------------------------------------------------------------
// Contains a list of files, determines their perforce status
//-----------------------------------------------------------------------------
class PerforceFileExplorer : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( PerforceFileExplorer, Frame );

public:
	// The context keyvalues are added to all messages sent by this dialog if they are specified
	PerforceFileExplorer( Panel *parent, const char *pPanelName );
	~PerforceFileExplorer();

	// Inherited from Frame
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

protected:
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", kv );
	MESSAGE_FUNC( OnItemDoubleClicked, "ItemDoubleClicked" );
	MESSAGE_FUNC( OnFolderUp, "FolderUp" );

	void PopulateFileList();
	void PopulateDriveList();

	// Returns the current directory
	void SetCurrentDirectory( const char *pCurrentDirectory );

	Button *m_pFolderUpButton;
	ComboBox *m_pFullPathCombo;
	PerforceFileList *m_pFileList;
	CUtlString m_CurrentDirectory;
};


} // namespace vgui

#endif // PERFORCEFILEEXPLORER_H
