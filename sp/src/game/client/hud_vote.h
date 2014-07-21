//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_VOTE_H
#define HUD_VOTE_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <networkstringtabledefs.h>

extern INetworkStringTable *g_pStringTableServerMapCycle;

#ifdef TF_CLIENT_DLL
extern INetworkStringTable *g_pStringTableServerPopFiles;
extern INetworkStringTable *g_pStringTableServerMapCycleMvM;
#endif

namespace vgui
{
	class SectionedListPanel;
	class ComboBox;
	class ImageList;
};

class VoteBarPanel : public vgui::Panel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( VoteBarPanel, vgui::Panel );

	VoteBarPanel( vgui::Panel *parent, const char *panelName );

	virtual void Paint( void );
	virtual void FireGameEvent( IGameEvent *event );

private:
	int m_nVoteOptionCount[MAX_VOTE_OPTIONS];	// Vote options counter
	int m_nPotentialVotes;						// If set, draw a line at this point to show the required bar length

	CPanelAnimationVarAliasType( int, m_iBoxSize, "box_size", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpacer, "spacer", "4", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBoxInset, "box_inset", "1", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nYesTextureId, "yes_texture", "vgui/hud/vote_yes", "textureid" );
	CPanelAnimationVarAliasType( int, m_nNoTextureId, "no_texture", "vgui/hud/vote_no", "textureid" );
};

//-----------------------------------------------------------------------------
// Purpose: A selection UI for votes that require additional parameters - such as players, maps
//-----------------------------------------------------------------------------

class CVoteSetupDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CVoteSetupDialog, vgui::Frame ); 

public:
	CVoteSetupDialog( vgui::Panel *parent );
	~CVoteSetupDialog();

	virtual void	Activate();
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PostApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	ApplySettings(KeyValues *inResourceData);

	void			UpdateCurrentMap( void );
	void			AddVoteIssues( CUtlStringList &m_VoteSetupIssues );
	void			AddVoteIssueParams_MapCycle( CUtlStringList &m_VoteSetupMapCycle );

#ifdef TF_CLIENT_DLL
	void			AddVoteIssueParams_PopFiles( CUtlStringList &m_VoteSetupPopFiles );
#endif

private:
	//MESSAGE_FUNC( OnItemSelected, "ItemSelected" );
	MESSAGE_FUNC_PTR( OnItemSelected, "ItemSelected", panel );

	virtual void	OnCommand( const char *command );
	virtual void	OnClose( void );

	void			RefreshIssueParameters( void );
	void			ResetData( void );

	vgui::ComboBox				*m_pComboBox;

	vgui::SectionedListPanel	*m_pVoteSetupList;
	vgui::SectionedListPanel	*m_pVoteParameterList;
	vgui::Button				*m_pCallVoteButton;
	vgui::ImageList				*m_pImageList;

	CUtlVector<const char*>	m_VoteIssues;
	CUtlVector<const char*>	m_VoteIssuesMapCycle;

#ifdef TF_CLIENT_DLL
	CUtlVector<const char*>	m_VoteIssuesPopFiles;
#endif

	CPanelAnimationVarAliasType( int, m_iIssueWidth, "issue_width", "100", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iParameterWidth, "parameter_width", "150", "proportional_int" );

	bool			m_bVoteButtonEnabled;
	char			m_szCurrentMap[MAX_MAP_NAME];

	vgui::HFont		m_hHeaderFont;
	Color			m_HeaderFGColor;
	vgui::HFont		m_hIssueFont;
	Color			m_IssueFGColor;
	Color			m_IssueFGColorDisabled;
};


class CHudVote : public vgui::EditablePanel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudVote, vgui::EditablePanel );

	CHudVote( const char *pElementName );

	virtual void	LevelInit( void );
	virtual void	Init( void );
	virtual bool	ShouldDraw( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	OnThink();
	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	// NOTE: Any MsgFunc_*() methods added here need to check IsPlayingDemo().
	void			MsgFunc_CallVoteFailed( bf_read &msg );
	void			MsgFunc_VoteStart( bf_read &msg );
	void			MsgFunc_VotePass( bf_read &msg );
	void			MsgFunc_VoteFailed( bf_read &msg );
	void			MsgFunc_VoteSetup( bf_read &msg );

	void			PropagateOptionParameters( void );
	void			ShowVoteUI( void );
	bool			IsVoteUIActive( void );

private:
	bool			IsPlayingDemo() const;
	void			SetVoteActive( bool bActive );

	EditablePanel		*m_pVoteActive;
	VoteBarPanel		*m_voteBar;
	EditablePanel		*m_pVoteFailed;
	EditablePanel		*m_pVotePassed;
	EditablePanel		*m_pCallVoteFailed;
	CVoteSetupDialog	*m_pVoteSetupDialog;

	CUtlStringList		m_VoteSetupIssues;
	CUtlStringList		m_VoteSetupMapCycle;
	
#ifdef TF_CLIENT_DLL
	CUtlStringList		m_VoteSetupPopFiles;
#endif

	CUtlStringList		m_VoteSetupChoices;

	bool				m_bVoteActive;
	float				m_flVoteResultCycleTime;	// what time will we cycle to the result
	float				m_flHideTime;				// what time will we hide
	bool				m_bVotePassed;				// what mode are we going to cycle to
	int					m_nVoteOptionCount[MAX_VOTE_OPTIONS];	// Vote options counter
	int					m_nPotentialVotes;						// If set, draw a line at this point to show the required bar length
	bool				m_bIsYesNoVote;
	int					m_nVoteChoicesCount;
	bool				m_bPlayerVoted;
	float				m_flPostVotedHideTime;
	bool				m_bShowVoteActivePanel;
	int					m_iVoteCallerIdx;
};

#endif // HUD_VOTE_H
