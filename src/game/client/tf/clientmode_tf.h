//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_CLIENTMODE_H
#define TF_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tf_viewport.h"
#include "GameUI/IGameUI.h"
#include "halloween/tf_weapon_spellbook.h"
#include "tf_hud_teamgoal_tournament.h"

class CHudMenuEngyBuild;
class CHudMenuEngyDestroy;
class CHudMenuSpyDisguise;
class CTFFreezePanel;
class CItemQuickSwitchPanel;
class CHudEurekaEffectTeleportMenu;
class CHudMenuTauntSelection;
class CHudInspectPanel;
class CHudUpgradePanel;
#if defined( _X360 )
class CTFClientScoreBoardDialog;
#endif

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual void	LevelInit( const char *newmap ) OVERRIDE;

//	virtual int		KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();
	virtual bool	ShouldDrawCrosshair( void );
	virtual bool	ShouldBlackoutAroundHUD() OVERRIDE;
	virtual HeadtrackMovementMode_t ShouldOverrideHeadtrackControl() OVERRIDE;

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui();

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *cmd );

	virtual int		HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
	virtual int		HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	virtual bool	DoPostScreenSpaceEffects( const CViewSetup *pSetup );
	virtual void	Update();
	virtual void	ComputeVguiResConditions( KeyValues *pkvConditions ) OVERRIDE;

	virtual bool	IsInfoPanelAllowed() OVERRIDE;
	virtual void	InfoPanelDisplayed() OVERRIDE;
	virtual bool	IsHTMLInfoPanelAllowed() OVERRIDE;

	IGameUI			*GameUI( void ) { return m_pGameUI; }

	const char		*GetLastConnectedServerName( void ) const;		// return the name of the last server we have connected to
	uint32			GetLastConnectedServerIP( void ) const;			// return the IP of the last server we have connected to
	int				GetLastConnectedServerPort( void ) const;		// return the port of the last server we have connected to

	void			PrintTextToChat( const char *pText, KeyValues *pKeyValues = NULL );
	void			PrintTextToChatPlayer( int iPlayerIndex, const char *pText, KeyValues *pKeyValues = NULL );

#if !defined(NO_STEAM)
	STEAM_CALLBACK_MANUAL( ClientModeTFNormal, OnScreenshotRequested, ScreenshotRequested_t, m_CallbackScreenshotRequested );
#endif

	bool IsEngyBuildVisible() const;
	bool IsEngyDestroyVisible() const;
	bool IsEngyEurekaTeleportVisible() const;
	bool IsSpyDisguiseVisible() const;
	bool IsUpgradePanelVisible() const;
	bool IsTauntSelectPanelVisible() const;

	void UpdateSteamRichPresence() const;
	// Given a client state, match group loc token and pretty map name, build a localized status line.
	// These are equivalent to 'state', 'matchgrouploc' and 'currentmap' rich presence keys from a player.
	static bool BuildRichPresenceStatusDirect( wchar_t *pwzOutStatus, size_t uOutSizeBytes,
	                                           const char *pszState,
	                                           const char *pszMatchGroupLocToken, const char *pszPrettyMapName );
	// Safe version
	template < size_t maxLenInChars >
	static inline bool BuildRichPresenceStatus( OUT_Z_ARRAY wchar_t (&pwzOutStatus)[maxLenInChars],
	                                            const char *pszState,
	                                            const char *pszMatchGroupLocToken, const char *pszPrettyMapName )
	{
		return BuildRichPresenceStatusDirect( pwzOutStatus, maxLenInChars, pszState,
		                                      pszMatchGroupLocToken, pszPrettyMapName );
	}

	virtual void OnDemoRecordStart( char const* pDemoBaseName ) OVERRIDE;
	virtual void OnDemoRecordStop() OVERRIDE;

	bool BIsFriendOrPartyMember( C_TFPlayer *pPlayer );

private:
	virtual bool BCanSendPartyChatMessages() const OVERRIDE;
	//	void	UpdateSpectatorMode( void );

private:
	CHudMenuEngyBuild		*m_pMenuEngyBuild;
	CHudMenuEngyDestroy 	*m_pMenuEngyDestroy;
	CHudMenuSpyDisguise 	*m_pMenuSpyDisguise;
	CHudMenuTauntSelection	*m_pMenuTauntSelection;
	CHudUpgradePanel		*m_pMenuUpgradePanel;
	CHudSpellMenu			*m_pMenuSpell;
	CHudEurekaEffectTeleportMenu *m_pEurekaTeleportMenu;
	CHudTeamGoalTournament	*m_pTeamGoalTournament;

	CTFFreezePanel			*m_pFreezePanel;
	CItemQuickSwitchPanel	*m_pQuickSwitch;
	CHudInspectPanel		*m_pInspectPanel;
	IGameUI					*m_pGameUI;
	bool					m_wasConnectedLastUpdate;

	char					*m_lastServerName;
	uint32					m_lastServerIP;
	int						m_lastServerPort;
	uint32					m_lastServerConnectTime;

	enum EConnectState {
		k_eConnectState_Disconnected,
		k_eConnectState_Connecting,
		k_eConnectState_Connected,
	};
	EConnectState			m_eConnectState           = k_eConnectState_Disconnected;
	// Valid only when m_eConnectState >= k_eConnectState_Connected
	// This is the base name of a map, and doesn't include workshop decorations/path/etc.
	char					m_szMapBaseName[MAX_MAP_NAME] = { 0 };

	float					m_flNextAllowedHighFiveHintTime;

	// When game events should trigger updates, we want to let all other systems think first (e.g. partyclient) as their
	// state is looked at by the update loop.  Setting this triggers an update on next think.
	bool					m_bPendingRichPresenceUpdate = false;
	bool					m_bInfoPanelShown;
	bool					m_bRestrictInfoPanel;

	void					AskFavoriteOrBlacklist() const;
	void					RemoveFilesInPath( const char *pszPath ) const;

#if defined( _X360 )
	CTFClientScoreBoardDialog	*m_pScoreboard;
#endif
};

inline const char *ClientModeTFNormal::GetLastConnectedServerName( void ) const
{
	return m_lastServerName;
}

inline uint32 ClientModeTFNormal::GetLastConnectedServerIP( void ) const
{
	return m_lastServerIP;
}

inline int ClientModeTFNormal::GetLastConnectedServerPort( void ) const
{
	return m_lastServerPort;
}

extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

void PlayOutOfGameSound( const char *pszSound );
float PlaySoundEntry( const char* pszSoundEntryName ); // Returns the duration of the sound
#endif // TF_CLIENTMODE_H
