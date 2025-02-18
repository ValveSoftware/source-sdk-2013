#include "shaderlib/cshader.h"
class pyro_vision_vs20_Static_Index
{
private:
	int m_nEFFECT;
#ifdef _DEBUG
	bool m_bEFFECT;
#endif
public:
	void SetEFFECT( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nEFFECT = i;
#ifdef _DEBUG
		m_bEFFECT = true;
#endif
	}
	void SetEFFECT( bool i )
	{
		m_nEFFECT = i ? 1 : 0;
#ifdef _DEBUG
		m_bEFFECT = true;
#endif
	}
private:
	int m_nVERTEXCOLOR;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR;
#endif
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
	void SetVERTEXCOLOR( bool i )
	{
		m_nVERTEXCOLOR = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
private:
	int m_nHALFLAMBERT;
#ifdef _DEBUG
	bool m_bHALFLAMBERT;
#endif
public:
	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif
	}
	void SetHALFLAMBERT( bool i )
	{
		m_nHALFLAMBERT = i ? 1 : 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif
	}
private:
	int m_nVERTEX_LIT;
#ifdef _DEBUG
	bool m_bVERTEX_LIT;
#endif
public:
	void SetVERTEX_LIT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEX_LIT = i;
#ifdef _DEBUG
		m_bVERTEX_LIT = true;
#endif
	}
	void SetVERTEX_LIT( bool i )
	{
		m_nVERTEX_LIT = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEX_LIT = true;
#endif
	}
private:
	int m_nFULLBRIGHT;
#ifdef _DEBUG
	bool m_bFULLBRIGHT;
#endif
public:
	void SetFULLBRIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFULLBRIGHT = i;
#ifdef _DEBUG
		m_bFULLBRIGHT = true;
#endif
	}
	void SetFULLBRIGHT( bool i )
	{
		m_nFULLBRIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bFULLBRIGHT = true;
#endif
	}
private:
	int m_nUSE_STATIC_CONTROL_FLOW;
#ifdef _DEBUG
	bool m_bUSE_STATIC_CONTROL_FLOW;
#endif
public:
	void SetUSE_STATIC_CONTROL_FLOW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSE_STATIC_CONTROL_FLOW = i;
#ifdef _DEBUG
		m_bUSE_STATIC_CONTROL_FLOW = true;
#endif
	}
	void SetUSE_STATIC_CONTROL_FLOW( bool i )
	{
		m_nUSE_STATIC_CONTROL_FLOW = i ? 1 : 0;
#ifdef _DEBUG
		m_bUSE_STATIC_CONTROL_FLOW = true;
#endif
	}
private:
	int m_nBASETEXTURE2;
#ifdef _DEBUG
	bool m_bBASETEXTURE2;
#endif
public:
	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
	void SetBASETEXTURE2( bool i )
	{
		m_nBASETEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
private:
	int m_nSTRIPES;
#ifdef _DEBUG
	bool m_bSTRIPES;
#endif
public:
	void SetSTRIPES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES = i;
#ifdef _DEBUG
		m_bSTRIPES = true;
#endif
	}
	void SetSTRIPES( bool i )
	{
		m_nSTRIPES = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTRIPES = true;
#endif
	}
private:
	int m_nSTRIPES_USE_NORMAL2;
#ifdef _DEBUG
	bool m_bSTRIPES_USE_NORMAL2;
#endif
public:
	void SetSTRIPES_USE_NORMAL2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES_USE_NORMAL2 = i;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = true;
#endif
	}
	void SetSTRIPES_USE_NORMAL2( bool i )
	{
		m_nSTRIPES_USE_NORMAL2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = true;
#endif
	}
public:
	pyro_vision_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bEFFECT = false;
#endif // _DEBUG
		m_nEFFECT = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bVERTEX_LIT = false;
#endif // _DEBUG
		m_nVERTEX_LIT = 0;
#ifdef _DEBUG
		m_bFULLBRIGHT = false;
#endif // _DEBUG
		m_nFULLBRIGHT = 0;
#ifdef _DEBUG
		m_bUSE_STATIC_CONTROL_FLOW = false;
#endif // _DEBUG
		m_nUSE_STATIC_CONTROL_FLOW = 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = false;
#endif // _DEBUG
		m_nBASETEXTURE2 = 0;
#ifdef _DEBUG
		m_bSTRIPES = false;
#endif // _DEBUG
		m_nSTRIPES = 0;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = false;
#endif // _DEBUG
		m_nSTRIPES_USE_NORMAL2 = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bEFFECT && m_bVERTEXCOLOR && m_bHALFLAMBERT && m_bVERTEX_LIT && m_bFULLBRIGHT && m_bUSE_STATIC_CONTROL_FLOW && m_bBASETEXTURE2 && m_bSTRIPES && m_bSTRIPES_USE_NORMAL2;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 48 * m_nEFFECT ) + ( 192 * m_nVERTEXCOLOR ) + ( 384 * m_nHALFLAMBERT ) + ( 768 * m_nVERTEX_LIT ) + ( 1536 * m_nFULLBRIGHT ) + ( 3072 * m_nUSE_STATIC_CONTROL_FLOW ) + ( 6144 * m_nBASETEXTURE2 ) + ( 12288 * m_nSTRIPES ) + ( 24576 * m_nSTRIPES_USE_NORMAL2 ) + 0;
	}
};
#define shaderStaticTest_pyro_vision_vs20 vsh_forgot_to_set_static_EFFECT + vsh_forgot_to_set_static_VERTEXCOLOR + vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_VERTEX_LIT + vsh_forgot_to_set_static_FULLBRIGHT + vsh_forgot_to_set_static_USE_STATIC_CONTROL_FLOW + vsh_forgot_to_set_static_BASETEXTURE2 + vsh_forgot_to_set_static_STRIPES + vsh_forgot_to_set_static_STRIPES_USE_NORMAL2 + 0
class pyro_vision_vs20_Dynamic_Index
{
private:
	int m_nCOMPRESSED_VERTS;
#ifdef _DEBUG
	bool m_bCOMPRESSED_VERTS;
#endif
public:
	void SetCOMPRESSED_VERTS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOMPRESSED_VERTS = i;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = true;
#endif
	}
	void SetCOMPRESSED_VERTS( bool i )
	{
		m_nCOMPRESSED_VERTS = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = true;
#endif
	}
private:
	int m_nSKINNING;
#ifdef _DEBUG
	bool m_bSKINNING;
#endif
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
	void SetSKINNING( bool i )
	{
		m_nSKINNING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
private:
	int m_nDYNAMIC_LIGHT;
#ifdef _DEBUG
	bool m_bDYNAMIC_LIGHT;
#endif
public:
	void SetDYNAMIC_LIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDYNAMIC_LIGHT = i;
#ifdef _DEBUG
		m_bDYNAMIC_LIGHT = true;
#endif
	}
	void SetDYNAMIC_LIGHT( bool i )
	{
		m_nDYNAMIC_LIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bDYNAMIC_LIGHT = true;
#endif
	}
private:
	int m_nSTATIC_LIGHT;
#ifdef _DEBUG
	bool m_bSTATIC_LIGHT;
#endif
public:
	void SetSTATIC_LIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTATIC_LIGHT = i;
#ifdef _DEBUG
		m_bSTATIC_LIGHT = true;
#endif
	}
	void SetSTATIC_LIGHT( bool i )
	{
		m_nSTATIC_LIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTATIC_LIGHT = true;
#endif
	}
private:
	int m_nNUM_LIGHTS;
#ifdef _DEBUG
	bool m_bNUM_LIGHTS;
#endif
public:
	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif
	}
	void SetNUM_LIGHTS( bool i )
	{
		m_nNUM_LIGHTS = i ? 1 : 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif
	}
public:
	pyro_vision_vs20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
#endif // _DEBUG
		m_nCOMPRESSED_VERTS = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
#ifdef _DEBUG
		m_bDYNAMIC_LIGHT = false;
#endif // _DEBUG
		m_nDYNAMIC_LIGHT = 0;
#ifdef _DEBUG
		m_bSTATIC_LIGHT = false;
#endif // _DEBUG
		m_nSTATIC_LIGHT = 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = false;
#endif // _DEBUG
		m_nNUM_LIGHTS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bCOMPRESSED_VERTS && m_bSKINNING && m_bDYNAMIC_LIGHT && m_bSTATIC_LIGHT && m_bNUM_LIGHTS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nSKINNING ) + ( 4 * m_nDYNAMIC_LIGHT ) + ( 8 * m_nSTATIC_LIGHT ) + ( 16 * m_nNUM_LIGHTS ) + 0;
	}
};
#define shaderDynamicTest_pyro_vision_vs20 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_DYNAMIC_LIGHT + vsh_forgot_to_set_dynamic_STATIC_LIGHT + vsh_forgot_to_set_dynamic_NUM_LIGHTS + 0
