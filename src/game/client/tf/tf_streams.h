//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//			
//=============================================================================

#ifndef TF_STREAMS_H
#define TF_STREAMS_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"

using namespace vgui;

#define MAX_STREAM_PANELS 5

class CStreamInfo
{
public:
	CStreamInfo() : m_numViewers( 0 ) {}
public:
	CUtlString m_sGlobalName;
	int m_numViewers;
	CUtlString m_sTextDescription;
	CUtlString m_sUpdatedAtStamp;
	CUtlString m_sPreviewImage;
	CUtlString m_sPreviewImageLocalFile;
	CUtlString m_sPreviewImageSF;
};

class CTFStreamPanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFStreamPanel, EditablePanel );

	CTFStreamPanel( Panel *parent, const char *panelName );
	virtual ~CTFStreamPanel() {}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	void SetGlobalName( const char *pszGlobalName ) { m_strStreamInfoGlobalName = pszGlobalName; }
	CStreamInfo *GetStreamInfo() const;

private:
	void UpdatePanels();
	void SetPreviewImage( const char *pszPreviewImageFile );

	CUtlString m_strStreamInfoGlobalName;

	class vgui::ImagePanel *m_pPreviewImage;
};

class CTFStreamListPanel : public EditablePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFStreamListPanel, EditablePanel );

	CTFStreamListPanel( Panel *parent, const char *panelName );
	virtual ~CTFStreamListPanel() {}

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	CTFStreamPanel *m_arrStreamPanels[MAX_STREAM_PANELS];
};

enum ETwitchTvState_t
{
	k_ETwitchTvState_None,
	k_ETwitchTvState_Loading,
	k_ETwitchTvState_NoLink,
	k_ETwitchTvState_Linked,
	k_ETwitchTvState_Error
};

struct TwitchTvAccountInfo_t
{
	uint64 m_uiSteamID;

	ETwitchTvState_t m_eTwitchTvState;
	double m_dblTimeStampTwitchTvUpdate;
	uint64 m_uiTwitchTvUserId;
	CUtlString m_sTwitchTvChannel;
};

class CTFStreamManager : public CAutoGameSystemPerFrame
{
public:
	CTFStreamManager();
	~CTFStreamManager();

	virtual bool Init() OVERRIDE;
	virtual void Update( float frametime ) OVERRIDE;
	void RequestTopStreams();

	CStreamInfo * GetStreamInfoByName( char const *szName );

	CUtlVector< CStreamInfo >& GetStreamInfoVec() { return m_streamInfoVec; }

	TwitchTvAccountInfo_t* GetTwitchTvAccountInfo( uint64 uiSteamID );

private: // cache stream info
	CUtlVector< CStreamInfo > m_streamInfoVec;

private: // query for top X viewers
	double m_dblTimeStampLastUpdate;
	HTTPRequestHandle m_hHTTPRequestHandle;
	CCallResult< CTFStreamManager, HTTPRequestCompleted_t > m_CallbackOnHTTPRequestCompleted;
	void Steam_OnHTTPRequestCompletedStreams( HTTPRequestCompleted_t *p, bool bError );

private: // checking for twitch account linking with steam account
	void UpdateTwitchTvAccounts();

	TwitchTvAccountInfo_t *m_pLoadingAccount;
	CUtlVector< TwitchTvAccountInfo_t* > m_vecTwitchTvAccounts; // list of steam id to updated twitch account
	HTTPRequestHandle m_hHTTPRequestHandleTwitchTv;
	CCallResult< CTFStreamManager, HTTPRequestCompleted_t > m_CallbackOnHTTPRequestCompletedTwitchTv;
	void Steam_OnHTTPRequestCompletedMyTwitchTv( HTTPRequestCompleted_t *p, bool bError );
};
CTFStreamManager *StreamManager();

#endif // TF_STREAMS_H
