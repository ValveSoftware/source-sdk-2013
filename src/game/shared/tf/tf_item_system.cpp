//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_system.h"
#include "econ_item_inventory.h"
#include "tf_item_inventory.h"

//-----------------------------------------------------------------------------
// Purpose: Generate the base item for a class's loadout slot 
//-----------------------------------------------------------------------------
item_definition_index_t CTFItemSystem::GenerateBaseItem( baseitemcriteria_t *pCriteria )
{
	Assert( pCriteria->iClass != 0 );
	Assert( pCriteria->iSlot != LOADOUT_POSITION_INVALID );

	// Some slots don't have base items (i.e. were added after launch)
	if ( !TFInventoryManager()->SlotContainsBaseItems( GEconItemSchema().GetEquipTypeFromClassIndex( pCriteria->iClass ), pCriteria->iSlot ) )
		return INVALID_ITEM_DEF_INDEX;

	CItemSelectionCriteria criteria;
	criteria.SetQuality( AE_NORMAL );
	criteria.SetItemLevel( 1 );
	criteria.BAddCondition( "baseitem", k_EOperator_String_EQ, "1", true );
	InventoryManager()->AddBaseItemCriteria( pCriteria, &criteria );
	int iChosenItem = GenerateRandomItem( &criteria, NULL );
	return iChosenItem;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItemSystem *TFItemSystem( void )
{
	CEconItemSystem *pItemSystem = ItemSystem();
	Assert( dynamic_cast<CTFItemSystem *>( pItemSystem ) != NULL );
	return (CTFItemSystem *)pItemSystem;
}