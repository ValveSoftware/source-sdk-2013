//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_DISCONNECT_PROMPT_H
#define TF_HUD_DISCONNECT_PROMPT_H
#ifdef _WIN32
#pragma once
#endif

#include "confirm_dialog.h"
#include "tf_gc_client.h"

enum eDisconnectReason
{
	REASON_QUIT,
	REASON_DISCONNECT
};

class CTFDisconnectConfirmDialog : public CTFGenericConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFDisconnectConfirmDialog, CTFGenericConfirmDialog );
public:
	CTFDisconnectConfirmDialog(	const char *pTitle, 
								const char *pTextKey, 
								const char *pConfirmBtnText,
								const char *pCancelBtnText, 
								GenericConfirmDialogCallback callback, 
								vgui::Panel *pParent );

	virtual const char *GetResFile();

	void SetReason( eDisconnectReason reason );
	virtual void OnCommand( const char *command );
	void AddConfirmCommand( const char *command );
	void AddCancelCommand( const char *command );

private:
	CUtlVector< CUtlString > m_confirmCommands;
	CUtlVector< CUtlString > m_cancelCommands;

	eDisconnectReason m_eReason;
	EAbandonGameStatus m_eAbandonStatus;

};
CTFDisconnectConfirmDialog * BuildDisconnectConfirmDialog();


class CTFRejoinConfirmDialog : public CTFGenericConfirmDialog
{
	DECLARE_CLASS_SIMPLE( CTFRejoinConfirmDialog, CTFGenericConfirmDialog );
public:
	CTFRejoinConfirmDialog(	const char *pTitle, 
		const char *pTextKey, 
		const char *pConfirmBtnText,
		const char *pCancelBtnText, 
		GenericConfirmDialogCallback callback, 
		vgui::Panel *pParent );

	virtual const char *GetResFile();

	void CloseRejoinWindow();

private:

	EAbandonGameStatus m_eAbandonStatus;

};
CTFRejoinConfirmDialog * BuildRejoinConfirmDialog();

bool HandleDisconnectAttempt();

#endif // TF_HUD_DISCONNECT_PROMPT_H
