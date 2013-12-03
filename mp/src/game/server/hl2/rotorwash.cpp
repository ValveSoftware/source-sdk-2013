//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "rotorwash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ==============================================
//  Rotorwash entity
// ==============================================

class CRotorWashEmitter : public CBaseEntity
{
public:
	DECLARE_CLASS( CRotorWashEmitter, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	void SetAltitude( float flAltitude ) { m_flAltitude = flAltitude; }
	void SetEmit( bool state ) { m_bEmit = state; }
	void Spawn ( void );
	void Precache( void );
	int ShouldTransmit( const CCheckTransmitInfo *pInfo );
	int UpdateTransmitState( void );

protected:

	CNetworkVar( bool, m_bEmit );
	CNetworkVar( float, m_flAltitude );
};

IMPLEMENT_SERVERCLASS_ST( CRotorWashEmitter, DT_RotorWashEmitter )
	SendPropFloat(SENDINFO(m_flAltitude), -1, SPROP_NOSCALE ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_rotorwash_emitter, CRotorWashEmitter );

BEGIN_DATADESC( CRotorWashEmitter )
	DEFINE_FIELD( 		m_bEmit, 		FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flAltitude, 	FIELD_FLOAT, "altitude" ),
END_DATADESC()

void CRotorWashEmitter::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	SetEmit( false );
}

void CRotorWashEmitter::Precache( void )
{
	PrecacheMaterial( "effects/splashwake3" );
}

int CRotorWashEmitter::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( GetParent() )
	{
		return GetParent()->ShouldTransmit( pInfo );
	}

	return FL_EDICT_PVSCHECK;
}

int CRotorWashEmitter::UpdateTransmitState( void )
{
	if ( GetParent() )
	{
		return SetTransmitState( FL_EDICT_FULLCHECK );
	}

	return SetTransmitState( FL_EDICT_PVSCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &localOrigin - 
//			&localAngles - 
//			*pOwner - 
//			flAltitude - 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CBaseEntity *CreateRotorWashEmitter( const Vector &localOrigin, const QAngle &localAngles, CBaseEntity *pOwner, float flAltitude )
{
	CRotorWashEmitter *pEmitter = (CRotorWashEmitter *) CreateEntityByName( "env_rotorwash_emitter" );

	if ( pEmitter == NULL )
		return NULL;

	pEmitter->SetAbsOrigin( localOrigin );
	pEmitter->SetAbsAngles( localAngles );
	pEmitter->FollowEntity( pOwner );

	pEmitter->SetAltitude( flAltitude );
	pEmitter->SetEmit( false );

	return pEmitter;
}