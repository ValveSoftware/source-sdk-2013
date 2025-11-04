//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_item_interface.h"
#include "rtime.h"
#include "econ_item_schema.h"

// --------------------------------------------------------------------------
const char	*IEconItemInterface::GetDefinitionString( const char *pszKeyName, const char *pszDefaultValue ) const
{
	const GameItemDefinition_t *pDef = GetItemDefinition();
	if ( pDef )
		return pDef->GetDefinitionString( pszKeyName, pszDefaultValue );
	return pszDefaultValue;
}

EEconItemQuality IEconItemInterface::GetMarketQuality() const
{
	return (EEconItemQuality)GetQuality();
}


bool IEconItemInterface::BIsStrange() const
{
	return BIsItemStrange( this );
}

bool IEconItemInterface::BIsUnusual() const
{
	return ItemHasUnusualAttribute( this );
}

// --------------------------------------------------------------------------
KeyValues *IEconItemInterface::GetDefinitionKey( const char *pszKeyName ) const
{
	const GameItemDefinition_t *pDef = GetItemDefinition();
	if ( pDef )
		return pDef->GetDefinitionKey( pszKeyName );
	return NULL;
}

bool GetStattrak( const IEconItemInterface *pItem, CAttribute_String *pAttrModule /*= NULL*/ )
{
	// check if this can be stattrack
	static CSchemaAttributeDefHandle pAttribDef_StatModule( "weapon_uses_stattrak_module" );
	CAttribute_String attrModule;
	bool bRet = pAttribDef_StatModule && pItem->FindAttribute( pAttribDef_StatModule, &attrModule ) && attrModule.has_value();

	if ( pAttrModule )
	{
		*pAttrModule = attrModule;
	}

	return bRet;
}