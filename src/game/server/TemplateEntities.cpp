//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Template entities are used by spawners to create copies of entities
//			that were configured by the level designer. This allows us to spawn
//			entities with arbitrary sets of key/value data and entity I/O
//			connections.
//
//			Template entities are marked with a special spawnflag which causes
//			them not to spawn, but to be saved as a string containing all the
//			map data (keyvalues and I/O connections) from the BSP. Template
//			entities are looked up by name by the spawner, which copies the
//			map data into a local string (that's how the template data is saved
//			and restored). Once all the entities in the map have been activated,
//			the template database is freed.
//
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "mapentities_shared.h"
#include "point_template.h"
#include "eventqueue.h"
#include "TemplateEntities.h"
#include "utldict.h"
#include "fgdlib/entitydefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar template_debug( "template_debug", "0" );

// This is appended to key's values that will need to be unique in template instances
const char *ENTITYIO_FIXUP_STRING = "&0000";

int MapEntity_GetNumKeysInEntity( const char *pEntData );

struct TemplateEntityData_t
{
	const char	*pszName;
	char		*pszMapData;
	string_t	iszMapData;
	int			iMapDataLength;
	bool		bNeedsEntityIOFixup;	// If true, this template has entity I/O in its mapdata that needs fixup before spawning.
	char		*pszFixedMapData;		// A single copy of this template that we used to fix up the Entity I/O whenever someone wants a fixed version of this template

	DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( TemplateEntityData_t )
	//DEFINE_FIELD( pszName,			FIELD_STRING	),		// Saved custom, see below
	//DEFINE_FIELD( pszMapData,			FIELD_STRING	),		// Saved custom, see below
	DEFINE_FIELD( iszMapData,				FIELD_STRING	),
	DEFINE_FIELD( iMapDataLength,			FIELD_INTEGER	),
	DEFINE_FIELD( bNeedsEntityIOFixup,	FIELD_BOOLEAN	),

	//DEFINE_FIELD( pszFixedMapData,	FIELD_STRING	),		// Not saved at all
END_DATADESC()

struct grouptemplate_t
{
	CEntityMapData *pMapDataParser;
	char			pszName[MAPKEY_MAXLENGTH];
	int				iIndex;
	bool			bChangeTargetname;
};

static CUtlVector<TemplateEntityData_t *> g_Templates;

int	g_iCurrentTemplateInstance;

//-----------------------------------------------------------------------------
// Purpose: Saves the given entity's keyvalue data for later use by a spawner.
//			Returns the index into the templates.
//-----------------------------------------------------------------------------
int Templates_Add(CBaseEntity *pEntity, const char *pszMapData, int nLen)
{
	const char *pszName = STRING(pEntity->GetEntityName());
	if ((!pszName) || (!strlen(pszName)))
	{
		DevWarning(1, "RegisterTemplateEntity: template entity with no name, class %s\n", pEntity->GetClassname());
		return -1;
	}

	TemplateEntityData_t *pEntData = (TemplateEntityData_t *)malloc(sizeof(TemplateEntityData_t));
	pEntData->pszName = strdup( pszName );

	// We may modify the values of the keys in this mapdata chunk later on to fix Entity I/O
	// connections. For this reason, we need to ensure we have enough memory to do that.
	int iKeys = MapEntity_GetNumKeysInEntity( pszMapData );
	int iExtraSpace = (strlen(ENTITYIO_FIXUP_STRING)+1) * iKeys;

	// Extra 1 because the mapdata passed in isn't null terminated
	pEntData->iMapDataLength = nLen + iExtraSpace + 1;
	pEntData->pszMapData = (char *)malloc( pEntData->iMapDataLength );
	memcpy(pEntData->pszMapData, pszMapData, nLen + 1);
	pEntData->pszMapData[nLen] = '\0';

	// We don't alloc these suckers right now because that gives us no time to
	// tweak them for Entity I/O purposes.
	pEntData->iszMapData = NULL_STRING;
	pEntData->bNeedsEntityIOFixup = false;
	pEntData->pszFixedMapData = NULL;

	return g_Templates.AddToTail(pEntData);
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if the specified index needs to be fixed up to be unique
//			when the template is spawned.
//-----------------------------------------------------------------------------
bool Templates_IndexRequiresEntityIOFixup( int iIndex )
{
	Assert( iIndex < g_Templates.Count() );
	return g_Templates[iIndex]->bNeedsEntityIOFixup;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up a template entity by its index in the templates
//			Used by point_templates because they often have multiple templates with the same name
//-----------------------------------------------------------------------------
string_t Templates_FindByIndex( int iIndex )
{
	Assert( iIndex < g_Templates.Count() );

	// First time through we alloc the mapdata onto the pool.
	// It's safe to do it now because this isn't called until post Entity I/O cleanup.
	if ( g_Templates[iIndex]->iszMapData == NULL_STRING )
	{
		g_Templates[iIndex]->iszMapData = AllocPooledString( g_Templates[iIndex]->pszMapData );
	}

	return g_Templates[iIndex]->iszMapData;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int Templates_GetStringSize( int iIndex )
{
	Assert( iIndex < g_Templates.Count() );
	return g_Templates[iIndex]->iMapDataLength;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up a template entity by name, returning the map data blob as
//			a null-terminated string containing key/value pairs.
//			NOTE: This can't handle multiple templates with the same targetname.
//-----------------------------------------------------------------------------
string_t Templates_FindByTargetName(const char *pszName)
{
	int nCount = g_Templates.Count();
	for (int i = 0; i < nCount; i++)
	{
		TemplateEntityData_t *pTemplate = g_Templates.Element(i);
		if ( !stricmp(pTemplate->pszName, pszName) )
			return Templates_FindByIndex( i );
	}

	return NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: A CPointTemplate has asked us to reconnect all the entity I/O links
//			inside it's templates. Go through the keys and add look for values
//			that match a name within the group's entity	names. Append %d to any
//			found values, which will later be filled out by a unique identifier
//			whenever the template is instanced.
//-----------------------------------------------------------------------------
void Templates_ReconnectIOForGroup( CPointTemplate *pGroup )
{
	int iCount = pGroup->GetNumTemplates();
	if ( !iCount )
		return;

	// First assemble a list of the targetnames of all the templates in the group.
	// We need to store off the original names here, because we're going to change
	// them as we go along.
	CUtlVector< grouptemplate_t > GroupTemplates;
	int i;
	for ( i = 0; i < iCount; i++ )
	{
		grouptemplate_t newGroupTemplate;
		newGroupTemplate.iIndex = pGroup->GetTemplateIndexForTemplate(i);
		newGroupTemplate.pMapDataParser = new CEntityMapData( g_Templates[ newGroupTemplate.iIndex ]->pszMapData, g_Templates[ newGroupTemplate.iIndex ]->iMapDataLength );
		Assert( newGroupTemplate.pMapDataParser );
		newGroupTemplate.pMapDataParser->ExtractValue( "targetname", newGroupTemplate.pszName );
		newGroupTemplate.bChangeTargetname = false;

		GroupTemplates.AddToTail( newGroupTemplate );
	}

	if (pGroup->AllowNameFixup())
	{
		char keyName[MAPKEY_MAXLENGTH];
		char value[MAPKEY_MAXLENGTH];
		char valueclipped[MAPKEY_MAXLENGTH];
		
		// Now go through all the entities in the group and parse their mapdata keyvalues.
		// We're looking for any values that match targetnames of any of the group entities.
		for ( i = 0; i < iCount; i++ )
		{
			// We need to know what instance of each key we're changing.
			// Store a table of the count of the keys we've run into.
			CUtlDict< int, int > KeyInstanceCount;
			CEntityMapData *mapData = GroupTemplates[i].pMapDataParser;

			// Loop through our keys
			if ( !mapData->GetFirstKey(keyName, value) )
				continue;

			do 
			{
				// Ignore targetnames
				if ( !stricmp( keyName, "targetname" ) )
					continue;

				// Add to the count for this 
				int idx = KeyInstanceCount.Find( keyName );
				if ( idx == KeyInstanceCount.InvalidIndex() )
				{
					idx = KeyInstanceCount.Insert( keyName, 0 );
				}
				KeyInstanceCount[idx]++;

				// Entity I/O values are stored as "Targetname,<data>", so we need to see if there's a ',' in the string
				char *sValue = value;
				// FIXME: This is very brittle. Any key with a , will not be found.
				char delimiter = VMF_IOPARAM_STRING_DELIMITER;
				if( strchr( value, delimiter ) == NULL )
				{
					delimiter = ',';
				}

				char *s = strchr( value, delimiter );
				if ( s )
				{
					// Grab just the targetname of the receiver
					Q_strncpy( valueclipped, value, (s - value+1) );
					sValue = valueclipped;
				}

				// Loop through our group templates
				for ( int iTName = 0; iTName < iCount; iTName++ )
				{
					char *pName = GroupTemplates[iTName].pszName;
					if ( stricmp( pName, sValue ) )
						continue;

					if ( template_debug.GetInt() )
					{
						Msg("Template Connection Found: Key %s (\"%s\") in entity named \"%s\"(%d) matches entity %d's targetname\n", keyName, sValue, GroupTemplates[i].pszName, i, iTName );
					}

					char newvalue[MAPKEY_MAXLENGTH];

					// Get the current key instance. (-1 because it's this one we're changing)
					int nKeyInstance = KeyInstanceCount[idx] - 1;

					// Add our IO value to the targetname
					// We need to append it if this isn't an Entity I/O value, or prepend it to the ',' if it is
					if ( s )
					{
						Q_strncpy( newvalue, valueclipped, MAPKEY_MAXLENGTH );
						Q_strncat( newvalue, ENTITYIO_FIXUP_STRING, sizeof(newvalue), COPY_ALL_CHARACTERS );
						Q_strncat( newvalue, s, sizeof(newvalue), COPY_ALL_CHARACTERS );
						mapData->SetValue( keyName, newvalue, nKeyInstance );
					}
					else
					{
						Q_strncpy( newvalue, sValue, MAPKEY_MAXLENGTH );
						Q_strncat( newvalue, ENTITYIO_FIXUP_STRING, sizeof(newvalue), COPY_ALL_CHARACTERS );
						mapData->SetValue( keyName, newvalue, nKeyInstance );
					}
					
					// Remember we changed this targetname
					GroupTemplates[iTName].bChangeTargetname = true;

					// Set both entity's flags telling them their template needs fixup when it's spawned
					g_Templates[ GroupTemplates[i].iIndex ]->bNeedsEntityIOFixup = true;
					g_Templates[ GroupTemplates[iTName].iIndex ]->bNeedsEntityIOFixup = true;
				}
			} 
			while ( mapData->GetNextKey(keyName, value) );
		}

		// Now change targetnames for all entities that need them changed
		for ( i = 0; i < iCount; i++ )
		{
			char value[MAPKEY_MAXLENGTH];

			if ( GroupTemplates[i].bChangeTargetname )
			{
				CEntityMapData *mapData = GroupTemplates[i].pMapDataParser;
				mapData->ExtractValue( "targetname", value );
				Q_strncat( value, ENTITYIO_FIXUP_STRING, sizeof(value), COPY_ALL_CHARACTERS );
				mapData->SetValue( "targetname", value );
			}
		}
	}

	// Delete our group parsers
	for ( i = 0; i < iCount; i++ )
	{
		delete GroupTemplates[i].pMapDataParser;
	}
	GroupTemplates.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Someone's about to start instancing a new group of entities.
//			Generate a unique identifier for this group.
//-----------------------------------------------------------------------------
void Templates_StartUniqueInstance( void )
{
	g_iCurrentTemplateInstance++;

	// Make sure there's enough room to fit it into the string
	int iMax = pow(10.0f, (int)((strlen(ENTITYIO_FIXUP_STRING) - 1)));	// -1 for the &
	if ( g_iCurrentTemplateInstance >= iMax )
	{
		// We won't hit this.
		Assert(0);
		// Hopefully there were still be instance number 0 around.
		g_iCurrentTemplateInstance = 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Someone wants to spawn an instance of a template that requires
//			entity IO fixup. Fill out the pMapData with a copy of the template
//			with unique key/values where the template requires them.
//-----------------------------------------------------------------------------
char *Templates_GetEntityIOFixedMapData( int iIndex )
{
	Assert( Templates_IndexRequiresEntityIOFixup( iIndex ) );

	// First time through?
	if ( !g_Templates[iIndex]->pszFixedMapData )
	{
		g_Templates[iIndex]->pszFixedMapData = new char[g_Templates[iIndex]->iMapDataLength];
		Q_strncpy( g_Templates[iIndex]->pszFixedMapData, g_Templates[iIndex]->pszMapData, g_Templates[iIndex]->iMapDataLength );
	}

	int iFixupSize = strlen(ENTITYIO_FIXUP_STRING); // don't include \0 when copying in the fixup
	char *sOurFixup = new char[iFixupSize+1]; // do alloc room here for the null terminator
	Q_snprintf( sOurFixup, iFixupSize+1, "%c%.4d", ENTITYIO_FIXUP_STRING[0], g_iCurrentTemplateInstance );

	// Now rip through the map data string and replace any instances of the fixup string with our unique identifier
	char *c = g_Templates[iIndex]->pszFixedMapData;
	do
	{
		if ( *c == ENTITYIO_FIXUP_STRING[0] )
		{
			// Make sure it's our fixup string
			bool bValid = true;
			for ( int i = 1; i < iFixupSize; i++ )
			{
				// Look for any number, because we've already used this string
				if ( !(*(c+i) >= '0' && *(c+i) <= '9') )
				{
					// Some other string
					bValid = false;
					break;
				}
			}

			// Stomp it with our unique string
			if ( bValid )
			{
				memcpy( c, sOurFixup, iFixupSize );
				c += iFixupSize;
			}
		}
		c++;
	} while (*c);

	return g_Templates[iIndex]->pszFixedMapData;
}

//-----------------------------------------------------------------------------
// Purpose: Frees all the template data. Called on level shutdown.
//-----------------------------------------------------------------------------
void Templates_RemoveAll(void)
{
	int nCount = g_Templates.Count();
	for (int i = 0; i < nCount; i++)
	{
		TemplateEntityData_t *pTemplate = g_Templates.Element(i);

		free((void *)pTemplate->pszName);
		free(pTemplate->pszMapData);
		if ( pTemplate->pszFixedMapData )
		{
			free(pTemplate->pszFixedMapData);
		}

		free(pTemplate);
	}

	g_Templates.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Hooks in the template manager's callbacks.
//-----------------------------------------------------------------------------
class CTemplatesHook : public CAutoGameSystem
{
public:
	CTemplatesHook( char const *name ) : CAutoGameSystem( name )
	{
	}

	virtual void LevelShutdownPostEntity( void )
	{
		Templates_RemoveAll();
	}
};

CTemplatesHook g_TemplateEntityHook( "CTemplatesHook" );


//-----------------------------------------------------------------------------
// TEMPLATE SAVE / RESTORE
//-----------------------------------------------------------------------------
static short TEMPLATE_SAVE_RESTORE_VERSION = 1;

class CTemplate_SaveRestoreBlockHandler : public CDefSaveRestoreBlockHandler
{
public:
	const char *GetBlockName()
	{
		return "Templates";
	}

	//---------------------------------

	void Save( ISave *pSave )
	{
		pSave->WriteInt( &g_iCurrentTemplateInstance );

		short nCount = g_Templates.Count();
		pSave->WriteShort( &nCount );
		for ( int i = 0; i < nCount; i++ )
		{
			TemplateEntityData_t *pTemplate = g_Templates[i];
			pSave->WriteAll( pTemplate );
			pSave->WriteString( pTemplate->pszName );
			pSave->WriteString( pTemplate->pszMapData );
		}
	}

	//---------------------------------

	void WriteSaveHeaders( ISave *pSave )
	{
		pSave->WriteShort( &TEMPLATE_SAVE_RESTORE_VERSION );
	}
	
	//---------------------------------

	void ReadRestoreHeaders( IRestore *pRestore )
	{
		// No reason why any future version shouldn't try to retain backward compatability. The default here is to not do so.
		short version;
		pRestore->ReadShort( &version );
		m_fDoLoad = ( version == TEMPLATE_SAVE_RESTORE_VERSION );
	}

	//---------------------------------

	void Restore( IRestore *pRestore, bool createPlayers )
	{
		if ( m_fDoLoad )
		{
			Templates_RemoveAll();
			g_Templates.Purge();
			g_iCurrentTemplateInstance = pRestore->ReadInt();

			int iTemplates = pRestore->ReadShort();
			while ( iTemplates-- )
			{
				TemplateEntityData_t *pNewTemplate = (TemplateEntityData_t *)malloc(sizeof(TemplateEntityData_t));
				pRestore->ReadAll( pNewTemplate );

				int sizeData = 0;//pRestore->SkipHeader();
				char szName[MAPKEY_MAXLENGTH];
				pRestore->ReadString( szName, MAPKEY_MAXLENGTH, sizeData );
				pNewTemplate->pszName = strdup( szName );
				//sizeData = pRestore->SkipHeader();
				pNewTemplate->pszMapData = (char *)malloc( pNewTemplate->iMapDataLength );
				pRestore->ReadString( pNewTemplate->pszMapData, pNewTemplate->iMapDataLength, sizeData );

				// Set this to NULL so it'll be created the first time it gets used
				pNewTemplate->pszFixedMapData = NULL;

				g_Templates.AddToTail( pNewTemplate );
			}
		}
		
	}

private:
	bool m_fDoLoad;
};

//-----------------------------------------------------------------------------

CTemplate_SaveRestoreBlockHandler g_Template_SaveRestoreBlockHandler;

//-------------------------------------

ISaveRestoreBlockHandler *GetTemplateSaveRestoreBlockHandler()
{
	return &g_Template_SaveRestoreBlockHandler;
}
