//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#pragma warning (disable:4514)

#include "utlsymbol.h"
#include "KeyValues.h"
#include "tier0/threadtools.h"
#include "tier0/memdbgon.h"
#include "stringpool.h"
#include "utlhashtable.h"
#include "utlstring.h"

// Ensure that everybody has the right compiler version installed. The version
// number can be obtained by looking at the compiler output when you type 'cl'
// and removing the last two digits and the periods: 16.00.40219.01 becomes 160040219
#ifdef _MSC_FULL_VER
	#if _MSC_FULL_VER > 160000000
		// VS 2010
		#if _MSC_FULL_VER < 160040219
			#error You must install VS 2010 SP1
		#endif
	#else
		// VS 2005
		#if _MSC_FULL_VER < 140050727
			#error You must install VS 2005 SP1
		#endif
	#endif
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INVALID_STRING_INDEX CStringPoolIndex( 0xFFFF, 0xFFFF )

#define MIN_STRING_POOL_SIZE	2048

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

CUtlSymbolTableMT* CUtlSymbol::s_pSymbolTable = 0; 
bool CUtlSymbol::s_bAllowStaticSymbolTable = true;


//-----------------------------------------------------------------------------
// symbol methods
//-----------------------------------------------------------------------------

void CUtlSymbol::Initialize()
{
	// If this assert fails, then the module that this call is in has chosen to disallow
	// use of the static symbol table. Usually, it's to prevent confusion because it's easy
	// to accidentally use the global symbol table when you really want to use a specific one.
	Assert( s_bAllowStaticSymbolTable );

	// necessary to allow us to create global symbols
	static bool symbolsInitialized = false;
	if (!symbolsInitialized)
	{
		s_pSymbolTable = new CUtlSymbolTableMT;
		symbolsInitialized = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Singleton to delete table on exit from module
//-----------------------------------------------------------------------------
class CCleanupUtlSymbolTable
{
public:
	~CCleanupUtlSymbolTable()
	{
		delete CUtlSymbol::s_pSymbolTable;
		CUtlSymbol::s_pSymbolTable = NULL;
	}
};

static CCleanupUtlSymbolTable g_CleanupSymbolTable;

CUtlSymbolTableMT* CUtlSymbol::CurrTable()
{
	Initialize();
	return s_pSymbolTable; 
}


//-----------------------------------------------------------------------------
// string->symbol->string
//-----------------------------------------------------------------------------

CUtlSymbol::CUtlSymbol( const char* pStr )
{
	m_Id = CurrTable()->AddString( pStr );
}

const char* CUtlSymbol::String( ) const
{
	return CurrTable()->String(m_Id);
}

void CUtlSymbol::DisableStaticSymbolTable()
{
	s_bAllowStaticSymbolTable = false;
}

//-----------------------------------------------------------------------------
// checks if the symbol matches a string
//-----------------------------------------------------------------------------

bool CUtlSymbol::operator==( const char* pStr ) const
{
	if (m_Id == UTL_INVAL_SYMBOL) 
		return false;
	return strcmp( String(), pStr ) == 0;
}



//-----------------------------------------------------------------------------
// symbol table stuff
//-----------------------------------------------------------------------------

inline const char* CUtlSymbolTable::StringFromIndex( const CStringPoolIndex &index ) const
{
	Assert( index.m_iPool < m_StringPools.Count() );
	Assert( index.m_iOffset < m_StringPools[index.m_iPool]->m_TotalLen );

	return &m_StringPools[index.m_iPool]->m_Data[index.m_iOffset];
}


bool CUtlSymbolTable::CLess::operator()( const CStringPoolIndex &i1, const CStringPoolIndex &i2 ) const
{
	// Need to do pointer math because CUtlSymbolTable is used in CUtlVectors, and hence
	// can be arbitrarily moved in memory on a realloc. Yes, this is portable. In reality,
	// right now at least, because m_LessFunc is the first member of CUtlRBTree, and m_Lookup
	// is the first member of CUtlSymbolTabke, this == pTable
	CUtlSymbolTable *pTable = (CUtlSymbolTable *)( (byte *)this - offsetof(CUtlSymbolTable::CTree, m_LessFunc) ) - offsetof(CUtlSymbolTable, m_Lookup );
	const char* str1 = (i1 == INVALID_STRING_INDEX) ? pTable->m_pUserSearchString :
													  pTable->StringFromIndex( i1 );
	const char* str2 = (i2 == INVALID_STRING_INDEX) ? pTable->m_pUserSearchString :
													  pTable->StringFromIndex( i2 );

	if ( !str1 && str2 )
		return false;
	if ( !str2 && str1 )
		return true;
	if ( !str1 && !str2 )
		return false;
	if ( !pTable->m_bInsensitive )
		return V_strcmp( str1, str2 ) < 0;
	else
		return V_stricmp( str1, str2 ) < 0;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CUtlSymbolTable::CUtlSymbolTable( int growSize, int initSize, bool caseInsensitive ) : 
	m_Lookup( growSize, initSize ), m_bInsensitive( caseInsensitive ), m_StringPools( 8 )
{
}

CUtlSymbolTable::~CUtlSymbolTable()
{
	// Release the stringpool string data
	RemoveAll();
}


CUtlSymbol CUtlSymbolTable::Find( const char* pString ) const
{	
	if (!pString)
		return CUtlSymbol();
	
	// Store a special context used to help with insertion
	m_pUserSearchString = pString;
	
	// Passing this special invalid symbol makes the comparison function
	// use the string passed in the context
	UtlSymId_t idx = m_Lookup.Find( INVALID_STRING_INDEX );

#ifdef _DEBUG
	m_pUserSearchString = NULL;
#endif

	return CUtlSymbol( idx );
}


int CUtlSymbolTable::FindPoolWithSpace( int len )	const
{
	for ( int i=0; i < m_StringPools.Count(); i++ )
	{
		StringPool_t *pPool = m_StringPools[i];

		if ( (pPool->m_TotalLen - pPool->m_SpaceUsed) >= len )
		{
			return i;
		}
	}

	return -1;
}


//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------

CUtlSymbol CUtlSymbolTable::AddString( const char* pString )
{
	if (!pString) 
		return CUtlSymbol( UTL_INVAL_SYMBOL );

	CUtlSymbol id = Find( pString );
	
	if (id.IsValid())
		return id;

	int len = V_strlen(pString) + 1;

	// Find a pool with space for this string, or allocate a new one.
	int iPool = FindPoolWithSpace( len );
	if ( iPool == -1 )
	{
		// Add a new pool.
		int newPoolSize = max( len, MIN_STRING_POOL_SIZE );
		StringPool_t *pPool = (StringPool_t*)malloc( sizeof( StringPool_t ) + newPoolSize - 1 );
		pPool->m_TotalLen = newPoolSize;
		pPool->m_SpaceUsed = 0;
		iPool = m_StringPools.AddToTail( pPool );
	}

	// Copy the string in.
	StringPool_t *pPool = m_StringPools[iPool];
	Assert( pPool->m_SpaceUsed < 0xFFFF );	// This should never happen, because if we had a string > 64k, it
											// would have been given its entire own pool.
	
	unsigned short iStringOffset = pPool->m_SpaceUsed;

	memcpy( &pPool->m_Data[pPool->m_SpaceUsed], pString, len );
	pPool->m_SpaceUsed += len;

	// didn't find, insert the string into the vector.
	CStringPoolIndex index;
	index.m_iPool = iPool;
	index.m_iOffset = iStringOffset;

	UtlSymId_t idx = m_Lookup.Insert( index );
	return CUtlSymbol( idx );
}


//-----------------------------------------------------------------------------
// Look up the string associated with a particular symbol
//-----------------------------------------------------------------------------

const char* CUtlSymbolTable::String( CUtlSymbol id ) const
{
	if (!id.IsValid()) 
		return "";
	
	Assert( m_Lookup.IsValidIndex((UtlSymId_t)id) );
	return StringFromIndex( m_Lookup[id] );
}


//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------

void CUtlSymbolTable::RemoveAll()
{
	m_Lookup.Purge();
	
	for ( int i=0; i < m_StringPools.Count(); i++ )
		free( m_StringPools[i] );

	m_StringPools.RemoveAll();
}



class CUtlFilenameSymbolTable::HashTable : public CUtlStableHashtable<CUtlConstString>
{
};

CUtlFilenameSymbolTable::CUtlFilenameSymbolTable()
{
	m_Strings = new HashTable;
}

CUtlFilenameSymbolTable::~CUtlFilenameSymbolTable()
{
	delete m_Strings;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFileName - 
// Output : FileNameHandle_t
//-----------------------------------------------------------------------------
FileNameHandle_t CUtlFilenameSymbolTable::FindOrAddFileName( const char *pFileName )
{
	if ( !pFileName )
	{
		return NULL;
	}

	// find first
	FileNameHandle_t hFileName = FindFileName( pFileName );
	if ( hFileName )
	{
		return hFileName;
	}

	// Fix slashes+dotslashes and make lower case first..
	char fn[ MAX_PATH ];
	Q_strncpy( fn, pFileName, sizeof( fn ) );
	Q_RemoveDotSlashes( fn );
#ifdef _WIN32
	Q_strlower( fn );
#endif

	// Split the filename into constituent parts
	char basepath[ MAX_PATH ];
	Q_ExtractFilePath( fn, basepath, sizeof( basepath ) );
	char filename[ MAX_PATH ];
	Q_strncpy( filename, fn + Q_strlen( basepath ), sizeof( filename ) );

	// not found, lock and look again
	FileNameHandleInternal_t handle;
	m_lock.LockForWrite();
	handle.path = m_Strings->Insert( basepath ) + 1;
	handle.file = m_Strings->Insert( filename ) + 1;
	//handle.path = m_StringPool.FindStringHandle( basepath );
	//handle.file = m_StringPool.FindStringHandle( filename );
	//if ( handle.path != m_Strings.InvalidHandle() && handle.file )
	//{
		// found
	//	m_lock.UnlockWrite();
	//	return *( FileNameHandle_t * )( &handle );
	//}

	// safely add it
	//handle.path = m_StringPool.ReferenceStringHandle( basepath );
	//handle.file = m_StringPool.ReferenceStringHandle( filename );
	m_lock.UnlockWrite();

	return *( FileNameHandle_t * )( &handle );
}

FileNameHandle_t CUtlFilenameSymbolTable::FindFileName( const char *pFileName )
{
	if ( !pFileName )
	{
		return NULL;
	}

	// Fix slashes+dotslashes and make lower case first..
	char fn[ MAX_PATH ];
	Q_strncpy( fn, pFileName, sizeof( fn ) );
	Q_RemoveDotSlashes( fn );
#ifdef _WIN32
	Q_strlower( fn );
#endif

	// Split the filename into constituent parts
	char basepath[ MAX_PATH ];
	Q_ExtractFilePath( fn, basepath, sizeof( basepath ) );
	char filename[ MAX_PATH ];
	Q_strncpy( filename, fn + Q_strlen( basepath ), sizeof( filename ) );

	FileNameHandleInternal_t handle;

	Assert( (uint16)(m_Strings->InvalidHandle() + 1) == 0 );

	m_lock.LockForRead();
	handle.path = m_Strings->Find(basepath) + 1;
	handle.file = m_Strings->Find(filename) + 1;
	//handle.path = m_StringPool.FindStringHandle(basepath);
	//handle.file = m_StringPool.FindStringHandle(filename);
	m_lock.UnlockRead();

	if ( handle.path == 0 || handle.file == 0 )
		return NULL;

	return *( FileNameHandle_t * )( &handle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : handle - 
// Output : const char
//-----------------------------------------------------------------------------
bool CUtlFilenameSymbolTable::String( const FileNameHandle_t& handle, char *buf, int buflen )
{
	buf[ 0 ] = 0;

	FileNameHandleInternal_t *internal = ( FileNameHandleInternal_t * )&handle;
	if ( !internal || !internal->file || !internal->path )
	{
		return false;
	}

	m_lock.LockForRead();
	//const char *path = m_StringPool.HandleToString(internal->path);
	//const char *fn = m_StringPool.HandleToString(internal->file);
	const char *path = (*m_Strings)[ internal->path - 1 ].Get();
	const char *fn = (*m_Strings)[ internal->file - 1].Get();
	m_lock.UnlockRead();

	if ( !path || !fn )
	{
		return false;
	}

	Q_strncpy( buf, path, buflen );
	Q_strncat( buf, fn, buflen, COPY_ALL_CHARACTERS );

	return true;
}

void CUtlFilenameSymbolTable::RemoveAll()
{
	m_Strings->Purge();
}
