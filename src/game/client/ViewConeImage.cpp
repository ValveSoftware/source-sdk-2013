//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: This is a panel which is rendered image on top of an entity
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "ViewConeImage.h"
#include <KeyValues.h>
#include <vgui_controls/Panel.h>
#include "VGuiMatSurface/IMatSystemSurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// initialization
//-----------------------------------------------------------------------------
bool CViewConeImage::Init( vgui::Panel *pParent, KeyValues* pInitData )
{
	Assert( pParent );

	// Load viewcone material
	if (!m_Image.Init( pParent->GetVPanel(), pInitData ))
		return false;

	// Position the view cone...
	int viewconesize = pInitData->GetInt( "size", 32 );
	m_Image.SetRenderSize( viewconesize, viewconesize );

	int cx, cy;
	pParent->GetSize( cx, cy );
	m_Image.SetPos( (cx - viewconesize) / 2, (cy - viewconesize) / 2 );

	return true;
}

//-----------------------------------------------------------------------------
// Paint the sucka
//-----------------------------------------------------------------------------
void CViewConeImage::Paint( float yaw )
{
	g_pMatSystemSurface->DisableClipping( true );

	m_Image.DoPaint( NULL, yaw );

	g_pMatSystemSurface->DisableClipping( false );
}

void CViewConeImage::SetColor( int r, int g, int b )
{
	m_Image.SetColor( Color( r, g, b, 255 ) );
}

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
	vgui::Panel *pParent, CViewConeImage* pViewConeImage )
{
	KeyValues *pViewConeImageSection;
	if (pSectionName)
	{
		pViewConeImageSection = pInitData->FindKey( pSectionName );
		if ( !pViewConeImageSection )
			return false;
	}
	else
	{
		pViewConeImageSection = pInitData;
	}

	return pViewConeImage->Init( pParent, pViewConeImageSection );
}

