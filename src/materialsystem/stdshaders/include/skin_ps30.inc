// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ($PIXELFOGTYPE == 0) && ($WRITEWATERFOGTODESTALPHA != 0)
// (! $DETAILTEXTURE) && ( $DETAIL_BLEND_MODE != 0 )
// ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )
// ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 )
// ( $SELFILLUM == 0 ) && ( $SELFILLUMFRESNEL == 1 )
// ( $FLASHLIGHT == 1 ) && ( $SELFILLUMFRESNEL == 1 )
// ( $FLASHLIGHT == 1 ) && ( $SELFILLUM == 1 )
// ( $BLENDTINTBYBASEALPHA ) && ( $SELFILLUM )
// $FASTPATH_NOBUMP && ( $RIMLIGHT || $DETAILTEXTURE || $PHONGWARPTEXTURE || $SELFILLUM || $BLENDTINTBYBASEALPHA )
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
class skin_ps30_Static_Index
{
	unsigned int m_nCONVERT_TO_SRGB : 1;
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nSELFILLUM : 2;
	unsigned int m_nSELFILLUMFRESNEL : 2;
	unsigned int m_nFLASHLIGHT : 2;
	unsigned int m_nLIGHTWARPTEXTURE : 2;
	unsigned int m_nPHONGWARPTEXTURE : 2;
	unsigned int m_nWRINKLEMAP : 2;
	unsigned int m_nDETAIL_BLEND_MODE : 3;
	unsigned int m_nDETAILTEXTURE : 2;
	unsigned int m_nRIMLIGHT : 2;
	unsigned int m_nFLASHLIGHTDEPTHFILTERMODE : 2;
	unsigned int m_nFASTPATH_NOBUMP : 2;
	unsigned int m_nBLENDTINTBYBASEALPHA : 2;
#ifdef _DEBUG
	bool m_bCONVERT_TO_SRGB : 1;
	bool m_bCUBEMAP : 1;
	bool m_bSELFILLUM : 1;
	bool m_bSELFILLUMFRESNEL : 1;
	bool m_bFLASHLIGHT : 1;
	bool m_bLIGHTWARPTEXTURE : 1;
	bool m_bPHONGWARPTEXTURE : 1;
	bool m_bWRINKLEMAP : 1;
	bool m_bDETAIL_BLEND_MODE : 1;
	bool m_bDETAILTEXTURE : 1;
	bool m_bRIMLIGHT : 1;
	bool m_bFLASHLIGHTDEPTHFILTERMODE : 1;
	bool m_bFASTPATH_NOBUMP : 1;
	bool m_bBLENDTINTBYBASEALPHA : 1;
#endif	// _DEBUG
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nCONVERT_TO_SRGB = i;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = true;
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

	void SetSELFILLUM( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUM = i;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif	// _DEBUG
	}

	void SetSELFILLUMFRESNEL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUMFRESNEL = i;
#ifdef _DEBUG
		m_bSELFILLUMFRESNEL = true;
#endif	// _DEBUG
	}

	void SetFLASHLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHT = i;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif	// _DEBUG
	}

	void SetLIGHTWARPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLIGHTWARPTEXTURE = i;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = true;
#endif	// _DEBUG
	}

	void SetPHONGWARPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONGWARPTEXTURE = i;
#ifdef _DEBUG
		m_bPHONGWARPTEXTURE = true;
#endif	// _DEBUG
	}

	void SetWRINKLEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRINKLEMAP = i;
#ifdef _DEBUG
		m_bWRINKLEMAP = true;
#endif	// _DEBUG
	}

	void SetDETAIL_BLEND_MODE( int i )
	{
		Assert( i >= 0 && i <= 6 );
		m_nDETAIL_BLEND_MODE = i;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = true;
#endif	// _DEBUG
	}

	void SetDETAILTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif	// _DEBUG
	}

	void SetRIMLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nRIMLIGHT = i;
#ifdef _DEBUG
		m_bRIMLIGHT = true;
#endif	// _DEBUG
	}

	void SetFLASHLIGHTDEPTHFILTERMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFLASHLIGHTDEPTHFILTERMODE = i;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
#endif	// _DEBUG
	}

	void SetFASTPATH_NOBUMP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH_NOBUMP = i;
#ifdef _DEBUG
		m_bFASTPATH_NOBUMP = true;
#endif	// _DEBUG
	}

	void SetBLENDTINTBYBASEALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLENDTINTBYBASEALPHA = i;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = true;
#endif	// _DEBUG
	}

	skin_ps30_Static_Index(  )
	{
		m_nCONVERT_TO_SRGB = 0;
		m_nCUBEMAP = 0;
		m_nSELFILLUM = 0;
		m_nSELFILLUMFRESNEL = 0;
		m_nFLASHLIGHT = 0;
		m_nLIGHTWARPTEXTURE = 0;
		m_nPHONGWARPTEXTURE = 0;
		m_nWRINKLEMAP = 0;
		m_nDETAIL_BLEND_MODE = 0;
		m_nDETAILTEXTURE = 0;
		m_nRIMLIGHT = 0;
		m_nFLASHLIGHTDEPTHFILTERMODE = 0;
		m_nFASTPATH_NOBUMP = 0;
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = false;
		m_bCUBEMAP = false;
		m_bSELFILLUM = false;
		m_bSELFILLUMFRESNEL = false;
		m_bFLASHLIGHT = false;
		m_bLIGHTWARPTEXTURE = false;
		m_bPHONGWARPTEXTURE = false;
		m_bWRINKLEMAP = false;
		m_bDETAIL_BLEND_MODE = false;
		m_bDETAILTEXTURE = false;
		m_bRIMLIGHT = false;
		m_bFLASHLIGHTDEPTHFILTERMODE = false;
		m_bFASTPATH_NOBUMP = false;
		m_bBLENDTINTBYBASEALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCONVERT_TO_SRGB && m_bCUBEMAP && m_bSELFILLUM && m_bSELFILLUMFRESNEL && m_bFLASHLIGHT && m_bLIGHTWARPTEXTURE && m_bPHONGWARPTEXTURE && m_bWRINKLEMAP && m_bDETAIL_BLEND_MODE && m_bDETAILTEXTURE && m_bRIMLIGHT && m_bFLASHLIGHTDEPTHFILTERMODE && m_bFASTPATH_NOBUMP && m_bBLENDTINTBYBASEALPHA );
		AssertMsg( !( !m_nDETAILTEXTURE && ( m_nDETAIL_BLEND_MODE != 0 ) ), "Invalid combo combination ( !DETAILTEXTURE && ( DETAIL_BLEND_MODE != 0 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 0 ) && ( m_nFLASHLIGHTDEPTHFILTERMODE != 0 ) ), "Invalid combo combination ( ( FLASHLIGHT == 0 ) && ( FLASHLIGHTDEPTHFILTERMODE != 0 ) )" );
		AssertMsg( !( ( m_nSELFILLUM == 0 ) && ( m_nSELFILLUMFRESNEL == 1 ) ), "Invalid combo combination ( ( SELFILLUM == 0 ) && ( SELFILLUMFRESNEL == 1 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 1 ) && ( m_nSELFILLUMFRESNEL == 1 ) ), "Invalid combo combination ( ( FLASHLIGHT == 1 ) && ( SELFILLUMFRESNEL == 1 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 1 ) && ( m_nSELFILLUM == 1 ) ), "Invalid combo combination ( ( FLASHLIGHT == 1 ) && ( SELFILLUM == 1 ) )" );
		AssertMsg( !( m_nBLENDTINTBYBASEALPHA && m_nSELFILLUM ), "Invalid combo combination ( BLENDTINTBYBASEALPHA && SELFILLUM )" );
		AssertMsg( !( m_nFASTPATH_NOBUMP && ( m_nRIMLIGHT || ( m_nDETAILTEXTURE || ( m_nPHONGWARPTEXTURE || ( m_nSELFILLUM || m_nBLENDTINTBYBASEALPHA ) ) ) ) ), "Invalid combo combination ( FASTPATH_NOBUMP && ( RIMLIGHT || ( DETAILTEXTURE || ( PHONGWARPTEXTURE || ( SELFILLUM || BLENDTINTBYBASEALPHA ) ) ) ) )" );
		return ( 240 * m_nCONVERT_TO_SRGB ) + ( 240 * m_nCUBEMAP ) + ( 480 * m_nSELFILLUM ) + ( 960 * m_nSELFILLUMFRESNEL ) + ( 1920 * m_nFLASHLIGHT ) + ( 3840 * m_nLIGHTWARPTEXTURE ) + ( 7680 * m_nPHONGWARPTEXTURE ) + ( 15360 * m_nWRINKLEMAP ) + ( 30720 * m_nDETAIL_BLEND_MODE ) + ( 215040 * m_nDETAILTEXTURE ) + ( 430080 * m_nRIMLIGHT ) + ( 860160 * m_nFLASHLIGHTDEPTHFILTERMODE ) + ( 2580480 * m_nFASTPATH_NOBUMP ) + ( 5160960 * m_nBLENDTINTBYBASEALPHA ) + 0;
	}
};

#define shaderStaticTest_skin_ps30 psh_forgot_to_set_static_CONVERT_TO_SRGB + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_SELFILLUMFRESNEL + psh_forgot_to_set_static_FLASHLIGHT + psh_forgot_to_set_static_LIGHTWARPTEXTURE + psh_forgot_to_set_static_PHONGWARPTEXTURE + psh_forgot_to_set_static_WRINKLEMAP + psh_forgot_to_set_static_DETAIL_BLEND_MODE + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_RIMLIGHT + psh_forgot_to_set_static_FLASHLIGHTDEPTHFILTERMODE + psh_forgot_to_set_static_FASTPATH_NOBUMP + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA


class skin_ps30_Dynamic_Index
{
	unsigned int m_nWRITEWATERFOGTODESTALPHA : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nNUM_LIGHTS : 3;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
	unsigned int m_nFLASHLIGHTSHADOWS : 2;
	unsigned int m_nPHONG_USE_EXPONENT_FACTOR : 2;
#ifdef _DEBUG
	bool m_bWRITEWATERFOGTODESTALPHA : 1;
	bool m_bPIXELFOGTYPE : 1;
	bool m_bNUM_LIGHTS : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
	bool m_bFLASHLIGHTSHADOWS : 1;
	bool m_bPHONG_USE_EXPONENT_FACTOR : 1;
#endif	// _DEBUG
public:
	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
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

	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
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

	void SetFLASHLIGHTSHADOWS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHTSHADOWS = i;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif	// _DEBUG
	}

	void SetPHONG_USE_EXPONENT_FACTOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONG_USE_EXPONENT_FACTOR = i;
#ifdef _DEBUG
		m_bPHONG_USE_EXPONENT_FACTOR = true;
#endif	// _DEBUG
	}

	skin_ps30_Dynamic_Index(  )
	{
		m_nWRITEWATERFOGTODESTALPHA = 0;
		m_nPIXELFOGTYPE = 0;
		m_nNUM_LIGHTS = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
		m_nFLASHLIGHTSHADOWS = 0;
		m_nPHONG_USE_EXPONENT_FACTOR = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
		m_bPIXELFOGTYPE = false;
		m_bNUM_LIGHTS = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
		m_bFLASHLIGHTSHADOWS = false;
		m_bPHONG_USE_EXPONENT_FACTOR = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bWRITEWATERFOGTODESTALPHA && m_bPIXELFOGTYPE && m_bNUM_LIGHTS && m_bWRITE_DEPTH_TO_DESTALPHA && m_bFLASHLIGHTSHADOWS && m_bPHONG_USE_EXPONENT_FACTOR );
		AssertMsg( !( ( m_nPIXELFOGTYPE == 0 ) && ( m_nWRITEWATERFOGTODESTALPHA != 0 ) ), "Invalid combo combination ( ( PIXELFOGTYPE == 0 ) && ( WRITEWATERFOGTODESTALPHA != 0 ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		return ( 1 * m_nWRITEWATERFOGTODESTALPHA ) + ( 2 * m_nPIXELFOGTYPE ) + ( 6 * m_nNUM_LIGHTS ) + ( 30 * m_nWRITE_DEPTH_TO_DESTALPHA ) + ( 60 * m_nFLASHLIGHTSHADOWS ) + ( 120 * m_nPHONG_USE_EXPONENT_FACTOR ) + 0;
	}
};

#define shaderDynamicTest_skin_ps30 psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_NUM_LIGHTS + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA + psh_forgot_to_set_dynamic_FLASHLIGHTSHADOWS + psh_forgot_to_set_dynamic_PHONG_USE_EXPONENT_FACTOR

