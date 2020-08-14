//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Controls and detects difficulty level changes
//
//=============================================================================

#include "cbase.h"

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CLogicSkill : public CLogicalEntity
{
	DECLARE_CLASS( CLogicSkill, CLogicalEntity );

private:
	// Inputs
	void InputTest( inputdata_t &inputdata );
	void InputStartListening( inputdata_t &inputdata ) {m_bListeningForSkillChanges = true;}
	void InputStopListening( inputdata_t &inputdata ) {m_bListeningForSkillChanges = false;}

	// Used by gamerules to fire OnSkillChanged.
	// Passes the level it changed to as well.
	void InputSkillLevelChanged(inputdata_t &inputdata) { m_bListeningForSkillChanges ? m_OnSkillChanged.Set(inputdata.value.Int(), inputdata.pActivator, this) : (void)0; }

	COutputInt m_OnSkillChanged;
	COutputEvent m_OnEasy;
	COutputEvent m_OnMedium;
	COutputEvent m_OnHard;

	bool m_bListeningForSkillChanges;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(logic_skill, CLogicSkill);

BEGIN_DATADESC( CLogicSkill )

	DEFINE_KEYFIELD( m_bListeningForSkillChanges, FIELD_BOOLEAN, "ListenForSkillChange" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Test", InputTest ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartListening", InputStartListening ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopListening", InputStopListening ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SkillLevelChanged", InputSkillLevelChanged ),

	DEFINE_OUTPUT( m_OnSkillChanged, "OnSkillChanged" ),
	DEFINE_OUTPUT( m_OnEasy, "OnEasy" ),
	DEFINE_OUTPUT( m_OnMedium, "OnNormal" ),
	DEFINE_OUTPUT( m_OnHard, "OnHard" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLogicSkill::InputTest( inputdata_t &inputdata )
{
	switch (g_pGameRules->GetSkillLevel())
	{
	case SKILL_EASY:	m_OnEasy.FireOutput(this, this); break;
	case SKILL_MEDIUM:	m_OnMedium.FireOutput(this, this); break;
	case SKILL_HARD:	m_OnHard.FireOutput(this, this); break;
	}
}
