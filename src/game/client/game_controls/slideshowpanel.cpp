//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#include "cbase.h"
#include "game_controls/slideshowpanel.h"
#include "vgui/IVGui.h"
#include "filesystem.h"

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

DECLARE_BUILD_FACTORY( CCrossfadableImagePanel );
DECLARE_BUILD_FACTORY( CSlideshowPanel );

//-----------------------------------------------------------------------------

CCrossfadableImagePanel::CCrossfadableImagePanel( Panel* pParent, const char *pName )
:	EditablePanel( pParent, pName ),
	m_iSrcImg( 0 ),
	m_flBlend( 0.0f ),
	m_flBlendTime( 0.0f ),
	m_flStartBlendTime( 0.0f ),
	m_bBlending( false )
{
	m_pImages[ 0 ] = new ImagePanel( this, "Image0" );
	m_pImages[ 1 ] = new ImagePanel( this, "Image1" );

	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CCrossfadableImagePanel::~CCrossfadableImagePanel()
{
	ivgui()->RemoveTickSignal( GetVPanel() );
}


void CCrossfadableImagePanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );
}

void CCrossfadableImagePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SrcImg()->SetTileImage( false );
	DstImg()->SetTileImage( false );
}

void CCrossfadableImagePanel::PerformLayout()
{
	BaseClass::PerformLayout();

	for ( int i = 0; i < 2; ++i )
	{
		m_pImages[ i ]->SetBounds( 0, 0, GetWide(), GetTall() );
		m_pImages[ i ]->SetVisible( true );
	}
}

// Helper macro to perform a call on both images
#define CALL_FUNC_ON_BOTH_IMAGES( _call )	\
	AssertMsg( m_pImages[ 0 ], "m_pImages[ 0 ] is NULL!" ); \
	AssertMsg( m_pImages[ 1 ], "m_pImages[ 1 ] is NULL!" ); \
	m_pImages[0]->_call; \
	m_pImages[1]->_call;

void CCrossfadableImagePanel::SetShouldScaleImage( bool bState )
{
	CALL_FUNC_ON_BOTH_IMAGES( SetShouldScaleImage( bState ) );
}

void CCrossfadableImagePanel::SetScaleAmount( float flScale )
{
	CALL_FUNC_ON_BOTH_IMAGES( SetScaleAmount( flScale ) );
}

void CCrossfadableImagePanel::SetFillColor( Color c )
{
	CALL_FUNC_ON_BOTH_IMAGES( SetFillColor( c ) );
}

void CCrossfadableImagePanel::SetDrawColor( Color clrDrawColor )
{
	CALL_FUNC_ON_BOTH_IMAGES( SetDrawColor( clrDrawColor ) );
}

void CCrossfadableImagePanel::InstallMouseHandler( Panel *pHandler )
{
	CALL_FUNC_ON_BOTH_IMAGES( InstallMouseHandler( pHandler ) );
}

IImage *CCrossfadableImagePanel::GetImage()
{
	return SrcImg()->GetImage();
}

const char *CCrossfadableImagePanel::GetImageName()
{
	return SrcImg()->GetImageName();
}

float CCrossfadableImagePanel::GetScaleAmount()
{
	return SrcImg()->GetScaleAmount();
}

bool CCrossfadableImagePanel::GetShouldScaleImage()
{
	return SrcImg()->GetShouldScaleImage();
}

Color CCrossfadableImagePanel::GetFillColor()
{
	return SrcImg()->GetFillColor();
}

Color CCrossfadableImagePanel::GetDrawColor()
{
	return SrcImg()->GetDrawColor();
}

void CCrossfadableImagePanel::SetImage( IImage *pImage, float flBlendTime/*=0.0f*/ )
{
	DstImg()->SetImage( pImage );
	SetupImageBlend( flBlendTime );
}

void CCrossfadableImagePanel::SetImage( const char *pImageName, float flBlendTime/*=0.0f*/ )
{
	DstImg()->SetImage( pImageName );
	SetupImageBlend( flBlendTime );
}

void CCrossfadableImagePanel::SetupImageBlend( float flBlendTime )
{
	m_bBlending = true;
	m_flBlendTime = flBlendTime;
	m_flStartBlendTime = gpGlobals->realtime;
	m_flBlend = 0.0f;
}

void CCrossfadableImagePanel::OnSizeChanged( int nWide, int nTall )
{
	m_pImages[ 0 ]->SetSize( nWide, nTall );
	m_pImages[ 1 ]->SetSize( nWide, nTall );
}

void CCrossfadableImagePanel::OnTick()
{
	if ( m_bBlending )
	{
		// Compute current blend value
		if ( m_flBlendTime == 0.0f )
		{
			m_flBlend = 1.0f;
		}
		else
		{
			float t = clamp( ( gpGlobals->realtime - m_flStartBlendTime ) / m_flBlendTime, 0.0f, 1.0f );
			m_flBlend = clamp( t * t * (3 - 2*t), 0.0f, 1.0f );	// S-curve
		}

		// Set alpha channel on source image
		Color clrDraw;
		clrDraw = SrcImg()->GetDrawColor();
		clrDraw[ 3 ] = ( int )( 255 * ( 1 - m_flBlend ) );
		SrcImg()->SetDrawColor( clrDraw );

		// Set alpha channel on destination image
		clrDraw = DstImg()->GetDrawColor();
		clrDraw[ 3 ] = ( int )( 255 * m_flBlend );
		DstImg()->SetDrawColor( clrDraw );

		// If we're done, don't blend next think
		if ( m_flBlend >= 1.0f )
		{
			m_bBlending = false;
			m_iSrcImg = !m_iSrcImg;
			m_flBlend = 0.0f;
		}
	}
}

//-----------------------------------------------------------------------------

CSlideshowPanel::CSlideshowPanel( Panel *pParent, const char *pName )
:	EditablePanel( pParent, pName ),
	m_iCurImg( 0 ),
	m_flInterval( 3.0f ),
	m_flTransitionLength( 0.5f )
{
	m_pImagePanel = new CCrossfadableImagePanel( this, "CrossfadableImage" );

	// Setup the first transition time
	UpdateNextTransitionTime();

	// Add tick signal
	ivgui()->AddTickSignal( GetVPanel(), 10 );
}

CSlideshowPanel::~CSlideshowPanel()
{
	// Remove tick signal
	ivgui()->RemoveTickSignal( GetVPanel() );
}

void CSlideshowPanel::SetInterval( float flInterval )
{
	m_flInterval = flInterval;
	UpdateNextTransitionTime();
}

void CSlideshowPanel::SetTransitionTime( float flTransitionLength )
{
	m_flTransitionLength = flTransitionLength;
	UpdateNextTransitionTime();
}

void CSlideshowPanel::UpdateNextTransitionTime()
{
	m_flNextTransitionTime = gpGlobals->realtime + m_flInterval;
}

void CSlideshowPanel::AddImage( const char *pImageName )
{
	if ( pImageName && strlen( pImageName ) > 0 )
	{
		AddImage( scheme()->GetImage( pImageName, m_pImagePanel->GetShouldScaleImage() ) );
	}
}

void CSlideshowPanel::AddImage( IImage *pImage )
{
	// Cache a pointer to the image
	m_vecImages.AddToTail( pImage );

	if ( m_vecImages.Count() == 1 )
	{
		GetImagePanel()->SetImage( pImage );
	}
}

void CSlideshowPanel::FillWithImages( const char *pBasePath )
{
	int i = 0;
	while ( 1 )
	{
		CFmtStr fmtImagePath( "materials/vgui/%s%i.vmt", pBasePath, i );
		V_FixDoubleSlashes( fmtImagePath.Access() );
		if ( !g_pFullFileSystem->FileExists( fmtImagePath.Access() ) )
			break;
		
		fmtImagePath.sprintf( "%s%i.vmt", pBasePath, i );
		AddImage( fmtImagePath.Access() );

		++i;
	}
}

void CSlideshowPanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	int iDefaultImage = pInResourceData->GetInt( "default_index", 0 ); 

	int i = 0;
	while ( 1 )
	{
		CFmtStr fmtImageKeyName( "image_%i", i );
		const char *pImagePath = pInResourceData->GetString( fmtImageKeyName.Access(), NULL );
		if ( !pImagePath )
			break;

		AddImage( pImagePath );

		if ( iDefaultImage == i )
		{
			GetImagePanel()->SetImage( pImagePath );
		}

		++i;
	}

	GetImagePanel()->SetSize(
		XRES( pInResourceData->GetInt( "wide" ) ),
		YRES( pInResourceData->GetInt( "tall" ) )
	);

	GetImagePanel()->SetShouldScaleImage( pInResourceData->GetBool( "scaleImage" ) );
	GetImagePanel()->SetScaleAmount( pInResourceData->GetFloat( "scaleAmount" ) );
}

void CSlideshowPanel::OnSizeChanged( int nWide, int nTall )
{
	GetImagePanel()->SetSize( nWide, nTall );
}

void CSlideshowPanel::OnTick()
{
	if ( GetImageCount() > 1 && gpGlobals->realtime >= m_flNextTransitionTime )
	{
		// Iterate to next image
		m_iCurImg = ( m_iCurImg + 1 ) % GetImageCount();

		// Setup new image
		GetImagePanel()->SetImage( m_vecImages[ m_iCurImg ], m_flTransitionLength );

		// Set transition time to be the end of the blend
		m_flNextTransitionTime = gpGlobals->realtime + m_flInterval;
	}
}

