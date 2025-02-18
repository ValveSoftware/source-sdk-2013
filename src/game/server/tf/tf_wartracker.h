//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Tracker for War Data on a player
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SPYENGY_WARTRACKER_H
#define TF_SPYENGY_WARTRACKER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_wardata.h"
#include "shared_object_tracker.h"

class CTFplayer;

//---------------------------------------------------------------------
// Purpose: Tracks kills between Team Engineer and Team Spy in the 2014
//			war.  Players are considered Team Engineer if they are wearing
//			the special Engy Hat or are unallied and playing as Engy.
//			Visa versa, players are considered to be on Team Spy if they
//			are wearing the special Spy Hat or are unallied and playing 
//			as Spy.
//---------------------------------------------------------------------
class CTFWarTracker : public CBaseSOTracker, public CGameEventListener
{
	DECLARE_CLASS( CTFWarTracker, CBaseSOTracker )
public:
	CTFWarTracker( const CSharedObject* pItem, CSteamID SteamIDOwner, CSOTrackerManager* pManager );

	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;

	virtual void CommitChangesToDB() OVERRIDE;
	virtual	void OnUpdate() OVERRIDE {}
	virtual void OnRemove() OVERRIDE {}
private:

	static const int m_kPointsPerKill = 5;
	static const int m_kBonusPointsPerKill = 2;
	CGCMsgGC_War_IndividualUpdate m_ProtoData;
};

//---------------------------------------------------------------------
// Purpose: Contains all the trackers for wars for each player.  Tells 
//			individual trackers to update the GC every m_kflThinkInterval
//			seconds, defined below.
//---------------------------------------------------------------------
class CTFWarTrackerManager : public CSOTrackerManager
{
	DECLARE_CLASS( CTFWarTrackerManager, CSOTrackerManager )
public:
	CTFWarTrackerManager();

	virtual SOTrackerMap_t::KeyType_t GetKeyForObjectTracker( const CSharedObject* pItem, CSteamID steamIDOwner ) OVERRIDE;

private:
#ifdef GAME_DLL
	void SendMessageForCommit( const ::google::protobuf::Message* pProtoMessage ) const;
#endif
	virtual int GetType() const OVERRIDE { return CWarData::k_nTypeID; }
	virtual const char* GetName() const { return "WarTrackerManager"; }
	virtual CFmtStr GetDebugObjectDescription( const CSharedObject* pItem ) const;
	virtual CBaseSOTracker* AllocateNewTracker( const CSharedObject* pItem, CSteamID steamIDOwner, CSOTrackerManager* pManager ) const OVERRIDE;
	virtual ::google::protobuf::Message* AllocateNewProtoMessage() const OVERRIDE;
	virtual void OnCommitRecieved( const ::google::protobuf::Message* pProtoMsg ) OVERRIDE;
	virtual bool ShouldTrackObject( const CSteamID & steamIDOwner, const CSharedObject *pObject ) const OVERRIDE;
	virtual int CompareRecords( const ::google::protobuf::Message* pNewProtoMsg, const ::google::protobuf::Message* pExistingProtoMsg ) const OVERRIDE;

};

CTFWarTrackerManager* GetWarTrackerManager();

#endif	// TF_SPYENGY_WARTRACKER_H

