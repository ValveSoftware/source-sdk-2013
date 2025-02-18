//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef TF_DEMO_SUPPORT_H
#define TF_DEMO_SUPPORT_H

#define EVENTS_FILENAME "_events.txt"

enum EDemoEventType
{
	eDemoEvent_Bookmark,
	eDemoEvent_Killstreak,

	// also need to update g_aDemoEventNames
	eDemoEvent_Last,
};

struct DemoEvent_t
{
	EDemoEventType type;
	int value;
	int tick;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFDemoSupport : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CTFDemoSupport();

	virtual bool Init() OVERRIDE;
	virtual void Update( float frametime ) OVERRIDE;
	virtual char const *Name() OVERRIDE { return "CTFDemoSupport"; }
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void LevelInitPostEntity() OVERRIDE;
	virtual void LevelShutdownPostEntity() OVERRIDE;
	bool StartRecording( void );
	void StopRecording( bool bFromEngine = false );
	bool IsRecording( void ){ return m_bRecording; }
	void BookMarkCurrentTick( const char *pszValue = NULL );
	void Status( void );

private:
	bool IsValidPath( const char *pszFolder );
	void LogEvent( EDemoEventType eType, int nValue = 0, const char *pszValue = NULL );
	void Notify( char *pszMessage );

	bool m_bRecording;
	char m_szFolder[24];
	char m_szFilename[MAX_PATH];
	char m_szFolderAndFilename[MAX_PATH];
	int m_nKillCount;
	float m_flLastKill;
	float m_flScreenshotTime;
	FileHandle_t m_hGlobalEventList;
	GCSDK::CWebAPIResponse m_DemoSpecificEventList;
	GCSDK::CWebAPIValues *m_pRoot;
	GCSDK::CWebAPIValues *m_pChildArray;
	bool m_bAlreadyAutoRecordedOnce;
	float m_flNextRecordStartCheckTime;
	bool m_bFirstEvent;
	int m_nStartingTickCount;
	bool m_bHasAtLeastOneEvent;
};

#endif // TF_DEMO_SUPPORT_H
