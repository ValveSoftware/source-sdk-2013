//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "entity_healthkit.h"
#include "tf_weapon_lunchbox.h"
#include "tf_gamestats.h"


//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_MODEL			"models/items/healthkit.mdl"
#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKit.Touch"

#define TF_AMMOPACK_PICKUP_SOUND	"AmmoPack.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

LINK_ENTITY_TO_CLASS( item_healthammokit, CHealthAmmoKit );

IMPLEMENT_AUTO_LIST( IHealthKitAutoList );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	PrecacheScriptSound( TF_HEALTHKIT_PICKUP_SOUND );
	PrecacheModel( TF_MEDKIT_LARGE_BDAY ); // always precache this for PyroVision
	PrecacheModel( TF_MEDKIT_LARGE_HALLOWEEN ); // always precache this for Halloween

	BaseClass::Precache();

	UpdateModelIndexOverrides();
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;

	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		Assert( pTFPlayer );

		const bool bIsAnyHeavyWithSandvichEquippedPickingUp = pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX ) && pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS );

		bool bPerformPickup = false;

		// In the case of sandvich's owner, only restore ammo
		if ( GetOwnerEntity() == pPlayer && bIsAnyHeavyWithSandvichEquippedPickingUp )
		{
			if ( pPlayer->GiveAmmo( 1, TF_AMMO_GRENADES1, false ) )
			{
				bSuccess = true;
				bPerformPickup = true;
				pTFPlayer->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 100.f );
			}
		}
		else
		{
			float flRuneHealthBonus = ( pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_KNOCKOUT ) ? pTFPlayer->GetRuneHealthBonus() : 0;
			
			float flHealth = ceil( ( pPlayer->GetMaxHealth() - flRuneHealthBonus ) * PackRatios[GetPowerupSize()] );

			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pPlayer, flHealth, mult_health_frompacks );

			int nHealthGiven = pPlayer->TakeHealth( flHealth, DMG_GENERIC );

			if ( nHealthGiven > 0 )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "player_healed" );
				if ( event )
				{
					CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
					int nHealerID = pOwner ? pOwner->GetUserID() : 0;

					event->SetInt( "priority", 1 );	// HLTV event priority
					event->SetInt( "patient", pPlayer->GetUserID() );
					event->SetInt( "healer", nHealerID );
					event->SetInt( "amount", nHealthGiven );
					gameeventmanager->FireEvent( event );
				}
			}

			if ( pTFPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pTFPlayer->m_Shared.GetCarryingRuneType() != RUNE_PLAGUE )
			{
				float flDisguiseHealth = pTFPlayer->m_Shared.GetDisguiseHealth();
				float flDisguiseMaxHealth = pTFPlayer->m_Shared.GetDisguiseMaxHealth();
				float flHealthToAdd = ceil(flDisguiseMaxHealth * PackRatios[GetPowerupSize()]);

				// don't want to add more than we're allowed to have
				if ( flHealthToAdd > flDisguiseMaxHealth - flDisguiseHealth )
				{
					flHealthToAdd = flDisguiseMaxHealth - flDisguiseHealth;
				}

				pTFPlayer->m_Shared.SetDisguiseHealth( flDisguiseHealth + flHealthToAdd );

				bSuccess = true;
			}

			if ( nHealthGiven > 0 || pTFPlayer->m_Shared.InCond( TF_COND_BLEEDING ) || pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) || pTFPlayer->m_Shared.InCond( TF_COND_PLAGUE ) )
			{
				bPerformPickup = true;
				bSuccess = true;

				// subtract this from the drowndmg in case they're drowning and being healed at the same time
				pPlayer->AdjustDrownDmg( -1.0 * flHealth );

				CSingleUserRecipientFilter user( pPlayer );
				EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );

				CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
				if ( pOwner && ( pOwner != pTFPlayer ) )
				{
					if ( pOwner->GetTeamNumber() == pTFPlayer->GetTeamNumber() )
					{
						if ( pTFPlayer->GetLastEntityDamaged() != pTFPlayer )
						{
							CTF_GameStats.Event_PlayerAwardBonusPoints( pOwner, pTFPlayer, 10 );
							CTF_GameStats.Event_PlayerHealedOtherAssist( pOwner, nHealthGiven );
						}

						if ( pOwner->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX ) && pOwner->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
						{
							CEconEntity *pEconItem = dynamic_cast<CEconEntity *>( pOwner->GetEntityForLoadoutSlot( LOADOUT_POSITION_SECONDARY ) );
							if ( pEconItem )
							{
								EconEntity_OnOwnerKillEaterEvent( pEconItem, pOwner, pTFPlayer, kKillEaterEvent_AllyHealingDone, nHealthGiven );

								if ( pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
								{
									EconEntity_OnOwnerKillEaterEvent( pEconItem, pOwner, pTFPlayer, kKillEaterEvent_BurningAllyExtinguished );
								}
							}
						}
					}
				}

				if ( pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
				{
					UserMessageBegin( user, "UpdateAchievement" );
					WRITE_SHORT( ACHIEVEMENT_TF_HEAVY_HEAL_MEDIKITS );
					WRITE_SHORT( nHealthGiven );
					MessageEnd();
				}

				pTFPlayer->m_Shared.HealthKitPickupEffects( nHealthGiven );

				CTF_GameStats.Event_PlayerHealthkitPickup( pTFPlayer );
			}
			else if ( !m_bThrownSingleInstance )
			{
				if ( bIsAnyHeavyWithSandvichEquippedPickingUp )
				{
					if ( pPlayer->GiveAmmo( 1, TF_AMMO_GRENADES1, false ) )
					{
						if ( pTFPlayer )
						{
							pTFPlayer->m_Shared.SetItemChargeMeter( LOADOUT_POSITION_SECONDARY, 100.f );
						}
						bPerformPickup = true;
						bSuccess = true;
					}
				}
			}
		}

		if ( bPerformPickup )
		{
			CSingleUserRecipientFilter user( pPlayer );
			user.MakeReliable();
			UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( GetClassname() );
			MessageEnd();

			IGameEvent * event = gameeventmanager->CreateEvent( "item_pickup" );
			if( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetString( "item", GetHealthKitName() );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	return bSuccess;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CHealthKit::GetRespawnDelay( void )
{
	return g_pGameRules->FlItemRespawnTime( this );
}


//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the health-ammo kit
//-----------------------------------------------------------------------------
bool CHealthAmmoKit::MyTouch( CBasePlayer *pPlayer )
{
	// First do regular health-kit behavior.
	bool bHealthSuccess = BaseClass::MyTouch( pPlayer );

	// Now do ammo-kit behavior (essentially a dupe of the logic in CAmmmoPack::MyTouch - no easy way to put in one spot
	// without larger refactoring).	Filtering out heavies picking up their own sandvich.
	bool bAmmoSuccess = false;
	if ( ValidTouch( pPlayer ) )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( pTFPlayer )
		{
			const bool bIsAnyHeavyWithSandvichEquippedPickingUp = pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_LUNCHBOX ) && pTFPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS );
			if ( !bIsAnyHeavyWithSandvichEquippedPickingUp || GetOwnerEntity() != pPlayer )
			{
				float flPackRatio = PackRatios[GetPowerupSize()];

				int iMaxPrimary = pTFPlayer->GetMaxAmmo( TF_AMMO_PRIMARY );
				if ( pTFPlayer->GiveAmmo( ceil( iMaxPrimary * flPackRatio ), TF_AMMO_PRIMARY, true, kAmmoSource_Pickup ) )
				{
					bAmmoSuccess = true;
				}

				int iMaxSecondary = pTFPlayer->GetMaxAmmo( TF_AMMO_SECONDARY );
				if ( pTFPlayer->GiveAmmo( ceil( iMaxSecondary * flPackRatio ), TF_AMMO_SECONDARY, true, kAmmoSource_Pickup ) )
				{
					bAmmoSuccess = true;
				}

				int iMaxMetal = pTFPlayer->GetMaxAmmo( TF_AMMO_METAL );
				if ( pTFPlayer->GiveAmmo( ceil( iMaxMetal * flPackRatio ), TF_AMMO_METAL, true, kAmmoSource_Pickup ) )
				{
					bAmmoSuccess = true;
				}

				if ( pTFPlayer->m_Shared.AddToSpyCloakMeter( 100.0f * flPackRatio ) )
				{
					bAmmoSuccess = true;
				}

				if ( pTFPlayer->AddToSpyKnife( 100.0f * flPackRatio, false ) )
				{
					bAmmoSuccess = true;
				}

				if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
				{
					int iMaxGrenades1 = pTFPlayer->GetMaxAmmo( TF_AMMO_GRENADES1 );
					if ( pTFPlayer->GiveAmmo( ceil( iMaxGrenades1 * flPackRatio ), TF_AMMO_GRENADES1, true, kAmmoSource_Pickup ) )
					{
						bAmmoSuccess = true;
					}
				}
			}
		}

		// If we didn't give them health already, but did give them ammo, do the ammo pickup flow.
		if ( !bHealthSuccess && bAmmoSuccess )
		{
			CSingleUserRecipientFilter filter( pPlayer );
			EmitSound( filter, entindex(), TF_AMMOPACK_PICKUP_SOUND );

			CTF_GameStats.Event_PlayerAmmokitPickup( pTFPlayer );

			IGameEvent * event = gameeventmanager->CreateEvent( "item_pickup" );
			if ( event )
			{
				event->SetInt( "userid", pPlayer->GetUserID() );
				event->SetString( "item", GetHealthKitName() );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	return bAmmoSuccess || bHealthSuccess;
}