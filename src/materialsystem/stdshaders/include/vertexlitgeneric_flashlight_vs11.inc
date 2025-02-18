class vertexlitgeneric_flashlight_vs11_Static_Index
{
private:
	int m_nTEETH;
#ifdef _DEBUG
	bool m_bTEETH;
#endif
public:
	void SetTEETH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTEETH = i;
#ifdef _DEBUG
		m_bTEETH = true;
#endif
	}
	void SetTEETH( bool i )
	{
		m_nTEETH = i ? 1 : 0;
#ifdef _DEBUG
		m_bTEETH = true;
#endif
	}
public:
	vertexlitgeneric_flashlight_vs11_Static_Index()
	{
#ifdef _DEBUG
		m_bTEETH = false;
#endif // _DEBUG
		m_nTEETH = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bTEETH;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nTEETH ) + 0;
	}
};
class vertexlitgeneric_flashlight_vs11_Dynamic_Index
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
private:
	int m_nSKINNING;
#ifdef _DEBUG
	bool m_bSKINNING;
#endif
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
	void SetSKINNING( bool i )
	{
		m_nSKINNING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
public:
	vertexlitgeneric_flashlight_vs11_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bDOWATERFOG = false;
#endif // _DEBUG
		m_nDOWATERFOG = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bDOWATERFOG && m_bSKINNING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nDOWATERFOG ) + ( 2 * m_nSKINNING ) + 0;
	}
};
