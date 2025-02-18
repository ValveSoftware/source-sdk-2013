// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// $USE_STATIC_CONTROL_FLOW && ( $NUM_LIGHTS > 0 )
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class vertexlit_and_unlit_generic_bump_vs20_Static_Index
{
	unsigned int m_nHALFLAMBERT : 2;
	unsigned int m_nUSE_WITH_2B : 2;
	unsigned int m_nUSE_STATIC_CONTROL_FLOW : 2;
#ifdef _DEBUG
	bool m_bHALFLAMBERT : 1;
	bool m_bUSE_WITH_2B : 1;
	bool m_bUSE_STATIC_CONTROL_FLOW : 1;
#endif	// _DEBUG
public:
	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif	// _DEBUG
	}

	void SetUSE_WITH_2B( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSE_WITH_2B = i;
#ifdef _DEBUG
		m_bUSE_WITH_2B = true;
#endif	// _DEBUG
	}

	void SetUSE_STATIC_CONTROL_FLOW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSE_STATIC_CONTROL_FLOW = i;
#ifdef _DEBUG
		m_bUSE_STATIC_CONTROL_FLOW = true;
#endif	// _DEBUG
	}

	vertexlit_and_unlit_generic_bump_vs20_Static_Index(  )
	{
		m_nHALFLAMBERT = 0;
		m_nUSE_WITH_2B = 0;
		m_nUSE_STATIC_CONTROL_FLOW = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
		m_bUSE_WITH_2B = false;
		m_bUSE_STATIC_CONTROL_FLOW = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bHALFLAMBERT && m_bUSE_WITH_2B && m_bUSE_STATIC_CONTROL_FLOW );
		return ( 24 * m_nHALFLAMBERT ) + ( 48 * m_nUSE_WITH_2B ) + ( 96 * m_nUSE_STATIC_CONTROL_FLOW ) + 0;
	}
};

#define shaderStaticTest_vertexlit_and_unlit_generic_bump_vs20 vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_USE_WITH_2B + vsh_forgot_to_set_static_USE_STATIC_CONTROL_FLOW


class vertexlit_and_unlit_generic_bump_vs20_Dynamic_Index
{
	unsigned int m_nCOMPRESSED_VERTS : 2;
	unsigned int m_nDOWATERFOG : 2;
	unsigned int m_nSKINNING : 2;
	unsigned int m_nNUM_LIGHTS : 2;
#ifdef _DEBUG
	bool m_bCOMPRESSED_VERTS : 1;
	bool m_bDOWATERFOG : 1;
	bool m_bSKINNING : 1;
	bool m_bNUM_LIGHTS : 1;
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

	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif	// _DEBUG
	}

	vertexlit_and_unlit_generic_bump_vs20_Dynamic_Index(  )
	{
		m_nCOMPRESSED_VERTS = 0;
		m_nDOWATERFOG = 0;
		m_nSKINNING = 0;
		m_nNUM_LIGHTS = 0;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
		m_bDOWATERFOG = false;
		m_bSKINNING = false;
		m_bNUM_LIGHTS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCOMPRESSED_VERTS && m_bDOWATERFOG && m_bSKINNING && m_bNUM_LIGHTS );
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nDOWATERFOG ) + ( 4 * m_nSKINNING ) + ( 8 * m_nNUM_LIGHTS ) + 0;
	}
};

#define shaderDynamicTest_vertexlit_and_unlit_generic_bump_vs20 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_NUM_LIGHTS

