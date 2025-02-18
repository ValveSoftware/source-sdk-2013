//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "decals.h"
#include "igamesystem.h"
#include "utlsymbol.h"
#include "utldict.h"
#include "KeyValues.h"
#include "filesystem.h"

#ifdef CLIENT_DLL
#include "iefx.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DECAL_LIST_FILE "scripts/decals_subrect.txt"
//#define DECAL_LIST_FILE "scripts/decals.txt"
#define TRANSLATION_DATA_SECTION "TranslationData"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDecalEmitterSystem : public IDecalEmitterSystem, public CAutoGameSystem
{
public:
	CDecalEmitterSystem( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual bool		Init();
	virtual void		Shutdown();
	virtual void		LevelInitPreEntity();

	// Public interface
	virtual int			GetDecalIndexForName( char const *decalname );
	virtual const char *GetDecalNameForIndex( int nIndex );
	virtual char const *TranslateDecalForGameMaterial( char const *decalName, unsigned char gamematerial );

private:
	char const		*ImpactDecalForGameMaterial( int gamematerial );
	void			LoadDecalsFromScript( char const *filename );
	void			Clear();

	struct DecalListEntry
	{
		DecalListEntry()
		{
			name			= UTL_INVAL_SYMBOL;
			precache_index	= -1;
			weight			= 1.0f;
		}

		CUtlSymbol	name;
		int			precache_index;
		float		weight;
	};

	struct DecalEntry
	{
		DecalEntry()
		{
		}

		DecalEntry( const DecalEntry& src )
		{
			int c = src.indices.Count();
			for ( int i = 0; i < c; i++ )
			{
				indices.AddToTail( src.indices[ i ] );
			}
		}

		DecalEntry& operator = ( const DecalEntry& src )
		{
			if ( this == &src )
				return *this;

			int c = src.indices.Count();
			for ( int i = 0; i < c; i++ )
			{
				indices.AddToTail( src.indices[ i ] );
			}

			return *this;
		}

		CUtlVector< int >	indices;
	};

	CUtlVector< DecalListEntry >	m_AllDecals;
	CUtlDict< DecalEntry, int >		m_Decals;
	CUtlSymbolTable					m_DecalFileNames;
	CUtlDict< int, int >			m_GameMaterialTranslation;
};

static CDecalEmitterSystem g_DecalSystem( "CDecalEmitterSystem" );
IDecalEmitterSystem *decalsystem = &g_DecalSystem;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *decalname - 
// Output : int
//-----------------------------------------------------------------------------
int CDecalEmitterSystem::GetDecalIndexForName( char const *decalname )
{
	if ( !decalname  || !decalname[ 0 ] )
		return -1;

	int idx = m_Decals.Find( decalname );
	if ( idx == m_Decals.InvalidIndex() )
		return -1;

	DecalEntry *e = &m_Decals[ idx ];
	Assert( e );
	int count = e->indices.Count();
	if ( count <= 0 )
		return -1;

	float totalweight = 0.0f;
	int slot = 0;

	for ( int i = 0; i < count; i++ )
	{
		idx = e->indices[ i ];
		DecalListEntry *item = &m_AllDecals[ idx ];
		Assert( item );
		
		if ( !totalweight )
		{
			slot = idx;
		}

		// Always assume very first slot will match
		totalweight += item->weight;
		if ( !totalweight || random->RandomFloat(0,totalweight) < item->weight )
		{
			slot = idx;
		}
	}

	return m_AllDecals[ slot ].precache_index;
}

const char *CDecalEmitterSystem::GetDecalNameForIndex( int nIndex )
{
	for ( int nDecal = 0; nDecal < m_AllDecals.Count(); ++nDecal )
	{
		if ( m_AllDecals[ nDecal ].precache_index == nIndex )
		{
			return m_DecalFileNames.String( m_AllDecals[ nDecal ].name );
		}
	}

	return "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CDecalEmitterSystem::Init()
{
	LoadDecalsFromScript( DECAL_LIST_FILE );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDecalEmitterSystem::LevelInitPreEntity()
{
	// Precache all entries
	int c = m_AllDecals.Count();
	for ( int i = 0 ; i < c; i++ )
	{
		DecalListEntry& e = m_AllDecals[ i ];
#if defined( CLIENT_DLL )
		e.precache_index = effects->Draw_DecalIndexFromName( (char *)m_DecalFileNames.String( e.name ) );
#else
		e.precache_index = engine->PrecacheDecal( m_DecalFileNames.String( e.name ) );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//-----------------------------------------------------------------------------
void CDecalEmitterSystem::LoadDecalsFromScript( char const *filename )
{
	KeyValues *kv = new KeyValues( filename );
	Assert( kv );
	if ( kv )
	{
		KeyValues *translation = NULL;
#ifndef _XBOX
		if ( kv->LoadFromFile( filesystem, filename ) )
#else
		if ( kv->LoadFromFile( filesystem, filename, "GAME" ) )
#endif
		{
			KeyValues *p = kv;
			while ( p )
			{
				if ( p->GetFirstSubKey() )
				{
					char const *keyname = p->GetName();

					if ( !Q_stricmp( keyname, TRANSLATION_DATA_SECTION ) )
					{
						translation = p;
					}
					else
					{
						DecalEntry entry;

						for ( KeyValues *sub = p->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
						{
							MEM_ALLOC_CREDIT();

							DecalListEntry decal;
							decal.precache_index = -1;
							decal.name = m_DecalFileNames.AddString( sub->GetName() );
							decal.weight = sub->GetFloat();

							// Add to global list
							int idx = m_AllDecals.AddToTail( decal );

							// Add index only to local list
							entry.indices.AddToTail( idx );
						}

						// Add entry to main dictionary
						m_Decals.Insert( keyname, entry );
					}
				}
				p = p->GetNextKey();
			}
		}
		else
		{
			Msg( "CDecalEmitterSystem::LoadDecalsFromScript:  Unable to load '%s'\n", filename );
		}

		if ( !translation )
		{
			Msg( "CDecalEmitterSystem::LoadDecalsFromScript:  Script '%s' missing section '%s'\n",
				filename,
				TRANSLATION_DATA_SECTION );
		}
		else
		{
			// Now parse game material to entry translation table
			for ( KeyValues *sub = translation->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
			{
				// Don't add NULL string to list
				if ( !Q_stricmp( sub->GetString(), "" ) )
					continue;

				int idx = m_Decals.Find( sub->GetString() );
				if ( idx != m_Decals.InvalidIndex() )
				{
					m_GameMaterialTranslation.Insert( sub->GetName(), idx );
				}
				else
				{
					Msg( "CDecalEmitterSystem::LoadDecalsFromScript:  Translation for game material type '%s' references unknown decal '%s'\n",
						sub->GetName(), sub->GetString() );
				}
			}
		}

		kv->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : gamematerial - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CDecalEmitterSystem::ImpactDecalForGameMaterial( int gamematerial )
{
	char gm[ 2 ];
	gm[0] = (char)gamematerial;
	gm[1] = 0;

	int idx = m_GameMaterialTranslation.Find( gm );
	if ( idx == m_GameMaterialTranslation.InvalidIndex() )
		return NULL;

	return m_Decals.GetElementName( m_GameMaterialTranslation.Element(idx) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDecalEmitterSystem::Shutdown()
{
	Clear();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDecalEmitterSystem::Clear()
{
	m_DecalFileNames.RemoveAll();
	m_Decals.Purge();
	m_AllDecals.Purge();
	m_GameMaterialTranslation.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *decalName - 
//			gamematerial - 
// Output : char const
//-----------------------------------------------------------------------------
char const *CDecalEmitterSystem::TranslateDecalForGameMaterial( char const *decalName, unsigned char gamematerial )
{
	if ( gamematerial == CHAR_TEX_CONCRETE )
		return decalName;

	if ( !Q_stricmp( decalName, "Impact.Concrete" ) )
	{
		if ( gamematerial == '-' )
			return "";

		char const *d = ImpactDecalForGameMaterial( gamematerial );
		if ( d )
		{
			return d;
		}
	}

	return decalName;
}


