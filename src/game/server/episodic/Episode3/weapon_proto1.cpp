//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		357 - hand gun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "NPCEvent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "beam_flags.h"
#include "IEffects.h"
#include "Sprite.h"
#include "explode.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL
ConVar weapon_proto1_rate_of_fire( "weapon_proto1_rate_of_fire", "5" );
ConVar weapon_proto1_mass_limit( "weapon_proto1_mass_limit", "250" );
ConVar weapon_proto1_alt_fire_range( "weapon_proto1_alt_fire_range", "250" );
ConVar weapon_proto1_ping_delay( "weapon_proto1_ping_delay", "1" );
ConVar weapon_proto1_ping_radius( "weapon_proto1_ping_radius", "1200" );
ConVar weapon_proto1_penetrate_depth( "weapon_proto1_penetrate_depth", "24" );
ConVar weapon_proto1_exit_blast_radius( "weapon_proto1_exit_blast_radius", "100" );
ConVar weapon_proto1_echo_sound_duration( "weapon_proto1_echo_sound_duration", "0.25" );
#endif

#define PENETRATING_BOLT_MODEL "models/crossbow_bolt.mdl"

// The sound that is heard when the ping detects an object.
#define SONAR_ECHO_SOUND	"Weapon_Proto1.Echo"

// The sound the player hears when activating a ping
#define SONAR_PING_SOUND "Weapon_Proto1.Ping"

// The sound played by a physics object that is being consumed
#define CONSUME_OBJECT_SOUND "Weapon_AR2.NPC_Double"

//-----------------------------------------------------------------------------
// CWeaponProto1
//-----------------------------------------------------------------------------
class CWeaponProto1 : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponProto1, CBaseHLCombatWeapon );
public:

	CWeaponProto1( void );

	void	Precache( void );
	void	ItemPostFrame( void );
	void	PrimaryAttack( void );
	void	SecondaryAttack( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void	Equip( CBaseCombatCharacter *pOwner );

	void	FireHitscanBolt();
	bool	CanHitscanBoltReachTarget();

	void	FirePenetratingBolt();
	bool	CanPenetratingBoltReachTarget( CBaseEntity *pTarget );
	bool	CanConsumeObject( CBaseEntity *pObject );
	void	ConsumeObject( CBaseEntity *pObject );
	void	SendPing();
	void	DoPingEffect();
	void	DoPingSearch();
	void	PingEntity( CBaseEntity *pEntity );

	CBaseEntity *DetectObject();

	float	m_flTimeNextPing;
	int		m_iPingSpriteTexture;

	int		m_iEchoSoundsToDo;
	float	m_flTimeNextEchoSound;
	float	m_flEchoSoundDelay;

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( weapon_proto1, CWeaponProto1 );

PRECACHE_WEAPON_REGISTER( weapon_proto1 );

IMPLEMENT_SERVERCLASS_ST( CWeaponProto1, DT_WeaponProto1 )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponProto1 )
	DEFINE_FIELD( m_flTimeNextPing, FIELD_TIME ),
	DEFINE_FIELD( m_iEchoSoundsToDo, FIELD_INTEGER ),
	DEFINE_FIELD( m_flTimeNextEchoSound, FIELD_TIME ),
	DEFINE_FIELD( m_flEchoSoundDelay, FIELD_FLOAT ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponProto1::CWeaponProto1( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
	m_iEchoSoundsToDo = 0;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponProto1::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six spent shells
				for ( int i = 0; i < 6; i++ )
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
					data.m_nEntIndex = entindex();

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::FireHitscanBolt()
{
	if( !HasAnyAmmo() )
	{
		WeaponSound( EMPTY );
		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

	// We're committed to the shot now. So take the ammo.
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

	trace_t tr;
	// Trace the initial shot from the weapon
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * MAX_COORD_INTEGER, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
	NDebugOverlay::Line( vecSrc + Vector( 0, 0, -3 ), tr.endpos, 255, 255, 0, false, 0.1f );

	if( tr.m_pEnt == NULL )
	{
		// Bail out if we hit nothing at all.
		return;
	}

	// Record the entity that the beam struck.
	CBaseEntity *pObjectStruck = tr.m_pEnt;
	
	if( pObjectStruck->MyNPCPointer() && pObjectStruck->IsAlive() )
	{
		CTakeDamageInfo damageInfo( this, pOwner, tr.m_pEnt->GetHealth(), DMG_GENERIC | DMG_DISSOLVE );
		Vector force( 0, 0, 100.0f );
		damageInfo.SetDamageForce( force );
		damageInfo.SetDamagePosition( tr.endpos );
		tr.m_pEnt->TakeDamage( damageInfo );
	}

	// Mark the point where the shot entered an object.
	//NDebugOverlay::Cross3D( tr.endpos, 8, 255, 255, 255, false, 60.0f );

	// Trace the second, punched-through shot. First, we have to locate the exit point.
	// There are two methods for this. If the shot struck the world, we have to trace out of solid.
	// If the shot struck an entity, we simply trace again, ignoring that entity.
	CBaseEntity *pIgnoreEnt = NULL;

	if( pObjectStruck->IsWorld() )
	{
		pIgnoreEnt = pOwner;
		// World method. Trace till you leave solid, then continue.
		
		// Move the trace "cursor" into the surface
		Vector vecStart = tr.endpos + vecAiming * 1.0f;

		// Now see if this solid is thin enough to penetrate.
		UTIL_TraceLine( vecStart, vecStart + vecAiming * weapon_proto1_penetrate_depth.GetFloat(), MASK_SHOT, pIgnoreEnt, COLLISION_GROUP_NONE, &tr );

		if( tr.fractionleftsolid <= 0.0f || tr.fractionleftsolid >= 1.0f )
		{
			// Wall can not be penetrated (too thick)
			return;
		}

		// This is a bit hacky, overwriting this value, but it lets us re-use code below.
		tr.endpos = vecStart + vecAiming * (weapon_proto1_penetrate_depth.GetFloat() * tr.fractionleftsolid);
	}
	else
	{
		// Ignore whatever was struck.
		pIgnoreEnt = tr.m_pEnt;
	}

	//NDebugOverlay::Cross3D( tr.endpos, 8, 255, 255, 255, false, 60.0f );

	// Now just ignore the ent and trace again.
	UTIL_TraceLine( tr.endpos, tr.endpos + vecAiming * MAX_COORD_INTEGER, MASK_SHOT, pIgnoreEnt, COLLISION_GROUP_NONE, &tr );

	if( tr.m_pEnt != NULL )
	{
		NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 255, false, 0.1f );

		if( tr.m_pEnt->MyNPCPointer() && tr.m_pEnt->IsAlive() )
		{
			Vector force( 0, 0, 1 );
			CTakeDamageInfo damageInfo( this, pOwner, tr.m_pEnt->GetHealth(), DMG_GENERIC | DMG_DISSOLVE );
			damageInfo.SetDamageForce( force );
			damageInfo.SetDamagePosition( tr.endpos );
			tr.m_pEnt->TakeDamage( damageInfo );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	pOwner->GiveAmmo( 3, "ammo_proto1" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::Precache( void )
{
	BaseClass::Precache();
	m_iPingSpriteTexture = PrecacheModel( "sprites/physbeam.vmt" );

	UTIL_PrecacheOther( "penetrating_bolt" );

	PrecacheSound( SONAR_ECHO_SOUND );
	PrecacheSound( SONAR_PING_SOUND );
	PrecacheSound( CONSUME_OBJECT_SOUND );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	CBaseEntity *pObject = DetectObject();

	if( pObject && CanConsumeObject( pObject ) )
	{
		g_pEffects->Sparks( pObject->WorldSpaceCenter() );
	}

	if( m_iEchoSoundsToDo > 0 && gpGlobals->curtime >= m_flTimeNextEchoSound )
	{
		EmitSound( SONAR_ECHO_SOUND );
		m_iEchoSoundsToDo--;

		m_flTimeNextEchoSound = gpGlobals->curtime + m_flEchoSoundDelay;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponProto1::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	//FirePenetratingBolt();
	FireHitscanBolt();

	float flRateOfFireDelay = 1.0f / weapon_proto1_rate_of_fire.GetFloat();
	m_flNextPrimaryAttack = gpGlobals->curtime + flRateOfFireDelay;

	pPlayer->ViewPunch( QAngle( -1, random->RandomFloat( -1, 1 ), 0 ) );

	if ( !HasAnyAmmo() )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;

	// See if we can detect an object that can be eaten
	CBaseEntity *pObject = DetectObject();
	if( pObject )
	{
		if( CanConsumeObject( pObject ) )
		{
			ConsumeObject( pObject );
		}

		return;
	}

	// Otherwise, try to Ping
	if( m_flTimeNextPing > gpGlobals->curtime )
	{
		// Too soon!
		return;
	}

	SendPing();
}

//-----------------------------------------------------------------------------
// Do a bunch of computations and determine whether a bolt fired from this location
// could punch through a wall and harm this target. This function assumes that
// the attacker does NOT have line of sight to the target.
//
// A bolt can reach the target IF:
//	-The target is not protected by a solid that is thicker than 
//	 the weapon's ability to penetrate solids
//
//	-Only one solid separates the attacker from the target.
//-----------------------------------------------------------------------------
bool CWeaponProto1::CanPenetratingBoltReachTarget( CBaseEntity *pTarget )
{
	trace_t tr;
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if( !pPlayer )
	{
		return false;
	}

	UTIL_TraceLine( pTarget->WorldSpaceCenter(), pPlayer->EyePosition(), MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr );

	// This target is too far from the wall if his world space center is farther from the wall than the weapon's blast radius.
	// (The explosion that accompanies the bolt's exit from the solid would not reach the target)
	float flDistFromWall = (tr.startpos - tr.endpos).Length();

	if( flDistFromWall > weapon_proto1_exit_blast_radius.GetFloat() )
		return false;
	
	// At this point in the function, we know that the target is close enough to the solid to be harmed by the explosion
	// caused when the bolt exits the solid. Now we just need to check to see if the solid is thin enough for the bolt to
	// pass through. To do this, we trace from the player to the target and measure the distance between the two traces' endpoints.
	Vector vecTargetEndPos = tr.endpos;
	UTIL_TraceLine( pPlayer->EyePosition(), pTarget->WorldSpaceCenter(), MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr );
	float flSolidThickness = (tr.endpos - vecTargetEndPos).Length();

	if( flSolidThickness > weapon_proto1_penetrate_depth.GetFloat() )
	{
		// In this case, the endpoints of the target's traceline and the player's traceline are farther apart than the bolt's penetration
		// depth. This either means that there is a single solid between the two that is too thick to penetrate, or that there are 
		// several solids between the two. Either is a failure case. 
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponProto1::CanConsumeObject( CBaseEntity *pObject )
{
	IPhysicsObject *pPhysObject = pObject->VPhysicsGetObject();

	if( !pPhysObject )
	{
		Msg("Object does not have VPhysics\n");
		return false;
	}

	if( pPhysObject->GetMass() > weapon_proto1_mass_limit.GetFloat() )
	{
		Msg("Object too heavy\n");
		return false;
	}

	if( !pObject->ClassMatches( "prop_physics" ) )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::ConsumeObject( CBaseEntity *pObject )
{
	CBaseAnimating *pAnimating = dynamic_cast<CBaseAnimating*>(pObject);

	if( !pAnimating )
	{
		Msg("FAILED to consume %s\n", pObject->GetClassname() );
		return;
	}

	pAnimating->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_CORE );
	pAnimating->EmitSound( CONSUME_OBJECT_SOUND );
		
	Msg("GULP!\n");

	m_flNextSecondaryAttack = gpGlobals->curtime + 1.0f;

	GetOwner()->GiveAmmo( 1, "ammo_proto1" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::SendPing()
{
	EmitSound( SONAR_PING_SOUND );

	DoPingEffect();
	DoPingSearch();
	m_flTimeNextPing = gpGlobals->curtime + weapon_proto1_ping_delay.GetFloat();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::DoPingEffect() 
{
	CBroadcastRecipientFilter filter;

	te->BeamRingPoint( filter, 0, GetAbsOrigin(),	//origin
		8.0f,	//start radius
		weapon_proto1_ping_radius.GetFloat() * 2.0f,		//end radius
		m_iPingSpriteTexture, //texture
		0,			//halo index
		0,			//start frame
		2,			//framerate
		0.5f,		//life
		64,			//width
		0,			//spread
		0,			//amplitude
		255,	//r
		255,	//g
		225,	//b
		128,	//a
		0,		//speed
		FBEAM_FADEOUT
		);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::DoPingSearch() 
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	Assert( pPlayer != NULL );

	CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAIs = g_AI_Manager.NumAIs();

	float flMaxDistSqr = Square( weapon_proto1_ping_radius.GetFloat() );
	int iFoundTargets = 0;

	for ( int i = 0; i < nAIs; i++ )
	{
		CAI_BaseNPC *pNPC = ppAIs[ i ];
		// Start rejecting.

		// Dead
		if( !pNPC->IsAlive() )
			continue;

		// Out of range
		if( pNPC->GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() ) > flMaxDistSqr )
			continue;

		PingEntity( pNPC );
		iFoundTargets++;
	}

	// Now we set up to make the echo sounds. Store off how many to make, (one per target discovered)
	// when to do the next one, and the time between them. This makes sure
	// that the weapon makes the correct number of echo sounds over the 
	// lifetime of the ping. It's not synchronized with the visual effect,
	// but it still works well. (sjb)
	m_iEchoSoundsToDo = iFoundTargets;
	m_flTimeNextEchoSound = gpGlobals->curtime;
	m_flEchoSoundDelay = weapon_proto1_echo_sound_duration.GetFloat() / ((float)iFoundTargets);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponProto1::PingEntity( CBaseEntity *pEntity )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

	if( !pPlayer || !pNPC )
		return;

	bool bEnemy = pNPC->IRelationType( pPlayer ) != D_LI;

	if( bEnemy )
	{
		pNPC->StartPingEffect();
	}
}

//-----------------------------------------------------------------------------
// See if the proto weapon is pointed at a physics object that can be eaten
//-----------------------------------------------------------------------------
CBaseEntity *CWeaponProto1::DetectObject()
{
	trace_t tr;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * weapon_proto1_alt_fire_range.GetFloat(), (MASK_SHOT|CONTENTS_GRATE), pPlayer, COLLISION_GROUP_NONE, &tr );

	if( tr.m_pEnt && tr.m_pEnt->VPhysicsGetObject() != NULL )
	{
		return tr.m_pEnt;
	}

	return NULL;
}
