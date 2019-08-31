#include "shaderlib/cshader.h"
class sdk_water_vs20_Static_Index
{
private:
	int m_nBASETEXTURE;
#ifdef _DEBUG
	bool m_bBASETEXTURE;
#endif
public:
	void SetBASETEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE = i;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
	void SetBASETEXTURE( bool i )
	{
		m_nBASETEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
private:
	int m_nMULTITEXTURE;
#ifdef _DEBUG
	bool m_bMULTITEXTURE;
#endif
public:
	void SetMULTITEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMULTITEXTURE = i;
#ifdef _DEBUG
		m_bMULTITEXTURE = true;
#endif
	}
	void SetMULTITEXTURE( bool i )
	{
		m_nMULTITEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bMULTITEXTURE = true;
#endif
	}
public:
	sdk_water_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bBASETEXTURE = false;
#endif // _DEBUG
		m_nBASETEXTURE = 0;
#ifdef _DEBUG
		m_bMULTITEXTURE = false;
#endif // _DEBUG
		m_nMULTITEXTURE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bBASETEXTURE && m_bMULTITEXTURE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nBASETEXTURE ) + ( 2 * m_nMULTITEXTURE ) + 0;
	}
};
#define shaderStaticTest_sdk_water_vs20 vsh_forgot_to_set_static_BASETEXTURE + vsh_forgot_to_set_static_MULTITEXTURE + 0
class sdk_water_vs20_Dynamic_Index
{
public:
	sdk_water_vs20_Dynamic_Index()
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
#define shaderDynamicTest_sdk_water_vs20 0
