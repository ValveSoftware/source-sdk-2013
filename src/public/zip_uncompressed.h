//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ZIP_UNCOMPRESSED_H
#define ZIP_UNCOMPRESSED_H
#ifdef _WIN32
#pragma once
#endif

#include "datamap.h"

#define PKID( a, b ) (((b)<<24)|((a)<<16)|('K'<<8)|'P')

// compressionMethod field
#define ZIP_COMPRESSION_NONE  0
#define ZIP_COMPRESSION_LZMA 14

#pragma pack(1)

struct ZIP_EndOfCentralDirRecord
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned int	signature; // 4 bytes PK56
	unsigned short	numberOfThisDisk;  // 2 bytes
	unsigned short	numberOfTheDiskWithStartOfCentralDirectory; // 2 bytes
	unsigned short	nCentralDirectoryEntries_ThisDisk;	// 2 bytes
	unsigned short	nCentralDirectoryEntries_Total;	// 2 bytes
	unsigned int	centralDirectorySize; // 4 bytes
	unsigned int	startOfCentralDirOffset; // 4 bytes
	unsigned short	commentLength; // 2 bytes
	// zip file comment follows
};

struct ZIP_FileHeader
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned int	signature; //  4 bytes PK12 
	unsigned short	versionMadeBy; // version made by 2 bytes 
	unsigned short	versionNeededToExtract; // version needed to extract 2 bytes 
	unsigned short	flags; // general purpose bit flag 2 bytes 
	unsigned short	compressionMethod; // compression method 2 bytes 
	unsigned short	lastModifiedTime; // last mod file time 2 bytes 
	unsigned short	lastModifiedDate; // last mod file date 2 bytes 
	unsigned int	crc32; // crc-32 4 bytes 
	unsigned int	compressedSize; // compressed size 4 bytes 
	unsigned int	uncompressedSize; // uncompressed size 4 bytes 
	unsigned short	fileNameLength; // file name length 2 bytes 
	unsigned short	extraFieldLength; // extra field length 2 bytes 
	unsigned short	fileCommentLength; // file comment length 2 bytes 
	unsigned short	diskNumberStart; // disk number start 2 bytes 
	unsigned short	internalFileAttribs; // internal file attributes 2 bytes 
	unsigned int	externalFileAttribs; // external file attributes 4 bytes 
	unsigned int	relativeOffsetOfLocalHeader; // relative offset of local header 4 bytes 
	// file name (variable size) 
	// extra field (variable size) 
	// file comment (variable size) 
};

struct ZIP_LocalFileHeader
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned int	signature; //local file header signature 4 bytes PK34 
	unsigned short	versionNeededToExtract; // version needed to extract 2 bytes 
	unsigned short	flags; // general purpose bit flag 2 bytes 
	unsigned short	compressionMethod; // compression method 2 bytes 
	unsigned short	lastModifiedTime; // last mod file time 2 bytes 
	unsigned short	lastModifiedDate; // last mod file date 2 bytes 
	unsigned int	crc32; // crc-32 4 bytes 
	unsigned int	compressedSize; // compressed size 4 bytes 
	unsigned int	uncompressedSize; // uncompressed size 4 bytes 
	unsigned short	fileNameLength; // file name length 2 bytes 
	unsigned short	extraFieldLength; // extra field length 2 bytes 
	// file name (variable size) 
	// extra field (variable size) 
};

//=============================================================================//
//	Valve Non standard Extension, Preload Section
//	An optional first file in an aligned zip that can be loaded into ram and
//	used by the FileSystem to supply header data rather than disk.
//	Is is an optimization to prevent the large of amount of small I/O performed
///	by the map loading process.
//=============================================================================//

#define PRELOAD_SECTION_NAME	"__preload_section.pre"
#define PRELOAD_HDR_VERSION		3
#define XZIP_COMMENT_LENGTH		32
#define INVALID_PRELOAD_ENTRY	( (unsigned short)-1 )

struct ZIP_PreloadHeader 
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned int Version;					// VERSION
	unsigned int DirectoryEntries;			// Number of zip directory entries.
	unsigned int PreloadDirectoryEntries;	// Number of preloaded directory entries (equal or less than the zip dir).
	unsigned int Alignment;					// File alignment of the zip
};

struct ZIP_PreloadDirectoryEntry
{
	DECLARE_BYTESWAP_DATADESC();
    unsigned int Length;					// Length of the file's preload data in bytes
    unsigned int DataOffset;				// Offset the file data in the .zip, relative to the logical beginning of the preload file.
};

struct ZIP_PreloadRemapTable
{
	DECLARE_BYTESWAP_DATADESC();
	unsigned short PreloadIndex;			// Index into preload directory, entry marked invalid if no preload entry present
};

#pragma pack()

#endif // ZIP_UNCOMPRESSED_H
