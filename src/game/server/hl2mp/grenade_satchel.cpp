//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "explode.h"
#include "Sprite.h"
#include "grenade_satchel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	SLAM_SPRITE	"sprites/redglow1.vmt"

ConVar    sk_plr_dmg_satchel		( "sk_plr_dmg_satchel","0");
ConVar    sk_npc_dmg_satchel		( "sk_npc_dmg_satchel","0");
ConVar    sk_satchel_radius			( "sk_satchel_radius","0");

BEGIN_DATADESC( CSatchelCharge )

	DEFINE_FIELD( m_flNextBounceSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vLastPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_pMyWeaponSLAM, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_bIsAttached, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_THINKFUNC( SatchelThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_satchel, CSatchelCharge );

//=========================================================
// Deactivate - do whatever it is we do to an orphaned 
// satchel when we don't want it in the world anymore.
//=========================================================
void CSatchelCharge::Deactivate( void )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
	UTIL_Remove( this );

	if ( m_hGlowSprite != NULL )
	{
		UTIL_Remove( m_hGlowSprite );
		m_hGlowSprite = NULL;
	}
}


void CSatchelCharge::Spawn( void )
{
	Precache( );
	SetModel( "models/Weapons/w_slam.mdl" );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_VPHYSICS );

	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetThink( &CSatchelCharge::SatchelThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_satchel.GetFloat();
	m_DmgRadius		= sk_satchel_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( UTIL_ScaleForGravity( 560 ) );	// slightly lower gravity
	SetFriction( 1.0 );
	SetSequence( 1 );
	SetDamage( 150 );

	m_bIsAttached			= false;
	m_bInAir				= true;
	m_flNextBounceSoundTime	= 0;

	m_vLastPosition	= vec3_origin;

	m_hGlowSprite = NULL;
	CreateEffects();
}

//-----------------------------------------------------------------------------
// Purpose: Start up any effects for us
//-----------------------------------------------------------------------------
void CSatchelCharge::CreateEffects( void )
{
	// Only do this once
	if ( m_hGlowSprite != NULL )
		return;

	// Create a blinking light to show we're an active SLAM
	m_hGlowSprite = CSprite::SpriteCreate( SLAM_SPRITE, GetAbsOrigin(), false );
	m_hGlowSprite->SetAttachment( this, 0 );
	m_hGlowSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxStrobeFast );
	m_hGlowSprite->SetBrightness( 255, 1.0f );
	m_hGlowSprite->SetScale( 0.2f, 0.5f );
	m_hGlowSprite->TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::InputExplode( inputdata_t &inputdata )
{
	ExplosionCreate( GetAbsOrigin() + Vector( 0, 0, 16 ), GetAbsAngles(), GetThrower(), GetDamage(), 200, 
		SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this);

	UTIL_Remove( this );
}


void CSatchelCharge::SatchelThink( void )
{
	// If attached resize so player can pick up off wall
	if (m_bIsAttached)
	{
		UTIL_SetSize(this, Vector( -2, -2, -6), Vector(2, 2, 6));
	}

	// See if I can lose my owner (has dropper moved out of way?)
	// Want do this so owner can shoot the satchel charge
	if (GetOwnerEntity())
	{
		trace_t tr;
		Vector	vUpABit = GetAbsOrigin();
		vUpABit.z += 5.0;

		CBaseEntity* saveOwner	= GetOwnerEntity();
		SetOwnerEntity( NULL );
		UTIL_TraceEntity( this, GetAbsOrigin(), vUpABit, MASK_SOLID, &tr );
		if ( tr.startsolid || tr.fraction != 1.0 )
		{
			SetOwnerEntity( saveOwner );
		}
	}
	
	// Bounce movement code gets this think stuck occasionally so check if I've 
	// succeeded in moving, otherwise kill my motions.
	else if ((GetAbsOrigin() - m_vLastPosition).LengthSqr()<1)
	{
		SetAbsVelocity( vec3_origin );

		QAngle angVel = GetLocalAngularVelocity();
		angVel.y  = 0;
		SetLocalAngularVelocity( angVel );

		// Clear think function
		SetThink(NULL);
		return;
	}
	m_vLastPosition= GetAbsOrigin();

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	// Is it attached to a wall?
	if (m_bIsAttached)
	{
		return;
	}
}

void CSatchelCharge::Precache( void )
{
	PrecacheModel("models/Weapons/w_slam.mdl");
	PrecacheModel(SLAM_SPRITE);
}

void CSatchelCharge::BounceSound( void )
{
	if (gpGlobals->curtime > m_flNextBounceSoundTime)
	{
		m_flNextBounceSoundTime = gpGlobals->curtime + 0.1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
// Input  :
// Output :
//-----------------------------------------------------------------------------
CSatchelCharge::CSatchelCharge(void)
{
	m_vLastPosition.Init();
	m_pMyWeaponSLAM = NULL;
}

CSatchelCharge::~CSatchelCharge(void)
{
	if ( m_hGlowSprite != NULL )
	{
		UTIL_Remove( m_hGlowSprite );
		m_hGlowSprite = NULL;
	}
}
