//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Controls the loading, parsing and creation of the entities from the BSP.
//
//=============================================================================//

#include "cbase.h"
#include "entitylist.h"
#include "mapentities_shared.h"
#include "soundent.h"
#include "TemplateEntities.h"
#include "point_template.h"
#include "ai_initutils.h"
#include "lights.h"
#include "mapentities.h"
#include "wcedit.h"
#include "stringregistry.h"
#include "datacache/imdlcache.h"
#include "world.h"
#include "toolframework/iserverenginetools.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


struct HierarchicalSpawnMapData_t
{
	const char	*m_pMapData;
	int			m_iMapDataLength;
};

static CStringRegistry *g_pClassnameSpawnPriority = NULL;
extern edict_t *g_pForceAttachEdict;

// creates an entity by string name, but does not spawn it
CBaseEntity *CreateEntityByName( const char *className, int iForceEdictIndex )
{
	if ( iForceEdictIndex != -1 )
	{
		g_pForceAttachEdict = engine->CreateEdict( iForceEdictIndex );
		if ( !g_pForceAttachEdict )
			Error( "CreateEntityByName( %s, %d ) - CreateEdict failed.", className, iForceEdictIndex );
	}

	IServerNetworkable *pNetwork = EntityFactoryDictionary()->Create( className );
	g_pForceAttachEdict = NULL;

	if ( !pNetwork )
		return NULL;

	CBaseEntity *pEntity = pNetwork->GetBaseEntity();
	Assert( pEntity );
	return pEntity;
}

CBaseNetworkable *CreateNetworkableByName( const char *className )
{
	IServerNetworkable *pNetwork = EntityFactoryDictionary()->Create( className );
	if ( !pNetwork )
		return NULL;

	CBaseNetworkable *pNetworkable = pNetwork->GetBaseNetworkable();
	Assert( pNetworkable );
	return pNetworkable;
}

void FreeContainingEntity( edict_t *ed )
{
	if ( ed )
	{
		CBaseEntity *ent = GetContainingEntity( ed );
		if ( ent )
		{
			ed->SetEdict( NULL, false );
			CBaseEntity::PhysicsRemoveTouchedList( ent );
			CBaseEntity::PhysicsRemoveGroundList( ent );
			UTIL_RemoveImmediate( ent );
		}
	}
}

// parent name may have a , in it to include an attachment point
string_t ExtractParentName(string_t parentName)
{
	if ( !strchr(STRING(parentName), ',') )
		return parentName;

	char szToken[256];
	nexttoken(szToken, STRING(parentName), ',', sizeof(szToken));
	return AllocPooledString(szToken);
}

//-----------------------------------------------------------------------------
// Purpose: Callback function for qsort, used to sort entities by their depth
//			in the movement hierarchy.
// Input  : pEnt1 - 
//			pEnt2 - 
// Output : Returns -1, 0, or 1 per qsort spec.
//-----------------------------------------------------------------------------
static int __cdecl CompareSpawnOrder(HierarchicalSpawn_t *pEnt1, HierarchicalSpawn_t *pEnt2)
{
	if (pEnt1->m_nDepth == pEnt2->m_nDepth)
	{
		if ( g_pClassnameSpawnPriority )
		{
			int o1 = pEnt1->m_pEntity ? g_pClassnameSpawnPriority->GetStringID( pEnt1->m_pEntity->GetClassname() ) : -1;
			int o2 = pEnt2->m_pEntity ? g_pClassnameSpawnPriority->GetStringID( pEnt2->m_pEntity->GetClassname() ) : -1;
			if ( o1 < o2 )
				return 1;
			if ( o2 < o1 )
				return -1;
		}
		return 0;
	}

	if (pEnt1->m_nDepth > pEnt2->m_nDepth)
		return 1;

	return -1;
}


//-----------------------------------------------------------------------------
// Computes the hierarchical depth of the entities to spawn..
//-----------------------------------------------------------------------------
static int ComputeSpawnHierarchyDepth_r( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return 1;

	if (pEntity->m_iParent == NULL_STRING)
		return 1;

	CBaseEntity *pParent = gEntList.FindEntityByName( NULL, ExtractParentName(pEntity->m_iParent) );
	if (!pParent)
		return 1;
	
	if (pParent == pEntity)
	{
		Warning( "LEVEL DESIGN ERROR: Entity %s is parented to itself!\n", pEntity->GetDebugName() );
		return 1;
	}

	return 1 + ComputeSpawnHierarchyDepth_r( pParent );
}

static void ComputeSpawnHierarchyDepth( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	// NOTE: This isn't particularly efficient, but so what? It's at the beginning of time
	// I did it this way because it simplified the parent setting in hierarchy (basically
	// eliminated questions about whether you should transform origin from global to local or not)
	int nEntity;
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if (pEntity && !pEntity->IsDormant())
		{
			pSpawnList[nEntity].m_nDepth = ComputeSpawnHierarchyDepth_r( pEntity );
		}
		else
		{
			pSpawnList[nEntity].m_nDepth = 1;
		}
	}
}

static void SortSpawnListByHierarchy( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	MEM_ALLOC_CREDIT();
	g_pClassnameSpawnPriority = new CStringRegistry;
	// this will cause the entities to be spawned in the indicated order
	// Highest string ID spawns first.  String ID is spawn priority.
	// by default, anything not in this list has priority -1
	g_pClassnameSpawnPriority->AddString( "func_wall", 10 );
	g_pClassnameSpawnPriority->AddString( "scripted_sequence", 9 );
	g_pClassnameSpawnPriority->AddString( "phys_hinge", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_ballsocket", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_slideconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_constraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_pulleyconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_lengthconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "phys_ragdollconstraint", 8 );
	g_pClassnameSpawnPriority->AddString( "info_mass_center", 8 ); // spawn these before physbox/prop_physics
	g_pClassnameSpawnPriority->AddString( "trigger_vphysics_motion", 8 ); // spawn these before physbox/prop_physics

	g_pClassnameSpawnPriority->AddString( "prop_physics", 7 );
	g_pClassnameSpawnPriority->AddString( "prop_ragdoll", 7 );
	// Sort the entities (other than the world) by hierarchy depth, in order to spawn them in
	// that order. This insures that each entity's parent spawns before it does so that
	// it can properly set up anything that relies on hierarchy.
#ifdef _WIN32
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (__cdecl *)(const void *, const void *))CompareSpawnOrder);
#elif POSIX
	qsort(&pSpawnList[0], nEntities, sizeof(pSpawnList[0]), (int (*)(const void *, const void *))CompareSpawnOrder);
#endif
	delete g_pClassnameSpawnPriority;
	g_pClassnameSpawnPriority = NULL;
}

void SetupParentsForSpawnList( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	int nEntity;
	for (nEntity = nEntities - 1; nEntity >= 0; nEntity--)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;
		if ( pEntity )
		{
			if ( strchr(STRING(pEntity->m_iParent), ',') )
			{
				char szToken[256];
				const char *pAttachmentName = nexttoken(szToken, STRING(pEntity->m_iParent), ',', sizeof(szToken));
				pEntity->m_iParent = AllocPooledString(szToken);
				CBaseEntity *pParent = gEntList.FindEntityByName( NULL, pEntity->m_iParent );

				// setparent in the spawn pass instead - so the model will have been set & loaded
				pSpawnList[nEntity].m_pDeferredParent = pParent;
				pSpawnList[nEntity].m_pDeferredParentAttachment = pAttachmentName;
			}
			else
			{
				CBaseEntity *pParent = gEntList.FindEntityByName( NULL, pEntity->m_iParent );

				if ((pParent != NULL) && (pParent->edict() != NULL))
				{
					pEntity->SetParent( pParent ); 
				}
			}
		}
	}
}

// this is a hook for edit mode
void RememberInitialEntityPositions( int nEntities, HierarchicalSpawn_t *pSpawnList )
{
	for (int nEntity = 0; nEntity < nEntities; nEntity++)
	{
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

		if ( pEntity )
		{
			NWCEdit::RememberEntityPosition( pEntity );
		}
	}
}


void SpawnAllEntities( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities )
{
	int nEntity;
	for (nEntity = 0; nEntity < nEntities; nEntity++)
	{
		VPROF( "MapEntity_ParseAllEntities_Spawn");
		CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

		if ( pSpawnList[nEntity].m_pDeferredParent )
		{
			// UNDONE: Promote this up to the root of this function?
			MDLCACHE_CRITICAL_SECTION();
			CBaseEntity *pParent = pSpawnList[nEntity].m_pDeferredParent;
			int iAttachment = -1;
			CBaseAnimating *pAnim = pParent->GetBaseAnimating();
			if ( pAnim )
			{
				iAttachment = pAnim->LookupAttachment(pSpawnList[nEntity].m_pDeferredParentAttachment);
			}
			pEntity->SetParent( pParent, iAttachment );
		}
		if ( pEntity )
		{
			if (DispatchSpawn(pEntity) < 0)
			{
				for ( int i = nEntity+1; i < nEntities; i++ )
				{
					// this is a child object that will be deleted now
					if ( pSpawnList[i].m_pEntity && pSpawnList[i].m_pEntity->IsMarkedForDeletion() )
					{
						pSpawnList[i].m_pEntity = NULL;
					}
				}
				// Spawn failed.
				gEntList.CleanupDeleteList();
				// Remove the entity from the spawn list
				pSpawnList[nEntity].m_pEntity = NULL;
			}
		}
	}

	if ( bActivateEntities )
	{
		VPROF( "MapEntity_ParseAllEntities_Activate");
		bool bAsyncAnims = mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, false );
		for (nEntity = 0; nEntity < nEntities; nEntity++)
		{
			CBaseEntity *pEntity = pSpawnList[nEntity].m_pEntity;

			if ( pEntity )
			{
				MDLCACHE_CRITICAL_SECTION();
				pEntity->Activate();
			}
		}
		mdlcache->SetAsyncLoad( MDLCACHE_ANIMBLOCK, bAsyncAnims );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only called on BSP load. Parses and spawns all the entities in the BSP.
// Input  : pMapData - Pointer to the entity data block to parse.
//-----------------------------------------------------------------------------
void MapEntity_ParseAllEntities(const char *pMapData, IMapEntityFilter *pFilter, bool bActivateEntities)
{
	VPROF("MapEntity_ParseAllEntities");

	HierarchicalSpawnMapData_t *pSpawnMapData = new HierarchicalSpawnMapData_t[NUM_ENT_ENTRIES];
	HierarchicalSpawn_t *pSpawnList = new HierarchicalSpawn_t[NUM_ENT_ENTRIES];

	CUtlVector< CPointTemplate* > pPointTemplates;
	int nEntities = 0;

	char szTokenBuffer[MAPKEY_MAXLENGTH];

	// Allow the tools to spawn different things
	if ( serverenginetools )
	{
		pMapData = serverenginetools->GetEntityData( pMapData );
	}

	//  Loop through all entities in the map data, creating each.
	for ( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) )
	{
		//
		// Parse the opening brace.
		//
		char token[MAPKEY_MAXLENGTH];
		pMapData = MapEntity_ParseToken( pMapData, token );

		//
		// Check to see if we've finished or not.
		//
		if (!pMapData)
			break;

		if (token[0] != '{')
		{
			Error( "MapEntity_ParseAllEntities: found %s when expecting {", token);
			continue;
		}

		//
		// Parse the entity and add it to the spawn list.
		//
		CBaseEntity *pEntity;
		const char *pCurMapData = pMapData;
		pMapData = MapEntity_ParseEntity(pEntity, pMapData, pFilter);
		if (pEntity == NULL)
			continue;

		if (pEntity->IsTemplate())
		{
			// It's a template entity. Squirrel away its keyvalue text so that we can
			// recreate the entity later via a spawner. pMapData points at the '}'
			// so we must add one to include it in the string.
			Templates_Add(pEntity, pCurMapData, (pMapData - pCurMapData) + 2);

			// Remove the template entity so that it does not show up in FindEntityXXX searches.
			UTIL_Remove(pEntity);
			gEntList.CleanupDeleteList();
			continue;
		}

		// To 
		if ( dynamic_cast<CWorld*>( pEntity ) )
		{
			VPROF( "MapEntity_ParseAllEntities_SpawnWorld");

			pEntity->m_iParent = NULL_STRING;	// don't allow a parent on the first entity (worldspawn)

			DispatchSpawn(pEntity);
			continue;
		}
				
		CNodeEnt *pNode = dynamic_cast<CNodeEnt*>(pEntity);
		if ( pNode )
		{
			VPROF( "MapEntity_ParseAllEntities_SpawnTransients");

			// We overflow the max edicts on large maps that have lots of entities.
			// Nodes & Lights remove themselves immediately on Spawn(), so dispatch their
			// spawn now, to free up the slot inside this loop.
			// NOTE: This solution prevents nodes & lights from being used inside point_templates.
			//
			// NOTE: Nodes spawn other entities (ai_hint) if they need to have a persistent presence.
			//		 To ensure keys are copied over into the new entity, we pass the mapdata into the
			//		 node spawn function.
			if ( pNode->Spawn( pCurMapData ) < 0 )
			{
				gEntList.CleanupDeleteList();
			}
			continue;
		}

		if ( dynamic_cast<CLight*>(pEntity) )
		{
			VPROF( "MapEntity_ParseAllEntities_SpawnTransients");

			// We overflow the max edicts on large maps that have lots of entities.
			// Nodes & Lights remove themselves immediately on Spawn(), so dispatch their
			// spawn now, to free up the slot inside this loop.
			// NOTE: This solution prevents nodes & lights from being used inside point_templates.
			if (DispatchSpawn(pEntity) < 0)
			{
				gEntList.CleanupDeleteList();
			}
			continue;
		}

		// Build a list of all point_template's so we can spawn them before everything else
		CPointTemplate *pTemplate = dynamic_cast< CPointTemplate* >(pEntity);
		if ( pTemplate )
		{
			pPointTemplates.AddToTail( pTemplate );
		}
		else
		{
			// Queue up this entity for spawning
			pSpawnList[nEntities].m_pEntity = pEntity;
			pSpawnList[nEntities].m_nDepth = 0;
			pSpawnList[nEntities].m_pDeferredParentAttachment = NULL;
			pSpawnList[nEntities].m_pDeferredParent = NULL;

			pSpawnMapData[nEntities].m_pMapData = pCurMapData;
			pSpawnMapData[nEntities].m_iMapDataLength = (pMapData - pCurMapData) + 2;
			nEntities++;
		}
	}

	// Now loop through all our point_template entities and tell them to make templates of everything they're pointing to
	int iTemplates = pPointTemplates.Count();
	for ( int i = 0; i < iTemplates; i++ )
	{
		VPROF( "MapEntity_ParseAllEntities_SpawnTemplates");
		CPointTemplate *pPointTemplate = pPointTemplates[i];

		// First, tell the Point template to Spawn
		if ( DispatchSpawn(pPointTemplate) < 0 )
		{
			UTIL_Remove(pPointTemplate);
			gEntList.CleanupDeleteList();
			continue;
		}

		pPointTemplate->StartBuildingTemplates();

		// Now go through all it's templates and turn the entities into templates
		int iNumTemplates = pPointTemplate->GetNumTemplateEntities();
		for ( int iTemplateNum = 0; iTemplateNum < iNumTemplates; iTemplateNum++ )
		{
			// Find it in the spawn list
			CBaseEntity *pEntity = pPointTemplate->GetTemplateEntity( iTemplateNum );
			for ( int iEntNum = 0; iEntNum < nEntities; iEntNum++ )
			{
				if ( pSpawnList[iEntNum].m_pEntity == pEntity )
				{
					// Give the point_template the mapdata
					pPointTemplate->AddTemplate( pEntity, pSpawnMapData[iEntNum].m_pMapData, pSpawnMapData[iEntNum].m_iMapDataLength );

					if ( pPointTemplate->ShouldRemoveTemplateEntities() )
					{
						// Remove the template entity so that it does not show up in FindEntityXXX searches.
						UTIL_Remove(pEntity);
						gEntList.CleanupDeleteList();

						// Remove the entity from the spawn list
						pSpawnList[iEntNum].m_pEntity = NULL;
					}
					break;
				}
			}
		}

		pPointTemplate->FinishBuildingTemplates();
	}

	SpawnHierarchicalList( nEntities, pSpawnList, bActivateEntities );

	delete [] pSpawnMapData;
	delete [] pSpawnList;
}

void SpawnHierarchicalList( int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities )
{
	// Compute the hierarchical depth of all entities hierarchically attached
	ComputeSpawnHierarchyDepth( nEntities, pSpawnList );

	// Sort the entities (other than the world) by hierarchy depth, in order to spawn them in
	// that order. This insures that each entity's parent spawns before it does so that
	// it can properly set up anything that relies on hierarchy.
	SortSpawnListByHierarchy( nEntities, pSpawnList );

	// save off entity positions if in edit mode
	if ( engine->IsInEditMode() )
	{
		RememberInitialEntityPositions( nEntities, pSpawnList );
	}
	// Set up entity movement hierarchy in reverse hierarchy depth order. This allows each entity
	// to use its parent's world spawn origin to calculate its local origin.
	SetupParentsForSpawnList( nEntities, pSpawnList );

	// Spawn all the entities in hierarchy depth order so that parents spawn before their children.
	SpawnAllEntities( nEntities, pSpawnList, bActivateEntities );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntData - 
//-----------------------------------------------------------------------------
void MapEntity_PrecacheEntity( const char *pEntData, int &nStringSize )
{
	CEntityMapData entData( (char*)pEntData, nStringSize );
	char className[MAPKEY_MAXLENGTH];
	
	if (!entData.ExtractValue("classname", className))
	{
		Error( "classname missing from entity!\n" );
	}

	// Construct via the LINK_ENTITY_TO_CLASS factory.
	CBaseEntity *pEntity = CreateEntityByName(className);

	//
	// Set up keyvalues, which can set the model name, which is why we don't just do UTIL_PrecacheOther here...
	//
	if ( pEntity != NULL )
	{
		pEntity->ParseMapData(&entData);
		pEntity->Precache();
		UTIL_RemoveImmediate( pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Takes a block of character data as the input
// Input  : pEntity - Receives the newly constructed entity, NULL on failure.
//			pEntData - Data block to parse to extract entity keys.
// Output : Returns the current position in the entity data block.
//-----------------------------------------------------------------------------
const char *MapEntity_ParseEntity(CBaseEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter)
{
	CEntityMapData entData( (char*)pEntData );
	char className[MAPKEY_MAXLENGTH];
	
	if (!entData.ExtractValue("classname", className))
	{
		Error( "classname missing from entity!\n" );
	}

	pEntity = NULL;
	if ( !pFilter || pFilter->ShouldCreateEntity( className ) )
	{
		//
		// Construct via the LINK_ENTITY_TO_CLASS factory.
		//
		if ( pFilter )
			pEntity = pFilter->CreateNextEntity( className );
		else
			pEntity = CreateEntityByName(className);

		//
		// Set up keyvalues.
		//
		if (pEntity != NULL)
		{
			pEntity->ParseMapData(&entData);
		}
		else
		{
			Warning("Can't init %s\n", className);
		}
	}
	else
	{
		// Just skip past all the keys.
		char keyName[MAPKEY_MAXLENGTH];
		char value[MAPKEY_MAXLENGTH];
		if ( entData.GetFirstKey(keyName, value) )
		{
			do 
			{
			} 
			while ( entData.GetNextKey(keyName, value) );
		}
	}

	//
	// Return the current parser position in the data block
	//
	return entData.CurrentBufferPosition();
}


