//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef ECON_NOTIFICATIONS_H
#define ECON_NOTIFICATIONS_H
#ifdef _WIN32
#pragma once
#endif

// forward declarations
class KeyValues;
namespace vgui
{
	class EditablePanel;
};
class CEconNotificationQueue;

/**
 * Base class for notifications, but is generic enough to use
 */
class CEconNotification
{
public:
	CEconNotification();
	virtual ~CEconNotification();

	void SetText( const char *pText );
	void AddStringToken( const char* pToken, const wchar_t* pValue );

	void SetKeyValues( KeyValues *pKeyValues );
	KeyValues *GetKeyValues() const;

	const char* GetUnlocalizedText() { return m_pText; }
	const wchar_t *GetText();
	int GetID() const;

	virtual void SetLifetime( float flSeconds );
	virtual float GetExpireTime() const;

	virtual float GetInGameLifeTime() const;
	
	void SetIsInUse( bool bInUse );
	bool GetIsInUse() const;

	void SetSteamID( const CSteamID &steamID );
	const CSteamID &GetSteamID() const;

	virtual bool BShowInGameElements() const { return true; }
	bool BCreateMainMenuPanel() const { return m_bCreateMainMenuPanel; }

	virtual void MarkForDeletion();

	enum EType {
		// Can only be deleted
		eType_Basic,
		// Can be accept or declined
		eType_AcceptDecline,
		// Can be triggered or deleted
		eType_Trigger,
		// Can only be triggered
		eType_MustTrigger,
	};

	virtual EType NotificationType();

	// Is this an important/high priority notification.  Triggers higher visibility etc..
	virtual bool BHighPriority();

	// eType_Trigger or eType_MustTrigger
	virtual void Trigger();

	// eType_AcceptDecline
	virtual void Accept();
	virtual void Decline();

	// eType_Basic or eType_Trigger
	virtual void Deleted();

	// All types, if expire time is set
	virtual void Expired();



	virtual void UpdateTick() { }

	virtual const char *GetUnlocalizedHelpText();

	virtual vgui::EditablePanel *CreateUIElement( bool bMainMenu ) const;

	void SetSoundFilename( const char *filename )
	{
		m_pSoundFilename = filename;
	}

	const char *GetSoundFilename() const
	{
		if ( m_pSoundFilename )
		{
			return m_pSoundFilename;
		}

		return "ui/notification_alert.wav";
	}

	int GetKVVersion() const { return m_iKVVersion; }

protected:
	const char *m_pText;
	const char *m_pSoundFilename;
	float m_flExpireTime;
	KeyValues *m_pKeyValues;
	wchar_t m_wszBuffer[1024];
	CSteamID m_steamID;
	bool m_bCreateMainMenuPanel = true;

private:
	friend class CEconNotificationQueue;
	int m_iID;
	bool m_bInUse;
	int m_iKVVersion = 0;
};



/**
 * Filter function for CEconNotification's, used to remove them
 * @return true if the notification matches, false otherwise
 */
typedef bool (*NotificationFilterFunc)( CEconNotification *pNotification );

/**
 * Visitor object for notifications.
 */
class CEconNotificationVisitor
{
public:
	virtual void Visit( CEconNotification &notification ) = 0;
};

/**
 * Adds the notification to the notification queue
 * @param pNotification
 * @return id to retrieve the notification later if necessary
 */
int NotificationQueue_Add( CEconNotification *pNotification );

/**
 * Retrieves a notification by ID
 * @param iID id of the notification
 * @return the CEconNotification, NULL if not found
 */
CEconNotification *NotificationQueue_Get( int iID );

/**
 * Retrieves a notification by index
 * @param idx Index of the notification relative to GetNumNotifications
 * @return the CEconNotification, NULL if not found
 */
CEconNotification *NotificationQueue_GetByIndex( int idx );

/**
 * Removes all notifications from the queue and deletes them
 * @param iID
 */
void NotificationQueue_RemoveAll();

/**
 * Removes the notification from the queue and deletes it
 * @param iID
 */
void NotificationQueue_Remove( int iID );

/**
 * Removes the notification from the queue and deletes it
 * @param pNotification
 */
void NotificationQueue_Remove( CEconNotification *pNotification );

/**
 * Removes notifications that pass the specified filter
 * @param func
 */
void NotificationQueue_Remove( NotificationFilterFunc func );

/**
 * Count up how many notifications of the given kind are already in the queue
 * @param func
 */
int NotificationQueue_Count( NotificationFilterFunc func );

/**
 * The visitor object will "visit" each notification and perform any work necessary.
 * @param visitor object
 */
void NotificationQueue_Visit( CEconNotificationVisitor &visitor );

/**
 * Update the notification queue
 */
void NotificationQueue_Update();

/*
*  @return the number of notifications that should show on the main menu
*/
int NotificationQueue_GetNumMainMenuNotifications();

/**
 * @return the number of notifications
 */
int NotificationQueue_GetNumNotifications();

/**
 * Create the main menu ui element
 * @param pParent
 * @param pElementName
 * @return the control that was created
 */
vgui::EditablePanel* NotificationQueue_CreateMainMenuUIElement( vgui::EditablePanel *pParent, const char *pElementName );

#endif // endif
