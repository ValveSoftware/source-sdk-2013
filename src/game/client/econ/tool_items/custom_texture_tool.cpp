//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

// for the tool
#include "econ_gcmessages.h"
#include "econ_item_system.h"
#include "econ_item_constants.h"
#include "tool_items.h"
#include "imageutils.h"
#include "econ_ui.h"
#include "econ_item_inventory.h"
#include "econ_item_tools.h"
#include "checksum_md5.h"
#include "gc_clientsystem.h"
#include "materialsystem/itexture.h"
#include "pixelwriter.h"

#include "filesystem.h"

// for UI
#include "confirm_dialog.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Slider.h"
#include "vgui/Cursor.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui/IImage.h"
#include "vgui/IBorder.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "bitmap/tgawriter.h"
#include "bitmap/bitmap.h"
#include "vgui_bitmappanel.h"
#include "tool_items/custom_texture_cache.h"
#include "util_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace CustomTextureSystem;


// Turn this on to run the filters on a bunch of test images when the dialog is opened
//#define TEST_FILTERS

#define DEFINE_BLEND(code) \
	for (int y = 0 ; y < imgSource.Height() ; ++y ) \
	{ \
		for (int x = 0 ; x < imgSource.Width() ; ++x ) \
		{ \
			Color sc = imgSource.GetColor( x,y ); \
			Color dc = imgDest.GetColor( x,y ); \
			float sr = (float)sc.r()/255.0f, sg = (float)sc.g()/255.0f, sb = (float)sc.b()/255.0f, sa = (float)sc.a()/255.0f; \
			float dr = (float)dc.r()/255.0f, dg = (float)dc.g()/255.0f, db = (float)dc.b()/255.0f, da = (float)dc.a()/255.0f; \
			float blendPct = sa * flOpacity; \
			code \
			imgDest.SetColor( x,y, FloatRGBAToColor( dr*255.0f, dg*255.0f, db*255.0f, da*255.0f ) ); \
		} \
	}

static void DoNormalBlend( const Bitmap_t &imgSource, Bitmap_t &imgDest, float flOpacity )
{
	DEFINE_BLEND(
		dr += (sr - dr) * blendPct;
		dg += (sg - dg) * blendPct;
		db += (sb - db) * blendPct;
	)
}

static void DoMultiplyBlend( const Bitmap_t &imgSource, Bitmap_t &imgDest, float flOpacity )
{
	DEFINE_BLEND(
		dr += (dr*sr - dr) * blendPct;
		dg += (dg*sg - dg) * blendPct;
		db += (db*sb - db) * blendPct;
	)
}

static inline float screen( float a, float b )
{
	return 1.0f - (1.0f-a)*(1.0f-b);
}

static void DoScreenBlend( const Bitmap_t &imgSource, Bitmap_t &imgDest, float flOpacity )
{
	DEFINE_BLEND(
		dr += (screen(dr,sr) - dr) * blendPct;
		dg += (screen(dg,sg) - dg) * blendPct;
		db += (screen(db,sb) - db) * blendPct;
	)
}

static inline float overlay( float a, float b )
{
	if ( a < .5f )
	{
		return a * b * 2.0f;
	}
	float t = a * 2.0f - 1.0f;
	return screen( t, b );
}

static void DoOverlayBlend( const Bitmap_t &imgSource, Bitmap_t &imgDest, float flOpacity )
{
	DEFINE_BLEND(
		dr += (overlay(dr,sr) - dr) * blendPct;
		dg += (overlay(dg,sg) - dg) * blendPct;
		db += (overlay(db,sb) - db) * blendPct;
	)
}

static void DoReplaceAlphaBlend( const Bitmap_t &imgSource, Bitmap_t &imgDest, float flOpacity )
{
	DEFINE_BLEND(
		float k = (sr + sb + sg) / 3.0f;
		da += (k - da) * blendPct;
	)
}

// Custom compositing blend operations.  Mostly these are direct translations of standard Photoshop operations.
// For most operations, the source alpha used as per-pixel blend factor (multiplied by the layer opacity), and
// the dest alpha is just copied
enum ELayerBlendOp
{
	eLayerBlendOp_Invalid, // Debugging placeholder value
	eLayerBlendOp_Normal, // Regular blend (lerp)
	eLayerBlendOp_Multiply, // Multiply color channels
	eLayerBlendOp_Screen, // 1 - (1-A) * (1-B)
	eLayerBlendOp_Overlay, // Multiply or screen, depending on source
	eLayerBlendOp_ReplaceAlpha, // Blend the source alpha channel with the greyscale value from the layer.  Color channel is not modified
};

/// A custom compositing step
struct SDecalBlendLayer
{

	/// Which operation to perform?
	ELayerBlendOp eLayerOp;

	/// The image data
	Bitmap_t m_image;

	/// Opacity multiplier.  The full blend color is calculated by performing the blend
	/// operation ignoring opacity.  Then this result is lerped with the dest fragment by
	/// the effective blend factor. The effective per-pixel blend factor is taken as the
	/// source alpha times this value.  
	float m_fLayerOpacity;

	/// Parse from keyvalues.
	bool FromKV( KeyValues *pkvLayerBlock, CUtlString &errMsg )
	{
		const char *op = pkvLayerBlock->GetString( "op", "(none)" );
		if      ( !Q_stricmp( op, "normal" ) ) eLayerOp = eLayerBlendOp_Normal;
		else if ( !Q_stricmp( op, "multiply" ) ) eLayerOp = eLayerBlendOp_Multiply;
		else if ( !Q_stricmp( op, "screen" ) ) eLayerOp = eLayerBlendOp_Screen;
		else if ( !Q_stricmp( op, "overlay" ) ) eLayerOp = eLayerBlendOp_Overlay;
		else if ( !Q_stricmp( op, "ReplaceAlpha" ) ) eLayerOp = eLayerBlendOp_ReplaceAlpha;
		else
		{
			errMsg.Format( "Invalid blend operation '%s'", op );
			return false;
		}

		const char *pszImageFilename = pkvLayerBlock->GetString( "image", NULL );
		if ( pszImageFilename == NULL )
		{
			errMsg = "Must specify 'image'";
			return false;
		}
		if ( ImgUtl_LoadBitmap( pszImageFilename, m_image ) != CE_SUCCESS )
		{
			errMsg.Format( "Can't load image '%s'", pszImageFilename );
			return false;
		}

		m_fLayerOpacity = pkvLayerBlock->GetFloat( "opacity", 1.0f );

		return true;
	}

	/// Apply the operation
	void Apply( Bitmap_t &imgDest ) const
	{
		if ( !m_image.IsValid() || !imgDest.IsValid() || imgDest.Width() != m_image.Width() || imgDest.Height() != m_image.Height() )
		{
			Assert( m_image.IsValid() );
			Assert( imgDest.IsValid() );
			Assert( imgDest.Width() == m_image.Width() );
			Assert( imgDest.Height() == m_image.Height() );
			return;
		}

		switch ( eLayerOp )
		{
			default:
			case eLayerBlendOp_Invalid:
				Assert( !"Bogus blend op!" );
			case eLayerBlendOp_Normal:
				DoNormalBlend( m_image, imgDest, m_fLayerOpacity );
				break;
			case eLayerBlendOp_Multiply:
				DoMultiplyBlend( m_image, imgDest, m_fLayerOpacity );
				break;
			case eLayerBlendOp_Screen:
				DoScreenBlend( m_image, imgDest, m_fLayerOpacity );
				break;
			case eLayerBlendOp_Overlay:
				DoOverlayBlend( m_image, imgDest, m_fLayerOpacity );
				break;
			case eLayerBlendOp_ReplaceAlpha:
				DoReplaceAlphaBlend( m_image, imgDest, m_fLayerOpacity );
				break;
		}
	}
};

// Note: uses a non-linear non-perceptual color space.  But it will be good enough,
// probably
inline int ApproxColorDistSq( const Color &a, const Color &b )
{
	int dr = (int)a.r() - (int)b.r();
	int dg = (int)a.g() - (int)b.g();
	int db = (int)a.b() - (int)b.b();
	return dr*dr + dg*dg + db*db;
}

// Return cheesy color distance calculation, approximately normalized from 0...1
inline float ApproxColorDist( const Color &a, const Color &b )
{
	return sqrt( (float)ApproxColorDistSq( a, b ) ) * ( 1.0f / 441.67f );
}

// Convert linear RGB -> XYZ color space.
Vector LinearRGBToXYZ( const Vector &rgb )
{

	// http://en.wikipedia.org/wiki/SRGB
	Vector xyz;
	xyz.x = rgb.x * 0.4124 + rgb.y*0.3576 + rgb.z*0.1805;
	xyz.y = rgb.x * 0.2126 + rgb.y*0.7152 + rgb.z*0.0722;
	xyz.z = rgb.x * 0.0193 + rgb.y*0.1192 + rgb.z*0.9505;
	return xyz;
}

inline float lab_f( float t )
{
	if ( t > (6.0/29.0)*(6.0/29.0)*(6.0/29.0) )
	{
		return pow( t, .333333f );
	}
	return ( (1.0f/3.0f) * (29.0f/6.0f) * (29.0f/6.0f) ) * t + (4.0f/29.0f);
}

// Convert CIE XYZ -> L*a*b*
Vector XYZToLab( const Vector &xyz )
{

	// http://en.wikipedia.org/wiki/Lab_color_space
	const float X_n = 0.9505;
	const float Y_n = 1.0000;
	const float Z_n = 1.0890;

	float f_X = lab_f( xyz.x / X_n );
	float f_Y = lab_f( xyz.y / Y_n );
	float f_Z = lab_f( xyz.z / Z_n );

	Vector lab;
	lab.x = 116.0f*f_Y - 16.0f; // L*
	lab.y = 500.0f * ( f_X - f_Y ); // a*
	lab.z = 200.0f * ( f_Y - f_Z ); // b*
	return lab;
}

// Convert texture-space RGB values to linear RGB space
Vector TextureToLinearRGB( Color c )
{
	Vector rgb;
	rgb.x = SrgbGammaToLinear( (float)c.r() / 255.0f );
	rgb.y = SrgbGammaToLinear( (float)c.g() / 255.0f );
	rgb.z = SrgbGammaToLinear( (float)c.b() / 255.0f );
	return rgb;
}

// Convert texture-space RGB values to perceptually linear L*a*b* space
Vector TextureToLab( Color c )
{
	Vector linearRGB = TextureToLinearRGB( c );
	Vector xyz = LinearRGBToXYZ( linearRGB );
	return XYZToLab( xyz );
}

static void SymmetricNearestNeighborFilter( const Bitmap_t &imgSrc, Bitmap_t &imgDest, int radius, float amount = 1.0f )
{

	// Make sure image is allocated properly
	int nWidth = imgSrc.Width();
	int nHeight = imgSrc.Height();
	imgDest.Init( nWidth, nHeight, IMAGE_FORMAT_RGBA8888 );

	float flWeightBias = (2 + radius + radius );
	int filteredBlendWeight = int(amount * 256.0f);
	int originalBlendWeight = 256 - filteredBlendWeight;

	// For each dest pixel
	for ( int y = 0 ; y < nHeight ; ++y )
	{
		for ( int x = 0 ; x < nWidth ; ++x )
		{
			Color c = imgSrc.GetColor( x, y );

			// Iterate over half of the kernel.  (Doesn't matter which half.)
			// Kernel pixels are examined in opposing pairs
			Vector4D sum(0,0,0,0);
			float flTotalWeight = 0.0f;
			for (int ry = 0 ; ry <= radius ; ++ry )
			{
				int sy1 = clamp(y + ry, 0, nHeight-1);
				int sy2 = clamp(y - ry, 0, nHeight-1);
				for (int rx = (ry == 0) ? 0 : -radius ; rx <= radius ; ++rx )
				{
					int sx1 = clamp(x + rx, 0, nWidth-1);
					int sx2 = clamp(x - rx, 0, nWidth-1);

					Color s1 = imgSrc.GetColor( sx1, sy1 );
					Color s2 = imgSrc.GetColor( sx2, sy2 );

					// Calculate difference.  Here, maybe we should be using
					// a perceptual difference in linear color space.  Who cares.
					int d1 = ApproxColorDistSq( c, s1 );
					int d2 = ApproxColorDistSq( c, s2 );

					float weight = flWeightBias - fabs((float)ry) - fabs((float)rx);
					if ( d1 < d2 )
					{
						sum.x += (float)s1.r() * weight;
						sum.y += (float)s1.g() * weight;
						sum.z += (float)s1.b() * weight;
						sum.w += (float)s1.a() * weight;
					}
					else
					{
						sum.x += (float)s2.r() * weight;
						sum.y += (float)s2.g() * weight;
						sum.z += (float)s2.b() * weight;
						sum.w += (float)s2.a() * weight;
					}
					flTotalWeight += weight;
				}
			}

			sum /= flTotalWeight;
			int filterR = (int)clamp(sum.x, 0.0f, 255.0f);
			int filterG = (int)clamp(sum.y, 0.0f, 255.0f);
			int filterB = (int)clamp(sum.z, 0.0f, 255.0f);
			int filterA = (int)clamp(sum.w, 0.0f, 255.0f);
			Color result(
				(filterR*filteredBlendWeight + c.r()*originalBlendWeight) >> 8,
				(filterG*filteredBlendWeight + c.g()*originalBlendWeight) >> 8,
				(filterB*filteredBlendWeight + c.b()*originalBlendWeight) >> 8,
				(filterA*filteredBlendWeight + c.a()*originalBlendWeight) >> 8
			);
			imgDest.SetColor( x, y, result );
		}
	}
}

static void BilateralFilter( const Bitmap_t &imgSrc, Bitmap_t &imgDest, int radius, float colorDiffThreshold, float amount = 1.0f )
{

	// Make sure image is allocated properly
	int nWidth = imgSrc.Width();
	int nHeight = imgSrc.Height();
	imgDest.Init( nWidth, nHeight, IMAGE_FORMAT_RGBA8888 );

	float flWeightBias = (2 + radius + radius );
	int filteredBlendWeight = int(amount * 256.0f);
	int originalBlendWeight = 256 - filteredBlendWeight;

	// For each dest pixel
	for ( int y = 0 ; y < nHeight ; ++y )
	{
		for ( int x = 0 ; x < nWidth ; ++x )
		{
			Color c = imgSrc.GetColor( x, y );

			// Iterate over the kernel
			Vector4D sum(0,0,0,0);
			float flTotalWeight = 0.0f;
			for (int ry = -radius ; ry <= radius ; ++ry )
			{
				int sy = clamp(y + ry, 0, nHeight-1);
				for (int rx = -radius ; rx <= radius ; ++rx )
				{
					int sx = clamp(x + rx, 0, nWidth-1);

					Color s = imgSrc.GetColor( sx, sy );

					// Calculate difference.  Here, maybe we should be using
					// a perceptual difference in linear color space.  Who cares.
					float colorDist = ApproxColorDist( c, s );

					// Geometry-based weight
					float geomWeight = flWeightBias - fabs((float)ry) - fabs((float)rx);

					// Distance-based weight
					float diffWeight = 1.0f - colorDist - colorDiffThreshold;

					// Total weight
					float weight = geomWeight * diffWeight;
					if ( weight > 0.0f )
					{
						sum.x += (float)s.r() * weight;
						sum.y += (float)s.g() * weight;
						sum.z += (float)s.b() * weight;
						sum.w += (float)s.a() * weight;
						flTotalWeight += weight;
					}
				}
			}

			sum /= flTotalWeight;
			int filterR = (int)clamp(sum.x, 0.0f, 255.0f);
			int filterG = (int)clamp(sum.y, 0.0f, 255.0f);
			int filterB = (int)clamp(sum.z, 0.0f, 255.0f);
			int filterA = (int)clamp(sum.w, 0.0f, 255.0f);
			Color result(
				(filterR*filteredBlendWeight + c.r()*originalBlendWeight) >> 8,
				(filterG*filteredBlendWeight + c.g()*originalBlendWeight) >> 8,
				(filterB*filteredBlendWeight + c.b()*originalBlendWeight) >> 8,
				(filterA*filteredBlendWeight + c.a()*originalBlendWeight) >> 8
			);
			imgDest.SetColor( x, y, result );
		}
	}
}

// Scan image and replace each pixel with the closest matching swatch
static void ColorReplace( const Bitmap_t &imgSrc, Bitmap_t &imgDest, int nSwatchCount, const Color *pSwatchList, float amount = 1.0f, const float *pSwatchWeightList = NULL )
{
	Assert( nSwatchCount >= 1 );

	CUtlVector<Vector> swatchLab;
	for ( int i = 0 ; i < nSwatchCount ; ++i )
	{
		swatchLab.AddToTail( TextureToLab( pSwatchList[i] ) );
	}

	// Make sure image is allocated properly
	int nWidth = imgSrc.Width();
	int nHeight = imgSrc.Height();
	imgDest.Init( nWidth, nHeight, IMAGE_FORMAT_RGBA8888 );

	CUtlVector<float> vecDistScale;
	if ( pSwatchWeightList )
	{
		float total = 0.0f;
		for (int i = 0 ; i < nSwatchCount ; ++i)
		{
			total += pSwatchWeightList[i];
		}
		total *= 1.05f;
		for (int i = 0 ; i < nSwatchCount ; ++i)
		{
			vecDistScale.AddToTail( total - pSwatchWeightList[i] );
		}
	}
	else
	{
		for (int i = 0 ; i < nSwatchCount ; ++i)
		{
			vecDistScale.AddToTail( 1.0f );
		}
	}

	// For each dest pixel
	for ( int y = 0 ; y < nHeight ; ++y )
	{
		for ( int x = 0 ; x < nWidth ; ++x )
		{
			// Fetch source color
			Color c = imgSrc.GetColor( x, y );
			Vector lab = TextureToLab( c );

			// Search for the closest matching swatch in the palette
			Color closestSwatchColor = pSwatchList[0];
			//int bestDist = ApproxColorDistSq( c, closestSwatchColor );
			float bestDist = lab.DistTo( swatchLab[0] ) * vecDistScale[0];
			for ( int i = 1 ; i < nSwatchCount ; ++i )
			{
				//int dist = ApproxColorDistSq( c, pSwatchList[i] );
				float dist = lab.DistTo( swatchLab[i] ) * vecDistScale[i];
				if ( dist < bestDist )
				{
					bestDist = dist;
					closestSwatchColor = pSwatchList[i];
				}
			}

			imgDest.SetColor( x, y, LerpColor( c, closestSwatchColor, amount ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Custom control for the gradient editing
//-----------------------------------------------------------------------------
class CustomTextureStencilGradientMapWidget : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CustomTextureStencilGradientMapWidget, vgui::Panel );

public:
	CustomTextureStencilGradientMapWidget(vgui::Panel *parent, const char *panelName);

	// Slam range count, forcing nobs to be spaced evenly
	void InitRangeCount( int nRangeCount );

	// Set new number of ranges, attempting to adjust nob positions in a "reasonable" way
	void AdjustRangeCount( int nRangeCount );
	int GetRangeCount() const { return m_nRangeCount; }
	int GetNobCount() const { return m_nRangeCount-1; }
	int GetNobValue( int nNobIndex ) const; // allows virtual "nobs" at indices -1 and m_nRangeCount
	void SetNobValue( int nNobIndex, int value ); // clamp to adjacent nobs
	void SlamNobValue( int nNobIndex, int value ); // force nob to particular value, and don't check it
	void SetRangeColors( const Color *rColors )
	{
		memcpy( m_colRangeColor, rColors, m_nRangeCount*sizeof(m_colRangeColor[0]) );
		ComputeGradient();
	}

	/// Convert local x coordinate to value
	int LocalXToVal( int x, bool bClamp = true );

	/// Convert value to local x coordinate
	int ValToLocalX( int value, bool bClamp = true );

	enum { k_nMaxRangeCount = 4 };

	virtual void OnCursorMoved(int x, int y);
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseDoublePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);

	Color m_colorGradient[ 256 ];

protected:
	virtual void Paint();
	virtual void PaintBackground();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);


	int m_iDraggedNob; // -1 if none
	int m_nRangeCount;
	int m_nNobVal[k_nMaxRangeCount-1];
	Color m_colRangeColor[k_nMaxRangeCount];
	int m_iNobSizeX;
	int m_iNobSizeY;
	int m_iNobRelPosY;
	int m_iRibbonSizeY;
	int m_iRibbonRelPosY;
	int m_iMinVal;
	int m_iMaxVal;
	int m_iNobValCushion; // closest that we allow two nobs to be together
	int m_iClickOffsetX;

	// size (in intensity values on 255 scale) of transition band centered on nob
	int m_iTransitionBandSize;

	// The regions between the nobs are not *quite* a sold color.  They have a slight
	// gradient in them.  This value control the max difference in the ends of this
	// gradient
	int m_iRegionGradientRange;

	Color m_TickColor;
	Color m_TrackColor;

	Color m_DisabledTextColor1;
	Color m_DisabledTextColor2;

	vgui::IBorder *_sliderBorder;
	vgui::IBorder *_insetBorder;

	void SendSliderMovedMessage();

	/// Mouse hit testing.  Returns index of the nob under the cursor, or
	/// -1 if none.  Coords are local
	int HitTest( int x, int y, int &outOffsetX );

	/// Fetch local rectangle for given nob
	void GetNobRect( int iNobIndex, int &x1, int &y1, int &xs, int &ys );

	void GetColorRibbonRect( int &x1, int &y1, int &xs, int &ys );

	void ComputeSizes();
	void ComputeGradient();

	bool m_bClickOnRanges;
};

DECLARE_BUILD_FACTORY( CustomTextureStencilGradientMapWidget );

//-----------------------------------------------------------------------------
CustomTextureStencilGradientMapWidget::CustomTextureStencilGradientMapWidget(Panel *parent, const char *panelName )
: Panel(parent, panelName)
{
	m_iDraggedNob = -1;
	m_nRangeCount = 4;
	m_nNobVal[0] = 64;
	m_nNobVal[1] = 128;
	m_nNobVal[2] = 192;
	m_colRangeColor[0] = Color(183,224,252,255);
	m_colRangeColor[1] = Color(83,109,205,255);
	m_colRangeColor[2] = Color(98,48,43,255);
	m_colRangeColor[3] = Color(234,198,113,255);
	m_iNobSizeX = 4;
	m_iNobSizeY = 4;
	m_iNobRelPosY = 0;
	m_iRibbonSizeY = 0;
	m_iRibbonRelPosY = 4;
	m_iMinVal = 0;
	m_iMaxVal = 255;
	m_iNobValCushion = 8;
	m_iClickOffsetX = 0;
	m_bClickOnRanges = false;

	m_iTransitionBandSize = 8;
	m_iRegionGradientRange = 8;

	_sliderBorder = NULL;
	_insetBorder = NULL;

	//AddActionSignalTarget( parent );
	SetBlockDragChaining( true );
}

//-----------------------------------------------------------------------------
// Purpose: Send a message to interested parties when the slider moves
//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::SendSliderMovedMessage()
{	
	// send a changed message
	KeyValues *pParams = new KeyValues("SliderMoved");
	pParams->SetPtr( "panel", this );
	PostActionSignal( pParams );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(GetSchemeColor("Slider.NobColor", pScheme));
	// this line is useful for debugging
	//SetBgColor(GetSchemeColor("0 0 0 255"));

	m_TickColor = pScheme->GetColor( "Slider.TextColor", GetFgColor() );
	m_TrackColor = pScheme->GetColor( "Slider.TrackColor", GetFgColor() );

	m_DisabledTextColor1 = pScheme->GetColor( "Slider.DisabledTextColor1", GetFgColor() );
	m_DisabledTextColor2 = pScheme->GetColor( "Slider.DisabledTextColor2", GetFgColor() );

	_sliderBorder = pScheme->GetBorder("ButtonBorder");
	_insetBorder = pScheme->GetBorder("ButtonDepressedBorder");

	ComputeSizes();
	ComputeGradient();
}

//-----------------------------------------------------------------------------
// Purpose: Draw everything on screen
//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::Paint()
{
//	DrawTicks();
//
//	DrawTickLabels();
//
//	// Draw nob last so it draws over ticks.
//	DrawNob();

	// Draw nobs last
	for ( int i = 0 ; i < GetNobCount() ; ++i )
	{
		int x1, y1, xs, ys;
		GetNobRect( i, x1, y1, xs, ys );

		Color col = GetFgColor();
		g_pMatSystemSurface->DrawSetColor(col);
		g_pMatSystemSurface->DrawFilledRect(
			x1, 
			y1, 
			x1+xs, 
			y1+ys
		);

	}

//	// border
//	if (_sliderBorder)
//	{
//		_sliderBorder->Paint(
//			_nobPos[0], 
//			y + tall / 2 - nobheight / 2, 
//			_nobPos[1], 
//			y + tall / 2 + nobheight / 2);
//	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw the slider track
//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::PaintBackground()
{
	BaseClass::PaintBackground();

	int x1, y1, xs, ys;
	GetColorRibbonRect( x1, y1, xs, ys );

	// This is utterly terrible.  It could be drawn a LOT more efficiently!
	for ( int x = 0 ; x < xs ; ++x )
	{
		int v = x * 256 / xs;
		vgui::surface()->DrawSetColor( m_colorGradient[v] ); 
		vgui::surface()->DrawFilledRect( x1 + x, y1, x1 + x + 1, y1 + ys );
	}

//	int x, y;
//	int wide,tall;
//
//	GetTrackRect( x, y, wide, tall );
//
//	surface()->DrawSetColor( m_TrackColor ); 
//	surface()->DrawFilledRect( x, y, x + wide, y + tall );
//	if (_insetBorder)
//	{
//		_insetBorder->Paint( x, y, x + wide, y + tall );
//	}
}

void CustomTextureStencilGradientMapWidget::ComputeGradient()
{

	struct GradientInterpolationPoint
	{
		int m_iVal;
		Color m_color;
	};

	GradientInterpolationPoint rGradPoints[ k_nMaxRangeCount * 2 ];
	int nGradPoints = 0;

	// Put two interpolation points per region.
	for ( int iRange = 0 ; iRange < m_nRangeCount ; ++iRange )
	{
		// Get nob values on either side
		// of the region
		int lVal = GetNobValue( iRange-1 );
		int rVal = GetNobValue( iRange );

		// Push them together slightly, to create a small gradient band
		// around the nobs
		int d = rVal - lVal;
		if ( d > 2 )
		{
			int iPush = MIN ( d, m_iTransitionBandSize ) / 2;
			if ( iRange > 0 )
			{
				lVal += iPush;
			}
			if ( iRange < m_nRangeCount-1 )
			{
				rVal -= iPush;
			}
		}

		Color lColor = m_colRangeColor[iRange];
		Color rColor = m_colRangeColor[iRange];
		// !FIXME! Nudge color towards neighbors

		// Insert interpolation points
		Assert( nGradPoints+2 <= ARRAYSIZE( rGradPoints ) );
		rGradPoints[ nGradPoints ].m_iVal = lVal;
		rGradPoints[ nGradPoints ].m_color = lColor;
		++nGradPoints;
		rGradPoints[ nGradPoints ].m_iVal = rVal;
		rGradPoints[ nGradPoints ].m_color = rColor;
		++nGradPoints;
	}

	// Now fill in gradient
	Assert( m_iMinVal == 0 );
	Assert( m_iMaxVal == 255 );
	COMPILE_TIME_ASSERT( ARRAYSIZE( m_colorGradient ) == 256 );

	int iRightIndex = 1; // current interpolation point on right hand side
	for ( int i = 0 ; i < 256 ; ++i )
	{
		while ( i >= rGradPoints[ iRightIndex ].m_iVal && iRightIndex < nGradPoints-1)
		{
			++iRightIndex;
		}
		int iLeftIndex = iRightIndex-1;
		int iLeftVal = rGradPoints[ iLeftIndex ].m_iVal;
		int iRightVal = rGradPoints[ iRightIndex ].m_iVal;
		Assert( i >= iLeftVal );
		Assert( i <= iRightVal );

		Color lColor = rGradPoints[ iLeftIndex ].m_color;
		Color rColor = rGradPoints[ iRightIndex ].m_color;

		if ( i <= iLeftVal )
		{
			m_colorGradient[i] = lColor;
		}
		else if ( i >= iRightVal  )
		{
			m_colorGradient[i] = rColor;
		}
		else
		{
			float pct = float( i - iLeftVal ) / float( iRightVal - iLeftVal );
			m_colorGradient[i] = LerpColor( lColor, rColor, pct );
		}
	}

}

void CustomTextureStencilGradientMapWidget::ComputeSizes()
{
	m_iNobSizeX = 5;
	int sizeY = GetTall();

	m_iNobSizeY = sizeY * 2 / 5;
	m_iNobRelPosY = sizeY - m_iNobSizeY;

	m_iRibbonRelPosY = 0;
	m_iRibbonSizeY = m_iNobRelPosY - 1;
}

void CustomTextureStencilGradientMapWidget::GetColorRibbonRect( int &x1, int &y1, int &xs, int &ys )
{
	int controlSizeX, controlSizeY;
	GetSize( controlSizeX, controlSizeY );

	x1 = 0;
	xs = controlSizeX;
	y1 = m_iRibbonRelPosY;
	ys = m_iRibbonSizeY;
}

void CustomTextureStencilGradientMapWidget::GetNobRect( int iNobIndex, int &x1, int &y1, int &xs, int &ys )
{

	Assert( iNobIndex >= 0 );
	Assert( iNobIndex < GetNobCount() );

	// Fetch x center position
	int iNobVal = GetNobValue( iNobIndex );
	int cx = ValToLocalX( iNobVal );

	int controlSizeX, controlSizeY;
	GetSize( controlSizeX, controlSizeY );

	x1 = cx - m_iNobSizeX/2;
	xs = m_iNobSizeX;
	y1 = m_iNobRelPosY;
	ys = m_iNobSizeY;
}

int CustomTextureStencilGradientMapWidget::HitTest( int x, int y, int &outOffsetX )
{
	int result = -1;
	const int k_Tol = 3;
	int bestDist = k_Tol;
	outOffsetX = 0;
	for ( int i = 0 ; i < GetNobCount() ; ++i )
	{
		int x1, y1, xs, ys;
		GetNobRect( i, x1, y1, xs, ys );

		// Reject if too far away on Y
		if ( !m_bClickOnRanges )
		{
			y1 = 0;
			ys = GetTall();
		}
		if ( y < y1-k_Tol ) continue;
		if ( y > y1+ys+k_Tol) continue;

		// Get horizontal error
		int d = 0;
		if ( x < x1 ) d = x1 - x;
		else if ( x > x1+xs) d = x - (x1+xs);

		// Closest match found so far?
		if ( d < bestDist )
		{
			bestDist = d;
			result = i;
			outOffsetX = (x1 + xs/2) - x;
		}
	}

	return result;
}

int CustomTextureStencilGradientMapWidget::ValToLocalX( int value, bool bClamp )
{
	int w = GetWide();
	if ( bClamp )
	{
		if ( value < m_iMinVal ) return 0;
		if ( value >= m_iMaxVal ) return w;
	}

	int r = m_iMaxVal - m_iMinVal;

	// Don't divide by zero
	if (r < 1 )
	{
		return 0;
	}

	return ( ( value - m_iMinVal ) * w + (w>>1) ) / r;
}

int CustomTextureStencilGradientMapWidget::LocalXToVal( int x, bool bClamp )
{
	int w = GetWide();

	// Don't divide by zero
	if (w < 1 )
	{
		return m_iMinVal;
	}

	if ( bClamp )
	{
		if ( x < 0 ) return m_iMinVal;
		if ( x >= w ) return m_iMaxVal;
	}

	int r = m_iMaxVal - m_iMinVal;
	return m_iMinVal + ( x * r + (r>>1) ) / w;
}

int CustomTextureStencilGradientMapWidget::GetNobValue( int nNobIndex ) const
{

	// Sentinel nob to the left?
	if ( nNobIndex < 0 )
	{
		Assert( nNobIndex == -1 );
		return m_iMinVal;
	}

	// Sentinel nob to the right?
	if ( nNobIndex >= GetNobCount() )
	{
		Assert( nNobIndex == GetNobCount() );
		return m_iMaxVal;
	}

	return m_nNobVal[ nNobIndex ];
}

void CustomTextureStencilGradientMapWidget::SetNobValue( int nNobIndex, int value )
{
	if ( nNobIndex < 0 || nNobIndex >= GetNobCount() )
	{
		Assert( nNobIndex >= 0 );
		Assert( nNobIndex < GetNobCount() );
		return;
	}

	// Get neighboring nob values
	int iValLeft = GetNobValue( nNobIndex-1 );
	int iValRight = GetNobValue( nNobIndex+1 );
	Assert( iValLeft < iValRight );

	// Subtract off the cushion
	iValLeft += m_iNobValCushion;
	iValRight -= m_iNobValCushion;

	// No wiggle room?!?!
	if ( iValLeft > iValRight )
	{
		Assert( iValLeft <= iValRight );

		// Do the best we can
		value = (iValLeft + iValRight) / 2;
	}
	else
	{
		if ( value < iValLeft )
		{
			value = iValLeft;
		}
		else if ( value > iValRight )
		{
			value = iValRight;
		}
	}

	// We've clamped the value --- now slam it in place
	SlamNobValue( nNobIndex, value );
}

void CustomTextureStencilGradientMapWidget::InitRangeCount( int nRangeCount )
{
	m_nRangeCount = clamp( nRangeCount, 2, k_nMaxRangeCount );
	for ( int i = 0 ; i < GetNobCount() ; ++i )
	{
		SlamNobValue( i, m_iMinVal + ( i + 1 ) * ( m_iMaxVal - m_iMinVal ) / m_nRangeCount );
	}
}

void CustomTextureStencilGradientMapWidget::AdjustRangeCount( int nRangeCount )
{
	nRangeCount = clamp( nRangeCount, 2, k_nMaxRangeCount );
	Assert( m_nRangeCount >= 2 );

	int oldNobCount = GetNobCount();
	int oldRangeCount = m_nRangeCount;
	m_nRangeCount = nRangeCount;
	if ( m_nRangeCount < oldRangeCount )
	{
		// Removing ranges / nobs.  Just need to space existing nobs further apart
		//
		// Work from back to front, so we won't
		// conflict with the safety checks in SetNobValue
		for ( int i = GetNobCount()-1 ; i >= 0 ; --i )
		{
			SetNobValue( i, m_iMinVal + ( GetNobValue( i ) - m_iMinVal ) * oldRangeCount / m_nRangeCount );
		}
	}
	else if ( m_nRangeCount > oldRangeCount )
	{
		// Adding ranges / nobs.  Compress existing nobs, and add the
		// new once evenly at the top

		// Slam new nob values to be space evenly in the space at the top
		for ( int i = oldNobCount ; i < GetNobCount() ; ++i )
		{
			SlamNobValue( i, m_iMinVal + ( i + 1 ) * ( m_iMaxVal - m_iMinVal ) / m_nRangeCount );
		}

		// Work from front to back, so we won't
		// conflict with the safety checks in SetNobValue
		for ( int i = 0 ; i < oldNobCount ; ++i )
		{
			SetNobValue( i, m_iMinVal + ( GetNobValue( i ) - m_iMinVal ) * oldRangeCount / m_nRangeCount );
		}
	}
}

//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::SlamNobValue( int nNobIndex, int value )
{
	if ( nNobIndex < 0 || nNobIndex >= GetNobCount() )
	{
		Assert( nNobIndex >= 0 );
		Assert( nNobIndex < GetNobCount() );
		return;
	}

	Assert( value >= m_iMinVal );
	Assert( value <= m_iMaxVal );
	m_nNobVal[ nNobIndex ] = value;
	ComputeGradient();
}

//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::OnCursorMoved(int x,int y)
{
	if( m_iDraggedNob < 0 )
	{
		return;
	}

	g_pVGuiInput->GetCursorPosition( x, y );
	ScreenToLocal(x,y);

	x += m_iClickOffsetX;
	int v = LocalXToVal( x, true );
	SetNobValue( m_iDraggedNob, v );

	Repaint();
	SendSliderMovedMessage();
}

//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::OnMousePressed(vgui::MouseCode code)
{
	int x,y;

    if (!IsEnabled())
        return;

	g_pVGuiInput->GetCursorPosition( x, y );

	ScreenToLocal(x,y);
    RequestFocus();

	m_iDraggedNob = HitTest( x, y, m_iClickOffsetX );

	if ( m_iDraggedNob >= 0 )
	{
		// drag the nob
		g_pVGuiInput->SetMouseCapture(GetVPanel());
	}
}

//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::OnMouseDoublePressed(vgui::MouseCode code)
{
	// Just handle double presses like mouse presses
	OnMousePressed(code);
}


//-----------------------------------------------------------------------------
void CustomTextureStencilGradientMapWidget::OnMouseReleased(vgui::MouseCode code)
{

	if ( m_iDraggedNob >= 0 )
	{
		m_iDraggedNob = -1;
		g_pVGuiInput->SetMouseCapture(null);
	}
}

//-----------------------------------------------------------------------------
// Purpose: UI to select the custom image and confirm tool application
//-----------------------------------------------------------------------------
class CConfirmCustomizeTextureDialog : public CBaseToolUsageDialog, private ITextureRegenerator
{
	DECLARE_CLASS_SIMPLE( CConfirmCustomizeTextureDialog, CBaseToolUsageDialog );

public:
	CConfirmCustomizeTextureDialog( vgui::Panel *pParent, CEconItemView *pTool, CEconItemView *pToolSubject );
	virtual ~CConfirmCustomizeTextureDialog( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Apply( void );
	virtual void	OnCommand( const char *command );
	virtual void	OnTick( void );

	void ConversionError( ConversionErrorType nError );

	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );
	//MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel ); // send by the filter combo box when it changes

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel )
	{
		if ( eCurrentPage != ePage_SelectImage )
		{
			Assert( eCurrentPage == ePage_SelectImage );
			return;
		}
		if ( panel == m_pUseAvatarRadioButton )
		{
			if ( !m_bUseAvatar )
			{
				UseAvatarImage();
			}
		}
		else if ( panel == m_pUseAnyImageRadioButton )
		{
			if ( m_bUseAvatar )
			{
				m_imgSource.Clear();
				m_bUseAvatar = false;
			}
			MarkSquareImageDirty();
			WriteSelectImagePageControls();
		}
		else
		{
			Assert( false ); // who else is talking to us?
		}
	}

	MESSAGE_FUNC_PTR( OnSliderMoved, "SliderMoved", panel )
	{
		if ( panel == m_pStencilGradientWidget )
		{
			MarkFilteredImageDirty();
		}
		else
		{
			// What other is talking to us?
			Assert( false );
		}
	}

	void OnImageUploadedToCloud( RemoteStorageFileShareResult_t *pResult, bool bIOFailure );

	void CleanSquareImage()
	{
		if ( m_bSquareImageDirty )
		{
			PerformSquarize();
			Assert( !m_bSquareImageDirty );
			Assert( m_bFilteredImageDirty );
		}
	}

	void CleanFilteredImage()
	{
		CleanSquareImage();
		if ( m_bFilteredImageDirty )
		{
			PerformFilter();
			Assert( !m_bFilteredImageDirty );
		}
	}

	void CloseWithGenericError();

private:

	struct CroppedImagePanel : public CBitmapPanel {

		CroppedImagePanel( CConfirmCustomizeTextureDialog *pDlg, vgui::Panel *parent )
			: CBitmapPanel( parent, "PreviewCroppedImage" )
			, m_pDlg(pDlg)
		{
		}

		CConfirmCustomizeTextureDialog *m_pDlg;

		void Paint()
		{
			m_pDlg->CleanSquareImage();
			CBitmapPanel::Paint();
		}
	};

	struct FilteredImagePanel : public CBitmapPanel {

		FilteredImagePanel( CConfirmCustomizeTextureDialog *pDlg, vgui::Panel *parent )
			: CBitmapPanel( parent, "PreviewFilteredImage" )
			, m_pDlg(pDlg)
		{
		}

		CConfirmCustomizeTextureDialog *m_pDlg;

		void Paint()
		{
			m_pDlg->CleanFilteredImage();
			CBitmapPanel::Paint();
		}
	};

	vgui::FileOpenDialog	*m_hImportImageDialog;
	CBitmapPanel			*m_pFilteredTextureImagePanel;
	CBitmapPanel			*m_pCroppedTextureImagePanel;
	bool					m_bFilteredImageDirty;
	bool					m_bSquareImageDirty;
	bool					m_bStencilShapeReducedImageDirty;
	bool					m_bUseAvatar;
	bool					m_bCropToSquare; // if false, we'll stretch
	int						m_nSelectedStencilPalette;
	CUtlVector< CUtlVector< Color > > m_vecStencilPalettes;
	CustomTextureStencilGradientMapWidget *m_pStencilGradientWidget;

	enum EPage
	{
		ePage_SelectImage,
		ePage_AdjustFilter,
		ePage_FinalConfirm,
		ePage_PerformingAction,

		k_NumPages
	};
	EPage eCurrentPage;
	void SetPage( EPage page );

	// Page container widgets
	vgui::EditablePanel *m_rpPagePanel[k_NumPages];
	CItemModelPanel *m_pItemModelPanel;

	Bitmap_t m_imgSource; // original resolution and aspect
	Bitmap_t m_imgSquare; // cropped/stretched to square at submitted res
	Bitmap_t m_imgSquareDisplay; // cropped/stretched to square at final res
	Bitmap_t m_imgFinal; // final output res
	Bitmap_t m_imgStencilShapeReduced;

	/// Custom compositing steps defined for this item
	CUtlVector<SDecalBlendLayer> m_vecBlendLayers;

	inline bool IsSourceImageSquare() const
	{
		// We must know the size
		Assert( m_imgSource.IsValid() );
		return
			m_imgSource.Width()*99 < m_imgSource.Height()*100
			&& m_imgSource.Height()*99 < m_imgSource.Width()*100;
	}

	ITexture *m_pCurrentPreviewedTexture;

	void ActivateFileOpenDialog();
	void PerformSquarize();
	void PerformFilter();

	vgui::ComboBox *m_pFilterCombo;
	vgui::ComboBox *m_pSquarizeCombo;
	vgui::ComboBox *m_pStencilModeCombo;
	vgui::RadioButton *m_pUseAvatarRadioButton;
	vgui::RadioButton *m_pUseAnyImageRadioButton;

	enum EFilter
	{
		eFilter_Stencil,
		eFilter_Identity,
		eFilter_Painterly,
	};

	void PerformIdentityFilter();
	void PerformStencilFilter();
	void PerformPainterlyFilter();

	// From ITextureRegenerator
	virtual void RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect );
	virtual void Release();

	void MarkSquareImageDirty()
	{
		m_bSquareImageDirty = true;
		MarkStencilShapeReducedImageDirty();
		MarkFilteredImageDirty();
	}

	void MarkStencilShapeReducedImageDirty()
	{
		m_bStencilShapeReducedImageDirty = true;
		MarkFilteredImageDirty();
	}

	void MarkFilteredImageDirty()
	{
		m_bFilteredImageDirty = true;
		g_pPreviewCustomTextureDirty = true;
	}

	void ShowFilterControls();
	void WriteSelectImagePageControls();
	void UseAvatarImage();
	void SelectStencilPalette( int nPalette );

	// Test harness, for tweaking various values
	#ifdef TEST_FILTERS
		void TestFilters();
	#endif
};

CConfirmCustomizeTextureDialog::CConfirmCustomizeTextureDialog( vgui::Panel *parent, CEconItemView *pTool, CEconItemView *pToolSubject ) 
: CBaseToolUsageDialog( parent, "ConfirmCustomizeTextureDialog", pTool, pToolSubject )
, m_hImportImageDialog( NULL )
, m_bFilteredImageDirty(true)
, m_bStencilShapeReducedImageDirty(true)
, m_bSquareImageDirty(true)
, m_bCropToSquare(false)
, m_bUseAvatar(true)
, m_pCurrentPreviewedTexture(NULL)
, m_pFilterCombo(NULL)
, m_pSquarizeCombo(NULL)
, m_pStencilModeCombo(NULL)
, m_pUseAvatarRadioButton(NULL)
, m_pUseAnyImageRadioButton(NULL)
, m_pStencilGradientWidget(NULL)
, m_nSelectedStencilPalette(-1)
{
	// clear so that the preview is accurate
	Assert( g_pPreviewCustomTexture == NULL );
	Assert( g_pPreviewEconItem == NULL );
	eCurrentPage = ePage_SelectImage;

	m_pItemModelPanel = new CItemModelPanel( this, "paint_model" );
	m_pItemModelPanel->SetItem( pToolSubject );
	m_pItemModelPanel->SetActAsButton( true, false );

	COMPILE_TIME_ASSERT( k_NumPages == 4 );
	m_rpPagePanel[ePage_SelectImage] = new vgui::EditablePanel( this, "SelectImagePage" );
	m_rpPagePanel[ePage_AdjustFilter] = new vgui::EditablePanel( this, "AdjustFilterPage" );
	m_rpPagePanel[ePage_FinalConfirm] = new vgui::EditablePanel( this, "FinalConfirmPage" );
	m_rpPagePanel[ePage_PerformingAction] = new vgui::EditablePanel( this, "PerformingActionPage" );

	vgui::EditablePanel *pSelectImagePreviewGroupBox = new vgui::EditablePanel( m_rpPagePanel[ePage_SelectImage], "PreviewImageGroupBox" );
	m_pCroppedTextureImagePanel = new CroppedImagePanel( this, pSelectImagePreviewGroupBox );

	vgui::EditablePanel *pAdjustFilterPreviewGroupBox = new vgui::EditablePanel( m_rpPagePanel[ePage_AdjustFilter], "PreviewImageGroupBox" );
	m_pFilteredTextureImagePanel = new FilteredImagePanel( this, pAdjustFilterPreviewGroupBox );

	//
	// Locate / create the procedoral material & texture to show the
	// results of the filtered texture
	//

	ITexture *pPreviewTexture = NULL;
	if ( g_pMaterialSystem->IsTextureLoaded( k_rchCustomTextureFilterPreviewTextureName ) )
	{
		pPreviewTexture = g_pMaterialSystem->FindTexture( k_rchCustomTextureFilterPreviewTextureName, TEXTURE_GROUP_VGUI );
		pPreviewTexture->AddRef();
		Assert( pPreviewTexture );
	}
	else
	{
		pPreviewTexture = g_pMaterialSystem->CreateProceduralTexture(
			k_rchCustomTextureFilterPreviewTextureName,
			TEXTURE_GROUP_VGUI,
			k_nCustomImageSize, k_nCustomImageSize, 
			IMAGE_FORMAT_RGBA8888,
			TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD
		);
		Assert( pPreviewTexture );
	}
	pPreviewTexture->SetTextureRegenerator( this ); // note carefully order of operations here.  See Release()
	g_pPreviewCustomTexture = pPreviewTexture;
	g_pPreviewCustomTextureDirty = true;
	g_pPreviewEconItem = m_pItemModelPanel->GetItem();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );

	// Parse blend operations from the tool definition KV
	KeyValues *pkvBlendLayers = pToolSubject->GetDefinitionKey( "custom_texture_blend_steps" );
	if ( pkvBlendLayers )
	{
		for ( KeyValues *kvLayer = pkvBlendLayers->GetFirstTrueSubKey() ; kvLayer ; kvLayer = kvLayer->GetNextTrueSubKey() )
		{
			int idx = m_vecBlendLayers.AddToTail();
			CUtlString sErrMsg;
			if ( !m_vecBlendLayers[idx].FromKV( kvLayer, sErrMsg ) )
			{
				Warning( "Bogus custom texture blend layer definition '%s'.  %s\n", kvLayer->GetName(), (const char *)sErrMsg );
				Assert( !"Bogus custom texture blend layer!" );
				m_vecBlendLayers.Remove( idx );
			}
		}
	}

	// Setup stencil palettes

	CUtlVector<Color> *pPalette;
	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 54, 38, 0, 255 ) );
	pPalette->AddToTail( Color( 236, 236, 217, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 54, 38, 0, 255 ) );
	pPalette->AddToTail( Color( 137, 131, 116, 255 ) );
	pPalette->AddToTail( Color( 236, 236, 217, 255 ) );
	pPalette->AddToTail( Color( 254, 255, 228, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 186, 80, 34, 255 ) );
	pPalette->AddToTail( Color( 243, 231, 194, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 186, 80, 34, 255 ) );
	pPalette->AddToTail( Color( 217, 162, 121, 255 ) );
	pPalette->AddToTail( Color( 243, 231, 194, 255 ) );
	pPalette->AddToTail( Color( 255, 247, 220, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 101, 72, 54, 255 ) );
	pPalette->AddToTail( Color( 229, 150, 73, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 101, 72, 54, 255 ) );
	pPalette->AddToTail( Color( 161, 100, 47, 255 ) );
	pPalette->AddToTail( Color( 229, 150, 73, 255 ) );
	pPalette->AddToTail( Color( 255, 207, 154, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 88, 84, 80, 255 ) );
	pPalette->AddToTail( Color( 160, 84, 72, 255 ) );
	pPalette->AddToTail( Color( 216, 212, 192, 255 ) );

	pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
	pPalette->AddToTail( Color( 54, 38, 0, 255 ) );
	pPalette->AddToTail( Color( 163, 110, 0, 255 ) );
	pPalette->AddToTail( Color( 215, 171, 2, 255 ) );
	pPalette->AddToTail( Color( 197, 192, 171, 255 ) );

	// !TEST! Import the palettes from an image
	#if 0
	{
		m_vecStencilPalettes.RemoveAll();
		Bitmap_t imgPal;
		Assert( ImgUtl_LoadBitmap( "d:/decal_tool_palettes_bay.png", imgPal ) == CE_SUCCESS );
		const int kSwatchSz = 10;
		for (int y = kSwatchSz/2 ; y < imgPal.Height() ; y += kSwatchSz )
		{
			CUtlVector<Color> palette;
			for (int x = kSwatchSz/2 ; x < imgPal.Width() ; x += kSwatchSz )
			{
				palette.AddToTail( imgPal.GetColor( x, y ) );
			}

			// Strip off solid white entries from the end.  (If these are in the palette,
			// they have to come first!)
			while ( palette.Count() > 0 && ApproxColorDistSq( palette[palette.Count()-1], Color(255,255,255,255) ) < 12 )
			{
				palette.Remove( palette.Count()-1 );
			}
			Assert( palette.Count() != 1 ); // only a single entry in the palette?  Should be 0, or at least 2
			if ( palette.Count() > 1 )
			{
				// Reverse the palette, so it is ordered dark -> light.
				for ( int l = 0, r = palette.Count()-1 ; l < r ; ++l, --r )
				{
					Color t = palette[l];
					palette[l] = palette[r];
					palette[r] = t;
				}

				CUtlVector<Color> *pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];
				Msg( "pPalette = &m_vecStencilPalettes[ m_vecStencilPalettes.AddToTail() ];\n" );
				for (int j = 0 ; j < palette.Count() ; ++j )
				{
					pPalette->AddToTail( palette[j] );
					Msg( "pPalette->AddToTail( Color( %d, %d, %d, 255 ) );\n", palette[j].r(), palette[j].g(), palette[j].b() );
				}
				Msg( "\n" );
			}
		}
	}
	#endif

}

CConfirmCustomizeTextureDialog::~CConfirmCustomizeTextureDialog( void )
{

	// Clean up filtered texture
	Release();

	delete m_hImportImageDialog;
	m_hImportImageDialog = NULL;
}

void CConfirmCustomizeTextureDialog::SetPage( EPage page )
{
	eCurrentPage = page;
	switch ( eCurrentPage )
	{
		default:
			Assert(false);
			eCurrentPage = ePage_SelectImage;
		case ePage_SelectImage:
			WriteSelectImagePageControls();
			break;

		case ePage_AdjustFilter:
			// Make sure proper controls are shown
			ShowFilterControls();
			break;

		case ePage_FinalConfirm:
			break;

		case ePage_PerformingAction:
			break;
	}

	// !KLUDGE! We need to hide ourselves while the file open dialog is up
	//SetVisible( eCurrentPage != ePage_SelectImage );

	for ( int i = 0 ; i < k_NumPages ; ++i )
	{
		if ( m_rpPagePanel[i] )
		{
			m_rpPagePanel[i]->SetVisible( i == eCurrentPage );
		}
	}

	vgui::EditablePanel *pPreviewProupPanel = NULL;
	if ( m_rpPagePanel[eCurrentPage] )
	{
		pPreviewProupPanel = dynamic_cast<vgui::EditablePanel *>( m_rpPagePanel[eCurrentPage]->FindChildByName( "PreviewModelGroupBox" ) );
	}
	if ( pPreviewProupPanel )
	{
		m_pItemModelPanel->SetVisible( true );
		m_pItemModelPanel->SetParent( pPreviewProupPanel );
		m_pItemModelPanel->SetPos( 10, 10 );
		m_pItemModelPanel->SetSize( pPreviewProupPanel->GetWide() - 20, pPreviewProupPanel->GetTall() - 20 );
		m_pItemModelPanel->UpdatePanels();
	}
	else
	{
		m_pItemModelPanel->SetVisible( false );
	}
}

void CConfirmCustomizeTextureDialog::ActivateFileOpenDialog()
{
	// Create the dialog the first time it's used
	if (m_hImportImageDialog == NULL)
	{
		m_hImportImageDialog = new vgui::FileOpenDialog( NULL, "#ToolCustomizeTextureBrowseDialogTitle", true );
#ifdef WIN32
		m_hImportImageDialog->AddFilter( "*.tga,*.jpg,*.png,*.bmp", "#GameUI_All_Images", true );
#else
		m_hImportImageDialog->AddFilter( "*.tga,*.jpg,*.png", "#GameUI_All_Images", true );
#endif
		m_hImportImageDialog->AddFilter( "*.tga", "#GameUI_TGA_Images", false );
		m_hImportImageDialog->AddFilter( "*.jpg", "#GameUI_JPEG_Images", false );
		m_hImportImageDialog->AddFilter( "*.png", "#GameUI_PNG_Images", false );
#ifdef WIN32
		m_hImportImageDialog->AddFilter( "*.bmp", "#GameUI_BMP_Images", false );
#endif
		m_hImportImageDialog->AddActionSignalTarget( this );
	}

	// Activate it
	m_hImportImageDialog->DoModal( false );
	m_hImportImageDialog->Activate();
}

void CConfirmCustomizeTextureDialog::OnCommand( const char *command )
{
	if (!stricmp( command, "pick_image" ) )
	{
		ActivateFileOpenDialog();
		return;
	}

	// !KLUDGE! Base class closes window.  I don't want to do this.
	if ( !Q_stricmp( command, "apply" ) )
	{
		Apply();
		return;
	}

	if ( !Q_stricmp( command, "next_page" ) )
	{
		if ( eCurrentPage < ePage_FinalConfirm )
		{
			SetPage( (EPage)(eCurrentPage + 1) );
		}
		return;
	}

	if ( !Q_stricmp( command, "prev_page" ) )
	{
		if ( eCurrentPage > ePage_SelectImage )
		{
			SetPage( (EPage)(eCurrentPage - 1) );
		}
		return;
	}

	if ( !Q_stricmp( command, "next_stencil_palette" ) )
	{
		SelectStencilPalette( m_nSelectedStencilPalette + 1 );
		return;
	}

	if ( !Q_stricmp( command, "prev_stencil_palette" ) )
	{
		SelectStencilPalette( m_nSelectedStencilPalette + m_vecStencilPalettes.Count() - 1 );
		return;
	}

	BaseClass::OnCommand( command );
}

void CConfirmCustomizeTextureDialog::SelectStencilPalette( int nPalette )
{
	while ( nPalette < 0 )
	{
		nPalette += m_vecStencilPalettes.Count();
	}
	m_nSelectedStencilPalette = nPalette % m_vecStencilPalettes.Count();
	MarkFilteredImageDirty();

	if ( m_pStencilGradientWidget )
	{
		const CUtlVector<Color> &pal = m_vecStencilPalettes[m_nSelectedStencilPalette];
		m_pStencilGradientWidget->InitRangeCount( pal.Count() );
		m_pStencilGradientWidget->SetRangeColors( pal.Base() );
	}
}


static void ListenToControlsRecursive( vgui::Panel *pPanel, vgui::Panel *pListener )
{
	if ( pPanel == NULL )
	{
		return;
	}
	if (
		dynamic_cast<vgui::Button *>( pPanel )
		|| dynamic_cast<vgui::Slider *>( pPanel )
		|| dynamic_cast<vgui::ComboBox *>( pPanel )
		|| dynamic_cast<CustomTextureStencilGradientMapWidget *>( pPanel )
	)
	{
		pPanel->AddActionSignalTarget( pListener );
	}
	else
	{
		for ( int i = 0 ; i < pPanel->GetChildCount() ; ++i )
		{
			ListenToControlsRecursive( pPanel->GetChild(i), pListener );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmCustomizeTextureDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	LoadControlSettings( "Resource/UI/econ/ConfirmCustomizeTextureDialog.res" );

	BaseClass::ApplySchemeSettings( pScheme );

	m_pFilterCombo = dynamic_cast<vgui::ComboBox *>( FindChildByName("FilterComboBox", true ) );
	Assert( m_pFilterCombo );
	if ( m_pFilterCombo )
	{
		m_pFilterCombo->RemoveAll();

		COMPILE_TIME_ASSERT( eFilter_Stencil == 0 );
		m_pFilterCombo->AddItem( "#ToolCustomizeTextureFilterStencil", NULL );

		//COMPILE_TIME_ASSERT( eFilter_Painterly == 0 );
		//m_pFilterCombo->AddItem( "#ToolCustomizeTextureFilterPainterly", NULL );

		COMPILE_TIME_ASSERT( eFilter_Identity == 1 );
		#ifdef _DEBUG
			m_pFilterCombo->AddItem( "None", NULL );
		#endif

		//m_pFilterCombo->SilentActivateItemByRow( eFilter_Painterly );
		m_pFilterCombo->SilentActivateItemByRow( eFilter_Stencil );
	}

	m_pSquarizeCombo = dynamic_cast<vgui::ComboBox *>( FindChildByName("SquarizeComboBox", true ) );
	Assert( m_pSquarizeCombo );

	m_pStencilModeCombo = dynamic_cast<vgui::ComboBox *>( FindChildByName("StencilModeComboBox", true ) );
	Assert( m_pStencilModeCombo );
	if ( m_pStencilModeCombo )
	{
		m_pStencilModeCombo->AddItem( "#ToolCustomizeTextureStencilMatchByIntensity", NULL );
		m_pStencilModeCombo->AddItem( "#ToolCustomizeTextureStencilMatchByColor", NULL );
		m_pStencilModeCombo->SilentActivateItemByRow( 0 );
	}

	m_pUseAvatarRadioButton = dynamic_cast<vgui::RadioButton *>( FindChildByName("UseAvatarRadio", true ) );
	m_pUseAnyImageRadioButton = dynamic_cast<vgui::RadioButton *>( FindChildByName("UseAnyimageRadio", true ) );

	m_pStencilGradientWidget = dynamic_cast<CustomTextureStencilGradientMapWidget *>( FindChildByName("StencilGradientMap", true ) );
	Assert( m_pStencilGradientWidget );

	for ( int i = 0 ; i < k_NumPages ; ++i )
	{
		ListenToControlsRecursive( m_rpPagePanel[i], this );
	}

	UseAvatarImage();

	SetPage( ePage_SelectImage );

	// Flip this flag to activate the test harness
	#ifdef TEST_FILTERS
		TestFilters();
	#endif

	SelectStencilPalette( 0 );
}

void CConfirmCustomizeTextureDialog::UseAvatarImage()
{
	// assume failure
	m_imgSource.Clear();
	m_bUseAvatar = false;

	if ( steamapicontext && steamapicontext->SteamUser() )
	{

		const int k_nAvatarImageSize = 184;
		m_imgSource.Init( k_nAvatarImageSize, k_nAvatarImageSize, IMAGE_FORMAT_RGBA8888 );
		int iAvatar = steamapicontext->SteamFriends()->GetLargeFriendAvatar( steamapicontext->SteamUser()->GetSteamID() );
		if ( !steamapicontext->SteamUtils()->GetImageRGBA( iAvatar, m_imgSource.GetBits(), k_nAvatarImageSize*k_nAvatarImageSize*4 ) )
		{
			m_imgSource.Clear();
		}
		else
		{
			m_bUseAvatar = true;
		}
	}

	WriteSelectImagePageControls();
	MarkSquareImageDirty();
}

void CConfirmCustomizeTextureDialog::WriteSelectImagePageControls()
{
	if ( !m_pSquarizeCombo )
	{
		return;
	}

	m_pSquarizeCombo->RemoveAll();

	CExButton *pNextButton = dynamic_cast<CExButton *>( m_rpPagePanel[ePage_SelectImage]->FindChildByName( "NextButton", true ) );
	if ( !pNextButton )
	{
		return;
	}

	if ( m_pUseAvatarRadioButton )
	{
		if ( !m_pUseAvatarRadioButton->IsSelected() )
		{
			m_pUseAvatarRadioButton->SetSelected( m_bUseAvatar );
		}
	}
	if ( m_pUseAnyImageRadioButton )
	{
		if ( !m_pUseAnyImageRadioButton->IsSelected() )
		{
			m_pUseAnyImageRadioButton->SetSelected( !m_bUseAvatar );
		}
	}

	if ( !m_imgSource.IsValid() )
	{
		// No image yet selected
		m_pSquarizeCombo->SetVisible( false );
		pNextButton->SetEnabled( false );
		m_pCroppedTextureImagePanel->SetVisible( false );
		return;
	}
	m_pCroppedTextureImagePanel->SetVisible( true );
	pNextButton->SetEnabled( true );

	// Nearly square already?
	if ( IsSourceImageSquare() )
	{
		// Nearly square.  No need to offer any options
		m_pSquarizeCombo->SetVisible( false );
	}
	else
	{
		m_pSquarizeCombo->AddItem( "#ToolCustomizeTextureStretch", NULL );
		m_pSquarizeCombo->AddItem( "#ToolCustomizeTextureCrop", NULL );
		m_pSquarizeCombo->SetVisible( true );
		m_pSquarizeCombo->ActivateItemByRow( m_bCropToSquare ? 1 : 0 );
	}
	

}

void CConfirmCustomizeTextureDialog::OnTick( void )
{
	BaseClass::OnTick();

	// Process, depending on currently selected page
	switch ( eCurrentPage )
	{
		default:
			Assert(false);
			eCurrentPage = ePage_SelectImage;
		case ePage_SelectImage:
			break;

		case ePage_AdjustFilter:
			break;

		case ePage_FinalConfirm:
			break;

		case ePage_PerformingAction:
			break;
	}
}

void CConfirmCustomizeTextureDialog::ShowFilterControls()
{
	EFilter f = (EFilter)m_pFilterCombo->GetActiveItem();

	vgui::Panel *p;

	p  = m_rpPagePanel[ePage_AdjustFilter]->FindChildByName( "PainterlyOptions", true );
	if ( p )
	{
		p->SetVisible( f == eFilter_Painterly );
	}

	p  = m_rpPagePanel[ePage_AdjustFilter]->FindChildByName( "StencilOptions", true );
	if ( p )
	{
		p->SetVisible( f == eFilter_Stencil );
	}
}

void CConfirmCustomizeTextureDialog::PerformSquarize()
{
	if ( m_bCropToSquare && !IsSourceImageSquare() )
	{
		// Select the smaller dimension as the size
		int nSize = MIN( m_imgSource.Width(), m_imgSource.Height() );

		// Crop it.
		// Yeah, the crop and resize could be done all in one step.
		// And...I don't care.
		int x0 = ( m_imgSource.Width() - nSize ) / 2;
		int y0 = ( m_imgSource.Height() - nSize ) / 2;
		m_imgSquare.Crop( x0, y0, nSize, nSize, &m_imgSource );
	}
	else
	{
		m_imgSquare.MakeLogicalCopyOf( m_imgSource );
	}

	// Reduce it for display purposes
	ImgUtl_ResizeBitmap( m_imgSquareDisplay, k_nCustomImageSize, k_nCustomImageSize, &m_imgSquare );

	// Square image is now up-to-date with options
	m_bSquareImageDirty = false;

	if ( m_pCroppedTextureImagePanel != NULL )
	{
		m_pCroppedTextureImagePanel->SetBitmap( m_imgSquareDisplay );
	}

	// We need to re-run our filter anytime this changes
	MarkFilteredImageDirty();
}

void CConfirmCustomizeTextureDialog::PerformFilter()
{

	// this can take a while, put up a waiting cursor
	vgui::surface()->SetCursor( vgui::dc_hourglass );

	switch ( (EFilter)m_pFilterCombo->GetActiveItem() )
	{
		case eFilter_Identity:
			// !FIXME! Only allow while in dev universe?
			PerformIdentityFilter();
			break;
		default:
			Assert( false );
		case eFilter_Stencil:
			PerformStencilFilter();
			break;
		case eFilter_Painterly:
			PerformPainterlyFilter();
			break;
	}

	// Now apply the blend layers
	static bool bDoBlendLayers = true;
	if ( bDoBlendLayers )
	{
		for ( int i = 0; i < m_vecBlendLayers.Size() ; ++i )
		{
			m_vecBlendLayers[i].Apply( m_imgFinal );
		}
	}

	// And the texture on the 3D model
	g_pPreviewCustomTextureDirty = true;

	m_bFilteredImageDirty = false;

	if ( m_pFilteredTextureImagePanel != NULL )
	{
		m_pFilteredTextureImagePanel->SetBitmap( m_imgFinal );
	}

	// change the cursor back to normal
	vgui::surface()->SetCursor( vgui::dc_user );
}

void CConfirmCustomizeTextureDialog::PerformIdentityFilter()
{
	ImgUtl_ResizeBitmap( m_imgFinal, k_nCustomImageSize, k_nCustomImageSize, &m_imgSquare );
}

void CConfirmCustomizeTextureDialog::PerformStencilFilter()
{

	// Check if the shape reduced image is dirty
	if ( m_bStencilShapeReducedImageDirty )
	{
		Bitmap_t imgTemp1, imgTemp2;

// Need a slider to control this.  Works OK for color match, poorly for intensity.
// Best for all cases is to just do nothing
//		// Downsample FIRST to 2X res
//		ImgUtl_ResizeBitmap( imgTemp1, k_nCustomImageSize*2, k_nCustomImageSize*2, &m_imgSquare );
//
//		// Run the bilateral filter several times
//		static float thresh1 = .7f; static int rad1 = 1; static float amount1 = 1.0f;
//		static float thresh2 = .8f; static int rad2 = 1; static float amount2 = 1.0f;
//		static float thresh3 = .9f; static int rad3 = 2; static float amount3 = 1.0f;
//		Bitmap_t t;
//		BilateralFilter( imgTemp1, imgTemp2, rad1, thresh1, amount1 );
//		static int rounds = 4;
//		for ( int r = 0 ; r < rounds ; ++r )
//		{
//			BilateralFilter( imgTemp2, imgTemp1, rad2, thresh2, amount2 );
//			BilateralFilter( imgTemp1, imgTemp2, rad2, thresh2, amount2 );
//		}
//		//BilateralFilter( imgTemp2, m_imgFinal, rad3, thresh3, amount3 );
//		BilateralFilter( imgTemp2, m_imgStencilShapeReduced, rad3, thresh3, amount3 );

		// Downsample FIRST to 2X res
		ImgUtl_ResizeBitmap( m_imgStencilShapeReduced, k_nCustomImageSize*2, k_nCustomImageSize*2, &m_imgSquare );

		m_bStencilShapeReducedImageDirty = false;
	}

	// Color matching
	{
//	Color swatches[] = 
//	{
//		Color( 255, 255, 255 ),
//		Color( 183, 224, 252 ), // sky light
//		Color( 83, 109, 205 ), // sky med
//		Color( 64, 68, 195 ), // sky dark
//		Color( 100, 68, 57 ), // skin demo
//		Color( 139, 101, 84 ), // skin demo light
//		Color( 133, 105, 68 ), // saxton hair
//		Color( 252, 169, 131 ), // skin light
//		Color( 194, 132, 106 ), // skin
//
//		//Color( 255, 255, 255 ),
//		//Color( 246, 231, 222 ),
//		//Color( 218, 189, 171 ),
//		//Color( 193, 161, 138 ),
//		//
//		//Color( 248, 185, 138 ),
//		//Color( 245, 173, 135 ),
//		//Color( 239, 152,  73 ),
//		//Color( 241, 129,  73 ),
//		//
//		//Color( 106,  69,  52 ),
//		//Color( 145,  58,  31 ),
//		//Color( 189,  58,  58 ),
//		//Color( 157,  48,  47 ),
//		//Color(  69,  44,  37 ),
//		//
//		//Color( 107, 106, 101 ),
//		//Color( 118, 138, 136 ),
//		//Color(  91, 122, 140 ),
//		//Color(  56,  92, 120 ),
//		//Color(  52,  47,  44 ),
//	};

		Bitmap_t imgTemp1;

		static float colorReplacePct = 1.0f;

		// match by color, or intensity?
		if ( m_pStencilModeCombo && m_pStencilModeCombo->GetActiveItem() == 0 && m_pStencilGradientWidget )
		{
			imgTemp1.Init( m_imgStencilShapeReduced.Width(), m_imgStencilShapeReduced.Height(), IMAGE_FORMAT_RGBA8888 );
			for ( int y = 0 ; y < imgTemp1.Height() ; ++y )
			{
				for ( int x = 0 ; x < imgTemp1.Width() ; ++x )
				{
					Color c = m_imgStencilShapeReduced.GetColor( x, y );
					Vector lab = TextureToLab( c );
					int index = clamp(lab.x * (255.0f/100.0f) + .5f, 0.0, 255.0f);
					imgTemp1.SetColor( x, y, m_pStencilGradientWidget->m_colorGradient[ index ] );
				}
			}
		}
		else
		{

			Assert( m_nSelectedStencilPalette >= 0 );
			Assert( m_nSelectedStencilPalette < m_vecStencilPalettes.Count() );
			const CUtlVector<Color> &pal = m_vecStencilPalettes[ m_nSelectedStencilPalette ];

			// Determine "weight" of each swatch, from the relative sizes of the
			// gradient widget ranges
			CUtlVector<float> vecSwatchWeight;
			for ( int i = 0 ; i < pal.Size() ; ++i )
			{
				float weight = 1.0f;
				if ( m_pStencilGradientWidget )
				{
					weight = float( m_pStencilGradientWidget->GetNobValue(i) - m_pStencilGradientWidget->GetNobValue(i - 1) );
				}
				vecSwatchWeight.AddToTail( weight );
				
			}

			ColorReplace( m_imgStencilShapeReduced, imgTemp1, pal.Count(), pal.Base(), colorReplacePct, vecSwatchWeight.Base() );
		}

		// Now downsample to the final size
		ImgUtl_ResizeBitmap( m_imgFinal, k_nCustomImageSize, k_nCustomImageSize, &imgTemp1 );\
	}

//	// !KLUDGE!
//	if ( m_pStencilGradientWidget == NULL )
//	{
//		PerformIdentityFilter();
//		return;
//	}
//
//	// Make sure temp image is properly allocated
//	imgTemp1.Init( m_imgSquare.Width(), m_imgSquare.Height(), IMAGE_FORMAT_RGBA8888 );
//
//	// Perform stencil operation
//	for ( int y = 0 ; y < m_imgSquare.Height() ; ++y )
//	{
//		for ( int x = 0 ; x < m_imgSquare.Height() ; ++x )
//		{
//			Color c = m_imgSquare.GetColor(x,y);
//
//			// Compute "value" using simple average.  (No visual
//			// weighting for this.)
//			int v = ( (int)c.r() + (int)c.g() + (int)c.g() ) / 3;
//
//			// Apply gradient map
//			Color result = m_pStencilGradientWidget->m_colorGradient[ v ];
//			imgTemp1.SetColor( x, y, result );
//		}
//	}
//
//	// Now downsample to the final size
//	ImgUtl_ResizeBitmap( m_imgFinal, k_nCustomImageSize, k_nCustomImageSize, &m_imgTemp );
}

const int k_BrushStrokeSize = 64;
static byte s_bBrushStrokeData[k_BrushStrokeSize][k_BrushStrokeSize] = 
{
	{ 0x8C, 0x86, 0x87, 0x87, 0x86, 0x88, 0x88, 0x87, 0x86, 0x88, 0x8F, 0x8E, 0x8C, 0x8E, 0x8D, 0x8E, 0x8B, 0x8A, 0x8A, 0x94, 0xAE, 0xB6, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB0, 0xB0, 0xB2, 0x9E, 0x9A, 0x9C, 0x9C, 0x9A, 0x99, 0x97, 0x98, 0x96, 0x9A, 0x9D, 0x9F, 0x9E, 0x9D, 0x9E, 0x9F, 0x9B, 0x9A, 0x99, 0x98, 0x95, 0x91, 0x8F, 0x8F, 0x8E, 0x89, 0x88, 0x89, 0x88, 0x85, 0x87, 0x8B }, 
	{ 0x85, 0x7C, 0x7D, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7D, 0x7F, 0x87, 0x88, 0x88, 0x89, 0x87, 0x86, 0x86, 0x83, 0x81, 0x8B, 0xA9, 0xB2, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xAF, 0xAD, 0xAD, 0xAE, 0x97, 0x92, 0x94, 0x94, 0x93, 0x92, 0x91, 0x92, 0x91, 0x94, 0x98, 0x9B, 0x9A, 0x99, 0x99, 0x99, 0x96, 0x95, 0x96, 0x98, 0x98, 0x98, 0x99, 0x9A, 0x93, 0x86, 0x84, 0x84, 0x80, 0x7D, 0x7D, 0x83 }, 
	{ 0x86, 0x7D, 0x7E, 0x81, 0x80, 0x7F, 0x7F, 0x82, 0x83, 0x84, 0x8A, 0x89, 0x86, 0x87, 0x88, 0x89, 0x86, 0x87, 0x85, 0x8E, 0xAA, 0xB2, 0xB0, 0xB1, 0xB0, 0xB1, 0xB1, 0xB1, 0xB0, 0xAE, 0xAD, 0xAD, 0x9B, 0x96, 0x96, 0x96, 0x94, 0x95, 0x94, 0x96, 0x97, 0x99, 0x9C, 0x9E, 0x9D, 0x9D, 0x9C, 0x9C, 0x9A, 0x99, 0x99, 0x99, 0x9A, 0x9B, 0x9D, 0x9E, 0x9B, 0x86, 0x85, 0x86, 0x83, 0x80, 0x7F, 0x88 }, 
	{ 0x84, 0x7D, 0x7F, 0x81, 0x7F, 0x80, 0x83, 0x88, 0x88, 0x88, 0x8B, 0x8A, 0x88, 0x89, 0x89, 0x89, 0x84, 0x85, 0x85, 0x8E, 0xAB, 0xB4, 0xB2, 0xB3, 0xB2, 0xB2, 0xB3, 0xB3, 0xB2, 0xB0, 0xAF, 0xAE, 0x99, 0x94, 0x94, 0x94, 0x93, 0x96, 0x97, 0x99, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9C, 0x99, 0x99, 0x99, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9D, 0x85, 0x85, 0x87, 0x84, 0x82, 0x7E, 0x87 }, 
	{ 0x85, 0x7D, 0x7E, 0x7E, 0x7F, 0x84, 0x88, 0x8A, 0x8A, 0x88, 0x88, 0x89, 0x87, 0x87, 0x89, 0x88, 0x89, 0x88, 0x86, 0x90, 0xAA, 0xB4, 0xB3, 0xB1, 0xB1, 0xB2, 0xB3, 0xB3, 0xB2, 0xB1, 0xAF, 0xAE, 0x97, 0x93, 0x94, 0x94, 0x95, 0x99, 0x9C, 0x9E, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x97, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9C, 0x9C, 0x86, 0x86, 0x86, 0x85, 0x85, 0x7F, 0x86 }, 
	{ 0x85, 0x7A, 0x76, 0x75, 0x77, 0x7D, 0x7D, 0x7A, 0x7B, 0x7D, 0x7D, 0x7D, 0x7A, 0x7F, 0x8E, 0x97, 0x90, 0x94, 0x9A, 0xA0, 0xAF, 0xB2, 0xB2, 0xB2, 0xB1, 0xB2, 0xB3, 0xB3, 0xB3, 0xB2, 0xB0, 0xAF, 0x96, 0x92, 0x94, 0x93, 0x93, 0x97, 0x99, 0x9A, 0x9A, 0x9B, 0x9C, 0x9E, 0x9E, 0x9D, 0x9C, 0x9C, 0x9A, 0x9A, 0x9B, 0x9C, 0x9E, 0x9F, 0x9F, 0x9E, 0x9B, 0x8A, 0x87, 0x87, 0x86, 0x85, 0x80, 0x86 }, 
	{ 0x89, 0x7B, 0x75, 0x74, 0x75, 0x77, 0x75, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7C, 0x82, 0x92, 0x9C, 0x97, 0x9F, 0xAB, 0xAE, 0xB1, 0xB1, 0xB4, 0xB4, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB3, 0xB2, 0x9A, 0x97, 0x99, 0x98, 0x97, 0x99, 0x98, 0x98, 0x9A, 0x9B, 0x9E, 0x9F, 0x9F, 0x9F, 0x9E, 0x9D, 0x9A, 0x9B, 0x9C, 0x9D, 0xA1, 0xA5, 0xA7, 0xA6, 0xA7, 0x9A, 0x94, 0x94, 0x93, 0x8A, 0x83, 0x89 }, 
	{ 0x8B, 0x7E, 0x7A, 0x7B, 0x79, 0x79, 0x78, 0x79, 0x75, 0x77, 0x7E, 0x90, 0xA0, 0xA8, 0xAC, 0xA7, 0xA9, 0xAA, 0xB0, 0xB0, 0xB1, 0xB1, 0xB4, 0xB2, 0xB3, 0xB5, 0xB6, 0xB5, 0xB5, 0xB6, 0xB5, 0xB3, 0x9B, 0x99, 0x9D, 0x9D, 0x9C, 0x9E, 0x9C, 0x9B, 0x9C, 0x9D, 0x9F, 0xA0, 0x9F, 0x9F, 0x9E, 0x9F, 0x9A, 0x9B, 0x9B, 0x9C, 0xA0, 0xA5, 0xA7, 0xA8, 0xA9, 0xA0, 0x9A, 0x9C, 0x9B, 0x8D, 0x85, 0x8B }, 
	{ 0x8A, 0x7E, 0x7B, 0x77, 0x79, 0x78, 0x77, 0x74, 0x75, 0x70, 0x87, 0xAC, 0xAF, 0xAF, 0xB1, 0xB1, 0xAE, 0xAD, 0xB1, 0xB0, 0xB1, 0xB0, 0xB5, 0xB5, 0xB5, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB4, 0xB1, 0x9A, 0x9B, 0x9C, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9E, 0x9F, 0x9F, 0x9F, 0x9F, 0x9E, 0x9A, 0x9A, 0x9C, 0x9D, 0xA0, 0xA5, 0xA7, 0xA9, 0xA8, 0xA0, 0x9B, 0x9A, 0x99, 0x8D, 0x83, 0x8B }, 
	{ 0x8B, 0x7D, 0x7A, 0x77, 0x78, 0x77, 0x78, 0x76, 0x77, 0x73, 0x88, 0xA8, 0xAC, 0xAE, 0xB2, 0xB3, 0xB2, 0xB1, 0xB1, 0xB0, 0xB5, 0xB4, 0xB5, 0xB4, 0xB6, 0xB5, 0xB6, 0xB6, 0xB5, 0xB5, 0xB5, 0xB2, 0x9A, 0x9B, 0x9C, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9E, 0x9D, 0x9E, 0x9E, 0xA2, 0xA7, 0xA7, 0xA7, 0xAB, 0xA0, 0x9A, 0x9B, 0x9B, 0x8E, 0x83, 0x8B }, 
	{ 0x88, 0x78, 0x77, 0x77, 0x77, 0x76, 0x76, 0x73, 0x72, 0x74, 0x8B, 0xA8, 0xAC, 0xAE, 0xB1, 0xB0, 0xAF, 0xB3, 0xB4, 0xB1, 0xB6, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB6, 0xB6, 0xB3, 0x99, 0x9A, 0x9B, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9B, 0x9B, 0x9B, 0x9C, 0x9C, 0x9D, 0x9D, 0x9E, 0x9E, 0x9D, 0x9E, 0x9F, 0xA3, 0xA8, 0xA8, 0xA9, 0xAB, 0x9F, 0x98, 0x9A, 0x9A, 0x8D, 0x81, 0x88 }, 
	{ 0x8C, 0x7A, 0x7A, 0x7C, 0x7C, 0x7C, 0x7E, 0x7A, 0x7A, 0x7A, 0x88, 0x97, 0x97, 0x96, 0x98, 0x97, 0x99, 0xA1, 0xA5, 0xA6, 0xB1, 0xB4, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB5, 0xB3, 0x9A, 0x9A, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9C, 0x9D, 0x9D, 0xA0, 0xA1, 0xA5, 0xAA, 0xAB, 0xAD, 0xAD, 0xA3, 0x9D, 0x9E, 0x9E, 0x92, 0x87, 0x8C }, 
	{ 0x92, 0x7F, 0x7C, 0x7B, 0x7A, 0x7A, 0x7E, 0x7C, 0x7B, 0x7B, 0x82, 0x87, 0x85, 0x83, 0x85, 0x84, 0x84, 0x8A, 0x8E, 0x96, 0xAD, 0xB5, 0xB4, 0xB2, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB5, 0xB5, 0xB2, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9A, 0x9A, 0x9A, 0x9B, 0x9B, 0x9C, 0x9C, 0x9C, 0x9D, 0x9D, 0x9D, 0x9E, 0x9E, 0xA1, 0xA3, 0xA8, 0xAE, 0xAE, 0xAF, 0xB0, 0xAD, 0xA9, 0xA8, 0xA9, 0xA1, 0x97, 0x9B }, 
	{ 0xA4, 0x92, 0x8D, 0x89, 0x85, 0x82, 0x82, 0x7F, 0x80, 0x81, 0x87, 0x89, 0x8A, 0x88, 0x8A, 0x89, 0x8A, 0x8F, 0x93, 0x9B, 0xAF, 0xB6, 0xB5, 0xB4, 0xB5, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB5, 0xB3, 0x9B, 0x9A, 0x99, 0x99, 0x9A, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9C, 0x9C, 0x9D, 0x9D, 0x9D, 0x9D, 0x9E, 0x9C, 0x9F, 0xA3, 0xAA, 0xB1, 0xB1, 0xB0, 0xB0, 0xB0, 0xAE, 0xAD, 0xAF, 0xAA, 0xA3, 0xA6 }, 
	{ 0xA8, 0x9E, 0x9F, 0xA0, 0xA2, 0xA1, 0xA1, 0x9F, 0xA0, 0x9E, 0x9E, 0x99, 0x9B, 0x99, 0x9C, 0x9D, 0x9D, 0xA0, 0xA2, 0xA5, 0xB1, 0xB4, 0xB5, 0xB6, 0xB5, 0xB4, 0xB5, 0xB4, 0xB3, 0xB4, 0xB4, 0xB2, 0x99, 0x98, 0x98, 0x97, 0x98, 0x99, 0x99, 0x99, 0x9A, 0x9A, 0x9A, 0x9B, 0x9C, 0x9D, 0x9D, 0x9E, 0x9F, 0xA0, 0xA3, 0xA6, 0xAA, 0xB0, 0xB0, 0xB0, 0xAE, 0xAD, 0xAC, 0xAC, 0xAF, 0xAB, 0xA4, 0xA9 }, 
	{ 0xA5, 0x9D, 0x9F, 0xA0, 0xA4, 0xA4, 0xA4, 0xA3, 0xA3, 0xA3, 0xA3, 0xA0, 0xA4, 0xA0, 0xA1, 0xA0, 0xA2, 0xA1, 0xA1, 0xA3, 0xAF, 0xB1, 0xB3, 0xB4, 0xB4, 0xB3, 0xB4, 0xB3, 0xB2, 0xB3, 0xB3, 0xB1, 0x8F, 0x8E, 0x8D, 0x8C, 0x8D, 0x8E, 0x8E, 0x8E, 0x91, 0x92, 0x94, 0x98, 0x9C, 0xA1, 0xA5, 0xA7, 0xA6, 0xA9, 0xAE, 0xAC, 0xAB, 0xAD, 0xAD, 0xB0, 0xB1, 0xAD, 0xAB, 0xAC, 0xB0, 0xAB, 0xA4, 0xA9 }, 
	{ 0xA9, 0x9A, 0x9C, 0xA4, 0xA5, 0xA6, 0xA2, 0x9D, 0xA0, 0xA0, 0x9F, 0x9F, 0x9F, 0x9F, 0xA0, 0xA0, 0xA0, 0xA1, 0xA0, 0xA2, 0xAA, 0xB0, 0xB2, 0xB4, 0xB3, 0xB2, 0xB0, 0xAF, 0xAF, 0xB1, 0xB2, 0xB1, 0x8B, 0x8A, 0x8A, 0x8C, 0x8D, 0x8F, 0x8F, 0x8A, 0x8B, 0x8E, 0x91, 0x95, 0x9B, 0xA2, 0xA5, 0xA7, 0xA6, 0xAA, 0xAE, 0xB0, 0xB0, 0xAF, 0xB0, 0xB1, 0xB1, 0xAE, 0xAC, 0xAC, 0xAC, 0xA7, 0xA2, 0xA8 }, 
	{ 0xAA, 0x9B, 0x9D, 0xA5, 0xA6, 0xA7, 0xA4, 0xA0, 0xA2, 0xA2, 0xA2, 0xA3, 0xA3, 0xA3, 0xA3, 0xA3, 0xA2, 0xA4, 0xA4, 0xA7, 0xAE, 0xB3, 0xB5, 0xB5, 0xB4, 0xB3, 0xB2, 0xB2, 0xB2, 0xB4, 0xB3, 0xB2, 0x96, 0x94, 0x95, 0x96, 0x96, 0x98, 0x97, 0x93, 0x95, 0x97, 0x99, 0x9C, 0xA0, 0xA5, 0xA7, 0xA8, 0xA9, 0xAC, 0xB0, 0xB1, 0xB0, 0xAF, 0xAF, 0xB0, 0xB1, 0xAE, 0xAD, 0xAD, 0xAE, 0xAA, 0xA5, 0xAB }, 
	{ 0xAA, 0x9C, 0x9D, 0xA5, 0xA6, 0xA7, 0xA6, 0xA3, 0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA6, 0xA6, 0xA5, 0xA5, 0xA6, 0xA9, 0xAC, 0xAF, 0xB2, 0xB4, 0xB3, 0xB5, 0xB4, 0xB1, 0xAF, 0xB0, 0xB1, 0xB0, 0xAF, 0xA2, 0xA1, 0xA2, 0xA2, 0xA2, 0xA4, 0xA3, 0xA1, 0xA3, 0xA4, 0xA5, 0xA7, 0xA9, 0xAD, 0xAE, 0xAE, 0xAD, 0xB0, 0xB3, 0xB3, 0xB3, 0xB1, 0xB1, 0xB1, 0xB4, 0xB2, 0xB1, 0xB0, 0xB1, 0xAC, 0xA7, 0xAB }, 
	{ 0xAA, 0x9B, 0x9D, 0xA4, 0xA4, 0xA5, 0xA5, 0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA6, 0xA7, 0xA7, 0xAA, 0xAD, 0xAC, 0xAE, 0xAF, 0xAD, 0xAF, 0xAC, 0xA6, 0xA2, 0xA3, 0xA4, 0xA4, 0xA4, 0xA1, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA5, 0xA4, 0xA6, 0xA7, 0xA8, 0xA9, 0xAC, 0xAF, 0xB0, 0xB0, 0xB0, 0xB2, 0xB4, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB6, 0xB4, 0xB4, 0xB3, 0xB4, 0xAF, 0xA8, 0xAB }, 
	{ 0xAB, 0x9C, 0x9D, 0xA4, 0xA3, 0xA4, 0xA4, 0xA5, 0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xAB, 0xAD, 0xAB, 0xAB, 0xAC, 0xAB, 0xAC, 0xA9, 0xA2, 0x9C, 0x9D, 0x9F, 0x9F, 0xA0, 0xA0, 0xA1, 0xA2, 0xA3, 0xA3, 0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAB, 0xAE, 0xB0, 0xB0, 0xB0, 0xB1, 0xB3, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB5, 0xB2, 0xAB, 0xAD }, 
	{ 0xAC, 0x9C, 0x9E, 0xA4, 0xA4, 0xA3, 0xA4, 0xA5, 0xA5, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA8, 0xA8, 0xA8, 0xAB, 0xAD, 0xAC, 0xAC, 0xAD, 0xAE, 0xAE, 0xAD, 0xA6, 0xA2, 0xA4, 0xA5, 0xA4, 0xA5, 0xA6, 0xA7, 0xA7, 0xA8, 0xA9, 0xA8, 0xA8, 0xAA, 0xA9, 0xAA, 0xAB, 0xAC, 0xAE, 0xB1, 0xB2, 0xB2, 0xB2, 0xB3, 0xB4, 0xB4, 0xB4, 0xB3, 0xB3, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB6, 0xB3, 0xAC, 0xAD }, 
	{ 0xAC, 0x9C, 0x9D, 0xA5, 0xA4, 0xA3, 0xA4, 0xA5, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA8, 0xA7, 0xA9, 0xAB, 0xAC, 0xAD, 0xAC, 0xAD, 0xAF, 0xAE, 0xAE, 0xA8, 0xA3, 0xA5, 0xA6, 0xA5, 0xA6, 0xA8, 0xA8, 0xA8, 0xA9, 0xAA, 0xA9, 0xA9, 0xAB, 0xAB, 0xAB, 0xAC, 0xAC, 0xAE, 0xB1, 0xB2, 0xB2, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB6, 0xB6, 0xB4, 0xB6, 0xB4, 0xAC, 0xAC }, 
	{ 0xAC, 0x9B, 0x9C, 0xA4, 0xA4, 0xA3, 0xA4, 0xA5, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA7, 0xA9, 0xAA, 0xAB, 0xAD, 0xAB, 0xAA, 0xAE, 0xAE, 0xAE, 0xA8, 0xA2, 0xA4, 0xA5, 0xA4, 0xA6, 0xA5, 0xA6, 0xA6, 0xA7, 0xA9, 0xA9, 0xA8, 0xAC, 0xAB, 0xAC, 0xAC, 0xAD, 0xAF, 0xB1, 0xB2, 0xB2, 0xB1, 0xB3, 0xB4, 0xB4, 0xB3, 0xB3, 0xB3, 0xB4, 0xB3, 0xB4, 0xB5, 0xB3, 0xB6, 0xB5, 0xAE, 0xAE }, 
	{ 0xAC, 0x9B, 0x9F, 0xA3, 0xA5, 0xA4, 0xA4, 0xA3, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA6, 0xA7, 0xA9, 0xAB, 0xAB, 0xAA, 0xAC, 0xAE, 0xAE, 0xAD, 0xA7, 0xA4, 0xA4, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0xA7, 0xA8, 0xA9, 0xA9, 0xAA, 0xAB, 0xAA, 0xAB, 0xAC, 0xAD, 0xAF, 0xB1, 0xB3, 0xB3, 0xB2, 0xB3, 0xB5, 0xB4, 0xB2, 0xB1, 0xB2, 0xB3, 0xB5, 0xB3, 0xB5, 0xB4, 0xB4, 0xB4, 0xAF, 0xAD }, 
	{ 0xAC, 0x9A, 0x9F, 0xA2, 0xA4, 0xA4, 0xA5, 0xA5, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA8, 0xAA, 0xAB, 0xAB, 0xAA, 0xAB, 0xAD, 0xAE, 0xAD, 0xA6, 0xA3, 0xA4, 0xA4, 0xA6, 0xA5, 0xA7, 0xA7, 0xA8, 0xA9, 0xA9, 0xA9, 0xAA, 0xAB, 0xAB, 0xAC, 0xAC, 0xAD, 0xAF, 0xB2, 0xB4, 0xB4, 0xB3, 0xB4, 0xB5, 0xB5, 0xB3, 0xB2, 0xB3, 0xB3, 0xB3, 0xB2, 0xB5, 0xB5, 0xB5, 0xB5, 0xB1, 0xAF }, 
	{ 0xB5, 0xA3, 0xA5, 0xA7, 0xA7, 0xA6, 0xA6, 0xA6, 0xA7, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA7, 0xA7, 0xA6, 0xA7, 0xA9, 0xAA, 0xAA, 0xA9, 0xAA, 0xAC, 0xAD, 0xAC, 0xA5, 0xA2, 0xA4, 0xA4, 0xA5, 0xA4, 0xA5, 0xA5, 0xA6, 0xA8, 0xA9, 0xA9, 0xAA, 0xAB, 0xAB, 0xAC, 0xAD, 0xAE, 0xB0, 0xB3, 0xB5, 0xB4, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB3, 0xB3, 0xB3, 0xB3, 0xB2, 0xB4, 0xB5, 0xB5, 0xB6, 0xB3, 0xB3 }, 
	{ 0xB5, 0xA2, 0xA4, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA6, 0xA7, 0xA7, 0xA8, 0xA8, 0xA7, 0xA6, 0xA6, 0xA3, 0xA5, 0xA7, 0xA9, 0xA9, 0xA9, 0xAA, 0xAB, 0xAC, 0xAA, 0xA3, 0xA1, 0xA3, 0xA3, 0xA4, 0xA2, 0xA0, 0xA1, 0xA3, 0xA6, 0xA8, 0xA9, 0xAA, 0xAC, 0xAA, 0xAC, 0xAD, 0xAF, 0xB2, 0xB4, 0xB4, 0xB3, 0xB1, 0xB2, 0xB3, 0xB4, 0xB3, 0xB2, 0xB2, 0xB2, 0xB4, 0xB3, 0xB4, 0xB4, 0xB5, 0xB6, 0xB4, 0xB6 }, 
	{ 0xB2, 0x9F, 0xA1, 0xA3, 0xA5, 0xA5, 0xA6, 0xA6, 0xA6, 0xA7, 0xA7, 0xA8, 0xA8, 0xA7, 0xA7, 0xA6, 0xA3, 0xA5, 0xA7, 0xA8, 0xA9, 0xAA, 0xAA, 0xAA, 0xAB, 0xAA, 0xA2, 0xA1, 0xA3, 0xA3, 0xA3, 0xA0, 0x9E, 0x9F, 0xA2, 0xA5, 0xA7, 0xA8, 0xA9, 0xAB, 0xAA, 0xAB, 0xAD, 0xAF, 0xB1, 0xB3, 0xB3, 0xB2, 0xAF, 0xB0, 0xB2, 0xB2, 0xB3, 0xB2, 0xB2, 0xB1, 0xB3, 0xB3, 0xB5, 0xB5, 0xB6, 0xB5, 0xB2, 0xB6 }, 
	{ 0xB4, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA7, 0xA7, 0xA7, 0xA7, 0xA4, 0xA6, 0xA8, 0xA9, 0xAA, 0xAA, 0xAA, 0xAA, 0xAB, 0xAA, 0xA2, 0xA0, 0xA2, 0xA2, 0xA2, 0xA0, 0x9F, 0xA0, 0xA2, 0xA4, 0xA6, 0xA6, 0xA7, 0xA9, 0xA9, 0xAB, 0xAC, 0xAD, 0xAF, 0xB2, 0xB3, 0xB4, 0xB0, 0xB1, 0xB2, 0xB2, 0xB3, 0xB2, 0xB2, 0xB2, 0xB1, 0xB3, 0xB5, 0xB5, 0xB6, 0xB4, 0xB1, 0xB6 }, 
	{ 0xB5, 0xA0, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA4, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA7, 0xA7, 0xA6, 0xA3, 0xA6, 0xA8, 0xA8, 0xA9, 0xAA, 0xAA, 0xA9, 0xAB, 0xA9, 0xA1, 0x9F, 0xA0, 0xA0, 0xA1, 0x9F, 0xA0, 0xA0, 0xA1, 0xA3, 0xA4, 0xA4, 0xA5, 0xA6, 0xA7, 0xAA, 0xAC, 0xAD, 0xAF, 0xB2, 0xB4, 0xB4, 0xB0, 0xB1, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB0, 0xB2, 0xB4, 0xB3, 0xB4, 0xB3, 0xB1, 0xB8 }, 
	{ 0xBB, 0xA6, 0xA3, 0xA2, 0xA1, 0xA0, 0xA0, 0xA0, 0xA0, 0xA2, 0xA5, 0xA8, 0xA8, 0xA7, 0xA5, 0xA4, 0xA1, 0xA4, 0xA6, 0xA7, 0xA8, 0xAA, 0xAA, 0xA9, 0xAA, 0xA8, 0xA0, 0x9C, 0x9E, 0x9E, 0x9F, 0x9E, 0x9F, 0x9F, 0xA0, 0xA1, 0xA2, 0xA2, 0xA3, 0xA4, 0xA4, 0xA8, 0xAD, 0xAF, 0xB1, 0xB3, 0xB3, 0xB3, 0xB1, 0xB1, 0xB2, 0xB2, 0xB1, 0xB0, 0xB1, 0xB1, 0xB1, 0xB2, 0xB1, 0xB0, 0xB1, 0xB1, 0xB2, 0xBB }, 
	{ 0xB7, 0xAC, 0xA8, 0xA8, 0xA7, 0xA6, 0xA4, 0xA5, 0xA4, 0xA1, 0xA8, 0xAC, 0xAD, 0xA3, 0x86, 0x88, 0x83, 0x8B, 0x8F, 0x91, 0x96, 0x98, 0x99, 0x9D, 0x9D, 0x9E, 0x9F, 0x9F, 0x9D, 0x9C, 0x9D, 0x9F, 0x9F, 0xA0, 0xA1, 0xA1, 0xA2, 0xA3, 0xA3, 0xA3, 0xA4, 0xA8, 0xAD, 0xB0, 0xB1, 0xB3, 0xB5, 0xB6, 0xB3, 0xB2, 0xB2, 0xB1, 0xB1, 0xB2, 0xB2, 0xB3, 0xB1, 0xB1, 0xB3, 0xB1, 0xB0, 0xB0, 0xB2, 0xBA }, 
	{ 0xB5, 0xAE, 0xAD, 0xAE, 0xAD, 0xAD, 0xAC, 0xAD, 0xAC, 0xAC, 0xB1, 0xB0, 0xAF, 0xA6, 0x88, 0x83, 0x81, 0x8B, 0x91, 0x93, 0x98, 0x9B, 0x9D, 0xA0, 0x9F, 0xA0, 0xA1, 0xA1, 0xA0, 0x9F, 0xA0, 0xA1, 0xA2, 0xA2, 0xA3, 0xA4, 0xA4, 0xA5, 0xA5, 0xA5, 0xA6, 0xAA, 0xAE, 0xB0, 0xB1, 0xB3, 0xB5, 0xB6, 0xB3, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB4, 0xB2, 0xAF, 0xAE, 0xAE, 0xB5 }, 
	{ 0xB2, 0xAE, 0xAF, 0xAF, 0xAE, 0xAF, 0xAF, 0xB0, 0xB1, 0xB2, 0xB7, 0xB5, 0xB5, 0xB1, 0x91, 0x84, 0x85, 0x90, 0x96, 0x98, 0x9B, 0x9E, 0x9E, 0xA0, 0xA1, 0xA2, 0xA4, 0xA4, 0xA4, 0xA3, 0xA3, 0xA3, 0xA3, 0xA2, 0xA2, 0xA2, 0xA3, 0xA4, 0xA5, 0xA5, 0xA7, 0xAA, 0xAE, 0xB0, 0xB2, 0xB4, 0xB6, 0xB6, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB2, 0xB1, 0xAF, 0xAE, 0xB4 }, 
	{ 0xB1, 0xAE, 0xAE, 0xAC, 0xAB, 0xB0, 0xB2, 0xB3, 0xB5, 0xB5, 0xB6, 0xB5, 0xB6, 0xB2, 0x93, 0x81, 0x82, 0x8D, 0x95, 0x97, 0x9A, 0x9E, 0xA0, 0xA1, 0xA1, 0xA2, 0xA3, 0xA3, 0xA3, 0xA3, 0xA2, 0xA1, 0xA2, 0xA2, 0xA1, 0xA2, 0xA4, 0xA6, 0xA9, 0xAB, 0xAD, 0xAF, 0xB1, 0xB2, 0xB3, 0xB5, 0xB6, 0xB5, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB2, 0xB2, 0xB4, 0xB4, 0xB3, 0xB2, 0xB0, 0xB5 }, 
	{ 0xB2, 0xAE, 0xAD, 0xAB, 0xAD, 0xB3, 0xB6, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB5, 0xB2, 0x9C, 0x8E, 0x8D, 0x96, 0x9C, 0x9D, 0x9F, 0xA2, 0xA4, 0xA5, 0xA7, 0xA7, 0xA8, 0xA9, 0xA9, 0xA8, 0xA6, 0xA5, 0xA6, 0xA6, 0xA6, 0xA7, 0xA9, 0xAE, 0xB2, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB5, 0xB6, 0xB6, 0xB5, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB3, 0xB5, 0xB4, 0xB4, 0xB3, 0xB0, 0xB4 }, 
	{ 0xB2, 0xAD, 0xAD, 0xAC, 0xAE, 0xB3, 0xB3, 0xB3, 0xB2, 0xB3, 0xB5, 0xB8, 0xB6, 0xB7, 0xB1, 0xAD, 0xAE, 0xB2, 0xB4, 0xB2, 0xB0, 0xB1, 0xB1, 0xB1, 0xB1, 0xB1, 0xB2, 0xB3, 0xB2, 0xB1, 0xB0, 0xAE, 0xAD, 0xAC, 0xAB, 0xAB, 0xAC, 0xAF, 0xB1, 0xB3, 0xB3, 0xB5, 0xB6, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB2, 0xB2, 0xB2, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB3, 0xB0, 0xB4 }, 
	{ 0xB3, 0xAF, 0xB0, 0xAF, 0xAF, 0xB0, 0xAF, 0xAF, 0xB0, 0xB1, 0xB3, 0xB7, 0xB4, 0xB7, 0xB7, 0xB7, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB3, 0xB2, 0xB1, 0xB0, 0xAF, 0xAE, 0xAE, 0xAE, 0xAF, 0xAF, 0xAF, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB2, 0xB2, 0xB2, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB2, 0xB4, 0xB4, 0xB5, 0xB4, 0xB3, 0xB7 }, 
	{ 0xB7, 0xB3, 0xB5, 0xB4, 0xB3, 0xB4, 0xB3, 0xB4, 0xB3, 0xB4, 0xB4, 0xB8, 0xB4, 0xB6, 0xB6, 0xB3, 0xB2, 0xB0, 0xB0, 0xB1, 0xB2, 0xB3, 0xB5, 0xB6, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB5, 0xB4, 0xB4, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB1, 0xB4, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB2, 0xB2, 0xB1, 0xB1, 0xB1, 0xB2, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB5, 0xB4, 0xB9 }, 
	{ 0xB6, 0xB3, 0xB3, 0xB4, 0xB3, 0xB3, 0xB4, 0xB3, 0xB3, 0xB4, 0xB5, 0xB6, 0xB5, 0xB5, 0xB6, 0xB6, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB7, 0xB7, 0xB8, 0xB7, 0xB6, 0xB4, 0xB4, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB3, 0xB3, 0xB3, 0xB4, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB4, 0xB7, 0xB1, 0xB1, 0xB1, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB7 }, 
	{ 0xB6, 0xB3, 0xB3, 0xB4, 0xB3, 0xB3, 0xB4, 0xB3, 0xB3, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB6, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB5, 0xB4, 0xB2, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB5, 0xB4, 0xB3, 0xB2, 0xB4, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB5, 0xB5, 0xB6, 0xB4, 0xB4, 0xB7 }, 
	{ 0xB6, 0xB3, 0xB3, 0xB4, 0xB3, 0xB3, 0xB4, 0xB4, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB2, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB3, 0xB2, 0xB2, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB6, 0xB5, 0xB5, 0xB8 }, 
	{ 0xB6, 0xB3, 0xB3, 0xB4, 0xB3, 0xB4, 0xB5, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB8, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB2, 0xB3, 0xB2, 0xB2, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB6, 0xB9 }, 
	{ 0xB7, 0xB4, 0xB3, 0xB4, 0xB3, 0xB4, 0xB5, 0xB4, 0xB4, 0xB5, 0xB6, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB8, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB8, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB1, 0xB2, 0xB2, 0xB1, 0xB4, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB9 }, 
	{ 0xB7, 0xB4, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB5, 0xB6, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB6, 0xB2, 0xB3, 0xB3, 0xB2, 0xB5, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xBA }, 
	{ 0xB8, 0xB4, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB5, 0xB5, 0xB4, 0xB4, 0xB5, 0xB4, 0xB3, 0xB2, 0xB4, 0xB4, 0xAD, 0xAC, 0xAD, 0xAE, 0xB5, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB6, 0xB6, 0xB7, 0xBA }, 
	{ 0xB8, 0xB5, 0xB4, 0xB5, 0xB4, 0xB4, 0xB4, 0xB2, 0xB3, 0xB4, 0xB5, 0xB5, 0xB4, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB5, 0xB4, 0xB4, 0xB4, 0xB3, 0xB2, 0xB0, 0xB0, 0xAF, 0xA4, 0xA2, 0xA3, 0xA7, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB5, 0xB6, 0xB6, 0xB7, 0xBA }, 
	{ 0xBA, 0xB5, 0xB5, 0xB6, 0xB5, 0xB6, 0xB4, 0xAE, 0xB0, 0xB1, 0xB2, 0xB3, 0xB3, 0xB3, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB5, 0xB5, 0xB4, 0xB4, 0xB3, 0xAF, 0xAA, 0xAB, 0xAC, 0x9C, 0x99, 0x9B, 0xA2, 0xB3, 0xB2, 0xB4, 0xB4, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB5, 0xB7, 0xB7, 0xB7, 0xB6, 0xB4, 0xBB }, 
	{ 0xB8, 0xB5, 0xB5, 0xB6, 0xB5, 0xB5, 0xB4, 0xAE, 0xB0, 0xB1, 0xB2, 0xB3, 0xB3, 0xB4, 0xB4, 0xB5, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB8, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB5, 0xB5, 0xB3, 0xB0, 0xAC, 0xAC, 0xAD, 0x9C, 0x98, 0x9A, 0xA1, 0xB3, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB7, 0xB5, 0xB6, 0xB5, 0xB6, 0xB7, 0xB6, 0xBB }, 
	{ 0xB9, 0xB5, 0xB5, 0xB6, 0xB5, 0xB6, 0xB6, 0xB2, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 0xB5, 0xB5, 0xB4, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB4, 0xB4, 0xB4, 0xB3, 0xB3, 0xB2, 0xB0, 0xAD, 0xAF, 0xAF, 0xA0, 0x9D, 0x9E, 0xA2, 0xB2, 0xB3, 0xB4, 0xB5, 0xB5, 0xB6, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB6, 0xB6, 0xB4, 0xB5, 0xB7, 0xB6, 0xB8 }, 
	{ 0xB1, 0xAD, 0xAD, 0xAF, 0xAF, 0xB2, 0xB6, 0xB5, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB5, 0xB5, 0xB4, 0xB3, 0xB2, 0xB1, 0xB0, 0xA3, 0x9F, 0x9E, 0xA1, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB2, 0xB3 }, 
	{ 0xB1, 0xAC, 0xAD, 0xAE, 0xAD, 0xB0, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB5, 0xB6, 0xB6, 0xB7, 0xB7, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB5, 0xB6, 0xB6, 0xB5, 0xB4, 0xB4, 0xB3, 0xB2, 0xB2, 0xB4, 0xB1, 0xA3, 0x9F, 0x9E, 0xA1, 0xAE, 0xB1, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB3, 0xB4, 0xB4 }, 
	{ 0xB0, 0xAC, 0xAD, 0xAF, 0xAF, 0xB0, 0xB4, 0xB4, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB5, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB5, 0xB4, 0xB2, 0xB1, 0xB0, 0xAF, 0xAE, 0xAD, 0xAD, 0xAC, 0xAA, 0xA1, 0x9F, 0x9F, 0xA0, 0xA9, 0xAB, 0xAB, 0xAB, 0xAC, 0xAF, 0xB1, 0xB3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB3, 0xB4, 0xB4, 0xB3, 0xB4, 0xB1 }, 
	{ 0x9C, 0x98, 0x9B, 0xA1, 0xA4, 0xA9, 0xB0, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB5, 0xB5, 0xB5, 0xB6, 0xB8, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xA4, 0xA3, 0xA1, 0xA2, 0xA1, 0xA0, 0xA0, 0xA0, 0xA0, 0xA0, 0x9C, 0x9E, 0x9E, 0x9C, 0xA0, 0x9F, 0xA0, 0xA0, 0xA1, 0xA5, 0xA9, 0xAD, 0xAD, 0xAD, 0xAC, 0xAE, 0xAD, 0xAD, 0xAD, 0xAE, 0xAB, 0xA0 }, 
	{ 0x8D, 0x88, 0x8A, 0x91, 0x95, 0x9D, 0xA7, 0xAB, 0xAF, 0xB1, 0xB3, 0xB4, 0xB4, 0xB5, 0xB6, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB6, 0xB6, 0xB6, 0xA0, 0x9E, 0x9D, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0xA0, 0x9F, 0x9C, 0x9F, 0x9F, 0x9C, 0x9E, 0x9D, 0x9E, 0x9D, 0x9E, 0xA1, 0xA6, 0xA8, 0xA8, 0xA7, 0xA8, 0xAD, 0xAC, 0xAB, 0xAB, 0xAD, 0xA5, 0x92 }, 
	{ 0x8E, 0x87, 0x8A, 0x91, 0x93, 0x9B, 0xA7, 0xAC, 0xAE, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB6, 0xB6, 0xB8, 0xB6, 0xB5, 0xB6, 0xB3, 0xB3, 0xB8, 0xB7, 0xB7, 0xB8, 0xB7, 0xB6, 0xB6, 0xB7, 0xB5, 0xA2, 0x9F, 0x9F, 0xA0, 0x9F, 0x9F, 0x9F, 0x9F, 0xA0, 0x9F, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9D, 0x9E, 0x9C, 0x9E, 0xA3, 0xA7, 0xA8, 0xA8, 0xA7, 0xA9, 0xAD, 0xAC, 0xAB, 0xAA, 0xAB, 0xA8, 0x94 }, 
	{ 0x8D, 0x87, 0x89, 0x8E, 0x90, 0x97, 0xA2, 0xA6, 0xAC, 0xAD, 0xAF, 0xAF, 0xB0, 0xB3, 0xB5, 0xB6, 0xB4, 0xB7, 0xB7, 0xB6, 0xB5, 0xB3, 0xB3, 0xB6, 0xB7, 0xB7, 0xB7, 0xB7, 0xB6, 0xB7, 0xB7, 0xB5, 0xA0, 0x9E, 0x9E, 0x9F, 0x9E, 0x9E, 0x9F, 0x9F, 0x9F, 0x9E, 0x9D, 0x9C, 0x9D, 0x9D, 0x9D, 0x9C, 0x9D, 0x9C, 0x9D, 0xA0, 0xA4, 0xA6, 0xA8, 0xA9, 0xA5, 0xAA, 0xAC, 0xAC, 0xAA, 0xA8, 0x9F, 0x8C }, 
	{ 0x8C, 0x86, 0x88, 0x8D, 0x8F, 0x95, 0x9E, 0xA2, 0xA5, 0xA7, 0xA8, 0xA8, 0xA8, 0xA9, 0xAB, 0xAC, 0xAF, 0xB2, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB3, 0xB5, 0xB4, 0xB5, 0xB5, 0xB4, 0xB5, 0xB6, 0xB3, 0x9E, 0x9C, 0x9E, 0xA0, 0x9F, 0xA0, 0xA0, 0xA0, 0xA0, 0x9F, 0x9E, 0x9E, 0x9E, 0x9E, 0x9E, 0x9D, 0x9C, 0x9C, 0x9D, 0x9F, 0xA1, 0xA3, 0xA5, 0xA6, 0xAA, 0xAA, 0xAB, 0xA8, 0xA7, 0xA9, 0x9D, 0x8C }, 
	{ 0x87, 0x82, 0x85, 0x8B, 0x8E, 0x95, 0x9D, 0xA1, 0xA5, 0xA7, 0xA9, 0xA9, 0xA9, 0xA9, 0xAA, 0xAA, 0xA6, 0xA8, 0xAA, 0xAC, 0xAF, 0xB2, 0xB3, 0xB1, 0xB3, 0xB3, 0xB4, 0xB4, 0xB4, 0xB5, 0xB5, 0xB2, 0x99, 0x98, 0x9B, 0x9E, 0x9E, 0x9E, 0x9F, 0x9E, 0xA0, 0x9E, 0x9D, 0x9D, 0x9D, 0x9D, 0x9C, 0x9C, 0x99, 0x9A, 0x9C, 0x9E, 0xA1, 0xA4, 0xA6, 0xA5, 0xA5, 0xA6, 0xAB, 0xA6, 0xA8, 0xAC, 0x98, 0x86 }, 
	{ 0x84, 0x80, 0x83, 0x8A, 0x8E, 0x94, 0x9C, 0x9F, 0xA3, 0xA6, 0xA8, 0xA8, 0xA9, 0xAA, 0xAC, 0xAC, 0xA8, 0xAC, 0xAD, 0xAD, 0xB0, 0xB2, 0xB3, 0xB3, 0xB3, 0xB3, 0xB4, 0xB5, 0xB5, 0xB5, 0xB4, 0xB1, 0x97, 0x97, 0x9A, 0x9D, 0x9E, 0x9E, 0x9E, 0x9D, 0x9F, 0x9E, 0x9C, 0x9C, 0x9C, 0x9D, 0x9C, 0x9B, 0x9A, 0x9A, 0x9B, 0x9D, 0xA1, 0xA5, 0xA6, 0xA6, 0xA3, 0xA6, 0xAC, 0xA3, 0xA4, 0xA9, 0x97, 0x89 }, 
	{ 0x85, 0x80, 0x81, 0x86, 0x88, 0x8A, 0x8E, 0x91, 0x91, 0x93, 0x94, 0x94, 0x93, 0x95, 0x97, 0x99, 0x9C, 0xA4, 0xA9, 0xAD, 0xB3, 0xB3, 0xB1, 0xB2, 0xB1, 0xB1, 0xB3, 0xB4, 0xB3, 0xB4, 0xB3, 0xAF, 0x97, 0x96, 0x99, 0x9C, 0x9C, 0x9C, 0x9C, 0x9B, 0x9C, 0x9B, 0x9B, 0x9B, 0x9B, 0x9B, 0x9A, 0x9A, 0x97, 0x96, 0x96, 0x99, 0x9E, 0xA1, 0xA4, 0xA6, 0xA6, 0xA7, 0xA4, 0x93, 0x8E, 0x91, 0x89, 0x87 }, 
	{ 0x85, 0x80, 0x7F, 0x82, 0x81, 0x81, 0x82, 0x83, 0x83, 0x85, 0x87, 0x86, 0x85, 0x85, 0x86, 0x87, 0x84, 0x8B, 0x91, 0x9D, 0xAF, 0xB3, 0xB0, 0xB1, 0xB2, 0xB2, 0xB3, 0xB3, 0xB2, 0xB3, 0xB2, 0xB0, 0x98, 0x96, 0x97, 0x99, 0x99, 0x98, 0x98, 0x97, 0x98, 0x99, 0x9A, 0x9A, 0x9A, 0x99, 0x98, 0x98, 0x95, 0x93, 0x91, 0x91, 0x8E, 0x8A, 0x89, 0x8B, 0x8D, 0x8E, 0x8B, 0x83, 0x81, 0x82, 0x80, 0x85 }, 
	{ 0x8A, 0x85, 0x85, 0x87, 0x87, 0x86, 0x87, 0x88, 0x87, 0x8B, 0x8E, 0x8F, 0x8E, 0x8E, 0x8D, 0x8D, 0x89, 0x89, 0x86, 0x94, 0xAF, 0xB9, 0xB5, 0xB7, 0xB5, 0xB5, 0xB6, 0xB5, 0xB4, 0xB4, 0xB5, 0xB3, 0x9E, 0x9B, 0x9B, 0x9C, 0x9B, 0x9A, 0x9A, 0x99, 0x9B, 0x9D, 0x9F, 0x9F, 0x9F, 0x9E, 0x9D, 0x9D, 0x99, 0x98, 0x98, 0x98, 0x92, 0x8A, 0x87, 0x8A, 0x88, 0x87, 0x85, 0x85, 0x88, 0x86, 0x85, 0x8D },
};

struct CheesyRand
{
	CheesyRand( int seed = 12345 )
	{
		z = seed | 0x00010000;
		w = ~seed | 0x00000100;
	}

	uint32 RandInt()
	{
		// http://en.wikipedia.org/wiki/Random_number_generation#Computational_methods
		z = 36969 * (z & 65535) + (z >> 16);
		w = 18000 * (w & 65535) + (w >> 16);
		return (z << 16) + w;
    }

	inline float RandFloat01()
	{
		return RandInt() / 4294970000.0f;
	}

	inline float RandFloatNeg1To1()
	{
		return RandFloat01() * 2.0f - 1.0f;
	}

	uint32 z, w;
};

void CConfirmCustomizeTextureDialog::PerformPainterlyFilter()
{

	// Resample it to 2x the final resolution.  Having a fixed resolution
	// for the "source" image makes it easier, since we can use fixed size
	// kernels, etc
	Bitmap_t imageTemp1, imageTemp2;
	ImgUtl_ResizeBitmap( imageTemp1, k_nCustomImageSize*2, k_nCustomImageSize*2, &m_imgSquare );

	//
	// Shape correction V1:
	//
	#if 1
		// Perform symmetric nearest neighbor
		float filterStrength = .95f;
		SymmetricNearestNeighborFilter( imageTemp1, imageTemp2, 5, filterStrength );
		BilateralFilter( imageTemp2, imageTemp1, 4, .5, .15 );
		BilateralFilter( imageTemp1, imageTemp2, 2, .9, .7 );
		imageTemp1.SetPixelData( imageTemp2 );
	#endif

	//
	// Shape correction V2:
	//
	#if 0
		// Perform symmetric nearest neighbor
		float snnFilterStrength = .7f;
		SymmetricNearestNeighborFilter( imageTemp1, imageTemp2, 4, snnFilterStrength );

		// And some bilateral filtering to smooth it
		BilateralFilter( imageTemp2, imageTemp1, 2, .75, .5 );
		BilateralFilter( imageTemp1, imageTemp2, 3, .7, .3 );
		BilateralFilter( imageTemp2, imageTemp1, 4, .6, .2 );
	#endif

//	// Load up brush strokes
//	if ( !m_imgBrushStrokes.IsValid() )
//	{
//		m_imgBrushStrokes.Load( "d:\\texture.jpg" );
//
//		for (int y = 0 ; y < m_imgBrushStrokes.Height() ; ++y )
//		{
//			for (int x = 0 ; x < m_imgBrushStrokes.Width() ; ++x )
//			{
//				Warning("0x%02X, ", m_imgBrushStrokes.GetColor(x,y).r() );
//			}
//			Warning("\n");
//		}
//		
//		m_imgBrushStrokes.Resize( m_imgTemp.Width(), m_imgTemp.Height() );
//	}

	//
	// Color correction
	//
	for ( int y = 0 ; y < imageTemp1.Height() ; ++y )
	{
		for ( int x = 0 ; x < imageTemp1.Width() ; ++x )
		{

			// Fetch original pixel in RGB space
			Color c = imageTemp1.GetColor( x,y );
			Vector rgb((float)c.r(), (float)c.g(), (float)c.b());

			// Convert to HSV
			Vector hsv;
			RGBtoHSV( rgb, hsv );

			//
			// Color correction V1
			//
			#if 0
				// Shift towards red, away from blue
				//rgb.x += rgb.z * .2f;
				//rgb.z *= 0.7f;
				// Desaturate
				float satMult = .65; // desaturate
				hsv.y *= satMult;
			#endif

			//
			// Color correction V2
			//

			#if 1
				static const Color swatches[] = 
				{
					Color( 183, 224, 252, 255 ), // sky light
					Color( 83, 109, 205, 255 ), // sky med
					Color( 64, 68, 195, 255 ), // sky dark
					Color( 100, 68, 57, 255 ), // skin demo
					Color( 139, 101, 84, 255 ), // skin demo light
					Color( 133, 105, 68, 255 ), // saxton hair
					Color( 252, 169, 131, 255 ), // skin light
					Color( 194, 132, 106, 255 ), // skin
	
					Color( 255, 255, 255, 255 ),
					Color( 246, 231, 222, 255 ),
					Color( 218, 189, 171, 255 ),
					Color( 193, 161, 138, 255 ),
	
					Color( 248, 185, 138, 255 ),
					Color( 245, 173, 135, 255 ),
					Color( 239, 152,  73, 255 ),
					Color( 241, 129,  73, 255 ),
	
					Color( 106,  69,  52, 255 ),
					Color( 145,  58,  31, 255 ),
					Color( 189,  58,  58, 255 ),
					Color( 157,  48,  47, 255 ),
					Color(  69,  44,  37, 255 ),
	
					Color( 107, 106, 101, 255 ),
					Color( 118, 138, 136, 255 ),
					Color(  91, 122, 140, 255 ),
					Color(  56,  92, 120, 255 ),
					Color(  52,  47,  44, 255 ),
				};
	
				static float selfWeight = .15f;
				static float thresh = .60f;
				Vector rgb2((float)c.r(), (float)c.g(), (float)c.b());
				float totalWeight = selfWeight;
				rgb2 *= selfWeight;
				for ( int i = 0 ; i < ARRAYSIZE(swatches) ; ++i )
				{
					float similarity = 1.0f - ApproxColorDist( c, swatches[i] ) - thresh;
					if ( similarity > 0.0f )
					{
						similarity /= (1.0f - thresh); // get in 0...1 scale
						similarity *= similarity*similarity;
						rgb2.x += similarity*(float)swatches[i].r();
						rgb2.y += similarity*(float)swatches[i].g();
						rgb2.z += similarity*(float)swatches[i].b();
						totalWeight += similarity;
					}
				}
				rgb2 /= totalWeight;
	
				// Calc hue for the shifted one
				Vector hsv2;
				RGBtoHSV( rgb2, hsv2 );
	
				// Replace hue and saturation
				hsv.x = hsv2.x;
				hsv.y = hsv2.y;
			#endif
			// Convert back to RGB space
			HSVtoRGB( hsv, rgb );

			// Overlay brush stroke noise
			Vector overlayValue;
			int brushX = x * k_BrushStrokeSize / imageTemp1.Width();
			int brushY = y * k_BrushStrokeSize / imageTemp1.Height();
			//float k = (float)m_imgBrushStrokes.GetColor( x, y ).r() / 255.0f;
			float k = (float)s_bBrushStrokeData[brushY][brushX] / 255.0f;
			if ( k < .5f )
			{
				overlayValue = rgb * k * 2.0f;
			}
			else
			{
				Vector kWhite( 255.0f, 255.0f, 255.0f );
				float q = 2.0f * ( 1.0f - k ); // 0.5 -> 1.0  ,  1.0 -> 0
				overlayValue = kWhite - ( kWhite - rgb ) * q;
			}

			float overlayStrength = .10f;
			rgb += (overlayValue - rgb) * overlayStrength;

			// Put back into the image
			Color result(
				(unsigned char)clamp(rgb.x, 0.0f, 255.0f),
				(unsigned char)clamp(rgb.y, 0.0f, 255.0f),
				(unsigned char)clamp(rgb.z, 0.0f, 255.0f),
				c.a()
			);
			imageTemp2.SetColor( x, y, result );
		}
	}

	// Now downsample to the final size
	ImgUtl_ResizeBitmap( m_imgFinal, k_nCustomImageSize, k_nCustomImageSize, &imageTemp2 );

	// Add noise to the final image
	// Use deterministic random number generator (i.e. let's not call rand()),
	// so uploading the same image twice will produce the same hash
	CheesyRand noiseRand;
	for ( int y = 0 ; y < m_imgFinal.Height() ; ++y )
	{
		for ( int x = 0 ; x < m_imgFinal.Width() ; ++x )
		{
			float noiseStrength = 2.0f;
			int noise = (int)floor( noiseRand.RandFloatNeg1To1() * noiseStrength + .5f );
			Color c = m_imgFinal.GetColor( x, y );
			Color result(
				clamp( c.r() + noise, 0, 255 ),
				clamp( c.g() + noise, 0, 255 ),
				clamp( c.b() + noise, 0, 255 ),
				c.a()
			);
			m_imgFinal.SetColor( x, y, result );
		}
	}
}

#ifdef TEST_FILTERS
void CConfirmCustomizeTextureDialog::TestFilters()
{
	const char *szTestImageFilenames[] =
	{
		"d:/custom_images/borat.jpg",
		"d:/custom_images/cloud_strife-profile.jpg",
		"d:/custom_images/ladies_man.png",
		"d:/custom_images/dota_hero.jpg",
		"d:/custom_images/elmo balls.jpg",
		"d:/custom_images/halolz-dot-com-teamfortress2-sexyheavy-prematureubers.jpg",
		"d:/custom_images/doug_loves_movies.jpg",
		"d:/custom_images/lolcat.jpg",
		"d:/custom_images/mario_3d.jpg",
		//"d:/custom_images/pulp_fiction_sam.gif",
		"d:/custom_images/RainbowBright.jpg",
		"d:/custom_images/elliot_and_travis.tga",
		"d:/custom_images/give_peace_a_chance.jpg",
	};
	const int k_nTestImages = ARRAYSIZE(szTestImageFilenames);

	Bitmap_t imageOutput;
	imageOutput.Init( k_nCustomImageSize*3, k_nCustomImageSize*k_nTestImages, IMAGE_FORMAT_RGBA8888 );

	for ( int i = 0 ; i < k_nTestImages ; ++i )
	{
		ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( szTestImageFilenames[i], m_imgSource );
		if ( nErrorCode != CE_SUCCESS )
		{
			Assert( nErrorCode == CE_SUCCESS );
			continue;
		}

		PerformSquarize();
		PerformPainterlyFilter();
		int y = i*k_nCustomImageSize;
		imageOutput.SetPixelData( m_imgSquareDisplay, 0, y );
		imageOutput.SetPixelData( m_imgFinal, k_nCustomImageSize, y );
	}

	CUtlBuffer pngFileData;
	ImgUtl_SavePNGBitmapToBuffer( pngFileData, imageOutput );
	
	g_pFullFileSystem->WriteFile( "d:/painterly.png", NULL, pngFileData );
}
#endif

void CConfirmCustomizeTextureDialog::RegenerateTextureBits( ITexture *pTexture, IVTFTexture *pVTFTexture, Rect_t *pRect )
{

	// Check if we need to redo the filter
	CleanFilteredImage();

	Assert( pVTFTexture->FrameCount() == 1 );
	Assert( pVTFTexture->FaceCount() == 1 );
	Assert( pTexture == g_pPreviewCustomTexture );
	Assert( !pTexture->IsMipmapped() );

	int nWidth, nHeight, nDepth;
	pVTFTexture->ComputeMipLevelDimensions( 0, &nWidth, &nHeight, &nDepth );
	Assert( nDepth == 1 );
	Assert( nWidth == m_imgFinal.Width() && nHeight == m_imgFinal.Height() );

	CPixelWriter pixelWriter;
	pixelWriter.SetPixelMemory( pVTFTexture->Format(), 
		pVTFTexture->ImageData( 0, 0, 0 ), pVTFTexture->RowSizeInBytes( 0 ) );

	// !SPEED! 'Tis probably DEATHLY slow...
	for ( int y = 0; y < nHeight; ++y )
	{
		pixelWriter.Seek( 0, y );
		for ( int x = 0; x < nWidth; ++x )
		{
			Color c = m_imgFinal.GetColor( x, y );
			pixelWriter.WritePixel( c.r(), c.g(), c.b(), c.a() );
		}
	}

	// We're no longer dirty
	g_pPreviewCustomTextureDirty = false;
}

void CConfirmCustomizeTextureDialog::Release()
{
	if ( g_pPreviewCustomTexture )
	{
		ITexture *tex = g_pPreviewCustomTexture;
		g_pPreviewCustomTexture = NULL; // clear pointer first, to prevent infinite recursion
		tex->SetTextureRegenerator( NULL );
		tex->Release();
	}
	g_pPreviewEconItem = NULL;
}

class CCustomizeTextureJobDialog : public CApplyCustomTextureJob
{
public:
	CCustomizeTextureJobDialog( const void *pPNGData, int nPNGDataBytes, CConfirmCustomizeTextureDialog *pDlg )
	: CApplyCustomTextureJob( pDlg->GetToolItem()->GetItemID(), pDlg->GetSubjectItem()->GetItemID(), pPNGData, nPNGDataBytes )
	, m_pDlg( pDlg )
	{
	}

protected:

	virtual EResult YieldingRunJob()
	{
		// Base class do the work
		EResult result = CApplyCustomTextureJob::YieldingRunJob();

		CloseWaitingDialog();

		// Show result
		if ( result == k_EResultOK )
		{
			m_pDlg->OnCommand("close");
		}
		else
		{
			m_pDlg->CloseWithGenericError();
		}

		// Return status code
		return result;
	}

	CConfirmCustomizeTextureDialog *m_pDlg;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmCustomizeTextureDialog::Apply( void )
{
	Assert( m_imgFinal.IsValid() );

	// Throw up a busy dialog
	SetPage( ePage_PerformingAction );

	// Write PNG data
	CUtlBuffer bufPNGData;
	if ( ImgUtl_SavePNGBitmapToBuffer( bufPNGData, m_imgFinal ) != CE_SUCCESS )
	{
		Warning( "Failed to write PNG\n" );
		CloseWithGenericError();
		return;
	}

	// Stats
	EconUI()->Gamestats_ItemTransaction( IE_ITEM_USED_TOOL, m_pToolModelPanel->GetItem(), "customized_texture" );

	// Start a job to do the async work
	CCustomizeTextureJobDialog *pJob = new CCustomizeTextureJobDialog( bufPNGData.Base(), bufPNGData.TellPut(), this );
	pJob->StartJob( NULL );
}

void CConfirmCustomizeTextureDialog::CloseWithGenericError()
{
	CloseWaitingDialog();

	// Show error message dialog
	ShowMessageBox( "#ToolCustomizeTextureError", "#ToolCustomizeTextureErrorMsg", "#GameUI_OK" );

	// Close this window
	OnCommand("close");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmCustomizeTextureDialog::ConversionError( ConversionErrorType nError )
{
	const char *pErrorText = NULL;

	switch ( nError )
	{
	case CE_MEMORY_ERROR:
		pErrorText = "#GameUI_Spray_Import_Error_Memory";
		break;

	case CE_CANT_OPEN_SOURCE_FILE:
		pErrorText = "#GameUI_Spray_Import_Error_Reading_Image";
		break;

	case CE_ERROR_PARSING_SOURCE:
		pErrorText = "#GameUI_Spray_Import_Error_Image_File_Corrupt";
		break;

	case CE_SOURCE_FILE_SIZE_NOT_SUPPORTED:
		pErrorText = "#GameUI_Spray_Import_Image_Wrong_Size";
		break;

	case CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED:
		pErrorText = "#GameUI_Spray_Import_Image_Wrong_Size";
		break;

	case CE_SOURCE_FILE_TGA_FORMAT_NOT_SUPPORTED:
		pErrorText = "#GameUI_Spray_Import_Error_TGA_Format_Not_Supported";
		break;

	case CE_SOURCE_FILE_BMP_FORMAT_NOT_SUPPORTED:
		pErrorText = "#GameUI_Spray_Import_Error_BMP_Format_Not_Supported";
		break;

	case CE_ERROR_WRITING_OUTPUT_FILE:
		pErrorText = "#GameUI_Spray_Import_Error_Writing_Temp_Output";
		break;

	case CE_ERROR_LOADING_DLL:
		pErrorText = "#GameUI_Spray_Import_Error_Cant_Load_VTEX_DLL";
		break;
	}

	if ( pErrorText )
	{
		ShowMessageBox( "#ToolCustomizeTextureError", pErrorText, "#GameUI_OK" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CConfirmCustomizeTextureDialog::OnFileSelected(const char *fullpath)
{
	// this can take a while, put up a waiting cursor
	vgui::surface()->SetCursor( vgui::dc_hourglass );

	// they apparently don't want to use their avatar
	m_bUseAvatar = false;

	// Will need to be restretched/cropped/filtered, no matter what happens next
	MarkSquareImageDirty();

	// Load up the data as raw RGBA
	ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( fullpath, m_imgSource );
	if ( nErrorCode != CE_SUCCESS )
	{
		// Report error, if any
		ConversionError( nErrorCode );
	}

	// Slam alpha to 255.  We do not support images with alpha
	for ( int y = 0 ; y < m_imgSource.Height() ; ++y )
	{
		for ( int x = 0 ; x < m_imgSource.Width() ; ++x )
		{
			Color c = m_imgSource.GetColor( x, y );
			c[3] = 255;
			m_imgSource.SetColor( x, y, c );
		}
	}

	// Show/hide controls as appropriate
	WriteSelectImagePageControls();

	// Tick the palette entries right now, no matter what else happened
	//OnTick();

	// change the cursor back to normal
	vgui::surface()->SetCursor( vgui::dc_user );
}

void CConfirmCustomizeTextureDialog::OnTextChanged( vgui::Panel *panel )
{
	// Check for known controls
	if ( panel == m_pFilterCombo )
	{

		// Mark us as dirty
		MarkFilteredImageDirty();

		// Update controls
		ShowFilterControls();
	}
	else if ( panel == m_pSquarizeCombo )
	{

		// If image is nearly square, ignore this, there shouldn't
		// be any options
		if ( !IsSourceImageSquare() )
		{

			// Set new option, if it is changing
			bool bNewOption = ( m_pSquarizeCombo->GetActiveItem() == 1 );
			if ( !bNewOption != !m_bCropToSquare )
			{
				m_bCropToSquare = bNewOption;
				MarkSquareImageDirty();
			}
		}
	}
	else if ( panel == m_pStencilModeCombo )
	{
		MarkFilteredImageDirty();
	}
	else
	{
		// Who else is talking to us?
		Assert( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconTool_CustomizeTexture::OnClientApplyTool( CEconItemView *pTool, CEconItemView *pSubject, vgui::Panel *pParent ) const
{
	CConfirmCustomizeTextureDialog *dialog = vgui::SETUP_PANEL( new CConfirmCustomizeTextureDialog( pParent, pTool, pSubject ) );
	MakeModalAndBringToFront( dialog );
}
