//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: A special brush that collides with clientside entities, primarily ragdolls.
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Purpose: basic solid geometry
// enabled state:	brush is visible
// disabled staute:	brush not visible
//-----------------------------------------------------------------------------
class CFuncClientClip : public CBaseEntity
{
public:
	DECLARE_CLASS( CFuncClientClip, CBaseEntity );
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	bool CreateVPhysics( void );

	virtual int DrawDebugTextOverlays( void );

	void TurnOff( void );
	void TurnOn( void );

	// Input handlers
	void InputTurnOff( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	CNetworkVar( bool, m_bDisabled );

	DECLARE_DATADESC();

	virtual bool IsOn( void );

	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

LINK_ENTITY_TO_CLASS( func_clip_client, CFuncClientClip );

BEGIN_DATADESC( CFuncClientClip )

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncClientClip, DT_FuncClientClip )
	SendPropBool( SENDINFO( m_bDisabled ) ),
END_SEND_TABLE()


void CFuncClientClip::Spawn( void )
{
	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything

	SetSolid( GetParent() ? SOLID_VPHYSICS : SOLID_BSP );
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	AddSolidFlags( FSOLID_NOT_SOLID );

	SetModel( STRING( GetModelName() ) );

	if ( m_bDisabled )
		TurnOff();
	
	// If it can't move/go away, it's really part of the world
	if ( !GetEntityName() || !m_iParent )
		AddFlag( FL_WORLDBRUSH );

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CFuncClientClip::CreateVPhysics( void )
{
	// NOTE: Don't init this static.  It's pretty common for these to be constrained
	// and dynamically parented.  Initing shadow avoids having to destroy the physics
	// object later and lose the constraints.
	IPhysicsObject *pPhys = VPhysicsInitShadow(false, false);
	if ( pPhys )
	{
		int contents = modelinfo->GetModelContents( GetModelIndex() );
		if ( ! (contents & (MASK_SOLID|MASK_PLAYERSOLID|MASK_NPCSOLID)) )
		{
			// leave the physics shadow there in case it has crap constrained to it
			// but disable collisions with it
			pPhys->EnableCollisions( false );
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CFuncClientClip::DrawDebugTextOverlays( void )
{
	int nOffset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf( tempstr,sizeof(tempstr), "angles: %g %g %g", (double)GetLocalAngles()[PITCH], (double)GetLocalAngles()[YAW], (double)GetLocalAngles()[ROLL] );
		EntityText( nOffset, tempstr, 0 );
		nOffset++;

		Q_snprintf(tempstr, sizeof(tempstr), "	enabled: %d", !m_bDisabled);
		EntityText(nOffset, tempstr, 0);
		nOffset++;
	}

	return nOffset;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the hidden/shown state of the brush.
//-----------------------------------------------------------------------------
void CFuncClientClip::InputToggle( inputdata_t &inputdata )
{
	if ( IsOn() )
	{
		TurnOff();
		return;
	}

	TurnOn();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for hiding the brush.
//-----------------------------------------------------------------------------
void CFuncClientClip::InputTurnOff( inputdata_t &inputdata )
{
	TurnOff();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for showing the brush.
//-----------------------------------------------------------------------------
void CFuncClientClip::InputTurnOn( inputdata_t &inputdata )
{
	TurnOn();
}

//-----------------------------------------------------------------------------
// Purpose: Hides the brush.
//-----------------------------------------------------------------------------
void CFuncClientClip::TurnOff( void )
{
	if ( !IsOn() )
		return;

	AddEffects( EF_NODRAW );
	m_bDisabled = true;
}


//-----------------------------------------------------------------------------
// Purpose: Shows the brush.
//-----------------------------------------------------------------------------
void CFuncClientClip::TurnOn( void )
{
	if ( IsOn() )
		return;

	RemoveEffects( EF_NODRAW );
	m_bDisabled = false;
}


inline bool CFuncClientClip::IsOn( void )
{
	return !m_bDisabled;
}
