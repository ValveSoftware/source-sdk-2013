// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// $MASKED && $BLUR
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// defined $HDRTYPE && defined $HDRENABLED && !$HDRTYPE && $HDRENABLED
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $HDRTYPE && $LIGHTING_PREVIEW && $HDRTYPE != 0
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class refract_ps20b_Static_Index
{
	unsigned int m_nCONVERT_TO_SRGB : 2;
	unsigned int m_nBLUR : 2;
	unsigned int m_nFADEOUTONSILHOUETTE : 2;
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nREFRACTTINTTEXTURE : 2;
	unsigned int m_nMASKED : 2;
	unsigned int m_nCOLORMODULATE : 2;
	unsigned int m_nSECONDARY_NORMAL : 2;
	unsigned int m_nNORMAL_DECODE_MODE : 1;
	unsigned int m_nSHADER_SRGB_READ : 2;
#ifdef _DEBUG
	bool m_bBLUR : 1;
	bool m_bFADEOUTONSILHOUETTE : 1;
	bool m_bCUBEMAP : 1;
	bool m_bREFRACTTINTTEXTURE : 1;
	bool m_bMASKED : 1;
	bool m_bCOLORMODULATE : 1;
	bool m_bSECONDARY_NORMAL : 1;
	bool m_bNORMAL_DECODE_MODE : 1;
	bool m_bSHADER_SRGB_READ : 1;
#endif	// _DEBUG
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_TO_SRGB = i;
	}

	void SetBLUR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLUR = i;
#ifdef _DEBUG
		m_bBLUR = true;
#endif	// _DEBUG
	}

	void SetFADEOUTONSILHOUETTE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFADEOUTONSILHOUETTE = i;
#ifdef _DEBUG
		m_bFADEOUTONSILHOUETTE = true;
#endif	// _DEBUG
	}

	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif	// _DEBUG
	}

	void SetREFRACTTINTTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nREFRACTTINTTEXTURE = i;
#ifdef _DEBUG
		m_bREFRACTTINTTEXTURE = true;
#endif	// _DEBUG
	}

	void SetMASKED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMASKED = i;
#ifdef _DEBUG
		m_bMASKED = true;
#endif	// _DEBUG
	}

	void SetCOLORMODULATE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLORMODULATE = i;
#ifdef _DEBUG
		m_bCOLORMODULATE = true;
#endif	// _DEBUG
	}

	void SetSECONDARY_NORMAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSECONDARY_NORMAL = i;
#ifdef _DEBUG
		m_bSECONDARY_NORMAL = true;
#endif	// _DEBUG
	}

	void SetNORMAL_DECODE_MODE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nNORMAL_DECODE_MODE = i;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = true;
#endif	// _DEBUG
	}

	void SetSHADER_SRGB_READ( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSHADER_SRGB_READ = i;
#ifdef _DEBUG
		m_bSHADER_SRGB_READ = true;
#endif	// _DEBUG
	}

	refract_ps20b_Static_Index(  )
	{
		m_nCONVERT_TO_SRGB = g_pHardwareConfig->NeedsShaderSRGBConversion();
		m_nBLUR = 0;
		m_nFADEOUTONSILHOUETTE = 0;
		m_nCUBEMAP = 0;
		m_nREFRACTTINTTEXTURE = 0;
		m_nMASKED = 0;
		m_nCOLORMODULATE = 0;
		m_nSECONDARY_NORMAL = 0;
		m_nNORMAL_DECODE_MODE = 0;
		m_nSHADER_SRGB_READ = 0;
#ifdef _DEBUG
		m_bBLUR = false;
		m_bFADEOUTONSILHOUETTE = false;
		m_bCUBEMAP = false;
		m_bREFRACTTINTTEXTURE = false;
		m_bMASKED = false;
		m_bCOLORMODULATE = false;
		m_bSECONDARY_NORMAL = false;
		m_bNORMAL_DECODE_MODE = false;
		m_bSHADER_SRGB_READ = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bBLUR && m_bFADEOUTONSILHOUETTE && m_bCUBEMAP && m_bREFRACTTINTTEXTURE && m_bMASKED && m_bCOLORMODULATE && m_bSECONDARY_NORMAL && m_bNORMAL_DECODE_MODE && m_bSHADER_SRGB_READ );
		AssertMsg( !( m_nMASKED && m_nBLUR ), "Invalid combo combination ( MASKED && BLUR )" );
		return ( 6 * m_nCONVERT_TO_SRGB ) + ( 12 * m_nBLUR ) + ( 24 * m_nFADEOUTONSILHOUETTE ) + ( 48 * m_nCUBEMAP ) + ( 96 * m_nREFRACTTINTTEXTURE ) + ( 192 * m_nMASKED ) + ( 384 * m_nCOLORMODULATE ) + ( 768 * m_nSECONDARY_NORMAL ) + ( 1536 * m_nNORMAL_DECODE_MODE ) + ( 1536 * m_nSHADER_SRGB_READ ) + 0;
	}
};

#define shaderStaticTest_refract_ps20b psh_forgot_to_set_static_BLUR + psh_forgot_to_set_static_FADEOUTONSILHOUETTE + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_REFRACTTINTTEXTURE + psh_forgot_to_set_static_MASKED + psh_forgot_to_set_static_COLORMODULATE + psh_forgot_to_set_static_SECONDARY_NORMAL + psh_forgot_to_set_static_NORMAL_DECODE_MODE + psh_forgot_to_set_static_SHADER_SRGB_READ


class refract_ps20b_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
#endif	// _DEBUG
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	void SetWRITE_DEPTH_TO_DESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITE_DEPTH_TO_DESTALPHA = i;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif	// _DEBUG
	}

	refract_ps20b_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE && m_bWRITE_DEPTH_TO_DESTALPHA );
		return ( 1 * m_nPIXELFOGTYPE ) + ( 3 * m_nWRITE_DEPTH_TO_DESTALPHA ) + 0;
	}
};

#define shaderDynamicTest_refract_ps20b psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA

