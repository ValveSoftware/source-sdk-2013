class debugbumpmappedlightmap_ps20b_Static_Index
{
public:
	debugbumpmappedlightmap_ps20b_Static_Index()
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
#define shaderStaticTest_debugbumpmappedlightmap_ps20b 0
class debugbumpmappedlightmap_ps20b_Dynamic_Index
{
private:
	int m_nMODE;
#ifdef _DEBUG
	bool m_bMODE;
#endif
public:
	void SetMODE( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nMODE = i;
#ifdef _DEBUG
		m_bMODE = true;
#endif
	}
	void SetMODE( bool i )
	{
		m_nMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bMODE = true;
#endif
	}
public:
	debugbumpmappedlightmap_ps20b_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bMODE = false;
#endif // _DEBUG
		m_nMODE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bMODE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nMODE ) + 0;
	}
};
#define shaderDynamicTest_debugbumpmappedlightmap_ps20b psh_forgot_to_set_dynamic_MODE + 0
