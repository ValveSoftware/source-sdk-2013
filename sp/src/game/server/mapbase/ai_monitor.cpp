//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: An entity that watches an NPC for certain things.
//
//=============================================================================

#include "cbase.h"
#include "ai_schedule.h"
#include "ai_hint.h"
#include "ai_route.h"
#include "ai_basenpc.h"
#include "saverestore_utlvector.h"


//#define AI_MONITOR_MAX_TARGETS 16

// Uses a CUtlVector instead of a CBitVec for conditions/schedules.
// Using a CUtlVector makes this a lot easier, if you ask me. Please note that the CBitVec version is incomplete.
#define AI_MONITOR_USE_UTLVECTOR 1

//-----------------------------------------------------------------------------
// Purpose: AI monitoring. Probably bad.
//-----------------------------------------------------------------------------
class CAI_Monitor : public CLogicalEntity
{
	DECLARE_CLASS( CAI_Monitor, CLogicalEntity );
public:
	CAI_Monitor();

	void Spawn();
	void Activate( void );

	virtual int			Save( ISave &save ); 
	virtual int			Restore( IRestore &restore );
#if !AI_MONITOR_USE_UTLVECTOR
	void				SaveConditions( ISave &save, const CAI_ScheduleBits &conditions );
	void				RestoreConditions( IRestore &restore, CAI_ScheduleBits *pConditions );
#endif

	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	//virtual bool GetKeyValue( const char *szKeyName, char *szValue, int iMaxLen );

	// Populates our NPC list.
	void PopulateNPCs(inputdata_t *inputdata);

	// Does evaluation, fires outputs, etc.
	bool NPCDoEval(CAI_BaseNPC *pNPC);

	// Thinks.
	void MonitorThink();

	CAI_BaseNPC *GetFirstTarget();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); PopulateNPCs(&inputdata); }
	void InputPopulateNPCs( inputdata_t &inputdata );
	void InputTest( inputdata_t &inputdata );
	void InputTestNPC( inputdata_t &inputdata );

	// Allows mappers to get condition/schedule names from ID
	void InputGetConditionName( inputdata_t &inputdata ) { m_OutConditionName.Set(AllocPooledString(ConditionName(inputdata.value.Int())), inputdata.pActivator, this); }
	void InputGetScheduleName( inputdata_t &inputdata ) { m_OutScheduleName.Set(AllocPooledString(ScheduleName(inputdata.value.Int())), inputdata.pActivator, this); }
	COutputString m_OutConditionName;
	COutputString m_OutScheduleName;

	void InputSetCondition( inputdata_t &inputdata ) { SetCondition(TranslateConditionString(inputdata.value.String())); }
	void InputClearCondition( inputdata_t &inputdata ) { ClearCondition(TranslateConditionString(inputdata.value.String())); }
#if AI_MONITOR_USE_UTLVECTOR
	void InputClearAllConditions( inputdata_t &inputdata ) { m_Conditions.RemoveAll(); }
#else
	void InputClearAllConditions( inputdata_t &inputdata ) { m_Conditions.ClearAll(); }
#endif

	void InputSetSchedule( inputdata_t &inputdata ) { SetSchedule(TranslateScheduleString(inputdata.value.String())); }
	void InputClearSchedule( inputdata_t &inputdata ) { ClearSchedule(TranslateScheduleString(inputdata.value.String())); }
#if AI_MONITOR_USE_UTLVECTOR
	void InputClearAllSchedules( inputdata_t &inputdata ) { m_Schedules.RemoveAll(); }
#else
	void InputClearAllSchedules( inputdata_t &inputdata ) { m_Schedules.ClearAll(); }
#endif

	void InputSetHint( inputdata_t &inputdata ) { SetHint(inputdata.value.Int()); }
	void InputClearHint( inputdata_t &inputdata ) { ClearHint(inputdata.value.Int()); }
	void InputClearAllHints( inputdata_t &inputdata ) { m_Hints.RemoveAll(); }

public:

	bool m_bStartDisabled;

	// The NPCs.
	CUtlVector<AIHANDLE> pNPCs;
	int m_iMaxEnts;

	// Stop and engage cooldown at first successful pass
	bool m_bCooldownAtFirstSuccess;

	// Interval between monitors
	float m_flThinkTime;
	#define GetThinkTime() (m_flThinkTime != 0 ? m_flThinkTime : TICK_INTERVAL)

	// Cooldown after something is satisfied
	float m_flCooldownTime;
	#define GetCooldownTime() (m_flCooldownTime != -1 ? m_flCooldownTime : GetThinkTime())

	// ------------------------------
	// Conditions
	// ------------------------------
#if AI_MONITOR_USE_UTLVECTOR
	CUtlVector<int> m_Conditions;
#else
	CAI_ScheduleBits m_Conditions;
#endif

	COutputInt m_OnNPCHasCondition;
	COutputInt m_OnNPCLacksCondition;

	// Condition functions, most of these are from CAI_BaseNPC.
#if AI_MONITOR_USE_UTLVECTOR
	inline void			SetCondition( int iCondition ) { m_Conditions.HasElement(iCondition) ? NULL : m_Conditions.AddToTail(iCondition); }
	inline void			ClearCondition( int iCondition ) { m_Conditions.FindAndRemove(iCondition); }
	inline bool			HasCondition( int iCondition ) { return m_Conditions.HasElement(iCondition); }
#else
	inline void			SetCondition( int iCondition ) { m_Conditions.Set(iCondition); }
	inline void			ClearCondition( int iCondition ) { m_Conditions.Clear(iCondition); }
	inline bool			HasCondition( int iCondition ) { return m_Conditions.IsBitSet(iCondition); }
#endif

	static int			GetConditionID(const char* condName) { return CAI_BaseNPC::GetSchedulingSymbols()->ConditionSymbolToId(condName); }
	const char			*ConditionName(int conditionID);

	int					TranslateConditionString(const char *condName);
	inline int			ConditionLocalToGlobal(CAI_BaseNPC *pTarget, int conditionID) { return pTarget->GetClassScheduleIdSpace()->ConditionLocalToGlobal(conditionID); }

	// ------------------------------
	// Schedules
	// ------------------------------
#if AI_MONITOR_USE_UTLVECTOR
	CUtlVector<int> m_Schedules;
#else
	CAI_ScheduleBits m_Schedules;
#endif

	bool m_bTranslateSchedules;

	COutputInt m_OnNPCRunningSchedule;
	COutputInt m_OnNPCNotRunningSchedule;

	// Schedule functions, some of these are from CAI_BaseNPC.
#if AI_MONITOR_USE_UTLVECTOR
	inline void			SetSchedule( int iSchedule ) { m_Schedules.HasElement(iSchedule) ? NULL : m_Schedules.AddToTail(iSchedule); }
	inline void			ClearSchedule( int iSchedule ) { m_Schedules.FindAndRemove(iSchedule); }
	inline bool			HasSchedule( int iSchedule ) { return m_Schedules.HasElement(iSchedule); }
#else
	inline void			SetSchedule( int iSchedule ) { m_Schedules.Set(iSchedule); }
	inline void			ClearSchedule( int iSchedule ) { m_Schedules.Clear(iSchedule); }
	inline bool			HasSchedule( int iSchedule ) { return m_Schedules.IsBitSet(iSchedule); }
#endif

	static int			GetScheduleID(const char* schedName) { return CAI_BaseNPC::GetSchedulingSymbols()->ScheduleSymbolToId(schedName); }
	const char			*ScheduleName(int scheduleID);

	int					TranslateScheduleString(const char *schedName);
	inline int			ScheduleLocalToGlobal(CAI_BaseNPC *pTarget, int scheduleID) { return pTarget->GetClassScheduleIdSpace()->ScheduleLocalToGlobal(scheduleID); }

	// ------------------------------
	// Tasks
	// ------------------------------

	// TODO

	// ------------------------------
	// Hints
	// ------------------------------
	CUtlVector<int> m_Hints;

	COutputInt m_OnNPCUsingHint;
	COutputInt m_OnNPCNotUsingHint;

	inline void			SetHint( int iHint ) { m_Hints.HasElement(iHint) ? NULL : m_Hints.AddToTail(iHint); }
	inline void			ClearHint( int iHint ) { m_Hints.FindAndRemove(iHint); }
	inline bool			HasHint( int iHint ) { return m_Hints.HasElement(iHint); }

	// Only register a hint as "being used" when the NPC is this distance away or less
	float m_flDistanceFromHint;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( ai_monitor, CAI_Monitor );


BEGIN_DATADESC( CAI_Monitor )

#if AI_MONITOR_USE_UTLVECTOR
	DEFINE_UTLVECTOR( m_Conditions, FIELD_INTEGER ),
	DEFINE_UTLVECTOR( m_Schedules, FIELD_INTEGER ),
#endif
	DEFINE_UTLVECTOR( m_Hints, FIELD_INTEGER ),

	// Keys
	DEFINE_KEYFIELD( m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_INPUT( m_flThinkTime, FIELD_FLOAT, "SetMonitorInterval" ),
	DEFINE_INPUT( m_flCooldownTime, FIELD_FLOAT, "SetCooldownTime" ),
	DEFINE_KEYFIELD( m_bCooldownAtFirstSuccess, FIELD_BOOLEAN, "CooldownAt" ),

	DEFINE_KEYFIELD( m_iMaxEnts, FIELD_INTEGER, "MaxEnts" ),

	DEFINE_KEYFIELD( m_bTranslateSchedules, FIELD_BOOLEAN, "TranslateSchedules" ),

	DEFINE_KEYFIELD( m_flDistanceFromHint, FIELD_FLOAT, "HintDistance" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UpdateActors", InputPopulateNPCs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Test", InputTest ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "TestNPC", InputTestNPC ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "GetConditionName", InputGetConditionName ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "GetScheduleName", InputGetScheduleName ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetCondition", InputSetCondition ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearCondition", InputClearCondition ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearAllConditions", InputClearAllConditions ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetSchedule", InputSetSchedule ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearSchedule", InputClearSchedule ),
	DEFINE_INPUTFUNC( FIELD_STRING, "ClearAllSchedules", InputClearAllSchedules ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHint", InputSetHint ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ClearHint", InputClearHint ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ClearAllHints", InputClearAllHints ),

	// Outputs
	DEFINE_OUTPUT(m_OutConditionName, "OutConditionName"),
	DEFINE_OUTPUT(m_OutScheduleName, "OutScheduleName"),
	DEFINE_OUTPUT(m_OnNPCHasCondition, "OnNPCHasCondition"),
	DEFINE_OUTPUT(m_OnNPCLacksCondition, "OnNPCLacksCondition"),
	DEFINE_OUTPUT(m_OnNPCRunningSchedule, "OnNPCRunningSchedule"),
	DEFINE_OUTPUT(m_OnNPCNotRunningSchedule, "OnNPCNotRunningSchedule"),
	DEFINE_OUTPUT(m_OnNPCUsingHint, "OnNPCUsingHint"),
	DEFINE_OUTPUT(m_OnNPCNotUsingHint, "OnNPCNotUsingHint"),

	DEFINE_THINKFUNC( MonitorThink ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_Monitor::CAI_Monitor()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Monitor::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Monitor::Activate( void )
{
	BaseClass::Activate();

	if (!m_bStartDisabled)
	{
		SetThink(&CAI_Monitor::MonitorThink);
		SetNextThink(gpGlobals->curtime + GetThinkTime());
	}

	PopulateNPCs(NULL);
}

//-----------------------------------------------------------------------------
// Enable, disable
//-----------------------------------------------------------------------------
void CAI_Monitor::InputEnable( inputdata_t &inputdata )
{
	PopulateNPCs(&inputdata);

	SetThink( &CAI_Monitor::MonitorThink );
	SetNextThink( gpGlobals->curtime + GetThinkTime() );
}

void CAI_Monitor::InputDisable( inputdata_t &inputdata )
{
	SetThink( NULL );
}

void CAI_Monitor::InputPopulateNPCs( inputdata_t &inputdata )
{
	PopulateNPCs(&inputdata);
}

void CAI_Monitor::InputTest( inputdata_t &inputdata )
{
	bool bFoundResults = false;
	for (int i = 0; i < pNPCs.Count(); i++)
	{
		if (pNPCs[i] != NULL)
		{
			if (!bFoundResults)
				bFoundResults = NPCDoEval(pNPCs[i]);
			else if (!m_bCooldownAtFirstSuccess)
				NPCDoEval(pNPCs[i]);
			else
				break;
		}
		else
		{
			// If we have a null NPC, we should probably update.
			// This could probably go wrong in more than one way...
			PopulateNPCs(NULL);
			i--;
		}
	}
}

void CAI_Monitor::InputTestNPC( inputdata_t &inputdata )
{
	CAI_BaseNPC *pNPC = inputdata.value.Entity()->MyNPCPointer();
	if (!inputdata.value.Entity() || !pNPC)
		return;

	NPCDoEval(pNPC);
}

//-----------------------------------------------------------------------------
// Purpose: Save/restore stuff from CAI_BaseNPC
//-----------------------------------------------------------------------------
int CAI_Monitor::Save( ISave &save )
{
#if !AI_MONITOR_USE_UTLVECTOR
	save.StartBlock();
	SaveConditions( save, m_Conditions );
	SaveConditions( save, m_Schedules );
	save.EndBlock();
#endif

	return BaseClass::Save(save);
}

int CAI_Monitor::Restore( IRestore &restore )
{
#if !AI_MONITOR_USE_UTLVECTOR
	restore.StartBlock();
	RestoreConditions( restore, &m_Conditions );
	RestoreConditions( restore, &m_Schedules );
	restore.EndBlock();
#endif

	return BaseClass::Restore(restore);
}

#if !AI_MONITOR_USE_UTLVECTOR
void CAI_Monitor::SaveConditions( ISave &save, const CAI_ScheduleBits &conditions )
{
	for (int i = 0; i < MAX_CONDITIONS; i++)
	{
		if (conditions.IsBitSet(i))
		{
			const char *pszConditionName = ConditionName(AI_RemapToGlobal(i));
			if ( !pszConditionName )
				break;
			save.WriteString( pszConditionName );
		}
	}
	save.WriteString( "" );
}

//-------------------------------------

void CAI_Monitor::RestoreConditions( IRestore &restore, CAI_ScheduleBits *pConditions )
{
	pConditions->ClearAll();
	char szCondition[256];
	for (;;)
	{
		restore.ReadString( szCondition, sizeof(szCondition), 0 );
		if ( !szCondition[0] )
			break;
		int iCondition = GetConditionID( szCondition );
		if ( iCondition != -1 )
			pConditions->Set( AI_RemapFromGlobal( iCondition ) );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Monitor::PopulateNPCs(inputdata_t *inputdata)
{
	// pNPCs[i] != NULL && !pNPCs[i]->IsMarkedForDeletion() && !pNPCs[i]->GetState() != NPC_STATE_DEAD
	//pNPCs = CUtlVector<CHandle<CAI_BaseNPC>>();
	pNPCs.RemoveAll();

	CBaseEntity *pActivator = inputdata ? inputdata->pActivator : NULL;
	CBaseEntity *pCaller = inputdata ? inputdata->pCaller : NULL;

	CBaseEntity *pEnt = gEntList.FindEntityGeneric(NULL, STRING(m_target), this, pActivator, pCaller);
	while (pEnt)
	{
		if (pEnt->IsNPC())
		{
			pNPCs.AddToTail(pEnt->MyNPCPointer());
			DevMsg("Added %s to element %i\n", pEnt->GetDebugName(), pNPCs.Count());

			// 0 = no limit because the list would already have at least one element by the time this is checked.
			if (pNPCs.Count() == m_iMaxEnts)
				break;
		}

		pEnt = gEntList.FindEntityGeneric(pEnt, STRING(m_target), this, pActivator, pCaller);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_BaseNPC *CAI_Monitor::GetFirstTarget()
{
	for (int i = 0; i < pNPCs.Count(); i++)
	{
		if (pNPCs[i] != NULL)
			return pNPCs[i];
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAI_Monitor::NPCDoEval(CAI_BaseNPC *pNPC)
{
	// Because we return based on this
	m_OnNPCHasCondition.Init(0);
	m_OnNPCRunningSchedule.Init(0);
	m_OnNPCUsingHint.Init(0);


	// ----------
	// Conditions
	// ----------
#if AI_MONITOR_USE_UTLVECTOR
	for (int cond = 0; cond < m_Conditions.Count(); cond++)
#else
	for (int cond = 0; cond < m_Conditions.GetNumBits(); cond++)
#endif
	{
		if (pNPC->HasCondition(m_Conditions[cond]))
		{
			DevMsg("NPC has condition %i, index %i, name %s\n", m_Conditions[cond], cond, ConditionName(m_Conditions[cond]));
			m_OnNPCHasCondition.Set(m_Conditions[cond], pNPC, this);
			m_OutConditionName.Set(MAKE_STRING(ConditionName(m_Conditions[cond])), pNPC, this);
		}
		else
		{
			DevMsg("NPC does not have condition %i, index %i, name %s\n", m_Conditions[cond], cond, ConditionName(m_Conditions[cond]));
			m_OnNPCLacksCondition.Set(m_Conditions[cond], pNPC, this);
			m_OutConditionName.Set(MAKE_STRING(ConditionName(m_Conditions[cond])), pNPC, this);
		}
		/*
		bool bDecisive = false;
		switch (m_ConditionsOp)
		{
			case AIMONITOR_CONDITIONAL_NOR:
				bConditionsTrue = true;
			case AIMONITOR_CONDITIONAL_OR:
			{
				if (pNPC->HasCondition(m_Conditions[cond]))
				{
					// One is valid, pass conditions
					bConditionsTrue = !bConditionsTrue;
					bDecisive = true;
					break;
				}
			} break;
			case AIMONITOR_CONDITIONAL_NAND:
				bConditionsTrue = true;
			case AIMONITOR_CONDITIONAL_AND:
			{
				if (!pNPCs[i]->HasCondition(m_Conditions[cond]))
				{
					// One is invalid, don't pass conditions
					bConditionsTrue = !bConditionsTrue;
					bDecisive = true;
					break;
				}
			} break;
		}

		if (bDecisive)
			break;
		*/
	}

	// ----------
	// Schedules
	// ----------
#if AI_MONITOR_USE_UTLVECTOR
	for (int sched = 0; sched < m_Schedules.Count(); sched++)
#else
	for (int sched = 0; sched < m_Schedules.GetNumBits(); sched++)
#endif
	{
		if (pNPC->IsCurSchedule(m_bTranslateSchedules ? pNPC->TranslateSchedule(m_Schedules[sched]) : m_Schedules[sched]))
		{
			DevMsg("NPC is running schedule %i, index %i, name %s\n", m_Schedules[sched], sched, ScheduleName(m_Schedules[sched]));
			m_OnNPCRunningSchedule.Set(m_Schedules[sched], pNPC, this);
			m_OutScheduleName.Set(AllocPooledString(ScheduleName(m_Schedules[sched])), pNPC, this);
		}
		else
		{
			DevMsg("NPC is not running schedule %i, index %i, name %s\n", m_Schedules[sched], sched, ScheduleName(m_Schedules[sched]));
			m_OnNPCNotRunningSchedule.Set(m_Schedules[sched], pNPC, this);
			m_OutScheduleName.Set(AllocPooledString(ScheduleName(m_Schedules[sched])), pNPC, this);
		}
	}

	// ----------
	// Hints
	// ----------
	CAI_Hint *pHint = pNPC->GetHintNode();
	if (m_Hints.Count() > 0)
	{
		if (!pHint || (m_flDistanceFromHint > 0 && pHint->GetLocalOrigin().DistTo(pNPC->GetLocalOrigin()) > m_flDistanceFromHint))
		{
			for (int hint = 0; hint < m_Hints.Count(); hint++)
			{
				m_OnNPCNotUsingHint.Set(m_Hints[hint], pNPC, this);
			}
		}
		else
		{
			for (int hint = 0; hint < m_Hints.Count(); hint++)
			{
				if (pHint->HintType() == m_Hints[hint])
				{
					DevMsg("NPC is using hint %i, index %i\n", m_Hints[hint], hint);
					m_OnNPCUsingHint.Set(m_Hints[hint], pNPC, this);
				}
				else
				{
					DevMsg("NPC is not using hint %i, index %i\n", m_Hints[hint], hint);
					m_OnNPCNotUsingHint.Set(m_Hints[hint], pNPC, this);
				}
			}
		}
	}


	// Return whether any of our "valid" outputs fired.
	return (m_OnNPCHasCondition.Get() != 0
	||		m_OnNPCRunningSchedule.Get() != 0
	||		m_OnNPCUsingHint.Get() != 0
			);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Monitor::MonitorThink()
{
	bool bMonitorFoundResults = false;
	for (int i = 0; i < pNPCs.Count(); i++)
	{
		if (pNPCs[i] != NULL)
		{
			if (!bMonitorFoundResults)
				bMonitorFoundResults = NPCDoEval(pNPCs[i]);
			else if (!m_bCooldownAtFirstSuccess)
				NPCDoEval(pNPCs[i]);
			else
				break;
		}
		else
		{
			// If we have a null NPC, we should probably update.
			// This could probably go wrong in more than one way...
			PopulateNPCs(NULL);
			i--;
		}
	}

	if (bMonitorFoundResults)
		SetNextThink(gpGlobals->curtime + GetCooldownTime());
	else
		SetNextThink(gpGlobals->curtime + GetThinkTime());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CAI_Monitor::ConditionName(int conditionID)
{
	if ( AI_IdIsLocal( conditionID ) )
	{
		// Get our first target and find a local condition
		CAI_BaseNPC *pTarget = GetFirstTarget();
		if (!pTarget)
			return NULL;

		conditionID = ConditionLocalToGlobal(pTarget, conditionID);
	}

	return CAI_BaseNPC::GetSchedulingSymbols()->ConditionIdToSymbol(conditionID);
}

const char *CAI_Monitor::ScheduleName(int scheduleID)
{
	if ( AI_IdIsLocal( scheduleID ) )
	{
		// Get our first target and find a local condition
		CAI_BaseNPC *pTarget = GetFirstTarget();
		if (!pTarget)
			return NULL;

		scheduleID = ScheduleLocalToGlobal(pTarget, scheduleID);
	}

	return CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol(scheduleID);
}

int CAI_Monitor::TranslateConditionString(const char *condName)
{
	if (condName[0] == 'C')
	{
		// String
		int cond = GetConditionID(condName);
		if (cond > -1)
		{
			DevMsg("Setting condition %i from %s\n", cond, condName);
			return cond;
		}
	}
	else
	{
		// Int
		DevMsg("Setting condition %s\n", condName);

		// Assume the mapper didn't compensate for global ID stuff.
		// (as if either of us understand it)
		return atoi(condName) + GLOBAL_IDS_BASE;
	}
	return 0;
}

int CAI_Monitor::TranslateScheduleString(const char *schedName)
{
	if (schedName[0] == 'S')
	{
		// String
		int sched = GetScheduleID(schedName);
		if (sched > -1)
		{
			DevMsg("Setting schedule %i from %s\n", sched, schedName);
			return sched;
		}
	}
	else
	{
		// Int
		DevMsg("Setting schedule %s\n", schedName);
		return atoi(schedName);
	}
	return 0;
}

template<typename Translator>
static void SetForEachDelimited( CAI_Monitor &monitor, const char *szValue, const char *delimiters, void (CAI_Monitor::*setter)(int), Translator translator)
{
	char *value = strdup(szValue);
	char *token = strtok(value, ":");
	while (token)
	{
		(monitor.*setter)(translator(token));

		token = strtok(NULL, ":");
	}
	free(value);
}

template<int (CAI_Monitor::*translator)(const char*)>
struct CAI_MonitorTranslator
{
	CAI_Monitor &monitor;

	CAI_MonitorTranslator(CAI_Monitor &monitor) : monitor(monitor) {}

	int operator()(const char *value)
	{
		return (monitor.*translator)(value);
	}
};

//-----------------------------------------------------------------------------
// Purpose: Cache user entity field values until spawn is called.
// Input  : szKeyName - Key to handle.
//			szValue - Value for key.
// Output : Returns true if the key was handled, false if not.
//-----------------------------------------------------------------------------
bool CAI_Monitor::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "ConditionsSimple"))
	{
		// Hammer SmartEdit helper that shouldn't be overridden.
		// It's not supposed to be overridden.
		SetCondition(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "Conditions"))
	{
		SetForEachDelimited(*this, szValue, ":", &CAI_Monitor::SetCondition, CAI_MonitorTranslator<&CAI_Monitor::TranslateConditionString>(*this));
	}
	else if (FStrEq(szKeyName, "SchedulesSimple"))
	{
		// Hammer SmartEdit helper that shouldn't be overridden.
		SetCondition(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "Schedules"))
	{
		SetForEachDelimited(*this, szValue, ":", &CAI_Monitor::SetSchedule, CAI_MonitorTranslator<&CAI_Monitor::TranslateScheduleString>(*this));
	}
	else if (FStrEq(szKeyName, "HintsSimple"))
	{
		// Hammer SmartEdit helper that shouldn't be overridden.
		SetHint(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "Hints"))
	{
		SetForEachDelimited(*this, szValue, ":", &CAI_Monitor::SetHint, atoi);
	}
	else
		return CBaseEntity::KeyValue( szKeyName, szValue );

	return true;
}
