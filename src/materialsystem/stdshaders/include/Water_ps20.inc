#include "shaderlib/cshader.h"
class water_ps20_Static_Index
{
private:
	int m_nBASETEXTURE;
#ifdef _DEBUG
	bool m_bBASETEXTURE;
#endif
public:
	void SetBASETEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE = i;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
	void SetBASETEXTURE( bool i )
	{
		m_nBASETEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
private:
	int m_nMULTITEXTURE;
#ifdef _DEBUG
	bool m_bMULTITEXTURE;
#endif
public:
	void SetMULTITEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMULTITEXTURE = i;
#ifdef _DEBUG
		m_bMULTITEXTURE = true;
#endif
	}
	void SetMULTITEXTURE( bool i )
	{
		m_nMULTITEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bMULTITEXTURE = true;
#endif
	}
private:
	int m_nREFLECT;
#ifdef _DEBUG
	bool m_bREFLECT;
#endif
public:
	void SetREFLECT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nREFLECT = i;
#ifdef _DEBUG
		m_bREFLECT = true;
#endif
	}
	void SetREFLECT( bool i )
	{
		m_nREFLECT = i ? 1 : 0;
#ifdef _DEBUG
		m_bREFLECT = true;
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
private:
	int m_nABOVEWATER;
#ifdef _DEBUG
	bool m_bABOVEWATER;
#endif
public:
	void SetABOVEWATER( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nABOVEWATER = i;
#ifdef _DEBUG
		m_bABOVEWATER = true;
#endif
	}
	void SetABOVEWATER( bool i )
	{
		m_nABOVEWATER = i ? 1 : 0;
#ifdef _DEBUG
		m_bABOVEWATER = true;
#endif
	}
private:
	int m_nNORMAL_DECODE_MODE;
#ifdef _DEBUG
	bool m_bNORMAL_DECODE_MODE;
#endif
public:
	void SetNORMAL_DECODE_MODE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nNORMAL_DECODE_MODE = i;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = true;
#endif
	}
	void SetNORMAL_DECODE_MODE( bool i )
	{
		m_nNORMAL_DECODE_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = true;
#endif
	}
public:
	water_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bBASETEXTURE = false;
#endif // _DEBUG
		m_nBASETEXTURE = 0;
#ifdef _DEBUG
		m_bMULTITEXTURE = false;
#endif // _DEBUG
		m_nMULTITEXTURE = 0;
#ifdef _DEBUG
		m_bREFLECT = false;
#endif // _DEBUG
		m_nREFLECT = 0;
#ifdef _DEBUG
		m_bREFRACT = false;
#endif // _DEBUG
		m_nREFRACT = 0;
#ifdef _DEBUG
		m_bABOVEWATER = false;
#endif // _DEBUG
		m_nABOVEWATER = 0;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = false;
#endif // _DEBUG
		m_nNORMAL_DECODE_MODE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bBASETEXTURE && m_bMULTITEXTURE && m_bREFLECT && m_bREFRACT && m_bABOVEWATER && m_bNORMAL_DECODE_MODE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nBASETEXTURE ) + ( 4 * m_nMULTITEXTURE ) + ( 8 * m_nREFLECT ) + ( 16 * m_nREFRACT ) + ( 32 * m_nABOVEWATER ) + ( 64 * m_nNORMAL_DECODE_MODE ) + 0;
	}
};
#define shaderStaticTest_water_ps20 psh_forgot_to_set_static_BASETEXTURE + psh_forgot_to_set_static_MULTITEXTURE + psh_forgot_to_set_static_REFLECT + psh_forgot_to_set_static_REFRACT + psh_forgot_to_set_static_ABOVEWATER + psh_forgot_to_set_static_NORMAL_DECODE_MODE + 0
class water_ps20_Dynamic_Index
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
	water_ps20_Dynamic_Index()
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
#define shaderDynamicTest_water_ps20 psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
