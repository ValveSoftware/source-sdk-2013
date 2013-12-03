//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <vgui/VGUI.h>
#include <Color.h>

#include <vgui_controls/ImageList.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: blank image, intentially draws nothing
//-----------------------------------------------------------------------------
class BlankImage : public IImage
{
public:
	virtual void Paint() {}
	virtual void SetPos(int x, int y) {}
	virtual void GetContentSize(int &wide, int &tall) { wide = 0; tall = 0; }
	virtual void GetSize(int &wide, int &tall) { wide = 0; tall = 0; }
	virtual void SetSize(int wide, int tall) {}
	virtual void SetColor(Color col) {}
	virtual bool Evict() { return false; }
	virtual int GetNumFrames() { return 0; }
	virtual void SetFrame( int nFrame ) {}
	virtual HTexture GetID() { return 0; }
	virtual void SetRotation( int iRotation ) { return; };
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ImageList::ImageList(bool deleteImagesWhenDone)
{
	m_bDeleteImagesWhenDone = deleteImagesWhenDone;
	AddImage(new BlankImage());
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ImageList::~ImageList()
{
	if (m_bDeleteImagesWhenDone)
	{
		// delete all the images, except for the first image (which is always the blank image)
		for (int i = 1; i < m_Images.Count(); i++)
		{
			delete m_Images[i];
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: adds a new image to the list, returning the index it was placed at
//-----------------------------------------------------------------------------
int ImageList::AddImage(vgui::IImage *image)
{
	return m_Images.AddToTail(image);
}

//-----------------------------------------------------------------------------
// Purpose: sets an image at a specified index, growing and adding NULL images if necessary
//-----------------------------------------------------------------------------
void ImageList::SetImageAtIndex(int index, vgui::IImage *image)
{
	// allocate more images if necessary
	while (m_Images.Count() <= index)
	{
		m_Images.AddToTail(NULL);
	}

	m_Images[index] = image;
}

//-----------------------------------------------------------------------------
// Purpose: returns the number of images
//-----------------------------------------------------------------------------
int ImageList::GetImageCount()
{
	return m_Images.Count();
}

//-----------------------------------------------------------------------------
// Purpose: gets an image, imageIndex is of range [0, GetImageCount)
//-----------------------------------------------------------------------------
vgui::IImage *ImageList::GetImage(int imageIndex)
{
	return m_Images[imageIndex];
}

//-----------------------------------------------------------------------------
// Purpose: returns true if an index is valid
//-----------------------------------------------------------------------------
bool ImageList::IsValidIndex(int imageIndex)
{
	return m_Images.IsValidIndex(imageIndex);
}

