#include "shaderlib/cshader.h"
class color_projection_ps20b_Static_Index
{
public:
	color_projection_ps20b_Static_Index( )
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
#define shaderStaticTest_color_projection_ps20b 0
class color_projection_ps20b_Dynamic_Index
{
private:
	int m_nNEED_BLINDMK;
#ifdef _DEBUG
	bool m_bNEED_BLINDMK;
#endif
public:
	void SetNEED_BLINDMK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_BLINDMK = i;
#ifdef _DEBUG
		m_bNEED_BLINDMK = true;
#endif
	}
	void SetNEED_BLINDMK( bool i )
	{
		m_nNEED_BLINDMK = i ? 1 : 0;
#ifdef _DEBUG
		m_bNEED_BLINDMK = true;
#endif
	}
private:
	int m_nNEED_ANOMYLIZE;
#ifdef _DEBUG
	bool m_bNEED_ANOMYLIZE;
#endif
public:
	void SetNEED_ANOMYLIZE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_ANOMYLIZE = i;
#ifdef _DEBUG
		m_bNEED_ANOMYLIZE = true;
#endif
	}
	void SetNEED_ANOMYLIZE( bool i )
	{
		m_nNEED_ANOMYLIZE = i ? 1 : 0;
#ifdef _DEBUG
		m_bNEED_ANOMYLIZE = true;
#endif
	}
private:
	int m_nNEED_MONOCHROME;
#ifdef _DEBUG
	bool m_bNEED_MONOCHROME;
#endif
public:
	void SetNEED_MONOCHROME( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNEED_MONOCHROME = i;
#ifdef _DEBUG
		m_bNEED_MONOCHROME = true;
#endif
	}
	void SetNEED_MONOCHROME( bool i )
	{
		m_nNEED_MONOCHROME = i ? 1 : 0;
#ifdef _DEBUG
		m_bNEED_MONOCHROME = true;
#endif
	}
public:
	color_projection_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bNEED_BLINDMK = false;
#endif // _DEBUG
		m_nNEED_BLINDMK = 0;
#ifdef _DEBUG
		m_bNEED_ANOMYLIZE = false;
#endif // _DEBUG
		m_nNEED_ANOMYLIZE = 0;
#ifdef _DEBUG
		m_bNEED_MONOCHROME = false;
#endif // _DEBUG
		m_nNEED_MONOCHROME = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bNEED_BLINDMK && m_bNEED_ANOMYLIZE && m_bNEED_MONOCHROME;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nNEED_BLINDMK ) + ( 2 * m_nNEED_ANOMYLIZE ) + ( 4 * m_nNEED_MONOCHROME ) + 0;
	}
};
#define shaderDynamicTest_color_projection_ps20b psh_forgot_to_set_dynamic_NEED_BLINDMK + psh_forgot_to_set_dynamic_NEED_ANOMYLIZE + psh_forgot_to_set_dynamic_NEED_MONOCHROME + 0
