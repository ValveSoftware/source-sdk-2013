#include "shaderlib/cshader.h"
class lightmappedgeneric_ps20_Static_Index
{
private:
	int m_nMASKEDBLENDING;
#ifdef _DEBUG
	bool m_bMASKEDBLENDING;
#endif
public:
	void SetMASKEDBLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMASKEDBLENDING = i;
#ifdef _DEBUG
		m_bMASKEDBLENDING = true;
#endif
	}
	void SetMASKEDBLENDING( bool i )
	{
		m_nMASKEDBLENDING = i ? 1 : 0;
#ifdef _DEBUG
		m_bMASKEDBLENDING = true;
#endif
	}
private:
	int m_nBASETEXTURE2;
#ifdef _DEBUG
	bool m_bBASETEXTURE2;
#endif
public:
	void SetBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
	void SetBASETEXTURE2( bool i )
	{
		m_nBASETEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = true;
#endif
	}
private:
	int m_nDETAILTEXTURE;
#ifdef _DEBUG
	bool m_bDETAILTEXTURE;
#endif
public:
	void SetDETAILTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif
	}
	void SetDETAILTEXTURE( bool i )
	{
		m_nDETAILTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif
	}
private:
	int m_nBUMPMAP;
#ifdef _DEBUG
	bool m_bBUMPMAP;
#endif
public:
	void SetBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nBUMPMAP = i;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif
	}
	void SetBUMPMAP( bool i )
	{
		m_nBUMPMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPMAP = true;
#endif
	}
private:
	int m_nBUMPMAP2;
#ifdef _DEBUG
	bool m_bBUMPMAP2;
#endif
public:
	void SetBUMPMAP2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAP2 = i;
#ifdef _DEBUG
		m_bBUMPMAP2 = true;
#endif
	}
	void SetBUMPMAP2( bool i )
	{
		m_nBUMPMAP2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPMAP2 = true;
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
	int m_nSELFILLUM;
#ifdef _DEBUG
	bool m_bSELFILLUM;
#endif
public:
	void SetSELFILLUM( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUM = i;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif
	}
	void SetSELFILLUM( bool i )
	{
		m_nSELFILLUM = i ? 1 : 0;
#ifdef _DEBUG
		m_bSELFILLUM = true;
#endif
	}
private:
	int m_nNORMALMAPALPHAENVMAPMASK;
#ifdef _DEBUG
	bool m_bNORMALMAPALPHAENVMAPMASK;
#endif
public:
	void SetNORMALMAPALPHAENVMAPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAPALPHAENVMAPMASK = i;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = true;
#endif
	}
	void SetNORMALMAPALPHAENVMAPMASK( bool i )
	{
		m_nNORMALMAPALPHAENVMAPMASK = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = true;
#endif
	}
private:
	int m_nDIFFUSEBUMPMAP;
#ifdef _DEBUG
	bool m_bDIFFUSEBUMPMAP;
#endif
public:
	void SetDIFFUSEBUMPMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDIFFUSEBUMPMAP = i;
#ifdef _DEBUG
		m_bDIFFUSEBUMPMAP = true;
#endif
	}
	void SetDIFFUSEBUMPMAP( bool i )
	{
		m_nDIFFUSEBUMPMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bDIFFUSEBUMPMAP = true;
#endif
	}
private:
	int m_nBASETEXTURENOENVMAP;
#ifdef _DEBUG
	bool m_bBASETEXTURENOENVMAP;
#endif
public:
	void SetBASETEXTURENOENVMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURENOENVMAP = i;
#ifdef _DEBUG
		m_bBASETEXTURENOENVMAP = true;
#endif
	}
	void SetBASETEXTURENOENVMAP( bool i )
	{
		m_nBASETEXTURENOENVMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURENOENVMAP = true;
#endif
	}
private:
	int m_nBASETEXTURE2NOENVMAP;
#ifdef _DEBUG
	bool m_bBASETEXTURE2NOENVMAP;
#endif
public:
	void SetBASETEXTURE2NOENVMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE2NOENVMAP = i;
#ifdef _DEBUG
		m_bBASETEXTURE2NOENVMAP = true;
#endif
	}
	void SetBASETEXTURE2NOENVMAP( bool i )
	{
		m_nBASETEXTURE2NOENVMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE2NOENVMAP = true;
#endif
	}
private:
	int m_nWARPLIGHTING;
#ifdef _DEBUG
	bool m_bWARPLIGHTING;
#endif
public:
	void SetWARPLIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWARPLIGHTING = i;
#ifdef _DEBUG
		m_bWARPLIGHTING = true;
#endif
	}
	void SetWARPLIGHTING( bool i )
	{
		m_nWARPLIGHTING = i ? 1 : 0;
#ifdef _DEBUG
		m_bWARPLIGHTING = true;
#endif
	}
private:
	int m_nFANCY_BLENDING;
#ifdef _DEBUG
	bool m_bFANCY_BLENDING;
#endif
public:
	void SetFANCY_BLENDING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFANCY_BLENDING = i;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif
	}
	void SetFANCY_BLENDING( bool i )
	{
		m_nFANCY_BLENDING = i ? 1 : 0;
#ifdef _DEBUG
		m_bFANCY_BLENDING = true;
#endif
	}
private:
	int m_nSEAMLESS;
#ifdef _DEBUG
	bool m_bSEAMLESS;
#endif
public:
	void SetSEAMLESS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS = i;
#ifdef _DEBUG
		m_bSEAMLESS = true;
#endif
	}
	void SetSEAMLESS( bool i )
	{
		m_nSEAMLESS = i ? 1 : 0;
#ifdef _DEBUG
		m_bSEAMLESS = true;
#endif
	}
private:
	int m_nOUTLINE;
#ifdef _DEBUG
	bool m_bOUTLINE;
#endif
public:
	void SetOUTLINE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nOUTLINE = i;
#ifdef _DEBUG
		m_bOUTLINE = true;
#endif
	}
	void SetOUTLINE( bool i )
	{
		m_nOUTLINE = i ? 1 : 0;
#ifdef _DEBUG
		m_bOUTLINE = true;
#endif
	}
private:
	int m_nSOFTEDGES;
#ifdef _DEBUG
	bool m_bSOFTEDGES;
#endif
public:
	void SetSOFTEDGES( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSOFTEDGES = i;
#ifdef _DEBUG
		m_bSOFTEDGES = true;
#endif
	}
	void SetSOFTEDGES( bool i )
	{
		m_nSOFTEDGES = i ? 1 : 0;
#ifdef _DEBUG
		m_bSOFTEDGES = true;
#endif
	}
private:
	int m_nBUMPMASK;
#ifdef _DEBUG
	bool m_bBUMPMASK;
#endif
public:
	void SetBUMPMASK( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMASK = i;
#ifdef _DEBUG
		m_bBUMPMASK = true;
#endif
	}
	void SetBUMPMASK( bool i )
	{
		m_nBUMPMASK = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPMASK = true;
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
	int m_nNORMALMASK_DECODE_MODE;
#ifdef _DEBUG
	bool m_bNORMALMASK_DECODE_MODE;
#endif
public:
	void SetNORMALMASK_DECODE_MODE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nNORMALMASK_DECODE_MODE = i;
#ifdef _DEBUG
		m_bNORMALMASK_DECODE_MODE = true;
#endif
	}
	void SetNORMALMASK_DECODE_MODE( bool i )
	{
		m_nNORMALMASK_DECODE_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMALMASK_DECODE_MODE = true;
#endif
	}
private:
	int m_nDETAIL_BLEND_MODE;
#ifdef _DEBUG
	bool m_bDETAIL_BLEND_MODE;
#endif
public:
	void SetDETAIL_BLEND_MODE( int i )
	{
		Assert( i >= 0 && i <= 11 );
		m_nDETAIL_BLEND_MODE = i;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = true;
#endif
	}
	void SetDETAIL_BLEND_MODE( bool i )
	{
		m_nDETAIL_BLEND_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = true;
#endif
	}
public:
	lightmappedgeneric_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bMASKEDBLENDING = false;
#endif // _DEBUG
		m_nMASKEDBLENDING = 0;
#ifdef _DEBUG
		m_bBASETEXTURE2 = false;
#endif // _DEBUG
		m_nBASETEXTURE2 = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bBUMPMAP = false;
#endif // _DEBUG
		m_nBUMPMAP = 0;
#ifdef _DEBUG
		m_bBUMPMAP2 = false;
#endif // _DEBUG
		m_nBUMPMAP2 = 0;
#ifdef _DEBUG
		m_bCUBEMAP = false;
#endif // _DEBUG
		m_nCUBEMAP = 0;
#ifdef _DEBUG
		m_bENVMAPMASK = false;
#endif // _DEBUG
		m_nENVMAPMASK = 0;
#ifdef _DEBUG
		m_bBASEALPHAENVMAPMASK = false;
#endif // _DEBUG
		m_nBASEALPHAENVMAPMASK = 0;
#ifdef _DEBUG
		m_bSELFILLUM = false;
#endif // _DEBUG
		m_nSELFILLUM = 0;
#ifdef _DEBUG
		m_bNORMALMAPALPHAENVMAPMASK = false;
#endif // _DEBUG
		m_nNORMALMAPALPHAENVMAPMASK = 0;
#ifdef _DEBUG
		m_bDIFFUSEBUMPMAP = false;
#endif // _DEBUG
		m_nDIFFUSEBUMPMAP = 0;
#ifdef _DEBUG
		m_bBASETEXTURENOENVMAP = false;
#endif // _DEBUG
		m_nBASETEXTURENOENVMAP = 0;
#ifdef _DEBUG
		m_bBASETEXTURE2NOENVMAP = false;
#endif // _DEBUG
		m_nBASETEXTURE2NOENVMAP = 0;
#ifdef _DEBUG
		m_bWARPLIGHTING = false;
#endif // _DEBUG
		m_nWARPLIGHTING = 0;
#ifdef _DEBUG
		m_bFANCY_BLENDING = false;
#endif // _DEBUG
		m_nFANCY_BLENDING = 0;
#ifdef _DEBUG
		m_bSEAMLESS = false;
#endif // _DEBUG
		m_nSEAMLESS = 0;
#ifdef _DEBUG
		m_bOUTLINE = false;
#endif // _DEBUG
		m_nOUTLINE = 0;
#ifdef _DEBUG
		m_bSOFTEDGES = false;
#endif // _DEBUG
		m_nSOFTEDGES = 0;
#ifdef _DEBUG
		m_bBUMPMASK = false;
#endif // _DEBUG
		m_nBUMPMASK = 0;
#ifdef _DEBUG
		m_bNORMAL_DECODE_MODE = false;
#endif // _DEBUG
		m_nNORMAL_DECODE_MODE = 0;
#ifdef _DEBUG
		m_bNORMALMASK_DECODE_MODE = false;
#endif // _DEBUG
		m_nNORMALMASK_DECODE_MODE = 0;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = false;
#endif // _DEBUG
		m_nDETAIL_BLEND_MODE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bMASKEDBLENDING && m_bBASETEXTURE2 && m_bDETAILTEXTURE && m_bBUMPMAP && m_bBUMPMAP2 && m_bCUBEMAP && m_bENVMAPMASK && m_bBASEALPHAENVMAPMASK && m_bSELFILLUM && m_bNORMALMAPALPHAENVMAPMASK && m_bDIFFUSEBUMPMAP && m_bBASETEXTURENOENVMAP && m_bBASETEXTURE2NOENVMAP && m_bWARPLIGHTING && m_bFANCY_BLENDING && m_bSEAMLESS && m_bOUTLINE && m_bSOFTEDGES && m_bBUMPMASK && m_bNORMAL_DECODE_MODE && m_bNORMALMASK_DECODE_MODE && m_bDETAIL_BLEND_MODE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 48 * m_nMASKEDBLENDING ) + ( 96 * m_nBASETEXTURE2 ) + ( 192 * m_nDETAILTEXTURE ) + ( 384 * m_nBUMPMAP ) + ( 1152 * m_nBUMPMAP2 ) + ( 2304 * m_nCUBEMAP ) + ( 4608 * m_nENVMAPMASK ) + ( 9216 * m_nBASEALPHAENVMAPMASK ) + ( 18432 * m_nSELFILLUM ) + ( 36864 * m_nNORMALMAPALPHAENVMAPMASK ) + ( 73728 * m_nDIFFUSEBUMPMAP ) + ( 147456 * m_nBASETEXTURENOENVMAP ) + ( 294912 * m_nBASETEXTURE2NOENVMAP ) + ( 589824 * m_nWARPLIGHTING ) + ( 1179648 * m_nFANCY_BLENDING ) + ( 2359296 * m_nSEAMLESS ) + ( 4718592 * m_nOUTLINE ) + ( 9437184 * m_nSOFTEDGES ) + ( 18874368 * m_nBUMPMASK ) + ( 37748736 * m_nNORMAL_DECODE_MODE ) + ( 37748736 * m_nNORMALMASK_DECODE_MODE ) + ( 37748736 * m_nDETAIL_BLEND_MODE ) + 0;
	}
};
#define shaderStaticTest_lightmappedgeneric_ps20 psh_forgot_to_set_static_MASKEDBLENDING + psh_forgot_to_set_static_BASETEXTURE2 + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_BUMPMAP + psh_forgot_to_set_static_BUMPMAP2 + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_ENVMAPMASK + psh_forgot_to_set_static_BASEALPHAENVMAPMASK + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_NORMALMAPALPHAENVMAPMASK + psh_forgot_to_set_static_DIFFUSEBUMPMAP + psh_forgot_to_set_static_BASETEXTURENOENVMAP + psh_forgot_to_set_static_BASETEXTURE2NOENVMAP + psh_forgot_to_set_static_WARPLIGHTING + psh_forgot_to_set_static_FANCY_BLENDING + psh_forgot_to_set_static_SEAMLESS + psh_forgot_to_set_static_OUTLINE + psh_forgot_to_set_static_SOFTEDGES + psh_forgot_to_set_static_BUMPMASK + psh_forgot_to_set_static_NORMAL_DECODE_MODE + psh_forgot_to_set_static_NORMALMASK_DECODE_MODE + psh_forgot_to_set_static_DETAIL_BLEND_MODE + 0
class lightmappedgeneric_ps20_Dynamic_Index
{
private:
	int m_nFASTPATHENVMAPCONTRAST;
#ifdef _DEBUG
	bool m_bFASTPATHENVMAPCONTRAST;
#endif
public:
	void SetFASTPATHENVMAPCONTRAST( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATHENVMAPCONTRAST = i;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = true;
#endif
	}
	void SetFASTPATHENVMAPCONTRAST( bool i )
	{
		m_nFASTPATHENVMAPCONTRAST = i ? 1 : 0;
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = true;
#endif
	}
private:
	int m_nFASTPATH;
#ifdef _DEBUG
	bool m_bFASTPATH;
#endif
public:
	void SetFASTPATH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH = i;
#ifdef _DEBUG
		m_bFASTPATH = true;
#endif
	}
	void SetFASTPATH( bool i )
	{
		m_nFASTPATH = i ? 1 : 0;
#ifdef _DEBUG
		m_bFASTPATH = true;
#endif
	}
private:
	int m_nWRITEWATERFOGTODESTALPHA;
#ifdef _DEBUG
	bool m_bWRITEWATERFOGTODESTALPHA;
#endif
public:
	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif
	}
	void SetWRITEWATERFOGTODESTALPHA( bool i )
	{
		m_nWRITEWATERFOGTODESTALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
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
private:
	int m_nLIGHTING_PREVIEW;
#ifdef _DEBUG
	bool m_bLIGHTING_PREVIEW;
#endif
public:
	void SetLIGHTING_PREVIEW( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nLIGHTING_PREVIEW = i;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = true;
#endif
	}
	void SetLIGHTING_PREVIEW( bool i )
	{
		m_nLIGHTING_PREVIEW = i ? 1 : 0;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = true;
#endif
	}
public:
	lightmappedgeneric_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bFASTPATHENVMAPCONTRAST = false;
#endif // _DEBUG
		m_nFASTPATHENVMAPCONTRAST = 0;
#ifdef _DEBUG
		m_bFASTPATH = false;
#endif // _DEBUG
		m_nFASTPATH = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif // _DEBUG
		m_nWRITEWATERFOGTODESTALPHA = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bLIGHTING_PREVIEW = false;
#endif // _DEBUG
		m_nLIGHTING_PREVIEW = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bFASTPATHENVMAPCONTRAST && m_bFASTPATH && m_bWRITEWATERFOGTODESTALPHA && m_bPIXELFOGTYPE && m_bLIGHTING_PREVIEW;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nFASTPATHENVMAPCONTRAST ) + ( 2 * m_nFASTPATH ) + ( 4 * m_nWRITEWATERFOGTODESTALPHA ) + ( 8 * m_nPIXELFOGTYPE ) + ( 16 * m_nLIGHTING_PREVIEW ) + 0;
	}
};
#define shaderDynamicTest_lightmappedgeneric_ps20 psh_forgot_to_set_dynamic_FASTPATHENVMAPCONTRAST + psh_forgot_to_set_dynamic_FASTPATH + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_LIGHTING_PREVIEW + 0
