//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_hint_sentrygun.h
// Designer-placed hint for bot sentry placement
// Michael Booth, October 2009

#ifndef TF_BOT_HINT_SENTRYGUN_H
#define TF_BOT_HINT_SENTRYGUN_H

#include "tf_bot_hint_entity.h"

class CTFPlayer;

class CTFBotHintSentrygun : public CBaseTFBotHintEntity
{
public:
	DECLARE_CLASS( CTFBotHintSentrygun, CBaseTFBotHintEntity );
	DECLARE_DATADESC();

	CTFBotHintSentrygun( void );
	virtual ~CTFBotHintSentrygun() { }

	bool IsSticky() const;
	bool IsInUse() const;

	CTFPlayer *GetPlayerOwner() const;
	void SetPlayerOwner( CTFPlayer *pPlayerOwner );

	void IncrementUseCount();
	void DecrementUseCount();

	void OnSentryGunDestroyed( CBaseEntity *pBaseEntity );

	bool IsAvailableForSelection( CTFPlayer *pRequestingPlayer ) const;

	virtual HintType GetHintType() const OVERRIDE { return HINT_SENTRYGUN; }

private:
	bool m_isSticky;
	int m_iUseCount;
	COutputEvent m_outputOnSentryGunDestroyed;

	CHandle< CTFPlayer > m_playerOwner;
};

inline bool CTFBotHintSentrygun::IsSticky() const
{
	return m_isSticky;
}

inline bool CTFBotHintSentrygun::IsInUse() const
{
	return m_iUseCount != 0;
}

inline CTFPlayer *CTFBotHintSentrygun::GetPlayerOwner() const
{
	return m_playerOwner;
}

inline void CTFBotHintSentrygun::SetPlayerOwner( CTFPlayer *pPlayerOwner )
{
	m_playerOwner = pPlayerOwner;
}

inline void CTFBotHintSentrygun::IncrementUseCount()
{
	++m_iUseCount;
}

inline void CTFBotHintSentrygun::DecrementUseCount()
{
	--m_iUseCount;
}

#endif // TF_BOT_HINT_SENTRYGUN_H
