class skin_ps20_Static_Index
{
private:
	int m_nBASETEXTURE;
#ifdef _DEBUG
	bool m_bBASETEXTURE;
#endif
public:
	void SetBASETEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBASETEXTURE = i;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
	void SetBASETEXTURE( bool i )
	{
		m_nBASETEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bBASETEXTURE = true;
#endif
	}
private:
	int m_nBUMPTEXTURE;
#ifdef _DEBUG
	bool m_bBUMPTEXTURE;
#endif
public:
	void SetBUMPTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPTEXTURE = i;
#ifdef _DEBUG
		m_bBUMPTEXTURE = true;
#endif
	}
	void SetBUMPTEXTURE( bool i )
	{
		m_nBUMPTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPTEXTURE = true;
#endif
	}
private:
	int m_nDIFFUSELIGHTING;
#ifdef _DEBUG
	bool m_bDIFFUSELIGHTING;
#endif
public:
	void SetDIFFUSELIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDIFFUSELIGHTING = i;
#ifdef _DEBUG
		m_bDIFFUSELIGHTING = true;
#endif
	}
	void SetDIFFUSELIGHTING( bool i )
	{
		m_nDIFFUSELIGHTING = i ? 1 : 0;
#ifdef _DEBUG
		m_bDIFFUSELIGHTING = true;
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
	int m_nHALFLAMBERT;
#ifdef _DEBUG
	bool m_bHALFLAMBERT;
#endif
public:
	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif
	}
	void SetHALFLAMBERT( bool i )
	{
		m_nHALFLAMBERT = i ? 1 : 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
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
public:
	skin_ps20_Static_Index()
	{
#ifdef _DEBUG
		m_bBASETEXTURE = false;
#endif // _DEBUG
		m_nBASETEXTURE = 0;
#ifdef _DEBUG
		m_bBUMPTEXTURE = false;
#endif // _DEBUG
		m_nBUMPTEXTURE = 0;
#ifdef _DEBUG
		m_bDIFFUSELIGHTING = false;
#endif // _DEBUG
		m_nDIFFUSELIGHTING = 0;
#ifdef _DEBUG
		m_bSELFILLUM = false;
#endif // _DEBUG
		m_nSELFILLUM = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bFLASHLIGHT = false;
#endif // _DEBUG
		m_nFLASHLIGHT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bBASETEXTURE && m_bBUMPTEXTURE && m_bDIFFUSELIGHTING && m_bSELFILLUM && m_bHALFLAMBERT && m_bFLASHLIGHT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 132 * m_nBASETEXTURE ) + ( 264 * m_nBUMPTEXTURE ) + ( 528 * m_nDIFFUSELIGHTING ) + ( 1056 * m_nSELFILLUM ) + ( 2112 * m_nHALFLAMBERT ) + ( 4224 * m_nFLASHLIGHT ) + 0;
	}
};
#define shaderStaticTest_skin_ps20 psh_forgot_to_set_static_BASETEXTURE + psh_forgot_to_set_static_BUMPTEXTURE + psh_forgot_to_set_static_DIFFUSELIGHTING + psh_forgot_to_set_static_SELFILLUM + psh_forgot_to_set_static_HALFLAMBERT + psh_forgot_to_set_static_FLASHLIGHT + 0
class skin_ps20_Dynamic_Index
{
private:
	int m_nLIGHT_COMBO;
#ifdef _DEBUG
	bool m_bLIGHT_COMBO;
#endif
public:
	void SetLIGHT_COMBO( int i )
	{
		Assert( i >= 0 && i <= 21 );
		m_nLIGHT_COMBO = i;
#ifdef _DEBUG
		m_bLIGHT_COMBO = true;
#endif
	}
	void SetLIGHT_COMBO( bool i )
	{
		m_nLIGHT_COMBO = i ? 1 : 0;
#ifdef _DEBUG
		m_bLIGHT_COMBO = true;
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
	int m_nFOGTYPE;
#ifdef _DEBUG
	bool m_bFOGTYPE;
#endif
public:
	void SetFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFOGTYPE = i;
#ifdef _DEBUG
		m_bFOGTYPE = true;
#endif
	}
	void SetFOGTYPE( bool i )
	{
		m_nFOGTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bFOGTYPE = true;
#endif
	}
public:
	skin_ps20_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bLIGHT_COMBO = false;
#endif // _DEBUG
		m_nLIGHT_COMBO = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif // _DEBUG
		m_nWRITEWATERFOGTODESTALPHA = 0;
#ifdef _DEBUG
		m_bFOGTYPE = false;
#endif // _DEBUG
		m_nFOGTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bLIGHT_COMBO && m_bWRITEWATERFOGTODESTALPHA && m_bFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nLIGHT_COMBO ) + ( 22 * m_nWRITEWATERFOGTODESTALPHA ) + ( 44 * m_nFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_skin_ps20 psh_forgot_to_set_dynamic_LIGHT_COMBO + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_FOGTYPE + 0
