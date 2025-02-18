//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SHARED_OBJECT_MANAGER_H
#define SHARED_OBJECT_MANAGER_H

#include "GameEventListener.h"
#include "econ_item_constants.h"
#include "econ_item_inventory.h"

#ifdef GAME_DLL
	#include "tf_player.h"
#else
	#include "c_tf_player.h"
	#include "local_steam_shared_object_listener.h"
#endif


#if defined( _WIN32 )
#pragma once
#endif

using namespace GCSDK;
class CSOTrackerManager;

extern short g_nQuestSpewFlags;
#define SO_TRACKER_SPEW_OBJECTIVES 1<<0
#define SO_TRACKER_SPEW_ITEM_TRACKER_MANAGEMENT 1<<1
#define SO_TRACKER_SPEW_GC_COMMITS 1<<2
#define SO_TRACKER_SPEW_OBJECTIVE_TRACKER_MANAGEMENT 1<<3
#define SO_TRACKER_SPEW_SOCACHE_ACTIVITY 1<<4
#define SO_TRACKER_SPEW_TRACKER_ACCEPTANCE 1<<5
void SOTrackerSpew( const char* pszBuff, int nType );
#define SO_TRACKER_SPEW( pszBuff, nType ) SOTrackerSpew( pszBuff, nType );		

class CBaseSOTracker
{
public:
	DECLARE_CLASS_NOBASE( CBaseSOTracker )

	CBaseSOTracker( const CSharedObject* pSObject, CSteamID steamIDOwner, CSOTrackerManager* pManager );
	virtual ~CBaseSOTracker();

	const CSharedObject* GetSObject() const			{ return m_pSObject; }
	const CSteamID		 GetOwnerSteamID() const	{ return m_steamIDOwner; }
	const CTFPlayer*	 GetTrackedPlayer() const	{ return ToTFPlayer( GetPlayerBySteamID( m_steamIDOwner ) ); }
	virtual void Spew() const;

	virtual void		 CommitChangesToDB() = 0;
	virtual	void		 OnUpdate() = 0;
	virtual void		 OnRemove();
protected:

	const CSharedObject* m_pSObject;
	CSteamID m_steamIDOwner;
	CSOTrackerManager* m_pManager;
};

struct CommitRecord_t
{
	CommitRecord_t( ::google::protobuf::Message* pMessage ) 
		: m_flLastCommitTime( Plat_FloatTime() )
		, m_flReportedTime( Plat_FloatTime() )
		, m_pProtoMsg( pMessage )
	{}

	~CommitRecord_t()
	{
		if ( m_pProtoMsg )
			delete m_pProtoMsg;
	}

	double m_flLastCommitTime;
	double m_flReportedTime;
	::google::protobuf::Message* m_pProtoMsg;

private:
	CommitRecord_t(); // Nope
};
typedef CUtlMap< uint64, CommitRecord_t* > CommitsMap_t;

// A class to handle the creation and deletion of shared object trackers. Automatically
// subscribes to the local player's SOCache and will subscribe to any connecting players'
// SOCaches when they connect.
#ifdef GAME_DLL
class CSOTrackerManager : public ISharedObjectListener, public CGameEventListener, public CAutoGameSystemPerFrame
#else
class CSOTrackerManager : public CLocalSteamSharedObjectListener, public CGameEventListener, public CAutoGameSystemPerFrame
#endif
{
public:

	DECLARE_CLASS_NOBASE( CSOTrackerManager )

	typedef CUtlMap< uint64, CBaseSOTracker * > SOTrackerMap_t;

	CSOTrackerManager();
	virtual ~CSOTrackerManager();

	virtual void Initialize();
	virtual void Shutdown();

	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;
	void EnsureTrackersForPlayer( const CSteamID& steamIDPlayer );
	void EnsureTrackersForPlayer( CTFPlayer* pPlayer );

	void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	void PreSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	void PostSOUpdate( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE {};
	void SODestroyed( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	void SOCacheSubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE;
	void SOCacheUnsubscribed( const CSteamID & steamIDOwner, ESOCacheEvent eEvent ) OVERRIDE;

#ifdef GAME_DLL
	virtual void FrameUpdatePreEntityThink() OVERRIDE;
	void AddCommitRecord( const ::google::protobuf::Message* pRecord, uint64 nKey, bool bRequireResponse );
	void AcknowledgeCommit( const ::google::protobuf::Message* pRecord, uint64 nKey );
	void DBG_SpewPendingCommits();
#endif
	void Spew();


	virtual SOTrackerMap_t::KeyType_t GetKeyForObjectTracker( const CSharedObject* pItem, CSteamID steamIDOwner ) = 0;
	CBaseSOTracker* GetTracker( SOTrackerMap_t::KeyType_t nKey ) const;
	template< class T >
	T GetTypedTracker( SOTrackerMap_t::KeyType_t nKey ) const { return assert_cast< T >( GetTracker( nKey ) ); }
	CommitRecord_t* GetCommitRecord( CommitsMap_t::KeyType_t );
protected:

	enum ETrackerHandling_t
	{
		TRACKER_CREATE_OR_UPDATE = 0,
		TRACKER_REMOVE,
	};

	void UpdateTrackerForItem( const CSharedObject* pItem, ETrackerHandling_t eHandling, CSteamID steamIDOwner );

private:

	virtual int GetType() const = 0;
	virtual const char* GetName() const = 0;
	virtual CFmtStr GetDebugObjectDescription( const CSharedObject* pItem ) const = 0;
	virtual CBaseSOTracker* AllocateNewTracker( const CSharedObject* pItem, CSteamID steamIDOwner, CSOTrackerManager* pManager ) const = 0;
	virtual ::google::protobuf::Message* AllocateNewProtoMessage() const = 0;
	virtual void OnCommitRecieved( const ::google::protobuf::Message* pProtoMsg ) = 0;
	virtual bool ShouldTrackObject( const CSteamID & steamIDOwner, const CSharedObject *pObject ) const = 0;
	virtual int CompareRecords( const ::google::protobuf::Message* pNewProtoMsg, const ::google::protobuf::Message* pExistingProtoMsg ) const = 0;

	void HandleSOEvent( const CSteamID & steamIDOwner, const CSharedObject *pObject, ETrackerHandling_t eHandling );
	void CommitAllChanges();
	void CreateAndAddTracker( const CSharedObject* pItem, CSteamID steamIDOwner );
	void RemoveAndDeleteTrackerAtIndex( SOTrackerMap_t::IndexType_t idx );
	void RemoveTrackersForSteamID( const CSteamID & steamIDOwner );

#ifdef GAME_DLL
	void CommitRecord( CommitRecord_t* pRecord ) const;
	virtual void SendMessageForCommit( const ::google::protobuf::Message* pProtoMessage ) const = 0;
#endif

	double m_flLastUnacknowledgeCommitTime;
	CommitsMap_t m_mapUnacknowledgedCommits;
	SOTrackerMap_t m_mapItemTrackers;
};



#endif // SHARED_OBJECT_MANAGER_H
