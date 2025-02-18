//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vtf/vtf.h"
#include "tier1/UtlBuffer.h"
#include "tier1/utlmap.h"
#include "bitmap/imageformat.h"
#include "mathlib/vector.h"
#include <conio.h>

void Usage( void )
{
	printf( "Usage: vtfdiff file1.vtf file2.vtf\n" );
}

bool LoadFileIntoBuffer( const char *pFileName, CUtlBuffer &buf )
{
	struct	_stat statBuf;
	if( _stat( pFileName, &statBuf ) != 0 )
	{
		goto error;
	}

	buf.EnsureCapacity( statBuf.st_size );
	FILE *fp;
	fp = fopen( pFileName, "rb" );
	if( !fp )
	{
		goto error;
	}
	
	int nBytesRead = fread( buf.Base(), 1, statBuf.st_size, fp );
	fclose( fp );

	buf.SeekPut( CUtlBuffer::SEEK_HEAD, nBytesRead );
	return true;

error:
	printf( "Can't find file %s\n", pFileName );
	return false;
}

char const * ResourceToString( uint32 uiResType )
{
	static char chBuffer[256];

	switch ( uiResType )
	{
	case VTF_LEGACY_RSRC_LOW_RES_IMAGE:
		return "VTF_LEGACY_RSRC_LOW_RES_IMAGE";
	case VTF_LEGACY_RSRC_IMAGE:
		return "VTF_LEGACY_RSRC_IMAGE";
	case VTF_RSRC_SHEET:
		return "VTF_RSRC_SHEET";
	case MK_VTF_RSRC_ID( 'C','R','C' ):
		return "CRC";
	case VTF_RSRC_TEXTURE_LOD_SETTINGS:
		return "VTF_RSRC_TEXTURE_LOD_SETTINGS";
	
	default:
		sprintf( chBuffer, "0x%08X", uiResType );
		return chBuffer;
	}

	return chBuffer;
}

void PrintFlags( int flags )
{
#define PRNFLAG( flagname ) \
	if ( ( flags & (flagname) ) == (flagname) ) \
	{ \
		flags &=~ (flagname); \
		printf( "%s%s", #flagname + strlen("TEXTUREFLAGS_"), flags ? "|" : "" ); \
	} \


	PRNFLAG( TEXTUREFLAGS_POINTSAMPLE	               )
	PRNFLAG( TEXTUREFLAGS_TRILINEAR		               )
	PRNFLAG( TEXTUREFLAGS_CLAMPS			           )
	PRNFLAG( TEXTUREFLAGS_CLAMPT			           )
	PRNFLAG( TEXTUREFLAGS_ANISOTROPIC	               )
	PRNFLAG( TEXTUREFLAGS_HINT_DXT5		               )
	PRNFLAG( TEXTUREFLAGS_SRGB				           )
	PRNFLAG( TEXTUREFLAGS_NORMAL			           )
	PRNFLAG( TEXTUREFLAGS_NOMIP			               )
	PRNFLAG( TEXTUREFLAGS_NOLOD			               )
	PRNFLAG( TEXTUREFLAGS_ALL_MIPS			           )
	PRNFLAG( TEXTUREFLAGS_PROCEDURAL		           )
	PRNFLAG( TEXTUREFLAGS_ONEBITALPHA	               )
	PRNFLAG( TEXTUREFLAGS_EIGHTBITALPHA	               )
	PRNFLAG( TEXTUREFLAGS_ENVMAP			           )
	PRNFLAG( TEXTUREFLAGS_RENDERTARGET	               )
	PRNFLAG( TEXTUREFLAGS_DEPTHRENDERTARGET	           )
	PRNFLAG( TEXTUREFLAGS_NODEBUGOVERRIDE              )
	PRNFLAG( TEXTUREFLAGS_SINGLECOPY		           )
	PRNFLAG( TEXTUREFLAGS_STAGING_MEMORY		       )
	PRNFLAG( TEXTUREFLAGS_IMMEDIATE_CLEANUP			   )
	PRNFLAG( TEXTUREFLAGS_IGNORE_PICMIP                )
	PRNFLAG( TEXTUREFLAGS_UNUSED_00400000			   )
	PRNFLAG( TEXTUREFLAGS_NODEPTHBUFFER                )
	PRNFLAG( TEXTUREFLAGS_UNUSED_01000000              )
	PRNFLAG( TEXTUREFLAGS_CLAMPU                       )
	PRNFLAG( TEXTUREFLAGS_VERTEXTEXTURE                )
	PRNFLAG( TEXTUREFLAGS_SSBUMP                       )
	PRNFLAG( TEXTUREFLAGS_UNUSED_10000000              )
	PRNFLAG( TEXTUREFLAGS_BORDER					   )
	PRNFLAG( TEXTUREFLAGS_UNUSED_40000000			   )
	PRNFLAG( TEXTUREFLAGS_UNUSED_80000000			   )

#undef PRNFLAG

	if ( flags )
	{
		printf( "0x%08X", flags );
	}
}

int main( int argc, char **argv )
{
	if( argc != 3 )
	{
		Usage();
		return 10;
	}

	CUtlBuffer file1;
	CUtlBuffer file2;

	if ( !LoadFileIntoBuffer( argv[1], file1 ) )
		return 21;
	if ( !LoadFileIntoBuffer( argv[2], file2 ) )
		return 22;

	IVTFTexture *pTexture1 = CreateVTFTexture();
	IVTFTexture *pTexture2 = CreateVTFTexture();

	IVTFTexture *arrTextures[2] = { pTexture1, pTexture2 };

	bool bMatch = true;

	if( !pTexture1->Unserialize( file1 ) )
	{
		printf( "error loading %s\n", argv[1] );
		return 31;
	}
	if( !pTexture2->Unserialize( file2 ) )
	{
		printf( "error loading %s\n", argv[2] );
		return 32;
	}

	if( pTexture1->Width() != pTexture2->Width() ||
		pTexture1->Height() != pTexture2->Height() ||
		pTexture1->Depth() != pTexture2->Depth() )
	{
		printf( "%s dimensions differ: %dx%dx%d != %dx%dx%d\n", 
			argv[1],
			( int )pTexture1->Width(), ( int )pTexture1->Height(), ( int )pTexture1->Depth(), 
			( int )pTexture2->Width(), ( int )pTexture2->Height(), ( int )pTexture2->Depth() );
		bMatch = false;
	}

	if( pTexture1->LowResWidth() != pTexture2->LowResWidth() ||
		pTexture1->LowResHeight() != pTexture2->LowResHeight() )
	{
		printf( "%s lowres dimensions differ: %dx%d != %dx%d\n", 
			argv[1],
			( int )pTexture1->LowResWidth(), ( int )pTexture1->LowResHeight(),
			( int )pTexture2->LowResWidth(), ( int )pTexture2->LowResHeight() );
		bMatch = false;
	}

	if( pTexture1->MipCount() != pTexture2->MipCount() )
	{
		printf( "%s differing mipcounts: %d != %d\n", 
			argv[1],
			( int )pTexture1->MipCount(), ( int )pTexture2->MipCount() );
		bMatch = false;
	}

	if( pTexture1->FaceCount() != pTexture2->FaceCount() )
	{
		printf( "%s differing facecount: %d != %d\n", 
			argv[1],
			( int )pTexture1->FaceCount(), ( int )pTexture2->FaceCount() );
		bMatch = false;
	}

	if( pTexture1->FrameCount() != pTexture2->FrameCount() )
	{
		printf( "%s differing framecount: %d != %d\n", 
			argv[1],
			( int )pTexture1->FrameCount(), ( int )pTexture2->FrameCount() );
		bMatch = false;
	}
	
	if( pTexture1->Flags() != pTexture2->Flags() )
	{
		printf( "%s differing flags: \"", 
			argv[1] );
		PrintFlags( pTexture1->Flags() );
		printf( "\" != \"" );
		PrintFlags( pTexture2->Flags() );
		printf( "\"\n" );
		bMatch = false;
	}
	
	if( pTexture1->BumpScale() != pTexture2->BumpScale() )
	{
		printf( "%s differing bumpscale: %f != %f\n", 
			argv[1],
			( float )pTexture1->BumpScale(), ( float )pTexture2->BumpScale() );
		bMatch = false;
	}
	
	if( pTexture1->Format() != pTexture2->Format() )
	{
		printf( "%s differing image format: %s != %s\n", 
			argv[1],
			ImageLoader::GetName( pTexture1->Format() ),
			ImageLoader::GetName( pTexture2->Format() ) );
		bMatch = false;
	}
	
	if( pTexture1->LowResFormat() != pTexture2->LowResFormat() )
	{
		Assert(0);
		printf( "%s differing lowres image format: %s != %s\n", 
			argv[1],
			ImageLoader::GetName( pTexture1->LowResFormat() ),
			ImageLoader::GetName( pTexture2->LowResFormat() ) );
		bMatch = false;
	}

	const Vector &vReflectivity1 = pTexture1->Reflectivity();
	const Vector &vReflectivity2 = pTexture2->Reflectivity();
	if( !VectorsAreEqual( vReflectivity1, vReflectivity2, 0.0001f ) )
	{
		printf( "%s differing reflectivity: [%f,%f,%f] != [%f,%f,%f]\n", 
			argv[1],
			( float )pTexture1->Reflectivity()[0],
			( float )pTexture1->Reflectivity()[1],
			( float )pTexture1->Reflectivity()[2],
			( float )pTexture2->Reflectivity()[0],
			( float )pTexture2->Reflectivity()[1],
			( float )pTexture2->Reflectivity()[2] );
		bMatch = false;
	}

	if ( pTexture1->ComputeTotalSize() != pTexture2->ComputeTotalSize() )
	{
		printf( "%s differing image data size: %d != %d\n", 
			argv[1],
			( int )pTexture1->ComputeTotalSize(), ( int )pTexture2->ComputeTotalSize() );
		bMatch = false;
	}
	
	if ( bMatch )
	{
		unsigned char const *pData1 = pTexture1->ImageData();
		unsigned char const *pData2 = pTexture2->ImageData();

		int const iSize = pTexture1->ComputeTotalSize();

		if( memcmp( pData1, pData2, iSize) != 0 )
		{
			printf( "%s image data different\n", argv[1] );

			if (( pTexture1->Format() == IMAGE_FORMAT_DXT1 ) || ( pTexture1->Format() == IMAGE_FORMAT_DXT3 ) ||
				( pTexture1->Format() == IMAGE_FORMAT_DXT5 ) || ( pTexture1->Format() == IMAGE_FORMAT_ATI2N ) ||
				( pTexture1->Format() == IMAGE_FORMAT_ATI1N ) )
			{
				int i, numOffsetsComplained = 0;
				for( i = 0; i < iSize; i++ )
				{
					if( pData1[i] != pData2[i] )
					{
						printf( "image data at offset %d different\n", i );
						if ( numOffsetsComplained ++ > 10 )
						{
							printf( "image data significantly differs!\n" );
							break;
						}
					}
				}
			}
			else
			{
				for( int iFrame = 0; iFrame < pTexture1->FrameCount(); ++iFrame )
				{
					for ( int iMipLevel = 0; iMipLevel < pTexture1->MipCount(); ++iMipLevel )
					{
						int nMipWidth, nMipHeight, nMipDepth;
						pTexture1->ComputeMipLevelDimensions( iMipLevel, &nMipWidth, &nMipHeight, &nMipDepth );

						for (int iCubeFace = 0; iCubeFace < pTexture1->FrameCount(); ++iCubeFace)
						{
							for ( int z = 0; z < nMipDepth; ++z )
							{
								pData1 = pTexture1->ImageData( iFrame, iCubeFace, iMipLevel, 0, 0, z );
								pData2 = pTexture2->ImageData( iFrame, iCubeFace, iMipLevel, 0, 0, z );

								int nMipSize = pTexture1->ComputeMipSize( iMipLevel );
								if ( memcmp( pData1, pData2, nMipSize ) )
								{
									bool bBreak = false;
									
									for ( int y = 0; y < nMipHeight; ++y )
									{
										for ( int x = 0; x < nMipWidth; ++x )
										{
											unsigned char const *pData1a = pTexture1->ImageData( iFrame, iCubeFace, iMipLevel, x, y, z );
											unsigned char const *pData2a = pTexture2->ImageData( iFrame, iCubeFace, iMipLevel, x, y, z );

											if ( memcmp( pData1a, pData2a, ImageLoader::SizeInBytes( pTexture1->Format() ) ) )
											{
												printf( "Frame %d Mip level %d Face %d Z-slice %d texel (%d,%d) different!\n", 
													iFrame, iMipLevel, iCubeFace, z, x, y );
												bBreak = true;
												break;
											}
										}

										if ( bBreak )
											break;
									}
								}
							}
						}
					}
				}
			}

			bMatch = false;
		}
	}

	// Lowres data
	{
		int iDummy, iSize1, iSize2;
		pTexture1->LowResFileInfo( &iDummy, &iSize1 );
		pTexture2->LowResFileInfo( &iDummy, &iSize2 );

		if ( iSize1 != iSize2 )
		{
			printf( "%s differing low res image data size: %d != %d\n", argv[1], iSize1, iSize2 );
			bMatch = false;
		}

		if ( bMatch )
		{
			if ( memcmp( pTexture1->LowResImageData(), pTexture2->LowResImageData(), iSize1 ) != 0 )
			{
				printf( "%s differing low res image data\n", 
					argv[1] );
				bMatch = false;
			}
		}
	}

	// Check other resources
	{
		int numRes1 = pTexture1->GetResourceTypes( NULL, 0 );
		int numRes2 = pTexture2->GetResourceTypes( NULL, 0 );

		// List of resource types checked or insignificant diffs
		typedef CUtlMap< int, bool > MapResTypes;
		MapResTypes mapTypes( DefLessFunc( int ) );
		mapTypes.Insert( VTF_LEGACY_RSRC_LOW_RES_IMAGE, true );
		mapTypes.Insert( VTF_LEGACY_RSRC_IMAGE, true );
		mapTypes.Insert( MK_VTF_RSRC_ID( 'C','R','C' ), true );

		uint32 *puiresbuffer = ( uint32 * ) stackalloc( ( numRes1 + numRes2 ) * sizeof( uint32 ) );

		int arrNums[2] = { numRes1, numRes2 };

		for ( int itx = 0; itx < 2; ++ itx )
		{
			arrTextures[itx]->GetResourceTypes( puiresbuffer, arrNums[itx] );
			while ( arrNums[itx] --> 0 )
			{
				uint32 uiResType = puiresbuffer[ arrNums[itx] ];
				if ( mapTypes.Find( uiResType ) != mapTypes.InvalidIndex() )
					continue;

				mapTypes.Insert( uiResType, true );

				size_t numBytes1, numBytes2;
				void const *pvResData1 = pTexture1->GetResourceData( uiResType, &numBytes1 );
				void const *pvResData2 = pTexture2->GetResourceData( uiResType, &numBytes2 );

				if ( !pvResData1 != !pvResData2 )
				{
					printf( "%s different resource %s %s\n", 
						argv[1],
						ResourceToString( uiResType ),
						pvResData1 ? "present" : "missing" );
					bMatch = false;
				}
				else if ( numBytes1 != numBytes2 )
				{
					printf( "%s different resource %s size %lld != %lld\n", 
						argv[1],
						ResourceToString( uiResType ),
						(long long)numBytes1, (long long)numBytes2 );
					bMatch = false;
				}
				else if ( memcmp( pvResData1, pvResData2, numBytes1 ) != 0 )
				{
					printf( "%s different resource %s data\n", 
						argv[1],
						ResourceToString( uiResType ) );
					bMatch = false;
				}
			}
		}
	}



	if( bMatch )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
