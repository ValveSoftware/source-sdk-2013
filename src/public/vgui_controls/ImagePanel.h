//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMAGEPANEL_H
#define IMAGEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{

class IImage;

//-----------------------------------------------------------------------------
// Purpose: Panel that holds a single image
//-----------------------------------------------------------------------------
class ImagePanel : public Panel
{
	DECLARE_CLASS_SIMPLE( ImagePanel, Panel );
public:
	ImagePanel(Panel *parent, const char *name);
	~ImagePanel();

	virtual void SetImage(IImage *image);
	virtual void SetImage(const char *imageName);
	virtual IImage *GetImage();
	char *GetImageName();

	void SetShouldCenterImage( bool state ) { m_bCenterImage = state; }
	bool GetShouldCenterImage() const { return m_bCenterImage; }

	// sets whether or not the image should scale to fit the size of the ImagePanel (defaults to false)
	void SetShouldScaleImage( bool state );
	bool GetShouldScaleImage();
	void SetScaleAmount( float scale );
	float GetScaleAmount( void );

	void SetTileImage( bool bTile )	{ m_bTileImage = bTile; }

	// set the color to fill with, if no image is specified
	void SetFillColor( Color col );
	Color GetFillColor();

	virtual Color GetDrawColor( void );
	virtual void SetDrawColor( Color drawColor );

	virtual void ApplySettings(KeyValues *inResourceData);

	// unhooks and evicts image if possible, caller must re-establish
	bool EvictImage();
	
	int GetNumFrames();
	void SetFrame( int nFrame );

	void SetRotation( int iRotation ) { m_iRotation = iRotation; }

protected:
	virtual void PaintBackground();
	virtual void GetSettings(KeyValues *outResourceData);
	virtual const char *GetDescription();
	virtual void OnSizeChanged(int newWide, int newTall);
	virtual void ApplySchemeSettings( IScheme *pScheme );

private:
	IImage *m_pImage;
	char *m_pszImageName;
	char *m_pszFillColorName;
	char *m_pszDrawColorName;
	bool m_bPositionImage;
	bool m_bCenterImage;
	bool m_bScaleImage;
	int m_nScaleProportional = 0;
	bool m_bTileImage;
	bool m_bTileHorizontally;
	bool m_bTileVertically;
	float m_fScaleAmount;
	Color m_FillColor;
	Color m_DrawColor;
	int m_iRotation;
};

} // namespace vgui

#endif // IMAGEPANEL_H
