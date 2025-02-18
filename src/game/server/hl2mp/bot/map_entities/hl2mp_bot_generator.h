//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_GENERATOR_H
#define HL2MP_BOT_GENERATOR_H

class CHL2MPBotGenerator : public CPointEntity
{
public:
	DECLARE_CLASS( CHL2MPBotGenerator, CPointEntity );
	DECLARE_DATADESC();

	CHL2MPBotGenerator( void );
	virtual ~CHL2MPBotGenerator() { }

	virtual void Activate();

	void GeneratorThink( void );
	void SpawnBot( void );

	// Input.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetSuppressFire( inputdata_t &inputdata );
	void InputSetDisableDodge( inputdata_t &inputdata );
	void InputSetDifficulty( inputdata_t &inputdata );
	void InputCommandGotoActionPoint( inputdata_t &inputdata );
	void InputSetAttentionFocus( inputdata_t &inputdata );
	void InputClearAttentionFocus( inputdata_t &inputdata );
	void InputSpawnBot( inputdata_t &inputdata );
	void InputRemoveBots( inputdata_t &inputdata );

	// Output
	void OnBotKilled( CHL2MPBot *pBot );

private:
	bool m_bSuppressFire;
	bool m_bDisableDodge;
	bool m_bUseTeamSpawnpoint;
	bool m_bRetainBuildings;
	bool m_bExpended;
	int m_iOnDeathAction;
	int m_spawnCount;
	int m_spawnCountRemaining;
	int m_maxActiveCount;
	float m_spawnInterval;
	string_t m_teamName;
	string_t m_actionPointName;
	string_t m_initialCommand;
	CHandle< CBaseEntity > m_moveGoal;
	int m_difficulty;
	bool m_bSpawnOnlyWhenTriggered;
	bool m_bEnabled;

	COutputEvent m_onSpawned;
	COutputEvent m_onExpended;
	COutputEvent m_onBotKilled;

	CUtlVector< CHandle< CHL2MPBot > > m_spawnedBotVector;
};

//---------------------------------------------------------------
//
// Bot generator may have one of these as an argument, which
// means "tell the bot I created to move here and do what this node says".
// Things like "stay here", "move to <next task point>", "face towards <X>", "shoot at <Y>", etc
//
class CHL2MPBotActionPoint : public CPointEntity
{
	DECLARE_CLASS( CHL2MPBotActionPoint, CPointEntity );
public:
	DECLARE_DATADESC();

 	CHL2MPBotActionPoint( void );
 	virtual ~CHL2MPBotActionPoint() { }

	virtual void Activate();

	bool IsWithinRange( CBaseEntity *entity );
	void ReachedActionPoint( CHL2MPBot* pBot );

	CHandle< CBaseEntity > m_moveGoal;

	// reflected
	float m_stayTime;
	float m_desiredDistance;
	string_t m_nextActionPointName;
	string_t m_command;

	COutputEvent m_onReachedActionPoint;
};

inline HSCRIPT ToHScript( CHL2MPBotActionPoint *pPoint )
{
	return ( pPoint ) ? pPoint->GetScriptInstance() : NULL;
}

#endif // HL2MP_BOT_GENERATOR_H
