#include "shaderlib/cshader.h"
class sdk_skin_ps20b_Static_Index
{
private:
	int m_nCONVERT_TO_SRGB;
#ifdef _DEBUG
	bool m_bCONVERT_TO_SRGB;
#endif
public:
	void SetCONVERT_TO_SRGB( int i )
	{
		Assert( i >= 0 && i <= 0 );
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
	int m_nSELFILLUMFRESNEL;
#ifdef _DEBUG
	bool m_bSELFILLUMFRESNEL;
#endif
public:
	void SetSELFILLUMFRESNEL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSELFILLUMFRESNEL = i;
#ifdef _DEBUG
		m_bSELFILLUMFRESNEL = true;
#endif
	}
	void SetSELFILLUMFRESNEL( bool i )
	{
		m_nSELFILLUMFRESNEL = i ? 1 : 0;
#ifdef _DEBUG
		m_bSELFILLUMFRESNEL = true;
#endif
	}
private:
	int m_nFLASHLIGHT;
#ifdef _DEBUG
	bool m_bFLASHLIGHT;
#endif
public:
	void SetFLASHLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHT = i;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
	void SetFLASHLIGHT( bool i )
	{
		m_nFLASHLIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = true;
#endif
	}
private:
	int m_nLIGHTWARPTEXTURE;
#ifdef _DEBUG
	bool m_bLIGHTWARPTEXTURE;
#endif
public:
	void SetLIGHTWARPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLIGHTWARPTEXTURE = i;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = true;
#endif
	}
	void SetLIGHTWARPTEXTURE( bool i )
	{
		m_nLIGHTWARPTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = true;
#endif
	}
private:
	int m_nPHONGWARPTEXTURE;
#ifdef _DEBUG
	bool m_bPHONGWARPTEXTURE;
#endif
public:
	void SetPHONGWARPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONGWARPTEXTURE = i;
#ifdef _DEBUG
		m_bPHONGWARPTEXTURE = true;
#endif
	}
	void SetPHONGWARPTEXTURE( bool i )
	{
		m_nPHONGWARPTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bPHONGWARPTEXTURE = true;
#endif
	}
private:
	int m_nWRINKLEMAP;
#ifdef _DEBUG
	bool m_bWRINKLEMAP;
#endif
public:
	void SetWRINKLEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRINKLEMAP = i;
#ifdef _DEBUG
		m_bWRINKLEMAP = true;
#endif
	}
	void SetWRINKLEMAP( bool i )
	{
		m_nWRINKLEMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bWRINKLEMAP = true;
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
		Assert( i >= 0 && i <= 6 );
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
	int m_nRIMLIGHT;
#ifdef _DEBUG
	bool m_bRIMLIGHT;
#endif
public:
	void SetRIMLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nRIMLIGHT = i;
#ifdef _DEBUG
		m_bRIMLIGHT = true;
#endif
	}
	void SetRIMLIGHT( bool i )
	{
		m_nRIMLIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bRIMLIGHT = true;
#endif
	}
private:
	int m_nFLASHLIGHTDEPTHFILTERMODE;
#ifdef _DEBUG
	bool m_bFLASHLIGHTDEPTHFILTERMODE;
#endif
public:
	void SetFLASHLIGHTDEPTHFILTERMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFLASHLIGHTDEPTHFILTERMODE = i;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
#endif
	}
	void SetFLASHLIGHTDEPTHFILTERMODE( bool i )
	{
		m_nFLASHLIGHTDEPTHFILTERMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
#endif
	}
private:
	int m_nFASTPATH_NOBUMP;
#ifdef _DEBUG
	bool m_bFASTPATH_NOBUMP;
#endif
public:
	void SetFASTPATH_NOBUMP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFASTPATH_NOBUMP = i;
#ifdef _DEBUG
		m_bFASTPATH_NOBUMP = true;
#endif
	}
	void SetFASTPATH_NOBUMP( bool i )
	{
		m_nFASTPATH_NOBUMP = i ? 1 : 0;
#ifdef _DEBUG
		m_bFASTPATH_NOBUMP = true;
#endif
	}
private:
	int m_nBLENDTINTBYBASEALPHA;
#ifdef _DEBUG
	bool m_bBLENDTINTBYBASEALPHA;
#endif
public:
	void SetBLENDTINTBYBASEALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLENDTINTBYBASEALPHA = i;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = true;
#endif
	}
	void SetBLENDTINTBYBASEALPHA( bool i )
	{
		m_nBLENDTINTBYBASEALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = true;
#endif
	}
private:
	int m_nPHONG_HALFLAMBERT;
#ifdef _DEBUG
	bool m_bPHONG_HALFLAMBERT;
#endif
public:
	void SetPHONG_HALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONG_HALFLAMBERT = i;
#ifdef _DEBUG
		m_bPHONG_HALFLAMBERT = true;
#endif
	}
	void SetPHONG_HALFLAMBERT( bool i )
	{
		m_nPHONG_HALFLAMBERT = i ? 1 : 0;
#ifdef _DEBUG
		m_bPHONG_HALFLAMBERT = true;
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
public:
	sdk_skin_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bCONVERT_TO_SRGB = false;
#endif // _DEBUG
		m_nCONVERT_TO_SRGB = 0;
#ifdef _DEBUG
		m_bCUBEMAP = false;
#endif // _DEBUG
		m_nCUBEMAP = 0;
#ifdef _DEBUG
		m_bSELFILLUM = false;
#endif // _DEBUG
		m_nSELFILLUM = 0;
#ifdef _DEBUG
		m_bSELFILLUMFRESNEL = false;
#endif // _DEBUG
		m_nSELFILLUMFRESNEL = 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = false;
#endif // _DEBUG
		m_nFLASHLIGHT = 0;
#ifdef _DEBUG
		m_bLIGHTWARPTEXTURE = false;
#endif // _DEBUG
		m_nLIGHTWARPTEXTURE = 0;
#ifdef _DEBUG
		m_bPHONGWARPTEXTURE = false;
#endif // _DEBUG
		m_nPHONGWARPTEXTURE = 0;
#ifdef _DEBUG
		m_bWRINKLEMAP = false;
#endif // _DEBUG
		m_nWRINKLEMAP = 0;
#ifdef _DEBUG
		m_bDETAIL_BLEND_MODE = false;
#endif // _DEBUG
		m_nDETAIL_BLEND_MODE = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bRIMLIGHT = false;
#endif // _DEBUG
		m_nRIMLIGHT = 0;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = false;
#endif // _DEBUG
		m_nFLASHLIGHTDEPTHFILTERMODE = 0;
#ifdef _DEBUG
		m_bFASTPATH_NOBUMP = false;
#endif // _DEBUG
		m_nFASTPATH_NOBUMP = 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = false;
#endif // _DEBUG
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bPHONG_HALFLAMBERT = false;
#endif // _DEBUG
		m_nPHONG_HALFLAMBERT = 0;
#ifdef _DEBUG
		m_bENVMAPMASK = false;
#endif // _DEBUG
		m_nENVMAPMASK = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bCONVERT_TO_SRGB && m_bCUBEMAP && m_bSELFILLUM && m_bSELFILLUMFRESNEL && m_bFLASHLIGHT && m_bLIGHTWARPTEXTURE && m_bPHONGWARPTEXTURE && m_bWRINKLEMAP && m_bDETAIL_BLEND_MODE && m_bDETAILTEXTURE && m_bRIMLIGHT && m_bFLASHLIGHTDEPTHFILTERMODE && m_bFASTPATH_NOBUMP && m_bBLENDTINTBYBASEALPHA && m_bPHONG_HALFLAMBERT && m_bENVMAPMASK;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 80 * m_nCONVERT_TO_SRGB ) + ( 80 * m_nCUBEMAP ) + ( 160 * m_nSELFILLUM ) + ( 320 * m_nSELFILLUMFRESNEL ) + ( 640 * m_nFLASHLIGHT ) + ( 1280 * m_nLIGHTWARPTEXTURE ) + ( 2560 * m_nPHONGWARPTEXTURE ) + ( 5120 * m_nWRINKLEMAP ) + ( 10240 * m_nDETAIL_BLEND_MODE ) + ( 71680 * m_nDETAILTEXTURE ) + ( 143360 * m_nRIMLIGHT ) + ( 286720 * m_nFLASHLIGHTDEPTHFILTERMODE ) + ( 860160 * m_nFASTPATH_NOBUMP ) + ( 1720320 * m_nBLENDTINTBYBASEALPHA ) + ( 3440640 * m_nPHONG_HALFLAMBERT ) + ( 6881280 * m_nENVMAPMASK ) + 0;
	}
};
#define shaderStaticTest_sdk_skin_ps20b psh_forgot_to_set_static_CONVERT_TO_SRGB + psh_forgot_to_set_static_CUBEMAP + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_SELFILLUMFRESNEL + psh_forgot_to_set_static_FLASHLIGHT + psh_forgot_to_set_static_LIGHTWARPTEXTURE + psh_forgot_to_set_static_PHONGWARPTEXTURE + psh_forgot_to_set_static_WRINKLEMAP + psh_forgot_to_set_static_DETAIL_BLEND_MODE + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_RIMLIGHT + psh_forgot_to_set_static_FLASHLIGHTDEPTHFILTERMODE + psh_forgot_to_set_static_FASTPATH_NOBUMP + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA + psh_forgot_to_set_static_PHONG_HALFLAMBERT + psh_forgot_to_set_static_ENVMAPMASK + 0
class sdk_skin_ps20b_Dynamic_Index
{
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
	int m_nNUM_LIGHTS;
#ifdef _DEBUG
	bool m_bNUM_LIGHTS;
#endif
public:
	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif
	}
	void SetNUM_LIGHTS( bool i )
	{
		m_nNUM_LIGHTS = i ? 1 : 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
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
private:
	int m_nFLASHLIGHTSHADOWS;
#ifdef _DEBUG
	bool m_bFLASHLIGHTSHADOWS;
#endif
public:
	void SetFLASHLIGHTSHADOWS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHTSHADOWS = i;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif
	}
	void SetFLASHLIGHTSHADOWS( bool i )
	{
		m_nFLASHLIGHTSHADOWS = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif
	}
public:
	sdk_skin_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif // _DEBUG
		m_nWRITEWATERFOGTODESTALPHA = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = false;
#endif // _DEBUG
		m_nNUM_LIGHTS = 0;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
#endif // _DEBUG
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = false;
#endif // _DEBUG
		m_nFLASHLIGHTSHADOWS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bWRITEWATERFOGTODESTALPHA && m_bPIXELFOGTYPE && m_bNUM_LIGHTS && m_bWRITE_DEPTH_TO_DESTALPHA && m_bFLASHLIGHTSHADOWS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nWRITEWATERFOGTODESTALPHA ) + ( 2 * m_nPIXELFOGTYPE ) + ( 4 * m_nNUM_LIGHTS ) + ( 20 * m_nWRITE_DEPTH_TO_DESTALPHA ) + ( 40 * m_nFLASHLIGHTSHADOWS ) + 0;
	}
};
#define shaderDynamicTest_sdk_skin_ps20b psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_NUM_LIGHTS + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA + psh_forgot_to_set_dynamic_FLASHLIGHTSHADOWS + 0
