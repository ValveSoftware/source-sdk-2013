//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface for the client to acknowledge/view notifications sent from the GC
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_NOTIFICATIONS_H
#define C_TF_NOTIFICATIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "econ/econ_notifications.h"
#include "tf_notification.h"


class CClientNotification : public CEconNotification
{
	friend class CTFSupportNotificationDialog;
public:
	CClientNotification();
	virtual ~CClientNotification() OVERRIDE;

	virtual EType NotificationType() OVERRIDE;
	virtual void Deleted() OVERRIDE;
	virtual void Expired() OVERRIDE;
	virtual void Trigger() OVERRIDE;

	virtual bool BHighPriority() OVERRIDE;

	// Should show up on the main menu only -- these go away on dismissal
	virtual bool BShowInGameElements() const OVERRIDE { return false; }

	void Update( const CTFNotification* notification );
	uint64 NotificationID() const { return m_ulNotificationID; }

private:
	void OnDialogAcknowledged();
	void GCAcknowledge();

	uint64 m_ulNotificationID;
	uint32 m_unAccountID;
	// m_pText sometimes points to a static string we don't own, so this guy owns any text that we do.
	CUtlString m_strText;

	// Is this a support message? If so, the user must trigger the notification to view the message in a pop-up before they can dismiss.
	bool   m_bSupportMessage;

};

#endif // C_TF_NOTIFICATIONS_H
