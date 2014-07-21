//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef RIFF_H
#define RIFF_H
#pragma once

#include "commonmacros.h"


//-----------------------------------------------------------------------------
// Purpose: This is a simple abstraction that the RIFF classes use to read from
//			files/memory
//-----------------------------------------------------------------------------
class IFileReadBinary
{
public:
	virtual int open( const char *pFileName ) = 0;
	virtual int read( void *pOutput, int size, int file ) = 0;
	virtual void close( int file ) = 0;
	virtual void seek( int file, int pos ) = 0;
	virtual unsigned int tell( int file ) = 0;
	virtual unsigned int size( int file ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Used to read/parse a RIFF format file
//-----------------------------------------------------------------------------
class InFileRIFF
{
public:
	InFileRIFF( const char *pFileName, IFileReadBinary &io );
	~InFileRIFF( void );

	unsigned int RIFFName( void ) { return m_riffName; }
	unsigned int RIFFSize( void ) { return m_riffSize; }

	int		ReadInt( void );
	int		ReadData( void *pOutput, int dataSize );
	int		PositionGet( void );
	void	PositionSet( int position );
	bool	IsValid( void ) { return m_file != 0; }

private:
	const InFileRIFF & operator=( const InFileRIFF & );

	IFileReadBinary		&m_io;
	int					m_file;
	unsigned int		m_riffName;
	unsigned int		m_riffSize;
};


//-----------------------------------------------------------------------------
// Purpose: Used to iterate over an InFileRIFF
//-----------------------------------------------------------------------------
class IterateRIFF
{
public:
	IterateRIFF( InFileRIFF &riff, int size );
	IterateRIFF( IterateRIFF &parent );

	bool			ChunkAvailable( void );
	bool			ChunkNext( void );

	unsigned int	ChunkName( void );
	unsigned int	ChunkSize( void );
	int				ChunkRead( void *pOutput );
	int				ChunkReadPartial( void *pOutput, int dataSize );
	int				ChunkReadInt( void );
	int				ChunkFilePosition( void ) { return m_chunkPosition; }

private:
	const IterateRIFF & operator=( const IterateRIFF & );

	void	ChunkSetup( void );
	void	ChunkClear( void );

	InFileRIFF			&m_riff;
	int					m_start;
	int					m_size;

	unsigned int		m_chunkName;
	int					m_chunkSize;
	int					m_chunkPosition;
};

class IFileWriteBinary
{
public:
	virtual int create( const char *pFileName ) = 0;
	virtual int write( void *pData, int size, int file ) = 0;
	virtual void close( int file ) = 0;
	virtual void seek( int file, int pos ) = 0;
	virtual unsigned int tell( int file ) = 0;
};
//-----------------------------------------------------------------------------
// Purpose: Used to write a RIFF format file
//-----------------------------------------------------------------------------
class OutFileRIFF
{
public:
	OutFileRIFF( const char *pFileName, IFileWriteBinary &io );
	~OutFileRIFF( void );

	bool	WriteInt( int number );
	bool 	WriteData( void *pOutput, int dataSize );
	int		PositionGet( void );
	void	PositionSet( int position );
	bool	IsValid( void ) { return m_file != 0; }
	
	void    HasLISETData( int position );

private:
	const OutFileRIFF & operator=( const OutFileRIFF & );

	IFileWriteBinary	&m_io;
	int					m_file;
	unsigned int		m_riffName;
	unsigned int		m_riffSize;
	unsigned int		m_nNamePos;

	// hack to make liset work correctly
	bool				m_bUseIncorrectLISETLength;
	int					m_nLISETSize;


};

//-----------------------------------------------------------------------------
// Purpose: Used to iterate over an InFileRIFF
//-----------------------------------------------------------------------------
class IterateOutputRIFF
{
public:
	IterateOutputRIFF( OutFileRIFF &riff );
	IterateOutputRIFF( IterateOutputRIFF &parent );

	void			ChunkStart( unsigned int chunkname );
	void			ChunkFinish( void );

	void			ChunkWrite( unsigned int chunkname, void *pOutput, int size );
	void			ChunkWriteInt( int number );
	void			ChunkWriteData( void *pOutput, int size );

	int				ChunkFilePosition( void ) { return m_chunkPosition; }

	unsigned int	ChunkGetPosition( void );
	void			ChunkSetPosition( int position );

	void			CopyChunkData( IterateRIFF& input );

	void			SetLISETData( int position );

private:

	const IterateOutputRIFF & operator=( const IterateOutputRIFF & );

	OutFileRIFF			&m_riff;
	int					m_start;
	int					m_size;

	unsigned int		m_chunkName;
	int					m_chunkSize;
	int					m_chunkPosition;
	int					m_chunkStart;
};

#define RIFF_ID					MAKEID('R','I','F','F')
#define RIFF_WAVE				MAKEID('W','A','V','E')
#define WAVE_FMT				MAKEID('f','m','t',' ')
#define WAVE_DATA				MAKEID('d','a','t','a')
#define WAVE_FACT				MAKEID('f','a','c','t')
#define WAVE_CUE				MAKEID('c','u','e',' ')
#define WAVE_SAMPLER			MAKEID('s','m','p','l')
#define WAVE_VALVEDATA			MAKEID('V','D','A','T')
#define WAVE_PADD				MAKEID('P','A','D','D')
#define WAVE_LIST				MAKEID('L','I','S','T') 

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM			0x0001
#endif
#ifndef WAVE_FORMAT_ADPCM
#define WAVE_FORMAT_ADPCM		0x0002
#endif
#define WAVE_FORMAT_XBOX_ADPCM	0x0069
#ifndef WAVE_FORMAT_XMA
#define WAVE_FORMAT_XMA			0x0165
#endif

#endif // RIFF_H
