//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_NAV_START_ON		0x0001

enum navproperties_t
{
	NAV_IGNORE = 1<<0,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLogicNavigation : public CLogicalEntity,
						 public IEntityListener
{
	DECLARE_CLASS( CLogicNavigation, CLogicalEntity );

	bool KeyValue( const char *szKeyName, const char *szValue );
	void Activate( void );

private:
	void UpdateOnRemove();

	void OnEntitySpawned( CBaseEntity *pEntity );

	// Inputs
	void InputTurnOn( inputdata_t &inputdata ) { TurnOn(); }
	void InputTurnOff( inputdata_t &inputdata ) { TurnOff(); }
	void InputToggle( inputdata_t &inputdata ) 
	{ 
		if ( m_isOn )
			TurnOff();
		else
			TurnOn();
	}

	void TurnOn();
	void TurnOff();
	void UpdateProperty();

	DECLARE_DATADESC();

	bool				m_isOn;
	navproperties_t		m_navProperty;
};

LINK_ENTITY_TO_CLASS(logic_navigation, CLogicNavigation);


BEGIN_DATADESC( CLogicNavigation )

	DEFINE_FIELD( m_isOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_navProperty, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOn", InputTurnOn),
	DEFINE_INPUTFUNC(FIELD_VOID, "TurnOff", InputTurnOff),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szKeyName - 
//			*szValue - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CLogicNavigation::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "navprop") )
	{
		if ( FStrEq( szValue, "Ignore" ) )
		{
			m_navProperty = NAV_IGNORE;
		}
		else
		{
			DevMsg( 1, "Unknown nav property %s\n", szValue );
		}
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicNavigation::Activate()
{
	BaseClass::Activate();

	if ( HasSpawnFlags( SF_NAV_START_ON ) )
	{
		TurnOn();
		RemoveSpawnFlags( SF_NAV_START_ON );
	}
	else if ( m_isOn )
	{
		gEntList.AddListenerEntity( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicNavigation::UpdateOnRemove()
{
	if ( m_isOn )
	{
		gEntList.RemoveListenerEntity( this );
	}

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicNavigation::OnEntitySpawned( CBaseEntity *pEntity )
{
	if ( m_isOn && ( m_navProperty & NAV_IGNORE ) && pEntity->NameMatches( m_target ) )
	{
		pEntity->SetNavIgnore();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicNavigation::TurnOn()
{
	if ( m_isOn )
		return;
	m_isOn = true;
	gEntList.AddListenerEntity( this );
	UpdateProperty();
}

void CLogicNavigation::UpdateProperty()
{
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByName( pEntity, STRING(m_target) ) ) != NULL )
	{
		if ( m_isOn )
		{
			if ( m_navProperty & NAV_IGNORE )
				pEntity->SetNavIgnore();
		}
		else
		{
			if ( m_navProperty & NAV_IGNORE )
				pEntity->ClearNavIgnore();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicNavigation::TurnOff()
{
	if ( !m_isOn )
		return;

	m_isOn = false;
	gEntList.RemoveListenerEntity( this );
	UpdateProperty();
}

