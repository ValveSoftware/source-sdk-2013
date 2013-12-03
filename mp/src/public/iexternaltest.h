//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: interface to external test DLLs
//
//=============================================================================

#ifndef IEXTERNALTEST
#define IEXTERNALTEST
#ifdef _WIN32
#pragma once
#endif

#include "steam/isteamfriends.h"
#include "clientenums.h"

// An equivalent to EConnectionPriority that can be used in
// external tests without needing to depend on Steam's version of the enum
enum EExternalTestConnectionPriority
{
	k_EExternalTestConnectionPriorityLow = 0,
	k_EExternalTestConnectionPriorityMedium = 1,
	k_EExternalTestConnectionPriorityHigh = 2,
};

// used to pass arbitrary buffers full of data back to the external test
typedef uint32 ExternalTestBuffer_t;
static const ExternalTestBuffer_t k_unExternalTestBufferInvalid = 0;


//-----------------------------------------------------------------------------
// Purpose: Represents a single test client (game client or game server) inside
//			of a test.
//-----------------------------------------------------------------------------
class IExternalTestClient
{
public:
	virtual ~IExternalTestClient() {}
	virtual void *GetISteamGenericInterface( const char *pchInterfaceVersion ) = 0;
	virtual ISteamUserStats *GetISteamUserStats( const char *pchVersion ) = 0;
	virtual ISteamGameServerStats *GetISteamGameServerStats( const char *pchVersion ) = 0;
	virtual CSteamID GetSteamID() = 0;
	virtual bool GetAccountName( char *pchBuf, int cchBuf ) = 0;

	virtual bool BLoggedOn() = 0;
	virtual void YieldingLogOff() = 0;
	virtual bool BYieldingLogOn() = 0;
	virtual void RequestAppInfoUpdate() = 0;
	virtual bool BGetAppInfoUpdateResult( EResult *peResult, uint32 *punAppsUpdated ) = 0;
	virtual void GetServerDetails( AppId_t *pnAppIdServed, uint32 *punIPGameServer, uint16 *pusPortGameServer, bool *pbSecure ) = 0;
	virtual bool BYieldingPrepareForApp( AppId_t unAppID ) = 0;
	virtual void SetConnectionPriority( EExternalTestConnectionPriority eConnectionPriority ) = 0;

	// methods to manipulate friend/social stuff that isn't in ISteamFriends
	virtual void SetPersonaState( EPersonaState ePersonaState ) = 0;
	virtual void AddFriend( const CSteamID & steamIDFriend ) = 0;
	virtual void RemoveFriend( const CSteamID & steamIDFriend ) = 0;

	// Removes every queued callback for this client
	virtual void ClearCallbacks() = 0;

	// Sets this user as an admin in this universe
	virtual void YieldingSetAsAdmin() = 0;

	// bans this user for the specified app ID
	virtual void YieldingBan( AppId_t unAppID, int nDeltaSeconds, bool bOneYearBan ) = 0;

	virtual bool BYieldingAwaitNewCallback( void *pCallback, uint32 unCallbackSize, int nCallbackType ) = 0;
	virtual bool BYieldingAwaitQueuedCallback( void * pCallback, uint32 unCallbackSize, int nCallbackType ) = 0;
	virtual bool BGetQueuedCallbackOfType( void * pCallback, uint32 unCallbackSize, int nCallbackType ) = 0;

	// Retrieves the current status of the API call. Returns false if the call is still in progress
	virtual bool BYieldingGetAPICallResult( SteamAPICall_t hSteamAPICall, void *pCallback, uint32 unCallbackSize, int nCallback, bool *pbAPICallFailed ) = 0;

	template< typename TCallback >
	inline bool BYieldingAwaitQueuedCallback( TCallback & callback ) { return BYieldingAwaitQueuedCallback( &callback, sizeof(TCallback), TCallback::k_iCallback ); }
	template< typename TCallback >
	inline bool BYieldingAwaitNewCallback( TCallback & callback ) { return BYieldingAwaitNewCallback( &callback, sizeof(TCallback), TCallback::k_iCallback ); }
	template< typename TCallback >
	inline bool BGetQueuedCallbackOfType( TCallback & callback ) { return BGetQueuedCallbackOfType( &callback, sizeof(TCallback), TCallback::k_iCallback ); }

	// more friends stuff that isnt in ISteamFriends
	virtual void SetIgnoreFriend( const CSteamID & steamIDFriend ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Helper functions for interacting with the steam client during tests
//-----------------------------------------------------------------------------
class IExternalTestUtils
{
public:
	virtual ~IExternalTestUtils() {}

	// creates a single user client that is logged into an optional gameserver
	virtual IExternalTestClient *YieldingCreateUserClient( IExternalTestClient *pGameserver ) = 0;

	// creates a single gameserver client at the specified (fake) IP addr and port
	virtual IExternalTestClient *YieldingCreateGameserverClient( uint32 unAddr, uint16 usPort ) = 0;

	// returns the ISteamClient object so the test can fetch other ISteam interfaces.
	virtual ISteamClient *SteamClient() = 0;

	// deploys the gctest dll at the current AppID. This is probably not useful outside of the 
	// gctest tests.
	virtual void YieldingDeployGC( ) = 0;

	// takes down the gctest dll at the current app DI
	virtual void YieldingDownGC( ) = 0;

	// returns the app ID this test is running as
	virtual AppId_t GetAppId() = 0;

	// draws a dot in the output
	virtual void DrawProgressDot() = 0;

	// spews some text in the output
	virtual void EmitTestOutput( const char *pchMessage ) = 0;

	// used by the external test framework to clean up some things between 
	// test cases. Test cases should not call this.
	virtual void YieldingResetBetweenTests() = 0;

	// Checks a bool condition and spews if it fails. If this fails the test
	// will be recorded as having failed.
	virtual bool BCheck( bool bCondition ) = 0;

	// Yields until the stress test is finished. If this test is running as a
	// regression test, this will return immediately.
	virtual void YieldingWaitForTestToFinish() = 0;

	// used by the external test framework to record test success/failure in 
	// a file that buildbot can use to build reports. Tests should not call this directly.
	virtual void UpdateProgressFile( const char *pchTestName, const char *pchStatus ) = 0;

	// Returns true if the test that ran most recently (and might still be running) passed.
	virtual bool BLastTestPassed() = 0;

	// Called by a test to report that the test has reached the "running" state. This is only
	// useful in stress tests.
	virtual void MarkTestRunning() = 0;

	// Logs on a bunch of clients all at once and returns when they are all logged on.
	virtual bool BYieldingCreateUserClientBatch( IExternalTestClient **prClients, int cClients, EExternalTestConnectionPriority eConnectionPriority ) = 0;

	// Logs on a bunch of gameservers all at once and returns when they are all logged on.
	virtual bool BYieldingCreateGameserverClientBatch( IExternalTestClient **prClients, int cClients, EExternalTestConnectionPriority eConnectionPriority ) = 0;

	// returns true if the test is finished
	virtual bool BIsStressTestFinished() = 0;

	// waits a single frame before resuming the test
	virtual void YieldingWaitOneFrame() = 0;

	// reports a stress test action success/failure
	virtual void ReportStressActionResult( uint32 unActionIndex, bool bResult, const char *pchFailureLocation ) = 0;

	// waits for the specified number of seconds to pass
	virtual void YieldingWaitForRealTime( uint32 unSeconds ) = 0;

	// deploys the specified zip file as the GC for the specified app ID
	virtual bool BYieldingGCDeploy( AppId_t appID, const char *pchGCZipFile ) = 0;

	// Downs the specified GC
	virtual bool BYieldingGCDown( AppId_t appID ) = 0;

	// Revives the specified GC
	virtual bool BYieldingGCRevive( AppId_t appID ) = 0;

	// Bounces the specified GC
	virtual bool BYieldingGCBounce( AppId_t appID ) = 0;

	// returns the universe the tests are running in
	virtual EUniverse GetUniverse() = 0;

	// returns the memory for a buffer
	virtual void *GetPvBuffer( ExternalTestBuffer_t unBuffer ) = 0;

	// returns the memory for a buffer
	virtual uint32 GetCubBuffer( ExternalTestBuffer_t unBuffer ) = 0;

	// makes a simulated WG request and returns the result
	virtual ExternalTestBuffer_t YieldingSimulateWGRequest( const CSteamID & actorID, const void *pvRequestKV, uint32 cubRequestKV ) = 0;

	// Verifies that there is no support ticket for the specified steam ID
	virtual void YieldingVerifyNoSupportTicket( const CSteamID & steamID ) = 0;

	// Verifies that there is a support event for the specified steam ID
	virtual void YieldingVerifySupportEventRecord( const CSteamID & victimID, const CSteamID & actorID, ESupportEvent eAction, GID_t gidTxn, const char *pchNote ) = 0;

	// forces an appinfo update for the specified app. (Pass invalid to use the default app ID
	virtual void YieldingForceAppInfoUpdate( AppId_t appId ) = 0;

	// returns the stats KV section serialized as a buffer
	virtual ExternalTestBuffer_t YieldingGetAppInfoStats( AppId_t appId ) = 0;

	// makes a web request and returns the result
	virtual bool BYieldingHTTPRequest( int nWebMethod, bool bUseSSL, const char *pchURL, const KeyValues *pkvPostParams, const char *pchAPIKey, const CSteamID & steamID, ExternalTestBuffer_t *punBuffer, int *pnResultCode ) = 0;

	// returns the full path to the DLL that these tests are running from
	virtual const char *GetPchDllPath() = 0;

	// returns true if these tests are running in dev mode (i.e. the server was started with -etdev)
	virtual bool BIsDevMode() = 0;

	// Sets the persona name for this user via the wg call so we can actually tell when
	// it's been set.
	virtual bool BYieldingSetPersonaName( const CSteamID & steamID, const char *pchPersonaName ) = 0;

	// waits for the writeback queue to clear
	virtual bool BYieldingWaitForCacheWriteback() = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Functions for sending and receiving messages from the Game Coordinator
//			for this application
//-----------------------------------------------------------------------------
class IExternalTest
{
public:

	// returns the number of tests present in this DLL
	virtual uint32 GetTestCount() = 0;

	// returns the name of the specified test
	virtual const char *GetTestName( uint32 unTest ) = 0;

	// returns the short name (no suite prefix) of the specified test
	virtual const char *GetTestShortName( uint32 unTest ) = 0;

	// returns the suite of the specified test
	virtual const char *GetTestSuite( uint32 unTest ) = 0;

	// returns the flags for the specified test
	virtual uint32 GetTestFlags( uint32 unTest ) = 0;

	// runs the specified tests. The tests should run to completion 
	// (yielding on a regular basis) and call pETestUtils->BCheck(false)
	// on failure
	virtual void BRunTests( uint32 *punTests, uint32 unTestCount, IExternalTestUtils * pETestUtils ) = 0;

	// Returns the name of a stress test action
	virtual const char *GetStressActionName( uint32 unAction ) = 0;

};
#define EXTERNALTEST_INTERFACE_VERSION "IExternalTest001"

// flags set by individual tests
static const uint32 k_unETFlag_ValidForStress		= 1<<0;
static const uint32 k_unETFlag_ValidForRegression	= 1<<1;


//-----------------------------------------------------------------------------
// Purpose: For one time init of an externaltests dll
//-----------------------------------------------------------------------------
class IExternalTestInitialize
{
public:
	virtual bool BDoBundleInit() = 0;	// called once *PER ATS* at DLL Load time
	virtual bool BDoOneTimeInit( IExternalTestUtils *pExternalTestUtils ) = 0; // called once PER TEST RUN by ATS0
	virtual bool BDoPerATSInit( IExternalTestUtils *pExternalTestUtils ) = 0; // called once PER TEST RUN PER ATS
	virtual void Cleanup() = 0;
};
#define EXTERNALTEST_INITIALIZE_INTERFACE_VERSION "IExternalTestInitialize001"


//-----------------------------------------------------------------------------
// Purpose: Allows an external test DLL to export its ability to validate its 
// own allocations
//-----------------------------------------------------------------------------
class IExternalTestValidation
{
public:

	virtual void Validate( CValidator &validator, const char *pchName ) = 0;
};
#define EXTERNALTEST_VALIDATION_INTERFACE_VERSION "IExternalTestValidation001"


#endif // IEXTERNALTEST
