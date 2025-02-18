//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_boss_battle_logic.h
// Boss battle game mode singleton manager
// Michael Booth, April 2011

#ifndef TF_BOSS_BATTLE_H
#define TF_BOSS_BATTLE_H

#ifdef TF_RAID_MODE

#include "tf_gamerules.h"

class CTFBotActionPoint;


//-----------------------------------------------------------------------
class CBossBattleLogic : public CPointEntity, public CGameEventListener
{
	DECLARE_CLASS( CBossBattleLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CBossBattleLogic();
	virtual ~CBossBattleLogic();

	virtual void Spawn( void );
	void Reset( void );
	void Update( void );

	void OnRoundStart( void );

	virtual void FireGameEvent( IGameEvent *event );

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}
};

extern CBossBattleLogic *g_pBossBattleLogic;

#endif // TF_RAID_MODE


#endif // TF_BOSS_BATTLE_H
