//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Pumpkin Bomb
//
//=============================================================================
#include "cbase.h"
#include "tf_pumpkin_bomb.h"
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

#define PUMPKIN_MODEL "models/props_halloween/pumpkin_explode.mdl"
#define TEAM_PUMPKIN "models/props_halloween/pumpkin_explode_teamcolor.mdl"

LINK_ENTITY_TO_CLASS( tf_pumpkin_bomb, CTFPumpkinBomb );

IMPLEMENT_NETWORKCLASS_ALIASED( TFPumpkinBomb, DT_TFPumpkinBomb )

BEGIN_NETWORK_TABLE( CTFPumpkinBomb, DT_TFPumpkinBomb )
END_NETWORK_TABLE()

IMPLEMENT_AUTO_LIST( ITFPumpkinBomb );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFPumpkinBomb::CTFPumpkinBomb()
{
#ifdef GAME_DLL
	m_bIsSpell = false;
#endif

	m_bDead = false;
	m_bPrecached = false;

	m_iTeam		= TF_TEAM_HALLOWEEN;
	m_flDamage	= 150.0f;
	m_flScale	= 1.0f;
	m_flRadius	= 300.0f;
	m_flLifeTime = -1.0f;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPumpkinBomb::Precache()
{
	BaseClass::Precache();

	// always allow late precaching, so we don't pay the cost of the
	// Halloween pumpkin bomb for the entire year

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	int iModel = PrecacheModel( PUMPKIN_MODEL );
	PrecacheGibsForModel( iModel );

	PrecacheModel( TEAM_PUMPKIN );

	PrecacheScriptSound( "Halloween.PumpkinExplode" );

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
	
	m_bPrecached = true;
}

//-----------------------------------------------------------------------------
void CTFPumpkinBomb::SetInitParams( float scale, float damage, float radius, int iTeam, float flLifeTime )
{
	m_iTeam		= iTeam;
	m_flDamage	= damage;
	m_flScale	= scale;
	m_flRadius	= radius;
	
	if ( flLifeTime > 0 )
	{
		m_flLifeTime = flLifeTime;
	}
}

//-----------------------------------------------------------------------------
void CTFPumpkinBomb::Spawn()
{
	if ( !m_bPrecached )
	{
		Precache();
	}

	if ( m_iTeam != TF_TEAM_HALLOWEEN )
	{
		SetModel( TEAM_PUMPKIN );
		m_nSkin = m_iTeam == TF_TEAM_BLUE ? 2 : 1;	// This is actually opposite of what you'd think so Blue team can Hurt Blue Pumpkins
		SetCollisionGroup( TFCOLLISION_GROUP_TANK );
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
		SetSolid( SOLID_BBOX );
		SetHealth( 1 );
	}
	else
	{
		SetModel( PUMPKIN_MODEL );
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
		SetHealth( 1 );
	}
	
	BaseClass::Spawn();
	
	SetModelScale( m_flScale );
	m_takedamage = DAMAGE_YES;
	m_bDead = false;

	SetTouch( &CTFPumpkinBombShim::Touch );
#ifdef GAME_DLL
	if ( m_flLifeTime > 0 )
	{
		SetContextThink( &CTFPumpkinBomb::RemovePumpkin, gpGlobals->curtime + m_flLifeTime, "RemovePumpkin" );
	}
#endif
}

//-----------------------------------------------------------------------------
void CTFPumpkinBomb::RemovePumpkin()
{
#ifdef GAME_DLL
	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, m_iTeam == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue", GetAbsOrigin(), vec3_angle );
	UTIL_Remove( this );
#endif // GAME_DLL
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPumpkinBomb::PumpkinTouch( CBaseEntity *pOther )
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

		if ( m_iTeam != TF_TEAM_HALLOWEEN && pOther->GetTeamNumber() != m_iTeam )
		{
			RemovePumpkin();
		}
		else
		{
			TakeDamage( CTakeDamageInfo( pOther, pAttacker, 10.f, DMG_CRUSH ) );
		}
	}
	else if ( FStrEq( STRING(pOther->m_iClassname), "trigger_hurt" ) )
	{
		RemovePumpkin();
	}
#endif
}

//-----------------------------------------------------------------------------
#ifdef GAME_DLL
int	CTFPumpkinBomb::OnTakeDamage( const CTakeDamageInfo &info )
{
	CPVSFilter filter( GetAbsOrigin() );
	if ( m_iTeam != TF_TEAM_HALLOWEEN )
	{
		TE_TFParticleEffect( filter, 0.0, m_iTeam == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue", GetAbsOrigin(), vec3_angle );
	}

	// if damage is from same team, Setlife to one and pass to base class
	if ( info.GetAttacker()->GetTeamNumber() == m_iTeam )
	{
		SetHealth( 1 );
	}
	else if ( m_iTeam != TF_TEAM_HALLOWEEN )
	{
		RemovePumpkin();
		return 0;
	}

	return BaseClass::OnTakeDamage( info );
}
//-----------------------------------------------------------------------------
void CTFPumpkinBomb::Event_Killed( const CTakeDamageInfo &info )
{
	if ( m_bDead )
		return;

	m_bDead = true;

	if ( m_iTeam != TF_TEAM_HALLOWEEN && info.GetAttacker()->GetTeamNumber() != m_iTeam )
	{
		RemovePumpkin();
		return;
	}
	
	trace_t tr;
	Vector vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

	// Explosion effect and gibs.
	Vector vecOrigin = GetAbsOrigin();
	QAngle vecAngles = GetAbsAngles();
	CPVSFilter filter( vecOrigin );
	TE_TFExplosion( filter, 0.0f, vecOrigin, tr.plane.normal, TF_WEAPON_PUMPKIN_BOMB, -1 );
	TE_TFParticleEffect( filter, 0.0f, "pumpkin_explode", vecOrigin, vecAngles );

	// Deal damage.
	SetSolid( SOLID_NONE );
	if ( info.GetAttacker() )
	{
		ChangeTeam( info.GetAttacker()->GetTeamNumber() );
	}
	CTakeDamageInfo damage_info( this, info.GetAttacker(), NULL, m_flDamage, DMG_BLAST | DMG_HALF_FALLOFF | DMG_NOCLOSEDISTANCEMOD );
	damage_info.SetDamageCustom( m_bIsSpell ? TF_DMG_CUSTOM_SPELL_MIRV : TF_DMG_CUSTOM_PUMPKIN_BOMB );

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

	Break();
	
	BaseClass::Event_Killed( info );
}

void CTFPumpkinBomb::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}
#endif
