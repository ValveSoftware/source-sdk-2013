//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the tripmine grenade.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "beam_shared.h"
#include "shake.h"
#include "hl2mp/grenade_tripmine.h" // Load the hl2mp header!!
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern const char* g_pModelNameLaser;

ConVar    sk_plr_dmg_tripmine		( "sk_plr_dmg_tripmine","0");
ConVar    sk_npc_dmg_tripmine		( "sk_npc_dmg_tripmine","0");
ConVar    sk_tripmine_radius		( "sk_tripmine_radius","0");

LINK_ENTITY_TO_CLASS( npc_tripmine, CTripmineGrenade );

BEGIN_DATADESC( CTripmineGrenade )

	DEFINE_FIELD( m_hOwner,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_flPowerUp,	FIELD_TIME ),
	DEFINE_FIELD( m_vecDir,		FIELD_VECTOR ),
	DEFINE_FIELD( m_vecEnd,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( m_pBeam,		FIELD_CLASSPTR ),
	DEFINE_FIELD( m_posOwner,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_angleOwner,	FIELD_VECTOR ),

	// Function Pointers
	DEFINE_THINKFUNC( WarningThink ),
	DEFINE_THINKFUNC( PowerupThink ),
	DEFINE_THINKFUNC( BeamBreakThink ),
	DEFINE_THINKFUNC( DelayDeathThink ),

END_DATADESC()

CTripmineGrenade::CTripmineGrenade()
{
	m_vecDir.Init();
	m_vecEnd.Init();
	m_posOwner.Init();
	m_angleOwner.Init();

	m_pConstraint = NULL;
	m_bAttached = false;
	m_hAttachEntity = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTripmineGrenade::~CTripmineGrenade( void )
{
	if (m_pConstraint)
	{
		physenv->DestroyConstraint(m_pConstraint);
		m_pConstraint = NULL;
	}
}
void CTripmineGrenade::Spawn( void )
{
	Precache( );
	// motor
	SetMoveType( MOVETYPE_FLY );
	SetSolid( SOLID_BBOX );
	SetModel( "models/Weapons/w_slam.mdl" );

	IPhysicsObject *pObject = VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, true );
	pObject->EnableMotion( false );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	SetCycle( 0.0f );
	m_nBody			= 3;
	m_flDamage		= sk_plr_dmg_tripmine.GetFloat();
	m_DmgRadius		= sk_tripmine_radius.GetFloat();

	ResetSequenceInfo( );
	m_flPlaybackRate	= 0;
	
	UTIL_SetSize(this, Vector( -4, -4, -2), Vector(4, 4, 2));

	m_flPowerUp = gpGlobals->curtime + 2.0;
	
	SetThink( &CTripmineGrenade::PowerupThink );
	SetNextThink( gpGlobals->curtime + 0.2 );

	m_takedamage		= DAMAGE_YES;

	m_iHealth = 1;

	EmitSound( "TripmineGrenade.Place" );
	SetDamage ( 200 );

	// Tripmine sits at 90 on wall so rotate back to get m_vecDir
	QAngle angles = GetAbsAngles();
	angles.x -= 90;

	AngleVectors( angles, &m_vecDir );
	m_vecEnd = GetAbsOrigin() + m_vecDir * 2048;

	AddEffects( EF_NOSHADOW );
}


void CTripmineGrenade::Precache( void )
{
	PrecacheModel("models/Weapons/w_slam.mdl"); 

	PrecacheScriptSound( "TripmineGrenade.Place" );
	PrecacheScriptSound( "TripmineGrenade.Activate" );
}


void CTripmineGrenade::WarningThink( void  )
{
	// set to power up
	SetThink( &CTripmineGrenade::PowerupThink );
	SetNextThink( gpGlobals->curtime + 1.0f );
}


void CTripmineGrenade::PowerupThink( void  )
{
	if (gpGlobals->curtime > m_flPowerUp)
	{
		MakeBeam( );
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		m_bIsLive			= true;

		// The moment it's live, then do this - incase the attach entity moves between placing it, and activation
		// use the absorigin of what we're attaching to for the check, if it moves - we blow up.
		if ( m_bAttached && m_hAttachEntity.Get() != NULL )
			m_vAttachedPosition = m_hAttachEntity.Get()->GetAbsOrigin();

		// play enabled sound
		EmitSound( "TripmineGrenade.Activate" );
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}


void CTripmineGrenade::KillBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}


void CTripmineGrenade::MakeBeam( void )
{
	trace_t tr;

	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	m_flBeamLength = tr.fraction;



	// If I hit a living thing, send the beam through me so it turns on briefly
	// and then blows the living thing up
	CBaseEntity *pEntity = tr.m_pEnt;
	CBaseCombatCharacter *pBCC  = ToBaseCombatCharacter( pEntity );

	// Draw length is not the beam length if entity is in the way
	float drawLength = tr.fraction;
	if (pBCC)
	{
		SetOwnerEntity( pBCC );
		UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		m_flBeamLength = tr.fraction;
		SetOwnerEntity( NULL );
		
	}

	// set to follow laser spot
	SetThink( &CTripmineGrenade::BeamBreakThink );

	// Delay first think slightly so beam has time
	// to appear if person right in front of it
	SetNextThink( gpGlobals->curtime + 1.0f );

	Vector vecTmpEnd = GetLocalOrigin() + m_vecDir * 2048 * drawLength;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 0.35 );
	m_pBeam->PointEntInit( vecTmpEnd, this );
	m_pBeam->SetColor( 255, 55, 52 );
	m_pBeam->SetScrollRate( 25.6 );
	m_pBeam->SetBrightness( 64 );
	
	int beamAttach = LookupAttachment("beam_attach");
	m_pBeam->SetEndAttachment( beamAttach );
}


void CTripmineGrenade::BeamBreakThink( void  )
{
	// See if I can go solid yet (has dropper moved out of way?)
	if (IsSolidFlagSet( FSOLID_NOT_SOLID ))
	{
		trace_t tr;
		Vector	vUpBit = GetAbsOrigin();
		vUpBit.z += 5.0;

		UTIL_TraceEntity( this, GetAbsOrigin(), vUpBit, MASK_SHOT, &tr );
		if ( !tr.startsolid && (tr.fraction == 1.0) )
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
		}
	}

	trace_t tr;

	// NOT MASK_SHOT because we want only simple hit boxes
	UTIL_TraceLine( GetAbsOrigin(), m_vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect. 
	if ( !m_pBeam )
	{
		MakeBeam( );
		if ( tr.m_pEnt )
			m_hOwner = tr.m_pEnt;	// reset owner too
	}


	CBaseEntity *pEntity = tr.m_pEnt;
	CBaseCombatCharacter *pBCC  = ToBaseCombatCharacter( pEntity );

	bool bAttachMoved = false;
	if ( m_bAttached && m_hAttachEntity.Get() != NULL )
	{
		if ( m_hAttachEntity.Get()->GetAbsOrigin() != m_vAttachedPosition )
			bAttachMoved = true;
	}

	// Also blow up if the attached entity goes away, ie: a crate
	if (pBCC || fabs( m_flBeamLength - tr.fraction ) > 0.001 || ( m_bAttached && m_hAttachEntity.Get() == NULL) || bAttachMoved )
	{
		m_iHealth = 0;
		if (m_pConstraint)
			m_pConstraint->Deactivate();

		Event_Killed( CTakeDamageInfo( (CBaseEntity*)m_hOwner, this, 100, GIB_NORMAL ) );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.05f );
}

#if 0 // FIXME: OnTakeDamage_Alive() is no longer called now that base grenade derives from CBaseAnimating
int CTripmineGrenade::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if (gpGlobals->curtime < m_flPowerUp && info.GetDamage() < m_iHealth)
	{
		// disable
		// Create( "weapon_tripmine", GetLocalOrigin() + m_vecDir * 24, GetAngles() );
		SetThink( &CTripmineGrenade::SUB_Remove );
		SetNextThink( gpGlobals->curtime + 0.1f );
		KillBeam();
		return FALSE;
	}
	return BaseClass::OnTakeDamage_Alive( info );
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CTripmineGrenade::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage		= DAMAGE_NO;

	if (m_pConstraint)
		m_pConstraint->Deactivate();

	SetThink( &CTripmineGrenade::DelayDeathThink );
	SetNextThink( gpGlobals->curtime + 0.25 );

	EmitSound( "TripmineGrenade.StopSound" );
}


void CTripmineGrenade::DelayDeathThink( void )
{
	KillBeam();
	trace_t tr;
	UTIL_TraceLine ( GetAbsOrigin() + m_vecDir * 8, GetAbsOrigin() - m_vecDir * 64,  MASK_SOLID, this, COLLISION_GROUP_NONE, & tr);
	UTIL_ScreenShake( GetAbsOrigin(), 25.0, 150.0, 1.0, 750, SHAKE_START );

	ExplosionCreate( GetAbsOrigin() + m_vecDir * 8, GetAbsAngles(), m_hOwner, GetDamage(), 200, 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this);

	UTIL_Remove( this );
}

bool CTripmineGrenade::MakeConstraint( CBaseEntity *pObject )
{
	IPhysicsObject *cMinePhysics = VPhysicsGetObject();

	Assert( cMinePhysics );

	IPhysicsObject *pAttached = pObject->VPhysicsGetObject();
	if ( !cMinePhysics || !pAttached )
		return false;

	// constraining to the world means object 1 is fixed
	if ( pAttached == g_PhysWorldObject )
		PhysSetGameFlags( cMinePhysics, FVPHYSICS_CONSTRAINT_STATIC );

	IPhysicsConstraintGroup *pGroup = NULL;

	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( cMinePhysics, pAttached );
	fixed.constraint.Defaults();

	m_pConstraint = physenv->CreateFixedConstraint( cMinePhysics, pAttached, pGroup, fixed );
	
	if (!m_pConstraint)
		return false;

	m_pConstraint->SetGameData( (void *)this );

	return true;
}

void CTripmineGrenade::AttachToEntity(CBaseEntity *pOther)
{
	if (!pOther)
		return;

	if ( !VPhysicsGetObject() )
		return;

	m_bAttached			= true;
	m_hAttachEntity		= pOther;

	SetMoveType			( MOVETYPE_NONE );

	if (pOther->GetSolid() == SOLID_VPHYSICS && pOther->VPhysicsGetObject() != NULL )
	{
		SetSolid(SOLID_BBOX); //Tony; switch to bbox solid instead of vphysics, because we've made the physics object non-solid
		MakeConstraint(pOther);
		SetMoveType		( MOVETYPE_VPHYSICS ); // use vphysics while constrained!!
	}
	//if it isnt vphysics or bsp, use SetParent to follow it.
	else if (pOther->GetSolid() != SOLID_BSP)
	{
		SetSolid(SOLID_BBOX); //Tony; switch to bbox solid instead of vphysics, because we've made the physics object non-solid
		SetParent( m_hAttachEntity.Get() );
	}
}
