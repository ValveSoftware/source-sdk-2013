//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements a moving target that moves along a path of path_tracks
//			and can be shot and killed. When the target it killed it fires an
//			OnDeath output.
//
//			m_flSpeed is the travel speed
//			m_iHealth is current health
//			m_iMaxHealth is the amount to reset to each time it starts
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entityoutput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FGUNTARGET_START_ON			0x0001


class CGunTarget : public CBaseToggle
{
	DECLARE_CLASS( CGunTarget, CBaseToggle );

public:

	virtual void Spawn( void );
	virtual void Activate( void );
	bool CreateVPhysics( void );

	virtual int BloodColor( void ) { return DONT_BLEED; }

#if defined( HL2_DLL )
	virtual Class_T Classify( void ) { return CLASS_MILITARY; }
#elif defined( HL1_DLL )
	virtual Class_T Classify( void ) { return CLASS_MACHINE; }
#else
	virtual Class_T Classify( void ) { return CLASS_NONE; }
#endif
	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual Vector BodyTarget( const Vector &posSrc, bool bNoisy = true ) { return GetAbsOrigin(); }

	// Input handlers
	void InputStart( inputdata_t &inputdata );
	void InputStop( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	DECLARE_DATADESC();

protected:

	void Next( void );
	void Start( void );
	void Wait( void );
	void Stop( void );

private:

	bool			m_on;
	EHANDLE			m_hTargetEnt;

	// Outputs
	COutputEvent	m_OnDeath;
};


LINK_ENTITY_TO_CLASS( func_guntarget, CGunTarget );

BEGIN_DATADESC( CGunTarget )

	DEFINE_FIELD( m_on, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hTargetEnt, FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_FUNCTION( Next ),
	DEFINE_FUNCTION( Start ),
	DEFINE_FUNCTION( Wait ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStart ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	// Outputs
	DEFINE_OUTPUT(m_OnDeath, "OnDeath"),

END_DATADESC()



void CGunTarget::Spawn( void )
{
	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	SetModel( STRING( GetModelName() ) );

	if ( m_flSpeed == 0 )
		m_flSpeed = 100;

	// Don't take damage until "on"
	m_takedamage = DAMAGE_NO;
	AddFlag( FL_NPC );

	m_on = false;
	m_iMaxHealth = m_iHealth;

	if ( HasSpawnFlags(FGUNTARGET_START_ON) )
	{
		SetMoveDone( &CGunTarget::Start );
		SetMoveDoneTime( 0.3 );
	}
	CreateVPhysics();
}


bool CGunTarget::CreateVPhysics( void )
{
	VPhysicsInitShadow( false, false );
	return true;
}

void CGunTarget::Activate( void )
{
	BaseClass::Activate();

	CBaseEntity	*pTarg;
	// now find our next target
	pTarg = GetNextTarget();
	if ( pTarg )
	{
		m_hTargetEnt = pTarg;
		Vector nextPos = pTarg->GetAbsOrigin();
		Teleport( &nextPos, NULL, NULL );
	}
}


void CGunTarget::Start( void )
{
	m_takedamage = DAMAGE_YES;
	AddFlag( FL_AIMTARGET );
	m_hTargetEnt = GetNextTarget();
	if ( m_hTargetEnt == NULL )
		return;
	m_iHealth = m_iMaxHealth;
	Next();
}


void CGunTarget::Next( void )
{
	SetThink( NULL );

	m_hTargetEnt = GetNextTarget();
	CBaseEntity *pTarget = m_hTargetEnt;
	
	if ( !pTarget )
	{
		Stop();
		return;
	}

	SetMoveDone( &CGunTarget::Wait );
	LinearMove( pTarget->GetLocalOrigin(), m_flSpeed );
}


void CGunTarget::Wait( void )
{
	CBaseEntity *pTarget = m_hTargetEnt;
	
	if ( !pTarget )
	{
		Stop();
		return;
	}

	variant_t emptyVariant;
	pTarget->AcceptInput( "InPass", this, this, emptyVariant, 0 );
		
	m_flWait = pTarget->GetDelay();

	m_target = pTarget->m_target;
	SetMoveDone( &CGunTarget::Next );
	if (m_flWait != 0)
	{// -1 wait will wait forever!		
		SetMoveDoneTime( m_flWait );
	}
	else
	{
		Next();// do it RIGHT now!
	}
}


void CGunTarget::Stop( void )
{
	SetAbsVelocity( vec3_origin );
	SetMoveDoneTime( -1 );
	m_takedamage = DAMAGE_NO;
}


int	CGunTarget::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_iHealth > 0 )
	{
		m_iHealth -= info.GetDamage();
		if ( m_iHealth <= 0 )
		{
			m_iHealth = 0;
			Stop();

			m_OnDeath.FireOutput( info.GetInflictor(), this );
		}
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that starts the target moving.
//-----------------------------------------------------------------------------
void CGunTarget::InputStart( inputdata_t &inputdata )
{
	Start();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that stops the target from moving.
//-----------------------------------------------------------------------------
void CGunTarget::InputStop( inputdata_t &inputdata )
{
	Stop();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that toggles the start/stop state of the target.
//-----------------------------------------------------------------------------
void CGunTarget::InputToggle( inputdata_t &inputdata )
{
	if ( m_on )
	{
		Stop();
	}
	else
	{
		Start();
	}
}
