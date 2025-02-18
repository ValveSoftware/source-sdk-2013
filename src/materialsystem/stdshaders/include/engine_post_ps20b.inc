#include "shaderlib/cshader.h"
class engine_post_ps20b_Static_Index
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
private:
	int m_nLINEAR_INPUT;
#ifdef _DEBUG
	bool m_bLINEAR_INPUT;
#endif
public:
	void SetLINEAR_INPUT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLINEAR_INPUT = i;
#ifdef _DEBUG
		m_bLINEAR_INPUT = true;
#endif
	}
	void SetLINEAR_INPUT( bool i )
	{
		m_nLINEAR_INPUT = i ? 1 : 0;
#ifdef _DEBUG
		m_bLINEAR_INPUT = true;
#endif
	}
private:
	int m_nLINEAR_OUTPUT;
#ifdef _DEBUG
	bool m_bLINEAR_OUTPUT;
#endif
public:
	void SetLINEAR_OUTPUT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLINEAR_OUTPUT = i;
#ifdef _DEBUG
		m_bLINEAR_OUTPUT = true;
#endif
	}
	void SetLINEAR_OUTPUT( bool i )
	{
		m_nLINEAR_OUTPUT = i ? 1 : 0;
#ifdef _DEBUG
		m_bLINEAR_OUTPUT = true;
#endif
	}
public:
	engine_post_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
#ifdef _DEBUG
		m_bLINEAR_INPUT = false;
#endif // _DEBUG
		m_nLINEAR_INPUT = 0;
#ifdef _DEBUG
		m_bLINEAR_OUTPUT = false;
#endif // _DEBUG
		m_nLINEAR_OUTPUT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bLINEAR_INPUT && m_bLINEAR_OUTPUT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 40 * m_nCONVERT_TO_SRGB ) + ( 80 * m_nLINEAR_INPUT ) + ( 160 * m_nLINEAR_OUTPUT ) + 0;
	}
};
#define shaderStaticTest_engine_post_ps20b psh_forgot_to_set_static_LINEAR_INPUT + psh_forgot_to_set_static_LINEAR_OUTPUT + 0
class engine_post_ps20b_Dynamic_Index
{
private:
	int m_nAA_ENABLE;
#ifdef _DEBUG
	bool m_bAA_ENABLE;
#endif
public:
	void SetAA_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nAA_ENABLE = i;
#ifdef _DEBUG
		m_bAA_ENABLE = true;
#endif
	}
	void SetAA_ENABLE( bool i )
	{
		m_nAA_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bAA_ENABLE = true;
#endif
	}
private:
	int m_nAA_QUALITY_MODE;
#ifdef _DEBUG
	bool m_bAA_QUALITY_MODE;
#endif
public:
	void SetAA_QUALITY_MODE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nAA_QUALITY_MODE = i;
#ifdef _DEBUG
		m_bAA_QUALITY_MODE = true;
#endif
	}
	void SetAA_QUALITY_MODE( bool i )
	{
		m_nAA_QUALITY_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bAA_QUALITY_MODE = true;
#endif
	}
private:
	int m_nAA_REDUCE_ONE_PIXEL_LINE_BLUR;
#ifdef _DEBUG
	bool m_bAA_REDUCE_ONE_PIXEL_LINE_BLUR;
#endif
public:
	void SetAA_REDUCE_ONE_PIXEL_LINE_BLUR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nAA_REDUCE_ONE_PIXEL_LINE_BLUR = i;
#ifdef _DEBUG
		m_bAA_REDUCE_ONE_PIXEL_LINE_BLUR = true;
#endif
	}
	void SetAA_REDUCE_ONE_PIXEL_LINE_BLUR( bool i )
	{
		m_nAA_REDUCE_ONE_PIXEL_LINE_BLUR = i ? 1 : 0;
#ifdef _DEBUG
		m_bAA_REDUCE_ONE_PIXEL_LINE_BLUR = true;
#endif
	}
private:
	int m_nCOL_CORRECT_NUM_LOOKUPS;
#ifdef _DEBUG
	bool m_bCOL_CORRECT_NUM_LOOKUPS;
#endif
public:
	void SetCOL_CORRECT_NUM_LOOKUPS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nCOL_CORRECT_NUM_LOOKUPS = i;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = true;
#endif
	}
	void SetCOL_CORRECT_NUM_LOOKUPS( bool i )
	{
		m_nCOL_CORRECT_NUM_LOOKUPS = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = true;
#endif
	}
public:
	engine_post_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bAA_ENABLE = false;
#endif // _DEBUG
		m_nAA_ENABLE = 0;
#ifdef _DEBUG
		m_bAA_QUALITY_MODE = false;
#endif // _DEBUG
		m_nAA_QUALITY_MODE = 0;
#ifdef _DEBUG
		m_bAA_REDUCE_ONE_PIXEL_LINE_BLUR = false;
#endif // _DEBUG
		m_nAA_REDUCE_ONE_PIXEL_LINE_BLUR = 0;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = false;
#endif // _DEBUG
		m_nCOL_CORRECT_NUM_LOOKUPS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bAA_ENABLE && m_bAA_QUALITY_MODE && m_bAA_REDUCE_ONE_PIXEL_LINE_BLUR && m_bCOL_CORRECT_NUM_LOOKUPS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nAA_ENABLE ) + ( 2 * m_nAA_QUALITY_MODE ) + ( 4 * m_nAA_REDUCE_ONE_PIXEL_LINE_BLUR ) + ( 8 * m_nCOL_CORRECT_NUM_LOOKUPS ) + 0;
	}
};
#define shaderDynamicTest_engine_post_ps20b psh_forgot_to_set_dynamic_AA_ENABLE + psh_forgot_to_set_dynamic_AA_QUALITY_MODE + psh_forgot_to_set_dynamic_AA_REDUCE_ONE_PIXEL_LINE_BLUR + psh_forgot_to_set_dynamic_COL_CORRECT_NUM_LOOKUPS + 0
