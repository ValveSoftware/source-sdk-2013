//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: vcd_sound_check.cpp : Defines the entry point for the console application.
//
//===========================================================================//
#include <stdio.h>
#include <windows.h>
#include "tier0/dbg.h"
#include "tier1/utldict.h"
#include "filesystem.h"
#include "cmdlib.h"
#include "scriplib.h"
#include "vstdlib/random.h"
#include "tier1/UtlBuffer.h"
#include "pacifier.h"
#include "appframework/tier3app.h"
#include "tier0/icommandline.h"
#include "vgui/IVGui.h"
#include "vgui_controls/controls.h"
#include "vgui/ILocalize.h"
#include "tier1/checksum_crc.h"
#include "tier1/UtlSortVector.h"
#include "tier1/utlmap.h"
#include "captioncompiler.h"

#include "tier0/fasttimer.h"

using namespace vgui;

// #define TESTING 1


bool uselogfile = false;
bool bX360 = false;

struct AnalysisData
{
	CUtlSymbolTable				symbols;
};

static AnalysisData g_Analysis;

IBaseFileSystem *filesystem = NULL;

static bool spewed = false;

SpewRetval_t SpewFunc( SpewType_t type, char const *pMsg )
{	
	spewed = true;

	printf( "%s", pMsg );
	OutputDebugString( pMsg );
	
	if ( type == SPEW_ERROR )
	{
		printf( "\n" );
		OutputDebugString( "\n" );
	}

	return SPEW_CONTINUE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : depth - 
//			*fmt - 
//			... - 
//-----------------------------------------------------------------------------
void vprint( int depth, const char *fmt, ... )
{
	char string[ 8192 ];
	va_list va;
	va_start( va, fmt );
	vsprintf( string, fmt, va );
	va_end( va );

	FILE *fp = NULL;

	if ( uselogfile )
	{
		fp = fopen( "log.txt", "ab" );
	}

	while ( depth-- > 0 )
	{
		printf( "  " );
		OutputDebugString( "  " );
		if ( fp )
		{
			fprintf( fp, "  " );
		}
	}

	::printf( "%s", string );
	OutputDebugString( string );

	if ( fp )
	{
		char *p = string;
		while ( *p )
		{
			if ( *p == '\n' )
			{
				fputc( '\r', fp );
			}
			fputc( *p, fp );
			p++;
		}
		fclose( fp );
	}
}

void logprint( char const *logfile, const char *fmt, ... )
{
	char string[ 8192 ];
	va_list va;
	va_start( va, fmt );
	vsprintf( string, fmt, va );
	va_end( va );

	FILE *fp = NULL;
	static bool first = true;
	if ( first )
	{
		first = false;
		fp = fopen( logfile, "wb" );
	}
	else
	{
		fp = fopen( logfile, "ab" );
	}
	if ( fp )
	{
		char *p = string;
		while ( *p )
		{
			if ( *p == '\n' )
			{
				fputc( '\r', fp );
			}
			fputc( *p, fp );
			p++;
		}
		fclose( fp );
	}
}


void Con_Printf( const char *fmt, ... )
{
	va_list args;
	static char output[1024];

	va_start( args, fmt );
	vprintf( fmt, args );
	vsprintf( output, fmt, args );

	vprint( 0, output );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void printusage( void )
{
	vprint( 0, "usage:  captioncompiler closecaptionfile.txt\n\
		\t-v = verbose output\n\
		\t-l = log to file log.txt\n\
		\ne.g.:  kvc -l u:/xbox/game/hl2x/resource/closecaption_english.txt" );

	// Exit app
	exit( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CheckLogFile( void )
{
	if ( uselogfile )
	{
		_unlink( "log.txt" );
		vprint( 0, "    Outputting to log.txt\n" );
	}
}

void PrintHeader()
{
	vprint( 0, "Valve Software - captioncompiler.exe (%s)\n", __DATE__ );
	vprint( 0, "--- Close Caption File compiler ---\n" );
}

//-----------------------------------------------------------------------------
// The application object
//-----------------------------------------------------------------------------
class CCompileCaptionsApp : public CTier3SteamApp
{
	typedef CTier3SteamApp BaseClass;

public:
	// Methods of IApplication
	virtual bool Create();
	virtual bool PreInit();
	virtual int Main();
	virtual void PostShutdown();
	virtual void Destroy();

private:
	// Sets up the search paths
	bool SetupSearchPaths();

	void CompileCaptionFile( char const *infile, char const *outfile );
	void DescribeCaptions( char const *file );
};


bool CCompileCaptionsApp::Create()
{
	SpewOutputFunc( SpewFunc );
	SpewActivate( "kvc", 2 );

	AppSystemInfo_t appSystems[] = 
	{
		{ "vgui2.dll",				VGUI_IVGUI_INTERFACE_VERSION },
		{ "", "" }	// Required to terminate the list
	};

	return AddSystems( appSystems );
}

void CCompileCaptionsApp::Destroy()
{
}


//-----------------------------------------------------------------------------
// Sets up the game path
//-----------------------------------------------------------------------------
bool CCompileCaptionsApp::SetupSearchPaths()
{
	if ( !BaseClass::SetupSearchPaths( NULL, false, true ) )
		return false;

	// Set gamedir.
	Q_MakeAbsolutePath( gamedir, sizeof( gamedir ), GetGameInfoPath() );
	Q_AppendSlash( gamedir, sizeof( gamedir ) );

	return true;
}


//-----------------------------------------------------------------------------
// Init, shutdown
//-----------------------------------------------------------------------------
bool CCompileCaptionsApp::PreInit( )
{
	if ( !BaseClass::PreInit() )
		return false;

	g_pFileSystem = g_pFullFileSystem;
	if ( !g_pFileSystem || !g_pVGui || !g_pVGuiLocalize )
	{
		Error( "Unable to load required library interface!\n" );
		return false;
	}

//	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f, false, false, false, false );
	g_pFullFileSystem->SetWarningFunc( Warning );

	// Add paths...
	if ( !SetupSearchPaths() )
		return false;

	return true; 
}

void CCompileCaptionsApp::PostShutdown()
{
	g_pFileSystem = NULL;
	BaseClass::PostShutdown();
}

void CCompileCaptionsApp::CompileCaptionFile( char const *infile, char const *outfile )
{
	StringIndex_t maxindex = (StringIndex_t)-1;
	int maxunicodesize = 0;
	int totalsize = 0;

	int c = 0;

	int curblock = 0;
	int usedBytes = 0;
	int blockSize = MAX_BLOCK_SIZE;

	int freeSpace = 0;

	CUtlVector< CaptionLookup_t >	directory;
	CUtlBuffer data;

	CUtlRBTree< unsigned int >	hashcollision( 0, 0, DefLessFunc( unsigned int ) );

	for ( StringIndex_t i = g_pVGuiLocalize->GetFirstStringIndex(); i != INVALID_LOCALIZE_STRING_INDEX; i = g_pVGuiLocalize->GetNextStringIndex( i ), ++c )
	{
		char const *entryName = g_pVGuiLocalize->GetNameByIndex( i );
		CaptionLookup_t entry;
		entry.SetHash( entryName );
		
		// 	vprint( 0, "%d / %d:  %s == %u\n", c, i, g_pVGuiLocalize->GetNameByIndex( i ), entry.hash );

		if ( hashcollision.Find( entry.hash ) != hashcollision.InvalidIndex() )
		{
			Error( "Hash name collision on %s!!!\n", g_pVGuiLocalize->GetNameByIndex( i ) );
		}

		hashcollision.Insert( entry.hash );

		const wchar_t *text = g_pVGuiLocalize->GetValueByIndex( i );
		if ( verbose )
		{
			vprint( 0, "Processing: '%30.30s' = '%S'\n", entryName, text );
		}
		int len = text ? ( wcslen( text ) + 1 ) * sizeof( short ) : 0;
		if ( len > maxunicodesize )
		{
			maxindex = i;
			maxunicodesize = len;
		}

		if ( len > blockSize )
		{
			Error( "Caption text file '%s' contains a single caption '%s' of %d bytes (%d is max), change MAX_BLOCK_SIZE in captioncompiler.h to fix!!!\n", g_pVGuiLocalize->GetNameByIndex( i ),
				entryName, len, blockSize );
		}
		totalsize += len;

		if ( usedBytes + len >= blockSize )
		{
			++curblock;

			int leftover = ( blockSize - usedBytes );

			totalsize += leftover;

            freeSpace += leftover;

			while ( --leftover >= 0 )
			{
				data.PutChar( 0 );
			}

			usedBytes = len;
			entry.offset = 0;

			data.Put( (const void *)text, len );
		}
		else
		{
			entry.offset = usedBytes;
			usedBytes += len;
			data.Put( (const void *)text, len );
		}

		entry.length = len;
		entry.blockNum = curblock;

		directory.AddToTail( entry );
	}
	
	int leftover = ( blockSize - usedBytes );
	totalsize += leftover;
	freeSpace += leftover;
	while ( --leftover >= 0 )
	{
		data.PutChar( 0 );
	}

	vprint( 0, "Found %i strings in '%s'\n", c, infile );

	if ( maxindex != INVALID_LOCALIZE_STRING_INDEX )
	{
		vprint( 0, "Longest string '%s' = (%i) bytes average(%.3f)\n%",
			g_pVGuiLocalize->GetNameByIndex( maxindex ), maxunicodesize, (float)totalsize/(float)c );
	}

	vprint( 0, "%d blocks (%d bytes each), %d bytes wasted (%.3f per block average), total bytes %d\n",
		curblock + 1, blockSize, freeSpace, (float)freeSpace/(float)( curblock + 1 ), totalsize );

	vprint( 0, "directory size %d entries, %d bytes, data size %d bytes\n",
		directory.Count(), directory.Count() * sizeof( CaptionLookup_t ), data.TellPut() );

	CompiledCaptionHeader_t header;
	header.magic			= COMPILED_CAPTION_FILEID;
	header.version			= COMPILED_CAPTION_VERSION;
	header.numblocks		= curblock + 1;
	header.blocksize		= blockSize;
	header.directorysize	= directory.Count();
	header.dataoffset		= 0;

	// Now write the outfile
	CUtlBuffer out;
	out.Put( &header, sizeof( header ) );
	out.Put( directory.Base(), directory.Count() * sizeof( CaptionLookup_t ) );
	int curOffset = out.TellPut();
	// Round it up to the next 512 byte boundary
	int nBytesDestBuffer = AlignValue( curOffset, 512 );  // align to HD sector
	int nPadding = nBytesDestBuffer - curOffset;
	while ( --nPadding >= 0 )
	{
		out.PutChar( 0 );
	}
	out.Put( data.Base(), data.TellPut() );

	// Write out a corrected header
	header.dataoffset = nBytesDestBuffer;
	int savePos = out.TellPut();
	out.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );
	out.Put( &header, sizeof( header ) );
	out.SeekPut( CUtlBuffer::SEEK_HEAD, savePos );

	g_pFullFileSystem->WriteFile( outfile, NULL, out );

	// Jeep: this function no longer exisits
	/*if ( bX360 )
	{
		UpdateOrCreateCaptionFile_X360( g_pFullFileSystem, outfile, NULL, true );
	}*/
}

void CCompileCaptionsApp::DescribeCaptions( char const *file )
{
	CUtlBuffer buf;
	if ( !g_pFullFileSystem->ReadFile( file, NULL, buf ) )
	{
		Error( "Unable to read '%s' into buffer\n", file );
	}

	CompiledCaptionHeader_t header;
	buf.Get( &header, sizeof( header ) );
	if ( header.magic != COMPILED_CAPTION_FILEID )
		Error( "Invalid file id for %s\n", file );
	if ( header.version != COMPILED_CAPTION_VERSION )
		Error( "Invalid file version for %s\n", file );

	// Read the directory
	CUtlSortVector< CaptionLookup_t, CCaptionLookupLess > directory;
	directory.EnsureCapacity( header.directorysize );
	directory.CopyArray( (const CaptionLookup_t *)buf.PeekGet(), header.directorysize );
	directory.RedoSort( true );
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, header.dataoffset );

	int i;
	CUtlVector< CaptionBlock_t >	blocks;
	for ( i = 0; i < header.numblocks; ++i )
	{
		CaptionBlock_t& newBlock = blocks[ blocks.AddToTail() ];
		Q_memset( newBlock.data, 0, sizeof( newBlock.data ) );
		buf.Get( newBlock.data, header.blocksize );
	}

	CUtlMap< unsigned int, StringIndex_t > inverseMap( 0, 0, DefLessFunc( unsigned int ) );
	for ( StringIndex_t idx = g_pVGuiLocalize->GetFirstStringIndex(); idx != INVALID_LOCALIZE_STRING_INDEX; idx = g_pVGuiLocalize->GetNextStringIndex( idx ) )
	{
		const char *name = g_pVGuiLocalize->GetNameByIndex( idx );
		CaptionLookup_t dummy;
		dummy.SetHash( name );

		inverseMap.Insert( dummy.hash, idx );
	}

	// Now print everything out...
	for ( i = 0; i < header.directorysize; ++i )
	{
		const CaptionLookup_t& entry = directory[ i ];
		char const *name = g_pVGuiLocalize->GetNameByIndex( inverseMap.Element( inverseMap.Find( entry.hash ) ) );
		const CaptionBlock_t& block = blocks[ entry.blockNum ];
		const wchar_t *data = (const wchar_t *)&block.data[ entry.offset ];
		wchar_t *temp = ( wchar_t * )_alloca( entry.length * sizeof( short ) );
		wcsncpy( temp, data, ( entry.length / sizeof( short ) ) - 1 );

		vprint( 0, "%3.3d:  (%40.40s) hash(%15.15u), block(%4.4d), offset(%4.4d), len(%4.4d) %S\n",
			i, name, entry.hash, entry.blockNum, entry.offset, entry.length, temp );
	}
}

//-----------------------------------------------------------------------------
// main application
//-----------------------------------------------------------------------------
int CCompileCaptionsApp::Main()
{
	CUtlVector< CUtlSymbol >	worklist;

	int i = 1;
	for ( i ; i<CommandLine()->ParmCount() ; i++)
	{
		if ( CommandLine()->GetParm( i )[ 0 ] == '-' )
		{
			switch( CommandLine()->GetParm( i )[ 1 ] )
			{
			case 'l':
				uselogfile = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'x':
				bX360 = true;
				break;
			case 'g': // -game
				++i;
				break;
			default:
				printusage();
				break;
			}
		}
		else if ( i != 0 )
		{
			char fn[ 512 ];
			Q_strncpy( fn, CommandLine()->GetParm( i ), sizeof( fn ) );
			Q_FixSlashes( fn );
			Q_strlower( fn );

			CUtlSymbol sym;
			sym = fn;
			worklist.AddToTail( sym );
		}
	}

	if ( CommandLine()->ParmCount() < 2 || ( i != CommandLine()->ParmCount() ) || worklist.Count() != 1 )
	{
		PrintHeader();
		printusage();
	}

	CheckLogFile();

	PrintHeader();

	char binaries[MAX_PATH];
	Q_strncpy( binaries, gamedir, MAX_PATH );
	Q_StripTrailingSlash( binaries );
	Q_strncat( binaries, "/../bin", MAX_PATH, MAX_PATH );

	char outfile[ 512 ];
	if ( Q_stristr( worklist[ worklist.Count() - 1 ].String(), gamedir ) )
	{
        Q_strncpy( outfile, &worklist[ worklist.Count() - 1 ].String()[ Q_strlen( gamedir ) ] , sizeof( outfile ) );
	}
	else
	{
        Q_snprintf( outfile, sizeof( outfile ), "resource\\%s", worklist[ worklist.Count() - 1 ].String() );
	}

	char infile[ 512 ];
	Q_strncpy( infile, outfile, sizeof( infile ) );

	Q_SetExtension( outfile, ".dat", sizeof( outfile ) );

	vprint( 0, "gamedir[ %s ]\n", gamedir );
	vprint( 0, "infile[ %s ]\n", infile );
	vprint( 0, "outfile[ %s ]\n", outfile );

	g_pFullFileSystem->AddSearchPath( binaries, "EXECUTABLE_PATH" );

	if ( !g_pVGuiLocalize->AddFile( infile, "MOD", false ) )
	{
		Error( "Unable to add localization file '%s'\n", infile );
	}

	vprint( 0, "    Compiling Captions for '%s'...\n", infile );

	CompileCaptionFile( infile, outfile );

	if ( verbose )
	{
		DescribeCaptions( outfile );
	}

	g_pVGuiLocalize->RemoveAll();

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Main entry point 
//-----------------------------------------------------------------------------
DEFINE_CONSOLE_STEAM_APPLICATION_OBJECT( CCompileCaptionsApp )
