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
		DevMsg("ERROR: Attempting to remove non-existing squad membmer!\n");
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
		DevMsg("Error!! Squad %s is too big!!! Replacing last member\n", STRING( this->m_Name ));
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
		*pIter = (AISquadIter_t)(intp)i;
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
		DevMsg( "ERROR: SquadNewEnemy() - pEnemy is NULL!\n" );
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
					DevMsg( "ERROR! Vacating an empty slot!\n");
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
		DevMsg( "ERROR! Vacating an empty slot!\n");
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

//=============================================================================

