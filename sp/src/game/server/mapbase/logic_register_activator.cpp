//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ====
//
// Purpose: Source SDK-based replication of logic_register_activator from later versions
//			of Source.
// 
//			This is based entirely on Source 2013 code and Portal 2's FGD entry.
//			It does not actually use code from Portal 2 or later.
//
//=============================================================================

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CLogicRegisterActivator : public CLogicalEntity
{
public:
	DECLARE_CLASS( CLogicRegisterActivator, CLogicalEntity );

	CLogicRegisterActivator();

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	void InputFireRegisteredAsActivator1( inputdata_t &inputdata );
	void InputFireRegisteredAsActivator2( inputdata_t &inputdata );
	void InputFireRegisteredAsActivator3( inputdata_t &inputdata );
	void InputFireRegisteredAsActivator4( inputdata_t &inputdata );
	void InputRegisterEntity( inputdata_t &inputdata );

	DECLARE_DATADESC();

	// Outputs
	COutputEvent m_OnRegisteredActivate[ 4 ];

	EHANDLE m_hActivator;
	
private:

	bool m_bDisabled;
};

LINK_ENTITY_TO_CLASS(logic_register_activator, CLogicRegisterActivator);


BEGIN_DATADESC( CLogicRegisterActivator )

	DEFINE_KEYFIELD(m_bDisabled, FIELD_BOOLEAN, "StartDisabled"),

	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireRegisteredAsActivator1", InputFireRegisteredAsActivator1),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireRegisteredAsActivator2", InputFireRegisteredAsActivator2),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireRegisteredAsActivator3", InputFireRegisteredAsActivator3),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireRegisteredAsActivator4", InputFireRegisteredAsActivator4),
	DEFINE_INPUTFUNC(FIELD_EHANDLE, "RegisterEntity", InputRegisterEntity),

	// Outputs
	DEFINE_OUTPUT(m_OnRegisteredActivate[0], "OnRegisteredActivate1"),
	DEFINE_OUTPUT(m_OnRegisteredActivate[1], "OnRegisteredActivate2"),
	DEFINE_OUTPUT(m_OnRegisteredActivate[2], "OnRegisteredActivate3"),
	DEFINE_OUTPUT(m_OnRegisteredActivate[3], "OnRegisteredActivate4"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CLogicRegisterActivator::CLogicRegisterActivator(void)
{
}

//------------------------------------------------------------------------------
// Purpose: Turns on the entity, allowing it to fire outputs.
//------------------------------------------------------------------------------
void CLogicRegisterActivator::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}

//------------------------------------------------------------------------------
// Purpose: Turns off the entity, preventing it from firing outputs.
//------------------------------------------------------------------------------
void CLogicRegisterActivator::InputDisable( inputdata_t &inputdata )
{ 
	m_bDisabled = true;
}

//------------------------------------------------------------------------------
// Purpose: Toggles the enabled/disabled state of the entity.
//------------------------------------------------------------------------------
void CLogicRegisterActivator::InputToggle( inputdata_t &inputdata )
{ 
	m_bDisabled = !m_bDisabled;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that fires its respective OnRegisteredActivate with the stored activator.
//-----------------------------------------------------------------------------
void CLogicRegisterActivator::InputFireRegisteredAsActivator1( inputdata_t &inputdata )
{
	m_OnRegisteredActivate[0].FireOutput(m_hActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that fires its respective OnRegisteredActivate with the stored activator.
//-----------------------------------------------------------------------------
void CLogicRegisterActivator::InputFireRegisteredAsActivator2( inputdata_t &inputdata )
{
	m_OnRegisteredActivate[1].FireOutput(m_hActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that fires its respective OnRegisteredActivate with the stored activator.
//-----------------------------------------------------------------------------
void CLogicRegisterActivator::InputFireRegisteredAsActivator3( inputdata_t &inputdata )
{
	m_OnRegisteredActivate[2].FireOutput(m_hActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that fires its respective OnRegisteredActivate with the stored activator.
//-----------------------------------------------------------------------------
void CLogicRegisterActivator::InputFireRegisteredAsActivator4( inputdata_t &inputdata )
{
	m_OnRegisteredActivate[3].FireOutput(m_hActivator, this);
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that stores an entity as the activator.
//-----------------------------------------------------------------------------
void CLogicRegisterActivator::InputRegisterEntity( inputdata_t &inputdata )
{
	m_hActivator = inputdata.value.Entity();
}
