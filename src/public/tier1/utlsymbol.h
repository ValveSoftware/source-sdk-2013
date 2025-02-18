//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a symbol table
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTLSYMBOL_H
#define UTLSYMBOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/threadtools.h"
#include "tier1/utlrbtree.h"
#include "tier1/utlvector.h"


//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class CUtlSymbolTable;
class CUtlSymbolTableMT;


//-----------------------------------------------------------------------------
// This is a symbol, which is a easier way of dealing with strings.
//-----------------------------------------------------------------------------
typedef unsigned short UtlSymId_t;

#define UTL_INVAL_SYMBOL  ((UtlSymId_t)~0)

class CUtlSymbol
{
public:
	// constructor, destructor
	CUtlSymbol() : m_Id(UTL_INVAL_SYMBOL) {}
	CUtlSymbol( UtlSymId_t id ) : m_Id(id) {}
	CUtlSymbol( const char* pStr );
	CUtlSymbol( CUtlSymbol const& sym ) : m_Id(sym.m_Id) {}
	
	// operator=
	CUtlSymbol& operator=( CUtlSymbol const& src ) { m_Id = src.m_Id; return *this; }
	
	// operator==
	bool operator==( CUtlSymbol const& src ) const { return m_Id == src.m_Id; }
	bool operator==( const char* pStr ) const;
	
	// Is valid?
	bool IsValid() const { return m_Id != UTL_INVAL_SYMBOL; }
	
	// Gets at the symbol
	operator UtlSymId_t const() const { return m_Id; }
	
	// Gets the string associated with the symbol
	const char* String( ) const;

	// Modules can choose to disable the static symbol table so to prevent accidental use of them.
	static void DisableStaticSymbolTable();
		
protected:
	UtlSymId_t   m_Id;
		
	// Initializes the symbol table
	static void Initialize();
	
	// returns the current symbol table
	static CUtlSymbolTableMT* CurrTable();
		
	// The standard global symbol table
	static CUtlSymbolTableMT* s_pSymbolTable; 

	static bool s_bAllowStaticSymbolTable;

	friend class CCleanupUtlSymbolTable;
};


//-----------------------------------------------------------------------------
// CUtlSymbolTable:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. The symbol class itself contains
//    a static version of this class for creating global strings, but this
//    class can also be instanced to create local symbol tables.
//-----------------------------------------------------------------------------

class CUtlSymbolTable
{
public:
	// constructor, destructor
	CUtlSymbolTable( int growSize = 0, int initSize = 32, bool caseInsensitive = false );
	~CUtlSymbolTable();
	
	// Finds and/or creates a symbol based on the string
	CUtlSymbol AddString( const char* pString );

	// Finds the symbol for pString
	CUtlSymbol Find( const char* pString ) const;
	
	// Look up the string associated with a particular symbol
	const char* String( CUtlSymbol id ) const;
	
	inline bool HasElement( const char* pStr ) const
	{
		return Find( pStr ) != UTL_INVAL_SYMBOL;
	}

	// Remove all symbols in the table.
	void  RemoveAll();

	int GetNumStrings( void ) const
	{
		return m_Lookup.Count();
	}

protected:
	class CStringPoolIndex
	{
	public:
		inline CStringPoolIndex()
		{
		}

		inline CStringPoolIndex( unsigned short iPool, unsigned short iOffset )
		{
			m_iPool = iPool;
			m_iOffset = iOffset;
		}

		inline bool operator==( const CStringPoolIndex &other )	const
		{
			return m_iPool == other.m_iPool && m_iOffset == other.m_iOffset;
		}

		unsigned short m_iPool;		// Index into m_StringPools.
		unsigned short m_iOffset;	// Index into the string pool.
	};

	class CLess
	{
	public:
		CLess( int ignored = 0 ) {} // permits default initialization to NULL in CUtlRBTree
		bool operator!() const { return false; }
		bool operator()( const CStringPoolIndex &left, const CStringPoolIndex &right ) const;
	};

	// Stores the symbol lookup
	class CTree : public CUtlRBTree<CStringPoolIndex, unsigned short, CLess>
	{
	public:
		CTree(  int growSize, int initSize ) : CUtlRBTree<CStringPoolIndex, unsigned short, CLess>( growSize, initSize ) {}
		friend class CUtlSymbolTable::CLess; // Needed to allow CLess to calculate pointer to symbol table
	};

	struct StringPool_t
	{	
		int m_TotalLen;		// How large is 
		int m_SpaceUsed;
		char m_Data[1];
	};

	CTree m_Lookup;
	bool m_bInsensitive;
	mutable const char* m_pUserSearchString;

	// stores the string data
	CUtlVector<StringPool_t*> m_StringPools;

private:
	int FindPoolWithSpace( int len ) const;
	const char* StringFromIndex( const CStringPoolIndex &index ) const;

	friend class CLess;
};

class CUtlSymbolTableMT : private CUtlSymbolTable
{
public:
	CUtlSymbolTableMT( int growSize = 0, int initSize = 32, bool caseInsensitive = false )
		: CUtlSymbolTable( growSize, initSize, caseInsensitive )
	{
	}

	CUtlSymbol AddString( const char* pString )
	{
		m_lock.LockForWrite();
		CUtlSymbol result = CUtlSymbolTable::AddString( pString );
		m_lock.UnlockWrite();
		return result;
	}

	CUtlSymbol Find( const char* pString ) const
	{
		m_lock.LockForRead();
		CUtlSymbol result = CUtlSymbolTable::Find( pString );
		m_lock.UnlockRead();
		return result;
	}

	const char* String( CUtlSymbol id ) const
	{
		m_lock.LockForRead();
		const char *pszResult = CUtlSymbolTable::String( id );
		m_lock.UnlockRead();
		return pszResult;
	}
	
private:
#if defined(WIN32) || defined(_WIN32)
	mutable CThreadSpinRWLock m_lock;
#else
	mutable CThreadRWLock m_lock;
#endif
};



//-----------------------------------------------------------------------------
// CUtlFilenameSymbolTable:
// description:
//    This class defines a symbol table of individual filenames, stored more
//	  efficiently than a standard symbol table.  Internally filenames are broken
//	  up into file and path entries, and a file handle class allows convenient 
//	  access to these.
//-----------------------------------------------------------------------------

// The handle is a CUtlSymbol for the dirname and the same for the filename, the accessor
//  copies them into a static char buffer for return.
typedef void* FileNameHandle_t;
#define FILENAMEHANDLE_INVALID 0

// Symbol table for more efficiently storing filenames by breaking paths and filenames apart.
// Refactored from BaseFileSystem.h
class CUtlFilenameSymbolTable
{
	// Internal representation of a FileHandle_t
	// If we get more than 64K filenames, we'll have to revisit...
	// Right now CUtlSymbol is a short, so this packs into an int/void * pointer size...
	struct FileNameHandleInternal_t
	{
		FileNameHandleInternal_t()
		{
			COMPILE_TIME_ASSERT( sizeof( *this ) == sizeof( FileNameHandle_t ) );

			path = 0;
			file = 0;

#ifdef PLATFORM_64BITS
			pad = 0;
#endif
		}

		// Part before the final '/' character
		unsigned short path;
		// Part after the final '/', including extension
		unsigned short file;

#ifdef PLATFORM_64BITS
		// some padding to make sure we are the same size as FileNameHandle_t on 64 bit.
		unsigned int pad;
#endif
	};

	class HashTable;

public:
	CUtlFilenameSymbolTable();
	~CUtlFilenameSymbolTable();
	FileNameHandle_t	FindOrAddFileName( const char *pFileName );
	FileNameHandle_t	FindFileName( const char *pFileName );
	int					PathIndex(const FileNameHandle_t &handle) { return (( const FileNameHandleInternal_t * )&handle)->path; }
	bool				String( const FileNameHandle_t& handle, char *buf, int buflen );
	void				RemoveAll();

private:
	//CCountedStringPool	m_StringPool;
	HashTable* m_Strings;
	mutable CThreadSpinRWLock m_lock;
};


#endif // UTLSYMBOL_H
