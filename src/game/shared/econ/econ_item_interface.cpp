//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_item_interface.h"
#include "rtime.h"
#include "econ_paintkit.h"
#include "econ_item_schema.h"

// --------------------------------------------------------------------------
const char	*IEconItemInterface::GetDefinitionString( const char *pszKeyName, const char *pszDefaultValue ) const
{
	const GameItemDefinition_t *pDef = GetItemDefinition();
	if ( pDef )
		return pDef->GetDefinitionString( pszKeyName, pszDefaultValue );
	return pszDefaultValue;
}

uint8 IEconItemInterface::GetRarity() const
{
	const CEconItemDefinition* pRarityItemDef = GetItemDefinition();
	uint32 unPaintKitDefIndex = 0;
	auto pCollection = GetItemSchema()->GetPaintKitCollectionFromItem( this, &unPaintKitDefIndex );
	if ( pCollection )
	{
		// treat this item as the paintkit tool
		pRarityItemDef = GetItemSchema()->GetPaintKitItemDefinition( unPaintKitDefIndex );
		Assert( pRarityItemDef );
	}

	if ( pRarityItemDef )
		return pRarityItemDef->GetRarity();

	return AE_UNIQUE;
}

EEconItemQuality IEconItemInterface::GetMarketQuality() const
{
	const bool bIsPaintkit = GetPaintKitDefIndex( this );
	// Paintkits need to do some special checks.  Everything else can
	// just return the regular GetQuality()
	if ( !bIsPaintkit )
	{
		return (EEconItemQuality)GetQuality();
	}

	// Paintkit items need to return AE_PAINTKITWEAPON if they're not
	// Self-Made, Unusual, or Strange.
	//
	if ( GetQuality() == AE_SELFMADE )
	{
		return AE_SELFMADE;
	}

	// Unusual is more valuable than Strange, so check it first
	if ( BIsUnusual() )
	{
		return AE_UNUSUAL;
	}
	
	if ( BIsStrange() )
	{
		return AE_STRANGE;
	}

	// By default, return AE_PAINTKITWEAPON, regardless of what our item
	// actually has set.
	return AE_PAINTKITWEAPON;
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


bool GetPaintKitWear( const IEconItemInterface *pItem, float &flWear )
{	

	static CSchemaAttributeDefHandle pAttrDef_PaintKitWear( "set_item_texture_wear" );
	float flPaintKitWear = 0;
	if ( pAttrDef_PaintKitWear && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrDef_PaintKitWear, &flPaintKitWear ) )
	{
		flWear = flPaintKitWear;
		return true;
	}

	static CSchemaAttributeDefHandle pAttrDef_DefaultWear( "texture_wear_default" );
	if ( pAttrDef_DefaultWear && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrDef_DefaultWear, &flPaintKitWear ) )
	{
		flWear = flPaintKitWear;
		return true;
	}

	bool bHasPaintkit = GetPaintKitDefIndex( pItem );

	// If you have no wear, you also should not have a paint kit
	AssertMsg( !bHasPaintkit, "No Wear Found on Item [%llu - %s] that has a Paintkit!", pItem->GetID(), pItem->GetItemDefinition()->GetDefinitionName() );

	return bHasPaintkit;
}

bool GetStattrak( const IEconItemInterface *pItem, CAttribute_String *pAttrModule /*= NULL*/ )
{
	// only paintkited item can have stattrack
	if ( !GetPaintKitDefIndex( pItem ) )
	{
		return false;
	}

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

const char *GetPaintKitMaterialOverride( const IEconItemInterface *pItem )
{
	uint32 unPaintKitDefIndex = 0;
	if ( GetPaintKitDefIndex( pItem, &unPaintKitDefIndex ) )
	{
		const CPaintKitDefinition* pPaintKitDef = assert_cast< const CPaintKitDefinition* >( GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( DEF_TYPE_PAINTKIT_DEFINITION, unPaintKitDefIndex ) ) );
		if ( pPaintKitDef )
		{
			const char *pszMaterialOverride = pPaintKitDef->GetMaterialOverride( pItem->GetItemDefIndex() );
			if ( pszMaterialOverride )
			{
				return pszMaterialOverride;
			}
		}
	}

	return NULL;
}

const CEconItemCollectionDefinition* GetCollection( const IEconItemInterface* pItem )
{
	auto pItemDef = pItem->GetItemDefinition();
	if ( !pItemDef )
	{
		Assert( false );
		return NULL;
	}

	const CEconItemCollectionDefinition *pCollection = pItemDef->GetItemCollectionDefinition();
	if ( pCollection )
	{
		return pCollection;
	}

	// see if this is part of paintkit collection
	uint32 unPaintKitDefIndex = 0;
	pCollection = GetItemSchema()->GetPaintKitCollectionFromItem( pItem, &unPaintKitDefIndex );
	if ( pCollection )
	{
		// treat this item as the paintkit tool
		auto pPaintkitItemDef = GetItemSchema()->GetPaintKitItemDefinition( unPaintKitDefIndex );
		Assert( pPaintkitItemDef );
		if ( pPaintkitItemDef )
		{
			return pPaintkitItemDef->GetItemCollectionDefinition();
		}
	}

	return NULL;
}