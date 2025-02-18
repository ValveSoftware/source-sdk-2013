//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ZIP_UTILS_H
#define ZIP_UTILS_H
#ifdef _WIN32
#pragma once
#endif

#include "utlsymbol.h"

class CUtlBuffer;
#include "tier0/dbg.h"

abstract_class IZip
{
public:
	virtual void			Reset() = 0;

	// Add a single file to a zip - maintains the zip's previous alignment state
	virtual void			AddFileToZip		( const char *relativename, const char *fullpath ) = 0;

	// Whether a file is contained in a zip - maintains alignment
	virtual bool			FileExistsInZip		( const char *pRelativeName ) = 0;

	// Reads a file from the zip - maintains alignement
	virtual bool			ReadFileFromZip		( const char *pRelativeName, bool bTextMode, CUtlBuffer &buf ) = 0;
	virtual bool			ReadFileFromZip		( HANDLE hFile, const char *pRelativeName, bool bTextMode, CUtlBuffer &buf ) = 0;

	// Removes a single file from the zip - maintains alignment
	virtual void			RemoveFileFromZip	( const char *relativename ) = 0;

	// Gets next filename in zip, for walking the directory - maintains alignment
	virtual int				GetNextFilename		( int id, char *pBuffer, int bufferSize, int &fileSize ) = 0;

	// Prints the zip's contents - maintains alignment
	virtual void			PrintDirectory		( void ) = 0;

	// Estimate the size of the Zip (including header, padding, etc.)
	virtual unsigned int	EstimateSize		( void ) = 0;

	// Add buffer to zip as a file with given name - uses current alignment size, default 0 (no alignment)
	virtual void			AddBufferToZip		( const char *relativename, void *data, int length, bool bTextMode ) = 0;

	// Writes out zip file to a buffer - uses current alignment size 
	// (set by file's previous alignment, or a call to ForceAlignment)
	virtual void			SaveToBuffer		( CUtlBuffer& outbuf ) = 0;

	// Writes out zip file to a filestream - uses current alignment size 
	// (set by file's previous alignment, or a call to ForceAlignment)
	virtual void			SaveToDisk			( FILE *fout ) = 0;
	virtual void			SaveToDisk			( HANDLE hFileOut ) = 0;
	
	// Reads a zip file from a buffer into memory - sets current alignment size to 
	// the file's alignment size, unless overridden by a ForceAlignment call)
	virtual void			ParseFromBuffer		( void *buffer, int bufferlength ) = 0;

	// Mounts a zip file from the disk
	// Only ReadFileFromZip() is supported because the zip file could be >2GB
	virtual HANDLE			ParseFromDisk		( const char *pFilename ) = 0;

	// Forces a specific alignment size for all subsequent file operations, overriding files' previous alignment size.
	// Return to using files' individual alignment sizes by passing FALSE.
	virtual void			ForceAlignment		( bool aligned, bool bCompatibleFormat, unsigned int alignmentSize=0 ) = 0;

	virtual unsigned int	GetAlignment() = 0;

	// Sets the endianess of the zip
	virtual void			SetBigEndian( bool bigEndian ) = 0;
	virtual void			ActivateByteSwapping( bool bActivate ) = 0;

	// Create/Release additional instances
	// Disk Caching is necessary for large zips
	static IZip *CreateZip( const char *pDiskCacheWritePath = NULL, bool bSortByName = false );
	static void ReleaseZip( IZip *zip );
};

#endif // ZIP_UTILS_H
