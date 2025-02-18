#include "cbase.h"
#include "softline.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


SoftLine::SoftLine(vgui::Panel *parent, const char *panelName, Color col) :
	vgui::Panel(parent, panelName)
{
	m_Color = col;
	m_iCornerType = 0;
}

void SoftLine::Paint()
{
	if (m_iCornerType == 1)
		DrawSoftLine(1,GetTall() - 2, GetWide() - 2, 1, m_Color);
	else
		DrawSoftLine(1,1, GetWide() - 2, GetTall() - 2, m_Color);
}

int SoftLine::s_nWhiteTexture = -1;

void SoftLine::DrawSoftLine(float x, float y, float x2, float y2, Color c)
{
	vgui::Vertex_t start, end;				

	if (s_nWhiteTexture == -1)
	{
		s_nWhiteTexture = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( s_nWhiteTexture, "vgui/white" , true, false);
		if (s_nWhiteTexture == -1)
			return;
		return;
	}

	// draw main line
	vgui::surface()->DrawSetTexture(s_nWhiteTexture);
	vgui::surface()->DrawSetColor(c);

	//vgui::surface()->DrawLine(x,y,x2,y2);
	start.Init(Vector2D(x,y), Vector2D(0,0));
	end.Init(Vector2D(x2,y2), Vector2D(1,1));
	DrawPolygonLine(start, end);
	
	// draw translucent ones around it to give it some softness	
	vgui::surface()->DrawSetColor(c);

	start.Init(Vector2D(x - 0.50f,y - 0.50f), Vector2D(0,0));
	end.Init(Vector2D(x2 - 0.50f,y2 - 0.50f), Vector2D(1,1));
	DrawPolygonLine(start, end);

	start.Init(Vector2D(x + 0.50f,y - 0.50f), Vector2D(0,0));
	end.Init(Vector2D(x2 + 0.50f,y2 - 0.50f), Vector2D(1,1));
	DrawPolygonLine(start, end);

	start.Init(Vector2D(x - 0.50f,y + 0.50f), Vector2D(0,0));
	end.Init(Vector2D(x2 - 0.50f,y2 + 0.50f), Vector2D(1,1));
	DrawPolygonLine(start, end);

	start.Init(Vector2D(x + 0.50f,y + 0.50f), Vector2D(0,0));
	end.Init(Vector2D(x2 + 0.50f,y2 + 0.50f), Vector2D(1,1));
	DrawPolygonLine(start, end);
}

// draws a line using polygon calls
void SoftLine::DrawPolygonLine(vgui::Vertex_t start, vgui::Vertex_t end, float width)
{
	DrawPolygonLine(start.m_Position.x, start.m_Position.y, end.m_Position.x, end.m_Position.y, width);
}

void SoftLine::DrawPolygonLine(float x, float y, float x2, float y2, float width)
{
	// find long edge
	Vector2D start(x, y);
	Vector2D end(x2, y2);
	Vector2D long_edge = end - start;
	// normalize and rotate 90 degrees to get our short edge
	Vector2D short_edge = long_edge;
	short_edge.NormalizeInPlace();
	float newx = cos(1.5708f) * short_edge.x - sin(1.5708f) * short_edge.y;
	float newy = sin(1.5708f) * short_edge.x - cos(1.5708f) * short_edge.y;
	short_edge.x = newx;
	short_edge.y = newy;
	short_edge *= width;
	vgui::Vertex_t points[4] = 
	{ 
	vgui::Vertex_t( Vector2D(x, y) - short_edge * 0.5f, Vector2D(0,0) ), 
	vgui::Vertex_t( Vector2D(x, y) - short_edge * 0.5f + long_edge, Vector2D(1,0) ), 
	vgui::Vertex_t( Vector2D(x, y) + short_edge * 0.5f + long_edge, Vector2D(1,1) ), 
	vgui::Vertex_t( Vector2D(x, y) + short_edge * 0.5f, Vector2D(0,1) ) 
	}; 
	vgui::surface()->DrawTexturedPolygon(4, points);
}