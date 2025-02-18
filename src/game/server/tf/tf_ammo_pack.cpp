//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_ammo_pack.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

extern void SendProxy_FuncRotatingAngle( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFAmmoPack, DT_AmmoPack )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 0), 7, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 1), 7, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angRotation, 2), 7, SPROP_CHANGES_OFTEN, SendProxy_FuncRotatingAngle ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFAmmoPack )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_ammo_pack, CTFAmmoPack );

PRECACHE_REGISTER( tf_ammo_pack );

#define HALLOWEEN_MODEL "models/props_halloween/pumpkin_loot.mdl"
#define CHRISTMAS_MODEL "models/items/tf_gift.mdl"

void CTFAmmoPack::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime + 0.75f );
	SetThink( &CTFAmmoPack::FlyThink );

	SetTouch( &CTFAmmoPack::PackTouch );

	m_flCreationTime = gpGlobals->curtime;

	// default to medium ammopack
	m_flAmmoRatio = 0.5f;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;
	m_bNoPickup = false;
	m_bHealthInstead = false;
	m_bEmptyPack = false;
	m_bObjGib = false;
	m_flBonusScale = 1.f;

	// no ammo to start
	memset( m_iAmmo, 0, sizeof(m_iAmmo) );

	// Die in 30 seconds
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 30, "DieContext" );

	if ( IsX360() )
	{
		RemoveEffects( EF_ITEM_BLINK );
	}
}

void CTFAmmoPack::Precache( void )
{
	PrecacheModel( "models/items/ammopack_medium.mdl" );

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) )
		{
			PrecacheModel( HALLOWEEN_MODEL );
			PrecacheScriptSound( "Halloween.PumpkinDrop" );
			PrecacheScriptSound( "Halloween.PumpkinPickup" );
		}
		else if ( TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			PrecacheModel( CHRISTMAS_MODEL );
			PrecacheScriptSound( "Christmas.GiftDrop" );
			PrecacheScriptSound( "Christmas.GiftPickup" );
		}
	}
}

CTFAmmoPack *CTFAmmoPack::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName )
{
	CTFAmmoPack *pAmmoPack = static_cast<CTFAmmoPack*>( CBaseAnimating::CreateNoSpawn( "tf_ammo_pack", vecOrigin, vecAngles, pOwner ) );
	if ( pAmmoPack )
	{
		pAmmoPack->SetModelName( AllocPooledString( pszModelName ) );
		DispatchSpawn( pAmmoPack );
	}

	return pAmmoPack;
}

ConVar tf_weapon_ragdoll_velocity_min( "tf_weapon_ragdoll_velocity_min", "100", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_weapon_ragdoll_velocity_max( "tf_weapon_ragdoll_velocity_max", "150", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );
ConVar tf_weapon_ragdoll_maxspeed( "tf_weapon_ragdoll_maxspeed", "300", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY );

void CTFAmmoPack::InitAmmoPack( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, int nSkin, bool bEmpty, bool bIsSuicide, float flAmmoRatio /*= 0.5f*/ )
{
	m_flAmmoRatio = flAmmoRatio;

	if ( !bEmpty )
	{
		// Might be a holiday pack.
		if ( !bIsSuicide && ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) || TFGameRules()->IsHolidayActive( kHoliday_TFBirthday ) ) )
		{
			float frand = (float)rand() / VALVE_RAND_MAX;
			if ( frand < 0.3f )
			{
				MakeHolidayPack();
			}
		}
		else if ( !bIsSuicide && TFGameRules()->IsHolidayActive( kHoliday_Christmas ) )
		{
			MakeHolidayPack();
		}

		// Fill the ammo pack with unused player ammo, if out add a minimum amount.
		int iPrimary = Max( 5, pPlayer->GetAmmoCount( TF_AMMO_PRIMARY ) );
		int iSecondary = Max( 5, pPlayer->GetAmmoCount( TF_AMMO_SECONDARY ) );
		int iMetal = Clamp( pPlayer->GetAmmoCount( TF_AMMO_METAL ), 5 , 100 );

		// Fill up the ammo pack.
		GiveAmmo( iPrimary, TF_AMMO_PRIMARY );			// Gets recalculated in PackTouch
		GiveAmmo( iSecondary, TF_AMMO_SECONDARY );		// Gets recalculated in PackTouch
		GiveAmmo( iMetal, TF_AMMO_METAL );
		SetHealthInstead( pWeapon && pWeapon->GetWeaponID() == TF_WEAPON_LUNCHBOX && pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) );
	}
	else
	{
		// This pack has nothing in it.
		MakeEmptyPack();
	}

	Vector vecRight, vecUp;
	AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );

	// Calculate the initial impulse on the weapon.
	Vector vecImpulse( 0.0f, 0.0f, 0.0f );
	vecImpulse += vecUp * random->RandomFloat( -0.25, 0.25 );
	vecImpulse += vecRight * random->RandomFloat( -0.25, 0.25 );
	VectorNormalize( vecImpulse );
	vecImpulse *= random->RandomFloat( tf_weapon_ragdoll_velocity_min.GetFloat(), tf_weapon_ragdoll_velocity_max.GetFloat() );
	vecImpulse += GetAbsVelocity();

	// Cap the impulse.
	float flSpeed = vecImpulse.Length();
	if ( flSpeed > tf_weapon_ragdoll_maxspeed.GetFloat() )
	{
		VectorScale( vecImpulse, tf_weapon_ragdoll_maxspeed.GetFloat() / flSpeed, vecImpulse );
	}

	if ( VPhysicsGetObject() )
	{
		// We can probably remove this when the mass on the weapons is correct!
		VPhysicsGetObject()->SetMass( 25.0f );
		AngularImpulse angImpulse( 0, random->RandomFloat( 0, 100 ), 0 );
		VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
	}

	SetInitialVelocity( vecImpulse );

	m_nSkin = nSkin; // Copy the skin from the model we're copying

	// Give the ammo pack some health, so that trains can destroy it.
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	m_takedamage = DAMAGE_YES;
	SetHealth( 900 );

	SetBodygroup( 1, 1 );
}

void CTFAmmoPack::MakeHolidayPack( void )
{
	// don't want special ammo packs during a competitive match
	if ( TFGameRules()->IsMatchTypeCompetitive() )
		return;

	// Only do this on the halloween maps.
	if ( TFGameRules()->IsHolidayActive( kHoliday_Halloween ) 
		&& TFGameRules()->IsHolidayMap( kHoliday_Halloween ) 
		&& !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER ) )
	{
		m_PackType = AP_HALLOWEEN;
		SetModelIndex( modelinfo->GetModelIndex( HALLOWEEN_MODEL ) );
		SetContextThink( &CTFAmmoPack::DropSoundThink, gpGlobals->curtime + 0.1f, "DROP_SOUND_THINK" );
	}
	else if ( TFGameRules()->ShouldMakeChristmasAmmoPack() )
	{
		m_PackType = AP_CHRISTMAS;
		SetModelIndex( modelinfo->GetModelIndex( CHRISTMAS_MODEL ) );
		SetContextThink( &CTFAmmoPack::DropSoundThink, gpGlobals->curtime + 0.1f, "DROP_SOUND_THINK" );
	}
}


void CTFAmmoPack::SetBonusScale( float flBonusScale /*= 1.f*/ )
{
	m_flBonusScale = flBonusScale;
}


void CTFAmmoPack::SetInitialVelocity( Vector &vecVelocity )
{ 
	if ( m_PackType != AP_NORMAL )
	{
		// Unusual physics for the halloween/christmas packs to make them noticable.
		SetMoveType( MOVETYPE_FLYGRAVITY );
		SetAbsVelocity( vecVelocity * 2.f + Vector(0,0,200) );
		SetAbsAngles( QAngle(0,0,0) );
		UseClientSideAnimation();
		ResetSequence( LookupSequence("idle") );
	}
	m_vecInitialVelocity = vecVelocity;
}

void CTFAmmoPack::SetPickupThinkTime( float flNewThinkTime )
{
	SetNextThink( gpGlobals->curtime + flNewThinkTime );
}

int CTFAmmoPack::GiveAmmo( int iCount, int iAmmoType )
{
	if (iAmmoType == -1 || iAmmoType >= TF_AMMO_COUNT )
	{
		Msg("ERROR: Attempting to give unknown ammo type (%d)\n", iAmmoType);
		return 0;
	}

	m_iAmmo[iAmmoType] = iCount;

	return iCount;
}

void CTFAmmoPack::DropSoundThink( void )
{
	if ( m_PackType == AP_HALLOWEEN )
	{
		EmitSound( "Halloween.PumpkinDrop" );
	}
	else if ( m_PackType == AP_CHRISTMAS )
	{
		EmitSound( "Christmas.GiftDrop" );
	}
}

void CTFAmmoPack::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
	m_bNoPickup = false;
}

void CTFAmmoPack::PackTouch( CBaseEntity *pOther )
{
	Assert( pOther );

	if ( pOther->IsWorld() && ( m_PackType != AP_NORMAL ) )
	{
		Vector absVel = GetAbsVelocity();
		SetAbsVelocity( Vector( 0,0,absVel.z ) );
		return;
	}

	if( !pOther->IsPlayer() )
		return;

	if( !pOther->IsAlive() )
		return;

	if ( m_bNoPickup )
		return;

	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if( GetOwnerEntity() == pOther && m_bAllowOwnerPickup == false )
		return;

	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	Assert( pPlayer );

	if ( m_bEmptyPack )
	{
		// Since we drop our empty packs as fakeouts, we never pick up our own empties while stealthed.
		if ( GetOwnerEntity() == pOther && ( pPlayer->m_Shared.IsStealthed() ||
			pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ) )
			return;

		// "Empty" packs can be picked up.
		// Packs that can't be grabbed don't fit the expectations of the player.
		GiveAmmo( 1, TF_AMMO_PRIMARY );
		UTIL_Remove( this );
		return;
	}

	// The sandwich gives health instead of ammo
	if ( m_bHealthInstead )
	{
		// Let the sandwich fall to the ground for a bit so that people see it
		if ( !m_bAllowOwnerPickup )
			return;

		// Scouts get a little more, as a reference to the scout movie
		int iAmount = ( pPlayer->IsPlayerClass(TF_CLASS_SCOUT) ) ? 75 : 50;
		pPlayer->TakeHealth( iAmount, DMG_GENERIC );
		IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
		if ( event )
		{
			event->SetInt( "amount", iAmount );
			event->SetInt( "entindex", pPlayer->entindex() );
			event->SetInt( "weapon_def_index", INVALID_ITEM_DEF_INDEX );
			gameeventmanager->FireEvent( event );
		}

		event = gameeventmanager->CreateEvent( "player_stealsandvich" );
		if ( event )
		{
			if ( ToTFPlayer( GetOwnerEntity() ) )
			{
				event->SetInt( "owner", ToTFPlayer( GetOwnerEntity() )->GetUserID() );
			}
			event->SetInt( "target", pPlayer->GetUserID() );
			gameeventmanager->FireEvent( event );
		}

		UTIL_Remove( this );
		return;
	}

	int iMaxPrimary = pPlayer->GetMaxAmmo(TF_AMMO_PRIMARY);
	GiveAmmo( ceil( iMaxPrimary * m_flAmmoRatio ), TF_AMMO_PRIMARY );

	int iMaxSecondary = pPlayer->GetMaxAmmo(TF_AMMO_SECONDARY);
	GiveAmmo( ceil( iMaxSecondary * m_flAmmoRatio ), TF_AMMO_SECONDARY );

	int iAmmoTaken = 0;

	for ( int i=0;i<TF_AMMO_COUNT;i++ )
	{
		int iAmmoGiven = pPlayer->GiveAmmo( m_iAmmo[i], i );
		if ( iAmmoGiven > 0 && i == TF_AMMO_METAL && m_bObjGib && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			pPlayer->AwardAchievement( ACHIEVEMENT_TF_ENGINEER_WASTE_METAL_GRIND, iAmmoGiven );
		}
		iAmmoTaken += iAmmoGiven;
	}

	// give them a chunk of cloak power
	if ( pPlayer->m_Shared.AddToSpyCloakMeter( 100.0f * m_flAmmoRatio ) )
	{
		iAmmoTaken++;
	}

	if ( pPlayer->AddToSpyKnife( 100.0f * m_flAmmoRatio, false ) )
	{
		iAmmoTaken++;
	}

	// Add Charge if applicable
	int iAmmoIsCharge = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pPlayer, iAmmoIsCharge, ammo_gives_charge );
	if ( iAmmoIsCharge )
	{
		float flCurrentCharge = pPlayer->m_Shared.GetDemomanChargeMeter();
		if ( flCurrentCharge < 100.0f )
		{
			if ( TFGameRules() && TFGameRules()->IsPowerupMode() )
			{
				m_flAmmoRatio *= 0.2;
			}
			pPlayer->m_Shared.SetDemomanChargeMeter( flCurrentCharge + m_flAmmoRatio * 100.0f );
			iAmmoTaken++;
		}
	}

	if ( pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		int iMaxGrenades1 = pPlayer->GetMaxAmmo( TF_AMMO_GRENADES1 );
		iAmmoTaken += pPlayer->GiveAmmo( ceil(iMaxGrenades1 * m_flAmmoRatio), TF_AMMO_GRENADES1 );
	}

	if ( m_PackType == AP_HALLOWEEN )
	{
		// Send a message for the achievement tracking.
		IGameEvent *event = gameeventmanager->CreateEvent( "halloween_pumpkin_grab" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			gameeventmanager->FireEvent( event );
		}

		float flBuffDuration = m_flBonusScale * 3.f;
		if ( !pPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED_PUMPKIN ) || (pPlayer->m_Shared.GetConditionDuration(TF_COND_CRITBOOSTED_PUMPKIN) < flBuffDuration) )
		{
			pPlayer->m_Shared.AddCond( TF_COND_CRITBOOSTED_PUMPKIN, flBuffDuration );
		}
		pPlayer->EmitSound( "Halloween.PumpkinPickup" );
		m_PackType = AP_NORMAL; // Touch once.
		iAmmoTaken++;
	}
	else if ( m_PackType == AP_CHRISTMAS )
	{
		// Send a message for the achievement tracking.
		IGameEvent *event = gameeventmanager->CreateEvent( "christmas_gift_grab" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			gameeventmanager->FireEvent( event );
		}
		pPlayer->EmitSound( "Christmas.GiftPickup" );
		m_PackType = AP_NORMAL; // Touch once.
		iAmmoTaken++;
	}

	if ( iAmmoTaken > 0 )
	{
		CTF_GameStats.Event_PlayerAmmokitPickup( pPlayer );

		IGameEvent * event = gameeventmanager->CreateEvent( "item_pickup" );
		if( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "item", "tf_ammo_pack" );
			gameeventmanager->FireEvent( event );
		}

		UTIL_Remove( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFAmmoPack::PhysicsSolidMaskForEntity( void ) const
{ 
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}
