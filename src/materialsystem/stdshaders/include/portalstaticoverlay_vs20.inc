#include "shaderlib/cshader.h"
class portalstaticoverlay_vs20_Static_Index
{
private:
	int m_nMODEL;
#ifdef _DEBUG
	bool m_bMODEL;
#endif
public:
	void SetMODEL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMODEL = i;
#ifdef _DEBUG
		m_bMODEL = true;
#endif
	}
	void SetMODEL( bool i )
	{
		m_nMODEL = i ? 1 : 0;
#ifdef _DEBUG
		m_bMODEL = true;
#endif
	}
public:
	portalstaticoverlay_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bMODEL = false;
#endif // _DEBUG
		m_nMODEL = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bMODEL;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nMODEL ) + 0;
	}
};
#define shaderStaticTest_portalstaticoverlay_vs20 vsh_forgot_to_set_static_MODEL + 0
class portalstaticoverlay_vs20_Dynamic_Index
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
public:
	portalstaticoverlay_vs20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bSKINNING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSKINNING ) + 0;
	}
};
#define shaderDynamicTest_portalstaticoverlay_vs20 vsh_forgot_to_set_dynamic_SKINNING + 0
