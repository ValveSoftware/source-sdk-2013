//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VOICE_STATUS_H
#define VOICE_STATUS_H
#pragma once


#include <vgui_controls/Label.h>
//#include "vgui_bitmap.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/Image.h>
#include "voice_common.h"
#include "voice_banmgr.h"
#include "hudelement.h"

#ifdef VOICE_VOX_ENABLE
extern ConVar voice_vox;
#endif // VOICE_VOX_ENABLE

class CVoiceStatus;
class IMaterial;
class BitmapImage;

// Voice Panel
class CVoicePanel : public vgui::Panel
{
public:
	CVoicePanel( );
	~CVoicePanel();

	virtual void Paint( void );
	virtual void setImage( BitmapImage *pImage );

private:
	BitmapImage *m_pImage;
};

class CVoiceLabel
{
public:
	vgui::Label			*m_pLabel;
	vgui::Label			*m_pBackground;
	CVoicePanel			*m_pIcon;		// Voice icon next to player name.
	int					m_clientindex;	// Client index of the speaker. -1 if this label isn't being used.
};

// This is provided by each mod to access data that may not be the same across mods.
abstract_class IVoiceStatusHelper
{
public:
	virtual					~IVoiceStatusHelper()	{}

	// Get RGB color for voice status text about this player.
	virtual void			GetPlayerTextColor(int entindex, int color[3]) = 0;

	// Force it to update the cursor state.
	virtual void			UpdateCursorState() = 0;

	// Return true if the voice manager is allowed to show speaker labels
	// (mods usually return false when the scoreboard is up).
	virtual bool			CanShowSpeakerLabels() = 0;
};

class CVoiceStatus /*: public vgui::CDefaultInputSignal*/
{
public:
				CVoiceStatus();
	virtual		~CVoiceStatus();

// CHudBase overrides.
public:
	
	// Initialize the cl_dll's voice manager.
	virtual int Init(
		IVoiceStatusHelper *m_pHelper,
		vgui::VPANEL pParentPanel);
	
	// ackPosition is the bottom position of where CVoiceStatus will draw the voice acknowledgement labels.
	virtual void VidInit();

public:
	
	// Call from HUD_Frame each frame.
	void	Frame(double frametime);

	// Called when a player starts or stops talking.
	// entindex is -1 to represent the local client talking (before the data comes back from the server). 
	// When the server acknowledges that the local client is talking, then entindex will be gEngfuncs.GetLocalPlayer().
	// entindex is -2 to represent the local client's voice being acked by the server.
	void	UpdateSpeakerStatus(int entindex, bool bTalking);

	// Call from the HUD_CreateEntities function so it can add sprites above player heads.
	void	DrawHeadLabels();
	void	SetHeadLabelOffset( float offset );
	float	GetHeadLabelOffset( void ) const;
	void	SetHeadLabelsDisabled( bool bDisabled ) { m_bHeadLabelsDisabled = bDisabled; }

	// Called when the server registers a change to who this client can hear.
	void	HandleVoiceMaskMsg(bf_read &msg);

	// The server sends this message initially to tell the client to send their state.
	void	HandleReqStateMsg(bf_read &msg);


// Squelch mode functions.
public:

	// When you enter squelch mode, pass in 
	void	StartSquelchMode();
	void	StopSquelchMode();
	bool	IsInSquelchMode();

	// returns true if the target client has been banned
	// playerIndex is of range 1..maxplayers
	bool	IsPlayerBlocked(int iPlayerIndex);

	// returns false if the player can't hear the other client due to game rules (eg. the other team)
	bool    IsPlayerAudible(int iPlayerIndex);

	// returns true if the player is currently speaking
	bool	IsPlayerSpeaking(int iPlayerIndex);

	// returns true if the local player is attempting to speak
	bool	IsLocalPlayerSpeaking( void );

	// blocks the target client from being heard
	void	SetPlayerBlockedState(int iPlayerIndex, bool blocked);

	void	SetHeadLabelMaterial( const char *pszMaterial );

	IMaterial *GetHeadLabelMaterial( void ) { return m_pHeadLabelMaterial; }

private:

	void			UpdateServerState(bool bForce);

	// Update the button artwork to reflect the client's current state.
	void			UpdateBanButton(int iClient);


private:
	float			m_LastUpdateServerState;		// Last time we called this function.
	int				m_bServerModEnable;				// What we've sent to the server about our "voice_modenable" cvar.

	vgui::VPANEL	m_pParentPanel;
	CPlayerBitVec	m_VoicePlayers;		// Who is currently talking. Indexed by client index.
	
	// This is the gamerules-defined list of players that you can hear. It is based on what teams people are on 
	// and is totally separate from the ban list. Indexed by client index.
	CPlayerBitVec	m_AudiblePlayers;

	// Players who have spoken at least once in the game so far
	CPlayerBitVec	m_VoiceEnabledPlayers;	

	// This is who the server THINKS we have banned (it can become incorrect when a new player arrives on the server).
	// It is checked periodically, and the server is told to squelch or unsquelch the appropriate players.
	CPlayerBitVec	m_ServerBannedPlayers;

	IVoiceStatusHelper	*m_pHelper;		// Each mod provides an implementation of this.

	// Squelch mode stuff.
	bool				m_bInSquelchMode;
	
	bool				m_bTalking;				// Set to true when the client thinks it's talking.
	bool				m_bServerAcked;			// Set to true when the server knows the client is talking.

public:
	
	CVoiceBanMgr		m_BanMgr;				// Tracks which users we have squelched and don't want to hear.

private:

	IMaterial			*m_pHeadLabelMaterial;	// For labels above players' heads.

	bool				m_bBanMgrInitialized;

	int					m_nControlSize;

	bool				m_bHeadLabelsDisabled;

#ifdef VOICE_VOX_ENABLE
	CountdownTimer		m_bAboveThresholdTimer;
#endif // VOICE_VOX_ENABLE
};


// Get the (global) voice manager. 
CVoiceStatus* GetClientVoiceMgr();
void ClientVoiceMgr_Init();
void ClientVoiceMgr_Shutdown();

#endif // VOICE_STATUS_H
