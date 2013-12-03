//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "grenade_satchel.h"
#include "player.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar    sk_plr_dmg_satchel		( "sk_plr_dmg_satchel","0");
ConVar    sk_npc_dmg_satchel		( "sk_npc_dmg_satchel","0");
ConVar    sk_satchel_radius			( "sk_satchel_radius","0");

BEGIN_DATADESC( CSatchelCharge )

	DEFINE_SOUNDPATCH( m_soundSlide ),

	DEFINE_FIELD( m_flSlideVolume, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextBounceSoundTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vLastPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_pMyWeaponSLAM, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_bIsAttached, FIELD_BOOLEAN ),

	// Function Pointers
	DEFINE_FUNCTION( SatchelTouch ),
	DEFINE_FUNCTION( SatchelThink ),
	DEFINE_FUNCTION( SatchelUse ),

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
}


void CSatchelCharge::Spawn( void )
{
	Precache( );
	// motor
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX ); 
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetModel( "models/Weapons/w_slam.mdl" );

	UTIL_SetSize(this, Vector( -6, -6, -2), Vector(6, 6, 2));

	SetTouch( SatchelTouch );
	SetUse( SatchelUse );
	SetThink( SatchelThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	m_flDamage		= sk_plr_dmg_satchel.GetFloat();
	m_DmgRadius		= sk_satchel_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetGravity( UTIL_ScaleForGravity( 560 ) );	// slightly lower gravity
	SetFriction( 1.0 );
	SetSequence( 1 );

	m_bIsAttached			= false;
	m_bInAir				= true;
	m_flSlideVolume			= -1.0;
	m_flNextBounceSoundTime	= 0;

	m_vLastPosition	= vec3_origin;

	InitSlideSound();
}

//-----------------------------------------------------------------------------

void CSatchelCharge::InitSlideSound(void)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );
	m_soundSlide = controller.SoundCreate( filter, entindex(), CHAN_STATIC, "SatchelCharge.Slide", ATTN_NORM );	
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::KillSlideSound(void)
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.CommandClear( m_soundSlide );
	controller.SoundFadeOut( m_soundSlide, 0.0 );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::SatchelUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	KillSlideSound();
	SetThink( Detonate );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CSatchelCharge::SatchelTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	// If successfully thrown and touching the 
	// NPC that released this grenade, pick it up
	if ( pOther == GetThrower() && GetOwnerEntity() == NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( m_pMyWeaponSLAM->GetOwner() );
		if (pPlayer)
		{
			// Give the player ammo
			pPlayer->GiveAmmo(1, m_pMyWeaponSLAM->m_iSecondaryAmmoType);

			CPASAttenuationFilter filter( pPlayer, "SatchelCharge.Pickup" );
			EmitSound( filter, pPlayer->entindex(), "SatchelCharge.Pickup" );

			m_bIsLive = false;

			// Take weapon out of detonate mode if necessary
			if (!m_pMyWeaponSLAM->AnyUndetonatedCharges())
			{
				m_pMyWeaponSLAM->m_bDetonatorArmed			= false;
				m_pMyWeaponSLAM->m_bNeedDetonatorHolster	= true;

				// Put detonator away right away
				m_pMyWeaponSLAM->SetWeaponIdleTime( gpGlobals->curtime );
			}

			// Kill any sliding sound
			KillSlideSound();

			// Remove satchel charge from world
			UTIL_Remove( this );
			return;
		}

	}

	StudioFrameAdvance( );

	// Is it attached to a wall?
	if (m_bIsAttached)
	{
		return;
	}

	SetGravity( 1 );// normal gravity now

	// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
	{
		// add a bit of static friction
		SetAbsVelocity( GetAbsVelocity() * 0.85 );
		SetLocalAngularVelocity( GetLocalAngularVelocity() * 0.8 );
	}

	UpdateSlideSound();

	if (m_bInAir)
	{
		BounceSound();
		m_bInAir = false;
	}

}

void CSatchelCharge::UpdateSlideSound( void )
{	
	if (!m_soundSlide)
	{
		return;
	}

	float volume = GetAbsVelocity().Length2D()/1000;
	if (volume < 0.01 && m_soundSlide)
	{
		KillSlideSound();
		return;
	}
		// HACKHACK - On ground isn't always set, so look for ground underneath
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,10), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	
	if ( tr.fraction < 1.0 )
	{
		if (m_flSlideVolume == -1.0)
		{
			controller.CommandClear( m_soundSlide );
			controller.Play( m_soundSlide, 1.0, 100 );
			m_flSlideVolume = 1.0;
		}
		else 
		{
			float volume = GetAbsVelocity().Length()/1000;
			if ( volume < m_flSlideVolume )
			{
				m_flSlideVolume = volume;
				controller.CommandClear( m_soundSlide );
				controller.SoundChangeVolume( m_soundSlide, volume, 0.1 );
			}
		}
	}
	else 
	{
		controller.CommandClear( m_soundSlide );
		controller.SoundChangeVolume( m_soundSlide, 0.0, 0.01 );
		m_flSlideVolume = -1.0;
		m_bInAir = true;
		return;
	}
}

void CSatchelCharge::SatchelThink( void )
{
	// If attached resize so player can pick up off wall
	if (m_bIsAttached)
	{
		UTIL_SetSize(this, Vector( -2, -2, -6), Vector(2, 2, 6));
	}

	UpdateSlideSound();

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

		// Kill any remaining sound
		KillSlideSound();

		// Clear think function
		SetThink(NULL);
		return;
	}
	m_vLastPosition= GetAbsOrigin();

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (!IsInWorld())
	{
		// Kill any remaining sound
		KillSlideSound();

		UTIL_Remove( this );
		return;
	}

	// Is it attached to a wall?
	if (m_bIsAttached)
	{
		return;
	}

	Vector vecNewVel = GetAbsVelocity();
	if (GetWaterLevel() == 3)
	{
		SetMoveType( MOVETYPE_FLY );
		vecNewVel *= 0.8;
		vecNewVel.z += 8;
		SetLocalAngularVelocity( GetLocalAngularVelocity() * 0.9 );
	}
	else if (GetWaterLevel() == 0)
	{
		SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	}
	else
	{
		vecNewVel.z -= 8;
	}
	SetAbsVelocity( vecNewVel );
}

void CSatchelCharge::Precache( void )
{
	PrecacheModel("models/Weapons/w_slam.mdl");

	PrecacheScriptSound( "SatchelCharge.Pickup" );
	PrecacheScriptSound( "SatchelCharge.Bounce" );

	PrecacheScriptSound( "SatchelCharge.Slide" );
}

void CSatchelCharge::BounceSound( void )
{
	if (gpGlobals->curtime > m_flNextBounceSoundTime)
	{
		EmitSound( "SatchelCharge.Bounce" );

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
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	controller.SoundDestroy( m_soundSlide );
}
