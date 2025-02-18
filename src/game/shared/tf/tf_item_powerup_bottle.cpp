//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_powerup_bottle.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_medigun.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifndef GAME_DLL
	extern ConVar cl_hud_minmode;
#endif


LINK_ENTITY_TO_CLASS( tf_powerup_bottle, CTFPowerupBottle );
IMPLEMENT_NETWORKCLASS_ALIASED( TFPowerupBottle, DT_TFPowerupBottle )

// Network Table --
BEGIN_NETWORK_TABLE( CTFPowerupBottle, DT_TFPowerupBottle )
#if defined( GAME_DLL )
	SendPropBool( SENDINFO( m_bActive ) ),
	SendPropInt( SENDINFO( m_usNumCharges ), -1, SPROP_UNSIGNED ),
#else
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropInt( RECVINFO( m_usNumCharges ) ),
#endif // GAME_DLL
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFPowerupBottle )
END_DATADESC()
// -- Data Desc

PRECACHE_REGISTER( tf_powerup_bottle );


//-----------------------------------------------------------------------------
// SHARED CODE
//-----------------------------------------------------------------------------

CTFPowerupBottle::CTFPowerupBottle() : CTFWearable()
{
	m_bActive = false;
	m_usNumCharges = 0;
	m_flLastSpawnTime = 0.f;

#ifdef TF_CLIENT_DLL
	ListenForGameEvent( "player_spawn" );
#endif
}

void CTFPowerupBottle::Precache( void )
{
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_generic.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_krit.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_uber.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_ammo.mdl" );
	PrecacheModel( "models/player/items/mvm_loot/all_class/mvm_flask_build.mdl" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the bottle to its initial state
//-----------------------------------------------------------------------------
void CTFPowerupBottle::Reset( void )
{
	m_bActive = false;
	SetNumCharges( 0 );

#ifdef GAME_DLL
	class CAttributeIterator_ZeroRefundableCurrency : public IEconItemUntypedAttributeIterator
	{
	public:
		CAttributeIterator_ZeroRefundableCurrency( CAttributeList *pAttrList )
			: m_pAttrList( pAttrList )
		{
			Assert( m_pAttrList );
		}

	private:
		virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef )
		{
			if ( ::FindAttribute( m_pAttrList, pAttrDef ) )
			{
				m_pAttrList->SetRuntimeAttributeRefundableCurrency( pAttrDef, 0 );
			}

			return true;
		}

		CAttributeList *m_pAttrList;
	};

	CAttributeIterator_ZeroRefundableCurrency it( GetAttributeList() );
	GetAttributeList()->IterateAttributes( &it );
#endif
}


PowerupBottleType_t CTFPowerupBottle::GetPowerupType( void ) const
{
	int iHasCritBoost = 0;
	CALL_ATTRIB_HOOK_INT( iHasCritBoost, critboost );
	if ( iHasCritBoost )
	{
		return POWERUP_BOTTLE_CRITBOOST;
	}

	int iHasUbercharge = 0;
	CALL_ATTRIB_HOOK_INT( iHasUbercharge, ubercharge );
	if ( iHasUbercharge )
	{
		return POWERUP_BOTTLE_UBERCHARGE;
	}

	int iHasRecall = 0;
	CALL_ATTRIB_HOOK_INT( iHasRecall, recall );
	if ( iHasRecall )
	{
		return POWERUP_BOTTLE_RECALL;
	}

	int iHasRefillAmmo = 0;
	CALL_ATTRIB_HOOK_INT( iHasRefillAmmo, refill_ammo );
	if ( iHasRefillAmmo )
	{
		return POWERUP_BOTTLE_REFILL_AMMO;
	}

	int iHasInstaBuildingUpgrade = 0;
	CALL_ATTRIB_HOOK_INT( iHasInstaBuildingUpgrade, building_instant_upgrade );
	if ( iHasInstaBuildingUpgrade )
	{
		return POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE;
	}


	return POWERUP_BOTTLE_NONE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerupBottle::ReapplyProvision( void )
{
	// let the base class do what it needs to do in terms of adding/removing itself from old and new owners
	BaseClass::ReapplyProvision();

	CBaseEntity *pOwner = GetOwnerEntity();
	IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pOwner );
	if ( pOwnerAttribInterface )
	{
		if ( m_bActive )
		{
			if ( !pOwnerAttribInterface->GetAttributeManager()->IsBeingProvidedToBy( this ) )
			{
				GetAttributeManager()->ProvideTo( pOwner );
			}
		}
		else
		{
			GetAttributeManager()->StopProvidingTo( pOwner );
		}

#ifdef GAME_DLL
		bool bBottleShared = false;
		CTFPlayer *pTFPlayer = dynamic_cast< CTFPlayer* >( pOwner );
		if ( pTFPlayer )
		{
			float flDuration = 0;
			CALL_ATTRIB_HOOK_FLOAT( flDuration, powerup_duration );
			
			// Add extra time?
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, flDuration, canteen_specialist );

			// This block of code checks if a medic has the ability to
			// share bottle charges with their heal target
			int iShareBottle = 0;
			CWeaponMedigun *pMedigun = NULL;
			CTFPlayer *pHealTarget = NULL;
			if ( pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
			{
				pMedigun = dynamic_cast<CWeaponMedigun *>( pTFPlayer->GetActiveWeapon() );
				if ( pMedigun )
				{
					pHealTarget = ToTFPlayer( pMedigun->GetHealTarget() );
					if ( pHealTarget )
					{
						CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFPlayer, iShareBottle, canteen_specialist );
					}
				}
			}

			// special stuff for conditions
			int iHasCritBoost = 0;
			CALL_ATTRIB_HOOK_INT( iHasCritBoost, critboost );
			if ( iHasCritBoost != 0 )
			{
				if ( m_bActive )
				{
					pTFPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_USER_BUFF, flDuration );

					if ( iShareBottle && pHealTarget )
					{
						pHealTarget->m_Shared.AddCond( TF_COND_CRITBOOSTED_USER_BUFF, flDuration );
						bBottleShared = true;
					}
				}
				else
				{
					pTFPlayer->m_Shared.RemoveCond( TF_COND_CRITBOOSTED_USER_BUFF, true );
				}
			}

			int iHasUbercharge = 0;
			CALL_ATTRIB_HOOK_INT( iHasUbercharge, ubercharge );
			if ( iHasUbercharge )
			{
				if ( m_bActive )
				{
					pTFPlayer->m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, flDuration );

					// Shield sentries
					if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
					{
						for ( int i = pTFPlayer->GetObjectCount()-1; i >= 0; i-- )
						{
							CObjectSentrygun *pSentry = dynamic_cast<CObjectSentrygun *>( pTFPlayer->GetObject(i) );
							if ( pSentry && !pSentry->IsCarried() )
							{
								pSentry->SetShieldLevel( SHIELD_MAX, flDuration );
							}		
						}
					}
					else if ( iShareBottle && pHealTarget )
					{
						pHealTarget->m_Shared.AddCond( TF_COND_INVULNERABLE_USER_BUFF, flDuration, pTFPlayer );
						bBottleShared = true;
					}
				}
				else
				{
					pTFPlayer->m_Shared.RemoveCond( TF_COND_INVULNERABLE_USER_BUFF, true );
				}
			}

			int iHasRecall = 0;
			CALL_ATTRIB_HOOK_INT( iHasRecall, recall );
			if ( iHasRecall )
			{
				if ( m_bActive )
				{
					pTFPlayer->ForceRespawn();
					pTFPlayer->m_Shared.AddCond( TF_COND_SPEED_BOOST, 7.f );
				}
			}

			int iHasRefillAmmo = 0;
			CALL_ATTRIB_HOOK_INT( iHasRefillAmmo, refill_ammo );
			if ( iHasRefillAmmo )
			{
				if ( m_bActive )
				{	
					// Refill weapon clips
					for ( int i = 0; i < MAX_WEAPONS; i++ )
					{
						CBaseCombatWeapon *pWeapon = pTFPlayer->GetWeapon(i);
						if ( !pWeapon )
							continue;

						// ACHIEVEMENT_TF_MVM_USE_AMMO_BOTTLE
						if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
						{
							if ( ( pWeapon->UsesPrimaryAmmo() && !pWeapon->HasPrimaryAmmo() ) ||
								( pWeapon->UsesSecondaryAmmo() && !pWeapon->HasSecondaryAmmo() ) )
							{
								pTFPlayer->AwardAchievement( ACHIEVEMENT_TF_MVM_USE_AMMO_BOTTLE ); 
							}
						}

						pWeapon->GiveDefaultAmmo();

						if ( iShareBottle && pHealTarget )
						{
							CBaseCombatWeapon *pPatientWeapon = pHealTarget->GetWeapon(i);
							if ( !pPatientWeapon )
								continue;

							pPatientWeapon->GiveDefaultAmmo();
							bBottleShared = true;
						}
					}

					// And give the player ammo
					for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
					{
						pTFPlayer->GiveAmmo( pTFPlayer->GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );

						if ( iShareBottle && pHealTarget )
						{
							pHealTarget->GiveAmmo( pHealTarget->GetMaxAmmo(iAmmo), iAmmo, true, kAmmoSource_Resupply );
							bBottleShared = true;
						}
					}
				}
			}

			int iHasInstaBuildingUpgrade = 0;
			CALL_ATTRIB_HOOK_INT( iHasInstaBuildingUpgrade, building_instant_upgrade );
			if ( iHasInstaBuildingUpgrade )
			{
				if ( m_bActive )
				{
					for ( int i = pTFPlayer->GetObjectCount()-1; i >= 0; i-- )
					{
						CBaseObject *pObj = pTFPlayer->GetObject(i);
						if ( pObj )
						{
							int nMaxLevel = pObj->GetMaxUpgradeLevel();

							// If object is carried, set the target max and move on
							if ( pObj->IsCarried() )
							{
								pObj->SetHighestUpgradeLevel( nMaxLevel );
								continue;
							}

							// If we're already at max level, heal
							if ( pObj->GetUpgradeLevel() == nMaxLevel )
							{
								pObj->SetHealth( pObj->GetMaxHealth() );
							}
							else
							{
								if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
								{
									if ( pObj->GetType() == OBJ_SENTRYGUN )
									{
										IGameEvent *event = gameeventmanager->CreateEvent( "mvm_quick_sentry_upgrade" );
										if ( event )
										{
											event->SetInt( "player", GetOwnerEntity()->entindex() );
											gameeventmanager->FireEvent( event );
										}
									}
								}

								pObj->DoQuickBuild( true );
							}
						}		
					}
				}
			}

			// ACHIEVEMENT_TF_MVM_MEDIC_SHARE_BOTTLES
			if ( bBottleShared )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "mvm_medic_powerup_shared" );
				if ( event )
				{
					event->SetInt( "player", pTFPlayer->entindex() );
					gameeventmanager->FireEvent( event );
				}
			}
		}
#endif
	}
}


//-----------------------------------------------------------------------------
// Purpose: Removes the item and deactivates any effect
//-----------------------------------------------------------------------------
void CTFPowerupBottle::UnEquip( CBasePlayer* pOwner )
{
	BaseClass::UnEquip( pOwner );
	RemoveEffect();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerupBottle::Use()
{
	if ( !m_bActive && GetNumCharges() > 0 )
	{
		if ( !AllowedToUse() )
			return false;

#ifdef GAME_DLL
		// Use up one charge worth of refundable money when a charge is used
		class CAttributeIterator_ConsumeOneRefundableCharge : public IEconItemUntypedAttributeIterator
		{
		public:
			CAttributeIterator_ConsumeOneRefundableCharge( CAttributeList *pAttrList, int iNumCharges )
				: m_pAttrList( pAttrList )
				, m_iNumCharges( iNumCharges )
			{
				Assert( m_pAttrList );
				Assert( m_iNumCharges > 0 );
			}

		private:
			virtual bool OnIterateAttributeValueUntyped( const CEconItemAttributeDefinition *pAttrDef )
			{
				if ( ::FindAttribute( m_pAttrList, pAttrDef ) )
				{
					int nRefundableCurrency = m_pAttrList->GetRuntimeAttributeRefundableCurrency( pAttrDef );
					if ( nRefundableCurrency > 0 )
					{
						m_pAttrList->SetRuntimeAttributeRefundableCurrency( pAttrDef, nRefundableCurrency - (nRefundableCurrency / m_iNumCharges) );
					}
				}

				// Backwards compatibility -- assume any number of attributes.
				return true;
			}

			CAttributeList *m_pAttrList;
			int m_iNumCharges;
		};

		CAttributeIterator_ConsumeOneRefundableCharge it( GetAttributeList(), GetNumCharges() );
		GetAttributeList()->IterateAttributes( &it );
#endif

		float flDuration = 0;
		CALL_ATTRIB_HOOK_FLOAT( flDuration, powerup_duration );
		
		// Add extra time?
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner )
		{
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, flDuration, canteen_specialist );
		}
		IGameEvent *event = gameeventmanager->CreateEvent( "player_used_powerup_bottle" );
		if ( event )
		{
			event->SetInt( "player", GetOwnerEntity()->entindex() );
			event->SetInt( "type", GetPowerupType() );
			event->SetFloat( "time", flDuration );
			gameeventmanager->FireEvent( event );
		}

#ifdef GAME_DLL
		if ( pOwner )
		{
			EconEntity_OnOwnerKillEaterEventNoPartner( dynamic_cast<CEconEntity *>( this ), pOwner, kKillEaterEvent_PowerupBottlesUsed );

			// we consumed an upgrade - forget it
			pOwner->ForgetFirstUpgradeForItem( GetAttributeContainer()->GetItem() );
		}
#endif
		
		SetNumCharges( GetNumCharges() - 1 );
		m_bActive = true;
		ReapplyProvision();

		SetContextThink( &CTFPowerupBottle::StatusThink, gpGlobals->curtime + flDuration, "PowerupBottleThink" );
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::StatusThink()
{
	RemoveEffect();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::RemoveEffect()
{
	m_bActive = false;
	ReapplyProvision();
	SetContextThink( NULL, 0, "PowerupBottleThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerupBottle::SetNumCharges( uint8 usNumCharges )
{
	static CSchemaAttributeDefHandle pAttrDef_PowerupCharges( "powerup charges" );

	m_usNumCharges = usNumCharges; 

	if ( !pAttrDef_PowerupCharges )
		return;

	CEconItemView *pEconItemView = GetAttributeContainer()->GetItem();
	if ( !pEconItemView )
		return;

	pEconItemView->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_PowerupCharges, float( usNumCharges ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
uint8 CTFPowerupBottle::GetNumCharges() const
{
	return m_usNumCharges; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
uint8 CTFPowerupBottle::GetMaxNumCharges() const
{
	int iMaxNumCharges = 0;
	CALL_ATTRIB_HOOK_INT( iMaxNumCharges, powerup_max_charges );

	// Default canteen has 3 charges.  Medic canteen specialist allows purchasing 3 more charges.
	// If anything else increases max charges, we need to refactor how canteen specialist is handled.
	Assert( iMaxNumCharges >= 0 && iMaxNumCharges <= 6 );
	
	iMaxNumCharges = Min( iMaxNumCharges, 6 );

	return (uint8)iMaxNumCharges;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerupBottle::AllowedToUse()
{
	if ( TFGameRules() && !( TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS || TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsObserver() || !pPlayer->IsAlive() )
		return false;

#ifdef GAME_DLL
	m_flLastSpawnTime = pPlayer->GetSpawnTime();
#endif

	if ( gpGlobals->curtime < m_flLastSpawnTime + 0.7f )
		return false;

	return true;
}

const char* CTFPowerupBottle::GetEffectLabelText( void )
{
#ifndef GAME_DLL
	if ( cl_hud_minmode.GetBool() )
	{
		return "#TF_PVE_UsePowerup_MinMode";
	}
#endif

	switch ( GetPowerupType() )
	{
	case POWERUP_BOTTLE_CRITBOOST:
		return "#TF_PVE_UsePowerup_CritBoost";

	case POWERUP_BOTTLE_UBERCHARGE:
		return "#TF_PVE_UsePowerup_Ubercharge";

	case POWERUP_BOTTLE_RECALL:
		return "#TF_PVE_UsePowerup_Recall";

	case POWERUP_BOTTLE_REFILL_AMMO:
		return "#TF_PVE_UsePowerup_RefillAmmo";

	case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
		return "#TF_PVE_UsePowerup_BuildinginstaUpgrade";

	case POWERUP_BOTTLE_RADIUS_STEALTH:
		return "#TF_PVE_UsePowerup_RadiusStealth";
	}

	return "#TF_PVE_UsePowerup_CritBoost";
}

const char* CTFPowerupBottle::GetEffectIconName( void )
{
	switch ( GetPowerupType() )
	{
	case POWERUP_BOTTLE_CRITBOOST:
		return "../hud/ico_powerup_critboost_red";

	case POWERUP_BOTTLE_UBERCHARGE:
		return "../hud/ico_powerup_ubercharge_red";

	case POWERUP_BOTTLE_RECALL:
		return "../hud/ico_powerup_recall_red";

	case POWERUP_BOTTLE_REFILL_AMMO:
		return "../hud/ico_powerup_refill_ammo_red";

	case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
		return "../hud/ico_powerup_building_instant_red";

	case POWERUP_BOTTLE_RADIUS_STEALTH:
		return "../vgui/achievements/tf_soldier_kill_spy_killer";
	}

	return "../hud/ico_powerup_critboost_red";
}

#ifdef TF_CLIENT_DLL
void CTFPowerupBottle::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();

	if ( FStrEq( pszEventName, "player_spawn" ) )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
		if ( !pTFOwner )
			return;

		const int nUserID = event->GetInt( "userid" );
		CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByUserId( nUserID ) );
		if ( pPlayer && pPlayer == pTFOwner )
		{
			m_flLastSpawnTime = gpGlobals->curtime;
		}
	}
}

int CTFPowerupBottle::GetWorldModelIndex( void )
{
	if ( IsBasePowerUpBottle() && ( GetNumCharges() > 0 ) )
	{
		switch ( GetPowerupType() )
		{
		case POWERUP_BOTTLE_CRITBOOST:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_krit.mdl" );

		case POWERUP_BOTTLE_UBERCHARGE:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_uber.mdl" );

		case POWERUP_BOTTLE_RECALL:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl" );

		case POWERUP_BOTTLE_REFILL_AMMO:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_ammo.mdl" );

		case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_build.mdl" );

		case POWERUP_BOTTLE_RADIUS_STEALTH:
			return modelinfo->GetModelIndex( "models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl" );
		}
	}

	return BaseClass::GetWorldModelIndex();
}
#endif

int CTFPowerupBottle::GetSkin()
{
	if ( !IsBasePowerUpBottle() )
	{
		return ( ( GetNumCharges() > 0 ) ? 1 : 0 );
	}

	return BaseClass::GetSkin();
}


#ifdef CLIENT_DLL
// ******************************************************************************************
// CEquipMvMCanteenNotification - Client notification to equip a canteen
// ******************************************************************************************
void CEquipMvMCanteenNotification::Accept()
{
	m_bHasTriggered = true;

	CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
	if ( !pLocalInv )
	{
		MarkForDeletion();
		return;
	}

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		MarkForDeletion();
		return;
	}

	// try to equip non-stock-spellbook first
	static CSchemaItemDefHandle pItemDef_Robo( "Battery Canteens" );
	static CSchemaItemDefHandle pItemDef_KritzOrTreat( "Kritz Or Treat Canteen" );
	static CSchemaItemDefHandle pItemDef_Canteen( "Power Up Canteen (MvM)" );
	static CSchemaItemDefHandle pItemDef_DefaultCanteen( "Default Power Up Canteen (MvM)" );

	CEconItemView *pCanteen= NULL;

	Assert( pItemDef_Robo );
	Assert( pItemDef_KritzOrTreat );
	Assert( pItemDef_Canteen );
	Assert( pItemDef_DefaultCanteen );
	
	for ( int i = 0; i < pLocalInv->GetItemCount(); ++i )
	{
		CEconItemView *pItem = pLocalInv->GetItem( i );
		Assert( pItem );

		if ( pItem->GetItemDefinition() == pItemDef_Robo
			|| pItem->GetItemDefinition() == pItemDef_KritzOrTreat
			|| pItem->GetItemDefinition() == pItemDef_Canteen
			|| pItem->GetItemDefinition() == pItemDef_DefaultCanteen
		) {
			pCanteen = pItem;
			break;
		}
	}

	// Default item becomes a spellbook in this mode
	itemid_t iItemId = INVALID_ITEM_ID;
	if ( pCanteen )
	{
		iItemId = pCanteen->GetItemID();
	}

	TFInventoryManager()->EquipItemInLoadout( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION, iItemId );

	// Tell the GC to tell server that we should respawn if we're in a respawn room

	MarkForDeletion();
}

//===========================================================================================
void CEquipMvMCanteenNotification::UpdateTick()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		CTFPowerupBottle *pCanteen = dynamic_cast<CTFPowerupBottle*>( TFInventoryManager()->GetItemInLoadoutForClass( pLocalPlayer->GetPlayerClass()->GetClassIndex(), LOADOUT_POSITION_ACTION ) );
		if ( pCanteen )
		{
			MarkForDeletion();
		}
	}
}
#endif // client

