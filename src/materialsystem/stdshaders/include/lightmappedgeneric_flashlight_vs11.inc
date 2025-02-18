#include "shaderlib/cshader.h"
class lightmappedgeneric_flashlight_vs11_Static_Index
{
private:
	int m_nNORMALMAP;
#ifdef _DEBUG
	bool m_bNORMALMAP;
#endif
public:
	void SetNORMALMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAP = i;
#ifdef _DEBUG
		m_bNORMALMAP = true;
#endif
	}
	void SetNORMALMAP( bool i )
	{
		m_nNORMALMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMALMAP = true;
#endif
	}
private:
	int m_nWORLDVERTEXTRANSITION;
#ifdef _DEBUG
	bool m_bWORLDVERTEXTRANSITION;
#endif
public:
	void SetWORLDVERTEXTRANSITION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWORLDVERTEXTRANSITION = i;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
#endif
	}
	void SetWORLDVERTEXTRANSITION( bool i )
	{
		m_nWORLDVERTEXTRANSITION = i ? 1 : 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
#endif
	}
private:
	int m_nVERTEXCOLOR;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR;
#endif
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
	void SetVERTEXCOLOR( bool i )
	{
		m_nVERTEXCOLOR = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
public:
	lightmappedgeneric_flashlight_vs11_Static_Index( )
	{
#ifdef _DEBUG
		m_bNORMALMAP = false;
#endif // _DEBUG
		m_nNORMALMAP = 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = false;
#endif // _DEBUG
		m_nWORLDVERTEXTRANSITION = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bNORMALMAP && m_bWORLDVERTEXTRANSITION && m_bVERTEXCOLOR;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nNORMALMAP ) + ( 4 * m_nWORLDVERTEXTRANSITION ) + ( 8 * m_nVERTEXCOLOR ) + 0;
	}
};
#define shaderStaticTest_lightmappedgeneric_flashlight_vs11 vsh_forgot_to_set_static_NORMALMAP + vsh_forgot_to_set_static_WORLDVERTEXTRANSITION + vsh_forgot_to_set_static_VERTEXCOLOR + 0
class lightmappedgeneric_flashlight_vs11_Dynamic_Index
{
private:
	int m_nDOWATERFOG;
#ifdef _DEBUG
	bool m_bDOWATERFOG;
#endif
public:
	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif
	}
	void SetDOWATERFOG( bool i )
	{
		m_nDOWATERFOG = i ? 1 : 0;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif
	}
public:
	lightmappedgeneric_flashlight_vs11_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bDOWATERFOG = false;
#endif // _DEBUG
		m_nDOWATERFOG = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bDOWATERFOG;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDOWATERFOG ) + 0;
	}
};
#define shaderDynamicTest_lightmappedgeneric_flashlight_vs11 vsh_forgot_to_set_dynamic_DOWATERFOG + 0
