//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMAGELIST_H
#define IMAGELIST_H

#ifdef _WIN32
#pragma once
#endif

#include <utlvector.h>
#include <vgui/VGUI.h>
#include <vgui/IImage.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: holds a collection of images
//			used by controls so that images can be refered to by indices
//-----------------------------------------------------------------------------
class ImageList
{
public:
	ImageList(bool deleteImagesWhenDone);
	~ImageList();

	// adds a new image to the list, returning the index it was placed at
	int AddImage(vgui::IImage *image);

	// returns the number of images
	int GetImageCount();

	// returns true if an index is valid
	bool IsValidIndex(int imageIndex);

	// sets an image at a specified index, growing and adding NULL images if necessary
	void SetImageAtIndex(int index, vgui::IImage *image);

	// gets an image, imageIndex is of range [0, GetImageCount)
	// image index 0 is always the blank image
	vgui::IImage *GetImage(int imageIndex);

private:
	CUtlVector<vgui::IImage *> m_Images;
	bool m_bDeleteImagesWhenDone;
};


} // namespace vgui

#endif // IMAGELIST_H
