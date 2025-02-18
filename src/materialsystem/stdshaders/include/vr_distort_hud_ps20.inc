#include "shaderlib/cshader.h"
class vr_distort_hud_ps20_Static_Index
{
public:
	vr_distort_hud_ps20_Static_Index( )
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
#define shaderStaticTest_vr_distort_hud_ps20 0
class vr_distort_hud_ps20_Dynamic_Index
{
private:
	int m_nCMBO_HUDUNDISTORT;
#ifdef _DEBUG
	bool m_bCMBO_HUDUNDISTORT;
#endif
public:
	void SetCMBO_HUDUNDISTORT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCMBO_HUDUNDISTORT = i;
#ifdef _DEBUG
		m_bCMBO_HUDUNDISTORT = true;
#endif
	}
	void SetCMBO_HUDUNDISTORT( bool i )
	{
		m_nCMBO_HUDUNDISTORT = i ? 1 : 0;
#ifdef _DEBUG
		m_bCMBO_HUDUNDISTORT = true;
#endif
	}
public:
	vr_distort_hud_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bCMBO_HUDUNDISTORT = false;
#endif // _DEBUG
		m_nCMBO_HUDUNDISTORT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bCMBO_HUDUNDISTORT;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCMBO_HUDUNDISTORT ) + 0;
	}
};
#define shaderDynamicTest_vr_distort_hud_ps20 psh_forgot_to_set_dynamic_CMBO_HUDUNDISTORT + 0
