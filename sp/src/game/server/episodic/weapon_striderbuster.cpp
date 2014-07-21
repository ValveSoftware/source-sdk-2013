//========= Copyright Valve Corporation, All rights reserved. ============//
//
// An ingenious device. We call it "The Magnusson Device". Not my chosen label,
// you understand, but it seemed to please the personnel.
//
// From your point of view you simply throw it at a strider and then blow it up.
//
//=============================================================================

#include "cbase.h"
#include "props.h"
#include "vphysics/constraints.h"
#include "physics_saverestore.h"
#include "model_types.h"
#include "ai_utils.h"
#include "particle_system.h"
#include "Sprite.h"
#include "citadel_effects_shared.h"
#include "soundent.h"
#include "SpriteTrail.h"
#include "te_effect_dispatch.h"
#include "beam_shared.h"
#include "npc_strider.h"
#include "npc_hunter.h"
#include "particle_parse.h"
#include "gameweaponmanager.h"
#include "gamestats.h"

extern ConVar hunter_hate_held_striderbusters;
extern ConVar hunter_hate_thrown_striderbusters;
extern ConVar hunter_hate_attached_striderbusters;


ConVar striderbuster_health( "striderbuster_health", "14" );
ConVar striderbuster_autoaim_radius( "striderbuster_autoaim_radius", "64.0f" );
ConVar striderbuster_shot_velocity( "striderbuster_shot_velocity", "2500.0", FCVAR_NONE, "Speed at which launch the bomb from the physcannon" );
ConVar striderbuster_allow_all_damage( "striderbuster_allow_all_damage", "0", FCVAR_NONE, "If set to '1' the bomb will detonate on any damage taken.  Otherwise only the player may trigger it." );

//ConVar striderbuster_magnetic_radius("striderbuster_magnetic_radius","400.0f", FCVAR_NONE,"Maximum distance at which magnade experiences attraction to a target. Set to 0 to disable magnetism.");
ConVar striderbuster_magnetic_force_strider("striderbuster_magnetic_force_strider", "750000.0f", FCVAR_NONE,"Intensity of magnade's attraction to a strider.");
ConVar striderbuster_magnetic_force_hunter("striderbuster_magnetic_force_hunter","1750000.0f",FCVAR_NONE,"Intensity of magnade's attraction to a hunter.");
ConVar striderbuster_falloff_power("striderbuster_falloff_power","4",FCVAR_NONE,"Order of the distance falloff. 1 = linear 2 = quadratic");
ConVar striderbuster_leg_stick_dist( "striderbuster_leg_stick_dist", "80.0", FCVAR_NONE, "If the buster hits a strider's leg, the max distance from the head at which it sticks anyway." );
ConVar striderbuster_debugseek( "striderbuster_debugseek", "0" );

ConVar sk_striderbuster_magnet_multiplier( "sk_striderbuster_magnet_multiplier", "2.25" );

ConVar striderbuster_die_detach( "striderbuster_die_detach", "1" );	// Drop off the strider if a hunter shoots me. (Instead of exploding)
ConVar striderbuster_dive_force( "striderbuster_dive_force", "-200" ); // How much force to apply to a nosediving (dead in the air) striderbuster

ConVar striderbuster_use_particle_flare( "striderbuster_use_particle_flare", "1" );

#define STRIDERBUSTER_FLAG_KNOCKED_OFF_STRIDER		0x00000001 // We were knocked off of a strider after the player attached me.

#define SF_DONT_WEAPON_MANAGE			0x800000

#define STRIDERBUSTER_SPRITE_TRAIL	"sprites/bluelaser1.vmt"

string_t g_iszVehicle;

#define BUSTER_PING_SOUND_FREQ		3.0f	// How often (seconds) to issue the ping sound to remind players we are attached 

static const char *s_pBusterPingThinkContext = "BusterPing";

class CWeaponStriderBuster : public CPhysicsProp
{
	DECLARE_CLASS( CWeaponStriderBuster, CPhysicsProp );
	DECLARE_DATADESC();

public:
					CWeaponStriderBuster( void );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );

	// Treat as a live target so hunters can attack us
	virtual bool IsAlive() { return true; }

	virtual void	OnRestore( void );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void	UpdateOnRemove( void );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual bool	ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return ( reason == PHYSGUN_FORCE_LAUNCHED ); }
	virtual QAngle	PreferredCarryAngles( void ) { return m_CarryAngles; }
	virtual bool	HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer ) { return true; }

	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );
	virtual Vector	PhysGunLaunchVelocity( const Vector &forward, float flMass );
	virtual float	GetAutoAimRadius( void ) { return striderbuster_autoaim_radius.GetFloat(); }
	virtual void	BusterTouch( CBaseEntity *pOther );

	virtual bool	ShouldAttractAutoAim( CBaseEntity *pAimingEnt ) { return IsAttachedToStrider(); }

	void	InputConstraintBroken( inputdata_t &inputdata );
	void BusterFlyThink();
	void BusterDetachThink();
	void BusterPingThink();

	void OnAddToCargoHold();
	void OnFlechetteAttach( Vector &vecForceDir );
	int NumFlechettesAttached() { return m_nAttachedFlechettes; }

	float GetPickupTime() { return m_PickupTime; }

	int GetStriderBusterFlags() { return m_iBusterFlags; } // I added a flags field so we don't have to keep added bools for all of these contingencies (sjb)

private:

	void	Launch( CBasePlayer *pPhysGunUser );
	void	Detonate( void );
	void	Shatter( CBaseEntity *pAttacker );
	bool	StickToEntity( CBaseEntity *pOther );
	bool	CreateConstraintToObject( CBaseEntity *pObject );
	void	DestroyConstraint( void );
	bool	ShouldStickToEntity( CBaseEntity *pEntity );
	void	CreateDestroyedEffect( void );

	inline bool IsAttachedToStrider( void ) const;

	bool						m_bDud;
	bool						m_bLaunched;
	bool						m_bNoseDiving;					// No magnetism, nosedive and break. Hunter flechettes set this.
	int							m_nAttachedFlechettes;
	float						m_flCollisionSpeedSqr;
	int							m_nAttachedBoneFollowerIndex;
	float						m_PickupTime;

	IPhysicsConstraint			*m_pConstraint;
	EHANDLE						m_hConstrainedEntity;

	CHandle<CSprite>			m_hGlowSprite;
	CHandle<CSprite>			m_hMainGlow;

	//CHandle<CParticleSystem>	m_hGlowTrail;
	EHANDLE						m_hParticleEffect;

	int							m_nRingTexture;

	QAngle						m_CarryAngles;

	int							m_iBusterFlags;

	COutputEvent m_OnAttachToStrider;
	COutputEvent m_OnDetonate;
	COutputEvent m_OnShatter;
	COutputEvent m_OnShotDown;

friend bool StriderBuster_IsAttachedStriderBuster( CBaseEntity *pEntity, CBaseEntity * );

};

LINK_ENTITY_TO_CLASS( prop_stickybomb, CWeaponStriderBuster );
LINK_ENTITY_TO_CLASS( weapon_striderbuster, CWeaponStriderBuster );

BEGIN_DATADESC( CWeaponStriderBuster )
	DEFINE_KEYFIELD( m_bDud, FIELD_BOOLEAN, "dud" ),

	DEFINE_FIELD( m_bLaunched, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNoseDiving, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nAttachedFlechettes, FIELD_INTEGER ),
	DEFINE_FIELD( m_flCollisionSpeedSqr, FIELD_FLOAT ),
	DEFINE_FIELD( m_hConstrainedEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hGlowSprite, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hMainGlow, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_hGlowTrail, FIELD_EHANDLE ),

	DEFINE_FIELD( m_nRingTexture, FIELD_INTEGER ),
	DEFINE_FIELD( m_nAttachedBoneFollowerIndex, FIELD_INTEGER ),

	DEFINE_FIELD( m_PickupTime, FIELD_TIME ),

	DEFINE_FIELD( m_hParticleEffect, FIELD_EHANDLE ),

	DEFINE_FIELD( m_CarryAngles, FIELD_VECTOR ),

	DEFINE_FIELD( m_iBusterFlags, FIELD_INTEGER ),
	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_INPUTFUNC( FIELD_VOID, "ConstraintBroken", InputConstraintBroken ),
	
	DEFINE_OUTPUT( m_OnAttachToStrider, "OnAttachToStrider" ),
	DEFINE_OUTPUT( m_OnDetonate, "OnDetonate" ),
	DEFINE_OUTPUT( m_OnShatter, "OnShatter" ),
	DEFINE_OUTPUT( m_OnShotDown, "OnShotDown" ),
		
	DEFINE_ENTITYFUNC( BusterTouch ),
	DEFINE_THINKFUNC( BusterFlyThink ),
	DEFINE_THINKFUNC( BusterDetachThink ),
	DEFINE_THINKFUNC( BusterPingThink ),
END_DATADESC()

CWeaponStriderBuster::CWeaponStriderBuster( void ) : 
	m_pConstraint( NULL ),
	m_flCollisionSpeedSqr( -1.0f ),
	m_hConstrainedEntity( NULL ),
	m_nAttachedBoneFollowerIndex( -1 )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Precache( void )
{
	PrecacheScriptSound( "Weapon_StriderBuster.StickToEntity" );
	PrecacheScriptSound( "Weapon_StriderBuster.Detonate" );
	PrecacheScriptSound( "Weapon_StriderBuster.Dud_Detonate" );
	PrecacheScriptSound( "Weapon_StriderBuster.Ping" );

	PrecacheModel("sprites/orangeflare1.vmt");

	UTIL_PrecacheOther( "env_citadel_energy_core" );
	UTIL_PrecacheOther( "sparktrail" );

	m_nRingTexture = PrecacheModel( "sprites/lgtning.vmt" );

	PrecacheParticleSystem( "striderbuster_attach" );
	PrecacheParticleSystem( "striderbuster_attached_pulse" );
	PrecacheParticleSystem( "striderbuster_explode_core" );
	PrecacheParticleSystem( "striderbuster_explode_dummy_core" );
	PrecacheParticleSystem( "striderbuster_break_flechette" );
	PrecacheParticleSystem( "striderbuster_trail" );
	PrecacheParticleSystem( "striderbuster_shotdown_trail" );
	PrecacheParticleSystem( "striderbuster_break" );
	PrecacheParticleSystem( "striderbuster_flechette_attached" );

	SetModelName( AllocPooledString("models/magnusson_device.mdl") );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Spawn( void )
{	
	SetModelName( AllocPooledString("models/magnusson_device.mdl") );
	BaseClass::Spawn();
	
	// Setup for being shot by the player
	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Ignore touches until launched.
	SetTouch ( NULL );

	AddFlag( FL_AIMTARGET|FL_OBJECT );

	m_hParticleEffect = CreateEntityByName( "info_particle_system" );
	if ( m_hParticleEffect )
	{
		m_hParticleEffect->KeyValue( "start_active", "1" );
		m_hParticleEffect->KeyValue( "effect_name", "striderbuster_smoke" );
		DispatchSpawn( m_hParticleEffect );
		if ( gpGlobals->curtime > 0.2f )
		{
			m_hParticleEffect->Activate();
		}
		m_hParticleEffect->SetAbsOrigin( GetAbsOrigin() );
		m_hParticleEffect->SetParent( this );
	}

	SetHealth( striderbuster_health.GetFloat() );
	
	SetNextThink(gpGlobals->curtime + 0.01f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Activate( void )
{	
	g_iszVehicle = AllocPooledString( "prop_vehicle_jeep" );
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::OnRestore( void )
{
	BaseClass::OnRestore();

	// If we have an entity we're attached to, attempt to reconstruct our bone follower setup
	if ( m_hConstrainedEntity != NULL )
	{
		CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(m_hConstrainedEntity.Get());
		if ( pStrider != NULL )
		{
			// Make sure we've done this step or we'll have no controller to attach to
			pStrider->InitBoneFollowers();

			// Attempt to make a connection to the same bone follower we attached to previously
			CBoneFollower *pBoneFollower = pStrider->GetBoneFollowerByIndex( m_nAttachedBoneFollowerIndex );
			if ( CreateConstraintToObject( pBoneFollower ) == false )
			{
				Msg( "Failed to reattach to bone follower %d\n", m_nAttachedBoneFollowerIndex );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::DestroyConstraint( void )
{
	// Destroy the constraint
	if ( m_pConstraint != NULL )
	{ 
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create a constraint between this object and another
// Input  : *pObject - Object to constrain ourselves to
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStriderBuster::CreateConstraintToObject( CBaseEntity *pObject )
{
	if ( m_pConstraint != NULL )
	{
		// Should we destroy the constraint and make a new one at this point?
		Assert( 0 );
		return false;
	}

	if ( pObject == NULL )
		return false;

	IPhysicsObject *pPhysObject = pObject->VPhysicsGetObject();
	if ( pPhysObject == NULL )
		return false;

	IPhysicsObject *pMyPhysObject = VPhysicsGetObject();
	if ( pPhysObject == NULL )
		return false;

	// Create the fixed constraint
	constraint_fixedparams_t fixedConstraint;
	fixedConstraint.Defaults();
	fixedConstraint.InitWithCurrentObjectState( pPhysObject, pMyPhysObject );

	IPhysicsConstraint *pConstraint = physenv->CreateFixedConstraint( pPhysObject, pMyPhysObject, NULL, fixedConstraint );
	if ( pConstraint == NULL )
		return false;

	// Hold on to us
	m_pConstraint = pConstraint;
	pConstraint->SetGameData( (void *)this );
	m_hConstrainedEntity = pObject->GetOwnerEntity();;

	// Disable collisions between the two ents
	PhysDisableObjectCollisions( pPhysObject, pMyPhysObject );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Physics system has just told us our constraint has been broken
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::InputConstraintBroken( inputdata_t &inputdata )
{
	// Shatter with no real explosion effect
	Shatter( NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::UpdateOnRemove( void )
{
	DestroyConstraint();

	if ( m_hGlowSprite != NULL )
	{
		m_hGlowSprite->FadeAndDie( 0.5f );
		m_hGlowSprite = NULL;
	}

	if ( m_hParticleEffect )
	{
		UTIL_Remove( m_hParticleEffect );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStriderBuster::ShouldStickToEntity( CBaseEntity *pEntity )
{
	if ( pEntity == NULL )
		return false;

	// Must have a follow parent
	CBaseEntity *pFollowParent = pEntity->GetOwnerEntity();
	if ( pFollowParent == NULL )
		return false;

	// Must be a strider
	CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(pFollowParent);
	if ( pStrider == NULL )
		return false;

	if( m_bNoseDiving )
		return false;

	// Don't attach to legs
	CBoneFollower *pFollower = static_cast<CBoneFollower *>(pEntity);
	if ( pStrider->IsLegBoneFollower( pFollower ) )
	{
		Vector vecDelta = pStrider->GetAdjustedOrigin() - GetAbsOrigin();
		if ( vecDelta.Length() > striderbuster_leg_stick_dist.GetFloat() )
		{
			return false;
		}
	}

	// Ick, this is kind of ugly, but it's also ugly having to pass pointer into this to avoid multiple castings!
	// Save this to patch up save/restore later
	m_nAttachedBoneFollowerIndex = pStrider->GetBoneFollowerIndex( pFollower );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Stick to an entity (using hierarchy if we can)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponStriderBuster::StickToEntity( CBaseEntity *pOther )
{
	// Make sure the object is travelling fast enough to stick
	if ( m_flCollisionSpeedSqr > 50 && !m_bNoseDiving )
	{
		// See if this is a valid strider bit
		if ( ShouldStickToEntity( pOther ) )
		{
			// Attempt to constraint to it
			if ( CreateConstraintToObject( pOther ) )
			{
				// Only works for striders, at the moment
				CBaseEntity *pFollowParent = pOther->GetOwnerEntity();
				if ( pFollowParent == NULL )
					return false;

				// Allows us to identify our constrained object later
				SetOwnerEntity( pFollowParent );

				// Make a sound
				EmitSound( "Weapon_StriderBuster.StickToEntity" );
				
				DispatchParticleEffect( "striderbuster_attach", GetAbsOrigin(), GetAbsAngles(), NULL );

				if( striderbuster_use_particle_flare.GetBool() )
				{
					// We don't have to save any pointers or handles to this because it's parented to the buster.
					// So it will die when the buster dies. Yay.
					CParticleSystem *pFlare = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
					
					if ( pFlare != NULL )
					{
						pFlare->KeyValue( "start_active", "1" );
						pFlare->KeyValue( "effect_name", "striderbuster_attached_pulse" );
						pFlare->SetParent( this );
						pFlare->SetLocalOrigin( vec3_origin );
						DispatchSpawn( pFlare );
						pFlare->Activate();
					}
				}
				else
				{
					// Create a glow sprite
					m_hGlowSprite = CSprite::SpriteCreate( "sprites/orangeflare1.vmt", GetLocalOrigin(), false );

					Assert( m_hGlowSprite );
					if ( m_hGlowSprite != NULL )
					{
						m_hGlowSprite->TurnOn();
						m_hGlowSprite->SetTransparency( kRenderWorldGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
						m_hGlowSprite->SetAbsOrigin( GetAbsOrigin() );
						m_hGlowSprite->SetScale( 5.0f );
						m_hGlowSprite->m_nRenderFX = kRenderFxStrobeFaster;
						m_hGlowSprite->SetGlowProxySize( 16.0f );
						m_hGlowSprite->SetParent( this );
					}
				}

				// Stop touching things
				SetTouch( NULL );

				// Must be a strider
				CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(pFollowParent);
				if ( pStrider == NULL )
					return false;

				// Notify the strider we're attaching to him
				pStrider->StriderBusterAttached( this );
				
				m_OnAttachToStrider.FireOutput( this, this );

				// Start the ping sound.
				SetContextThink( &CWeaponStriderBuster::BusterPingThink, gpGlobals->curtime + BUSTER_PING_SOUND_FREQ, s_pBusterPingThinkContext );
				
				// Don't autodelete this one!
				WeaponManager_RemoveManaged( this );

				return true;
			}

			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Create the explosion effect for the final big boom
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::CreateDestroyedEffect( void )
{
	CBaseEntity *pTrail;

	StopParticleEffects( this );
	
	for ( int i = 0; i < 3; i++ )
	{
		pTrail = CreateEntityByName( "sparktrail" );
		pTrail->SetOwnerEntity( this );
		DispatchSpawn( pTrail );
	}
	
	DispatchParticleEffect( "striderbuster_explode_core", GetAbsOrigin(), GetAbsAngles() );

	// Create liquid fountain gushtacular effect here!
	CEffectData	data;

	int nNumSteps = 6;
	float flRadStep = (2*M_PI) / nNumSteps;
	for ( int i = 0; i < nNumSteps; i++ )
	{
		data.m_vOrigin = GetAbsOrigin() + RandomVector( -32.0f, 32.0f );
		data.m_vNormal.x = cos( flRadStep*i );
		data.m_vNormal.y = sin( flRadStep*i );
		data.m_vNormal.z = 0.0f;
		data.m_flScale = ( random->RandomInt( 0, 5 ) == 0 ) ? 1 : 2;

		DispatchEffect( "StriderBlood", data );
	}

	// More effects
	UTIL_ScreenShake( GetAbsOrigin(), 20.0f, 150.0, 1.0, 1250.0f, SHAKE_START );

	data.m_vOrigin = GetAbsOrigin();
	DispatchEffect( "cball_explode", data );
}

//-----------------------------------------------------------------------------
// Purpose: Handle a collision using our special behavior
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Find out what we hit.
	// Don't do anything special if we're already attached to a strider.
	CBaseEntity *pVictim = pEvent->pEntities[!index];
	if ( pVictim == NULL || m_pConstraint != NULL )
	{
		BaseClass::VPhysicsCollision( index, pEvent );
		return;
	}

	// Don't attach if we're being held by the player
	if ( VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		BaseClass::VPhysicsCollision( index, pEvent );
		return;
	}

	// Save off the speed of the object
	m_flCollisionSpeedSqr = ( pEvent->preVelocity[ index ] ).LengthSqr();

	// Break if we hit the world while going fast enough.
	// Launched duds detonate if they hit the world at any speed.
	if ( pVictim->IsWorld() && ( ( m_bDud && m_bLaunched ) || m_flCollisionSpeedSqr > Square( 500 ) ) )
	{
		m_OnShatter.FireOutput( this, this );
		Shatter( pVictim );
		return;
	}

	// We'll handle this later in our touch call
	if ( ShouldStickToEntity( pVictim ) )
		return;

	// Determine if we should shatter
	CBaseEntity *pOwnerEntity = pVictim->GetOwnerEntity();
	bool bVictimIsStrider = ( ( pOwnerEntity != NULL ) && FClassnameIs( pOwnerEntity, "npc_strider" ) );

	// Break if we hit anything other than a strider while going fast enough.
	// Launched duds detonate if they hit anything other than a strider any speed.
	if ( ( bVictimIsStrider == false ) && ( ( m_bDud && m_bLaunched ) || m_flCollisionSpeedSqr > Square( 500 ) ) )
	{
		m_OnShatter.FireOutput( this, this );
		Shatter( pVictim );
		return;
	}

	// Just bounce
	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Called to see if we should attach to the victim
// Input  : *pOther - the victim
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::BusterTouch( CBaseEntity *pOther )
{
	// Attempt to stick to the entity
	StickToEntity( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
inline bool CWeaponStriderBuster::IsAttachedToStrider( void ) const
{
	CBaseEntity *pAttachedEnt = GetOwnerEntity();
	if ( pAttachedEnt && FClassnameIs( pAttachedEnt, "npc_strider" ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Detonate( void )
{
	CBaseEntity *pVictim = GetOwnerEntity();
	if ( !m_bDud && pVictim )
	{
		// Kill the strider (with magic effect)
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		CTakeDamageInfo info( pPlayer, this, RandomVector( -100.0f, 100.0f ), GetAbsOrigin(), pVictim->GetHealth(), DMG_GENERIC );
		pVictim->TakeDamage( info );

		gamestats->Event_WeaponHit( ToBasePlayer( pPlayer ), true, GetClassname(), info );

		// Tracker 62293:  There's a bug where the inflictor/attacker are reversed when calling TakeDamage above so the player never gets
		//  credit for the strider buster kills.  The code has a bunch of assumptions lower level, so it's safer to just fix it here by 
		//  crediting a kill to the player directly.
		gamestats->Event_PlayerKilledOther( pPlayer, pVictim, info );
	}

	m_OnDetonate.FireOutput( this, this );

	// Explode
	if ( !m_bDud )
	{
		CreateDestroyedEffect();
		EmitSound( "Weapon_StriderBuster.Detonate" );
	}
	else
	{
		DispatchParticleEffect( "striderbuster_explode_dummy_core", GetAbsOrigin(), GetAbsAngles() );
		EmitSound( "Weapon_StriderBuster.Dud_Detonate" );
	}

	// Go to bits!
	Shatter( pVictim );
}

//-----------------------------------------------------------------------------
// Purpose: Intercept damage and decide whether or not we want to trigger
// Input  : &info - 
//-----------------------------------------------------------------------------
int CWeaponStriderBuster::OnTakeDamage( const CTakeDamageInfo &info )
{
	// If we're attached, any damage from the player makes us trigger
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pAttacker = info.GetAttacker();
	bool bInflictorIsPlayer = ( pInflictor != NULL && pInflictor->IsPlayer() );
	bool bAttackerIsPlayer = ( pAttacker != NULL && pAttacker->IsPlayer() );

	if ( GetParent() && GetParent()->ClassMatches( g_iszVehicle ) )
	{
		return 0;
	}

	// Only take damage from a player, for the moment
	if ( striderbuster_allow_all_damage.GetBool() || ( IsAttachedToStrider() && ( bAttackerIsPlayer || bInflictorIsPlayer ) ) )
	{
		Detonate();
		return 0;
	}

	if ( pAttacker && ( pAttacker->Classify() == CLASS_COMBINE || pAttacker->Classify() == CLASS_COMBINE_HUNTER ) )
	{
		if ( VPhysicsGetObject() && !VPhysicsGetObject()->IsMoveable() )
		{
			return 0;
		}
	}

	// Hunters are able to destroy strider busters
	if ( hunter_hate_held_striderbusters.GetBool() || hunter_hate_thrown_striderbusters.GetBool() || hunter_hate_attached_striderbusters.GetBool() )
	{
		if ( ( GetHealth() > 0 ) && ( pInflictor != NULL ) && FClassnameIs( pInflictor, "hunter_flechette" ) )
		{
			//
			// Flechette impacts don't hurt the striderbuster unless it's attached to a strider,
			// but the explosions always do. This is so that held or thrown striderbusters fly
			// awry because of the flechette, but attached striderbusters break instantly to make
			// the hunters more effective at defending the strider.
			//
			if ( IsAttachedToStrider() || !( info.GetDamageType() & DMG_NEVERGIB ) )
			{
				if( striderbuster_die_detach.GetBool() && IsAttachedToStrider() )
				{
					// Make the buster fall off and break.
					m_takedamage = DAMAGE_NO;

					CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(GetOwnerEntity());
					Assert( pStrider != NULL );
					pStrider->StriderBusterDetached( this );
					DestroyConstraint();

					// Amplify some lateral force.
					Vector vecForce = info.GetDamageForce();
					vecForce.z = 0.0f;
					VPhysicsGetObject()->ApplyForceCenter( vecForce * 5.0f );

					SetContextThink( NULL, gpGlobals->curtime, s_pBusterPingThinkContext );

					SetThink( &CWeaponStriderBuster::BusterDetachThink );
					SetNextThink( gpGlobals->curtime );
					m_iBusterFlags |= STRIDERBUSTER_FLAG_KNOCKED_OFF_STRIDER;

					return 0;
				}
				else
				{
					// Destroy the buster in place
					// Make sure they know it blew up prematurely.
					EmitSound( "Weapon_StriderBuster.Dud_Detonate" );
					DispatchParticleEffect( "striderbuster_break_flechette", GetAbsOrigin(), GetAbsAngles() );
					SetHealth( 0 );

					Shatter( info.GetAttacker() );
					return 0;
				}
			}

			if ( info.GetDamage() < 5 )
			{
				bool bFirst = ( m_CarryAngles.x == 45 && m_CarryAngles.y == 0 && m_CarryAngles.z == 0);
				float sinTime = sin( gpGlobals->curtime );
				bool bSubtractX = ( bFirst ) ? ( sinTime < 0 ) : ( m_CarryAngles.x < 45 );

				m_CarryAngles.x += ( 10.0 + 10.0 * fabsf( sinTime ) + random->RandomFloat( -2.5, 2.5 ) + random->RandomFloat( -2.5, 2.5 ) ) * ( ( bSubtractX ) ? -1.0 : 1.0 );
				m_CarryAngles.y = 15 * ( sin( gpGlobals->curtime ) + cos( gpGlobals->curtime * 0.5 ) ) * .5  + random->RandomFloat( -15, 15 );
				m_CarryAngles.z = 7.5 * ( sin( gpGlobals->curtime ) + sin( gpGlobals->curtime * 2.0 ) ) * .5 + random->RandomFloat( -7.5, 7.5 );
			}

			return 1;
		}
	}
	
	// Allow crushing damage
	if ( info.GetDamageType() & DMG_CRUSH )
		return BaseClass::OnTakeDamage( info );

	return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	m_PickupTime = gpGlobals->curtime;
	m_CarryAngles.Init( 45, 0, 0 );
	if ( ( reason == PICKED_UP_BY_CANNON ) && ( !HasSpawnFlags( SF_DONT_WEAPON_MANAGE ) ) )
	{
		WeaponManager_RemoveManaged( this );
	}
	else if ( reason == PUNTED_BY_CANNON )
	{
		Launch( pPhysGunUser );
	}
	
	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	if ( Reason == LAUNCHED_BY_CANNON )
	{
		Launch( pPhysGunUser );
	}
	else if ( ( Reason == DROPPED_BY_CANNON ) && ( !HasSpawnFlags( SF_DONT_WEAPON_MANAGE ) ) )
	{
		// This striderbuster is now fair game for autodeletion.
		WeaponManager_AddManaged( this );
	}

	BaseClass::OnPhysGunDrop( pPhysGunUser, Reason );
}


//-----------------------------------------------------------------------------
// Fling the buster with the physcannon either via punt or launch.
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Launch( CBasePlayer *pPhysGunUser )
{
	if ( !HasSpawnFlags( SF_DONT_WEAPON_MANAGE ) )
	{
		WeaponManager_RemoveManaged( this );
	}

	m_bLaunched = true;

	// Notify all nearby hunters that we were launched.
	Hunter_StriderBusterLaunched( this );

	// Start up the eye glow
	m_hMainGlow = CSprite::SpriteCreate( "sprites/blueglow1.vmt", GetLocalOrigin(), false );

	if ( m_hMainGlow != NULL )
	{
		m_hMainGlow->FollowEntity( this );
		m_hMainGlow->SetTransparency( kRenderGlow, 255, 255, 255, 140, kRenderFxNoDissipation );
		m_hMainGlow->SetScale( 2.0f );
		m_hMainGlow->SetGlowProxySize( 8.0f );
	}

	if ( !m_bNoseDiving )
	{
		DispatchParticleEffect( "striderbuster_trail", PATTACH_ABSORIGIN_FOLLOW, this );
	}
	else
	{
		DispatchParticleEffect( "striderbuster_shotdown_trail", PATTACH_ABSORIGIN_FOLLOW, this );
	}

	// We get our touch function from the physics system
	SetTouch ( &CWeaponStriderBuster::BusterTouch );

	SetThink( &CWeaponStriderBuster::BusterFlyThink );
	SetNextThink( gpGlobals->curtime );

	gamestats->Event_WeaponFired( pPhysGunUser, true, GetClassname() );
}
		

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &forward - 
//			flMass - 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CWeaponStriderBuster::PhysGunLaunchVelocity( const Vector &forward, float flMass )
{
	return ( striderbuster_shot_velocity.GetFloat() * forward );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::Shatter( CBaseEntity *pAttacker )
{
	if( m_bNoseDiving )
		m_OnShotDown.FireOutput( this, this );

	m_takedamage = DAMAGE_YES;

	if( !IsAttachedToStrider() )
	{
		// Don't display this particular effect if we're attached to a strider. This effect just gets lost 
		// in the big strider explosion anyway, so let's recover some perf.
		DispatchParticleEffect( "striderbuster_break", GetAbsOrigin(), GetAbsAngles() );
	}

	// Buster is useless now. Stop thinking, touching.
	SetThink( NULL );
	SetTouch( NULL );
	SetContextThink( NULL, gpGlobals->curtime, s_pBusterPingThinkContext );

	// Deal deadly damage to ourselves (DMG_CRUSH is allowed, others are blocked)
	CTakeDamageInfo info( pAttacker, pAttacker, RandomVector( -100, 100 ), GetAbsOrigin(), 100.0f, DMG_CRUSH );
	TakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: Give the buster a slight attraction to striders.
//			Ported back from the magnade.
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::BusterFlyThink()
{
	if (IsAttachedToStrider())
		return; // early out. Think no more.

	// If we're nosediving, forget about magnetism.
	if ( m_bNoseDiving )
	{
		if ( VPhysicsGetObject() )
			VPhysicsGetObject()->ApplyForceCenter( Vector( 0, 0, striderbuster_dive_force.GetFloat() ) );
		SetNextThink(gpGlobals->curtime + 0.01f);
		return;
	}

	// seek?	
	const float magradius = 38.0 * sk_striderbuster_magnet_multiplier.GetFloat(); // radius of strider hull times multiplier
	if (magradius > 0 &&
		GetMoveType() == MOVETYPE_VPHYSICS &&
		VPhysicsGetObject()
		)
	{
		// find the nearest enemy.
		CBaseEntity *pList[16];
		Vector origin = GetAbsOrigin();

		// do a find in box ( a little faster than sphere )
		int count;
		{
			Vector mins,maxs;
			mins = origin; 
			mins -= magradius;
			
			maxs = origin; 
			maxs += magradius;

			count = UTIL_EntitiesInBox(pList, 16, mins, maxs, FL_NPC); 
		}

		float magradiusSq = Square( magradius );	
		float nearestDistSq = magradiusSq + 1;
		int bestFit = -1;
		Vector toTarget; // will be garbage unless something good is found
		CNPC_Strider *pBestStrider  = NULL;

		for ( int ii = 0 ; ii < count ; ++ii )
		{
			CNPC_Strider *pStrider = dynamic_cast<CNPC_Strider *>(pList[ii]);
			if ( pStrider && !pStrider->CarriedByDropship() ) // ShouldStickToEntity() doesn't work because the strider NPC isn't what we glue to
			{
				// get distance squared
				VectorSubtract( pStrider->GetAdjustedOrigin(), GetAbsOrigin(), toTarget );

				//NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + toTarget, 128, 0, 128, false, 0.1 );

				float dSq = toTarget.LengthSqr();
				if (dSq < nearestDistSq)
				{
					bestFit = ii; nearestDistSq = dSq;
					pBestStrider = pStrider;
				}
			}
		}

		if (bestFit >= 0) // we found something and should attract towards it. (hysterisis later?)
		{
			if ( striderbuster_debugseek.GetBool() )
			{
				NDebugOverlay::Circle( GetAbsOrigin() + toTarget, magradius, 255, 255, 255, 255, true, .1 );
				NDebugOverlay::Cross3D( GetAbsOrigin() + toTarget, magradius, 255, 255, 255, true, .1 );
			}

			// force magnitude. 
			float magnitude = GetMass() * striderbuster_magnetic_force_strider.GetFloat();
			int falloff = striderbuster_falloff_power.GetInt();
			switch (falloff) 
			{
			case 1:
				VPhysicsGetObject()->ApplyForceCenter( toTarget * (magnitude / nearestDistSq) ); // dividing through by distance squared normalizes toTarget and gives a linear falloff
				break;
			case 2:
				VPhysicsGetObject()->ApplyForceCenter( toTarget * (magnitude / (nearestDistSq * sqrtf(nearestDistSq))) ); // dividing through by distance cubed normalizes toTarget and gives a quadratic falloff
				break;
			case 3:
				VPhysicsGetObject()->ApplyForceCenter( toTarget * (magnitude / (nearestDistSq * nearestDistSq)) ); // dividing through by distance fourth normalizes toTarget and gives a cubic falloff
				break;
			case 4:
				{
					Vector toTarget;
					pBestStrider->GetAttachment( "buster_target", toTarget );

					if ( striderbuster_debugseek.GetBool() )
					{
						NDebugOverlay::Cross3D( toTarget, magradius, 255, 0, 255, true, .1 );
						NDebugOverlay::Cross3D( toTarget, magradius, 255, 0, 255, true, .1 );
					}

					toTarget -= GetAbsOrigin();
					toTarget.NormalizeInPlace();
					VPhysicsGetObject()->ApplyForceCenter( toTarget * magnitude );

				}
				break;
			default: // arbitrary powers
				VPhysicsGetObject()->ApplyForceCenter( toTarget * (magnitude * powf(nearestDistSq,(falloff+1.0f)/2)) );  // square root for distance instead of squared, add one to normalize toTarget 
				break;
			}
		}

		SetNextThink(gpGlobals->curtime + 0.01f);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::BusterDetachThink()
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector( 0, 0, 1200), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if( fabs(tr.startpos.z - tr.endpos.z) < 240.0f )
	{
		SetThink(NULL);
		EmitSound( "Weapon_StriderBuster.Dud_Detonate" );
		DispatchParticleEffect( "striderbuster_break_flechette", GetAbsOrigin(), GetAbsAngles() );
		SetHealth( 0 );
		CTakeDamageInfo info;
		info.SetDamage( 1.0f );
		info.SetAttacker( this );
		info.SetInflictor( this );
		Shatter(this);
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::BusterPingThink()
{
	EmitSound( "Weapon_StriderBuster.Ping" );

	SetContextThink( &CWeaponStriderBuster::BusterPingThink, gpGlobals->curtime + BUSTER_PING_SOUND_FREQ, s_pBusterPingThinkContext );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::OnAddToCargoHold()
{
	if ( !HasSpawnFlags( SF_DONT_WEAPON_MANAGE ) )
	{
		WeaponManager_RemoveManaged( this );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CWeaponStriderBuster::OnFlechetteAttach( Vector &vecFlechetteVelocity )
{
	if ( m_bLaunched )
	{
		Vector vecForce = vecFlechetteVelocity;
		VectorNormalize( vecForce );

		vecForce *= 1000;
		vecForce.z = -5000;

		VPhysicsGetObject()->ApplyForceCenter( vecForce );
	}

	if ( !GetParent() || !GetParent()->ClassMatches( g_iszVehicle ) )
	{
		if ( !m_bNoseDiving )
		{
			//m_hGlowTrail->StopParticleSystem();
			StopParticleEffects( this );

			if( m_iBusterFlags & STRIDERBUSTER_FLAG_KNOCKED_OFF_STRIDER )
			{
				DispatchParticleEffect( "striderbuster_shotdown_trail", PATTACH_ABSORIGIN_FOLLOW, this );
			}
			else
			{
				DispatchParticleEffect( "striderbuster_flechette_attached", PATTACH_ABSORIGIN_FOLLOW, this );
			}
		}

		m_bNoseDiving = true;
	}
	m_nAttachedFlechettes++;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool StriderBuster_IsAttachedStriderBuster( CBaseEntity *pEntity, CBaseEntity *pAttachedTo )
{
	Assert(dynamic_cast<CWeaponStriderBuster *>(pEntity));
	if ( !pAttachedTo )
		return static_cast<CWeaponStriderBuster *>(pEntity)->m_hConstrainedEntity != NULL;
	else
		return static_cast<CWeaponStriderBuster *>(pEntity)->m_hConstrainedEntity == pAttachedTo;
}


//-----------------------------------------------------------------------------
// Called when the striderbuster is placed in the jalopy's cargo container.
//-----------------------------------------------------------------------------
void StriderBuster_OnAddToCargoHold( CBaseEntity *pEntity )
{
	CWeaponStriderBuster *pBuster = dynamic_cast <CWeaponStriderBuster *>( pEntity );
	if ( pBuster )
	{
		pBuster->OnAddToCargoHold();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool StriderBuster_OnFlechetteAttach( CBaseEntity *pEntity, Vector &vecFlechetteVelocity )
{
	CWeaponStriderBuster *pBuster = dynamic_cast <CWeaponStriderBuster *>( pEntity );
	if ( pBuster )
	{
		pBuster->OnFlechetteAttach( vecFlechetteVelocity );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int StriderBuster_NumFlechettesAttached( CBaseEntity *pEntity )
{
	CWeaponStriderBuster *pBuster = dynamic_cast <CWeaponStriderBuster *>( pEntity );
	if ( pBuster )
	{
		return pBuster->NumFlechettesAttached();
	}
	return 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
float StriderBuster_GetPickupTime( CBaseEntity *pEntity )
{
	CWeaponStriderBuster *pBuster = dynamic_cast <CWeaponStriderBuster *>( pEntity );
	if ( pBuster )
	{
		return pBuster->GetPickupTime();
	}
	return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool StriderBuster_WasKnockedOffStrider( CBaseEntity *pEntity )
{
	CWeaponStriderBuster *pBuster = dynamic_cast <CWeaponStriderBuster *>( pEntity );
	if ( pBuster )
	{
		return ((pBuster->GetStriderBusterFlags() & STRIDERBUSTER_FLAG_KNOCKED_OFF_STRIDER) != 0);
	}

	return false;
}
