//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "c_vguiscreen.h"
#include "vgui_controls/Label.h"
#include "vgui_bitmappanel.h"
#include <vgui/IVGui.h>
#include "c_slideshow_display.h"
#include "ienginevgui.h"
#include "fmtstr.h"
#include "vgui_controls/ImagePanel.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Control screen 
//-----------------------------------------------------------------------------
class CSlideshowDisplayScreen : public CVGuiScreenPanel
{
	DECLARE_CLASS( CSlideshowDisplayScreen, CVGuiScreenPanel );

public:
	CSlideshowDisplayScreen( vgui::Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings( IScheme *pScheme );

	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	virtual void OnTick();

private:
	void Update( C_SlideshowDisplay *pSlideshowDisplay );

private:
	vgui::Label *m_pDisplayTextLabel;
	CUtlVector<ImagePanel*> m_pSlideshowImages;

	int iLastSlideIndex;

	Color m_cDefault;
	Color m_cInvisible;

	bool bIsAlreadyVisible;
};


DECLARE_VGUI_SCREEN_FACTORY( CSlideshowDisplayScreen, "slideshow_display_screen" );

//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CSlideshowDisplayScreen::CSlideshowDisplayScreen( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, "CSlideshowDisplayScreen", vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/SlideshowDisplayScreen.res", "SlideshowDisplayScreen" ) ) 
{
	m_pDisplayTextLabel = new vgui::Label( this, "NumberDisplay", "x" );
	iLastSlideIndex = 0;
}

void CSlideshowDisplayScreen::ApplySchemeSettings( IScheme *pScheme )
{
	assert( pScheme );

	m_cDefault = pScheme->GetColor( "CSlideshowDisplayScreen_Default", GetFgColor() );
	m_cInvisible = Color( 0, 0, 0, 0 );	

	m_pDisplayTextLabel->SetFgColor( m_cDefault );
}

//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CSlideshowDisplayScreen::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	if (!BaseClass::Init(pKeyValues, pInitData))
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Update the display string
//-----------------------------------------------------------------------------
void CSlideshowDisplayScreen::OnTick()
{
	BaseClass::OnTick();

	if ( g_SlideshowDisplays.Count() <= 0 )
		return;

	C_SlideshowDisplay *pSlideshowDisplay = NULL;

	for ( int iDisplayScreens = 0; iDisplayScreens < g_SlideshowDisplays.Count(); ++iDisplayScreens  )
	{
		C_SlideshowDisplay *pSlideshowDisplayTemp = g_SlideshowDisplays[ iDisplayScreens ];
		if ( pSlideshowDisplayTemp && pSlideshowDisplayTemp->IsEnabled() )
		{
			pSlideshowDisplay = pSlideshowDisplayTemp;
			break;
		}
	}

	if( !pSlideshowDisplay )
	{
		// All display screens are disabled
		if ( bIsAlreadyVisible )
		{
			SetVisible( false );
			bIsAlreadyVisible = false;
		}
		return;
	}

	if ( !bIsAlreadyVisible )
	{
		SetVisible( true );
		bIsAlreadyVisible = true;
	}

	Update( pSlideshowDisplay );
}

void CSlideshowDisplayScreen::Update( C_SlideshowDisplay *pSlideshowDisplay )
{
	char szBuff[ 256 ];
	pSlideshowDisplay->GetDisplayText( szBuff );
	m_pDisplayTextLabel->SetText( szBuff );

	if ( m_pSlideshowImages.Count() == 0 )
	{
		// Build the list of image panels!
		for ( int iSlide = 0; iSlide < pSlideshowDisplay->NumMaterials(); ++iSlide )
		{
			m_pSlideshowImages.AddToTail( SETUP_PANEL( new ImagePanel( this, "SlideshowImage" ) ) );

			int iMatIndex = pSlideshowDisplay->GetMaterialIndex( iSlide );

			if ( iMatIndex > 0 )
			{
				const char *pMaterialName = GetMaterialNameFromIndex( iMatIndex );
				if ( pMaterialName )
				{
					pMaterialName = Q_strnchr( pMaterialName, '/', Q_strlen( pMaterialName ) );

					if ( pMaterialName )
					{
						pMaterialName = pMaterialName + 1;
						m_pSlideshowImages[ iSlide ]->SetImage( pMaterialName );
						m_pSlideshowImages[ iSlide ]->SetVisible( false );
						m_pSlideshowImages[ iSlide ]->SetZPos( -3 );
						m_pSlideshowImages[ iSlide ]->SetWide( GetWide() );
						m_pSlideshowImages[ iSlide ]->SetTall( GetTall() );
					}
				}
			}
		}
	}

	int iCurrentSlideIndex = pSlideshowDisplay->CurrentSlideIndex();

	if ( iCurrentSlideIndex != iLastSlideIndex )
	{
		m_pSlideshowImages[ iLastSlideIndex ]->SetVisible( false );
		m_pSlideshowImages[ iCurrentSlideIndex ]->SetVisible( true );
		iLastSlideIndex = iCurrentSlideIndex;
	}
}
