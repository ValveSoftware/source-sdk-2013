//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_ammo_pack.h"
#include "particle_parse.h"
#include "tf/halloween/ghost/ghost.h"
#include "tf_player.h"
#include "tf_gamerules.h"

#include "merasmus.h"
#include "merasmus_trick_or_treat_prop.h"

LINK_ENTITY_TO_CLASS( tf_merasmus_trick_or_treat_prop, CTFMerasmusTrickOrTreatProp );

IMPLEMENT_AUTO_LIST( ITFMerasmusTrickOrTreatProp );

ConVar tf_merasmus_prop_health( "tf_merasmus_prop_health", "150", FCVAR_CHEAT | FCVAR_GAMEDLL );

CTFMerasmusTrickOrTreatProp::CTFMerasmusTrickOrTreatProp()
{
}


void CTFMerasmusTrickOrTreatProp::Spawn()
{
	Precache();

	SetModel( CMerasmus::GetRandomPropModelName() );

	BaseClass::Spawn();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	m_takedamage = DAMAGE_YES;
	SetHealth( tf_merasmus_prop_health.GetInt() );

	DispatchParticleEffect( "merasmus_object_spawn", WorldSpaceCenter(), GetAbsAngles() );
}


void CTFMerasmusTrickOrTreatProp::Event_Killed( const CTakeDamageInfo &info )
{
	SpawnTrickOrTreatItem();

	DispatchParticleEffect( "merasmus_object_spawn", WorldSpaceCenter(), GetAbsAngles() );
	EmitSound( "Halloween.Merasmus_Hiding_Explode" );

	if ( TFGameRules()->GetActiveBoss() )
	{
		CMerasmus* pMerasmus = assert_cast< CMerasmus* >( TFGameRules()->GetActiveBoss() );
		if ( pMerasmus )
		{
			if ( pMerasmus->IsNextKilledPropMerasmus() )
			{
				// move merasmus to the destroyed prop before we reveal him
				pMerasmus->SetAbsOrigin( GetAbsOrigin() );

				if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
				{
					CTFPlayer* pTFPlayer = assert_cast< CTFPlayer* >( info.GetAttacker() );
					if ( pTFPlayer )
					{
						pMerasmus->SetRevealer( pTFPlayer );

						IGameEvent *pEvent = gameeventmanager->CreateEvent( "merasmus_prop_found" );
						if ( pEvent )
						{
							pEvent->SetInt( "player", pTFPlayer->GetUserID() );
							gameeventmanager->FireEvent( pEvent, true );
						}
					}
				}
			}
			else
			{
				CPVSFilter filter( pMerasmus->WorldSpaceCenter() );
				if (RandomInt(1,3) == 1)
				{
					pMerasmus->PlayLowPrioritySound( filter, "Halloween.MerasmusTauntFakeProp" ); 
				}
			}
		}
	}

	BaseClass::Event_Killed( info );
}


int CTFMerasmusTrickOrTreatProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	DispatchParticleEffect( "merasmus_blood", info.GetDamagePosition(), GetAbsAngles() );

	CTakeDamageInfo newinfo = info;

	CTFPlayer *pTFPlayer = ToTFPlayer( newinfo.GetAttacker() );
	if ( pTFPlayer && ( pTFPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || pTFPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) ) && ( newinfo.GetDamageType() & DMG_BLAST ) )
	{
		newinfo.SetDamage( GetHealth() * 2.f );
	}

	return BaseClass::OnTakeDamage( newinfo );
}


void CTFMerasmusTrickOrTreatProp::Touch( CBaseEntity *pOther )
{
	BaseClass::Touch( pOther );

	if ( pOther && pOther->IsPlayer() )
	{
		CTFPlayer *pPlayer = ToTFPlayer( pOther );
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) )
		{
			pPlayer->m_Shared.RemoveCond( TF_COND_HALLOWEEN_BOMB_HEAD );
			pPlayer->m_Shared.RemoveCond( TF_COND_STUNNED );
			pPlayer->MerasmusPlayerBombExplode( false );

			// force kill
			CTakeDamageInfo info( pPlayer, pPlayer, 99999, DMG_BLAST, TF_DMG_CUSTOM_NONE );
			Event_Killed( info );
		}
	}
}


CTFMerasmusTrickOrTreatProp* CTFMerasmusTrickOrTreatProp::Create( const Vector& vPosition, const QAngle& qAngles )
{
	CTFMerasmusTrickOrTreatProp *pTrickOrTreatProp = static_cast<CTFMerasmusTrickOrTreatProp*>( CBaseEntity::Create( "tf_merasmus_trick_or_treat_prop", vPosition, qAngles, NULL ) );

	// must be on a team different from player(s) in order for some
	// weapons to hit (ie: pipe bombs)
	if ( pTrickOrTreatProp )
	{
		pTrickOrTreatProp->ChangeTeam( TF_TEAM_HALLOWEEN );
	}

	return pTrickOrTreatProp;
}


void CTFMerasmusTrickOrTreatProp::SpawnTrickOrTreatItem()
{
	int nNumAmmo = 1/*RandomInt( 1, 3 )*/;
	for ( int i=0; i<nNumAmmo; ++i )
	{
		// Create the ammo pack.
		CTFAmmoPack *pAmmoPack = CTFAmmoPack::Create( WorldSpaceCenter(), vec3_angle, this, "models/items/ammopack_medium.mdl" );
		Assert( pAmmoPack );
		if ( pAmmoPack )
		{
			pAmmoPack->MakeHolidayPack();
			pAmmoPack->SetBonusScale( 2.f );
			pAmmoPack->SetModelScale( 1.4f );

			Vector vecRight, vecUp;
			AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );

			// Calculate the initial impulse on the weapon.
			Vector vecImpulse = RandomVector( 40.f, 80.f );
			vecImpulse.z *= Sign( vecImpulse.z ); // always go up

			pAmmoPack->SetInitialVelocity( vecImpulse );
			pAmmoPack->ApplyAbsVelocityImpulse( vecImpulse );


			// Give the ammo pack some health, so that trains can destroy it.
			pAmmoPack->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			pAmmoPack->m_takedamage = DAMAGE_YES;	
			pAmmoPack->SetHealth( 900 );

			pAmmoPack->SetBodygroup( 1, 1 );	
		}
	}
}
