//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements doors that move when you look at them.
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "basecombatcharacter.h"
#include "entitylist.h"
#include "func_movelinear.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_LDOOR_THRESHOLD			8192
#define SF_LDOOR_INVERT				16384
#define SF_LDOOR_FROM_OPEN			32768


class CLookDoor : public CFuncMoveLinear
{
public:
	DECLARE_CLASS( CLookDoor, CFuncMoveLinear );

	void	Spawn( void );
	void	MoveThink( void );

	// Inputs
	void InputInvertOn( inputdata_t &inputdata );
	void InputInvertOff( inputdata_t &inputdata );

	float	m_flProximityDistance;	// How far before I start reacting
	float	m_flProximityOffset;	
	float	m_flFieldOfView;
	
	EHANDLE	m_hLooker;				// Who is looking

	DECLARE_DATADESC();
};


class CLookDoorThinker : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLookDoorThinker, CLogicalEntity );

	void	LookThink( void );
	EHANDLE	m_hLookDoor;				// Who owns me
	
	DECLARE_DATADESC();
};


BEGIN_DATADESC( CLookDoorThinker )

	DEFINE_FIELD( m_hLookDoor, FIELD_EHANDLE ),

	// Function Pointers
	DEFINE_FUNCTION(LookThink),

END_DATADESC()


LINK_ENTITY_TO_CLASS( lookdoorthinker, CLookDoorThinker );


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CLookDoorThinker::LookThink(void)
{
	if (m_hLookDoor)
	{
		((CLookDoor*)(CBaseEntity*)m_hLookDoor)->MoveThink();
		SetNextThink( gpGlobals->curtime + 0.01f );
	}
	else
	{
		UTIL_Remove(this);
	}
}


BEGIN_DATADESC( CLookDoor )

	DEFINE_KEYFIELD( m_flProximityDistance,	FIELD_FLOAT, "ProximityDistance"),
	DEFINE_KEYFIELD( m_flProximityOffset,	FIELD_FLOAT, "ProximityOffset"),
	DEFINE_KEYFIELD( m_flFieldOfView,		FIELD_FLOAT, "FieldOfView" ),
	DEFINE_FIELD(m_hLooker,				FIELD_EHANDLE),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,		"InvertOn",		InputInvertOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"InvertOff",	InputInvertOff ),

	// Function Pointers
	DEFINE_FUNCTION(MoveThink),

END_DATADESC()

LINK_ENTITY_TO_CLASS( func_lookdoor, CLookDoor );


//------------------------------------------------------------------------------
// Purpose : Input handlers.
//------------------------------------------------------------------------------
void CLookDoor::InputInvertOn( inputdata_t &inputdata )
{
	m_spawnflags |= SF_LDOOR_INVERT;
}

void CLookDoor::InputInvertOff( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_LDOOR_INVERT;
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CLookDoor::Spawn(void)
{
	BaseClass::Spawn();

	if (m_target == NULL_STRING)
	{
		Warning( "ERROR: DoorLook (%s) given no target.  Rejecting spawn.\n",GetDebugName());
		return;
	}
	CLookDoorThinker* pLookThinker = (CLookDoorThinker*)CreateEntityByName("lookdoorthinker");
	if (pLookThinker)
	{
		pLookThinker->SetThink(&CLookDoorThinker::LookThink);
		pLookThinker->m_hLookDoor = this;
		pLookThinker->SetNextThink( gpGlobals->curtime + 0.1f );
	}
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CLookDoor::MoveThink(void)
{
	// --------------------------------
	// Make sure we have a looker
	// --------------------------------
	if (m_hLooker == NULL)
	{
		m_hLooker = (CBaseEntity*)gEntList.FindEntityByName( NULL, m_target );

		if (m_hLooker == NULL)
		{
			return;
		}
	}

	//--------------------------------------
	// Calculate an orgin for the door
	//--------------------------------------
	Vector vOrigin = WorldSpaceCenter() - GetAbsOrigin();

	// If FROM_OPEN flag is set, door proximity is measured
	// from the open and not the closed position
	if (FBitSet (m_spawnflags, SF_LDOOR_FROM_OPEN))
	{
		vOrigin += m_vecPosition2;
	}

	// ------------------------------------------------------
	//  First add movement based on proximity
	// ------------------------------------------------------
	float flProxMove = 0;
	if (m_flProximityDistance > 0)
	{
		float flDist = (m_hLooker->GetAbsOrigin() - vOrigin).Length()-m_flProximityOffset;
		if (flDist < 0) flDist = 0;

		if (flDist < m_flProximityDistance)
		{
			if (FBitSet (m_spawnflags, SF_LDOOR_THRESHOLD))
			{
				flProxMove = 1.0;
			}
			else
			{
				flProxMove = 1-flDist/m_flProximityDistance;
			}
		}
	}

	// ------------------------------------------------------
	//  Then add movement based on view angle
	// ------------------------------------------------------
	float flViewMove = 0;
	if (m_flFieldOfView > 0)
	{
		// ----------------------------------------
		// Check that toucher is facing the target
		// ----------------------------------------
		Assert( dynamic_cast< CBaseCombatCharacter* >( m_hLooker.Get() ) );
		CBaseCombatCharacter* pBCC = (CBaseCombatCharacter*)m_hLooker.Get();
		Vector vTouchDir = pBCC->EyeDirection3D( );
		Vector vTargetDir =  vOrigin - pBCC->EyePosition();
		VectorNormalize(vTargetDir);

		float flDotPr = DotProduct(vTouchDir,vTargetDir);
		if (flDotPr < m_flFieldOfView)
		{
			flViewMove = 0.0;
		}
		else
		{
			flViewMove = (flDotPr-m_flFieldOfView)/(1.0 - m_flFieldOfView);
		}
	}

	//---------------------------------------
	// Summate the two moves
	//---------------------------------------
	float flMove = flProxMove + flViewMove;
	if (flMove > 1.0)
	{
		flMove = 1.0;
	}

	// If behavior is inverted do the reverse
	if (FBitSet (m_spawnflags, SF_LDOOR_INVERT))
	{
		flMove = 1-flMove;
	}

	// Move the door
	SetPosition( flMove );
}

