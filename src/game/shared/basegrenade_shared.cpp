//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "decals.h"
#include "basegrenade_shared.h"
#include "shake.h"
#include "engine/IEngineSound.h"

#if !defined( CLIENT_DLL )

#include "soundent.h"
#include "entitylist.h"
#include "gamestats.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion
extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud
extern ConVar    sk_plr_dmg_grenade;

#if !defined( CLIENT_DLL )

// Global Savedata for friction modifier
BEGIN_DATADESC( CBaseGrenade )
	//					nextGrenade
	DEFINE_FIELD( m_hThrower, FIELD_EHANDLE ),
	//					m_fRegisteredSound ???
	DEFINE_FIELD( m_bIsLive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_DmgRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDetonateTime, FIELD_TIME ),
	DEFINE_FIELD( m_flWarnAITime, FIELD_TIME ),
	DEFINE_FIELD( m_flDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_iszBounceSound, FIELD_STRING ),
	DEFINE_FIELD( m_bHasWarnedAI,	FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_THINKFUNC( Smoke ),
	DEFINE_ENTITYFUNC( BounceTouch ),
	DEFINE_ENTITYFUNC( SlideTouch ),
	DEFINE_ENTITYFUNC( ExplodeTouch ),
	DEFINE_USEFUNC( DetonateUse ),
	DEFINE_THINKFUNC( DangerSoundThink ),
	DEFINE_THINKFUNC( PreDetonate ),
	DEFINE_THINKFUNC( Detonate ),
	DEFINE_THINKFUNC( TumbleThink ),

END_DATADESC()

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( BaseGrenade, DT_BaseGrenade )

BEGIN_NETWORK_TABLE( CBaseGrenade, DT_BaseGrenade )
#if !defined( CLIENT_DLL )
	SendPropFloat( SENDINFO( m_flDamage ), 10, SPROP_ROUNDDOWN, 0.0, 256.0f ),
	SendPropFloat( SENDINFO( m_DmgRadius ), 10, SPROP_ROUNDDOWN, 0.0, 1024.0f ),
	SendPropInt( SENDINFO( m_bIsLive ), 1, SPROP_UNSIGNED ),
//	SendPropTime( SENDINFO( m_flDetonateTime ) ),
	SendPropEHandle( SENDINFO( m_hThrower ) ),

	SendPropVector( SENDINFO( m_vecVelocity ), 0, SPROP_NOSCALE ), 
	// HACK: Use same flag bits as player for now
	SendPropInt			( SENDINFO(m_fFlags), 0, SPROP_UNSIGNED ),
#else
	RecvPropFloat( RECVINFO( m_flDamage ) ),
	RecvPropFloat( RECVINFO( m_DmgRadius ) ),
	RecvPropInt( RECVINFO( m_bIsLive ) ),
//	RecvPropTime( RECVINFO( m_flDetonateTime ) ),
	RecvPropEHandle( RECVINFO( m_hThrower ) ),

	// Need velocity from grenades to make animation system work correctly when running
	RecvPropVector( RECVINFO(m_vecVelocity), 0, RecvProxy_LocalVelocity ),

	RecvPropInt( RECVINFO( m_fFlags ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( grenade, CBaseGrenade );

#if defined( CLIENT_DLL )

BEGIN_PREDICTION_DATA( CBaseGrenade  )

	DEFINE_PRED_FIELD( m_hThrower, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsLive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_DmgRadius, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD_TOL( m_flDetonateTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( m_flDamage, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD_TOL( m_vecVelocity, FIELD_VECTOR, FTYPEDESC_INSENDTABLE, 0.5f ),
	DEFINE_PRED_FIELD_TOL( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),

//	DEFINE_FIELD( m_fRegisteredSound, FIELD_BOOLEAN ),
//	DEFINE_FIELD( m_iszBounceSound, FIELD_STRING ),

END_PREDICTION_DATA()

#endif

// Grenades flagged with this will be triggered when the owner calls detonateSatchelCharges
#define SF_DETONATE		0x0001

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CBaseGrenade::Explode( trace_t *pTrace, int bitsDamageType )
{
#if !defined( CLIENT_DLL )
	
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + (pTrace->plane.normal * 0.6) );
	}

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );

#if defined( TF_DLL )
	// Since this code only runs on the server, make sure it shows the tempents it creates.
	// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
	CDisablePredictionFiltering disabler;
#endif

	if ( pTrace->fraction != 1.0 )
	{
		Vector vecNormal = pTrace->plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace->surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );

		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage,
			&vecNormal,
			(char) pdata->game.material );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius * .03, 
			25,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			m_flDamage );
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
#endif

	// Use the thrower's position as the reported position
	Vector vecReported = m_hThrower ? m_hThrower->GetAbsOrigin() : vec3_origin;
	
	CTakeDamageInfo info( this, m_hThrower, GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );

	RadiusDamage( info, GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );

	UTIL_DecalTrace( pTrace, "Scorch" );

	EmitSound( "BaseGrenade.Explode" );

	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );
	SetSolid( SOLID_NONE );
	
	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );

#if HL2_EPISODIC
	// Because the grenade is zipped out of the world instantly, the EXPLOSION sound that it makes for
	// the AI is also immediately destroyed. For this reason, we now make the grenade entity inert and
	// throw it away in 1/10th of a second instead of right away. Removing the grenade instantly causes
	// intermittent bugs with env_microphones who are listening for explosions. They will 'randomly' not
	// hear explosion sounds when the grenade is removed and the SoundEnt thinks (and removes the sound)
	// before the env_microphone thinks and hears the sound.
	SetNextThink( gpGlobals->curtime + 0.1 );
#else
	SetNextThink( gpGlobals->curtime );
#endif//HL2_EPISODIC

#if defined( HL2_DLL )
	CBasePlayer *pPlayer = ToBasePlayer( m_hThrower.Get() );
	if ( pPlayer )
	{
		gamestats->Event_WeaponHit( pPlayer, true, "weapon_frag", info );
	}
#endif

#endif
}


void CBaseGrenade::Smoke( void )
{
	Vector vecAbsOrigin = GetAbsOrigin();
	if ( UTIL_PointContents ( vecAbsOrigin ) & MASK_WATER )
	{
		UTIL_Bubbles( vecAbsOrigin - Vector( 64, 64, 64 ), vecAbsOrigin + Vector( 64, 64, 64 ), 100 );
	}
	else
	{
		CPVSFilter filter( vecAbsOrigin );

		te->Smoke( filter, 0.0, 
			&vecAbsOrigin, g_sModelIndexSmoke,
			m_DmgRadius * 0.03,
			24 );
	}
#if !defined( CLIENT_DLL )
	SetThink ( &CBaseGrenade::SUB_Remove );
#endif
	SetNextThink( gpGlobals->curtime );
}

void CBaseGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	Detonate( );
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseGrenade::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Support player pickup
	if ( useType == USE_TOGGLE )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
			return;
		}
	}

	// Pass up so we still call any custom Use function
	BaseClass::Use( pActivator, pCaller, useType, value );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Timed grenade, this think is called when time runs out.
//-----------------------------------------------------------------------------
void CBaseGrenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime );
}

void CBaseGrenade::PreDetonate( void )
{
#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this );
#endif

	SetThink( &CBaseGrenade::Detonate );
	SetNextThink( gpGlobals->curtime + 1.5 );
}


void CBaseGrenade::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	if( tr.startsolid )
	{
		// Since we blindly moved the explosion origin vertically, we may have inadvertently moved the explosion into a solid,
		// in which case nothing is going to be harmed by the grenade's explosion because all subsequent traces will startsolid.
		// If this is the case, we do the downward trace again from the actual origin of the grenade. (sjb) 3/8/2007  (for ep2_outland_09)
		UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );
	}

	Explode( &tr, DMG_BLAST );

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}


//
// Contact grenade, explode when it touches something
// 
void CBaseGrenade::ExplodeTouch( CBaseEntity *pOther )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	Explode( &tr, DMG_BLAST );
}


void CBaseGrenade::DangerSoundThink( void )
{
	if (!IsInWorld())
	{
		Remove( );
		return;
	}

#if !defined( CLIENT_DLL )
	CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * 0.5, GetAbsVelocity().Length( ), 0.2, this );
#endif

	SetNextThink( gpGlobals->curtime + 0.2 );

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
	}
}


void CBaseGrenade::BounceTouch( CBaseEntity *pOther )
{
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS) )
		return;

	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() )
		return;

	// only do damage if we're moving fairly fast
	if ( (pOther->m_takedamage != DAMAGE_NO) && (m_flNextAttack < gpGlobals->curtime && GetAbsVelocity().Length() > 100))
	{
		if (m_hThrower)
		{
#if !defined( CLIENT_DLL )
			trace_t tr;
			tr = CBaseEntity::GetTouchTrace( );
			ClearMultiDamage( );
			Vector forward;
			AngleVectors( GetLocalAngles(), &forward, NULL, NULL );
			CTakeDamageInfo info( this, m_hThrower, 1, DMG_CLUB );
			CalculateMeleeDamageForce( &info, GetAbsVelocity(), GetAbsOrigin() );
			pOther->DispatchTraceAttack( info, forward, &tr ); 
			ApplyMultiDamage();
#endif
		}
		m_flNextAttack = gpGlobals->curtime + 1.0; // debounce
	}

	Vector vecTestVelocity;
	// m_vecAngVelocity = Vector (300, 300, 300);

	// this is my heuristic for modulating the grenade velocity because grenades dropped purely vertical
	// or thrown very far tend to slow down too quickly for me to always catch just by testing velocity. 
	// trimming the Z velocity a bit seems to help quite a bit.
	vecTestVelocity = GetAbsVelocity(); 
	vecTestVelocity.z *= 0.45;

	if ( !m_bHasWarnedAI && vecTestVelocity.Length() <= 60 )
	{
		// grenade is moving really slow. It's probably very close to where it will ultimately stop moving. 
		// emit the danger sound.
		
		// register a radius louder than the explosion, so we make sure everyone gets out of the way
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), m_flDamage / 0.4, 0.3, this );
#endif
		m_bHasWarnedAI = true;
	}

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.8 );

		// SetSequence( random->RandomInt( 1, 1 ) ); // FIXME: missing tumble animations
	}
	else
	{
		// play bounce sound
		BounceSound();
	}
	m_flPlaybackRate = GetAbsVelocity().Length() / 200.0;
	if (m_flPlaybackRate > 1.0)
		m_flPlaybackRate = 1;
	else if (m_flPlaybackRate < 0.5)
		m_flPlaybackRate = 0;

}



void CBaseGrenade::SlideTouch( CBaseEntity *pOther )
{
	// don't hit the guy that launched this grenade
	if ( pOther == GetThrower() )
		return;

	// m_vecAngVelocity = Vector (300, 300, 300);

	if (GetFlags() & FL_ONGROUND)
	{
		// add a bit of static friction
//		SetAbsVelocity( GetAbsVelocity() * 0.95 );  

		if (GetAbsVelocity().x != 0 || GetAbsVelocity().y != 0)
		{
			// maintain sliding sound
		}
	}
	else
	{
		BounceSound();
	}
}

void CBaseGrenade ::BounceSound( void )
{
	// Doesn't need to do anything anymore! Physics makes the sound.
}

void CBaseGrenade ::TumbleThink( void )
{
	if (!IsInWorld())
	{
		Remove( );
		return;
	}

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	//
	// Emit a danger sound one second before exploding.
	//
	if (m_flDetonateTime - 1 < gpGlobals->curtime)
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin() + GetAbsVelocity() * (m_flDetonateTime - gpGlobals->curtime), 400, 0.1, this );
#endif
	}

	if (m_flDetonateTime <= gpGlobals->curtime)
	{
		SetThink( &CBaseGrenade::Detonate );
	}

	if (GetWaterLevel() != 0)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.5 );
		m_flPlaybackRate = 0.2;
	}
}

void CBaseGrenade::Precache( void )
{
	BaseClass::Precache( );

	PrecacheScriptSound( "BaseGrenade.Explode" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatCharacter
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CBaseGrenade::GetThrower( void )
{
	CBaseCombatCharacter *pResult = ToBaseCombatCharacter( m_hThrower );
	if ( !pResult && GetOwnerEntity() != NULL )
	{
		pResult = ToBaseCombatCharacter( GetOwnerEntity() );
	}
	return pResult;
}

//-----------------------------------------------------------------------------

void CBaseGrenade::SetThrower( CBaseCombatCharacter *pThrower )
{
	m_hThrower = pThrower;

	// if this is the first thrower, set it as the original thrower
	if ( NULL == m_hOriginalThrower )
	{
		m_hOriginalThrower = pThrower;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseGrenade::~CBaseGrenade(void)
{
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseGrenade::CBaseGrenade(void)
{
	m_hThrower			= NULL;
	m_hOriginalThrower	= NULL;
	m_bIsLive			= false;
	m_DmgRadius			= 100;
	m_flDetonateTime	= 0;
	m_bHasWarnedAI		= false;

	SetSimulatedEveryTick( true );
};














