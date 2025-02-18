//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Enables sending of notifications (custom messages of various kinds) to the client
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_NOTIFICATION_H
#define TF_NOTIFICATION_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"


//---------------------------------------------------------------------------------
// Purpose: Send a notification to the client
//---------------------------------------------------------------------------------

// LOCALIZED NOTIFICATIONS
//
// Some types of notification are on-the-fly localized. These notifications are stored in the database as unlocalized
// strings (#TF_Foo), but on-the-fly localized when loaded as a shared-object.  Clients only see the final localized
// version. BLocalizeAndMaybeDirty() will update the localization for a newer language.

class CTFNotification : public GCSDK::CProtoBufSharedObject< CMsgGCNotification, k_EEconTypeNotification >
{
public:
	// If using this form, ensure you call BLocalize after filling fields for localized types.

};

#endif // TF_NOTIFICATION_H
