// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class skin_vs30_Static_Index
{
	unsigned int m_nDECAL : 2;
#ifdef _DEBUG
	bool m_bDECAL : 1;
#endif	// _DEBUG
public:
	void SetDECAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDECAL = i;
#ifdef _DEBUG
		m_bDECAL = true;
#endif	// _DEBUG
	}

	skin_vs30_Static_Index(  )
	{
		m_nDECAL = 0;
#ifdef _DEBUG
		m_bDECAL = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bDECAL );
		return ( 32 * m_nDECAL ) + 0;
	}
};

#define shaderStaticTest_skin_vs30 vsh_forgot_to_set_static_DECAL


class skin_vs30_Dynamic_Index
{
	unsigned int m_nCOMPRESSED_VERTS : 2;
	unsigned int m_nDOWATERFOG : 2;
	unsigned int m_nSKINNING : 2;
	unsigned int m_nLIGHTING_PREVIEW : 2;
	unsigned int m_nMORPHING : 2;
#ifdef _DEBUG
	bool m_bCOMPRESSED_VERTS : 1;
	bool m_bDOWATERFOG : 1;
	bool m_bSKINNING : 1;
	bool m_bLIGHTING_PREVIEW : 1;
	bool m_bMORPHING : 1;
#endif	// _DEBUG
public:
	void SetCOMPRESSED_VERTS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOMPRESSED_VERTS = i;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = true;
#endif	// _DEBUG
	}

	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif	// _DEBUG
	}

	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif	// _DEBUG
	}

	void SetLIGHTING_PREVIEW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLIGHTING_PREVIEW = i;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = true;
#endif	// _DEBUG
	}

	void SetMORPHING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMORPHING = i;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif	// _DEBUG
	}

	skin_vs30_Dynamic_Index(  )
	{
		m_nCOMPRESSED_VERTS = 0;
		m_nDOWATERFOG = 0;
		m_nSKINNING = 0;
		m_nLIGHTING_PREVIEW = 0;
		m_nMORPHING = 0;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
		m_bDOWATERFOG = false;
		m_bSKINNING = false;
		m_bLIGHTING_PREVIEW = false;
		m_bMORPHING = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCOMPRESSED_VERTS && m_bDOWATERFOG && m_bSKINNING && m_bLIGHTING_PREVIEW && m_bMORPHING );
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nDOWATERFOG ) + ( 4 * m_nSKINNING ) + ( 8 * m_nLIGHTING_PREVIEW ) + ( 16 * m_nMORPHING ) + 0;
	}
};

#define shaderDynamicTest_skin_vs30 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_LIGHTING_PREVIEW + vsh_forgot_to_set_dynamic_MORPHING

