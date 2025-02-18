#include "shaderlib/cshader.h"
class debugtextureview_ps20_Static_Index
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
	debugtextureview_ps20_Static_Index( )
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
		bool bAllStaticVarsDefined = m_bSHOWALPHA;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nSHOWALPHA ) + 0;
	}
};
#define shaderStaticTest_debugtextureview_ps20 psh_forgot_to_set_static_SHOWALPHA + 0
class debugtextureview_ps20_Dynamic_Index
{
private:
	int m_nISCUBEMAP;
#ifdef _DEBUG
	bool m_bISCUBEMAP;
#endif
public:
	void SetISCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nISCUBEMAP = i;
#ifdef _DEBUG
		m_bISCUBEMAP = true;
#endif
	}
	void SetISCUBEMAP( bool i )
	{
		m_nISCUBEMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bISCUBEMAP = true;
#endif
	}
public:
	debugtextureview_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bISCUBEMAP = false;
#endif // _DEBUG
		m_nISCUBEMAP = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bISCUBEMAP;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nISCUBEMAP ) + 0;
	}
};
#define shaderDynamicTest_debugtextureview_ps20 psh_forgot_to_set_dynamic_ISCUBEMAP + 0
