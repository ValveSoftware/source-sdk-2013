//====== Copyright ©, Valve Corporation, All rights reserved. =======
//
// Purpose: Maps message types to strings and vice versa
//
//=============================================================================

#ifndef MESSAGELIST_H
#define MESSAGELIST_H
#ifdef _WIN32
#pragma once
#endif

// Protobuf headers interfere with the valve min/max/malloc overrides. so we need to do all
// this funky wrapping to make the include happy.
#include <tier0/valve_minmax_off.h>
#include "google/protobuf/descriptor.h"
#include <tier0/valve_minmax_on.h>
#include "gcsdk/jobtime.h"

namespace GCSDK
{

extern CGCEmitGroup g_EGMessages;
extern const char *PchMsgNameFromEMsg( MsgType_t eMsg );

//-----------------------------------------------------------------------------
// message type flags
//-----------------------------------------------------------------------------

static const int MT_GC	= 0x01;		// this message is sent to or from a Game Coordinator (will be proxied by servers along the way)
static const int MT_GC_SYSTEM = 0x02; // this message was sent to or from the steam servers as a system message. Clients can't send these

//-----------------------------------------------------------------------------
// Various info about each message type
//-----------------------------------------------------------------------------
struct MsgInfo_t
{
	MsgType_t eMsg;
	int	nFlags;
	const char *pchMsgName;

	struct Stats_t
	{
		Stats_t() : nSourceMask(0), nCount( 0 ), uBytes( 0 ) {}
		uint32 nSourceMask;
		uint32 nCount;
		uint64 uBytes;
	};

	enum EStatsType
	{
		k_EStatsTypeSent,
		k_EStatsTypeReceived,
		k_EStatsTypeMultiplexedSends,
		k_EStatsTypeMultiplexedSendsRaw,

		k_EStatsType_Count
	};

	enum EStatsGroup
	{
		k_EStatsGroupGlobal,
		k_EStatsGroupProfile,
		k_EStatsGroupWindow,

		k_EStatsGroup_Count
	};


	Stats_t stats[ k_EStatsGroup_Count ][ k_EStatsType_Count ];
};

//-----------------------------------------------------------------------------
// Purpose: Using protobuf reflection, bind them into a message list
//-----------------------------------------------------------------------------
void MsgRegistrationFromEnumDescriptor( const ::google::protobuf::EnumDescriptor *pEnumDescriptor, int nTypeMask );

//-----------------------------------------------------------------------------
// manages a hashed list of messages, allowing fast tests for validity and
// info lookup.
//-----------------------------------------------------------------------------

class CMessageList
{
public:
	CMessageList();
	~CMessageList();

	bool BInit( );

	// returns false if a message isn't valid or isn't one of the types specified
	//   or true if the message is valid. ppMsgName can be NULL.
	bool GetMessage( MsgType_t eMsg, const char **ppMsgName, int nTypeMask );

	// make stats about sending messages
	void TallySendMessage( MsgType_t eMsg, uint32 uMsgSize, uint32 nSourceMask = 0 );
	void TallyReceiveMessage( MsgType_t eMsg, uint32 uMsgSize, uint32 nSourceMask = 0);
	void TallyMultiplexedMessage( MsgType_t eMsg, uint32 uSent, uint32 cRecipients, uint32 uMsgSize, uint32 nSourceMask = 0 );

	// profiling
	void EnableProfiling( bool bEnableProfiling );

	// print out our stats
	void PrintStats( bool bShowAll, bool bSortByFrequency, MsgInfo_t::EStatsGroup eGroup, MsgInfo_t::EStatsType eType, uint32 nSourceMask = 0 ) const; 
	void PrintMultiplexStats( MsgInfo_t::EStatsGroup eGroup, bool bSortByFrequency, uint32 nSourceMask = 0 ) const;

	// Window management - This is similar to profiling in many ways, but is separate so that the base system can monitor traffic rates without
	// interfering with profiles.
	
	//called to obtain the totals for the timing window
	const MsgInfo_t::Stats_t& GetWindowTotal( MsgInfo_t::EStatsType eType ) const;
	//called to reset the window timings
	void ResetWindow();
	//returns how long this window has been running in microseconds
	uint64 GetWindowDuration() const		{ return GetGroupDuration( MsgInfo_t::k_EStatsGroupWindow ); }
	

private:
	void TallyMessageInternal( MsgInfo_t &msgInfo, MsgInfo_t::EStatsType eBucket, uint32 unMsgSize, uint32 nSourceMask, uint32 cMessages = 1 );

	void AssureBucket( int nBucket );

	//-----------------------------------------------------------------------------
	// given a particular message ID, find out what bucket it would be in,
	// as well as which slot in that bucket.
	//-----------------------------------------------------------------------------
	static int HashMessage( MsgType_t eMsg, int &nSlot )
	{
		// hash is everything except the lowest nibble,
		// because buckets are 16 entries
		int nBucket = eMsg / m_kcBucketSize;
		nSlot = eMsg % m_kcBucketSize;
		return nBucket;
	}

	short GetMessageIndex( MsgType_t eMsg );

	//given a group, this will return the time that the stats have been collected over
	uint64 GetGroupDuration( MsgInfo_t::EStatsGroup eGroup ) const;

private:

	//totalled stats for the current window. It would be too costly to total these all the time
	MsgInfo_t::Stats_t		m_WindowTotals[ MsgInfo_t::k_EStatsType_Count ];

	//the time that we have been collecting each of these buckets
	CJobTime				m_sCollectTime[ MsgInfo_t::k_EStatsGroup_Count ];	
	
	//are we currently actively tracking a profile?
	bool m_bProfiling;
	//the duration of the last finished profile (if it is no longer running, otherwise use the timer)
	uint64 m_ulProfileMicrosecs;

	CUtlVector< short* > m_vecMessageInfoBuckets;
	CUtlVector<MsgInfo_t> m_vecMsgInfo;
	static const int m_kcBucketSize = 16;
};

extern CMessageList g_theMessageList;

//-----------------------------------------------------------------------------
// Purpose: Returns the true if the specified message is a valid GC system message.
// Input  : eMsg -		message type to test
//			ppMsgName - Optional pointer to receive message name
//-----------------------------------------------------------------------------
inline bool BIsValidSystemMsg( MsgType_t eMsg, const char **ppMsgName )
{
	return g_theMessageList.GetMessage( eMsg, ppMsgName, MT_GC_SYSTEM );
}


} // namespace GCSDK

#endif // MESSAGELIST_H
