//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_notification.h"
#include "c_tf_notification.h"
#include "tf_gc_client.h"
#include "econ/econ_notifications.h"

///
/// Support message notification dialog
///

class CTFSupportNotificationDialog : public CTFMessageBoxDialog
{
	DECLARE_CLASS_SIMPLE( CTFSupportNotificationDialog, CTFMessageBoxDialog );
public:
	CTFSupportNotificationDialog( int iNotificationID, const char *pszSupportMessage )
		: CTFMessageBoxDialog( NULL, pszSupportMessage, NULL, NULL, NULL )
		, m_iNotificationID( iNotificationID )
		, m_pConfirmDialog( NULL )
	{
		SetDialogVariable( "text", GetText() );
	}

	void CleanupConfirmDialog()
	{
		if ( m_pConfirmDialog )
		{
			m_pConfirmDialog->SetVisible( false );
			m_pConfirmDialog->MarkForDeletion();
			m_pConfirmDialog = NULL;
		}
	}

	virtual ~CTFSupportNotificationDialog() {
		CleanupConfirmDialog();
	}

	void ConfirmDialogCallback( bool bConfirmed )
	{
		CleanupConfirmDialog();

		if ( bConfirmed )
		{
			// User acknowledged the message, tell the notification it can go away now
			CClientNotification *pNotification = dynamic_cast< CClientNotification * >( NotificationQueue_Get( m_iNotificationID ) );
			if ( pNotification )
			{
				pNotification->OnDialogAcknowledged();
			}

			BaseClass::OnCommand( "confirm" );
		}
	}

	static void StaticConfirmDialogCallback( bool bConfirmed, void *pContext )
	{
		static_cast< CTFSupportNotificationDialog * >( pContext )->ConfirmDialogCallback( bConfirmed );
	}

	virtual void OnCommand( const char *command ) OVERRIDE
	{
		if ( FStrEq( "acknowledge", command ) )
		{
			// Confirm this, it's going away forever!
			CleanupConfirmDialog();
			m_pConfirmDialog = ShowConfirmDialog( "#DeleteConfirmDefault",
			                                      "#TF_Support_Message_Confirm_Acknowledge_Text",
			                                      "#TF_Support_Message_Acknowledge", "#Cancel",
			                                      &StaticConfirmDialogCallback );
			m_pConfirmDialog->SetContext( this );
			return;
		}
		else if ( FStrEq( "show_later", command ) )
		{
			// User selected "show this later" -- leave notification as is and close.
			CleanupConfirmDialog();
			BaseClass::OnCommand( "confirm" );
			return;
		}

		BaseClass::OnCommand( command );
	}

	virtual const char *GetResFile() OVERRIDE
	{
		return "Resource/UI/SupportNotificationDialog.res";
	}

private:
	// Associated notification to clear
	int m_iNotificationID;
	CTFGenericConfirmDialog *m_pConfirmDialog;
};

///
/// The notification class
///

CClientNotification::CClientNotification()
{
	m_pText = NULL;
	m_flExpireTime = CRTime::RTime32TimeCur();
	m_ulNotificationID = 0;
	m_unAccountID = 0;
	m_bSupportMessage = false;
}

CClientNotification::~CClientNotification()
{}

void CClientNotification::Update( const CTFNotification* notification )
{
	// Custom type handling. For now only support message does anything special.
	m_bSupportMessage = false;
	switch ( notification->Obj().type() )
	{
		case CMsgGCNotification_NotificationType_NOTIFICATION_REPORTED_PLAYER_BANNED:
		case CMsgGCNotification_NotificationType_NOTIFICATION_CUSTOM_STRING:
		case CMsgGCNotification_NotificationType_NOTIFICATION_MM_BAN_DUE_TO_EXCESSIVE_REPORTS:
		case CMsgGCNotification_NotificationType_NOTIFICATION_REPORTED_PLAYER_WAS_BANNED:
			// All identical.
			//
			// Really, the other types could be used to avoid having to send a localization string down? Otherwise
			// they're all just redundant with CUSTOM_STRING for now.
			break;
		case CMsgGCNotification_NotificationType_NOTIFICATION_SUPPORT_MESSAGE:
			m_bSupportMessage = true;
			break;
		default:
			Assert( !"Unhandled enum value" );
	}

	m_pText = NULL;
	m_strText = notification->Obj().notification_string().c_str();

	if ( m_bSupportMessage )
	{
		// Use generic notification, save actual notification contents for dialog
		m_pText = "#TF_Support_Message_Notification";
	}
	else
	{
		// Just use our message
		m_pText = m_strText.Get();
	}

	// 0 -> does not expire
	RTime32 rtExpire = notification->Obj().expiration_time();
	m_flExpireTime = rtExpire > 0 ? (float)rtExpire : FLT_MAX;
	m_ulNotificationID = notification->Obj().notification_id();
	m_unAccountID = notification->Obj().account_id();

}

void CClientNotification::GCAcknowledge() {

	GTFGCClientSystem()->AcknowledgeNotification( m_unAccountID, m_ulNotificationID );
}

void CClientNotification::Deleted()
{
	if ( m_bSupportMessage )
	{
		AssertMsg( !m_bSupportMessage,
		           "Support messages should only be able to be triggered, not deleted" );
		return;
	}

	GCAcknowledge();
}

void CClientNotification::Expired()
{
	// No action, we don't want de-sync'd client clock to acknowledge these incorrectly, GC will expire them on its end.
}

CClientNotification::EType CClientNotification::NotificationType()
{
	// Support messages are "must trigger" type -- no delete action, user must click "view"
	if ( m_bSupportMessage )
	{
		return eType_MustTrigger;
	}

	return eType_Basic;
}

bool CClientNotification::BHighPriority()
{
	return m_bSupportMessage;
}

void CClientNotification::Trigger()
{
	if ( !m_bSupportMessage )
	{
		AssertMsg( m_bSupportMessage,
		           "Don't expect to be trigger-able when not in support message mode" );
		return;
	}

	CTFSupportNotificationDialog *pDialog = vgui::SETUP_PANEL( new CTFSupportNotificationDialog( GetID(), m_strText.Get() ) );
	pDialog->Show();
}

void CClientNotification::OnDialogAcknowledged()
{
	if ( !m_bSupportMessage )
	{
		AssertMsg( m_bSupportMessage,
		           "Don't expect to be getting callbacks from the support message dialog when not in support message mode" );
		return;
	}

	GCAcknowledge();
	MarkForDeletion();
}

