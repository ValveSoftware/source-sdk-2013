//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:			The "weapon" used to build objects
//					
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player.h"
#include "entitylist.h"
#include "in_buttons.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/IEngineSound.h"
#include "tf_obj.h"
#include "sendproxy.h"
#include "tf_weapon_builder.h"
#include "vguiscreen.h"
#include "tf_gamerules.h"
#include "tf_obj_teleporter.h"
#include "tf_obj_sapper.h"

extern ISoundEmitterSystemBase *soundemitterbase;

extern ConVar tf2_object_hard_limits;
extern ConVar tf_fastbuild;


EXTERN_SEND_TABLE(DT_BaseCombatWeapon)

BEGIN_NETWORK_TABLE_NOBASE( CTFWeaponBuilder, DT_BuilderLocalData )
	SendPropInt( SENDINFO( m_iObjectType ), BUILDER_OBJECT_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hObjectBeingBuilt ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_aBuildableObjectTypes ), SendPropBool( SENDINFO_ARRAY( m_aBuildableObjectTypes ) ) ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST(CTFWeaponBuilder, DT_TFWeaponBuilder)
	SendPropInt( SENDINFO( m_iBuildState ), 4, SPROP_UNSIGNED ),
	SendPropDataTable( "BuilderLocalData", 0, &REFERENCE_SEND_TABLE( DT_BuilderLocalData ), SendProxy_SendLocalWeaponDataTable ),
	SendPropInt( SENDINFO( m_iObjectMode ) , 4, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flWheatleyTalkingUntil) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_builder, CTFWeaponBuilder );
PRECACHE_WEAPON_REGISTER( tf_weapon_builder );

//

IMPLEMENT_SERVERCLASS_ST( CTFWeaponSapper, DT_TFWeaponSapper )
	SendPropFloat( SENDINFO( m_flChargeBeginTime ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_weapon_sapper, CTFWeaponSapper );
PRECACHE_WEAPON_REGISTER( tf_weapon_sapper );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder::CTFWeaponBuilder()
{
	m_iObjectType.Set( BUILDER_INVALID_OBJECT );
	m_iObjectMode = 0;
	m_bAttack3Down = false;

	//Sapper VO Pack stuff
	WheatleyReset( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBuilder::~CTFWeaponBuilder()
{
	StopPlacement();
	if (m_pkvWavList)
	{
		m_pkvWavList->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetSubType( int iSubType )
{
	m_iObjectType = iSubType;
	
	// m_iViewModelIndex is set by the base Precache(), which didn't know what
	// type of object we built, so it didn't get the right viewmodel index.
	// Now that our data is filled in, go and get the right index.
	const char *pszViewModel = GetViewModel(0);
	if ( pszViewModel && pszViewModel[0] )
	{
		m_iViewModelIndex = CBaseEntity::PrecacheModel( pszViewModel );
	}

	if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER )
	{
		if ( IsWheatleySapper() )
		{
			if (m_pkvWavList)
			{
				m_pkvWavList->deleteThis();
			}
			m_pkvWavList = new KeyValues("sappervo");
		}
	}

	BaseClass::SetSubType( iSubType );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::Precache( void )
{
	BaseClass::Precache();

	// Precache all the viewmodels we could possibly be building
	for ( int iObj=0; iObj < OBJ_LAST; iObj++ )
	{
		const CObjectInfo *pInfo = GetObjectInfo( iObj );

		if ( pInfo )
		{
			if ( pInfo->m_pViewModel )
			{
				PrecacheModel( pInfo->m_pViewModel );
			}

			if ( pInfo->m_pPlayerModel )
			{
				PrecacheModel( pInfo->m_pPlayerModel );
			}
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::CanDeploy( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if (!pPlayer)
		return false;

	if ( pPlayer->m_Shared.IsCarryingObject() )
		return BaseClass::CanDeploy();

	if ( pPlayer->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
	{
		return false;
	}

	return BaseClass::CanDeploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::Deploy( void )
{
	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		SetCurrentState( BS_PLACING );
		StartPlacement(); 
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.35f;
		m_flNextSecondaryAttack = gpGlobals->curtime;		// asap

		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if (!pPlayer)
			return false;

		pPlayer->SetNextAttack( gpGlobals->curtime );

		m_iWorldModelIndex = modelinfo->GetModelIndex( GetWorldModel() );

		m_flNextDenySound = 0;

		// Set off the hint here, because we don't know until now if our building
		// is rotate-able or not.
		if ( m_hObjectBeingBuilt && !m_hObjectBeingBuilt->MustBeBuiltOnAttachmentPoint() )
		{
			// set the alt-fire hint so it gets removed when we holster
			m_iAltFireHint = HINT_ALTFIRE_ROTATE_BUILDING;
			pPlayer->StartHintTimer( m_iAltFireHint );
		}

		pPlayer->PlayWearableAnimsForPlaybackEvent( WAP_START_BUILDING );
	}

	return bDeploy;
}

Activity CTFWeaponBuilder::GetDrawActivity( void )
{
	// sapper used to call different draw animations , one when invis and one when not.
	// now you can go invis *while* deploying, so let's always use the one-handed deploy.
	if ( GetType() == OBJ_ATTACHMENT_SAPPER )
	{
		return ACT_VM_DRAW_DEPLOYED;
	}

	return BaseClass::GetDrawActivity();
}

//-----------------------------------------------------------------------------
// Purpose: Stop placement when holstering
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false; 

	if ( pOwner->m_Shared.IsCarryingObject() )
		return false;

	if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER )
	{
		if( IsWheatleySapper() )
		{
			pOwner->ClearSappingTracking();
			if ( pOwner->m_Shared.GetState() == TF_STATE_DYING)
			{
				if ( RandomInt( 0, 4) == 0 )
				{
					WheatleyEmitSound( "PSap.DeathLong", true );
				}
				else
				{
					WheatleyEmitSound( "PSap.Death", true );
				}
			}
			else
			{
				float flSoundDuration;
				if ( gpGlobals->curtime - m_flWheatleyLastDeploy < 1.5 && gpGlobals->curtime - m_flWheatleyLastDeploy > -1.0 )
				{
					flSoundDuration = WheatleyEmitSound( "PSap.HolsterFast");
				}
				else
				{
					flSoundDuration = WheatleyEmitSound( "PSap.Holster");
				}
				m_flWheatleyLastHolster = gpGlobals->curtime + flSoundDuration;
			}
		}
	}

	m_flNextVoicePakIdleStartTime = -1.0f;

	if ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID )
	{
		SetCurrentState( BS_IDLE );
	}
	StopPlacement();

	pOwner->PlayWearableAnimsForPlaybackEvent( WAP_STOP_BUILDING );

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// If we're building, and our team has lost, stop placing the object
	if ( m_hObjectBeingBuilt.Get() && 
		TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && 
		pOwner->GetTeamNumber() != TFGameRules()->GetWinningTeam() )
	{
		StopPlacement();
		return;
	}

	// Check that I still have enough resources to build this item
	if ( pOwner->CanBuild( m_iObjectType, m_iObjectMode ) != CB_CAN_BUILD )
	{
		SwitchOwnersWeaponToLast();
	}

	if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		PrimaryAttack();
	}

	if ( pOwner->m_nButtons & IN_ATTACK2 )
	{
		if ( m_flNextSecondaryAttack <= gpGlobals->curtime )
		{
			SecondaryAttack();
		}
	}
	else
	{
		m_bInAttack2 = false;
	}

	// Attrib
	int iMarkForDeathOnPickup = 0;
	if ( pOwner->m_Shared.IsCarryingObject () )
	{
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, iMarkForDeathOnPickup, mark_for_death_on_building_pickup );
		if ( iMarkForDeathOnPickup )
		{
			pOwner->m_Shared.AddCond( TF_COND_MARKEDFORDEATH_SILENT, 3.f );
		}
	}

	WeaponIdle();
}

//-----------------------------------------------------------------------------
// Purpose: Start placing or building the currently selected object
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::PrimaryAttack( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
		return;

	// Necessary so that we get the latest building position for the test, otherwise
	// we are one frame behind.
	UpdatePlacementState();

	// What state should we move to?
	switch( m_iBuildState )
	{
	case BS_IDLE:
		{
			// Idle state starts selection
			SetCurrentState( BS_SELECTING );
		}
		break;

	case BS_SELECTING:
		{
			// Do nothing, client handles selection
			return;
		}
		break;

	case BS_PLACING:
		{
			if ( m_hObjectBeingBuilt )
			{
				int iFlags = m_hObjectBeingBuilt->GetObjectFlags();

				// Tricky, because this can re-calc the object position and change whether its a valid 
				// pos or not. Best not to do this only in debug, but we can be pretty sure that this
				// will give the same result as was calculated in UpdatePlacementState() above.
				Assert( IsValidPlacement() );

				// If we're placing an attachment, like a sapper, play a placement animation on the owner
				if ( m_hObjectBeingBuilt->MustBeBuiltOnAttachmentPoint() )
				{
					pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
				}

				CBaseEntity *pBuiltOnObject = m_hObjectBeingBuilt->GetBuiltOnObject();
				
				if ( pBuiltOnObject && m_iObjectType == OBJ_ATTACHMENT_SAPPER )
				{
					m_vLastKnownSapPos = pBuiltOnObject->GetAbsOrigin();
					m_hLastSappedBuilding = pBuiltOnObject;
				}
				StartBuilding();

				if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER )
				{
					// tell players a sapper was just placed (so bots can react)
					CUtlVector< CTFPlayer * > playerVector;
					CollectPlayers( &playerVector, TEAM_ANY, COLLECT_ONLY_LIVING_PLAYERS );

					for( int i=0; i<playerVector.Count(); ++i )
						playerVector[i]->OnSapperPlaced( pBuiltOnObject );

					// if we just placed a sapper on a teleporter...try to sap the match, too?
					if ( pBuiltOnObject )
					{
						CObjectTeleporter *pTeleporter = dynamic_cast<CObjectTeleporter*>( pBuiltOnObject );
						if ( pTeleporter && pTeleporter->GetMatchingTeleporter() && !pTeleporter->GetMatchingTeleporter()->HasSapper() )
						{
							// Start placing another
							SetCurrentState( BS_PLACING );
							StartPlacement(); 
	
							if ( m_hObjectBeingBuilt.Get() )
							{
								m_hObjectBeingBuilt->UpdateAttachmentPlacement( pTeleporter->GetMatchingTeleporter() );
								StartBuilding();
							}
						}
					}
				}

				// Should we switch away?
				if ( iFlags & OF_ALLOW_REPEAT_PLACEMENT )
				{
					// Start placing another
					SetCurrentState( BS_PLACING );
					StartPlacement(); 
				}
				else
				{
					SwitchOwnersWeaponToLast();
				}
			}
		}
		break;

	case BS_PLACING_INVALID:
		{
			if ( m_flNextDenySound < gpGlobals->curtime )
			{
				CSingleUserRecipientFilter filter( pOwner );
				EmitSound( filter, entindex(), "Player.DenyWeaponSelection" );

				m_flNextDenySound = gpGlobals->curtime + 0.5;
			}
		}
		break;
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
}

void CTFWeaponBuilder::SecondaryAttack( void )
{
	if ( m_bInAttack2 )
		return;

	// require a re-press
	m_bInAttack2 = true;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	UpdatePlacementState();

	if ( !pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) && pOwner->DoClassSpecialSkill() )
	{
		// Spies do the special skill first.
	}
	else if ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID )
	{
		if ( m_hObjectBeingBuilt )
		{
			pOwner->StopHintTimer( HINT_ALTFIRE_ROTATE_BUILDING );
			m_hObjectBeingBuilt->RotateBuildAngles();
		}
	}
	else if ( pOwner->DoClassSpecialSkill() )
	{
		// Engineers do the special skill last.
	}

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}

//-----------------------------------------------------------------------------
// Purpose: Set the builder to the specified state
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetCurrentState( int iState )
{
	m_iBuildState = iState;
}

//-----------------------------------------------------------------------------
// Purpose: Set the owner's weapon and last weapon appropriately when we need to
//			switch away from the builder weapon.  
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SwitchOwnersWeaponToLast()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// for engineer, switch to wrench and set last weapon appropriately
	if ( pOwner->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		// Switch to wrench if possible. if not, then best weapon
		CBaseCombatWeapon *pWpn = pOwner->Weapon_GetSlot( 2 );

		// Don't store last weapon when we autoswitch off builder
		CBaseCombatWeapon *pLastWpn = pOwner->GetLastWeapon();

		if ( pWpn )
		{
			pOwner->Weapon_Switch( pWpn );
		}
		else
		{
			pOwner->SwitchToNextBestWeapon( NULL );
		}

		if ( pWpn == pLastWpn )
		{
			// We had the wrench out before we started building. Go ahead and set out last
			// weapon to our primary weapon.
			pWpn = pOwner->Weapon_GetSlot( 0 );
			pOwner->Weapon_SetLast( pWpn );
		}
		else
		{
			pOwner->Weapon_SetLast( pLastWpn );
		}
	}
	else
	{
		// for all other classes, just switch to last weapon used
		pOwner->Weapon_Switch( pOwner->GetLastWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates the building postion and checks the new postion
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::UpdatePlacementState( void )
{
	// This updates the building position
	bool bValidPos = IsValidPlacement();

	// If we're in placement mode, update the placement model
	switch( m_iBuildState )
	{
	case BS_PLACING:
	case BS_PLACING_INVALID:
		{
			if ( bValidPos )
			{
				SetCurrentState( BS_PLACING );
			}
			else
			{
				SetCurrentState( BS_PLACING_INVALID );
			}
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Idle updates the position of the build placement model
//-----------------------------------------------------------------------------
/*#define SAPPER_VOPAK_DEFAULT_WAIT 0.0f*/
void CTFWeaponBuilder::WeaponIdle( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	WheatleySapperIdle( pOwner );

	if ( HasWeaponIdleTimeElapsed() )
	{
		SendWeaponAnim( ACT_VM_IDLE );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Special Item Idle
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::IsWheatleySapper( void )
{
	float flVoicePak = 0.0;
	CALL_ATTRIB_HOOK_FLOAT( flVoicePak, sapper_voice_pak );
	return (flVoicePak == 1.0);
}

//-----------------------------------------------------------------------------
// Purpose: Special Item Reset
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WheatleyReset( bool bResetIntro )
{
	if ( IsWheatleySapper() )
	{
		WheatleyEmitSound( "PSap.null" );
	}
	if ( bResetIntro )
	{
		m_bWheatleyIntroPlayed = false;
	}
	m_flNextVoicePakIdleStartTime = -1.0f;
	SetWheatleyState( TF_PSAPSTATE_IDLE );
	m_flWheatleyTalkingUntil = 0.00;
	m_flWheatleyLastDamage = 0.00;
	m_iWheatleyVOSequenceOffset = 0;
	m_flWheatleyLastDeploy = 0.00;
	m_flWheatleyLastHolster = 0.00;
}

bool CTFWeaponBuilder::IsWheatleyTalking( void )
{
	return gpGlobals->curtime <= m_flWheatleyTalkingUntil;
}

float CTFWeaponBuilder::WheatleyEmitSound( const char *snd, bool bEmitToAll /*= false*/, bool bNoRepeats /*= false */ )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	CSoundParameters params;
	if ( !soundemitterbase->GetParametersForSound( snd, params, GENDER_NONE ) )
	{
		return 0.00;
	}
	//Should we check to see if it's already been played?
	if ( bNoRepeats && m_pkvWavList )
	{
		if ( m_pkvWavList->GetInt( params.soundname , 0 ) )
		{
			return 0;
		}
		else
		{
			m_pkvWavList->SetInt( params.soundname , 1);
		}
	}
	//Look for special cases that require us to pick the next lines from a sequential list
	if ( Q_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle38.mp3") == 0 )
	{
		SetWheatleyState( TF_PSAPSTATE_SPECIALIDLE_HARMLESS );
		m_iWheatleyVOSequenceOffset = 0;
	}
	else if ( Q_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle41.mp3") == 0 )
	{
		SetWheatleyState( TF_PSAPSTATE_SPECIALIDLE_HACK );
		m_iWheatleyVOSequenceOffset = 0;
	}
	else if ( Q_strcmp( params.soundname, "vo/items/wheatley_sapper/wheatley_sapper_idle35.mp3") == 0 )
	{
		SetWheatleyState( TF_PSAPSTATE_SPECIALIDLE_KNIFE );
		m_iWheatleyVOSequenceOffset = 0;
	}
	
	//Play the sound
	// When playing a sound to all players, do it at the last known sapper location
	// This is not played on the object itself (building or sapper) cause it may not exist in the case of death and follow up audio
	// Also having it played on this entity itself prevents multiple VO playing in the case of mass sapping
	if ( bEmitToAll )
	{
		//int entIndex = 0;
		CBroadcastNonOwnerRecipientFilter filter( pOwner );
		EmitSound( filter, entindex(), snd, &m_vLastKnownSapPos );
	}

	// GetSoundDuration is not supported on Linux or for MP3s.  So lets just put in a good number
	//float flSoundDuration = enginesound->GetSoundDuration( params.soundname );
	float flSoundDuration = 3.0f;
	CSingleUserRecipientFilter filter( pOwner );
	EmitSound( filter, entindex(), params );
	m_flWheatleyTalkingUntil = gpGlobals->curtime + flSoundDuration;
	return flSoundDuration;
}

//-----------------------------------------------------------------------------
// Purpose: Set Wheatley Sapper State
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::SetWheatleyState( int iNewState )
{
	m_iSapState = iNewState;
}

int CTFWeaponBuilder::GetWheatleyIdleWait()
{
	return RandomInt( WHEATLEY_IDLE_WAIT_SECS_MIN, WHEATLEY_IDLE_WAIT_SECS_MAX );
}


//-----------------------------------------------------------------------------
// Purpose: Set Wheatley Sapper State
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WheatleyDamage( void )
{
	if ( (gpGlobals->curtime - m_flWheatleyLastDamage) > 10.0)
	{
		if ( RandomInt(0,2) == 0 )
		{
			CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
			if (pOwner)
			{
				pOwner->ClearSappingEvent();
			}
			SetWheatleyState( TF_PSAPSTATE_IDLE );
			m_flWheatleyLastDamage = gpGlobals->curtime;
			WheatleyEmitSound( "PSap.Damage" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Special Item Idle
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WheatleySapperIdle( CTFPlayer *pOwner )
{
	if ( pOwner && m_iObjectType == OBJ_ATTACHMENT_SAPPER && IsWheatleySapper())
	{
		//Is Wheatley coming out of the player's pocket?
		if( m_flNextVoicePakIdleStartTime < 0.0f )
		{			
			pOwner->ClearSappingTracking();
			float flSoundDuration;
			if ( gpGlobals->curtime - m_flWheatleyLastHolster < 2.0 && gpGlobals->curtime - m_flWheatleyLastHolster >= -1.00 )
			{
				flSoundDuration = WheatleyEmitSound( "Psap.DeployAgain" );
			}
			else
			{
				flSoundDuration = WheatleyEmitSound( (m_bWheatleyIntroPlayed) ? "Psap.Deploy" : "Psap.DeployIntro" );
			}
			m_flWheatleyLastDeploy = gpGlobals->curtime + flSoundDuration;
			if ( (!m_bWheatleyIntroPlayed) && (RandomInt(0,2) == 0) )
			{
				SetWheatleyState( TF_PSAPSTATE_INTRO );
				m_bWheatleyIntroPlayed = true;
				m_iWheatleyVOSequenceOffset = 0;
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + flSoundDuration + 3.0;
			}
			else
			{
				m_bWheatleyIntroPlayed = true;
				SetWheatleyState( TF_PSAPSTATE_IDLE );
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + flSoundDuration + GetWheatleyIdleWait();
			}
		}
		//Is there a sapper event? (sapper placed / sapper finished)
		else if( pOwner->GetSappingEvent() != TF_SAPEVENT_NONE)
		{
			char *pVoicePakString = NULL;
			switch ( pOwner->GetSappingEvent() )
			{
			case TF_SAPEVENT_PLACED:
				if (RandomInt(0,1) == 0)
				{
					if ( RandomInt(0,3) == 0 )
					{
						pVoicePakString = "PSap.AttachedPW";
						SetWheatleyState( TF_PSAPSTATE_WAITINGHACK );
					}
					else 
					{
						pVoicePakString = "PSap.Attached";
						SetWheatleyState( TF_PSAPSTATE_WAITINGHACKPW );
					}
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 0.2;
				}
				else
				{
					pVoicePakString = "PSap.Hacking";
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
				}
				break;
			case TF_SAPEVENT_DONE:
				if ( IsWheatleyTalking() )
				{
					if ( m_hLastSappedBuilding && m_hLastSappedBuilding.Get() )
					{
						//Building Alive, Sapper died
						pVoicePakString = "PSap.Death";
					}
					else
					{
						pVoicePakString = "PSap.HackedLoud";
					}
					
					if ( RandomInt( 0, 3 ) == 0 )
					{
						SetWheatleyState( TF_PSAPSTATE_WAITINGFOLLOWUP);
						m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 1.3;
					}
					else
					{
						m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
					}
				}
				else
				{
					SetWheatleyState( TF_PSAPSTATE_WAITINGHACKED );
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 0.5;
				}
				break;
			default:
				break;
			}
			pOwner->ClearSappingEvent();
			if ( pVoicePakString )
			{
				float flSoundDuration = WheatleyEmitSound( pVoicePakString, true );
				m_flNextVoicePakIdleStartTime += flSoundDuration;
			}
		}
		//Are we in the intro sequence?
		else if ( m_iSapState == TF_PSAPSTATE_INTRO && gpGlobals->curtime > m_flNextVoicePakIdleStartTime )
		{
			if ( !IsWheatleyTalking() )
			{
				char szVoicePakString[128];
				szVoicePakString[0] = '\0';
				if ( m_iWheatleyVOSequenceOffset >= 0 && m_iWheatleyVOSequenceOffset <=3 )
				{
					V_sprintf_safe( szVoicePakString, "PSap.IdleIntro0%i", ++m_iWheatleyVOSequenceOffset);
					float flSoundDuration = WheatleyEmitSound( szVoicePakString );
					if ( m_iWheatleyVOSequenceOffset == 4 )
					{
						m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait() + flSoundDuration;
					}
					else
					{
						m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 1.0 + flSoundDuration;
					}
				}
				else
				{
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_iWheatleyVOSequenceOffset = 0;
				}
			}
		}
		//Does a generic timed event need to be serviced?
		else if( gpGlobals->curtime > m_flNextVoicePakIdleStartTime )
		{
			bool bNoRepeats = false;
			bool bEmitAll = false;
			char *pVoicePakString = NULL;
			//Sapped! vo
			if ( m_iSapState == TF_PSAPSTATE_WAITINGHACKED )
			{
				bEmitAll = true;
				SetWheatleyState( TF_PSAPSTATE_IDLE );
				if ( IsWheatleyTalking() )
				{
					pVoicePakString = "PSap.HackedLoud";
				}
				else
				{
					pVoicePakString = "PSap.Hacked";
				}
				if ( RandomInt( 0, 3 ) == 0 )
				{
					SetWheatleyState( TF_PSAPSTATE_WAITINGFOLLOWUP);
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 1.3;
				}
				else
				{
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
				}
			}
			//Waiting to start the password guessing vo
			else if ( m_iSapState == TF_PSAPSTATE_WAITINGHACKPW )
			{
				bEmitAll = true;
				SetWheatleyState( TF_PSAPSTATE_IDLE );
				pVoicePakString = "PSap.HackingPW";
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			//Waiting to start regular hacking vo
			else if ( m_iSapState == TF_PSAPSTATE_WAITINGHACK )
			{
				bEmitAll = true;
				SetWheatleyState( TF_PSAPSTATE_IDLE );
				if ( RandomInt( 0, 2 ) == 0 )
				{
					pVoicePakString = "PSap.HackingShort";
				}
				else
				{
					pVoicePakString = "PSap.Hacking";
				}
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			//Waiting to start successful hack followup vo
			else if ( m_iSapState == TF_PSAPSTATE_WAITINGFOLLOWUP )
			{
				bEmitAll = true;
				SetWheatleyState( TF_PSAPSTATE_IDLE );
				pVoicePakString = "PSap.HackedFollowup";
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			//If Wheatley's talking, skip & check again later
			else if ( IsWheatleyTalking() )
			{
				pVoicePakString = NULL;
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 5.0;
			}
			//Are we in the SPECIAL IDLE SEQUENCE "HACK"?
			else if ( m_iSapState == TF_PSAPSTATE_SPECIALIDLE_HACK )
			{
				switch ( m_iWheatleyVOSequenceOffset )
				{
				case 0:
					pVoicePakString = "PSap.IdleHack02";
					m_iWheatleyVOSequenceOffset++;
					break;
				default:
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_iWheatleyVOSequenceOffset = 0;
					break;
				}
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			//Are we in the SPECIAL IDLE SEQUENCE "KNIFE"?
			else if ( m_iSapState == TF_PSAPSTATE_SPECIALIDLE_KNIFE )
			{
				switch ( m_iWheatleyVOSequenceOffset )
				{
				case 0:
					pVoicePakString = "PSap.IdleKnife02";
					m_iWheatleyVOSequenceOffset++;
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + 0.3;
					break;
				case 1:
					pVoicePakString = "PSap.IdleKnife03";
					m_iWheatleyVOSequenceOffset++;
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
					break;
				default:
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_iWheatleyVOSequenceOffset = 0;
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
					break;
				}
			}
			//Are we in the SPECIAL IDLE SEQUENCE "HARMLESS"?
			else if ( m_iSapState == TF_PSAPSTATE_SPECIALIDLE_HARMLESS )
			{
				switch ( m_iWheatleyVOSequenceOffset )
				{
				case 0:
					pVoicePakString = "PSap.IdleHarmless02";
					m_iWheatleyVOSequenceOffset++;
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
					break;
				default:
					SetWheatleyState( TF_PSAPSTATE_IDLE );
					m_iWheatleyVOSequenceOffset = 0;
					m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
					break;
				}
			}
			//Is the player stealthed?
			else if ( pOwner->m_Shared.IsStealthed() )
			{
				if ( RandomInt(0,1) == 0 )
				{
					pVoicePakString = "PSap.Sneak";
				}
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else if ( m_iSapState == TF_PSAPSTATE_IDLE )
			{
				pVoicePakString = "PSap.Idle";
				bNoRepeats = true;
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
			}
			else
			{
				pVoicePakString = NULL;
			}
			if (!pVoicePakString)
			{
				m_flNextVoicePakIdleStartTime = gpGlobals->curtime + GetWheatleyIdleWait();
				return;
			}
			float flSoundDuration = WheatleyEmitSound( pVoicePakString, bEmitAll, bNoRepeats );
			m_flNextVoicePakIdleStartTime += flSoundDuration;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Start placing the object
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartPlacement( void )
{
	StopPlacement();

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
	if ( !pTFPlayer )
		return;

	if ( pTFPlayer->m_Shared.IsCarryingObject() )
	{
		m_hObjectBeingBuilt = pTFPlayer->m_Shared.GetCarriedObject();
		m_hObjectBeingBuilt->StopFollowingEntity();
	}
	else
	{
		m_hObjectBeingBuilt = (CBaseObject*)CreateEntityByName( GetObjectInfo( m_iObjectType )->m_pClassName );
	}

	if ( m_hObjectBeingBuilt )
	{
		// Set the builder before Spawn() so attributes can hook correctly
		m_hObjectBeingBuilt->SetBuilder( pTFPlayer );

		bool bIsCarried = m_hObjectBeingBuilt->IsCarried();

		// split this off from the block at the bottom because we need to know what type of building
		// this is before we spawn so things like the teleporters have the correct placement models
		// but we need to set the starting construction health after we've called spawn
		if ( !bIsCarried )
		{
			m_hObjectBeingBuilt->SetObjectMode( m_iObjectMode );
		}

		m_hObjectBeingBuilt->Spawn();
		m_hObjectBeingBuilt->StartPlacement( pTFPlayer );

		if ( !bIsCarried )
		{
			m_hObjectBeingBuilt->m_iHealth = OBJECT_CONSTRUCTION_STARTINGHEALTH;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StopPlacement( void )
{
	if ( m_hObjectBeingBuilt )
	{
		if ( m_hObjectBeingBuilt->IsCarried() )
		{
			m_hObjectBeingBuilt->MakeCarriedObject( ToTFPlayer( GetOwner() ) );
		}
		else
		{
			m_hObjectBeingBuilt->StopPlacement();
		}
		m_hObjectBeingBuilt = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::WeaponReset( void )
{
	//Check to see if the active weapon is the wheatley sapper, and, if so, reset him
	if ( m_iObjectType == OBJ_ATTACHMENT_SAPPER )
	{
		if ( IsWheatleySapper() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
			if ( pPlayer )
			{
				pPlayer->ClearSappingTracking();
			}
			WheatleyReset();
		}
	}

	BaseClass::WeaponReset();

	StopPlacement();
}


//-----------------------------------------------------------------------------
// Purpose: Move the placement model to the current position. Return false if it's an invalid position
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::IsValidPlacement( void )
{
	if ( !m_hObjectBeingBuilt )
		return false;

	CBaseObject *pObj = m_hObjectBeingBuilt.Get();

	pObj->UpdatePlacement();

	return m_hObjectBeingBuilt->IsValidPlacement();
}

//-----------------------------------------------------------------------------
// Purpose: Player holding this weapon has started building something
// Assumes we are in a valid build position
//-----------------------------------------------------------------------------
void CTFWeaponBuilder::StartBuilding( void )
{
	CBaseObject *pObj = m_hObjectBeingBuilt.Get();

	Assert( pObj );

	pObj->StartBuilding( GetOwner() );

	m_hObjectBeingBuilt = NULL;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		pOwner->RemoveInvisibility();
		pOwner->m_Shared.SetCarriedObject( NULL );

		if ( TFGameRules() && TFGameRules()->GameModeUsesUpgrades() )
		{
			if ( pObj->ObjectType() == OBJ_ATTACHMENT_SAPPER )
			{
				// Let human players place player-targeted sappers in modes that allow upgrades
				if ( !pOwner->IsBot() && pObj->GetBuiltOnObject() && pObj->GetBuiltOnObject()->IsPlayer() )
				{
					int iRoboSapper = 0;
					CALL_ATTRIB_HOOK_INT_ON_OTHER( pOwner, iRoboSapper, robo_sapper );

					int nMode = iRoboSapper ? MODE_SAPPER_ANTI_ROBOT_RADIUS : MODE_SAPPER_ANTI_ROBOT;
					pObj->SetObjectMode( nMode );

					pOwner->RemoveAmmo( 1, TF_AMMO_GRENADES2 );
					StartEffectBarRegen();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::HasAmmo( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	int iCost = pOwner->m_Shared.CalculateObjectCost( pOwner, m_iObjectType );
	return ( pOwner->GetBuildResources() >= iCost );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBuilder::GetSlot( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBuilder::GetPosition( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetPrintName( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_pStatusName;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBuilder::CanBuildObjectType( int iObjectType )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return false;

	return m_aBuildableObjectTypes.Get( iObjectType );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBuilder::SetObjectTypeAsBuildable( int iObjectType )
{
	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return;

	m_aBuildableObjectTypes.Set( iObjectType, true );
	SetSubType( iObjectType );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
Activity CTFWeaponBuilder::TranslateViewmodelHandActivity( Activity actBase )
{
	if ( GetObjectInfo( m_iObjectType )->m_bUseItemInfo )
	{
		return BaseClass::TranslateViewmodelHandActivity( actBase );
	}
	else
	{
		return actBase;
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetViewModel( int iViewModel ) const
{
	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		if ( GetObjectInfo( m_iObjectType )->m_bUseItemInfo )
			return BaseClass::GetViewModel();

		return GetObjectInfo( m_iObjectType )->m_pViewModel;
	}

	return BaseClass::GetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBuilder::GetWorldModel( void ) const	
{
	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		return GetObjectInfo( m_iObjectType )->m_pPlayerModel;
	}

	return BaseClass::GetWorldModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBuilder::AllowsAutoSwitchTo( void ) const
{
	// ask the object we're building
	return GetObjectInfo( m_iObjectType )->m_bAutoSwitchTo;
}

// ****************************************************************************
// SAPPER
// ****************************************************************************
CTFWeaponSapper::CTFWeaponSapper()
{
	m_flChargeBeginTime = 0;
	m_bAttackDown = false;
}
//-----------------------------------------------------------------------------
void CTFWeaponSapper::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();
}
//-----------------------------------------------------------------------------
const char *CTFWeaponSapper::GetViewModel( int iViewModel ) const
{
	// Skip over Builder's version
	return CTFWeaponBase::GetViewModel();
}
//-----------------------------------------------------------------------------
const char *CTFWeaponSapper::GetWorldModel( void ) const	
{
	// Skip over Builder's version
	return CTFWeaponBase::GetWorldModel();
}
//-----------------------------------------------------------------------------
Activity CTFWeaponSapper::TranslateViewmodelHandActivity( Activity actBase )
{
	return BaseClass::TranslateViewmodelHandActivity( actBase );

	// Skip over Builder's version
	//return CTFWeaponBase::TranslateViewmodelHandActivity( actBase );
}

