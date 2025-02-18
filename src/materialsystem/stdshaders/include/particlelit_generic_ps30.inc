class particlelit_generic_ps30_Static_Index
{
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
	int m_nHDRTYPE;
#ifdef _DEBUG
	bool m_bHDRTYPE;
#endif
public:
	void SetHDRTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nHDRTYPE = i;
#ifdef _DEBUG
		m_bHDRTYPE = true;
#endif
	}
	void SetHDRTYPE( bool i )
	{
		m_nHDRTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bHDRTYPE = true;
#endif
	}
public:
	particlelit_generic_ps30_Static_Index()
	{
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bHDRTYPE = false;
#endif // _DEBUG
		m_nHDRTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bHALFLAMBERT && m_bHDRTYPE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 264 * m_nHALFLAMBERT ) + ( 528 * m_nHDRTYPE ) + 0;
	}
};
#define shaderStaticTest_particlelit_generic_ps30 psh_forgot_to_set_static_HALFLAMBERT + psh_forgot_to_set_static_HDRTYPE + 0
class particlelit_generic_ps30_Dynamic_Index
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
private:
	int m_nHDRENABLED;
#ifdef _DEBUG
	bool m_bHDRENABLED;
#endif
public:
	void SetHDRENABLED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHDRENABLED = i;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif
	}
	void SetHDRENABLED( bool i )
	{
		m_nHDRENABLED = i ? 1 : 0;
#ifdef _DEBUG
		m_bHDRENABLED = true;
#endif
	}
public:
	particlelit_generic_ps30_Dynamic_Index()
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
#ifdef _DEBUG
		m_bHDRENABLED = false;
#endif // _DEBUG
		m_nHDRENABLED = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bLIGHT_COMBO && m_bWRITEWATERFOGTODESTALPHA && m_bFOGTYPE && m_bHDRENABLED;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nLIGHT_COMBO ) + ( 22 * m_nWRITEWATERFOGTODESTALPHA ) + ( 44 * m_nFOGTYPE ) + ( 132 * m_nHDRENABLED ) + 0;
	}
};
#define shaderDynamicTest_particlelit_generic_ps30 psh_forgot_to_set_dynamic_LIGHT_COMBO + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_FOGTYPE + psh_forgot_to_set_dynamic_HDRENABLED + 0
