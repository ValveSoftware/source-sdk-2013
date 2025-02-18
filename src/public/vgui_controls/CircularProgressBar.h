//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CIRCULARPROGRESSBAR_H
#define CIRCULARPROGRESSBAR_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ProgressBar.h>

enum progress_textures_t
{
	PROGRESS_TEXTURE_FG,
	PROGRESS_TEXTURE_BG,

	NUM_PROGRESS_TEXTURES,
};

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Progress Bar in the shape of a pie graph
//-----------------------------------------------------------------------------
class CircularProgressBar : public ProgressBar
{
	DECLARE_CLASS_SIMPLE( CircularProgressBar, ProgressBar );

public:
	CircularProgressBar(Panel *parent, const char *panelName);
	~CircularProgressBar();

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void ApplySchemeSettings(IScheme *pScheme);

	void SetFgImage(const char *imageName) { SetImage( imageName, PROGRESS_TEXTURE_FG ); }
	void SetBgImage(const char *imageName) { SetImage( imageName, PROGRESS_TEXTURE_BG ); }

	enum CircularProgressDir_e
	{
		PROGRESS_CW,
		PROGRESS_CCW
	};
	int GetProgressDirection() const { return m_iProgressDirection; }
	void SetProgressDirection( int val ) { m_iProgressDirection = val; }
	void SetStartSegment( int val ) { m_iStartSegment = val; }
	void SetReverseProgress( bool bReverse ) { m_bReverseProgress = bReverse; }

protected:
	virtual void Paint();
	virtual void PaintBackground();
	
	void DrawCircleSegment( Color c, float flEndDegrees, bool clockwise /* = true */ );
	void SetImage(const char *imageName, progress_textures_t iPos);

private:
	int m_iProgressDirection;
	int m_iStartSegment;
	bool m_bReverseProgress;

	int m_nTextureId[NUM_PROGRESS_TEXTURES];
	char *m_pszImageName[NUM_PROGRESS_TEXTURES];
	int   m_lenImageName[NUM_PROGRESS_TEXTURES];
};

} // namespace vgui

#endif // CIRCULARPROGRESSBAR_H
