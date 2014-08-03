//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <Color.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>

#include <vgui_controls/Image.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Conctructor. Start with default position and default color.
//-----------------------------------------------------------------------------
Image::Image()
{
	SetPos(0,0);
	SetSize(0,0);
	SetColor(Color(255,255,255,255));
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
Image::~Image()
{
}

//-----------------------------------------------------------------------------
// Purpose: Set the position of the image, you need to reset this every time you 
// call Paint()
//-----------------------------------------------------------------------------
void Image::SetPos(int x,int y)
{
	_pos[0]=x;
	_pos[1]=y;
}

//-----------------------------------------------------------------------------
// Purpose: Get the position of the image
//-----------------------------------------------------------------------------
void Image::GetPos(int& x,int& y)
{
	x=_pos[0];
	y=_pos[1];
}

//-----------------------------------------------------------------------------
// Purpose: Get the size of the image
//-----------------------------------------------------------------------------
void Image::GetSize(int &wide, int &tall)
{
	wide = _size[0];
	tall = _size[1];
}

//-----------------------------------------------------------------------------
// Purpose: Gets the size of the image contents (by default the set size)
//-----------------------------------------------------------------------------
void Image::GetContentSize(int &wide, int &tall)
{
	GetSize(wide, tall);
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the image
//-----------------------------------------------------------------------------
void Image::SetSize(int wide, int tall)
{
	_size[0]=wide;
	_size[1]=tall;
}

//-----------------------------------------------------------------------------
// Purpose: Set the draw color using a Color struct.
//-----------------------------------------------------------------------------
void Image::DrawSetColor(Color col)
{
	surface()->DrawSetColor(col[0], col[1], col[2], col[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Set the draw color using RGBA ints
//-----------------------------------------------------------------------------
void Image::DrawSetColor(int r,int g,int b,int a)
{
	surface()->DrawSetColor(r,g,b,a);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a filled rectangle
//-----------------------------------------------------------------------------
void Image::DrawFilledRect(int x0,int y0,int x1,int y1)
{
	x0+=_pos[0];
	y0+=_pos[1];
	x1+=_pos[0];
	y1+=_pos[1];
	surface()->DrawFilledRect(x0,y0,x1,y1);
}

//-----------------------------------------------------------------------------
// Purpose: Draw an outlined rectangle
//-----------------------------------------------------------------------------
void Image::DrawOutlinedRect(int x0,int y0,int x1,int y1)
{
	x0+=_pos[0];
	y0+=_pos[1];
	x1+=_pos[0];
	y1+=_pos[1];
	surface()->DrawOutlinedRect(x0,y0,x1,y1);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a line between two points
//-----------------------------------------------------------------------------
void Image::DrawLine(int x0,int y0,int x1,int y1)
{
	x0+=_pos[0];
	y0+=_pos[1];
	x1+=_pos[0];
	y1+=_pos[1];
	surface()->DrawLine(x0,y0,x1,y1);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a line between a list of 'numPoints' points
//-----------------------------------------------------------------------------
void Image::DrawPolyLine(int *px, int *py, int numPoints)
{
	// update the positions to be relative to this panel
	for(int i=0;i<numPoints;i++)
	{
		px[i] += _pos[0];
		py[i] += _pos[1];
	}

	surface()->DrawPolyLine(px, py, numPoints);
}

//-----------------------------------------------------------------------------
// Purpose: Set the font
//-----------------------------------------------------------------------------
void Image::DrawSetTextFont(HFont font)
{
	surface()->DrawSetTextFont(font);
}

//-----------------------------------------------------------------------------
// Purpose: Set the text color using a color struct
//-----------------------------------------------------------------------------
void Image::DrawSetTextColor(Color sc)
{
	surface()->DrawSetTextColor(sc[0], sc[1], sc[2], sc[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Set the text color useing RGBA ints
//-----------------------------------------------------------------------------
void Image::DrawSetTextColor(int r,int g,int b,int a)
{
	surface()->DrawSetTextColor(r,g,b,a);
}	

//-----------------------------------------------------------------------------
// Purpose: Set the text position
//-----------------------------------------------------------------------------
void Image::DrawSetTextPos(int x,int y)
{
	x+=_pos[0];
	y+=_pos[1];
	surface()->DrawSetTextPos(x,y);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a text string
//-----------------------------------------------------------------------------
void Image::DrawPrintText(const wchar_t *str,int strlen)
{
	surface()->DrawPrintText(str, strlen);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a text string at the given coords.
//-----------------------------------------------------------------------------
void Image::DrawPrintText(int x, int y, const wchar_t *str, int strlen)
{
	x += _pos[0];
	y += _pos[1];

	surface()->DrawSetTextPos(x, y);
	surface()->DrawPrintText(str, strlen);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a character
//-----------------------------------------------------------------------------
void Image::DrawPrintChar(wchar_t ch)
{
	surface()->DrawUnicodeChar(ch);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a character at the given coords
//-----------------------------------------------------------------------------
void Image::DrawPrintChar(int x, int y, wchar_t ch)
{
	x+=_pos[0];
	y+=_pos[1];

	surface()->DrawSetTextPos(x, y);
	surface()->DrawUnicodeChar(ch);
}

//-----------------------------------------------------------------------------
// Purpose: Set a texture
//-----------------------------------------------------------------------------
void Image::DrawSetTexture(int id)
{
	surface()->DrawSetTexture(id);
}

//-----------------------------------------------------------------------------
// Purpose: Draw a rectangle filled with the current texture
//-----------------------------------------------------------------------------
void Image::DrawTexturedRect(int x0,int y0,int x1,int y1)
{
	surface()->DrawTexturedRect(x0,y0,x1,y1);
}

//-----------------------------------------------------------------------------
// Purpose: Paint the contents of the image on screen.
// You must call this explicitly each frame.
//-----------------------------------------------------------------------------
void Image::Paint()
{
}

//-----------------------------------------------------------------------------
// Purpose: Set the current color using a color struct
//-----------------------------------------------------------------------------
void Image::SetColor(Color color)
{
	_color=color;
	DrawSetTextColor(color); // now update the device context underneath us :)
}

//-----------------------------------------------------------------------------
//  Purpose: Get the current color as a color struct
//-----------------------------------------------------------------------------
Color Image::GetColor()
{
	return _color;
}

bool Image::Evict()
{
	return false;
}

int Image::GetNumFrames()
{
	return 0;
}

void Image::SetFrame( int nFrame )
{
}

HTexture Image::GetID()
{
	return 0;
}

