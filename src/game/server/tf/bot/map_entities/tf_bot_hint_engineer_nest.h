//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_BOT_HINT_ENGINEER_NEST_H
#define TF_BOT_HINT_ENGINEER_NEST_H

#include "tf_bot_hint_entity.h"

typedef CUtlVector< CHandle< CBaseTFBotHintEntity > > HintVector_t;

class CTFBotHintSentrygun;
class CTFBotHintTeleporterExit;

class CTFBotHintEngineerNest : public CBaseTFBotHintEntity
{
	DECLARE_CLASS( CTFBotHintEngineerNest, CBaseTFBotHintEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFBotHintEngineerNest( void );
	virtual ~CTFBotHintEngineerNest() { }

	virtual void Spawn() OVERRIDE;

	virtual HintType GetHintType() const OVERRIDE { return HINT_ENGINEER_NEST; }

	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

	void HintThink();
	void HintTeleporterThink();

	bool IsStaleNest() const;
	void DetonateStaleNest();

	CTFBotHintSentrygun* GetSentryHint() const;
	CTFBotHintTeleporterExit* GetTeleporterHint() const;
private:
	void DetonateObjectsFromHints( const HintVector_t& hints );
	CBaseTFBotHintEntity* GetHint( const HintVector_t& hints ) const;

	HintVector_t m_sentries;
	HintVector_t m_teleporters;

	CNetworkVar( bool, m_bHasActiveTeleporter );
};

#endif // TF_BOT_HINT_ENGINEER_NEST_H
