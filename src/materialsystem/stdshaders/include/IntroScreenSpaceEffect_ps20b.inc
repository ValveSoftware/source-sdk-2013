#include "shaderlib/cshader.h"
class introscreenspaceeffect_ps20b_Static_Index
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
	int m_nLINEAR_TO_SRGB;
#ifdef _DEBUG
	bool m_bLINEAR_TO_SRGB;
#endif
public:
	void SetLINEAR_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLINEAR_TO_SRGB = i;
#ifdef _DEBUG
		m_bLINEAR_TO_SRGB = true;
#endif
	}
	void SetLINEAR_TO_SRGB( bool i )
	{
		m_nLINEAR_TO_SRGB = i ? 1 : 0;
#ifdef _DEBUG
		m_bLINEAR_TO_SRGB = true;
#endif
	}
public:
	introscreenspaceeffect_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
#ifdef _DEBUG
		m_bLINEAR_TO_SRGB = false;
#endif // _DEBUG
		m_nLINEAR_TO_SRGB = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bLINEAR_TO_SRGB;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 10 * m_nCONVERT_TO_SRGB ) + ( 20 * m_nLINEAR_TO_SRGB ) + 0;
	}
};
#define shaderStaticTest_introscreenspaceeffect_ps20b psh_forgot_to_set_static_LINEAR_TO_SRGB + 0
class introscreenspaceeffect_ps20b_Dynamic_Index
{
private:
	int m_nMODE;
#ifdef _DEBUG
	bool m_bMODE;
#endif
public:
	void SetMODE( int i )
	{
		Assert( i >= 0 && i <= 9 );
		m_nMODE = i;
#ifdef _DEBUG
		m_bMODE = true;
#endif
	}
	void SetMODE( bool i )
	{
		m_nMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bMODE = true;
#endif
	}
public:
	introscreenspaceeffect_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bMODE = false;
#endif // _DEBUG
		m_nMODE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bMODE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nMODE ) + 0;
	}
};
#define shaderDynamicTest_introscreenspaceeffect_ps20b psh_forgot_to_set_dynamic_MODE + 0
