//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Real-Time Hierarchical Profiling
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPROF_H
#define VPROF_H

#include "tier0/dbg.h"
#include "tier0/fasttimer.h"
#include "tier0/l2cache.h"
#include "tier0/threadtools.h"
#include "tier0/vprof_telemetry.h"

// VProf is enabled by default in all configurations -except- X360 Retail.
#if !( defined( _X360 ) && defined( _CERT ) )
#define VPROF_ENABLED
#endif

#if defined(_X360) && defined(VPROF_ENABLED)
#include "tier0/pmc360.h"
#ifndef USE_PIX
#define VPROF_UNDO_PIX
#undef _PIX_H_
#undef PIXBeginNamedEvent
#undef PIXEndNamedEvent
#undef PIXSetMarker
#undef PIXNameThread
#define USE_PIX
#include <pix.h>
#undef USE_PIX
#else
#include <pix.h>
#endif
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

// enable this to get detailed nodes beneath budget
// #define VPROF_LEVEL 1

// enable this to use pix (360 only)
// #define VPROF_PIX 1

#if defined(VPROF_PIX)
#pragma comment( lib, "Xapilibi" )
#endif

//-----------------------------------------------------------------------------
//
// Profiling instrumentation macros
//

#define MAXCOUNTERS 256


#ifdef VPROF_ENABLED

#define VPROF_VTUNE_GROUP

#define	VPROF( name )						VPROF_(name, 1, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0)
#define	VPROF_ASSERT_ACCOUNTED( name )		VPROF_(name, 1, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, true, 0)
#define	VPROF_( name, detail, group, bAssertAccounted, budgetFlags )		VPROF_##detail(name,group, bAssertAccounted, budgetFlags)

#define VPROF_BUDGET( name, group )					VPROF_BUDGET_FLAGS(name, group, BUDGETFLAG_OTHER)
#define VPROF_BUDGET_FLAGS( name, group, flags )	VPROF_(name, 0, group, false, flags)

#define VPROF_SCOPE_BEGIN( tag )	do { VPROF( tag )
#define VPROF_SCOPE_END()			} while (0)

#define VPROF_ONLY( expression )	expression

#define VPROF_ENTER_SCOPE( name )			g_VProfCurrentProfile.EnterScope( name, 1, VPROF_BUDGETGROUP_OTHER_UNACCOUNTED, false, 0 )
#define VPROF_EXIT_SCOPE()					g_VProfCurrentProfile.ExitScope()

#define VPROF_BUDGET_GROUP_ID_UNACCOUNTED 0


// Budgetgroup flags. These are used with VPROF_BUDGET_FLAGS.
// These control which budget panels the groups show up in.
// If a budget group uses VPROF_BUDGET, it gets the default 
// which is BUDGETFLAG_OTHER.
#define BUDGETFLAG_CLIENT	(1<<0)		// Shows up in the client panel.
#define BUDGETFLAG_SERVER	(1<<1)		// Shows up in the server panel.
#define BUDGETFLAG_OTHER	(1<<2)		// Unclassified (the client shows these but the dedicated server doesn't).
#define BUDGETFLAG_HIDDEN	(1<<15)
#define BUDGETFLAG_ALL		0xFFFF


// NOTE: You can use strings instead of these defines. . they are defined here and added
// in vprof.cpp so that they are always in the same order.
#define VPROF_BUDGETGROUP_OTHER_UNACCOUNTED			_T("Unaccounted")
#define VPROF_BUDGETGROUP_WORLD_RENDERING			_T("World Rendering")
#define VPROF_BUDGETGROUP_DISPLACEMENT_RENDERING	_T("Displacement_Rendering")
#define VPROF_BUDGETGROUP_GAME						_T("Game")
#define VPROF_BUDGETGROUP_NPCS						_T("NPCs")
#define VPROF_BUDGETGROUP_SERVER_ANIM				_T("Server Animation")
#define VPROF_BUDGETGROUP_PHYSICS					_T("Physics")
#define VPROF_BUDGETGROUP_STATICPROP_RENDERING		_T("Static_Prop_Rendering")
#define VPROF_BUDGETGROUP_MODEL_RENDERING			_T("Other_Model_Rendering")
#define VPROF_BUDGETGROUP_MODEL_FAST_PATH_RENDERING _T("Fast Path Model Rendering")
#define VPROF_BUDGETGROUP_BRUSHMODEL_RENDERING		_T("Brush_Model_Rendering")
#define VPROF_BUDGETGROUP_SHADOW_RENDERING			_T("Shadow_Rendering")
#define VPROF_BUDGETGROUP_DETAILPROP_RENDERING		_T("Detail_Prop_Rendering")
#define VPROF_BUDGETGROUP_PARTICLE_RENDERING		_T("Particle/Effect_Rendering")
#define VPROF_BUDGETGROUP_ROPES						_T("Ropes")
#define VPROF_BUDGETGROUP_DLIGHT_RENDERING			_T("Dynamic_Light_Rendering")
#define VPROF_BUDGETGROUP_OTHER_NETWORKING			_T("Networking")
#define VPROF_BUDGETGROUP_CLIENT_ANIMATION			_T("Client_Animation")
#define VPROF_BUDGETGROUP_OTHER_SOUND				_T("Sound")
#define VPROF_BUDGETGROUP_OTHER_VGUI				_T("VGUI")
#define VPROF_BUDGETGROUP_OTHER_FILESYSTEM			_T("FileSystem")
#define VPROF_BUDGETGROUP_PREDICTION				_T("Prediction")
#define VPROF_BUDGETGROUP_INTERPOLATION				_T("Interpolation")
#define VPROF_BUDGETGROUP_SWAP_BUFFERS				_T("Swap_Buffers")
#define VPROF_BUDGETGROUP_PLAYER					_T("Player")
#define VPROF_BUDGETGROUP_OCCLUSION					_T("Occlusion")
#define VPROF_BUDGETGROUP_OVERLAYS					_T("Overlays")
#define VPROF_BUDGETGROUP_TOOLS						_T("Tools")
#define VPROF_BUDGETGROUP_LIGHTCACHE				_T("Light_Cache")
#define VPROF_BUDGETGROUP_DISP_HULLTRACES			_T("Displacement_Hull_Traces")
#define VPROF_BUDGETGROUP_TEXTURE_CACHE				_T("Texture_Cache")
#define VPROF_BUDGETGROUP_REPLAY					_T("Replay")
#define VPROF_BUDGETGROUP_PARTICLE_SIMULATION		_T("Particle Simulation")
#define VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING	_T("Flashlight Shadows")
#define VPROF_BUDGETGROUP_CLIENT_SIM				_T("Client Simulation") // think functions, tempents, etc.
#define VPROF_BUDGETGROUP_STEAM						_T("Steam") 
#define VPROF_BUDGETGROUP_CVAR_FIND					_T("Cvar_Find") 
#define VPROF_BUDGETGROUP_CLIENTLEAFSYSTEM			_T("ClientLeafSystem")
#define VPROF_BUDGETGROUP_JOBS_COROUTINES			_T("Jobs/Coroutines")
#define VPROF_BUDGETGROUP_SLEEPING					_T("Sleeping")
#define VPROF_BUDGETGROUP_THREADINGMAIN				_T("ThreadingMain")
#define VPROF_BUDGETGROUP_CHROMEHTML				_T("Chromehtml")
#define VPROF_BUDGETGROUP_VGUI						VPROF_BUDGETGROUP_CHROMEHTML
#define VPROF_BUDGETGROUP_TENFOOT					VPROF_BUDGETGROUP_CHROMEHTML
#define VPROF_BUDGETGROUP_STEAMUI					VPROF_BUDGETGROUP_CHROMEHTML
#define VPROF_BUDGETGROUP_ATTRIBUTES				_T("Attributes")
	
#ifdef _X360
// update flags
#define VPROF_UPDATE_BUDGET				0x01	// send budget data every frame
#define VPROF_UPDATE_TEXTURE_GLOBAL		0x02	// send global texture data every frame
#define VPROF_UPDATE_TEXTURE_PERFRAME	0x04	// send perframe texture data every frame
#endif

//-------------------------------------

#ifndef VPROF_LEVEL
#define VPROF_LEVEL 0
#endif

//these macros exist to create VProf_<line number> variables. This is important because it avoids /analyze warnings about variable aliasing when VPROF's are nested within each other, and allows
//for multiple VPROF's to exist within the same scope. Three macros must be used to force the __LINE__ to be resolved prior to the token concatenation, but just ignore the _INTERNAL macros and use
//the VPROF_VAR_NAME
#define VPROF_VAR_NAME_INTERNAL_CAT(a, b)	a##b
#define VPROF_VAR_NAME_INTERNAL( a, b )		VPROF_VAR_NAME_INTERNAL_CAT( a, b )
#define VPROF_VAR_NAME( a )					VPROF_VAR_NAME_INTERNAL( a, __LINE__ )

#define	VPROF_0(name,group,assertAccounted,budgetFlags)	TM_ZONE( TELEMETRY_LEVEL2, TMZF_NONE, "(%s)%s", group, name ); CVProfScope VPROF_VAR_NAME( VProf_ )(name, 0, group, assertAccounted, budgetFlags);

#if VPROF_LEVEL > 0 
#define	VPROF_1(name,group,assertAccounted,budgetFlags)	TM_ZONE( TELEMETRY_LEVEL3, TMZF_NONE, "(%s)%s", group, name ); CVProfScope VPROF_VAR_NAME( VProf_ )(name, 1, group, assertAccounted, budgetFlags);
#else
#define	VPROF_1(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 1 
#define	VPROF_2(name,group,assertAccounted,budgetFlags)	TM_ZONE( TELEMETRY_LEVEL4, TMZF_NONE, "(%s)%s", group, name ); CVProfScope VPROF_VAR_NAME( VProf_ )(name, 2, group, assertAccounted, budgetFlags);
#else
#define	VPROF_2(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 2 
#define	VPROF_3(name,group,assertAccounted,budgetFlags)	TM_ZONE( TELEMETRY_LEVEL5, TMZF_NONE, "(%s)%s", group, name ); CVProfScope VPROF_VAR_NAME( VProf_ )(name, 3, group, assertAccounted, budgetFlags);
#else
#define	VPROF_3(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

#if VPROF_LEVEL > 3 
#define	VPROF_4(name,group,assertAccounted,budgetFlags)	TM_ZONE( TELEMETRY_LEVEL6, TMZF_NONE, "(%s)%s", group, name ); CVProfScope VPROF_VAR_NAME( VProf_ )(name, 4, group, assertAccounted, budgetFlags);
#else
#define	VPROF_4(name,group,assertAccounted,budgetFlags)	((void)0)
#endif

//-------------------------------------

#ifdef _MSC_VER
#define VProfCode( code ) \
	if ( 0 ) \
		; \
	else \
	{ \
	VPROF( __FUNCTION__ ": " #code ); \
		code; \
	}
#else
#define VProfCode( code ) \
	if ( 0 ) \
		; \
	else \
	{ \
		VPROF( #code ); \
		code; \
	} 
#endif


//-------------------------------------

#define VPROF_INCREMENT_COUNTER(name,amount)			do { static CVProfCounter _counter( name ); _counter.Increment( amount ); } while( 0 )
#define VPROF_INCREMENT_GROUP_COUNTER(name,group,amount)			do { static CVProfCounter _counter( name, group ); _counter.Increment( amount ); } while( 0 )

#else

#define	VPROF( name )									((void)0)
#define	VPROF_ASSERT_ACCOUNTED( name )					((void)0)
#define	VPROF_( name, detail, group, bAssertAccounted, budgetFlags )	((void)0)
#define VPROF_BUDGET( name, group )						((void)0)
#define VPROF_BUDGET_FLAGS( name, group, flags )		((void)0)

#define VPROF_SCOPE_BEGIN( tag )	do {
#define VPROF_SCOPE_END()			} while (0)

#define VPROF_ONLY( expression )	((void)0)

#define VPROF_ENTER_SCOPE( name )
#define VPROF_EXIT_SCOPE()

#define VPROF_INCREMENT_COUNTER(name,amount)			((void)0)
#define VPROF_INCREMENT_GROUP_COUNTER(name,group,amount)	((void)0)

#define VPROF_TEST_SPIKE( msec )	((void)0)

#define VProfCode( code ) code

#endif
 
//-----------------------------------------------------------------------------

#ifdef VPROF_ENABLED

//-----------------------------------------------------------------------------
//
// A node in the call graph hierarchy
//

class DBG_CLASS CVProfNode 
{
friend class CVProfRecorder;
friend class CVProfile;

public:
	CVProfNode( const tchar * pszName, int detailLevel, CVProfNode *pParent, const tchar *pBudgetGroupName, int budgetFlags );
	~CVProfNode();
	
	CVProfNode *GetSubNode( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, int budgetFlags );
	CVProfNode *GetSubNode( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName );
	CVProfNode *GetParent();
	CVProfNode *GetSibling();		
	CVProfNode *GetPrevSibling();	
	CVProfNode *GetChild();		
	
	void MarkFrame();
	void ResetPeak();
	
	void Pause();
	void Resume();
	void Reset();

	void EnterScope();
	bool ExitScope();

	const tchar *GetName();

	int GetBudgetGroupID()
	{
		return m_BudgetGroupID;
	}

	// Only used by the record/playback stuff.
	void SetBudgetGroupID( int id )
	{
		m_BudgetGroupID = id;
	}

	int	GetCurCalls();
	double GetCurTime();		
	int GetPrevCalls();
	double GetPrevTime();
	int	GetTotalCalls();
	double GetTotalTime();		
	double GetPeakTime();		

	double GetCurTimeLessChildren();
	double GetPrevTimeLessChildren();
	double GetTotalTimeLessChildren();

	int GetPrevL2CacheMissLessChildren();
	int GetPrevLoadHitStoreLessChildren();

	void ClearPrevTime();

	int GetL2CacheMisses();

	// Not used in the common case...
	void SetCurFrameTime( unsigned long milliseconds );
	
	void SetClientData( int iClientData )	{ m_iClientData = iClientData; }
	int GetClientData() const				{ return m_iClientData; }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE


// Used by vprof record/playback.
private:

	void SetUniqueNodeID( int id )
	{
		m_iUniqueNodeID = id;
	}

	int GetUniqueNodeID() const
	{
		return m_iUniqueNodeID;
	}

	static int s_iCurrentUniqueNodeID;


private:
	const tchar *m_pszName;
	CFastTimer	m_Timer;

	// L2 Cache data.
	int			m_iPrevL2CacheMiss;
	int			m_iCurL2CacheMiss;
	int			m_iTotalL2CacheMiss;

#ifndef _X360	
	// L2 Cache data.
	CL2Cache	m_L2Cache;
#else // 360:
	
	unsigned int m_iBitFlags; // see enum below for settings
	CPMCData	m_PMCData;
	int			m_iPrevLoadHitStores;
	int			m_iCurLoadHitStores;
	int			m_iTotalLoadHitStores;

	public:
	enum FlagBits
	{
		kRecordL2 = 0x01,
		kCPUTrace = 0x02, ///< cause a PIX trace inside this node.
	};
	// call w/ true to enable L2 and LHS recording; false to turn it off
	inline void EnableL2andLHS(bool enable)
	{
		if (enable)
			m_iBitFlags |= kRecordL2;
		else
			m_iBitFlags &= (~kRecordL2);
	}

	inline bool IsL2andLHSEnabled( void )
	{
		return (m_iBitFlags & kRecordL2) != 0;
	}

	int GetLoadHitStores();

	private:
	
#endif

	int			m_nRecursions;
	
	unsigned	m_nCurFrameCalls;
	CCycleCount	m_CurFrameTime;
	
	unsigned	m_nPrevFrameCalls;
	CCycleCount	m_PrevFrameTime;

	unsigned	m_nTotalCalls;
	CCycleCount	m_TotalTime;

	CCycleCount	m_PeakTime;

	CVProfNode *m_pParent;
	CVProfNode *m_pChild;
	CVProfNode *m_pSibling;

	int m_BudgetGroupID;
	
	int m_iClientData;
	int m_iUniqueNodeID;
};

//-----------------------------------------------------------------------------
//
// Coordinator and root node of the profile hierarchy tree
//

enum VProfReportType_t
{
	VPRT_SUMMARY									= ( 1 << 0 ),
	VPRT_HIERARCHY									= ( 1 << 1 ),
	VPRT_HIERARCHY_TIME_PER_FRAME_AND_COUNT_ONLY	= ( 1 << 2 ),
	VPRT_LIST_BY_TIME								= ( 1 << 3 ),
	VPRT_LIST_BY_TIME_LESS_CHILDREN					= ( 1 << 4 ),
	VPRT_LIST_BY_AVG_TIME							= ( 1 << 5 ),	
	VPRT_LIST_BY_AVG_TIME_LESS_CHILDREN				= ( 1 << 6 ),
	VPRT_LIST_BY_PEAK_TIME							= ( 1 << 7 ),
	VPRT_LIST_BY_PEAK_OVER_AVERAGE					= ( 1 << 8 ),
	VPRT_LIST_TOP_ITEMS_ONLY						= ( 1 << 9 ),

	VPRT_FULL = (0xffffffff & ~(VPRT_HIERARCHY_TIME_PER_FRAME_AND_COUNT_ONLY|VPRT_LIST_TOP_ITEMS_ONLY)),
};

enum CounterGroup_t
{
	COUNTER_GROUP_DEFAULT=0,
	COUNTER_GROUP_NO_RESET,				// The engine doesn't reset these counters. Usually, they are used 
										// like global variables that can be accessed across modules.
	COUNTER_GROUP_TEXTURE_GLOBAL,		// Global texture usage counters (totals for what is currently in memory).
	COUNTER_GROUP_TEXTURE_PER_FRAME,	// Per-frame texture usage counters.

	COUNTER_GROUP_TELEMETRY,
}; 

class DBG_CLASS CVProfile 
{
public:
	CVProfile();
	~CVProfile();

	void Term();
	
	//
	// Runtime operations
	//
	
	void Start();
	void Stop();

	void SetTargetThreadId( unsigned id ) { m_TargetThreadId = id; }
	unsigned GetTargetThreadId() { return m_TargetThreadId; }
	bool InTargetThread() { return ( m_TargetThreadId == ThreadGetCurrentId() ); }

#ifdef _X360
	enum VXConsoleReportMode_t
	{
		VXCONSOLE_REPORT_TIME = 0,
		VXCONSOLE_REPORT_L2CACHE_MISSES,
		VXCONSOLE_REPORT_LOAD_HIT_STORE,
		VXCONSOLE_REPORT_COUNT,
	};

	void VXProfileStart();
	void VXProfileUpdate();
	void VXEnableUpdateMode( int event, bool bEnable );
	void VXSendNodes( void );
	
	void PMCDisableAllNodes(CVProfNode *pStartNode = NULL);  ///< turn off l2 and lhs recording for everywhere
	bool PMCEnableL2Upon(const tchar *pszNodeName, bool bRecursive = false); ///< enable l2 and lhs recording for one given node
	bool PMCDisableL2Upon(const tchar *pszNodeName, bool bRecursive = false); ///< enable l2 and lhs recording for one given node

	void DumpEnabledPMCNodes( void );

	void VXConsoleReportMode( VXConsoleReportMode_t mode );
	void VXConsoleReportScale( VXConsoleReportMode_t mode, float flScale );

	// the CPU trace mode is actually a small state machine; it can be off, primed for
	// single capture, primed for everything-in-a-frame capture, or currently in everything-in-a-frame
	// capture.
	enum CPUTraceState
	{
		kDisabled,
		kFirstHitNode,						// record from the first time we hit the node until that node ends
		kAllNodesInFrame_WaitingForMark,	// we're going to record all the times a node is hit in a frame, but are waiting for the frame to start
		kAllNodesInFrame_Recording,			// we're recording all hits on a node this frame.

		// Same as above, but going to record for > 1 frame
		kAllNodesInFrame_WaitingForMarkMultiFrame,	// we're going to record all the times a node is hit in a frame, but are waiting for the frame to start
		kAllNodesInFrame_RecordingMultiFrame, 
	};

	// Global switch to turn CPU tracing on or off at all. The idea is you set up a node first,
	// then trigger tracing by throwing this to true. It'll reset back to false after the trace 
	// happens.
	inline CPUTraceState GetCPUTraceMode();
	inline void SetCPUTraceEnabled( CPUTraceState enabled, bool bTraceCompleteEvent = false, int nNumFrames = -1 );
	inline void IncrementMultiTraceIndex(); // tick up the counter that gets appended to the multi-per-frame traces
	inline unsigned int GetMultiTraceIndex(); // return the counter
	void CPUTraceDisableAllNodes( CVProfNode *pStartNode = NULL ); // disable the cpu trace flag wherever it may be
	CVProfNode *CPUTraceEnableForNode( const tchar *pszNodeName ); // enable cpu trace on this node only, disabling it wherever else it may be on.
	CVProfNode *CPUTraceGetEnabledNode( CVProfNode *pStartNode = NULL ); // return the node enabled for CPU tracing, or NULL.
	const char *GetCPUTraceFilename(); // get the filename the trace should write into.
	const char *SetCPUTraceFilename( const char *filename ); // set the filename the trace should write into. (don't specify the extension; I'll do that.)
	inline bool TraceCompleteEvent( void );

#ifdef _X360
	void LatchMultiFrame( int64 cycles );
	void SpewWorstMultiFrame();
#endif

#endif

	void EnterScope( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted );
	void EnterScope( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted, int budgetFlags );
	void ExitScope();

	void MarkFrame();
	void ResetPeaks();
	
	void Pause();
	void Resume();
	void Reset();
	
	bool IsEnabled() const;
	int GetDetailLevel() const;

	bool AtRoot() const;

	//
	// Queries
	//

#ifdef VPROF_VTUNE_GROUP
#	define MAX_GROUP_STACK_DEPTH 1024

	void EnableVTuneGroup( const tchar *pGroupName )
	{
		m_nVTuneGroupID = BudgetGroupNameToBudgetGroupID( pGroupName );
		m_bVTuneGroupEnabled = true;
	}
	void DisableVTuneGroup( void )
	{
		m_bVTuneGroupEnabled = false;
	}
	
	inline void PushGroup( int nGroupID );
	inline void PopGroup( void );
#endif
	
	int NumFramesSampled()	{ return m_nFrames; }
	double GetPeakFrameTime();
	double GetTotalTimeSampled();
	double GetTimeLastFrame();
	
	CVProfNode *GetRoot();
	CVProfNode *FindNode( CVProfNode *pStartNode, const tchar *pszNode );
	CVProfNode *GetCurrentNode();

	typedef void ( __cdecl *StreamOut_t )( const char* pszFormat, ... );
	// Set the output function used for all vprof reports. Call this with NULL
	// to set it to the default output function.
	void SetOutputStream( StreamOut_t outputStream );
	void OutputReport( int type = VPRT_FULL, const tchar *pszStartNode = NULL, int budgetGroupID = -1 );

	const tchar *GetBudgetGroupName( int budgetGroupID );
	int GetBudgetGroupFlags( int budgetGroupID ) const;	// Returns a combination of BUDGETFLAG_ defines.
	int GetNumBudgetGroups( void );
	void GetBudgetGroupColor( int budgetGroupID, int &r, int &g, int &b, int &a );
	int BudgetGroupNameToBudgetGroupID( const tchar *pBudgetGroupName );
	int BudgetGroupNameToBudgetGroupID( const tchar *pBudgetGroupName, int budgetFlagsToORIn );
	void RegisterNumBudgetGroupsChangedCallBack( void (*pCallBack)(void) );

	int BudgetGroupNameToBudgetGroupIDNoCreate( const tchar *pBudgetGroupName ) { return FindBudgetGroupName( pBudgetGroupName ); }

	void HideBudgetGroup( int budgetGroupID, bool bHide = true );
	void HideBudgetGroup( const char *pszName, bool bHide = true ) { HideBudgetGroup( BudgetGroupNameToBudgetGroupID( pszName), bHide ); }

	int *FindOrCreateCounter( const tchar *pName, CounterGroup_t eCounterGroup=COUNTER_GROUP_DEFAULT  );
	void ResetCounters( CounterGroup_t eCounterGroup );
	
	int GetNumCounters( void ) const;
	
	const tchar *GetCounterName( int index ) const;
	int GetCounterValue( int index ) const;
	const tchar *GetCounterNameAndValue( int index, int &val ) const;
	CounterGroup_t GetCounterGroup( int index ) const;

	// Performance monitoring events.
	void PMEInitialized( bool bInit )		{ m_bPMEInit = bInit; }
	void PMEEnable( bool bEnable )			{ m_bPMEEnabled = bEnable; }

#ifndef _X360
	bool UsePME( void )						{ return ( m_bPMEInit && m_bPMEEnabled ); }
#else
	bool UsePME( void )						{ return ( CPMCData::IsInitialized() && m_bPMEEnabled ); }
#endif

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

protected:

	void FreeNodes_R( CVProfNode *pNode );

#ifdef VPROF_VTUNE_GROUP
	bool VTuneGroupEnabled()
	{ 
		return m_bVTuneGroupEnabled; 
	}
	int VTuneGroupID() 
	{ 
		return m_nVTuneGroupID; 
	}
#endif

	void SumTimes( const tchar *pszStartNode, int budgetGroupID );
	void SumTimes( CVProfNode *pNode, int budgetGroupID );
	void DumpNodes( CVProfNode *pNode, int indent, bool bAverageAndCountOnly );
	int FindBudgetGroupName( const tchar *pBudgetGroupName );
	int AddBudgetGroupName( const tchar *pBudgetGroupName, int budgetFlags );

#ifdef VPROF_VTUNE_GROUP
	bool		m_bVTuneGroupEnabled;
	int			m_nVTuneGroupID;
	int			m_GroupIDStack[MAX_GROUP_STACK_DEPTH];
	int			m_GroupIDStackDepth;
#endif
	int 		m_enabled;
	bool		m_fAtRoot; // tracked for efficiency of the "not profiling" case
	CVProfNode *m_pCurNode;
	CVProfNode	m_Root;
	int			m_nFrames;
	int			m_ProfileDetailLevel;
	int			m_pausedEnabledDepth;

	class CBudgetGroup
	{
	public:
		tchar *m_pName;
		int m_BudgetFlags;
	};
	
	CBudgetGroup	*m_pBudgetGroups;
	int			m_nBudgetGroupNamesAllocated;
	int			m_nBudgetGroupNames;
	void		(*m_pNumBudgetGroupsChangedCallBack)(void);

	// Performance monitoring events.
	bool		m_bPMEInit;
	bool		m_bPMEEnabled;

	int m_Counters[MAXCOUNTERS];
	char m_CounterGroups[MAXCOUNTERS]; // (These are CounterGroup_t's).
	tchar *m_CounterNames[MAXCOUNTERS];
	int m_NumCounters;

#ifdef _X360
	int						m_UpdateMode;
	CPUTraceState			m_iCPUTraceEnabled;
	int						m_nFramesRemaining;
	int						m_nFrameCount;
	int64					m_WorstCycles;
	char					m_WorstTraceFilename[128];
	char					m_CPUTraceFilename[128];
	unsigned int			m_iSuccessiveTraceIndex;
	VXConsoleReportMode_t	m_ReportMode;
	float					m_pReportScale[VXCONSOLE_REPORT_COUNT];
	bool					m_bTraceCompleteEvent;
#endif

	unsigned m_TargetThreadId;

	StreamOut_t				m_pOutputStream;
};

//-------------------------------------

DBG_INTERFACE CVProfile g_VProfCurrentProfile;


//-----------------------------------------------------------------------------

DBG_INTERFACE bool g_VProfSignalSpike;

class CVProfSpikeDetector
{
public:
	CVProfSpikeDetector( float spike ) :
		m_timeLast( GetTimeLast() )
	{
		m_spike = spike;
		m_Timer.Start();
	}

	~CVProfSpikeDetector()
	{
		m_Timer.End();
		if ( Plat_FloatTime() - m_timeLast > 2.0 )
		{
			m_timeLast = Plat_FloatTime();
			if ( m_Timer.GetDuration().GetMillisecondsF() > m_spike )
			{
				g_VProfSignalSpike = true;
			}
		}
	}

private:
	static float &GetTimeLast() { static float timeLast = 0; return timeLast; }
	CFastTimer	m_Timer;
	float m_spike;
	float &m_timeLast;
};


// Macro to signal a local spike. Meant as temporary instrumentation, do not leave in code
#define VPROF_TEST_SPIKE( msec ) CVProfSpikeDetector UNIQUE_ID( msec )

//-----------------------------------------------------------------------------

#ifdef VPROF_VTUNE_GROUP
inline void CVProfile::PushGroup( int nGroupID )
{
	// There is always at least one item on the stack since we force 
	// the first element to be VPROF_BUDGETGROUP_OTHER_UNACCOUNTED.
	Assert( m_GroupIDStackDepth > 0 );
	Assert( m_GroupIDStackDepth < MAX_GROUP_STACK_DEPTH );
	m_GroupIDStack[m_GroupIDStackDepth] = nGroupID;
	m_GroupIDStackDepth++;
	if( m_GroupIDStack[m_GroupIDStackDepth-2] != nGroupID && 
		VTuneGroupEnabled() &&
		nGroupID == VTuneGroupID() )
	{
		vtune( true );
	}
}
#endif // VPROF_VTUNE_GROUP

#ifdef VPROF_VTUNE_GROUP
inline void CVProfile::PopGroup( void )
{
	m_GroupIDStackDepth--;
	// There is always at least one item on the stack since we force 
	// the first element to be VPROF_BUDGETGROUP_OTHER_UNACCOUNTED.
	Assert( m_GroupIDStackDepth > 0 );
	if(	m_GroupIDStack[m_GroupIDStackDepth] != m_GroupIDStack[m_GroupIDStackDepth+1] && 
		VTuneGroupEnabled() &&
		m_GroupIDStack[m_GroupIDStackDepth+1] == VTuneGroupID() )
	{
		vtune( false );
	}
}
#endif // VPROF_VTUNE_GROUP

//-----------------------------------------------------------------------------

class CVProfScope
{
public:
	CVProfScope( const tchar * pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted, int budgetFlags );
	~CVProfScope();

private:
	bool m_bEnabled;
};

//-----------------------------------------------------------------------------
//
// CVProfNode, inline methods
//

inline CVProfNode::CVProfNode( const tchar * pszName, int detailLevel, CVProfNode *pParent, const tchar *pBudgetGroupName, int budgetFlags )
 :	m_pszName( pszName ),
	m_nCurFrameCalls( 0 ),
	m_nPrevFrameCalls( 0 ),
	m_nRecursions( 0 ),
	m_pParent( pParent ),
	m_pChild( NULL ),
	m_pSibling( NULL ),
	m_iClientData( -1 )
#ifdef _X360
	, m_iBitFlags( 0 )
#endif
{
	m_iUniqueNodeID = s_iCurrentUniqueNodeID++;

	if ( m_iUniqueNodeID > 0 )
	{
		m_BudgetGroupID = g_VProfCurrentProfile.BudgetGroupNameToBudgetGroupID( pBudgetGroupName, budgetFlags );
	}
	else
	{
		m_BudgetGroupID = 0; // "m_Root" can't call BudgetGroupNameToBudgetGroupID because g_VProfCurrentProfile not yet initialized
	}

	Reset();

	if( m_pParent && ( m_BudgetGroupID == VPROF_BUDGET_GROUP_ID_UNACCOUNTED ) )
	{
		m_BudgetGroupID = m_pParent->GetBudgetGroupID();
	}
}


//-------------------------------------

inline CVProfNode *CVProfNode::GetParent()		
{ 
	Assert( m_pParent );
	return m_pParent; 
}

//-------------------------------------

inline CVProfNode *CVProfNode::GetSibling()		
{ 
	return m_pSibling; 
}

//-------------------------------------
// Hacky way to the previous sibling, only used from vprof panel at the moment,
// so it didn't seem like it was worth the memory waste to add the reverse
// link per node.

inline CVProfNode *CVProfNode::GetPrevSibling()		
{ 
	CVProfNode* p = GetParent();

	if(!p) 
		return NULL;

	CVProfNode* s;
	for( s = p->GetChild(); 
	     s && ( s->GetSibling() != this ); 
		 s = s->GetSibling() )
		;

	return s;	
}

//-------------------------------------

inline CVProfNode *CVProfNode::GetChild()			
{ 
	return m_pChild; 
}

//-------------------------------------

inline const tchar *CVProfNode::GetName()				
{ 
	Assert( m_pszName );
	return m_pszName; 
}

//-------------------------------------

inline int	CVProfNode::GetTotalCalls()		
{ 
	return m_nTotalCalls; 
}

//-------------------------------------

inline double CVProfNode::GetTotalTime()		
{ 
	return m_TotalTime.GetMillisecondsF();
}

//-------------------------------------

inline int	CVProfNode::GetCurCalls()		
{ 
	return m_nCurFrameCalls; 
}

//-------------------------------------

inline double CVProfNode::GetCurTime()		
{ 
	return m_CurFrameTime.GetMillisecondsF();
}

//-------------------------------------

inline int CVProfNode::GetPrevCalls()
{
	return m_nPrevFrameCalls;
}

//-------------------------------------

inline double CVProfNode::GetPrevTime()		
{ 
	return m_PrevFrameTime.GetMillisecondsF();
}

//-------------------------------------

inline double CVProfNode::GetPeakTime()		
{ 
	return m_PeakTime.GetMillisecondsF();
}

//-------------------------------------

inline double CVProfNode::GetTotalTimeLessChildren()
{
	double result = GetTotalTime();
	CVProfNode *pChild = GetChild();
	while ( pChild )
	{
		result -= pChild->GetTotalTime();
		pChild = pChild->GetSibling();
	}
	return result;
}

//-------------------------------------

inline double CVProfNode::GetCurTimeLessChildren()
{
	double result = GetCurTime();
	CVProfNode *pChild = GetChild();
	while ( pChild )
	{
		result -= pChild->GetCurTime();
		pChild = pChild->GetSibling();
	}
	return result;
}

inline double CVProfNode::GetPrevTimeLessChildren()
{
	double result = GetPrevTime();
	CVProfNode *pChild = GetChild();
	while ( pChild )
	{
		result -= pChild->GetPrevTime();
		pChild = pChild->GetSibling();
	}
	return result;
}

//-----------------------------------------------------------------------------
inline int CVProfNode::GetPrevL2CacheMissLessChildren()
{
	int result = m_iPrevL2CacheMiss;
	CVProfNode *pChild = GetChild();
	while ( pChild )
	{
		result -= pChild->m_iPrevL2CacheMiss;
		pChild = pChild->GetSibling();
	}
	return result;
}

//-----------------------------------------------------------------------------
inline int CVProfNode::GetPrevLoadHitStoreLessChildren()
{
#ifndef _X360
	return 0;
#else
	int result = m_iPrevLoadHitStores;
	CVProfNode *pChild = GetChild();
	while ( pChild )
	{
		result -= pChild->m_iPrevLoadHitStores;
		pChild = pChild->GetSibling();
	}
	return result;
#endif
}


//-----------------------------------------------------------------------------
inline void CVProfNode::ClearPrevTime()
{
	m_PrevFrameTime.Init();
}

//-----------------------------------------------------------------------------
inline int CVProfNode::GetL2CacheMisses( void )
{ 
#ifndef _X360
	return m_L2Cache.GetL2CacheMisses(); 
#else
	return m_iTotalL2CacheMiss;
#endif
}

#ifdef _X360
inline int CVProfNode::GetLoadHitStores( void )
{
	return m_iTotalLoadHitStores;
}
#endif

//-----------------------------------------------------------------------------
//
// CVProfile, inline methods
//

//-------------------------------------

inline bool CVProfile::IsEnabled() const	
{ 
	return ( m_enabled != 0 ); 
}

//-------------------------------------

inline int CVProfile::GetDetailLevel() const	
{ 
	return m_ProfileDetailLevel; 
}

	
//-------------------------------------

inline bool CVProfile::AtRoot() const
{
	return m_fAtRoot;
}
	
//-------------------------------------

inline void CVProfile::Start()	
{ 
	if ( ++m_enabled == 1 )
	{
		m_Root.EnterScope();
#ifdef _X360
		VXProfileStart();
		CPMCData::InitializeOnceProgramWide();
#endif
	}
}

//-------------------------------------

inline void CVProfile::Stop()		
{ 
	if ( --m_enabled == 0 )
		m_Root.ExitScope();
}

//-------------------------------------

inline void CVProfile::EnterScope( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted, int budgetFlags )
{
	if ( ( m_enabled != 0 || !m_fAtRoot ) && InTargetThread() ) // if became disabled, need to unwind back to root before stopping
	{
		// Only account for vprof stuff on the primary thread.
		//if( !Plat_IsPrimaryThread() )
		//	return;

		if ( pszName != m_pCurNode->GetName() ) 
		{
			m_pCurNode = m_pCurNode->GetSubNode( pszName, detailLevel, pBudgetGroupName, budgetFlags );
		}
		m_pBudgetGroups[m_pCurNode->GetBudgetGroupID()].m_BudgetFlags |= budgetFlags;

#if defined( _DEBUG ) && !defined( _X360 )
		// 360 doesn't want this to allow tier0 debug/release .def files to match
		if ( bAssertAccounted )
		{
			// FIXME
			AssertOnce( m_pCurNode->GetBudgetGroupID() != 0 );
		}
#endif
		m_pCurNode->EnterScope();
		m_fAtRoot = false;
	}
#if defined(_X360) && defined(VPROF_PIX)
	if ( m_pCurNode->GetBudgetGroupID() != VPROF_BUDGET_GROUP_ID_UNACCOUNTED )
		PIXBeginNamedEvent( 0, pszName );
#endif
}

inline void CVProfile::EnterScope( const tchar *pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted )
{
	EnterScope( pszName, detailLevel, pBudgetGroupName, bAssertAccounted, BUDGETFLAG_OTHER );
}

//-------------------------------------

inline void CVProfile::ExitScope()
{
#if defined(_X360) && defined(VPROF_PIX)
#ifdef PIXBeginNamedEvent
#error
#endif
	if ( m_pCurNode->GetBudgetGroupID() != VPROF_BUDGET_GROUP_ID_UNACCOUNTED )
		PIXEndNamedEvent();
#endif
	if ( ( !m_fAtRoot || m_enabled != 0 ) && InTargetThread() )
	{
		// Only account for vprof stuff on the primary thread.
		//if( !Plat_IsPrimaryThread() )
		//	return;

		// ExitScope will indicate whether we should back up to our parent (we may
		// be profiling a recursive function)
		if (m_pCurNode->ExitScope()) 
		{
			m_pCurNode = m_pCurNode->GetParent();
		}
		m_fAtRoot = ( m_pCurNode == &m_Root );
	}
}

//-------------------------------------

inline void CVProfile::Pause()
{
	m_pausedEnabledDepth = m_enabled;
	m_enabled = 0;
	if ( !AtRoot() )
		m_Root.Pause(); 
}

//-------------------------------------

inline void CVProfile::Resume()
{
	m_enabled = m_pausedEnabledDepth;
	if ( !AtRoot() )
		m_Root.Resume(); 
}

//-------------------------------------

inline void CVProfile::Reset()
{
	m_Root.Reset(); 
	m_nFrames = 0;
}

//-------------------------------------

inline void CVProfile::ResetPeaks()
{
	m_Root.ResetPeak(); 
}

//-------------------------------------

inline void CVProfile::MarkFrame()
{
	if ( m_enabled )
	{
		++m_nFrames;
		m_Root.ExitScope();
		m_Root.MarkFrame(); 
		m_Root.EnterScope();

#ifdef _X360
		// update the CPU trace state machine if enabled
		switch ( GetCPUTraceMode() )
		{
		case kAllNodesInFrame_WaitingForMark:
			// mark! Start recording a zillion traces.
			m_iCPUTraceEnabled = kAllNodesInFrame_Recording;
			break;
		case kAllNodesInFrame_WaitingForMarkMultiFrame:
			m_iCPUTraceEnabled = kAllNodesInFrame_RecordingMultiFrame;
			break;
		case kAllNodesInFrame_Recording:
			// end of frame. stop recording if no more frames needed
			m_iCPUTraceEnabled = kDisabled;
			Msg("Frame ended. Recording no more CPU traces\n");

			break;
		case kAllNodesInFrame_RecordingMultiFrame:
			// end of frame. stop recording if no more frames needed
			if ( --m_nFramesRemaining == 0 )
			{
				m_iCPUTraceEnabled = kDisabled;
				Msg("Frames ended. Recording no more CPU traces\n");

				SpewWorstMultiFrame();
			}

			++m_nFrameCount;

			break;
		default:
			// no default
			break;
		}
#endif
	}
}

//-------------------------------------

inline double CVProfile::GetTotalTimeSampled()
{
	return m_Root.GetTotalTime();
}

//-------------------------------------

inline double CVProfile::GetPeakFrameTime()
{
	return m_Root.GetPeakTime();
}

//-------------------------------------

inline double CVProfile::GetTimeLastFrame()
{
	return m_Root.GetCurTime();
}
	
//-------------------------------------

inline CVProfNode *CVProfile::GetRoot()
{
	return &m_Root;
}

//-------------------------------------

inline CVProfNode *CVProfile::GetCurrentNode()
{
	return m_pCurNode;
}


inline const tchar *CVProfile::GetBudgetGroupName( int budgetGroupID )
{
	Assert( budgetGroupID >= 0 && budgetGroupID < m_nBudgetGroupNames );
	return m_pBudgetGroups[budgetGroupID].m_pName;
}

inline int CVProfile::GetBudgetGroupFlags( int budgetGroupID ) const
{
	Assert( budgetGroupID >= 0 && budgetGroupID < m_nBudgetGroupNames );
	return m_pBudgetGroups[budgetGroupID].m_BudgetFlags;
}

#ifdef _X360

inline CVProfile::CPUTraceState CVProfile::GetCPUTraceMode()
{
	return m_iCPUTraceEnabled;
}

inline void CVProfile::SetCPUTraceEnabled( CPUTraceState enabled, bool bTraceCompleteEvent /*=true*/, int nNumFrames /*= -1*/ )
{
	m_iCPUTraceEnabled = enabled;
	m_bTraceCompleteEvent = bTraceCompleteEvent;
	if ( nNumFrames != -1 )
	{
		m_nFramesRemaining = nNumFrames;
		m_nFrameCount = 0;
		m_WorstCycles = 0;
		m_WorstTraceFilename[ 0 ] = 0;
	}
}

inline void CVProfile::IncrementMultiTraceIndex()
{
	++m_iSuccessiveTraceIndex;
}

inline unsigned int CVProfile::GetMultiTraceIndex()
{
	return m_iSuccessiveTraceIndex;
}

#endif


//-----------------------------------------------------------------------------

inline CVProfScope::CVProfScope( const tchar * pszName, int detailLevel, const tchar *pBudgetGroupName, bool bAssertAccounted, int budgetFlags )
	: m_bEnabled( g_VProfCurrentProfile.IsEnabled() )
{ 
	if ( m_bEnabled )
	{
		g_VProfCurrentProfile.EnterScope( pszName, detailLevel, pBudgetGroupName, bAssertAccounted, budgetFlags ); 
	}
}

//-------------------------------------

inline CVProfScope::~CVProfScope()					
{ 
	if ( m_bEnabled )
	{
		g_VProfCurrentProfile.ExitScope(); 
	}
}

class CVProfCounter
{
public:
	CVProfCounter( const tchar *pName, CounterGroup_t group=COUNTER_GROUP_DEFAULT )
	{
		m_pCounter = g_VProfCurrentProfile.FindOrCreateCounter( pName, group );
		Assert( m_pCounter );
	}
	~CVProfCounter()
	{
	}
	void Increment( int val ) 
	{ 
		Assert( m_pCounter );
		*m_pCounter += val; 
	}
private:
	int *m_pCounter;
};

#endif

#ifdef _X360

#include "xbox/xbox_console.h"
#include "tracerecording.h"
#include  "tier1/fmtstr.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment( lib, "xbdm.lib" )

class CPIXRecorder
{
public:
	CPIXRecorder() : m_bActive( false ) {}
	~CPIXRecorder() { Stop(); }

	void Start( const char *pszFilename = "capture" )
	{
		if ( !m_bActive )
		{
			if ( !XTraceStartRecording( CFmtStr( "e:\\%s.pix2", pszFilename ) ) )
			{
				Msg( "XTraceStartRecording failed, error code %d\n", GetLastError() );
			}
			else
			{
				m_bActive = true;
			}
		}
	}

	void Stop()
	{
		if ( m_bActive )
		{
			m_bActive = false;
			if ( XTraceStopRecording() )
			{
				Msg( "CPU trace finished.\n" );
				// signal VXConsole that trace is completed
				XBX_rTraceComplete();
			}
		}
	}

private:
	bool m_bActive;
};

#define VPROF_BEGIN_PIX_BLOCK( convar ) \
	{ \
	bool bRunPix = 0; \
	static CFastTimer PIXTimer; \
	extern ConVar convar; \
	ConVar &PIXConvar = convar; \
	CPIXRecorder PIXRecorder; \
		{ \
		PIXLabel: \
			if ( bRunPix ) \
			{ \
				PIXRecorder.Start(); \
			} \
			else \
			{ \
				if ( PIXConvar.GetBool() ) \
				{ \
					PIXTimer.Start(); \
				} \
			} \
				{


#define VPROF_END_PIX_BLOCK() \
				} \
			\
			if ( !bRunPix ) \
			{ \
				if ( PIXConvar.GetBool() ) \
				{ \
					PIXTimer.End(); \
					if ( PIXTimer.GetDuration().GetMillisecondsF() > PIXConvar.GetFloat() ) \
					{ \
						PIXConvar.SetValue( 0 ); \
						bRunPix = true; \
						goto PIXLabel; \
					} \
				} \
			} \
			else \
			{ \
				PIXRecorder.Stop(); \
			} \
		} \
	}
#else
#define VPROF_BEGIN_PIX_BLOCK( PIXConvar ) {
#define VPROF_END_PIX_BLOCK() }
#endif


#ifdef VPROF_UNDO_PIX
#undef USE_PIX
#undef _PIX_H_
#undef PIXBeginNamedEvent
#undef PIXEndNamedEvent
#undef PIXSetMarker
#undef PIXNameThread
#include <pix.h>
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

//=============================================================================
