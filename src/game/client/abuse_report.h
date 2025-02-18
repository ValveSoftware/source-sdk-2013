//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#ifndef ABUSE_REPORT_H
#define ABUSE_REPORT_H
#ifdef _WIN32
#pragma once
#endif

#include <igamesystem.h>
#include <GameEventListener.h>
#include <bitmap/bitmap.h>
#include <netadr.h>

/// Different content types that can be reported as abusive.
///
/// WARNING: These enum values MUST MATCH the values in Steam's
/// ECommunityContentType!
enum EAbuseReportContentType
{
	k_EAbuseReportContentNoSelection = -1, // dummy ilegal value: the user has not made a selection
	k_EAbuseReportContentUnspecified = 0, // we use this to mean "other"
	//k_EAbuseReportContentAll = 1,			// reset all community content
	k_EAbuseReportContentAvatarImage = 2,  // clear avatar image
	//k_EAbuseReportContentProfileText = 3,  // reset profile text
	//k_EAbuseReportContentWebLinks = 4,		// delete web links
	//k_EAbuseReportContentAnnouncement = 5,
	//k_EAbuseReportContentEventText = 6,
	//k_EAbuseReportContentCustomCSS = 7,
	//k_EAbuseReportContentProfileURL = 8,	// delete community URL ID
	k_EAbuseReportContentComments = 9,		// just comments this guy has written
	k_EAbuseReportContentPersonaName = 10,	// persona name
	//k_EAbuseReportContentScreenshot = 11,  // screenshot
	//k_EAbuseReportContentVideo = 12,		// videos
	k_EAbuseReportContentCheating = 13,	// cheating
	k_EAbuseReportContentUGCImage = 14,	// Image stored in UGC --- the report is accusing the image of being offensive
	k_EAbuseReportContentActorUGCImage = 15,	// Abuse report actor has uploaded a UGC image to server as supporting documentation of their claim
};


/// Types of reasons why a violation report was issued
///
/// WARNING: These enum values MUST MATCH the values in Steam's
/// EAbuseReportType!
enum EAbuseReportType
{
	k_EAbuseReportTypeNoSelection = -1, // dummy ilegal value: the user has not made a selection
	k_EAbuseReportTypeUnspecified = 0,
	k_EAbuseReportTypeInappropriate = 1,	// just not ok to post
	k_EAbuseReportTypeProhibited = 2,		// prohibited by EULA or general law
	k_EAbuseReportTypeSpamming = 3,		// excessive spamming
	k_EAbuseReportTypeAdvertisement = 4,	// unwanted advertisement
	//k_EAbuseReportTypeExploit = 5,		// content data attempts to exploit code issue
	k_EAbuseReportTypeSpoofing = 6,		// user/group is impersonating an official contact
	k_EAbuseReportTypeLanguage = 7,		// bad language
	k_EAbuseReportTypeAdultContent = 8,	// any kind of adult material, references etc
	k_EAbuseReportTypeHarassment = 9,		// harassment, discrimination, racism etc
	k_EAbuseReportTypeCheating = 10,		// cheating
};

/// Container class that has everything we need to know in order to file
/// an abuse report, which is significantly more than the data we actually
/// include in a particular abuse report.  It's everything we save off at the
/// time the user initiates the abuse reporting mechanism.  Games can derive
/// their own report types and put game-specific data in here.
struct AbuseIncidentData_t
{
	AbuseIncidentData_t();
	virtual ~AbuseIncidentData_t();

	enum EPlayerImageType
	{
		k_PlayerImageType_UGC,
		k_PlayerImageType_Spray,
	};

	/// A custom image of the player's that could be considered offensive
	struct PlayerImage_t
	{

		/// What kind of image is it?
		EPlayerImageType m_eType;

		/// For UGC images, what's the handle?
		uint64 m_hUGCHandle;
	};

	/// Info we remember for one player.
	struct PlayerData_t
	{
		PlayerData_t()
		{
			m_iClientIndex = -1;
			m_iSteamAvatarIndex = -1;
		}

		/// The client index.  (See UTIL_PlayerByIndex).  Note that this
		/// index is really only valid at the time the incident is captured.
		/// Because players can leave after the incident is captured.
		int m_iClientIndex;

		/// The name they were going by at the time
		CUtlString m_sPersona;

		/// Their steam ID.  This is essential so we can file
		/// an abuse report!
		CSteamID m_steamID;

		/// Index of steam friends icon for their avatar.
		/// 0 if they don't have one!
		int m_iSteamAvatarIndex;

		/// Do we have an entity for this player?  They might not have spawned,
		/// or might be outside our PVS, etc.
		bool m_bHasEntity;

		/// Model transform for the player's render stuff
		VMatrix m_matModelToWorld;

		/// Model->clip matrix for the player's render stuff
		VMatrix m_matModelToClip;

		/// True if the render bounds are approximately correct, false if not
		bool m_bRenderBoundsValid;

		/// Bounds (in model space) of the player's renderable stuff
		Vector m_vecRenderBoundsMin, m_vecRenderBoundsMax;

		/// Bounds (in normalized screen space coords 0...1) of the player's
		/// renderable stuff
		Vector2D m_screenBoundsMin, m_screenBoundsMax;

		/// List of his images
		CUtlVector<PlayerImage_t> m_vecImages;
	};

	/// List of base player data.  You got more data per player in your derived
	/// incident type?  Store it in a parallel array.
	CUtlVector<PlayerData_t> m_vecPlayers;

	/// Camera world -> clip matrix.
	VMatrix m_matWorldToClip;

	/// Screenshot
	Bitmap_t m_bitmapScreenshot;

	// Screenshot file data
	CUtlBuffer m_bufScreenshotFileData;

	/// Number of frames we're willing to wait for the engine to write out a screenshot.
	/// Zero if we already failed
	int m_nScreenShotWaitFrames;

	/// Is it possible to report the game server itself for abuse?
	bool m_bCanReportGameServer;

	/// What Game Server/IP are we on? Will be an invalid address if we don't know
	netadr_t m_adrGameServer;

	/// Steam ID of the game server / IP we are on
	CSteamID m_steamIDGameServer;

	/// Poll report (some data may have to be fetched asynchronously),
	/// return true if everything is ready
	virtual bool Poll();
};

/// Generic abuse reporting panel.  Your
class CAbuseReportManager : public CBaseGameSystemPerFrame, public CGameEventListener
{
public:
	CAbuseReportManager();
	virtual ~CAbuseReportManager();

	//
	// CAutoGameSystemPerFrame overrides
	//
	virtual char const *Name();
	virtual bool Init();
	virtual void Shutdown();
	virtual void LevelShutdownPreEntity();

	//
	// CGameEventListener overrides
	//
	virtual void FireGameEvent( IGameEvent *event );

// CAutoGameSystemPerFrame defines different stuff depending on which DLL we're building
#ifdef CLIENT_DLL

	// Do our frame-time processing
	virtual void Update( float frametime );

#else
	#error "Why is this being included?"
#endif

	/// Called when the console command is executed to capture data for a report
	virtual void QueueReport();

	/// Called when the console command is executed to submit data for a report
	virtual void SubmitReportUIRequested();

	/// Called to actually trigger the report UI, after all data is ready
	virtual void ActivateSubmitReportUI() = 0;

	/// Fetch the incident that's queued to be reported
	AbuseIncidentData_t *GetIncidentData() const { return m_pIncidentData; }

	/// Delete any current report incident.  Also should clean
	/// out any temporary files used by the incident system.
	virtual void DestroyIncidentData();

	/// Show a message box complaining about lack of steam
	/// connection
	virtual void ShowNoSteamErrorMessage();

	/// Insert a a notification into the queue indicating that an unfiled report is ready
	virtual void CreateReportReadyNotification( bool bInGame, float flLifetime );

	/// Test harness.  Set this to true, to generate fake data
	bool m_bTestReport;

	static const char k_rchScreenShotFilenameBase[];
	static const char k_rchScreenShotFilename[];

protected:

	/// Your app will probably define its own abuse report types.
	/// if so, you will need to override this function.
	/// The base class just calls new to create an object, then calls
	/// PopulateIncident()
	virtual bool CreateAndPopulateIncident();

	/// Fill in the details about the current incident.  This just fills in the
	/// base class data, and it should be called from CreateAndPopulateIncident
	bool PopulateIncident();

	/// Current incident that is pending to be reported or is being generated.
	/// Might be NULL.
	AbuseIncidentData_t *m_pIncidentData;

	/// Status of incident data.
	enum EIncidentDataStatus
	{
		k_EIncidentDataStatus_None,
		k_EIncidentDataStatus_Preparing, // we shuld call Poll() until it's ready
		k_EIncidentDataStatus_Ready,	 // it's ready
	};
	EIncidentDataStatus m_eIncidentDataStatus;

	/// Do we want to show the report UI as soon as the report is ready?
	bool m_bReportUIPending;

	void CheckCreateReportReadyNotification( float flMinSecondsSinceLastNotification, bool bInGame, float flLifetime );

	/// Time when we last pestered them about filing their report
	double m_timeLastReportReadyNotification;

	/// Address of the lasts server we connected to
	netadr_t m_adrCurrentServer;
	CSteamID m_steamIDCurrentServer;

};

/// Pointer to the app-specific instance.  This pointer mght be NULL!  Your
/// app should define set this pointer if it uses the system
extern CAbuseReportManager *g_AbuseReportMgr;

#endif	// ABUSE_REPORT_H
