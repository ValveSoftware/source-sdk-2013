// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $FLASHLIGHT != 0 ) && ( $NUM_LIGHTS > 0 )
// ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTSHADOWS == 1 )
// ( $FLASHLIGHT == 0 ) && ( $FLASHLIGHTDEPTHFILTERMODE != 0 )
// (! $DETAILTEXTURE) && ( $DETAIL_BLEND_MODE != 0 )
// ( $FLASHLIGHT == 1 ) && ( $LIGHTWARPTEXTURE == 1 )
// ( $DIFFUSELIGHTING == 0 ) && ( $LIGHTWARPTEXTURE == 1 )
// ( $SELFILLUMFRESNEL == 1 ) && ( $LIGHTWARPTEXTURE == 1 )
// ( $SELFILLUM == 0 ) && ( $SELFILLUMFRESNEL == 1 )
// ( $FLASHLIGHT == 1 ) && ( $SELFILLUMFRESNEL == 1 )
// ( $FLASHLIGHT == 1 ) && ( $SELFILLUM == 1 )
// ( $SELFILLUMFRESNEL == 1 ) && ( $DETAILTEXTURE == 1 )
// ( $SELFILLUMFRESNEL == 1 ) && ( $NORMALMAPALPHAENVMAPMASK == 1 )
// ($BLENDTINTBYBASEALPHA) && ($SELFILLUM)
// $FLASHLIGHT && $CUBEMAP
// $NORMALMAPALPHAENVMAPMASK && !$CUBEMAP
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
class vertexlit_and_unlit_generic_bump_ps30_Static_Index
{
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nDIFFUSELIGHTING : 2;
	unsigned int m_nLIGHTWARPTEXTURE : 2;
	unsigned int m_nSELFILLUM : 2;
	unsigned int m_nSELFILLUMFRESNEL : 2;
	unsigned int m_nNORMALMAPALPHAENVMAPMASK : 2;
	unsigned int m_nHALFLAMBERT : 2;
	unsigned int m_nFLASHLIGHT : 2;
	unsigned int m_nDETAILTEXTURE : 2;
	unsigned int m_nDETAIL_BLEND_MODE : 3;
	unsigned int m_nFLASHLIGHTDEPTHFILTERMODE : 2;
	unsigned int m_nBLENDTINTBYBASEALPHA : 2;
#ifdef _DEBUG
	bool m_bCUBEMAP : 1;
	bool m_bDIFFUSELIGHTING : 1;
	bool m_bLIGHTWARPTEXTURE : 1;
	bool m_bSELFILLUM : 1;
	bool m_bSELFILLUMFRESNEL : 1;
	bool m_bNORMALMAPALPHAENVMAPMASK : 1;
	bool m_bHALFLAMBERT : 1;
	bool m_bFLASHLIGHT : 1;
	bool m_bDETAILTEXTURE : 1;
	bool m_bDETAIL_BLEND_MODE : 1;
	bool m_bFLASHLIGHTDEPTHFILTERMODE : 1;
	bool m_bBLENDTINTBYBASEALPHA : 1;
#endif	// _DEBUG
public:
	void SetCUBEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCUBEMAP = i;
#ifdef _DEBUG
		m_bCUBEMAP = true;
#endif	// _DEBUG
	}

	void SetDIFFUSELIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDIFFUSELIGHTING = i;
#ifdef _DEBUG
		m_bDIFFUSELIGHTING = true;
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

	void SetNORMALMAPALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAPALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
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

	void SetDETAILTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
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

	void SetFLASHLIGHTDEPTHFILTERMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFLASHLIGHTDEPTHFILTERMODE = i;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
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

	vertexlit_and_unlit_generic_bump_ps30_Static_Index(  )
	{
		m_nCUBEMAP = 0;
		m_nDIFFUSELIGHTING = 0;
		m_nLIGHTWARPTEXTURE = 0;
		m_nSELFILLUM = 0;
		m_nSELFILLUMFRESNEL = 0;
		m_nNORMALMAPALPHAENVMAPMASK = 0;
		m_nHALFLAMBERT = 0;
		m_nFLASHLIGHT = 0;
		m_nDETAILTEXTURE = 0;
		m_nDETAIL_BLEND_MODE = 0;
		m_nFLASHLIGHTDEPTHFILTERMODE = 0;
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bCUBEMAP = false;
		m_bDIFFUSELIGHTING = false;
		m_bLIGHTWARPTEXTURE = false;
		m_bSELFILLUM = false;
		m_bSELFILLUMFRESNEL = false;
		m_bNORMALMAPALPHAENVMAPMASK = false;
		m_bHALFLAMBERT = false;
		m_bFLASHLIGHT = false;
		m_bDETAILTEXTURE = false;
		m_bDETAIL_BLEND_MODE = false;
		m_bFLASHLIGHTDEPTHFILTERMODE = false;
		m_bBLENDTINTBYBASEALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCUBEMAP && m_bDIFFUSELIGHTING && m_bLIGHTWARPTEXTURE && m_bSELFILLUM && m_bSELFILLUMFRESNEL && m_bNORMALMAPALPHAENVMAPMASK && m_bHALFLAMBERT && m_bFLASHLIGHT && m_bDETAILTEXTURE && m_bDETAIL_BLEND_MODE && m_bFLASHLIGHTDEPTHFILTERMODE && m_bBLENDTINTBYBASEALPHA );
		AssertMsg( !( ( m_nFLASHLIGHT == 0 ) && ( m_nFLASHLIGHTDEPTHFILTERMODE != 0 ) ), "Invalid combo combination ( ( FLASHLIGHT == 0 ) && ( FLASHLIGHTDEPTHFILTERMODE != 0 ) )" );
		AssertMsg( !( !m_nDETAILTEXTURE && ( m_nDETAIL_BLEND_MODE != 0 ) ), "Invalid combo combination ( !DETAILTEXTURE && ( DETAIL_BLEND_MODE != 0 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 1 ) && ( m_nLIGHTWARPTEXTURE == 1 ) ), "Invalid combo combination ( ( FLASHLIGHT == 1 ) && ( LIGHTWARPTEXTURE == 1 ) )" );
		AssertMsg( !( ( m_nDIFFUSELIGHTING == 0 ) && ( m_nLIGHTWARPTEXTURE == 1 ) ), "Invalid combo combination ( ( DIFFUSELIGHTING == 0 ) && ( LIGHTWARPTEXTURE == 1 ) )" );
		AssertMsg( !( ( m_nSELFILLUMFRESNEL == 1 ) && ( m_nLIGHTWARPTEXTURE == 1 ) ), "Invalid combo combination ( ( SELFILLUMFRESNEL == 1 ) && ( LIGHTWARPTEXTURE == 1 ) )" );
		AssertMsg( !( ( m_nSELFILLUM == 0 ) && ( m_nSELFILLUMFRESNEL == 1 ) ), "Invalid combo combination ( ( SELFILLUM == 0 ) && ( SELFILLUMFRESNEL == 1 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 1 ) && ( m_nSELFILLUMFRESNEL == 1 ) ), "Invalid combo combination ( ( FLASHLIGHT == 1 ) && ( SELFILLUMFRESNEL == 1 ) )" );
		AssertMsg( !( ( m_nFLASHLIGHT == 1 ) && ( m_nSELFILLUM == 1 ) ), "Invalid combo combination ( ( FLASHLIGHT == 1 ) && ( SELFILLUM == 1 ) )" );
		AssertMsg( !( ( m_nSELFILLUMFRESNEL == 1 ) && ( m_nDETAILTEXTURE == 1 ) ), "Invalid combo combination ( ( SELFILLUMFRESNEL == 1 ) && ( DETAILTEXTURE == 1 ) )" );
		AssertMsg( !( ( m_nSELFILLUMFRESNEL == 1 ) && ( m_nNORMALMAPALPHAENVMAPMASK == 1 ) ), "Invalid combo combination ( ( SELFILLUMFRESNEL == 1 ) && ( NORMALMAPALPHAENVMAPMASK == 1 ) )" );
		AssertMsg( !( m_nBLENDTINTBYBASEALPHA && m_nSELFILLUM ), "Invalid combo combination ( BLENDTINTBYBASEALPHA && SELFILLUM )" );
		AssertMsg( !( m_nFLASHLIGHT && m_nCUBEMAP ), "Invalid combo combination ( FLASHLIGHT && CUBEMAP )" );
		AssertMsg( !( m_nNORMALMAPALPHAENVMAPMASK && !m_nCUBEMAP ), "Invalid combo combination ( NORMALMAPALPHAENVMAPMASK && !CUBEMAP )" );
		return ( 60 * m_nCUBEMAP ) + ( 120 * m_nDIFFUSELIGHTING ) + ( 240 * m_nLIGHTWARPTEXTURE ) + ( 480 * m_nSELFILLUM ) + ( 960 * m_nSELFILLUMFRESNEL ) + ( 1920 * m_nNORMALMAPALPHAENVMAPMASK ) + ( 3840 * m_nHALFLAMBERT ) + ( 7680 * m_nFLASHLIGHT ) + ( 15360 * m_nDETAILTEXTURE ) + ( 30720 * m_nDETAIL_BLEND_MODE ) + ( 215040 * m_nFLASHLIGHTDEPTHFILTERMODE ) + ( 645120 * m_nBLENDTINTBYBASEALPHA ) + 0;
	}
};

#define shaderStaticTest_vertexlit_and_unlit_generic_bump_ps30 psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_DIFFUSELIGHTING + psh_forgot_to_set_static_LIGHTWARPTEXTURE + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_SELFILLUMFRESNEL + psh_forgot_to_set_static_NORMALMAPALPHAENVMAPMASK + psh_forgot_to_set_static_HALFLAMBERT + psh_forgot_to_set_static_FLASHLIGHT + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_DETAIL_BLEND_MODE + psh_forgot_to_set_static_FLASHLIGHTDEPTHFILTERMODE + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA


class vertexlit_and_unlit_generic_bump_ps30_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nNUM_LIGHTS : 3;
	unsigned int m_nAMBIENT_LIGHT : 2;
	unsigned int m_nFLASHLIGHTSHADOWS : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
	bool m_bNUM_LIGHTS : 1;
	bool m_bAMBIENT_LIGHT : 1;
	bool m_bFLASHLIGHTSHADOWS : 1;
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

	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif	// _DEBUG
	}

	void SetAMBIENT_LIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nAMBIENT_LIGHT = i;
#ifdef _DEBUG
		m_bAMBIENT_LIGHT = true;
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

	vertexlit_and_unlit_generic_bump_ps30_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
		m_nNUM_LIGHTS = 0;
		m_nAMBIENT_LIGHT = 0;
		m_nFLASHLIGHTSHADOWS = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
		m_bNUM_LIGHTS = false;
		m_bAMBIENT_LIGHT = false;
		m_bFLASHLIGHTSHADOWS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE && m_bNUM_LIGHTS && m_bAMBIENT_LIGHT && m_bFLASHLIGHTSHADOWS );
		return ( 1 * m_nPIXELFOGTYPE ) + ( 3 * m_nNUM_LIGHTS ) + ( 15 * m_nAMBIENT_LIGHT ) + ( 30 * m_nFLASHLIGHTSHADOWS ) + 0;
	}
};

#define shaderDynamicTest_vertexlit_and_unlit_generic_bump_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_NUM_LIGHTS + psh_forgot_to_set_dynamic_AMBIENT_LIGHT + psh_forgot_to_set_dynamic_FLASHLIGHTSHADOWS

