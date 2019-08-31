#include "shaderlib/cshader.h"
class sdk_refract_ps20b_Static_Index
{
private:
	int m_nCONVERT_TO_SRGB;
#ifdef _DEBUG
	bool m_bCONVERT_TO_SRGB;
#endif
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_TO_SRGB = i;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif
	}
	void SetCONVERT_TO_SRGB( bool i )
	{
		m_nCONVERT_TO_SRGB = i ? 1 : 0;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif
	}
private:
	int m_nBLUR;
#ifdef _DEBUG
	bool m_bBLUR;
#endif
public:
	void SetBLUR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLUR = i;
#ifdef _DEBUG
		m_bBLUR = true;
#endif
	}
	void SetBLUR( bool i )
	{
		m_nBLUR = i ? 1 : 0;
#ifdef _DEBUG
		m_bBLUR = true;
#endif
	}
private:
	int m_nFADEOUTONSILHOUETTE;
#ifdef _DEBUG
	bool m_bFADEOUTONSILHOUETTE;
#endif
public:
	void SetFADEOUTONSILHOUETTE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFADEOUTONSILHOUETTE = i;
#ifdef _DEBUG
		m_bFADEOUTONSILHOUETTE = true;
#endif
	}
	void SetFADEOUTONSILHOUETTE( bool i )
	{
		m_nFADEOUTONSILHOUETTE = i ? 1 : 0;
#ifdef _DEBUG
		m_bFADEOUTONSILHOUETTE = true;
#endif
	}
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
	int m_nREFRACTTINTTEXTURE;
#ifdef _DEBUG
	bool m_bREFRACTTINTTEXTURE;
#endif
public:
	void SetREFRACTTINTTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nREFRACTTINTTEXTURE = i;
#ifdef _DEBUG
		m_bREFRACTTINTTEXTURE = true;
#endif
	}
	void SetREFRACTTINTTEXTURE( bool i )
	{
		m_nREFRACTTINTTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bREFRACTTINTTEXTURE = true;
#endif
	}
private:
	int m_nMASKED;
#ifdef _DEBUG
	bool m_bMASKED;
#endif
public:
	void SetMASKED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMASKED = i;
#ifdef _DEBUG
		m_bMASKED = true;
#endif
	}
	void SetMASKED( bool i )
	{
		m_nMASKED = i ? 1 : 0;
#ifdef _DEBUG
		m_bMASKED = true;
#endif
	}
private:
	int m_nCOLORMODULATE;
#ifdef _DEBUG
	bool m_bCOLORMODULATE;
#endif
public:
	void SetCOLORMODULATE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLORMODULATE = i;
#ifdef _DEBUG
		m_bCOLORMODULATE = true;
#endif
	}
	void SetCOLORMODULATE( bool i )
	{
		m_nCOLORMODULATE = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOLORMODULATE = true;
#endif
	}
private:
	int m_nSECONDARY_NORMAL;
#ifdef _DEBUG
	bool m_bSECONDARY_NORMAL;
#endif
public:
	void SetSECONDARY_NORMAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSECONDARY_NORMAL = i;
#ifdef _DEBUG
		m_bSECONDARY_NORMAL = true;
#endif
	}
	void SetSECONDARY_NORMAL( bool i )
	{
		m_nSECONDARY_NORMAL = i ? 1 : 0;
#ifdef _DEBUG
		m_bSECONDARY_NORMAL = true;
#endif
	}
private:
	int m_nNORMAL_DECODE_MODE;
#ifdef _DEBUG
	bool m_bNORMAL_DECODE_MODE;
#endif
public:
	void SetNORMAL_DECODE_MODE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nNORMAL_DECODE_MODE = i;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = true;
#endif
	}
	void SetNORMAL_DECODE_MODE( bool i )
	{
		m_nNORMAL_DECODE_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = true;
#endif
	}
private:
	int m_nSHADER_SRGB_READ;
#ifdef _DEBUG
	bool m_bSHADER_SRGB_READ;
#endif
public:
	void SetSHADER_SRGB_READ( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSHADER_SRGB_READ = i;
#ifdef _DEBUG
		m_bSHADER_SRGB_READ = true;
#endif
	}
	void SetSHADER_SRGB_READ( bool i )
	{
		m_nSHADER_SRGB_READ = i ? 1 : 0;
#ifdef _DEBUG
		m_bSHADER_SRGB_READ = true;
#endif
	}
public:
	sdk_refract_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB =  g_pHardwareConfig->NeedsShaderSRGBConversion();
#ifdef _DEBUG
		m_bBLUR = false;
#endif // _DEBUG
		m_nBLUR = 0;
#ifdef _DEBUG
		m_bFADEOUTONSILHOUETTE = false;
#endif // _DEBUG
		m_nFADEOUTONSILHOUETTE = 0;
#ifdef _DEBUG
		m_bCUBEMAP = false;
#endif // _DEBUG
		m_nCUBEMAP = 0;
#ifdef _DEBUG
		m_bREFRACTTINTTEXTURE = false;
#endif // _DEBUG
		m_nREFRACTTINTTEXTURE = 0;
#ifdef _DEBUG
		m_bMASKED = false;
#endif // _DEBUG
		m_nMASKED = 0;
#ifdef _DEBUG
		m_bCOLORMODULATE = false;
#endif // _DEBUG
		m_nCOLORMODULATE = 0;
#ifdef _DEBUG
		m_bSECONDARY_NORMAL = false;
#endif // _DEBUG
		m_nSECONDARY_NORMAL = 0;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = false;
#endif // _DEBUG
		m_nNORMAL_DECODE_MODE = 0;
#ifdef _DEBUG
		m_bSHADER_SRGB_READ = false;
#endif // _DEBUG
		m_nSHADER_SRGB_READ = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bBLUR && m_bFADEOUTONSILHOUETTE && m_bCUBEMAP && m_bREFRACTTINTTEXTURE && m_bMASKED && m_bCOLORMODULATE && m_bSECONDARY_NORMAL && m_bNORMAL_DECODE_MODE && m_bSHADER_SRGB_READ;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nCONVERT_TO_SRGB ) + ( 8 * m_nBLUR ) + ( 16 * m_nFADEOUTONSILHOUETTE ) + ( 32 * m_nCUBEMAP ) + ( 64 * m_nREFRACTTINTTEXTURE ) + ( 128 * m_nMASKED ) + ( 256 * m_nCOLORMODULATE ) + ( 512 * m_nSECONDARY_NORMAL ) + ( 1024 * m_nNORMAL_DECODE_MODE ) + ( 1024 * m_nSHADER_SRGB_READ ) + 0;
	}
};
#define shaderStaticTest_sdk_refract_ps20b psh_forgot_to_set_static_BLUR + psh_forgot_to_set_static_FADEOUTONSILHOUETTE + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_REFRACTTINTTEXTURE + psh_forgot_to_set_static_MASKED + psh_forgot_to_set_static_COLORMODULATE + psh_forgot_to_set_static_SECONDARY_NORMAL + psh_forgot_to_set_static_NORMAL_DECODE_MODE + psh_forgot_to_set_static_SHADER_SRGB_READ + 0
class sdk_refract_ps20b_Dynamic_Index
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
	int m_nWRITE_DEPTH_TO_DESTALPHA;
#ifdef _DEBUG
	bool m_bWRITE_DEPTH_TO_DESTALPHA;
#endif
public:
	void SetWRITE_DEPTH_TO_DESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITE_DEPTH_TO_DESTALPHA = i;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif
	}
	void SetWRITE_DEPTH_TO_DESTALPHA( bool i )
	{
		m_nWRITE_DEPTH_TO_DESTALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif
	}
public:
	sdk_refract_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
#endif // _DEBUG
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE && m_bWRITE_DEPTH_TO_DESTALPHA;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nWRITE_DEPTH_TO_DESTALPHA ) + 0;
	}
};
#define shaderDynamicTest_sdk_refract_ps20b psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA + 0
