//========= Copyright Valve Corporation, All rights reserved. ============//
//
// NOTE: To make use of this file, g_pFullFileSystem must be defined, or you can modify
// this source to take an IFileSystem * as input.
//
//=======================================================================================//

// @note Tom Bui: we need to use fopen below in the jpeg code, so we can't have this on...
#ifdef PROTECTED_THINGS_ENABLE
#if !defined( POSIX )
#undef fopen
#endif // POSIX
#endif

#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#elif defined( POSIX )
#include <stdio.h>
#include <sys/stat.h>
#ifdef OSX
#include <copyfile.h>
#endif
#endif

#include "imageutils.h"
#include "filesystem.h"
#include "utlbuffer.h"
#include "bitmap/bitmap.h"
#include "vtf/vtf.h"

// clang3 on OSX folks the attribute into the prototype, causing a compile failure
// filed radar bug 10397783
#if ( __clang_major__ == 3 )
#include <setjmp.h>
extern void longjmp( jmp_buf, int ) __attribute__((noreturn));
#endif


#ifdef ENGINE_DLL
	#include "common.h"
#elif CLIENT_DLL
	// @note Tom Bui: instead of forcing the project to include EngineInterface.h...
	#include "cdll_int.h"
	// engine interface singleton accessors
	extern IVEngineClient *engine;
	extern class IGameUIFuncs *gameuifuncs;
	extern class IEngineSound *enginesound;
	extern class IXboxSystem  *xboxsystem;
	extern class IAchievementMgr *achievementmgr; 
	extern class CSteamAPIContext *steamapicontext;
#elif REPLAY_DLL
	#include "replay/ienginereplay.h"
	extern IEngineReplay *g_pEngine;
#elif ENGINE_DLL
	#include "EngineInterface.h"
#else
	#include "cdll_int.h"
	extern IVEngineClient *engine;
#endif

// use the JPEGLIB_USE_STDIO define so that we can read in jpeg's from outside the game directory tree.
#define JPEGLIB_USE_STDIO
#include "jpeglib/jpeglib.h"
#undef JPEGLIB_USE_STDIO

#include "../thirdparty/libspng/spng/spng.h"

#include <setjmp.h>

#include "bitmap/tgawriter.h"
#include "ivtex.h"
#ifdef WIN32
#include <io.h>
#endif
#ifdef OSX
#include <copyfile.h>
#endif

#ifndef WIN32
#define DeleteFile(s)	remove(s)
#endif

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif



// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct ValveJpegErrorHandler_t 
{
	// The default manager
	struct jpeg_error_mgr	m_Base;
	// For handling any errors
	jmp_buf					m_ErrorContext;
};

#define JPEG_OUTPUT_BUF_SIZE  4096   

struct JPEGDestinationManager_t
{
	struct jpeg_destination_mgr pub; // public fields

	CUtlBuffer  *pBuffer;       // target/final buffer
	byte        *buffer;        // start of temp buffer
};


//-----------------------------------------------------------------------------
// Purpose: We'll override the default error handler so we can deal with errors without having to exit the engine
//-----------------------------------------------------------------------------
static void ValveJpegErrorHandler( j_common_ptr cinfo )
{
	ValveJpegErrorHandler_t *pError = reinterpret_cast< ValveJpegErrorHandler_t * >( cinfo->err );

	char buffer[ JMSG_LENGTH_MAX ];

	/* Create the message */
	( *cinfo->err->format_message )( cinfo, buffer );

	Warning( "%s\n", buffer );

	// Bail
	longjmp( pError->m_ErrorContext, 1 );
}


// convert the JPEG file given to a TGA file at the given output path.
ConversionErrorType ImgUtl_ConvertJPEGToTGA( const char *jpegpath, const char *tgaPath, bool bRequirePowerOfTwo )
{
#if !defined( _X360 )

	//
	// !FIXME! This really probably should use ImgUtl_ReadJPEGAsRGBA, to avoid duplicated code.
	//

	struct jpeg_decompress_struct jpegInfo;
	struct ValveJpegErrorHandler_t jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	int cur_row = 0;

	// image attributes
	int image_height;
	int image_width;

	// open the jpeg image file.
	FILE *infile = fopen(jpegpath, "rb");
	if (infile == NULL)
	{
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// setup error to print to stderr.
	jpegInfo.err = jpeg_std_error(&jerr.m_Base);

	jpegInfo.err->error_exit = &ValveJpegErrorHandler;

	// create the decompress struct.
	jpeg_create_decompress(&jpegInfo);

	if ( setjmp( jerr.m_ErrorContext ) )
	{
		// Get here if there is any error
		jpeg_destroy_decompress( &jpegInfo );

		fclose(infile);

		return CE_ERROR_PARSING_SOURCE;
	}

	jpeg_stdio_src(&jpegInfo, infile);

	// read in the jpeg header and make sure that's all good.
	if (jpeg_read_header(&jpegInfo, TRUE) != JPEG_HEADER_OK)
	{
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	// start the decompress with the jpeg engine.
	if ( !jpeg_start_decompress(&jpegInfo) )
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	// Check for valid width and height (ie. power of 2 and print out an error and exit if not).
	if ( ( bRequirePowerOfTwo && ( !IsPowerOfTwo(jpegInfo.image_height) || !IsPowerOfTwo(jpegInfo.image_width) ) )
		|| jpegInfo.output_components != 3 )
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose( infile );
		return CE_SOURCE_FILE_SIZE_NOT_SUPPORTED;
	}

	// now that we've started the decompress with the jpeg lib, we have the attributes of the
	// image ready to be read out of the decompress struct.
	row_stride = jpegInfo.output_width * jpegInfo.output_components;
	image_height = jpegInfo.image_height;
	image_width = jpegInfo.image_width;
	int mem_required = jpegInfo.image_height * jpegInfo.image_width * jpegInfo.output_components;

	// allocate the memory to read the image data into.
	unsigned char *buf = (unsigned char *)malloc(mem_required);
	if (buf == NULL)
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_MEMORY_ERROR;
	}

	// read in all the scan lines of the image into our image data buffer.
	bool working = true;
	while (working && (jpegInfo.output_scanline < jpegInfo.output_height))
	{
		row_pointer[0] = &(buf[cur_row * row_stride]);
		if ( !jpeg_read_scanlines(&jpegInfo, row_pointer, 1) )
		{
			working = false;
		}
		++cur_row;
	}

	if (!working)
	{
		free(buf);
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	jpeg_finish_decompress(&jpegInfo);

	fclose(infile);
	
	// ok, at this point we have read in the JPEG image to our buffer, now we need to write it out as a TGA file.
	CUtlBuffer outBuf;
	bool bRetVal = TGAWriter::WriteToBuffer( buf, outBuf, image_width, image_height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
	if ( bRetVal )
	{
		if ( !g_pFullFileSystem->WriteFile( tgaPath, NULL, outBuf ) )
		{
			bRetVal = false;
		}
	}

	free(buf);
	return bRetVal ? CE_SUCCESS : CE_ERROR_WRITING_OUTPUT_FILE;

#else
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
#endif
}

// convert the bmp file given to a TGA file at the given destination path.
ConversionErrorType ImgUtl_ConvertBMPToTGA(const char *bmpPath, const char *tgaPath)
{
	if ( !IsPC() )
		return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;

#ifdef WIN32

	int nWidth, nHeight;
	ConversionErrorType result;
	unsigned char *pBufRGBA = ImgUtl_ReadBMPAsRGBA( bmpPath, nWidth, nHeight, result );
	if ( result != CE_SUCCESS)
	{
		Assert( !pBufRGBA );
		free( pBufRGBA );
		return result;
	}
	Assert( pBufRGBA );

	// write out the TGA file using the RGB data buffer.
	CUtlBuffer outBuf;
	bool retval = TGAWriter::WriteToBuffer(pBufRGBA, outBuf, nWidth, nHeight, IMAGE_FORMAT_RGBA8888, IMAGE_FORMAT_RGB888);
	free( pBufRGBA );

	if ( retval )
	{
		if ( !g_pFullFileSystem->WriteFile( tgaPath, NULL, outBuf ) )
		{
			retval = false;
		}
	}

	return retval ? CE_SUCCESS : CE_ERROR_WRITING_OUTPUT_FILE;

#else // WIN32
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
#endif
}

unsigned char *ImgUtl_ReadVTFAsRGBA( const char *vtfPath, int &width, int &height, ConversionErrorType &errcode )
{
	// Just load the whole file into a memory buffer
	CUtlBuffer bufFileContents;
	if ( !g_pFullFileSystem->ReadFile( vtfPath, NULL, bufFileContents ) )
	{
		errcode = CE_CANT_OPEN_SOURCE_FILE;
		return NULL;
	}

	IVTFTexture *pVTFTexture = CreateVTFTexture();
	if ( !pVTFTexture->Unserialize( bufFileContents ) )
	{
		DestroyVTFTexture( pVTFTexture );
		errcode = CE_ERROR_PARSING_SOURCE;
		return NULL;
	}

	width = pVTFTexture->Width();
	height = pVTFTexture->Height();
	pVTFTexture->ConvertImageFormat( IMAGE_FORMAT_RGBA8888, false );

	int nMemSize = ImageLoader::GetMemRequired( width, height, 1, IMAGE_FORMAT_RGBA8888, false );
	unsigned char *pMemImage = (unsigned char *)malloc(nMemSize);
	if ( pMemImage == NULL )
	{
		DestroyVTFTexture( pVTFTexture );
		errcode = CE_MEMORY_ERROR;
		return NULL;
	}
	Q_memcpy( pMemImage, pVTFTexture->ImageData(), nMemSize );

	DestroyVTFTexture( pVTFTexture );

	errcode = CE_SUCCESS;
	return pMemImage;
}

// read a TGA header from the current point in the file stream.
static void ImgUtl_ReadTGAHeader(FILE *infile, TGAHeader &header)
{
	if (infile == NULL)
	{
		return;
	}

	fread(&header.identsize, sizeof(header.identsize), 1, infile);
	fread(&header.colourmaptype, sizeof(header.colourmaptype), 1, infile);
	fread(&header.imagetype, sizeof(header.imagetype), 1, infile);
	fread(&header.colourmapstart, sizeof(header.colourmapstart), 1, infile);
	fread(&header.colourmaplength, sizeof(header.colourmaplength), 1, infile);
	fread(&header.colourmapbits, sizeof(header.colourmapbits), 1, infile);
	fread(&header.xstart, sizeof(header.xstart), 1, infile);
	fread(&header.ystart, sizeof(header.ystart), 1, infile);
	fread(&header.width, sizeof(header.width), 1, infile);
	fread(&header.height, sizeof(header.height), 1, infile);
	fread(&header.bits, sizeof(header.bits), 1, infile);
	fread(&header.descriptor, sizeof(header.descriptor), 1, infile);
}

// write a TGA header to the current point in the file stream.
static void WriteTGAHeader(FILE *outfile, TGAHeader &header)
{
	if (outfile == NULL)
	{
		return;
	}

	fwrite(&header.identsize, sizeof(header.identsize), 1, outfile);
	fwrite(&header.colourmaptype, sizeof(header.colourmaptype), 1, outfile);
	fwrite(&header.imagetype, sizeof(header.imagetype), 1, outfile);
	fwrite(&header.colourmapstart, sizeof(header.colourmapstart), 1, outfile);
	fwrite(&header.colourmaplength, sizeof(header.colourmaplength), 1, outfile);
	fwrite(&header.colourmapbits, sizeof(header.colourmapbits), 1, outfile);
	fwrite(&header.xstart, sizeof(header.xstart), 1, outfile);
	fwrite(&header.ystart, sizeof(header.ystart), 1, outfile);
	fwrite(&header.width, sizeof(header.width), 1, outfile);
	fwrite(&header.height, sizeof(header.height), 1, outfile);
	fwrite(&header.bits, sizeof(header.bits), 1, outfile);
	fwrite(&header.descriptor, sizeof(header.descriptor), 1, outfile);
}

// reads in a TGA file and converts it to 32 bit RGBA color values in a memory buffer.
unsigned char * ImgUtl_ReadTGAAsRGBA(const char *tgaPath, int &width, int &height, ConversionErrorType &errcode, TGAHeader &tgaHeader )
{
	FILE *tgaFile = fopen(tgaPath, "rb");
	if (tgaFile == NULL)
	{
		errcode = CE_CANT_OPEN_SOURCE_FILE;
		return NULL;
	}

	// read header for TGA file.
	ImgUtl_ReadTGAHeader(tgaFile, tgaHeader);

	if (
		( tgaHeader.imagetype != 2 ) // image type 2 is uncompressed RGB, other types not supported.
		|| ( tgaHeader.descriptor & 0x10 ) // Origin on righthand side (flipped horizontally from common sense) --- nobody ever uses this
		|| ( tgaHeader.bits != 24 && tgaHeader.bits != 32 ) // Must be 24- ot 32-bit
	)
	{
		fclose(tgaFile);

		errcode = CE_SOURCE_FILE_TGA_FORMAT_NOT_SUPPORTED;
		return NULL;
	}

	int tgaDataSize = tgaHeader.width * tgaHeader.height * tgaHeader.bits / 8;
	unsigned char *tgaData = (unsigned char *)malloc(tgaDataSize);
	if (tgaData == NULL)
	{
		fclose(tgaFile);

		errcode = CE_MEMORY_ERROR;
		return NULL;
	}

	fread(tgaData, 1, tgaDataSize, tgaFile);

	fclose(tgaFile);

	width = tgaHeader.width;
	height = tgaHeader.height;
	int numPixels = tgaHeader.width * tgaHeader.height;

	if (tgaHeader.bits == 24)
	{
		// image needs to be converted to a 32-bit image.

		unsigned char *retBuf = (unsigned char *)malloc(numPixels * 4);
		if (retBuf == NULL)
		{
			free(tgaData);

			errcode = CE_MEMORY_ERROR;
			return NULL;
		}

		// convert from BGR to RGBA color format.
		for (int index = 0; index < numPixels; ++index)
		{
			retBuf[index * 4] = tgaData[index * 3 + 2];
			retBuf[index * 4 + 1] = tgaData[index * 3 + 1];
			retBuf[index * 4 + 2] = tgaData[index * 3];
			retBuf[index * 4 + 3] = 0xff;
		}

		free(tgaData);
		tgaData = retBuf;
		tgaHeader.bits = 32;
	}
	else if (tgaHeader.bits == 32)
	{
		// Swap blue and red to convert BGR -> RGB
		for (int index = 0; index < numPixels; ++index)
		{
			V_swap( tgaData[index*4], tgaData[index*4 + 2] );
		}
	}

	// Flip image vertically if necessary
	if ( !( tgaHeader.descriptor & 0x20 ) )
	{
		int y0 = 0;
		int y1 = height-1;
		int iStride = width*4;
		while ( y0 < y1 )
		{
			unsigned char *ptr0 = tgaData + y0*iStride;
			unsigned char *ptr1 = tgaData + y1*iStride;
			for ( int i = 0 ; i < iStride ; ++i )
			{
				V_swap( ptr0[i], ptr1[i] );
			}
			++y0;
			--y1;
		}
		tgaHeader.descriptor |= 0x20;
	}

	errcode = CE_SUCCESS;
	return tgaData;
}

unsigned char *ImgUtl_ReadJPEGAsRGBA( const char *jpegPath, int &width, int &height, ConversionErrorType &errcode )
{
#if !defined( _X360 )
	struct jpeg_decompress_struct jpegInfo;
	struct ValveJpegErrorHandler_t jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	int cur_row = 0;

	// image attributes
	int image_height;
	int image_width;

	// open the jpeg image file.
	FILE *infile = fopen(jpegPath, "rb");
	if (infile == NULL)
	{
		errcode = CE_CANT_OPEN_SOURCE_FILE;
		return NULL;
	}

	//CJpegSourceMgr src;
	//FileHandle_t fileHandle = g_pFullFileSystem->Open( jpegPath, "rb" );
	//if ( fileHandle == FILESYSTEM_INVALID_HANDLE )
	//{
	//	errcode = CE_CANT_OPEN_SOURCE_FILE;
	//	return NULL;
	//}
	//if ( !src.Init( g_pFullFileSystem, fileHandle ) ) {
	//	errcode = CE_CANT_OPEN_SOURCE_FILE;
	//	g_pFullFileSystem->Close( fileHandle );
	//	return NULL;
	//}

	// setup error to print to stderr.
	memset( &jpegInfo, 0, sizeof( jpegInfo ) );
	jpegInfo.err = jpeg_std_error(&jerr.m_Base);

	jpegInfo.err->error_exit = &ValveJpegErrorHandler;

	// create the decompress struct.
	jpeg_create_decompress(&jpegInfo);

	if ( setjmp( jerr.m_ErrorContext ) )
	{
		// Get here if there is any error
		jpeg_destroy_decompress( &jpegInfo );

		fclose( infile );
		//g_pFullFileSystem->Close( fileHandle );

		errcode = CE_ERROR_PARSING_SOURCE;
		return NULL;
	}

	jpeg_stdio_src(&jpegInfo, infile);
	//jpegInfo.src = &src;

	// read in the jpeg header and make sure that's all good.
	if (jpeg_read_header(&jpegInfo, TRUE) != JPEG_HEADER_OK)
	{
		fclose( infile );
		//g_pFullFileSystem->Close( fileHandle );
		errcode = CE_ERROR_PARSING_SOURCE;
		return NULL;
	}

	// start the decompress with the jpeg engine.
	if ( !jpeg_start_decompress(&jpegInfo) )
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose( infile );
		//g_pFullFileSystem->Close( fileHandle );
		errcode = CE_ERROR_PARSING_SOURCE;
		return NULL;
	}

	// We only support 24-bit JPEG's
	if ( jpegInfo.out_color_space != JCS_RGB || jpegInfo.output_components != 3 )
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose( infile );
		//g_pFullFileSystem->Close( fileHandle );
		errcode = CE_SOURCE_FILE_SIZE_NOT_SUPPORTED;
		return NULL;
	}

	// now that we've started the decompress with the jpeg lib, we have the attributes of the
	// image ready to be read out of the decompress struct.
	row_stride = jpegInfo.output_width * 4;
	image_height = jpegInfo.image_height;
	image_width = jpegInfo.image_width;
	int mem_required = jpegInfo.image_height * row_stride;

	// allocate the memory to read the image data into.
	unsigned char *buf = (unsigned char *)malloc(mem_required);
	if (buf == NULL)
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose( infile );
		//g_pFullFileSystem->Close( fileHandle );
		errcode = CE_MEMORY_ERROR;
		return NULL;
	}

	// read in all the scan lines of the image into our image data buffer.
	bool working = true;
	while (working && (jpegInfo.output_scanline < jpegInfo.output_height))
	{
		unsigned char *pRow = &(buf[cur_row * row_stride]);
		row_pointer[0] = pRow;
		if ( !jpeg_read_scanlines(&jpegInfo, row_pointer, 1) )
		{
			working = false;
		}

		// Expand the row RGB -> RGBA
		for ( int x = image_width-1 ; x >= 0 ; --x )
		{
			pRow[x*4+3] = 0xff;
			pRow[x*4+2] = pRow[x*3+2];
			pRow[x*4+1] = pRow[x*3+1];
			pRow[x*4] = pRow[x*3];
		}

		++cur_row;
	}

	// Clean up
	fclose( infile );
	//g_pFullFileSystem->Close( fileHandle );
	jpeg_destroy_decompress(&jpegInfo);

	// Check success status
	if (!working)
	{
		free(buf);
		errcode = CE_ERROR_PARSING_SOURCE;
		return NULL;
	}

	// OK!
	width = image_width;
	height = image_height;
	errcode = CE_SUCCESS;
	return buf;

#else
	errcode = CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	return NULL;
#endif
}


unsigned char *ImgUtl_ReadPNGAsRGBA( const char *pngPath, int &width, int &height, ConversionErrorType &errcode )
{
#if !defined( _X360 )

	// Just load the whole file into a memory buffer
	CUtlBuffer bufFileContents;
	if ( !g_pFullFileSystem->ReadFile( pngPath, NULL, bufFileContents ) )
	{
		errcode = CE_CANT_OPEN_SOURCE_FILE;
		return NULL;
	}

	// Load it
	return ImgUtl_ReadPNGAsRGBAFromBuffer( bufFileContents, width, height, errcode );

#else
	errcode = CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	return NULL;
#endif
}

unsigned char *ImgUtl_ReadPNGAsRGBAFromBuffer( CUtlBuffer &buffer, int &width, int &height, ConversionErrorType &errcode )
{
	errcode = CE_MEMORY_ERROR;
	spng_ctx* ctx = spng_ctx_new( 0 );
	if ( ctx == nullptr )
		return nullptr;

	RunCodeAtScopeExit(
		spng_ctx_free( ctx );
	);

	if ( spng_set_png_buffer( ctx, buffer.Base(), buffer.TellPut() ) ) 
		return nullptr;

	errcode = CE_ERROR_PARSING_SOURCE;

	struct spng_ihdr ihdr;
	if ( spng_get_ihdr( ctx, &ihdr ) )
		 return nullptr;

	size_t size;
	if ( spng_decoded_image_size( ctx, SPNG_FMT_RGBA8, &size ) )
		return nullptr;

	unsigned char *pResultData = (unsigned char*)MemAlloc_Alloc( size );
	if ( spng_decode_image( ctx, pResultData, size, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS ) )
	{
		MemAlloc_Free( pResultData );
		return nullptr;
	}

	// OK!
	width = ihdr.width;
	height = ihdr.height;
	errcode = CE_SUCCESS;
	return pResultData;
}

unsigned char *ImgUtl_ReadBMPAsRGBA( const char *bmpPath, int &width, int &height, ConversionErrorType &errcode )
{
#ifdef WIN32
	// Load up bitmap
	HBITMAP hBitmap = (HBITMAP)LoadImage(NULL, bmpPath, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_DEFAULTSIZE);

	// Handle failure
	if ( hBitmap == NULL)
	{

		// !KLUDGE! Try to detect what went wrong
		FILE *fp = fopen( bmpPath, "rb" );
		if (fp == NULL)
		{
			errcode = CE_CANT_OPEN_SOURCE_FILE;
		}
		else
		{
			errcode = CE_ERROR_PARSING_SOURCE;
		}
		return NULL;
	}

	BITMAP bitmap;

	GetObject(hBitmap, sizeof(bitmap), &bitmap);

	BITMAPINFO *bitmapInfo;

	bool bUseColorTable = false;
	if (bitmap.bmBitsPixel == 24 || bitmap.bmBitsPixel == 32)
	{
		bitmapInfo = (BITMAPINFO *)malloc(sizeof(BITMAPINFO));
	}
	else if (bitmap.bmBitsPixel == 8 || bitmap.bmBitsPixel == 4 || bitmap.bmBitsPixel == 1)
	{
		int colorsUsed = 1 << bitmap.bmBitsPixel;
		bitmapInfo = (BITMAPINFO *)malloc(colorsUsed * sizeof(RGBQUAD) + sizeof(BITMAPINFO));
		bUseColorTable = true;
	}
	else
	{
		DeleteObject(hBitmap);
		errcode = CE_SOURCE_FILE_BMP_FORMAT_NOT_SUPPORTED;
		return NULL;
	}

	memset(bitmapInfo, 0, sizeof(BITMAPINFO));
	bitmapInfo->bmiHeader.biSize = sizeof(bitmapInfo->bmiHeader);
	if (bUseColorTable)
	{
		bitmapInfo->bmiHeader.biBitCount = bitmap.bmBitsPixel; // need to specify the bits per pixel so GDI will generate a color table for us.
	}

	HDC dc = CreateCompatibleDC(NULL);

	int retcode = GetDIBits(dc, hBitmap, 0, bitmap.bmHeight, NULL, bitmapInfo, DIB_RGB_COLORS);

	DeleteDC(dc);

	if (retcode == 0)
	{
		// error getting the bitmap info for some reason.
		free(bitmapInfo);
		errcode = CE_SOURCE_FILE_BMP_FORMAT_NOT_SUPPORTED;
		return NULL;
	}

	int nDestStride = 4 * bitmap.bmWidth;
	int mem_required = nDestStride * bitmap.bmHeight;  // mem required for copying the data out into RGBA format.

	unsigned char *buf = (unsigned char *)malloc(mem_required);
	if (buf == NULL)
	{
		free(bitmapInfo);
		errcode = CE_MEMORY_ERROR;
		return NULL;
	}

	if (bitmapInfo->bmiHeader.biBitCount == 32)
	{
		for (int y = 0; y < bitmap.bmHeight; ++y)
		{
			unsigned char *pDest = buf + nDestStride * ( ( bitmap.bmHeight - 1 ) - y ); // BMPs are stored upside down
			const unsigned char *pSrc = (unsigned char *)(bitmap.bmBits) + (y * bitmap.bmWidthBytes);

			for (int x = 0; x < bitmap.bmWidth; ++x)
			{

				// Swap BGR -> RGB while copying data
				pDest[0] = pSrc[2]; // R
				pDest[1] = pSrc[1]; // G
				pDest[2] = pSrc[0]; // B
				pDest[3] = pSrc[3]; // A

				pSrc += 4;
				pDest += 4;
			}
		}
	}
	else if (bitmapInfo->bmiHeader.biBitCount == 24)
	{
		for (int y = 0; y < bitmap.bmHeight; ++y)
		{
			unsigned char *pDest = buf + nDestStride * ( ( bitmap.bmHeight - 1 ) - y ); // BMPs are stored upside down
			const unsigned char *pSrc = (unsigned char *)(bitmap.bmBits) + (y * bitmap.bmWidthBytes);

			for (int x = 0; x < bitmap.bmWidth; ++x)
			{

				// Swap BGR -> RGB while copying data
				pDest[0] = pSrc[2]; // R
				pDest[1] = pSrc[1]; // G
				pDest[2] = pSrc[0]; // B
				pDest[3] = 0xff; // A

				pSrc += 3;
				pDest += 4;
			}
		}
	}
	else if (bitmapInfo->bmiHeader.biBitCount == 8)
	{
		// 8-bit 256 color bitmap.
		for (int y = 0; y < bitmap.bmHeight; ++y)
		{
			unsigned char *pDest = buf + nDestStride * ( ( bitmap.bmHeight - 1 ) - y ); // BMPs are stored upside down
			const unsigned char *pSrc = (unsigned char *)(bitmap.bmBits) + (y * bitmap.bmWidthBytes);

			for (int x = 0; x < bitmap.bmWidth; ++x)
			{

				// compute the color map entry for this pixel
				int colorTableEntry = *pSrc;

				// get the color for this color map entry.
				RGBQUAD *rgbQuad = &(bitmapInfo->bmiColors[colorTableEntry]);

				// copy the color values for this pixel to the destination buffer.
				pDest[0] = rgbQuad->rgbRed;
				pDest[1] = rgbQuad->rgbGreen;
				pDest[2] = rgbQuad->rgbBlue;
				pDest[3] = 0xff;

				++pSrc;
				pDest += 4;
			}
		}
	}
	else if (bitmapInfo->bmiHeader.biBitCount == 4)
	{
		// 4-bit 16 color bitmap.
		for (int y = 0; y < bitmap.bmHeight; ++y)
		{
			unsigned char *pDest = buf + nDestStride * ( ( bitmap.bmHeight - 1 ) - y ); // BMPs are stored upside down
			const unsigned char *pSrc = (unsigned char *)(bitmap.bmBits) + (y * bitmap.bmWidthBytes);

			// Two pixels at a time
			for (int x = 0; x < bitmap.bmWidth; x += 2)
			{

				// get the color table entry for this pixel
				int colorTableEntry = (0xf0 & *pSrc) >> 4;

				// get the color values for this pixel's color table entry.
				RGBQUAD *rgbQuad = &(bitmapInfo->bmiColors[colorTableEntry]);

				// copy the pixel's color values to the destination buffer.
				pDest[0] = pSrc[2]; // R
				pDest[1] = pSrc[1]; // G
				pDest[2] = pSrc[0]; // B
				pDest[3] = 0xff; // A

				// make sure we haven't reached the end of the row.
				if ((x + 1) > bitmap.bmWidth)
				{
					break;
				}

				pDest += 4;

				// get the color table entry for this pixel.
				colorTableEntry = 0x0f & *pSrc;

				// get the color values for this pixel's color table entry.
				rgbQuad = &(bitmapInfo->bmiColors[colorTableEntry]);

				// copy the pixel's color values to the destination buffer.
				pDest[0] = pSrc[2]; // R
				pDest[1] = pSrc[1]; // G
				pDest[2] = pSrc[0]; // B
				pDest[3] = 0xff; // A

				++pSrc;
				pDest += 4;
			}
		}
	}
	else if (bitmapInfo->bmiHeader.biBitCount == 1)
	{
		// 1-bit monochrome bitmap.
		for (int y = 0; y < bitmap.bmHeight; ++y)
		{
			unsigned char *pDest = buf + nDestStride * ( ( bitmap.bmHeight - 1 ) - y ); // BMPs are stored upside down
			const unsigned char *pSrc = (unsigned char *)(bitmap.bmBits) + (y * bitmap.bmWidthBytes);

			// Eight pixels at a time
			int x = 0;
			while (x < bitmap.bmWidth)
			{

				RGBQUAD *rgbQuad = NULL;
				int bitMask = 0x80;

				// go through all 8 bits in this byte to get all 8 pixel colors.
				do
				{
					// get the value of the bit for this pixel.
					int bit = *pSrc & bitMask;

					// bit will either be 0 or non-zero since there are only two colors.
					if (bit == 0)
					{
						rgbQuad = &(bitmapInfo->bmiColors[0]);
					}
					else
					{
						rgbQuad = &(bitmapInfo->bmiColors[1]);
					}

					// copy this pixel's color values into the destination buffer.
					pDest[0] = pSrc[2]; // R
					pDest[1] = pSrc[1]; // G
					pDest[2] = pSrc[0]; // B
					pDest[3] = 0xff; // A
					pDest += 4;

					// go to the next pixel.
					++x;
					bitMask = bitMask >> 1;
				} while ((x < bitmap.bmWidth) && (bitMask > 0));

				++pSrc;
			}
		}
	}
	else
	{
		free(bitmapInfo);
		free(buf);
		DeleteObject(hBitmap);
		errcode = CE_SOURCE_FILE_BMP_FORMAT_NOT_SUPPORTED;
		return NULL;
	}

	free(bitmapInfo);
	DeleteObject(hBitmap);

	// OK!
	width = bitmap.bmWidth;
	height = bitmap.bmHeight;
	errcode = CE_SUCCESS;
	return buf;
#else
	errcode = CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	return NULL;
#endif
}

unsigned char *ImgUtl_ReadImageAsRGBA( const char *path, int &width, int &height, ConversionErrorType &errcode )
{

	// Split out the file extension
	const char *pExt = V_GetFileExtension( path );
	if ( pExt )
	{
		if ( !Q_stricmp(pExt, "vtf") )
		{
			return ImgUtl_ReadVTFAsRGBA( path, width, height, errcode );
		}
		if ( !Q_stricmp(pExt, "bmp") )
		{
			return ImgUtl_ReadBMPAsRGBA( path, width, height, errcode );
		}
		if ( !Q_stricmp(pExt, "jpg") || !Q_stricmp(pExt, "jpeg") )
		{
			return ImgUtl_ReadJPEGAsRGBA( path, width, height, errcode );
		}
		if ( !Q_stricmp(pExt, "png") )
		{
			return ImgUtl_ReadPNGAsRGBA( path, width, height, errcode );
		}
		if ( !Q_stricmp(pExt, "tga") )
		{
			TGAHeader header;
			return ImgUtl_ReadTGAAsRGBA( path, width, height, errcode, header );
		}
	}

	errcode = CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	return NULL;
}

// resizes the file specified by tgaPath so that it has dimensions that are
// powers-of-two and is equal to or smaller than (nMaxWidth)x(nMaxHeight).
// also converts from 24-bit RGB to 32-bit RGB (with 8-bit alpha)
ConversionErrorType ImgUtl_ConvertTGA(const char *tgaPath, int nMaxWidth/*=-1*/, int nMaxHeight/*=-1*/)
{
	int tgaWidth = 0, tgaHeight = 0;
	ConversionErrorType errcode;
	TGAHeader tgaHeader;
	unsigned char *srcBuffer = ImgUtl_ReadTGAAsRGBA(tgaPath, tgaWidth, tgaHeight, errcode, tgaHeader);

	if (srcBuffer == NULL)
	{
		return errcode;
	}

	int paddedImageWidth, paddedImageHeight;

	if ((tgaWidth <= 0) || (tgaHeight <= 0))
	{
		free(srcBuffer);
		return CE_ERROR_PARSING_SOURCE;
	}

	// get the nearest power of two that is greater than the width of the image.
	paddedImageWidth = tgaWidth;
	if (!IsPowerOfTwo(paddedImageWidth))
	{
		// width is not a power of two, calculate the next highest power of two value.
		int i = 1;
		while (paddedImageWidth > 1)
		{
			paddedImageWidth = paddedImageWidth >> 1;
			++i;
		}

		paddedImageWidth = paddedImageWidth << i;
	}

	// make sure the width is less than or equal to nMaxWidth
	if (nMaxWidth != -1 && paddedImageWidth > nMaxWidth)
	{
		paddedImageWidth = nMaxWidth;
	}

	// get the nearest power of two that is greater than the height of the image
	paddedImageHeight = tgaHeight;
	if (!IsPowerOfTwo(paddedImageHeight))
	{
		// height is not a power of two, calculate the next highest power of two value.
		int i = 1;
		while (paddedImageHeight > 1)
		{
			paddedImageHeight = paddedImageHeight >> 1;
			++i;
		}

		paddedImageHeight = paddedImageHeight << i;
	}

	// make sure the height is less than or equal to nMaxHeight
	if (nMaxHeight != -1 && paddedImageHeight > nMaxHeight)
	{
		paddedImageHeight = nMaxHeight;
	}

	// compute the amount of stretching that needs to be done to both width and height to get the image to fit.
	float widthRatio = (float)paddedImageWidth / tgaWidth;
	float heightRatio = (float)paddedImageHeight / tgaHeight;

	int finalWidth;
	int finalHeight;

	// compute the final dimensions of the stretched image.
	if (widthRatio < heightRatio)
	{
		finalWidth = paddedImageWidth;
		finalHeight = (int)(tgaHeight * widthRatio + 0.5f);
		// i.e.  for 1x1 size pixels in the resized image we will take color from sourceRatio x sourceRatio sized pixels in the source image.
	}
	else if (heightRatio < widthRatio)
	{
		finalHeight = paddedImageHeight;
		finalWidth = (int)(tgaWidth * heightRatio + 0.5f);
	}
	else
	{
		finalHeight = paddedImageHeight;
		finalWidth = paddedImageWidth;
	}

	unsigned char *resizeBuffer = (unsigned char *)malloc(finalWidth * finalHeight * 4);

	// do the actual stretching
	ImgUtl_StretchRGBAImage(srcBuffer, tgaWidth, tgaHeight, resizeBuffer, finalWidth, finalHeight);

	free(srcBuffer);  // don't need this anymore.

	///////////////////////////////////////////////////////////////////////
	///// need to pad the image so both dimensions are power of two's /////
	///////////////////////////////////////////////////////////////////////
	unsigned char *finalBuffer = (unsigned char *)malloc(paddedImageWidth * paddedImageHeight * 4);
	ImgUtl_PadRGBAImage(resizeBuffer, finalWidth, finalHeight, finalBuffer, paddedImageWidth, paddedImageHeight);

	FILE *outfile = fopen(tgaPath, "wb");
	if (outfile == NULL)
	{
		free(resizeBuffer);
		free(finalBuffer);

		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	tgaHeader.width = paddedImageWidth;
	tgaHeader.height = paddedImageHeight;

	WriteTGAHeader(outfile, tgaHeader);

	// Write the image data --- remember that TGA uses BGRA data
	int numPixels = paddedImageWidth * paddedImageHeight;
	for (int i = 0 ; i < numPixels ; ++i )
	{
		fputc( finalBuffer[i*4 + 2], outfile ); // B
		fputc( finalBuffer[i*4 + 1], outfile ); // G
		fputc( finalBuffer[i*4    ], outfile ); // R
		fputc( finalBuffer[i*4 + 3], outfile ); // A
	}

	fclose(outfile);

	free(resizeBuffer);
	free(finalBuffer);

	return CE_SUCCESS;
}

// resize by stretching (or compressing) an RGBA image pointed to by srcBuf into the buffer pointed to by destBuf.
// the buffers are assumed to be sized appropriately to accomidate RGBA images of the given widths and heights.
ConversionErrorType ImgUtl_StretchRGBAImage(const unsigned char *srcBuf, const int srcWidth, const int srcHeight,
									 unsigned char *destBuf, const int destWidth, const int destHeight)
{
	if ((srcBuf == NULL) || (destBuf == NULL))
	{
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	int destRow,destColumn;

	float ratioX = (float)srcWidth / (float)destWidth;
	float ratioY = (float)srcHeight / (float)destHeight;

	// loop through all the pixels in the destination image.
	for (destRow = 0; destRow < destHeight; ++destRow)
	{
		for (destColumn = 0; destColumn < destWidth; ++destColumn)
		{
			// calculate the center of the pixel in the source image.
			float srcCenterX = ratioX * (destColumn + 0.5f);
			float srcCenterY = ratioY * (destRow + 0.5f);

			// calculate the starting and ending coords for this destination pixel in the source image.
			float srcStartX = srcCenterX - (ratioX / 2.0f);
			if (srcStartX < 0.0f)
			{
				srcStartX = 0.0f; // this should never happen, but just in case.
			}

			float srcStartY = srcCenterY - (ratioY / 2.0f);
			if (srcStartY < 0.0f)
			{
				srcStartY = 0.0f; // this should never happen, but just in case.
			}

			float srcEndX = srcCenterX + (ratioX / 2.0f);
			if (srcEndX > srcWidth)
			{
				srcEndX = srcWidth; // this should never happen, but just in case.
			}

			float srcEndY = srcCenterY + (ratioY / 2.0f);
			if (srcEndY > srcHeight)
			{
				srcEndY = srcHeight; // this should never happen, but just in case.
			}

			// Calculate the percentage of each source pixels' contribution to the destination pixel color.

			float srcCurrentX; // initialized at the start of the y loop.
			float srcCurrentY = srcStartY;

			float destRed = 0.0f;
			float destGreen = 0.0f;
			float destBlue = 0.0f;
			float destAlpha = 0.0f;

			//// loop for the parts of the source image that will contribute color to the destination pixel.
			while (srcCurrentY < srcEndY)
			{
				float srcCurrentEndY = (float)((int)srcCurrentY + 1);
				if (srcCurrentEndY > srcEndY)
				{
					srcCurrentEndY = srcEndY;
				}

				float srcCurrentHeight = srcCurrentEndY - srcCurrentY;

				srcCurrentX = srcStartX;

				while (srcCurrentX < srcEndX)
				{
					float srcCurrentEndX = (float)((int)srcCurrentX + 1);
					if (srcCurrentEndX > srcEndX)
					{
						srcCurrentEndX = srcEndX;
					}
					float srcCurrentWidth = srcCurrentEndX - srcCurrentX;

					// compute the percentage of the destination pixel's color this source pixel will contribute.
					float srcColorPercentage = (srcCurrentWidth / ratioX) * (srcCurrentHeight / ratioY);

					int srcCurrentPixelX = (int)srcCurrentX;
					int srcCurrentPixelY = (int)srcCurrentY;

					// get the color values for this source pixel.
					unsigned char srcCurrentRed = srcBuf[(srcCurrentPixelY * srcWidth * 4) + (srcCurrentPixelX * 4)];
					unsigned char srcCurrentGreen = srcBuf[(srcCurrentPixelY * srcWidth * 4) + (srcCurrentPixelX * 4) + 1];
					unsigned char srcCurrentBlue = srcBuf[(srcCurrentPixelY * srcWidth * 4) + (srcCurrentPixelX * 4) + 2];
					unsigned char srcCurrentAlpha = srcBuf[(srcCurrentPixelY * srcWidth * 4) + (srcCurrentPixelX * 4) + 3];

					// add the color contribution from this source pixel to the destination pixel.
					destRed += srcCurrentRed * srcColorPercentage;
					destGreen += srcCurrentGreen * srcColorPercentage;
					destBlue += srcCurrentBlue * srcColorPercentage;
					destAlpha += srcCurrentAlpha * srcColorPercentage;

					srcCurrentX = srcCurrentEndX;
				}

				srcCurrentY = srcCurrentEndY;
			}

			// assign the computed color to the destination pixel, round to the nearest value.  Make sure the value doesn't exceed 255.
			destBuf[(destRow * destWidth * 4) + (destColumn * 4)] = min((int)(destRed + 0.5f), 255);
			destBuf[(destRow * destWidth * 4) + (destColumn * 4) + 1] = min((int)(destGreen + 0.5f), 255);
			destBuf[(destRow * destWidth * 4) + (destColumn * 4) + 2] = min((int)(destBlue + 0.5f), 255);
			destBuf[(destRow * destWidth * 4) + (destColumn * 4) + 3] = min((int)(destAlpha + 0.5f), 255);
		} // column loop
	} // row loop

	return CE_SUCCESS;
}

ConversionErrorType ImgUtl_PadRGBAImage(const unsigned char *srcBuf, const int srcWidth, const int srcHeight,
								 unsigned char *destBuf, const int destWidth, const int destHeight)
{
	if ((srcBuf == NULL) || (destBuf == NULL))
	{
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	memset(destBuf, 0, destWidth * destHeight * 4);

	if ((destWidth < srcWidth) || (destHeight < srcHeight))
	{
		return CE_ERROR_PARSING_SOURCE;
	}

	if ((srcWidth == destWidth) && (srcHeight == destHeight))
	{
		// no padding is needed, just copy the buffer straight over and call it done.
		memcpy(destBuf, srcBuf, destWidth * destHeight * 4);
		return CE_SUCCESS;
	}

	if (destWidth == srcWidth)
	{
		// only the top and bottom of the image need padding.
		// do this separately since we can do this more efficiently than the other cases.
		int numRowsToPad = (destHeight - srcHeight) / 2;
		memcpy(destBuf + (numRowsToPad * destWidth * 4), srcBuf, srcWidth * srcHeight * 4);
	}
	else
	{
		int numColumnsToPad = (destWidth - srcWidth) / 2;
		int numRowsToPad = (destHeight - srcHeight) / 2;
		int lastRow = numRowsToPad + srcHeight;
		int row;
		for (row = numRowsToPad; row < lastRow; ++row)
		{
			unsigned char * destOffset = destBuf + (row * destWidth * 4) + (numColumnsToPad * 4);
			const unsigned char * srcOffset = srcBuf + ((row - numRowsToPad) * srcWidth * 4);
			memcpy(destOffset, srcOffset, srcWidth * 4);
		}
	}

	return CE_SUCCESS;
}

// convert TGA file at the given location to a VTF file of the same root name at the same location.
ConversionErrorType ImgUtl_ConvertTGAToVTF(const char *tgaPath, int nMaxWidth/*=-1*/, int nMaxHeight/*=-1*/ )
{
	FILE *infile = fopen(tgaPath, "rb");
	if (infile == NULL)
	{
		Msg( "Failed to open TGA: %s\n", tgaPath);
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// read out the header of the image.
	TGAHeader header;
	ImgUtl_ReadTGAHeader(infile, header);

	// check to make sure that the TGA has the proper dimensions and size.
	if (!IsPowerOfTwo(header.width) || !IsPowerOfTwo(header.height))
	{
		fclose(infile);
		Msg( "Failed to open TGA - size dimensions (%d, %d) not power of 2: %s\n", header.width, header.height, tgaPath);
		return CE_SOURCE_FILE_SIZE_NOT_SUPPORTED;
	}

	// check to make sure that the TGA isn't too big, if we care.
	if ( ( nMaxWidth != -1 && header.width > nMaxWidth ) || ( nMaxHeight != -1 && header.height > nMaxHeight ) )
	{
		fclose(infile);
		Msg( "Failed to open TGA - dimensions too large (%d, %d) (max: %d, %d): %s\n", header.width, header.height, nMaxWidth, nMaxHeight, tgaPath);
		return CE_SOURCE_FILE_SIZE_NOT_SUPPORTED;
	}

	int imageMemoryFootprint = header.width * header.height * header.bits / 8;

	CUtlBuffer inbuf(0, imageMemoryFootprint);

	// read in the image
	int nBytesRead = (int)fread(inbuf.Base(), imageMemoryFootprint, 1, infile);

	fclose(infile);
	inbuf.SeekPut( CUtlBuffer::SEEK_HEAD, nBytesRead );

	// load vtex_dll.dll and get the interface to it.
	CSysModule *vtexmod = Sys_LoadModule("vtex_dll");
	if (vtexmod == NULL)
	{
		Msg( "Failed to open TGA conversion module vtex_dll: %s\n", tgaPath);
		return CE_ERROR_LOADING_DLL;
	}

	CreateInterfaceFn factory = Sys_GetFactory(vtexmod);
	if (factory == NULL)
	{
		Sys_UnloadModule(vtexmod);
		Msg( "Failed to open TGA conversion module vtex_dll Factory: %s\n", tgaPath);
		return CE_ERROR_LOADING_DLL;
	}

	IVTex *vtex = (IVTex *)factory(IVTEX_VERSION_STRING, NULL);
	if (vtex == NULL)
	{
		Sys_UnloadModule(vtexmod);
		Msg( "Failed to open TGA conversion module vtex_dll Factory (is null): %s\n", tgaPath);
		return CE_ERROR_LOADING_DLL;
	}

	char *vtfParams[4];

	// the 0th entry is skipped cause normally thats the program name.
	vtfParams[0] = "";
	vtfParams[1] = "-quiet";
	vtfParams[2] = "-dontusegamedir";
	vtfParams[3] = (char *)tgaPath;

	// call vtex to do the conversion.
	vtex->VTex(4, vtfParams);  // how do we know this works?

	Sys_UnloadModule(vtexmod);

	return CE_SUCCESS;
}

static void DoCopyFile( const char *source, const char *destination )
{
#if defined( WIN32 )
	CopyFile( source, destination, true );
#elif defined( OSX )
	copyfile( source, destination, NULL, COPYFILE_ALL );
#elif defined( ENGINE_DLL )
	::COM_CopyFile( source, destination );
#elif REPLAY_DLL
	g_pEngine->CopyFile( source, destination );
#else
	engine->CopyLocalFile( source, destination );
#endif
}

static void DoDeleteFile( const char *filename )
{
#ifdef WIN32
	DeleteFile( filename );
#else
	unlink( filename );
#endif
}

ConversionErrorType	ImgUtl_ConvertToVTFAndDumpVMT( const char *pInPath, const char *pMaterialsSubDir, int nMaxWidth/*=-1*/, int nMaxHeight/*=-1*/ )
{
#ifndef _XBOX
	if ((pInPath == NULL) || (pInPath[0] == 0))
	{
		return CE_ERROR_PARSING_SOURCE;
	}

	ConversionErrorType nErrorCode = CE_SUCCESS;

	// get the extension of the file we're to convert
	char extension[MAX_PATH];
	const char *constchar = pInPath + strlen(pInPath);
	while ((constchar > pInPath) && (*(constchar-1) != '.'))
	{
		--constchar;
	}
	Q_strncpy(extension, constchar, MAX_PATH);

	bool deleteIntermediateTGA = false;
	bool deleteIntermediateVTF = false;
	bool convertTGAToVTF = true;
	char tgaPath[MAX_PATH*2];
	char *c;
	bool failed = false;

	Q_strncpy(tgaPath, pInPath, sizeof(tgaPath));

	// Construct a TGA version if necessary
	if (stricmp(extension, "tga"))
	{
		//  It is not a TGA file, so create a temporary file name for the TGA you have to create

		c = tgaPath + strlen(tgaPath);
		while ((c > tgaPath) && (*(c-1) != '\\') && (*(c-1) != '/'))
		{
			--c;
		}
		*c = 0;

		char origpath[MAX_PATH*2];
		Q_strncpy(origpath, tgaPath, sizeof(origpath));

		//  Look for an empty temp file - find the first one that doesn't exist.
		int index = 0;
		do {
			Q_snprintf(tgaPath, sizeof(tgaPath), "%stemp%d.tga", origpath, index);
			++index;
		} while (_access(tgaPath, 0) != -1);


		//  Convert the other formats to TGA

		//  jpeg files
		//
		if (!stricmp(extension, "jpg") || !stricmp(extension, "jpeg"))
		{
			// convert from the jpeg file format to the TGA file format
			nErrorCode = ImgUtl_ConvertJPEGToTGA(pInPath, tgaPath, false);
			if (nErrorCode == CE_SUCCESS)
			{
				deleteIntermediateTGA = true;
			}
			else
			{
				failed = true;
			}
		}
		//  bmp files
		//
		else if (!stricmp(extension, "bmp"))
		{
			// convert from the bmp file format to the TGA file format
			nErrorCode = ImgUtl_ConvertBMPToTGA(pInPath, tgaPath);

			if (nErrorCode == CE_SUCCESS)
			{
				deleteIntermediateTGA = true;
			}
			else
			{
				failed = true;
			}
		}
		//  vtf files
		//
		else if (!stricmp(extension, "vtf"))
		{
			// if the file is already in the vtf format there's no need to convert it.
			convertTGAToVTF = false;
			
		}
	}

	// if we now have a TGA file, convert it to VTF 
	if (convertTGAToVTF && !failed)
	{
		nErrorCode = ImgUtl_ConvertTGA( tgaPath, nMaxWidth, nMaxHeight ); // resize TGA so that it has power-of-two dimensions with a max size of (nMaxWidth)x(nMaxHeight).
		if (nErrorCode != CE_SUCCESS)
		{
			failed = true;
		}

		if (!failed)
		{
			char tempPath[MAX_PATH*2];
			Q_strncpy(tempPath, tgaPath, sizeof(tempPath));

			nErrorCode = ImgUtl_ConvertTGAToVTF( tempPath, nMaxWidth, nMaxHeight );
			if (nErrorCode == CE_SUCCESS)
			{
				deleteIntermediateVTF = true;
			}
			else
			{
				Msg( "Failed to convert TGA to VTF: %s\n", tempPath);
				failed = true;
			}
		}
	}

	//  At this point everything should be a VTF file

	char finalPath[MAX_PATH*2];
	finalPath[0] = 0;
	char vtfPath[MAX_PATH*2];
	vtfPath[0] = 0;


	//  If we haven't failed so far, create a VMT to go with this VTF 
	if (!failed)
	{

		//  If I had to convert from another filetype (i.e. the original was NOT a .vtf)
		if ( convertTGAToVTF )
		{

			Q_strncpy(vtfPath, tgaPath, sizeof(vtfPath));

			// rename the tga file to be a vtf file.
			c = vtfPath + strlen(vtfPath);
			while ((c > vtfPath) && (*(c-1) != '.'))
			{
				--c;
			}
			*c = 0;
			Q_strncat(vtfPath, "vtf", sizeof(vtfPath), COPY_ALL_CHARACTERS);

		} 
		else
		{
			// We were handed a vtf file originally, so use it.
			Q_strncpy(vtfPath, pInPath, sizeof(vtfPath));
		}

		// get the vtfFilename from the path.
		const char *vtfFilename = pInPath + strlen(pInPath);
		while ((vtfFilename > pInPath) && (*(vtfFilename-1) != '\\') && (*(vtfFilename-1) != '/'))
		{
			--vtfFilename;
		}

		// Create a safe version of pOutDir with corrected slashes
		char szOutDir[MAX_PATH*2];
		V_strcpy_safe( szOutDir, IsPosix() ? "/materials/" : "\\materials\\" );
		if ( pMaterialsSubDir[0] == '\\' || pMaterialsSubDir[0] == '/' )
			pMaterialsSubDir = pMaterialsSubDir + 1;
		V_strcat_safe(szOutDir, pMaterialsSubDir, sizeof(szOutDir) );
		Q_StripTrailingSlash( szOutDir );
		Q_AppendSlash( szOutDir, sizeof(szOutDir) );
		Q_FixSlashes( szOutDir, CORRECT_PATH_SEPARATOR );

#ifdef ENGINE_DLL
		Q_strncpy(finalPath, com_gamedir, sizeof(finalPath));
#elif REPLAY_DLL
		Q_strncpy(finalPath, g_pEngine->GetGameDir(), sizeof(finalPath));
#else
		Q_strncpy(finalPath, engine->GetGameDirectory(), sizeof(finalPath));
#endif
		Q_strncat(finalPath, szOutDir, sizeof(finalPath), COPY_ALL_CHARACTERS);
		Q_strncat(finalPath, vtfFilename, sizeof(finalPath), COPY_ALL_CHARACTERS);
	
		c = finalPath + strlen(finalPath);
		while ((c > finalPath) && (*(c-1) != '.'))
		{
			--c;
		}
		*c = 0;
		Q_strncat(finalPath,"vtf", sizeof(finalPath), COPY_ALL_CHARACTERS);

		// make sure the directory exists before we try to copy the file.
		g_pFullFileSystem->CreateDirHierarchy(szOutDir + 1, "GAME");
		//g_pFullFileSystem->CreateDirHierarchy("materials/VGUI/logos/", "GAME");

		// write out the spray VMT file.
		if ( strcmp(vtfPath, finalPath) )  // If they're not already the same
		{
			nErrorCode = ImgUtl_WriteGenericVMT(finalPath, pMaterialsSubDir);
			if (nErrorCode != CE_SUCCESS)
			{
				failed = true;
			}

			if (!failed)
			{
				// copy vtf file to the final location, only if we're not already in vtf

				DoCopyFile( vtfPath, finalPath );
			}
		}
	}

	// delete the intermediate VTF file if one was made.
	if (deleteIntermediateVTF)
	{
		DoDeleteFile( vtfPath );
		
		// the TGA->VTF conversion process generates a .txt file if one wasn't already there.
		// in this case, delete the .txt file.
		c = vtfPath + strlen(vtfPath);
		while ((c > vtfPath) && (*(c-1) != '.'))
		{
			--c;
		}
		Q_strncpy(c, "txt", (int)(sizeof(vtfPath)-(c-vtfPath)));

		DoDeleteFile( vtfPath );
	}

	// delete the intermediate TGA file if one was made.
	if (deleteIntermediateTGA)
	{
		DoDeleteFile( tgaPath );
	}

	return nErrorCode;
#endif
}

ConversionErrorType ImgUtl_WriteGenericVMT( const char *vtfPath, const char *pMaterialsSubDir )
{
	if (vtfPath == NULL || pMaterialsSubDir == NULL )
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	// make the vmt filename
	char vmtPath[MAX_PATH*4];
	Q_strncpy(vmtPath, vtfPath, sizeof(vmtPath));
	char *c = vmtPath + strlen(vmtPath);
	while ((c > vmtPath) && (*(c-1) != '.'))
	{
		--c;
	}
	Q_strncpy(c, "vmt", (int)(sizeof(vmtPath) - (c - vmtPath)));

	// get the root filename for the vtf file
	char filename[MAX_PATH];
	while ((c > vmtPath) && (*(c-1) != '/') && (*(c-1) != '\\'))
	{
		--c;
	}

	int i = 0;
	while ((*c != 0) && (*c != '.'))
	{
		filename[i++] = *(c++);
	}
	filename[i] = 0;

	// create the vmt file.
	FILE *vmtFile = fopen(vmtPath, "w");
	if (vmtFile == NULL)
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	// make a copy of the subdir and remove any trailing slash
	char szMaterialsSubDir[ MAX_PATH*2 ];
	V_strcpy_safe( szMaterialsSubDir, pMaterialsSubDir );
	V_StripTrailingSlash( szMaterialsSubDir );

	// fix slashes
	V_FixSlashes( szMaterialsSubDir );

	// write the contents of the file.
	fprintf(vmtFile, "\"UnlitGeneric\"\n{\n\t\"$basetexture\"	\"%s%c%s\"\n\t\"$translucent\" \"1\"\n\t\"$ignorez\" \"1\"\n\t\"$vertexcolor\" \"1\"\n\t\"$vertexalpha\" \"1\"\n}\n", szMaterialsSubDir, CORRECT_PATH_SEPARATOR, filename);

	fclose(vmtFile);

	return CE_SUCCESS;
}

ConversionErrorType ImgUtl_WriteRGBAAsPNGToBuffer( const unsigned char *pRGBAData, int nWidth, int nHeight, CUtlBuffer &bufOutData, bool bIncludesAlpha )
{
	spng_ctx* enc = spng_ctx_new( SPNG_CTX_ENCODER );
	if ( enc == nullptr ) {
		return CE_MEMORY_ERROR;
	}

	RunCodeAtScopeExit( 
		spng_ctx_free( enc );
	);

	int nPixelByteSize = ( bIncludesAlpha ) ? 4 : 3;

	struct spng_ihdr ihdr;
	memset( &ihdr, 0, sizeof( ihdr ) );
	ihdr.width = nWidth;
	ihdr.height = nHeight;
	ihdr.bit_depth = 8;
	ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR;
	if ( bIncludesAlpha )
	{
		ihdr.color_type = SPNG_COLOR_TYPE_TRUECOLOR_ALPHA;
	}
	
	
	ihdr.compression_method = 0;
	ihdr.filter_method = SPNG_FILTER_NONE;
	ihdr.interlace_method = SPNG_INTERLACE_NONE;

	spng_set_ihdr( enc, &ihdr );
	spng_set_option( enc, SPNG_ENCODE_TO_BUFFER, 1 );

	if ( spng_encode_image( enc, pRGBAData, ( nWidth * nHeight * nPixelByteSize ), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE ) )
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	int err;
	size_t png_buf_sz;
	void* png_buf = spng_get_png_buffer( enc, &png_buf_sz, &err );
	if ( err != 0 )
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	bufOutData.Put( png_buf, int( png_buf_sz ) );

	MemAlloc_Free( png_buf );

	return CE_SUCCESS;
}

//-----------------------------------------------------------------------------
// Purpose:  Initialize destination --- called by jpeg_start_compress
//  before any data is actually written.
//-----------------------------------------------------------------------------
METHODDEF(void) init_destination (j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = ( JPEGDestinationManager_t *) cinfo->dest;

	// Allocate the output buffer --- it will be released when done with image
	dest->buffer = (byte *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		JPEG_OUTPUT_BUF_SIZE * sizeof(byte));

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = JPEG_OUTPUT_BUF_SIZE;
}

//-----------------------------------------------------------------------------
// Purpose: Empty the output buffer --- called whenever buffer fills up.
// Input  : boolean - 
//-----------------------------------------------------------------------------
METHODDEF(boolean) empty_output_buffer (j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = ( JPEGDestinationManager_t * ) cinfo->dest;

	CUtlBuffer *buf = dest->pBuffer;

	// Add some data
	buf->Put( dest->buffer, JPEG_OUTPUT_BUF_SIZE );

	dest->pub.next_output_byte = dest->buffer;
	dest->pub.free_in_buffer = JPEG_OUTPUT_BUF_SIZE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: Terminate destination --- called by jpeg_finish_compress
// after all data has been written.  Usually needs to flush buffer.
//
// NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
// application must deal with any cleanup that should happen even
// for error exit.
//-----------------------------------------------------------------------------
METHODDEF(void) term_destination (j_compress_ptr cinfo)
{
	JPEGDestinationManager_t *dest = (JPEGDestinationManager_t *) cinfo->dest;
	long long int datacount = ( long long int )JPEG_OUTPUT_BUF_SIZE - (long long int)dest->pub.free_in_buffer;

	CUtlBuffer *buf = dest->pBuffer;

	/* Write any data remaining in the buffer */
    if (datacount > 0)
    {
        buf->Put( dest->buffer, (int)Min(datacount, (long long int)INT_MAX) );
    }
}

//-----------------------------------------------------------------------------
// Purpose: Set up functions for writing data to a CUtlBuffer instead of FILE *
//-----------------------------------------------------------------------------
GLOBAL(void) jpeg_UtlBuffer_dest (j_compress_ptr cinfo, CUtlBuffer *pBuffer )
{
    JPEGDestinationManager_t *dest;
    
    /* The destination object is made permanent so that multiple JPEG images
    * can be written to the same file without re-executing jpeg_stdio_dest.
    * This makes it dangerous to use this manager and a different destination
    * manager serially with the same JPEG object, because their private object
    * sizes may be different.  Caveat programmer.
    */
    if (cinfo->dest == NULL) {  /* first time for this JPEG object? */
        cinfo->dest = (struct jpeg_destination_mgr *)
            (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
            sizeof(JPEGDestinationManager_t));
    }
    
    dest = ( JPEGDestinationManager_t * ) cinfo->dest;

    dest->pub.init_destination      = init_destination;
    dest->pub.empty_output_buffer   = empty_output_buffer;
    dest->pub.term_destination      = term_destination;
    dest->pBuffer                   = pBuffer;
}

//-----------------------------------------------------------------------------
// Purpose: Write three channel RGB data to a JPEG file
//-----------------------------------------------------------------------------
bool ImgUtl_WriteRGBToJPEG( unsigned char *pSrcBuf, unsigned int nSrcWidth, unsigned int nSrcHeight, const char *lpszFilename )
{
	CUtlBuffer dstBuf;

	JSAMPROW row_pointer[1];     // pointer to JSAMPLE row[s]
	int row_stride;              // physical row width in image buffer

	// stderr handler
	struct jpeg_error_mgr jerr;

	// compression data structure
	struct jpeg_compress_struct cinfo;

	row_stride = nSrcWidth * 3; // JSAMPLEs per row in image_buffer

	// point at stderr
	cinfo.err = jpeg_std_error(&jerr);

	// create compressor
	jpeg_create_compress(&cinfo);

	// Hook CUtlBuffer to compression
	jpeg_UtlBuffer_dest(&cinfo, &dstBuf );

	// image width and height, in pixels
	cinfo.image_width = nSrcWidth;
	cinfo.image_height = nSrcHeight;
	// RGB is 3 component
	cinfo.input_components = 3;
	// # of color components per pixel
	cinfo.in_color_space = JCS_RGB;

	// Apply settings
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, TRUE );

	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Write scanlines
	while ( cinfo.next_scanline < cinfo.image_height ) 
	{
		row_pointer[ 0 ] = &pSrcBuf[ cinfo.next_scanline * row_stride ];
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	}

	// Finalize image
	jpeg_finish_compress(&cinfo);

	// Cleanup
	jpeg_destroy_compress(&cinfo);
	
	return CE_SUCCESS;
}

ConversionErrorType ImgUtl_WriteRGBAAsJPEGToBuffer( const unsigned char *pRGBAData, int nWidth, int nHeight, CUtlBuffer &bufOutData, int nStride )
{
#if !defined( _X360 )

	JSAMPROW row_pointer[1];     // pointer to JSAMPLE row[s]
	int row_stride;              // physical row width in image buffer

	// stderr handler
	struct jpeg_error_mgr jerr;

	// compression data structure
	struct jpeg_compress_struct cinfo;

	row_stride = nWidth * 4;

	// point at stderr
	cinfo.err = jpeg_std_error(&jerr);

	// create compressor
	jpeg_create_compress(&cinfo);

	// Hook CUtlBuffer to compression
	jpeg_UtlBuffer_dest(&cinfo, &bufOutData );

	// image width and height, in pixels
	cinfo.image_width = nWidth;
	cinfo.image_height = nHeight;
	// RGB is 3 component
	cinfo.input_components = 3;
	// # of color components per pixel
	cinfo.in_color_space = JCS_RGB;

	// Apply settings
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, 100, TRUE );

	// Start compressor
	jpeg_start_compress(&cinfo, TRUE);

	// Write scanlines
	unsigned char *pDstRow = (unsigned char *)malloc( sizeof(unsigned char) * nWidth * 4 );
	while ( cinfo.next_scanline < cinfo.image_height ) 
	{
		const unsigned char *pSrcRow = &(pRGBAData[cinfo.next_scanline * row_stride]);
		// convert row from RGBA to RGB
		for ( int x = nWidth-1 ; x >= 0 ; --x )
		{
			pDstRow[x*3+2] = pSrcRow[x*4+2];
			pDstRow[x*3+1] = pSrcRow[x*4+1];
			pDstRow[x*3] = pSrcRow[x*4];
		}
		row_pointer[ 0 ] = pDstRow;
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	}

	// Finalize image
	jpeg_finish_compress(&cinfo);

	// Cleanup
	jpeg_destroy_compress(&cinfo);

	free( pDstRow );

	return CE_SUCCESS;
#else
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
#endif
}

ConversionErrorType ImgUtl_LoadBitmap( const char *pszFilename, Bitmap_t &bitmap )
{
	bitmap.Clear();
	ConversionErrorType nErrorCode;
	int width, height;
	unsigned char *buffer = ImgUtl_ReadImageAsRGBA( pszFilename, width, height, nErrorCode );
	if ( nErrorCode != CE_SUCCESS )
	{
		return nErrorCode;
	}

	// Install the buffer into the bitmap, and transfer ownership
	bitmap.SetBuffer( width, height, IMAGE_FORMAT_RGBA8888, buffer, true, width*4 );
	return CE_SUCCESS;
}

static ConversionErrorType ImgUtl_LoadJPEGBitmapFromBuffer( CUtlBuffer &fileData, Bitmap_t &bitmap )
{
	// @todo implement
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
}

static ConversionErrorType ImgUtl_SaveJPEGBitmapToBuffer( CUtlBuffer &fileData, const Bitmap_t &bitmap )
{
	if ( !bitmap.IsValid() )
	{
		Assert( bitmap.IsValid() );
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// Sorry, only RGBA8888 supported right now
	if ( bitmap.Format() != IMAGE_FORMAT_RGBA8888 )
	{
		Assert( bitmap.Format() == IMAGE_FORMAT_RGBA8888 );
		return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	}

	// Do it
	ConversionErrorType result = ImgUtl_WriteRGBAAsJPEGToBuffer(
		bitmap.GetBits(),
		bitmap.Width(),
		bitmap.Height(),
		fileData,
		bitmap.Stride()
		);
	return result;
}

ConversionErrorType ImgUtl_LoadBitmapFromBuffer( CUtlBuffer &fileData, Bitmap_t &bitmap, ImageFileFormat eImageFileFormat )
{
	switch ( eImageFileFormat )
	{
	case kImageFileFormat_PNG:
		return ImgUtl_LoadPNGBitmapFromBuffer( fileData, bitmap );
	case kImageFileFormat_JPG:
		return ImgUtl_LoadJPEGBitmapFromBuffer( fileData, bitmap );
	}
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
}

ConversionErrorType ImgUtl_SaveBitmapToBuffer( CUtlBuffer &fileData, const Bitmap_t &bitmap, ImageFileFormat eImageFileFormat )
{
	switch ( eImageFileFormat )
	{
	case kImageFileFormat_PNG:
		return ImgUtl_SavePNGBitmapToBuffer( fileData, bitmap );
	case kImageFileFormat_JPG:
		return ImgUtl_SaveJPEGBitmapToBuffer( fileData, bitmap );
	}
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
}

ConversionErrorType ImgUtl_LoadPNGBitmapFromBuffer( CUtlBuffer &fileData, Bitmap_t &bitmap )
{
	bitmap.Clear();
	ConversionErrorType nErrorCode;
	int width, height;
	unsigned char *buffer = ImgUtl_ReadPNGAsRGBAFromBuffer( fileData, width, height, nErrorCode );
	if ( nErrorCode != CE_SUCCESS )
	{
		return nErrorCode;
	}

	// Install the buffer into the bitmap, and transfer ownership
	bitmap.SetBuffer( width, height, IMAGE_FORMAT_RGBA8888, buffer, true, width*4 );
	return CE_SUCCESS;
}

ConversionErrorType ImgUtl_GetPNGSize( CUtlBuffer &fileData, uint32_t &uWidth, uint32_t &uHeight )
{
	ConversionErrorType errcode = CE_MEMORY_ERROR;	
	spng_ctx* ctx = spng_ctx_new( 0 );
	if ( ctx == nullptr )
		return errcode;

	RunCodeAtScopeExit(
		spng_ctx_free( ctx );
	);

	if ( spng_set_png_buffer( ctx, fileData.Base(), fileData.TellPut() ) ) 
		return errcode;

	errcode = CE_ERROR_PARSING_SOURCE;

	struct spng_ihdr ihdr;
	if ( spng_get_ihdr( ctx, &ihdr ) )
		 return errcode;

	uWidth = ihdr.width;
	uHeight = ihdr.height;
	errcode = CE_SUCCESS;

	return errcode;
}

ConversionErrorType ImgUtl_SavePNGBitmapToBuffer( CUtlBuffer &fileData, const Bitmap_t &bitmap )
{
	if ( !bitmap.IsValid() )
	{
		Assert( bitmap.IsValid() );
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// Sorry, only RGBA8888 supported right now
	if ( bitmap.Format() != IMAGE_FORMAT_RGBA8888 )
	{
		Assert( bitmap.Format() == IMAGE_FORMAT_RGBA8888 );
		return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	}

	// Do it
	ConversionErrorType result = ImgUtl_WriteRGBAAsPNGToBuffer(
		bitmap.GetBits(),
		bitmap.Width(),
		bitmap.Height(),
		fileData,
		bitmap.Stride()
	);
	return result;
}

ConversionErrorType ImgUtl_ResizeBitmap( Bitmap_t &destBitmap, int nWidth, int nHeight, const Bitmap_t *pImgSource )
{

	// Check for resizing in place, then save off data into a temp
	Bitmap_t temp;
	if ( pImgSource == NULL || pImgSource == &destBitmap )
	{
		temp.MakeLogicalCopyOf( destBitmap, destBitmap.GetOwnsBuffer() );
		pImgSource = &temp;
	}

	// No source image?
	if ( !pImgSource->IsValid() )
	{
		Assert( pImgSource->IsValid() );
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// Sorry, we're using an existing rescaling routine that
	// only withs for RGBA images with assumed stride
	if (
		pImgSource->Format() != IMAGE_FORMAT_RGBA8888
		|| pImgSource->Stride() != pImgSource->Width()*4
	) {
		Assert( pImgSource->Format() == IMAGE_FORMAT_RGBA8888 );
		Assert( pImgSource->Stride() == pImgSource->Width()*4 );
		return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	}

	// Allocate buffer
	destBitmap.Init( nWidth, nHeight, IMAGE_FORMAT_RGBA8888 );

	// Something wrong?
	if ( !destBitmap.IsValid() )
	{
		Assert( destBitmap.IsValid() );
		return CE_MEMORY_ERROR;
	}

	// Do it
	return ImgUtl_StretchRGBAImage(
		pImgSource->GetBits(), pImgSource->Width(), pImgSource->Height(),
		destBitmap.GetBits(), destBitmap.Width(), destBitmap.Height()
	);
}
