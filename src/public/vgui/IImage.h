//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IIMAGE_H
#define IIMAGE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

class Color;

namespace vgui
{

typedef uint32 HTexture;

enum iimage_rotation_t
{
	ROTATED_UNROTATED = 0,
	ROTATED_CLOCKWISE_90,
	ROTATED_ANTICLOCKWISE_90,
	ROTATED_FLIPPED,
};

//-----------------------------------------------------------------------------
// Purpose: Interface to drawing an image
//-----------------------------------------------------------------------------
class IImage
{
public:
	// Call to Paint the image
	// Image will draw within the current panel context at the specified position
	virtual void Paint() = 0;

	// Set the position of the image
	virtual void SetPos(int x, int y) = 0;

	// Gets the size of the content
	virtual void GetContentSize(int &wide, int &tall) = 0;

	// Get the size the image will actually draw in (usually defaults to the content size)
	virtual void GetSize(int &wide, int &tall) = 0;

	// Sets the size of the image
	virtual void SetSize(int wide, int tall) = 0;

	// Set the draw color 
	virtual void SetColor(Color col) = 0;

	// virtual destructor
	virtual ~IImage() {}

	// not for general purpose use
	// evicts the underlying image from memory if refcounts permit, otherwise ignored
	// returns true if eviction occurred, otherwise false
	virtual bool Evict() = 0;

	virtual int GetNumFrames() = 0;
	virtual void SetFrame( int nFrame ) = 0;
	virtual HTexture GetID() = 0;

	virtual void SetRotation( int iRotation ) = 0;
};

} // namespace vgui


#endif // IIMAGE_H
