//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_hint_sentrygun.cpp
// Designer-placed hint for bot sentry placement
// Michael Booth, October 2009

#include "cbase.h"
#include "bot/tf_bot.h"
#include "tf_bot_hint_sentrygun.h"


BEGIN_DATADESC( CTFBotHintSentrygun )
	DEFINE_KEYFIELD( m_isSticky, FIELD_BOOLEAN, "sticky" ),
	DEFINE_OUTPUT( m_outputOnSentryGunDestroyed, "OnSentryGunDestroyed" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_hint_sentrygun, CTFBotHintSentrygun );

//------------------------------------------------------------------------------
CTFBotHintSentrygun::CTFBotHintSentrygun( void )
	: m_isSticky( false )
	, m_iUseCount( 0 )
{
}

//------------------------------------------------------------------------------
void CTFBotHintSentrygun::OnSentryGunDestroyed( CBaseEntity *pEntity )
{
	m_outputOnSentryGunDestroyed.FireOutput( pEntity, pEntity );
}

//------------------------------------------------------------------------------
bool CTFBotHintSentrygun::IsAvailableForSelection( CTFPlayer *pRequestingPlayer ) const
{
	// sentry hint is eligible as long as there is no owner (or the owner is no longer an engineer)
	// if the hint is enabled and the hint is not in use and it is on the same team as me
	if ( ( GetPlayerOwner() == NULL || !GetPlayerOwner()->IsPlayerClass( TF_CLASS_ENGINEER ) ) && 
		 ( IsEnabled() && IsInUse() == false && InSameTeam( pRequestingPlayer ) ) )
	{
		return true;
	}
	return false;
}
