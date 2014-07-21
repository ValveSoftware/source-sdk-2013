//========= Copyright Valve Corporation, All rights reserved. ============//
//
// This dialog asks if you want to save your work
//
//=============================================================================

#ifndef SAVEDOCUMENTQUERY_H
#define SAVEDOCUMENTQUERY_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;
namespace vgui
{
class Panel;
}


//-----------------------------------------------------------------------------
// Show the save document query dialog
// NOTE: The following commands will be posted to the action signal target:
//		"OnExit" - when we want to quit
//		"OnSave" - when we want to save the file
//		"OnCloseNoSave" - when we want to close the file without saving it
//		"commandname" - additional command send after saving (SAVEDOC_POSTCOMMAND_AFTER_SAVE)
//		"OnMarkNotDirty" - when we want to mark the file not dirty
//-----------------------------------------------------------------------------
void ShowSaveDocumentQuery( vgui::Panel *pParent, const char *pFileName, const char *pFileType, int nContext, vgui::Panel *pActionSignalTarget, KeyValues *pPostSaveCommand );


#endif // SAVEDOCUMENTQUERY_H
