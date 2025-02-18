#include "shaderlib/cshader.h"
class depthwrite_ps30_Static_Index
{
private:
	int m_nCOLOR_DEPTH;
#ifdef _DEBUG
	bool m_bCOLOR_DEPTH;
#endif
public:
	void SetCOLOR_DEPTH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLOR_DEPTH = i;
#ifdef _DEBUG
		m_bCOLOR_DEPTH = true;
#endif
	}
	void SetCOLOR_DEPTH( bool i )
	{
		m_nCOLOR_DEPTH = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOLOR_DEPTH = true;
#endif
	}
public:
	depthwrite_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bCOLOR_DEPTH = false;
#endif // _DEBUG
		m_nCOLOR_DEPTH = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCOLOR_DEPTH;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nCOLOR_DEPTH ) + 0;
	}
};
#define shaderStaticTest_depthwrite_ps30 psh_forgot_to_set_static_COLOR_DEPTH + 0
class depthwrite_ps30_Dynamic_Index
{
private:
	int m_nALPHACLIP;
#ifdef _DEBUG
	bool m_bALPHACLIP;
#endif
public:
	void SetALPHACLIP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nALPHACLIP = i;
#ifdef _DEBUG
		m_bALPHACLIP = true;
#endif
	}
	void SetALPHACLIP( bool i )
	{
		m_nALPHACLIP = i ? 1 : 0;
#ifdef _DEBUG
		m_bALPHACLIP = true;
#endif
	}
public:
	depthwrite_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bALPHACLIP = false;
#endif // _DEBUG
		m_nALPHACLIP = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bALPHACLIP;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nALPHACLIP ) + 0;
	}
};
#define shaderDynamicTest_depthwrite_ps30 psh_forgot_to_set_dynamic_ALPHACLIP + 0
