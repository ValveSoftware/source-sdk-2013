#include "shaderlib/cshader.h"
class screenspaceeffect_vs20_Static_Index
{
private:
	int m_nX360APPCHOOSER;
#ifdef _DEBUG
	bool m_bX360APPCHOOSER;
#endif
public:
	void SetX360APPCHOOSER( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nX360APPCHOOSER = i;
#ifdef _DEBUG
		m_bX360APPCHOOSER = true;
#endif
	}
	void SetX360APPCHOOSER( bool i )
	{
		m_nX360APPCHOOSER = i ? 1 : 0;
#ifdef _DEBUG
		m_bX360APPCHOOSER = true;
#endif
	}
public:
	screenspaceeffect_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bX360APPCHOOSER = true;
#endif // _DEBUG
		m_nX360APPCHOOSER =  0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bX360APPCHOOSER;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nX360APPCHOOSER ) + 0;
	}
};
#define shaderStaticTest_screenspaceeffect_vs20 0
class screenspaceeffect_vs20_Dynamic_Index
{
public:
	screenspaceeffect_vs20_Dynamic_Index()
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
#define shaderDynamicTest_screenspaceeffect_vs20 0
