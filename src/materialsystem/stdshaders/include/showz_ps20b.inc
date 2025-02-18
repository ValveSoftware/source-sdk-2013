#include "shaderlib/cshader.h"
class showz_ps20b_Static_Index
{
private:
	int m_nDEPTH_IN_ALPHA;
#ifdef _DEBUG
	bool m_bDEPTH_IN_ALPHA;
#endif
public:
	void SetDEPTH_IN_ALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDEPTH_IN_ALPHA = i;
#ifdef _DEBUG
		m_bDEPTH_IN_ALPHA = true;
#endif
	}
	void SetDEPTH_IN_ALPHA( bool i )
	{
		m_nDEPTH_IN_ALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bDEPTH_IN_ALPHA = true;
#endif
	}
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
	showz_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bDEPTH_IN_ALPHA = false;
#endif // _DEBUG
		m_nDEPTH_IN_ALPHA = 0;
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
		bool bAllStaticVarsDefined = m_bDEPTH_IN_ALPHA && m_bCONVERT_TO_SRGB;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDEPTH_IN_ALPHA ) + ( 2 * m_nCONVERT_TO_SRGB ) + 0;
	}
};
#define shaderStaticTest_showz_ps20b psh_forgot_to_set_static_DEPTH_IN_ALPHA + 0
class showz_ps20b_Dynamic_Index
{
public:
	showz_ps20b_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_showz_ps20b 0
