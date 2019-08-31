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

#ifdef MAPBASE
	// For func_areaportal_oneway.
	int				GetPortalState() { return m_state; }
#endif

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

#ifdef MAPBASE
// An areaportal that automatically closes and opens depending on the direction of the client.
// http://developer.valvesoftware.com/wiki/CAreaPortalOneWay
class CAreaPortalOneWay : public CAreaPortal // CAPOW!
{
	DECLARE_CLASS( CAreaPortalOneWay, CAreaPortal );
	DECLARE_DATADESC();

public:
	Vector	 m_vecOpenVector;
	bool	 m_bAvoidPop;
	bool	 m_bOneWayActive;
	
	void Spawn();
	void Activate();
	int  Restore(IRestore &restore);
	bool UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient );

	void InputDisableOneWay( inputdata_t &inputdata );
	void InputEnableOneWay( inputdata_t &inputdata );
	void InputToggleOneWay( inputdata_t &inputdata );
	void InputInvertOneWay( inputdata_t &inputdata );

protected:
	void RemoteUpdate( bool IsOpen );

	bool m_bRemotelyUpdated;
	bool m_bRemoteCalcWasOpen;
	CHandle<CAreaPortalOneWay> m_hNextPortal; // This get saved to disc...
	CAreaPortalOneWay* m_pNextPortal; // ...while this gets used at runtime, avoiding loads of casts

private:
	void UpdateNextPortal( bool IsOpen );

	// These two are irrelevant once the entity has established itself
	string_t m_strGroupName;
	Vector	 m_vecOrigin_; // The portal won't compile properly if vecOrigin itself has a value, but it's okay to move something in at runtime
};

LINK_ENTITY_TO_CLASS( func_areaportal_oneway, CAreaPortalOneWay );

BEGIN_DATADESC( CAreaPortalOneWay )
	DEFINE_KEYFIELD( m_vecOpenVector, FIELD_VECTOR, "onewayfacing" ),
	DEFINE_KEYFIELD( m_bAvoidPop, FIELD_BOOLEAN, "avoidpop" ),
	DEFINE_KEYFIELD_NOT_SAVED( m_vecOrigin_, FIELD_VECTOR, "origin_" ),
	DEFINE_KEYFIELD_NOT_SAVED( m_strGroupName, FIELD_STRING, "group" ),
	DEFINE_FIELD( m_bOneWayActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hNextPortal, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableOneWay", InputDisableOneWay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableOneWay", InputEnableOneWay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleOneWay", InputToggleOneWay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "InvertOneWay", InputInvertOneWay ),
END_DATADESC()

void CAreaPortalOneWay::Spawn()
{
	// Convert our angle from Hammer to a proper vector
	QAngle angOpenDir = QAngle( m_vecOpenVector.x, m_vecOpenVector.y, m_vecOpenVector.z );
	AngleVectors( angOpenDir, &m_vecOpenVector );

	SetLocalOrigin(m_vecOrigin_);
	m_bOneWayActive = true;
	m_bRemotelyUpdated = false;

	BaseClass::Spawn();
}

void CAreaPortalOneWay::Activate()
{
	// Optimisation: share open/closed value for CAPOWs with the same GroupName.
	if (m_strGroupName != NULL_STRING)
	{
		for( unsigned short i = GetPortalListElement(); i != g_AreaPortals.InvalidIndex(); i = g_AreaPortals.Next(i) )
		{
			CAreaPortalOneWay* pCur = dynamic_cast<CAreaPortalOneWay*>(g_AreaPortals[i]);

			if ( pCur && pCur != this && strcmp( STRING(m_strGroupName),STRING(pCur->m_strGroupName) ) == 0 )
			{
				m_pNextPortal = pCur;
				m_hNextPortal = pCur;
				break;
			}
		}
	}

	BaseClass::Activate();
}

int CAreaPortalOneWay::Restore(IRestore &restore)
{
	if ( m_hNextPortal.IsValid() )
		m_pNextPortal = m_hNextPortal.Get();

	return BaseClass::Restore(restore);
}

// Disable the CAPOW (becomes a normal AP)
void CAreaPortalOneWay::InputDisableOneWay( inputdata_t &inputdata )
{
	m_bOneWayActive = false;
}

// Re-enable the CAPOW
void CAreaPortalOneWay::InputEnableOneWay( inputdata_t &inputdata )
{
	m_bOneWayActive = true;
}

// Toggle CAPOW
void CAreaPortalOneWay::InputToggleOneWay( inputdata_t &inputdata )
{
	m_bOneWayActive = !m_bOneWayActive;
}

// Flip the one way direction
void CAreaPortalOneWay::InputInvertOneWay( inputdata_t &inputdata )
{
	m_vecOpenVector.Negate();
}

// Recieve a shared state from another CAPOW, then pass it on to the next
void CAreaPortalOneWay::RemoteUpdate( bool IsOpen )
{
	m_bRemotelyUpdated = true;
	m_bRemoteCalcWasOpen = IsOpen;
	UpdateNextPortal(IsOpen);
}

// Inline func since this code is required three times
inline void CAreaPortalOneWay::UpdateNextPortal( bool IsOpen )
{
	if (m_pNextPortal)
		m_pNextPortal->RemoteUpdate(IsOpen);
}

#define VIEWER_PADDING 80 // Value copied from func_areaportalbase.cpp

bool CAreaPortalOneWay::UpdateVisibility( const Vector &vOrigin, float fovDistanceAdjustFactor, bool &bIsOpenOnClient )
{
	if (!m_bOneWayActive)
		return BaseClass::UpdateVisibility( vOrigin, fovDistanceAdjustFactor, bIsOpenOnClient );

	if( m_portalNumber == -1 || GetPortalState() == AREAPORTAL_CLOSED )
	{
		bIsOpenOnClient = false;
		return false;
	}

	// Has another CAPOW on our plane already done a calculation?
	// Note that the CAPOW chain is traversed with new values in RemoteUpdate(), NOT here
	if (m_bRemotelyUpdated)
	{
		m_bRemotelyUpdated = false;
		return m_bRemoteCalcWasOpen ? BaseClass::UpdateVisibility( vOrigin, fovDistanceAdjustFactor, bIsOpenOnClient ) : false;
	}

	// ***********************
	// If we've got this far then we're the first CAPOW in the chain this frame
	// and need to calculate a value and pass it along said chain ourselves
	// ***********************

	float dist = VIEWER_PADDING; // Assume open for backfacing tests...
	VPlane plane;
	if( engine->GetAreaPortalPlane(vOrigin,m_portalNumber,&plane) )
		dist = plane.DistTo(vOrigin);	// ...but if we find a plane, use a genuine figure instead.
										// This is done because GetAreaPortalPlane only works for 
										// portals facing the current area.

	// We can use LocalOrigin here because APs never have parents.
	float dot = DotProduct(m_vecOpenVector,vOrigin - GetLocalOrigin());

	if( dot > 0 )
	{
		// We are on the open side of the portal. Pass the result on!
		UpdateNextPortal(true);
		
		// The following backfacing check is the inverse of CFuncAreaPortalBase's: 
		// it /closes/ the portal if the camera is /behind/ the plane. IsOpenOnClient
		// is left alone as per func_areaportalbase.h
		return dist < -VIEWER_PADDING ? false : true;
	}
	else // Closed side
	{
		// To avoid latency pop when crossing the portal's plane, it is only 
		// closed on the client if said client is outside the "padding zone".
		if ( !m_bAvoidPop || (m_bAvoidPop && dist > VIEWER_PADDING) )
			bIsOpenOnClient = false;

		// We are definitely closed on the server, however.
		UpdateNextPortal(false);
		return false;
	}
}
#endif
