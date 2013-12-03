//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that can be used to constrain the player's movement around it
//
//=============================================================================//

#include "cbase.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	SF_TELEPORT_TO_SPAWN_POS	0x00000001

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointPlayerMoveConstraint : public CBaseEntity
{
	DECLARE_CLASS( CPointPlayerMoveConstraint, CBaseEntity );
public:
	DECLARE_DATADESC();

	int		UpdateTransmitState( void );
	void	Activate( void );
	void	ConstraintThink( void );

	void	InputTurnOn( inputdata_t &inputdata );
	void	InputTurnOff( inputdata_t &inputdata );

private:
	float				m_flRadius;	
	float				m_flConstraintWidth;
	float				m_flSpeedFactor;
	float				m_flRadiusSquared;	
	CUtlVector<EHANDLE>	m_hConstrainedPlayers;
	COutputEvent		m_OnConstraintBroken;
};

LINK_ENTITY_TO_CLASS( point_playermoveconstraint, CPointPlayerMoveConstraint );

BEGIN_DATADESC( CPointPlayerMoveConstraint )

	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_flConstraintWidth, FIELD_FLOAT, "width" ),
	DEFINE_KEYFIELD( m_flSpeedFactor, FIELD_FLOAT, "speedfactor" ),
	// DEFINE_FIELD( m_flRadiusSquared, FIELD_FLOAT ),		// Don't Save
	DEFINE_UTLVECTOR( m_hConstrainedPlayers, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( ConstraintThink ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

	DEFINE_OUTPUT( m_OnConstraintBroken, "OnConstraintBroken" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPointPlayerMoveConstraint::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointPlayerMoveConstraint::Activate( void )
{
	BaseClass::Activate();

	m_flRadiusSquared = (m_flRadius * m_flRadius);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPointPlayerMoveConstraint::InputTurnOn( inputdata_t &inputdata )
{
	// Find all players within our radius and constraint them
	float flRadius = m_flRadius;
	// If we're in singleplayer, blow the radius a bunch
	if ( gpGlobals->maxClients == 1 )
	{
		flRadius = MAX_COORD_RANGE;
	}
	CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassnameWithin( pEntity, "player", GetLocalOrigin(), flRadius)) != NULL )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pEntity );
		Assert( pPlayer );
		
		// Only add him if he's not already constrained
		if ( m_hConstrainedPlayers.Find( pPlayer ) == m_hConstrainedPlayers.InvalidIndex() )
		{
			m_hConstrainedPlayers.AddToTail( pPlayer );

			pPlayer->ActivateMovementConstraint( this, GetAbsOrigin(), m_flRadius, m_flConstraintWidth, m_flSpeedFactor );
		}
	}

	// Only think if we found any
	if ( m_hConstrainedPlayers.Count() )
	{
		SetThink( &CPointPlayerMoveConstraint::ConstraintThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//------------------------------------------------------------------------------
// Purpose: Release all players we've constrained
//------------------------------------------------------------------------------
void CPointPlayerMoveConstraint::InputTurnOff( inputdata_t &inputdata )
{
	int iCount = m_hConstrainedPlayers.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CBasePlayer *pPlayer = ToBasePlayer( m_hConstrainedPlayers[i] );
		if ( pPlayer )
		{
			pPlayer->DeactivateMovementConstraint();
		}
	}

	m_hConstrainedPlayers.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if any of our constrained players have broken the constraint
//-----------------------------------------------------------------------------
void CPointPlayerMoveConstraint::ConstraintThink( void )
{
	int iCount = m_hConstrainedPlayers.Count();

	// Count backwards, because we might drop them if they've broken the constraint
	for ( int i = (iCount-1); i >= 0; i-- )
	{
		CBasePlayer *pPlayer = ToBasePlayer( m_hConstrainedPlayers[i] );
		if ( pPlayer )
		{
			float flDistanceSqr = (pPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
			if ( flDistanceSqr > m_flRadiusSquared )
			{
				// Break the constraint to this player
				pPlayer->DeactivateMovementConstraint();
				m_hConstrainedPlayers.Remove(i);

				// Fire the broken output
				m_OnConstraintBroken.FireOutput( this, pPlayer );
			}
		}
	}

	// Only keep thinking if we any left
	if ( m_hConstrainedPlayers.Count() )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}
