#include "shaderlib/cshader.h"
class sdk_lightmappedgeneric_flashlight_vs20_Static_Index
{
private:
	int m_nNORMALMAP;
#ifdef _DEBUG
	bool m_bNORMALMAP;
#endif
public:
	void SetNORMALMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALMAP = i;
#ifdef _DEBUG
		m_bNORMALMAP = true;
#endif
	}
	void SetNORMALMAP( bool i )
	{
		m_nNORMALMAP = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMALMAP = true;
#endif
	}
private:
	int m_nWORLDVERTEXTRANSITION;
#ifdef _DEBUG
	bool m_bWORLDVERTEXTRANSITION;
#endif
public:
	void SetWORLDVERTEXTRANSITION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWORLDVERTEXTRANSITION = i;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
#endif
	}
	void SetWORLDVERTEXTRANSITION( bool i )
	{
		m_nWORLDVERTEXTRANSITION = i ? 1 : 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
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
	int m_nDETAIL;
#ifdef _DEBUG
	bool m_bDETAIL;
#endif
public:
	void SetDETAIL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAIL = i;
#ifdef _DEBUG
		m_bDETAIL = true;
#endif
	}
	void SetDETAIL( bool i )
	{
		m_nDETAIL = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAIL = true;
#endif
	}
private:
	int m_nPHONG;
#ifdef _DEBUG
	bool m_bPHONG;
#endif
public:
	void SetPHONG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONG = i;
#ifdef _DEBUG
		m_bPHONG = true;
#endif
	}
	void SetPHONG( bool i )
	{
		m_nPHONG = i ? 1 : 0;
#ifdef _DEBUG
		m_bPHONG = true;
#endif
	}
private:
	int m_nBASETEXTURETRANSFORM2;
#ifdef _DEBUG
	bool m_bBASETEXTURETRANSFORM2;
#endif
public:
	void SetBASETEXTURETRANSFORM2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURETRANSFORM2 = i;
#ifdef _DEBUG
		m_bBASETEXTURETRANSFORM2 = true;
#endif
	}
	void SetBASETEXTURETRANSFORM2( bool i )
	{
		m_nBASETEXTURETRANSFORM2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURETRANSFORM2 = true;
#endif
	}
public:
	sdk_lightmappedgeneric_flashlight_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bNORMALMAP = false;
#endif // _DEBUG
		m_nNORMALMAP = 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = false;
#endif // _DEBUG
		m_nWORLDVERTEXTRANSITION = 0;
#ifdef _DEBUG
		m_bSEAMLESS = false;
#endif // _DEBUG
		m_nSEAMLESS = 0;
#ifdef _DEBUG
		m_bDETAIL = false;
#endif // _DEBUG
		m_nDETAIL = 0;
#ifdef _DEBUG
		m_bPHONG = false;
#endif // _DEBUG
		m_nPHONG = 0;
#ifdef _DEBUG
		m_bBASETEXTURETRANSFORM2 = false;
#endif // _DEBUG
		m_nBASETEXTURETRANSFORM2 = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bNORMALMAP && m_bWORLDVERTEXTRANSITION && m_bSEAMLESS && m_bDETAIL && m_bPHONG && m_bBASETEXTURETRANSFORM2;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nNORMALMAP ) + ( 4 * m_nWORLDVERTEXTRANSITION ) + ( 8 * m_nSEAMLESS ) + ( 16 * m_nDETAIL ) + ( 32 * m_nPHONG ) + ( 64 * m_nBASETEXTURETRANSFORM2 ) + 0;
	}
};
#define shaderStaticTest_sdk_lightmappedgeneric_flashlight_vs20 vsh_forgot_to_set_static_NORMALMAP + vsh_forgot_to_set_static_WORLDVERTEXTRANSITION + vsh_forgot_to_set_static_SEAMLESS + vsh_forgot_to_set_static_DETAIL + vsh_forgot_to_set_static_PHONG + vsh_forgot_to_set_static_BASETEXTURETRANSFORM2 + 0
class sdk_lightmappedgeneric_flashlight_vs20_Dynamic_Index
{
private:
	int m_nDOWATERFOG;
#ifdef _DEBUG
	bool m_bDOWATERFOG;
#endif
public:
	void SetDOWATERFOG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDOWATERFOG = i;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif
	}
	void SetDOWATERFOG( bool i )
	{
		m_nDOWATERFOG = i ? 1 : 0;
#ifdef _DEBUG
		m_bDOWATERFOG = true;
#endif
	}
public:
	sdk_lightmappedgeneric_flashlight_vs20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bDOWATERFOG = false;
#endif // _DEBUG
		m_nDOWATERFOG = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bDOWATERFOG;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDOWATERFOG ) + 0;
	}
};
#define shaderDynamicTest_sdk_lightmappedgeneric_flashlight_vs20 vsh_forgot_to_set_dynamic_DOWATERFOG + 0
