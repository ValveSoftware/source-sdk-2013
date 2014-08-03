//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ISURFACE_V30_H
#define ISURFACE_V30_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/IHTML.h> // CreateHTML, PaintHTML 
#include "interface.h"
#include "IVguiMatInfo.h"

#include "appframework/IAppSystem.h"
#include "bitmap/ImageFormat.h"
#include "Vector2D.h"  // must be before the namespace line

#ifdef CreateFont
#undef CreateFont
#endif

#ifdef PlaySound
#undef PlaySound
#endif

class Color;

namespace vgui
{

	class Image;
	class Point;

	// handles
	typedef unsigned long HCursor;
	typedef unsigned long HTexture;
	typedef unsigned long HFont;
}



namespace SurfaceV30
{

	//SRC only defines


	struct Vertex_t
	{
		Vertex_t() {}
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
		// In:
		FontDrawType_t	drawType;
		wchar_t			ch;

		// Out
		bool			valid;

		// In/Out (true by default)
		bool			shouldclip;
		// Text pos
		int				x, y;
		// Top left and bottom right
		Vertex_t		verts[ 2 ];
		int				textureId;
		int				abcA;
		int				abcB;
		int				abcC;
		int				fontTall;
		vgui::HFont		currentFont;
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
	virtual vgui::VPANEL GetEmbeddedPanel() = 0;
	virtual void SetEmbeddedPanel( vgui::VPANEL pPanel ) = 0;

	// drawing context
	virtual void PushMakeCurrent(vgui::VPANEL panel, bool useInsets) = 0;
	virtual void PopMakeCurrent(vgui::VPANEL panel) = 0;

	// rendering functions
	virtual void DrawSetColor(int r, int g, int b, int a) = 0;
	virtual void DrawSetColor(Color col) = 0;
	
	virtual void DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawFilledRectArray( IntRect *pRects, int numRects ) = 0;
	virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;

	virtual void DrawLine(int x0, int y0, int x1, int y1) = 0;
	virtual void DrawPolyLine(int *px, int *py, int numPoints) = 0;

	virtual void DrawSetTextFont(vgui::HFont font) = 0;
	virtual void DrawSetTextColor(int r, int g, int b, int a) = 0;
	virtual void DrawSetTextColor(Color col) = 0;
	virtual void DrawSetTextPos(int x, int y) = 0;
	virtual void DrawGetTextPos(int& x,int& y) = 0;
	virtual void DrawPrintText(const wchar_t *text, int textLen, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;
	virtual void DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;

	virtual void DrawFlushText() = 0;		// flushes any buffered text (for rendering optimizations)
	virtual vgui::IHTML *CreateHTMLWindow(vgui::IHTMLEvents *events,vgui::VPANEL context)=0;
	virtual void PaintHTMLWindow(vgui::IHTML *htmlwin) =0;
	virtual void DeleteHTMLWindow(vgui::IHTML *htmlwin)=0;

	virtual int	 DrawGetTextureId( char const *filename ) = 0;
	virtual bool DrawGetTextureFile(int id, char *filename, int maxlen ) = 0;
	virtual void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) = 0;
	virtual void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)=0;
	virtual void DrawSetTexture(int id) = 0;
	virtual void DrawGetTextureSize(int id, int &wide, int &tall) = 0;
	virtual void DrawTexturedRect(int x0, int y0, int x1, int y1) = 0;
	virtual bool IsTextureIDValid(int id) = 0;

	virtual int CreateNewTextureID( bool procedural = false ) = 0;
#ifdef _XBOX
	virtual void DestroyTextureID( int id ) = 0;
	virtual bool IsCachedForRendering( int id, bool bSyncWait ) = 0;
	virtual void CopyFrontBufferToBackBuffer() = 0;
	virtual void UncacheUnusedMaterials() = 0;
#endif

	virtual void GetScreenSize(int &wide, int &tall) = 0;
	virtual void SetAsTopMost(vgui::VPANEL panel, bool state) = 0;
	virtual void BringToFront(vgui::VPANEL panel) = 0;
	virtual void SetForegroundWindow (vgui::VPANEL panel) = 0;
	virtual void SetPanelVisible(vgui::VPANEL panel, bool state) = 0;
	virtual void SetMinimized(vgui::VPANEL panel, bool state) = 0;
	virtual bool IsMinimized(vgui::VPANEL panel) = 0;
	virtual void FlashWindow(vgui::VPANEL panel, bool state) = 0;
	virtual void SetTitle(vgui::VPANEL panel, const wchar_t *title) = 0;
	virtual void SetAsToolBar(vgui::VPANEL panel, bool state) = 0;		// removes the window's task bar entry (for context menu's, etc.)

	// windows stuff
	virtual void CreatePopup(vgui::VPANEL panel, bool minimised, bool showTaskbarIcon = true, bool disabled = false, bool mouseInput = true , bool kbInput = true) = 0;
	virtual void SwapBuffers(vgui::VPANEL panel) = 0;
	virtual void Invalidate(vgui::VPANEL panel) = 0;
	virtual void SetCursor(vgui::HCursor cursor) = 0;
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
	virtual void RestrictPaintToSinglePanel(vgui::VPANEL panel) = 0;

	// these two functions obselete, use IInput::SetAppModalSurface() instead
	virtual void SetModalPanel(vgui::VPANEL ) = 0;
	virtual vgui::VPANEL GetModalPanel() = 0;

	virtual void UnlockCursor() = 0;
	virtual void LockCursor() = 0;
	virtual void SetTranslateExtendedKeys(bool state) = 0;
	virtual vgui::VPANEL GetTopmostPopup() = 0;

	// engine-only focus handling (replacing WM_FOCUS windows handling)
	virtual void SetTopLevelFocus(vgui::VPANEL panel) = 0;

	// fonts
	// creates an empty handle to a vgui font.  windows fonts can be add to this via SetFontGlyphSet().
	virtual vgui::HFont CreateFont() = 0;

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

	virtual bool SetFontGlyphSet(vgui::HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags) = 0;

	// adds a custom font file (only supports true type font files (.ttf) for now)
	virtual bool AddCustomFontFile(const char *fontName, const char *fontFileName) = 0;

	// returns the details about the font
	virtual int GetFontTall(vgui::HFont font) = 0;
	virtual int GetFontAscent(vgui::HFont font, wchar_t wch) = 0;
	virtual bool IsFontAdditive(vgui::HFont font) = 0;
	virtual void GetCharABCwide(vgui::HFont font, int ch, int &a, int &b, int &c) = 0;
	virtual int GetCharacterWidth(vgui::HFont font, int ch) = 0;
	virtual void GetTextSize(vgui::HFont font, const wchar_t *text, int &wide, int &tall) = 0;

	// notify icons?!?
	virtual vgui::VPANEL GetNotifyPanel() = 0;
	virtual void SetNotifyIcon(vgui::VPANEL context, vgui::HTexture icon, vgui::VPANEL panelToReceiveMessages, const char *text) = 0;

	// plays a sound
	virtual void PlaySound(const char *fileName) = 0;

	//!! these functions should not be accessed directly, but only through other vgui items
	//!! need to move these to seperate interface
	virtual int GetPopupCount() = 0;
	virtual vgui::VPANEL GetPopup(int index) = 0;
	virtual bool ShouldPaintChildPanel(vgui::VPANEL childPanel) = 0;
	virtual bool RecreateContext(vgui::VPANEL panel) = 0;
	virtual void AddPanel(vgui::VPANEL panel) = 0;
	virtual void ReleasePanel(vgui::VPANEL panel) = 0;
	virtual void MovePopupToFront(vgui::VPANEL panel) = 0;
	virtual void MovePopupToBack(vgui::VPANEL panel) = 0;

	virtual void SolveTraverse(vgui::VPANEL panel, bool forceApplySchemeSettings = false) = 0;
	virtual void PaintTraverse(vgui::VPANEL panel) = 0;

	virtual void EnableMouseCapture(vgui::VPANEL panel, bool state) = 0;

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
	virtual void DrawTexturedPolygon(int n, Vertex_t *pVertices) = 0;
	virtual const wchar_t *GetTitle(vgui::VPANEL panel) = 0;
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
#if !defined( _XBOX )
	virtual vgui::HCursor	CreateCursorFromFile( char const *curOrAniFile, char const *pPathID = 0 ) = 0;
#endif
	// create IVguiMatInfo object ( IMaterial wrapper in VguiMatSurface, NULL in CWin32Surface )
	virtual IVguiMatInfo *DrawGetTextureMatInfoFactory( int id ) = 0;

	virtual void PaintTraverseEx(vgui::VPANEL panel, bool paintPopups = false ) = 0;

	virtual float GetZPos() const = 0;

	// From the Xbox
	virtual void SetPanelForInput( vgui::VPANEL vpanel ) = 0;
	virtual void DrawFilledRectFade( int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal ) = 0;
	virtual void DrawSetTextureRGBAEx(int id, const unsigned char *rgba, int wide, int tall, ImageFormat imageFormat ) = 0;
	virtual void DrawSetTextScale(float sx, float sy) = 0;
	virtual bool SetBitmapFontGlyphSet(vgui::HFont font, const char *windowsFontName, float scalex, float scaley, int flags) = 0;
	// adds a bitmap font file
	virtual bool AddBitmapFontFile(const char *fontFileName) = 0;
	// sets a symbol for the bitmap font
	virtual void SetBitmapFontName( const char *pName, const char *pFontFilename ) = 0;
	// gets the bitmap font filename
	virtual const char *GetBitmapFontName( const char *pName ) = 0;

	virtual vgui::IImage *GetIconImageForFullPath( char const *pFullPath ) = 0;
	virtual void DrawUnicodeString( const wchar_t *pwString, FontDrawType_t drawType = FONT_DRAW_DEFAULT ) = 0;
	};

} // end namespace

//-----------------------------------------------------------------------------
// FIXME: This works around using scoped interfaces w/ EXPOSE_SINGLE_INTERFACE
//-----------------------------------------------------------------------------
class ISurfaceV30 : public SurfaceV30::ISurface
{
public:
};


#define VGUI_SURFACE_INTERFACE_VERSION_30 "VGUI_Surface030"

#endif // ISURFACE_V30_H
