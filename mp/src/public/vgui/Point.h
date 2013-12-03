//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef POINT_H
#define POINT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Basic handler for a Points in 2 dimensions
//			This class is fully inline
//-----------------------------------------------------------------------------
class Point
{
public:
	// constructors
	Point()
	{
		SetPoint(0, 0);
	}
	Point(int x,int y)
	{
		SetPoint(x,y);
	}

	void SetPoint(int x1, int y1)
	{
		x=x1;
		y=y1;	
	}

	void GetPoint(int &x1, int &y1) const
	{
		x1 = x;
		y1 = y;
	
	}

	bool operator == (Point &rhs) const
	{
		return (x == rhs.x && y == rhs.y);
	}

private:
	int x, y;
};

} // namespace vgui

#endif // POINT_H
