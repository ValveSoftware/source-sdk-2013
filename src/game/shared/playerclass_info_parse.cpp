//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Weapon data file parsing, shared by game & client dlls.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <KeyValues.h>
#include <tier0/mem.h>
#include "filesystem.h"
#include "utldict.h"
#include "ammodef.h"

#include "playerclass_info_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CUtlDict< FilePlayerClassInfo_t*, unsigned short > m_PlayerClassInfoDatabase;

#define MAX_PLAYERCLASSES	32

#ifdef _DEBUG

// used to track whether or not two player classes have been mistakenly assigned the same slot
bool g_bUsedPlayerClassSlots[MAX_PLAYERCLASSES] = { 0 };

#endif


#ifdef DEBUG

void CC_ReloadPlayerClasses_f (void)
{
	//ResetFilePlayerClassInfoDatabase();
}

static ConCommand dod_reloadplayerclasses("dod_reloadplayerclasses", CC_ReloadPlayerClasses_f, "Reset player class info cache" );

#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : FilePlayerClassInfo_t
//-----------------------------------------------------------------------------
static PLAYERCLASS_FILE_INFO_HANDLE FindPlayerClassInfoSlot( const char *name )
{
	// Complain about duplicately defined metaclass names...
	unsigned short lookup = m_PlayerClassInfoDatabase.Find( name );
	if ( lookup != m_PlayerClassInfoDatabase.InvalidIndex() )
	{
		return lookup;
	}

	FilePlayerClassInfo_t *insert = CreatePlayerClassInfo();

	lookup = m_PlayerClassInfoDatabase.Insert( name, insert );
	Assert( lookup != m_PlayerClassInfoDatabase.InvalidIndex() );
	return lookup;
}

// Find a class slot, assuming the weapon's data has already been loaded.
PLAYERCLASS_FILE_INFO_HANDLE LookupPlayerClassInfoSlot( const char *name )
{
	return m_PlayerClassInfoDatabase.Find( name );
}



// FIXME, handle differently?
static FilePlayerClassInfo_t gNullPlayerClassInfo;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : FilePlayerClassInfo_t
//-----------------------------------------------------------------------------
FilePlayerClassInfo_t *GetFilePlayerClassInfoFromHandle( PLAYERCLASS_FILE_INFO_HANDLE handle )
{
	if ( handle == GetInvalidPlayerClassInfoHandle() )
	{
		Assert( !"bad index into playerclass info UtlDict" );
		return &gNullPlayerClassInfo;
	}

	return m_PlayerClassInfoDatabase[ handle ];
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : PLAYERCLASS_FILE_INFO_HANDLE
//-----------------------------------------------------------------------------
PLAYERCLASS_FILE_INFO_HANDLE GetInvalidPlayerClassInfoHandle( void )
{
	return (PLAYERCLASS_FILE_INFO_HANDLE)m_PlayerClassInfoDatabase.InvalidIndex();
}

void ResetFilePlayerClassInfoDatabase( void )
{
	m_PlayerClassInfoDatabase.PurgeAndDeleteElements();

#ifdef _DEBUG
	memset(g_bUsedPlayerClassSlots, 0, sizeof(g_bUsedPlayerClassSlots));
#endif
}

#ifndef _XBOX
KeyValues* ReadEncryptedKVPlayerClassFile( IFileSystem *pFilesystem, const char *szFilenameWithoutExtension, const unsigned char *pICEKey )
{
	Assert( strchr( szFilenameWithoutExtension, '.' ) == NULL );
	char szFullName[512];

	// Open the weapon data file, and abort if we can't
	KeyValues *pKV = new KeyValues( "PlayerClassDatafile" );

	Q_snprintf(szFullName,sizeof(szFullName), "%s.txt", szFilenameWithoutExtension);

	if ( !pKV->LoadFromFile( pFilesystem, szFullName, "GAME" ) ) // try to load the normal .txt file first
	{
		if ( pICEKey )
		{
			Q_snprintf(szFullName,sizeof(szFullName), "%s.ctx", szFilenameWithoutExtension); // fall back to the .ctx file

			FileHandle_t f = pFilesystem->Open( szFullName, "rb", "GAME");

			if (!f)
			{
				pKV->deleteThis();
				return NULL;
			}
			// load file into a null-terminated buffer
			int fileSize = pFilesystem->Size(f);
			char *buffer = (char*)MemAllocScratch(fileSize + 1);
		
			Assert(buffer);
		
			pFilesystem->Read(buffer, fileSize, f); // read into local buffer
			buffer[fileSize] = 0; // null terminate file as EOF
			pFilesystem->Close( f );	// close file after reading

			UTIL_DecodeICE( (unsigned char*)buffer, fileSize, pICEKey );

			bool retOK = pKV->LoadFromBuffer( szFullName, buffer, pFilesystem );

			MemFreeScratch();

			if ( !retOK )
			{
				pKV->deleteThis();
				return NULL;
			}
		}
		else
		{
			pKV->deleteThis();
			return NULL;
		}
	}

	return pKV;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Read data on weapon from script file
// Output:  true  - if data2 successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------
bool ReadPlayerClassDataFromFileForSlot( IFileSystem* pFilesystem, const char *szPlayerClassName, PLAYERCLASS_FILE_INFO_HANDLE *phandle, const unsigned char *pICEKey )
{
	if ( !phandle )
	{
		Assert( 0 );
		return false;
	}

	*phandle = FindPlayerClassInfoSlot( szPlayerClassName );
	FilePlayerClassInfo_t *pFileInfo = GetFilePlayerClassInfoFromHandle( *phandle );
	Assert( pFileInfo );

	if ( pFileInfo->m_bParsedScript )
		return true;

	char sz[128];
	Q_snprintf( sz, sizeof( sz ), "scripts/playerclass_%s", szPlayerClassName );
	KeyValues *pKV = ReadEncryptedKVFile( pFilesystem, sz, pICEKey );
	if ( !pKV )
		return false;

	pFileInfo->Parse( pKV, szPlayerClassName );

	pKV->deleteThis();

	return true;
}


//-----------------------------------------------------------------------------
// FilePlayerClassInfo_t implementation.
//-----------------------------------------------------------------------------

FilePlayerClassInfo_t::FilePlayerClassInfo_t()
{
	m_bParsedScript = false;

	m_szPlayerClassName[0] = 0;
	m_szPrintName[0] = 0;
	m_szPlayerModel[0] = 0;
	m_szSelectCmd[0] = 0;
}

void FilePlayerClassInfo_t::Parse( KeyValues *pKeyValuesData, const char *szPlayerClassName )
{
	// Okay, we tried at least once to look this up...
	m_bParsedScript = true;

	// Classname
	Q_strncpy( m_szPlayerClassName, szPlayerClassName, MAX_WEAPON_STRING );

	// Printable name
	Q_strncpy( m_szPrintName, pKeyValuesData->GetString( "printname", "!! Missing printname on Player Class" ), MAX_PLAYERCLASS_NAME_LENGTH );

	// Player Model
	Q_strncpy( m_szPlayerModel, pKeyValuesData->GetString( "playermodel", "!! Missing playermodel on Player Class" ), MAX_PLAYERCLASS_NAME_LENGTH );

	// Select command
	Q_strncpy( m_szSelectCmd, pKeyValuesData->GetString( "selectcmd", "!! Missing selectcmd on Player Class" ), 32 );


#if defined(_DEBUG) && defined(HL2_CLIENT_DLL)

	// Use this for class select keys

	/*
	// make sure two weapons aren't in the same slot & position
	if (g_bUsedPlayerClassSlots[iSlot])
	{
		Msg( "Weapon slot info: %s (%d, %d)\n", szPrintName, iSlot, iPosition );
		Warning( "Duplicately assigned weapon to slots in selection hud\n" );
	}
	g_bUsedPlayerClassSlots[iSlot][iPosition] = true;
	*/
#endif
}

