#include "shaderlib/cshader.h"
class eyes_cloak_vs20b_Static_Index
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
public:
	eyes_cloak_vs20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bINTRO = false;
#endif // _DEBUG
		m_nINTRO = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bINTRO && m_bHALFLAMBERT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 32 * m_nINTRO ) + ( 64 * m_nHALFLAMBERT ) + 0;
	}
};
#define shaderStaticTest_eyes_cloak_vs20b vsh_forgot_to_set_static_INTRO + vsh_forgot_to_set_static_HALFLAMBERT + 0
class eyes_cloak_vs20b_Dynamic_Index
{
private:
	int m_nNUM_BONES;
#ifdef _DEBUG
	bool m_bNUM_BONES;
#endif
public:
	void SetNUM_BONES( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nNUM_BONES = i;
#ifdef _DEBUG
		m_bNUM_BONES = true;
#endif
	}
	void SetNUM_BONES( bool i )
	{
		m_nNUM_BONES = i ? 1 : 0;
#ifdef _DEBUG
		m_bNUM_BONES = true;
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
public:
	eyes_cloak_vs20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bNUM_BONES = false;
#endif // _DEBUG
		m_nNUM_BONES = 0;
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
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bNUM_BONES && m_bDOWATERFOG && m_bDYNAMIC_LIGHT && m_bSTATIC_LIGHT;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nNUM_BONES ) + ( 4 * m_nDOWATERFOG ) + ( 8 * m_nDYNAMIC_LIGHT ) + ( 16 * m_nSTATIC_LIGHT ) + 0;
	}
};
#define shaderDynamicTest_eyes_cloak_vs20b vsh_forgot_to_set_dynamic_NUM_BONES + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_DYNAMIC_LIGHT + vsh_forgot_to_set_dynamic_STATIC_LIGHT + 0
