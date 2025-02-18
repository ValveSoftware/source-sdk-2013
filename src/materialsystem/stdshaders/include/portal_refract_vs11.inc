#include "shaderlib/cshader.h"
class portal_refract_vs11_Static_Index
{
private:
	int m_nSTAGE;
#ifdef _DEBUG
	bool m_bSTAGE;
#endif
public:
	void SetSTAGE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nSTAGE = i;
#ifdef _DEBUG
		m_bSTAGE = true;
#endif
	}
	void SetSTAGE( bool i )
	{
		m_nSTAGE = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTAGE = true;
#endif
	}
public:
	portal_refract_vs11_Static_Index( )
	{
#ifdef _DEBUG
		m_bSTAGE = false;
#endif // _DEBUG
		m_nSTAGE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bSTAGE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSTAGE ) + 0;
	}
};
#define shaderStaticTest_portal_refract_vs11 vsh_forgot_to_set_static_STAGE + 0
class portal_refract_vs11_Dynamic_Index
{
public:
	portal_refract_vs11_Dynamic_Index()
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
#define shaderDynamicTest_portal_refract_vs11 0
