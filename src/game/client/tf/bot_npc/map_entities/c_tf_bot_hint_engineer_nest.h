//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_BOT_HINT_ENGINEER_NEST_H
#define TF_BOT_HINT_ENGINEER_NEST_H

#include "c_baseentity.h"

class C_TFBotHintEngineerNest : public C_BaseEntity
{
	DECLARE_CLASS( C_TFBotHintEngineerNest, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_TFBotHintEngineerNest( void );
	virtual ~C_TFBotHintEngineerNest();

	virtual void UpdateOnRemove() OVERRIDE;
	virtual void OnPreDataChanged( DataUpdateType_t type ) OVERRIDE;
	virtual void OnDataChanged( DataUpdateType_t type ) OVERRIDE;
private:
	bool m_bHadActiveTeleporter;
	CNetworkVar( bool, m_bHasActiveTeleporter );

	void StartEffect();
	void StopEffect();
	CNewParticleEffect			*m_pMvMActiveTeleporter;
};

#endif // TF_BOT_HINT_ENGINEER_NEST_H
