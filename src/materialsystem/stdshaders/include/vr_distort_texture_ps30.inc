#include "shaderlib/cshader.h"
class vr_distort_texture_ps30_Static_Index
{
public:
	vr_distort_texture_ps30_Static_Index( )
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
#define shaderStaticTest_vr_distort_texture_ps30 0
class vr_distort_texture_ps30_Dynamic_Index
{
private:
	int m_nCMBO_USERENDERTARGET;
#ifdef _DEBUG
	bool m_bCMBO_USERENDERTARGET;
#endif
public:
	void SetCMBO_USERENDERTARGET( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCMBO_USERENDERTARGET = i;
#ifdef _DEBUG
		m_bCMBO_USERENDERTARGET = true;
#endif
	}
	void SetCMBO_USERENDERTARGET( bool i )
	{
		m_nCMBO_USERENDERTARGET = i ? 1 : 0;
#ifdef _DEBUG
		m_bCMBO_USERENDERTARGET = true;
#endif
	}
public:
	vr_distort_texture_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bCMBO_USERENDERTARGET = false;
#endif // _DEBUG
		m_nCMBO_USERENDERTARGET = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bCMBO_USERENDERTARGET;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCMBO_USERENDERTARGET ) + 0;
	}
};
#define shaderDynamicTest_vr_distort_texture_ps30 psh_forgot_to_set_dynamic_CMBO_USERENDERTARGET + 0
