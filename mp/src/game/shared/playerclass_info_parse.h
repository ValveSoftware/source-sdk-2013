//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Player class data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERCLASS_INFO_PARSE_H
#define PLAYERCLASS_INFO_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "shareddefs.h"

class IFileSystem;
class KeyValues;

typedef unsigned short PLAYERCLASS_FILE_INFO_HANDLE;

#define MAX_PLAYERCLASS_NAME_LENGTH		128

//-----------------------------------------------------------------------------
// Purpose: Contains the data read from the player class script files. 
// It's cached so we only read each script file once.
// Each game provides a CreatePlayerClassInfo function so it can have game-specific
// data in the player class scripts.
//-----------------------------------------------------------------------------
class FilePlayerClassInfo_t
{
public:

	FilePlayerClassInfo_t();
	
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szClassName );

	
public:	
	bool		m_bParsedScript;

public:
	// Class properties

	// todo : better lengths for these arrays ?

	char		m_szPlayerClassName[MAX_PLAYERCLASS_NAME_LENGTH];
	char		m_szPrintName[MAX_PLAYERCLASS_NAME_LENGTH];		// localization key for print name
	char		m_szPlayerModel[MAX_PLAYERCLASS_NAME_LENGTH];
	char		m_szSelectCmd[32];		//command the player can issue to switch to this class
};

// The weapon parse function
bool ReadPlayerClassDataFromFileForSlot( IFileSystem* filesystem, const char *szClassName, 
	PLAYERCLASS_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey = NULL );

// If player class info has been loaded for the specified class name, this returns it.
PLAYERCLASS_FILE_INFO_HANDLE LookupPlayerClassInfoSlot( const char *name );

// Given a handle to the player class info, return the class data
FilePlayerClassInfo_t *GetFilePlayerClassInfoFromHandle( PLAYERCLASS_FILE_INFO_HANDLE handle );

// Get the null Player Class object
PLAYERCLASS_FILE_INFO_HANDLE GetInvalidPlayerClassInfoHandle( void );

// Initialize all player class info
void ResetFilePlayerClassInfoDatabase( void );


// 
// Read a possibly-encrypted KeyValues file in. 
// If pICEKey is NULL, then it appends .txt to the filename and loads it as an unencrypted file.
// If pICEKey is non-NULL, then it appends .ctx to the filename and loads it as an encrypted file.
//
// (This should be moved into a more appropriate place).
//
extern KeyValues* ReadEncryptedKVPlayerClassFile( IFileSystem *filesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey );


// Each game implements this. It can return a derived class and override Parse() if it wants.
extern FilePlayerClassInfo_t* CreatePlayerClassInfo();


#endif // PLAYERCLASS_INFO_PARSE_H
