//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef SLIDESHOWPANEL_H
#define SLIDESHOWPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/ImagePanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Allows fade from one image to another
//-----------------------------------------------------------------------------
class CCrossfadableImagePanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCrossfadableImagePanel, EditablePanel );
public:
	CCrossfadableImagePanel( Panel* pParent, const char *pName );
	~CCrossfadableImagePanel();

	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	// A duplicate of the ImagePanel interface - the only difference being that
	// here we have blend times for SetImage().
	void SetImage( IImage *pImage, float flBlendTime = 0.0f );
	void SetImage( const char *pImageName, float flBlendTime = 0.0f );

	void SetShouldScaleImage( bool bState );
	void SetScaleAmount( float flScale );
	void SetDrawColor( Color clrDrawColor );

	IImage *GetImage();
	const char *GetImageName();
	void SetFillColor( Color c );
	float GetScaleAmount();
	bool GetShouldScaleImage();
	Color GetFillColor();
	Color GetDrawColor();

	virtual void InstallMouseHandler( Panel *pHandler );

private:
	virtual void OnSizeChanged( int nWide, int nTall );
	virtual void OnTick();

	inline ImagePanel *SrcImg()	{ return m_pImages[  m_iSrcImg ]; }
	inline ImagePanel *DstImg() { return m_pImages[ !m_iSrcImg ]; }

	void SetupImageBlend( float flBlendTime );

	float			m_flStartBlendTime;	// the gpGlobals->realtime when we started blending
	float			m_flBlendTime;		// amount of time to blend from one image to the next one
	float			m_flBlend;			// blend value in [0,1] where 0 maps to the source image, and 1 maps to the dest image
	bool			m_bBlending;		// are we currently transitioning/blending from one image to another?
	ImagePanel*		m_pImages[2];
	int				m_iSrcImg;	// 0 or 1
};

//-----------------------------------------------------------------------------
// Purpose: Displays a slideshow of images at a set interval
//-----------------------------------------------------------------------------
class CSlideshowPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSlideshowPanel, EditablePanel );
public:
	CSlideshowPanel( Panel *pParent, const char *pName );
	~CSlideshowPanel();

	// Pass in a vgui-relative path, like "training/screenshots/cp_dustbowl_", and
	// the slideshow panel will add "0.vmt", "1.vmt" etc. until it can't find anymore images.
	void FillWithImages( const char *pBasePath );

	void AddImage( const char *pImageName );
	void AddImage( IImage *pImage );

	void SetInterval( float flInterval );
	void SetTransitionTime( float flTransitionLength );

	CCrossfadableImagePanel *GetImagePanel()			{ return m_pImagePanel; }

	int GetImageCount() const							{ return m_vecImages.Count(); }

private:
	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void OnSizeChanged( int nWide, int nTall );
	virtual void OnTick();
	void UpdateNextTransitionTime();

	CCrossfadableImagePanel		*m_pImagePanel;
	CUtlVector< IImage * >		m_vecImages;
	float						m_flNextTransitionTime;	// gpGlobals->realtime of time when we should transition next
	float						m_flInterval;			// Amount of time between blend begins to next image, in seconds
	float						m_flTransitionLength;	// Length of each transition
	int							m_iCurImg;				// Current image index
};

#endif // SLIDESHOWPANEL_H
