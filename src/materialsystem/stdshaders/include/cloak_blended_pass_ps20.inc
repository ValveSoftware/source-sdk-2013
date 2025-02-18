#include "shaderlib/cshader.h"
class cloak_blended_pass_ps20_Static_Index
{
private:
	int m_nBUMPMAP;
#ifdef _DEBUG
	bool m_bBUMPMAP;
#endif
public:
	void SetBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP = i;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif
	}
	void SetBUMPMAP( bool i )
	{
		m_nBUMPMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif
	}
public:
	cloak_blended_pass_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bBUMPMAP = false;
#endif // _DEBUG
		m_nBUMPMAP = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bBUMPMAP;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nBUMPMAP ) + 0;
	}
};
#define shaderStaticTest_cloak_blended_pass_ps20 psh_forgot_to_set_static_BUMPMAP + 0
class cloak_blended_pass_ps20_Dynamic_Index
{
public:
	cloak_blended_pass_ps20_Dynamic_Index()
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
#define shaderDynamicTest_cloak_blended_pass_ps20 0
