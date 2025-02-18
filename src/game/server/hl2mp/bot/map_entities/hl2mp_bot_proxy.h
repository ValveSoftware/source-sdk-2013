//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_PROXY_H
#define HL2MP_BOT_PROXY_H


class CHL2MPBot;
class CHL2MPBotActionPoint;


class CHL2MPBotProxy : public CPointEntity
{
	DECLARE_CLASS( CHL2MPBotProxy, CPointEntity );
public:
	DECLARE_DATADESC();

	CHL2MPBotProxy( void );
	virtual ~CHL2MPBotProxy() { }

	void Think( void );

	// Input
	void InputSetTeam( inputdata_t &inputdata );
	void InputSetMovementGoal( inputdata_t &inputdata );
	void InputSpawn( inputdata_t &inputdata );
	void InputDelete( inputdata_t &inputdata );

	void OnInjured( void );
	void OnKilled( void );
	void OnAttackingEnemy( void );
	void OnKilledEnemy( void );

protected:
	// Output
	COutputEvent m_onSpawned;
	COutputEvent m_onInjured;
	COutputEvent m_onKilled;
	COutputEvent m_onAttackingEnemy;
	COutputEvent m_onKilledEnemy;

	char m_botName[64];
	char m_teamName[64];

	string_t m_spawnOnStart;
	string_t m_actionPointName;
	float m_respawnInterval;

	CHandle< CHL2MPBot > m_bot;
	CHandle< CHL2MPBotActionPoint > m_moveGoal;
};


#endif // HL2MP_BOT_PROXY_H
