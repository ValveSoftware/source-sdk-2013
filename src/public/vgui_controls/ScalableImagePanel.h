//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SCALABLEIMAGEPANEL_H
#define SCALABLEIMAGEPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

namespace vgui
{
	//-----------------------------------------------------------------------------
	// Purpose: 9-way Segmented background
	//-----------------------------------------------------------------------------
	class ScalableImagePanel : public Panel
	{
		DECLARE_CLASS_SIMPLE( ScalableImagePanel, Panel );
	public:
		ScalableImagePanel(Panel *parent, const char *name);
		~ScalableImagePanel();

		virtual void SetImage(const char *imageName);
		void		 SetDrawColor( Color color ) { m_DrawColor = color; }

	protected:
		virtual void PaintBackground();
		virtual void GetSettings(KeyValues *outResourceData);
		virtual void ApplySettings(KeyValues *inResourceData);
		virtual void PerformLayout( void );
		virtual const char *GetDescription();

	private:
		int m_iSrcCornerHeight;	// in pixels, how tall is the corner inside the image
		int m_iSrcCornerWidth; // same for width
		int m_iCornerHeight;	// output size of the corner height in pixels
		int m_iCornerWidth;		// same for width

		int m_iTextureID;

		float m_flCornerWidthPercent;	// corner width as percentage of image width
		float m_flCornerHeightPercent;	// same for height

		char *m_pszImageName;

		char *m_pszDrawColorName;
		Color m_DrawColor;
	};

} // namespace vgui

#endif // SCALABLEIMAGEPANEL_H
