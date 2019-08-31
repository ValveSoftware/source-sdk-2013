#include "shaderlib/cshader.h"
class sdk_eye_refract_vs20_Static_Index
{
private:
	int m_nINTRO;
#ifdef _DEBUG
	bool m_bINTRO;
#endif
public:
	void SetINTRO( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nINTRO = i;
#ifdef _DEBUG
		m_bINTRO = true;
#endif
	}
	void SetINTRO( bool i )
	{
		m_nINTRO = i ? 1 : 0;
#ifdef _DEBUG
		m_bINTRO = true;
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
	int m_nFLASHLIGHT;
#ifdef _DEBUG
	bool m_bFLASHLIGHT;
#endif
public:
	void SetFLASHLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHT = i;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
	void SetFLASHLIGHT( bool i )
	{
		m_nFLASHLIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
private:
	int m_nLIGHTWARPTEXTURE;
#ifdef _DEBUG
	bool m_bLIGHTWARPTEXTURE;
#endif
public:
	void SetLIGHTWARPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLIGHTWARPTEXTURE = i;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = true;
#endif
	}
	void SetLIGHTWARPTEXTURE( bool i )
	{
		m_nLIGHTWARPTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = true;
#endif
	}
public:
	sdk_eye_refract_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bINTRO = false;
#endif // _DEBUG
		m_nINTRO = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = false;
#endif // _DEBUG
		m_nFLASHLIGHT = 0;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = false;
#endif // _DEBUG
		m_nLIGHTWARPTEXTURE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bINTRO && m_bHALFLAMBERT && m_bFLASHLIGHT && m_bLIGHTWARPTEXTURE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 160 * m_nINTRO ) + ( 320 * m_nHALFLAMBERT ) + ( 640 * m_nFLASHLIGHT ) + ( 1280 * m_nLIGHTWARPTEXTURE ) + 0;
	}
};
#define shaderStaticTest_sdk_eye_refract_vs20 vsh_forgot_to_set_static_INTRO + vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_FLASHLIGHT + vsh_forgot_to_set_static_LIGHTWARPTEXTURE + 0
class sdk_eye_refract_vs20_Dynamic_Index
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
	int m_nDOWATERFOG;
#ifdef _DEBUG
	bool m_bDOWATERFOG;
#endif
public:
	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif
	}
	void SetDOWATERFOG( bool i )
	{
		m_nDOWATERFOG = i ? 1 : 0;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
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
		Assert( i >= 0 && i <= 4 );
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
	sdk_eye_refract_vs20_Dynamic_Index()
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
		m_bDOWATERFOG = false;
#endif // _DEBUG
		m_nDOWATERFOG = 0;
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
		bool bAllDynamicVarsDefined = m_bCOMPRESSED_VERTS && m_bSKINNING && m_bDOWATERFOG && m_bDYNAMIC_LIGHT && m_bSTATIC_LIGHT && m_bNUM_LIGHTS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nSKINNING ) + ( 4 * m_nDOWATERFOG ) + ( 8 * m_nDYNAMIC_LIGHT ) + ( 16 * m_nSTATIC_LIGHT ) + ( 32 * m_nNUM_LIGHTS ) + 0;
	}
};
#define shaderDynamicTest_sdk_eye_refract_vs20 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_DYNAMIC_LIGHT + vsh_forgot_to_set_dynamic_STATIC_LIGHT + vsh_forgot_to_set_dynamic_NUM_LIGHTS + 0
