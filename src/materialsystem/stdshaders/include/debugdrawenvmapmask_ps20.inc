#include "shaderlib/cshader.h"
class debugdrawenvmapmask_ps20_Static_Index
{
public:
	debugdrawenvmapmask_ps20_Static_Index( )
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
#define shaderStaticTest_debugdrawenvmapmask_ps20 0
class debugdrawenvmapmask_ps20_Dynamic_Index
{
private:
	int m_nSHOWALPHA;
#ifdef _DEBUG
	bool m_bSHOWALPHA;
#endif
public:
	void SetSHOWALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSHOWALPHA = i;
#ifdef _DEBUG
		m_bSHOWALPHA = true;
#endif
	}
	void SetSHOWALPHA( bool i )
	{
		m_nSHOWALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bSHOWALPHA = true;
#endif
	}
public:
	debugdrawenvmapmask_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bSHOWALPHA = false;
#endif // _DEBUG
		m_nSHOWALPHA = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bSHOWALPHA;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSHOWALPHA ) + 0;
	}
};
#define shaderDynamicTest_debugdrawenvmapmask_ps20 psh_forgot_to_set_dynamic_SHOWALPHA + 0
