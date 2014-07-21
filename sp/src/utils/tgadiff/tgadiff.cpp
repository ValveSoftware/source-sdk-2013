//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>
#include "bitmap/tgaloader.h"
#include "bitmap/tgawriter.h"
#include "tier1/utlbuffer.h"
#include "tier2/tier2.h"
#include "mathlib/mathlib.h"
#include "filesystem.h"

void Usage( void )
{
	printf( "Usage: tgadiff src1.tga src2.tga diff.tga\n" );
	exit( -1 );
}

int main( int argc, char **argv )
{
	if( argc != 4 )
	{
		Usage();
	}

	MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );
	InitDefaultFileSystem();

	char pCurrentDirectory[MAX_PATH];
	if ( _getcwd( pCurrentDirectory, sizeof(pCurrentDirectory) ) == NULL )
	{
		fprintf( stderr, "Unable to get the current directory\n" );
		return -1;
	}
	Q_FixSlashes( pCurrentDirectory );
	Q_StripTrailingSlash( pCurrentDirectory );

	char pBuf[3][MAX_PATH];
	const char *pFileName[3];
	for ( int i = 0; i < 3; ++i )
	{
		if ( !Q_IsAbsolutePath( argv[i+1] ) )
		{
			Q_snprintf( pBuf[i], sizeof(pBuf[i]), "%s\\%s", pCurrentDirectory, argv[i+1] );
			pFileName[i] = pBuf[i];
		}
		else
		{
			pFileName[i] = argv[i+1];
		}
	}

	int width1, height1;
	ImageFormat imageFormat1;
	float gamma1;

	CUtlBuffer buf1;
	if ( !g_pFullFileSystem->ReadFile( pFileName[0], NULL, buf1 ) )
	{
		fprintf( stderr, "%s not found\n", pFileName[0] );
		return -1;
	}

	if( !TGALoader::GetInfo( buf1, &width1, &height1, &imageFormat1, &gamma1 ) )
	{
		printf( "error loading %s\n", pFileName[0] );
		exit( -1 );
	}

	int width2, height2;
	ImageFormat imageFormat2;
	float gamma2;

	CUtlBuffer buf2;
	if ( !g_pFullFileSystem->ReadFile( pFileName[1], NULL, buf2 ) )
	{
		fprintf( stderr, "%s not found\n", pFileName[1] );
		return -1;
	}

	if( !TGALoader::GetInfo( buf2, &width2, &height2, &imageFormat2, &gamma2 ) )
	{
		printf( "error loading %s\n", pFileName[1] );
		exit( -1 );
	}

	if( width1 != width2 || height1 != height2 )
	{
		printf( "image dimensions different (%dx%d!=%dx%d): can't do diff for %s\n", 
			width1, height1, width2, height2, pFileName[2] );
		exit( -1 );
	}
#if 0
	// have to allow for different formats for now due to *.txt file screwup.
	if( imageFormat1 != imageFormat2 )
	{
		printf( "image format different (%s!=%s). . can't do diff for %s\n", 
			ImageLoader::GetName( imageFormat1 ), ImageLoader::GetName( imageFormat2 ), pFileName[2] );
		exit( -1 );
	}
#endif
	if( gamma1 != gamma2 )
	{
		printf( "image gamma different (%f!=%f). . can't do diff for %s\n", gamma1, gamma2, pFileName[2] );
		exit( -1 );
	}

	unsigned char *pImage1Tmp = new unsigned char[ImageLoader::GetMemRequired( width1, height1, 1, imageFormat1, false )];
	unsigned char *pImage2Tmp = new unsigned char[ImageLoader::GetMemRequired( width2, height2, 1, imageFormat2, false )];

	buf1.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	if( !TGALoader::Load( pImage1Tmp, buf1, width1, height1, imageFormat1, 2.2f, false ) )
	{
		printf( "error loading %s\n", pFileName[0] );
		exit( -1 );
	}
	
	buf2.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
	if( !TGALoader::Load( pImage2Tmp, buf2, width2, height2, imageFormat2, 2.2f, false ) )
	{
		printf( "error loading %s\n", pFileName[1] );
		exit( -1 );
	}

	unsigned char *pImage1 = new unsigned char[ImageLoader::GetMemRequired( width1, height1, 1, IMAGE_FORMAT_ABGR8888, false )];
	unsigned char *pImage2 = new unsigned char[ImageLoader::GetMemRequired( width2, height2, 1, IMAGE_FORMAT_ABGR8888, false )];
	unsigned char *pDiff = new unsigned char[ImageLoader::GetMemRequired( width2, height2, 1, IMAGE_FORMAT_ABGR8888, false )];
	ImageLoader::ConvertImageFormat( pImage1Tmp, imageFormat1, pImage1, IMAGE_FORMAT_ABGR8888, width1, height1, 0, 0 );
	ImageLoader::ConvertImageFormat( pImage2Tmp, imageFormat2, pImage2, IMAGE_FORMAT_ABGR8888, width2, height2, 0, 0 );

	int sizeInBytes = ImageLoader::SizeInBytes( IMAGE_FORMAT_ABGR8888 );
	bool isDifferent = false;
	for( int i = 0; i < width1 * height1 * sizeInBytes; i++ )
	{
		int d;
		d = pImage2[i] - pImage1[i];
		pDiff[i] = d > 0 ? d : -d;
		if( d != 0 )
		{
			isDifferent = true;
		}
	}

	if( !isDifferent )
	{
		printf( "Files are the same %s %s : not generating %s\n", pFileName[0], pFileName[1], pFileName[2] );
		exit( -1 );
	}
	else
	{
		printf( "Generating diff: %s!\n", pFileName[2] );
	}

	ImageFormat dstImageFormat;
	// get rid of this until we get the formats matching
//	if( sizeInBytes == 3 )
//	{
//		dstImageFormat = IMAGE_FORMAT_RGB888;
//	}
//	else
	{
		dstImageFormat = IMAGE_FORMAT_RGBA8888;
	}
	
	CUtlBuffer outBuffer;
	if ( !TGAWriter::WriteToBuffer( pDiff, outBuffer, width1, height1, dstImageFormat, dstImageFormat ) )
	{
		printf( "error writing %s to buffer\n", pFileName[2] );
		exit( -1 );
	}
	
	if ( !g_pFullFileSystem->WriteFile( pFileName[2], NULL, outBuffer ) )
	{
		fprintf( stderr, "unable to write %s\n", pFileName[2] );
		return -1;
	}

	return 0;	
}
