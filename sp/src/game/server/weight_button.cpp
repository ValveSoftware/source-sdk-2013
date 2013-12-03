
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 'Button' which activates after a specified amount of weight is touching it.
//
//=============================================================================//

#include "cbase.h"

class CWeightButton : public CBaseEntity
{
public:

	DECLARE_DATADESC();
	DECLARE_CLASS( CWeightButton, CBaseEntity ); 

	void Spawn( void );
	bool CreateVPhysics( void );

	COutputEvent m_OnPressed;				// After threshold weight has been reached
	COutputEvent m_OnReleased;				// After weight has been removed to go below weight threshold

	float m_fStressToActivate;				// Amount of weight required to activate
	bool m_bHasBeenPressed;					// Once the button has been pressed, fire one 
											// output until the weight is reduced below the threshold

	void TriggerThink ( void );

};

LINK_ENTITY_TO_CLASS( func_weight_button, CWeightButton );

BEGIN_DATADESC( CWeightButton )

	DEFINE_KEYFIELD( m_fStressToActivate, FIELD_FLOAT, "WeightToActivate" ),
	DEFINE_FIELD( m_bHasBeenPressed, FIELD_BOOLEAN ),

	DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),
	DEFINE_OUTPUT( m_OnReleased, "OnReleased" ),
	
	DEFINE_THINKFUNC( TriggerThink ),

END_DATADESC()


void CWeightButton::Spawn()
{
	BaseClass::Spawn();

	// Convert movedir from angles to a vector
	SetMoveType( MOVETYPE_VPHYSICS );
	SetSolid( SOLID_VPHYSICS );
 	SetModel( STRING( GetModelName() ) );
	CreateVPhysics();
	SetThink( &CWeightButton::TriggerThink );
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
	m_bHasBeenPressed = false;
}

//-----------------------------------------------------------------------------
// Purpose: Create VPhysics collision for this entity
//-----------------------------------------------------------------------------
bool CWeightButton::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Every second, check total stress and fire an output if we have reached 
//			our threshold. If the stress is relieved below our threshold, fire a different output.
//-----------------------------------------------------------------------------
void CWeightButton::TriggerThink( void )
{
	vphysics_objectstress_t vpobj_StressOut;
	IPhysicsObject* pMyPhysics = VPhysicsGetObject();

	if ( !pMyPhysics )
	{
		SetNextThink( TICK_NEVER_THINK );
		return;
	}

 	float fStress = CalculateObjectStress( pMyPhysics, this, &vpobj_StressOut );

//	fStress = vpobj_StressOut.receivedStress;

	if ( fStress > m_fStressToActivate && !m_bHasBeenPressed )
	{
		m_OnPressed.FireOutput( this, this );
		m_bHasBeenPressed = true;
	}
	else if ( fStress < m_fStressToActivate && m_bHasBeenPressed )
	{
		m_OnReleased.FireOutput( this, this );
		m_bHasBeenPressed = false;
	}

	// think every tick
	SetNextThink( gpGlobals->curtime + TICK_INTERVAL );
}
