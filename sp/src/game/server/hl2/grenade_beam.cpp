//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Projectile shot by mortar synth.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "grenade_beam.h"
#include "beam_shared.h"
#include "ndebugoverlay.h"
#include "decals.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADEBEAM_DEFAULTWIDTH 2.0

// ==============================================================================
//  > CGrenadeBeamChaser
// ==============================================================================
BEGIN_DATADESC( CGrenadeBeamChaser )

	DEFINE_FIELD( m_pTarget, FIELD_CLASSPTR ),

	// Function pointers
	DEFINE_FUNCTION( ChaserThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_beam_chaser, CGrenadeBeamChaser );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeamChaser::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLY );
	SetThink(&CGrenadeBeamChaser::ChaserThink);
	SetNextThink( gpGlobals->curtime );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeamChaser::ChaserThink( void )
{
	Vector vTargetPos;
	m_pTarget->GetChaserTargetPos(&vTargetPos);
	Vector vTargetDir = (vTargetPos - GetLocalOrigin());

	// -------------------------------------------------
	// Check to see if we'll pass our target this frame
	// If so get the next target
	// -------------------------------------------------
	float flTargetDist = vTargetDir.Length();
	if ((gpGlobals->frametime * m_pTarget->m_flBeamSpeed) > flTargetDist)
	{
		m_pTarget->GetNextTargetPos(&vTargetPos);
		vTargetDir = (vTargetPos - GetLocalOrigin());
		flTargetDist = vTargetDir.Length();
	}

	if (flTargetDist != 0)
	{
		//--------------------------------------
		// Set our velocity to chase the target
		//--------------------------------------
		VectorNormalize(vTargetDir);
		SetAbsVelocity( vTargetDir * m_pTarget->m_flBeamSpeed );
	}
	SetNextThink( gpGlobals->curtime );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CGrenadeBeamChaser* CGrenadeBeamChaser::ChaserCreate( CGrenadeBeam *pTarget )
{
	CGrenadeBeamChaser *pChaser = (CGrenadeBeamChaser *)CreateEntityByName( "grenade_beam_chaser" );
	pChaser->SetLocalOrigin( pTarget->GetLocalOrigin() );
	pChaser->m_pTarget		= pTarget;
	pChaser->Spawn();
	return pChaser;
}

// ==============================================================================
//  > CGrenadeBeam
// ==============================================================================
BEGIN_DATADESC( CGrenadeBeam )

	DEFINE_FIELD( m_vLaunchPos,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flBeamWidth,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flBeamSpeed,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flBeamLag,		FIELD_FLOAT ),
	DEFINE_FIELD( m_flLaunchTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flLastTouchTime,	FIELD_TIME ),
	DEFINE_FIELD( m_hBeamChaser,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_nNumHits,			FIELD_INTEGER ),

	DEFINE_ARRAY( m_pHitLocation,		FIELD_VECTOR,	GRENADEBEAM_MAXHITS ),
	DEFINE_ARRAY( m_pBeam,			FIELD_CLASSPTR, GRENADEBEAM_MAXBEAMS ),

	// Function pointers
	DEFINE_ENTITYFUNC( GrenadeBeamTouch ),
	DEFINE_THINKFUNC( KillBeam ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( grenade_beam, CGrenadeBeam );

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::Spawn( void )
{
	Precache( );
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	
	//UNDONE/HACK: this model is never used but one is needed
	SetModel( "Models/weapons/flare.mdl" );
	AddEffects( EF_NODRAW );

	SetTouch( &CGrenadeBeam::GrenadeBeamTouch );
	SetNextThink( gpGlobals->curtime );

	m_takedamage	= DAMAGE_NO;
	m_iHealth		= 1;
	SetGravity( 0.0001 );
	m_nNumHits		= 0;
	UTIL_SetSize( this, vec3_origin, vec3_origin );
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CGrenadeBeam* CGrenadeBeam::Create( CBaseEntity* pOwner, const Vector &vStart)
{
	CGrenadeBeam *pEnergy = (CGrenadeBeam *)CreateEntityByName( "grenade_beam" );
	pEnergy->Spawn();
	pEnergy->SetOwnerEntity( pOwner );
	pEnergy->SetRenderColor( 255, 0, 0, 0 );
	pEnergy->m_flBeamWidth		= GRENADEBEAM_DEFAULTWIDTH;
	UTIL_SetOrigin( pEnergy, vStart );

	return pEnergy;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::Format(color32 clrColor, float flWidth)
{
	m_clrRender		= clrColor;
	m_flBeamWidth	= flWidth;
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::Shoot(Vector vDirection, float flSpeed, float flLifetime, float flLag, float flDamage )
{
	SetThink ( &CGrenadeBeam::KillBeam );
	SetNextThink( gpGlobals->curtime + flLifetime );
	m_hBeamChaser		= CGrenadeBeamChaser::ChaserCreate(this);
	m_flBeamSpeed		= flSpeed;
	SetAbsVelocity( vDirection * flSpeed );
	m_flBeamLag			= flLag;
	m_flDamage			= flDamage;
	m_flLaunchTime		= gpGlobals->curtime;
	m_vLaunchPos		= GetAbsOrigin();
	m_flLastTouchTime  = 0;
	CreateBeams();
	UpdateBeams();
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::KillBeam(void)
{
	SetThink(NULL);
	SetTouch(NULL);
	m_hBeamChaser->SetThink(NULL);
	UTIL_Remove(m_hBeamChaser);
	UTIL_Remove(this);

	for (int i=0;i<GRENADEBEAM_MAXBEAMS;i++)
	{
		if (m_pBeam[i])
		{
			UTIL_Remove(m_pBeam[i]);
		}
	}
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::GrenadeBeamTouch( CBaseEntity *pOther )
{
	//---------------------------------------------------------
	// Make sure I'm not caught in a corner, if so remove me
	//---------------------------------------------------------
	if (gpGlobals->curtime - m_flLastTouchTime < 0.01)
	{
		KillBeam();
		return;
	}
	m_flLastTouchTime = gpGlobals->curtime;

	// ---------------------------------------
	// If I have room for another hit, add it
	// ---------------------------------------
	if (m_nNumHits < GRENADEBEAM_MAXHITS)
	{
		m_pHitLocation[m_nNumHits] = GetLocalOrigin();
		m_nNumHits++;
	}
	// Otherwise copy over old hit, and force chaser into last hit position
	else
	{
		m_hBeamChaser->SetLocalOrigin( m_pHitLocation[0] );
		for (int i=0;i<m_nNumHits-1;i++)
		{
			m_pHitLocation[i] = m_pHitLocation[i+1];
		}
		m_pHitLocation[m_nNumHits-1]=GetLocalOrigin();
	}
	UpdateBeams();

	// --------------------------------------
	//  Smoke or bubbles effect
	// --------------------------------------
	if (UTIL_PointContents ( GetAbsOrigin() ) & MASK_WATER)
	{
		UTIL_Bubbles(GetAbsOrigin()-Vector(3,3,3),GetAbsOrigin()+Vector(3,3,3),10);
	}
	else 
	{
		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(5, 10), 10);
	}

	// --------------------------------------------
	//  Play burn sounds
	// --------------------------------------------
	if (pOther->m_takedamage)
	{
		pOther->TakeDamage( CTakeDamageInfo( this, this, m_flDamage, DMG_BURN ) );
		KillBeam();
		return;
	}
	
	EmitSound( "GrenadeBeam.HitSound" );

	trace_t tr;
	Vector vDirection = GetAbsVelocity();
	VectorNormalize(vDirection);
	UTIL_TraceLine( GetAbsOrigin()-vDirection, GetAbsOrigin()+vDirection, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	UTIL_DecalTrace( &tr, "RedGlowFade" );
	UTIL_ImpactTrace( &tr, DMG_ENERGYBEAM );
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::GetNextTargetPos(Vector *vPosition)
{
	// Only advance if tail launch time has passed
	if (gpGlobals->curtime - m_flLaunchTime > m_flBeamLag)
	{
		if (m_nNumHits > 0)
		{
			for (int i=0;i<m_nNumHits-1;i++)
			{
				m_pHitLocation[i] = m_pHitLocation[i+1];
			}
			m_nNumHits--;

			UpdateBeams();
		}
	}
	GetChaserTargetPos(vPosition);
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::GetChaserTargetPos(Vector *vPosition)
{
	// -----------------------------
	//  Launch chaser after a delay
	// -----------------------------
	if (gpGlobals->curtime - m_flLaunchTime < m_flBeamLag) 
	{
		*vPosition = m_vLaunchPos;
	}
	else if (m_nNumHits > 0)
	{
		*vPosition = m_pHitLocation[0];
	}
	else
	{
		*vPosition = GetLocalOrigin();
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::CreateBeams(void)
{
	for ( int i=0; i < GRENADEBEAM_MAXBEAMS; ++i )
	{
		m_pBeam[i] = CBeam::BeamCreate( "sprites/laser.vmt", m_flBeamWidth );
		m_pBeam[i]->SetColor( m_clrRender->r, m_clrRender->g, m_clrRender->b );
		m_pBeam[i]->EntsInit( this, m_hBeamChaser );
		m_pBeam[i]->SetBrightness( 255 );
		m_pBeam[i]->SetNoise( 1 );
		m_pBeam[i]->SetBeamFlag( FBEAM_SHADEIN );
		m_pBeam[i]->SetBeamFlag( FBEAM_SHADEOUT );
	}
}
/*
void CGrenadeBeam::DebugBeams(void)
{
		if (m_nNumHits > 0)
	{
		NDebugOverlay::Line(GetLocalOrigin(), m_pHitLocation[m_nNumHits-1], 255,255,25, true, 0.1);
		NDebugOverlay::Line(m_hBeamChaser->GetLocalOrigin(), m_pHitLocation[0], 255,255,25, true, 0.1);

		for (int i=0;i<m_nNumHits-1;i++)
		{
			NDebugOverlay::Line(m_pHitLocation[i], m_pHitLocation[i+1], 255,255,25, true, 0.1);
		}
	}
	else
	{
		NDebugOverlay::Line(GetLocalOrigin(), m_hBeamChaser->GetLocalOrigin(), 255,255,25, true, 0.1);
	}
	
	for (int i=0;i<m_nNumHits;i++)
	{
		NDebugOverlay::Cross3D(m_pHitLocation[i],	Vector(-8,-8,-8),Vector(8,8,8),0,255,0,true,0.1);
	}
}
*/

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::UpdateBeams(void)
{
	// ------------------------------------------------------------------
	// If no hits, draw a single beam between the grenade and the chaser
	// ------------------------------------------------------------------
	if (m_nNumHits == 0)
	{
		m_pBeam[0]->EntsInit( this, m_hBeamChaser );
		for (int i=1;i<GRENADEBEAM_MAXBEAMS;i++)
		{
			m_pBeam[i]->SetBrightness(0);
		}
	}
	// ------------------------------------------------------------------
	//  Otherwise draw beams between hits
	// ------------------------------------------------------------------
	else
	{
		m_pBeam[0]->PointEntInit( m_pHitLocation[0], m_hBeamChaser );

		for (int i=1;i<GRENADEBEAM_MAXBEAMS-1;i++)
		{
			if (i<m_nNumHits)
			{
				m_pBeam[i]->PointsInit(m_pHitLocation[i-1],m_pHitLocation[i]);
				m_pBeam[i]->SetBrightness(255);
			}
			else
			{
				m_pBeam[i]->SetBrightness(0);
			}
		}

		m_pBeam[GRENADEBEAM_MAXBEAMS-1]->PointEntInit( m_pHitLocation[m_nNumHits-1], this );
		m_pBeam[GRENADEBEAM_MAXBEAMS-1]->SetBrightness(255);
	}
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGrenadeBeam::Precache( void )
{
	PrecacheModel("sprites/laser.vmt");

	//UNDONE/HACK: this model is never used but one is needed
	PrecacheModel("Models/weapons/flare.mdl");

	PrecacheScriptSound( "GrenadeBeam.HitSound" );
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CGrenadeBeam::UpdateTransmitState(void)
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

