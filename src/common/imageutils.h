//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef IMAGECONVERSION_H
#define IMAGECONVERSION_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/basetypes.h"

class CUtlBuffer;
struct Bitmap_t;

enum ConversionErrorType
{
	CE_SUCCESS,
	CE_MEMORY_ERROR,
	CE_CANT_OPEN_SOURCE_FILE,
	CE_ERROR_PARSING_SOURCE,
	CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED,
	CE_SOURCE_FILE_TGA_FORMAT_NOT_SUPPORTED,
	CE_SOURCE_FILE_BMP_FORMAT_NOT_SUPPORTED,
	CE_SOURCE_FILE_SIZE_NOT_SUPPORTED,
	CE_ERROR_WRITING_OUTPUT_FILE,
	CE_ERROR_LOADING_DLL
};

enum ImageFileFormat
{
	kImageFileFormat_PNG,
	kImageFileFormat_JPG,
};

struct TGAHeader {
	byte  identsize;          // size of ID field that follows 18 byte header (0 usually)
	byte  colourmaptype;      // type of colour map 0=none, 1=has palette
	byte  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	short colourmapstart;     // first colour map entry in palette
	short colourmaplength;    // number of colours in palette
	byte  colourmapbits;      // number of bits per palette entry 15,16,24,32

	short xstart;             // image x origin
	short ystart;             // image y origin
	short width;              // image width in pixels
	short height;             // image height in pixels
	byte  bits;               // image bits per pixel 8,16,24,32
	byte  descriptor;         // image descriptor bits (vh flip bits)
};

ConversionErrorType ImgUtl_ConvertJPEGToTGA( const char *jpgPath, const char *tgaPath, bool bRequirePowerOfTwoSize = true );
ConversionErrorType ImgUtl_ConvertBMPToTGA( const char *bmpPath, const char *tgaPath );
ConversionErrorType ImgUtl_ConvertTGA( const char *tgaPath, int nMaxWidth = -1, int nMaxHeight = -1 );
unsigned char		*ImgUtl_ReadVTFAsRGBA( const char *vtfPath, int &width, int &height, ConversionErrorType &errcode );
unsigned char		*ImgUtl_ReadTGAAsRGBA( const char *tgaPath, int &width, int &height, ConversionErrorType &errcode, TGAHeader &tgaHeader );
unsigned char		*ImgUtl_ReadJPEGAsRGBA( const char *jpegPath, int &width, int &height, ConversionErrorType &errcode );
unsigned char		*ImgUtl_ReadBMPAsRGBA( const char *bmpPath, int &width, int &height, ConversionErrorType &errcode );
unsigned char		*ImgUtl_ReadPNGAsRGBA( const char *bmpPath, int &width, int &height, ConversionErrorType &errcode );
unsigned char		*ImgUtl_ReadPNGAsRGBAFromBuffer( CUtlBuffer &buffer, int &width, int &height, ConversionErrorType &errcode );
unsigned char		*ImgUtl_ReadImageAsRGBA( const char *path, int &width, int &height, ConversionErrorType &errcode );
ConversionErrorType ImgUtl_StretchRGBAImage( const unsigned char *srcBuf, const int srcWidth, const int srcHeight, unsigned char *destBuf, const int destWidth, const int destHeight );
ConversionErrorType ImgUtl_PadRGBAImage( const unsigned char *srcBuf, const int srcWidth, const int srcHeight, unsigned char *destBuf, const int destWidth, const int destHeight );
ConversionErrorType ImgUtl_ConvertTGAToVTF( const char *tgaPath, int nMaxWidth = -1, int nMaxHeight = -1 );
ConversionErrorType ImgUtl_WriteGenericVMT( const char *vtfPath, const char *pMaterialsSubDir );
ConversionErrorType ImgUtl_WriteRGBAAsPNGToBuffer( const unsigned char *pRGBAData, int nWidth, int nHeight, CUtlBuffer &bufOutData, bool bIncludesAlpha = true );
ConversionErrorType ImgUtl_WriteRGBAAsJPEGToBuffer( const unsigned char *pRGBAData, int nWidth, int nHeight, CUtlBuffer &bufOutData, int nStride = 0 );

//
// Converts pInPath (which can be a TGA, BMP, or JPG file) to a VTF in pMaterialsSubDir, where
// pMaterialsSubDir is a directory located relative to the game/materials directory.  If the
// output directory doesn't exist, it will be created.  Dumps a generic VMT in pMaterialsSubDir
// as well.
//
ConversionErrorType	ImgUtl_ConvertToVTFAndDumpVMT( const char *pInPath, const char *pMaterialsSubDir,
												   int nMaxWidth = -1, int nMaxHeight = -1 );


/// Load from image file.  We use the file extension to
/// decide what file format to use
ConversionErrorType ImgUtl_LoadBitmap( const char *pszFilename, Bitmap_t &bitmap );

/// Load an image direct from memory buffer
ConversionErrorType ImgUtl_LoadBitmapFromBuffer( CUtlBuffer &fileData, Bitmap_t &bitmap, ImageFileFormat eImageFileFormat );

/// Save a bitmap to memory buffer
ConversionErrorType ImgUtl_SaveBitmapToBuffer( CUtlBuffer &fileData, const Bitmap_t &bitmap, ImageFileFormat eImageFileFormat );

/// Load a PNG direct from memory buffer
ConversionErrorType ImgUtl_LoadPNGBitmapFromBuffer( CUtlBuffer &fileData, Bitmap_t &bitmap );

/// Get the size of a PNG from a buffer
ConversionErrorType ImgUtl_GetPNGSize( CUtlBuffer &fileData, uint32_t &uWidth, uint32_t &uHeight );

/// Save a bitmap in PNG format to memory buffer
ConversionErrorType ImgUtl_SavePNGBitmapToBuffer( CUtlBuffer &fileData, const Bitmap_t &bitmap );

/// Resize a bitmap.  This currently only works for RGBA images!
ConversionErrorType ImgUtl_ResizeBitmap( Bitmap_t &destBitmap, int nWidth, int nHeight, const Bitmap_t *pImgSource = NULL );

#endif // IMAGECONVERSION_H
