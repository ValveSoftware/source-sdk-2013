//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "econ_item_interface.h"
#include "econ_item_tools.h"				// needed for CEconTool_WrappedGift definition for IsMarketable()
#include "rtime.h"
#include "econ_paintkit.h"
#include "econ_item_schema.h"


// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool IEconItemInterface::IsTemporaryItem() const
{
	// store preview items are also temporary
	if ( GetOrigin() == kEconItemOrigin_PreviewItem )
		return true;

	static CSchemaAttributeDefHandle pLoanerIDLowAttrib( "quest loaner id low" );
	if ( !pLoanerIDLowAttrib )
		return false;

	static CSchemaAttributeDefHandle pLoanerIDHiAttrib( "quest loaner id hi" );
	if ( !pLoanerIDHiAttrib )
		return false;

	if ( FindAttribute( pLoanerIDLowAttrib ) || FindAttribute( pLoanerIDHiAttrib ) )
		return true;

	RTime32 rtTime = GetExpirationDate();
	if ( rtTime > 0 )
		return true;

	return false;
}

// --------------------------------------------------------------------------
RTime32 IEconItemInterface::GetExpirationDate() const
{
	COMPILE_TIME_ASSERT( sizeof( float ) == sizeof( RTime32 ) );

	// dynamic attributes, if present, will override any static expiration timer
	static CSchemaAttributeDefHandle pAttrib_ExpirationDate( "expiration date" );

	attrib_value_t unAttribExpirationTimeBits;
	COMPILE_TIME_ASSERT( sizeof( unAttribExpirationTimeBits ) == sizeof( RTime32 ) );

	if ( pAttrib_ExpirationDate && FindAttribute( pAttrib_ExpirationDate, &unAttribExpirationTimeBits ) )
		return *(RTime32 *)&unAttribExpirationTimeBits;

	// do we have a static timer set in the schema for all instances to expire?
	return GetItemDefinition()
		 ? GetItemDefinition()->GetExpirationDate()
		 : RTime32( 0 );
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
RTime32 IEconItemInterface::GetTradableAfterDateTime() const
{
	static CSchemaAttributeDefHandle pAttrib_TradableAfter( "tradable after date" );
	Assert( pAttrib_TradableAfter );

	if ( !pAttrib_TradableAfter )
		return 0;

	RTime32 rtTimestamp;
	if ( !FindAttribute( pAttrib_TradableAfter, &rtTimestamp ) )
		return 0;

	return rtTimestamp;
}

// --------------------------------------------------------------------------
// Purpose: Return true if this item can never be traded
// --------------------------------------------------------------------------
bool IEconItemInterface::IsPermanentlyUntradable() const
{
	if ( GetItemDefinition() == NULL )
		return true;

	// tagged to not be a part of the economy?
	if ( ( kEconItemFlag_NonEconomy & GetFlags() ) != 0 )
		return true;

	// check attributes
	
	static CSchemaAttributeDefHandle pAttrib_AlwaysTradable( "always tradable" );
	static CSchemaAttributeDefHandle pAttrib_CannotTrade( "cannot trade" );
	static CSchemaAttributeDefHandle pAttrib_NonEconomy( "non economy" );
	
	Assert( pAttrib_AlwaysTradable != NULL );
	Assert( pAttrib_CannotTrade != NULL );

	if ( pAttrib_AlwaysTradable == NULL || pAttrib_CannotTrade == NULL || pAttrib_NonEconomy == NULL )
		return true;

	// Order matters, check for nonecon first.  Always tradable overrides cannot trade.
	if ( FindAttribute( pAttrib_NonEconomy ) )			
		return true;

	if ( FindAttribute( pAttrib_AlwaysTradable ) )		// *sigh*
		return false;

	if ( FindAttribute( pAttrib_CannotTrade ) )
		return true;

	// items gained in this way are not tradable
	switch ( GetOrigin() )
	{
	case kEconItemOrigin_Invalid:
	case kEconItemOrigin_Achievement:
	case kEconItemOrigin_Foreign:
	case kEconItemOrigin_PreviewItem:
	case kEconItemOrigin_SteamWorkshopContribution:
	case kEconItemOrigin_UntradableFreeContractReward:
		return true;
	}

	// temporary items (items that will expire for any reason) cannot be traded
	if ( IsTemporaryItem() )
		return true;

	// certain quality levels are not tradable
	if ( GetQuality() >= AE_COMMUNITY && GetQuality() <= AE_SELFMADE )
		return true;

	// explicitly marked cannot trade?
	if ( ( kEconItemFlag_CannotTrade & GetFlags() ) != 0 )
		return true;

	return false;
}

// --------------------------------------------------------------------------
// Purpose: Return true if this item is a commodity on the Market (can place buy orders)
// --------------------------------------------------------------------------
bool IEconItemInterface::IsCommodity() const
{
	if ( GetItemDefinition() == NULL )
		return false;

	static CSchemaAttributeDefHandle pAttrib_IsCommodity( "is commodity" );
	uint32 nIsCommodity = 0;
	if ( FindAttribute( pAttrib_IsCommodity, &nIsCommodity ) && nIsCommodity != 0 )
		return true;

	return false;
}

// --------------------------------------------------------------------------
// Purpose: Return true if temporarily untradable
// --------------------------------------------------------------------------
bool IEconItemInterface::IsTemporarilyUntradable() const
{
	// Temporary untradability does NOT take "always tradable" into account
	if ( GetTradableAfterDateTime() >= CRTime::RTime32TimeCur() )
		return true;

	return false;
}

// --------------------------------------------------------------------------
// Purpose: Return true if this item is untradable
// --------------------------------------------------------------------------
bool IEconItemInterface::IsTradable() const
{
	// Items that are expired are never listable, regardless of other rules.
	//RTime32 timeExpirationDate = GetExpirationDate();
	//if ( timeExpirationDate > 0 && timeExpirationDate < CRTime::RTime32TimeCur() )
	//	return false;

	return GetUntradabilityFlags() == 0;
}

// --------------------------------------------------------------------------
// Purpose: Return untradability flags
// --------------------------------------------------------------------------
int IEconItemInterface::GetUntradabilityFlags() const
{
	int nFlags = 0;
	if ( IsTemporarilyUntradable() )
	{
		nFlags |= k_Untradability_Temporary;
	}
	
	if ( IsPermanentlyUntradable() )
	{
		nFlags |= k_Untradability_Permanent;
	}

	return nFlags;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool IEconItemInterface::IsUsableInCrafting() const
{
	if ( GetItemDefinition() == NULL )
		return false;

	// tagged to not be a part of the economy?
	if ( ( kEconItemFlag_NonEconomy & GetFlags() ) != 0 )
		return false;

	// always craftable?
	static CSchemaAttributeDefHandle pAttrib_AlwaysUsableInCraft( "always tradable" );
	Assert( pAttrib_AlwaysUsableInCraft );

	if ( FindAttribute( pAttrib_AlwaysUsableInCraft ) )
		return true;

	// never craftable?
	static CSchemaAttributeDefHandle pAttrib_NeverCraftable( "never craftable" );
	Assert( pAttrib_NeverCraftable );

	if ( FindAttribute( pAttrib_NeverCraftable ) )
		return false;

	// temporary items (items that will expire for any reason) cannot be turned into
	// permanent items
	if ( IsTemporaryItem() )
		return false;

	// explicitly marked not usable in crafting?
	if ( ( kEconItemFlag_CannotBeUsedInCrafting & GetFlags() ) != 0 )
		return false;

	// items gained in this way are not craftable
	switch ( GetOrigin() )
	{
	case kEconItemOrigin_Invalid:
	case kEconItemOrigin_Foreign:
	case kEconItemOrigin_StorePromotion:
	case kEconItemOrigin_SteamWorkshopContribution:
		return false;

	// purchased items can be used in crafting if explicitly tagged, but not by default
	case kEconItemOrigin_Purchased:
		// deny items the GC didn't flag at purchase time
		if ( (GetFlags() & kEconItemFlag_PurchasedAfterStoreCraftabilityChanges2012) == 0 )
			return false;

		// deny items that can never be used
		if ( (GetItemDefinition()->GetCapabilities() & ITEM_CAP_CAN_BE_CRAFTED_IF_PURCHASED) == 0 )
			return false;

		break;
	}

	// certain quality levels are not craftable
	if ( GetQuality() >= AE_COMMUNITY && GetQuality() <= AE_SELFMADE )
		return false;

	return true;
}

// --------------------------------------------------------------------------
// Purpose:
// --------------------------------------------------------------------------
bool IEconItemInterface::IsMarketable() const
{
	const CEconItemDefinition *pItemDef = GetItemDefinition();
	if ( pItemDef == NULL )
		return false;

	// Untradeable items can never be marketed, regardless of other rules.
	// Temporarily untradable items can be marketed, only permanent untradable items cannot be marketed
	if ( IsPermanentlyUntradable() )
		return false;

	// Items that are expired are never listable, regardless of other rules.
	RTime32 timeExpirationDate = GetExpirationDate();
	if ( timeExpirationDate > 0 && timeExpirationDate < CRTime::RTime32TimeCur() )
		return false;

	// Initially, only TF2 supports listing items in the Marketplace.
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL ) || defined( TF_GC_DLL )
	{
		// User-created wrapped gifts are untradeable for the moment. This would provide a backdoor
		// for users to sell anything they wanted, which is interesting but not what we want in
		// the initial launch.
		if ( pItemDef->GetTypedEconTool<CEconTool_WrappedGift>() )
			return false;

		// All other tools are listable. This includes keys, paints, backpack expanders, strange
		// parts, Halloween spells, wedding rings, etc. It does not includes gifts (see above),
		// noisemakers, or crates (see below).
		if ( pItemDef->IsTool() )
			return true;

		// All crates are listable. Anything with the "decodable" flag is considered a crate.
		if ( (pItemDef->GetCapabilities() & ITEM_CAP_DECODABLE) != 0 )
			return true;

		// Genuine-quality items come from time-limited purchase promos and are listable. Vintage
		// items are from one-time transitions and are all finite quality. Haunted quality items are
		// TF-Halloween-event specific. Some of the older haunted items didn't generate revenue, but
		// the content is all old and there seems to be little harm in letting it be listed. The
		// haunted items from 2013 all come from crates, which means they all generated revenue.
		// Collectors items are created from a finite set of recipes.
		// Paintkit Weapons are from cases or operations
		if ( GetQuality() == AE_RARITY1 || GetQuality() == AE_VINTAGE || GetQuality() == AE_HAUNTED
			|| GetQuality() == AE_COLLECTORS || GetQuality() == AE_PAINTKITWEAPON )
			return true;

		// All festive items are from time-limited holiday crates and are listable. This code seems
		// safe. (...) (This code is in fact so safe that if we just do a substring match we'll also
		// allow "A Rather Festive Tree".)
		if ( !V_strncmp( pItemDef->GetDefinitionName(), "Festive", 7 ) )
			return true;

		// All botkiller items come from MvM rewards and are listable. This does a substring search
		// to find all varieties (gold, silver, rust, etc.), etc.
		if ( V_strstr( pItemDef->GetDefinitionName(), " Botkiller " ) )
			return true;

		// Mvm V2 Robit Parts
		if ( V_strstr( pItemDef->GetDefinitionName(), "Robits " ) )
			return true;

		// MvM Killstreak Weapons
		static CSchemaAttributeDefHandle pAttr_killstreak( "killstreak tier" );
		if ( FindAttribute( pAttr_killstreak ) )
			return true;

		// Australium Items
		static CSchemaAttributeDefHandle pAttrDef_IsAustralium( "is australium item" );
		if ( FindAttribute( pAttrDef_IsAustralium ) )
			return true;

		// Glitch GateHat Replacement Item
		static CSchemaItemDefHandle pItemDef_GlitchedCircuit( "Glitched Circuit Board" );
		if ( pItemDef == pItemDef_GlitchedCircuit )
			return true;

		// Anything that says it wants to be marketable.
		static CSchemaAttributeDefHandle pAttrDef_IsMarketable( "is marketable" );
		if ( FindAttribute( pAttrDef_IsMarketable ) )
			return true;

		// Anything that is of limited quantity (ie limited promos)
		static CSchemaAttributeDefHandle pAttrDef_IsLimited( "limited quantity item" );
		if ( FindAttribute( pAttrDef_IsLimited ) )
			return true;

		// Allow the Giving items (not a wrapped_gift but a gift, ie Secret Saxton, Pile O Gifts, Pallet of Keys)
		const CEconTool_Gift *pEconToolGift = pItemDef->GetTypedEconTool<CEconTool_Gift>();
		if ( pEconToolGift )
			return true;

		// Unusual Cosmetics and Taunts
		if ( GetQuality() == AE_UNUSUAL && ( GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_MISC || GetItemDefinition()->GetLoadoutSlot( 0 ) == LOADOUT_POSITION_TAUNT ) )
			return true;

		// Strange items.  Dont just check for strange quality, actually check for a strange attribute.
		// See if we've got any strange attributes.
		for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
		{
			if ( FindAttribute( GetKillEaterAttr_Score( i ) ) )
			{
				return true;
			}
		}
	}
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL ) || defined( TF_GC_DLL )

	// By default, items aren't listable.
	return false;
}

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