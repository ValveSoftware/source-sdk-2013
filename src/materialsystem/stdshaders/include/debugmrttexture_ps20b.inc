#include "shaderlib/cshader.h"
class debugmrttexture_ps20b_Static_Index
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
	int m_nMRTINDEX;
#ifdef _DEBUG
	bool m_bMRTINDEX;
#endif
public:
	void SetMRTINDEX( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMRTINDEX = i;
#ifdef _DEBUG
		m_bMRTINDEX = true;
#endif
	}
	void SetMRTINDEX( bool i )
	{
		m_nMRTINDEX = i ? 1 : 0;
#ifdef _DEBUG
		m_bMRTINDEX = true;
#endif
	}
public:
	debugmrttexture_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
#ifdef _DEBUG
		m_bMRTINDEX = false;
#endif // _DEBUG
		m_nMRTINDEX = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bMRTINDEX;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCONVERT_TO_SRGB ) + ( 2 * m_nMRTINDEX ) + 0;
	}
};
#define shaderStaticTest_debugmrttexture_ps20b psh_forgot_to_set_static_MRTINDEX + 0
class debugmrttexture_ps20b_Dynamic_Index
{
public:
	debugmrttexture_ps20b_Dynamic_Index()
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
#define shaderDynamicTest_debugmrttexture_ps20b 0
