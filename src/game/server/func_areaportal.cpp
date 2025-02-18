//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: area portal entity: toggles visibility areas on/off
//
// NOTE: These are not really brush entities.  They are brush entities from a 
// designer/worldcraft perspective, but by the time they reach the game, the 
// brush model is gone and this is, in effect, a point entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "func_areaportalbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

enum areaportal_state
{
	AREAPORTAL_CLOSED = 0,
	AREAPORTAL_OPEN = 1,
};


class CAreaPortal : public CFuncAreaPortalBase
{
public:
	DECLARE_CLASS( CAreaPortal, CFuncAreaPortalBase );

					CAreaPortal();

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
	virtual int		UpdateTransmitState();

	// Input handlers
	void InputOpen( inputdata_t &inputdata );
	void InputClose( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	virtual bool	UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient );

	DECLARE_DATADESC();

private:
	bool	UpdateState( void );

	int		m_state;
};

LINK_ENTITY_TO_CLASS( func_areaportal, CAreaPortal );

BEGIN_DATADESC( CAreaPortal )

	DEFINE_KEYFIELD( m_portalNumber, FIELD_INTEGER, "portalnumber" ),
	DEFINE_FIELD( m_state, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Open",  InputOpen ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Close", InputClose ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle",  InputToggle ),

	// TODO: obsolete! remove	
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn",  InputClose ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputOpen ),

END_DATADESC()



CAreaPortal::CAreaPortal()
{
	m_state = AREAPORTAL_OPEN;
}


void CAreaPortal::Spawn( void )
{
	AddEffects( EF_NORECEIVESHADOW | EF_NOSHADOW );
	Precache();
}


//-----------------------------------------------------------------------------
// Purpose: notify the engine of the state at startup/restore
//-----------------------------------------------------------------------------
void CAreaPortal::Precache( void )
{
	UpdateState();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAreaPortal::InputClose( inputdata_t &inputdata )
{
	m_state = AREAPORTAL_CLOSED;
	UpdateState();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAreaPortal::InputOpen( inputdata_t &inputdata )
{
	m_state = AREAPORTAL_OPEN;
	UpdateState();
}


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CAreaPortal::InputToggle( inputdata_t &inputdata )
{
	m_state = ((m_state == AREAPORTAL_OPEN) ? AREAPORTAL_CLOSED : AREAPORTAL_OPEN);
	UpdateState();
}


bool CAreaPortal::UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient )
{
	if ( m_state )
	{
		// We're not closed, so give the base class a chance to close it.
		return BaseClass::UpdateVisibility( vOrigin, fovDistanceAdjustFactor, bIsOpenOnClient );
	}
	else
	{
		bIsOpenOnClient = false;
		return false;
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CAreaPortal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_ON )
	{
		m_state = AREAPORTAL_OPEN;
	}
	else if ( useType == USE_OFF )
	{
		m_state = AREAPORTAL_CLOSED;
	}
	else
	{
		return;
	}

	UpdateState();
}


bool CAreaPortal::KeyValue( const char *szKeyName, const char *szValue )
{
	if( FStrEq( szKeyName, "StartOpen" ) )
	{
		m_state = (atoi(szValue) != 0) ? AREAPORTAL_OPEN : AREAPORTAL_CLOSED;

		return true;
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}
}


bool CAreaPortal::UpdateState()
{
	engine->SetAreaPortalState( m_portalNumber, m_state );
	return !!m_state;
}


int CAreaPortal::UpdateTransmitState()
{
	// Our brushes are kept around so don't transmit anything since we don't want to draw them.
	return SetTransmitState( FL_EDICT_DONTSEND );
}

