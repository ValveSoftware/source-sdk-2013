//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "iclassmap.h"
#include "utldict.h"

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
	#include "srcpy_entities.h"
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class classentry_t
{
public:
	classentry_t()
	{
		mapname[ 0 ] = 0;
		factory = 0;
		size = -1;
#ifdef ENABLE_PYTHON
		pyfactory = 0;
#endif // ENABLE_PYTHON
	}

	char const *GetMapName() const
	{
		return mapname;
	}

	void SetMapName( char const *newname )
	{
		Q_strncpy( mapname, newname, sizeof( mapname ) );
	}

	DISPATCHFUNCTION	factory;
	int					size;

#ifdef ENABLE_PYTHON
	PyEntityFactory		*pyfactory;
#endif // ENABLE_PYTHON
private:
	char				mapname[ 40 ];
};

class CClassMap : public IClassMap
{
public:
	virtual void			Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory /*= 0*/ );
	virtual const char		*Lookup( const char *classname );
	virtual C_BaseEntity	*CreateEntity( const char *mapname );
	virtual int				GetClassSize( const char *classname );

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
	virtual void			PyAdd( const char *mapname, const char *classname, int size, PyEntityFactory *factory );
	virtual void			PyRemove( const char *classname );
	virtual PyEntityFactory* PyGetFactory( const char *classname );
	virtual PyEntityFactory* PyGetFactoryByMapName( const char *classname );
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================

private:
	CUtlDict< classentry_t, unsigned short > m_ClassDict;
};

IClassMap& GetClassMap( void )
{
	static CClassMap g_Classmap;
	return g_Classmap;
}

void CClassMap::Add( const char *mapname, const char *classname, int size, DISPATCHFUNCTION factory = 0 )
{
	const char *map = Lookup( classname );
	if ( map && !Q_strcasecmp( mapname, map ) )
		return;

	if ( map )
	{
		int index = m_ClassDict.Find( classname );
		Assert( index != m_ClassDict.InvalidIndex() );
		m_ClassDict.RemoveAt( index );
	}

	classentry_t element;
	element.SetMapName( mapname );
	element.factory = factory;
	element.size = size;
	m_ClassDict.Insert( classname, element );
}

const char *CClassMap::Lookup( const char *classname )
{
	unsigned short index;
	static classentry_t lookup; 

	index = m_ClassDict.Find( classname );
	if ( index == m_ClassDict.InvalidIndex() )
		return NULL;

	lookup = m_ClassDict.Element( index );
	return lookup.GetMapName();
}

C_BaseEntity *CClassMap::CreateEntity( const char *mapname )
{
	int c = m_ClassDict.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		classentry_t *lookup = &m_ClassDict[ i ];
		if ( !lookup )
			continue;

		if ( Q_stricmp( lookup->GetMapName(), mapname ) )
			continue;

// =======================================
// PySource Additions
// =======================================
#ifndef ENABLE_PYTHON
		if ( !lookup->factory )
		{
#if defined( _DEBUG )
			Msg( "No factory for %s/%s\n", lookup->GetMapName(), m_ClassDict.GetElementName( i ) );
#endif
			continue;
		}

		return ( *lookup->factory )();
#else
		if( lookup->factory )
		{
			return ( *lookup->factory )();
		}
		else if( lookup->pyfactory )
		{
			return lookup->pyfactory->Create();
		}
		else
		{
#if defined( _DEBUG )
			Msg( "No factory for %s/%s\n", lookup->GetMapName(), m_ClassDict.GetElementName( i ) );
#endif
		}
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================
	}

	return NULL;
}

int CClassMap::GetClassSize( const char *classname )
{
	int c = m_ClassDict.Count();
	int i;

	for ( i = 0; i < c; i++ )
	{
		classentry_t *lookup = &m_ClassDict[ i ];
		if ( !lookup )
			continue;

		if ( Q_strcmp( lookup->GetMapName(), classname ) )
			continue;

		return lookup->size;
	}

	return -1;
}

// =======================================
// PySource Additions
// =======================================
#ifdef ENABLE_PYTHON
void CClassMap::PyAdd( const char *mapname, const char *classname, int size, PyEntityFactory *factory )
{
	if( !factory )
		return;

	const char *map = Lookup( classname );
	if ( map && !Q_strcasecmp( mapname, map ) )
		return;

	if ( map )
	{
		int index = m_ClassDict.Find( classname );
		Assert( index != m_ClassDict.InvalidIndex() );
		m_ClassDict.RemoveAt( index );
	}

	classentry_t element;
	element.SetMapName( mapname );
	element.pyfactory = factory;
	element.size = size;
	m_ClassDict.Insert( classname, element );
}

void CClassMap::PyRemove( const char *classname )
{
	int index = m_ClassDict.Find( classname );
	Assert( index != m_ClassDict.InvalidIndex() );
	m_ClassDict.RemoveAt( index );
}

PyEntityFactory* CClassMap::PyGetFactory( const char *classname )
{
	int index = m_ClassDict.Find( classname );
	if( index == m_ClassDict.InvalidIndex() )
		return NULL;
	return m_ClassDict[index].pyfactory;
}

PyEntityFactory* CClassMap::PyGetFactoryByMapName( const char *mapname )
{
	unsigned short idx = m_ClassDict.First();
	while( idx != m_ClassDict.InvalidIndex() )
	{
		if( !Q_stricmp (m_ClassDict[idx].GetMapName(), mapname ) )
		{
			return m_ClassDict[idx].pyfactory;
		}

		idx = m_ClassDict.Next( idx );
	}
	return NULL;
}
#endif // ENABLE_PYTHON
// =======================================
// END PySource Additions
// =======================================