#include "shaderlib/cshader.h"
class warp_ps20_Static_Index
{
public:
	warp_ps20_Static_Index( )
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
#define shaderStaticTest_warp_ps20 0
class warp_ps20_Dynamic_Index
{
private:
	int m_nDISTORT_TYPE;
#ifdef _DEBUG
	bool m_bDISTORT_TYPE;
#endif
public:
	void SetDISTORT_TYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nDISTORT_TYPE = i;
#ifdef _DEBUG
		m_bDISTORT_TYPE = true;
#endif
	}
	void SetDISTORT_TYPE( bool i )
	{
		m_nDISTORT_TYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDISTORT_TYPE = true;
#endif
	}
public:
	warp_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bDISTORT_TYPE = false;
#endif // _DEBUG
		m_nDISTORT_TYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bDISTORT_TYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDISTORT_TYPE ) + 0;
	}
};
#define shaderDynamicTest_warp_ps20 psh_forgot_to_set_dynamic_DISTORT_TYPE + 0
