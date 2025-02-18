#include "shaderlib/cshader.h"
class shatteredglass_ps20_Static_Index
{
private:
	int m_nCUBEMAP;
#ifdef _DEBUG
	bool m_bCUBEMAP;
#endif
public:
	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif
	}
	void SetCUBEMAP( bool i )
	{
		m_nCUBEMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bCUBEMAP = true;
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
private:
	int m_nENVMAPMASK;
#ifdef _DEBUG
	bool m_bENVMAPMASK;
#endif
public:
	void SetENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPMASK = i;
#ifdef _DEBUG
		m_bENVMAPMASK = true;
#endif
	}
	void SetENVMAPMASK( bool i )
	{
		m_nENVMAPMASK = i ? 1 : 0;
#ifdef _DEBUG
		m_bENVMAPMASK = true;
#endif
	}
private:
	int m_nBASEALPHAENVMAPMASK;
#ifdef _DEBUG
	bool m_bBASEALPHAENVMAPMASK;
#endif
public:
	void SetBASEALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASEALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = true;
#endif
	}
	void SetBASEALPHAENVMAPMASK( bool i )
	{
		m_nBASEALPHAENVMAPMASK = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = true;
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
public:
	shatteredglass_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bCUBEMAP = false;
#endif // _DEBUG
		m_nCUBEMAP = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
#ifdef _DEBUG
		m_bENVMAPMASK = false;
#endif // _DEBUG
		m_nENVMAPMASK = 0;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = false;
#endif // _DEBUG
		m_nBASEALPHAENVMAPMASK = 0;
#ifdef _DEBUG
		m_bHDRTYPE = false;
#endif // _DEBUG
		m_nHDRTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCUBEMAP && m_bVERTEXCOLOR && m_bENVMAPMASK && m_bBASEALPHAENVMAPMASK && m_bHDRTYPE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nCUBEMAP ) + ( 8 * m_nVERTEXCOLOR ) + ( 16 * m_nENVMAPMASK ) + ( 32 * m_nBASEALPHAENVMAPMASK ) + ( 64 * m_nHDRTYPE ) + 0;
	}
};
#define shaderStaticTest_shatteredglass_ps20 psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_VERTEXCOLOR + psh_forgot_to_set_static_ENVMAPMASK + psh_forgot_to_set_static_BASEALPHAENVMAPMASK + psh_forgot_to_set_static_HDRTYPE + 0
class shatteredglass_ps20_Dynamic_Index
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
	shatteredglass_ps20_Dynamic_Index()
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
#define shaderDynamicTest_shatteredglass_ps20 psh_forgot_to_set_dynamic_HDRENABLED + psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
