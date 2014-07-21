//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	Squad classes
//
//=============================================================================//

#ifndef AI_SQUAD_H
#define AI_SQUAD_H

#include "ai_memory.h"
#include "ai_squadslot.h"
#include "bitstring.h"

class CAI_Squad;
typedef CHandle<CAI_BaseNPC> AIHANDLE;

#define PER_ENEMY_SQUADSLOTS 1


//-----------------------------------------------------------------------------

DECLARE_POINTER_HANDLE(AISquadsIter_t);
DECLARE_POINTER_HANDLE(AISquadIter_t);

#define	MAX_SQUAD_MEMBERS	16
#define MAX_SQUAD_DATA_SLOTS 4

//-----------------------------------------------------------------------------
// CAI_SquadManager
//
// Purpose: Manages all the squads in the system
//
//-----------------------------------------------------------------------------

class CAI_SquadManager
{
public:
	CAI_SquadManager()
	{
		m_pSquads = NULL;
	}

	CAI_Squad *		GetFirstSquad( AISquadsIter_t *pIter );
	CAI_Squad *		GetNextSquad( AISquadsIter_t *pIter );
	int				NumSquads();

	CAI_Squad *		FindSquad( string_t squadName );	// Returns squad of the given name
	CAI_Squad *		CreateSquad( string_t squadName );	// Returns squad of the given name
	CAI_Squad *		FindCreateSquad( string_t squadName );	// Returns squad of the given name
	CAI_Squad *		FindCreateSquad( CAI_BaseNPC *pNPC, string_t squadName );	// Returns squad of the given name

	void			DeleteSquad( CAI_Squad *pSquad );
	void			DeleteAllSquads(void);

private:

	CAI_Squad *		m_pSquads;										// A linked list of all squads

};

//-------------------------------------

extern CAI_SquadManager g_AI_SquadManager;

//-----------------------------------------------------------------------------

#ifdef PER_ENEMY_SQUADSLOTS

struct AISquadEnemyInfo_t
{
	EHANDLE 						hEnemy;
	CBitVec<MAX_SQUADSLOTS>	slots;									// What squad slots are filled?

	DECLARE_SIMPLE_DATADESC();
};

#endif

//-----------------------------------------------------------------------------
// CAI_Squad
//
// Purpose: Tracks enemies, squad slots, squad members
//
//-----------------------------------------------------------------------------

class CAI_Squad
{
public:

	const char *			GetName() const	{ return STRING(m_Name); }

	void					RemoveFromSquad( CAI_BaseNPC *pNPC, bool bDeath = false );

	CAI_BaseNPC *			GetFirstMember( AISquadIter_t *pIter = NULL, bool bIgnoreSilentMembers = true );
	CAI_BaseNPC *			GetNextMember( AISquadIter_t *pIter, bool bIgnoreSilentMembers = true );
	CAI_BaseNPC *			GetAnyMember();
	int						NumMembers( bool bIgnoreSilentMembers = true );
	int						GetSquadIndex( CAI_BaseNPC * );

	void					SquadNewEnemy ( CBaseEntity *pEnemy );
	void					UpdateEnemyMemory( CAI_BaseNPC *pUpdater, CBaseEntity *pEnemy, const Vector &position );

	bool 					OccupyStrategySlotRange( CBaseEntity *pEnemy, int slotIDStart, int slotIDEnd, int *pSlot );
	void 					VacateStrategySlot( CBaseEntity *pEnemy, int slot);
	bool					IsStrategySlotRangeOccupied( CBaseEntity *pEnemy, int slotIDStart, int slotIDEnd );
	
	CAI_BaseNPC	*			SquadMemberInRange( const Vector &vecLocation, float flDist );
	CAI_BaseNPC *			NearestSquadMember( CAI_BaseNPC *pMember );
	int						GetVisibleSquadMembers( CAI_BaseNPC *pMember );
	CAI_BaseNPC *			GetSquadMemberNearestTo( const Vector &vecLocation );
	bool					SquadIsMember( CBaseEntity *pMember );
	bool					IsLeader( CAI_BaseNPC *pLeader );
	CAI_BaseNPC				*GetLeader( void );

	int						BroadcastInteraction( int interactionType, void *data, CBaseCombatCharacter *sender = NULL );

	void					AddToSquad(CAI_BaseNPC *pNPC);
	bool					FOkToMakeSound( int soundPriority );
	void					JustMadeSound( int soundPriority, float time );
	float					GetSquadSoundWaitTime() const		{ return m_flSquadSoundWaitTime; }
	void					SetSquadSoundWaitTime( float time ) { m_flSquadSoundWaitTime = time; }
	void					SquadRemember( int iMemory );

	void					SetSquadInflictor( CBaseEntity *pInflictor );
	bool					IsSquadInflictor( CBaseEntity *pInflictor );

	static bool				IsSilentMember( const CAI_BaseNPC *pNPC );

	template <typename T>
	void					SetSquadData( unsigned slot, const T &data )
	{
		Assert( slot < MAX_SQUAD_DATA_SLOTS );
		if ( slot < MAX_SQUAD_DATA_SLOTS )
		{
			m_SquadData[slot] = *((int *)&data);
		}
	}

	template <typename T>
	void					GetSquadData( unsigned slot, T *pData )
	{
		Assert( slot < MAX_SQUAD_DATA_SLOTS );
		if ( slot < MAX_SQUAD_DATA_SLOTS )
		{
			*pData = *((T *)&m_SquadData[slot]);
		}
	}


private:
	void OccupySlot( CBaseEntity *pEnemy, int i );
	void VacateSlot( CBaseEntity *pEnemy, int i );
	bool IsSlotOccupied( CBaseEntity *pEnemy, int i ) const;

private:
	friend class CAI_SaveRestoreBlockHandler;
	friend class CAI_SquadManager;

	CAI_Squad();
	CAI_Squad(string_t squadName);
	~CAI_Squad(void);

	CAI_Squad*				GetNext() { return m_pNextSquad; }

	void Init( string_t squadName );

	CAI_Squad *										m_pNextSquad;								// The next squad is list of all squads

	string_t										m_Name;
	CUtlVectorFixed<AIHANDLE, MAX_SQUAD_MEMBERS>	m_SquadMembers;

	float											m_flSquadSoundWaitTime;			// Time when I'm allowed to make another sound
	int												m_nSquadSoundPriority;			// if we're still waiting, this is the priority of the current sound

	EHANDLE											m_hSquadInflictor;

	int												m_SquadData[MAX_SQUAD_DATA_SLOTS];

#ifdef PER_ENEMY_SQUADSLOTS

	AISquadEnemyInfo_t *FindEnemyInfo( CBaseEntity *pEnemy );
	const AISquadEnemyInfo_t *FindEnemyInfo( CBaseEntity *pEnemy ) const	{ return const_cast<CAI_Squad *>(this)->FindEnemyInfo( pEnemy ); }

	AISquadEnemyInfo_t *			m_pLastFoundEnemyInfo; // Occupy/Vacate need to be reworked to not want this
	
	CUtlVector<AISquadEnemyInfo_t>	m_EnemyInfos;
	float							m_flEnemyInfoCleanupTime;

#else
	
	CVarBitVec	m_squadSlotsUsed;							// What squad slots are filled?

#endif

	//---------------------------------
public:
	DECLARE_SIMPLE_DATADESC();
};

//-----------------------------------------------------------------------------
//
// Purpose: CAI_SquadManager inline functions
//
//-----------------------------------------------------------------------------

inline CAI_Squad *CAI_SquadManager::GetFirstSquad( AISquadsIter_t *pIter )
{
	*pIter = (AISquadsIter_t)m_pSquads;
	return m_pSquads;
}

//-------------------------------------

inline CAI_Squad *CAI_SquadManager::GetNextSquad( AISquadsIter_t *pIter )
{
	CAI_Squad *pSquad = (CAI_Squad *)*pIter;
	if ( pSquad )
		pSquad = pSquad->m_pNextSquad;
	*pIter = (AISquadsIter_t)pSquad;
	return pSquad;
}

//-------------------------------------
// Purpose: Returns squad of the given name or creates a new squad with the
//			given name if none exists and add pNPC to the list of members
//-------------------------------------

inline CAI_Squad *CAI_SquadManager::FindCreateSquad(CAI_BaseNPC *pNPC, string_t squadName)
{
	CAI_Squad* pSquad = FindSquad( squadName );
	
	if ( !pSquad )
		pSquad = CreateSquad( squadName );
	
	pSquad->AddToSquad( pNPC );

	return pSquad;
}

//-----------------------------------------------------------------------------

inline CAI_Squad *CAI_SquadManager::FindCreateSquad(string_t squadName)
{
	CAI_Squad* pSquad = FindSquad( squadName );
	
	if ( !pSquad )
		pSquad = CreateSquad( squadName );
	
	return pSquad;
}

//-------------------------------------

inline CAI_BaseNPC *CAI_Squad::GetAnyMember()
{
	if ( m_SquadMembers.Count() )
		return m_SquadMembers[random->RandomInt( 0, m_SquadMembers.Count()-1 )];
	return NULL;
}

//-------------------------------------

inline int CAI_Squad::GetSquadIndex( CAI_BaseNPC *pAI )
{
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		if ( m_SquadMembers[i] == pAI )
			return i;
	}
	return -1;
}


//-----------------------------------------------------------------------------

#endif // AI_SQUAD_H
