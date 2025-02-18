//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#ifdef _WIN32
#include <process.h>
#include <io.h>
#endif
#include <stddef.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "tier1/utlbuffer.h"
#include "tier1/strtools.h"
#include "tier2/riff.h"

#if defined( _WIN32 ) && !defined( _X360 )
#include <windows.h>
#endif

#ifdef MAKE_GAMEDATA_TOOL
	#include "../public/materialsystem/shader_vcs_version.h"
	#include "../public/materialsystem/imaterial.h"
	#include "../public/materialsystem/hardwareverts.h"
	#include "../public/vtf/vtf.h"
#else
	#include "materialsystem/shader_vcs_version.h"
	#include "materialsystem/imaterial.h"
	#include "materialsystem/hardwareverts.h"
#endif

#include "xwvfile.h"
#include "xzp.h"

CByteswap g_xzpSwap;
extern IFileReadBinary *g_pSndIO;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Datadesc blocks for byteswapping:
//-----------------------------------------------------------------------------
BEGIN_BYTESWAP_DATADESC( xZipHeader_t )
	DEFINE_FIELD( Magic, FIELD_INTEGER ),
	DEFINE_FIELD( Version, FIELD_INTEGER ),
	DEFINE_FIELD( PreloadDirectoryEntries, FIELD_INTEGER ),
	DEFINE_FIELD( DirectoryEntries, FIELD_INTEGER ),
	DEFINE_FIELD( PreloadBytes, FIELD_INTEGER ),
	DEFINE_FIELD( HeaderLength, FIELD_INTEGER ),
	DEFINE_FIELD( FilenameEntries, FIELD_INTEGER ),
	DEFINE_FIELD( FilenameStringsOffset, FIELD_INTEGER ),
	DEFINE_FIELD( FilenameStringsLength, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( xZipDirectoryEntry_t )
	DEFINE_FIELD( FilenameCRC, FIELD_INTEGER ),
	DEFINE_FIELD( Length, FIELD_INTEGER ),
	DEFINE_FIELD( StoredOffset, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( xZipFilenameEntry_t )
	DEFINE_FIELD( FilenameCRC, FIELD_INTEGER ),
	DEFINE_FIELD( FilenameOffset, FIELD_INTEGER ),
	DEFINE_FIELD( TimeStamp, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

BEGIN_BYTESWAP_DATADESC( xZipFooter_t )
	DEFINE_FIELD( Size, FIELD_INTEGER ),
	DEFINE_FIELD( Magic, FIELD_INTEGER ),
END_BYTESWAP_DATADESC()

CXZip::CXZip()
{
	// Ensure that the header doesn't contain a valid magic yet.
	m_Header.Magic = 0;	
	m_pPreloadedData = NULL;
	m_nPreloadStart = 0;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;

	m_bByteSwapped = false;

	m_pFilenames = NULL;
	m_hZip = NULL;
	
	m_pRead = NULL;
	m_hUser = 0;
	m_nMonitorLevel = 0;
}

CXZip::CXZip( const char* filename )
{
	// Ensure that the header doesn't contain a valid magic yet.
	m_Header.Magic = 0;	
	m_nPreloadStart = 0;
	m_pPreloadedData = NULL;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;

	m_bByteSwapped = false;

	m_pFilenames = NULL;
	m_hZip = NULL;

	m_pRead = NULL;
	m_hUser = 0;
	m_nMonitorLevel = 0;

	Load( filename );
}

CXZip::CXZip( FILE* handle, int offset, int size )	// file handle and offset of the zip file
{
	m_pRead = NULL;
	m_hUser = 0;
	m_nPreloadStart = 0;
	m_pDirectory = NULL;
	m_pPreloadDirectory = NULL;
	m_nRegular2PreloadEntryMapping = NULL;

	m_bByteSwapped = false;

	m_pFilenames = NULL;
	m_pPreloadedData = NULL;
	m_nMonitorLevel = 0;

	Load( handle, offset, size );
}

CXZip::~CXZip()
{
	Unload();
}

bool CXZip::InstallAlternateIO( int (*read)( void* buffer, int offset, int length, int nDestLength, int hUser), int hUser )
{
	m_pRead = read;
	m_hUser = hUser;
	return true;
}


// Loads an xZip file into memory:
bool CXZip::Load( const char* filename, bool bPreload )
{
	FILE* hZip = fopen( filename, "rb" );
	fseek(hZip,0,SEEK_END);
	int nSize = ftell( hZip );
	return Load( hZip, 0, nSize, bPreload );
}

bool CXZip::Load( FILE* handle, int nOffset, int nSize, bool bPreload )	// Load a pack file into this instance.  Returns true on success.
{
	Unload();

	m_bByteSwapped = false;

	m_hZip = handle;
	m_nOffset = nOffset;
	m_nSize = nSize;

	// Hacky, clean up:
	if( m_hZip && !m_pRead )
	{
		InstallAlternateIO( defaultRead, (int)m_hZip );
	}

	if( m_hZip == NULL && m_pRead == NULL )
	{
		return false;
	}

	// Read the header:
	m_pRead( &m_Header, 0, -1, sizeof(m_Header), m_hUser );

	// Validate the Magic number and at the same time determine if I am reading a regular or swappped xZip file:
	switch( m_Swap.SourceIsNativeEndian<int>( m_Header.Magic, xZipHeader_t::MAGIC ) )
	{
		// Does the magic match exactly?
		case 1:
			m_Swap.ActivateByteSwapping( false );
			m_bByteSwapped = false;
			break;

		// Does the magic match, but is swapped?
		case 0:
			m_bByteSwapped = true;
			m_Swap.ActivateByteSwapping( true );	// We must be reading the opposite endianness.
			m_Swap.SwapFieldsToTargetEndian<xZipHeader_t>( &m_Header );
			break;

		default: 
			assert( 0 );
			// Fail gently in release:
		
		// The magic doesn't match in any respect:
		case -1:
			{
				printf("Invalid xZip file\n");

				if( m_hZip )
				{
					fclose( m_hZip );
					m_hZip = NULL;
				}
				return false;
			}
	}

	// Validate the archive version:
	if( m_Header.Version != xZipHeader_t::VERSION )
	{
		// Backward compatable support for version 1
		Msg("Incorrect xZip version found %u - expected %u\n", m_Header.Version, xZipHeader_t::VERSION );
		if( m_hZip )
		{
			fclose( m_hZip );
			m_hZip = NULL;
		}

		m_Header.Magic = xZipHeader_t::FREE;
		return false;
	}

	// Read the directory:
	{
		MEM_ALLOC_CREDIT();

		m_pDirectory = (xZipDirectoryEntry_t*)malloc( sizeof(xZipDirectoryEntry_t) * m_Header.DirectoryEntries );
		m_pRead( m_pDirectory, m_Header.HeaderLength, -1, sizeof( xZipDirectoryEntry_t ) * m_Header.DirectoryEntries, m_hUser );

		// Swap the directory entries if nessecary
		if( m_bByteSwapped )
		{
			for( unsigned nDirectoryEntry = 0; nDirectoryEntry < m_Header.DirectoryEntries; nDirectoryEntry++ )
			{
				m_Swap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>(  &( m_pDirectory[nDirectoryEntry] ) );
			}
		}


		m_nPreloadStart = m_Header.HeaderLength + ( sizeof( xZipDirectoryEntry_t ) * m_Header.DirectoryEntries );
	}

	// Preload the preload chunk if desired:
	if( bPreload )
	{
		PreloadData();
	}

	return true;
}

void CXZip::Unload()
{
	DiscardPreloadedData();

	// Dump the directory:
	if( m_pDirectory )
	{
		free( m_pDirectory );
		m_pDirectory = NULL;
	}

	if( m_pFilenames )
	{
		free( m_pFilenames );
		m_pFilenames = NULL;
	}

	// Invalidate the header:
	m_Header.Magic = 0;

	if( m_hZip )
	{
		fclose( m_hZip );
		m_hZip = NULL;
	}

}

//-----------------------------------------------------------------------------
// CXZip::PreloadData
//
// Loads the preloaded data if it isn't already.
//-----------------------------------------------------------------------------

void CXZip::PreloadData()
{
	Assert( IsValid() );

	// Ensure it isn't already preloaded
	if( m_pPreloadedData )
		return;

	// If I don't have a preloaded section, ignore the request.
	if( !m_Header.PreloadBytes || !m_Header.PreloadDirectoryEntries )
		return;

	// Allocate and read the data block in:
#ifndef _X360
	MEM_ALLOC_CREDIT_( "xZip" );
	m_pPreloadedData = malloc( m_Header.PreloadBytes );

	// Just drop out if allocation fails;
	if ( !m_pPreloadedData )
		return;

	m_pRead( m_pPreloadedData, m_nPreloadStart, -1, m_Header.PreloadBytes, m_hUser );
#else
	int nAlignedStart = AlignValue( ( m_nPreloadStart - XBOX_HDD_SECTORSIZE ) + 1, XBOX_HDD_SECTORSIZE );
	int nBytesToRead = AlignValue( ( m_nPreloadStart - nAlignedStart ) + m_Header.PreloadBytes, XBOX_HDD_SECTORSIZE );
	int nBytesBuffer = AlignValue( nBytesToRead, XBOX_HDD_SECTORSIZE );
	byte *pReadData = (byte *)malloc( nBytesBuffer );
	
	// Just drop out if allocation fails;
	if ( !pReadData )
		return; 
	
	MEM_ALLOC_CREDIT_( "xZip" );
	m_pRead( pReadData, nAlignedStart, nBytesBuffer,nBytesToRead, m_hUser );
	m_pPreloadedData = pReadData + ( m_nPreloadStart - nAlignedStart );
#endif

	// Set up the preload directory:
	m_pPreloadDirectory = (xZipDirectoryEntry_t*)m_pPreloadedData;

	// Swap the preload directory:
	if ( m_bByteSwapped )
	{
		for ( unsigned nDirectoryEntry = 0; nDirectoryEntry < m_Header.PreloadDirectoryEntries; nDirectoryEntry++ )
		{
			m_Swap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>( &( m_pPreloadDirectory[nDirectoryEntry] ) );
		}
	}

	// Set up the regular 2 preload mapping section:
	m_nRegular2PreloadEntryMapping = (unsigned short*)(((unsigned char*)m_pPreloadDirectory) + ( sizeof(xZipDirectoryEntry_t) * m_Header.PreloadDirectoryEntries ));

	// Swap the regular to preload mapping
	if ( m_bByteSwapped )
	{
		m_Swap.SwapBufferToTargetEndian<short>( (short *)m_nRegular2PreloadEntryMapping,  (short *)m_nRegular2PreloadEntryMapping, m_Header.DirectoryEntries );
	}

}

//-----------------------------------------------------------------------------
// CXZip::DiscardPreloadedData
//
// frees the preloaded data cache if it's present.
//-----------------------------------------------------------------------------

void CXZip::DiscardPreloadedData()
{
	if ( m_pPreloadedData )
	{
#ifndef _X360
		free( m_pPreloadedData );
#else
		int nAlignedStart = AlignValue( ( m_nPreloadStart - XBOX_HDD_SECTORSIZE ) + 1, XBOX_HDD_SECTORSIZE );
		byte *pReadData = (byte *)m_pPreloadedData - ( m_nPreloadStart - nAlignedStart );
		free( pReadData );
#endif
		m_pPreloadedData = NULL;
		m_pPreloadDirectory = NULL;
		m_nRegular2PreloadEntryMapping = NULL;
	}
}

int CXZip::defaultRead( void* buffer, int offset, int destLength, int length, int hUser)
{
	fseek( (FILE*)hUser, offset, SEEK_SET );
	return fread( buffer, 1, length, (FILE*)hUser );
}

char* CXZip::GetEntryFileName( unsigned CRC, char* pDefault )
{
	Assert( IsValid() );

	if( IsRetail() )
	{
		return pDefault;
	}
	else
	{

		// Make sure I have a filename section:
		if( m_Header.FilenameStringsOffset == 0 || m_Header.FilenameEntries == 0 || CRC == 0 )
		{
			return pDefault;
		}

		// If the filename chunk isn't here, load it up:
		if( !m_pFilenames )
		{
			MEM_ALLOC_CREDIT_("xZip");
			m_pFilenames = (xZipFilenameEntry_t*)malloc( m_Header.FilenameStringsLength );
			m_pRead( m_pFilenames, m_Header.FilenameStringsOffset, -1, m_Header.FilenameStringsLength, m_hUser );

			// TODO: Swap!
			for( unsigned int i=0; i< m_Header.FilenameEntries;i++ )
			{
				m_Swap.SwapFieldsToTargetEndian<xZipFilenameEntry_t>(&m_pFilenames[i]);
			}
		}

		// Find this entry in the preload directory
		xZipFilenameEntry_t entry;
		entry.FilenameCRC = CRC;

		xZipFilenameEntry_t* found = (xZipFilenameEntry_t*)bsearch( &entry, m_pFilenames, m_Header.FilenameEntries, sizeof(xZipFilenameEntry_t), xZipFilenameEntry_t::xZipFilenameEntryCompare );

		if( !found ) 
			return pDefault;

		return (((char*)m_pFilenames) + found->FilenameOffset) - m_Header.FilenameStringsOffset; 	
	}
}

// Sanity checks that the zip file is ready and readable:
bool CXZip::IsValid()
{
	if( m_Header.Magic != xZipHeader_t::MAGIC )
		return false;

	if( m_Header.Version > xZipHeader_t::VERSION )
		return false;

	if( !m_pDirectory )
		return false;

	return true;
}

void CXZip::WarningDir()
{
	Assert( IsValid());
	
	for( unsigned i = 0; i< m_Header.DirectoryEntries; i++ )
	{
		Msg( GetEntryFileName( m_pDirectory[i].FilenameCRC ) );
	}
}


int CXZip::ReadIndex( int nEntryIndex, int nFileOffset, int nDestBytes, int nLength, void* pBuffer )
{
	Assert( IsValid() );

	if( nLength <=0 || nEntryIndex < 0 ) 
		return 0;

	// HACK HACK HACK - convert the pack file index to a local file index (ie, assuming the full file index is being passed in)
	nFileOffset -= m_pDirectory[nEntryIndex].StoredOffset;
	// HACK HACK HACK

	// If I've got my preload section loaded, first check there:
	xZipDirectoryEntry_t* pPreloadEntry = GetPreloadEntry(nEntryIndex);

	if( pPreloadEntry )
	{
		Assert( pPreloadEntry->FilenameCRC == m_pDirectory[nEntryIndex].FilenameCRC );

		if( nFileOffset + nLength <= (int)pPreloadEntry->Length )
		{
			if( m_nMonitorLevel >= 2 )
			{
				char* filename = GetEntryFileName( m_pDirectory[nEntryIndex].FilenameCRC, "(!!! unknown !!!)" );

				Msg("PACK(preload) %s: length:%i offset:%i",filename,nLength, nFileOffset);

			}

			memcpy( pBuffer, (char*)m_pPreloadedData + pPreloadEntry->StoredOffset + nFileOffset - m_nPreloadStart, nLength );
			return nLength;
		}
	}

	// Offset int the zip to start the read:
	int ZipOffset = m_pDirectory[nEntryIndex].StoredOffset + nFileOffset;
	int nBytesRead = m_pRead( pBuffer, ZipOffset, nDestBytes, nLength, m_hUser);

	if( m_nMonitorLevel )
	{
		char* filename = GetEntryFileName( m_pDirectory[nEntryIndex].FilenameCRC, "(!!! unknown !!!)" );

		unsigned preload = 0;
		if( m_pPreloadedData && m_nRegular2PreloadEntryMapping[nEntryIndex] != 0xFFFF )
		{
			// Find this entry in the preload directory
			xZipDirectoryEntry_t* entry = &(m_pPreloadDirectory[m_nRegular2PreloadEntryMapping[nEntryIndex]]);
			Assert(entry->FilenameCRC == m_pDirectory[nEntryIndex].FilenameCRC);

			preload = entry->Length;
		}

		Msg("PACK %s: length:%i offset:%i (preload bytes:%i)",filename,nLength, nFileOffset, preload);
	}
	
	return nBytesRead;
}

bool CXZip::GetSimpleFileOffsetLength( const char* FileName, int& nBaseIndex, int &nFileOffset, int &nLength )
{
	Assert( IsValid() );

	xZipDirectoryEntry_t entry;
	entry.FilenameCRC = xZipCRCFilename( FileName );

	xZipDirectoryEntry_t* found = (xZipDirectoryEntry_t*)bsearch( &entry, m_pDirectory, m_Header.DirectoryEntries, sizeof(xZipDirectoryEntry_t), xZipDirectoryEntry_t::xZipDirectoryEntryFindCompare );

	if( found == NULL )
		return false;

	nFileOffset = found[0].StoredOffset;
	nLength = found[0].Length;
	nBaseIndex = (((int)((char*)found - (char*)m_pDirectory))/sizeof(xZipDirectoryEntry_t));

	return true;
}

bool CXZip::ExtractFile( const char* FileName )
{
	return false;
}

// Compares to xZipDirectoryEntries.
//
// Sorts in the following order:  
//		FilenameCRC
//		FileOffset
//		Length
//		StoredOffset
//
// The sort function may look overly complex, but it is actually useful for locating different pieces of 
// the same file in a meaningful order.
//
int __cdecl xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare( const void* left, const void* right )
{
	xZipDirectoryEntry_t *l = (xZipDirectoryEntry_t*)left,
					     *r = (xZipDirectoryEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	// else l->FileOffset == r->FileOffset
	if( l->Length < r->Length )
	{
		return -1;
	}
	else if( l->Length > r->Length )
	{
		return 1;
	}

	// else l->Length == r->Length
	if( l->StoredOffset < r->StoredOffset )
	{
		return -1;
	}
	else if( l->StoredOffset > r->StoredOffset )
	{
		return 1;
	}

	// else everything is identical:
	return 0;

}

// Find an entry with matching CRC only
int __cdecl xZipDirectoryEntry_t::xZipDirectoryEntryFindCompare( const void* left, const void* right )
{
	xZipDirectoryEntry_t *l = (xZipDirectoryEntry_t*)left,
					     *r = (xZipDirectoryEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	return 0;

}

int __cdecl xZipFilenameEntry_t::xZipFilenameEntryCompare( const void* left, const void* right )
{
	xZipFilenameEntry_t *l = (xZipFilenameEntry_t*)left,
					    *r = (xZipFilenameEntry_t*)right;

	if( l->FilenameCRC < r->FilenameCRC )
	{
		return -1;
	}

	else if( l->FilenameCRC > r->FilenameCRC )
	{
		return 1;
	}

	return 0;

}


// CRC's an individual xZip filename:
unsigned xZipCRCFilename( const char* filename )
{
	unsigned hash = 0xAAAAAAAA;	// Alternating 1's and 0's

	for( ; *filename ; filename++ )
	{
		char c = *filename;
	
		// Fix slashes
		if( c == '/' )
			c = '\\';
		else
			c = (char)tolower(c);

		hash = hash * 33 + c;
	}

	return hash;
}

#if defined( MAKE_GAMEDATA_TOOL )

// ------------
xZipHeader_t Header;
xZipDirectoryEntry_t	 *pDirectoryEntries = NULL;
xZipDirectoryEntry_t	 *pPreloadDirectoryEntries = NULL;
xZipFilenameEntry_t		 *pFilenameEntries = NULL;
char					 *pFilenameData = NULL;
unsigned				  nFilenameDataLength = 0;

unsigned			     InputFileBytes = 0;

char* CleanFilename( char* filename )
{
	// Trim leading white space:
	while( isspace(*filename) )
		filename++;

	// Trim trailing white space:
	while( isspace( filename[strlen(filename)-1] ) )
	{
		filename[strlen(filename)-1] = '\0';
	}

	return filename;
}


bool CopyFileBytes( FILE* hDestination, FILE* hSource, unsigned nBytes )
{
	char buffer[16384];

	while( nBytes > 0 )
	{
		int nBytesRead = fread( buffer, 1, nBytes > sizeof(buffer) ? sizeof(buffer) : nBytes,  hSource );
		fwrite(buffer, 1, nBytesRead,  hDestination );
		nBytes -= nBytesRead;
	}

	return true;
}

bool WriteFileBytes( FILE* hDestination, CUtlBuffer &source, unsigned nBytes )
{
	unsigned int nBytesWritten = fwrite(source.Base(), 1, nBytes,  hDestination );
	return (nBytesWritten == nBytes);
}

void PadFileBytes(FILE* hFile, int nPreloadPadding )
{
	if( nPreloadPadding < 0 || nPreloadPadding >= 512)
	{
		puts("Invalid padding");
		return;
	}
	
	char padding[512];
	memset(padding,0,nPreloadPadding);
	fwrite(padding,1,nPreloadPadding,hFile);
}

void AddFilename( const char* filename )
{
	unsigned CRCfilename = xZipCRCFilename( filename );

	// If we already have this filename don't add it again:
	for( int i = 0; i < (int)Header.FilenameEntries; i++ )
	{
		if( pFilenameEntries[i].FilenameCRC == CRCfilename )
		{
			return;
		}
	}

	Header.FilenameEntries++;
	
	// Add the file to the file string table:
	pFilenameEntries = (xZipFilenameEntry_t*)realloc( pFilenameEntries, sizeof(xZipFilenameEntry_t) * Header.FilenameEntries );
	
	int filenameLength = (int)strlen(filename) + 1;
	pFilenameEntries[Header.FilenameEntries-1].FilenameCRC = CRCfilename;
	pFilenameEntries[Header.FilenameEntries-1].FilenameOffset = nFilenameDataLength;

	// Grab the timestamp for the file:
	struct stat buf;
	if( stat( filename, &buf ) != -1 )
	{
		pFilenameEntries[Header.FilenameEntries - 1].TimeStamp = buf.st_mtime;
	}
	else
	{
		pFilenameEntries[Header.FilenameEntries - 1].TimeStamp = 0;
	}

	nFilenameDataLength += filenameLength;
	pFilenameData = (char*)realloc(pFilenameData, nFilenameDataLength);
	memcpy(pFilenameData + nFilenameDataLength - filenameLength, filename, filenameLength);
}

FILE* hTempFilePreload;
FILE* hTempFileData;
FILE* hOutputFile;

bool xZipAddFile( const char* filename, CUtlBuffer &fileBuff, bool bPrecacheEntireFile, bool bProcessPrecacheHeader, bool bProcessPrecacheHeaderOnly )
{
	unsigned int fileSize = fileBuff.TellMaxPut();

	// Track total input bytes for stats reasons
	InputFileBytes += fileSize;

	unsigned customPreloadSize = 0;
	
	if( bPrecacheEntireFile  )
	{
		customPreloadSize = fileSize;
	} 
	else if( bProcessPrecacheHeader )
	{
		customPreloadSize = xZipComputeCustomPreloads( filename );
	}
	else if( bProcessPrecacheHeaderOnly )
	{
		customPreloadSize = xZipComputeCustomPreloads( filename );
		fileSize = min( fileSize, customPreloadSize );
	}

	unsigned CRC = xZipCRCFilename( filename );

	// Does this file have a split header?
	if( customPreloadSize > 0  ) 
	{
		// Initialize the entry header:
		xZipDirectoryEntry_t entry;
		memset( &entry, 0, sizeof( entry ) );
		
		entry.FilenameCRC = CRC;
		entry.Length = customPreloadSize;
		entry.StoredOffset = ftell(hTempFilePreload);

		// Add the directory entry to the preload table:
		Header.PreloadDirectoryEntries++;
		pPreloadDirectoryEntries = (xZipDirectoryEntry_t*)realloc( pPreloadDirectoryEntries, sizeof( xZipDirectoryEntry_t ) * Header.PreloadDirectoryEntries );
		memcpy( pPreloadDirectoryEntries + Header.PreloadDirectoryEntries - 1, &entry, sizeof( entry ) );

		// Concatenate the data in the preload file:
		fileBuff.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
		WriteFileBytes( hTempFilePreload, fileBuff, entry.Length );
		fileBuff.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );

		
		// Add the filename entry:
		AddFilename( filename );

		// Spew it:
		printf("+Preload: \"%s\": Length:%u\n", filename, entry.Length );
	}

	// Copy the file to the regular data region:
	xZipDirectoryEntry_t entry;
	memset(&entry,0,sizeof(entry));
	entry.FilenameCRC = CRC;
	entry.Length = fileSize;
	entry.StoredOffset = ftell(hTempFileData);		

	// Add the directory entry to the table:
	Header.DirectoryEntries++;
	pDirectoryEntries = (xZipDirectoryEntry_t*)realloc( pDirectoryEntries, sizeof( xZipDirectoryEntry_t ) * Header.DirectoryEntries );
	memcpy( pDirectoryEntries + Header.DirectoryEntries - 1, &entry, sizeof( entry ) );

	WriteFileBytes( hTempFileData, fileBuff, entry.Length );

	// Align the data region to a 512 byte boundry:  (has to be on last entry as well to ensure enough space to perform the final read,
	// and initial alignment is taken careof by assembexzip)
	int nPadding = ( XBOX_HDD_SECTORSIZE - ( ftell( hTempFileData ) % XBOX_HDD_SECTORSIZE) ) % XBOX_HDD_SECTORSIZE;

	PadFileBytes( hTempFileData, nPadding );

	// Add the file to the file string table:
	AddFilename( filename );

	// Print a summary
	printf("+File: \"%s\": Length:%u Padding:%i\n", filename,  entry.Length, nPadding );

	return true;
}

bool xZipAddFile( const char* zipname, bool bPrecacheEntireFile, bool bProcessPrecacheHeader, bool bProcessPrecacheHeaderOnly )
{
	// Clean up the filename:
	char buffer[MAX_PATH];
	strcpy(buffer, zipname);

	// Fix slashes and convert it to lower case:
	char *filename;
	for( filename = buffer; *filename; filename++ )
	{
		if( *filename == '/' )
			*filename = '\\';
		else
		{
			*filename = (char)tolower(*filename);
		}
	}

	// Skip leading white space:
	for( filename = buffer; isspace(*filename); filename++ )
		;

	// Obliterate trailing white space:
	for(;;)
	{
		int len = (int)strlen( filename );
		if( len <= 0 )
		{
			printf("!!!! BAD FILENAME: \"%s\"\n", filename );
			return false;
		}
		
		if( isspace( filename[len-1] ) )
			filename[len-1]='\0';
		else
			break;
	}

	// Ensure we don't already have this file:
	unsigned CRC = xZipCRCFilename( filename );

	for( unsigned i=0; i < Header.DirectoryEntries; i++ )
	{
		if( pDirectoryEntries[i].FilenameCRC == CRC )
		{
			printf("!!!! NOT ADDING DUPLICATE FILENAME: \"%s\"\n", filename );
			return false;
		}
	}

	// Attempt to open the file:
	FILE* hFile = fopen( filename, "rb" );
	if( !hFile )
	{
		printf("!!!! FAILED TO OPEN FILE: \"%s\"\n", filename );
		return false;
	}

	// Get the length of the file:
	fseek(hFile,0,SEEK_END);
	unsigned fileSize = ftell(hFile);
	fseek(hFile,0,SEEK_SET);

	CUtlBuffer fileBuff;
	fileBuff.EnsureCapacity( fileSize );
	fread( fileBuff.Base(), fileSize, 1, hFile );
	fclose( hFile );

	fileBuff.SeekPut( CUtlBuffer::SEEK_HEAD, fileSize );

	return xZipAddFile( zipname, fileBuff, bPrecacheEntireFile, bProcessPrecacheHeader, bProcessPrecacheHeaderOnly );
}

int xZipBegin( const char* fileNameXzip )
{
	// Create and initialize the header:
	memset( &Header, 0, sizeof(Header) );	// Zero out the header:
	Header.Magic = xZipHeader_t::MAGIC;
	Header.Version = xZipHeader_t::VERSION;
	Header.HeaderLength = sizeof(Header);

	// Open the output file:
	hOutputFile = fopen(fileNameXzip,"wb+");
	if( !hOutputFile )
	{
		printf("Failed to open \"%s\" for writing.\n", fileNameXzip);
		exit( EXIT_FAILURE);
	}
	
	// Create a temporary file for storing the preloaded data:
	hTempFilePreload = tmpfile();
	if( !hTempFilePreload )
	{
		printf( "Error: failed to create temporary file\n" );
		return EXIT_FAILURE;
	}

	// Create a temporary file for storing the non preloaded data
	hTempFileData = tmpfile();
	if( !hTempFileData )
	{
		printf( "Error: failed to create temporary file\n");
		return EXIT_FAILURE;
	}
	
	return EXIT_SUCCESS;
}

bool xZipEnd()
{
	int nPreloadDirectorySize = sizeof(xZipDirectoryEntry_t)*Header.PreloadDirectoryEntries;
	int nRegular2PreloadSize = sizeof(unsigned short) * Header.DirectoryEntries;

	// Compute the size of the preloaded section:
	if( Header.PreloadDirectoryEntries )
	{
		fseek( hTempFilePreload, 0, SEEK_END );
		Header.PreloadBytes = ftell(hTempFilePreload) + nPreloadDirectorySize + nRegular2PreloadSize;	// Raw# of bytes to preload
		fseek( hTempFilePreload, 0, SEEK_SET );
	}
	else
	{
		Header.PreloadBytes = 0;
	}

	// Number of bytes preceeding the preloaded section:
	int nPreloadOffset = sizeof( Header ) + ( sizeof( xZipDirectoryEntry_t ) * Header.DirectoryEntries );

	// Number of bytes to pad between the end of the preload section and the start of the data section:
	int nPadding = ( 512 - ( ( nPreloadOffset + Header.PreloadBytes ) % 512) ) %512;				// Number of alignment bytes after the preload section

	// Offset past the preload section:
	int nDataOffset = nPreloadOffset + Header.PreloadBytes + nPadding;

	// Write out the header: (will need to be rewritten at the end as well) - note: not even bothering to byteswap at this point
	fwrite(&Header,sizeof(Header),1,hOutputFile);


	// Fixup each of the directory entries to make them relative to the beginning of the file.
	for( unsigned i=0; i< Header.DirectoryEntries;i++ )
	{
		xZipDirectoryEntry_t* pDir = &(pDirectoryEntries[i]);

		// Adjust files in the regular data area:
		pDir->StoredOffset = nDataOffset + pDir->StoredOffset;
	}

	// Sort and write the directory:
	printf("Sorting and writing %i directory entries...\n",Header.DirectoryEntries);
	qsort(pDirectoryEntries,Header.DirectoryEntries,sizeof(xZipDirectoryEntry_t),&xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare); 

	// Swap the directory entries:
	for( unsigned i=0; i < Header.DirectoryEntries; i++ )
	{
		g_xzpSwap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>(&pDirectoryEntries[i]);
	}

	fwrite(pDirectoryEntries,Header.DirectoryEntries*sizeof(xZipDirectoryEntry_t),1, hOutputFile);

	// Swap the directory back for later use:
	for( unsigned i=0; i < Header.DirectoryEntries; i++ )
	{
		g_xzpSwap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>(&pDirectoryEntries[i]);
	}

	// Copy the preload section:
	if( Header.PreloadBytes > 0 )
	{
		printf("Generating the preload section...(%u)\n", Header.PreloadBytes);


		// Fixup each of the directory entries to make them relative to the beginning of the file.
		for( unsigned i=0; i< Header.PreloadDirectoryEntries;i++ )
		{
			xZipDirectoryEntry_t* pDir = &(pPreloadDirectoryEntries[i]);

			// Shift preload data down by preload bytes (and skipping over the directory):
			pDir->StoredOffset += nPreloadOffset + nPreloadDirectorySize + nRegular2PreloadSize;
		}

		printf("Sorting %u preload directory entries...\n",Header.PreloadDirectoryEntries);
		qsort(pPreloadDirectoryEntries,Header.PreloadDirectoryEntries,sizeof(xZipDirectoryEntry_t),&xZipDirectoryEntry_t::xZipDirectoryEntrySortCompare); 

		printf("Building regular to preload mapping table for %u entries...\n", Header.DirectoryEntries );
		unsigned short* Regular2Preload = (unsigned short*)malloc( nRegular2PreloadSize );
		for( unsigned i = 0; i < Header.DirectoryEntries; i++ )
		{
			unsigned short j;
			for( j = 0; j < Header.PreloadDirectoryEntries; j++ )
			{
				if( pDirectoryEntries[i].FilenameCRC == pPreloadDirectoryEntries[j].FilenameCRC )
					break;
			}

			// If I couldn't find it mark it as non-existant:
			if( j == Header.PreloadDirectoryEntries )
				j = 0xFFFF;

			Regular2Preload[i] = j;
		}

		printf("Writing preloaded directory entreis...\n" );

		// Swap the preload directory entries:
		for( unsigned i=0; i < Header.PreloadDirectoryEntries; i++ )
		{
			g_xzpSwap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>(&pPreloadDirectoryEntries[i]);
		}

		fwrite( pPreloadDirectoryEntries, Header.PreloadDirectoryEntries*sizeof(xZipDirectoryEntry_t),1, hOutputFile );

		// Swap them back:
		for( unsigned i=0; i < Header.PreloadDirectoryEntries; i++ )
		{
			g_xzpSwap.SwapFieldsToTargetEndian<xZipDirectoryEntry_t>(&pPreloadDirectoryEntries[i]);
		}

		printf("Writing regular to preload mapping (%u bytes)...\n", sizeof(unsigned short)*Header.DirectoryEntries );
		
		// Swap regular to preload mapping:
		g_xzpSwap.SwapBufferToTargetEndian<short>((short*)Regular2Preload, (short*)Regular2Preload, nRegular2PreloadSize / sizeof(short) );

		fwrite( Regular2Preload, nRegular2PreloadSize,1,hOutputFile );

		// Swap it back
		g_xzpSwap.SwapBufferToTargetEndian<short>((short*)Regular2Preload, (short*)Regular2Preload, nRegular2PreloadSize / sizeof(short) );

		printf("Copying %u Preloadable Bytes...\n", Header.PreloadBytes - nPreloadDirectorySize - nRegular2PreloadSize );
		fseek(hTempFilePreload,0,SEEK_SET);
		CopyFileBytes(hOutputFile, hTempFilePreload, Header.PreloadBytes - nPreloadDirectorySize - nRegular2PreloadSize  );
	}

	// Align the data section following the preload section:
	if( nPadding )
	{
		printf("Aligning Data Section Start by %u bytes...\n", nPadding );
		PadFileBytes(hOutputFile, nPadding );
	}

	// Copy the data section:
	fseek(hTempFileData, 0, SEEK_END );
	unsigned length = ftell( hTempFileData );
	fseek(hTempFileData, 0, SEEK_SET );
	printf("Copying %u Bytes...\n",length);

	CopyFileBytes(hOutputFile, hTempFileData, length);

	// Write out the filename data if present:
	if( nFilenameDataLength && Header.FilenameEntries )
	{
		Header.FilenameStringsOffset = ftell(hOutputFile);
		Header.FilenameStringsLength = (Header.FilenameEntries*sizeof(xZipFilenameEntry_t)) + nFilenameDataLength;
		
		// Adjust the offset in each of the filename offsets to absolute position in the file.
		for( unsigned i=0;i<Header.FilenameEntries;i++ )
		{
			pFilenameEntries[i].FilenameOffset += ( Header.FilenameStringsOffset + (Header.DirectoryEntries*sizeof(xZipFilenameEntry_t)));
		}

		printf("Sorting and writing %u filename directory entries...\n",Header.FilenameEntries);

		// Sort the data:
		qsort(pFilenameEntries,Header.FilenameEntries,sizeof(xZipFilenameEntry_t),&xZipFilenameEntry_t::xZipFilenameEntryCompare); 

		// Write the data out:
		for( unsigned int i = 0; i < Header.FilenameEntries; i++ )
		{
			g_xzpSwap.SwapFieldsToTargetEndian<xZipFilenameEntry_t>(&pFilenameEntries[i]);
		}

		fwrite(pFilenameEntries,1,Header.FilenameEntries*sizeof(xZipFilenameEntry_t),hOutputFile);

		// Swap them back:
		for( unsigned int i = 0; i < Header.FilenameEntries; i++ )
		{
			g_xzpSwap.SwapFieldsToTargetEndian<xZipFilenameEntry_t>(&pFilenameEntries[i]);
		}

		printf("Writing %u bytes of filename data...\n",nFilenameDataLength);
		fwrite(pFilenameData,1,nFilenameDataLength,hOutputFile);
	}

	// Compute the total file size, including the size of the footer:
	unsigned OutputFileBytes = ftell(hOutputFile) + sizeof(xZipFooter_t);

	// Write the footer:  (block used to keep possibly swapped footer from being used later)
	{
		xZipFooter_t footer;
		footer.Magic = xZipFooter_t::MAGIC;
		footer.Size = OutputFileBytes;

		g_xzpSwap.SwapFieldsToTargetEndian<xZipFooter_t>( &footer );	// Swap the footer
		fwrite( &footer, 1, sizeof(footer), hOutputFile );
	}

	// Seek back and rewrite the header (filename data changes it for example)
	fseek(hOutputFile,0,SEEK_SET);
	g_xzpSwap.SwapFieldsToTargetEndian<xZipHeader_t>( &Header );	// Swap it to write out:
	fwrite(&Header,1,sizeof(Header),hOutputFile);
	g_xzpSwap.SwapFieldsToTargetEndian<xZipHeader_t>( &Header );	// But then swap it back so we can use it in  memory

	// Shut down
	fclose(hOutputFile);

	// Print the summary
	printf("\n\nSummary: Input:%u, XZip:%u, Directory Entries:%u (%u preloaded), Preloaded Bytes:%u\n\n",InputFileBytes,OutputFileBytes,Header.DirectoryEntries, Header.PreloadDirectoryEntries, Header.PreloadBytes);

	// Shut down:
	fclose(hTempFileData);
	fclose(hTempFilePreload);

	return true;
}

#define PADD_ID	MAKEID('P','A','D','D')

//-----------------------------------------------------------------------------
// xZipComputeWAVPreload
//
// Returns the number of bytes from a xbox compliant WAV file that should go into 
// the preload section:
//-----------------------------------------------------------------------------
unsigned xZipComputeWAVPreload( char *pFileName )
{
	InFileRIFF riff( pFileName, *g_pSndIO );
	if ( riff.RIFFName() != RIFF_WAVE )
	{
		return 0;
	}

	IterateRIFF walk( riff, riff.RIFFSize() );

	while ( walk.ChunkAvailable() )
	{
		// xbox compliant wavs have a single PADD chunk
		if ( walk.ChunkName() == PADD_ID )
		{
			// want to preload data up through PADD chunk header
			// and not the actual pad bytes
			return walk.ChunkFilePosition() + 2*sizeof( int );
		}
		walk.ChunkNext();
	}

	return 0;
}


//-----------------------------------------------------------------------------
// xZipComputeXWVPreload
//
// Returns the number of bytes from a XWV file that should go into the preload
// section:
//-----------------------------------------------------------------------------
unsigned xZipComputeXWVPreload( const char* filename )
{
	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf( "Failed to open xwv file: %s\n", filename );
		return 0;
	}

	// Read and validate the XWV header:
	xwvHeader_t header;
	memset( &header, 0, sizeof(header) );
	fread( &header, 1, sizeof(header), hFile );
	fclose( hFile );
	
	if ( header.id != XWV_ID || header.headerSize != sizeof(header) )
		return 0;

	return header.GetPreloadSize();
}

unsigned xZipComputeXTFPreload( const char* filename )
{
#if 0 // X360TBD: Not using XTF anymore
	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	XTFFileHeader_t header;
	memset( &header,0, sizeof( header ) );
	fread( &header,1,sizeof(header),hFile);

	fclose(hFile);

	if ( !strncmp( header.fileTypeString, "XTF", 4 ) )
		return header.preloadDataSize;
#endif
	return 0;
}

// TODO: ONLY store them in the preload section:
unsigned xZipComputeVMTPreload( const char* filename )
{
	// Store VMT's entirely
	if ( !strstr(filename,".vmt") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	fseek( hFile, 0, SEEK_END );
	unsigned offset = ftell( hFile );
	fclose( hFile );
	return offset;
}

// TODO: ONLY store them in the preload section:
unsigned xZipComputeVHVPreload( const char* filename )
{
	// Store VMT's entirely
	if ( !strstr(filename,".vhv") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	fclose( hFile );

	// Just load the header:
	return sizeof(HardwareVerts::FileHeader_t);
}

unsigned xZipComputeXCSPreload( const char* filename )
{
	if( !strstr(filename,".vcs") )
		return 0;

	FILE* hFile = fopen( filename, "rb" );
	if ( !hFile )
	{
		printf("Failed to open file: %s\n", filename);
		return 0;
	}

	XShaderHeader_t header;
	fread(&header,1,sizeof(XShaderHeader_t), hFile);
	fseek(hFile,0,SEEK_END);
	fclose(hFile);
	
	if (!header.IsValid())
		return 0;

	return header.BytesToPreload();
}

unsigned xZipComputeCustomPreloads( const char* filename )
{
	// X360TBD: These all need to act on a utlbuffer
	Assert( 0 );
	return 0;

//	strlwr(filename);

	unsigned offset = xZipComputeXWVPreload( filename );
	if ( offset )
		return offset;

	offset = xZipComputeVMTPreload( filename );
	if ( offset )
		return offset;

	offset = xZipComputeXCSPreload( filename );
	if ( offset )
		return offset;

	offset = xZipComputeVHVPreload( filename );
	if ( offset )
		return offset;

	return xZipComputeXTFPreload( filename );
}

#endif // MAKE_GAMEDATA_TOOL
