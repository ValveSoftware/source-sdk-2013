//========= Copyright Valve Corporation, All rights reserved. ============//
//
// A class representing an image
//
//=============================================================================

#ifndef DMEIMAGE_H
#define DMEIMAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "datamodel/dmelement.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
enum ImageFormat;


//-----------------------------------------------------------------------------
// A class representing an image (2d or 3d bitmap)
//-----------------------------------------------------------------------------
class CDmeImage : public CDmElement
{
	DEFINE_ELEMENT( CDmeImage, CDmElement );

public:
	// Methods related to image format
	ImageFormat Format() const;
	const char *FormatName() const;

	// returns a pointer to the image bits buffer
	const void *ImageBits() const;

public:
	CDmAttributeVar<int> m_Width;
	CDmAttributeVar<int> m_Height;
	CDmAttributeVar<int> m_Depth;

private:
	CDmAttributeVar<int> m_Format;
	CDmAttributeVarBinaryBlock m_Bits;
};


//-----------------------------------------------------------------------------
// returns a pointer to the image bits buffer
//-----------------------------------------------------------------------------
inline const void *CDmeImage::ImageBits() const
{
	return m_Bits.Get();
}


#endif // DMEIMAGE_H
