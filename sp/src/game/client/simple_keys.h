//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SIMPLE_KEYS_H
#define SIMPLE_KEYS_H
#ifdef _WIN32
#pragma once
#endif

enum simplekeyinterp_t
{
	KEY_LINEAR = 0,
	KEY_SPLINE,
	KEY_ACCELERATE,
	KEY_DECELERATE,
};

class CSimpleKeyInterp : public Vector
{
public:
	CSimpleKeyInterp( float t, simplekeyinterp_t interp, float x, float y = 0, float z = 0 ) : Vector( x, y, z ) 
	{
		m_interp = interp;
		m_keyTime = t;
	}
	
	float GetTime() const { return m_keyTime; }
	
	// out = t*start + (1-t) * end (may be splinear or linear)
	static void Interp( Vector &out, float t, const CSimpleKeyInterp &start, const CSimpleKeyInterp &end );

	float				m_keyTime;
	simplekeyinterp_t	m_interp;
};


class CSimpleKeyList
{
public:
	int		Insert( const CSimpleKeyInterp &key );
	bool	Interp( Vector &out, float t );

	CUtlVector<CSimpleKeyInterp>	m_list;
};

#endif // SIMPLE_KEYS_H
