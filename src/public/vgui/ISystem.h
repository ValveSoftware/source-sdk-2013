//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISYSTEM_H
#define ISYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/interface.h"
#include <vgui/VGUI.h>
#include <vgui/KeyCode.h>

#ifdef PlaySound
#undef PlaySound
#endif

class KeyValues;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class ISystem : public IBaseInterface
{
public:
	// call when done with ISystem to clean up any memory allocation
	virtual void Shutdown() = 0;

	// called every frame
	virtual void RunFrame() = 0;

	// use this with the "open" command to launch web browsers/explorer windows, eg. ShellExecute("open", "www.valvesoftware.com")
	virtual void ShellExecute(const char *command, const char *file) = 0;

	// returns the time at the start of the frame, in seconds
	virtual double GetFrameTime() = 0;

	// returns the current time, in seconds
	virtual double GetCurrentTime() = 0;

	// returns the current time, in milliseconds
	virtual long GetTimeMillis() = 0;

	// clipboard access
	virtual int GetClipboardTextCount() = 0;
	virtual void SetClipboardText(const char *text, int textLen) = 0;
	virtual void SetClipboardText(const wchar_t *text, int textLen) = 0;
	virtual int GetClipboardText(int offset, char *buf, int bufLen) = 0;
	virtual int GetClipboardText(int offset, wchar_t *buf, int bufLen) = 0;

	// windows registry
	virtual bool SetRegistryString(const char *key, const char *value) = 0;
	virtual bool GetRegistryString(const char *key, char *value, int valueLen) = 0;
	virtual bool SetRegistryInteger(const char *key, int value) = 0;
	virtual bool GetRegistryInteger(const char *key, int &value) = 0;

	// user config
	virtual KeyValues *GetUserConfigFileData(const char *dialogName, int dialogID) = 0;
	// sets the name of the config file to save/restore from.  Settings are loaded immediately.
	virtual void SetUserConfigFile(const char *fileName, const char *pathName) = 0;
	// saves all the current settings to the user config file
	virtual void SaveUserConfigFile() = 0;

	// sets the watch on global computer use
	// returns true if supported
	virtual bool SetWatchForComputerUse(bool state) = 0;
	// returns the time, in seconds, since the last computer use.
	virtual double GetTimeSinceLastUse() = 0;

	// Get a string containing the available drives
	// If the function succeeds, the return value is the length, in characters, 
	// of the strings copied to the buffer, 
	// not including the terminating null character.
	virtual int GetAvailableDrives(char *buf, int bufLen) = 0;

	// exe command line options accessors
	// returns whether or not the parameter was on the command line
	virtual bool CommandLineParamExists(const char *paramName) = 0;

	// returns the full command line, including the exe name
	virtual const char *GetFullCommandLine() = 0;

	// Convert a windows virtual key code to a VGUI key code.
	virtual KeyCode KeyCode_VirtualKeyToVGUI( int keyCode ) = 0;

	// returns the current local time and date
	// fills in every field that a pointer is given to it for
	virtual bool GetCurrentTimeAndDate(int *year, int *month, int *dayOfWeek, int *day, int *hour, int *minute, int *second) = 0;

	// returns the amount of available disk space, in bytes, on the drive
	// path can be any path, drive letter is stripped out
	virtual double GetFreeDiskSpace(const char *path) = 0;

	// shortcut (.lnk) modification functions
	virtual bool CreateShortcut(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory, const char *iconFile) = 0;
	virtual bool GetShortcutTarget(const char *linkFileName, char *targetPath, char *arguments, int destBufferSizes) = 0;
	virtual bool ModifyShortcutTarget(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory) = 0;

	// gets the string following a command line param
	//!! move this function up on changing interface version number
	virtual bool GetCommandLineParamValue(const char *paramName, char *value, int valueBufferSize) = 0;

	// recursively deletes a registry key and all it's subkeys
	//!! move this function next to other registry function on changing interface version number
	virtual bool DeleteRegistryKey(const char *keyName) = 0;

	virtual const char *GetDesktopFolderPath() = 0;

	// use this with the "open" command to launch web browsers/explorer windows, eg. ShellExecute("open", "www.valvesoftware.com")
	virtual void ShellExecuteEx( const char *command, const char *file, const char *pParams ) = 0;

	// Copy a portion of the application client area to the clipboard
	//  (x1,y1) specifies the top-left corner of the client rect to copy
	//  (x2,y2) specifies the bottom-right corner of the client rect to copy
	// Requires: x2 > x1 && y2 > y1
	// Dimensions of the copied rectangle are (x2 - x1) x (y2 - y1)
	// Pixel at (x1,y1) is copied, pixels at column x2 and row y2 are *not* copied
	virtual void SetClipboardImage( void *pWnd, int x1, int y1, int x2, int y2 ) = 0;
};

}

#define VGUI_SYSTEM_INTERFACE_VERSION "VGUI_System010"


#endif // ISYSTEM_H
