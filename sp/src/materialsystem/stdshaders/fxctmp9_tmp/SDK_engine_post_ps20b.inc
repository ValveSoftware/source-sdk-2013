#include "shaderlib/cshader.h"
class sdk_engine_post_ps20b_Static_Index
{
private:
	int m_nTOOL_MODE;
#ifdef _DEBUG
	bool m_bTOOL_MODE;
#endif
public:
	void SetTOOL_MODE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTOOL_MODE = i;
#ifdef _DEBUG
		m_bTOOL_MODE = true;
#endif
	}
	void SetTOOL_MODE( bool i )
	{
		m_nTOOL_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bTOOL_MODE = true;
#endif
	}
private:
	int m_nDEPTH_BLUR_ENABLE;
#ifdef _DEBUG
	bool m_bDEPTH_BLUR_ENABLE;
#endif
public:
	void SetDEPTH_BLUR_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDEPTH_BLUR_ENABLE = i;
#ifdef _DEBUG
		m_bDEPTH_BLUR_ENABLE = true;
#endif
	}
	void SetDEPTH_BLUR_ENABLE( bool i )
	{
		m_nDEPTH_BLUR_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDEPTH_BLUR_ENABLE = true;
#endif
	}
public:
	sdk_engine_post_ps20b_Static_Index( )
	{
#ifdef _DEBUG
		m_bTOOL_MODE = false;
#endif // _DEBUG
		m_nTOOL_MODE = 0;
#ifdef _DEBUG
		m_bDEPTH_BLUR_ENABLE = false;
#endif // _DEBUG
		m_nDEPTH_BLUR_ENABLE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bTOOL_MODE && m_bDEPTH_BLUR_ENABLE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2048 * m_nTOOL_MODE ) + ( 4096 * m_nDEPTH_BLUR_ENABLE ) + 0;
	}
};
#define shaderStaticTest_sdk_engine_post_ps20b psh_forgot_to_set_static_TOOL_MODE + psh_forgot_to_set_static_DEPTH_BLUR_ENABLE + 0
class sdk_engine_post_ps20b_Dynamic_Index
{
private:
	int m_nAA_ENABLE;
#ifdef _DEBUG
	bool m_bAA_ENABLE;
#endif
public:
	void SetAA_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nAA_ENABLE = i;
#ifdef _DEBUG
		m_bAA_ENABLE = true;
#endif
	}
	void SetAA_ENABLE( bool i )
	{
		m_nAA_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bAA_ENABLE = true;
#endif
	}
private:
	int m_nCOL_CORRECT_NUM_LOOKUPS;
#ifdef _DEBUG
	bool m_bCOL_CORRECT_NUM_LOOKUPS;
#endif
public:
	void SetCOL_CORRECT_NUM_LOOKUPS( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nCOL_CORRECT_NUM_LOOKUPS = i;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = true;
#endif
	}
	void SetCOL_CORRECT_NUM_LOOKUPS( bool i )
	{
		m_nCOL_CORRECT_NUM_LOOKUPS = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = true;
#endif
	}
private:
	int m_nCONVERT_FROM_LINEAR;
#ifdef _DEBUG
	bool m_bCONVERT_FROM_LINEAR;
#endif
public:
	void SetCONVERT_FROM_LINEAR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_FROM_LINEAR = i;
#ifdef _DEBUG
		m_bCONVERT_FROM_LINEAR = true;
#endif
	}
	void SetCONVERT_FROM_LINEAR( bool i )
	{
		m_nCONVERT_FROM_LINEAR = i ? 1 : 0;
#ifdef _DEBUG
		m_bCONVERT_FROM_LINEAR = true;
#endif
	}
private:
	int m_nCONVERT_TO_LINEAR;
#ifdef _DEBUG
	bool m_bCONVERT_TO_LINEAR;
#endif
public:
	void SetCONVERT_TO_LINEAR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCONVERT_TO_LINEAR = i;
#ifdef _DEBUG
		m_bCONVERT_TO_LINEAR = true;
#endif
	}
	void SetCONVERT_TO_LINEAR( bool i )
	{
		m_nCONVERT_TO_LINEAR = i ? 1 : 0;
#ifdef _DEBUG
		m_bCONVERT_TO_LINEAR = true;
#endif
	}
private:
	int m_nNOISE_ENABLE;
#ifdef _DEBUG
	bool m_bNOISE_ENABLE;
#endif
public:
	void SetNOISE_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNOISE_ENABLE = i;
#ifdef _DEBUG
		m_bNOISE_ENABLE = true;
#endif
	}
	void SetNOISE_ENABLE( bool i )
	{
		m_nNOISE_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bNOISE_ENABLE = true;
#endif
	}
private:
	int m_nVIGNETTE_ENABLE;
#ifdef _DEBUG
	bool m_bVIGNETTE_ENABLE;
#endif
public:
	void SetVIGNETTE_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVIGNETTE_ENABLE = i;
#ifdef _DEBUG
		m_bVIGNETTE_ENABLE = true;
#endif
	}
	void SetVIGNETTE_ENABLE( bool i )
	{
		m_nVIGNETTE_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bVIGNETTE_ENABLE = true;
#endif
	}
private:
	int m_nLOCAL_CONTRAST_ENABLE;
#ifdef _DEBUG
	bool m_bLOCAL_CONTRAST_ENABLE;
#endif
public:
	void SetLOCAL_CONTRAST_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nLOCAL_CONTRAST_ENABLE = i;
#ifdef _DEBUG
		m_bLOCAL_CONTRAST_ENABLE = true;
#endif
	}
	void SetLOCAL_CONTRAST_ENABLE( bool i )
	{
		m_nLOCAL_CONTRAST_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bLOCAL_CONTRAST_ENABLE = true;
#endif
	}
private:
	int m_nBLURRED_VIGNETTE_ENABLE;
#ifdef _DEBUG
	bool m_bBLURRED_VIGNETTE_ENABLE;
#endif
public:
	void SetBLURRED_VIGNETTE_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLURRED_VIGNETTE_ENABLE = i;
#ifdef _DEBUG
		m_bBLURRED_VIGNETTE_ENABLE = true;
#endif
	}
	void SetBLURRED_VIGNETTE_ENABLE( bool i )
	{
		m_nBLURRED_VIGNETTE_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bBLURRED_VIGNETTE_ENABLE = true;
#endif
	}
private:
	int m_nVOMIT_ENABLE;
#ifdef _DEBUG
	bool m_bVOMIT_ENABLE;
#endif
public:
	void SetVOMIT_ENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVOMIT_ENABLE = i;
#ifdef _DEBUG
		m_bVOMIT_ENABLE = true;
#endif
	}
	void SetVOMIT_ENABLE( bool i )
	{
		m_nVOMIT_ENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bVOMIT_ENABLE = true;
#endif
	}
private:
	int m_nTV_GAMMA;
#ifdef _DEBUG
	bool m_bTV_GAMMA;
#endif
public:
	void SetTV_GAMMA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTV_GAMMA = i;
#ifdef _DEBUG
		m_bTV_GAMMA = true;
#endif
	}
	void SetTV_GAMMA( bool i )
	{
		m_nTV_GAMMA = i ? 1 : 0;
#ifdef _DEBUG
		m_bTV_GAMMA = true;
#endif
	}
private:
	int m_nDESATURATEENABLE;
#ifdef _DEBUG
	bool m_bDESATURATEENABLE;
#endif
public:
	void SetDESATURATEENABLE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDESATURATEENABLE = i;
#ifdef _DEBUG
		m_bDESATURATEENABLE = true;
#endif
	}
	void SetDESATURATEENABLE( bool i )
	{
		m_nDESATURATEENABLE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDESATURATEENABLE = true;
#endif
	}
public:
	sdk_engine_post_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bAA_ENABLE = false;
#endif // _DEBUG
		m_nAA_ENABLE = 0;
#ifdef _DEBUG
		m_bCOL_CORRECT_NUM_LOOKUPS = false;
#endif // _DEBUG
		m_nCOL_CORRECT_NUM_LOOKUPS = 0;
#ifdef _DEBUG
		m_bCONVERT_FROM_LINEAR = false;
#endif // _DEBUG
		m_nCONVERT_FROM_LINEAR = 0;
#ifdef _DEBUG
		m_bCONVERT_TO_LINEAR = false;
#endif // _DEBUG
		m_nCONVERT_TO_LINEAR = 0;
#ifdef _DEBUG
		m_bNOISE_ENABLE = false;
#endif // _DEBUG
		m_nNOISE_ENABLE = 0;
#ifdef _DEBUG
		m_bVIGNETTE_ENABLE = false;
#endif // _DEBUG
		m_nVIGNETTE_ENABLE = 0;
#ifdef _DEBUG
		m_bLOCAL_CONTRAST_ENABLE = false;
#endif // _DEBUG
		m_nLOCAL_CONTRAST_ENABLE = 0;
#ifdef _DEBUG
		m_bBLURRED_VIGNETTE_ENABLE = false;
#endif // _DEBUG
		m_nBLURRED_VIGNETTE_ENABLE = 0;
#ifdef _DEBUG
		m_bVOMIT_ENABLE = false;
#endif // _DEBUG
		m_nVOMIT_ENABLE = 0;
#ifdef _DEBUG
		m_bTV_GAMMA = false;
#endif // _DEBUG
		m_nTV_GAMMA = 0;
#ifdef _DEBUG
		m_bDESATURATEENABLE = false;
#endif // _DEBUG
		m_nDESATURATEENABLE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bAA_ENABLE && m_bCOL_CORRECT_NUM_LOOKUPS && m_bCONVERT_FROM_LINEAR && m_bCONVERT_TO_LINEAR && m_bNOISE_ENABLE && m_bVIGNETTE_ENABLE && m_bLOCAL_CONTRAST_ENABLE && m_bBLURRED_VIGNETTE_ENABLE && m_bVOMIT_ENABLE && m_bTV_GAMMA && m_bDESATURATEENABLE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nAA_ENABLE ) + ( 1 * m_nCOL_CORRECT_NUM_LOOKUPS ) + ( 4 * m_nCONVERT_FROM_LINEAR ) + ( 8 * m_nCONVERT_TO_LINEAR ) + ( 16 * m_nNOISE_ENABLE ) + ( 32 * m_nVIGNETTE_ENABLE ) + ( 64 * m_nLOCAL_CONTRAST_ENABLE ) + ( 128 * m_nBLURRED_VIGNETTE_ENABLE ) + ( 256 * m_nVOMIT_ENABLE ) + ( 512 * m_nTV_GAMMA ) + ( 1024 * m_nDESATURATEENABLE ) + 0;
	}
};
#define shaderDynamicTest_sdk_engine_post_ps20b psh_forgot_to_set_dynamic_AA_ENABLE + psh_forgot_to_set_dynamic_COL_CORRECT_NUM_LOOKUPS + psh_forgot_to_set_dynamic_CONVERT_FROM_LINEAR + psh_forgot_to_set_dynamic_CONVERT_TO_LINEAR + psh_forgot_to_set_dynamic_NOISE_ENABLE + psh_forgot_to_set_dynamic_VIGNETTE_ENABLE + psh_forgot_to_set_dynamic_LOCAL_CONTRAST_ENABLE + psh_forgot_to_set_dynamic_BLURRED_VIGNETTE_ENABLE + psh_forgot_to_set_dynamic_VOMIT_ENABLE + psh_forgot_to_set_dynamic_TV_GAMMA + psh_forgot_to_set_dynamic_DESATURATEENABLE + 0
