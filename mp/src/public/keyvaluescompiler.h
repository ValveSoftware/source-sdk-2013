//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef KEYVALUESCOMPILER_H
#define KEYVALUESCOMPILER_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlsymbol.h"
#include "tier1/utldict.h"

class KeyValues;

#define COMPILED_KEYVALUES_ID	MAKEID( 'V', 'K', 'V', 'F' )

#define COMPILED_KEYVALUES_VERSION	1

struct KVHeader_t
{
	int			fileid;
	int			version;
	int			numStrings;
};

#pragma pack(1)
struct KVFile_t
{
	KVFile_t() :
		filename( 0 ),
		firstElement( 0 ),
		numElements( 0 )
	{
	}
	short			filename;
	short			firstElement;
	short			numElements;
};

struct KVInfo_t
{
	KVInfo_t() :
		key( 0 ),
		value( 0 ),
		parentIndex( -1 ),
		issubtree( false )
	{
	}

	inline void SetParent( int index )
	{
		Assert( index <= 32768 );
		parentIndex = index;
	}

	inline short GetParent() const
	{
		return parentIndex;
	}

	inline void SetSubTree( bool state )
	{
		issubtree = state;
	}

	inline bool IsSubTree() const
	{
		return issubtree;
	}

	short		key;
	short		value;

private:

	short		parentIndex;
	bool		issubtree;
};
#pragma pack()

//-----------------------------------------------------------------------------
// Purpose: stringtable is a session global string table.
//-----------------------------------------------------------------------------
class CCompiledKeyValuesWriter
{
public:

	CCompiledKeyValuesWriter()
	{
		m_StringTable.AddString( "" );
	}

	void AppendKeyValuesFile( char const *filename );
	void WriteFile( char const *outfile );

private:

	void Describe( const KVFile_t& file );

	void BuildKVData_R( KeyValues *kv, int parent );

	void WriteStringTable( CUtlBuffer& buf );
	void WriteData( CUtlBuffer& buf );
	void WriteFiles( CUtlBuffer &buf );

	CUtlVector< KVFile_t >		m_Files;
	CUtlVector< KVInfo_t >		m_Data;

	CUtlSymbolTable				m_StringTable;
};


class CRunTimeKeyValuesStringTable
{
public:

	bool ReadStringTable( int numStrings, CUtlBuffer& buf );
	
	inline int Count() const
	{
		return m_Strings.Count();
	}

	inline char const *Lookup( short index )
	{
		return m_Strings[ index ];
	}

private:
	CUtlVector< const char * >	m_Strings; 
};

class CCompiledKeyValuesReader
{
public:

	CCompiledKeyValuesReader();
	
	bool		LoadFile( char const *filename );

	KeyValues	*Instance( char const *kvfilename );
	bool		InstanceInPlace( KeyValues& head, char const *kvfilename );
	bool		LookupKeyValuesRootKeyName( char const *kvfilename, char *outbuf, size_t bufsize );

	int			First() const;
	int			Next( int i ) const;
	int			InvalidIndex() const;

	void		GetFileName( int index, char *buf, size_t bufsize );

private:

	struct FileInfo_t
	{
		FileInfo_t() : 
			hFile( 0 ),
			nFirstIndex( 0 ), 
			nCount( 0 ) 
		{
		}
		FileNameHandle_t	hFile;
		short				nFirstIndex;
		short				nCount;

		static bool Less( const FileInfo_t& lhs, const FileInfo_t& rhs )
		{
			return lhs.hFile < rhs.hFile;
		}
	};

	KeyValues *CreateFromData( const FileInfo_t& info );
	bool CreateInPlaceFromData( KeyValues& head, const FileInfo_t& info );

	// Now get the actual files
	CUtlRBTree< FileInfo_t, unsigned short >	m_Dict;
	CUtlVector< KVInfo_t >		m_Data;

	CRunTimeKeyValuesStringTable		m_StringTable;

	CUtlBuffer							m_LoadBuffer;
};

#endif // KEYVALUESCOMPILER_H
