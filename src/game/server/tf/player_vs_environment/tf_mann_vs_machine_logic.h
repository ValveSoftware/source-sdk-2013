//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_mann_vs_machine_logic.h
// Mann Vs Machine game mode singleton manager
// Michael Booth, June 2011

#ifndef TF_MANN_VS_MACHINE_LOGIC_H
#define TF_MANN_VS_MACHINE_LOGIC_H

#include "tf_gamerules.h"

class CTFBotActionPoint;


//-----------------------------------------------------------------------
class CMannVsMachineLogic : public CPointEntity
{
	DECLARE_CLASS( CMannVsMachineLogic, CPointEntity );
public:
	DECLARE_DATADESC();

	CMannVsMachineLogic();
	virtual ~CMannVsMachineLogic();

	virtual void Spawn( void );
	void Update( void );

	void SetupOnRoundStart( void );

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

private:
	CHandle< CPopulationManager > m_populationManager;
	void InitPopulationManager( void );

	float m_flNextAlarmCheck;
};

extern CHandle<CMannVsMachineLogic> g_hMannVsMachineLogic;

#endif // TF_MANN_VS_MACHINE_LOGIC_H
