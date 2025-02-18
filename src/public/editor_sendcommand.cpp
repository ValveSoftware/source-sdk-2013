//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements an interface to the map editor for the execution of
//			editor shell commands from another application. Commands allow the
//			creation and deletion of entities, AI nodes, and AI node connections.
//
// $NoKeywords: $
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)

#if !defined(_X360) && defined(_WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include "editor_sendcommand.h"
#include "tier1/strtools.h"
#include "mathlib/vector.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const int MAX_COMMAND_BUFFER = 2048;
//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to begin an editing session
//			of a given map.
// Input  : pszMapName - Name of the bsp, without the path or extension: "c1a3a_port"
//			nMapVersion - Map version number, from the BSP file.
// Output : Returns Editor_OK on success, an error code if the session was rejected.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_BeginSession(const char *pszMapName, int nMapVersion, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "session_begin %s %d", pszMapName, nMapVersion);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to verify the version and
//			map name being edited.
// Input  : pszMapName - Name of the bsp, without the path or extension: "c1a3a_port"
//			nMapVersion - Map version number, from the BSP file.
// Output : Returns Editor_OK on success, an error code if the session was rejected.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_CheckVersion(const char *pszMapName, int nMapVersion, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "map_check_version %s %d", pszMapName, nMapVersion);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to create an entity at
//			a given (x, y, z) coordinate.
// Input  : pszEntity - Class name of entity to create, ie "info_player_start".
//			x, y, z - World coordinates at which to create entity.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_CreateEntity(const char *pszEntity, float x, float y,  float z, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "entity_create %s %g %g %g", pszEntity, x, y, z);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to create an entity at
//			a given (x, y, z) coordinate.
// Input  : pszNodeClass - Class name of node to create, ie "info_node".
//			nID - Unique ID to assign the node.
//			x, y, z - World coordinates at which to create node.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_CreateNode(const char *pszNodeClass, int nID, float x, float y,  float z, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "node_create %s %d %g %g %g", pszNodeClass, nID, x, y, z);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to create an entity at
//			a given (x, y, z) coordinate.
// Input  : pszNodeClass - Class name of node to create, ie "info_node".
//			nID - Unique ID to assign the node.
//			x, y, z - World coordinates at which to create node.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_CreateNodeLink(int nStartID, int nEndID, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "nodelink_create %d %d", nStartID, nEndID);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to delete an entity at
//			a given (x, y, z) coordinate.
// Input  : pszEntity - Class name of entity to delete, ie "info_player_start".
//			x, y, z - World coordinates of entity to delete.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_DeleteEntity(const char *pszEntity, float x, float y, float z, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "entity_delete %s %g %g %g", pszEntity, x, y, z);
	return(Editor_SendCommand(szCommand, bShowUI));
}


// sets an arbitrary key/value pair in the entity
EditorSendResult_t Editor_SetKeyValue(const char *pszEntity, float x, float y, float z, const char *pKey, const char *pValue, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "entity_set_keyvalue %s %f %f %f \"%s\" \"%s\"", pszEntity, x, y, z, pKey, pValue);
	return(Editor_SendCommand(szCommand, bShowUI));
}


// applies an incremental rotation to an entity
EditorSendResult_t Editor_RotateEntity(const char *pszEntity, float x, float y, float z, const QAngle &incrementalRotation, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "entity_rotate_incremental %s %f %f %f %f %f %f", pszEntity, x, y, z, incrementalRotation.x, incrementalRotation.y, incrementalRotation.z );
	return(Editor_SendCommand(szCommand, bShowUI));
}
//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to delete an entity at
//			a given (x, y, z) coordinate.
// Input  : nID - unique ID of node to delete.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_DeleteNode(int nID, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "node_delete %d", nID);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to delete an entity at
//			a given (x, y, z) coordinate.
// Input  : nStartID - unique ID of one node that the link is connected to.
//			nEndID - unique ID of the other node that the link is connected to.
// Output : Returns Editor_OK on success, an error code on failure.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_DeleteNodeLink(int nStartID, int nEndID, bool bShowUI)
{
	char szCommand[MAX_COMMAND_BUFFER];
	Q_snprintf(szCommand,sizeof(szCommand), "nodelink_delete %d %d", nStartID, nEndID);
	return(Editor_SendCommand(szCommand, bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Sends a command to the editor (if running) to end the current remote
//			editing session.
// Output : Returns Editor_OK on success, an error code if the session was rejected.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_EndSession(bool bShowUI)
{
	return(Editor_SendCommand("session_end", bShowUI));
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to sends a shell command to the editor.
// Input  : pszCommand - Shell command to send.
//			bShowUI - Whether to display mesage boxes on failure.
// Output : Returns one of the following values:
//				Editor_OK - The command was executed successfully.
//				Editor_NotRunning - Unable to establish a communications channel with the editor.
//				Editor_BadCommand - The editor did not accept the command.
//-----------------------------------------------------------------------------
EditorSendResult_t Editor_SendCommand(const char *pszCommand, bool bShowUI)
{
#ifdef _WIN32
	HWND hwnd = FindWindow("Worldcraft_ShellMessageWnd", "Worldcraft_ShellMessageWnd");
	if (hwnd != NULL)
	{
		//
		// Fill out the data structure to send to the editor.
		//

		COPYDATASTRUCT CopyData;
		CopyData.cbData = V_strlen(pszCommand) + 1;
		CopyData.dwData = 0;
		CopyData.lpData = (void *)pszCommand;
		
		if (!SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&CopyData))
		{
			if (bShowUI)
			{
				char szError[1024];
				Q_snprintf(szError,sizeof(szError), "Worldcraft did not accept the command: \n\n\"%s\"\n\n Make sure the command is valid and that Worldcraft is still running properly.", pszCommand);
				MessageBox(NULL, szError, "Editor_SendCommand Error", MB_OK);
			}
		
			return(Editor_BadCommand);
		}
	}
	else
	{
		if (bShowUI)
		{
			char szError[1024];
			Q_snprintf(szError,sizeof(szError), "Could not contact Worldcraft to send the command: \n\n\"%s\"\n\n Worldcraft does not appear to be running.", pszCommand);
			MessageBox(NULL, szError, "Editor_SendCommand Error", MB_OK);
		}

		return(Editor_NotRunning);
	}
#endif

	return(Editor_OK);
}

#endif // !_STATIC_LINKED || _SHARED_LIB
