//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLOR_H
#define COLOR_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/strtools.h"

//-----------------------------------------------------------------------------
// Purpose: Basic handler for an rgb set of colors
//			This class is fully inline
//-----------------------------------------------------------------------------
class Color
{
public:
	// constructors
	Color()
	{
		*((int *)this) = 0;
	}
	Color(int _r,int _g,int _b)
	{
		SetColor(_r, _g, _b, 0);
	}
	Color(int _r,int _g,int _b,int _a)
	{
		SetColor(_r, _g, _b, _a);
	}
	Color( char *_hexCode )
	{
		char *col = _hexCode;
		
		//check if FF or #F exist at the beginning.
		//if they don't, return a raw color of 0.
		if ( !col[0] && !col[1] )
		{
			*( ( int* )this ) = 0;
			return;
		}

		if ( col[8] )
		{
			col = col + 1;
		}

		int r = V_nibble( col[0] ) << 4 | V_nibble( col[1] );
		int g = V_nibble( col[2] ) << 4 | V_nibble( col[3] );
		int b = V_nibble( col[4] ) << 4 | V_nibble( col[5] );
		int a = 0;

		if ( col[6] && col[7] )
		{
			a = V_nibble( col[6] ) << 4 | V_nibble( col[7] );
		}

		SetColor( r, g, b, a );
	}
	Color( wchar_t *_hexCode )
	{
		wchar_t *col = _hexCode;
		
		//check if FF or #F exist at the beginning.
		//if they don't, return a raw color of 0.
		if ( !col[0] && !col[1] )
		{
			*( ( int* )this ) = 0;
			return;
		}

		if ( col[8] )
		{
			col = col + 1;
		}

		int r = V_nibble( col[0] ) << 4 | V_nibble( col[1] );
		int g = V_nibble( col[2] ) << 4 | V_nibble( col[3] );
		int b = V_nibble( col[4] ) << 4 | V_nibble( col[5] );
		int a = 0;

		if ( col[6] && col[7] )
		{
			a = V_nibble( col[6] ) << 4 | V_nibble( col[7] );
		}

		SetColor( r, g, b, a );
	}
	
	// set the color
	// r - red component (0-255)
	// g - green component (0-255)
	// b - blue component (0-255)
	// a - alpha component, controls transparency (0 - transparent, 255 - opaque);
	void SetColor(int _r, int _g, int _b, int _a = 0)
	{
		_color[0] = (unsigned char)_r;
		_color[1] = (unsigned char)_g;
		_color[2] = (unsigned char)_b;
		_color[3] = (unsigned char)_a;
	}

	void GetColor(int &_r, int &_g, int &_b, int &_a) const
	{
		_r = _color[0];
		_g = _color[1];
		_b = _color[2];
		_a = _color[3];
	}

	void SetRawColor( int color32 )
	{
		*((int *)this) = color32;
	}

	int GetRawColor() const
	{
		return *((int *)this);
	}

	inline int r() const	{ return _color[0]; }
	inline int g() const	{ return _color[1]; }
	inline int b() const	{ return _color[2]; }
	inline int a() const	{ return _color[3]; }
	
	unsigned char &operator[](int index)
	{
		return _color[index];
	}

	const unsigned char &operator[](int index) const
	{
		return _color[index];
	}

	bool operator == (const Color &rhs) const
	{
		return ( *((int *)this) == *((int *)&rhs) );
	}

	bool operator != (const Color &rhs) const
	{
		return !(operator==(rhs));
	}

	Color &operator=( const Color &rhs )
	{
		SetRawColor( rhs.GetRawColor() );
		return *this;
	}

private:
	unsigned char _color[4];
};


#endif // COLOR_H
