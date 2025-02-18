#include "shaderlib/cshader.h"
class pyro_vision_ps20b_Static_Index
{
private:
	int m_nEFFECT;
#ifdef _DEBUG
	bool m_bEFFECT;
#endif
public:
	void SetEFFECT( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nEFFECT = i;
#ifdef _DEBUG
		m_bEFFECT = true;
#endif
	}
	void SetEFFECT( bool i )
	{
		m_nEFFECT = i ? 1 : 0;
#ifdef _DEBUG
		m_bEFFECT = true;
#endif
	}
private:
	int m_nVERTEX_LIT;
#ifdef _DEBUG
	bool m_bVERTEX_LIT;
#endif
public:
	void SetVERTEX_LIT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEX_LIT = i;
#ifdef _DEBUG
		m_bVERTEX_LIT = true;
#endif
	}
	void SetVERTEX_LIT( bool i )
	{
		m_nVERTEX_LIT = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEX_LIT = true;
#endif
	}
private:
	int m_nBASETEXTURE2;
#ifdef _DEBUG
	bool m_bBASETEXTURE2;
#endif
public:
	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
	void SetBASETEXTURE2( bool i )
	{
		m_nBASETEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
private:
	int m_nFANCY_BLENDING;
#ifdef _DEBUG
	bool m_bFANCY_BLENDING;
#endif
public:
	void SetFANCY_BLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFANCY_BLENDING = i;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif
	}
	void SetFANCY_BLENDING( bool i )
	{
		m_nFANCY_BLENDING = i ? 1 : 0;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif
	}
private:
	int m_nSELFILLUM;
#ifdef _DEBUG
	bool m_bSELFILLUM;
#endif
public:
	void SetSELFILLUM( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUM = i;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif
	}
	void SetSELFILLUM( bool i )
	{
		m_nSELFILLUM = i ? 1 : 0;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif
	}
private:
	int m_nCOLOR_BAR;
#ifdef _DEBUG
	bool m_bCOLOR_BAR;
#endif
public:
	void SetCOLOR_BAR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLOR_BAR = i;
#ifdef _DEBUG
		m_bCOLOR_BAR = true;
#endif
	}
	void SetCOLOR_BAR( bool i )
	{
		m_nCOLOR_BAR = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOLOR_BAR = true;
#endif
	}
private:
	int m_nSTRIPES;
#ifdef _DEBUG
	bool m_bSTRIPES;
#endif
public:
	void SetSTRIPES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES = i;
#ifdef _DEBUG
		m_bSTRIPES = true;
#endif
	}
	void SetSTRIPES( bool i )
	{
		m_nSTRIPES = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTRIPES = true;
#endif
	}
private:
	int m_nSTRIPES_USE_NORMAL2;
#ifdef _DEBUG
	bool m_bSTRIPES_USE_NORMAL2;
#endif
public:
	void SetSTRIPES_USE_NORMAL2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRIPES_USE_NORMAL2 = i;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = true;
#endif
	}
	void SetSTRIPES_USE_NORMAL2( bool i )
	{
		m_nSTRIPES_USE_NORMAL2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = true;
#endif
	}
public:
	pyro_vision_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bEFFECT = false;
#endif // _DEBUG
		m_nEFFECT = 0;
#ifdef _DEBUG
		m_bVERTEX_LIT = false;
#endif // _DEBUG
		m_nVERTEX_LIT = 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = false;
#endif // _DEBUG
		m_nBASETEXTURE2 = 0;
#ifdef _DEBUG
		m_bFANCY_BLENDING = false;
#endif // _DEBUG
		m_nFANCY_BLENDING = 0;
#ifdef _DEBUG
		m_bSELFILLUM = false;
#endif // _DEBUG
		m_nSELFILLUM = 0;
#ifdef _DEBUG
		m_bCOLOR_BAR = false;
#endif // _DEBUG
		m_nCOLOR_BAR = 0;
#ifdef _DEBUG
		m_bSTRIPES = false;
#endif // _DEBUG
		m_nSTRIPES = 0;
#ifdef _DEBUG
		m_bSTRIPES_USE_NORMAL2 = false;
#endif // _DEBUG
		m_nSTRIPES_USE_NORMAL2 = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bEFFECT && m_bVERTEX_LIT && m_bBASETEXTURE2 && m_bFANCY_BLENDING && m_bSELFILLUM && m_bCOLOR_BAR && m_bSTRIPES && m_bSTRIPES_USE_NORMAL2;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 8 * m_nEFFECT ) + ( 32 * m_nVERTEX_LIT ) + ( 64 * m_nBASETEXTURE2 ) + ( 128 * m_nFANCY_BLENDING ) + ( 256 * m_nSELFILLUM ) + ( 512 * m_nCOLOR_BAR ) + ( 1024 * m_nSTRIPES ) + ( 2048 * m_nSTRIPES_USE_NORMAL2 ) + 0;
	}
};
#define shaderStaticTest_pyro_vision_ps20b psh_forgot_to_set_static_EFFECT + psh_forgot_to_set_static_VERTEX_LIT + psh_forgot_to_set_static_BASETEXTURE2 + psh_forgot_to_set_static_FANCY_BLENDING + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_COLOR_BAR + psh_forgot_to_set_static_STRIPES + psh_forgot_to_set_static_STRIPES_USE_NORMAL2 + 0
class pyro_vision_ps20b_Dynamic_Index
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
private:
	int m_nVISUALIZE_DOF;
#ifdef _DEBUG
	bool m_bVISUALIZE_DOF;
#endif
public:
	void SetVISUALIZE_DOF( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVISUALIZE_DOF = i;
#ifdef _DEBUG
		m_bVISUALIZE_DOF = true;
#endif
	}
	void SetVISUALIZE_DOF( bool i )
	{
		m_nVISUALIZE_DOF = i ? 1 : 0;
#ifdef _DEBUG
		m_bVISUALIZE_DOF = true;
#endif
	}
private:
	int m_nHEATHAZE;
#ifdef _DEBUG
	bool m_bHEATHAZE;
#endif
public:
	void SetHEATHAZE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHEATHAZE = i;
#ifdef _DEBUG
		m_bHEATHAZE = true;
#endif
	}
	void SetHEATHAZE( bool i )
	{
		m_nHEATHAZE = i ? 1 : 0;
#ifdef _DEBUG
		m_bHEATHAZE = true;
#endif
	}
public:
	pyro_vision_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bVISUALIZE_DOF = false;
#endif // _DEBUG
		m_nVISUALIZE_DOF = 0;
#ifdef _DEBUG
		m_bHEATHAZE = false;
#endif // _DEBUG
		m_nHEATHAZE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE && m_bVISUALIZE_DOF && m_bHEATHAZE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nVISUALIZE_DOF ) + ( 4 * m_nHEATHAZE ) + 0;
	}
};
#define shaderDynamicTest_pyro_vision_ps20b psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_VISUALIZE_DOF + psh_forgot_to_set_dynamic_HEATHAZE + 0
