#include "shaderlib/cshader.h"
class worldvertexalpha_ps20_Static_Index
{
private:
	int m_nPASS;
#ifdef _DEBUG
	bool m_bPASS;
#endif
public:
	void SetPASS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPASS = i;
#ifdef _DEBUG
		m_bPASS = true;
#endif
	}
	void SetPASS( bool i )
	{
		m_nPASS = i ? 1 : 0;
#ifdef _DEBUG
		m_bPASS = true;
#endif
	}
public:
	worldvertexalpha_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bPASS = false;
#endif // _DEBUG
		m_nPASS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bPASS;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPASS ) + 0;
	}
};
#define shaderStaticTest_worldvertexalpha_ps20 psh_forgot_to_set_static_PASS + 0
class worldvertexalpha_ps20_Dynamic_Index
{
public:
	worldvertexalpha_ps20_Dynamic_Index()
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
#define shaderDynamicTest_worldvertexalpha_ps20 0
