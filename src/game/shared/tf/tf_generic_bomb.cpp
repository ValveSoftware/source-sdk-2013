//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Generic Bomb
//
//=============================================================================
#include "cbase.h"
#include "tf_generic_bomb.h"
#include "takedamageinfo.h"
#include "tf_shareddefs.h"
#include "props_shared.h"
#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "tf_projectile_base.h"
#include "basegrenade_shared.h"
#include "tf_gamerules.h"
#include "tf_weaponbase_rocket.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_generic_bomb, CTFGenericBomb );

IMPLEMENT_NETWORKCLASS_ALIASED( TFGenericBomb, DT_TFGenericBomb )

BEGIN_NETWORK_TABLE( CTFGenericBomb, DT_TFGenericBomb )
END_NETWORK_TABLE()

IMPLEMENT_AUTO_LIST( ITFGenericBomb );

#ifdef GAME_DLL
BEGIN_DATADESC( CTFGenericBomb )
	// Keyfields
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "damage" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_nHealth, FIELD_INTEGER, "health" ),
	DEFINE_KEYFIELD( m_strExplodeParticleName, FIELD_STRING, "explode_particle" ),
	DEFINE_KEYFIELD( m_strExplodeSoundName, FIELD_STRING, "sound" ),
	DEFINE_KEYFIELD( m_eWhoToDamage, FIELD_INTEGER, "friendlyfire" ),
	DEFINE_KEYFIELD( m_bPassActivator, FIELD_BOOLEAN, "passActivator" ),

	// Output
	DEFINE_OUTPUT( m_OnDetonate, "OnDetonate" ),

	// Input
	DEFINE_INPUTFUNC( FIELD_VOID, "Detonate", Detonate ),

END_DATADESC()
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFGenericBomb::CTFGenericBomb()
{
	m_bDead = false;
	m_bPrecached = false;
	m_bPassActivator = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGenericBomb::Precache()
{
	BaseClass::Precache();

	// always allow late precaching
	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	int iModel = PrecacheModel( STRING( GetModelName() ) );
	PrecacheGibsForModel( iModel );
	PrecacheModel( STRING( GetModelName() ) );
	if ( STRING( m_strExplodeParticleName ) && STRING( m_strExplodeParticleName )[0] )
	{
		PrecacheParticleSystem( STRING( m_strExplodeParticleName ) );
	}

	if ( STRING( m_strExplodeSoundName ) && STRING( m_strExplodeSoundName )[0] )
	{
		PrecacheScriptSound( STRING( m_strExplodeSoundName ) );
	}
	
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
	
	m_bPrecached = true;
}

//-----------------------------------------------------------------------------
void CTFGenericBomb::Spawn()
{
	if ( !m_bPrecached )
	{
		Precache();
	}

#ifdef GAME_DLL
	SetModel( STRING( GetModelName() ) );
#endif
	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );
	
	SetHealth( m_nHealth );

	BaseClass::Spawn();
	
	m_takedamage = DAMAGE_YES;
	m_bDead = false;

	SetTouch( &CTFGenericBombShim::Touch );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFGenericBomb::GenericTouch( CBaseEntity *pOther )
{
	if ( !pOther )
		return;

#ifdef GAME_DLL
	if ( pOther->GetFlags() & FL_GRENADE )
	{
		// Only let my team destroy
		CBaseEntity *pAttacker = NULL;

		CBaseGrenade *pGrenade = dynamic_cast<CBaseGrenade*>(pOther);
		if ( pGrenade )
		{
			pAttacker = pGrenade->GetThrower();
			// Do a proper explosion
			Vector velDir = pGrenade->GetAbsVelocity();
			VectorNormalize( velDir );
			Vector vecSpot = pGrenade->GetAbsOrigin() - velDir * 32;
			trace_t	tr;
			UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
			// Boom
			pGrenade->Explode( &tr, DMG_BLAST );
		}
		else
		{
			CTFBaseRocket *pRocket = dynamic_cast<CTFBaseRocket*>(pOther);
			if ( pRocket )
			{
				Vector velDir = pRocket->GetAbsVelocity();
				VectorNormalize( velDir );
				Vector vecSpot = pRocket->GetAbsOrigin() - velDir * 32;
				trace_t	tr;
				UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );
				pRocket->Explode( &tr, this );
			}
		}

		if ( !pAttacker )
		{
			CTFBaseProjectile *pProj = dynamic_cast<CTFBaseProjectile*>(pOther);
			if ( pProj )
			{
				pAttacker = pProj->GetScorer();
			}
		}

		TakeDamage( CTakeDamageInfo( pOther, pAttacker, 10.f, DMG_CRUSH ) );
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef GAME_DLL

void CTFGenericBomb::Detonate( inputdata_t& inputdata )
{
	CTakeDamageInfo info;
	Event_Killed( info );
}

//-----------------------------------------------------------------------------
void CTFGenericBomb::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_bDead )
		return;

	m_bDead = true;
	
	trace_t tr;
	Vector vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	// Explosion effect and gibs.
	Vector vecOrigin = GetAbsOrigin();
	QAngle vecAngles = GetAbsAngles();
	int iAttachment = LookupAttachment( "alt-origin" );
	if ( iAttachment > 0 )
	{
		GetAttachment( iAttachment, vecOrigin, vecAngles );
	}
	CPVSFilter pvsFilter( vecOrigin );
	if ( STRING( m_strExplodeParticleName ) && STRING( m_strExplodeParticleName )[0] )
	{
		TE_TFParticleEffect( pvsFilter, 0.0f, STRING( m_strExplodeParticleName ), vecOrigin, vecAngles );
	}

	if ( STRING( m_strExplodeSoundName ) && STRING( m_strExplodeSoundName )[0] )
	{
		EmitSound( STRING( m_strExplodeSoundName ) );
	}

	// Get the owner out of the attacker in case of arrows hitting the bomb.
	CBaseEntity* pAttacker = info.GetAttacker();
	if ( pAttacker && pAttacker->GetOwnerEntity() )
	{
		pAttacker = pAttacker->GetOwnerEntity();
	}

	// Deal damage.
	SetSolid( SOLID_NONE );
	if ( pAttacker )
	{
		ChangeTeam( pAttacker->GetTeamNumber() );
	}

	CTakeDamageInfo damage_info( this, pAttacker, NULL, m_flDamage, DMG_BLAST | DMG_HALF_FALLOFF | DMG_NOCLOSEDISTANCEMOD );
	damage_info.SetDamageCustom( TF_DMG_CUSTOM_NONE );
	if ( !V_stricmp( gpGlobals->mapname.ToCStr(), "koth_slaughter_event" ) || !V_stricmp( gpGlobals->mapname.ToCStr(), "koth_slime" ) )
	{
		damage_info.SetDamageCustom( TF_DMG_CUSTOM_PUMPKIN_BOMB );
	}

	damage_info.SetForceFriendlyFire( m_eWhoToDamage == DAMAGE_EVERYONE );

	if ( TFGameRules() )
	{
		CTFRadiusDamageInfo radiusinfo( &damage_info, vecOrigin, m_flRadius, this );
		TFGameRules()->RadiusDamage( radiusinfo );
	}

	// Don't decal players with scorch.
	if ( tr.m_pEnt && !tr.m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( &tr, "Scorch" );
	}

	// Spawns gibs on the client
	UserMessageBegin( pvsFilter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( vecOrigin );
		WRITE_ANGLES( vecAngles );
		WRITE_SHORT( m_nSkin );
	MessageEnd();

	CBaseEntity *pArg = m_bPassActivator ? pAttacker : this;
	m_OnDetonate.FireOutput( pArg, this );
	
	BaseClass::Event_Killed( info );
}

#endif
