//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef ISURFACE_H
#define ISURFACE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/IHTML.h> // CreateHTML, PaintHTML 
#include "tier1/interface.h"
#include "bitmap/imageformat.h"

#include "appframework/IAppSystem.h"
#include "mathlib/vector2d.h"  // must be before the namespace line

#include "IVguiMatInfo.h"

#ifdef CreateFont
#undef CreateFont
#endif

#ifdef PlaySound
#undef PlaySound
#endif

class Color;
class ITexture;

namespace vgui
{

class IImage;
class Image;
class Point;

// handles
typedef uint32 HCursor;
typedef uint32 HTexture;
typedef uint32 HFont;


//SRC only defines


struct Vertex_t
{
	Vertex_t() = default;
	Vertex_t( const Vector2D &pos, const Vector2D &coord = Vector2D( 0, 0 ) )
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	void Init( const Vector2D &pos, const Vector2D &coord = Vector2D( 0, 0 ) )
	{
		m_Position = pos;
		m_TexCoord = coord;
	}
	
	Vector2D	m_Position;
	Vector2D	m_TexCoord;
};


enum FontDrawType_t
{
	// Use the "additive" value from the scheme file
	FONT_DRAW_DEFAULT = 0,

	// Overrides
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,

	FONT_DRAW_TYPE_COUNT = 2,
};


// Refactor these two
struct CharRenderInfo
{
	// Text pos
	int				x, y;
	// Top left and bottom right
	// This is now a pointer to an array maintained by the surface, to avoid copying the data on the 360
	Vertex_t		*verts;
	int				textureId;
	int				abcA;
	int				abcB;
	int				abcC;
	int				fontTall;
	HFont			currentFont;
	// In:
	FontDrawType_t	drawType;
	wchar_t			ch;

	// Out
	bool			valid;
	// In/Out (true by default)
	bool			shouldclip;
};


struct IntRect
{
	int x0;
	int y0;
	int x1;
	int y1;
};

//-----------------------------------------------------------------------------
// Purpose: Wraps contextless windows system functions
//-----------------------------------------------------------------------------
class ISurface : public IAppSystem
{
public:
	// call to Shutdown surface; surface can no longer be used after this is called
	virtual void Shutdown() = 0;

	// frame
	virtual void RunFrame() = 0;

	// hierarchy root
	virtual VPANEL GetEmbeddedPanel() = 0;
	virtual void SetEmbeddedPanel( VPANEL pPanel ) = 0;

	// drawing context
	virtual void PushMakeCurrent(VPANEL panel, bool useInsets) = 0;
	virtual void PopMakeCurrent(VPANEL panel) = 0;

	// rendering functions
	virtual void DrawSetColor(int r, int g, int b, int a) = 0;
	virtual void DrawSetColor(Color col) = 0;
	
	virtual void DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawFilledRectArray( IntRect *pRects, int numRects ) = 0;
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;

	virtual void DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawPolyLine(int *px, int *py, int numPoints) = 0;

	virtual void DrawSetTextFont(HFont font) = 0;
	virtual void DrawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void DrawSetTextColor(Color col) = 0;
	virtual void DrawSetTextPos(int x, int y) = 0;
	virtual void DrawGetTextPos(int& x,int& y) = 0;
	virtual void DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;
	virtual void DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;

	virtual void DrawFlushText() = 0;		// flushes any buffered text (for rendering optimizations)
	virtual IHTML *CreateHTMLWindow(vgui::IHTMLEvents *events,VPANEL context)=0;
	virtual void PaintHTMLWindow(vgui::IHTML *htmlwin) =0;
	virtual void DeleteHTMLWindow(IHTML *htmlwin)=0;

	enum ETextureFormat
	{
		eTextureFormat_RGBA,
		eTextureFormat_BGRA,
		eTextureFormat_BGRA_Opaque, // bgra format but alpha is always 255, CEF does this, we can use this fact for better perf on win32 gdi
	};

	virtual int	 DrawGetTextureId( char const *filename ) = 0;
	virtual bool DrawGetTextureFile(int id, char *filename, int maxlen ) = 0;
	virtual void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) = 0;
	virtual void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)=0;
	virtual void DrawSetTexture(int id) = 0;
	virtual void DrawGetTextureSize(int id, int &wide, int &tall) = 0;
	virtual void DrawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual bool IsTextureIDValid(int id) = 0;
	virtual bool DeleteTextureByID(int id) = 0;

	virtual int CreateNewTextureID( bool procedural = false ) = 0;

	virtual void GetScreenSize(int &wide, int &tall) = 0;
	virtual void SetAsTopMost(VPANEL panel, bool state) = 0;
	virtual void BringToFront(VPANEL panel) = 0;
	virtual void SetForegroundWindow (VPANEL panel) = 0;
	virtual void SetPanelVisible(VPANEL panel, bool state) = 0;
	virtual void SetMinimized(VPANEL panel, bool state) = 0;
	virtual bool IsMinimized(VPANEL panel) = 0;
	virtual void FlashWindow(VPANEL panel, bool state) = 0;
	virtual void SetTitle(VPANEL panel, const wchar_t *title) = 0;
	virtual void SetAsToolBar(VPANEL panel, bool state) = 0;		// removes the window's task bar entry (for context menu's, etc.)

	// windows stuff
	virtual void CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true , bool kbInput = true) = 0;
	virtual void SwapBuffers(VPANEL panel) = 0;
	virtual void Invalidate(VPANEL panel) = 0;
	virtual void SetCursor(HCursor cursor) = 0;
	virtual void SetCursorAlwaysVisible( bool visible ) = 0;
	virtual bool IsCursorVisible() = 0;
	virtual void ApplyChanges() = 0;
	virtual bool IsWithin(int x, int y) = 0;
	virtual bool HasFocus() = 0;
	
	// returns true if the surface supports minimize & maximize capabilities
	enum SurfaceFeature_e
	{
		ANTIALIASED_FONTS	= 1,
		DROPSHADOW_FONTS	= 2,
		ESCAPE_KEY			= 3,
		OPENING_NEW_HTML_WINDOWS = 4,
		FRAME_MINIMIZE_MAXIMIZE	 = 5,
		OUTLINE_FONTS	= 6,
		DIRECT_HWND_RENDER		= 7,
	};
	virtual bool SupportsFeature(SurfaceFeature_e feature) = 0;

	// restricts what gets drawn to one panel and it's children
	// currently only works in the game
	virtual void RestrictPaintToSinglePanel(VPANEL panel) = 0;

	// these two functions obselete, use IInput::SetAppModalSurface() instead
	virtual void SetModalPanel(VPANEL ) = 0;
	virtual VPANEL GetModalPanel() = 0;

	virtual void UnlockCursor() = 0;
	virtual void LockCursor() = 0;
	virtual void SetTranslateExtendedKeys(bool state) = 0;
	virtual VPANEL GetTopmostPopup() = 0;

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	virtual void SetTopLevelFocus(VPANEL panel) = 0;

	// fonts
	// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
	virtual HFont CreateFont() = 0;

	// adds to the font
	enum EFontFlags
	{
		FONTFLAG_NONE,
		FONTFLAG_ITALIC			= 0x001,
		FONTFLAG_UNDERLINE		= 0x002,
		FONTFLAG_STRIKEOUT		= 0x004,
		FONTFLAG_SYMBOL			= 0x008,
		FONTFLAG_ANTIALIAS		= 0x010,
		FONTFLAG_GAUSSIANBLUR	= 0x020,
		FONTFLAG_ROTARY			= 0x040,
		FONTFLAG_DROPSHADOW		= 0x080,
		FONTFLAG_ADDITIVE		= 0x100,
		FONTFLAG_OUTLINE		= 0x200,
		FONTFLAG_CUSTOM			= 0x400,		// custom generated font - never fall back to asian compatibility mode
		FONTFLAG_BITMAP			= 0x800,		// compiled bitmap font - no fallbacks
	};

	virtual bool SetFontGlyphSet(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0) = 0;

	// adds a custom font file (only supports true type font files (.ttf) for now)
	virtual bool AddCustomFontFile(const char *fontName, const char *fontFileName) = 0;

	// returns the details about the font
	virtual int GetFontTall(HFont font) = 0;
	virtual int GetFontTallRequested(HFont font) = 0;
	virtual int GetFontAscent(HFont font, wchar_t wch) = 0;
	virtual bool IsFontAdditive(HFont font) = 0;
	virtual void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) = 0;
	virtual int GetCharacterWidth(HFont font, int ch) = 0;
	virtual void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) = 0;

	// notify icons?!?
	virtual VPANEL GetNotifyPanel() = 0;
	virtual void SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text) = 0;

	// plays a sound
	virtual void PlaySound(const char *fileName) = 0;

	//!! these functions should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	virtual int GetPopupCount() = 0;
	virtual VPANEL GetPopup(int index) = 0;
	virtual bool ShouldPaintChildPanel(VPANEL childPanel) = 0;
	virtual bool RecreateContext(VPANEL panel) = 0;
	virtual void AddPanel(VPANEL panel) = 0;
	virtual void ReleasePanel(VPANEL panel) = 0;
	virtual void MovePopupToFront(VPANEL panel) = 0;
	virtual void MovePopupToBack(VPANEL panel) = 0;

	virtual void SolveTraverse(VPANEL panel, bool forceApplySchemeSettings = false) = 0;
	virtual void PaintTraverse(VPANEL panel) = 0;

	virtual void EnableMouseCapture(VPANEL panel, bool state) = 0;

	// returns the size of the workspace
	virtual void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) = 0;

	// gets the absolute coordinates of the screen (in windows space)
	virtual void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) = 0;

	// gets the base resolution used in proportional mode
	virtual void GetProportionalBase( int &width, int &height ) = 0;

	virtual void CalculateMouseVisible() = 0;
	virtual bool NeedKBInput() = 0;

	virtual bool HasCursorPosFunctions() = 0;
	virtual void SurfaceGetCursorPos(int &x, int &y) = 0;
	virtual void SurfaceSetCursorPos(int x, int y) = 0;

	// SRC only functions!!!
	virtual void DrawTexturedLine( const Vertex_t &a, const Vertex_t &b ) = 0;
	virtual void DrawOutlinedCircle(int x, int y, int radius, int segments) = 0;
	virtual void DrawTexturedPolyLine( const Vertex_t *p,int n ) = 0; // (Note: this connects the first and last points).
	virtual void DrawTexturedSubRect( int x0, int y0, int x1, int y1, float texs0, float text0, float texs1, float text1 ) = 0;
	virtual void DrawTexturedPolygon(int n, Vertex_t *pVertice, bool bClipVertices = true ) = 0;
	virtual const wchar_t *GetTitle(VPANEL panel) = 0;
	virtual bool IsCursorLocked( void ) const = 0;
	virtual void SetWorkspaceInsets( int left, int top, int right, int bottom ) = 0;

	// Lower level char drawing code, call DrawGet then pass in info to DrawRender
	virtual bool DrawGetUnicodeCharRenderInfo( wchar_t ch, CharRenderInfo& info ) = 0;
	virtual void DrawRenderCharFromInfo( const CharRenderInfo& info ) = 0;

	// global alpha setting functions
	// affect all subsequent draw calls - shouldn't normally be used directly, only in Panel::PaintTraverse()
	virtual void DrawSetAlphaMultiplier( float alpha /* [0..1] */ ) = 0;
	virtual float DrawGetAlphaMultiplier() = 0;

	// web browser
	virtual void SetAllowHTMLJavaScript( bool state ) = 0;

	// video mode changing
	virtual void OnScreenSizeChanged( int nOldWidth, int nOldHeight ) = 0;

	virtual vgui::HCursor CreateCursorFromFile( char const *curOrAniFile, char const *pPathID = 0 ) = 0;

	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	virtual IVguiMatInfo *DrawGetTextureMatInfoFactory( int id ) = 0;

	virtual void PaintTraverseEx(VPANEL panel, bool paintPopups = false ) = 0;

	virtual float GetZPos() const = 0;

	// From the Xbox
	virtual void SetPanelForInput( VPANEL vpanel ) = 0;
	virtual void DrawFilledRectFastFade( int x0, int y0, int x1, int y1, int fadeStartPt, int fadeEndPt, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) = 0;
	virtual void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) = 0;
	virtual void DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat ) = 0;
	virtual void DrawSetTextScale(float sx, float sy) = 0;
	virtual bool SetBitmapFontGlyphSet(HFont font, const char *windowsFontName, float scalex, float scaley, int flags) = 0;
	// adds a bitmap font file
	virtual bool AddBitmapFontFile(const char *fontFileName) = 0;
	// sets a symbol for the bitmap font
	virtual void SetBitmapFontName( const char *pName, const char *pFontFilename ) = 0;
	// gets the bitmap font filename
	virtual const char *GetBitmapFontName( const char *pName ) = 0;
	virtual void ClearTemporaryFontCache( void ) = 0;

	virtual IImage *GetIconImageForFullPath( char const *pFullPath ) = 0;
	virtual void DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;
	virtual void PrecacheFontCharacters(HFont font, const wchar_t *pCharacters) = 0;
	// Console-only.  Get the string to use for the current video mode for layout files.
	virtual const char *GetResolutionKey( void ) const = 0;
	
	virtual const char *GetFontName( HFont font ) = 0;
	virtual const char *GetFontFamilyName( HFont font ) = 0;
	virtual void GetKernedCharWidth( HFont font, wchar_t ch, wchar_t chBefore, wchar_t chAfter, float &wide, float &abcA ) = 0;

	virtual bool ForceScreenSizeOverride( bool bState, int wide, int tall ) = 0;
	// LocalToScreen, ParentLocalToScreen fixups for explicit PaintTraverse calls on Panels not at 0, 0 position
	virtual bool ForceScreenPosOffset( bool bState, int x, int y ) = 0;
	virtual void OffsetAbsPos( int &x, int &y ) = 0;


	// Causes fonts to get reloaded, etc.
	virtual void ResetFontCaches() = 0;

	virtual int GetTextureNumFrames( int id ) = 0;
	virtual void DrawSetTextureFrame( int id, int nFrame, unsigned int *pFrameCache ) = 0;
	virtual bool IsScreenSizeOverrideActive( void ) = 0;
	virtual bool IsScreenPosOverrideActive( void ) = 0;

	virtual void DestroyTextureID( int id ) = 0;

	virtual void DrawUpdateRegionTextureRGBA( int nTextureID, int x, int y, const unsigned char *pchData, int wide, int tall, ImageFormat imageFormat ) = 0;
	virtual bool BHTMLWindowNeedsPaint(IHTML *htmlwin) = 0 ;

	virtual const char *GetWebkitHTMLUserAgentString() = 0;

	virtual void *Deprecated_AccessChromeHTMLController() = 0;

	// the origin of the viewport on the framebuffer (Which might not be 0,0 for stereo)
	virtual void SetFullscreenViewport( int x, int y, int w, int h ) = 0; // this uses NULL for the render target.
	virtual void GetFullscreenViewport( int & x, int & y, int & w, int & h ) = 0;
	virtual void PushFullscreenViewport() = 0;
	virtual void PopFullscreenViewport() = 0;

	// handles support for software cursors
	virtual void SetSoftwareCursor( bool bUseSoftwareCursor ) = 0;
	virtual void PaintSoftwareCursor() = 0;


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !! WARNING! YOU MUST NOT ADD YOUR NEW METHOD HERE OR YOU WILL BREAK MODS !!
	// !! Add your new stuff to the bottom of IMatSystemSurface instead.        !!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
};

}

#define VGUI_SURFACE_INTERFACE_VERSION "VGUI_Surface030"

#endif // ISURFACE_H
