#include "shaderlib/cshader.h"
class spritecard_ps20_Static_Index
{
private:
	int m_nDUALSEQUENCE;
#ifdef _DEBUG
	bool m_bDUALSEQUENCE;
#endif
public:
	void SetDUALSEQUENCE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDUALSEQUENCE = i;
#ifdef _DEBUG
		m_bDUALSEQUENCE = true;
#endif
	}
	void SetDUALSEQUENCE( bool i )
	{
		m_nDUALSEQUENCE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDUALSEQUENCE = true;
#endif
	}
private:
	int m_nSEQUENCE_BLEND_MODE;
#ifdef _DEBUG
	bool m_bSEQUENCE_BLEND_MODE;
#endif
public:
	void SetSEQUENCE_BLEND_MODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nSEQUENCE_BLEND_MODE = i;
#ifdef _DEBUG
		m_bSEQUENCE_BLEND_MODE = true;
#endif
	}
	void SetSEQUENCE_BLEND_MODE( bool i )
	{
		m_nSEQUENCE_BLEND_MODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bSEQUENCE_BLEND_MODE = true;
#endif
	}
private:
	int m_nADDBASETEXTURE2;
#ifdef _DEBUG
	bool m_bADDBASETEXTURE2;
#endif
public:
	void SetADDBASETEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDBASETEXTURE2 = i;
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = true;
#endif
	}
	void SetADDBASETEXTURE2( bool i )
	{
		m_nADDBASETEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = true;
#endif
	}
private:
	int m_nMAXLUMFRAMEBLEND1;
#ifdef _DEBUG
	bool m_bMAXLUMFRAMEBLEND1;
#endif
public:
	void SetMAXLUMFRAMEBLEND1( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMAXLUMFRAMEBLEND1 = i;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND1 = true;
#endif
	}
	void SetMAXLUMFRAMEBLEND1( bool i )
	{
		m_nMAXLUMFRAMEBLEND1 = i ? 1 : 0;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND1 = true;
#endif
	}
private:
	int m_nMAXLUMFRAMEBLEND2;
#ifdef _DEBUG
	bool m_bMAXLUMFRAMEBLEND2;
#endif
public:
	void SetMAXLUMFRAMEBLEND2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMAXLUMFRAMEBLEND2 = i;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND2 = true;
#endif
	}
	void SetMAXLUMFRAMEBLEND2( bool i )
	{
		m_nMAXLUMFRAMEBLEND2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND2 = true;
#endif
	}
private:
	int m_nEXTRACTGREENALPHA;
#ifdef _DEBUG
	bool m_bEXTRACTGREENALPHA;
#endif
public:
	void SetEXTRACTGREENALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nEXTRACTGREENALPHA = i;
#ifdef _DEBUG
		m_bEXTRACTGREENALPHA = true;
#endif
	}
	void SetEXTRACTGREENALPHA( bool i )
	{
		m_nEXTRACTGREENALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bEXTRACTGREENALPHA = true;
#endif
	}
private:
	int m_nCOLORRAMP;
#ifdef _DEBUG
	bool m_bCOLORRAMP;
#endif
public:
	void SetCOLORRAMP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOLORRAMP = i;
#ifdef _DEBUG
		m_bCOLORRAMP = true;
#endif
	}
	void SetCOLORRAMP( bool i )
	{
		m_nCOLORRAMP = i ? 1 : 0;
#ifdef _DEBUG
		m_bCOLORRAMP = true;
#endif
	}
private:
	int m_nANIMBLEND;
#ifdef _DEBUG
	bool m_bANIMBLEND;
#endif
public:
	void SetANIMBLEND( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nANIMBLEND = i;
#ifdef _DEBUG
		m_bANIMBLEND = true;
#endif
	}
	void SetANIMBLEND( bool i )
	{
		m_nANIMBLEND = i ? 1 : 0;
#ifdef _DEBUG
		m_bANIMBLEND = true;
#endif
	}
private:
	int m_nADDSELF;
#ifdef _DEBUG
	bool m_bADDSELF;
#endif
public:
	void SetADDSELF( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nADDSELF = i;
#ifdef _DEBUG
		m_bADDSELF = true;
#endif
	}
	void SetADDSELF( bool i )
	{
		m_nADDSELF = i ? 1 : 0;
#ifdef _DEBUG
		m_bADDSELF = true;
#endif
	}
public:
	spritecard_ps20_Static_Index( )
	{
#ifdef _DEBUG
		m_bDUALSEQUENCE = false;
#endif // _DEBUG
		m_nDUALSEQUENCE = 0;
#ifdef _DEBUG
		m_bSEQUENCE_BLEND_MODE = false;
#endif // _DEBUG
		m_nSEQUENCE_BLEND_MODE = 0;
#ifdef _DEBUG
		m_bADDBASETEXTURE2 = false;
#endif // _DEBUG
		m_nADDBASETEXTURE2 = 0;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND1 = false;
#endif // _DEBUG
		m_nMAXLUMFRAMEBLEND1 = 0;
#ifdef _DEBUG
		m_bMAXLUMFRAMEBLEND2 = false;
#endif // _DEBUG
		m_nMAXLUMFRAMEBLEND2 = 0;
#ifdef _DEBUG
		m_bEXTRACTGREENALPHA = false;
#endif // _DEBUG
		m_nEXTRACTGREENALPHA = 0;
#ifdef _DEBUG
		m_bCOLORRAMP = false;
#endif // _DEBUG
		m_nCOLORRAMP = 0;
#ifdef _DEBUG
		m_bANIMBLEND = false;
#endif // _DEBUG
		m_nANIMBLEND = 0;
#ifdef _DEBUG
		m_bADDSELF = false;
#endif // _DEBUG
		m_nADDSELF = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bDUALSEQUENCE && m_bSEQUENCE_BLEND_MODE && m_bADDBASETEXTURE2 && m_bMAXLUMFRAMEBLEND1 && m_bMAXLUMFRAMEBLEND2 && m_bEXTRACTGREENALPHA && m_bCOLORRAMP && m_bANIMBLEND && m_bADDSELF;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDUALSEQUENCE ) + ( 2 * m_nSEQUENCE_BLEND_MODE ) + ( 6 * m_nADDBASETEXTURE2 ) + ( 12 * m_nMAXLUMFRAMEBLEND1 ) + ( 24 * m_nMAXLUMFRAMEBLEND2 ) + ( 48 * m_nEXTRACTGREENALPHA ) + ( 96 * m_nCOLORRAMP ) + ( 192 * m_nANIMBLEND ) + ( 384 * m_nADDSELF ) + 0;
	}
};
#define shaderStaticTest_spritecard_ps20 psh_forgot_to_set_static_DUALSEQUENCE + psh_forgot_to_set_static_SEQUENCE_BLEND_MODE + psh_forgot_to_set_static_ADDBASETEXTURE2 + psh_forgot_to_set_static_MAXLUMFRAMEBLEND1 + psh_forgot_to_set_static_MAXLUMFRAMEBLEND2 + psh_forgot_to_set_static_EXTRACTGREENALPHA + psh_forgot_to_set_static_COLORRAMP + psh_forgot_to_set_static_ANIMBLEND + psh_forgot_to_set_static_ADDSELF + 0
class spritecard_ps20_Dynamic_Index
{
public:
	spritecard_ps20_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_spritecard_ps20 0
