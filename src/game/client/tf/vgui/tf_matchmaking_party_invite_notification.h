//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_PARTY_INVITE_NOTIFICATION_H
#define TF_MATCHMAKING_PARTY_INVITE_NOTIFICATION_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_party.h"
#include "tf_matchmaking_dashboard_notification.h"
#include "econ_notifications.h"

typedef std::tuple< CSteamID, CTFParty::EPendingType, bool > InviteKey_t;

//
// A party invite notification that creates a vgui panel on the matchmaking dashboard as well as an econ notification
// so users in-game can see the invite as well.
//
class CInviteNotification : public CTFDashboardNotification
						  , public CEconNotification
{
	DECLARE_CLASS_SIMPLE( CInviteNotification, CTFDashboardNotification );
public:
	CInviteNotification( CSteamID steamID, CTFParty::EPendingType eType, bool bIncoming );
	virtual ~CInviteNotification();

	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnExpire() OVERRIDE;
	virtual float GetInGameLifeTime() const OVERRIDE;

	virtual EType NotificationType() OVERRIDE	{ return eType_AcceptDecline; }
	virtual void Accept() OVERRIDE				{ Action( true ); }
	virtual void Decline() OVERRIDE				{ Action( false ); }

	void Action( bool bConfirmed );
	
	InviteKey_t GetKey() const;

private:

	class CAvatarImagePanel* m_pAvatar;
	CTFParty::EPendingType m_eType;
	bool m_bIncoming;
};

#endif // TF_MATCHMAKING_PARTY_INVITE_NOTIFICATION_H
