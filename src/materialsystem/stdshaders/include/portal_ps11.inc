#include "shaderlib/cshader.h"
class portal_ps11_Static_Index
{
private:
	int m_nMAXTEXTURESTAGES;
#ifdef _DEBUG
	bool m_bMAXTEXTURESTAGES;
#endif
public:
	void SetMAXTEXTURESTAGES( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nMAXTEXTURESTAGES = i;
#ifdef _DEBUG
		m_bMAXTEXTURESTAGES = true;
#endif
	}
	void SetMAXTEXTURESTAGES( bool i )
	{
		m_nMAXTEXTURESTAGES = i ? 1 : 0;
#ifdef _DEBUG
		m_bMAXTEXTURESTAGES = true;
#endif
	}
private:
	int m_nHASALPHAMASK;
#ifdef _DEBUG
	bool m_bHASALPHAMASK;
#endif
public:
	void SetHASALPHAMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHASALPHAMASK = i;
#ifdef _DEBUG
		m_bHASALPHAMASK = true;
#endif
	}
	void SetHASALPHAMASK( bool i )
	{
		m_nHASALPHAMASK = i ? 1 : 0;
#ifdef _DEBUG
		m_bHASALPHAMASK = true;
#endif
	}
private:
	int m_nHASSTATICTEXTURE;
#ifdef _DEBUG
	bool m_bHASSTATICTEXTURE;
#endif
public:
	void SetHASSTATICTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHASSTATICTEXTURE = i;
#ifdef _DEBUG
		m_bHASSTATICTEXTURE = true;
#endif
	}
	void SetHASSTATICTEXTURE( bool i )
	{
		m_nHASSTATICTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bHASSTATICTEXTURE = true;
#endif
	}
public:
	portal_ps11_Static_Index( )
	{
#ifdef _DEBUG
		m_bMAXTEXTURESTAGES = false;
#endif // _DEBUG
		m_nMAXTEXTURESTAGES = 0;
#ifdef _DEBUG
		m_bHASALPHAMASK = false;
#endif // _DEBUG
		m_nHASALPHAMASK = 0;
#ifdef _DEBUG
		m_bHASSTATICTEXTURE = false;
#endif // _DEBUG
		m_nHASSTATICTEXTURE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bMAXTEXTURESTAGES && m_bHASALPHAMASK && m_bHASSTATICTEXTURE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nMAXTEXTURESTAGES ) + ( 12 * m_nHASALPHAMASK ) + ( 24 * m_nHASSTATICTEXTURE ) + 0;
	}
};
#define shaderStaticTest_portal_ps11 psh_forgot_to_set_static_MAXTEXTURESTAGES + psh_forgot_to_set_static_HASALPHAMASK + psh_forgot_to_set_static_HASSTATICTEXTURE + 0
class portal_ps11_Dynamic_Index
{
private:
	int m_nADDSTATIC;
#ifdef _DEBUG
	bool m_bADDSTATIC;
#endif
public:
	void SetADDSTATIC( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDSTATIC = i;
#ifdef _DEBUG
		m_bADDSTATIC = true;
#endif
	}
	void SetADDSTATIC( bool i )
	{
		m_nADDSTATIC = i ? 1 : 0;
#ifdef _DEBUG
		m_bADDSTATIC = true;
#endif
	}
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
	portal_ps11_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bADDSTATIC = false;
#endif // _DEBUG
		m_nADDSTATIC = 0;
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
		bool bAllDynamicVarsDefined = m_bADDSTATIC && m_bPIXELFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nADDSTATIC ) + ( 2 * m_nPIXELFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_portal_ps11 psh_forgot_to_set_dynamic_ADDSTATIC + psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
