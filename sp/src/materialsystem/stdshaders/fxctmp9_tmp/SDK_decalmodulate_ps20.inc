#include "shaderlib/cshader.h"
class sdk_decalmodulate_ps20_Static_Index
{
private:
	int m_nVERTEXALPHA;
#ifdef _DEBUG
	bool m_bVERTEXALPHA;
#endif
public:
	void SetVERTEXALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXALPHA = i;
#ifdef _DEBUG
		m_bVERTEXALPHA = true;
#endif
	}
	void SetVERTEXALPHA( bool i )
	{
		m_nVERTEXALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXALPHA = true;
#endif
	}
private:
	int m_nFLASHLIGHT;
#ifdef _DEBUG
	bool m_bFLASHLIGHT;
#endif
public:
	void SetFLASHLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHT = i;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
	void SetFLASHLIGHT( bool i )
	{
		m_nFLASHLIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
public:
	sdk_decalmodulate_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bVERTEXALPHA = false;
#endif // _DEBUG
		m_nVERTEXALPHA = 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = false;
#endif // _DEBUG
		m_nFLASHLIGHT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bVERTEXALPHA && m_bFLASHLIGHT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nVERTEXALPHA ) + ( 4 * m_nFLASHLIGHT ) + 0;
	}
};
#define shaderStaticTest_sdk_decalmodulate_ps20 psh_forgot_to_set_static_VERTEXALPHA + psh_forgot_to_set_static_FLASHLIGHT + 0
class sdk_decalmodulate_ps20_Dynamic_Index
{
private:
	int m_nPIXELFOGTYPE;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE;
#endif
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
	void SetPIXELFOGTYPE( bool i )
	{
		m_nPIXELFOGTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
public:
	sdk_decalmodulate_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_sdk_decalmodulate_ps20 psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
