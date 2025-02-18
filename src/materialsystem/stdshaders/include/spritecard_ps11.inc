#include "shaderlib/cshader.h"
class spritecard_ps11_Static_Index
{
private:
	int m_nADDBASETEXTURE2;
#ifdef _DEBUG
	bool m_bADDBASETEXTURE2;
#endif
public:
	void SetADDBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = true;
#endif
	}
	void SetADDBASETEXTURE2( bool i )
	{
		m_nADDBASETEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = true;
#endif
	}
private:
	int m_nADDSELF;
#ifdef _DEBUG
	bool m_bADDSELF;
#endif
public:
	void SetADDSELF( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDSELF = i;
#ifdef _DEBUG
		m_bADDSELF = true;
#endif
	}
	void SetADDSELF( bool i )
	{
		m_nADDSELF = i ? 1 : 0;
#ifdef _DEBUG
		m_bADDSELF = true;
#endif
	}
private:
	int m_nUSEALPHAASRGB;
#ifdef _DEBUG
	bool m_bUSEALPHAASRGB;
#endif
public:
	void SetUSEALPHAASRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUSEALPHAASRGB = i;
#ifdef _DEBUG
		m_bUSEALPHAASRGB = true;
#endif
	}
	void SetUSEALPHAASRGB( bool i )
	{
		m_nUSEALPHAASRGB = i ? 1 : 0;
#ifdef _DEBUG
		m_bUSEALPHAASRGB = true;
#endif
	}
public:
	spritecard_ps11_Static_Index( )
	{
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = false;
#endif // _DEBUG
		m_nADDBASETEXTURE2 = 0;
#ifdef _DEBUG
		m_bADDSELF = false;
#endif // _DEBUG
		m_nADDSELF = 0;
#ifdef _DEBUG
		m_bUSEALPHAASRGB = false;
#endif // _DEBUG
		m_nUSEALPHAASRGB = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bADDBASETEXTURE2 && m_bADDSELF && m_bUSEALPHAASRGB;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nADDBASETEXTURE2 ) + ( 2 * m_nADDSELF ) + ( 4 * m_nUSEALPHAASRGB ) + 0;
	}
};
#define shaderStaticTest_spritecard_ps11 psh_forgot_to_set_static_ADDBASETEXTURE2 + psh_forgot_to_set_static_ADDSELF + psh_forgot_to_set_static_USEALPHAASRGB + 0
class spritecard_ps11_Dynamic_Index
{
public:
	spritecard_ps11_Dynamic_Index()
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
#define shaderDynamicTest_spritecard_ps11 0
