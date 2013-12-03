//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// -----------------------
// cmdlib.c
// -----------------------
#include "tier0/platform.h"
#ifdef IS_WINDOWS_PC
#include <windows.h>
#endif
#include "cmdlib.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "tier1/strtools.h"
#ifdef _WIN32
#include <conio.h>
#endif
#include "utlvector.h"
#include "filesystem_helpers.h"
#include "utllinkedlist.h"
#include "tier0/icommandline.h"
#include "KeyValues.h"
#include "filesystem_tools.h"

#if defined( MPI )

	#include "vmpi.h"
	#include "vmpi_tools_shared.h"

#endif


#if defined( _WIN32 ) || defined( WIN32 )
#include <direct.h>
#endif

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// set these before calling CheckParm
int myargc;
char **myargv;

char		com_token[1024];

qboolean		archive;
char			archivedir[1024];

FileHandle_t g_pLogFile = 0;

CUtlLinkedList<CleanupFn, unsigned short> g_CleanupFunctions;
CUtlLinkedList<SpewHookFn, unsigned short> g_ExtraSpewHooks;

bool g_bStopOnExit = false;
void (*g_ExtraSpewHook)(const char*) = NULL;

#if defined( _WIN32 ) || defined( WIN32 )

void CmdLib_FPrintf( FileHandle_t hFile, const char *pFormat, ... )
{
	static CUtlVector<char> buf;
	if ( buf.Count() == 0 )
		buf.SetCount( 1024 );

	va_list marker;
	va_start( marker, pFormat );
	
	while ( 1 )
	{
		int ret = Q_vsnprintf( buf.Base(), buf.Count(), pFormat, marker );
		if ( ret >= 0 )
		{
			// Write the string.
			g_pFileSystem->Write( buf.Base(), ret, hFile );
			
			break;
		}
		else
		{
			// Make the buffer larger.
			int newSize = buf.Count() * 2;
			buf.SetCount( newSize );
			if ( buf.Count() != newSize )
			{
				Error( "CmdLib_FPrintf: can't allocate space for text." );
			}
		}
	}

	va_end( marker );
}

char* CmdLib_FGets( char *pOut, int outSize, FileHandle_t hFile )
{
	int iCur=0;
	for ( ; iCur < (outSize-1); iCur++ )
	{
		char c;
		if ( !g_pFileSystem->Read( &c, 1, hFile ) )
		{
			if ( iCur == 0 )
				return NULL;
			else
				break;
		}

		pOut[iCur] = c;
		if ( c == '\n' )
			break;

		if ( c == EOF )
		{
			if ( iCur == 0 )
				return NULL;
			else
				break;
		}
	}

	pOut[iCur] = 0;
	return pOut;
}

#if !defined( _X360 )
#include <wincon.h>
#endif

// This pauses before exiting if they use -StopOnExit. Useful for debugging.
class CExitStopper
{
public:
	~CExitStopper()
	{
		if ( g_bStopOnExit )
		{
			Warning( "\nPress any key to quit.\n" );
			getch();
		}
	}
} g_ExitStopper;


static unsigned short g_InitialColor = 0xFFFF;
static unsigned short g_LastColor = 0xFFFF;
static unsigned short g_BadColor = 0xFFFF;
static WORD g_BackgroundFlags = 0xFFFF;
static void GetInitialColors( )
{
#if !defined( _X360 )
	// Get the old background attributes.
	CONSOLE_SCREEN_BUFFER_INFO oldInfo;
	GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE ), &oldInfo );
	g_InitialColor = g_LastColor = oldInfo.wAttributes & (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY);
	g_BackgroundFlags = oldInfo.wAttributes & (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY);

	g_BadColor = 0;
	if (g_BackgroundFlags & BACKGROUND_RED)
		g_BadColor |= FOREGROUND_RED;
	if (g_BackgroundFlags & BACKGROUND_GREEN)
		g_BadColor |= FOREGROUND_GREEN;
	if (g_BackgroundFlags & BACKGROUND_BLUE)
		g_BadColor |= FOREGROUND_BLUE;
	if (g_BackgroundFlags & BACKGROUND_INTENSITY)
		g_BadColor |= FOREGROUND_INTENSITY;
#endif
}

WORD SetConsoleTextColor( int red, int green, int blue, int intensity )
{
	WORD ret = g_LastColor;
#if !defined( _X360 )
	
	g_LastColor = 0;
	if( red )	g_LastColor |= FOREGROUND_RED;
	if( green ) g_LastColor |= FOREGROUND_GREEN;
	if( blue )  g_LastColor |= FOREGROUND_BLUE;
	if( intensity ) g_LastColor |= FOREGROUND_INTENSITY;

	// Just use the initial color if there's a match...
	if (g_LastColor == g_BadColor)
		g_LastColor = g_InitialColor;

	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), g_LastColor | g_BackgroundFlags );
#endif
	return ret;
}

void RestoreConsoleTextColor( WORD color )
{
#if !defined( _X360 )
	SetConsoleTextAttribute( GetStdHandle( STD_OUTPUT_HANDLE ), color | g_BackgroundFlags );
	g_LastColor = color;
#endif
}


#if defined( CMDLIB_NODBGLIB )

// This can go away when everything is in bin.
void Error( char const *pMsg, ... )
{
	va_list marker;
	va_start( marker, pMsg );
	vprintf( pMsg, marker );
	va_end( marker );

	exit( -1 );
}

#else

CRITICAL_SECTION g_SpewCS;
bool g_bSpewCSInitted = false;
bool g_bSuppressPrintfOutput = false;

SpewRetval_t CmdLib_SpewOutputFunc( SpewType_t type, char const *pMsg )
{
	// Hopefully two threads won't call this simultaneously right at the start!
	if ( !g_bSpewCSInitted )
	{
		InitializeCriticalSection( &g_SpewCS );
		g_bSpewCSInitted = true;
	}

	WORD old;
	SpewRetval_t retVal;
	
	EnterCriticalSection( &g_SpewCS );
	{
		if (( type == SPEW_MESSAGE ) || (type == SPEW_LOG ))
		{
			Color c = *GetSpewOutputColor();
			if ( c.r() != 255 || c.g() != 255 || c.b() != 255 )
			{
				// custom color
				old = SetConsoleTextColor( c.r(), c.g(), c.b(), c.a() );
			}
			else
			{
				old = SetConsoleTextColor( 1, 1, 1, 0 );
			}
			retVal = SPEW_CONTINUE;
		}
		else if( type == SPEW_WARNING )
		{
			old = SetConsoleTextColor( 1, 1, 0, 1 );
			retVal = SPEW_CONTINUE;
		}
		else if( type == SPEW_ASSERT )
		{
			old = SetConsoleTextColor( 1, 0, 0, 1 );
			retVal = SPEW_DEBUGGER;

#ifdef MPI
			// VMPI workers don't want to bring up dialogs and suchlike.
			// They need to have a special function installed to handle
			// the exceptions and write the minidumps.
			// Install the function after VMPI_Init with a call:
			// SetupToolsMinidumpHandler( VMPI_ExceptionFilter );
			if ( g_bUseMPI && !g_bMPIMaster && !Plat_IsInDebugSession() )
			{
				// Generating an exception and letting the
				// installed handler handle it
				::RaiseException
					(
					0,							// dwExceptionCode
					EXCEPTION_NONCONTINUABLE,	// dwExceptionFlags
					0,							// nNumberOfArguments,
					NULL						// const ULONG_PTR* lpArguments
					);

					// Never get here (non-continuable exception)
				
				VMPI_HandleCrash( pMsg, NULL, true );
				exit( 0 );
			}
#endif
		}
		else if( type == SPEW_ERROR )
		{
			old = SetConsoleTextColor( 1, 0, 0, 1 );
			retVal = SPEW_ABORT; // doesn't matter.. we exit below so we can return an errorlevel (which dbg.dll doesn't do).
		}
		else
		{
			old = SetConsoleTextColor( 1, 1, 1, 1 );
			retVal = SPEW_CONTINUE;
		}

		if ( !g_bSuppressPrintfOutput || type == SPEW_ERROR )
			printf( "%s", pMsg );

		OutputDebugString( pMsg );
		
		if ( type == SPEW_ERROR )
		{
			printf( "\n" );
			OutputDebugString( "\n" );
		}

		if( g_pLogFile )
		{
			CmdLib_FPrintf( g_pLogFile, "%s", pMsg );
			g_pFileSystem->Flush( g_pLogFile );
		}

		// Dispatch to other spew hooks.
		FOR_EACH_LL( g_ExtraSpewHooks, i )
			g_ExtraSpewHooks[i]( pMsg );

		RestoreConsoleTextColor( old );
	}
	LeaveCriticalSection( &g_SpewCS );

	if ( type == SPEW_ERROR )
	{
		CmdLib_Exit( 1 );
	}

	return retVal;
}


void InstallSpewFunction()
{
	setvbuf( stdout, NULL, _IONBF, 0 );
	setvbuf( stderr, NULL, _IONBF, 0 );

	SpewOutputFunc( CmdLib_SpewOutputFunc );
	GetInitialColors();
}


void InstallExtraSpewHook( SpewHookFn pFn )
{
	g_ExtraSpewHooks.AddToTail( pFn );
}

#if 0
void CmdLib_AllocError( unsigned long size )
{
	Error( "Error trying to allocate %d bytes.\n", size );
}


int CmdLib_NewHandler( size_t size )
{
	CmdLib_AllocError( size );
	return 0;
}
#endif

void InstallAllocationFunctions()
{
//	_set_new_mode( 1 ); // so if malloc() fails, we exit.
//	_set_new_handler( CmdLib_NewHandler );
}

void SetSpewFunctionLogFile( char const *pFilename )
{
	Assert( (!g_pLogFile) );
	g_pLogFile = g_pFileSystem->Open( pFilename, "a" );

	Assert( g_pLogFile );
	if (!g_pLogFile)
		Error("Can't create LogFile:\"%s\"\n", pFilename );

	CmdLib_FPrintf( g_pLogFile, "\n\n\n" );
}


void CloseSpewFunctionLogFile()
{
	if ( g_pFileSystem && g_pLogFile )
	{
		g_pFileSystem->Close( g_pLogFile );
		g_pLogFile = FILESYSTEM_INVALID_HANDLE;
	}
}


void CmdLib_AtCleanup( CleanupFn pFn )
{
	g_CleanupFunctions.AddToTail( pFn );
}


void CmdLib_Cleanup()
{
	CloseSpewFunctionLogFile();

	CmdLib_TermFileSystem();

	FOR_EACH_LL( g_CleanupFunctions, i )
		g_CleanupFunctions[i]();

#if defined( MPI )
	// Unfortunately, when you call exit(), even if you have things registered with atexit(),
	// threads go into a seemingly undefined state where GetExitCodeThread gives STILL_ACTIVE
	// and WaitForSingleObject will stall forever on the thread. Because of this, we must cleanup
	// everything that uses threads before exiting.
	VMPI_Finalize();
#endif
}


void CmdLib_Exit( int exitCode )
{
	TerminateProcess( GetCurrentProcess(), 1 );
}	



#endif

#endif




/*
===================
ExpandWildcards

Mimic unix command line expansion
===================
*/
#define	MAX_EX_ARGC	1024
int		ex_argc;
char	*ex_argv[MAX_EX_ARGC];
#if defined( _WIN32 ) && !defined( _X360 )
#include "io.h"
void ExpandWildcards (int *argc, char ***argv)
{
	struct _finddata_t fileinfo;
	int		handle;
	int		i;
	char	filename[1024];
	char	filebase[1024];
	char	*path;

	ex_argc = 0;
	for (i=0 ; i<*argc ; i++)
	{
		path = (*argv)[i];
		if ( path[0] == '-'
			|| ( !strstr(path, "*") && !strstr(path, "?") ) )
		{
			ex_argv[ex_argc++] = path;
			continue;
		}

		handle = _findfirst (path, &fileinfo);
		if (handle == -1)
			return;

		Q_ExtractFilePath (path, filebase, sizeof( filebase ));

		do
		{
			sprintf (filename, "%s%s", filebase, fileinfo.name);
			ex_argv[ex_argc++] = copystring (filename);
		} while (_findnext( handle, &fileinfo ) != -1);

		_findclose (handle);
	}

	*argc = ex_argc;
	*argv = ex_argv;
}
#else
void ExpandWildcards (int *argc, char ***argv)
{
}
#endif


// only printf if in verbose mode
qboolean verbose = false;
void qprintf (const char *format, ...)
{
	if (!verbose)
		return;

	va_list argptr;
	va_start (argptr,format);

	char str[2048];
	Q_vsnprintf( str, sizeof(str), format, argptr );

#if defined( CMDLIB_NODBGLIB )
	printf( "%s", str );
#else
	Msg( "%s", str );
#endif

	va_end (argptr);
}


// ---------------------------------------------------------------------------------------------------- //
// Helpers.
// ---------------------------------------------------------------------------------------------------- //

static void CmdLib_getwd( char *out, int outSize )
{
#if defined( _WIN32 ) || defined( WIN32 )
	_getcwd( out, outSize );
	Q_strncat( out, "\\", outSize, COPY_ALL_CHARACTERS );
#else
	getcwd(out, outSize);
	strcat(out, "/");
#endif
	Q_FixSlashes( out );
}

char *ExpandArg (char *path)
{
	static char full[1024];

	if (path[0] != '/' && path[0] != '\\' && path[1] != ':')
	{
		CmdLib_getwd (full, sizeof( full ));
		Q_strncat (full, path, sizeof( full ), COPY_ALL_CHARACTERS);
	}
	else
		Q_strncpy (full, path, sizeof( full ));
	return full;
}


char *ExpandPath (char *path)
{
	static char full[1024];
	if (path[0] == '/' || path[0] == '\\' || path[1] == ':')
		return path;
	sprintf (full, "%s%s", qdir, path);
	return full;
}



char *copystring(const char *s)
{
	char	*b;
	b = (char *)malloc(strlen(s)+1);
	strcpy (b, s);
	return b;
}


void GetHourMinuteSeconds( int nInputSeconds, int &nHours, int &nMinutes, int &nSeconds )
{
}


void GetHourMinuteSecondsString( int nInputSeconds, char *pOut, int outLen )
{
	int nMinutes = nInputSeconds / 60;
	int nSeconds = nInputSeconds - nMinutes * 60;
	int nHours = nMinutes / 60;
	nMinutes -= nHours * 60;

	const char *extra[2] = { "", "s" };
	
	if ( nHours > 0 )
		Q_snprintf( pOut, outLen, "%d hour%s, %d minute%s, %d second%s", nHours, extra[nHours != 1], nMinutes, extra[nMinutes != 1], nSeconds, extra[nSeconds != 1] );
	else if ( nMinutes > 0 )
		Q_snprintf( pOut, outLen, "%d minute%s, %d second%s", nMinutes, extra[nMinutes != 1], nSeconds, extra[nSeconds != 1] );
	else
		Q_snprintf( pOut, outLen, "%d second%s", nSeconds, extra[nSeconds != 1] );
}


void Q_mkdir (char *path)
{
#if defined( _WIN32 ) || defined( WIN32 )
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
//	if (errno != EEXIST)
	Error ("mkdir failed %s\n", path );
}

void CmdLib_InitFileSystem( const char *pFilename, int maxMemoryUsage )
{
	FileSystem_Init( pFilename, maxMemoryUsage );
	if ( !g_pFileSystem )
		Error( "CmdLib_InitFileSystem failed." );
}

void CmdLib_TermFileSystem()
{
	FileSystem_Term();
}

CreateInterfaceFn CmdLib_GetFileSystemFactory()
{
	return FileSystem_GetFactory();
}


/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}



/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char *data)
{
	return (char*)ParseFile( data, com_token, NULL );
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
	int             i;

	for (i = 1;i<myargc;i++)
	{
		if ( !Q_strcasecmp(check, myargv[i]) )
			return i;
	}

	return 0;
}



/*
================
Q_filelength
================
*/
int Q_filelength (FileHandle_t f)
{
	return g_pFileSystem->Size( f );
}


FileHandle_t SafeOpenWrite ( const char *filename )
{
	FileHandle_t f = g_pFileSystem->Open(filename, "wb");

	if (!f)
	{
		//Error ("Error opening %s: %s",filename,strerror(errno));
		// BUGBUG: No way to get equivalent of errno from IFileSystem!
		Error ("Error opening %s! (Check for write enable)\n",filename);
	}

	return f;
}

#define MAX_CMDLIB_BASE_PATHS 10
static char g_pBasePaths[MAX_CMDLIB_BASE_PATHS][MAX_PATH];
static int g_NumBasePaths = 0;

void CmdLib_AddBasePath( const char *pPath )
{
//	printf( "CmdLib_AddBasePath( \"%s\" )\n", pPath );
	if( g_NumBasePaths < MAX_CMDLIB_BASE_PATHS )
	{
		Q_strncpy( g_pBasePaths[g_NumBasePaths], pPath, MAX_PATH );
		Q_FixSlashes( g_pBasePaths[g_NumBasePaths] );
		g_NumBasePaths++;
	}
	else
	{
		Assert( 0 );
	}
}

bool CmdLib_HasBasePath( const char *pFileName_, int &pathLength )
{
	char *pFileName = ( char * )_alloca( strlen( pFileName_ ) + 1 );
	strcpy( pFileName, pFileName_ );
	Q_FixSlashes( pFileName );
	pathLength = 0;
	int i;
	for( i = 0; i < g_NumBasePaths; i++ )
	{
		// see if we can rip the base off of the filename.
		if( Q_strncasecmp( g_pBasePaths[i], pFileName, strlen( g_pBasePaths[i] ) ) == 0 )
		{
			pathLength = strlen( g_pBasePaths[i] );
			return true;
		}
	}
	return false;
}

int CmdLib_GetNumBasePaths( void )
{
	return g_NumBasePaths;
}

const char *CmdLib_GetBasePath( int i )
{
	Assert( i >= 0 && i < g_NumBasePaths );
	return g_pBasePaths[i];
}


//-----------------------------------------------------------------------------
// Like ExpandPath but expands the path for each base path like SafeOpenRead
//-----------------------------------------------------------------------------
int CmdLib_ExpandWithBasePaths( CUtlVector< CUtlString > &expandedPathList, const char *pszPath )
{
	int nPathLength = 0;

	pszPath = ExpandPath( const_cast< char * >( pszPath ) );	// Kind of redundant but it's how CmdLib_HasBasePath needs things

	if ( CmdLib_HasBasePath( pszPath, nPathLength ) )
	{
		pszPath = pszPath + nPathLength;
		for ( int i = 0; i < CmdLib_GetNumBasePaths(); ++i )
		{
			CUtlString &expandedPath = expandedPathList[ expandedPathList.AddToTail( CmdLib_GetBasePath( i ) ) ];
			expandedPath += pszPath;
		}
	}
	else
	{
		expandedPathList.AddToTail( pszPath );
	}

	return expandedPathList.Count();
}


FileHandle_t SafeOpenRead( const char *filename )
{
	int pathLength;
	FileHandle_t f = 0;
	if( CmdLib_HasBasePath( filename, pathLength ) )
	{
		filename = filename + pathLength;
		int i;
		for( i = 0; i < g_NumBasePaths; i++ )
		{
			char tmp[MAX_PATH];
			strcpy( tmp, g_pBasePaths[i] );
			strcat( tmp, filename );
			f = g_pFileSystem->Open( tmp, "rb" );
			if( f )
			{
				return f;
			}
		}
		Error ("Error opening %s\n",filename );
		return f;
	}
	else
	{
		f = g_pFileSystem->Open( filename, "rb" );
		if ( !f )
			Error ("Error opening %s",filename );

		return f;
	}
}

void SafeRead( FileHandle_t f, void *buffer, int count)
{
	if ( g_pFileSystem->Read (buffer, count, f) != (size_t)count)
		Error ("File read failure");
}


void SafeWrite ( FileHandle_t f, void *buffer, int count)
{
	if (g_pFileSystem->Write (buffer, count, f) != (size_t)count)
		Error ("File write failure");
}


/*
==============
FileExists
==============
*/
qboolean	FileExists ( const char *filename )
{
	FileHandle_t hFile = g_pFileSystem->Open( filename, "rb" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
	{
		return false;
	}
	else
	{
		g_pFileSystem->Close( hFile );
		return true;
	}
}

/*
==============
LoadFile
==============
*/
int    LoadFile ( const char *filename, void **bufferptr )
{
	int    length = 0;
	void    *buffer;

	FileHandle_t f = SafeOpenRead (filename);
	if ( FILESYSTEM_INVALID_HANDLE != f )
	{
		length = Q_filelength (f);
		buffer = malloc (length+1);
		((char *)buffer)[length] = 0;
		SafeRead (f, buffer, length);
		g_pFileSystem->Close (f);
		*bufferptr = buffer;
	}
	else
	{
		*bufferptr = NULL;
	}
	return length;
}



/*
==============
SaveFile
==============
*/
void    SaveFile ( const char *filename, void *buffer, int count )
{
	FileHandle_t f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	g_pFileSystem->Close (f);
}

/*
====================
Extract file parts
====================
*/
// FIXME: should include the slash, otherwise
// backing to an empty path will be wrong when appending a slash



/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
	char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: %s",hex);
		str++;
	}

	return num;
}


int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}

/*
============
CreatePath
============
*/
void CreatePath (char *path)
{
	char	*ofs, c;

	// strip the drive
	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		c = *ofs;
		if (c == '/' || c == '\\')
		{	// create the directory
			*ofs = 0;
			Q_mkdir (path);
			*ofs = c;
		}
	}
}

//-----------------------------------------------------------------------------
// Creates a path, path may already exist
//-----------------------------------------------------------------------------
#if defined( _WIN32 ) || defined( WIN32 )
void SafeCreatePath( char *path )
{
	char *ptr;

	// skip past the drive path, but don't strip
	if ( path[1] == ':' )
	{
		ptr = strchr( path, '\\' );
	}
	else
	{
		ptr = path;
	}
	while ( ptr )
	{		
		ptr = strchr( ptr+1, '\\' );
		if ( ptr )
		{
			*ptr = '\0';
			_mkdir( path );
			*ptr = '\\';
		}
	}
}
#endif

/*
============
QCopyFile

  Used to archive source files
============
*/
void QCopyFile (char *from, char *to)
{
	void	*buffer;
	int		length;

	length = LoadFile (from, &buffer);
	CreatePath (to);
	SaveFile (to, buffer, length);
	free (buffer);
}



