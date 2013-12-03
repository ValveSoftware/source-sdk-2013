//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef TEAMBITMAPIMAGE_H
#define TEAMBITMAPIMAGE_H

#ifdef _WIN32
#pragma once
#endif

//#include "tf_shareddefs.h"

#include <vgui/VGUI.h>

namespace vgui
{
	class Panel;
}

class BitmapImage;
class C_BaseEntity;
class KeyValues;

//-----------------------------------------------------------------------------
// A multiplexer bitmap that chooses a bitmap based on team
//-----------------------------------------------------------------------------
class CTeamBitmapImage
{
public:
	// construction, destruction
	CTeamBitmapImage();
	~CTeamBitmapImage();

	// initialization
	bool Init( vgui::Panel *pParent, KeyValues* pInitData, C_BaseEntity* pEntity );

	// Alpha override...
	void SetAlpha( float alpha );

	// Paint the sucka. Paint it the size of the parent panel
	void Paint( float yaw = 0.0f );

protected:
	// Wrapper so we can implement this with EHANDLES some day
	C_BaseEntity *GetEntity() { return m_pEntity; }

private:
	enum
	{
		// NOTE: Was MAX_TF_TEAMS not 4, but I don't like the dependency here.
		BITMAP_COUNT = 4 + 1
	};

	BitmapImage *m_ppImage[ BITMAP_COUNT ];
	C_BaseEntity *m_pEntity;
	float m_Alpha;
	bool m_bRelativeTeams;
};


//-----------------------------------------------------------------------------
// Helper method to initialize a team image from KeyValues data..
// KeyValues contains the bitmap data. pSectionName, if it exists,
// indicates which subsection of pInitData should be looked at to get at the
// image data. The final argument is the bitmap image to initialize.
// The function returns true if it succeeded.
//
// NOTE: This function looks for the key values 'material' and 'color'
// and uses them to set up the material + modulation color of the image
//-----------------------------------------------------------------------------
bool InitializeTeamImage( KeyValues *pInitData, const char* pSectionName, 
	vgui::Panel *pParent, C_BaseEntity *pEntity, CTeamBitmapImage* pBitmapImage );


#endif //  TEAMBITMAPIMAGE_H