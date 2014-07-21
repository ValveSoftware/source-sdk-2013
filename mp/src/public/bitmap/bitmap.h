//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef BITMAP_H
#define BITMAP_H

#ifdef _WIN32
#pragma once
#endif


#include "bitmap/imageformat.h"
#include "Color.h"
#include "dbg.h"

class CUtlBuffer;

//-----------------------------------------------------------------------------
// A Bitmap
//-----------------------------------------------------------------------------
struct Bitmap_t
{
	Bitmap_t() { Reset(); }
	~Bitmap_t() { Clear(); }

	//
	// Accessors
	//
	inline int Width() const { return m_nWidth; }
	inline int Height() const { return m_nHeight; }
	inline ImageFormat Format() const { return m_ImageFormat; }
	inline unsigned char *GetBits() const { return m_pBits; }
	inline int Stride() const { return m_nStride; }
	inline bool GetOwnsBuffer() const { return m_bOwnsBuffer; }

	/// Allocate the buffer.  Discards existing data, freeing it if we own it
	void Init( int nWidth, int nHeight, ImageFormat imageFormat, int nStride = 0 );

	/// Set the bitmap to the specified buffer.  Any existing data is discarded/freed
	/// as appropriate.
	void SetBuffer( int nWidth, int nHeight, ImageFormat imageFormat, unsigned char *pBits, bool bAssumeOwnership, int nStride = 0 );

	/// Sets / releases ownershp of the buffer.  This does not otherwise alter the
	/// state of the bitmap.
	void SetOwnsBuffer( bool bOwnsBuffer )
	{
		Assert( m_pBits );
		m_bOwnsBuffer = bOwnsBuffer;
	}

	/// Free up all memory and reset to default state
	void Clear();

	/// Return true if we have a valid size and buffer
	bool IsValid() const;

	/// Get pointer to raw pixel data.
	unsigned char *GetPixel( int x, int y );
	const unsigned char *GetPixel( int x, int y ) const;

	/// Get pixel value at specified coordinates
	Color GetColor( int x, int y ) const;

	/// Set pixel value at specified coordinates
	void SetColor( int x, int y, Color c );

	/// Set this bitmap to be a logical copy of the specified
	/// bitmap.  No memory is allocated or copied, just copying
	/// some pointers.  We can also optionally transfer ownership
	/// of the buffer.
	void MakeLogicalCopyOf( Bitmap_t &src, bool bTransferBufferOwnership = false );

	/// Set this bitmap to be a cropped rectangle from the given bitmap.
	/// The source pointer can be NULL or point to this, which means to do
	/// the crop in place.
	void Crop( int x0, int y0, int nWidth, int nHeight, const Bitmap_t *pImgSource = NULL );

	/// Blit a rectangle of pixel data into this image.
	void SetPixelData( const Bitmap_t &src, int nSrcX1, int nSrcY1, int nCopySizeX, int nCopySizeY, int nDestX1, int nDestY1 );

	/// Blit the entire source image into this image, at the specified offset.
	/// the rectangle is clipped if necessary
	void SetPixelData( const Bitmap_t &src, int nDestX1 = 0, int nDestY1 = 0 );

private:
	void Reset();

	/// Dimensions
	int m_nWidth;
	int m_nHeight;

	/// Size, in bytes, of one pixel
	int m_nPixelSize;

	/// Image row stride, in bytes
	int m_nStride;

	// Do we own this buffer?
	bool m_bOwnsBuffer;

	/// Pixel format
	ImageFormat m_ImageFormat;

	/// Bitmap data.  Must be allocated with malloc/free.  Don't use
	/// new/delete
	unsigned char *m_pBits;
};

inline void Bitmap_t::Reset()
{
	m_nWidth = 0;
	m_nHeight = 0;
	m_ImageFormat = IMAGE_FORMAT_UNKNOWN;
	m_pBits = NULL;
	m_nPixelSize = 0;
	m_bOwnsBuffer = false;
	m_nStride = 0;
}

inline unsigned char *Bitmap_t::GetPixel( int x, int y )
{
	if ( !m_pBits )
		return NULL;

	return m_pBits + (y*m_nStride) + x* m_nPixelSize;
}

inline const unsigned char *Bitmap_t::GetPixel( int x, int y ) const
{
	if ( !m_pBits )
		return NULL;

	return m_pBits + (y*m_nStride) + x* m_nPixelSize;
}


#endif // BITMAP_H
