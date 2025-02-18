//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Load item upgrade data from KeyValues
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "tf_shareddefs.h"
#include "tf_upgrades.h"
#include "tf_upgrades_shared.h"
#include "econ_item_system.h"
#include "dt_utlvector_send.h"
#include "tf_player.h"
#include "econ_wearable.h"
#include "tf_item_powerup_bottle.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_weapon_wrench.h"
#include "tf_weapon_builder.h"
#include "tf_objective_resource.h"


extern ConVar tf_mvm_skill;
extern ConVar *sv_cheats;

CHandle<CUpgrades>	g_hUpgradeEntity;


BEGIN_DATADESC( CUpgrades )
	DEFINE_KEYFIELD( m_nStartDisabled, FIELD_INTEGER, "start_disabled" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_ENTITYFUNC( UpgradeTouch ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_upgradestation, CUpgrades );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::Spawn( void )
{
	BaseClass::Spawn();

	// Don't do anything if we don't have raid mode.
	g_hUpgradeEntity = this;

	AddSpawnFlags(SF_TRIGGER_ALLOW_CLIENTS);

	InitTrigger();

	SetTouch( &CUpgrades::UpgradeTouch );

	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "teamplay_round_start" );

	m_bIsEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputEnable( inputdata_t &inputdata )
{
	m_bIsEnabled = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputDisable( inputdata_t &inputdata )
{
	m_bIsEnabled = false;

	int iCount = m_hTouchingEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CBaseEntity *pEntity = m_hTouchingEntities[i];
		if ( pEntity && pEntity->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pEntity );
			if ( pTFPlayer )
			{
				pTFPlayer->m_Shared.SetInUpgradeZone( false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::InputReset( inputdata_t &inputdata )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::FireGameEvent( IGameEvent *gameEvent )
{
	if ( FStrEq( gameEvent->GetName(), "round_start" ) || FStrEq( gameEvent->GetName(), "teamplay_round_start" ) )
	{
		// Enable/disable based on round state
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::UpgradeTouch( CBaseEntity *pOther )
{
	if ( m_bIsEnabled )
	{
		if ( PassesTriggerFilters(pOther) )
		{
			if ( pOther->IsPlayer() )
			{
				CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
				pTFPlayer->m_Shared.SetInUpgradeZone( true );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::EndTouch( CBaseEntity *pOther )
{
	if ( IsTouching( pOther ) )
	{
		if ( pOther->IsPlayer() )
		{
			CTFPlayer *pTFPlayer = ToTFPlayer( pOther );
			pTFPlayer->m_Shared.SetInUpgradeZone( false );
		}
	}

	BaseClass::EndTouch( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::GrantOrRemoveAllUpgrades( CTFPlayer *pTFPlayer, bool bRemove /*= false*/, bool bRefund /*= true*/ )
{
	// If we're being asked to remove and refund everything, it's a respec (the population manager actually handles refunding later)
	bool bRespec = bRemove && bRefund;

	if ( pTFPlayer && ( ( sv_cheats && sv_cheats->GetBool() ) || bRemove ) )
	{
		pTFPlayer->BeginPurchasableUpgrades();

		// for each upgrade
		for ( int iUpgrade = 0 ; iUpgrade < g_MannVsMachineUpgrades.m_Upgrades.Count() ; iUpgrade++ )
		{
			CMannVsMachineUpgrades upgrade = g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ];
			CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( upgrade.szAttrib );

			// don't process bottle upgrades
			if ( upgrade.nUIGroup == UIGROUP_POWERUPBOTTLE )
				continue;

			if ( pAttribDef )
			{
				loadout_positions_t nLastLoadoutPos = bRespec ? LOADOUT_POSITION_MISC2 : LOADOUT_POSITION_HEAD;
				// for each item
				for ( int iItemSlot = LOADOUT_POSITION_PRIMARY ; iItemSlot < nLastLoadoutPos ; iItemSlot++ )	
				{
					// Don't respec bottle charges
					if ( bRespec && iItemSlot == LOADOUT_POSITION_ACTION )
						continue;

					// can this item use this upgrade?
					if ( bRespec || ( TFGameRules() && TFGameRules()->CanUpgradeWithAttrib( pTFPlayer, iItemSlot, pAttribDef->GetDefinitionIndex(), &upgrade ) ) )
					{
						// If we're not removing, assume we're giving the player everything for free (cheat)
						bool bFree = ( !bRemove || !bRefund ) ? true : false;
						
						// compute number of upgrade steps this upgrade has
						bool bOverCap = false;
						int iCurrentUpgradeStep = 0;
						const int iNumMaxUpgradeStep = GetUpgradeStepData( pTFPlayer, iItemSlot, iUpgrade, iCurrentUpgradeStep, bOverCap );

						// for each upgrade step
						for ( int iStep = 0; iStep < iNumMaxUpgradeStep; ++iStep )
						{
							PlayerPurchasingUpgrade( pTFPlayer, iItemSlot, iUpgrade, bRemove, bFree, bRespec );
						}
					}
				}
			}
		}
		pTFPlayer->EndPurchasableUpgrades();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles a player upgrade purchase request.
//   Returns false if upgrade request is invalid.
//-----------------------------------------------------------------------------
bool CUpgrades::PlayerPurchasingUpgrade( CTFPlayer *pTFPlayer, int iItemSlot, int iUpgrade, bool bDowngrade, bool bFree /*= false */, bool bRespec /*= false*/ )
{
	if ( !pTFPlayer ||
		 iUpgrade < 0 ||
		 iUpgrade >= g_MannVsMachineUpgrades.m_Upgrades.Count() )
	{
		return false;
	}

	// Verify that this upgrade can be accepted on this player
	CMannVsMachineUpgrades upgrade = g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ];
	CEconItemAttributeDefinition *pAttribDef = ItemSystem()->GetStaticDataForAttributeByName( upgrade.szAttrib );
	if ( !bRespec && ( !TFGameRules() || !TFGameRules()->CanUpgradeWithAttrib( pTFPlayer, iItemSlot, pAttribDef->GetDefinitionIndex(), &upgrade ) ) )
	{
		return false;
	}

	int nCost = 0;

	if ( bDowngrade )
	{
		// See if we know the actual price paid, rather than asking what the current price is, which is exploitable
		CUtlVector< CUpgradeInfo > *pUpgrades = pTFPlayer->GetRefundableUpgrades();
		if ( pUpgrades && pUpgrades->Count() )
		{
			FOR_EACH_VEC( *pUpgrades, i )
			{
				CUpgradeInfo *pInfo = &pUpgrades->Element( i );
				if ( pInfo && pInfo->m_upgrade == iUpgrade )
				{
					nCost = pInfo->m_nCost;
					break;
				}
			}
		}
	}
	if ( !nCost )
	{
		nCost = TFGameRules()->GetCostForUpgrade( &g_MannVsMachineUpgrades.m_Upgrades[iUpgrade], iItemSlot, pTFPlayer->GetPlayerClass()->GetClassIndex(), pTFPlayer );
	}
	if ( bDowngrade )
	{
		nCost *= -1;
	}

	if ( !bFree )
	{
		// Make sure the player can afford it
		if ( pTFPlayer->GetCurrency() < nCost )
			{ return false; }
	}

	CEconItemView *pItem = NULL;

	// Make sure the item slot is correct for attributes that need to attach to an item
	if ( g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ].nUIGroup != UIGROUP_UPGRADE_ATTACHED_TO_PLAYER )
	{
		if ( !( iItemSlot == LOADOUT_POSITION_ACTION || ( iItemSlot >= LOADOUT_POSITION_PRIMARY && iItemSlot <= LOADOUT_POSITION_PDA2 ) ) )
			{ return false; }

		pItem = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pTFPlayer, iItemSlot );
	}

	if ( bDowngrade )
	{
		if ( bRespec )
		{
			// Approve everything and don't refund currency here.  The population manager handles that later.
			nCost = 0;
		}
		else
		{
			bool bCanSell = false;

			// Before we sell anything, make sure it's verified as valid
			item_definition_index_t iItemIndex = pItem ? pItem->GetItemDefIndex() : INVALID_ITEM_DEF_INDEX;

			for ( int i = 0; i < pTFPlayer->GetRefundableUpgrades()->Count(); ++i )
			{
				CUpgradeInfo pInfo = pTFPlayer->GetRefundableUpgrades()->Element( i );
				if ( ( pInfo.m_iPlayerClass == pTFPlayer->GetPlayerClass()->GetClassIndex() ) && ( pInfo.m_itemDefIndex == iItemIndex ) && ( pInfo.m_upgrade == iUpgrade ) )
				{
					// Found a matching upgrade that we can sell
					nCost = ( pInfo.m_nCost * -1 );
					bCanSell = true;
					break;
				}
			}

			if ( !bCanSell )
			{
				// No matched recent purchases!
				// No sale!
				return false;
			}
		}
	}
	else
	{
		// If the upgrade has a tier, it's mutually exclusive with upgrades of the same tier for the same itemslot
		int nTier = TFGameRules()->GetUpgradeTier( iUpgrade );
		if ( nTier && !TFGameRules()->IsUpgradeTierEnabled( pTFPlayer, iItemSlot, iUpgrade ) )
			{ return false; }
	}

	const attrib_definition_index_t nUpgradedAttrDefIndex = ApplyUpgradeToItem( pTFPlayer, pItem, iUpgrade, nCost, bDowngrade, !bFree );

	// Failed to apply
	if ( nUpgradedAttrDefIndex == INVALID_ATTRIB_DEF_INDEX )
		{ return false; }

	if ( !bFree )
	{
		// Remove Currency
		pTFPlayer->RemoveCurrency( nCost );
	}

	// remember our upgrades so we can restore them at a checkpoint
	pTFPlayer->RememberUpgrade( pTFPlayer->GetPlayerClass()->GetClassIndex(), pItem, iUpgrade, nCost, bDowngrade );

	// Only regenerate if between waves
	pTFPlayer->Regenerate( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() );

	// If we're upgrading an item, figure out if it's a weapon and then notify it (gives it a chance to re-hook attributes)
	if ( pItem )
	{
		for ( int i = 0; i < MAX_WEAPONS; i++ )
		{
			CTFWeaponBase *pWeapon = (CTFWeaponBase *)pTFPlayer->GetWeapon( i );
			if ( !pWeapon )
				continue;

			if ( pWeapon->GetAttributeContainer()->GetItem() == pItem )
			{
				pWeapon->OnUpgraded();
			}
		}
	}

	// See if we need to notify items about an upgrade
	NotifyItemOnUpgrade( pTFPlayer, nUpgradedAttrDefIndex, bDowngrade );

	// Upgrade succeeded
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static attrib_definition_index_t ApplyUpgrade_Bottle( int iUpgrade, CTFPlayer *pTFPlayer, CEconItemView *pEconItemView, int nCost, bool bDowngrade )
{
	Assert( pTFPlayer );

	if ( !pEconItemView )
		return INVALID_ATTRIB_DEF_INDEX;

	const CMannVsMachineUpgrades& upgrade = g_MannVsMachineUpgrades.m_Upgrades[iUpgrade];

	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttrib );
	if ( !pAttrDef )
		return INVALID_ATTRIB_DEF_INDEX;

	CTFWearable *pWearable = pTFPlayer->GetEquippedWearableForLoadoutSlot( LOADOUT_POSITION_ACTION );
	CTFPowerupBottle *pPowerupBottle = dynamic_cast< CTFPowerupBottle * >( pWearable );
	if ( !pPowerupBottle )
		return INVALID_ATTRIB_DEF_INDEX;

	Assert( pPowerupBottle->GetAttributeContainer()->GetItem() == pEconItemView );

	const bool bMvM = TFGameRules() && TFGameRules()->IsMannVsMachineMode();
	Assert( !bMvM || g_pPopulationManager );

	CAttributeList *pAttrList = pEconItemView->GetAttributeList();
	Assert( pAttrList );

	// Attribute doesn't exist, remove any other attributes -- this code assumes that this is the
	// only code path by which bottles will manipulate on-item attributes!
	if ( !::FindAttribute( pAttrList, pAttrDef ) )
	{
		// Can't downgrade an attribute that doesn't exist.
		if ( bDowngrade )
			return INVALID_ATTRIB_DEF_INDEX;

		// Remove the old attributes
		pPowerupBottle->RemoveEffect();
		pAttrList->DestroyAllAttributes();

		// Forget charges for other attributes and refund player money
		if ( bMvM )
		{
			g_pPopulationManager->ForgetOtherBottleUpgrades( pTFPlayer, pEconItemView, iUpgrade );
		}

		// set this afterwards, since it may alter attributes
		pPowerupBottle->SetNumCharges( 1 );

		pAttrList->SetRuntimeAttributeValue( pAttrDef, upgrade.flIncrement );
		pAttrList->SetRuntimeAttributeRefundableCurrency( pAttrDef, nCost );
	}			
	// attribute exists, just increase number of charges
	else
	{
		const int nChange = bDowngrade ? -1 : 1;
		const int nNewCharges =  pPowerupBottle->GetNumCharges() + nChange;

		// is the bottle full?
		if ( nNewCharges < 0 || nNewCharges > pPowerupBottle->GetMaxNumCharges() )
			return INVALID_ATTRIB_DEF_INDEX;

		pPowerupBottle->SetNumCharges( nNewCharges );

#ifdef DBGFLAG_ASSERT
		float flExistingValue;
		Assert( ::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrDef, &flExistingValue ) );
		Assert( AlmostEqual( flExistingValue, upgrade.flIncrement ) );
#endif // DBGFLAG_ASSERT
		pAttrList->AdjustRuntimeAttributeRefundableCurrency( pAttrDef, nCost * nChange );
		
		if ( nNewCharges == 0 )
		{
			Assert( bDowngrade );

			// Downgraded to 0... remove attributes
			pPowerupBottle->RemoveEffect();
			pAttrList->DestroyAllAttributes();

			// Forget charges for other attributes and refund player money
			if ( bMvM )
			{
				g_pPopulationManager->ForgetOtherBottleUpgrades( pTFPlayer, pEconItemView, iUpgrade );
			}
		}
	}

	return pAttrDef->GetDefinitionIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static attrib_definition_index_t ApplyUpgrade_Default( const CMannVsMachineUpgrades& upgrade, CTFPlayer *pTFPlayer, CEconItemView *pEconItemView, int nCost, bool bDowngrade )
{
	Assert( pTFPlayer );
	Assert( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER || upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM );
	// Assert( pEconItemView || upgrade.nUIGroup != UIGROUP_UPGRADE_ATTACHED_TO_ITEM );		// ugh, if loadouts change behind the scenes or if we have bugs elsewhere, we might
																							// feed an empty slot in here -- check for this case below if ATTACHED_TO_ITEM
	
	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttrib );
	if ( !pAttrDef )
		return INVALID_ATTRIB_DEF_INDEX;

	Assert( !pAttrDef->BIsSetBonusAttribute() );

	
	CAttributeList *pAttrList = upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER
							  ? pTFPlayer->GetAttributeList()
							  : pEconItemView->GetAttributeList();
	Assert( pAttrList );
	
	// ...
	float fDefaultValue = pAttrDef->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_PERCENTAGE || pAttrDef->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE
						? 1.0f
						: 0.0f;

	// ...
	if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM )
	{
		// If we're trying to attach to an item and we don't have an item to attach to, give up.
		if ( !pEconItemView )
			return INVALID_ATTRIB_DEF_INDEX;

		::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pEconItemView->GetItemDefinition(), pAttrDef, &fDefaultValue );
	}
	
	// if the attribute exists, add the increment (but not if it's a set bonus attribute, they're recreated on each respawn)
	float flIncrement = upgrade.flIncrement;

	float flCurrentValue;
	if ( ::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrDef, &flCurrentValue ) )
	{
		const float flCap = upgrade.flCap;

		if ( !bDowngrade && BIsAttributeValueWithDeltaOverCap( flCurrentValue, flIncrement, flCap ) )
			return INVALID_ATTRIB_DEF_INDEX;

		// Add the increment
		float flNewValue = 0.0f;
		
		if ( bDowngrade )
		{
			float flInitialValue = fDefaultValue;
			flIncrement = fabsf( flIncrement );

			if ( AlmostEqual( flCurrentValue, flCap ) && !AlmostEqual( flInitialValue, fDefaultValue ) )
			{
				// If we're at the cap and the initial value isn't normal, we might need to do a smaller increment
				// This is because incrementing from the initial value in steps would have gone past the hard cap
				float flStart = flInitialValue;
				
				if ( flIncrement > 0 )
				{
					while ( flStart < flCap && !AlmostEqual( flStart, flCap ) )
					{
						flStart += flIncrement;
					}
				}
				else
				{
					while ( flStart > flCap && !AlmostEqual( flStart, flCap ) )
					{
						flStart += flIncrement;
					}
				}

				const float flDiff = fabsf( flCap - flStart );
				if ( !AlmostEqual( flIncrement, flDiff ) )
				{
					flIncrement -= flDiff;
				}
			}

			flNewValue = Approach( flInitialValue, flCurrentValue, flIncrement );

			// We downgraded back to the point of not needing the attribute
			if ( AlmostEqual( flNewValue, flInitialValue ) )
			{
				Assert( bDowngrade );

				pAttrList->RemoveAttribute( pAttrDef );
				return pAttrDef->GetDefinitionIndex();
			}
		}
		else
		{
			flNewValue = Approach( flCap, flCurrentValue, fabsf( flIncrement ) );
		}

		pAttrList->SetRuntimeAttributeValue( pAttrDef, flNewValue );
		pAttrList->AdjustRuntimeAttributeRefundableCurrency( pAttrDef, nCost );
		return pAttrDef->GetDefinitionIndex();
	}

	if ( bDowngrade )
	{
		// Can't downgrade an attribute we didn't find
		return INVALID_ATTRIB_DEF_INDEX;
	}

	// Didn't exist, so we need to add the attribute.

	// Convert the increment into an actual multiplier amount
	pAttrList->SetRuntimeAttributeValue( pAttrDef, fDefaultValue + flIncrement );
	pAttrList->SetRuntimeAttributeRefundableCurrency( pAttrDef, nCost );

	// For NotifyItemOnUpgrade() - can't do it here because we Regenerate() downstream
	return pAttrDef->GetDefinitionIndex();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
attrib_definition_index_t CUpgrades::ApplyUpgradeToItem( CTFPlayer *pTFPlayer, CEconItemView *pView, int iUpgrade, int nCost, bool bDowngrade, bool bIsFresh )
{
	Assert( pTFPlayer );
	Assert( g_MannVsMachineUpgrades.m_Upgrades.IsValidIndex( iUpgrade ) );

	if ( !pTFPlayer || !pTFPlayer->CanPurchaseUpgrades() )
		return INVALID_ATTRIB_DEF_INDEX;

	const CMannVsMachineUpgrades& upgrade = g_MannVsMachineUpgrades.m_Upgrades[iUpgrade];

	const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName( upgrade.szAttrib );
	if ( !pAttrDef )
		return INVALID_ATTRIB_DEF_INDEX;

	bool bIsBottle = upgrade.nUIGroup == UIGROUP_POWERUPBOTTLE;

	ReportUpgrade( 
		pTFPlayer, 
		pView ? pView->GetItemDefIndex() : 0, 
		pAttrDef->GetDefinitionIndex(), 
		upgrade.nQuality, 
		nCost,
		bDowngrade, 
		bIsFresh,
		bIsBottle
	);

	// ...powerup bottle?
	if ( bIsBottle )
		return ApplyUpgrade_Bottle( iUpgrade, pTFPlayer, pView, nCost, bDowngrade );

	// ...player upgrade?
	if ( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_PLAYER )
		return ApplyUpgrade_Default( upgrade, pTFPlayer, pView, nCost, bDowngrade );

	Assert( upgrade.nUIGroup == UIGROUP_UPGRADE_ATTACHED_TO_ITEM );
	return ApplyUpgrade_Default( upgrade, pTFPlayer, pView, nCost, bDowngrade );
}


//-----------------------------------------------------------------------------
// Purpose: Given an upgrade index, return it's associated attribute description
//-----------------------------------------------------------------------------
const char *CUpgrades::GetUpgradeAttributeName( int iUpgrade ) const
{
	if ( iUpgrade < 0 || iUpgrade >= g_MannVsMachineUpgrades.m_Upgrades.Count() )
		return NULL;

	return g_MannVsMachineUpgrades.m_Upgrades[ iUpgrade ].szAttrib;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CUpgrades::NotifyItemOnUpgrade( CTFPlayer *pTFPlayer, attrib_definition_index_t nAttrDefIndex, bool bDowngrade /*= false*/ )
{
	if ( !pTFPlayer )
		return;

	switch( nAttrDefIndex )
	{
	case 286:	// "engy building health bonus"
		{
			// Tell the wrench we've upgraded our object health (which handles the rest)
			CTFWrench *pWrench = dynamic_cast<CTFWrench *>( pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_WRENCH ) );
			if ( pWrench )
			{
				pWrench->ApplyBuildingHealthUpgrade();
			}
		}
		break;
	case 320:	// "robot sapper"
		{
			// Sets the UI active
			CTFWeaponBuilder *pBuilder = dynamic_cast<CTFWeaponBuilder *>( pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );
			if ( pBuilder )
			{
				pBuilder->m_bRoboSapper.Set( true );
			}
		}
		break;
	case 351:
		if ( bDowngrade )
		{
			// if we're refunding the engy_disposable_sentries attribute we need to destroy the disposable sentry if we have one
			if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
			{
				for ( int i = pTFPlayer->GetObjectCount() - 1; i >= 0; i-- )
				{
					CBaseObject *pObj = pTFPlayer->GetObject( i );
					if ( pObj )
					{
						if ( ( pObj->GetType() == OBJ_SENTRYGUN ) && ( pObj->IsDisposableBuilding() ) )
						{
							pObj->DetonateObject();
						}
					}
				}
			}
		}
		break;
	case 375:
		{
			// Reduce buff duration when a Heavy gets the Rage upgrade in MvM
			if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
			{
				if ( pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
				{
					const int mod_buff_duration = 319;
					const float flMod = 0.5f;
					CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinition( mod_buff_duration );
					if ( !pDef )
						return;

					pTFPlayer->GetAttributeList()->SetRuntimeAttributeValue( pDef, flMod );
				}
			}
		}
		break;
	case 499:
		{
			// Increase buff duration for Medics with projectile shields
			if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
			{
				if ( pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
				{
					const int mod_buff_duration = 319;
					const float flMod = 1.2f;
					CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinition( mod_buff_duration );
					if ( !pDef )
						return;

					pTFPlayer->GetAttributeList()->SetRuntimeAttributeValue( pDef, flMod );
				}
			}
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reports the Upgrade to systems that care (clients / ogs)
//-----------------------------------------------------------------------------
void CUpgrades::ReportUpgrade( CTFPlayer *pTFPlayer, int nItemDef, int nAttributeDef, int nQuality, int nCost, bool bDowngrade, bool bIsFresh, bool bIsBottle /*= false*/ )
{
	if ( !pTFPlayer )
	{
		return;
	}

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		// Calculate how much money is being used on active class / items
		int nSpending = 0;
		int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		CUtlVector< CUpgradeInfo > *upgrades = g_pPopulationManager->GetPlayerUpgradeHistory( pTFPlayer );
		if ( upgrades )
		{
			for( int u = 0; u < upgrades->Count(); ++u )
			{
				// Class Match, Check to see if we have this item equipped
				if ( iClass == upgrades->Element(u).m_iPlayerClass) 
				{
					// Player upgrade
					if ( upgrades->Element( u ).m_itemDefIndex == INVALID_ITEM_DEF_INDEX )
					{
						nSpending += upgrades->Element(u).m_nCost;
						continue;
					}

					// Item upgrade, look at equipment only not miscs or bottle
					for ( int itemIndex = 0; itemIndex <= LOADOUT_POSITION_PDA2; itemIndex++ )
					{
						CEconItemView *pItem = pTFPlayer->GetLoadoutItem( iClass, itemIndex, true );
						if ( upgrades->Element(u).m_itemDefIndex == pItem->GetItemDefIndex() )
						{
							nSpending += upgrades->Element(u).m_nCost;
							break;
						}
					}
				}
			}
		}

		CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
		if ( pStats )
		{
			pStats->NotifyPlayerActiveUpgradeCosts( pTFPlayer, nSpending );
		}

		// Only report fresh upgrades
		if ( !bIsFresh )
			return;

		MannVsMachineStats_PlayerEvent_Upgraded( pTFPlayer, nItemDef, nAttributeDef, nQuality, nCost, bIsBottle );
	}

	if ( bDowngrade )
	{
		return;
	}

	pTFPlayer->EmitSound( "MVM.PlayerUpgraded" );

	IGameEvent *event = gameeventmanager->CreateEvent( "player_upgraded" );
	if ( event )
	{
		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CUpgrades::RestoreItemAttributeToBaseValue( CEconItemAttributeDefinition *pAttrib, CEconItemView *pItem )
{
	Assert( pAttrib );
	Assert( pItem );
	
	CAttributeList *pAttrList = pItem->GetAttributeList();
	Assert( pAttrList );
	
	float flCurrentValue;
	if ( ::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrib, &flCurrentValue ) )
	{
		float fDefaultValue = pAttrib->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_PERCENTAGE || pAttrib->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE ? 1.f : 0.f;
		::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrib, &fDefaultValue );

		// We don't need the attribute
		if ( AlmostEqual( flCurrentValue, fDefaultValue ) )
		{
			pAttrList->RemoveAttribute( pAttrib );
			return;
		}

		pAttrList->SetRuntimeAttributeValue( pAttrib, fDefaultValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CUpgrades::RestorePlayerAttributeToBaseValue( CEconItemAttributeDefinition *pAttrib, CTFPlayer *pTFPlayer )
{
	Assert( pAttrib );
	Assert( pTFPlayer );
	
	CAttributeList *pAttrList = pTFPlayer->GetAttributeList();
	Assert( pAttrList );
	
	float flCurrentValue;
	if ( ::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrib, &flCurrentValue ) )
	{
		float fDefaultValue = pAttrib->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_PERCENTAGE || pAttrib->GetDescriptionFormat() == ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE ? 1.f : 0.f;
		::FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pAttrList, pAttrib, &fDefaultValue );

		// We don't need the attribute
		if ( AlmostEqual( flCurrentValue, fDefaultValue ) )
		{
			pAttrList->RemoveAttribute( pAttrib );
			return;
		}

		pAttrList->SetRuntimeAttributeValue( pAttrib, fDefaultValue );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CUpgrades::ApplyUpgradeAttributeBlock( UpgradeAttribBlock_t *upgradeBlock, int upgradeCount, CTFPlayer *pPlayer, bool bDowngrade )
{
	if ( !pPlayer )
		return;

	for ( int i = 0; i < upgradeCount; i++ )
	{
		if ( !upgradeBlock[i].szName[0] )
			continue;

		CAttributeList *pAttribList = NULL;
		CEconItemView *pItem = NULL;

		// Player or item?
		if ( upgradeBlock[i].iSlot == LOADOUT_POSITION_INVALID )
		{
			pAttribList = pPlayer->GetAttributeList();
		}
		else
		{
			pItem = CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot( pPlayer, upgradeBlock[i].iSlot );
			if ( !pItem )
				continue;

			pAttribList = pItem->GetAttributeList();
		}

		if ( !pAttribList )
			continue;

		CEconItemAttributeDefinition *pDef = ItemSystem()->GetItemSchema()->GetAttributeDefinitionByName( upgradeBlock[i].szName );
		if ( !pDef )
			continue;

		if ( bDowngrade )
		{
			if ( pItem )
			{
				RestoreItemAttributeToBaseValue( pDef, pItem );
				continue;
			}
			else
			{
				RestorePlayerAttributeToBaseValue( pDef, pPlayer );
			}
		}
		else
		{
			pAttribList->SetRuntimeAttributeValue( pDef, upgradeBlock[i].flValue );
		}
	}
			
	pPlayer->NetworkStateChanged();
}





