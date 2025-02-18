// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// $SEAMLESS && $RELIEF_MAPPING
// (! $DETAILTEXTURE) && ( $DETAIL_BLEND_MODE != 0 )
// $SEAMLESS && ( $OUTLINE || $SOFTEDGES)
// $BASETEXTURE2 && ( $OUTLINE || $SOFTEDGES)
// $BUMPMAP2 && ( $OUTLINE || $SOFTEDGES)
// $SELFILLUM && ( $OUTLINE || $SOFTEDGES)
// $MASKEDBLENDING && ( $OUTLINE || $SOFTEDGES)
// $FANCY_BLENDING && ( $OUTLINE || $SOFTEDGES)
// $LIGHTING_PREVIEW && ( $OUTLINE || $SOFTEDGES)
// ($FASTPATH == 0) && ( $OUTLINE || $SOFTEDGES)
// ($DETAILTEXTURE && $BUMPMAP) && ( $OUTLINE || $SOFTEDGES)
// ($WARPLIGHTING) && ( $OUTLINE || $SOFTEDGES)
// ($BUMPMAP) && ( $OUTLINE || $SOFTEDGES)
// ($DETAIL_BLEND_MODE == 2 ) || ($DETAIL_BLEND_MODE == 3 ) || ($DETAIL_BLEND_MODE == 4 )
// ($DETAIL_BLEND_MODE == 5 ) || ($DETAIL_BLEND_MODE == 6 ) || ($DETAIL_BLEND_MODE == 7 )
// ($DETAIL_BLEND_MODE == 8 ) || ($DETAIL_BLEND_MODE == 9 )
// $BUMPMAP2 && $WARPLIGHTING
// $WARPLIGHTING && $DETAILTEXTURE
// $ENVMAPMASK && $BUMPMAP
// $NORMALMAPALPHAENVMAPMASK && $BASEALPHAENVMAPMASK
// $NORMALMAPALPHAENVMAPMASK && $ENVMAPMASK
// $BASEALPHAENVMAPMASK && $ENVMAPMASK
// $BASEALPHAENVMAPMASK && $SELFILLUM
// !$FASTPATH && $FASTPATHENVMAPCONTRAST
// !$FASTPATH && $FASTPATHENVMAPTINT
// !$BUMPMAP && $DIFFUSEBUMPMAP
// !$BUMPMAP && $BUMPMAP2
// $ENVMAPMASK && $BUMPMAP2
// $BASETEXTURENOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
// $BASETEXTURE2NOENVMAP && ( !$BASETEXTURE2 || !$CUBEMAP )
// $BASEALPHAENVMAPMASK && $BUMPMAP
// $PARALLAXMAP && $DETAILTEXTURE
// $SEAMLESS && $RELIEF_MAPPING
// $SEAMLESS && $DETAILTEXTURE
// $SEAMLESS && $MASKEDBLENDING
// $BUMPMASK && ( $SEAMLESS || $DETAILTEXTURE || $SELFILLUM || $BASETEXTURENOENVMAP || $BASETEXTURE2 )
// !$BUMPMAP && ($NORMAL_DECODE_MODE == 1)
// !$BUMPMAP && ($NORMAL_DECODE_MODE == 2)
// !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 1)
// !$BUMPMAP && ($NORMALMASK_DECODE_MODE == 2)
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
class lightmappedgeneric_ps20b_Static_Index
{
	unsigned int m_nMASKEDBLENDING : 2;
	unsigned int m_nBASETEXTURE2 : 2;
	unsigned int m_nDETAILTEXTURE : 2;
	unsigned int m_nBUMPMAP : 2;
	unsigned int m_nBUMPMAP2 : 2;
	unsigned int m_nCUBEMAP : 2;
	unsigned int m_nENVMAPMASK : 2;
	unsigned int m_nBASEALPHAENVMAPMASK : 2;
	unsigned int m_nSELFILLUM : 2;
	unsigned int m_nNORMALMAPALPHAENVMAPMASK : 2;
	unsigned int m_nDIFFUSEBUMPMAP : 2;
	unsigned int m_nBASETEXTURENOENVMAP : 2;
	unsigned int m_nBASETEXTURE2NOENVMAP : 2;
	unsigned int m_nWARPLIGHTING : 2;
	unsigned int m_nFANCY_BLENDING : 2;
	unsigned int m_nRELIEF_MAPPING : 1;
	unsigned int m_nSEAMLESS : 2;
	unsigned int m_nOUTLINE : 2;
	unsigned int m_nSOFTEDGES : 2;
	unsigned int m_nBUMPMASK : 2;
	unsigned int m_nNORMAL_DECODE_MODE : 1;
	unsigned int m_nNORMALMASK_DECODE_MODE : 1;
	unsigned int m_nDETAIL_BLEND_MODE : 4;
#ifdef _DEBUG
	bool m_bMASKEDBLENDING : 1;
	bool m_bBASETEXTURE2 : 1;
	bool m_bDETAILTEXTURE : 1;
	bool m_bBUMPMAP : 1;
	bool m_bBUMPMAP2 : 1;
	bool m_bCUBEMAP : 1;
	bool m_bENVMAPMASK : 1;
	bool m_bBASEALPHAENVMAPMASK : 1;
	bool m_bSELFILLUM : 1;
	bool m_bNORMALMAPALPHAENVMAPMASK : 1;
	bool m_bDIFFUSEBUMPMAP : 1;
	bool m_bBASETEXTURENOENVMAP : 1;
	bool m_bBASETEXTURE2NOENVMAP : 1;
	bool m_bWARPLIGHTING : 1;
	bool m_bFANCY_BLENDING : 1;
	bool m_bRELIEF_MAPPING : 1;
	bool m_bSEAMLESS : 1;
	bool m_bOUTLINE : 1;
	bool m_bSOFTEDGES : 1;
	bool m_bBUMPMASK : 1;
	bool m_bNORMAL_DECODE_MODE : 1;
	bool m_bNORMALMASK_DECODE_MODE : 1;
	bool m_bDETAIL_BLEND_MODE : 1;
#endif	// _DEBUG
public:
	void SetMASKEDBLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMASKEDBLENDING = i;
#ifdef _DEBUG
		m_bMASKEDBLENDING = true;
#endif	// _DEBUG
	}

	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
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

	void SetBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nBUMPMAP = i;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif	// _DEBUG
	}

	void SetBUMPMAP2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP2 = i;
#ifdef _DEBUG
		m_bBUMPMAP2 = true;
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

	void SetENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPMASK = i;
#ifdef _DEBUG
		m_bENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetBASEALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASEALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = true;
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

	void SetNORMALMAPALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAPALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = true;
#endif	// _DEBUG
	}

	void SetDIFFUSEBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDIFFUSEBUMPMAP = i;
#ifdef _DEBUG
		m_bDIFFUSEBUMPMAP = true;
#endif	// _DEBUG
	}

	void SetBASETEXTURENOENVMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURENOENVMAP = i;
#ifdef _DEBUG
		m_bBASETEXTURENOENVMAP = true;
#endif	// _DEBUG
	}

	void SetBASETEXTURE2NOENVMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2NOENVMAP = i;
#ifdef _DEBUG
		m_bBASETEXTURE2NOENVMAP = true;
#endif	// _DEBUG
	}

	void SetWARPLIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWARPLIGHTING = i;
#ifdef _DEBUG
		m_bWARPLIGHTING = true;
#endif	// _DEBUG
	}

	void SetFANCY_BLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFANCY_BLENDING = i;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif	// _DEBUG
	}

	void SetRELIEF_MAPPING( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nRELIEF_MAPPING = i;
#ifdef _DEBUG
		m_bRELIEF_MAPPING = true;
#endif	// _DEBUG
	}

	void SetSEAMLESS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS = i;
#ifdef _DEBUG
		m_bSEAMLESS = true;
#endif	// _DEBUG
	}

	void SetOUTLINE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nOUTLINE = i;
#ifdef _DEBUG
		m_bOUTLINE = true;
#endif	// _DEBUG
	}

	void SetSOFTEDGES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSOFTEDGES = i;
#ifdef _DEBUG
		m_bSOFTEDGES = true;
#endif	// _DEBUG
	}

	void SetBUMPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMASK = i;
#ifdef _DEBUG
		m_bBUMPMASK = true;
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

	void SetNORMALMASK_DECODE_MODE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nNORMALMASK_DECODE_MODE = i;
#ifdef _DEBUG
		m_bNORMALMASK_DECODE_MODE = true;
#endif	// _DEBUG
	}

	void SetDETAIL_BLEND_MODE( int i )
	{
		Assert( i >= 0 && i <= 11 );
		m_nDETAIL_BLEND_MODE = i;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = true;
#endif	// _DEBUG
	}

	lightmappedgeneric_ps20b_Static_Index(  )
	{
		m_nMASKEDBLENDING = 0;
		m_nBASETEXTURE2 = 0;
		m_nDETAILTEXTURE = 0;
		m_nBUMPMAP = 0;
		m_nBUMPMAP2 = 0;
		m_nCUBEMAP = 0;
		m_nENVMAPMASK = 0;
		m_nBASEALPHAENVMAPMASK = 0;
		m_nSELFILLUM = 0;
		m_nNORMALMAPALPHAENVMAPMASK = 0;
		m_nDIFFUSEBUMPMAP = 0;
		m_nBASETEXTURENOENVMAP = 0;
		m_nBASETEXTURE2NOENVMAP = 0;
		m_nWARPLIGHTING = 0;
		m_nFANCY_BLENDING = 0;
		m_nRELIEF_MAPPING = 0;
		m_nSEAMLESS = 0;
		m_nOUTLINE = 0;
		m_nSOFTEDGES = 0;
		m_nBUMPMASK = 0;
		m_nNORMAL_DECODE_MODE = 0;
		m_nNORMALMASK_DECODE_MODE = 0;
		m_nDETAIL_BLEND_MODE = 0;
#ifdef _DEBUG
		m_bMASKEDBLENDING = false;
		m_bBASETEXTURE2 = false;
		m_bDETAILTEXTURE = false;
		m_bBUMPMAP = false;
		m_bBUMPMAP2 = false;
		m_bCUBEMAP = false;
		m_bENVMAPMASK = false;
		m_bBASEALPHAENVMAPMASK = false;
		m_bSELFILLUM = false;
		m_bNORMALMAPALPHAENVMAPMASK = false;
		m_bDIFFUSEBUMPMAP = false;
		m_bBASETEXTURENOENVMAP = false;
		m_bBASETEXTURE2NOENVMAP = false;
		m_bWARPLIGHTING = false;
		m_bFANCY_BLENDING = false;
		m_bRELIEF_MAPPING = false;
		m_bSEAMLESS = false;
		m_bOUTLINE = false;
		m_bSOFTEDGES = false;
		m_bBUMPMASK = false;
		m_bNORMAL_DECODE_MODE = false;
		m_bNORMALMASK_DECODE_MODE = false;
		m_bDETAIL_BLEND_MODE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bMASKEDBLENDING && m_bBASETEXTURE2 && m_bDETAILTEXTURE && m_bBUMPMAP && m_bBUMPMAP2 && m_bCUBEMAP && m_bENVMAPMASK && m_bBASEALPHAENVMAPMASK && m_bSELFILLUM && m_bNORMALMAPALPHAENVMAPMASK && m_bDIFFUSEBUMPMAP && m_bBASETEXTURENOENVMAP && m_bBASETEXTURE2NOENVMAP && m_bWARPLIGHTING && m_bFANCY_BLENDING && m_bRELIEF_MAPPING && m_bSEAMLESS && m_bOUTLINE && m_bSOFTEDGES && m_bBUMPMASK && m_bNORMAL_DECODE_MODE && m_bNORMALMASK_DECODE_MODE && m_bDETAIL_BLEND_MODE );
		AssertMsg( !( m_nSEAMLESS && m_nRELIEF_MAPPING ), "Invalid combo combination ( SEAMLESS && RELIEF_MAPPING )" );
		AssertMsg( !( !m_nDETAILTEXTURE && ( m_nDETAIL_BLEND_MODE != 0 ) ), "Invalid combo combination ( !DETAILTEXTURE && ( DETAIL_BLEND_MODE != 0 ) )" );
		AssertMsg( !( m_nSEAMLESS && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( SEAMLESS && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nBASETEXTURE2 && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( BASETEXTURE2 && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nBUMPMAP2 && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( BUMPMAP2 && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nSELFILLUM && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( SELFILLUM && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nMASKEDBLENDING && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( MASKEDBLENDING && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nFANCY_BLENDING && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( FANCY_BLENDING && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( ( m_nDETAILTEXTURE && m_nBUMPMAP ) && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( ( DETAILTEXTURE && BUMPMAP ) && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nWARPLIGHTING && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( WARPLIGHTING && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( m_nBUMPMAP && ( m_nOUTLINE || m_nSOFTEDGES ) ), "Invalid combo combination ( BUMPMAP && ( OUTLINE || SOFTEDGES ) )" );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 2 ) || ( ( m_nDETAIL_BLEND_MODE == 3 ) || ( m_nDETAIL_BLEND_MODE == 4 ) ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 2 ) || ( ( DETAIL_BLEND_MODE == 3 ) || ( DETAIL_BLEND_MODE == 4 ) ) )" );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 5 ) || ( ( m_nDETAIL_BLEND_MODE == 6 ) || ( m_nDETAIL_BLEND_MODE == 7 ) ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 5 ) || ( ( DETAIL_BLEND_MODE == 6 ) || ( DETAIL_BLEND_MODE == 7 ) ) )" );
		AssertMsg( !( ( m_nDETAIL_BLEND_MODE == 8 ) || ( m_nDETAIL_BLEND_MODE == 9 ) ), "Invalid combo combination ( ( DETAIL_BLEND_MODE == 8 ) || ( DETAIL_BLEND_MODE == 9 ) )" );
		AssertMsg( !( m_nBUMPMAP2 && m_nWARPLIGHTING ), "Invalid combo combination ( BUMPMAP2 && WARPLIGHTING )" );
		AssertMsg( !( m_nWARPLIGHTING && m_nDETAILTEXTURE ), "Invalid combo combination ( WARPLIGHTING && DETAILTEXTURE )" );
		AssertMsg( !( m_nENVMAPMASK && m_nBUMPMAP ), "Invalid combo combination ( ENVMAPMASK && BUMPMAP )" );
		AssertMsg( !( m_nNORMALMAPALPHAENVMAPMASK && m_nBASEALPHAENVMAPMASK ), "Invalid combo combination ( NORMALMAPALPHAENVMAPMASK && BASEALPHAENVMAPMASK )" );
		AssertMsg( !( m_nNORMALMAPALPHAENVMAPMASK && m_nENVMAPMASK ), "Invalid combo combination ( NORMALMAPALPHAENVMAPMASK && ENVMAPMASK )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nENVMAPMASK ), "Invalid combo combination ( BASEALPHAENVMAPMASK && ENVMAPMASK )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nSELFILLUM ), "Invalid combo combination ( BASEALPHAENVMAPMASK && SELFILLUM )" );
		AssertMsg( !( !m_nBUMPMAP && m_nDIFFUSEBUMPMAP ), "Invalid combo combination ( !BUMPMAP && DIFFUSEBUMPMAP )" );
		AssertMsg( !( !m_nBUMPMAP && m_nBUMPMAP2 ), "Invalid combo combination ( !BUMPMAP && BUMPMAP2 )" );
		AssertMsg( !( m_nENVMAPMASK && m_nBUMPMAP2 ), "Invalid combo combination ( ENVMAPMASK && BUMPMAP2 )" );
		AssertMsg( !( m_nBASETEXTURENOENVMAP && ( !m_nBASETEXTURE2 || !m_nCUBEMAP ) ), "Invalid combo combination ( BASETEXTURENOENVMAP && ( !BASETEXTURE2 || !CUBEMAP ) )" );
		AssertMsg( !( m_nBASETEXTURE2NOENVMAP && ( !m_nBASETEXTURE2 || !m_nCUBEMAP ) ), "Invalid combo combination ( BASETEXTURE2NOENVMAP && ( !BASETEXTURE2 || !CUBEMAP ) )" );
		AssertMsg( !( m_nBASEALPHAENVMAPMASK && m_nBUMPMAP ), "Invalid combo combination ( BASEALPHAENVMAPMASK && BUMPMAP )" );
		AssertMsg( !( m_nSEAMLESS && m_nRELIEF_MAPPING ), "Invalid combo combination ( SEAMLESS && RELIEF_MAPPING )" );
		AssertMsg( !( m_nSEAMLESS && m_nDETAILTEXTURE ), "Invalid combo combination ( SEAMLESS && DETAILTEXTURE )" );
		AssertMsg( !( m_nSEAMLESS && m_nMASKEDBLENDING ), "Invalid combo combination ( SEAMLESS && MASKEDBLENDING )" );
		AssertMsg( !( m_nBUMPMASK && ( m_nSEAMLESS || ( m_nDETAILTEXTURE || ( m_nSELFILLUM || ( m_nBASETEXTURENOENVMAP || m_nBASETEXTURE2 ) ) ) ) ), "Invalid combo combination ( BUMPMASK && ( SEAMLESS || ( DETAILTEXTURE || ( SELFILLUM || ( BASETEXTURENOENVMAP || BASETEXTURE2 ) ) ) ) )" );
		AssertMsg( !( !m_nBUMPMAP && ( m_nNORMAL_DECODE_MODE == 1 ) ), "Invalid combo combination ( !BUMPMAP && ( NORMAL_DECODE_MODE == 1 ) )" );
		AssertMsg( !( !m_nBUMPMAP && ( m_nNORMAL_DECODE_MODE == 2 ) ), "Invalid combo combination ( !BUMPMAP && ( NORMAL_DECODE_MODE == 2 ) )" );
		AssertMsg( !( !m_nBUMPMAP && ( m_nNORMALMASK_DECODE_MODE == 1 ) ), "Invalid combo combination ( !BUMPMAP && ( NORMALMASK_DECODE_MODE == 1 ) )" );
		AssertMsg( !( !m_nBUMPMAP && ( m_nNORMALMASK_DECODE_MODE == 2 ) ), "Invalid combo combination ( !BUMPMAP && ( NORMALMASK_DECODE_MODE == 2 ) )" );
		return ( 288 * m_nMASKEDBLENDING ) + ( 576 * m_nBASETEXTURE2 ) + ( 1152 * m_nDETAILTEXTURE ) + ( 2304 * m_nBUMPMAP ) + ( 6912 * m_nBUMPMAP2 ) + ( 13824 * m_nCUBEMAP ) + ( 27648 * m_nENVMAPMASK ) + ( 55296 * m_nBASEALPHAENVMAPMASK ) + ( 110592 * m_nSELFILLUM ) + ( 221184 * m_nNORMALMAPALPHAENVMAPMASK ) + ( 442368 * m_nDIFFUSEBUMPMAP ) + ( 884736 * m_nBASETEXTURENOENVMAP ) + ( 1769472 * m_nBASETEXTURE2NOENVMAP ) + ( 3538944 * m_nWARPLIGHTING ) + ( 7077888 * m_nFANCY_BLENDING ) + ( 14155776 * m_nRELIEF_MAPPING ) + ( 14155776 * m_nSEAMLESS ) + ( 28311552 * m_nOUTLINE ) + ( 56623104 * m_nSOFTEDGES ) + ( 113246208 * m_nBUMPMASK ) + ( 226492416 * m_nNORMAL_DECODE_MODE ) + ( 226492416 * m_nNORMALMASK_DECODE_MODE ) + ( 226492416 * m_nDETAIL_BLEND_MODE ) + 0;
	}
};

#define shaderStaticTest_lightmappedgeneric_ps20b psh_forgot_to_set_static_MASKEDBLENDING + psh_forgot_to_set_static_BASETEXTURE2 + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_BUMPMAP + psh_forgot_to_set_static_BUMPMAP2 + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_ENVMAPMASK + psh_forgot_to_set_static_BASEALPHAENVMAPMASK + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_NORMALMAPALPHAENVMAPMASK + psh_forgot_to_set_static_DIFFUSEBUMPMAP + psh_forgot_to_set_static_BASETEXTURENOENVMAP + psh_forgot_to_set_static_BASETEXTURE2NOENVMAP + psh_forgot_to_set_static_WARPLIGHTING + psh_forgot_to_set_static_FANCY_BLENDING + psh_forgot_to_set_static_RELIEF_MAPPING + psh_forgot_to_set_static_SEAMLESS + psh_forgot_to_set_static_OUTLINE + psh_forgot_to_set_static_SOFTEDGES + psh_forgot_to_set_static_BUMPMASK + psh_forgot_to_set_static_NORMAL_DECODE_MODE + psh_forgot_to_set_static_NORMALMASK_DECODE_MODE + psh_forgot_to_set_static_DETAIL_BLEND_MODE


class lightmappedgeneric_ps20b_Dynamic_Index
{
	unsigned int m_nFASTPATHENVMAPCONTRAST : 2;
	unsigned int m_nFASTPATH : 2;
	unsigned int m_nWRITEWATERFOGTODESTALPHA : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nLIGHTING_PREVIEW : 2;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
	unsigned int m_nBICUBIC_LIGHTMAP : 2;
#ifdef _DEBUG
	bool m_bFASTPATHENVMAPCONTRAST : 1;
	bool m_bFASTPATH : 1;
	bool m_bWRITEWATERFOGTODESTALPHA : 1;
	bool m_bPIXELFOGTYPE : 1;
	bool m_bLIGHTING_PREVIEW : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
	bool m_bBICUBIC_LIGHTMAP : 1;
#endif	// _DEBUG
public:
	void SetFASTPATHENVMAPCONTRAST( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATHENVMAPCONTRAST = i;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = true;
#endif	// _DEBUG
	}

	void SetFASTPATH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH = i;
#ifdef _DEBUG
		m_bFASTPATH = true;
#endif	// _DEBUG
	}

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

	void SetLIGHTING_PREVIEW( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nLIGHTING_PREVIEW = i;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = true;
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

	void SetBICUBIC_LIGHTMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBICUBIC_LIGHTMAP = i;
#ifdef _DEBUG
		m_bBICUBIC_LIGHTMAP = true;
#endif	// _DEBUG
	}

	lightmappedgeneric_ps20b_Dynamic_Index(  )
	{
		m_nFASTPATHENVMAPCONTRAST = 0;
		m_nFASTPATH = 0;
		m_nWRITEWATERFOGTODESTALPHA = 0;
		m_nPIXELFOGTYPE = 0;
		m_nLIGHTING_PREVIEW = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
		m_nBICUBIC_LIGHTMAP = 0;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = false;
		m_bFASTPATH = false;
		m_bWRITEWATERFOGTODESTALPHA = false;
		m_bPIXELFOGTYPE = false;
		m_bLIGHTING_PREVIEW = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
		m_bBICUBIC_LIGHTMAP = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bFASTPATHENVMAPCONTRAST && m_bFASTPATH && m_bWRITEWATERFOGTODESTALPHA && m_bPIXELFOGTYPE && m_bLIGHTING_PREVIEW && m_bWRITE_DEPTH_TO_DESTALPHA && m_bBICUBIC_LIGHTMAP );
		AssertMsg( !( !m_nFASTPATH && m_nFASTPATHENVMAPCONTRAST ), "Invalid combo combination ( !FASTPATH && FASTPATHENVMAPCONTRAST )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( m_nLIGHTING_PREVIEW && m_nFASTPATHENVMAPCONTRAST ) ) ), "Invalid combo combination ( 1 && ( 1 && ( LIGHTING_PREVIEW && FASTPATHENVMAPCONTRAST ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( m_nLIGHTING_PREVIEW && m_nFASTPATH ) ) ), "Invalid combo combination ( 1 && ( 1 && ( LIGHTING_PREVIEW && FASTPATH ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( m_nLIGHTING_PREVIEW && m_nFASTPATHENVMAPCONTRAST ) ) ), "Invalid combo combination ( 1 && ( 1 && ( LIGHTING_PREVIEW && FASTPATHENVMAPCONTRAST ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( m_nLIGHTING_PREVIEW && m_nFASTPATH ) ) ), "Invalid combo combination ( 1 && ( 1 && ( LIGHTING_PREVIEW && FASTPATH ) ) )" );
		return ( 1 * m_nFASTPATHENVMAPCONTRAST ) + ( 2 * m_nFASTPATH ) + ( 4 * m_nWRITEWATERFOGTODESTALPHA ) + ( 8 * m_nPIXELFOGTYPE ) + ( 24 * m_nLIGHTING_PREVIEW ) + ( 72 * m_nWRITE_DEPTH_TO_DESTALPHA ) + ( 144 * m_nBICUBIC_LIGHTMAP ) + 0;
	}
};

#define shaderDynamicTest_lightmappedgeneric_ps20b psh_forgot_to_set_dynamic_FASTPATHENVMAPCONTRAST + psh_forgot_to_set_dynamic_FASTPATH + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_LIGHTING_PREVIEW + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA + psh_forgot_to_set_dynamic_BICUBIC_LIGHTMAP

