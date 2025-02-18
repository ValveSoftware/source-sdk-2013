//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which draws a viewcone
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef VIEWCONEIMAGE_H
#define VIEWCONEIMAGE_H

#include "shareddefs.h"
#include "vgui_bitmapimage.h"

namespace vgui
{
	class Panel;
}

class C_BaseEntity;
class KeyValues;

//-----------------------------------------------------------------------------
// A bitmap that renders a view cone based on angles
//-----------------------------------------------------------------------------
class CViewConeImage
{
public:
	// initialization
	bool Init( vgui::Panel *pParent, KeyValues* pInitData );

	// Paint the sucka
	void Paint( float yaw );

	void SetColor( int r, int g, int b );

private:
	BitmapImage m_Image;
};


//-----------------------------------------------------------------------------
// Helper method to initialize a view cone image from KeyValues data..
// KeyValues contains the bitmap data, pSectionName, if it exists,
// indicates which subsection of pInitData should be looked at to get at the
// image data. The final argument is the bitmap image to initialize.
// The function returns true if it succeeded.
//
// NOTE: This function looks for the key values 'material' and 'color'
// and uses them to set up the material + modulation color of the image
//-----------------------------------------------------------------------------
bool InitializeViewConeImage( KeyValues *pInitData, const char* pSectionName, 
	vgui::Panel *pParent, CViewConeImage* pViewConeImage );


#endif //  VIEWCONEIMAGE_H
