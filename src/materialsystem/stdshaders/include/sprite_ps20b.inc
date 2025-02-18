// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
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
class sprite_ps20b_Static_Index
{
	unsigned int m_nCONVERT_TO_SRGB : 2;
	unsigned int m_nVERTEXCOLOR : 2;
	unsigned int m_nCONSTANTCOLOR : 2;
	unsigned int m_nHDRTYPE : 2;
	unsigned int m_nSRGB : 2;
	unsigned int m_nSRGB_OUTPUT_ADAPTER : 2;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR : 1;
	bool m_bCONSTANTCOLOR : 1;
	bool m_bHDRTYPE : 1;
	bool m_bSRGB : 1;
	bool m_bSRGB_OUTPUT_ADAPTER : 1;
#endif	// _DEBUG
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_TO_SRGB = i;
	}

	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif	// _DEBUG
	}

	void SetCONSTANTCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONSTANTCOLOR = i;
#ifdef _DEBUG
		m_bCONSTANTCOLOR = true;
#endif	// _DEBUG
	}

	void SetHDRTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nHDRTYPE = i;
#ifdef _DEBUG
		m_bHDRTYPE = true;
#endif	// _DEBUG
	}

	void SetSRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSRGB = i;
#ifdef _DEBUG
		m_bSRGB = true;
#endif	// _DEBUG
	}

	void SetSRGB_OUTPUT_ADAPTER( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSRGB_OUTPUT_ADAPTER = i;
#ifdef _DEBUG
		m_bSRGB_OUTPUT_ADAPTER = true;
#endif	// _DEBUG
	}

	sprite_ps20b_Static_Index(  )
	{
		m_nCONVERT_TO_SRGB = g_pHardwareConfig->NeedsShaderSRGBConversion();
		m_nVERTEXCOLOR = 0;
		m_nCONSTANTCOLOR = 0;
		m_nHDRTYPE = 0;
		m_nSRGB = 0;
		m_nSRGB_OUTPUT_ADAPTER = 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
		m_bCONSTANTCOLOR = false;
		m_bHDRTYPE = false;
		m_bSRGB = false;
		m_bSRGB_OUTPUT_ADAPTER = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bVERTEXCOLOR && m_bCONSTANTCOLOR && m_bHDRTYPE && m_bSRGB && m_bSRGB_OUTPUT_ADAPTER );
		return ( 6 * m_nCONVERT_TO_SRGB ) + ( 12 * m_nVERTEXCOLOR ) + ( 24 * m_nCONSTANTCOLOR ) + ( 48 * m_nHDRTYPE ) + ( 144 * m_nSRGB ) + ( 288 * m_nSRGB_OUTPUT_ADAPTER ) + 0;
	}
};

#define shaderStaticTest_sprite_ps20b psh_forgot_to_set_static_VERTEXCOLOR + psh_forgot_to_set_static_CONSTANTCOLOR + psh_forgot_to_set_static_HDRTYPE + psh_forgot_to_set_static_SRGB + psh_forgot_to_set_static_SRGB_OUTPUT_ADAPTER


class sprite_ps20b_Dynamic_Index
{
	unsigned int m_nHDRENABLED : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
#ifdef _DEBUG
	bool m_bHDRENABLED : 1;
	bool m_bPIXELFOGTYPE : 1;
#endif	// _DEBUG
public:
	void SetHDRENABLED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHDRENABLED = i;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif	// _DEBUG
	}

	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	sprite_ps20b_Dynamic_Index(  )
	{
		m_nHDRENABLED = 0;
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bHDRENABLED = false;
		m_bPIXELFOGTYPE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bHDRENABLED && m_bPIXELFOGTYPE );
		return ( 1 * m_nHDRENABLED ) + ( 2 * m_nPIXELFOGTYPE ) + 0;
	}
};

#define shaderDynamicTest_sprite_ps20b psh_forgot_to_set_dynamic_HDRENABLED + psh_forgot_to_set_dynamic_PIXELFOGTYPE

