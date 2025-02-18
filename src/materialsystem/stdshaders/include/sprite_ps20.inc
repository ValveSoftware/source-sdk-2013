#include "shaderlib/cshader.h"
class sprite_ps20_Static_Index
{
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
private:
	int m_nCONSTANTCOLOR;
#ifdef _DEBUG
	bool m_bCONSTANTCOLOR;
#endif
public:
	void SetCONSTANTCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONSTANTCOLOR = i;
#ifdef _DEBUG
		m_bCONSTANTCOLOR = true;
#endif
	}
	void SetCONSTANTCOLOR( bool i )
	{
		m_nCONSTANTCOLOR = i ? 1 : 0;
#ifdef _DEBUG
		m_bCONSTANTCOLOR = true;
#endif
	}
private:
	int m_nHDRTYPE;
#ifdef _DEBUG
	bool m_bHDRTYPE;
#endif
public:
	void SetHDRTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nHDRTYPE = i;
#ifdef _DEBUG
		m_bHDRTYPE = true;
#endif
	}
	void SetHDRTYPE( bool i )
	{
		m_nHDRTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bHDRTYPE = true;
#endif
	}
private:
	int m_nSRGB;
#ifdef _DEBUG
	bool m_bSRGB;
#endif
public:
	void SetSRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSRGB = i;
#ifdef _DEBUG
		m_bSRGB = true;
#endif
	}
	void SetSRGB( bool i )
	{
		m_nSRGB = i ? 1 : 0;
#ifdef _DEBUG
		m_bSRGB = true;
#endif
	}
public:
	sprite_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
#ifdef _DEBUG
		m_bCONSTANTCOLOR = false;
#endif // _DEBUG
		m_nCONSTANTCOLOR = 0;
#ifdef _DEBUG
		m_bHDRTYPE = false;
#endif // _DEBUG
		m_nHDRTYPE = 0;
#ifdef _DEBUG
		m_bSRGB = false;
#endif // _DEBUG
		m_nSRGB = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bVERTEXCOLOR && m_bCONSTANTCOLOR && m_bHDRTYPE && m_bSRGB;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nVERTEXCOLOR ) + ( 8 * m_nCONSTANTCOLOR ) + ( 16 * m_nHDRTYPE ) + ( 48 * m_nSRGB ) + 0;
	}
};
#define shaderStaticTest_sprite_ps20 psh_forgot_to_set_static_VERTEXCOLOR + psh_forgot_to_set_static_CONSTANTCOLOR + psh_forgot_to_set_static_HDRTYPE + psh_forgot_to_set_static_SRGB + 0
class sprite_ps20_Dynamic_Index
{
private:
	int m_nHDRENABLED;
#ifdef _DEBUG
	bool m_bHDRENABLED;
#endif
public:
	void SetHDRENABLED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHDRENABLED = i;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif
	}
	void SetHDRENABLED( bool i )
	{
		m_nHDRENABLED = i ? 1 : 0;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif
	}
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
	sprite_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bHDRENABLED = false;
#endif // _DEBUG
		m_nHDRENABLED = 0;
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
		bool bAllDynamicVarsDefined = m_bHDRENABLED && m_bPIXELFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nHDRENABLED ) + ( 2 * m_nPIXELFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_sprite_ps20 psh_forgot_to_set_dynamic_HDRENABLED + psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
