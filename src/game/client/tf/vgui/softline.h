#ifndef _INCLUDED_SOFT_LINE_H
#define _INCLUDED_SOFT_LINE_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

// this is a vgui panel that draws a line between opposite corners
// the line is softened with translucent lines around it

class SoftLine : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( SoftLine, vgui::Panel );
public:
	SoftLine(vgui::Panel *parent, const char *panelName, Color col);
	virtual void Paint();
	void DrawSoftLine(float x, float y, float x2, float y2, Color c);
	void SetCornerType(int i) { m_iCornerType = i; }

	Color m_Color;
	int m_iCornerType;

	static int s_nWhiteTexture;

	// draws a line between two points using polygon rather than line drawing functions (since line doesn't work sometimes)
	static void DrawPolygonLine(float x, float y, float x2, float y2, float width=1.0f);
	static void DrawPolygonLine(vgui::Vertex_t start, vgui::Vertex_t end, float width=1.0f);
};


#endif // _INCLUDED_SOFT_LINE_H
