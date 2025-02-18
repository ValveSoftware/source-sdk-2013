//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>		// for WideCharToMultiByte and MultiByteToWideChar
#elif defined(POSIX)
#include <wchar.h> // wcslen()
#define _alloca alloca
#define _wtoi(arg) wcstol(arg, NULL, 10)
#define _wtoi64(arg) wcstoll(arg, NULL, 10)
#endif

#include <KeyValues.h>
#include "filesystem.h"
#include <vstdlib/IKeyValuesSystem.h>
#include "tier0/icommandline.h"
#include "tier0/vprof_telemetry.h"
#include <Color.h>
#include <stdlib.h>
#include "tier0/dbg.h"
#include "tier0/mem.h"
#include "utlbuffer.h"
#include "utlhash.h"
#include "utlvector.h"
#include "utlqueue.h"
#include "UtlSortVector.h"
#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

static const char * s_LastFileLoadingFrom = "unknown"; // just needed for error messages

// Statics for the growable string table
int (*KeyValues::s_pfGetSymbolForString)( const char *name, bool bCreate ) = &KeyValues::GetSymbolForStringClassic;
const char *(*KeyValues::s_pfGetStringForSymbol)( int symbol ) = &KeyValues::GetStringForSymbolClassic;
CKeyValuesGrowableStringTable *KeyValues::s_pGrowableStringTable = NULL;

#define KEYVALUES_TOKEN_SIZE	4096
static char s_pTokenBuf[KEYVALUES_TOKEN_SIZE];


#define INTERNALWRITE( pData, len ) InternalWrite( filesystem, f, pBuf, pData, len )


// a simple class to keep track of a stack of valid parsed symbols
const int MAX_ERROR_STACK = 64;
class CKeyValuesErrorStack
{
public:
	CKeyValuesErrorStack() : m_pFilename("NULL"), m_errorIndex(0), m_maxErrorIndex(0) {}

	void SetFilename( const char *pFilename )
	{
		m_pFilename = pFilename;
		m_maxErrorIndex = 0;
	}

	// entering a new keyvalues block, save state for errors
	// Not save symbols instead of pointers because the pointers can move!
	int Push( int symName )
	{
		if ( m_errorIndex < MAX_ERROR_STACK )
		{
			m_errorStack[m_errorIndex] = symName;
		}
		m_errorIndex++;
		m_maxErrorIndex = max( m_maxErrorIndex, (m_errorIndex-1) );
		return m_errorIndex-1;
	}

	// exiting block, error isn't in this block, remove.
	void Pop()
	{
		m_errorIndex--;
		Assert(m_errorIndex>=0);
	}

	// Allows you to keep the same stack level, but change the name as you parse peers
	void Reset( int stackLevel, int symName )
	{
		Assert( stackLevel >= 0 );
		Assert( stackLevel < m_errorIndex );
		if ( stackLevel < MAX_ERROR_STACK )
			m_errorStack[stackLevel] = symName;
	}

	// Hit an error, report it and the parsing stack for context
	void ReportError( const char *pError )
	{
		bool bSpewCR = false;

		Warning( "KeyValues Error: %s in file %s\n", pError, m_pFilename );
		for ( int i = 0; i < m_maxErrorIndex; i++ )
		{
			if ( i < MAX_ERROR_STACK && m_errorStack[i] != INVALID_KEY_SYMBOL )
			{
				if ( i < m_errorIndex )
				{
					Warning( "%s, ", KeyValues::CallGetStringForSymbol(m_errorStack[i]) );
				}
				else
				{
					Warning( "(*%s*), ", KeyValues::CallGetStringForSymbol(m_errorStack[i]) );
				}

				bSpewCR = true;
			}
		}

		if ( bSpewCR )
			Warning( "\n" );
	}

private:
	int		m_errorStack[MAX_ERROR_STACK];
	const char *m_pFilename;
	int		m_errorIndex;
	int		m_maxErrorIndex;
} g_KeyValuesErrorStack;


// a simple helper that creates stack entries as it goes in & out of scope
class CKeyErrorContext
{
public:
	CKeyErrorContext( KeyValues *pKv )
	{
		Init( pKv->GetNameSymbol() );
	}

	~CKeyErrorContext()
	{
		g_KeyValuesErrorStack.Pop();
	}
	CKeyErrorContext( int symName )
	{
		Init( symName );
	}
	void Reset( int symName )
	{
		g_KeyValuesErrorStack.Reset( m_stackLevel, symName );
	}
	int GetStackLevel() const
	{
		return m_stackLevel;
	}
private:
	void Init( int symName )
	{
		m_stackLevel = g_KeyValuesErrorStack.Push( symName );
	}

	int m_stackLevel;
};

// Uncomment this line to hit the ~CLeakTrack assert to see what's looking like it's leaking
// #define LEAKTRACK

#ifdef LEAKTRACK

class CLeakTrack
{
public:
	CLeakTrack()
	{
	}
	~CLeakTrack()
	{
		if ( keys.Count() != 0 )
		{
			Assert( 0 );
		}
	}

	struct kve
	{
		KeyValues *kv;
		char		name[ 256 ];
	};

	void AddKv( KeyValues *kv, char const *name )
	{
		kve k;
		Q_strncpy( k.name, name ? name : "NULL", sizeof( k.name ) );
		k.kv = kv;

		keys.AddToTail( k );
	}

	void RemoveKv( KeyValues *kv )
	{
		int c = keys.Count();
		for ( int i = 0; i < c; i++ )
		{
			if ( keys[i].kv == kv )
			{
				keys.Remove( i );
				break;
			}
		}
	}

	CUtlVector< kve > keys;
};

static CLeakTrack track;

#define TRACK_KV_ADD( ptr, name )	track.AddKv( ptr, name )
#define TRACK_KV_REMOVE( ptr )		track.RemoveKv( ptr )

#else

#define TRACK_KV_ADD( ptr, name ) 
#define TRACK_KV_REMOVE( ptr )	

#endif


//-----------------------------------------------------------------------------
// Purpose: An arbitrarily growable string table for KeyValues key names. 
//	See the comment in the header for more info.
//-----------------------------------------------------------------------------
class CKeyValuesGrowableStringTable
{
public: 
	// Constructor
	CKeyValuesGrowableStringTable() :
		#ifdef PLATFORM_64BITS
			m_vecStrings( 0, 4 * 512 * 1024 )
		#else
			m_vecStrings( 0, 512 * 1024 )
		#endif
		, m_hashLookup( 2048, 0, 0, m_Functor, m_Functor )
	{
		m_vecStrings.AddToTail( '\0' );
	}

	// Translates a string to an index
	int GetSymbolForString( const char *name, bool bCreate = true )
	{
		AUTO_LOCK( m_mutex );

		// Put the current details into our hash functor
		m_Functor.SetCurString( name );
		m_Functor.SetCurStringBase( (const char *)m_vecStrings.Base() );

		if ( bCreate )
		{
			bool bInserted = false;
			UtlHashHandle_t hElement = m_hashLookup.Insert( -1, &bInserted );
			if ( bInserted )
			{
				int iIndex = m_vecStrings.AddMultipleToTail( V_strlen( name ) + 1, name );
				m_hashLookup[ hElement ] = iIndex;
			}

			return m_hashLookup[ hElement ];
		}
		else
		{
			UtlHashHandle_t hElement = m_hashLookup.Find( -1 );
			if ( m_hashLookup.IsValidHandle( hElement ) )
				return m_hashLookup[ hElement ];
			else
				return -1;
		}
	}

	// Translates an index back to a string
	const char *GetStringForSymbol( int symbol )
	{
		return (const char *)m_vecStrings.Base() + symbol;
	}

private:
	
	// A class plugged into CUtlHash that allows us to change the behavior of the table
	// and store only the index in the table.
	class CLookupFunctor
	{
	public:
		CLookupFunctor() : m_pchCurString( NULL ), m_pchCurBase( NULL ) {}

		// Sets what we are currently inserting or looking for.
		void SetCurString( const char *pchCurString ) { m_pchCurString = pchCurString; }
		void SetCurStringBase( const char *pchCurBase ) { m_pchCurBase = pchCurBase; }

		// The compare function.
		bool operator()( int nLhs, int nRhs ) const
		{
			const char *pchLhs = nLhs > 0 ? m_pchCurBase + nLhs : m_pchCurString;
			const char *pchRhs = nRhs > 0 ? m_pchCurBase + nRhs : m_pchCurString;
			
			return ( 0 == V_stricmp( pchLhs, pchRhs ) );
		}

		// The hash function.
		unsigned int operator()( int nItem ) const
		{
			return HashStringCaseless( m_pchCurString );
		}

	private:
		const char *m_pchCurString;
		const char *m_pchCurBase;
	};

	CThreadFastMutex m_mutex;
	CLookupFunctor	m_Functor;
	CUtlHash<int, CLookupFunctor &, CLookupFunctor &> m_hashLookup;
	CUtlVector<char> m_vecStrings;
};


//-----------------------------------------------------------------------------
// Purpose: Sets whether the KeyValues system should use an arbitrarily growable
//	string table. See the comment in the header for more info.
//-----------------------------------------------------------------------------
void KeyValues::SetUseGrowableStringTable( bool bUseGrowableTable )
{
	if ( bUseGrowableTable )
	{
		s_pfGetStringForSymbol = &(KeyValues::GetStringForSymbolGrowable);
		s_pfGetSymbolForString = &(KeyValues::GetSymbolForStringGrowable);

		if ( NULL == s_pGrowableStringTable )
		{
			s_pGrowableStringTable = new CKeyValuesGrowableStringTable;
		}
	}
	else
	{
		s_pfGetStringForSymbol = &(KeyValues::GetStringForSymbolClassic);
		s_pfGetSymbolForString = &(KeyValues::GetSymbolForStringClassic);

		delete s_pGrowableStringTable;
		s_pGrowableStringTable = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Bodys of the function pointers used for interacting with the key
//	name string table
//-----------------------------------------------------------------------------
int KeyValues::GetSymbolForStringClassic( const char *name, bool bCreate )
{
	return KeyValuesSystem()->GetSymbolForString( name, bCreate );
}

const char *KeyValues::GetStringForSymbolClassic( int symbol )
{
	return KeyValuesSystem()->GetStringForSymbol( symbol );
}

int KeyValues::GetSymbolForStringGrowable( const char *name, bool bCreate )
{
	return s_pGrowableStringTable->GetSymbolForString( name, bCreate );
}

const char *KeyValues::GetStringForSymbolGrowable( int symbol )
{
	return s_pGrowableStringTable->GetStringForSymbol( symbol );
}



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName ( setName );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName, const char *firstKey, const char *firstValue )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName( setName );
	SetString( firstKey, firstValue );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName, const char *firstKey, const wchar_t *firstValue )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName( setName );
	SetWString( firstKey, firstValue );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName, const char *firstKey, int firstValue )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName( setName );
	SetInt( firstKey, firstValue );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName, const char *firstKey, const char *firstValue, const char *secondKey, const char *secondValue )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName( setName );
	SetString( firstKey, firstValue );
	SetString( secondKey, secondValue );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
KeyValues::KeyValues( const char *setName, const char *firstKey, int firstValue, const char *secondKey, int secondValue )
{
	TRACK_KV_ADD( this, setName );

	Init();
	SetName( setName );
	SetInt( firstKey, firstValue );
	SetInt( secondKey, secondValue );
}

//-----------------------------------------------------------------------------
// Purpose: Initialize member variables
//-----------------------------------------------------------------------------
void KeyValues::Init()
{
	m_iKeyName = INVALID_KEY_SYMBOL;
	m_iDataType = TYPE_NONE;

	m_pSub = NULL;
	m_pPeer = NULL;
	m_pChain = NULL;

	m_sValue = NULL;
	m_wsValue = NULL;
	m_pValue = NULL;
	
	m_bHasEscapeSequences = false;
	m_bEvaluateConditionals = true;

	// for future proof
	memset( unused, 0, sizeof(unused) );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
KeyValues::~KeyValues()
{
	TRACK_KV_REMOVE( this );

	RemoveEverything();
}

//-----------------------------------------------------------------------------
// Purpose: remove everything
//-----------------------------------------------------------------------------
void KeyValues::RemoveEverything()
{
	KeyValues *dat;
	KeyValues *datNext = NULL;
	for ( dat = m_pSub; dat != NULL; dat = datNext )
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;
		delete dat;
	}

	for ( dat = m_pPeer; dat && dat != this; dat = datNext )
	{
		datNext = dat->m_pPeer;
		dat->m_pPeer = NULL;
		delete dat;
	}

	delete [] m_sValue;
	m_sValue = NULL;
	delete [] m_wsValue;
	m_wsValue = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *f - 
//-----------------------------------------------------------------------------

void KeyValues::RecursiveSaveToFile( CUtlBuffer& buf, int indentLevel, bool sortKeys /*= false*/, bool bAllowEmptyString /*= false*/ )
{
	RecursiveSaveToFile( NULL, FILESYSTEM_INVALID_HANDLE, &buf, indentLevel, sortKeys, bAllowEmptyString );
}

//-----------------------------------------------------------------------------
// Adds a chain... if we don't find stuff in this keyvalue, we'll look
// in the one we're chained to.
//-----------------------------------------------------------------------------

void KeyValues::ChainKeyValue( KeyValues* pChain )
{
	m_pChain = pChain;
}

//-----------------------------------------------------------------------------
// Purpose: Get the name of the current key section
//-----------------------------------------------------------------------------
const char *KeyValues::GetName( void ) const
{
	return s_pfGetStringForSymbol( m_iKeyName );
}

//-----------------------------------------------------------------------------
// Purpose: Read a single token from buffer (0 terminated)
//-----------------------------------------------------------------------------
#pragma warning (disable:4706)
const char *KeyValues::ReadToken( CUtlBuffer &buf, bool &wasQuoted, bool &wasConditional )
{
	wasQuoted = false;
	wasConditional = false;

	if ( !buf.IsValid() )
		return NULL; 

	// eating white spaces and remarks loop
	while ( true )
	{
		buf.EatWhiteSpace();
		if ( !buf.IsValid() )
			return NULL;	// file ends after reading whitespaces

		// stop if it's not a comment; a new token starts here
		if ( !buf.EatCPPComment() )
			break;
	}

	const char *c = (const char*)buf.PeekGet( sizeof(char), 0 );
	if ( !c )
		return NULL;

	// read quoted strings specially
	if ( *c == '\"' )
	{
		wasQuoted = true;
		buf.GetDelimitedString( m_bHasEscapeSequences ? GetCStringCharConversion() : GetNoEscCharConversion(), 
			s_pTokenBuf, KEYVALUES_TOKEN_SIZE );
		return s_pTokenBuf;
	}

	if ( *c == '{' || *c == '}' )
	{
		// it's a control char, just add this one char and stop reading
		s_pTokenBuf[0] = *c;
		s_pTokenBuf[1] = 0;
		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, 1 );
		return s_pTokenBuf;
	}

	// read in the token until we hit a whitespace or a control character
	bool bReportedError = false;
	bool bConditionalStart = false;
	int nCount = 0;
	while ( ( c = (const char*)buf.PeekGet( sizeof(char), 0 ) ) )
	{
		// end of file
		if ( *c == 0 )
			break;

		// break if any control character appears in non quoted tokens
		if ( *c == '"' || *c == '{' || *c == '}' )
			break;

		if ( *c == '[' )
			bConditionalStart = true;

		if ( *c == ']' && bConditionalStart )
		{
			wasConditional = true;
		}

		// break on whitespace
		if ( isspace(*c) )
			break;

		if (nCount < (KEYVALUES_TOKEN_SIZE-1) )
		{
			s_pTokenBuf[nCount++] = *c;	// add char to buffer
		}
		else if ( !bReportedError )
		{
			bReportedError = true;
			g_KeyValuesErrorStack.ReportError(" ReadToken overflow" );
		}

		buf.SeekGet( CUtlBuffer::SEEK_CURRENT, 1 );
	}
	s_pTokenBuf[ nCount ] = 0;
	return s_pTokenBuf;
}
#pragma warning (default:4706)

	

//-----------------------------------------------------------------------------
// Purpose: if parser should translate escape sequences ( /n, /t etc), set to true
//-----------------------------------------------------------------------------
void KeyValues::UsesEscapeSequences(bool state)
{
	m_bHasEscapeSequences = state;
}


//-----------------------------------------------------------------------------
// Purpose: if parser should evaluate conditional blocks ( [$WINDOWS] etc. )
//-----------------------------------------------------------------------------
void KeyValues::UsesConditionals(bool state)
{
	m_bEvaluateConditionals = state;
}


//-----------------------------------------------------------------------------
// Purpose: Load keyValues from disk
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID, bool refreshCache )
{
	TM_ZONE_DEFAULT( TELEMETRY_LEVEL0 );
	TM_ZONE_DEFAULT_PARAM( TELEMETRY_LEVEL0, resourceName );

	Assert(filesystem);
#ifdef WIN32
	Assert( IsX360() || ( IsPC() && _heapchk() == _HEAPOK ) );
#endif

	/*
	People are cheating with the keyvalue cache enabled by doing the below, so disable it.

	For example if one is to allow a blue demoman texture on sv_pure they
	change it to this, "$basetexture" "temp/demoman_blue". Remember to move the
	demoman texture to the temp folder in the materials folder. It will likely
	not be there so make a new folder for it. Once the directory in the
	demoman_blue vmt is changed to the temp folder and the vtf texture is in
	the temp folder itself you are finally done.

	I packed my mods into a vpk but I don't think it's required. Once in game
	you must create a server via the create server button and select the map
	that will load the custom texture before you join a valve server. I suggest
	you only do this with player textures and such as they are always loaded.
	After you load the map you join the valve server and the textures should
	appear and work on valve servers.

	This can be done on any sv_pure 1 server but it depends on what is type of
	files are allowed. All valve servers allow temp files so that is the
	example I used here."

	So all vmt's files can bypass sv_pure 1. And I believe this mod is mostly
	made of vmt files, so valve's sv_pure 1 bull is pretty redundant.
	*/
	const bool bUseCache = false;

	// If pathID is null, we cannot cache the result because that has a weird iterate-through-a-bunch-of-locations behavior.
	const bool bUseCacheForRead = bUseCache && !refreshCache && pathID != NULL; 
	const bool bUseCacheForWrite = bUseCache && pathID != NULL;

	COM_TimestampedLog( "KeyValues::LoadFromFile(%s%s%s): Begin", pathID ? pathID : "", pathID && resourceName ? "/" : "", resourceName ? resourceName : "" );

	// Keep a cache of keyvalues, try to load it here.
	if ( bUseCacheForRead && KeyValuesSystem()->LoadFileKeyValuesFromCache( this, resourceName, pathID, filesystem ) ) {
		COM_TimestampedLog( "KeyValues::LoadFromFile(%s%s%s): End / CacheHit", pathID ? pathID : "", pathID && resourceName ? "/" : "", resourceName ? resourceName : "" );
		return true;
	}

	FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);
	if ( !f )
	{
		COM_TimestampedLog("KeyValues::LoadFromFile(%s%s%s): End / FileNotFound", pathID ? pathID : "", pathID && resourceName ? "/" : "", resourceName ? resourceName : "");
		return false;
	}

	s_LastFileLoadingFrom = (char*)resourceName;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size( f );
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 2 );

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
	Assert( buffer );
	
	// read into local buffer
	bool bRetOK = ( ((IFileSystem *)filesystem)->ReadEx( buffer, bufSize, fileSize, f ) != 0 );

	filesystem->Close( f );	// close file after reading

	if ( bRetOK )
	{
		buffer[fileSize] = 0; // null terminate file as EOF
		buffer[fileSize+1] = 0; // double NULL terminating in case this is a unicode file
		bRetOK = LoadFromBuffer( resourceName, buffer, filesystem );
	}
	
	// The cache relies on the KeyValuesSystem string table, which will only be valid if we're
	// using classic mode. 
	if ( bUseCacheForWrite && bRetOK )
	{
		KeyValuesSystem()->AddFileKeyValuesToCache( this, resourceName, pathID );
	}

	( (IFileSystem *)filesystem )->FreeOptimalReadBuffer( buffer );

	COM_TimestampedLog("KeyValues::LoadFromFile(%s%s%s): End / Success", pathID ? pathID : "", pathID && resourceName ? "/" : "", resourceName ? resourceName : "");

	return bRetOK;
}

//-----------------------------------------------------------------------------
// Purpose: Save the keyvalues to disk
//			Creates the path to the file if it doesn't exist
//-----------------------------------------------------------------------------
bool KeyValues::SaveToFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID, bool sortKeys /*= false*/, bool bAllowEmptyString /*= false*/, bool bCacheResult /*= false*/ )
{
	// create a write file
	FileHandle_t f = filesystem->Open(resourceName, "wb", pathID);

	if ( f == FILESYSTEM_INVALID_HANDLE )
	{
		DevMsg(1, "KeyValues::SaveToFile: couldn't open file \"%s\" in path \"%s\".\n",
			resourceName?resourceName:"NULL", pathID?pathID:"NULL" );
		return false;
	}

	KeyValuesSystem()->InvalidateCacheForFile( resourceName, pathID );
	if ( bCacheResult ) {
		KeyValuesSystem()->AddFileKeyValuesToCache( this, resourceName, pathID );
	}
	RecursiveSaveToFile(filesystem, f, NULL, 0, sortKeys, bAllowEmptyString );
	filesystem->Close(f);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Write out a set of indenting
//-----------------------------------------------------------------------------
void KeyValues::WriteIndents( IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel )
{
	for ( int i = 0; i < indentLevel; i++ )
	{
		INTERNALWRITE( "\t", 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Write out a string where we convert the double quotes to backslash double quote
//-----------------------------------------------------------------------------
void KeyValues::WriteConvertedString( IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const char *pszString )
{
	// handle double quote chars within the string
	// the worst possible case is that the whole string is quotes
	int len = Q_strlen(pszString);
	char *convertedString = (char *) _alloca ((len + 1)  * sizeof(char) * 2);
	int j=0;
	for (int i=0; i <= len; i++)
	{
		if (pszString[i] == '\"')
		{
			convertedString[j] = '\\';
			j++;
		}
		else if ( m_bHasEscapeSequences && pszString[i] == '\\' )
		{
			convertedString[j] = '\\';
			j++;
		}
		convertedString[j] = pszString[i];
		j++;
	}		

	INTERNALWRITE(convertedString, Q_strlen(convertedString));
}


void KeyValues::InternalWrite( IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, const void *pData, int len )
{
	if ( filesystem )
	{
		filesystem->Write( pData, len, f );
	}

	if ( pBuf )
	{
		pBuf->Put( pData, len );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Save keyvalues from disk, if subkey values are detected, calls
//			itself to save those
//-----------------------------------------------------------------------------
void KeyValues::RecursiveSaveToFile( IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel, bool sortKeys, bool bAllowEmptyString )
{
	// write header
	WriteIndents( filesystem, f, pBuf, indentLevel );
	INTERNALWRITE("\"", 1);
	WriteConvertedString(filesystem, f, pBuf, GetName());	
	INTERNALWRITE("\"\n", 2);
	WriteIndents( filesystem, f, pBuf, indentLevel );
	INTERNALWRITE("{\n", 2);

	// loop through all our keys writing them to disk
	if ( sortKeys )
	{
		CUtlSortVector< KeyValues*, CUtlSortVectorKeyValuesByName > vecSortedKeys;

		for ( KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer )
		{
			vecSortedKeys.InsertNoSort(dat);
		}
		vecSortedKeys.RedoSort();
		
		FOR_EACH_VEC( vecSortedKeys, i )
		{
			SaveKeyToFile( vecSortedKeys[i], filesystem, f, pBuf, indentLevel, sortKeys, bAllowEmptyString );
		}
	}
	else
	{
		for ( KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer )
			SaveKeyToFile( dat, filesystem, f, pBuf, indentLevel, sortKeys, bAllowEmptyString );
	}

	// write tail
	WriteIndents(filesystem, f, pBuf, indentLevel);
	INTERNALWRITE("}\n", 2);
}

void KeyValues::SaveKeyToFile( KeyValues *dat, IBaseFileSystem *filesystem, FileHandle_t f, CUtlBuffer *pBuf, int indentLevel, bool sortKeys, bool bAllowEmptyString )
{
	if ( dat->m_pSub )
	{
		dat->RecursiveSaveToFile( filesystem, f, pBuf, indentLevel + 1, sortKeys, bAllowEmptyString );
	}
	else
	{
		// only write non-empty keys

		switch (dat->m_iDataType)
		{
		case TYPE_STRING:
			{
				if ( dat->m_sValue && ( bAllowEmptyString || *(dat->m_sValue) ) )
				{
					WriteIndents(filesystem, f, pBuf, indentLevel + 1);
					INTERNALWRITE("\"", 1);
					WriteConvertedString(filesystem, f, pBuf, dat->GetName());	
					INTERNALWRITE("\"\t\t\"", 4);

					WriteConvertedString(filesystem, f, pBuf, dat->m_sValue);	

					INTERNALWRITE("\"\n", 2);
				}
				break;
			}
		case TYPE_WSTRING:
			{
				if ( dat->m_wsValue )
				{
					static char buf[KEYVALUES_TOKEN_SIZE];
					// make sure we have enough space
					int result = Q_UnicodeToUTF8( dat->m_wsValue, buf, KEYVALUES_TOKEN_SIZE);
					if (result)
					{
						WriteIndents(filesystem, f, pBuf, indentLevel + 1);
						INTERNALWRITE("\"", 1);
						INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
						INTERNALWRITE("\"\t\t\"", 4);

						WriteConvertedString(filesystem, f, pBuf, buf);

						INTERNALWRITE("\"\n", 2);
					}
				}
				break;
			}

		case TYPE_INT:
			{
				WriteIndents(filesystem, f, pBuf, indentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[32];
				Q_snprintf(buf, sizeof( buf ), "%d", dat->m_iValue);

				INTERNALWRITE(buf, Q_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}

		case TYPE_UINT64:
			{
				WriteIndents(filesystem, f, pBuf, indentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[32];
				// write "0x" + 16 char 0-padded hex encoded 64 bit value
#ifdef WIN32
				Q_snprintf( buf, sizeof( buf ), "0x%016I64X", *( (uint64 *)dat->m_sValue ) );
#else
				Q_snprintf( buf, sizeof( buf ), "0x%016llX", *( (uint64 *)dat->m_sValue ) );
#endif

				INTERNALWRITE(buf, Q_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}

		case TYPE_FLOAT:
			{
				WriteIndents(filesystem, f, pBuf, indentLevel + 1);
				INTERNALWRITE("\"", 1);
				INTERNALWRITE(dat->GetName(), Q_strlen(dat->GetName()));
				INTERNALWRITE("\"\t\t\"", 4);

				char buf[48];
				Q_snprintf(buf, sizeof( buf ), "%f", dat->m_flValue);

				INTERNALWRITE(buf, Q_strlen(buf));
				INTERNALWRITE("\"\n", 2);
				break;
			}
		case TYPE_COLOR:
			DevMsg(1, "KeyValues::RecursiveSaveToFile: TODO, missing code for TYPE_COLOR.\n");
			break;

		default:
			break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: looks up a key by symbol name
//-----------------------------------------------------------------------------
KeyValues *KeyValues::FindKey(int keySymbol) const
{
	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		if (dat->m_iKeyName == keySymbol)
			return dat;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Find a keyValue, create it if it is not found.
//			Set bCreate to true to create the key if it doesn't already exist 
//			(which ensures a valid pointer will be returned)
//-----------------------------------------------------------------------------
KeyValues *KeyValues::FindKey(const char *keyName, bool bCreate)
{
	// return the current key if a NULL subkey is asked for
	if (!keyName || !keyName[0])
		return this;

	// look for '/' characters deliminating sub fields
	char szBuf[256] = { 0 };
	const char *subStr = strchr(keyName, '/');
	const char *searchStr = keyName;

	// pull out the substring if it exists
	if (subStr)
	{
		int size = Min( (int)(subStr - keyName + 1), (int)V_ARRAYSIZE( szBuf ) );
		V_strncpy( szBuf, keyName, size );
		searchStr = szBuf;
	}

	// lookup the symbol for the search string
	HKeySymbol iSearchStr = s_pfGetSymbolForString( searchStr, bCreate );

	if ( iSearchStr == INVALID_KEY_SYMBOL )
	{
		// not found, couldn't possibly be in key value list
		return NULL;
	}

	KeyValues *lastItem = NULL;
	KeyValues *dat;
	// find the searchStr in the current peer list
	for (dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		lastItem = dat;	// record the last item looked at (for if we need to append to the end of the list)

		// symbol compare
		if (dat->m_iKeyName == iSearchStr)
		{
			break;
		}
	}

	if ( !dat && m_pChain )
	{
		dat = m_pChain->FindKey(keyName, false);
	}

	// make sure a key was found
	if (!dat)
	{
		if (bCreate)
		{
			// we need to create a new key
			dat = new KeyValues( searchStr );
//			Assert(dat != NULL);

			dat->UsesEscapeSequences( m_bHasEscapeSequences != 0 );	// use same format as parent
			dat->UsesConditionals( m_bEvaluateConditionals != 0 );

			// insert new key at end of list
			if (lastItem)
			{
				lastItem->m_pPeer = dat;
			}
			else
			{
				m_pSub = dat;
			}
			dat->m_pPeer = NULL;

			// a key graduates to be a submsg as soon as it's m_pSub is set
			// this should be the only place m_pSub is set
			m_iDataType = TYPE_NONE;
		}
		else
		{
			return NULL;
		}
	}
	
	// if we've still got a subStr we need to keep looking deeper in the tree
	if ( subStr )
	{
		// recursively chain down through the paths in the string
		return dat->FindKey(subStr + 1, bCreate);
	}

	return dat;
}

//-----------------------------------------------------------------------------
// Purpose: Create a new key, with an autogenerated name.  
//			Name is guaranteed to be an integer, of value 1 higher than the highest 
//			other integer key name
//-----------------------------------------------------------------------------
KeyValues *KeyValues::CreateNewKey()
{
	int newID = 1;

	// search for any key with higher values
	KeyValues *pLastChild = NULL;
	for (KeyValues *dat = m_pSub; dat != NULL; dat = dat->m_pPeer)
	{
		// case-insensitive string compare
		int val = atoi(dat->GetName());
		if (newID <= val)
		{
			newID = val + 1;
		}

		pLastChild = dat;
	}

	char buf[12];
	Q_snprintf( buf, sizeof(buf), "%d", newID );

	return CreateKeyUsingKnownLastChild( buf, pLastChild );
}


//-----------------------------------------------------------------------------
// Create a key
//-----------------------------------------------------------------------------
KeyValues* KeyValues::CreateKey( const char *keyName )
{
	KeyValues *pLastChild = FindLastSubKey();
	return CreateKeyUsingKnownLastChild( keyName, pLastChild );
}

//-----------------------------------------------------------------------------
KeyValues* KeyValues::CreateKeyUsingKnownLastChild( const char *keyName, KeyValues *pLastChild )
{
	// Create a new key
	KeyValues* dat = new KeyValues( keyName );

	dat->UsesEscapeSequences( m_bHasEscapeSequences != 0 ); // use same format as parent does
	dat->UsesConditionals( m_bEvaluateConditionals != 0 );
	
	// add into subkey list
	AddSubkeyUsingKnownLastChild( dat, pLastChild );

	return dat;
}

//-----------------------------------------------------------------------------
void KeyValues::AddSubkeyUsingKnownLastChild( KeyValues *pSubkey, KeyValues *pLastChild )
{
	// Make sure the subkey isn't a child of some other keyvalues
	Assert( pSubkey != NULL );
	Assert( pSubkey->m_pPeer == NULL );

	// Empty child list?
	if ( pLastChild == NULL )
	{
		Assert( m_pSub == NULL );
		m_pSub = pSubkey;
	}
	else
	{
		Assert( m_pSub != NULL );
		Assert( pLastChild->m_pPeer == NULL );

//		// In debug, make sure that they really do know which child is the last one
//		#ifdef _DEBUG
//			KeyValues *pTempDat = m_pSub;
//			while ( pTempDat->GetNextKey() != NULL )
//			{
//				pTempDat = pTempDat->GetNextKey();
//			}
//			Assert( pTempDat == pLastChild );
//		#endif

		pLastChild->SetNextKey( pSubkey );
	}
}


//-----------------------------------------------------------------------------
// Adds a subkey. Make sure the subkey isn't a child of some other keyvalues
//-----------------------------------------------------------------------------
void KeyValues::AddSubKey( KeyValues *pSubkey )
{
	// Make sure the subkey isn't a child of some other keyvalues
	Assert( pSubkey != NULL );
	Assert( pSubkey->m_pPeer == NULL );

	// add into subkey list
	if ( m_pSub == NULL )
	{
		m_pSub = pSubkey;
	}
	else
	{
		KeyValues *pTempDat = m_pSub;
		while ( pTempDat->GetNextKey() != NULL )
		{
			pTempDat = pTempDat->GetNextKey();
		}

		pTempDat->SetNextKey( pSubkey );
	}
}


	
//-----------------------------------------------------------------------------
// Purpose: Remove a subkey from the list
//-----------------------------------------------------------------------------
void KeyValues::RemoveSubKey(KeyValues *subKey)
{
	if (!subKey)
		return;

	// check the list pointer
	if (m_pSub == subKey)
	{
		m_pSub = subKey->m_pPeer;
	}
	else
	{
		// look through the list
		KeyValues *kv = m_pSub;
		while (kv->m_pPeer)
		{
			if (kv->m_pPeer == subKey)
			{
				kv->m_pPeer = subKey->m_pPeer;
				break;
			}
			
			kv = kv->m_pPeer;
		}
	}

	subKey->m_pPeer = NULL;
}



//-----------------------------------------------------------------------------
// Purpose: Locate last child.  Returns NULL if we have no children
//-----------------------------------------------------------------------------
KeyValues *KeyValues::FindLastSubKey()
{

	// No children?
	if ( m_pSub == NULL )
		return NULL;

	// Scan for the last one
	KeyValues *pLastChild = m_pSub;
	while ( pLastChild->m_pPeer )
		pLastChild = pLastChild->m_pPeer;
	return pLastChild;
}

//-----------------------------------------------------------------------------
// Purpose: Sets this key's peer to the KeyValues passed in
//-----------------------------------------------------------------------------
void KeyValues::SetNextKey( KeyValues *pDat )
{
	m_pPeer = pDat;
}


KeyValues* KeyValues::GetFirstTrueSubKey()
{
	KeyValues *pRet = m_pSub;
	while ( pRet && pRet->m_iDataType != TYPE_NONE )
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetNextTrueSubKey()
{
	KeyValues *pRet = m_pPeer;
	while ( pRet && pRet->m_iDataType != TYPE_NONE )
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetFirstValue()
{
	KeyValues *pRet = m_pSub;
	while ( pRet && pRet->m_iDataType == TYPE_NONE )
		pRet = pRet->m_pPeer;

	return pRet;
}

KeyValues* KeyValues::GetNextValue()
{
	KeyValues *pRet = m_pPeer;
	while ( pRet && pRet->m_iDataType == TYPE_NONE )
		pRet = pRet->m_pPeer;

	return pRet;
}


//-----------------------------------------------------------------------------
// Purpose: Get the integer value of a keyName. Default value is returned
//			if the keyName can't be found.
//-----------------------------------------------------------------------------
int KeyValues::GetInt( const char *keyName, int defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_STRING:
			return atoi(dat->m_sValue);
		case TYPE_WSTRING:
			return _wtoi(dat->m_wsValue);
		case TYPE_FLOAT:
			return (int)dat->m_flValue;
		case TYPE_UINT64:
			// can't convert, since it would lose data
			Assert(0);
			return 0;
		case TYPE_INT:
		case TYPE_PTR:
		default:
			return dat->m_iValue;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the integer value of a keyName. Default value is returned
//			if the keyName can't be found.
//-----------------------------------------------------------------------------
uint64 KeyValues::GetUint64( const char *keyName, uint64 defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_STRING:
			return (uint64)Q_atoi64(dat->m_sValue);
		case TYPE_WSTRING:
			return _wtoi64(dat->m_wsValue);
		case TYPE_FLOAT:
			return (int)dat->m_flValue;
		case TYPE_UINT64:
			return *((uint64 *)dat->m_sValue);
		case TYPE_PTR:
			return (uint64)(uintp)dat->m_pValue;
		case TYPE_INT:
		default:
			return dat->m_iValue;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the pointer value of a keyName. Default value is returned
//			if the keyName can't be found.
//-----------------------------------------------------------------------------
void *KeyValues::GetPtr( const char *keyName, void *defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_PTR:
			return dat->m_pValue;

		case TYPE_WSTRING:
		case TYPE_STRING:
		case TYPE_FLOAT:
		case TYPE_INT:
		case TYPE_UINT64:
		default:
			return NULL;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the float value of a keyName. Default value is returned
//			if the keyName can't be found.
//-----------------------------------------------------------------------------
float KeyValues::GetFloat( const char *keyName, float defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		switch ( dat->m_iDataType )
		{
		case TYPE_STRING:
			return (float)atof(dat->m_sValue);
		case TYPE_WSTRING:
#ifdef WIN32
			return (float) _wtof(dat->m_wsValue);		// no wtof
#else
			Assert( !"impl me" );
			return 0.0;
#endif
			case TYPE_FLOAT:
			return dat->m_flValue;
		case TYPE_INT:
			return (float)dat->m_iValue;
		case TYPE_UINT64:
			return (float)(*((uint64 *)dat->m_sValue));
		case TYPE_PTR:
		default:
			return 0.0f;
		};
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get the string pointer of a keyName. Default value is returned
//			if the keyName can't be found.
//-----------------------------------------------------------------------------
const char *KeyValues::GetString( const char *keyName, const char *defaultValue )
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		// convert the data to string form then return it
		char buf[64];
		switch ( dat->m_iDataType )
		{
		case TYPE_FLOAT:
			Q_snprintf( buf, sizeof( buf ), "%f", dat->m_flValue );
			SetString( keyName, buf );
			break;
		case TYPE_PTR:
			V_snprintf( buf, sizeof( buf ), "%lld", CastPtrToInt64( dat->m_pValue ) );
			SetString( keyName, buf );
			break;
		case TYPE_INT:
			Q_snprintf( buf, sizeof( buf ), "%d", dat->m_iValue );
			SetString( keyName, buf );
			break;
		case TYPE_UINT64:
			Q_snprintf( buf, sizeof( buf ), "%lld", *((uint64 *)(dat->m_sValue)) );
			SetString( keyName, buf );
			break;

		case TYPE_WSTRING:
		{
			// convert the string to char *, set it for future use, and return it
			char wideBuf[512];
			int result = Q_UnicodeToUTF8(dat->m_wsValue, wideBuf, 512);
			if ( result )
			{
				// note: this will copy wideBuf
				SetString( keyName, wideBuf );
			}
			else
			{
				return defaultValue;
			}
			break;
		}
		case TYPE_STRING:
			break;
		default:
			return defaultValue;
		};
		
		return dat->m_sValue;
	}
	return defaultValue;
}


const wchar_t *KeyValues::GetWString( const char *keyName, const wchar_t *defaultValue)
{
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		wchar_t wbuf[64];
		switch ( dat->m_iDataType )
		{
		case TYPE_FLOAT:
			swprintf(wbuf, Q_ARRAYSIZE(wbuf), L"%f", dat->m_flValue);
			SetWString( keyName, wbuf);
			break;
		case TYPE_PTR:
			swprintf( wbuf, Q_ARRAYSIZE(wbuf), L"%lld", (int64)(size_t)dat->m_pValue );
			SetWString( keyName, wbuf );
			break;
		case TYPE_INT:
			swprintf( wbuf, Q_ARRAYSIZE(wbuf), L"%d", dat->m_iValue );
			SetWString( keyName, wbuf );
			break;
		case TYPE_UINT64:
			{
				swprintf( wbuf, Q_ARRAYSIZE(wbuf), L"%lld", *((uint64 *)(dat->m_sValue)) );
				SetWString( keyName, wbuf );
			}
			break;

		case TYPE_WSTRING:
			break;
		case TYPE_STRING:
		{
			int bufSize = Q_strlen(dat->m_sValue) + 1;
			wchar_t *pWBuf = new wchar_t[ bufSize ];
			int result = Q_UTF8ToUnicode(dat->m_sValue, pWBuf, bufSize * sizeof( wchar_t ) );
			if ( result >= 0 ) // may be a zero length string
			{
				SetWString( keyName, pWBuf);
			}
			else
			{
				delete [] pWBuf;
				return defaultValue;
			}
			delete [] pWBuf;
			break;
		}
		default:
			return defaultValue;
		};
		
		return (const wchar_t* )dat->m_wsValue;
	}
	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Get a bool interpretation of the key.
//-----------------------------------------------------------------------------
bool KeyValues::GetBool( const char *keyName, bool defaultValue, bool* optGotDefault )
{
	if ( FindKey( keyName ) )
    {
        if ( optGotDefault )
		{
            *optGotDefault = false;
		}

		return 0 != GetInt( keyName, 0 );
    }
    
    if ( optGotDefault )
	{
        *optGotDefault = true;
	}

	return defaultValue;
}

//-----------------------------------------------------------------------------
// Purpose: Gets a color
//-----------------------------------------------------------------------------
Color KeyValues::GetColor( const char *keyName )
{
	Color color(0, 0, 0, 0);
	KeyValues *dat = FindKey( keyName, false );
	if ( dat )
	{
		if ( dat->m_iDataType == TYPE_COLOR )
		{
			color[0] = dat->m_Color[0];
			color[1] = dat->m_Color[1];
			color[2] = dat->m_Color[2];
			color[3] = dat->m_Color[3];
		}
		else if ( dat->m_iDataType == TYPE_FLOAT )
		{
			color[0] = dat->m_flValue;
		}
		else if ( dat->m_iDataType == TYPE_INT )
		{
			color[0] = dat->m_iValue;
		}
		else if ( dat->m_iDataType == TYPE_STRING )
		{
			// parse the colors out of the string
			float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
			sscanf(dat->m_sValue, "%f %f %f %f", &a, &b, &c, &d);
			color[0] = (unsigned char)a;
			color[1] = (unsigned char)b;
			color[2] = (unsigned char)c;
			color[3] = (unsigned char)d;
		}
	}
	return color;
}

//-----------------------------------------------------------------------------
// Purpose: Sets a color
//-----------------------------------------------------------------------------
void KeyValues::SetColor( const char *keyName, Color value)
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_iDataType = TYPE_COLOR;
		dat->m_Color[0] = value[0];
		dat->m_Color[1] = value[1];
		dat->m_Color[2] = value[2];
		dat->m_Color[3] = value[3];
	}
}

void KeyValues::SetStringValue( char const *strValue )
{
	// delete the old value
	delete [] m_sValue;
	// make sure we're not storing the WSTRING  - as we're converting over to STRING
	delete [] m_wsValue;
	m_wsValue = NULL;

	if (!strValue)
	{
		// ensure a valid value
		strValue = "";
	}

	// allocate memory for the new value and copy it in
	int len = Q_strlen( strValue );
	m_sValue = new char[len + 1];
	Q_memcpy( m_sValue, strValue, len+1 );

	m_iDataType = TYPE_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetString( const char *keyName, const char *value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		if ( dat->m_iDataType == TYPE_STRING && dat->m_sValue == value )
		{
			return;
		}

		// delete the old value
		delete [] dat->m_sValue;
		// make sure we're not storing the WSTRING  - as we're converting over to STRING
		delete [] dat->m_wsValue;
		dat->m_wsValue = NULL;

		if (!value)
		{
			// ensure a valid value
			value = "";
		}

		// allocate memory for the new value and copy it in
		int len = Q_strlen( value );
		dat->m_sValue = new char[len + 1];
		Q_memcpy( dat->m_sValue, value, len+1 );

		dat->m_iDataType = TYPE_STRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the string value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetWString( const char *keyName, const wchar_t *value )
{
	KeyValues *dat = FindKey( keyName, true );
	if ( dat )
	{
		// delete the old value
		delete [] dat->m_wsValue;
		// make sure we're not storing the STRING  - as we're converting over to WSTRING
		delete [] dat->m_sValue;
		dat->m_sValue = NULL;

		if (!value)
		{
			// ensure a valid value
			value = L"";
		}

		// allocate memory for the new value and copy it in
		int len = Q_wcslen( value );
		dat->m_wsValue = new wchar_t[len + 1];
		Q_memcpy( dat->m_wsValue, value, (len+1) * sizeof(wchar_t) );

		dat->m_iDataType = TYPE_WSTRING;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetInt( const char *keyName, int value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_iValue = value;
		dat->m_iDataType = TYPE_INT;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the integer value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetUint64( const char *keyName, uint64 value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		// delete the old value
		delete [] dat->m_sValue;
		// make sure we're not storing the WSTRING  - as we're converting over to STRING
		delete [] dat->m_wsValue;
		dat->m_wsValue = NULL;

		dat->m_sValue = new char[sizeof(uint64)];
		*((uint64 *)dat->m_sValue) = value;
		dat->m_iDataType = TYPE_UINT64;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the float value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetFloat( const char *keyName, float value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_flValue = value;
		dat->m_iDataType = TYPE_FLOAT;
	}
}

void KeyValues::SetName( const char * setName )
{
	m_iKeyName = s_pfGetSymbolForString( setName, true );
}

//-----------------------------------------------------------------------------
// Purpose: Set the pointer value of a keyName. 
//-----------------------------------------------------------------------------
void KeyValues::SetPtr( const char *keyName, void *value )
{
	KeyValues *dat = FindKey( keyName, true );

	if ( dat )
	{
		dat->m_pValue = value;
		dat->m_iDataType = TYPE_PTR;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copies the tree from the other KeyValues into this one, recursively
// beginning with the root specified by rootSrc.
//-----------------------------------------------------------------------------
void KeyValues::CopyKeyValuesFromRecursive( const KeyValues& rootSrc )
{
	// This code used to be recursive, which was more elegant. Unfortunately, it also blew the stack for large 
	// KeyValues. So now we have the iterative version which is uglier but doesn't blow the stack.
	// This uses breadth-first traversal.

	struct CopyStruct
	{
		KeyValues* dst;
		const KeyValues* src;
	};

	char tmp[256];
	KeyValues* localDst = NULL;

	CUtlQueue<CopyStruct> nodeQ;
	nodeQ.Insert({ this, &rootSrc });

	while ( nodeQ.Count() > 0 ) 
	{
		CopyStruct cs = nodeQ.RemoveAtHead();

		// Process all the siblings of the current node. If anyone has a child, add it to the queue.
		while (cs.src)
		{
			Assert( (cs.src != NULL) == (cs.dst != NULL) );

			// Copy the node contents
			cs.dst->CopyKeyValue( *cs.src, sizeof(tmp), tmp );

			// Add children to the queue to process later. 
			if (cs.src->m_pSub) {
				cs.dst->m_pSub = localDst = new KeyValues( NULL );
				nodeQ.Insert({ localDst, cs.src->m_pSub });
			}

			// Process siblings until we hit the end of the line. 
			if (cs.src->m_pPeer) {
				cs.dst->m_pPeer = new KeyValues( NULL );
			}
			else {
				cs.dst->m_pPeer = NULL;
			}

			// Advance to the next peer.
			cs.src = cs.src->m_pPeer;
			cs.dst = cs.dst->m_pPeer;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Copies a single KeyValue from src to this, using the provided temporary
// buffer if the keytype requires it. Does NOT recurse.
//-----------------------------------------------------------------------------
void KeyValues::CopyKeyValue( const KeyValues& src, size_t tmpBufferSizeB, char* tmpBuffer )
{
	m_iKeyName = src.GetNameSymbol();

	if ( src.m_pSub )
		return;

	m_iDataType = src.m_iDataType;
		
	switch( src.m_iDataType )
	{
	case TYPE_NONE:
		break;
	case TYPE_STRING:
		if( src.m_sValue )
		{
			int len = Q_strlen(src.m_sValue) + 1;
			m_sValue = new char[len];
			Q_strncpy( m_sValue, src.m_sValue, len );
		}
		break;
	case TYPE_INT:
		{
			m_iValue = src.m_iValue;
			Q_snprintf( tmpBuffer, (int)tmpBufferSizeB, "%d", m_iValue );
			int len = Q_strlen(tmpBuffer) + 1;
			m_sValue = new char[len];
			Q_strncpy( m_sValue, tmpBuffer, len  );
		}
		break;
	case TYPE_FLOAT:
		{
			m_flValue = src.m_flValue;
			Q_snprintf( tmpBuffer, (int)tmpBufferSizeB, "%f", m_flValue );
			int len = Q_strlen(tmpBuffer) + 1;
			m_sValue = new char[len];
			Q_strncpy( m_sValue, tmpBuffer, len );
		}
		break;
	case TYPE_PTR:
		{
			m_pValue = src.m_pValue;
		}
		break;
	case TYPE_UINT64:
		{
			m_sValue = new char[sizeof(uint64)];
			Q_memcpy( m_sValue, src.m_sValue, sizeof(uint64) );
		}
		break;
	case TYPE_COLOR:
		{
			m_Color[0] = src.m_Color[0];
			m_Color[1] = src.m_Color[1];
			m_Color[2] = src.m_Color[2];
			m_Color[3] = src.m_Color[3];
		}
		break;
			
	default:
		{
			// do nothing . .what the heck is this?
			Assert( 0 );
		}
		break;
	}
}

KeyValues& KeyValues::operator=( const KeyValues& src )
{
	RemoveEverything();
	Init();	// reset all values
	CopyKeyValuesFromRecursive( src );
	return *this;
}


//-----------------------------------------------------------------------------
// Make a new copy of all subkeys, add them all to the passed-in keyvalues
//-----------------------------------------------------------------------------
void KeyValues::CopySubkeys( KeyValues *pParent ) const
{
	// recursively copy subkeys
	// Also maintain ordering....
	KeyValues *pPrev = NULL;
	for ( KeyValues *sub = m_pSub; sub != NULL; sub = sub->m_pPeer )
	{
		// take a copy of the subkey
		KeyValues *dat = sub->MakeCopy();
		 
		// add into subkey list
		if (pPrev)
		{
			pPrev->m_pPeer = dat;
		}
		else
		{
			pParent->m_pSub = dat;
		}
		dat->m_pPeer = NULL;
		pPrev = dat;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Makes a copy of the whole key-value pair set
//-----------------------------------------------------------------------------
KeyValues *KeyValues::MakeCopy( void ) const
{
	KeyValues *newKeyValue = new KeyValues(GetName());

	newKeyValue->UsesEscapeSequences( m_bHasEscapeSequences != 0 );
	newKeyValue->UsesConditionals( m_bEvaluateConditionals != 0 );

	// copy data
	newKeyValue->m_iDataType = m_iDataType;
	switch ( m_iDataType )
	{
	case TYPE_STRING:
		{
			if ( m_sValue )
			{
				int len = Q_strlen( m_sValue );
				Assert( !newKeyValue->m_sValue );
				newKeyValue->m_sValue = new char[len + 1];
				Q_memcpy( newKeyValue->m_sValue, m_sValue, len+1 );
			}
		}
		break;
	case TYPE_WSTRING:
		{
			if ( m_wsValue )
			{
				int len = Q_wcslen( m_wsValue );
				newKeyValue->m_wsValue = new wchar_t[len+1];
				Q_memcpy( newKeyValue->m_wsValue, m_wsValue, (len+1)*sizeof(wchar_t));
			}
		}
		break;

	case TYPE_INT:
		newKeyValue->m_iValue = m_iValue;
		break;

	case TYPE_FLOAT:
		newKeyValue->m_flValue = m_flValue;
		break;

	case TYPE_PTR:
		newKeyValue->m_pValue = m_pValue;
		break;
		
	case TYPE_COLOR:
		newKeyValue->m_Color[0] = m_Color[0];
		newKeyValue->m_Color[1] = m_Color[1];
		newKeyValue->m_Color[2] = m_Color[2];
		newKeyValue->m_Color[3] = m_Color[3];
		break;

	case TYPE_UINT64:
		newKeyValue->m_sValue = new char[sizeof(uint64)];
		Q_memcpy( newKeyValue->m_sValue, m_sValue, sizeof(uint64) );
		break;
	};

	// recursively copy subkeys
	CopySubkeys( newKeyValue );
	return newKeyValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *KeyValues::MakeCopy( bool copySiblings ) const
{
	KeyValues* rootDest = MakeCopy();
	if ( !copySiblings )
		return rootDest;

	const KeyValues* curSrc = GetNextKey();
	KeyValues* curDest = rootDest;
	while (curSrc) {
		curDest->SetNextKey( curSrc->MakeCopy() );
		curDest = curDest->GetNextKey();
		curSrc = curSrc->GetNextKey();
	}

	return rootDest;
}

//-----------------------------------------------------------------------------
// Purpose: Check if a keyName has no value assigned to it.
//-----------------------------------------------------------------------------
bool KeyValues::IsEmpty(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);
	if (!dat)
		return true;

	if (dat->m_iDataType == TYPE_NONE && dat->m_pSub == NULL)
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Clear out all subkeys, and the current value
//-----------------------------------------------------------------------------
void KeyValues::Clear( void )
{
	delete m_pSub;
	m_pSub = NULL;
	m_iDataType = TYPE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Get the data type of the value stored in a keyName
//-----------------------------------------------------------------------------
KeyValues::types_t KeyValues::GetDataType(const char *keyName)
{
	KeyValues *dat = FindKey(keyName, false);
	if (dat)
		return (types_t)dat->m_iDataType;

	return TYPE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: Deletion, ensures object gets deleted from correct heap
//-----------------------------------------------------------------------------
void KeyValues::deleteThis()
{
	delete this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : includedKeys - 
//-----------------------------------------------------------------------------
void KeyValues::AppendIncludedKeys( CUtlVector< KeyValues * >& includedKeys )
{
	// Append any included keys, too...
	KeyValues *insertSpot = this;
	int includeCount = includedKeys.Count();
	for ( int i = 0; i < includeCount; i++ )
	{
		KeyValues *kv = includedKeys[ i ];
		Assert( kv );

		while ( insertSpot->GetNextKey() )
		{
			insertSpot = insertSpot->GetNextKey();
		}

		insertSpot->SetNextKey( kv );
	}
}

void KeyValues::ParseIncludedKeys( char const *resourceName, const char *filetoinclude, 
		IBaseFileSystem* pFileSystem, const char *pPathID, CUtlVector< KeyValues * >& includedKeys )
{
	Assert( resourceName );
	Assert( filetoinclude );
	Assert( pFileSystem );
	
	// Load it...
	if ( !pFileSystem )
	{
		return;
	}

	// Get relative subdirectory
	char fullpath[ 512 ];
	Q_strncpy( fullpath, resourceName, sizeof( fullpath ) );

	// Strip off characters back to start or first /
	int len = Q_strlen( fullpath );
	for (;;)
	{
		if ( len <= 0 )
		{
			break;
		}
		
		if ( fullpath[ len - 1 ] == '\\' || 
			 fullpath[ len - 1 ] == '/' )
		{
			break;
		}

		// zero it
		fullpath[ len - 1 ] = 0;
		--len;
	}

	// Append included file
	Q_strncat( fullpath, filetoinclude, sizeof( fullpath ), COPY_ALL_CHARACTERS );

	KeyValues *newKV = new KeyValues( fullpath );

	// CUtlSymbol save = s_CurrentFileSymbol;	// did that had any use ???

	newKV->UsesEscapeSequences( m_bHasEscapeSequences != 0 );	// use same format as parent
	newKV->UsesConditionals( m_bEvaluateConditionals != 0 );

	if ( newKV->LoadFromFile( pFileSystem, fullpath, pPathID ) )
	{
		includedKeys.AddToTail( newKV );
	}
	else
	{
		DevMsg( "KeyValues::ParseIncludedKeys: Couldn't load included keyvalue file %s\n", fullpath );
		newKV->deleteThis();
	}

	// s_CurrentFileSymbol = save;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseKeys - 
//-----------------------------------------------------------------------------
void KeyValues::MergeBaseKeys( CUtlVector< KeyValues * >& baseKeys )
{
	int includeCount = baseKeys.Count();
	int i;
	for ( i = 0; i < includeCount; i++ )
	{
		KeyValues *kv = baseKeys[ i ];
		Assert( kv );

		RecursiveMergeKeyValues( kv );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : baseKV - keyvalues we're basing ourselves on
//-----------------------------------------------------------------------------
void KeyValues::RecursiveMergeKeyValues( KeyValues *baseKV )
{
	// Merge ourselves
	// we always want to keep our value, so nothing to do here

	// Now merge our children
	for ( KeyValues *baseChild = baseKV->m_pSub; baseChild != NULL; baseChild = baseChild->m_pPeer )
	{
		// for each child in base, see if we have a matching kv

		bool bFoundMatch = false;

		// If we have a child by the same name, merge those keys
		for ( KeyValues *newChild = m_pSub; newChild != NULL; newChild = newChild->m_pPeer )
		{
			if ( !Q_strcmp( baseChild->GetName(), newChild->GetName() ) )
			{
				newChild->RecursiveMergeKeyValues( baseChild );
				bFoundMatch = true;
				break;
			}	
		}

		// If not merged, append this key
		if ( !bFoundMatch )
		{
			KeyValues *dat = baseChild->MakeCopy();
			Assert( dat );
			AddSubKey( dat );
		}
	}
}

bool IsSteamDeck( bool bTrulyHardwareOnly )
{
	static int s_nSteamDeckCached = -1;
	static int s_nGamepadUICached = -1;

	if ( s_nGamepadUICached == -1 || s_nSteamDeckCached == -1 )
	{
		bool bIsDeck = false;
		bool bIsGamepadUI = false;

		if ( CommandLine()->CheckParm( "-nogamepadui" ) )
			bIsGamepadUI = false;
		else if ( CommandLine()->CheckParm( "-gamepadui" ) )
			bIsGamepadUI = true;
		else
		{
			const char *deckEnv = getenv( "SteamDeck" );
			bIsDeck = deckEnv && *deckEnv && atoi( deckEnv ) != 0;

			const char *bigPictureEnv = getenv( "SteamTenFoot" );
			bIsGamepadUI = bigPictureEnv && *bigPictureEnv && atoi( bigPictureEnv ) != 0;
		}

		s_nSteamDeckCached = bIsDeck ? 1 : 0;
		s_nGamepadUICached = bIsGamepadUI ? 1 : 0;
	}

	if ( bTrulyHardwareOnly )
		return s_nSteamDeckCached == 1;
	return s_nGamepadUICached == 1 || s_nSteamDeckCached == 1;
}

//-----------------------------------------------------------------------------
// Returns whether a keyvalues conditional evaluates to true or false
// Needs more flexibility with conditionals, checking convars would be nice.
//-----------------------------------------------------------------------------
bool EvaluateConditional( const char *str )
{
	if ( !str )
		return false;

	if ( *str == '[' )
		str++;

	bool bNot = false; // should we negate this command?
	if ( *str == '!' )
		bNot = true;

	if ( Q_stristr( str, "$DECK" ) )
		return IsSteamDeck() ^ bNot;

	if ( Q_stristr( str, "$X360" ) )
		return IsX360() ^ bNot;
	
	if ( Q_stristr( str, "$WIN32" ) )
		return IsPC() ^ bNot; // hack hack - for now WIN32 really means IsPC

	if ( Q_stristr( str, "$WINDOWS" ) )
		return IsWindows() ^ bNot;
	
	if ( Q_stristr( str, "$OSX" ) )
		return IsOSX() ^ bNot;
	
	if ( Q_stristr( str, "$LINUX" ) )
		return IsLinux() ^ bNot;

	if ( Q_stristr( str, "$POSIX" ) )
		return IsPosix() ^ bNot;
	
	return false;
}

// prevent two threads from entering this at the same time and trying to share the global error reporting and parse buffers
static CThreadFastMutex g_KVMutex;
//-----------------------------------------------------------------------------
// Read from a buffer...
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem* pFileSystem, const char *pPathID )
{
	AUTO_LOCK( g_KVMutex );
	KeyValues *pPreviousKey = NULL;
	KeyValues *pCurrentKey = this;
	CUtlVector< KeyValues * > includedKeys;
	CUtlVector< KeyValues * > baseKeys;
	bool wasQuoted;
	bool wasConditional;
	g_KeyValuesErrorStack.SetFilename( resourceName );	
	do 
	{
		bool bAccepted = true;

		// the first thing must be a key
		const char *s = ReadToken( buf, wasQuoted, wasConditional );
		if ( !buf.IsValid() || !s || *s == 0 )
			break;

		if ( !Q_stricmp( s, "#include" ) )	// special include macro (not a key name)
		{
			s = ReadToken( buf, wasQuoted, wasConditional );
			// Name of subfile to load is now in s

			if ( !s || *s == 0 )
			{
				g_KeyValuesErrorStack.ReportError("#include is NULL " );
			}
			else
			{
				ParseIncludedKeys( resourceName, s, pFileSystem, pPathID, includedKeys );
			}

			continue;
		}
		else if ( !Q_stricmp( s, "#base" ) )
		{
			s = ReadToken( buf, wasQuoted, wasConditional );
			// Name of subfile to load is now in s

			if ( !s || *s == 0 )
			{
				g_KeyValuesErrorStack.ReportError("#base is NULL " );
			}
			else
			{
				ParseIncludedKeys( resourceName, s, pFileSystem, pPathID, baseKeys );
			}

			continue;
		}

		if ( !pCurrentKey )
		{
			pCurrentKey = new KeyValues( s );
			Assert( pCurrentKey );

			pCurrentKey->UsesEscapeSequences( m_bHasEscapeSequences != 0 ); // same format has parent use
			pCurrentKey->UsesConditionals( m_bEvaluateConditionals != 0 );

			if ( pPreviousKey )
			{
				pPreviousKey->SetNextKey( pCurrentKey );
			}
		}
		else
		{
			pCurrentKey->SetName( s );
		}

		// get the '{'
		s = ReadToken( buf, wasQuoted, wasConditional );

		if ( wasConditional )
		{
			bAccepted = !m_bEvaluateConditionals || EvaluateConditional( s );

			// Now get the '{'
			s = ReadToken( buf, wasQuoted, wasConditional );
		}

		if ( s && *s == '{' && !wasQuoted )
		{
			// header is valid so load the file
			pCurrentKey->RecursiveLoadFromBuffer( resourceName, buf );
		}
		else
		{
			g_KeyValuesErrorStack.ReportError("LoadFromBuffer: missing {" );
		}

		if ( !bAccepted )
		{
			if ( pPreviousKey )
			{
				pPreviousKey->SetNextKey( NULL );
			}
			pCurrentKey->Clear();
		}
		else
		{
			pPreviousKey = pCurrentKey;
			pCurrentKey = NULL;
		}
	} while ( buf.IsValid() );

	AppendIncludedKeys( includedKeys );
	{
		// delete included keys!
		int i;
		for ( i = includedKeys.Count() - 1; i > 0; i-- )
		{
			KeyValues *kv = includedKeys[ i ];
			kv->deleteThis();
		}
	}

	MergeBaseKeys( baseKeys );
	{
		// delete base keys!
		int i;
		for ( i = baseKeys.Count() - 1; i >= 0; i-- )
		{
			KeyValues *kv = baseKeys[ i ];
			kv->deleteThis();
		}
	}

	g_KeyValuesErrorStack.SetFilename( "" );	

	return true;
}


//-----------------------------------------------------------------------------
// Read from a buffer...
//-----------------------------------------------------------------------------
bool KeyValues::LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem, const char *pPathID )
{
	if ( !pBuffer )
		return true;

	COM_TimestampedLog("KeyValues::LoadFromBuffer(%s%s%s): Begin", pPathID ? pPathID : "", pPathID && resourceName ? "/" : "", resourceName ? resourceName : "");

	int nLen = Q_strlen( pBuffer );
	CUtlBuffer buf( pBuffer, nLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );

	// Translate Unicode files into UTF-8 before proceeding
	if ( nLen > 2 && (uint8)pBuffer[0] == 0xFF && (uint8)pBuffer[1] == 0xFE )
	{
		int nUTF8Len = V_UnicodeToUTF8( (wchar_t*)(pBuffer+2), NULL, 0 );
		char *pUTF8Buf = new char[nUTF8Len];
		V_UnicodeToUTF8( (wchar_t*)(pBuffer+2), pUTF8Buf, nUTF8Len );
		buf.AssumeMemory( pUTF8Buf, nUTF8Len, nUTF8Len, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );
	}

	bool retVal = LoadFromBuffer( resourceName, buf, pFileSystem, pPathID );

	COM_TimestampedLog("KeyValues::LoadFromBuffer(%s%s%s): End", pPathID ? pPathID : "", pPathID && resourceName ? "/" : "", resourceName ? resourceName : "");

	return retVal;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void KeyValues::RecursiveLoadFromBuffer( char const *resourceName, CUtlBuffer &buf )
{
	CKeyErrorContext errorReport(this);
	bool wasQuoted;
	bool wasConditional;
	if ( errorReport.GetStackLevel() > 100 )
	{
		g_KeyValuesErrorStack.ReportError( "RecursiveLoadFromBuffer:  recursion overflow" );
		return;
	}

	// keep this out of the stack until a key is parsed
	CKeyErrorContext errorKey( INVALID_KEY_SYMBOL );

	// Locate the last child.  (Almost always, we will not have any children.)
	// We maintain the pointer to the last child here, so we don't have to re-locate
	// it each time we append the next subkey, which causes O(N^2) time
	KeyValues *pLastChild = FindLastSubKey();;

	// Keep parsing until we hit the closing brace which terminates this block, or a parse error
	while ( 1 )
	{
		bool bAccepted = true;

		// get the key name
		const char * name = ReadToken( buf, wasQuoted, wasConditional );

		if ( !name )	// EOF stop reading
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got EOF instead of keyname" );
			break;
		}

		if ( !*name ) // empty token, maybe "" or EOF
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got empty keyname" );
			break;
		}

		if ( *name == '}' && !wasQuoted )	// top level closed, stop reading
			break;

		// Always create the key; note that this could potentially
		// cause some duplication, but that's what we want sometimes
		KeyValues *dat = CreateKeyUsingKnownLastChild( name, pLastChild );

		errorKey.Reset( dat->GetNameSymbol() );

		// get the value
		const char * value = ReadToken( buf, wasQuoted, wasConditional );

		if ( wasConditional && value )
		{
			bAccepted = !m_bEvaluateConditionals || EvaluateConditional( value );

			// get the real value
			value = ReadToken( buf, wasQuoted, wasConditional );
		}

		if ( !value )
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got NULL key" );
			break;
		}
		
		if ( *value == '}' && !wasQuoted )
		{
			g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got } in key" );
			break;
		}

		if ( *value == '{' && !wasQuoted )
		{
			// this isn't a key, it's a section
			errorKey.Reset( INVALID_KEY_SYMBOL );
			// sub value list
			dat->RecursiveLoadFromBuffer( resourceName, buf );
		}
		else 
		{
			if ( wasConditional )
			{
				g_KeyValuesErrorStack.ReportError("RecursiveLoadFromBuffer:  got conditional between key and value" );
				break;
			}
			
			if (dat->m_sValue)
			{
				delete[] dat->m_sValue;
				dat->m_sValue = NULL;
			}

			int len = Q_strlen( value );

			// Here, let's determine if we got a float or an int....
			char* pIEnd;	// pos where int scan ended
			char* pFEnd;	// pos where float scan ended
			const char* pSEnd = value + len ; // pos where token ends

			int ival = strtol( value, &pIEnd, 10 );
			float fval = (float)strtod( value, &pFEnd );
			bool bOverflow = ( ival == LONG_MAX || ival == LONG_MIN ) && errno == ERANGE;
#ifdef POSIX
			// strtod supports hex representation in strings under posix but we DON'T
			// want that support in keyvalues, so undo it here if needed
			if ( len > 1 &&  tolower(value[1]) == 'x' )
			{
				fval = 0.0f;
				pFEnd = (char *)value;
			}
#endif
				
			if ( *value == 0 )
			{
				dat->m_iDataType = TYPE_STRING;	
			}
			else if ( ( 18 == len ) && ( value[0] == '0' ) && ( value[1] == 'x' ) )
			{
				// an 18-byte value prefixed with "0x" (followed by 16 hex digits) is an int64 value
				int64 retVal = 0;
				for( int i=2; i < 2 + 16; i++ )
				{
					char digit = value[i];
					if ( digit >= 'a' ) 
						digit -= 'a' - ( '9' + 1 );
					else
						if ( digit >= 'A' )
							digit -= 'A' - ( '9' + 1 );
					retVal = ( retVal * 16 ) + ( digit - '0' );
				}
				dat->m_sValue = new char[sizeof(uint64)];
				*((uint64 *)dat->m_sValue) = retVal;
				dat->m_iDataType = TYPE_UINT64;
			}
			else if ( (pFEnd > pIEnd) && (pFEnd == pSEnd) )
			{
				dat->m_flValue = fval; 
				dat->m_iDataType = TYPE_FLOAT;
			}
			else if (pIEnd == pSEnd && !bOverflow)
			{
				dat->m_iValue = ival; 
				dat->m_iDataType = TYPE_INT;
			}
			else
			{
				dat->m_iDataType = TYPE_STRING;
			}

			if (dat->m_iDataType == TYPE_STRING)
			{
				// copy in the string information
				dat->m_sValue = new char[len+1];
				Q_memcpy( dat->m_sValue, value, len+1 );
			}

			// Look ahead one token for a conditional tag
			int prevPos = buf.TellGet();
			const char *peek = ReadToken( buf, wasQuoted, wasConditional );
			if ( wasConditional )
			{
				bAccepted = !m_bEvaluateConditionals || EvaluateConditional( peek );
			}
			else
			{
				buf.SeekGet( CUtlBuffer::SEEK_HEAD, prevPos );
			}
		}

		Assert( dat->m_pPeer == NULL );
		if ( bAccepted )
		{
			Assert( pLastChild == NULL || pLastChild->m_pPeer == dat );
			pLastChild = dat;
		}
		else
		{
			//this->RemoveSubKey( dat );
			if ( pLastChild == NULL )
			{
				Assert( m_pSub == dat );
				m_pSub = NULL;
			}
			else
			{
				Assert( pLastChild->m_pPeer == dat );
				pLastChild->m_pPeer = NULL;
			}

			dat->deleteThis();
			dat = NULL;
		}
	}
}



// writes KeyValue as binary data to buffer
bool KeyValues::WriteAsBinary( CUtlBuffer &buffer )
{
	if ( buffer.IsText() ) // must be a binary buffer
		return false;

	if ( !buffer.IsValid() ) // must be valid, no overflows etc
		return false;

	// Write subkeys:
	
	// loop through all our peers
	for ( KeyValues *dat = this; dat != NULL; dat = dat->m_pPeer )
	{
		// write type
		buffer.PutUnsignedChar( dat->m_iDataType );

		// write name
		buffer.PutString( dat->GetName() );

		// write type
		switch (dat->m_iDataType)
		{
		case TYPE_NONE:
			{
				dat->m_pSub->WriteAsBinary( buffer );
				break;
			}
		case TYPE_STRING:
			{
				if (dat->m_sValue && *(dat->m_sValue))
				{
					buffer.PutString( dat->m_sValue );
				}
				else
				{
					buffer.PutString( "" );
				}
				break;
			}
		case TYPE_WSTRING:
			{
				Assert( !"TYPE_WSTRING" );
				break;
			}

		case TYPE_INT:
			{
				buffer.PutInt( dat->m_iValue );				
				break;
			}

		case TYPE_UINT64:
			{
				buffer.PutDouble( *((double *)dat->m_sValue) );
				break;
			}

		case TYPE_FLOAT:
			{
				buffer.PutFloat( dat->m_flValue );
				break;
			}
		case TYPE_COLOR:
			{
				buffer.PutUnsignedChar( dat->m_Color[0] );
				buffer.PutUnsignedChar( dat->m_Color[1] );
				buffer.PutUnsignedChar( dat->m_Color[2] );
				buffer.PutUnsignedChar( dat->m_Color[3] );
				break;
			}
		case TYPE_PTR:
			{
#if defined( PLATFORM_64BITS )
				// We only put an int here, because 32-bit clients do not expect 64 bits. It'll cause them to read the wrong
				// amount of data and then crash. Longer term, we may bump this up in size on all platforms, but short term 
				// we don't really have much of a choice other than sticking in something that appears to not be NULL.
				if ( dat->m_pValue != 0 && ( ( (int)(intp)dat->m_pValue ) == 0 ) )
					buffer.PutInt( 31337 ); // Put not 0, but not a valid number. Yuck.
				else
					buffer.PutInt( ( (int)(intp)dat->m_pValue ) );
#else
				buffer.PutPtr( dat->m_pValue );
#endif
				break;
			}

		default:
			break;
		}
	}

	// write tail, marks end of peers
	buffer.PutUnsignedChar( TYPE_NUMTYPES ); 

	return buffer.IsValid();
}

// read KeyValues from binary buffer, returns true if parsing was successful
bool KeyValues::ReadAsBinary( CUtlBuffer &buffer, int nStackDepth )
{
	if ( buffer.IsText() ) // must be a binary buffer
		return false;

	if ( !buffer.IsValid() ) // must be valid, no overflows etc
		return false;

	RemoveEverything(); // remove current content
	Init();	// reset
	
	if ( nStackDepth > 100 )
	{
		AssertMsgOnce( false, "KeyValues::ReadAsBinary() stack depth > 100\n" );
		return false;
	}

	KeyValues	*dat = this;
	types_t		type = (types_t)buffer.GetUnsignedChar();
	
	// loop through all our peers
	while ( true )
	{
		if ( type == TYPE_NUMTYPES )
			break; // no more peers

		dat->m_iDataType = type;

		{
			char token[KEYVALUES_TOKEN_SIZE];
			buffer.GetString( token );
			token[KEYVALUES_TOKEN_SIZE-1] = 0;
			dat->SetName( token );
		}

		switch ( type )
		{
		case TYPE_NONE:
			{
				dat->m_pSub = new KeyValues("");
				if ( !dat->m_pSub->ReadAsBinary( buffer, nStackDepth + 1 ) )
					return false;
				break;
			}
		case TYPE_STRING:
			{
				char token[KEYVALUES_TOKEN_SIZE];
				buffer.GetString( token );
				token[KEYVALUES_TOKEN_SIZE-1] = 0;

				int len = Q_strlen( token );
				dat->m_sValue = new char[len + 1];
				Q_memcpy( dat->m_sValue, token, len+1 );
								
				break;
			}
		case TYPE_WSTRING:
			{
				Assert( !"TYPE_WSTRING" ); // !! MERGE WARNING: Other branches were found to have security issues here, use caution if taking this from another branch (CS:GO known fixed)
				break;
			}

		case TYPE_INT:
			{
				dat->m_iValue = buffer.GetInt();
				break;
			}

		case TYPE_UINT64:
			{
				dat->m_sValue = new char[sizeof(uint64)];
				*((uint64 *)dat->m_sValue) = buffer.GetInt64();
				break;
			}

		case TYPE_FLOAT:
			{
				dat->m_flValue = buffer.GetFloat();
				break;
			}
		case TYPE_COLOR:
			{
				dat->m_Color[0] = buffer.GetUnsignedChar();
				dat->m_Color[1] = buffer.GetUnsignedChar();
				dat->m_Color[2] = buffer.GetUnsignedChar();
				dat->m_Color[3] = buffer.GetUnsignedChar();
				break;
			}
		case TYPE_PTR:
			{
#if defined( PLATFORM_64BITS )
				// We need to ensure we only read 32 bits out of the stream because 32 bit clients only wrote 
				// 32 bits of data there. The actual pointer is irrelevant, all that we really care about here
				// contractually is whether the pointer is zero or not zero.
				dat->m_pValue = ( void* )( intp )buffer.GetInt();
#else
				dat->m_pValue = buffer.GetPtr();
#endif
			}

		default:
			break;
		}

		if ( !buffer.IsValid() ) // error occured
			return false;

		type = (types_t)buffer.GetUnsignedChar();

		if ( type == TYPE_NUMTYPES )
			break;

		// new peer follows
		dat->m_pPeer = new KeyValues("");
		dat = dat->m_pPeer;
	}

	return buffer.IsValid();
}

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// Purpose: memory allocator
//-----------------------------------------------------------------------------
void *KeyValues::operator new( size_t iAllocSize )
{
	MEM_ALLOC_CREDIT();
	return KeyValuesSystem()->AllocKeyValuesMemory( (int)iAllocSize );
}

void *KeyValues::operator new( size_t iAllocSize, int nBlockUse, const char *pFileName, int nLine )
{
	MemAlloc_PushAllocDbgInfo( pFileName, nLine );
	void *p = KeyValuesSystem()->AllocKeyValuesMemory( (int)iAllocSize );
	MemAlloc_PopAllocDbgInfo();
	return p;
}

//-----------------------------------------------------------------------------
// Purpose: deallocator
//-----------------------------------------------------------------------------
void KeyValues::operator delete( void *pMem )
{
	KeyValuesSystem()->FreeKeyValuesMemory(pMem);
}

void KeyValues::operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine )
{
	KeyValuesSystem()->FreeKeyValuesMemory(pMem);
}

void KeyValues::UnpackIntoStructure( KeyValuesUnpackStructure const *pUnpackTable, void *pDest, size_t DestSizeInBytes )
{
#ifdef DBGFLAG_ASSERT
	void *pDestEnd = ( char * )pDest + DestSizeInBytes + 1;
#endif

	uint8 *dest=(uint8 *) pDest;
	while( pUnpackTable->m_pKeyName )
	{
		uint8 *dest_field=dest+pUnpackTable->m_nFieldOffset;
		KeyValues *find_it=FindKey( pUnpackTable->m_pKeyName );

		switch( pUnpackTable->m_eDataType )
		{
			case UNPACK_TYPE_FLOAT:
			{
				Assert( dest_field + sizeof( float ) < pDestEnd );

				float default_value=(pUnpackTable->m_pKeyDefault)?atof(pUnpackTable->m_pKeyDefault):0.0;
				*( ( float *) dest_field)=GetFloat( pUnpackTable->m_pKeyName, default_value );
				break;
			}
			break;

			case UNPACK_TYPE_VECTOR:
			{
				Assert( dest_field + sizeof( Vector ) < pDestEnd );

				Vector *dest_v=(Vector *) dest_field;
				char const *src_string=
					GetString( pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault );
				if ( (!src_string) ||
					 ( sscanf(src_string,"%f %f %f",
							  &(dest_v->x), &(dest_v->y), &(dest_v->z)) != 3))
					dest_v->Init( 0, 0, 0 );
			}
			break;

			case UNPACK_TYPE_FOUR_FLOATS:
			{
				Assert( dest_field + sizeof( float ) * 4 < pDestEnd );

				float *dest_f=(float *) dest_field;
				char const *src_string=
					GetString( pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault );
				if ( (!src_string) ||
					 ( sscanf(src_string,"%f %f %f %f",
							  dest_f,dest_f+1,dest_f+2,dest_f+3)) != 4)
					memset( dest_f, 0, 4*sizeof(float) );
			}
			break;

			case UNPACK_TYPE_TWO_FLOATS:
			{
				Assert( dest_field + sizeof( float ) * 2 < pDestEnd );

				float *dest_f=(float *) dest_field;
				char const *src_string=
					GetString( pUnpackTable->m_pKeyName, pUnpackTable->m_pKeyDefault );
				if ( (!src_string) ||
					 ( sscanf(src_string,"%f %f",
							  dest_f,dest_f+1)) != 2)
					memset( dest_f, 0, 2*sizeof(float) );
			}
			break;

			case UNPACK_TYPE_STRING:
			{
				Assert( dest_field + pUnpackTable->m_nFieldSize < pDestEnd );

				char *dest_s=(char *) dest_field;
				strncpy( dest_s, GetString( pUnpackTable->m_pKeyName,
											pUnpackTable->m_pKeyDefault ),
						 pUnpackTable->m_nFieldSize );

			}
			break;

			case UNPACK_TYPE_INT:
			{
				Assert( dest_field + sizeof( int ) < pDestEnd );

				int *dest_i=(int *) dest_field;
				int default_int=0;
				if ( pUnpackTable->m_pKeyDefault)
					default_int = atoi( pUnpackTable->m_pKeyDefault );
				*(dest_i)=GetInt( pUnpackTable->m_pKeyName, default_int );
			}
			break;

			case UNPACK_TYPE_VECTOR_COLOR:
			{
				Assert( dest_field + sizeof( Vector ) < pDestEnd );

				Vector *dest_v=(Vector *) dest_field;
				if (find_it)
				{
					Color c=GetColor( pUnpackTable->m_pKeyName );
					dest_v->x = c.r();
					dest_v->y = c.g();
					dest_v->z = c.b();
				}
				else
				{
					if ( pUnpackTable->m_pKeyDefault )
						sscanf(pUnpackTable->m_pKeyDefault,"%f %f %f",
							   &(dest_v->x), &(dest_v->y), &(dest_v->z));
					else
						dest_v->Init( 0, 0, 0 );
				}
				*(dest_v) *= (1.0/255);
			}
		}
		pUnpackTable++;
	}
}

//-----------------------------------------------------------------------------
// Helper function for processing a keyvalue tree for console resolution support.
// Alters key/values for easier console video resolution support. 
// If running SD (640x480), the presence of "???_lodef" creates or slams "???".
// If running HD (1280x720), the presence of "???_hidef" creates or slams "???".
//-----------------------------------------------------------------------------
bool KeyValues::ProcessResolutionKeys( const char *pResString )
{	
	if ( !pResString )
	{
		// not for pc, console only
		return false;
	}

	KeyValues *pSubKey = GetFirstSubKey();
	if ( !pSubKey )
	{
		// not a block
		return false;
	}

	for ( ; pSubKey != NULL; pSubKey = pSubKey->GetNextKey() )
	{
		// recursively descend each sub block
		pSubKey->ProcessResolutionKeys( pResString );

		// check to see if our substring is present
		if ( Q_stristr( pSubKey->GetName(), pResString ) != NULL )
		{
			char normalKeyName[128];
			V_strncpy( normalKeyName, pSubKey->GetName(), sizeof( normalKeyName ) );

			// substring must match exactly, otherwise keys like "_lodef" and "_lodef_wide" would clash.
			char *pString = Q_stristr( normalKeyName, pResString );
			if ( pString && !Q_stricmp( pString, pResString ) )
			{
				*pString = '\0';

				// find and delete the original key (if any)
				KeyValues *pKey = FindKey( normalKeyName );
				if ( pKey )
				{		
					// remove the key
					RemoveSubKey( pKey );
				}

				// rename the marked key
				pSubKey->SetName( normalKeyName );
			}
		}
	}

	return true;
}



//
// KeyValues dumping implementation
//
bool KeyValues::Dump( IKeyValuesDumpContext *pDump, int nIndentLevel /* = 0 */,  bool bSorted /*= false*/ )
{
	if ( !pDump->KvBeginKey( this, nIndentLevel ) )
		return false;

	if ( bSorted )
	{
		CUtlSortVector< KeyValues*, CUtlSortVectorKeyValuesByName > vecSortedKeys;
	
		// Dump values
		for ( KeyValues *val = this ? GetFirstValue() : NULL; val; val = val->GetNextValue() )
		{
			vecSortedKeys.InsertNoSort( val );
		}
		vecSortedKeys.RedoSort();

		FOR_EACH_VEC( vecSortedKeys, i )
		{
			if ( !pDump->KvWriteValue( vecSortedKeys[i], nIndentLevel + 1 ) )
				return false;
		}
		
		vecSortedKeys.Purge();

		// Dump subkeys
		for ( KeyValues *sub = this ? GetFirstTrueSubKey() : NULL; sub; sub = sub->GetNextTrueSubKey() )
		{
			vecSortedKeys.InsertNoSort( sub );
		}
		vecSortedKeys.RedoSort();

		FOR_EACH_VEC( vecSortedKeys, i )
		{
			if ( !vecSortedKeys[i]->Dump( pDump, nIndentLevel + 1, bSorted ) )
				return false;
		}
	}
	else
	{
		// Dump values
		for ( KeyValues *val = this ? GetFirstValue() : NULL; val; val = val->GetNextValue() )
		{
			if ( !pDump->KvWriteValue( val, nIndentLevel + 1 ) )
				return false;
		}

		// Dump subkeys
		for ( KeyValues *sub = this ? GetFirstTrueSubKey() : NULL; sub; sub = sub->GetNextTrueSubKey() )
		{
			if ( !sub->Dump( pDump, nIndentLevel + 1 ) )
				return false;
		}
	}

	return pDump->KvEndKey( this, nIndentLevel );
}

bool IKeyValuesDumpContextAsText::KvBeginKey( KeyValues *pKey, int nIndentLevel )
{
	if ( pKey )
	{
		return
			KvWriteIndent( nIndentLevel ) &&
			KvWriteText( pKey->GetName() ) &&
			KvWriteText( "\n" ) &&
			KvWriteIndent( nIndentLevel ) &&
			KvWriteText( "{\n" );
	}
	else
	{
		return
			KvWriteIndent( nIndentLevel ) &&
			KvWriteText( "<< NULL >>\n" );
	}
}

bool IKeyValuesDumpContextAsText::KvWriteValue( KeyValues *val, int nIndentLevel )
{
	if ( !val )
	{
		return
			KvWriteIndent( nIndentLevel ) &&
			KvWriteText( "<< NULL >>\n" );
	}

	if ( !KvWriteIndent( nIndentLevel ) )
		return false;

	if ( !KvWriteText( val->GetName() ) )
		return false;

	if ( !KvWriteText( " " ) )
		return false;

	switch ( val->GetDataType() )
	{
	case KeyValues::TYPE_STRING:
		{
			if ( !KvWriteText( val->GetString() ) )
				return false;
		}
		break;

	case KeyValues::TYPE_INT:
		{
			int n = val->GetInt();
			char *chBuffer = ( char * ) stackalloc( 128 );
			V_snprintf( chBuffer, 128, "int( %d = 0x%X )", n, n );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;
	
	case KeyValues::TYPE_FLOAT:
		{
			float fl = val->GetFloat();
			char *chBuffer = ( char * ) stackalloc( 128 );
			V_snprintf( chBuffer, 128, "float( %f )", fl );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;

	case KeyValues::TYPE_PTR:
		{
			void *ptr = val->GetPtr();
			char *chBuffer = ( char * ) stackalloc( 128 );
			V_snprintf( chBuffer, 128, "ptr( 0x%p )", ptr );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;

	case KeyValues::TYPE_WSTRING:
		{
			wchar_t const *wsz = val->GetWString();
			int nLen = V_wcslen( wsz );
			int numBytes = nLen*2 + 64;
			char *chBuffer = ( char * ) stackalloc( numBytes );
			V_snprintf( chBuffer, numBytes, "%ls [wstring, len = %d]", wsz, nLen );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;

	case KeyValues::TYPE_UINT64:
		{
			uint64 n = val->GetUint64();
			char *chBuffer = ( char * ) stackalloc( 128 );
			V_snprintf( chBuffer, 128, "u64( %lld = 0x%llX )", n, n );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;

	default:
		break;
		{
			int n = val->GetDataType();
			char *chBuffer = ( char * ) stackalloc( 128 );
			V_snprintf( chBuffer, 128, "??kvtype[%d]", n );
			if ( !KvWriteText( chBuffer ) )
				return false;
		}
		break;
	}

	return KvWriteText( "\n" );
}

bool IKeyValuesDumpContextAsText::KvEndKey( KeyValues *pKey, int nIndentLevel )
{
	if ( pKey )
	{
		return
			KvWriteIndent( nIndentLevel ) &&
			KvWriteText( "}\n" );
	}
	else
	{
		return true;
	}
}

bool IKeyValuesDumpContextAsText::KvWriteIndent( int nIndentLevel )
{
	int numIndentBytes = ( nIndentLevel * 2 + 1 );
	char *pchIndent = ( char * ) stackalloc( numIndentBytes );
	memset( pchIndent, ' ', numIndentBytes - 1 );
	pchIndent[ numIndentBytes - 1 ] = 0;
	return KvWriteText( pchIndent );
}


bool CKeyValuesDumpContextAsDevMsg::KvBeginKey( KeyValues *pKey, int nIndentLevel )
{
	static ConVarRef r_developer( "developer" );
	if ( r_developer.IsValid() && r_developer.GetInt() < m_nDeveloperLevel )
		// If "developer" is not the correct level, then avoid evaluating KeyValues tree early
		return false;
	else
		return IKeyValuesDumpContextAsText::KvBeginKey( pKey, nIndentLevel );
}

bool CKeyValuesDumpContextAsDevMsg::KvWriteText( char const *szText )
{
	if ( m_nDeveloperLevel > 0 )
	{
		DevMsg( m_nDeveloperLevel, "%s", szText );
	}
	else
	{
		Msg( "%s", szText );
	}
	return true;
}
