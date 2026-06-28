//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "matsys_controls/colorpickerpanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "matsys_controls/proceduraltexturepanel.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/itexture.h"
#include "pixelwriter.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/RadioButton.h"
#include "vgui/IInput.h"
#include "tier1/KeyValues.h"
#include "bitmap/imageformat.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Color picker
//-----------------------------------------------------------------------------
enum ColorType_t
{
	COLOR_TYPE_RGB = 0,
	COLOR_TYPE_HSV,
};

enum ColorChannel_t
{
	CHANNEL_RED = 0,
	CHANNEL_GREEN,
	CHANNEL_BLUE,

	CHANNEL_HUE = 0,
	CHANNEL_SATURATION,
	CHANNEL_VALUE,
};


//-----------------------------------------------------------------------------
// Converts RGB to normalized
//-----------------------------------------------------------------------------
static void RGB888ToVector( RGB888_t inColor, Vector *pOutVector )
{
	pOutVector->Init( inColor.r / 255.0f, inColor.g / 255.0f, inColor.b / 255.0f ); 
}

static void VectorToRGB888( const Vector &inVector, RGB888_t &outColor )
{
	int r = (int)((inVector.x * 255.0f) + 0.5f);
	int g = (int)((inVector.y * 255.0f) + 0.5f);
	int b = (int)((inVector.z * 255.0f) + 0.5f);
	outColor.r = clamp( r, 0, 255 );
	outColor.g = clamp( g, 0, 255 );
	outColor.b = clamp( b, 0, 255 );
}


//-----------------------------------------------------------------------------
// Convert RGB to HSV
//-----------------------------------------------------------------------------
static inline void RGBtoHSV( const RGB888_t &rgb, Vector &hsv )
{
	Vector vecRGB;
	RGB888ToVector( rgb, &vecRGB );
	RGBtoHSV( vecRGB, hsv );
}


//-----------------------------------------------------------------------------
// Convert HSV to RGB
//-----------------------------------------------------------------------------
static inline void HSVtoRGB( const Vector &hsv, RGB888_t &rgb )
{
	Vector vecRGB;
	HSVtoRGB( hsv, vecRGB );
	VectorToRGB888( vecRGB, rgb );
}


//-----------------------------------------------------------------------------
// This previews the 'xy' color
//-----------------------------------------------------------------------------
class CColorXYPreview : public CProceduralTexturePanel
{
	DECLARE_CLASS_SIMPLE( CColorXYPreview, CProceduralTexturePanel );

public:
	// constructor
	CColorXYPreview( vgui::Panel *pParent, const char *pName );

	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect );
 	virtual void Paint( void );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );
	virtual void OnCursorMoved( int x, int y );

	void SetMode( ColorType_t type, ColorChannel_t channel );
	void SetColor( const RGB888_t &color, const Vector &hsvColor );

private:
	// Computes a color given a particular x,y value 
	void ComputeColorForPoint( int x, int y, RGB888_t &color );
	void ComputeHSVColorForPoint( int x, int y, Vector &vscHSV );

	// Updates the color based on the mouse position
	void UpdateColorFromMouse( int x, int y );

	static ColorChannel_t s_pHSVRemapX[3];
	static ColorChannel_t s_pHSVRemapY[3];
	static ColorChannel_t s_pRGBRemapX[3];
	static ColorChannel_t s_pRGBRemapY[3];

	ColorType_t m_Type;
	ColorChannel_t m_Channel;
	RGB888_t m_CurrentColor;
	Vector m_CurrentHSVColor;
	vgui::HCursor m_hPickerCursor;
	bool m_bDraggingMouse;
};


ColorChannel_t CColorXYPreview::s_pHSVRemapX[3] =
{
	CHANNEL_SATURATION, CHANNEL_HUE, CHANNEL_HUE
};

ColorChannel_t CColorXYPreview::s_pHSVRemapY[3] = 
{
	CHANNEL_VALUE, CHANNEL_VALUE, CHANNEL_SATURATION
};

ColorChannel_t CColorXYPreview::s_pRGBRemapX[3] =
{
	CHANNEL_BLUE, CHANNEL_BLUE, CHANNEL_RED
};

ColorChannel_t CColorXYPreview::s_pRGBRemapY[3] = 
{
	CHANNEL_GREEN, CHANNEL_RED, CHANNEL_GREEN
};

	
//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CColorXYPreview::CColorXYPreview( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	Init( 256, 256, false );
	m_CurrentColor.r = m_CurrentColor.g = m_CurrentColor.b = 255;
	SetMode( COLOR_TYPE_HSV, CHANNEL_HUE );
	SetMouseInputEnabled( true );
	m_hPickerCursor = surface()->CreateCursorFromFile( "resource/colorpicker.cur" );
	SetCursor( m_hPickerCursor );
	m_bDraggingMouse = false;
}

	
//-----------------------------------------------------------------------------
// Sets the mode for the preview
//-----------------------------------------------------------------------------
void CColorXYPreview::SetMode( ColorType_t type, ColorChannel_t channel )
{
	if ( m_Type != type || m_Channel != channel )
	{
		m_Type = type;
		m_Channel = channel;
		DownloadTexture();
	}
}

void CColorXYPreview::SetColor( const RGB888_t &color, const Vector &hsvColor )
{
	if ( color != m_CurrentColor || m_CurrentHSVColor != hsvColor )
	{
		m_CurrentColor = color;
		m_CurrentHSVColor = hsvColor;
		DownloadTexture();
	}
}

	
//-----------------------------------------------------------------------------
// Computes a color given a particular x,y value 
//-----------------------------------------------------------------------------
void CColorXYPreview::ComputeColorForPoint( int x, int y, RGB888_t &color )
{
	color = m_CurrentColor;
	((unsigned char*)&color)[ s_pRGBRemapX[m_Channel] ] = x;
	((unsigned char*)&color)[ s_pRGBRemapY[m_Channel] ] = GetImageHeight() - y - 1;
}

void CColorXYPreview::ComputeHSVColorForPoint( int x, int y, Vector &vscHSV )
{
	vscHSV = m_CurrentHSVColor;
	vscHSV[ s_pHSVRemapX[m_Channel] ] = (float)x / 255.0f;
	vscHSV[ s_pHSVRemapY[m_Channel] ] = (float)(GetImageHeight() - y - 1) / 255.0f;
	if ( vscHSV.y == 0.0f )
	{
		vscHSV.x = -1.0f;
	}

	if ( m_Channel != CHANNEL_HUE )
	{
		if ( vscHSV.x != -1.0f )
		{
			vscHSV.x *= 360.0f;
		}
	}
}


//-----------------------------------------------------------------------------
// Fills the texture w/ the image buffer 
//-----------------------------------------------------------------------------
void CColorXYPreview::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
{
	Assert( pVTFTexture->FrameCount() == 1 );
	Assert( pVTFTexture->FaceCount() == 1 );
	Assert( !pTexture->IsMipmapped() );

	int nWidth, nHeight, nDepth;
	pVTFTexture->ComputeMipLevelDimensions( 0, &nWidth, &nHeight, &nDepth );
	Assert( nDepth == 1 );
	Assert( nWidth == m_nWidth && nHeight == m_nHeight );

	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
		pVTFTexture->ImageData( 0, 0, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );

	for ( int y = 0; y < nHeight; ++y )
	{
		pixelWriter.Seek( 0, y );

		for ( int x = 0; x < nWidth; ++x )
		{
			RGB888_t color;
			if ( m_Type != COLOR_TYPE_RGB )
			{
				Vector vecHSV;
				ComputeHSVColorForPoint( x, y, vecHSV );
				HSVtoRGB( vecHSV, color );
			}
			else
			{
				ComputeColorForPoint( x, y, color );
			}

			pixelWriter.WritePixel( color.r, color.g, color.b, 255 );
		}
	}
}


//-----------------------------------------------------------------------------
// Paints a circle over the currently selected color
//-----------------------------------------------------------------------------
void CColorXYPreview::Paint( void )
{
	BaseClass::Paint();

	int x, y;
	if ( m_Type != COLOR_TYPE_RGB )
	{
		Vector vecHSVNormalized = m_CurrentHSVColor;
		if ( vecHSVNormalized.x != -1.0f )
		{
			vecHSVNormalized.x *= 255.0f / 360.0f;
		}
		vecHSVNormalized.y *= 255.0f;
		vecHSVNormalized.z *= 255.0f;

		x = (int)( vecHSVNormalized[ s_pHSVRemapX[m_Channel] ] + 0.5f);
		y = GetImageHeight() - 1 - (int)( vecHSVNormalized[ s_pHSVRemapY[m_Channel] ] + 0.5f );
	}
	else
	{
		x = ((unsigned char*)&m_CurrentColor)[ s_pRGBRemapX[m_Channel] ];
		y = GetImageHeight() - 1 - ((unsigned char*)&m_CurrentColor)[ s_pRGBRemapY[m_Channel] ];
	}
			   
	// Renormalize x, y to actual size
	int w, h;
	GetSize( w, h );
	x = (int)( w * (float)x / 255.0f + 0.5f );
	y = (int)( h * (float)y / 255.0f + 0.5f );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawOutlinedCircle( x, y, 5, 8 );
	vgui::surface()->DrawSetColor( 0, 0, 0, 255 );
	vgui::surface()->DrawOutlinedCircle( x, y, 6, 8 );
}


//-----------------------------------------------------------------------------
// Updates the color based on the mouse position
//-----------------------------------------------------------------------------
void CColorXYPreview::UpdateColorFromMouse( int x, int y )
{
	int w, h;
	GetSize( w, h );

	float flNormalizedX = (float)x / (w-1);
	float flNormalizedY = (float)y / (h-1);
	flNormalizedX = clamp( flNormalizedX, 0.0f, 1.0f );
	flNormalizedY = clamp( flNormalizedY, 0.0f, 1.0f );

	int tx = (int)( (GetImageWidth()-1) * flNormalizedX + 0.5f );
	int ty = (int)( (GetImageHeight()-1) * flNormalizedY + 0.5f );
	if ( m_Type != COLOR_TYPE_RGB )
	{
		Vector vecHSV;
		ComputeHSVColorForPoint( tx, ty, vecHSV );

		KeyValues *pKeyValues = new KeyValues( "HSVSelected" );
		pKeyValues->SetFloat( "hue", vecHSV.x );
		pKeyValues->SetFloat( "saturation", vecHSV.y );
		pKeyValues->SetFloat( "value", vecHSV.z );
		PostActionSignal( pKeyValues );
		
		// This prevents a 1-frame lag in the current color position
		RGB888_t color;
		HSVtoRGB( vecHSV, color );
		SetColor( color, vecHSV );
	}
	else
	{
		RGB888_t color;
		ComputeColorForPoint( tx, ty, color );

		Color c( color.r, color.g, color.b, 255 );
		KeyValues *pKeyValues = new KeyValues( "ColorSelected" );
		pKeyValues->SetColor( "color", c );
		PostActionSignal( pKeyValues );

		// This prevents a 1-frame lag in the current color position
		Vector vecHSV;
		RGBtoHSV( color, vecHSV );
		SetColor( color, vecHSV );
	}
}


//-----------------------------------------------------------------------------
// Handle input
//-----------------------------------------------------------------------------
void CColorXYPreview::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if ( code == MOUSE_LEFT )
	{
		if ( !m_bDraggingMouse )
		{
			m_bDraggingMouse = true;
			input()->SetMouseCapture(GetVPanel());

			int x, y;
			input()->GetCursorPos( x, y );
			ScreenToLocal( x, y );

			UpdateColorFromMouse( x, y );
		}
	}
}

void CColorXYPreview::OnMouseReleased( vgui::MouseCode code )
{
	BaseClass::OnMouseReleased( code );

	if ( code == MOUSE_LEFT )
	{
		if ( m_bDraggingMouse )
		{
			m_bDraggingMouse = false;
			input()->SetMouseCapture( (VPANEL)0 );
		}
	}
}

void CColorXYPreview::OnCursorMoved( int x, int y )
{
	BaseClass::OnCursorMoved( x, y );

	if ( m_bDraggingMouse )
	{
		UpdateColorFromMouse( x, y );
	}
}


//-----------------------------------------------------------------------------
// This previews the 'z' color
//-----------------------------------------------------------------------------
class CColorZPreview : public CProceduralTexturePanel
{
	DECLARE_CLASS_SIMPLE( CColorZPreview, CProceduralTexturePanel );

public:
	// constructor
	CColorZPreview( vgui::Panel *pParent, const char *pName );

	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect );
 	virtual void PerformLayout();
	virtual void Paint( void );
	virtual void OnCursorMoved( int x,int y );
	virtual void OnMousePressed( vgui::MouseCode code );
	virtual void OnMouseReleased( vgui::MouseCode code );

	void SetMode( ColorType_t type, ColorChannel_t channel );
	void SetColor( const RGB888_t &color, const Vector &hsvColor );

	// Computes a color given a particular x,y value 
	void ComputeColorForPoint( int y, RGB888_t &color );
	void ComputeHSVColorForPoint( int y, bool bProceduralTexture, Vector &vecHSV );

private:
	// Updates the color based on the mouse position
	void UpdateColorFromMouse( int x, int y );

	ColorType_t m_Type;
	ColorChannel_t m_Channel;
	RGB888_t m_CurrentColor;
	Vector m_CurrentHSVColor;
	bool m_bDraggingMouse;
};

#define MARKER_WIDTH 6


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CColorZPreview::CColorZPreview( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	Init( 8, 256, false );
	m_CurrentColor.r = m_CurrentColor.g = m_CurrentColor.b = 255;

	Vector vecRGB;
	RGB888ToVector( m_CurrentColor, &vecRGB );
	RGBtoHSV( vecRGB, m_CurrentHSVColor );
    m_bDraggingMouse = false;

	SetMouseInputEnabled( true );
	SetMode( COLOR_TYPE_HSV, CHANNEL_HUE );
}

	
//-----------------------------------------------------------------------------
// Sets the mode for the preview
//-----------------------------------------------------------------------------
void CColorZPreview::SetMode( ColorType_t type, ColorChannel_t channel )
{
	if ( m_Type != type || m_Channel != channel )
	{
		m_Type = type;
		m_Channel = channel;
		DownloadTexture();
	}
}

void CColorZPreview::SetColor( const RGB888_t &color, const Vector &hsvColor )
{
	if ( color != m_CurrentColor || m_CurrentHSVColor != hsvColor )
	{
		m_CurrentColor = color;
		m_CurrentHSVColor = hsvColor;
		DownloadTexture();
	}
}

	
//-----------------------------------------------------------------------------
// Lays out the panel
//-----------------------------------------------------------------------------
void CColorZPreview::PerformLayout()
{
	BaseClass::PerformLayout();

	int w, h;
	GetSize( w, h );
	Rect_t r;
	r.x = MARKER_WIDTH;
	r.y = MARKER_WIDTH;
	r.width = w - (MARKER_WIDTH*2);
	r.height = h - (MARKER_WIDTH*2);

	SetPaintRect( &r );
}

	
//-----------------------------------------------------------------------------
// Computes a color given a particular x,y value 
//-----------------------------------------------------------------------------
void CColorZPreview::ComputeColorForPoint( int y, RGB888_t &color )
{
	color = m_CurrentColor;
	((unsigned char*)&color)[ m_Channel ] = GetImageHeight() - y - 1;
}

void CColorZPreview::ComputeHSVColorForPoint( int y, bool bProceduralTexture, Vector &vecHSV )
{
	vecHSV = m_CurrentHSVColor;
	vecHSV[ m_Channel ] = (float)(GetImageHeight() - y - 1) / 255.0f;
	if ( m_Channel == CHANNEL_HUE )
	{
		if ( vecHSV.x != -1.0f )
		{
			vecHSV.x *= 360.0f;
		}

		if ( bProceduralTexture )
		{
			vecHSV.y = 1.0f;
			vecHSV.z = 1.0f;
		}
	}
}


//-----------------------------------------------------------------------------
// Fills the texture w/ the image buffer 
//-----------------------------------------------------------------------------
void CColorZPreview::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
{
	Assert( pVTFTexture->FrameCount() == 1 );
	Assert( pVTFTexture->FaceCount() == 1 );
	Assert( !pTexture->IsMipmapped() );

	int nWidth, nHeight, nDepth;
	pVTFTexture->ComputeMipLevelDimensions( 0, &nWidth, &nHeight, &nDepth );
	Assert( nDepth == 1 );
	Assert( nWidth == m_nWidth && nHeight == m_nHeight );

	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
		pVTFTexture->ImageData( 0, 0, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );

	for ( int y = 0; y < nHeight; ++y )
	{
		pixelWriter.Seek( 0, y );

		RGB888_t color;
		if ( m_Type != COLOR_TYPE_RGB )
		{
			Vector vecHSV;
			ComputeHSVColorForPoint( y, true, vecHSV );
			HSVtoRGB( vecHSV, color );
		}
		else
		{
			ComputeColorForPoint( y, color );
		}

		for ( int x = 0; x < nWidth; ++x )
		{
			pixelWriter.WritePixel( color.r, color.g, color.b, 255 );
		}
	}
}


//-----------------------------------------------------------------------------
// Updates the color based on the mouse position
//-----------------------------------------------------------------------------
void CColorZPreview::UpdateColorFromMouse( int x, int y )
{
	int w, h;
	GetSize( w, h );
	h -= 2 * MARKER_WIDTH;

	float flNormalizedY = (float)( y - MARKER_WIDTH ) / (h-1);
	flNormalizedY = clamp( flNormalizedY, 0.0f, 1.0f );

	int ty = (int)( (GetImageHeight() - 1) * flNormalizedY + 0.5f );
	if ( m_Type != COLOR_TYPE_RGB )
	{
		Vector vecHSV;
		ComputeHSVColorForPoint( ty, false, vecHSV );

		KeyValues *pKeyValues = new KeyValues( "HSVSelected" );
		pKeyValues->SetFloat( "hue", vecHSV.x );
		pKeyValues->SetFloat( "saturation", vecHSV.y );
		pKeyValues->SetFloat( "value", vecHSV.z );
		PostActionSignal( pKeyValues );
		
		// This prevents a 1-frame lag in the current color position
		RGB888_t color;
		HSVtoRGB( vecHSV, color );
		SetColor( color, vecHSV );
	}
	else
	{
		RGB888_t color;
		ComputeColorForPoint( ty, color );

		Color c( color.r, color.g, color.b, 255 );
		KeyValues *pKeyValues = new KeyValues( "ColorSelected" );
		pKeyValues->SetColor( "color", c );
		PostActionSignal( pKeyValues );

		// This prevents a 1-frame lag in the current color position
		Vector vecHSV;
		RGBtoHSV( color, vecHSV );
		SetColor( color, vecHSV );
	}
}


//-----------------------------------------------------------------------------
// Handle input
//-----------------------------------------------------------------------------
void CColorZPreview::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if ( code == MOUSE_LEFT )
	{
		if ( !m_bDraggingMouse )
		{
			m_bDraggingMouse = true;
			input()->SetMouseCapture(GetVPanel());

			int x, y;
			input()->GetCursorPos( x, y );
			ScreenToLocal( x, y );

			UpdateColorFromMouse( x, y );
		}
	}
}

void CColorZPreview::OnMouseReleased( vgui::MouseCode code )
{
	BaseClass::OnMouseReleased( code );

	if ( code == MOUSE_LEFT )
	{
		if ( m_bDraggingMouse )
		{
			m_bDraggingMouse = false;
			input()->SetMouseCapture( (VPANEL)0 );
		}
	}
}

void CColorZPreview::OnCursorMoved( int x, int y )
{
	BaseClass::OnCursorMoved( x, y );

	if ( m_bDraggingMouse )
	{
		UpdateColorFromMouse( x, y );
	}
}


//-----------------------------------------------------------------------------
// Paints the panel (the two arrows, specifically)
//-----------------------------------------------------------------------------
void CColorZPreview::Paint( void )
{
	BaseClass::Paint();

	int y;
	if ( m_Type != COLOR_TYPE_RGB )
	{
		Vector vecHSVNormalized = m_CurrentHSVColor;
		if ( vecHSVNormalized.x != -1.0f )
		{
			vecHSVNormalized.x *= 255.0f / 360.0f;
		}
		vecHSVNormalized.y *= 255.0f;
		vecHSVNormalized.z *= 255.0f;

		y = GetImageHeight() - 1 - (int)( vecHSVNormalized[ m_Channel ] + 0.5f );
	}
	else
	{
		y = GetImageHeight() - 1 - ((unsigned char*)&m_CurrentColor)[ m_Channel ];
	}

	// Renormalize y to actual size
	int w, h;
	GetSize( w, h );
	h -= 2 * MARKER_WIDTH;
	y = (int)( h * (float)y / 255.0f + 0.5f );

	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	int px[3] = { 0, 0, MARKER_WIDTH };
	int py[3] = { MARKER_WIDTH + y - MARKER_WIDTH, MARKER_WIDTH + y + MARKER_WIDTH, MARKER_WIDTH + y };
	vgui::surface()->DrawPolyLine( px, py, 3 );

	px[0] = px[1] = w-1;
	px[2] = w - 1 - MARKER_WIDTH;
	vgui::surface()->DrawPolyLine( px, py, 3 );
}


//-----------------------------------------------------------------------------
//
// Color picker panel
//
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
CColorPickerPanel::CColorPickerPanel( vgui::Panel *pParent, const char *pName ) :
	BaseClass( pParent, pName )
{
	m_pColorXYPreview = new CColorXYPreview( this, "ColorXYPreview" );
	m_pColorZPreview = new CColorZPreview( this, "ColorZPreview" );
	m_pColorXYPreview->AddActionSignalTarget( this );

	m_pHueRadio = new RadioButton( this, "HueRadio", "H" );
	m_pSaturationRadio = new RadioButton( this, "SaturationRadio", "S" );
	m_pValueRadio = new RadioButton( this, "ValueRadio", "V" );
	m_pRedRadio = new RadioButton( this, "RedRadio", "R" );
	m_pGreenRadio = new RadioButton( this, "GreenRadio", "G" );
	m_pBlueRadio = new RadioButton( this, "BlueRadio", "B" );

	m_pHueText = new TextEntry( this, "HueText" );
	m_pSaturationText = new TextEntry( this, "SaturationText");
	m_pValueText = new TextEntry( this, "ValueText" );
	m_pRedText = new TextEntry( this, "RedText" );
	m_pGreenText = new TextEntry( this, "GreenText" );
	m_pBlueText = new TextEntry( this, "BlueText" );
	m_pAlphaText= new TextEntry( this, "AlphaText" );

	m_pInitialColor = new Panel( this, "InitialColor" );
	m_pCurrentColor = new Panel( this, "CurrentColor" );
	m_pInitialColor->SetVisible( true );
	m_pCurrentColor->SetVisible( true );
	m_pInitialColor->SetPaintBackgroundEnabled( true );
	m_pCurrentColor->SetPaintBackgroundEnabled( true );

	m_pInitialColor->SetMouseInputEnabled( false );
	SetMouseInputEnabled( true );

	Color c( 255, 255, 255, 255 );
	SetInitialColor( c );

	LoadControlSettings( "resource/colorpicker.res" );
}


//-----------------------------------------------------------------------------
// Sets the initial color
//-----------------------------------------------------------------------------
void CColorPickerPanel::SetInitialColor( Color initialColor )
{
	m_InitialColor.r = initialColor.r();
	m_InitialColor.g = initialColor.g();
	m_InitialColor.b = initialColor.b();

	m_CurrentAlpha = initialColor.a();
	m_InitialAlpha = m_CurrentAlpha;
	m_CurrentColor = m_InitialColor;

	RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
	if ( m_CurrentHSVColor.x == -1 )
	{
		m_CurrentHSVColor.x = 0;
	}
	OnColorChanged();
}


//-----------------------------------------------------------------------------
// Handle input
//-----------------------------------------------------------------------------
void CColorPickerPanel::OnMousePressed( vgui::MouseCode code )
{
	BaseClass::OnMousePressed( code );

	if ( code == MOUSE_LEFT )
	{
		// Clicking inside the initial color window
		// resets the current color to the initial color
		int x, y;
		input()->GetCursorPos( x, y );
		ScreenToLocal( x, y );

		int cx, cy, cw, ch;
		m_pInitialColor->GetBounds( cx, cy, cw, ch );
		if ( ( cx <= x ) && ( cx+cw > x ) && ( cy <= y ) && ( cy+ch > y ) )
		{
			m_CurrentColor = m_InitialColor;
			m_CurrentAlpha = m_InitialAlpha;
			RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
			if ( m_CurrentHSVColor.x == -1 )
			{
				m_CurrentHSVColor.x = 0;
			}
			OnColorChanged();
		}
	}
}


//-----------------------------------------------------------------------------
// Gets the current/initial color
//-----------------------------------------------------------------------------
void CColorPickerPanel::GetCurrentColor( Color *pColor )
{
	pColor->SetColor( m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, m_CurrentAlpha );
}

void CColorPickerPanel::GetInitialColor( Color *pColor )
{
	pColor->SetColor( m_InitialColor.r, m_InitialColor.g, m_InitialColor.b, m_InitialAlpha );
}


//-----------------------------------------------------------------------------
// Updates the preview colors
//-----------------------------------------------------------------------------
void CColorPickerPanel::UpdatePreviewColors()
{
	Color c;
	c.SetColor( m_InitialColor.r, m_InitialColor.g, m_InitialColor.b, 255 );
	m_pInitialColor->SetBgColor( c );
	c.SetColor( m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b, 255 );
	m_pCurrentColor->SetBgColor( c );
}


//-----------------------------------------------------------------------------
// Used to make sure we win over the scheme settings 
//-----------------------------------------------------------------------------
void CColorPickerPanel::ApplySchemeSettings(IScheme *pScheme)
{
	// Need to override the scheme settings for this button
	BaseClass::ApplySchemeSettings( pScheme );
	UpdatePreviewColors();
}


//-----------------------------------------------------------------------------
// Callbacks from the color preview dialogs
//-----------------------------------------------------------------------------
void CColorPickerPanel::OnHSVSelected( KeyValues *data )
{
	m_CurrentHSVColor.x = data->GetFloat( "hue" );
	m_CurrentHSVColor.y = data->GetFloat( "saturation" );
	m_CurrentHSVColor.z = data->GetFloat( "value" );
	HSVtoRGB( m_CurrentHSVColor, m_CurrentColor );
	OnColorChanged();
}

void CColorPickerPanel::OnColorSelected( KeyValues *data )
{
	Color c = data->GetColor( "color" );
	m_CurrentColor.r = c.r();
	m_CurrentColor.g = c.g();
	m_CurrentColor.b = c.b();
	RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
	OnColorChanged();
}


//-----------------------------------------------------------------------------
// Radio buttons
//-----------------------------------------------------------------------------
void CColorPickerPanel::OnRadioButtonChecked( KeyValues *pKeyValues )
{
	// NOTE: The radio button command strings are defined in the colorpicker.res file
	// in game/platform/resource.
	vgui::Panel *pPanel = (vgui::Panel *)pKeyValues->GetPtr( "panel" );
	if ( pPanel == m_pRedRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_RED );
		m_pColorZPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_RED );
	}
	else if ( pPanel == m_pGreenRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_GREEN );
		m_pColorZPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_GREEN );
	}
	else if ( pPanel == m_pBlueRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_BLUE );
		m_pColorZPreview->SetMode( COLOR_TYPE_RGB, CHANNEL_BLUE );
	}
	else if ( pPanel == m_pHueRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_HUE );
		m_pColorZPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_HUE );
	}
	else if ( pPanel == m_pSaturationRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_SATURATION );
		m_pColorZPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_SATURATION );
	}
	else if ( pPanel == m_pValueRadio )
	{
		m_pColorXYPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_VALUE );
		m_pColorZPreview->SetMode( COLOR_TYPE_HSV, CHANNEL_VALUE );
	}
}


//-----------------------------------------------------------------------------
// Called when the color changes
//-----------------------------------------------------------------------------
void CColorPickerPanel::OnColorChanged( vgui::TextEntry *pChanged )
{
	char temp[256];

	if ( pChanged != m_pRedText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", m_CurrentColor.r );
		m_pRedText->SetText( temp );
	}
	if ( pChanged != m_pGreenText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", m_CurrentColor.g );
		m_pGreenText->SetText( temp );
	}
	if ( pChanged != m_pBlueText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", m_CurrentColor.b );
		m_pBlueText->SetText( temp );
	}
	if ( pChanged != m_pAlphaText )
	{
		Q_snprintf( temp, sizeof( temp ), "%d", m_CurrentAlpha );
		m_pAlphaText->SetText( temp );
	}

	if ( pChanged != m_pHueText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", (int)(m_CurrentHSVColor.x + 0.5f) );
		m_pHueText->SetText( temp );
	}
	if ( pChanged != m_pSaturationText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", (int)(m_CurrentHSVColor.y * 100 + 0.5f) );
		m_pSaturationText->SetText( temp );
	}
	if ( pChanged != m_pValueText )
	{
		Q_snprintf( temp, sizeof(temp), "%d", (int)(m_CurrentHSVColor.z * 100 + 0.5f) );
		m_pValueText->SetText( temp );
	}

	m_pColorXYPreview->SetColor( m_CurrentColor, m_CurrentHSVColor );
	m_pColorZPreview->SetColor( m_CurrentColor, m_CurrentHSVColor );
	UpdatePreviewColors();
	PostActionSignal( new KeyValues( "command", "command", "preview" ) );
}

	
//-----------------------------------------------------------------------------
// Called when the color text entry panels change
//-----------------------------------------------------------------------------
void CColorPickerPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = (Panel *)data->GetPtr( "panel", NULL );

	float flHue = m_CurrentHSVColor.x;

	char buf[256];
	if ( pPanel == m_pRedText )
	{
		m_pRedText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		m_CurrentColor.r = clamp( val, 0, 255 );
		RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
	}
	else if ( pPanel == m_pGreenText )
	{
		m_pGreenText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		m_CurrentColor.g = clamp( val, 0, 255 );
		RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
	}
	else if ( pPanel == m_pBlueText )
	{
		m_pBlueText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		m_CurrentColor.b = clamp( val, 0, 255 );
		RGBtoHSV( m_CurrentColor, m_CurrentHSVColor );
	}
	else if ( pPanel == m_pAlphaText )
	{
		m_pAlphaText->GetText( buf, sizeof( buf ) );
		int val = atoi( buf );
		m_CurrentAlpha = clamp( val, 0, 255 );
	}
	else if ( pPanel == m_pHueText )
	{
		m_pHueText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		m_CurrentHSVColor.x = clamp( val, 0, 360 );
		HSVtoRGB( m_CurrentHSVColor, m_CurrentColor );
	}
	else if ( pPanel == m_pSaturationText )
	{
		m_pSaturationText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		val = clamp( val, 0, 100 );
		m_CurrentHSVColor.y = (float)val / 100.0f;
		HSVtoRGB( m_CurrentHSVColor, m_CurrentColor );
	}
	else if ( pPanel == m_pValueText )
	{
		m_pValueText->GetText( buf, sizeof(buf) );
		int val = atoi( buf );
		val = clamp( val, 0, 100 );
		m_CurrentHSVColor.z = (float)val / 100.0f;
		HSVtoRGB( m_CurrentHSVColor, m_CurrentColor );
	}

	// Preserve hue
	if ( m_CurrentHSVColor.x == -1 )
	{
		m_CurrentHSVColor.x = flHue;
	}
	OnColorChanged( static_cast<vgui::TextEntry*>(pPanel) );
}


//-----------------------------------------------------------------------------
//
// Purpose: Modal picker frame
//
//-----------------------------------------------------------------------------
CColorPickerFrame::CColorPickerFrame( vgui::Panel *pParent, const char *pTitle ) : 
	BaseClass( pParent, "ColorPickerFrame" )
{
	m_pContextKeys = NULL;
	SetDeleteSelfOnClose( true );
	m_pPicker = new CColorPickerPanel( this, "ColorPicker" );
	m_pPicker->AddActionSignalTarget( this );
	m_pOpenButton = new Button( this, "OkButton", "Ok", this, "Ok" );
	m_pCancelButton = new Button( this, "CancelButton", "#FileOpenDialog_Cancel", this, "Cancel" );
	SetBlockDragChaining( true );

	LoadControlSettings( "resource/colorpickerframe.res" );

	int w, h;
	GetSize( w, h );
	SetMinimumSize( w, h );

	SetTitle( pTitle, false );
}

CColorPickerFrame::~CColorPickerFrame()
{
	CleanUpMessage();
}


//-----------------------------------------------------------------------------
// Deletes the message
//-----------------------------------------------------------------------------
void CColorPickerFrame::CleanUpMessage()
{
	if ( m_pContextKeys )
	{
		m_pContextKeys->deleteThis();
		m_pContextKeys = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Activate the dialog
//-----------------------------------------------------------------------------
void CColorPickerFrame::DoModal( Color initialColor, KeyValues *pContextKeys )
{
	CleanUpMessage();
	m_pPicker->SetInitialColor( initialColor );
	m_pContextKeys = pContextKeys;

	BaseClass::DoModal();
}


//-----------------------------------------------------------------------------
// Gets the initial color
//-----------------------------------------------------------------------------
void CColorPickerFrame::GetInitialColor( Color *pColor )
{
	m_pPicker->GetInitialColor( pColor );
}


//-----------------------------------------------------------------------------
// On command
//-----------------------------------------------------------------------------
void CColorPickerFrame::OnCommand( const char *pCommand )
{
	if ( !Q_stricmp( pCommand, "Ok" ) )
	{
		Color c;
		m_pPicker->GetCurrentColor( &c );

		KeyValues *pActionKeys = new KeyValues( "ColorPickerPicked" );
		pActionKeys->SetColor( "color", c );
		if ( m_pContextKeys )
		{
			pActionKeys->AddSubKey( m_pContextKeys );
			m_pContextKeys = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	if ( !Q_stricmp( pCommand, "Cancel" ) )
	{
		vgui::input()->ReleaseAppModalSurface();
		KeyValues *pActionKeys = new KeyValues( "ColorPickerCancel" );
		if ( m_pContextKeys )
		{
			pActionKeys->AddSubKey( m_pContextKeys );
			m_pContextKeys = NULL;
		}
		CloseModal();
		PostActionSignal( pActionKeys );
		return;
	}

	if ( !Q_stricmp( pCommand, "Preview" ) )
	{
		Color c;
		m_pPicker->GetCurrentColor( &c );

		KeyValues *pActionKeys = new KeyValues( "ColorPickerPreview" );
		pActionKeys->SetColor( "color", c );
		if ( m_pContextKeys )
		{
			pActionKeys->AddSubKey( m_pContextKeys->MakeCopy() );
		}
		PostActionSignal( pActionKeys );
		return;
	}

	BaseClass::OnCommand( pCommand );
}


//-----------------------------------------------------------------------------
//
// Purpose: A button which brings up the color picker
//
//-----------------------------------------------------------------------------
CColorPickerButton::CColorPickerButton( vgui::Panel *pParent, const char *pName, vgui::Panel *pActionSignalTarget ) :
	BaseClass( pParent, pName, "" )
{
	m_CurrentColor.SetColor( 255, 255, 255, 255 );
	if ( pActionSignalTarget )
	{
		AddActionSignalTarget( pActionSignalTarget );
	}
}

CColorPickerButton::~CColorPickerButton()
{
}

void CColorPickerButton::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	UpdateButtonColor();
}


//-----------------------------------------------------------------------------
// Called when the picker gets a new color
//-----------------------------------------------------------------------------
void CColorPickerButton::OnPicked( KeyValues *data )
{
	SetColor( data->GetColor( "color" ) );

	// Fire normal action signal messages
	KeyValues *pMessage = new KeyValues( "ColorPickerPicked" );
	pMessage->SetColor( "color", m_CurrentColor );
	PostActionSignal( pMessage );
	PlayButtonReleasedSound();
	SetSelected( false );
}


//-----------------------------------------------------------------------------
// Called when a color is previewed
//-----------------------------------------------------------------------------
void CColorPickerButton::OnPreview( KeyValues *data )
{
	KeyValues *pMessage = new KeyValues( "ColorPickerPreview" );
	pMessage->SetColor( "color", data->GetColor( "color" ) );
	PostActionSignal( pMessage );
}


//-----------------------------------------------------------------------------
// Called when cancel is hit in the picker
//-----------------------------------------------------------------------------
void CColorPickerButton::OnCancelled( )
{
	SetSelected( false );

	KeyValues *pMessage = new KeyValues( "ColorPickerCancel" );
	pMessage->SetColor( "startingColor", m_CurrentColor );
	PostActionSignal( pMessage );
}


//-----------------------------------------------------------------------------
// Perform the click
//-----------------------------------------------------------------------------
void CColorPickerButton::DoClick()
{
	SetSelected( true );

	CColorPickerFrame *pColorPickerDialog = new CColorPickerFrame( this, "Select Color" );
	pColorPickerDialog->AddActionSignalTarget( this );
	pColorPickerDialog->DoModal( m_CurrentColor );
}


//-----------------------------------------------------------------------------
// Set current color
//-----------------------------------------------------------------------------
void CColorPickerButton::SetColor( const Color& clr )
{
	m_CurrentColor = clr;
	UpdateButtonColor();
}

void CColorPickerButton::SetColor( int r, int g, int b, int a )
{
	m_CurrentColor.SetColor( r, g, b, a );
	UpdateButtonColor();
}


//-----------------------------------------------------------------------------
// Update button color
//-----------------------------------------------------------------------------
void CColorPickerButton::UpdateButtonColor()
{
	SetDefaultColor( m_CurrentColor, m_CurrentColor );
	SetArmedColor( m_CurrentColor, m_CurrentColor );
	SetDepressedColor( m_CurrentColor, m_CurrentColor );
}
