//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	Squad classes
//
//=============================================================================//

#include "cbase.h"
#include "ai_squad.h"
#include "ai_squadslot.h"
#include "ai_basenpc.h"
#include "saverestore_bitstring.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

CAI_SquadManager g_AI_SquadManager;

//-----------------------------------------------------------------------------
// CAI_SquadManager
//
// Purpose: Manages all the squads in the system
//
//-----------------------------------------------------------------------------

CAI_Squad *CAI_SquadManager::FindSquad( string_t squadName )
{
	CAI_Squad* pSquad = m_pSquads;

	while (pSquad)
	{
		if (FStrEq(STRING(squadName),pSquad->GetName()))
		{
			return pSquad;
		}
		pSquad = pSquad->m_pNextSquad;
	}
	return NULL;
}

//-------------------------------------

CAI_Squad *CAI_SquadManager::CreateSquad(string_t squadName)
{
	CAI_Squad *pResult = new CAI_Squad(squadName);

	// ---------------------------------
	// Only named squads get added to the squad list
	if ( squadName != NULL_STRING )
	{
		pResult->m_pNextSquad = m_pSquads;
		m_pSquads = pResult;
	}
	else
		pResult->m_pNextSquad = NULL;
	return pResult;
}

//-------------------------------------

int CAI_SquadManager::NumSquads()
{
	int nSquads = 0;
	CAI_Squad* pSquad = m_pSquads;

	while (pSquad)
	{
		nSquads++;
		pSquad = pSquad->GetNext();
	}
	return nSquads;
}

//-------------------------------------

void CAI_SquadManager::DeleteSquad( CAI_Squad *pSquad )
{
	CAI_Squad *pCurSquad = m_pSquads;
	if (pCurSquad == pSquad)
	{
		g_AI_SquadManager.m_pSquads = pCurSquad->m_pNextSquad;
	}
	else
	{
		while (pCurSquad)
		{
			if (pCurSquad->m_pNextSquad == pSquad)
			{
				pCurSquad->m_pNextSquad = pCurSquad->m_pNextSquad->m_pNextSquad;
				break;
			}
			pCurSquad= pCurSquad->m_pNextSquad;
		}
	}
	delete pSquad;
}

//-------------------------------------
// Purpose: Delete all the squads (called between levels / loads)
//-------------------------------------

void CAI_SquadManager::DeleteAllSquads(void) 
{
	CAI_Squad *squad = CAI_SquadManager::m_pSquads;

	while (squad) 
	{
		CAI_Squad *temp = squad->m_pNextSquad;
		delete squad;
		squad = temp;
	}
	CAI_SquadManager::m_pSquads = NULL;
}

#ifdef MAPBASE_VSCRIPT
//-------------------------------------
// Purpose: 
//-------------------------------------
HSCRIPT CAI_SquadManager::ScriptGetFirstSquad()
{
	return m_pSquads ? g_pScriptVM->RegisterInstance( m_pSquads ) : NULL;
}

HSCRIPT CAI_SquadManager::ScriptGetNextSquad( HSCRIPT hStart )
{
	CAI_Squad *pSquad = HScriptToClass<CAI_Squad>( hStart );
	return (pSquad && pSquad->m_pNextSquad) ? g_pScriptVM->RegisterInstance( pSquad->m_pNextSquad ) : NULL;
}

//-------------------------------------
// Purpose: 
//-------------------------------------
HSCRIPT CAI_SquadManager::ScriptFindSquad( const char *squadName )
{
	CAI_Squad *pSquad = FindSquad( MAKE_STRING(squadName) );
	return pSquad ? g_pScriptVM->RegisterInstance( pSquad ) : NULL;
}

HSCRIPT CAI_SquadManager::ScriptFindCreateSquad( const char *squadName )
{
	CAI_Squad *pSquad = FindCreateSquad( MAKE_STRING( squadName ) );
	return pSquad ? g_pScriptVM->RegisterInstance( pSquad ) : NULL;
}

BEGIN_SCRIPTDESC_ROOT( CAI_SquadManager, SCRIPT_SINGLETON "Manager for NPC squads." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstSquad, "GetFirstSquad", "Get the first squad in the squad list." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetNextSquad, "GetNextSquad", "Get the next squad in the squad list starting from the specified squad." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindSquad, "FindSquad", "Find the specified squad in the squad list. Returns null if none found." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptFindCreateSquad, "FindCreateSquad", "Find the specified squad in the squad list or create it if it doesn't exist." )

	DEFINE_SCRIPTFUNC( NumSquads, "Get the number of squads in the list." )

END_SCRIPTDESC();
#endif

//-----------------------------------------------------------------------------
// CAI_Squad
//
// Purpose: Tracks enemies, squad slots, squad members
//
//-----------------------------------------------------------------------------

#ifdef PER_ENEMY_SQUADSLOTS
BEGIN_SIMPLE_DATADESC( AISquadEnemyInfo_t )

	DEFINE_FIELD( hEnemy,	FIELD_EHANDLE ),
	DEFINE_BITSTRING( slots),

END_DATADESC()
#endif

BEGIN_SIMPLE_DATADESC( CAI_Squad )

	// 							m_pNextSquad		(rebuilt)
	// 							m_Name				(rebuilt)
	// 							m_SquadMembers		(rebuilt)
  	// 			 				m_SquadMembers.Count()		(rebuilt)
 	DEFINE_FIELD( m_flSquadSoundWaitTime,		FIELD_TIME ),
 	DEFINE_FIELD( m_nSquadSoundPriority,		FIELD_INTEGER ),
	DEFINE_FIELD( m_hSquadInflictor,			FIELD_EHANDLE ),
	DEFINE_AUTO_ARRAY( m_SquadData,				FIELD_INTEGER ),
 	//							m_pLastFoundEnemyInfo  (think transient)

#ifdef PER_ENEMY_SQUADSLOTS
	DEFINE_UTLVECTOR(m_EnemyInfos,				FIELD_EMBEDDED ),
	DEFINE_FIELD( m_flEnemyInfoCleanupTime,	FIELD_TIME ),
#else
	DEFINE_EMBEDDED( m_squadSlotsUsed ),
#endif

END_DATADESC()

#ifdef MAPBASE_VSCRIPT
BEGIN_SCRIPTDESC_ROOT( CAI_Squad, "NPC squads used for schedule coordination, sharing information about enemies, etc." )

	DEFINE_SCRIPTFUNC( GetName, "Get the squad's name." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptGetFirstMember, "GetFirstMember", "Get the squad's first member. The parameter is for whether to ignore silent members (see CAI_Squad::IsSilentMember() for more info)." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetMember, "GetMember", "Get one of the squad's members by their index." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAnyMember, "GetAnyMember", "Randomly get any one of the squad's members." )
	DEFINE_SCRIPTFUNC( NumMembers, "Get the squad's number of members. The parameter is for whether to ignore silent members (see CAI_Squad::IsSilentMember() for more info)." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSquadIndex, "GetSquadIndex", "Get the index of the specified NPC in the squad." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptUpdateEnemyMemory, "UpdateEnemyMemory", "Updates the squad's memory of an enemy. The first parameter is the updater, the second parameter is the enemy, and the third parameter is the position." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptSquadMemberInRange, "SquadMemberInRange", "Get the first squad member found around the specified position in the specified range." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptNearestSquadMember, "NearestSquadMember", "Get the squad member nearest to the specified member." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetVisibleSquadMembers, "GetVisibleSquadMembers", "Get the number of squad members visible to the specified member." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSquadMemberNearestTo, "GetSquadMemberNearestTo", "Get the squad member nearest to a point." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsMember, "IsMember", "Returns true if the specified NPC is a member of the squad." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsLeader, "IsLeader", "Returns true if the specified NPC is the squad's leader." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetLeader, "GetLeader", "Get the squad's leader." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptAddToSquad, "AddToSquad", "Adds a NPC to the squad." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptRemoveFromSquad, "RemoveFromSquad", "Removes a NPC from the squad." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptIsSilentMember, "IsSilentMember", "Returns true if the specified NPC is a \"silent squad member\", which means it's only in squads for enemy information purposes and does not actually participate in any tactics. For example, this is used for npc_enemyfinder and vital allies (e.g. Alyx) in the player's squad. Please note that this does not check if the NPC is in the squad first." )

	DEFINE_SCRIPTFUNC_NAMED( ScriptSetSquadData, "SetSquadData", "Set the squad data in the specified slot." )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetSquadData, "GetSquadData", "Get the squad data in the specified slot." )

END_SCRIPTDESC();
#endif

//-------------------------------------

CAI_Squad::CAI_Squad(string_t newName) 
#ifndef PER_ENEMY_SQUADSLOTS
 :	m_squadSlotsUsed(MAX_SQUADSLOTS)
#endif
{
	Init( newName );
}

//-------------------------------------

CAI_Squad::CAI_Squad() 
#ifndef PER_ENEMY_SQUADSLOTS
 :	m_squadSlotsUsed(MAX_SQUADSLOTS)
#endif
{
	Init( NULL_STRING );
}

//-------------------------------------

void CAI_Squad::Init(string_t newName) 
{
	m_Name = AllocPooledString( STRING(newName) );
	m_pNextSquad = NULL;
	m_flSquadSoundWaitTime = 0;
	m_SquadMembers.RemoveAll();

	m_flSquadSoundWaitTime	= 0;

	SetSquadInflictor( NULL );

#ifdef PER_ENEMY_SQUADSLOTS
	m_flEnemyInfoCleanupTime = 0;
	m_pLastFoundEnemyInfo = NULL;
#endif

}

//-------------------------------------

CAI_Squad::~CAI_Squad(void)
{
}

//-------------------------------------

bool CAI_Squad::IsSilentMember( const CAI_BaseNPC *pNPC )
{
	if ( !pNPC || ( pNPC->GetMoveType() == MOVETYPE_NONE && pNPC->GetSolid() == SOLID_NONE ) ) // a.k.a., enemy finder
		return true;
	return pNPC->IsSilentSquadMember();
}

//-------------------------------------
// Purpose: Removes an NPC from a squad
//-------------------------------------

void CAI_Squad::RemoveFromSquad( CAI_BaseNPC *pNPC, bool bDeath )
{
	if ( !pNPC )
		return;

	// Find the index of this squad member
	int member;
	int myIndex = m_SquadMembers.Find(pNPC);
	if (myIndex == -1)
	{
		CGMsg( 1, CON_GROUP_NPC_AI, "ERROR: Attempting to remove non-existing squad member!\n" );
		return;
	}
	m_SquadMembers.Remove(myIndex);

	// Notify squad members of death 
	if ( bDeath )
	{
		for (member = 0; member < m_SquadMembers.Count(); member++)
		{
			CAI_BaseNPC* pSquadMem = m_SquadMembers[member];
			if (pSquadMem)
			{
				pSquadMem->NotifyDeadFriend(pNPC);
			}
		}
	}

	pNPC->SetSquad(NULL);
	pNPC->SetSquadName( NULL_STRING );
}

//-------------------------------------
// Purpose: Addes the given NPC to the squad
//-------------------------------------
void CAI_Squad::AddToSquad(CAI_BaseNPC *pNPC)
{
	if ( !pNPC || !pNPC->IsAlive() )
	{
		Assert(0);
		return;
	}

	if ( pNPC->GetSquad() == this )
		return;

	if ( pNPC->GetSquad() )
	{
		pNPC->GetSquad()->RemoveFromSquad(pNPC);
	}

	if (m_SquadMembers.Count() == MAX_SQUAD_MEMBERS)
	{
		CGMsg( 1, CON_GROUP_NPC_AI, "Error!! Squad %s is too big!!! Replacing last member\n", STRING( this->m_Name ) );
		m_SquadMembers.Remove(m_SquadMembers.Count()-1);
	}
	m_SquadMembers.AddToTail(pNPC);
	pNPC->SetSquad( this );
	pNPC->SetSquadName( m_Name );

	if ( m_SquadMembers.Count() > 1 )
	{
		CAI_BaseNPC *pCopyFrom = m_SquadMembers[0];
		CAI_Enemies *pEnemies = pCopyFrom->GetEnemies();
		AIEnemiesIter_t iter;
		AI_EnemyInfo_t *pInfo = pEnemies->GetFirst( &iter );
		while ( pInfo )
		{
			pNPC->UpdateEnemyMemory( pInfo->hEnemy, pInfo->vLastKnownLocation, pCopyFrom );
			pInfo = pEnemies->GetNext( &iter );
		}
	}

}

//-------------------------------------

CAI_BaseNPC *CAI_Squad::SquadMemberInRange( const Vector &vecLocation, float flDist )
{
	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		if (m_SquadMembers[i] != NULL && (vecLocation - m_SquadMembers[i]->GetAbsOrigin() ).Length2D() <= flDist)
			return m_SquadMembers[i];
	}
	return NULL;
}

//-------------------------------------
// Purpose: Returns the nearest squad member to the given squad member
//-------------------------------------

CAI_BaseNPC *CAI_Squad::NearestSquadMember( CAI_BaseNPC *pMember )
{
	float			fBestDist	= MAX_COORD_RANGE;
	CAI_BaseNPC		*fNearestEnt = NULL;
	Vector			fStartLoc = pMember->GetAbsOrigin();
	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		if (m_SquadMembers[i] != NULL)
		{
			float fDist = (fStartLoc - m_SquadMembers[i]->GetAbsOrigin()).Length();
			if (m_SquadMembers[i]	!=	pMember	&&
				fDist				<	fBestDist	)
			{
				fBestDist	= fDist;
				fNearestEnt	= m_SquadMembers[i];
			}
		}
	}
	return fNearestEnt;
}

//-------------------------------------
// Purpose: Return the number of squad members visible to the specified member
//-------------------------------------
int	CAI_Squad::GetVisibleSquadMembers( CAI_BaseNPC *pMember )
{
	int iCount = 0;

	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		// Make sure it's not the specified member
		if ( m_SquadMembers[i] != NULL && pMember != m_SquadMembers[i] )
		{
			if ( pMember->FVisible( m_SquadMembers[i] ) )
			{
				iCount++;
			}
		}
	}

	return iCount;
}

//-------------------------------------
//
//-------------------------------------
CAI_BaseNPC *CAI_Squad::GetSquadMemberNearestTo( const Vector &vecLocation )
{
	CAI_BaseNPC *pNearest = NULL;
	float		flNearest = FLT_MAX;

	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		float flDist;
		flDist = m_SquadMembers[i]->GetAbsOrigin().DistToSqr( vecLocation );

		if( flDist < flNearest )
		{
			flNearest = flDist;
			pNearest = m_SquadMembers[i];
		}
	}

	Assert( pNearest != NULL );
	return pNearest;
}

//-------------------------------------
// Purpose: Returns true if given entity is in the squad
//-------------------------------------
bool CAI_Squad::SquadIsMember( CBaseEntity *pMember )
{
	CAI_BaseNPC *pNPC = pMember->MyNPCPointer();
	if ( pNPC && pNPC->GetSquad() == this )
		return true;

	return false;
}

//-------------------------------------

bool CAI_Squad::IsLeader( CAI_BaseNPC *pNPC )
{
	if ( IsSilentMember( pNPC ) )
		return false;

	if ( !pNPC )
		return false;

	if ( GetLeader() == pNPC )
		return true;

	return false;
}

//-------------------------------------

CAI_BaseNPC *CAI_Squad::GetLeader( void )
{
	CAI_BaseNPC *pLeader = NULL;
	int nSilentMembers = 0;
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		if ( !IsSilentMember( m_SquadMembers[i] ) )
		{
			if ( !pLeader )
				pLeader = m_SquadMembers[i];
		}
		else
		{
			nSilentMembers++;
		}
	}
	return ( m_SquadMembers.Count() - nSilentMembers > 1) ? pLeader : NULL;
}

//-----------------------------------------------------------------------------
CAI_BaseNPC *CAI_Squad::GetFirstMember( AISquadIter_t *pIter, bool bIgnoreSilentMembers )
{
	int i = 0;
	if ( bIgnoreSilentMembers )
	{
		for ( ; i < m_SquadMembers.Count(); i++ )
		{
			if ( !IsSilentMember( m_SquadMembers[i] ) )
				break;
		}
	}

	if ( pIter )
		*pIter = (AISquadIter_t)i;
	if ( i >= m_SquadMembers.Count() )
		return NULL;

	return m_SquadMembers[i];
}

//-------------------------------------

CAI_BaseNPC *CAI_Squad::GetNextMember( AISquadIter_t *pIter, bool bIgnoreSilentMembers )
{
	int &i = (int &)*pIter;
	i++;
	if ( bIgnoreSilentMembers )
	{
		for ( ; i < m_SquadMembers.Count(); i++ )
		{
			if ( !IsSilentMember( m_SquadMembers[i] ) )
				break;
		}
	}

	if ( i >= m_SquadMembers.Count() )
		return NULL;

	return m_SquadMembers[i];
}

//-------------------------------------
// Purpose: Alert everyone in the squad to the presence of a new enmey
//-------------------------------------

int	CAI_Squad::NumMembers( bool bIgnoreSilentMembers )
{
	int nSilentMembers = 0;
	if ( bIgnoreSilentMembers )
	{
		for ( int i = 0; i < m_SquadMembers.Count(); i++ )
		{
			if ( IsSilentMember( m_SquadMembers[i] ) )
				nSilentMembers++;
		}
	}
	return ( m_SquadMembers.Count() - nSilentMembers );
}

//-------------------------------------
// Purpose: Alert everyone in the squad to the presence of a new enmey
//-------------------------------------

void CAI_Squad::SquadNewEnemy( CBaseEntity *pEnemy )
{
	if ( !pEnemy )
	{
		CGMsg( 1, CON_GROUP_NPC_AI, "ERROR: SquadNewEnemy() - pEnemy is NULL!\n" );
		return;
	}

	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		CAI_BaseNPC *pMember = m_SquadMembers[i];
		if (pMember)
		{
			// reset members who aren't activly engaged in fighting (only do this if the NPC's using the squad memory, or it'll fail)
			if ( !pMember->GetEnemy() || 
				 ( pMember->GetEnemy() != pEnemy && 
				   !pMember->HasCondition( COND_SEE_ENEMY) &&
				   gpGlobals->curtime - pMember->GetEnemyLastTimeSeen() > 3.0 ) )
			{
				// give them a new enemy
				if( !hl2_episodic.GetBool() || pMember->IsValidEnemy(pEnemy) )
				{
					pMember->SetEnemy( pEnemy );
				}
				// pMember->SetLastAttackTime( 0 );
			}
		}
	}
}

//-------------------------------------
// Purpose: Broadcast a message to all squad members
// Input:	messageID - generic message handle
//			data - generic data handle
//			sender - who sent the message (NULL by default, if not, will not resend to the sender)
//-------------------------------------

int	CAI_Squad::BroadcastInteraction( int interactionType, void *data, CBaseCombatCharacter *sender )
{
	//Must have a squad
	if ( m_SquadMembers.Count() == 0 )
		return false;

	//Broadcast to all members of the squad
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		CAI_BaseNPC *pMember = m_SquadMembers[i]->MyNPCPointer();
		
		//Validate and don't send again to the sender
		if ( ( pMember != NULL) && ( pMember != sender ) )
		{
			//Send it
			pMember->DispatchInteraction( interactionType, data, sender );
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: is it ok to make a sound of the given priority?  Check for conflicts
// Input  : soundPriority - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Squad::FOkToMakeSound( int soundPriority )
{
	if (gpGlobals->curtime <= m_flSquadSoundWaitTime)
	{
		if ( soundPriority <= m_nSquadSoundPriority )
			return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: A squad member made an exclusive sound.  Keep track so other squad
//			members don't talk over it
// Input  : soundPriority - for sorting
//			time - 
//-----------------------------------------------------------------------------
void CAI_Squad::JustMadeSound( int soundPriority, float time )
{
	m_flSquadSoundWaitTime = time;
	m_nSquadSoundPriority = soundPriority;
}

//-----------------------------------------------------------------------------
// Purpose: Try to get one of a contiguous range of slots
// Input  : slotIDStart - start of slot range
//			slotIDEnd - end of slot range
//			hEnemy - enemy this slot is for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Squad::OccupyStrategySlotRange( CBaseEntity *pEnemy, int slotIDStart, int slotIDEnd, int *pSlot )
{
#ifndef PER_ENEMY_SQUADSLOTS
	// FIXME: combat slots need to be per enemy, not per squad.  
	// As it is, once a squad is occupied it stops making even simple attacks to other things nearby.
	// This code may make soldiers too aggressive
	if (GetLeader() && pEnemy != GetLeader()->GetEnemy())
	{
		*pSlot = SQUAD_SLOT_NONE;
		return true;
	}
#endif

	// If I'm already occupying this slot
	if ( *pSlot >= slotIDStart && *pSlot <= slotIDEnd)
		return true;

	for ( int i = slotIDStart; i <= slotIDEnd; i++ )
	{
		// Check enemy to see if slot already occupied
		if (!IsSlotOccupied(pEnemy, i))
		{
			// Clear any previous spot;
			if (*pSlot != SQUAD_SLOT_NONE)
			{
				// As a debug measure check to see if slot was filled
				if (!IsSlotOccupied(pEnemy, *pSlot))
				{
					CGMsg( 1, CON_GROUP_NPC_AI, "ERROR! Vacating an empty slot!\n" );
				}

				// Free the slot
				VacateSlot(pEnemy, *pSlot);
			}

			// Fill the slot
			OccupySlot(pEnemy, i);
			*pSlot = i;
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------

bool CAI_Squad::IsStrategySlotRangeOccupied( CBaseEntity *pEnemy, int slotIDStart, int slotIDEnd )
{
	for ( int i = slotIDStart; i <= slotIDEnd; i++ )
	{
		if (!IsSlotOccupied(pEnemy, i))
			return false;
	}
	return true;
}

//------------------------------------------------------------------------------

void CAI_Squad::VacateStrategySlot( CBaseEntity *pEnemy, int slot)
{
	// If I wasn't taking up a squad slot I'm done
	if (slot == SQUAD_SLOT_NONE)
		return;

	// As a debug measure check to see if slot was filled
	if (!IsSlotOccupied(pEnemy, slot))
	{
		CGMsg( 1, CON_GROUP_NPC_AI, "ERROR! Vacating an empty slot!\n" );
	}

	// Free the slot
	VacateSlot(pEnemy, slot);
}

//------------------------------------------------------------------------------

void CAI_Squad::UpdateEnemyMemory( CAI_BaseNPC *pUpdater, CBaseEntity *pEnemy, const Vector &position )
{
	//Broadcast to all members of the squad
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		if ( m_SquadMembers[i] != pUpdater )
		{
			m_SquadMembers[i]->UpdateEnemyMemory( pEnemy, position, pUpdater );
		}
	}
}

//------------------------------------------------------------------------------

#ifdef PER_ENEMY_SQUADSLOTS

AISquadEnemyInfo_t *CAI_Squad::FindEnemyInfo( CBaseEntity *pEnemy )
{
	int i;
	if ( gpGlobals->curtime > m_flEnemyInfoCleanupTime )
	{
		if ( m_EnemyInfos.Count() )
		{
			m_pLastFoundEnemyInfo = NULL;
			CUtlRBTree<CBaseEntity *> activeEnemies;
			SetDefLessFunc( activeEnemies );

			// Gather up the set of active enemies
			for ( i = 0; i < m_SquadMembers.Count(); i++ )
			{
				CBaseEntity *pMemberEnemy = m_SquadMembers[i]->GetEnemy();
				if ( pMemberEnemy && activeEnemies.Find( pMemberEnemy ) == activeEnemies.InvalidIndex() )
				{
					activeEnemies.Insert( pMemberEnemy );
				}
			}
			
			// Remove the records for deleted or unused enemies
			for ( i = m_EnemyInfos.Count() - 1; i >= 0; --i )
			{
				if ( m_EnemyInfos[i].hEnemy == NULL || activeEnemies.Find( m_EnemyInfos[i].hEnemy ) == activeEnemies.InvalidIndex() )
				{
					m_EnemyInfos.FastRemove( i );
				}
			}
		}
		
		m_flEnemyInfoCleanupTime = gpGlobals->curtime + 30;
	}

	if ( m_pLastFoundEnemyInfo && m_pLastFoundEnemyInfo->hEnemy == pEnemy )
		return m_pLastFoundEnemyInfo;

	for ( i = 0; i < m_EnemyInfos.Count(); i++ )
	{
		if ( m_EnemyInfos[i].hEnemy == pEnemy )
		{
			m_pLastFoundEnemyInfo = &m_EnemyInfos[i];
			return &m_EnemyInfos[i];
		}
	}

	m_pLastFoundEnemyInfo = NULL;
	i = m_EnemyInfos.AddToTail();
	m_EnemyInfos[i].hEnemy = pEnemy;

	m_pLastFoundEnemyInfo = &m_EnemyInfos[i];
	return &m_EnemyInfos[i];
}

#endif
	
//------------------------------------------------------------------------------

void CAI_Squad::OccupySlot( CBaseEntity *pEnemy, int i )			
{ 
#ifdef PER_ENEMY_SQUADSLOTS
	AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	pInfo->slots.Set(i);
#else
	m_squadSlotsUsed.Set(i); 
#endif
}

//------------------------------------------------------------------------------

void CAI_Squad::VacateSlot( CBaseEntity *pEnemy, int i )			
{ 
#ifdef PER_ENEMY_SQUADSLOTS
	AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	pInfo->slots.Clear(i);
#else
	m_squadSlotsUsed.Clear(i); 
#endif
}

//------------------------------------------------------------------------------

bool CAI_Squad::IsSlotOccupied( CBaseEntity *pEnemy, int i ) const	
{ 
#ifdef PER_ENEMY_SQUADSLOTS
	const AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	return pInfo->slots.IsBitSet(i);
#else
	return m_squadSlotsUsed.IsBitSet(i); 
#endif
}

void CAI_Squad::SquadRemember( int iMemory )
{
	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		if (m_SquadMembers[i] != NULL )
		{
			m_SquadMembers[i]->Remember( iMemory );
		}
	}
}

//------------------------------------------------------------------------------
void CAI_Squad::SetSquadInflictor( CBaseEntity *pInflictor )
{
	m_hSquadInflictor.Set(pInflictor);
}

//------------------------------------------------------------------------------
bool CAI_Squad::IsSquadInflictor( CBaseEntity *pInflictor )
{
	return (m_hSquadInflictor.Get() == pInflictor);
}

#ifdef MAPBASE_VSCRIPT
//------------------------------------------------------------------------------

// Functions tailored specifically for VScript.

HSCRIPT		CAI_Squad::ScriptGetFirstMember( bool bIgnoreSilentMembers ) { return ToHScript( GetFirstMember( NULL, bIgnoreSilentMembers ) ); }
HSCRIPT		CAI_Squad::ScriptGetMember( int iIndex ) { return iIndex < m_SquadMembers.Count() ? ToHScript( m_SquadMembers[iIndex] ) : NULL; }
HSCRIPT		CAI_Squad::ScriptGetAnyMember() { return ToHScript( GetAnyMember() ); }
//int		CAI_Squad::ScriptNumMembers( bool bIgnoreSilentMembers ) { return NumMembers( bIgnoreSilentMembers ); }
int			CAI_Squad::ScriptGetSquadIndex( HSCRIPT hNPC ) { return GetSquadIndex( HScriptToClass<CAI_BaseNPC>( hNPC ) ); }

void		CAI_Squad::ScriptUpdateEnemyMemory( HSCRIPT hUpdater, HSCRIPT hEnemy, const Vector &position ) { UpdateEnemyMemory( HScriptToClass<CAI_BaseNPC>( hUpdater ), ToEnt( hEnemy ), position ); }

HSCRIPT		CAI_Squad::ScriptSquadMemberInRange( const Vector &vecLocation, float flDist ) { return ToHScript( SquadMemberInRange( vecLocation, flDist ) ); }
HSCRIPT		CAI_Squad::ScriptNearestSquadMember( HSCRIPT hMember ) { return ToHScript( NearestSquadMember( HScriptToClass<CAI_BaseNPC>( hMember ) ) ); }
int			CAI_Squad::ScriptGetVisibleSquadMembers( HSCRIPT hMember ) { return GetVisibleSquadMembers( HScriptToClass<CAI_BaseNPC>( hMember ) ); }
HSCRIPT		CAI_Squad::ScriptGetSquadMemberNearestTo( const Vector &vecLocation ) { return ToHScript( GetSquadMemberNearestTo( vecLocation ) ); }
bool		CAI_Squad::ScriptIsMember( HSCRIPT hMember ) { return SquadIsMember( HScriptToClass<CAI_BaseNPC>( hMember ) ); }
bool		CAI_Squad::ScriptIsLeader( HSCRIPT hLeader ) { return IsLeader( HScriptToClass<CAI_BaseNPC>( hLeader ) ); }
HSCRIPT		CAI_Squad::ScriptGetLeader( void ) { return ToHScript( GetLeader() ); }

void		CAI_Squad::ScriptAddToSquad( HSCRIPT hNPC ) { AddToSquad( HScriptToClass<CAI_BaseNPC>( hNPC ) ); }
void		CAI_Squad::ScriptRemoveFromSquad( HSCRIPT hNPC ) { RemoveFromSquad( HScriptToClass<CAI_BaseNPC>( hNPC ), false ); }

bool		CAI_Squad::ScriptIsSilentMember( HSCRIPT hNPC ) { return IsSilentMember( HScriptToClass<CAI_BaseNPC>( hNPC ) ); }

void CAI_Squad::ScriptSetSquadData( int iSlot, const char *data )
{
	SetSquadData( iSlot, data );
}

const char *CAI_Squad::ScriptGetSquadData( int iSlot )
{
	const char *data;
	GetSquadData( iSlot, &data );
	return data;
}
#endif

//=============================================================================

