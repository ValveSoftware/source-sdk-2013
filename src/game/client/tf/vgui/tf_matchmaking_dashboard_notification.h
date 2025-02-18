//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_NOTIFICATION_H
#define TF_MATCHMAKING_DASHBOARD_NOTIFICATION_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

//
// The base dashboard notification
//
class CTFDashboardNotification : public vgui::EditablePanel
{
public:
	enum ENotificationType
	{
		TYPE_CHAT = 0,
		TYPE_PARTY_INVITE,
		TYPE_LOBBY_INVITE,
	};

	enum EAlignment
	{
		LEFT = 0,
		CENTER,
		RIGHT,
		NUM_ALIGNMENTS,
	};

	DECLARE_CLASS_SIMPLE( CTFDashboardNotification, vgui::EditablePanel );
	CTFDashboardNotification( ENotificationType eType,
							  EAlignment eAlignment,
							  float flLifetime, // 0 for infinite
							  const char* pszName );
	virtual ~CTFDashboardNotification();

	void SetToExpire( float flDelay = 1.f );
	ENotificationType GetType() const { return m_eType; }
	EAlignment GetAlignment() const { return m_eAlignment; }
	float GetCreationTime() const { return m_flCreationTime; }
	virtual int GetYMargin() const { return 0; }
private:
	MESSAGE_FUNC( OnExpire, "Expire" );
	ENotificationType m_eType;
	EAlignment m_eAlignment;
	float m_flCreationTime;
};

#endif // TF_MATCHMAKING_DASHBOARD_NOTIFICATION_H
