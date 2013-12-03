//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef SCRIPLIB_H
#define SCRIPLIB_H

#ifdef _WIN32
#pragma once
#endif


enum ScriptPathMode_t
{
	SCRIPT_USE_ABSOLUTE_PATH,
	SCRIPT_USE_RELATIVE_PATH
};


// scriplib.h

#define	MAXTOKEN	1024

extern	char	token[MAXTOKEN];
extern	char	*scriptbuffer,*script_p,*scriptend_p;
extern	int		grabbed;
extern	int		scriptline;
extern	qboolean	endofscript;


// If pathMode is SCRIPT_USE_ABSOLUTE_PATH, then it uses ExpandPath() on the filename before
// trying to open it. Otherwise, it passes the filename straight into the filesystem
// (so you can leave it as a relative path).
void LoadScriptFile (char *filename, ScriptPathMode_t pathMode=SCRIPT_USE_ABSOLUTE_PATH);
void ParseFromMemory (char *buffer, int size);

qboolean GetToken (qboolean crossline);
qboolean GetExprToken (qboolean crossline);
void UnGetToken (void);
qboolean TokenAvailable (void);
qboolean GetTokenizerStatus( char **pFilename, int *pLine );
bool SetCheckSingleCharTokens( bool bCheck );

// SCRIPT_LOADED_CALLBACK:
//	Is called after the contents of a file is loaded.
//	pFilenameLoaded			is the path of a file that got loaded.
//	pIncludedFromFileName	is the name of the parent file or NULL if loaded because of "LoadScriptFile" toplevel call.
//	nIncludeLineNumber		is the number of the line in the parent file with $include statement or 0 in case of "LoadScriptFile"
typedef void ( * SCRIPT_LOADED_CALLBACK )( char const *pFilenameLoaded, char const *pIncludedFromFileName, int nIncludeLineNumber );

// SetScriptLoadedCallback:
//	Sets the new callback for script loading.
//	Returns the previous callback function.
SCRIPT_LOADED_CALLBACK SetScriptLoadedCallback( SCRIPT_LOADED_CALLBACK pfnNewScriptLoadedCallback );

#include "tier1/utlstring.h"
#include "tier1/utlvector.h"

CUtlString SetSingleCharTokenList( const char *pszSingleCharTokenList );

class CUtlBuffer;

enum DiskWriteMode_t
{
	WRITE_TO_DISK_NEVER,	
	WRITE_TO_DISK_ALWAYS,
	WRITE_TO_DISK_UPDATE,	// file must exist
};

struct fileList_t
{
	CUtlString	fileName;
	time_t		timeWrite;
};

class IScriptLib
{
public:
	virtual bool ReadFileToBuffer( const char *pSourceName, CUtlBuffer &buffer, bool bText = false, bool bNoOpenFailureWarning = false ) = 0;
	virtual bool WriteBufferToFile( const char *pTargetName, CUtlBuffer &buffer, DiskWriteMode_t writeMode ) = 0;
	virtual int	FindFiles( char* pFileMask, bool bRecurse, CUtlVector<fileList_t> &fileList ) = 0;
	virtual char *MakeTemporaryFilename( char const *pchModPath, char *pPath, int pathSize ) = 0;
	virtual void DeleteTemporaryFiles( const char *pFileMask ) = 0;
	virtual int CompareFileTime( const char *pFilenameA, const char *pFilenameB ) = 0;
	virtual bool DoesFileExist( const char *pFilename ) = 0;
};

extern IScriptLib *scriptlib;


#endif // SCRIPLIB_H
