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
	// These first two support RGB and RGBA in Color255 and Color1 formats.
	Color( float _r, float _g, float _b )
	{
		int r = (int)_r;
		if ( _r < 1 )
		{
			r = ( _r * 255.0f );
		}

		int g = (int)_g;
		if ( _g < 1 )
		{
			g = ( _g * 255.0f );
		}

		int b = (int)_b;
		if ( _b < 1 )
		{
			b = ( _b * 255.0f );
		}

		SetColor( r, g, b, 0 );
	}
	Color( float _r, float _g, float _b, float _a )
	{
		int r = (int)_r;
		if ( _r < 1 )
		{
			r = ( _r * 255.0f );
		}

		int g = (int)_g;
		if ( _g < 1 )
		{
			g = ( _g * 255.0f );
		}

		int b = (int)_b;
		if ( _b < 1 )
		{
			b = ( _b * 255.0f );
		}

		int a = (int)_a;
		if ( _a < 1 )
		{
			a = ( _a * 255.0f );
		}

		SetColor( r, g, b, a );
	}
	// These two support hex code strings in 3 digit (without alpha), 6 digit (without alpha) and 9 digit (with alpha)
	Color( char *_hexCode )
	{
		char *col = _hexCode;

		//check if # exists at the beginning.
		//if it doesn't, return a raw color of 0.
		if ( !Q_strncmp( col, "#", 1 ) )
		{
			col = col + 1;
		}
		else
		{
			*((int *)this) = 0;
			return;
		}

		int r = 0;
		int g = 0;
		int b = 0;
		int a = 0;

		if ( Q_strlen(col) == 3 )
		{
			r = V_nibble( col[0] ) << 4;
			g = V_nibble( col[1] ) << 4;
			b = V_nibble( col[2] ) << 4;
		}
		else
		{
			r = V_nibble( col[0] ) << 4 | V_nibble( col[1] );
			g = V_nibble( col[2] ) << 4 | V_nibble( col[3] );
			b = V_nibble( col[4] ) << 4 | V_nibble( col[5] );

			if ( col[6] && col[7] )
			{
				a = V_nibble( col[6]) << 4 | V_nibble(col[7] );
			}
		}

		SetColor( r, g, b, a );
	}
	Color( wchar_t *_hexCode )
	{
		wchar_t *col = _hexCode;

		//check if # exists at the beginning.
		//if it doesn't, return a raw color of 0.
		if ( !wcsncmp( col, L"#", 1 ) )
		{
			col = col + 1;
		}
		else
		{
			*((int *)this) = 0;
			return;
		}

		int r = 0;
		int g = 0;
		int b = 0;
		int a = 0;

		if ( wcslen( col ) == 3 )
		{
			r = V_nibble( col[0] ) << 4;
			g = V_nibble( col[1] ) << 4;
			b = V_nibble( col[2] ) << 4;
		}
		else
		{
			r = V_nibble( col[0] ) << 4 | V_nibble( col[1] );
			g = V_nibble( col[2] ) << 4 | V_nibble( col[3] );
			b = V_nibble( col[4] ) << 4 | V_nibble( col[5] );

			if ( col[6] && col[7] )
			{
				a = V_nibble( col[6] ) << 4 | V_nibble( col[7] );
			}
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
