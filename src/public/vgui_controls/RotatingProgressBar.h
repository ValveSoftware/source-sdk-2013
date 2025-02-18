//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ROTATINGPROGRESSBAR_H
#define ROTATINGPROGRESSBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ProgressBar.h>

namespace vgui
{

	//-----------------------------------------------------------------------------
	// Purpose: Progress Bar that rotates an image around its center 
	//-----------------------------------------------------------------------------
	class RotatingProgressBar : public ProgressBar
	{
		DECLARE_CLASS_SIMPLE( RotatingProgressBar, ProgressBar );

	public:
		RotatingProgressBar(Panel *parent, const char *panelName);
		~RotatingProgressBar();

		virtual void ApplySettings(KeyValues *inResourceData);
		virtual void ApplySchemeSettings(IScheme *pScheme);

		void SetImage( const char *imageName );

	protected:
		virtual void Paint();
		virtual void PaintBackground();
		virtual void OnTick();

	private:
		int m_nTextureId;
		char *m_pszImageName;

		float m_flStartRadians;
		float m_flEndRadians;

		float m_flLastAngle;

		float m_flTickDelay;
		float m_flApproachSpeed;

		float m_flRotOriginX;
		float m_flRotOriginY;

		float m_flRotatingX;
		float m_flRotatingY;
		float m_flRotatingWide;
		float m_flRotatingTall;

	};		  

} // namespace vgui

#endif // ROTATINGPROGRESSBAR_H
