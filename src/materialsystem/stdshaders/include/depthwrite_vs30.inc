#include "shaderlib/cshader.h"
class depthwrite_vs30_Static_Index
{
private:
	int m_nONLY_PROJECT_POSITION;
#ifdef _DEBUG
	bool m_bONLY_PROJECT_POSITION;
#endif
public:
	void SetONLY_PROJECT_POSITION( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nONLY_PROJECT_POSITION = i;
#ifdef _DEBUG
		m_bONLY_PROJECT_POSITION = true;
#endif
	}
	void SetONLY_PROJECT_POSITION( bool i )
	{
		m_nONLY_PROJECT_POSITION = i ? 1 : 0;
#ifdef _DEBUG
		m_bONLY_PROJECT_POSITION = true;
#endif
	}
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
private:
	int m_nTREESWAY;
#ifdef _DEBUG
	bool m_bTREESWAY;
#endif
public:
	void SetTREESWAY( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nTREESWAY = i;
#ifdef _DEBUG
		m_bTREESWAY = true;
#endif
	}
	void SetTREESWAY( bool i )
	{
		m_nTREESWAY = i ? 1 : 0;
#ifdef _DEBUG
		m_bTREESWAY = true;
#endif
	}
public:
	depthwrite_vs30_Static_Index( )
	{
#ifdef _DEBUG
		m_bONLY_PROJECT_POSITION = false;
#endif // _DEBUG
		m_nONLY_PROJECT_POSITION = 0;
#ifdef _DEBUG
		m_bCOLOR_DEPTH = false;
#endif // _DEBUG
		m_nCOLOR_DEPTH = 0;
#ifdef _DEBUG
		m_bTREESWAY = false;
#endif // _DEBUG
		m_nTREESWAY = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bONLY_PROJECT_POSITION && m_bCOLOR_DEPTH && m_bTREESWAY;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 8 * m_nONLY_PROJECT_POSITION ) + ( 8 * m_nCOLOR_DEPTH ) + ( 16 * m_nTREESWAY ) + 0;
	}
};
#define shaderStaticTest_depthwrite_vs30 vsh_forgot_to_set_static_ONLY_PROJECT_POSITION + vsh_forgot_to_set_static_COLOR_DEPTH + vsh_forgot_to_set_static_TREESWAY + 0
class depthwrite_vs30_Dynamic_Index
{
private:
	int m_nCOMPRESSED_VERTS;
#ifdef _DEBUG
	bool m_bCOMPRESSED_VERTS;
#endif
public:
	void SetCOMPRESSED_VERTS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOMPRESSED_VERTS = i;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = true;
#endif
	}
	void SetCOMPRESSED_VERTS( bool i )
	{
		m_nCOMPRESSED_VERTS = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = true;
#endif
	}
private:
	int m_nSKINNING;
#ifdef _DEBUG
	bool m_bSKINNING;
#endif
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
	void SetSKINNING( bool i )
	{
		m_nSKINNING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
private:
	int m_nMORPHING;
#ifdef _DEBUG
	bool m_bMORPHING;
#endif
public:
	void SetMORPHING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMORPHING = i;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
	void SetMORPHING( bool i )
	{
		m_nMORPHING = i ? 1 : 0;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
public:
	depthwrite_vs30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bCOMPRESSED_VERTS = false;
#endif // _DEBUG
		m_nCOMPRESSED_VERTS = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
#ifdef _DEBUG
		m_bMORPHING = false;
#endif // _DEBUG
		m_nMORPHING = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bCOMPRESSED_VERTS && m_bSKINNING && m_bMORPHING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nCOMPRESSED_VERTS ) + ( 2 * m_nSKINNING ) + ( 4 * m_nMORPHING ) + 0;
	}
};
#define shaderDynamicTest_depthwrite_vs30 vsh_forgot_to_set_dynamic_COMPRESSED_VERTS + vsh_forgot_to_set_dynamic_SKINNING + vsh_forgot_to_set_dynamic_MORPHING + 0
