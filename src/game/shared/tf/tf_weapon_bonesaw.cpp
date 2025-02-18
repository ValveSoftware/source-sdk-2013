//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_bonesaw.h"
#include "tf_weapon_medigun.h"
#include "tf_gamerules.h"
#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif


#define UBERSAW_CHARGE_POSEPARAM		"syringe_charge_level"
#define VITASAW_CHARGE_PER_HIT 0.15f

//=============================================================================
//
// Weapon Bonesaw tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBonesaw, DT_TFWeaponBonesaw )

BEGIN_NETWORK_TABLE( CTFBonesaw, DT_TFWeaponBonesaw )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFBonesaw )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_bonesaw, CTFBonesaw );
PRECACHE_WEAPON_REGISTER( tf_weapon_bonesaw );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::Activate( void )
{
	BaseClass::Activate();
}
//-----------------------------------------------------------------------------
void CTFBonesaw::SecondaryAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

#ifdef GAME_DLL
	int iSpecialTaunt = 0;
	CALL_ATTRIB_HOOK_INT( iSpecialTaunt, special_taunt );
	if ( iSpecialTaunt )
	{
		pPlayer->Taunt( TAUNT_BASE_WEAPON );
		return;
	}
#endif
	BaseClass::SecondaryAttack();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBonesaw::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	if ( BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt ) )
	{
#ifdef CLIENT_DLL
		UpdateChargePoseParam();
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::DoMeleeDamage( CBaseEntity* ent, trace_t& trace )
{
	if ( !TFGameRules() || !TFGameRules()->IsTruceActive() )
	{
		if ( ent && ent->IsPlayer() )
		{
			CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
			if ( pTFOwner && pTFOwner->GetTeamNumber() != ent->GetTeamNumber() )
			{
				int iDecaps = pTFOwner->m_Shared.GetDecapitations() + 1;

				int iTakeHeads = 0;
				CALL_ATTRIB_HOOK_INT( iTakeHeads, add_head_on_hit );
				if ( iTakeHeads )
				{
					// We hit a target, take a head
					pTFOwner->m_Shared.SetDecapitations( iDecaps );
					pTFOwner->TeamFortress_SetSpeed();
				}

				float flPreserveUber = 0.f;
				CALL_ATTRIB_HOOK_FLOAT( flPreserveUber, ubercharge_preserved_on_spawn_max );
				if ( flPreserveUber )
				{
					pTFOwner->m_Shared.SetDecapitations( iDecaps );

					CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pTFOwner->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
					if ( pMedigun )
					{
						pMedigun->SetChargeLevelToPreserve( ( iDecaps * VITASAW_CHARGE_PER_HIT ) );
					}
				}
			}
		}
	}

	BaseClass::DoMeleeDamage( ent, trace );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBonesaw::GetBoneSawSpeedMod( void ) 
{ 
	const int MAX_HEADS_FOR_SPEED = 10;
	// Calculate Speed based on heads
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );

	int iTakeHeads = 0;
	CALL_ATTRIB_HOOK_INT( iTakeHeads, add_head_on_hit );
	if ( pPlayer && iTakeHeads )
	{
		int iDecaps = Min( MAX_HEADS_FOR_SPEED, pPlayer->m_Shared.GetDecapitations() );
		return 1.f + (iDecaps * 0.05f);
	}
	return 1.f; 
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::OnPlayerKill( CTFPlayer *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::OnPlayerKill( pVictim, info );

	CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pTFOwner )
		return;

	int iTakeHeads = 0;
	CALL_ATTRIB_HOOK_INT( iTakeHeads, add_head_on_kill );
	if ( iTakeHeads )
	{
		int nOrgans = pTFOwner->m_Shared.GetDecapitations() + 1;
		pTFOwner->m_Shared.SetDecapitations( nOrgans );

		CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pTFOwner->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
		if ( pMedigun )
		{
			pMedigun->SetChargeLevelToPreserve( ( nOrgans * VITASAW_CHARGE_PER_HIT ) );
		}
	}
}
#endif


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	UpdateChargePoseParam(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::UpdateAttachmentModels( void )
{
	BaseClass::UpdateAttachmentModels();

	if ( m_hViewmodelAttachment )
	{
		m_iUberChargePoseParam = m_hViewmodelAttachment->LookupPoseParameter( m_hViewmodelAttachment->GetModelPtr(), UBERSAW_CHARGE_POSEPARAM );
	}
	else
	{
		m_iUberChargePoseParam = LookupPoseParameter( GetModelPtr(), UBERSAW_CHARGE_POSEPARAM );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::UpdateChargePoseParam( void )
{
	if ( m_iUberChargePoseParam >= 0 )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );
		if ( pTFPlayer && pTFPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			CWeaponMedigun *pMedigun = (CWeaponMedigun *)pTFPlayer->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
			if ( pMedigun )
			{
				m_flChargeLevel = pMedigun->GetChargeLevel();

				// On the local client, we push the pose parameters onto the attached model
				if ( m_hViewmodelAttachment )
				{
					m_hViewmodelAttachment->SetPoseParameter( m_iUberChargePoseParam, pMedigun->GetChargeLevel() );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBonesaw::GetPoseParameters( CStudioHdr *pStudioHdr, float poseParameter[MAXSTUDIOPOSEPARAM] )
{
	if ( !pStudioHdr )
		return;

	BaseClass::GetPoseParameters( pStudioHdr, poseParameter );

	if ( m_iUberChargePoseParam >= 0 )
	{
		poseParameter[m_iUberChargePoseParam] = m_flChargeLevel;
	}
}

#endif
