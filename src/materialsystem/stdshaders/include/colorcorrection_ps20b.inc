#include "shaderlib/cshader.h"
class colorcorrection_ps20b_Static_Index
{
private:
	int m_nCONVERT_TO_SRGB;
#ifdef _DEBUG
	bool m_bCONVERT_TO_SRGB;
#endif
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_TO_SRGB = i;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif
	}
	void SetCONVERT_TO_SRGB( bool i )
	{
		m_nCONVERT_TO_SRGB = i ? 1 : 0;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif
	}
public:
	colorcorrection_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 5 * m_nCONVERT_TO_SRGB ) + 0;
	}
};
#define shaderStaticTest_colorcorrection_ps20b 0
class colorcorrection_ps20b_Dynamic_Index
{
private:
	int m_nNUM_LOOKUPS;
#ifdef _DEBUG
	bool m_bNUM_LOOKUPS;
#endif
public:
	void SetNUM_LOOKUPS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LOOKUPS = i;
#ifdef _DEBUG
		m_bNUM_LOOKUPS = true;
#endif
	}
	void SetNUM_LOOKUPS( bool i )
	{
		m_nNUM_LOOKUPS = i ? 1 : 0;
#ifdef _DEBUG
		m_bNUM_LOOKUPS = true;
#endif
	}
public:
	colorcorrection_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bNUM_LOOKUPS = false;
#endif // _DEBUG
		m_nNUM_LOOKUPS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bNUM_LOOKUPS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nNUM_LOOKUPS ) + 0;
	}
};
#define shaderDynamicTest_colorcorrection_ps20b psh_forgot_to_set_dynamic_NUM_LOOKUPS + 0
