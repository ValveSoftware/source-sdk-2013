#include "shaderlib/cshader.h"
class sdk_vertexlit_and_unlit_generic_bump_vs30_Static_Index
{
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
	int m_nUSE_WITH_2B;
#ifdef _DEBUG
	bool m_bUSE_WITH_2B;
#endif
public:
	void SetUSE_WITH_2B( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSE_WITH_2B = i;
#ifdef _DEBUG
		m_bUSE_WITH_2B = true;
#endif
	}
	void SetUSE_WITH_2B( bool i )
	{
		m_nUSE_WITH_2B = i ? 1 : 0;
#ifdef _DEBUG
		m_bUSE_WITH_2B = true;
#endif
	}
private:
	int m_nDECAL;
#ifdef _DEBUG
	bool m_bDECAL;
#endif
public:
	void SetDECAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDECAL = i;
#ifdef _DEBUG
		m_bDECAL = true;
#endif
	}
	void SetDECAL( bool i )
	{
		m_nDECAL = i ? 1 : 0;
#ifdef _DEBUG
		m_bDECAL = true;
#endif
	}
private:
	int m_nSM30_VERTEXID;
#ifdef _DEBUG
	bool m_bSM30_VERTEXID;
#endif
public:
	void SetSM30_VERTEXID( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSM30_VERTEXID = i;
#ifdef _DEBUG
		m_bSM30_VERTEXID = true;
#endif
	}
	void SetSM30_VERTEXID( bool i )
	{
		m_nSM30_VERTEXID = i ? 1 : 0;
#ifdef _DEBUG
		m_bSM30_VERTEXID = true;
#endif
	}
public:
	sdk_vertexlit_and_unlit_generic_bump_vs30_Static_Index( )
	{
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bUSE_WITH_2B = false;
#endif // _DEBUG
		m_nUSE_WITH_2B = 0;
#ifdef _DEBUG
		m_bDECAL = false;
#endif // _DEBUG
		m_nDECAL = 0;
#ifdef _DEBUG
		m_bSM30_VERTEXID = false;
#endif // _DEBUG
		m_nSM30_VERTEXID = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bHALFLAMBERT && m_bUSE_WITH_2B && m_bDECAL && m_bSM30_VERTEXID;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 16 * m_nHALFLAMBERT ) + ( 32 * m_nUSE_WITH_2B ) + ( 64 * m_nDECAL ) + ( 128 * m_nSM30_VERTEXID ) + 0;
	}
};
#define shaderStaticTest_sdk_vertexlit_and_unlit_generic_bump_vs30 vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_USE_WITH_2B + vsh_forgot_to_set_static_DECAL + vsh_forgot_to_set_static_SM30_VERTEXID + 0
class sdk_vertexlit_and_unlit_generic_bump_vs30_Dynamic_Index
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
	int m_nMORPHING;
#ifdef _DEBUG
	bool m_bMORPHING;
#endif
public:
	void SetMORPHING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMORPHING = i;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
	void SetMORPHING( bool i )
	{
		m_nMORPHING = i ? 1 : 0;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
public:
	sdk_vertexlit_and_unlit_generic_bump_vs30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
#endif // _DEBUG
		m_nCOMPRESSED_VERTS = 0;
#ifdef _DEBUG
		m_bDOWATERFOG = false;
#endif // _DEBUG
		m_nDOWATERFOG = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
#ifdef _DEBUG
		m_bMORPHING = false;
#endif // _DEBUG
		m_nMORPHING = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bCOMPRESSED_VERTS && m_bDOWATERFOG && m_bSKINNING && m_bMORPHING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nDOWATERFOG ) + ( 4 * m_nSKINNING ) + ( 8 * m_nMORPHING ) + 0;
	}
};
#define shaderDynamicTest_sdk_vertexlit_and_unlit_generic_bump_vs30 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_MORPHING + 0
