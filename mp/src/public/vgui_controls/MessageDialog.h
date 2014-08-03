//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the CMessageDialog declaration
//
// $NoKeywords: $
//=============================================================================//

#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H
#ifdef _WIN32
#pragma once
#endif

// styles
#define MD_WARNING				0x0001
#define MD_ERROR				0x0002

// button configurations
#define MD_OK					0x0004	// 1 button - OK
#define MD_CANCEL				0x0008	// 1 button - CANCEL
#define MD_OKCANCEL				0x0010	// 2 buttons - OK and CANCEL
#define MD_YESNO				0x0020	// 2 buttons - YES and NO

// behavior
#define MD_SIMPLEFRAME			0x0100	// legacy corners
#define MD_COMMANDAFTERCLOSE	0x0200	// send command at dialog termination (i.e. after fade)
#define MD_RESTRICTPAINT		0x0400	// only paint this dialog (hide any other ui elements)
#define MD_COMMANDONFORCECLOSE	0x0800	// send command when the dialog is closed assuming A input

// dialog type
enum EDialogType
{
	MD_SAVE_BEFORE_QUIT,
	MD_QUIT_CONFIRMATION,
	MD_QUIT_CONFIRMATION_TF,
	MD_KICK_CONFIRMATION,
	MD_CLIENT_KICKED,
	MD_LOST_HOST,
	MD_LOST_SERVER,
	MD_SEARCHING_FOR_GAMES,
	MD_CREATING_GAME,
	MD_MODIFYING_SESSION,
	MD_SESSION_SEARCH_FAILED,
	MD_SESSION_CREATE_FAILED,
	MD_SESSION_CONNECTING,
	MD_SESSION_CONNECT_NOTAVAILABLE,
	MD_SESSION_CONNECT_SESSIONFULL,
	MD_SESSION_CONNECT_FAILED,
	MD_EXIT_SESSION_CONFIRMATION,
	MD_STORAGE_DEVICES_NEEDED,
	MD_STORAGE_DEVICES_CHANGED,
	MD_STORAGE_DEVICES_TOO_FULL,
	MD_NOT_ONLINE_ENABLED,
	MD_NOT_ONLINE_SIGNEDIN,
	MD_DEFAULT_CONTROLS_CONFIRM,
	MD_AUTOSAVE_EXPLANATION,
	MD_COMMENTARY_EXPLANATION,
	MD_COMMENTARY_EXPLANATION_MULTI,
	MD_COMMENTARY_CHAPTER_UNLOCK_EXPLANATION,
	MD_SAVE_BEFORE_LANGUAGE_CHANGE,
	MD_SAVE_BEFORE_NEW_GAME,
	MD_SAVE_BEFORE_LOAD,
	MD_DELETE_SAVE_CONFIRM,
	MD_SAVE_OVERWRITE,
	MD_SAVING_WARNING,
	MD_SAVE_COMPLETE,
	MD_STANDARD_SAMPLE,
	MD_WARNING_SAMPLE,
	MD_ERROR_SAMPLE,
	MD_PROMPT_SIGNIN,
	MD_PROMPT_SIGNIN_REQUIRED,
	MD_PROMPT_STORAGE_DEVICE,
	MD_PROMPT_STORAGE_DEVICE_REQUIRED,
	MD_DISCONNECT_CONFIRMATION,
	MD_DISCONNECT_CONFIRMATION_HOST,
	MD_LOAD_FAILED_WARNING,
	MD_OPTION_CHANGE_FROM_X360_DASHBOARD,
	MD_STORAGE_DEVICES_CORRUPT,
	MD_CHECKING_STORAGE_DEVICE
};

#include "vgui_controls/Frame.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/AnimatingImagePanel.h"
#include "vgui_controls/ImagePanel.h"

//-----------------------------------------------------------------------------
// Purpose: Simple modal dialog box for Xbox 360 warnings and messages
//-----------------------------------------------------------------------------
class CMessageDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CMessageDialog, vgui::Frame ); 

public:
	CMessageDialog( vgui::Panel *parent, const uint nType, const char *pTitle, const char *pMsg, const char *pCmdA, const char *pCmdB, vgui::Panel *pParent, bool bShowActivity );
	~CMessageDialog();

	enum
	{
		BTN_INVALID = -1,
		BTN_B,
		BTN_A,
		MAX_BUTTONS,
	};

	struct ButtonLabel_s
	{
		vgui::Label *pIcon;
		vgui::Label *pText;
		int			nWide;
		bool		bCreated;
	};

	virtual void		OnKeyCodePressed( vgui::KeyCode code );
	virtual void		ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void		ApplySettings( KeyValues *inResourceData );
	virtual void		PaintBackground();
	uint				GetType( void );
	void				SetControlSettingsKeys( KeyValues *pKeys );

private:	
	void				CreateButtonLabel( ButtonLabel_s *pButton, const char *pIcon, const char *pText );
	void				DoCommand( int button );

	vgui::Panel			*m_pCreator;

	vgui::Label			*m_pTitle;
	vgui::Label			*m_pMsg;
	vgui::ImagePanel	*m_pBackground;

	vgui::AnimatingImagePanel	*m_pAnimatingPanel;

	vgui::HFont			m_hButtonFont;
	vgui::HFont			m_hTextFont;
	uint				m_nType;
	Color				m_ButtonTextColor;
	int					m_ButtonPressed;
	KeyValues			*m_pControlSettings;

	int					m_FooterTall;
	int					m_ButtonMargin;
	Color				m_clrNotSimpleBG;
	Color				m_clrNotSimpleBGBlack;
	int					m_ButtonIconLabelSpace;

	int					m_ActivityIndent;

	bool				m_bShowActivity; // should we show an animating image panel?

	ButtonLabel_s		m_Buttons[MAX_BUTTONS];
	char				*m_pCommands[MAX_BUTTONS];
};

#endif	// MESSAGEDIALOG_H
