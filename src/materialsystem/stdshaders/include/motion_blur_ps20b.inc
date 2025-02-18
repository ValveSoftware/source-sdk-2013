#include "shaderlib/cshader.h"
class motion_blur_ps20b_Static_Index
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
	motion_blur_ps20b_Static_Index( )
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
		return ( 4 * m_nCONVERT_TO_SRGB ) + 0;
	}
};
#define shaderStaticTest_motion_blur_ps20b 0
class motion_blur_ps20b_Dynamic_Index
{
private:
	int m_nQUALITY;
#ifdef _DEBUG
	bool m_bQUALITY;
#endif
public:
	void SetQUALITY( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nQUALITY = i;
#ifdef _DEBUG
		m_bQUALITY = true;
#endif
	}
	void SetQUALITY( bool i )
	{
		m_nQUALITY = i ? 1 : 0;
#ifdef _DEBUG
		m_bQUALITY = true;
#endif
	}
public:
	motion_blur_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bQUALITY = false;
#endif // _DEBUG
		m_nQUALITY = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bQUALITY;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nQUALITY ) + 0;
	}
};
#define shaderDynamicTest_motion_blur_ps20b psh_forgot_to_set_dynamic_QUALITY + 0
