class vortwarp_vs11_Static_Index
{
private:
	int m_nUNLIT;
#ifdef _DEBUG
	bool m_bUNLIT;
#endif
public:
	void SetUNLIT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUNLIT = i;
#ifdef _DEBUG
		m_bUNLIT = true;
#endif
	}
	void SetUNLIT( bool i )
	{
		m_nUNLIT = i ? 1 : 0;
#ifdef _DEBUG
		m_bUNLIT = true;
#endif
	}
private:
	int m_nHALF_LAMBERT;
#ifdef _DEBUG
	bool m_bHALF_LAMBERT;
#endif
public:
	void SetHALF_LAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALF_LAMBERT = i;
#ifdef _DEBUG
		m_bHALF_LAMBERT = true;
#endif
	}
	void SetHALF_LAMBERT( bool i )
	{
		m_nHALF_LAMBERT = i ? 1 : 0;
#ifdef _DEBUG
		m_bHALF_LAMBERT = true;
#endif
	}
public:
	vortwarp_vs11_Static_Index()
	{
#ifdef _DEBUG
		m_bUNLIT = false;
#endif // _DEBUG
		m_nUNLIT = 0;
#ifdef _DEBUG
		m_bHALF_LAMBERT = false;
#endif // _DEBUG
		m_nHALF_LAMBERT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bUNLIT && m_bHALF_LAMBERT;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 44 * m_nUNLIT ) + ( 88 * m_nHALF_LAMBERT ) + 0;
	}
};
class vortwarp_vs11_Dynamic_Index
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
	vortwarp_vs11_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bLIGHT_COMBO = false;
#endif // _DEBUG
		m_nLIGHT_COMBO = 0;
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
		bool bAllDynamicVarsDefined = m_bLIGHT_COMBO && m_bSKINNING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nLIGHT_COMBO ) + ( 22 * m_nSKINNING ) + 0;
	}
};
