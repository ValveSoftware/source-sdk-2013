//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose:
//
// $NoKeywords: $
//
// Author: samisalreadytaken
//
//=============================================================================//


#include "cbase.h"
#include "tier1/utlcommon.h"

#include "inputsystem/iinputsystem.h"
#include "iinput.h"

#include <vgui/VGUI.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui/ILocalize.h>

#include <ienginevgui.h>

#include "matsys_controls/matsyscontrols.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/TextEntry.h>

#include <vgui_controls/Image.h>
#include <vgui_controls/TextImage.h>
//#include <vgui_controls/Tooltip.h>

#if VGUI_TGA_IMAGE_PANEL
#include "bitmap/tgaloader.h"
#endif

#if !defined(NO_STEAM)
#include "steam/steam_api.h"
#include "vgui_avatarimage.h"
#endif

#include "view.h"
#include "hudelement.h"
#include "iclientmode.h" // g_pClientMode->GetViewport()

#include "vscript_vgui.h"
#include "vscript_vgui.nut"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//=============================================================================
//
// Exposing a new panel class (e.g. vgui::FileOpenDialog):
//
// 1. Create C++ bindings using 'CLASS_HELPER_INTERFACE( FileOpenDialog, Frame ){};'
// 2. Define script bindings using '#define DEFINE_VGUI_SCRIPTFUNC_FileOpenDialog()'
// 3. Create 'class CScript_FileOpenDialog : FileOpenDialog' with vgui message callbacks and overrides if needed
// 4. Create script helper using 'BEGIN_VGUI_HELPER( FileOpenDialog )', 'END_VGUI_HELPER()'. This determines the script class name.
// 5. Register script bindings with 'BEGIN_SCRIPTDESC_VGUI( FileOpenDialog )', 'END_SCRIPTDESC()'
// 6. Add new condition in CScriptVGUI::CreatePanel()
//
//
//
// CScript_FileOpenDialog_Helper
// ^^
//   IScript_FileOpenDialog << CScript_FileOpenDialog
//   ^^                        ^^
//     IScript_Frame             FileOpenDialog
//     ^^                        ^^
//       IScript_Panel             Frame
//       ^^                        ^^
//         CScriptVGUIObject         Panel
//
//=============================================================================


// When enabled, script panels will be parented to custom root panels.
// When disabled, script panels will be parented to engine root panels, and allow Z values for script panels to be interplaced amongst non-script panels.
// Changing this is not backwards compatible, as existing top level script panel depth would then change relative to non-script panels.
#define SCRIPT_ENGINE_ROOT_PANELS 1

//
// Options to restrict where script panels can be parented to.
// The safest options any game can have are HUD viewport and clientdll.
//

#define ALLOW_ROOT_PANEL_PARENT 1

#define ALLOW_HUD_VIEWPORT_ROOT_PARENT 1

#define ALLOW_CLIENTDLL_ROOT_PARENT 1

#define ALLOW_GAMEUI_ROOT_PARENT 0

// On level transitions Restore is called up to 4 times in a row (due to .hl? client state files), each time
// trying to restore script panels from pre and post transitions, failing every time because script panels are
// destroyed on level shutdown but after client state files are written.
//
// Script variables are also reset between each OnRestore callback, causing duplicate panels if user scripts create panels
// by checking restored script variables.
//
// The workaround hack is to queue OnRestore callbacks with a think function.
//
// This code is left here for testing.
#define SCRIPT_VGUI_SAVERESTORE 0

#define SCRIPT_VGUI_SIGNAL_INTERFACE 0



#ifdef _DEBUG
#define DebugMsg(...) ConColorMsg( Color(196, 196, 156, 255), __VA_ARGS__ )
#define DebugWarning(...) Warning( __VA_ARGS__ )
#define DebugDevMsg(...) DevMsg( __VA_ARGS__ )

#define DBG_PARAM(...) __VA_ARGS__
#else
#define DebugMsg(...) (void)(0)
#define DebugWarning(...) (void)(0)
#define DebugDevMsg(...) (void)(0)

#define DBG_PARAM(...)
#endif



template< typename T >
class CCopyableUtlVectorConservative : public CUtlVectorConservative< T >
{
	typedef CUtlVectorConservative< T > BaseClass;
public:
	explicit CCopyableUtlVectorConservative( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	explicit CCopyableUtlVectorConservative( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
	CCopyableUtlVectorConservative( CCopyableUtlVectorConservative const& vec ) { this->CopyArray( vec.Base(), vec.Count() ); }
};


using namespace vgui;
class IScriptVGUIObject;
struct FontData_t;

// Aliases contain only one font definition unless 'yres' was defined
typedef CCopyableUtlVectorConservative< FontData_t > fontalias_t;
typedef CUtlDict< fontalias_t > CFontDict;


CFontDict g_ScriptFonts( k_eDictCompareTypeCaseSensitive );
CUtlVector< int > g_ScriptTextureIDs;
CUtlLinkedList< IScriptVGUIObject*, unsigned short > g_ScriptPanels;


// Boundary is not checked in Surface, keep count manually to sanitise user input.
static int g_nFontCount = 0;

static inline HFont IntToFontHandle( int i )
{
	if ( i < 0 || i > g_nFontCount )
		return INVALID_FONT;
	return static_cast< unsigned int >(i);
}

// vscript does not support unsigned int,
// but the representation of the handle does not matter,
// and these handles are CUtlVector indices
static inline int HandleToInt( unsigned int i )
{
	return static_cast< int >(i);
}


struct FontData_t
{
	HFont font;
	char *name;
	int tall;
	int weight;
	int blur;
	int scanlines;
	int flags;
	//int range_min;
	//int range_max;
	int yres_min;
	int yres_max;
	bool proportional;
};

static const char *GetFixedFontName( const char *name, bool proportional )
{
	static char fontName[64];
	V_snprintf( fontName, sizeof(fontName), "%s-%s", name, proportional ? "p" : "no" );
	return fontName;
}

CON_COMMAND( vgui_spew_fonts_script, "" )
{
	char fontName[64];

	FOR_EACH_DICT_FAST( g_ScriptFonts, i )
	{
		const FontData_t &data = g_ScriptFonts[i].Head();
		const char *name = surface()->GetFontName( data.font );
		const char *alias = g_ScriptFonts.GetElementName(i);

		// Strip off the appendix "-p" / "-no"
		V_StrLeft( alias, V_strlen(alias) - (data.proportional ? 2 : 3), fontName, sizeof(fontName) );

		Msg( "  %2d: HFont:0x%8.8lx, %s, %s, font:%s, tall:%d(%d) {%d}\n",
			i,
			data.font,
			fontName,
			alias,
			name ? name : "??",
			surface()->GetFontTall( data.font ),
			surface()->GetFontTallRequested( data.font ),
			g_ScriptFonts[i].Count() );
	}
}

bool LoadFont( const FontData_t &font DBG_PARAM(, const char *fontAlias) )
{
	if ( font.yres_min )
	{
		int nScreenWide, nScreenTall;
		surface()->GetScreenSize( nScreenWide, nScreenTall );

		if ( nScreenTall < font.yres_min )
			return false;

		if ( font.yres_max && nScreenTall > font.yres_max )
			return false;
	}

	int tall = font.tall;
	int blur = font.blur;
	int scanlines = font.scanlines;

	if ( font.proportional && !font.yres_min )
	{
		tall = scheme()->GetProportionalScaledValue( tall );
		blur = scheme()->GetProportionalScaledValue( blur );
		scanlines = scheme()->GetProportionalScaledValue( scanlines );
	}

	bool bSuccess = surface()->SetFontGlyphSet(
		font.font,
		font.name,
		tall,
		font.weight,
		blur,
		scanlines,
		font.flags );

	NOTE_UNUSED( bSuccess );
	if ( bSuccess )
	{
		if ( font.yres_min )
			DebugMsg( "Load font [%li]%s [%d %d]\n", font.font, fontAlias, font.yres_min, font.yres_max );
		else
			DebugMsg( "Load font [%li]%s\n", font.font, fontAlias );
	}
	else
	{
		DebugWarning( "Failed to load font [%li]%s\n", font.font, fontAlias );
	}

	return true;
}

void ReloadScriptFontGlyphs()
{
	// Invalidate cached values
	if ( g_pScriptVM )
		g_pScriptVM->Run( "ISurface.__OnScreenSizeChanged()" );

	FOR_EACH_DICT_FAST( g_ScriptFonts, i )
	{
		const fontalias_t &alias = g_ScriptFonts[i];
		for ( int j = 0; j < alias.Count(); ++j )
		{
			if ( LoadFont( alias.Element(j) DBG_PARAM(, g_ScriptFonts.GetElementName(i)) ) )
				break;
		}
	}
}


static inline void InitRootPanel( Panel *p, VGuiPanel_t parent, const char *name )
{
	int w, h;
	surface()->GetScreenSize( w, h );
	p->Init( 0, 0, w, h );
	p->SetName( name );
	p->SetVisible( true );
	p->SetPaintEnabled( false );
	p->SetPaintBackgroundEnabled( false );
	p->SetPaintBorderEnabled( false );
	p->SetPostChildPaintEnabled( false );
	p->SetParent( enginevgui->GetPanel( parent ) );
}

class CScriptRootPanel : public Panel
{
public:
	CScriptRootPanel()
	{
		InitRootPanel( this, PANEL_ROOT, "VScriptRoot" );
	}

	void OnTick()
	{
		if ( m_nLastFrame == gpGlobals->framecount )
			return;

		ReloadScriptFontGlyphs();
		ivgui()->RemoveTickSignal( GetVPanel() );
	}

	// Used as a callback to font invalidation.
	// Ideally script fonts would be loaded along with others in engine.
	// In that case CScriptRootPanel would be removed, and
	// g_pScriptRootPanel would be CScriptRootDLLPanel inside #if SCRIPT_ENGINE_ROOT_PANELS
	void OnScreenSizeChanged( int w, int t )
	{
		// Reload fonts in the next vgui frame
		ivgui()->AddTickSignal( GetVPanel() );
		m_nLastFrame = gpGlobals->framecount;

		// Invalidate cached values
		if ( g_pScriptVM )
			g_pScriptVM->Run( "ISurface.__OnScreenSizeChanged()" );

		Panel::OnScreenSizeChanged( w, t );
	}

private:
	int m_nLastFrame;
};

CScriptRootPanel *g_pScriptRootPanel = NULL;

#if SCRIPT_ENGINE_ROOT_PANELS
class CScriptRootDLLPanel : public Panel
{
public:
	CScriptRootDLLPanel( VGuiPanel_t parent, const char *name )
	{
		InitRootPanel( this, parent, name );
	}
};

#if ALLOW_CLIENTDLL_ROOT_PARENT
CScriptRootDLLPanel *g_pScriptClientDLLPanel = NULL;
#endif
#if ALLOW_GAMEUI_ROOT_PARENT
CScriptRootDLLPanel *g_pScriptGameUIDLLPanel = NULL;
#endif
#endif

void VGUI_DestroyScriptRootPanels()
{
	if ( g_pScriptRootPanel )
	{
		delete g_pScriptRootPanel;
		g_pScriptRootPanel = NULL;
	}
#if SCRIPT_ENGINE_ROOT_PANELS
#if ALLOW_CLIENTDLL_ROOT_PARENT
	if ( g_pScriptClientDLLPanel )
	{
		delete g_pScriptClientDLLPanel;
		g_pScriptClientDLLPanel = NULL;
	}
#endif
#if ALLOW_GAMEUI_ROOT_PARENT
	if ( g_pScriptGameUIDLLPanel )
	{
		delete g_pScriptGameUIDLLPanel;
		g_pScriptGameUIDLLPanel = NULL;
	}
#endif
#endif
}

VPANEL VGUI_GetScriptRootPanel( VGuiPanel_t type )
{
#if SCRIPT_ENGINE_ROOT_PANELS
	switch ( type )
	{
		case PANEL_ROOT:
#if ALLOW_ROOT_PANEL_PARENT
		{
			if ( !g_pScriptRootPanel )
				g_pScriptRootPanel = new CScriptRootPanel();

			return g_pScriptRootPanel->GetVPanel();
		}
#endif
		case PANEL_CLIENTDLL:
#if ALLOW_CLIENTDLL_ROOT_PARENT
		{
			if ( !g_pScriptClientDLLPanel )
				g_pScriptClientDLLPanel = new CScriptRootDLLPanel( PANEL_CLIENTDLL, "VScriptClient" );

			return g_pScriptClientDLLPanel->GetVPanel();
		}
#endif
		case PANEL_GAMEUIDLL:
#if ALLOW_GAMEUI_ROOT_PARENT
		{
			if ( !g_pScriptGameUIDLLPanel )
				g_pScriptGameUIDLLPanel = new CScriptRootDLLPanel( PANEL_GAMEUIDLL, "VScriptGameUI" );

			return g_pScriptGameUIDLLPanel->GetVPanel();
		}
#endif
		default: return NULL;
	}
#else
	return enginevgui->GetPanel(type);
#endif
}


//
// Escapes "vgui/" prepended to the file name in CSchemeManager::GetImage().
//
IImage *vgui_GetImage( const char *imageName, bool hardwareFilter )
{
	char fileName[MAX_PATH];
	V_snprintf( fileName, sizeof( fileName ), "../%s", imageName );

	return scheme()->GetImage( fileName, hardwareFilter );
}


//--------------------------------------------------------------
//
//--------------------------------------------------------------
class CScriptSurface
{
public:
	void PlaySound( const char* sound );
	void SetColor( int r, int g, int b, int a );
	void DrawFilledRect( int x0, int y0, int width, int height );
	void DrawFilledRectFade( int x0, int y0, int width, int height, int a0, int a1, bool bHorz );
	void DrawOutlinedRect( int x0, int y0, int width, int height, int thickness );
	void DrawLine( int x0, int y0, int x1, int y1 );
	void DrawOutlinedCircle( int x, int y, int radius, int segments );

	void SetTextColor( int r, int g, int b, int a );
	void SetTextPos( int x, int y );
	void SetTextFont( int font );
	void DrawText( const char *text, int drawType/* = FONT_DRAW_DEFAULT*/ );
	void DrawUnicodeChar( int ch, int drawType/* = FONT_DRAW_DEFAULT*/ );

	int GetFont( const char* name, bool proportional, const char* schema );
	int GetTextWidth( int font, const char* text );
	int GetFontTall( int font );
	int GetCharacterWidth( int font, int ch );

	void CreateFont( const char *customName, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int yresMin, int yresMax, bool proportional );
	bool AddCustomFontFile( const char *fontFileName );

	int GetTextureID( char const *filename );
	int ValidateTexture( const char *filename, bool hardwareFilter, bool forceReload, bool procedural );
	void SetTextureFile( int id, const char *filename, bool hardwareFilter );
	int GetTextureWide( int id );
	int GetTextureTall( int id );
	void SetTexture( int id );

	void DrawTexturedRect( int x0, int y0, int width, int height );
	void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 );

	// ------------------------------------------------------------
	// Utility functions
	// ------------------------------------------------------------

	void DrawTexturedBox( int texture, int x, int y, int wide, int tall, int r, int g, int b, int a );
	void DrawColoredText( int font, int x, int y, int r, int g, int b, int a, const char *text );
	void DrawColoredTextRect( int font, int x, int y, int w, int h, int r, int g, int b, int a, const char *text );
	void DrawTexturedRectRotated( int x, int y, int w, int t, float yaw );

} script_surface;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptSurface, "ISurface", SCRIPT_SINGLETON )
	DEFINE_SCRIPTFUNC( PlaySound, "" )

	DEFINE_SCRIPTFUNC( SetColor, "" )
	DEFINE_SCRIPTFUNC( DrawFilledRect, "" )
	DEFINE_SCRIPTFUNC( DrawFilledRectFade, "" )
	DEFINE_SCRIPTFUNC( DrawOutlinedRect, "" )
	DEFINE_SCRIPTFUNC( DrawLine, "" )
	DEFINE_SCRIPTFUNC( DrawOutlinedCircle, "" )

	DEFINE_SCRIPTFUNC( SetTextColor, "" )
	DEFINE_SCRIPTFUNC( SetTextPos, "" )
	DEFINE_SCRIPTFUNC( SetTextFont, "" )
	DEFINE_SCRIPTFUNC( DrawText, "" )
	DEFINE_SCRIPTFUNC( DrawUnicodeChar, "" )

	DEFINE_SCRIPTFUNC( GetFont, "" )
	DEFINE_SCRIPTFUNC( GetTextWidth, "" )
	DEFINE_SCRIPTFUNC( GetFontTall, "" )
	DEFINE_SCRIPTFUNC( GetCharacterWidth, "" )

	DEFINE_SCRIPTFUNC( CreateFont, SCRIPT_HIDE )
	DEFINE_SCRIPTFUNC( AddCustomFontFile, "" )

	DEFINE_SCRIPTFUNC( GetTextureID, "" )
	DEFINE_SCRIPTFUNC( ValidateTexture, "" )
	DEFINE_SCRIPTFUNC( SetTextureFile, "" )
	DEFINE_SCRIPTFUNC( GetTextureWide, "" )
	DEFINE_SCRIPTFUNC( GetTextureTall, "" )
	DEFINE_SCRIPTFUNC( SetTexture, "" )

	DEFINE_SCRIPTFUNC( DrawTexturedRect, "" )
	DEFINE_SCRIPTFUNC( DrawTexturedSubRect, "" )

	DEFINE_SCRIPTFUNC( DrawTexturedBox, "" )
	DEFINE_SCRIPTFUNC( DrawColoredText, "" )
	DEFINE_SCRIPTFUNC( DrawColoredTextRect, "" )
	DEFINE_SCRIPTFUNC( DrawTexturedRectRotated, "" )
END_SCRIPTDESC()


void CScriptSurface::PlaySound( const char* sound )
{
	surface()->PlaySound(sound);
}

void CScriptSurface::SetColor( int r, int g, int b, int a )
{
	surface()->DrawSetColor( r, g, b, a );
}

void CScriptSurface::DrawFilledRect( int x0, int y0, int width, int height )
{
	surface()->DrawFilledRect( x0, y0, x0 + width, y0 + height );
}

void CScriptSurface::DrawFilledRectFade( int x0, int y0, int width, int height, int a0, int a1, bool bHorz )
{
	surface()->DrawFilledRectFade( x0, y0, x0 + width, y0 + height, a0, a1, bHorz );
}

void CScriptSurface::DrawOutlinedRect( int x0, int y0, int width, int height, int thickness )
{
	int x1 = x0 + width;
	int y1 = y0 + height - thickness;
	y0 += thickness;

	surface()->DrawFilledRect( x0, y0 - thickness, x1, y0 ); // top
	surface()->DrawFilledRect( x1 - thickness, y0, x1, y1 ); // right
	surface()->DrawFilledRect( x0, y1, x1, y1 + thickness ); // bottom
	surface()->DrawFilledRect( x0, y0, x0 + thickness, y1 ); // left
}

void CScriptSurface::DrawLine( int x0, int y0, int x1, int y1 )
{
	surface()->DrawLine( x0, y0, x1, y1 );
}
#if 0
void CScriptSurface::DrawPolyLine( HSCRIPT ax, HSCRIPT ay, int count )
{
	if (count < 1)
		return;

	if (count > 4096)
		count = 4096;

	int *px = (int*)stackalloc( count * sizeof(int) );
	int *py = (int*)stackalloc( count * sizeof(int) );
	ScriptVariant_t vx, vy;

	int i = count;
	while ( i-- )
	{
		g_pScriptVM->GetValue( ax, i, &vx );
		g_pScriptVM->GetValue( ay, i, &vy );

		px[i] = vx.m_int;
		py[i] = vy.m_int;
	}

	surface()->DrawPolyLine( px, py, count );
}
#endif
void CScriptSurface::DrawOutlinedCircle( int x, int y, int radius, int segments )
{
	surface()->DrawOutlinedCircle( x, y, radius, segments );
}

void CScriptSurface::SetTextColor( int r, int g, int b, int a )
{
	surface()->DrawSetTextColor( r, g, b, a );
}

void CScriptSurface::SetTextPos( int x, int y )
{
	surface()->DrawSetTextPos( x, y );
}

void CScriptSurface::SetTextFont( int font )
{
	surface()->DrawSetTextFont( IntToFontHandle(font) );
}

void CScriptSurface::DrawText( const char *text, int drawType )
{
	wchar_t wcs[512];
	g_pVGuiLocalize->ConvertANSIToUnicode( text, wcs, sizeof(wcs) );
	surface()->DrawPrintText( wcs, wcslen(wcs), (FontDrawType_t)drawType );
}

void CScriptSurface::DrawUnicodeChar( int ch, int drawType )
{
	surface()->DrawUnicodeChar( (wchar_t)ch, (FontDrawType_t)drawType );
}

int CScriptSurface::GetFont( const char* name, bool proportional, const char* schema )
{
	HFont font = INVALID_FONT;

	if ( !schema || !schema[0] )
	{
		int idx = g_ScriptFonts.Find( GetFixedFontName( name, proportional ) );
		if ( idx != g_ScriptFonts.InvalidIndex() )
		{
			font = g_ScriptFonts[idx].Head().font;
		}
	}
	else
	{
		HScheme sch = scheme()->GetScheme( schema );
		font = scheme()->GetIScheme(sch)->GetFont( name, proportional );

		// Update known count
		if ( font > (unsigned int)g_nFontCount )
			g_nFontCount = font;
	}

	return HandleToInt( font );
}

int CScriptSurface::GetTextWidth( int font, const char* text )
{
	int w, t;
	wchar_t wcs[512];
	g_pVGuiLocalize->ConvertANSIToUnicode( text, wcs, sizeof(wcs) );
	surface()->GetTextSize( IntToFontHandle(font), wcs, w, t );
	return w;
}

int CScriptSurface::GetFontTall( int font )
{
	return surface()->GetFontTall( IntToFontHandle(font) );
}

int CScriptSurface::GetCharacterWidth( int font, int ch )
{
	return surface()->GetCharacterWidth( IntToFontHandle(font), ch );
}

void CScriptSurface::CreateFont( const char *customName, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int yresMin, int yresMax, bool proportional )
{
	// Make sure font invalidation callback is established.
	// Not necessary if script fonts are reloaded in engine.
	if ( !g_pScriptRootPanel )
		g_pScriptRootPanel = new CScriptRootPanel();

	if ( flags & ISurface::FONTFLAG_BITMAP )
	{
		AssertMsg( 0, "Bitmap fonts are not supported!" );
		return;
	}

	if ( proportional && yresMin )
	{
		AssertMsg( 0, "Resolution cannot be defined on a proportional font!" );
		return;
	}

	if ( (yresMin < 0 || yresMax < 0) || (!!yresMin != !!yresMax) )
	{
		AssertMsg( 0, "Invalid resolution!" );
		return;
	}

#if 0
	bool bProportionalFallbackFont = false;
	if ( proportional )
	{
		// Find if this is a resolution filtered font alias
		const char *fontAlias = GetFixedFontName( customName, false );
		int idx = g_ScriptFonts.Find( fontAlias );
		if ( idx != g_ScriptFonts.InvalidIndex() )
		{
			fontalias_t &alias = g_ScriptFonts[idx];
			for ( int i = 0; i < alias.Count(); ++i )
			{
				FontData_t &data = alias.Element(i);
				if ( data.yres_min && data.yres_max )
				{
					bProportionalFallbackFont = true;

					// Save this proportional font in non-proportional alias
					proportional = false;
					break;
				}
			}
		}
	}
#endif

	const char *fontAlias = GetFixedFontName( customName, proportional );

	int idx = g_ScriptFonts.Find( fontAlias );
	if ( idx != g_ScriptFonts.InvalidIndex() )
	{
		fontalias_t &alias = g_ScriptFonts[idx];

#ifdef _DEBUG
		if ( !yresMin && !yresMax /*&& !bProportionalFallbackFont*/ )
		{
			// There must be only one font registered.
			Assert( alias.Count() == 1 );

			HFont font = alias.Head().font;
			int oldTall = surface()->GetFontTallRequested( font );
			int newTall = proportional ? scheme()->GetProportionalScaledValue( tall ) : tall;
			const char *oldName = surface()->GetFontName( font );

			// Font changes will not be applied.
			Assert( oldTall == newTall );
			if ( oldName ) // can be null
				AssertMsg( !V_stricmp( oldName, windowsFontName ), "'%s' != '%s'", oldName, windowsFontName );
		}
#endif

		// if input resolutions match any of the existing fonts,
		// then this must be a duplicate call.
		for ( int i = 0; i < alias.Count(); ++i )
		{
			FontData_t &data = alias.Element(i);

			if ( yresMin == data.yres_min && yresMax == data.yres_max )
				return;
		}
#if 0
		if ( bProportionalFallbackFont )
			proportional = true;
#endif
		DebugMsg( "Create font add '%s' [%d %d]\n", fontAlias, yresMin, yresMax );

		FontData_t &newFont = alias.Element( alias.AddToTail() );
		newFont.font = alias.Head().font;
		newFont.name = strdup( windowsFontName );
		newFont.tall = tall;
		newFont.weight = weight;
		newFont.blur = blur;
		newFont.scanlines = scanlines;
		newFont.flags = flags;
		newFont.yres_min = yresMin;
		newFont.yres_max = yresMax;
		newFont.proportional = proportional;

#if 0
		// Put the proportional font in the very end so that it is loaded only when no resolution is matched
		struct L
		{
			static int __cdecl F( const FontData_t* a, const FontData_t* b )
			{
				if ( !a->proportional && b->proportional )
					return -1;
				if ( a->proportional && !b->proportional )
					return 1;
				return 0;
			}
		};
		alias.Sort( L::F );
#endif

		LoadFont( newFont DBG_PARAM(, fontAlias) );
	}
	else
	{
		HFont font = surface()->CreateFont();

		// Sanity check
		Assert( font > (unsigned int)g_nFontCount && font < INT_MAX );

		// Update known count
		if ( font > (unsigned int)g_nFontCount )
			g_nFontCount = font;

		if ( yresMax && yresMin > yresMax )
		{
			int t = yresMin;
			yresMin = yresMax;
			yresMax = t;
		}

		if ( yresMin )
			DebugMsg( "Create font new '%s' [%d %d]\n", fontAlias, yresMin, yresMax );
		else
			DebugMsg( "Create font new '%s'\n", fontAlias );

		fontalias_t &alias = g_ScriptFonts.Element( g_ScriptFonts.Insert( fontAlias ) );
		FontData_t &newFont = alias.Element( alias.AddToTail() );
		newFont.font = font;
		newFont.name = strdup( windowsFontName );
		newFont.tall = tall;
		newFont.weight = weight;
		newFont.blur = blur;
		newFont.scanlines = scanlines;
		newFont.flags = flags;
		newFont.yres_min = yresMin;
		newFont.yres_max = yresMax;
		newFont.proportional = proportional;

		LoadFont( newFont DBG_PARAM(, fontAlias) );
	}
}

bool CScriptSurface::AddCustomFontFile( const char *fontFileName )
{
	return surface()->AddCustomFontFile( NULL, fontFileName );
}

int CScriptSurface::GetTextureID( char const *filename )
{
	return surface()->DrawGetTextureId( filename );
}

// Create texture if it does not already exist
int CScriptSurface::ValidateTexture( const char *filename, bool hardwareFilter, bool forceReload, bool procedural )
{
	int id = surface()->DrawGetTextureId( filename );
	if ( id <= 0 )
	{
		id = surface()->CreateNewTextureID( procedural );
		g_ScriptTextureIDs.AddToTail( id );

		surface()->DrawSetTextureFile( id, filename, hardwareFilter, forceReload );

#ifdef _DEBUG
		char tex[MAX_PATH];
		surface()->DrawGetTextureFile( id, tex, sizeof(tex)-1 );
		if ( !V_stricmp( filename, tex ) )
		{
			DebugMsg( "Create texture [%i]%s\n", id, filename );
		}
		else
		{
			DebugWarning( "Create texture [%i]%s(%s)\n", id, tex, filename );
		}
#endif
	}
	else if ( forceReload && g_ScriptTextureIDs.HasElement( id ) )
	{
		surface()->DrawSetTextureFile( id, filename, hardwareFilter, forceReload );
	}
	else
	{
		surface()->DrawSetTexture( id );
	}

	return id;
}

// Replace existing texture
void CScriptSurface::SetTextureFile( int id, const char *filename, bool hardwareFilter )
{
	if ( g_ScriptTextureIDs.HasElement(id) )
	{
		Assert( surface()->IsTextureIDValid(id) );
		surface()->DrawSetTextureFile( id, filename, hardwareFilter, true );

#ifdef _DEBUG
		char tex[MAX_PATH];
		surface()->DrawGetTextureFile( id, tex, sizeof(tex)-1 );
		if ( !V_stricmp( filename, tex ) )
		{
			DebugMsg( "Set texture [%i]%s\n", id, filename );
		}
		else
		{
			DebugWarning( "Set texture [%i]%s(%s)\n", id, tex, filename );
		}
#endif
	}

#ifdef _DEBUG
	if ( !g_ScriptTextureIDs.HasElement(id) && surface()->IsTextureIDValid(id) )
	{
		DebugWarning( "Tried to set non-script created texture! [%i]%s\n", id, filename );
	}

	if ( !surface()->IsTextureIDValid(id) )
	{
		DebugWarning( "Tried to set invalid texture id! [%i]%s\n", id, filename );
	}
#endif
}
#if 0
void CScriptSurface::SetTextureMaterial( int id, HSCRIPT hMaterial )
{
	IMaterial *pMaterial = (IMaterial*)HScriptToClass< IScriptMaterial >( hMaterial );
	if ( !IsValid( pMaterial ) )
		return;

	if ( g_ScriptTextureIDs.HasElement(id) )
	{
		Assert( surface()->IsTextureIDValid(id) );
		MatSystemSurface()->DrawSetTextureMaterial( id, pMaterial );

		DebugMsg( "Set texture [%i]%s\n", id, pMaterial->GetName() );
	}

#ifdef _DEBUG
	if ( !g_ScriptTextureIDs.HasElement(id) && surface()->IsTextureIDValid(id) )
	{
		DebugWarning( "Tried to set non-script created texture! [%i]\n", id );
	}

	if ( !surface()->IsTextureIDValid(id) )
	{
		DebugWarning( "Tried to set invalid texture id! [%i]\n", id );
	}
#endif
}
#endif
int CScriptSurface::GetTextureWide( int id )
{
	int w, t;
	surface()->DrawGetTextureSize( id, w, t );
	return w;
}

int CScriptSurface::GetTextureTall( int id )
{
	int w, t;
	surface()->DrawGetTextureSize( id, w, t );
	return t;
}

void CScriptSurface::SetTexture( int id )
{
	surface()->DrawSetTexture( id );
}

void CScriptSurface::DrawTexturedRect( int x0, int y0, int width, int height )
{
	surface()->DrawTexturedRect( x0, y0, x0 + width, y0 + height );
}

void CScriptSurface::DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 )
{
	surface()->DrawTexturedSubRect( x0, y0, x1, y1, texs0, text0, texs1, text1 );
}

void CScriptSurface::DrawTexturedRectRotated( int x, int y, int w, int t, float yaw )
{
	Vertex_t verts[4];
	Vector2D axis[2];

	float sy, cy;
	SinCos( DEG2RAD( -yaw ), &sy, &cy );

	axis[0].x = cy;
	axis[0].y = sy;
	axis[1].x = -axis[0].y;
	axis[1].y = axis[0].x;

	verts[0].m_TexCoord.Init( 0, 0 );
	Vector2DMA( Vector2D( x + w * 0.5f, y + t * 0.5f ), w * -0.5f, axis[0], verts[0].m_Position );
	Vector2DMA( verts[0].m_Position, t * -0.5f, axis[1], verts[0].m_Position );

	verts[1].m_TexCoord.Init( 1, 0 );
	Vector2DMA( verts[0].m_Position, w, axis[0], verts[1].m_Position );

	verts[2].m_TexCoord.Init( 1, 1 );
	Vector2DMA( verts[1].m_Position, t, axis[1], verts[2].m_Position );

	verts[3].m_TexCoord.Init( 0, 1 );
	Vector2DMA( verts[0].m_Position, t, axis[1], verts[3].m_Position );

	surface()->DrawTexturedPolygon( 4, verts );
}

void CScriptSurface::DrawTexturedBox( int texture, int x, int y, int wide, int tall, int r, int g, int b, int a )
{
	surface()->DrawSetColor( r, g, b, a );
	surface()->DrawSetTexture( texture );
	surface()->DrawTexturedRect( x, y, x + wide, y + tall );
}

void CScriptSurface::DrawColoredText( int font, int x, int y, int r, int g, int b, int a, const char *text )
{
	wchar_t wcs[512];
	g_pVGuiLocalize->ConvertANSIToUnicode( text, wcs, sizeof(wcs) );

	surface()->DrawSetTextFont( IntToFontHandle(font) );
	surface()->DrawSetTextColor( r, g, b, a );
	surface()->DrawSetTextPos( x, y );
	surface()->DrawPrintText( wcs, wcslen(wcs) );
}

void CScriptSurface::DrawColoredTextRect( int font, int x, int y, int w, int h, int r, int g, int b, int a, const char *text )
{
	MatSystemSurface()->DrawColoredTextRect( IntToFontHandle(font), x, y, w, h, r, g, b, a, text );
}


//==============================================================
//==============================================================

#define __base() this->_base

#define BEGIN_SCRIPTDESC_VGUI( panelClass )\
	BEGIN_SCRIPTDESC_NAMED( CScript_##panelClass##_Helper, IScriptVGUIObject, #panelClass, "" )\
		DEFINE_VGUI_SCRIPTFUNC_##panelClass()

//
// Script helpers are wrappers that only redirect to VGUI panels (such as CScript_Panel : Panel),
// these macros help to simplify definitions.
//

//
// BEGIN_VGUI_HELPER() assumes the VGUI panel class has the prefix 'CScript_'
// Use BEGIN_VGUI_HELPER_EX() to manually define VGUI panel class name.
//
#define BEGIN_VGUI_HELPER( panelClass )\
	BEGIN_VGUI_HELPER_EX( panelClass, CScript_##panelClass )

#define BEGIN_VGUI_HELPER_DEFAULT_TEXT( panelClass )\
	BEGIN_VGUI_HELPER_DEFAULT_TEXT_EX( panelClass, CScript_##panelClass )

#define BEGIN_VGUI_HELPER_EX( panelClass, baseClass )\
	class CScript_##panelClass##_Helper : public IScript_##panelClass< baseClass >\
	{\
		void Create( const char *panelName ) override\
		{\
			Assert( !_base && !_vpanel );\
			_base = new baseClass( NULL, panelName );\
		}\
\
	public:

#define BEGIN_VGUI_HELPER_DEFAULT_TEXT_EX( panelClass, baseClass )\
	class CScript_##panelClass##_Helper : public IScript_##panelClass< baseClass >\
	{\
		void Create( const char *panelName ) override\
		{\
			Assert( !_base && !_vpanel );\
			_base = new baseClass( NULL, panelName, (const char*)NULL );\
		}\
\
	public:
#define END_VGUI_HELPER()\
	};


#define CLASS_HELPER_INTERFACE_ROOT( panelClass )\
	template <class T>\
	class IScript_##panelClass : public CScriptVGUIObject<T>

#define CLASS_HELPER_INTERFACE( panelClass, baseClass )\
	template <class T>\
	class IScript_##panelClass : public IScript_##baseClass<T>


#ifdef _DEBUG
#define DEBUG_DESTRUCTOR( panelClass, baseClass )\
	panelClass()\
	{\
		DebugDestructor( baseClass )\
	}

#define DebugDestructor( panelClass )\
	{\
		DebugDevMsg( " ~" #panelClass "() '%s'\n", GetName() );\
	}
#else
#define DEBUG_DESTRUCTOR( panelClass, baseClass )
#define DebugDestructor( panelClass )
#endif

#define DECLARE_SCRIPTVGUI_CLASS( baseClass )\
	DECLARE_SCRIPTVGUI_CLASS_EX( CScript_##baseClass, baseClass )\
	DEBUG_DESTRUCTOR( ~CScript_##baseClass, baseClass )

#define DECLARE_SCRIPTVGUI_CLASS_EX( panelClass, baseClass )\
	typedef baseClass BaseClass;\
	typedef panelClass ThisClass;\
public:\
	void OnDelete()\
	{\
		DebugMsg( #baseClass "::OnDelete() '%s'\n", GetName() );\
		int i;\
		IScriptVGUIObject *obj = FindInScriptPanels( GetVPanel(), i );\
		if ( obj )\
		{\
			obj->Destroy( i );\
		}\
		BaseClass::OnDelete();\
	}

//
// Definitions for 'empty' vgui objects that do not have any script specific implementation - overrides or callbacks.
// These are required to shutdown script objects on panel death
// (on save restore where panel destructor is called after the VM is restarted while HSCRIPT members are invalid but not nullified,
// and on C++ deletion where IScriptVGUIObject::Destroy() is not automatically called).
//
#define DEFINE_VGUI_CLASS_EMPTY( panelClass )\
	class CScript_##panelClass : public panelClass\
	{\
		DECLARE_SCRIPTVGUI_CLASS( panelClass )\
		void ScriptShutdown() {}\
\
	public:\
		CScript_##panelClass( Panel *parent, const char *name )\
			: BaseClass( parent, name )\
		{}\
	};\
\
	BEGIN_VGUI_HELPER( panelClass )\
	END_VGUI_HELPER()\
\
	BEGIN_SCRIPTDESC_VGUI( panelClass )\
	END_SCRIPTDESC()

#define DEFINE_VGUI_CLASS_EMPTY_DEFAULT_TEXT( panelClass )\
	class CScript_##panelClass : public panelClass\
	{\
		DECLARE_SCRIPTVGUI_CLASS( panelClass )\
		void ScriptShutdown() {}\
\
	public:\
		CScript_##panelClass( Panel *parent, const char *name, const char *text )\
			: BaseClass( parent, name, text )\
		{}\
	};\
\
	BEGIN_VGUI_HELPER_DEFAULT_TEXT( panelClass )\
	END_VGUI_HELPER()\
\
	BEGIN_SCRIPTDESC_VGUI( panelClass )\
	END_SCRIPTDESC()

class IScriptVGUIObject
{
public:
	virtual ~IScriptVGUIObject() {}

#ifdef _DEBUG
	virtual const char *GetName() = 0;
#endif
	//-----------------------------------------------------
	// Free the VGUI panel and script instance.
	//-----------------------------------------------------
	virtual void Destroy( int ) = 0;

	//-----------------------------------------------------
	// Create new panel
	//-----------------------------------------------------
	virtual void Create( const char *panelName ) = 0;

public:
	VPANEL GetVPanel() { return _vpanel; }
	HSCRIPT GetScriptInstance() { return m_hScriptInstance; }

protected:
	VPANEL _vpanel;
	HSCRIPT m_hScriptInstance;

	// Called on deletion
	static void ResolveChildren_r( VPANEL panel DBG_PARAM(, int level) );

public:
#if SCRIPT_VGUI_SAVERESTORE
	IScriptVGUIObject() {}
	void SetScriptInstance( HSCRIPT h ) { m_hScriptInstance = h; }
	char m_pszScriptId[16];
#endif

#ifdef _DEBUG
	#if SCRIPT_VGUI_SAVERESTORE
		const char *GetDebugName() { return m_pszScriptId; }
	#else
		const char *GetDebugName() { return ""; }
	#endif
#endif
};

BEGIN_SCRIPTDESC_ROOT( IScriptVGUIObject, SCRIPT_HIDE )
END_SCRIPTDESC()


#if SCRIPT_VGUI_SAVERESTORE
class CScriptVGUIScriptInstanceHelper : public IScriptInstanceHelper
{
	void *BindOnRead( HSCRIPT hInstance, void *pOld, const char *pszId )
	{
		DebugMsg( "BindOnRead (0x%p)   (%s)   (count %d)\n", (uint)hInstance, pszId, g_ScriptPanels.Count() );

		FOR_EACH_LL( g_ScriptPanels, i )
		{
			IScriptVGUIObject *pPanel = g_ScriptPanels[i];
			// DebugMsg( "   cmp                    (%s)\n", pPanel->m_pszScriptId );
			if ( !V_stricmp( pPanel->m_pszScriptId, pszId ) )
			{
				pPanel->SetScriptInstance( hInstance );
				DebugMsg( "   ret                    (%s)\n", pPanel->m_pszScriptId );
				return pPanel;
			}
		}
		DebugMsg( "      ret                 (null)\n" );
		return NULL;
	}
};

static CScriptVGUIScriptInstanceHelper g_ScriptVGUIScriptInstanceHelper;

#define DEFINE_VGUI_SCRIPT_INSTANCE_HELPER() DEFINE_SCRIPT_INSTANCE_HELPER( &g_ScriptVGUIScriptInstanceHelper )
#else
#define DEFINE_VGUI_SCRIPT_INSTANCE_HELPER()
#endif


IScriptVGUIObject *ToScriptVGUIObj( HSCRIPT inst )
{
	return (IScriptVGUIObject *)g_pScriptVM->GetInstanceValue( inst, ::GetScriptDesc( (IScriptVGUIObject *)0 ) );
}

template < typename T > inline T* AllocScriptPanel()
{
	return new T;
}

inline IScriptVGUIObject *FindInScriptPanels( VPANEL panel, int &I )
{
	for ( int i = g_ScriptPanels.Head(); i != g_ScriptPanels.InvalidIndex(); i = g_ScriptPanels.Next(i) )
	{
		IScriptVGUIObject *obj = g_ScriptPanels[i];
		if ( obj->GetVPanel() == panel )
		{
			I = i;
			return obj;
		}
	}
	return NULL;
}

void IScriptVGUIObject::ResolveChildren_r( VPANEL panel DBG_PARAM(, int level = 0) )
{
#ifdef _DEBUG
	char indent[32];

	int l = level, c = 0;
	if ( l > 15 )
		l = 15;

	while ( l-- )
	{
		indent[c++] = ' ';
		indent[c++] = ' ';
	}
	indent[c] = 0;

	if ( level > 15 )
	{
		indent[c-1] = '.';
		indent[c-2] = '.';
	}
#endif

	CUtlVector< VPANEL > &children = ipanel()->GetChildren( panel );
	FOR_EACH_VEC_BACK( children, i )
	{
		VPANEL child = children[i];
		int j;
		IScriptVGUIObject *obj = FindInScriptPanels( child, j );
		if ( obj )
		{
			if ( ipanel()->IsAutoDeleteSet(child) )
			{
				DebugMsg( "    %sResolveChildren: '%s' (autodelete)\n", indent, obj->GetName() );

				if ( g_pScriptVM )
					g_pScriptVM->RemoveInstance( obj->m_hScriptInstance );
				g_ScriptPanels.Remove( j );
				delete obj;

				ResolveChildren_r( child DBG_PARAM(, level+1) );
			}
			else
			{
				DebugMsg( "    %sResolveChildren: '%s'\n", indent, obj->GetName() );

				// Panel::SetAutoDelete should not be added until
				// what to do on their parent death is finalised.
				//
				// This assert will be hit if a deleted panel has
				// C++ created and autodelete disabled children who are
				// also registered to script.
				Assert(0);
			}
		}
	}
}

template <class T>
class CScriptVGUIObject : public IScriptVGUIObject
{
public:
	T *_base;

	CScriptVGUIObject() : _base(0)
	{
		_vpanel = 0;
		m_hScriptInstance = 0;
	}

	void Destroy( int i = -1 )
	{
		if ( i != -1 )
		{
			Assert( g_ScriptPanels.IsValidIndex(i) );
			Assert( g_ScriptPanels[i] == this );

			g_ScriptPanels.Remove( i );
		}
		else
		{
			Assert( g_ScriptPanels.Find( this ) != g_ScriptPanels.InvalidIndex() );

			g_ScriptPanels.FindAndRemove( this );
		}

		if ( GetVPanel() )
		{
			DebugMsg( "  Destroy panel '%s'   %s\n", _base->GetName(), GetDebugName() );
			_base->ScriptShutdown();
			ResolveChildren_r( _vpanel );
			_base->MarkForDeletion();
		}

		if ( m_hScriptInstance )
		{
			if ( g_pScriptVM )
				g_pScriptVM->RemoveInstance( m_hScriptInstance );
		}

		delete this;
	}

	template <typename CHelper>
	void CreateFromScript( HSCRIPT parent, const char *panelName, int root )
	{
		Assert( !_vpanel && !m_hScriptInstance && !g_ScriptPanels.IsValidIndex( g_ScriptPanels.Find( this ) ) );

		Create( panelName && *panelName ? panelName : NULL );
		_vpanel = _base->GetVPanel();
		m_hScriptInstance = g_pScriptVM->RegisterInstance< CHelper >( static_cast< CHelper* >( this ) );

#if SCRIPT_VGUI_SAVERESTORE
		g_pScriptVM->GenerateUniqueKey( "", m_pszScriptId, sizeof(m_pszScriptId) );
		g_pScriptVM->SetInstanceUniqeId( m_hScriptInstance, m_pszScriptId );
#endif

		if ( parent )
		{
			IScriptVGUIObject *obj = ToScriptVGUIObj( parent );
			if ( obj )
			{
				// Insert this after the parent to make sure children come after their parents,
				// and their removal is done inside ResolveChildren_r(), not by individual Destroy() calls from LevelShutdown.
				unsigned short parentIdx = g_ScriptPanels.Find( obj );

				// My parent can't not be in the list.
				Assert( parentIdx != g_ScriptPanels.InvalidIndex() && g_ScriptPanels.IsInList( parentIdx ) );

				g_ScriptPanels.InsertAfter( parentIdx, this );

				_base->SetParent( obj->GetVPanel() );
				return;
			}

			AssertMsg( 0, "invalid parent" );

			g_ScriptPanels.AddToTail( this );

			// leave me parentless
			return;
		}

		g_ScriptPanels.AddToTail( this );

		// Script specified root panel - a cheap alternative to registering uneditable panel instances.
		// Match the values to vscript_vgui.nut.
		//
		// This parameter is hidden in script, and is defined by the return value of dummy functions.
		VPANEL vparent = 0;

		switch ( root )
		{
	#if ALLOW_ROOT_PANEL_PARENT
			case 0:
				vparent = VGUI_GetScriptRootPanel( PANEL_ROOT );
				break;
	#endif
	#if ALLOW_GAMEUI_ROOT_PARENT
			case 1:
				vparent = VGUI_GetScriptRootPanel( PANEL_GAMEUIDLL );
				break;
	#endif
	#if ALLOW_CLIENTDLL_ROOT_PARENT
			case 2:
				vparent = VGUI_GetScriptRootPanel( PANEL_CLIENTDLL );
				break;
	#endif
	#if ALLOW_HUD_VIEWPORT_ROOT_PARENT
			case 10: // Hud viewport
				Assert( g_pClientMode && g_pClientMode->GetViewport() );
				vparent = g_pClientMode->GetViewport()->GetVPanel();
				break;
	#endif
			default:
	#if SCRIPT_ENGINE_ROOT_PANELS
				UNREACHABLE(); // Invalid parent panel
	#else
				// Allow everything defined in vscript_vgui.nut
				vparent = VGUI_GetScriptRootPanel( (VGuiPanel_t)root );
	#endif
		}

		_base->SetParent( vparent );
	}
};

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE_ROOT( Panel )
{
public:
	void Destroy()
	{
		CScriptVGUIObject<T>::Destroy();
	}

	void MakeReadyForUse()
	{
		__base()->MakeReadyForUse();
	}

	const char *GetName()
	{
		return __base()->GetName();
	}

	void AddTickSignal( int i )
	{
		ivgui()->AddTickSignal( this->GetVPanel(), i );
	}

	void RemoveTickSignal()
	{
		ivgui()->RemoveTickSignal( this->GetVPanel() );
	}
#if SCRIPT_VGUI_SIGNAL_INTERFACE
	void AddActionSignalTarget( HSCRIPT messageTarget )
	{
		IScriptVGUIObject *obj = ToScriptVGUIObj( messageTarget );
		if ( obj )
		{
			__base()->AddActionSignalTarget( obj->GetVPanel() );
		}
	}
#endif
	//-----------------------------------------------------
	// Get script created parent
	//-----------------------------------------------------
	HSCRIPT	GetParent()
	{
		VPANEL parent = ipanel()->GetParent( this->GetVPanel() );
		if ( !parent )
			return NULL;

		int i;
		IScriptVGUIObject* obj = FindInScriptPanels( parent, i );
		if ( obj )
		{
			// My parent can't be invalid.
			Assert( ToScriptVGUIObj( obj->GetScriptInstance() ) );

			return obj->GetScriptInstance();
		}

#ifdef _DEBUG
		// Is my parent one of the root panels?
		bool bRootParent = false;
#if SCRIPT_ENGINE_ROOT_PANELS
		if ( ( parent == g_pScriptRootPanel->GetVPanel() )
	#if ALLOW_GAMEUI_ROOT_PARENT
			|| ( g_pScriptGameUIDLLPanel && parent == g_pScriptGameUIDLLPanel->GetVPanel() )
	#endif
	#if ALLOW_CLIENTDLL_ROOT_PARENT
			|| ( g_pScriptClientDLLPanel && parent == g_pScriptClientDLLPanel->GetVPanel() )
	#endif
		)
		{
			bRootParent = true;
		}
		else
#endif
		for ( int i = PANEL_ROOT; i <= PANEL_CLIENTDLL_TOOLS; ++i )
		{
			if ( parent == enginevgui->GetPanel( (VGuiPanel_t)i ) )
			{
				bRootParent = true;
				break;
			}
		}
#if ALLOW_HUD_VIEWPORT_ROOT_PARENT
		if ( g_pClientMode && g_pClientMode->GetViewport() && ( parent == g_pClientMode->GetViewport()->GetVPanel() ) )
			bRootParent = true;
#endif
		// My parent wasn't registered.
		AssertMsg1( bRootParent, "'%s'", ipanel()->GetName(parent) );
#endif

		return NULL;
	}

	//-----------------------------------------------------
	// Set script created parent
	//-----------------------------------------------------
	void SetParent( HSCRIPT parent )
	{
		if ( !parent )
		{
			__base()->SetParent( (VPANEL)NULL );
			return;
		}

		IScriptVGUIObject *obj = ToScriptVGUIObj( parent );
		if ( obj )
		{
			__base()->SetParent( obj->GetVPanel() );
			return;
		}

		AssertMsg( 0, "invalid parent" );
	}

	void GetChildren( HSCRIPT arr )
	{
		CUtlVector< VPANEL > &children = ipanel()->GetChildren( this->GetVPanel() );
		FOR_EACH_VEC( children, i )
		{
			VPANEL child = children[i];
			int j;
			IScriptVGUIObject* obj = FindInScriptPanels( child, j );
			if ( obj )
			{
				g_pScriptVM->ArrayAppend( arr, obj->GetScriptInstance() );
			}
			// Beware of dangling pointers if C++ created children are to be registered
		}
	}

	int GetXPos()
	{
		int x, y;
		ipanel()->GetPos( this->GetVPanel(), x, y );
		return x;
	}

	int GetYPos()
	{
		int x, y;
		ipanel()->GetPos( this->GetVPanel(), x, y );
		return y;
	}

	void SetPos( int x, int y )
	{
		ipanel()->SetPos( this->GetVPanel(), x, y );
	}

	void SetZPos( int i )
	{
		ipanel()->SetZPos( this->GetVPanel(), i );
	}

	int GetZPos()
	{
		return ipanel()->GetZPos( this->GetVPanel() );
	}

	void SetSize( int w, int t )
	{
		ipanel()->SetSize( this->GetVPanel(), w, t );
	}

	void SetWide( int w )
	{
		ipanel()->SetSize( this->GetVPanel(), w, GetTall() );
	}

	int GetWide()
	{
		int w, t;
		ipanel()->GetSize( this->GetVPanel(), w, t );
		return w;
	}

	void SetTall( int t )
	{
		ipanel()->SetSize( this->GetVPanel(), GetWide(), t );
	}

	int GetTall()
	{
		int w, t;
		ipanel()->GetSize( this->GetVPanel(), w, t );
		return t;
	}

	int GetAlpha()
	{
		return __base()->GetAlpha();
	}

	void SetAlpha( int i )
	{
		__base()->SetAlpha( i );
	}

	void SetVisible( bool i )
	{
		ipanel()->SetVisible( this->GetVPanel(), i );
	}

	bool IsVisible()
	{
		return ipanel()->IsVisible( this->GetVPanel() );
	}
#if BUILD_GROUPS_ENABLED
	void SetProportional( bool i )
	{
		__base()->SetProportional(i);
	}
#endif
#if 0
	void LocalToScreen( HSCRIPT out )
	{
		int px, py;
		ipanel()->GetAbsPos( this->GetVPanel(), px, py );

		ScriptVariant_t x, y;
		g_pScriptVM->GetValue( out, (ScriptVariant_t)0, &x );
		g_pScriptVM->GetValue( out, 1, &y );

		g_pScriptVM->SetValue( out, (ScriptVariant_t)0, x.m_int + px );
		g_pScriptVM->SetValue( out, 1, y.m_int + py );
	}

	void ScreenToLocal( HSCRIPT out )
	{
		int px, py;
		ipanel()->GetAbsPos( this->GetVPanel(), px, py );

		ScriptVariant_t x, y;
		g_pScriptVM->GetValue( out, (ScriptVariant_t)0, &x );
		g_pScriptVM->GetValue( out, 1, &y );

		g_pScriptVM->SetValue( out, (ScriptVariant_t)0, x.m_int - px );
		g_pScriptVM->SetValue( out, 1, y.m_int - py );
	}
#endif
	bool IsWithin( int x, int y )
	{
		return __base()->IsWithin( x, y );
	}

	void SetEnabled( bool i )
	{
		__base()->SetEnabled(i);
	}

	bool IsEnabled()
	{
		return __base()->IsEnabled();
	}

	void SetPaintEnabled( bool i )
	{
		__base()->SetPaintEnabled(i);
	}

	void SetPaintBackgroundEnabled( bool i )
	{
		__base()->SetPaintBackgroundEnabled(i);
	}

	void SetPaintBorderEnabled( bool i )
	{
		__base()->SetPaintBorderEnabled(i);
	}

	void SetPostChildPaintEnabled( bool i )
	{
		__base()->SetPostChildPaintEnabled(i);
	}

	// 0 for normal(opaque), 1 for single texture from Texture1, and 2 for rounded box w/ four corner textures
	void SetPaintBackgroundType( int i )
	{
		__base()->SetPaintBackgroundType(i);
	}

	void SetFgColor( int r, int g, int b, int a )
	{
		__base()->SetFgColor( Color( r, g, b, a ) );
	}

	void SetBgColor( int r, int g, int b, int a )
	{
		__base()->SetBgColor( Color( r, g, b, a ) );
	}
#if 0
	void SetScheme( const char *tag )
	{
		return __base()->SetScheme( tag );
	}
#endif
	void SetCursor( int cursor )
	{
		AssertMsg( cursor >= 0 && cursor < dc_last, "invalid cursor" );

		// do nothing
		if ( cursor < 0 || cursor >= dc_last )
			return;

		return __base()->SetCursor( (HCursor)cursor );
	}

	bool IsCursorOver()
	{
		return __base()->IsCursorOver();
	}

	bool HasFocus()
	{
		return __base()->HasFocus();
	}

	void RequestFocus()
	{
		__base()->RequestFocus();
	}

	void MakePopup()
	{
		__base()->MakePopup();
	}

	void MoveToFront()
	{
		__base()->MoveToFront();
	}

	void SetMouseInputEnabled( bool i )
	{
		__base()->SetMouseInputEnabled(i);
	}

	void SetKeyBoardInputEnabled( bool i )
	{
		__base()->SetKeyBoardInputEnabled(i);
	}

	// -----------------------
	// Drawing utility
	// -----------------------
	//void SetRoundedCorners( int cornerFlags )
	//{
	//	__base()->SetRoundedCorners( cornerFlags & 0xff );
	//}

	void DrawBox( int x, int y, int wide, int tall, int r, int g, int b, int a, bool hollow )
	{
		__base()->DrawBox( x, y, wide, tall, Color(r, g, b, a), 1.0f, hollow );
	}

	void DrawBoxFade( int x, int y, int wide, int tall, int r, int g, int b, int a, int alpha0, int alpha1, bool bHorizontal, bool hollow )
	{
		__base()->DrawBoxFade( x, y, wide, tall, Color(r, g, b, a), 1.0f, alpha0, alpha1, bHorizontal, hollow );
	}
#if 0
	// -----------------------
	// drag drop
	// -----------------------
	void SetDragEnabled( bool i )
	{
		__base()->SetDragEnabled(i);
	}

	bool IsDragEnabled()
	{
		return __base()->IsDragEnabled();
	}

	void SetDropEnabled( bool i )
	{
		__base()->SetDropEnabled( i, 0.0f );
	}

	bool IsDropEnabled()
	{
		return __base()->IsDropEnabled();
	}

	void SetShowDragHelper( int i )
	{
		__base()->SetShowDragHelper(i);
	}

	int GetDragStartTolerance()
	{
		return __base()->GetDragStartTolerance();
	}

	void SetDragStartTolerance( int i )
	{
		__base()->SetDragSTartTolerance(i);
	}
#endif
#if 0
	void SetTooltip( const char *text )
	{
		__base()->GetTooltip()->SetText( text );
	}

	void SetTooltipDelay( int delay )
	{
		__base()->GetTooltip()->SetTooltipDelay( delay );
	}
#endif
};

#define DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_VGUI_SCRIPT_INSTANCE_HELPER()\
\
	DEFINE_SCRIPTFUNC( Destroy, "" )\
	DEFINE_SCRIPTFUNC( MakeReadyForUse, "" )\
	DEFINE_SCRIPTFUNC( GetName, "" )\
	DEFINE_SCRIPTFUNC( AddTickSignal, "" )\
	DEFINE_SCRIPTFUNC( RemoveTickSignal, "" )\
\
	DEFINE_SCRIPTFUNC( GetParent, "" )\
	DEFINE_SCRIPTFUNC( SetParent, "" )\
	DEFINE_SCRIPTFUNC( GetChildren, "" )\
\
	DEFINE_SCRIPTFUNC( GetXPos, "" )\
	DEFINE_SCRIPTFUNC( GetYPos, "" )\
	DEFINE_SCRIPTFUNC( SetPos, "" )\
\
	DEFINE_SCRIPTFUNC( GetZPos, "" )\
	DEFINE_SCRIPTFUNC( SetZPos, "" )\
\
	DEFINE_SCRIPTFUNC( SetSize, "" )\
	DEFINE_SCRIPTFUNC( GetWide, "" )\
	DEFINE_SCRIPTFUNC( SetWide, "" )\
\
	DEFINE_SCRIPTFUNC( GetTall, "" )\
	DEFINE_SCRIPTFUNC( SetTall, "" )\
\
	DEFINE_SCRIPTFUNC( GetAlpha, "" )\
	DEFINE_SCRIPTFUNC( SetAlpha, "" )\
\
	DEFINE_SCRIPTFUNC( SetVisible, "" )\
	DEFINE_SCRIPTFUNC( IsVisible, "" )\
\
	DEFINE_SCRIPTFUNC( IsWithin, "" )\
\
	DEFINE_SCRIPTFUNC( SetEnabled, "" )\
	DEFINE_SCRIPTFUNC( IsEnabled, "" )\
\
	DEFINE_SCRIPTFUNC( SetPaintEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetPaintBackgroundEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetPaintBorderEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetPostChildPaintEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetPaintBackgroundType, "" )\
\
	DEFINE_SCRIPTFUNC( SetFgColor, "" )\
	DEFINE_SCRIPTFUNC( SetBgColor, "" )\
\
	DEFINE_SCRIPTFUNC( SetCursor, "" )\
	DEFINE_SCRIPTFUNC( IsCursorOver, "" )\
\
	DEFINE_SCRIPTFUNC( HasFocus, "" )\
	DEFINE_SCRIPTFUNC( RequestFocus, "" )\
	DEFINE_SCRIPTFUNC( MakePopup, "" )\
	DEFINE_SCRIPTFUNC( MoveToFront, "" )\
\
	DEFINE_SCRIPTFUNC( SetMouseInputEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetKeyBoardInputEnabled, "" )\
\
	DEFINE_SCRIPTFUNC( DrawBox, "" )\
	DEFINE_SCRIPTFUNC( DrawBoxFade, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------
// These need more testing.
// TODO: DECLARE_BUILD_FACTORY_SCRIPT() to create overridable script panels from controls file
#if BUILD_GROUPS_ENABLED
CLASS_HELPER_INTERFACE( EditablePanel, Panel )
{
public:
	// Call on creation or on ApplySchemeSettings()
	void LoadControlSettings( const char *resName )
	{
		__base()->LoadControlSettings( resName );
	}

	HSCRIPT FindChildByName( const char *childName )
	{
		Panel *pPanel = __base()->FindChildByName( childName, false );
		if ( pPanel )
		{
			int i;
			IScriptVGUIObject* obj = FindInScriptPanels( child, i );
			if ( obj )
			{
				return obj->GetScriptInstance();
			}
		}
		return NULL;
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_EditablePanel()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( LoadControlSettings, "" )\
	DEFINE_SCRIPTFUNC( FindChildByName, "" )
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( Label, Panel )
{
public:
	void SetText( const char *text )
	{
		wchar_t wcs[512];
		g_pVGuiLocalize->ConvertANSIToUnicode( text, wcs, sizeof(wcs) );
		__base()->SetText( wcs );
	}

	void SetFont( int i )
	{
		__base()->SetFont( IntToFontHandle(i) );
	}

	void SetAllCaps( bool i )
	{
		__base()->SetAllCaps(i);
	}

	void SetWrap( bool i )
	{
		__base()->SetWrap(i);
	}

	void SetCenterWrap( bool i )
	{
		__base()->SetCenterWrap(i);
	}

	void SetContentAlignment( int i )
	{
		__base()->SetContentAlignment( (Label::Alignment)i );
	}

	void SetTextInset( int x, int y )
	{
		__base()->SetTextInset( x, y );
	}

	void SizeToContents()
	{
		__base()->SizeToContents();
	}

	void SetAssociatedControl( HSCRIPT control )
	{
		IScriptVGUIObject *obj = ToScriptVGUIObj( control );
		if ( obj )
		{
			__base()->SetAssociatedControl( ipanel()->GetPanel( obj->GetVPanel(), GetControlsModuleName() ) );
		}
	}

	void AddColorChange( int r, int g, int b, int a, int iTextStreamIndex )
	{
		__base()->GetTextImage()->AddColorChange( Color( r, g, b, a ), iTextStreamIndex );
	}

	void ClearColorChangeStream()
	{
		__base()->GetTextImage()->ClearColorChangeStream();
	}
#if 0
	void SetTextImageIndex( int index )
	{
		__base()->SetTextImageIndex( index );
	}

	void SetImageAtIndex( int index, const char *imageName, bool hardwareFilter, int offset )
	{
		return __base()->SetImageAtIndex( index, vgui_GetImage( imageName, hardwareFilter ), offset );
	}

	int AddImage( const char *imageName, bool hardwareFilter, int offset )
	{
		return __base()->AddImage( vgui_GetImage( imageName, hardwareFilter ), offset );
	}
#endif
};

#define DEFINE_VGUI_SCRIPTFUNC_Label()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetText, "" )\
	DEFINE_SCRIPTFUNC( SetFont, "" )\
	DEFINE_SCRIPTFUNC( SetAllCaps, "" )\
	DEFINE_SCRIPTFUNC( SetWrap, "" )\
	DEFINE_SCRIPTFUNC( SetCenterWrap, "" )\
	DEFINE_SCRIPTFUNC( SetContentAlignment, "" )\
	DEFINE_SCRIPTFUNC( SetTextInset, "" )\
	DEFINE_SCRIPTFUNC( SizeToContents, "" )\
	DEFINE_SCRIPTFUNC( SetAssociatedControl, "" )\
	DEFINE_SCRIPTFUNC( AddColorChange, "" )\
	DEFINE_SCRIPTFUNC( ClearColorChangeStream, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( Button, Label )
{
public:
#if SCRIPT_VGUI_SIGNAL_INTERFACE
	// Sets the command message to send to the action signal target when the button is pressed
	void SetCommand( const char *command )
	{
		if ( !V_strnicmp( command, "url ", 4 ) )
		{
			__base()->SetCommand( (KeyValues*)NULL );

			g_pScriptVM->RaiseException("invalid button command");
			return;
		}

		__base()->SetCommand( command );
	}
#endif
	void SetButtonActivationType( int activationType )
	{
		__base()->SetButtonActivationType( (Button::ActivationType_t)activationType );
	}

	bool IsArmed()
	{
		return __base()->IsArmed();
	}

	void SetArmed( bool state )
	{
		__base()->SetArmed(state);
	}

	bool IsSelected()
	{
		return __base()->IsSelected();
	}

	void SetSelected( bool state )
	{
		__base()->SetSelected(state);
	}

	bool IsDepressed()
	{
		return __base()->IsDepressed();
	}

	void ForceDepressed( bool state )
	{
		__base()->ForceDepressed(state);
	}

	void SetMouseClickEnabled( int code, bool state )
	{
		__base()->SetMouseClickEnabled( (MouseCode)code, state );
	}

	bool IsMouseClickEnabled( int code )
	{
		return __base()->IsMouseClickEnabled( (MouseCode)code );
	}

	void SetDefaultColor( int fr, int fg, int fb, int fa, int br, int bg, int bb, int ba )
	{
		__base()->SetDefaultColor( Color(fr, fg, fb, fa), Color(br, bg, bb, ba) );
	}

	void SetArmedColor( int fr, int fg, int fb, int fa, int br, int bg, int bb, int ba )
	{
		__base()->SetArmedColor( Color(fr, fg, fb, fa), Color(br, bg, bb, ba) );
	}

	void SetSelectedColor( int fr, int fg, int fb, int fa, int br, int bg, int bb, int ba )
	{
		__base()->SetSelectedColor( Color(fr, fg, fb, fa), Color(br, bg, bb, ba) );
	}

	void SetDepressedColor( int fr, int fg, int fb, int fa, int br, int bg, int bb, int ba )
	{
		__base()->SetDepressedColor( Color(fr, fg, fb, fa), Color(br, bg, bb, ba) );
	}

	void SetArmedSound( const char *sound )
	{
		__base()->SetArmedSound( sound );
	}

	void SetDepressedSound( const char *sound )
	{
		__base()->SetDepressedSound( sound );
	}

	void SetReleasedSound( const char *sound )
	{
		__base()->SetReleasedSound( sound );
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_Button()\
	DEFINE_VGUI_SCRIPTFUNC_Label()\
	DEFINE_SCRIPTFUNC( SetButtonActivationType, "" )\
	DEFINE_SCRIPTFUNC( IsArmed, "" )\
	DEFINE_SCRIPTFUNC( SetArmed, "" )\
	DEFINE_SCRIPTFUNC( IsSelected, "" )\
	DEFINE_SCRIPTFUNC( SetSelected, "" )\
	DEFINE_SCRIPTFUNC( IsDepressed, "" )\
	DEFINE_SCRIPTFUNC( ForceDepressed, "" )\
	DEFINE_SCRIPTFUNC( SetMouseClickEnabled, "" )\
	DEFINE_SCRIPTFUNC( IsMouseClickEnabled, "" )\
	DEFINE_SCRIPTFUNC( SetDefaultColor, "" )\
	DEFINE_SCRIPTFUNC( SetArmedColor, "" )\
	DEFINE_SCRIPTFUNC( SetSelectedColor, "" )\
	DEFINE_SCRIPTFUNC( SetDepressedColor, "" )\
	DEFINE_SCRIPTFUNC( SetArmedSound, "" )\
	DEFINE_SCRIPTFUNC( SetDepressedSound, "" )\
	DEFINE_SCRIPTFUNC( SetReleasedSound, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( ImagePanel, Panel )
{
public:
	void SetImage( const char *imageName, bool hardwareFilter )
	{
		__base()->EvictImage();
		__base()->SetImage( vgui_GetImage( imageName, hardwareFilter ) );
	}

	void SetDrawColor( int r, int g, int b, int a )
	{
		__base()->SetDrawColor( Color( r, g, b, a ) );
	}

	void SetTileImage( bool bTile )
	{
		__base()->SetTileImage( bTile );
	}

	void SetShouldScaleImage( bool state )
	{
		__base()->SetShouldScaleImage( state );
	}

	void SetRotation( int rotation )
	{
		Assert( rotation == ROTATED_UNROTATED ||
			rotation == ROTATED_CLOCKWISE_90 ||
			rotation == ROTATED_ANTICLOCKWISE_90 ||
			rotation == ROTATED_FLIPPED );

		__base()->SetRotation( rotation );
	}
#if 0
	void SetFrame( int nFrame )
	{
		__base()->SetFrame( nFrame );
	}
#endif
};

#define DEFINE_VGUI_SCRIPTFUNC_ImagePanel()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetImage, "" )\
	DEFINE_SCRIPTFUNC( SetDrawColor, "" )\
	DEFINE_SCRIPTFUNC( SetTileImage, "" )\
	DEFINE_SCRIPTFUNC( SetShouldScaleImage, "" )\
	DEFINE_SCRIPTFUNC( SetRotation, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( Frame, Panel )
{
public:
	void SetMinimumSize( int wide, int tall )
	{
		__base()->SetMinimumSize( wide, tall );
	}

	void SetTitle( const char* titel )
	{
		__base()->SetTitle( titel, false );
	}

	void Close()
	{
		__base()->Close();
	}

	void SetDeleteSelfOnClose( bool state )
	{
		__base()->SetDeleteSelfOnClose( state );
	}

	void SetMoveable( bool state )
	{
		__base()->SetMoveable( state );
	}

	void SetSizeable( bool state )
	{
		__base()->SetSizeable( state );
	}

	void SetCloseButtonVisible( bool state )
	{
		__base()->SetCloseButtonVisible( state );
	}

	void SetTitleBarVisible( bool state )
	{
		__base()->SetTitleBarVisible( state );
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_Frame()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetMinimumSize, "" )\
	DEFINE_SCRIPTFUNC( SetTitle, "" )\
	DEFINE_SCRIPTFUNC( Close, "" )\
	DEFINE_SCRIPTFUNC( SetDeleteSelfOnClose, "" )\
	DEFINE_SCRIPTFUNC( SetMoveable, "" )\
	DEFINE_SCRIPTFUNC( SetSizeable, "" )\
	DEFINE_SCRIPTFUNC( SetCloseButtonVisible, "" )\
	DEFINE_SCRIPTFUNC( SetTitleBarVisible, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( RichText, Panel )
{
public:
	void SetText( const char* text )
	{
		__base()->SetText( text );
	}

	void SetFont( int font )
	{
		__base()->SetFont( IntToFontHandle(font) );
	}

	void InsertString( const char* text )
	{
		__base()->InsertString( text );
	}

	void SetPanelInteractive( bool bInteractive )
	{
		__base()->SetPanelInteractive( bInteractive );
	}

	void SetUnusedScrollbarInvisible( bool bInvis )
	{
		__base()->SetUnusedScrollbarInvisible( bInvis );
	}

	void GotoTextStart()
	{
		__base()->GotoTextStart();
	}

	void GotoTextEnd()
	{
		__base()->GotoTextEnd();
	}

	void SetMaximumCharCount( int maxChars )
	{
		__base()->SetMaximumCharCount( maxChars );
	}

	void InsertColorChange( int r, int g, int b, int a )
	{
		__base()->InsertColorChange( Color( r, g, b, a ) );
	}

	int GetNumLines()
	{
		return __base()->GetNumLines();
	}

	void SetDrawTextOnly()
	{
		__base()->SetDrawTextOnly();
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_RichText()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetText, "" )\
	DEFINE_SCRIPTFUNC( SetFont, "" )\
	DEFINE_SCRIPTFUNC( InsertString, "" )\
	DEFINE_SCRIPTFUNC( SetPanelInteractive, "" )\
	DEFINE_SCRIPTFUNC( SetUnusedScrollbarInvisible, "" )\
	DEFINE_SCRIPTFUNC( GotoTextStart, "" )\
	DEFINE_SCRIPTFUNC( GotoTextEnd, "" )\
	DEFINE_SCRIPTFUNC( SetMaximumCharCount, "" )\
	DEFINE_SCRIPTFUNC( InsertColorChange, "" )\
	DEFINE_SCRIPTFUNC( GetNumLines, "" )\
	DEFINE_SCRIPTFUNC( SetDrawTextOnly, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------

CLASS_HELPER_INTERFACE( TextEntry, Panel )
{
public:
	void SetText( const char* text )
	{
		wchar_t wcs[512];
		g_pVGuiLocalize->ConvertANSIToUnicode( text, wcs, sizeof(wcs) );
		__base()->SetText( wcs );
	}

	const char *GetText()
	{
		static char sz[512];
		__base()->GetText( sz, sizeof(sz) );
		return sz;
	}

	void SetFont( int font )
	{
		__base()->SetFont( IntToFontHandle(font) );
	}

	void SetEditable( bool state )
	{
		__base()->SetEditable( state );
	}

	void GotoTextStart()
	{
		__base()->GotoTextStart();
	}

	void GotoTextEnd()
	{
		__base()->GotoTextEnd();
	}

	void InsertString( const char* text )
	{
		__base()->InsertString( text );
	}

	void SelectNone()
	{
		__base()->SelectNone();
	}

	void SetMultiline( bool state )
	{
		__base()->SetMultiline( state );
	}

	void SetVerticalScrollbar( bool state )
	{
		__base()->SetVerticalScrollbar( state );
	}
#if 0
	void SetHorizontalScrolling( bool status )
	{
		__base()->SetHorizontalScrolling( status );
	}
#endif
	void SetCatchEnterKey( bool state )
	{
		__base()->SetCatchEnterKey( state );
	}

	void SetMaximumCharCount( int maxChars )
	{
		__base()->SetMaximumCharCount( maxChars );
	}
#if 0
	void SetWrap( bool wrap )
	{
		__base()->SetWrap( wrap );
	}
#endif
	void SetAllowNumericInputOnly( bool state )
	{
		__base()->SetAllowNumericInputOnly( state );
	}
#if 0
	void SetDisabledBgColor( int r, int g, int b, int a )
	{
		__base()->SetDisabledBgColor( Color( r, g, b, a ) );
	}

	void SetSelectionTextColor( int r, int g, int b, int a )
	{
		__base()->SetSelectionTextColor( Color( r, g, b, a ) );
	}

	void SetSelectionBgColor( int r, int g, int b, int a )
	{
		__base()->SetSelectionBgColor( Color( r, g, b, a ) );
	}

	void SetSelectionUnfocusedBgColor( int r, int g, int b, int a )
	{
		__base()->SetSelectionUnfocusedBgColor( Color( r, g, b, a ) );
	}
#endif
};

#define DEFINE_VGUI_SCRIPTFUNC_TextEntry()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetText, "" )\
	DEFINE_SCRIPTFUNC( GetText, "" )\
	DEFINE_SCRIPTFUNC( SetFont, "" )\
	DEFINE_SCRIPTFUNC( SetEditable, "" )\
	DEFINE_SCRIPTFUNC( GotoTextStart, "" )\
	DEFINE_SCRIPTFUNC( GotoTextEnd, "" )\
	DEFINE_SCRIPTFUNC( InsertString, "" )\
	DEFINE_SCRIPTFUNC( SelectNone, "" )\
	DEFINE_SCRIPTFUNC( SetMultiline, "" )\
	DEFINE_SCRIPTFUNC( SetVerticalScrollbar, "" )\
	DEFINE_SCRIPTFUNC( SetCatchEnterKey, "" )\
	DEFINE_SCRIPTFUNC( SetMaximumCharCount, "" )\
	DEFINE_SCRIPTFUNC( SetAllowNumericInputOnly, "" )\

//--------------------------------------------------------------
//--------------------------------------------------------------
#if !defined(NO_STEAM)
CLASS_HELPER_INTERFACE( AvatarImage, Panel )
{
public:
	void SetPlayer( const char *steam2id, int eAvatarSize )
	{
		uint32 __SteamInstanceID;
		uint32 __SteamLocalUserID_Low32Bits;
		uint32 __SteamLocalUserID_High32Bits;

		int c = sscanf( steam2id, "STEAM_%u:%u:%u",
			&__SteamInstanceID, &__SteamLocalUserID_High32Bits, &__SteamLocalUserID_Low32Bits );

		if ( c < 3 )
			return;

		CSteamID id( __SteamLocalUserID_Low32Bits * 2 + __SteamLocalUserID_High32Bits,
			k_EUniversePublic,
			k_EAccountTypeIndividual );

		__base()->SetPlayer( id, (EAvatarSize)eAvatarSize );
	}

	void SetPlayerByIndex( int entindex, int eAvatarSize )
	{
		if ( !entindex )
		{
			__base()->ClearAvatar();
			return;
		}

		__base()->SetPlayer( entindex, (EAvatarSize)eAvatarSize );
	}

	void SetDefaultAvatar( const char *imageName )
	{
		__base()->SetDefaultAvatar( vgui_GetImage( imageName, false ) );
	}

	void SetShouldScaleImage( bool state )
	{
		__base()->SetShouldScaleImage( state );
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_AvatarImage()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetPlayer, "" )\
	DEFINE_SCRIPTFUNC( SetPlayerByIndex, "" )\
	DEFINE_SCRIPTFUNC( SetDefaultAvatar, "" )\
	DEFINE_SCRIPTFUNC( SetShouldScaleImage, "" )
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------
#if VGUI_TGA_IMAGE_PANEL
CLASS_HELPER_INTERFACE( TGAImage, Panel )
{
public:
	void SetImage( const char *p )
	{
		__base()->SetTGAImage( p );
	}

	void SetDrawColor( int r, int g, int b, int a )
	{
		__base()->SetDrawColor( r, g, b, a );
	}

	void SetShouldScaleImage( bool i )
	{
		__base()->SetShouldScaleImage( i );
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_TGAImage()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetImage, "" )\
	DEFINE_SCRIPTFUNC( SetDrawColor, "" )\
	DEFINE_SCRIPTFUNC( SetShouldScaleImage, "" )
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------
#if 0
CLASS_HELPER_INTERFACE( PNGImage, Panel )
{
public:
	void SetImage( const char *p )
	{
		__base()->SetPNGImage( p );
	}

	void SetDrawColor( int r, int g, int b, int a )
	{
		__base()->SetDrawColor( r, g, b, a );
	}

	void SetShouldScaleImage( bool i )
	{
		__base()->SetShouldScaleImage( i );
	}
};

#define DEFINE_VGUI_SCRIPTFUNC_PNGImage()\
	DEFINE_VGUI_SCRIPTFUNC_Panel()\
	DEFINE_SCRIPTFUNC( SetImage, "" )\
	DEFINE_SCRIPTFUNC( SetDrawColor, "" )\
	DEFINE_SCRIPTFUNC( SetShouldScaleImage, "" )
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------

//--------------------------------------------------------------
//--------------------------------------------------------------


//==============================================================
//==============================================================


static inline void SetHScript( HSCRIPT &var, HSCRIPT val )
{
	if ( var && g_pScriptVM )
		g_pScriptVM->ReleaseScript( var );
	var = val;
}

#define CheckCallback(s)\
	if ( !V_strcmp( cb, #s ) )\
	{\
		SetHScript( m_hfn##s, fn );\
		return;\
	}

//--------------------------------------------------------
// C++ objects for vgui overrides and messages.
//--------------------------------------------------------


class CScript_Panel : public Panel
{
	DECLARE_SCRIPTVGUI_CLASS( Panel );

private:
	HSCRIPT m_hfnPaint;
	HSCRIPT m_hfnPaintBackground;
	HSCRIPT m_hfnPostChildPaint;

	HSCRIPT m_hfnPerformLayout;
	HSCRIPT m_hfnOnTick;
	HSCRIPT m_hfnOnScreenSizeChanged;

	HSCRIPT m_hfnOnCursorEntered;
	HSCRIPT m_hfnOnCursorExited;
	HSCRIPT m_hfnOnCursorMoved;

	HSCRIPT m_hfnOnMousePressed;
	HSCRIPT m_hfnOnMouseDoublePressed;
	HSCRIPT m_hfnOnMouseReleased;
	HSCRIPT m_hfnOnMouseWheeled;

	HSCRIPT m_hfnOnKeyCodePressed;
	HSCRIPT m_hfnOnKeyCodeReleased;
	HSCRIPT m_hfnOnKeyCodeTyped;

#if SCRIPT_VGUI_SIGNAL_INTERFACE
	HSCRIPT m_hfnOnCommand;
#endif

public:
	CScript_Panel( Panel *parent, const char *name ) :
		BaseClass( parent, name ),

		m_hfnPaint(NULL),
		m_hfnPaintBackground(NULL),
		m_hfnPostChildPaint(NULL),

		m_hfnPerformLayout(NULL),
		m_hfnOnTick(NULL),
		m_hfnOnScreenSizeChanged(NULL),
#if SCRIPT_VGUI_SIGNAL_INTERFACE
		m_hfnOnCommand(NULL),
#endif
		m_hfnOnCursorEntered(NULL),
		m_hfnOnCursorExited(NULL),
		m_hfnOnCursorMoved(NULL),

		m_hfnOnMousePressed(NULL),
		m_hfnOnMouseDoublePressed(NULL),
		m_hfnOnMouseReleased(NULL),
		m_hfnOnMouseWheeled(NULL),

		m_hfnOnKeyCodePressed(NULL),
		m_hfnOnKeyCodeReleased(NULL),
		m_hfnOnKeyCodeTyped(NULL)
	{}

	void ScriptShutdown()
	{
		ivgui()->RemoveTickSignal( GetVPanel() );

		SetHScript( m_hfnPaint, NULL );
		SetHScript( m_hfnPaintBackground, NULL );
		SetHScript( m_hfnPostChildPaint, NULL );

		SetHScript( m_hfnPerformLayout, NULL );
		SetHScript( m_hfnOnTick, NULL );
		SetHScript( m_hfnOnScreenSizeChanged, NULL );

		SetHScript( m_hfnOnCursorEntered, NULL );
		SetHScript( m_hfnOnCursorExited, NULL );
		SetHScript( m_hfnOnCursorMoved, NULL );

		SetHScript( m_hfnOnMousePressed, NULL );
		SetHScript( m_hfnOnMouseDoublePressed, NULL );
		SetHScript( m_hfnOnMouseReleased, NULL );
		SetHScript( m_hfnOnMouseWheeled, NULL );

		SetHScript( m_hfnOnKeyCodePressed, NULL );
		SetHScript( m_hfnOnKeyCodeReleased, NULL );
		SetHScript( m_hfnOnKeyCodeTyped, NULL );

#if SCRIPT_VGUI_SIGNAL_INTERFACE
		SetHScript( m_hfnOnCommand, NULL );
#endif
	}

public:
	void Paint()
	{
		g_pScriptVM->ExecuteFunction( m_hfnPaint, NULL, 0, NULL, NULL, true );
	}

	void PaintBackground()
	{
		if ( m_hfnPaintBackground )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPaintBackground, NULL, 0, NULL, NULL, true );
		}
		else
		{
			BaseClass::PaintBackground();
		}
	}

	void PostChildPaint()
	{
		g_pScriptVM->ExecuteFunction( m_hfnPostChildPaint, NULL, 0, NULL, NULL, true );
	}

	void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_hfnPerformLayout )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPerformLayout, NULL, 0, NULL, NULL, true );
		}
	}

	void OnTick()
	{
		g_pScriptVM->ExecuteFunction( m_hfnOnTick, NULL, 0, NULL, NULL, true );
	}

	void OnScreenSizeChanged( int oldwide, int oldtall )
	{
		BaseClass::OnScreenSizeChanged( oldwide, oldtall );

		if ( m_hfnOnScreenSizeChanged )
		{
			ScriptVariant_t args[2] = { oldwide, oldtall };
			g_pScriptVM->ExecuteFunction( m_hfnOnScreenSizeChanged, args, 2, NULL, NULL, true );
		}
	}
#if SCRIPT_VGUI_SIGNAL_INTERFACE
	void OnCommand( const char *command )
	{
		if ( m_hfnOnCommand )
		{
			ScriptVariant_t ret, arg = command;
			g_pScriptVM->ExecuteFunction( m_hfnOnCommand, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnCommand( command );
	}
#endif
	void OnCursorEntered()
	{
		if ( m_hfnOnCursorEntered )
		{
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorEntered, NULL, 0, NULL, NULL, true );
		}
	}

	void OnCursorExited()
	{
		if ( m_hfnOnCursorExited )
		{
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorExited, NULL, 0, NULL, NULL, true );
		}
	}

	void OnCursorMoved( int x, int y )
	{
		if ( m_hfnOnCursorMoved )
		{
			ScriptVariant_t args[2] = { x, y };
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorMoved, args, 2, NULL, NULL, true );
		}
		else
		{
			Assert( !ParentNeedsCursorMoveEvents() );
		}
	}

	void OnMousePressed( MouseCode code )
	{
		if ( m_hfnOnMousePressed )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMousePressed, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseDoublePressed( MouseCode code )
	{
		if ( m_hfnOnMouseDoublePressed )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseDoublePressed, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseReleased( MouseCode code )
	{
		if ( m_hfnOnMouseReleased )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseReleased, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseWheeled( int delta )
	{
		if ( m_hfnOnMouseWheeled )
		{
			ScriptVariant_t arg = (int)delta;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseWheeled, &arg, 1, NULL, NULL, true );
		}
	}

	void OnKeyCodePressed( KeyCode code )
	{
		if ( m_hfnOnKeyCodePressed )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodePressed, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnKeyCodePressed( code );
	}

	void OnKeyCodeReleased( KeyCode code )
	{
		if ( m_hfnOnKeyCodeReleased )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodeReleased, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnKeyCodeReleased( code );
	}

	void OnKeyCodeTyped( KeyCode code )
	{
		if ( m_hfnOnKeyCodeTyped )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodeTyped, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnKeyCodeTyped( code );
	}

public:
	void SetCallback( const char* cb, HSCRIPT fn )
	{
		CheckCallback( Paint );
		CheckCallback( PaintBackground );
		CheckCallback( PostChildPaint );

		CheckCallback( PerformLayout );
		CheckCallback( OnTick );
		CheckCallback( OnScreenSizeChanged );

		CheckCallback( OnCursorEntered );
		CheckCallback( OnCursorExited );
		CheckCallback( OnCursorMoved );

		CheckCallback( OnMousePressed );
		CheckCallback( OnMouseDoublePressed );
		CheckCallback( OnMouseReleased );
		CheckCallback( OnMouseWheeled );

		CheckCallback( OnKeyCodePressed );
		CheckCallback( OnKeyCodeReleased );
		CheckCallback( OnKeyCodeTyped );

#if SCRIPT_VGUI_SIGNAL_INTERFACE
		CheckCallback( OnCommand );
#endif

		g_pScriptVM->RaiseException("invalid callback");
	}
};

//--------------------------------------------------------------
//--------------------------------------------------------------

class CScript_Frame : public Frame
{
	DECLARE_SCRIPTVGUI_CLASS( Frame );

private:
	HSCRIPT m_hfnPaint;
	HSCRIPT m_hfnPaintBackground;

	HSCRIPT m_hfnPerformLayout;
	HSCRIPT m_hfnOnTick;
	HSCRIPT m_hfnOnScreenSizeChanged;

	HSCRIPT m_hfnOnCursorEntered;
	HSCRIPT m_hfnOnCursorExited;
	HSCRIPT m_hfnOnCursorMoved;

	HSCRIPT m_hfnOnMousePressed;
	HSCRIPT m_hfnOnMouseDoublePressed;
	HSCRIPT m_hfnOnMouseReleased;
	HSCRIPT m_hfnOnMouseWheeled;

	HSCRIPT m_hfnOnKeyCodePressed;
	HSCRIPT m_hfnOnKeyCodeReleased;
	HSCRIPT m_hfnOnKeyCodeTyped;

#if SCRIPT_VGUI_SIGNAL_INTERFACE
	HSCRIPT m_hfnOnCommand;
#endif

public:
	CScript_Frame( Panel *parent, const char *name ) :

		// Start without popup
		BaseClass( parent, name, false, false ),

		m_hfnPaint(NULL),
		m_hfnPaintBackground(NULL),

		m_hfnPerformLayout(NULL),
		m_hfnOnTick(NULL),
		m_hfnOnScreenSizeChanged(NULL),
#if SCRIPT_VGUI_SIGNAL_INTERFACE
		m_hfnOnCommand(NULL),
#endif

		m_hfnOnCursorEntered(NULL),
		m_hfnOnCursorExited(NULL),
		m_hfnOnCursorMoved(NULL),

		m_hfnOnMousePressed(NULL),
		m_hfnOnMouseDoublePressed(NULL),
		m_hfnOnMouseReleased(NULL),
		m_hfnOnMouseWheeled(NULL),

		m_hfnOnKeyCodePressed(NULL),
		m_hfnOnKeyCodeReleased(NULL),
		m_hfnOnKeyCodeTyped(NULL)
	{
		SetFadeEffectDisableOverride( true );
	}

	void ScriptShutdown()
	{
		ivgui()->RemoveTickSignal( GetVPanel() );

		SetHScript( m_hfnPaint, NULL );
		SetHScript( m_hfnPaintBackground, NULL );

		SetHScript( m_hfnPerformLayout, NULL );
		SetHScript( m_hfnOnTick, NULL );
		SetHScript( m_hfnOnScreenSizeChanged, NULL );

		SetHScript( m_hfnOnMousePressed, NULL );
		SetHScript( m_hfnOnMouseDoublePressed, NULL );
		SetHScript( m_hfnOnMouseReleased, NULL );
		SetHScript( m_hfnOnMouseWheeled, NULL );

		SetHScript( m_hfnOnKeyCodePressed, NULL );
		SetHScript( m_hfnOnKeyCodeReleased, NULL );
		SetHScript( m_hfnOnKeyCodeTyped, NULL );

#if SCRIPT_VGUI_SIGNAL_INTERFACE
		SetHScript( m_hfnOnCommand, NULL );
#endif
	}

public:
	void Paint()
	{
		g_pScriptVM->ExecuteFunction( m_hfnPaint, NULL, 0, NULL, NULL, true );
	}

	void PaintBackground()
	{
		if ( m_hfnPaintBackground )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPaintBackground, NULL, 0, NULL, NULL, true );
		}
		else
		{
			BaseClass::PaintBackground();
		}
	}

	void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_hfnPerformLayout )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPerformLayout, NULL, 0, NULL, NULL, true );
		}
	}
#if 0
	void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		if ( m_hfnApplySchemeSettings )
		{
			ScriptVariant_t arg;
			g_pScriptVM->ExecuteFunction( m_hfnApplySchemeSettings, &arg, 1, NULL, NULL, true );
		}
	}
#endif
	void OnTick()
	{
		g_pScriptVM->ExecuteFunction( m_hfnOnTick, NULL, 0, NULL, NULL, true );
	}

	void OnScreenSizeChanged( int oldwide, int oldtall )
	{
		BaseClass::OnScreenSizeChanged( oldwide, oldtall );

		if ( m_hfnOnScreenSizeChanged )
		{
			ScriptVariant_t args[2] = { oldwide, oldtall };
			g_pScriptVM->ExecuteFunction( m_hfnOnScreenSizeChanged, args, 2, NULL, NULL, true );
		}
	}
#if SCRIPT_VGUI_SIGNAL_INTERFACE
	void OnCommand( const char *command )
	{
		if ( m_hfnOnCommand )
		{
			ScriptVariant_t ret, arg = command;
			g_pScriptVM->ExecuteFunction( m_hfnOnCommand, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnCommand( command );
	}
#endif
	void OnCursorEntered()
	{
		if ( m_hfnOnCursorEntered )
		{
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorEntered, NULL, 0, NULL, NULL, true );
		}
	}

	void OnCursorExited()
	{
		if ( m_hfnOnCursorExited )
		{
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorExited, NULL, 0, NULL, NULL, true );
		}
	}

	void OnCursorMoved( int x, int y )
	{
		if ( m_hfnOnCursorMoved )
		{
			ScriptVariant_t args[2] = { x, y };
			g_pScriptVM->ExecuteFunction( m_hfnOnCursorMoved, args, 2, NULL, NULL, true );
		}
		else
		{
			Assert( !ParentNeedsCursorMoveEvents() );
		}
	}

	void OnMousePressed( MouseCode code )
	{
		BaseClass::OnMousePressed( code );

		if ( m_hfnOnMousePressed )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMousePressed, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseDoublePressed( MouseCode code )
	{
		if ( m_hfnOnMouseDoublePressed )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseDoublePressed, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseReleased( MouseCode code )
	{
		if ( m_hfnOnMouseReleased )
		{
			ScriptVariant_t arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseReleased, &arg, 1, NULL, NULL, true );
		}
	}

	void OnMouseWheeled( int delta )
	{
		if ( m_hfnOnMouseWheeled )
		{
			ScriptVariant_t arg = (int)delta;
			g_pScriptVM->ExecuteFunction( m_hfnOnMouseWheeled, &arg, 1, NULL, NULL, true );
		}
	}

	void OnKeyCodePressed( KeyCode code )
	{
		if ( m_hfnOnKeyCodePressed )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodePressed, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnKeyCodePressed( code );
	}

	void OnKeyCodeReleased( KeyCode code )
	{
		if ( m_hfnOnKeyCodeReleased )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodeReleased, &arg, 1, &ret, NULL, true );

			// Return true to swallow
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;
		}

		BaseClass::OnKeyCodeReleased( code );
	}

	void OnKeyCodeTyped( KeyCode code )
	{
		if ( m_hfnOnKeyCodeTyped )
		{
			ScriptVariant_t ret, arg = (int)code;
			g_pScriptVM->ExecuteFunction( m_hfnOnKeyCodeTyped, &arg, 1, &ret, NULL, true );

			// Return true to swallow the CanChainKeysToParent() override check and fallback,
			// which by default swallows the input.
			if ( ret.m_type == FIELD_BOOLEAN && ret.m_bool )
				return;

			if ( CanChainKeysToParent() )
			{
				BaseClass::OnKeyCodeTyped( code );
			}
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

public:
	void SetCallback( const char* cb, HSCRIPT fn )
	{
		CheckCallback( Paint );
		CheckCallback( PaintBackground );

		CheckCallback( PerformLayout );
		CheckCallback( OnTick );
		CheckCallback( OnScreenSizeChanged );

		CheckCallback( OnCursorEntered );
		CheckCallback( OnCursorExited );
		CheckCallback( OnCursorMoved );

		CheckCallback( OnMousePressed );
		CheckCallback( OnMouseDoublePressed );
		CheckCallback( OnMouseReleased );
		CheckCallback( OnMouseWheeled );

		CheckCallback( OnKeyCodePressed );
		CheckCallback( OnKeyCodeReleased );
		CheckCallback( OnKeyCodeTyped );

#if SCRIPT_VGUI_SIGNAL_INTERFACE
		CheckCallback( OnCommand );
#endif

		g_pScriptVM->RaiseException("invalid callback");
	}
};

//--------------------------------------------------------------
//--------------------------------------------------------------

class CScript_Button : public Button
{
	DECLARE_SCRIPTVGUI_CLASS( Button );

private:
	HSCRIPT m_hfnPaint;
	HSCRIPT m_hfnPaintBackground;
	HSCRIPT m_hfnDoClick;

public:
	CScript_Button( Panel *parent, const char *name, const char *text ) :
		BaseClass( parent, name, text ),

		m_hfnPaint(NULL),
		m_hfnPaintBackground(NULL),

		m_hfnDoClick(NULL)
	{}

	void ScriptShutdown()
	{
		SetHScript( m_hfnPaint, NULL );
		SetHScript( m_hfnPaintBackground, NULL );

		SetHScript( m_hfnDoClick, NULL );
	}

public:
	void Paint()
	{
		if ( m_hfnPaint )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPaint, NULL, 0, NULL, NULL, true );
		}
		else
		{
			BaseClass::Paint();
		}
	}

	void PaintBackground()
	{
		if ( m_hfnPaintBackground )
		{
			g_pScriptVM->ExecuteFunction( m_hfnPaintBackground, NULL, 0, NULL, NULL, true );
		}
		else
		{
			BaseClass::PaintBackground();
		}
	}

	void DoClick()
	{
		BaseClass::DoClick();

		if ( m_hfnDoClick )
		{
			g_pScriptVM->ExecuteFunction( m_hfnDoClick, NULL, 0, NULL, NULL, true );
		}
	}

public:
	void SetCallback( const char* cb, HSCRIPT fn )
	{
		CheckCallback( Paint );
		CheckCallback( PaintBackground );
		CheckCallback( DoClick );

		g_pScriptVM->RaiseException("invalid callback");
	}
};

//--------------------------------------------------------------
//--------------------------------------------------------------

class CScript_TextEntry : public TextEntry
{
	DECLARE_SCRIPTVGUI_CLASS( TextEntry );

private:
	HSCRIPT m_hfnTextChanged;

public:
	CScript_TextEntry( Panel *parent, const char *name ) :
		BaseClass( parent, name ),

		m_hfnTextChanged(NULL)
	{}

	void ScriptShutdown()
	{
		SetHScript( m_hfnTextChanged, NULL );
	}

public:
	//---------------------------------------------
	// On "TextMessage" message.
	// Used for responding to user input as it is typed.
	//---------------------------------------------
	void FireActionSignal()
	{
		BaseClass::FireActionSignal();

		if ( m_hfnTextChanged )
		{
			g_pScriptVM->ExecuteFunction( m_hfnTextChanged, NULL, 0, NULL, NULL, true );
		}
	}

public:
	void SetCallback( const char* cb, HSCRIPT fn )
	{
		CheckCallback( TextChanged );

		g_pScriptVM->RaiseException("invalid callback");
	}
};

//--------------------------------------------------------------
//--------------------------------------------------------------
#if !defined(NO_STEAM)
class CScript_AvatarImage : public CAvatarImagePanel
{
	DECLARE_SCRIPTVGUI_CLASS_EX( CScript_AvatarImage, CAvatarImagePanel );

public:
	CScript_AvatarImage( Panel *parent, const char *name ) :
		BaseClass( parent, name )
	{
		SetShouldDrawFriendIcon( false );
	}

	~CScript_AvatarImage()
	{
		DebugDestructor( CAvatarImagePanel );
	}

	void ScriptShutdown() {}
};
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------
#if VGUI_TGA_IMAGE_PANEL
class CTGAImagePanel : public Panel
{
	DECLARE_SCRIPTVGUI_CLASS_EX( CTGAImagePanel, Panel );

private:
	int m_iTexture;
	int m_nWidth;
	int m_nHeight;
	Color m_ImageColor;
	bool m_bScaleImage;

public:
	CTGAImagePanel( Panel *parent, const char *name ) :
		BaseClass( parent, name ),
		m_iTexture(-1),
		m_bScaleImage(0),
		m_ImageColor( 255, 255, 255, 255 )
	{
		SetPaintBackgroundEnabled( false );
	}

	~CTGAImagePanel()
	{
		DebugDestructor( CTGAImagePanel );

		if ( m_iTexture != -1 )
		{
			surface()->DestroyTextureID( m_iTexture );
		}
	}

	void ScriptShutdown() {}

public:
	void Paint()
	{
		if ( m_iTexture != -1 )
		{
			surface()->DrawSetColor( m_ImageColor );
			surface()->DrawSetTexture( m_iTexture );

			if ( m_bScaleImage )
			{
				int w, t;
				GetSize( w, t );
				surface()->DrawTexturedRect( 0, 0, w, t );
			}
			else
			{
				surface()->DrawTexturedRect( 0, 0, m_nWidth, m_nHeight );
			}
		}
		else
		{
			int w, t;
			GetSize( w, t );
			surface()->DrawSetColor( 200, 50, 150, 255 );
			surface()->DrawFilledRect( 0, 0, w, t );
		}
	}

public:
	void SetTGAImage( const char *fileName )
	{
		const char *ext = V_GetFileExtension( fileName );

		if ( ext && V_stricmp( ext, "tga" ) != 0 )
			return;

		CUtlMemory< unsigned char > tga;

		if ( TGALoader::LoadRGBA8888( fileName, tga, m_nWidth, m_nHeight ) )
		{
			if ( m_iTexture == -1 )
			{
				m_iTexture = surface()->CreateNewTextureID( true );
			}

			surface()->DrawSetTextureRGBA( m_iTexture, tga.Base(), m_nWidth, m_nHeight, false, false );
		}
		else
		{
			Warning( "Failed to load TGA image: '%s'\n", fileName );
		}
	}

	void SetDrawColor( int r, int g, int b, int a )
	{
		m_ImageColor.SetColor( r, g, b, a );
	}

	void SetShouldScaleImage( bool state )
	{
		m_bScaleImage = state;
	}
};
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------

//--------------------------------------------------------------
//--------------------------------------------------------------


//==============================================================
//==============================================================

//--------------------------------------------------------
// Script objects
//--------------------------------------------------------

DEFINE_VGUI_CLASS_EMPTY_DEFAULT_TEXT( Label )
DEFINE_VGUI_CLASS_EMPTY( ImagePanel )
DEFINE_VGUI_CLASS_EMPTY( RichText )

//--------------------------------------------------------------
//--------------------------------------------------------------

BEGIN_VGUI_HELPER( Panel )
	void SetCallback( const char *a, HSCRIPT b ) { __base()->SetCallback( a, b ); }
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( Panel )
	DEFINE_SCRIPTFUNC( SetCallback, "" )
END_SCRIPTDESC()

//--------------------------------------------------------------
//--------------------------------------------------------------

BEGIN_VGUI_HELPER( Frame )
	void SetCallback( const char *a, HSCRIPT b ) { __base()->SetCallback( a, b ); }
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( Frame )
	DEFINE_SCRIPTFUNC( SetCallback, "" )
END_SCRIPTDESC()

//--------------------------------------------------------------
//--------------------------------------------------------------

BEGIN_VGUI_HELPER_DEFAULT_TEXT( Button )
	void SetCallback( const char *a, HSCRIPT b ) { __base()->SetCallback( a, b ); }
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( Button )
	DEFINE_SCRIPTFUNC( SetCallback, "" )
END_SCRIPTDESC()

//--------------------------------------------------------------
//--------------------------------------------------------------

BEGIN_VGUI_HELPER( TextEntry )
	void SetCallback( const char *a, HSCRIPT b ) { __base()->SetCallback( a, b ); }
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( TextEntry )
	DEFINE_SCRIPTFUNC( SetCallback, "" )
END_SCRIPTDESC()

//--------------------------------------------------------------
//--------------------------------------------------------------
#if !defined(NO_STEAM)
BEGIN_VGUI_HELPER( AvatarImage )
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( AvatarImage )
END_SCRIPTDESC()
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------
#if VGUI_TGA_IMAGE_PANEL
BEGIN_VGUI_HELPER_EX( TGAImage, CTGAImagePanel )
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( TGAImage )
END_SCRIPTDESC()
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------
#if 0
BEGIN_VGUI_HELPER_EX( PNGImage, CPNGImagePanel )
END_VGUI_HELPER()

BEGIN_SCRIPTDESC_VGUI( PNGImage )
END_SCRIPTDESC()
#endif
//--------------------------------------------------------------
//--------------------------------------------------------------

//--------------------------------------------------------------
//--------------------------------------------------------------


//==============================================================
//==============================================================


struct hudelementcache_t
{
	CUtlConstString name;
	int bits;
};
CUtlVector< hudelementcache_t > m_HudElementCache;

// Check if hud elements were changed in this level to shortcut on level shutdown
bool m_bHudVisiblityChangedThisLevel = false;



class CScriptVGUI : public CAutoGameSystem
{
public:
	void LevelShutdownPostEntity();
	void Shutdown();

public:
	HSCRIPT CreatePanel( const char* panelClass, HSCRIPT parent, const char* panelName, int root );
	//void LoadSchemeFromFile( const char *filename, const char *tag );

} script_vgui;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptVGUI, "IVGui", SCRIPT_SINGLETON )
	DEFINE_SCRIPTFUNC( CreatePanel, SCRIPT_HIDE )
END_SCRIPTDESC()


HSCRIPT CScriptVGUI::CreatePanel( const char* panelClass, HSCRIPT parent, const char* panelName, int root )
{
	if ( (unsigned)g_ScriptPanels.Count() >= (unsigned)g_ScriptPanels.InvalidIndex()-1 )
	{
		Warning( "CScriptVGUI::CreatePanel() exhausted vgui panel storage!\n" );
		return NULL;
	}

#define Check( _name )\
	if ( !V_strcmp( panelClass, #_name ) )\
	{\
		CScript_##_name##_Helper *helper = AllocScriptPanel< CScript_##_name##_Helper >();\
		helper->CreateFromScript< CScript_##_name##_Helper >( (HSCRIPT)parent, panelName, root );\
		DebugDevMsg( "%3d | Create vgui %s '%s'   %s\n", g_ScriptPanels.Count(), panelClass, panelName, helper->GetDebugName() );\
		return helper->GetScriptInstance();\
	}

	Check( Panel );
	Check( Label );
	Check( Button );
	Check( ImagePanel );
	Check( Frame );
	Check( RichText );
	Check( TextEntry );
#if !defined(NO_STEAM)
	Check( AvatarImage );
#endif
#if VGUI_TGA_IMAGE_PANEL
	Check( TGAImage );
#endif

	g_pScriptVM->RaiseException("invalid vgui class");
	return NULL;

#undef Check
}

void CScriptVGUI::LevelShutdownPostEntity()
{
	DebugMsg( "LevelShutdownPostEntity()\n" );

	if ( g_ScriptPanels.Count() )
	{
		while ( g_ScriptPanels.Count() )
		{
			Assert( g_ScriptPanels.Head() != g_ScriptPanels.InvalidIndex() );

			int head = g_ScriptPanels.Head();
			g_ScriptPanels[ head ]->Destroy( head );
		}

		g_ScriptPanels.Purge();
	}

	if ( int i = g_ScriptTextureIDs.Count() )
	{
		while ( i-- )
		{
#ifdef _DEBUG
			char tex[MAX_PATH];
			surface()->DrawGetTextureFile( g_ScriptTextureIDs[i], tex, sizeof(tex)-1 );
			DebugMsg( "Destroy texture [%i]%s\n", g_ScriptTextureIDs[i], tex );
#endif
			surface()->DestroyTextureID( g_ScriptTextureIDs[i] );
		}

		g_ScriptTextureIDs.Purge();
	}

	//
	// Reset hud element visibility
	//
	if ( m_bHudVisiblityChangedThisLevel )
	{
		m_bHudVisiblityChangedThisLevel = false;

		FOR_EACH_VEC( m_HudElementCache, i )
		{
			const hudelementcache_t &cache = m_HudElementCache[i];
			Assert( !cache.name.IsEmpty() );
			CHudElement *elem = gHUD.FindElement( cache.name );
			Assert( elem );
			if ( elem )
			{
				elem->SetHiddenBits( cache.bits );
			}
		}
	}
}

void CScriptVGUI::Shutdown()
{
	VGUI_DestroyScriptRootPanels();

	FOR_EACH_DICT_FAST( g_ScriptFonts, i )
	{
		fontalias_t &alias = g_ScriptFonts[i];
		for ( int j = 0; j < alias.Count(); ++j )
		{
			char *pName = alias.Element(j).name;
			if ( pName )
			{
				free( pName );
				alias.Element(j).name = NULL;
			}
		}

		alias.Purge();
	}

	g_ScriptFonts.Purge();

	m_HudElementCache.Purge();
}


void SetHudElementVisible( const char *name, bool state )
{
	CHudElement *elem = gHUD.FindElement( name );
	if ( !elem )
		return;

	int iOldBits = -2;

	FOR_EACH_VEC( m_HudElementCache, i )
	{
		const hudelementcache_t &cache = m_HudElementCache[i];
		if ( !V_stricmp( cache.name, name ) )
		{
			iOldBits = cache.bits;
			break;
		}
	}

	if ( iOldBits == -2 )
	{
		if ( state ) // no change
			return;

		// First time setting the visibility of this element, save the original bits
		hudelementcache_t &cache = m_HudElementCache.Element( m_HudElementCache.AddToTail() );
		cache.name.Set( name );
		cache.bits = elem->GetHiddenBits();
	}

	elem->SetHiddenBits( state ? iOldBits : -1 );

	m_bHudVisiblityChangedThisLevel = true;
}

#ifdef _DEBUG
CON_COMMAND( dump_hud_elements, "" )
{
	int size = gHUD.m_HudList.Size();

	CUtlVector< const char* > list( 0, size );

	for ( int i = 0; i < size; i++ )
	{
		list.AddToTail( gHUD.m_HudList[i]->GetName() );
	}

	struct _cmp
	{
		static int __cdecl fn( const char * const *a, const char * const *b ) { return strcmp( *a, *b ); }
	};

	list.Sort( _cmp::fn );

	for ( int i = 0; i < size; i++ )
	{
		Msg( "%s\n", list[i] );
	}
}
#endif


class CScriptIInput
{
public:
	void MakeWeaponSelection( HSCRIPT weapon )
	{
		::input->MakeWeaponSelection( HScriptToClass< C_BaseCombatWeapon >( weapon ) );
	}
#if 0
	int GetButtonBits()
	{
		return ::input->GetButtonBits(0);
	}

	void ClearInputButton( int i )
	{
		return ::input->ClearInputButton(i);
	}
#endif
	void SetCursorPos( int x, int y )
	{
		vgui::input()->SetCursorPos( x, y );
	}

	int GetAnalogValue( int code )
	{
		Assert( code >= 0 && code < ANALOG_CODE_LAST );

		if ( code < 0 || code >= ANALOG_CODE_LAST )
			return 0;

		return inputsystem->GetAnalogValue( (AnalogCode_t)code );
	}

	int GetAnalogDelta( int code )
	{
		Assert( code >= 0 && code < ANALOG_CODE_LAST );

		if ( code < 0 || code >= ANALOG_CODE_LAST )
			return 0;

		return inputsystem->GetAnalogDelta( (AnalogCode_t)code );
	}

	bool IsButtonDown( int code )
	{
		Assert( code >= BUTTON_CODE_NONE && code < BUTTON_CODE_LAST );

		if ( code < BUTTON_CODE_NONE || code >= BUTTON_CODE_LAST )
			return 0;

		return inputsystem->IsButtonDown( (ButtonCode_t)code );
	}

	// key -> button
	int StringToButtonCode( const char *key )
	{
		return inputsystem->StringToButtonCode( key );
	}

	// button -> key
	const char *ButtonCodeToString( int code )
	{
		Assert( code >= BUTTON_CODE_NONE && code < BUTTON_CODE_LAST );

		if ( code < BUTTON_CODE_NONE || code >= BUTTON_CODE_LAST )
			return 0;

		return inputsystem->ButtonCodeToString( (ButtonCode_t)code );
	}

	// bind -> key
	const char *LookupBinding( const char *bind )
	{
		return engine->Key_LookupBinding( bind );
	}

	// button -> bind
	const char *BindingForKey( int code )
	{
		return engine->Key_BindingForKey( (ButtonCode_t)code );
	}
#if 0
	const char *GetIMELanguageShortCode()
	{
		static char ret[5];
		wchar_t get[5];
		get[0] = L'\0';
		vgui::input()->GetIMELanguageShortCode( get, wcslen(get) );
		g_pVGuiLocalize->ConvertUnicodeToANSI( get, ret, sizeof(ret) );
		return ret;
	}
#endif
} script_input;

BEGIN_SCRIPTDESC_ROOT_NAMED( CScriptIInput, "IInput", SCRIPT_SINGLETON )
	DEFINE_SCRIPTFUNC( MakeWeaponSelection, "" )

	DEFINE_SCRIPTFUNC( SetCursorPos, "" )

	DEFINE_SCRIPTFUNC( GetAnalogValue, "" )
	DEFINE_SCRIPTFUNC( GetAnalogDelta, "" )
	DEFINE_SCRIPTFUNC( IsButtonDown, "" )

	DEFINE_SCRIPTFUNC( StringToButtonCode, "" )
	DEFINE_SCRIPTFUNC( ButtonCodeToString, "" )
	DEFINE_SCRIPTFUNC( LookupBinding, "" )
	DEFINE_SCRIPTFUNC( BindingForKey, "" )
END_SCRIPTDESC()


void SetClipboardText( const char *text )
{
	system()->SetClipboardText( text, V_strlen(text) );
}

//==============================================================
//==============================================================

#if 0
//-----------------------------------------------------------------------------
// Get world position in screen space [0,1]. Return true if on screen.
//-----------------------------------------------------------------------------
inline bool WorldToScreen( const Vector &pos, int &ix, int &iy )
{
	int scrw, scrh;
	surface()->GetScreenSize( scrw, scrh );

	const VMatrix &worldToScreen = engine->WorldToScreenMatrix();
	bool bOnScreen;

	// VMatrix * Vector (position projective)
	vec_t w = worldToScreen[3][0] * pos[0] + worldToScreen[3][1] * pos[1] + worldToScreen[3][2] * pos[2] + worldToScreen[3][3];
	vec_t fx = worldToScreen[0][0] * pos[0] + worldToScreen[0][1] * pos[1] + worldToScreen[0][2] * pos[2] + worldToScreen[0][3];
	vec_t fy = worldToScreen[1][0] * pos[0] + worldToScreen[1][1] * pos[1] + worldToScreen[1][2] * pos[2] + worldToScreen[1][3];

	if ( w < 0.001f )
	{
		fx *= 1e5f;
		fy *= 1e5f;
		bOnScreen = false;
	}
	else
	{
		w = 1.0f / w;
		fx *= w;
		fy *= w;
		bOnScreen = true;
	}

	ix = (int)( scrw * 0.5f * ( 1.0f + fx ) + 0.5f );
	iy = (int)( scrh * 0.5f * ( 1.0f - fy ) + 0.5f );

	return bOnScreen;
}
#endif
//-----------------------------------------------------------------------------
// Get screen pixel position [0,1] in world space.
//-----------------------------------------------------------------------------
inline void ScreenToWorld( int x, int y, Vector &out )
{
	int scrw, scrh;
	surface()->GetScreenSize( scrw, scrh );
	float scrx = (float)x / (float)scrw;
	float scry = (float)y / (float)scrh;

	vec_t tmp[2];
	tmp[0] = 2.0f * scrx - 1.0f;
	tmp[1] = 1.0f - 2.0f * scry;
	//tmp[2] = 1.0f;
	//tmp[3] = 1.0f;

	VMatrix screenToWorld;
	MatrixInverseGeneral( engine->WorldToScreenMatrix(), screenToWorld );

	// VMatrix * Vector (position projective)
	vec_t iw = 1.0f / ( screenToWorld[3][0] * tmp[0] + screenToWorld[3][1] * tmp[1] + screenToWorld[3][2] + screenToWorld[3][3] );
	out[0] = iw * ( screenToWorld[0][0] * tmp[0] + screenToWorld[0][1] * tmp[1] + screenToWorld[0][2] + screenToWorld[0][3] );
	out[1] = iw * ( screenToWorld[1][0] * tmp[0] + screenToWorld[1][1] * tmp[1] + screenToWorld[1][2] + screenToWorld[1][3] );
	out[2] = iw * ( screenToWorld[2][0] * tmp[0] + screenToWorld[2][1] * tmp[1] + screenToWorld[2][2] + screenToWorld[2][3] );
}

#if 0
static bool ScriptWorldToScreen( const Vector &pos, HSCRIPT out )
{
	int ix, iy;
	bool r = WorldToScreen( pos, ix, iy );

	g_pScriptVM->SetValue( out, (ScriptVariant_t)0, ix );
	g_pScriptVM->SetValue( out, 1, iy );
	return r;
}
#endif
static const Vector& ScriptScreenToWorld( int x, int y )
{
	static Vector out;
	ScreenToWorld( x, y, out );
	return out;
}

static const Vector& ScreenToRay( int x, int y )
{
	static Vector out;
	ScreenToWorld( x, y, out );
	VectorSubtract( out, CurrentViewOrigin(), out );
	VectorNormalize( out );
	return out;
}

//-----------------------------------------------------------------------------
// Get world position normalised in screen space. Return true if on screen.
//-----------------------------------------------------------------------------
int ScreenTransform( const Vector& point, Vector& screen );
static bool ScriptScreenTransform( const Vector &pos, HSCRIPT out )
{
	Vector v;
	bool r = ScreenTransform( pos, v );
	float x = 0.5f * ( 1.0f + v[0] );
	float y = 0.5f * ( 1.0f - v[1] );

	g_pScriptVM->SetValue( out, (ScriptVariant_t)0, x );
	g_pScriptVM->SetValue( out, 1, y );
	return !r;
}

int ScriptScreenWidth()
{
	int w, h;
	surface()->GetScreenSize( w, h );
	return w;
}

int ScriptScreenHeight()
{
	int w, h;
	surface()->GetScreenSize( w, h );
	return h;
}

//
// Saving the static (ScreenWidth/640) ratio in a script closure
// messes up on save/restore at differing resolutions -
// the closure and the user script funcs retain the ratio at the time of the save.
// It is not possible to update restored script closure outer variables without writing language specific functions.
//
// NOTE: Returns int! int usage is more common than float operations.
//
static int ScriptXRES( float x )
{
	return x * ( (float)ScriptScreenWidth() / 640.0f );
}

static int ScriptYRES( float y )
{
	return y * ( (float)ScriptScreenHeight() / 480.0f );
}

vgui::HFont GetScriptFont( const char *name, bool proportional )
{
	return script_surface.GetFont( name, proportional, NULL );
}


void RegisterScriptVGUI()
{
	ScriptRegisterFunction( g_pScriptVM, SetHudElementVisible, "" );

	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptXRES, "XRES", "" );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptYRES, "YRES", "" );

	ScriptRegisterFunction( g_pScriptVM, SetClipboardText, "" );
	//ScriptRegisterFunctionNamed( g_pScriptVM, ScriptWorldToScreen, "WorldToScreen", "Get world position in screen space [0,1]. Return true if on screen." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptScreenToWorld, "ScreenToWorld", "Get screen pixel position [0,1] in world space." );
	ScriptRegisterFunction( g_pScriptVM, ScreenToRay, "Get a ray from screen pixel position to world space." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptScreenTransform, "ScreenTransform", "Get world position normalised in screen space. Return true if on screen." );

	g_pScriptVM->Run( g_Script_vgui_init );

	g_pScriptVM->RegisterInstance( &script_surface, "surface" );
	g_pScriptVM->RegisterInstance( &script_input, "input" );
	g_pScriptVM->RegisterInstance( &script_vgui, "vgui" );
}
