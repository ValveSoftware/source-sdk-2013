#include "shaderlib/cshader.h"
class core_ps20b_Static_Index
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
	int m_nCUBEMAP;
#ifdef _DEBUG
	bool m_bCUBEMAP;
#endif
public:
	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif
	}
	void SetCUBEMAP( bool i )
	{
		m_nCUBEMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif
	}
private:
	int m_nFLOWMAP;
#ifdef _DEBUG
	bool m_bFLOWMAP;
#endif
public:
	void SetFLOWMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLOWMAP = i;
#ifdef _DEBUG
		m_bFLOWMAP = true;
#endif
	}
	void SetFLOWMAP( bool i )
	{
		m_nFLOWMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLOWMAP = true;
#endif
	}
private:
	int m_nCORECOLORTEXTURE;
#ifdef _DEBUG
	bool m_bCORECOLORTEXTURE;
#endif
public:
	void SetCORECOLORTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCORECOLORTEXTURE = i;
#ifdef _DEBUG
		m_bCORECOLORTEXTURE = true;
#endif
	}
	void SetCORECOLORTEXTURE( bool i )
	{
		m_nCORECOLORTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bCORECOLORTEXTURE = true;
#endif
	}
private:
	int m_nREFRACT;
#ifdef _DEBUG
	bool m_bREFRACT;
#endif
public:
	void SetREFRACT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nREFRACT = i;
#ifdef _DEBUG
		m_bREFRACT = true;
#endif
	}
	void SetREFRACT( bool i )
	{
		m_nREFRACT = i ? 1 : 0;
#ifdef _DEBUG
		m_bREFRACT = true;
#endif
	}
public:
	core_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
#ifdef _DEBUG
		m_bCUBEMAP = false;
#endif // _DEBUG
		m_nCUBEMAP = 0;
#ifdef _DEBUG
		m_bFLOWMAP = false;
#endif // _DEBUG
		m_nFLOWMAP = 0;
#ifdef _DEBUG
		m_bCORECOLORTEXTURE = false;
#endif // _DEBUG
		m_nCORECOLORTEXTURE = 0;
#ifdef _DEBUG
		m_bREFRACT = false;
#endif // _DEBUG
		m_nREFRACT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bCUBEMAP && m_bFLOWMAP && m_bCORECOLORTEXTURE && m_bREFRACT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nCONVERT_TO_SRGB ) + ( 4 * m_nCUBEMAP ) + ( 8 * m_nFLOWMAP ) + ( 16 * m_nCORECOLORTEXTURE ) + ( 32 * m_nREFRACT ) + 0;
	}
};
#define shaderStaticTest_core_ps20b psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_FLOWMAP + psh_forgot_to_set_static_CORECOLORTEXTURE + psh_forgot_to_set_static_REFRACT + 0
class core_ps20b_Dynamic_Index
{
private:
	int m_nPIXELFOGTYPE;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE;
#endif
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
	void SetPIXELFOGTYPE( bool i )
	{
		m_nPIXELFOGTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
public:
	core_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_core_ps20b psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
