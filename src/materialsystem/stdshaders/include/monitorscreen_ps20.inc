#include "shaderlib/cshader.h"
class monitorscreen_ps20_Static_Index
{
private:
	int m_nTEXTURE2;
#ifdef _DEBUG
	bool m_bTEXTURE2;
#endif
public:
	void SetTEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTEXTURE2 = i;
#ifdef _DEBUG
		m_bTEXTURE2 = true;
#endif
	}
	void SetTEXTURE2( bool i )
	{
		m_nTEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bTEXTURE2 = true;
#endif
	}
public:
	monitorscreen_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bTEXTURE2 = false;
#endif // _DEBUG
		m_nTEXTURE2 = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bTEXTURE2;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nTEXTURE2 ) + 0;
	}
};
#define shaderStaticTest_monitorscreen_ps20 psh_forgot_to_set_static_TEXTURE2 + 0
class monitorscreen_ps20_Dynamic_Index
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
	monitorscreen_ps20_Dynamic_Index()
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
#define shaderDynamicTest_monitorscreen_ps20 psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
