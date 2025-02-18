//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "keyvaluescompiler.h"
#include "filesystem.h"
#include "tier1/KeyValues.h"

extern IFileSystem *g_pFullFileSystem;

bool CRunTimeKeyValuesStringTable::ReadStringTable( int numStrings, CUtlBuffer& buf )
{
	Assert( m_Strings.Count() == 0 );

	CUtlVector< int > offsets;
	offsets.EnsureCapacity( numStrings );

	offsets.CopyArray( (int *)( buf.PeekGet() ), numStrings );

	// Skip over data
	buf.SeekGet( CUtlBuffer::SEEK_HEAD, buf.TellGet() + numStrings * sizeof( int ) );

	int stringSize = buf.GetInt();
	
	// Read in the string table
	m_Strings.EnsureCapacity( numStrings );
	int i;
	for ( i = 0 ; i < numStrings; ++i )
	{
		m_Strings.AddToTail( (const char *)buf.PeekGet( offsets[ i ] ) );
	}

	buf.SeekGet( CUtlBuffer::SEEK_HEAD, buf.TellGet() + stringSize );

	return true;
}

void CCompiledKeyValuesWriter::BuildKVData_R( KeyValues *kv, int parent )
{
	// Add self
	KVInfo_t info;
	info.key = m_StringTable.AddString( kv->GetName() );
	info.value = m_StringTable.AddString( kv->GetString() );

	info.SetSubTree( kv->GetFirstSubKey() != NULL ? true : false );
	info.SetParent( parent );

	int newParent = m_Data.AddToTail( info );

	// Then add children
	for ( KeyValues *sub = kv->GetFirstSubKey(); sub; sub = sub->GetNextKey() )
	{
		BuildKVData_R( sub, newParent );
	}

	// Then add peers
	if ( parent == -1 )
	{
		if ( kv->GetNextKey() )
		{
			BuildKVData_R( kv->GetNextKey(), parent );
		}
	}
}

void CCompiledKeyValuesWriter::Describe( const KVFile_t& file )
{
	Msg( "file( %s )\n", m_StringTable.String( file.filename ) );

	int c = file.numElements;
	for ( int i = 0; i < c; ++i )
	{
		KVInfo_t &info = m_Data[ file.firstElement + i ];
		if ( info.IsSubTree() )
		{
			Msg( "%d:  %s -> subtree at parent %i\n",
				file.firstElement + i,
				m_StringTable.String( info.key ),
				info.GetParent() );
		}
		else
		{
			Msg( "%d:  %s -> %s at parent %i\n",
				file.firstElement + i,
				m_StringTable.String( info.key ),
				m_StringTable.String( info.value ),
				info.GetParent() );
		}
	}
}

void CCompiledKeyValuesWriter::AppendKeyValuesFile( char const *filename )
{
	KVFile_t kvf;
	kvf.filename = m_StringTable.AddString( filename );
	kvf.firstElement = m_Data.Count();

	KeyValues *kv = new KeyValues( filename );
	if ( kv->LoadFromFile( g_pFullFileSystem, filename ) )
	{
		// Add to dictionary
		// do a depth first traversal of the keyvalues
		BuildKVData_R( kv, -1 );
	}
	kv->deleteThis();

	kvf.numElements = m_Data.Count() - kvf.firstElement;

//	Describe( kvf );

	m_Files.AddToTail( kvf );
}

void CCompiledKeyValuesWriter::WriteData( CUtlBuffer& buf )
{
	int c = m_Data.Count();
	buf.PutInt( c );
	for ( int i = 0; i < c; ++i )
	{
		KVInfo_t &info = m_Data[ i ];
		buf.PutShort( info.key );
		buf.PutShort( info.value );
		buf.PutShort( info.GetParent() );
		buf.PutChar( info.IsSubTree() ? 1 : 0 );
	}
}

void CCompiledKeyValuesWriter::WriteFiles( CUtlBuffer &buf )
{
	int c = m_Files.Count();
	buf.PutInt( c );
	for ( int i = 0; i < c; ++i )
	{
		KVFile_t &file = m_Files[ i ];
		buf.PutShort( file.filename );
		buf.PutShort( file.firstElement );
		buf.PutShort( file.numElements );
	}
}

void CCompiledKeyValuesWriter::WriteStringTable( CUtlBuffer& buf )
{
	int i;
	CUtlVector< int >	offsets;

	CUtlBuffer stringBuffer;

	offsets.AddToTail( stringBuffer.TellPut() );

	stringBuffer.PutString( "" );
	// save all the rest
	int c = m_StringTable.GetNumStrings();
	for ( i = 1; i < c; i++)
	{
		offsets.AddToTail( stringBuffer.TellPut() );
		stringBuffer.PutString( m_StringTable.String( i ) );
	}

	buf.Put( offsets.Base(), offsets.Count() * sizeof( int ) );

	buf.PutInt( stringBuffer.TellPut() );
	buf.Put( stringBuffer.Base(), stringBuffer.TellPut() );
}

void CCompiledKeyValuesWriter::WriteFile( char const *outfile )
{
	CUtlBuffer buf;

	// Write the data file out
	KVHeader_t header;
	header.fileid		= COMPILED_KEYVALUES_ID;
	header.version		= COMPILED_KEYVALUES_VERSION;
	header.numStrings	= m_StringTable.GetNumStrings();

	buf.Put( &header, sizeof( header ) );

	WriteStringTable( buf );
	WriteData( buf );
	WriteFiles( buf );

	g_pFullFileSystem->WriteFile( outfile, NULL, buf );
}

CCompiledKeyValuesReader::CCompiledKeyValuesReader()
	: m_Dict( 0, 0, FileInfo_t::Less )
{
}

int CCompiledKeyValuesReader::First() const
{
	return m_Dict.FirstInorder();
}

int CCompiledKeyValuesReader::Next( int i ) const
{
	return m_Dict.NextInorder( i );
}

int CCompiledKeyValuesReader::InvalidIndex() const
{
	return m_Dict.InvalidIndex();
}

void CCompiledKeyValuesReader::GetFileName( int index, char *buf, size_t bufsize )
{
	Assert( buf );
	buf[ 0 ] = 0;
	FileNameHandle_t& handle = m_Dict[ index ].hFile;
	g_pFullFileSystem->String( handle, buf, bufsize );
}

bool CCompiledKeyValuesReader::LoadFile( char const *filename )
{
	int i;
	m_LoadBuffer.Purge();

	g_pFullFileSystem->ReadFile( filename, NULL, m_LoadBuffer );

	KVHeader_t header;
	m_LoadBuffer.Get( &header, sizeof( header ) );

	if ( header.fileid != COMPILED_KEYVALUES_ID )
	{
		return false;
	}

	if ( header.version != COMPILED_KEYVALUES_VERSION )
	{
		return false;
	}

	if ( !m_StringTable.ReadStringTable( header.numStrings, m_LoadBuffer ) )
	{
		return false;
	}

	// Now parse the data
	int dataCount = m_LoadBuffer.GetInt();
	m_Data.EnsureCapacity( dataCount );
	for ( i = 0; i < dataCount; ++i )
	{
		KVInfo_t info;
		info.key = m_LoadBuffer.GetShort();
		info.value = m_LoadBuffer.GetShort();
		info.SetParent( m_LoadBuffer.GetShort() );
		info.SetSubTree( m_LoadBuffer.GetChar() == 1 ? true : false );
		m_Data.AddToTail( info );
	}

	int fileCount = m_LoadBuffer.GetInt();
	for ( i = 0; i < fileCount; ++i )
	{
		FileInfo_t kvf;
		short fileNameString	= m_LoadBuffer.GetShort();

		kvf.hFile		= g_pFullFileSystem->FindOrAddFileName( m_StringTable.Lookup( fileNameString ) );
		kvf.nFirstIndex = m_LoadBuffer.GetShort();
		kvf.nCount		= m_LoadBuffer.GetShort();

		m_Dict.Insert( kvf );
	}

	return true;
}

struct CreateHelper_t
{
	int			index;
	KeyValues	*kv;
	KeyValues	*tail;

	static bool Less( const CreateHelper_t& lhs, const CreateHelper_t& rhs )
	{
		return lhs.index < rhs.index;
	}
};

KeyValues *CCompiledKeyValuesReader::CreateFromData( const FileInfo_t& info )
{
	KeyValues *head = new KeyValues( "" );
	if ( CreateInPlaceFromData( *head, info ) )
	{
		return head;
	}
	else
	{
		head->deleteThis();
		return NULL;
	}
}

bool CCompiledKeyValuesReader::CreateInPlaceFromData( KeyValues& head, const FileInfo_t& info )
{
	int first = info.nFirstIndex;
	int num = info.nCount;

	KeyValues *root = NULL;
	KeyValues *tail = NULL;

	CUtlRBTree< CreateHelper_t, int >	helper( 0, 0, CreateHelper_t::Less );

	for ( int i = 0; i < num; ++i )
	{
		int offset = first + i;
		KVInfo_t& info = m_Data[ offset ];

		if ( info.GetParent() != -1 )
		{
			CreateHelper_t search;
			search.index = info.GetParent();
			int idx = helper.Find( search );
			if ( idx == helper.InvalidIndex() )
			{
				return NULL;
			}

			KeyValues *parent = helper[ idx ].kv;
			Assert( parent );

			KeyValues *sub = new KeyValues( m_StringTable.Lookup( info.key ) );

			if ( !info.IsSubTree() )
			{
				sub->SetStringValue(m_StringTable.Lookup( info.value ) );
			}

			if ( !parent->GetFirstSubKey() )
			{
				parent->AddSubKey( sub );
			}
			else
			{
				KeyValues *last = helper[ idx ].tail;
				last->SetNextKey( sub );
			}

			helper[ idx ].tail = sub;

			CreateHelper_t insert;
			insert.index = offset;
			insert.kv = sub;
			insert.tail = NULL;
			helper.Insert( insert );
		}
		else
		{
			if ( !root )
			{
				root = &head;
				root->SetName( m_StringTable.Lookup( info.key ) );
				tail = root;

				CreateHelper_t insert;
				insert.index = offset;
				insert.kv = root;
				insert.tail = NULL;
				helper.Insert( insert );
			}
			else
			{
				CreateHelper_t insert;
				insert.index = offset;
				insert.kv = new KeyValues( m_StringTable.Lookup( info.key ) );
				insert.tail = NULL;
				helper.Insert( insert );

				tail->SetNextKey( insert.kv );
				tail = insert.kv;
			}
		}
	}
	return true;
}


bool CCompiledKeyValuesReader::InstanceInPlace( KeyValues& head, char const *kvfilename )
{
	char sz[ 512 ];
	Q_strncpy( sz, kvfilename, sizeof( sz ) );
	Q_FixSlashes( sz );

	FileInfo_t search;
	search.hFile = g_pFullFileSystem->FindOrAddFileName( sz );

	int idx = m_Dict.Find( search );
	if ( idx == m_Dict.InvalidIndex() )
	{
		return false;
	}

	const FileInfo_t& info = m_Dict[ idx ];

	return CreateInPlaceFromData( head, info );
}

KeyValues *CCompiledKeyValuesReader::Instance( char const *kvfilename )
{
	char sz[ 512 ];
	Q_strncpy( sz, kvfilename, sizeof( sz ) );
	Q_FixSlashes( sz );

	FileInfo_t search;
	search.hFile = g_pFullFileSystem->FindOrAddFileName( sz );

	int idx = m_Dict.Find( search );
	if ( idx == m_Dict.InvalidIndex() )
	{
		return NULL;
	}

	const FileInfo_t& info = m_Dict[ idx ];

	return CreateFromData( info );
}

bool CCompiledKeyValuesReader::LookupKeyValuesRootKeyName( char const *kvfilename, char *outbuf, size_t bufsize )
{
	char sz[ 512 ];
	Q_strncpy( sz, kvfilename, sizeof( sz ) );
	Q_FixSlashes( sz );

	FileInfo_t search;
	search.hFile = g_pFullFileSystem->FindOrAddFileName( sz );

	int idx = m_Dict.Find( search );
	if ( idx == m_Dict.InvalidIndex() )
	{
		return false;
	}

	const FileInfo_t& info = m_Dict[ idx ];

	Q_strncpy( outbuf, m_StringTable.Lookup( m_Data[ info.nFirstIndex ].key ), bufsize );
	return true;
}
