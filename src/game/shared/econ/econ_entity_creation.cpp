//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_entity_creation.h"
#include "utldict.h"
#include "filesystem.h"
#include "gamestringpool.h"
#include "KeyValues.h"
#include "attribute_manager.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"
#include "util_shared.h"

#ifdef TF_CLIENT_DLL
#include "c_tf_player.h"
#endif // TF_CLIENT_DLL

//==================================================================================
// GENERATION SYSTEM
//==================================================================================
CItemGeneration g_ItemGenerationSystem;
CItemGeneration *ItemGeneration( void )
{
	return &g_ItemGenerationSystem;
}

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CItemGeneration::CItemGeneration( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Generate a random item matching the specified criteria
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateRandomItem( CItemSelectionCriteria *pCriteria, const Vector &vecOrigin, const QAngle &vecAngles, const char* pszOverrideClassName )
{
	entityquality_t iQuality;
	int iChosenItem = ItemSystem()->GenerateRandomItem( pCriteria, &iQuality );
	if ( iChosenItem == INVALID_ITEM_DEF_INDEX )
		return NULL;

	return SpawnItem( iChosenItem, vecOrigin, vecAngles, pCriteria->GetItemLevel(), iQuality, pszOverrideClassName );
}

//-----------------------------------------------------------------------------
// Purpose: Generate a random item matching the specified definition index
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateItemFromDefIndex( int iDefIndex, const Vector &vecOrigin, const QAngle &vecAngles )
{
	return SpawnItem( iDefIndex, vecOrigin, vecAngles, 1, AE_UNIQUE, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Generate an item from the specified item data
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateItemFromScriptData( const CEconItemView *pData, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszOverrideClassName )
{
	return SpawnItem( pData, vecOrigin, vecAngles, pszOverrideClassName );
}

//-----------------------------------------------------------------------------
// Purpose: Generate the base item for a class's loadout slot 
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::GenerateBaseItem( struct baseitemcriteria_t *pCriteria )
{
	int iChosenItem = ItemSystem()->GenerateBaseItem( pCriteria );
	if ( iChosenItem == INVALID_ITEM_DEF_INDEX )
		return NULL;

	return SpawnItem( iChosenItem, vec3_origin, vec3_angle, 1, AE_NORMAL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Create a new instance of the chosen item
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::SpawnItem( int iChosenItem, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, int iItemLevel, entityquality_t entityQuality, const char *pszOverrideClassName )
{
	CEconItemDefinition *pData = ItemSystem()->GetStaticDataForItemByDefIndex( iChosenItem );
	if ( !pData )
		return NULL;

	CBaseEntity* pItem = NULL;

	// Josh: Attempt to spawn with a pszOverrideClassName, otherwise, fallback
	// to the ItemClass of the EconItemDefinition.
	//
	// pszOverrideClassName exists now because of a fatal problem
	// related to disguising as classes with generic weapons (ie. stock shotgun)
	// where the item script calls the class name tf_weapon_shotgun (unlocalized for the class)
	// which doesn't exist, but we already know on the outer caller what the classname is, passed
	// in via pszOverrideClassName.
	if ( pszOverrideClassName )
		pItem = CreateEntityByName( pszOverrideClassName );

	if ( !pItem )
	{
		pszOverrideClassName = pData->GetItemClass();

		if ( !pszOverrideClassName )
			return NULL;

		pItem = CreateEntityByName( pszOverrideClassName );
	}

	if ( !pItem )
		return NULL;

	// Set the item level & quality
	IHasAttributes *pItemInterface = GetAttribInterface( pItem );
	Assert( pItemInterface );
	if ( pItemInterface )
	{
		// Setup the script item. Don't generate attributes here, because it'll be done during entity spawn.
		CEconItemView *pScriptItem = pItemInterface->GetAttributeContainer()->GetItem();
		pScriptItem->Init( iChosenItem, entityQuality, iItemLevel, false );
	}

	return PostSpawnItem( pItem, pItemInterface, vecAbsOrigin, vecAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Create a base entity for the specified item data
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::SpawnItem( const CEconItemView *pData, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles, const char *pszOverrideClassName )
{
	if ( !pData->GetStaticData() )
		return NULL;

	if ( !pszOverrideClassName )
	{
		pszOverrideClassName = pData->GetStaticData()->GetItemClass();
	}

	if ( !pszOverrideClassName )
		return NULL;

	CBaseEntity *pItem = CreateEntityByName( pszOverrideClassName );
	if ( !pItem )
		return NULL;

	// Set the item level & quality
	IHasAttributes *pItemInterface = GetAttribInterface( pItem );
	Assert( pItemInterface );
	if ( pItemInterface )
	{
		pItemInterface->GetAttributeContainer()->SetItem( pData );
	}

	return PostSpawnItem( pItem, pItemInterface, vecAbsOrigin, vecAbsAngles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CItemGeneration::PostSpawnItem( CBaseEntity *pItem, IHasAttributes *pItemInterface, const Vector &vecAbsOrigin, const QAngle &vecAbsAngles )
{
#ifdef CLIENT_DLL
	const char *pszPlayerModel = NULL;
	if ( pItemInterface )
	{
		CEconItemView *pScriptItem = pItemInterface->GetAttributeContainer()->GetItem();
		
		int iClass = 0;
		int iTeam = 0;
#ifdef TF_CLIENT_DLL
		C_TFPlayer *pTFPlayer = ToTFPlayer( GetPlayerByAccountID( pScriptItem->GetAccountID() ) );
		if ( pTFPlayer )
		{
			iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
			iTeam = pTFPlayer->GetTeamNumber();
		}
#endif // TF_CLIENT_DLL
		pszPlayerModel = pScriptItem->GetPlayerDisplayModel( iClass, iTeam );
	}

	// If we create a clientside item, we need to force it to initialize attributes
	if ( pItem->InitializeAsClientEntity( pszPlayerModel, RENDER_GROUP_OPAQUE_ENTITY ) == false )
		return NULL;
#endif // CLIENT_DLL

	pItem->SetAbsOrigin( vecAbsOrigin );
	pItem->SetAbsAngles( vecAbsAngles );

	pItem->Spawn();
	pItem->Activate();
	return pItem;
}

