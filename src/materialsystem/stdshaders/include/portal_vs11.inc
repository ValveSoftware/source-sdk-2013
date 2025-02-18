#include "shaderlib/cshader.h"
class portal_vs11_Static_Index
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
private:
	int m_nUSEALTERNATEVIEW;
#ifdef _DEBUG
	bool m_bUSEALTERNATEVIEW;
#endif
public:
	void SetUSEALTERNATEVIEW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSEALTERNATEVIEW = i;
#ifdef _DEBUG
		m_bUSEALTERNATEVIEW = true;
#endif
	}
	void SetUSEALTERNATEVIEW( bool i )
	{
		m_nUSEALTERNATEVIEW = i ? 1 : 0;
#ifdef _DEBUG
		m_bUSEALTERNATEVIEW = true;
#endif
	}
public:
	portal_vs11_Static_Index( )
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
#ifdef _DEBUG
		m_bUSEALTERNATEVIEW = false;
#endif // _DEBUG
		m_nUSEALTERNATEVIEW = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bMAXTEXTURESTAGES && m_bHASALPHAMASK && m_bHASSTATICTEXTURE && m_bUSEALTERNATEVIEW;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nMAXTEXTURESTAGES ) + ( 12 * m_nHASALPHAMASK ) + ( 24 * m_nHASSTATICTEXTURE ) + ( 48 * m_nUSEALTERNATEVIEW ) + 0;
	}
};
#define shaderStaticTest_portal_vs11 vsh_forgot_to_set_static_MAXTEXTURESTAGES + vsh_forgot_to_set_static_HASALPHAMASK + vsh_forgot_to_set_static_HASSTATICTEXTURE + vsh_forgot_to_set_static_USEALTERNATEVIEW + 0
class portal_vs11_Dynamic_Index
{
private:
	int m_nSKINNING;
#ifdef _DEBUG
	bool m_bSKINNING;
#endif
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
	void SetSKINNING( bool i )
	{
		m_nSKINNING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
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
public:
	portal_vs11_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
#ifdef _DEBUG
		m_bADDSTATIC = false;
#endif // _DEBUG
		m_nADDSTATIC = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bSKINNING && m_bADDSTATIC;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSKINNING ) + ( 2 * m_nADDSTATIC ) + 0;
	}
};
#define shaderDynamicTest_portal_vs11 vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_ADDSTATIC + 0
