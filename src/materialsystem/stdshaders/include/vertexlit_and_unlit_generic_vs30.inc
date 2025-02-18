// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ($SEPARATE_DETAIL_UVS) && ($SEAMLESS_DETAIL)
// ($DONT_GAMMA_CONVERT_VERTEX_COLOR && ( ! $VERTEXCOLOR ) )
// ($TREESWAY) && ($MORPHING)
// ($TREESWAY) && ($TESSELLATION)
// ( $TREESWAY ) && ( $SEAMLESS_DETAIL || $SEAMLESS_BASE )
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class vertexlit_and_unlit_generic_vs30_Static_Index
{
	unsigned int m_nVERTEXCOLOR : 2;
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nHALFLAMBERT : 2;
	unsigned int m_nFLASHLIGHT : 2;
	unsigned int m_nSEAMLESS_BASE : 2;
	unsigned int m_nSEAMLESS_DETAIL : 2;
	unsigned int m_nSEPARATE_DETAIL_UVS : 2;
	unsigned int m_nDECAL : 2;
	unsigned int m_nDONT_GAMMA_CONVERT_VERTEX_COLOR : 2;
	unsigned int m_nTREESWAY : 2;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR : 1;
	bool m_bCUBEMAP : 1;
	bool m_bHALFLAMBERT : 1;
	bool m_bFLASHLIGHT : 1;
	bool m_bSEAMLESS_BASE : 1;
	bool m_bSEAMLESS_DETAIL : 1;
	bool m_bSEPARATE_DETAIL_UVS : 1;
	bool m_bDECAL : 1;
	bool m_bDONT_GAMMA_CONVERT_VERTEX_COLOR : 1;
	bool m_bTREESWAY : 1;
#endif	// _DEBUG
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif	// _DEBUG
	}

	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif	// _DEBUG
	}

	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif	// _DEBUG
	}

	void SetFLASHLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHT = i;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif	// _DEBUG
	}

	void SetSEAMLESS_BASE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS_BASE = i;
#ifdef _DEBUG
		m_bSEAMLESS_BASE = true;
#endif	// _DEBUG
	}

	void SetSEAMLESS_DETAIL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS_DETAIL = i;
#ifdef _DEBUG
		m_bSEAMLESS_DETAIL = true;
#endif	// _DEBUG
	}

	void SetSEPARATE_DETAIL_UVS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEPARATE_DETAIL_UVS = i;
#ifdef _DEBUG
		m_bSEPARATE_DETAIL_UVS = true;
#endif	// _DEBUG
	}

	void SetDECAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDECAL = i;
#ifdef _DEBUG
		m_bDECAL = true;
#endif	// _DEBUG
	}

	void SetDONT_GAMMA_CONVERT_VERTEX_COLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDONT_GAMMA_CONVERT_VERTEX_COLOR = i;
#ifdef _DEBUG
		m_bDONT_GAMMA_CONVERT_VERTEX_COLOR = true;
#endif	// _DEBUG
	}

	void SetTREESWAY( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nTREESWAY = i;
#ifdef _DEBUG
		m_bTREESWAY = true;
#endif	// _DEBUG
	}

	vertexlit_and_unlit_generic_vs30_Static_Index(  )
	{
		m_nVERTEXCOLOR = 0;
		m_nCUBEMAP = 0;
		m_nHALFLAMBERT = 0;
		m_nFLASHLIGHT = 0;
		m_nSEAMLESS_BASE = 0;
		m_nSEAMLESS_DETAIL = 0;
		m_nSEPARATE_DETAIL_UVS = 0;
		m_nDECAL = 0;
		m_nDONT_GAMMA_CONVERT_VERTEX_COLOR = 0;
		m_nTREESWAY = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
		m_bCUBEMAP = false;
		m_bHALFLAMBERT = false;
		m_bFLASHLIGHT = false;
		m_bSEAMLESS_BASE = false;
		m_bSEAMLESS_DETAIL = false;
		m_bSEPARATE_DETAIL_UVS = false;
		m_bDECAL = false;
		m_bDONT_GAMMA_CONVERT_VERTEX_COLOR = false;
		m_bTREESWAY = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bVERTEXCOLOR && m_bCUBEMAP && m_bHALFLAMBERT && m_bFLASHLIGHT && m_bSEAMLESS_BASE && m_bSEAMLESS_DETAIL && m_bSEPARATE_DETAIL_UVS && m_bDECAL && m_bDONT_GAMMA_CONVERT_VERTEX_COLOR && m_bTREESWAY );
		AssertMsg( !( m_nSEPARATE_DETAIL_UVS && m_nSEAMLESS_DETAIL ), "Invalid combo combination ( SEPARATE_DETAIL_UVS && SEAMLESS_DETAIL )" );
		AssertMsg( !( m_nDONT_GAMMA_CONVERT_VERTEX_COLOR && !m_nVERTEXCOLOR ), "Invalid combo combination ( DONT_GAMMA_CONVERT_VERTEX_COLOR && !VERTEXCOLOR )" );
		AssertMsg( !( m_nTREESWAY && ( m_nSEAMLESS_DETAIL || m_nSEAMLESS_BASE ) ), "Invalid combo combination ( TREESWAY && ( SEAMLESS_DETAIL || SEAMLESS_BASE ) )" );
		return ( 256 * m_nVERTEXCOLOR ) + ( 512 * m_nCUBEMAP ) + ( 1024 * m_nHALFLAMBERT ) + ( 2048 * m_nFLASHLIGHT ) + ( 4096 * m_nSEAMLESS_BASE ) + ( 8192 * m_nSEAMLESS_DETAIL ) + ( 16384 * m_nSEPARATE_DETAIL_UVS ) + ( 32768 * m_nDECAL ) + ( 65536 * m_nDONT_GAMMA_CONVERT_VERTEX_COLOR ) + ( 131072 * m_nTREESWAY ) + 0;
	}
};

#define shaderStaticTest_vertexlit_and_unlit_generic_vs30 vsh_forgot_to_set_static_VERTEXCOLOR + vsh_forgot_to_set_static_CUBEMAP + vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_FLASHLIGHT + vsh_forgot_to_set_static_SEAMLESS_BASE + vsh_forgot_to_set_static_SEAMLESS_DETAIL + vsh_forgot_to_set_static_SEPARATE_DETAIL_UVS + vsh_forgot_to_set_static_DECAL + vsh_forgot_to_set_static_DONT_GAMMA_CONVERT_VERTEX_COLOR + vsh_forgot_to_set_static_TREESWAY


class vertexlit_and_unlit_generic_vs30_Dynamic_Index
{
	unsigned int m_nCOMPRESSED_VERTS : 2;
	unsigned int m_nDYNAMIC_LIGHT : 2;
	unsigned int m_nSTATIC_LIGHT_VERTEX : 2;
	unsigned int m_nSTATIC_LIGHT_LIGHTMAP : 2;
	unsigned int m_nDOWATERFOG : 2;
	unsigned int m_nSKINNING : 2;
	unsigned int m_nLIGHTING_PREVIEW : 2;
	unsigned int m_nMORPHING : 2;
#ifdef _DEBUG
	bool m_bCOMPRESSED_VERTS : 1;
	bool m_bDYNAMIC_LIGHT : 1;
	bool m_bSTATIC_LIGHT_VERTEX : 1;
	bool m_bSTATIC_LIGHT_LIGHTMAP : 1;
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

	void SetDYNAMIC_LIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDYNAMIC_LIGHT = i;
#ifdef _DEBUG
		m_bDYNAMIC_LIGHT = true;
#endif	// _DEBUG
	}

	void SetSTATIC_LIGHT_VERTEX( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTATIC_LIGHT_VERTEX = i;
#ifdef _DEBUG
		m_bSTATIC_LIGHT_VERTEX = true;
#endif	// _DEBUG
	}

	void SetSTATIC_LIGHT_LIGHTMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTATIC_LIGHT_LIGHTMAP = i;
#ifdef _DEBUG
		m_bSTATIC_LIGHT_LIGHTMAP = true;
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

	vertexlit_and_unlit_generic_vs30_Dynamic_Index(  )
	{
		m_nCOMPRESSED_VERTS = 0;
		m_nDYNAMIC_LIGHT = 0;
		m_nSTATIC_LIGHT_VERTEX = 0;
		m_nSTATIC_LIGHT_LIGHTMAP = 0;
		m_nDOWATERFOG = 0;
		m_nSKINNING = 0;
		m_nLIGHTING_PREVIEW = 0;
		m_nMORPHING = 0;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
		m_bDYNAMIC_LIGHT = false;
		m_bSTATIC_LIGHT_VERTEX = false;
		m_bSTATIC_LIGHT_LIGHTMAP = false;
		m_bDOWATERFOG = false;
		m_bSKINNING = false;
		m_bLIGHTING_PREVIEW = false;
		m_bMORPHING = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCOMPRESSED_VERTS && m_bDYNAMIC_LIGHT && m_bSTATIC_LIGHT_VERTEX && m_bSTATIC_LIGHT_LIGHTMAP && m_bDOWATERFOG && m_bSKINNING && m_bLIGHTING_PREVIEW && m_bMORPHING );
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nDYNAMIC_LIGHT ) + ( 4 * m_nSTATIC_LIGHT_VERTEX ) + ( 8 * m_nSTATIC_LIGHT_LIGHTMAP ) + ( 16 * m_nDOWATERFOG ) + ( 32 * m_nSKINNING ) + ( 64 * m_nLIGHTING_PREVIEW ) + ( 128 * m_nMORPHING ) + 0;
	}
};

#define shaderDynamicTest_vertexlit_and_unlit_generic_vs30 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_DYNAMIC_LIGHT + vsh_forgot_to_set_dynamic_STATIC_LIGHT_VERTEX + vsh_forgot_to_set_dynamic_STATIC_LIGHT_LIGHTMAP + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_LIGHTING_PREVIEW + vsh_forgot_to_set_dynamic_MORPHING

