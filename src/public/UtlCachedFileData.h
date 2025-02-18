//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef UTLCACHEDFILEDATA_H
#define UTLCACHEDFILEDATA_H
#if defined( WIN32 )
#pragma once
#endif

#include "filesystem.h" // FileNameHandle_t
#include "utlrbtree.h"
#include "utlbuffer.h"
#include "UtlSortVector.h"
#include "tier1/strtools.h"

#include "tier0/memdbgon.h"

// If you change to serialization protocols, this must be bumped...
#define UTL_CACHE_SYSTEM_VERSION		2

#define UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO	(long)-2

// Cacheable types must derive from this and implement the appropriate methods...
abstract_class IBaseCacheInfo
{
public:
	virtual void Save( CUtlBuffer& buf ) = 0;
	virtual void Restore( CUtlBuffer& buf ) = 0;

	virtual void Rebuild( char const *filename ) = 0;
};

typedef unsigned int (*PFNCOMPUTECACHEMETACHECKSUM)( void );

typedef enum
{
	UTL_CACHED_FILE_USE_TIMESTAMP = 0,
	UTL_CACHED_FILE_USE_FILESIZE,
} UtlCachedFileDataType_t;

template <class T>
class CUtlCachedFileData
{
public:
	CUtlCachedFileData
	( 
		char const *repositoryFileName, 
		int version, 
		PFNCOMPUTECACHEMETACHECKSUM checksumfunc = NULL, 
		UtlCachedFileDataType_t fileCheckType = UTL_CACHED_FILE_USE_TIMESTAMP,
		bool nevercheckdisk = false,
		bool readonly = false,
		bool savemanifest = false
	)
		: m_Elements( 0, 0, FileNameHandleLessFunc ),
		m_sRepositoryFileName( repositoryFileName ),
		m_nVersion( version ),
		m_pfnMetaChecksum( checksumfunc ),
		m_bDirty( false ),
		m_bInitialized( false ),
		m_uCurrentMetaChecksum( 0u ),
		m_fileCheckType( fileCheckType ),
		m_bNeverCheckDisk( nevercheckdisk ),
		m_bReadOnly( readonly ),
		m_bSaveManifest( savemanifest )
	{
		Assert( !m_sRepositoryFileName.IsEmpty() );
	}

	virtual ~CUtlCachedFileData()
	{
		m_Elements.RemoveAll();
		int c = m_Data.Count();
		for ( int i = 0; i < c ; ++i )
		{
			delete m_Data[ i ];
		}
		m_Data.RemoveAll();
	}

	// If bExpectMissing is set, don't complain if this causes synchronous disk I/O to build the cache.
	T* Get( char const *filename );
	const T* Get( char const *filename ) const;

	T* operator[]( int i );
	const T* operator[]( int i ) const;

	int Count() const;

	void GetElementName( int i, char *buf, int buflen )
	{
		buf[ 0 ] = 0;
		if ( !m_Elements.IsValidIndex( i ) )
			return;

		g_pFullFileSystem->String( m_Elements[ i ].handle, buf, buflen );
	}

	bool EntryExists( char const *filename ) const
	{
		ElementType_t element;
		element.handle = g_pFullFileSystem->FindOrAddFileName( filename );
		int idx = m_Elements.Find( element );
		return idx != m_Elements.InvalidIndex() ? true : false;
	}

	void SetElement( char const *name, long fileinfo, T* src )
	{
		SetDirty( true );

		int idx = GetIndex( name );

		Assert( idx != m_Elements.InvalidIndex() );

		ElementType_t& e = m_Elements[ idx ];

		CUtlBuffer buf( 0, 0, 0 );

		Assert( e.dataIndex != m_Data.InvalidIndex() );

		T *dest = m_Data[ e.dataIndex ];

		Assert( dest );

		// I suppose we could do an assignment operator, but this should save/restore the data element just fine for
		//  tool purposes
		((IBaseCacheInfo *)src)->Save( buf );
		((IBaseCacheInfo *)dest)->Restore( buf );

		e.fileinfo = fileinfo;
		if ( ( e.fileinfo == -1 ) &&
			( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) )
		{
			e.fileinfo = 0;
		}
		// Force recheck
		e.diskfileinfo = UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO;
	}

	// If you create a cache and don't call init/shutdown, you can call this to do a quick check to see if the checksum/version
	//  will cause a rebuild...
	bool	IsUpToDate();

	void	Shutdown();
	bool	Init();

	void	Save();

	void	Reload();

	void	ForceRecheckDiskInfo();
	// Iterates all entries and gets filesystem info and optionally causes rebuild on any existing items which are out of date
	void	CheckDiskInfo( bool force_rebuild, long cacheFileTime = 0L );

	void	SaveManifest();
	bool	ManifestExists();

	const char *GetRepositoryFileName() const { return m_sRepositoryFileName; }

	long	GetFileInfo( char const *filename )
	{
		ElementType_t element;
		element.handle = g_pFullFileSystem->FindOrAddFileName( filename );
		int idx = m_Elements.Find( element );
		if ( idx == m_Elements.InvalidIndex() )
		{
			return 0L;
		}

		return m_Elements[ idx ].fileinfo;
	}

	int		GetNumElements()
	{
		return m_Elements.Count();
	}

	bool		IsDirty() const
	{
		return m_bDirty;
	}

	T *RebuildItem( const char *filename );

private:

	void		InitSmallBuffer( FileHandle_t& fh, int fileSize, bool& deleteFile );
	void		InitLargeBuffer( FileHandle_t& fh, bool& deleteFile );

	int			GetIndex( const char *filename )
	{
		ElementType_t element;
		element.handle = g_pFullFileSystem->FindOrAddFileName( filename );
		int idx = m_Elements.Find( element );
		if ( idx == m_Elements.InvalidIndex() )
		{
			T *data = new T();

			int dataIndex = m_Data.AddToTail( data );
			idx = m_Elements.Insert( element );
			m_Elements[ idx ].dataIndex = dataIndex;
		}

		return idx;
	}

	void		CheckInit();

	void		SetDirty( bool dirty )
	{
		m_bDirty = dirty;
	}

	void		RebuildCache( char const *filename, T *data );

	struct ElementType_t
	{
		ElementType_t() :
			handle( 0 ),
			fileinfo( 0 ),
			diskfileinfo( UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO ),
			dataIndex( -1 )
		{
		}

		FileNameHandle_t	handle;
		long				fileinfo;
		long				diskfileinfo;
		int					dataIndex;
	};

	static bool FileNameHandleLessFunc( ElementType_t const &lhs, ElementType_t const &rhs )
	{
		return lhs.handle < rhs.handle;
	}

	CUtlRBTree< ElementType_t >		m_Elements;
	CUtlVector< T * >				m_Data;
	CUtlString						m_sRepositoryFileName;
	int								m_nVersion;
	PFNCOMPUTECACHEMETACHECKSUM		m_pfnMetaChecksum;
	unsigned int					m_uCurrentMetaChecksum;
	UtlCachedFileDataType_t			m_fileCheckType;
	bool							m_bNeverCheckDisk : 1;
	bool							m_bReadOnly		  : 1;
	bool							m_bSaveManifest   : 1;
	bool							m_bDirty          : 1;
	bool							m_bInitialized    : 1;

};


template <class T>
T* CUtlCachedFileData<T>::Get( char const *filename )
{
	int idx = GetIndex( filename );

	ElementType_t& e = m_Elements[ idx ];

	if ( e.fileinfo == -1 &&
		m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE )
	{
		e.fileinfo = 0;
	}
	long cachefileinfo = e.fileinfo;
	// Set the disk fileinfo the first time we encounter the filename
	if ( e.diskfileinfo == UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO )
	{
		if ( m_bNeverCheckDisk )
		{
			e.diskfileinfo = cachefileinfo;
		}
		else
		{
			if ( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) 
			{
				e.diskfileinfo = g_pFullFileSystem->Size( filename, "GAME" );
				// Missing files get a disk file size of 0
				if ( e.diskfileinfo == -1 )
				{
					e.diskfileinfo = 0;
				}
			}
			else
			{
				e.diskfileinfo = g_pFullFileSystem->GetFileTime( filename, "GAME" );
			}
		}
	}

	Assert( e.dataIndex != m_Data.InvalidIndex() );

	T *data = m_Data[ e.dataIndex ];

	Assert( data );

	// Compare fileinfo to disk fileinfo and rebuild cache if out of date or not correct...
	if ( cachefileinfo != e.diskfileinfo )
	{
		if ( !m_bReadOnly )
		{
			RebuildCache( filename, data );
		}
		e.fileinfo = e.diskfileinfo;
	}

	return data;
}

template <class T>
const T* CUtlCachedFileData<T>::Get( char const *filename ) const
{
	return const_cast< CUtlCachedFileData<T> * >(this)->Get( filename );
}

template <class T>
T* CUtlCachedFileData<T>::operator[]( int i )
{
	return m_Data[ m_Elements[ i ].dataIndex ];
}

template <class T>
const T* CUtlCachedFileData<T>::operator[]( int i ) const
{
	return m_Data[ m_Elements[ i ].dataIndex ];
}

template <class T>
int CUtlCachedFileData<T>::Count() const
{
	return m_Elements.Count();
}

template <class T>
void CUtlCachedFileData<T>::Reload()
{
	Shutdown();
	Init();
}

template <class T>
bool CUtlCachedFileData<T>::IsUpToDate()
{
	// Don't call Init/Shutdown if using this method!!!
	Assert( !m_bInitialized );

	if ( m_sRepositoryFileName.IsEmpty() )
	{
		Error( "CUtlCachedFileData:  Can't IsUpToDate, no repository file specified." );
		return false;
	}

	// Always compute meta checksum
	m_uCurrentMetaChecksum = m_pfnMetaChecksum ? (*m_pfnMetaChecksum)() : 0;

	FileHandle_t fh;

	fh = g_pFullFileSystem->Open( m_sRepositoryFileName, "rb", "MOD" );
	if ( fh == FILESYSTEM_INVALID_HANDLE )
	{
		return false;
	}

	// Version data is in first 12 bytes of file
	byte	header[ 12 ];
	g_pFullFileSystem->Read( header, sizeof( header ), fh );
	g_pFullFileSystem->Close( fh );

	int cacheversion = *( int *)&header[ 0 ];

	if ( UTL_CACHE_SYSTEM_VERSION != cacheversion )
	{
		DevMsg( "Discarding repository '%s' due to cache system version change\n", m_sRepositoryFileName.String() );
		Assert( !m_bReadOnly );
		if ( !m_bReadOnly )
		{
			g_pFullFileSystem->RemoveFile( m_sRepositoryFileName, "MOD" );
		}
		return false;
	}

	// Now parse data from the buffer
	int version = *( int *)&header[ 4 ];
	if ( version != m_nVersion )
	{
		DevMsg( "Discarding repository '%s' due to version change\n", m_sRepositoryFileName.String() );
		Assert( !m_bReadOnly );
		if ( !m_bReadOnly )
		{
			g_pFullFileSystem->RemoveFile( m_sRepositoryFileName, "MOD" );
		}
		return false;
	}

	// This is a checksum based on any meta data files which the cache depends on (supplied by a passed in
	//  meta data function
	unsigned int cache_meta_checksum = (unsigned int)*( int *)&header[ 8 ];

	if ( cache_meta_checksum != m_uCurrentMetaChecksum )
	{
		DevMsg( "Discarding repository '%s' due to meta checksum change\n", m_sRepositoryFileName.String() );
		Assert( !m_bReadOnly );
		if ( !m_bReadOnly )
		{
			g_pFullFileSystem->RemoveFile( m_sRepositoryFileName, "MOD" );
		}
		return false;
	}

	// Looks valid
	return true;
}

template <class T>
void CUtlCachedFileData<T>::InitSmallBuffer( FileHandle_t& fh, int fileSize, bool& deleteFile )
{
	deleteFile = false;

	CUtlBuffer loadBuf;
	g_pFullFileSystem->ReadToBuffer( fh, loadBuf );
	g_pFullFileSystem->Close( fh );

	int cacheversion = 0;
	loadBuf.Get( &cacheversion, sizeof( cacheversion ) );

	if ( UTL_CACHE_SYSTEM_VERSION == cacheversion )
	{
		// Now parse data from the buffer
		int version = loadBuf.GetInt();
		
		if ( version == m_nVersion )
		{
			// This is a checksum based on any meta data files which the cache depends on (supplied by a passed in
			//  meta data function
			unsigned int cache_meta_checksum = loadBuf.GetInt();
			
			if ( cache_meta_checksum == m_uCurrentMetaChecksum )
			{
				int count = loadBuf.GetInt();
				
				Assert( count < 2000000 );

				CUtlBuffer buf( 0, 0, 0 );

				for ( int i = 0 ; i < count; ++i )
				{
					int bufsize = loadBuf.GetInt();
					Assert( bufsize < 1000000 );

					buf.Clear();
					buf.EnsureCapacity( bufsize );
					
					loadBuf.Get( buf.Base(), bufsize );

					buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
					buf.SeekPut( CUtlBuffer::SEEK_HEAD, bufsize );

					// Read the element name
					char elementFileName[ 512 ];
					buf.GetString( elementFileName );

					// Now read the element
					int slot = GetIndex( elementFileName );

					Assert( slot != m_Elements.InvalidIndex() );

					ElementType_t& element = m_Elements[ slot ];

					element.fileinfo = buf.GetInt();
					if ( ( element.fileinfo == -1 ) &&
						( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) )
					{
						element.fileinfo = 0;
					}

					Assert( element.dataIndex != m_Data.InvalidIndex() );

					T *data = m_Data[ element.dataIndex ];

					Assert( data );

					((IBaseCacheInfo *)data)->Restore( buf );
				}
			}
			else
			{
				Msg( "Discarding repository '%s' due to meta checksum change\n", m_sRepositoryFileName.String() );
				deleteFile = true;
			}
		}
		else
		{
			Msg( "Discarding repository '%s' due to version change\n", m_sRepositoryFileName.String() );
			deleteFile = true;
		}
	}
	else
	{
		DevMsg( "Discarding repository '%s' due to cache system version change\n", m_sRepositoryFileName.String() );
		deleteFile = true;
	}
}

template <class T>
void CUtlCachedFileData<T>::InitLargeBuffer( FileHandle_t& fh, bool& deleteFile )
{
	deleteFile = false;

	int cacheversion = 0;
	g_pFullFileSystem->Read( &cacheversion, sizeof( cacheversion ), fh );

	if ( UTL_CACHE_SYSTEM_VERSION == cacheversion )
	{
		// Now parse data from the buffer
		int version = 0;
		g_pFullFileSystem->Read( &version, sizeof( version ), fh );
		
		if ( version == m_nVersion )
		{
			// This is a checksum based on any meta data files which the cache depends on (supplied by a passed in
			//  meta data function
			unsigned int cache_meta_checksum = 0;
			
			g_pFullFileSystem->Read( &cache_meta_checksum, sizeof( cache_meta_checksum ), fh );

			if ( cache_meta_checksum == m_uCurrentMetaChecksum )
			{
				int count = 0;
				
				g_pFullFileSystem->Read( &count, sizeof( count ), fh );

				Assert( count < 2000000 );

				CUtlBuffer buf( 0, 0, 0 );

				for ( int i = 0 ; i < count; ++i )
				{
					int bufsize = 0;
					g_pFullFileSystem->Read( &bufsize, sizeof( bufsize ), fh );

					Assert( bufsize < 1000000 );
					if ( bufsize > 1000000 )
					{
						Msg( "Discarding repository '%s' due to corruption\n", m_sRepositoryFileName.String() );
						deleteFile = true;
						break;
					}


					buf.Clear();
					buf.EnsureCapacity( bufsize );
					
					int nBytesRead = g_pFullFileSystem->Read( buf.Base(), bufsize, fh );

					buf.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
					buf.SeekPut( CUtlBuffer::SEEK_HEAD, nBytesRead );

					// Read the element name
					char elementFileName[ 512 ];
					buf.GetString( elementFileName );

					// Now read the element
					int slot = GetIndex( elementFileName );

					Assert( slot != m_Elements.InvalidIndex() );

					ElementType_t& element = m_Elements[ slot ];

					element.fileinfo = buf.GetInt();
					if ( ( element.fileinfo == -1 ) &&
						( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) )
					{
						element.fileinfo = 0;
					}

					Assert( element.dataIndex != m_Data.InvalidIndex() );

					T *data = m_Data[ element.dataIndex ];

					Assert( data );

					((IBaseCacheInfo *)data)->Restore( buf );
				}
			}
			else
			{
				Msg( "Discarding repository '%s' due to meta checksum change\n", m_sRepositoryFileName.String() );
				deleteFile = true;
			}
		}
		else
		{
			Msg( "Discarding repository '%s' due to version change\n", m_sRepositoryFileName.String() );
			deleteFile = true;
		}
	}
	else
	{
		DevMsg( "Discarding repository '%s' due to cache system version change\n", m_sRepositoryFileName.String() );
		deleteFile = true;
	}

	g_pFullFileSystem->Close( fh );
}

template <class T>
bool CUtlCachedFileData<T>::Init()
{
	if ( m_bInitialized )
	{
		return true;
	}

	m_bInitialized = true;

	if ( m_sRepositoryFileName.IsEmpty() )
	{
		Error( "CUtlCachedFileData:  Can't Init, no repository file specified." );
		return false;
	}

	// Always compute meta checksum
	m_uCurrentMetaChecksum = m_pfnMetaChecksum ? (*m_pfnMetaChecksum)() : 0;

	FileHandle_t fh;

	fh = g_pFullFileSystem->Open( m_sRepositoryFileName, "rb", "MOD" );
	if ( fh == FILESYSTEM_INVALID_HANDLE )
	{
		// Nothing on disk, we'll recreate everything from scratch...
		SetDirty( true );
		return true;
	}
	long fileTime = g_pFullFileSystem->GetFileTime( m_sRepositoryFileName, "MOD" );
	int size = g_pFullFileSystem->Size( fh );

	bool deletefile = false;

	if ( size > 1024 * 1024 )
	{
		InitLargeBuffer( fh, deletefile );
	}
	else
	{
		InitSmallBuffer( fh, size, deletefile );
	}

	if ( deletefile )
	{
		Assert( !m_bReadOnly );
		if ( !m_bReadOnly )
		{
			g_pFullFileSystem->RemoveFile( m_sRepositoryFileName, "MOD" );
		}
		SetDirty( true );
	}
	CheckDiskInfo( false, fileTime );
	return true;
}

template <class T>
void CUtlCachedFileData<T>::Save()
{
	char path[ 512 ];
	Q_strncpy( path, m_sRepositoryFileName, sizeof( path ) );
	Q_StripFilename( path );

	g_pFullFileSystem->CreateDirHierarchy( path, "MOD" );

	if ( g_pFullFileSystem->FileExists( m_sRepositoryFileName, "MOD" ) && 
		!g_pFullFileSystem->IsFileWritable( m_sRepositoryFileName, "MOD" ) )
	{
		g_pFullFileSystem->SetFileWritable( m_sRepositoryFileName, true, "MOD" );
	}

	// Now write to file
	FileHandle_t fh;
	fh = g_pFullFileSystem->Open( m_sRepositoryFileName, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE == fh )
	{
		ExecuteNTimes( 25, Warning( "Unable to persist cache '%s', check file permissions\n", m_sRepositoryFileName.String() ) );
	}
	else
	{
		SetDirty( false );

		int v = UTL_CACHE_SYSTEM_VERSION;
		g_pFullFileSystem->Write( &v, sizeof( v ), fh );
		v = m_nVersion;
		g_pFullFileSystem->Write( &v, sizeof( v ), fh );
		v = (int)m_uCurrentMetaChecksum;
		g_pFullFileSystem->Write( &v, sizeof( v ), fh );

		// Element count
		int c = Count();

		g_pFullFileSystem->Write( &c, sizeof( c ), fh );

		// Save repository back out to disk...
		CUtlBuffer buf( 0, 0, 0 );

		for ( int i = m_Elements.FirstInorder(); i != m_Elements.InvalidIndex(); i = m_Elements.NextInorder( i ) )
		{
			buf.SeekPut( CUtlBuffer::SEEK_HEAD, 0 );

			ElementType_t& element = m_Elements[ i ];

			char fn[ 512 ];
			g_pFullFileSystem->String( element.handle, fn, sizeof( fn ) );

			buf.PutString( fn );
			buf.PutInt( element.fileinfo );

			Assert( element.dataIndex != m_Data.InvalidIndex() );

			T *data = m_Data[ element.dataIndex ];

			Assert( data );

			((IBaseCacheInfo *)data)->Save( buf );

			int bufsize = buf.TellPut();
			g_pFullFileSystem->Write( &bufsize, sizeof( bufsize ), fh );
			g_pFullFileSystem->Write( buf.Base(), bufsize, fh );
		}

		g_pFullFileSystem->Close( fh );
	}

	if ( m_bSaveManifest )
	{
		SaveManifest();
	}
}

template <class T>
void CUtlCachedFileData<T>::Shutdown()
{
	if ( !m_bInitialized )
		return;

	m_bInitialized = false;

	if ( IsDirty() )
	{
		Save();
	}
	// No matter what, create the manifest if it doesn't exist on the HD yet
	else if ( m_bSaveManifest && !ManifestExists() )
	{
		SaveManifest();
	}

	m_Elements.RemoveAll();
}

template <class T>
bool CUtlCachedFileData<T>::ManifestExists()
{
	char manifest_name[ 512 ];
	Q_strncpy( manifest_name, m_sRepositoryFileName, sizeof( manifest_name ) );

	Q_SetExtension( manifest_name, ".manifest", sizeof( manifest_name ) );

	return g_pFullFileSystem->FileExists( manifest_name, "MOD" );
}

template <class T>
void CUtlCachedFileData<T>::SaveManifest()
{
	// Save manifest out to disk...
	CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

	for ( int i = m_Elements.FirstInorder(); i != m_Elements.InvalidIndex(); i = m_Elements.NextInorder( i ) )
	{
		ElementType_t& element = m_Elements[ i ];

		char fn[ 512 ];
		g_pFullFileSystem->String( element.handle, fn, sizeof( fn ) );

		buf.Printf( "\"%s\"\r\n", fn );
	}

	char path[ 512 ];
	Q_strncpy( path, m_sRepositoryFileName, sizeof( path ) );
	Q_StripFilename( path );

	g_pFullFileSystem->CreateDirHierarchy( path, "MOD" );

	char manifest_name[ 512 ];
	Q_strncpy( manifest_name, m_sRepositoryFileName, sizeof( manifest_name ) );

	Q_SetExtension( manifest_name, ".manifest", sizeof( manifest_name ) );

	if ( g_pFullFileSystem->FileExists( manifest_name, "MOD" ) && 
		!g_pFullFileSystem->IsFileWritable( manifest_name, "MOD" ) )
	{
		g_pFullFileSystem->SetFileWritable( manifest_name, true, "MOD" );
	}

	// Now write to file
	FileHandle_t fh;
	fh = g_pFullFileSystem->Open( manifest_name, "wb" );
	if ( FILESYSTEM_INVALID_HANDLE != fh )
	{
		g_pFullFileSystem->Write( buf.Base(), buf.TellPut(), fh );
		g_pFullFileSystem->Close( fh );

		// DevMsg( "Persisting cache manifest '%s' (%d entries)\n", manifest_name, c );
	}
	else
	{
		Warning( "Unable to persist cache manifest '%s', check file permissions\n", manifest_name );
	}
}

template <class T>
T *CUtlCachedFileData<T>::RebuildItem( const char *filename )
{
	int idx = GetIndex( filename );
	ElementType_t& e = m_Elements[ idx ];

	ForceRecheckDiskInfo();

	long cachefileinfo = e.fileinfo;
	// Set the disk fileinfo the first time we encounter the filename
	if ( e.diskfileinfo == UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO )
	{
		if ( m_bNeverCheckDisk )
		{
			e.diskfileinfo = cachefileinfo;
		}
		else
		{
			if ( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) 
			{
				e.diskfileinfo = g_pFullFileSystem->Size( filename, "GAME" );
				// Missing files get a disk file size of 0
				if ( e.diskfileinfo == -1 )
				{
					e.diskfileinfo = 0;
				}
			}
			else
			{
				e.diskfileinfo = g_pFullFileSystem->GetFileTime( filename, "GAME" );
			}
		}
	}

	Assert( e.dataIndex != m_Data.InvalidIndex() );

	T *data = m_Data[ e.dataIndex ];

	Assert( data );

	// Compare fileinfo to disk fileinfo and rebuild cache if out of date or not correct...
	if ( !m_bReadOnly )
	{
		RebuildCache( filename, data );
	}
	e.fileinfo = e.diskfileinfo;

	return data;
}

template <class T>
void CUtlCachedFileData<T>::RebuildCache( char const *filename, T *data )
{
	Assert( !m_bReadOnly );

	// Recache item, mark self as dirty
	SetDirty( true );

	((IBaseCacheInfo *)data)->Rebuild( filename );
}

template <class T>
void CUtlCachedFileData<T>::ForceRecheckDiskInfo()
{
	for ( int i = m_Elements.FirstInorder(); i != m_Elements.InvalidIndex(); i = m_Elements.NextInorder( i ) )
	{
		ElementType_t& element = m_Elements[ i ];
		element.diskfileinfo = UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO;
	}
}

class CSortedCacheFile
{
public:
	FileNameHandle_t	handle;
	int					index;

	 bool Less( const CSortedCacheFile &file0, const CSortedCacheFile &file1, void * )
	 {
		 char name0[ 512 ];
		 char name1[ 512 ];
		 g_pFullFileSystem->String( file0.handle, name0, sizeof( name0 ) );
		 g_pFullFileSystem->String( file1.handle, name1, sizeof( name1 ) );
		 return Q_stricmp( name0, name1 ) < 0 ? true : false;
	 }
};

// Iterates all entries and causes rebuild on any existing items which are out of date
template <class T>
void	CUtlCachedFileData<T>::CheckDiskInfo( bool forcerebuild, long cacheFileTime )
{
	char fn[ 512 ];
	int i;
	if ( forcerebuild )
	{
		for ( i = m_Elements.FirstInorder(); i != m_Elements.InvalidIndex(); i = m_Elements.NextInorder( i ) )
		{
			ElementType_t& element = m_Elements[ i ];
			g_pFullFileSystem->String( element.handle, fn, sizeof( fn ) );
			Get( fn );
		}
		return;
	}

	CUtlSortVector<CSortedCacheFile, CSortedCacheFile> list;
	for ( i = m_Elements.FirstInorder(); i != m_Elements.InvalidIndex(); i = m_Elements.NextInorder( i ) )
	{
		ElementType_t& element = m_Elements[ i ];
		CSortedCacheFile insert;
		insert.handle = element.handle;
		insert.index = i;
		list.InsertNoSort( insert );
	}
	list.RedoSort();

	if ( !list.Count() )
		return;

	for ( int listStart = 0, listEnd = 0; listStart < list.Count(); listStart = listEnd+1 )
	{
		int pathIndex = g_pFullFileSystem->GetPathIndex( m_Elements[list[listStart].index].handle );
		for ( listEnd = listStart; listEnd < list.Count(); listEnd++ )
		{
			ElementType_t& element = m_Elements[ list[listEnd].index ];

			int pathTest = g_pFullFileSystem->GetPathIndex( element.handle );
			if ( pathTest != pathIndex )
				break;
		}
		g_pFullFileSystem->String( m_Elements[list[listStart].index].handle, fn, sizeof( fn ) );
		Q_StripFilename( fn );
		bool bCheck = true;
		
		if ( m_bNeverCheckDisk )
		{
			bCheck = false;
		}
		else 
		{
			long pathTime = g_pFullFileSystem->GetPathTime( fn, "GAME" );
			bCheck = (pathTime > cacheFileTime) ? true : false;
		}

		for ( i = listStart; i < listEnd; i++ )
		{
			ElementType_t& element = m_Elements[ list[i].index ];

			if ( element.diskfileinfo == UTL_CACHED_FILE_DATA_UNDEFINED_DISKINFO )
			{
				if ( !bCheck )
				{
					element.diskfileinfo = element.fileinfo;
				}
				else
				{
					g_pFullFileSystem->String( element.handle, fn, sizeof( fn ) );
					if ( m_fileCheckType == UTL_CACHED_FILE_USE_FILESIZE ) 
					{
						element.diskfileinfo = g_pFullFileSystem->Size( fn, "GAME" );

						// Missing files get a disk file size of 0
						if ( element.diskfileinfo == -1 )
						{
							element.diskfileinfo = 0;
						}
					}
					else
					{
						element.diskfileinfo = g_pFullFileSystem->GetFileTime( fn, "GAME" );
					}
				}
			}
		}
	}
}

#include "tier0/memdbgoff.h"

#endif // UTLCACHEDFILEDATA_H
