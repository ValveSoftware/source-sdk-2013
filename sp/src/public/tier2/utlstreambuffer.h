//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================//

#ifndef UTLSTREAMBUFFER_H
#define UTLSTREAMBUFFER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"
#include "filesystem.h"


//-----------------------------------------------------------------------------
// Command parsing..
//-----------------------------------------------------------------------------
class CUtlStreamBuffer : public CUtlBuffer
{
	typedef CUtlBuffer BaseClass;

public:
	// See CUtlBuffer::BufferFlags_t for flags
	CUtlStreamBuffer( );
	CUtlStreamBuffer( const char *pFileName, const char *pPath, int nFlags = 0, bool bDelayOpen = false );
	~CUtlStreamBuffer();

	// Open the file. normally done in constructor
	void Open( const char *pFileName, const char *pPath, int nFlags );

	// close the file. normally done in destructor
	void Close();

	// Is the file open?
	bool IsOpen() const;

private:
	// error flags
	enum
	{
		FILE_OPEN_ERROR = MAX_ERROR_FLAG << 1,
		FILE_WRITE_ERROR = MAX_ERROR_FLAG << 2,
	};

	// Overflow functions
	bool StreamPutOverflow( int nSize );
	bool StreamGetOverflow( int nSize );

	// Grow allocation size to fit requested size
	void GrowAllocatedSize( int nSize );

	// Reads bytes from the file; fixes up maxput if necessary and null terminates
	int ReadBytesFromFile( int nBytesToRead, int nReadOffset );

	FileHandle_t OpenFile( const char *pFileName, const char *pPath );

	FileHandle_t m_hFileHandle;

	char *m_pFileName;
	char *m_pPath;
};


#endif // UTLSTREAMBUFFER_H

