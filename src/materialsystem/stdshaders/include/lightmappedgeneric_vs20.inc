// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// !$BUMPMAP && $DIFFUSEBUMPMAP
// $SEAMLESS && $RELIEF_MAPPING
// $BUMPMASK && $RELIEF_MAPPING
// $BUMPMASK && $SEAMLESS
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH

#pragma once
#include "shaderlib/cshader.h"
class lightmappedgeneric_vs20_Static_Index
{
	unsigned int m_nENVMAP_MASK : 2;
	unsigned int m_nTANGENTSPACE : 2;
	unsigned int m_nBUMPMAP : 2;
	unsigned int m_nDIFFUSEBUMPMAP : 2;
	unsigned int m_nVERTEXCOLOR : 2;
	unsigned int m_nVERTEXALPHATEXBLENDFACTOR : 2;
	unsigned int m_nRELIEF_MAPPING : 1;
	unsigned int m_nSEAMLESS : 2;
	unsigned int m_nBUMPMASK : 2;
#ifdef _DEBUG
	bool m_bENVMAP_MASK : 1;
	bool m_bTANGENTSPACE : 1;
	bool m_bBUMPMAP : 1;
	bool m_bDIFFUSEBUMPMAP : 1;
	bool m_bVERTEXCOLOR : 1;
	bool m_bVERTEXALPHATEXBLENDFACTOR : 1;
	bool m_bRELIEF_MAPPING : 1;
	bool m_bSEAMLESS : 1;
	bool m_bBUMPMASK : 1;
#endif	// _DEBUG
public:
	void SetENVMAP_MASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAP_MASK = i;
#ifdef _DEBUG
		m_bENVMAP_MASK = true;
#endif	// _DEBUG
	}

	void SetTANGENTSPACE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTANGENTSPACE = i;
#ifdef _DEBUG
		m_bTANGENTSPACE = true;
#endif	// _DEBUG
	}

	void SetBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP = i;
#ifdef _DEBUG
		m_bBUMPMAP = true;
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

	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif	// _DEBUG
	}

	void SetVERTEXALPHATEXBLENDFACTOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXALPHATEXBLENDFACTOR = i;
#ifdef _DEBUG
		m_bVERTEXALPHATEXBLENDFACTOR = true;
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

	void SetBUMPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMASK = i;
#ifdef _DEBUG
		m_bBUMPMASK = true;
#endif	// _DEBUG
	}

	lightmappedgeneric_vs20_Static_Index(  )
	{
		m_nENVMAP_MASK = 0;
		m_nTANGENTSPACE = 0;
		m_nBUMPMAP = 0;
		m_nDIFFUSEBUMPMAP = 0;
		m_nVERTEXCOLOR = 0;
		m_nVERTEXALPHATEXBLENDFACTOR = 0;
		m_nRELIEF_MAPPING = 0;
		m_nSEAMLESS = 0;
		m_nBUMPMASK = 0;
#ifdef _DEBUG
		m_bENVMAP_MASK = false;
		m_bTANGENTSPACE = false;
		m_bBUMPMAP = false;
		m_bDIFFUSEBUMPMAP = false;
		m_bVERTEXCOLOR = false;
		m_bVERTEXALPHATEXBLENDFACTOR = false;
		m_bRELIEF_MAPPING = false;
		m_bSEAMLESS = false;
		m_bBUMPMASK = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bENVMAP_MASK && m_bTANGENTSPACE && m_bBUMPMAP && m_bDIFFUSEBUMPMAP && m_bVERTEXCOLOR && m_bVERTEXALPHATEXBLENDFACTOR && m_bRELIEF_MAPPING && m_bSEAMLESS && m_bBUMPMASK );
		AssertMsg( !( !m_nBUMPMAP && m_nDIFFUSEBUMPMAP ), "Invalid combo combination ( !BUMPMAP && DIFFUSEBUMPMAP )" );
		AssertMsg( !( m_nSEAMLESS && m_nRELIEF_MAPPING ), "Invalid combo combination ( SEAMLESS && RELIEF_MAPPING )" );
		AssertMsg( !( m_nBUMPMASK && m_nRELIEF_MAPPING ), "Invalid combo combination ( BUMPMASK && RELIEF_MAPPING )" );
		AssertMsg( !( m_nBUMPMASK && m_nSEAMLESS ), "Invalid combo combination ( BUMPMASK && SEAMLESS )" );
		return ( 8 * m_nENVMAP_MASK ) + ( 16 * m_nTANGENTSPACE ) + ( 32 * m_nBUMPMAP ) + ( 64 * m_nDIFFUSEBUMPMAP ) + ( 128 * m_nVERTEXCOLOR ) + ( 256 * m_nVERTEXALPHATEXBLENDFACTOR ) + ( 512 * m_nRELIEF_MAPPING ) + ( 512 * m_nSEAMLESS ) + ( 1024 * m_nBUMPMASK ) + 0;
	}
};

#define shaderStaticTest_lightmappedgeneric_vs20 vsh_forgot_to_set_static_ENVMAP_MASK + vsh_forgot_to_set_static_TANGENTSPACE + vsh_forgot_to_set_static_BUMPMAP + vsh_forgot_to_set_static_DIFFUSEBUMPMAP + vsh_forgot_to_set_static_VERTEXCOLOR + vsh_forgot_to_set_static_VERTEXALPHATEXBLENDFACTOR + vsh_forgot_to_set_static_RELIEF_MAPPING + vsh_forgot_to_set_static_SEAMLESS + vsh_forgot_to_set_static_BUMPMASK


class lightmappedgeneric_vs20_Dynamic_Index
{
	unsigned int m_nFASTPATH : 2;
	unsigned int m_nDOWATERFOG : 2;
	unsigned int m_nLIGHTING_PREVIEW : 2;
#ifdef _DEBUG
	bool m_bFASTPATH : 1;
	bool m_bDOWATERFOG : 1;
	bool m_bLIGHTING_PREVIEW : 1;
#endif	// _DEBUG
public:
	void SetFASTPATH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH = i;
#ifdef _DEBUG
		m_bFASTPATH = true;
#endif	// _DEBUG
	}

	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif	// _DEBUG
	}

	void SetLIGHTING_PREVIEW( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLIGHTING_PREVIEW = i;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = true;
#endif	// _DEBUG
	}

	lightmappedgeneric_vs20_Dynamic_Index(  )
	{
		m_nFASTPATH = 0;
		m_nDOWATERFOG = 0;
		m_nLIGHTING_PREVIEW = 0;
#ifdef _DEBUG
		m_bFASTPATH = false;
		m_bDOWATERFOG = false;
		m_bLIGHTING_PREVIEW = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bFASTPATH && m_bDOWATERFOG && m_bLIGHTING_PREVIEW );
		AssertMsg( !( 1 && ( 1 && ( m_nLIGHTING_PREVIEW && m_nFASTPATH ) ) ), "Invalid combo combination ( 1 && ( 1 && ( LIGHTING_PREVIEW && FASTPATH ) ) )" );
		return ( 1 * m_nFASTPATH ) + ( 2 * m_nDOWATERFOG ) + ( 4 * m_nLIGHTING_PREVIEW ) + 0;
	}
};

#define shaderDynamicTest_lightmappedgeneric_vs20 vsh_forgot_to_set_dynamic_FASTPATH + vsh_forgot_to_set_dynamic_DOWATERFOG + vsh_forgot_to_set_dynamic_LIGHTING_PREVIEW

