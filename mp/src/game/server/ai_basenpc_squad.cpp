//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_hull.h"
#include "ai_squadslot.h"
#include "ai_squad.h"
#include "bitstring.h"
#include "entitylist.h"
#include "ai_hint.h"
#include "IEffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: If requested slot is available return true and take the slot
//			Otherwise return false
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::OccupyStrategySlot( int squadSlotID )
{
	return OccupyStrategySlotRange( squadSlotID, squadSlotID );
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::OccupyStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I'm not in a squad a I don't fill slots
	return ( !m_pSquad || m_pSquad->OccupyStrategySlotRange( GetEnemy(), slotIDStart, slotIDEnd, &m_iMySquadSlot ) );

}

//-----------------------------------------------------------------------------
// Returns true if all in the range are full
//-----------------------------------------------------------------------------
bool CAI_BaseNPC::IsStrategySlotRangeOccupied( int slotIDStart, int slotIDEnd )
{
	return m_pSquad && m_pSquad->IsStrategySlotRangeOccupied( GetEnemy(), slotIDStart, slotIDEnd );
}


//=========================================================
// HasStrategySlot 
//=========================================================
bool CAI_BaseNPC::HasStrategySlot( int squadSlotID )
{
	// If I wasn't taking up a squad slot I'm done
	return (m_iMySquadSlot == squadSlotID);
}

bool CAI_BaseNPC::HasStrategySlotRange( int slotIDStart, int slotIDEnd )
{
	// If I wasn't taking up a squad slot I'm done
	if (m_iMySquadSlot < slotIDStart || m_iMySquadSlot > slotIDEnd)
	{
		return false;
	}
	return true;
}

//=========================================================
// VacateSlot 
//=========================================================

void CAI_BaseNPC::VacateStrategySlot(void)
{
	if (m_pSquad)
	{
		m_pSquad->VacateStrategySlot(GetEnemy(), m_iMySquadSlot);
		m_iMySquadSlot = SQUAD_SLOT_NONE;
	}
}


//------------------------------------------------------------------------------
// Purpose :  Is cover node valid
// Input   :
// Output  :
//------------------------------------------------------------------------------
bool CAI_BaseNPC::IsValidCover( const Vector &vecCoverLocation, CAI_Hint const *pHint )
{
	// firstly, limit choices to hint groups
	string_t iszHint = GetHintGroup();
	char *pszHint = (char *)STRING(iszHint);
	if ((iszHint != NULL_STRING) && (pszHint[0] != '\0'))
	{
		if (!pHint || pHint->GetGroup() != GetHintGroup())
		{
			return false;
		}
	}

	/*
	// If I'm in a squad don't pick cover node it other squad member
	// is already nearby
	if (m_pSquad)
	{
		return m_pSquad->IsValidCover( vecCoverLocation, pHint );
	}
	*/
	
	// UNDONE: Do we really need this test?
	// ----------------------------------------------------------------
	// Make sure my hull can fit at this node before accepting it. 
	// Could be another NPC there or it could be blocked
	// ----------------------------------------------------------------
	// FIXME: shouldn't this see that if I crouch behind it it'll be safe?
	Vector startPos = vecCoverLocation;
	startPos.z -= GetHullMins().z;  // Move hull bottom up to node
	Vector endPos	= startPos;
	endPos.z += 0.01;
	trace_t tr;
	AI_TraceEntity( this, vecCoverLocation, endPos, MASK_NPCSOLID, &tr );
	if (tr.startsolid)
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Is squad member in my way from shooting here
// Input  :
// Output :
//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsValidShootPosition( const Vector &vecShootLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	// limit choices to hint groups
	if (GetHintGroup() != NULL_STRING)
	{
		if (!pHint || pHint->GetGroup() != GetHintGroup())
		{
			if ( ( vecShootLocation - GetAbsOrigin() ).Length2DSqr() > 1 )
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------

bool CAI_BaseNPC::IsSquadmateInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter )
{
	if( !m_pSquad ) 
		return false;

	AISquadIter_t iter;

	CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		// Ignore squadmates that can't take damage. This is primarily to ignore npc_enemyfinders.
		if ( pSquadmate->m_takedamage != DAMAGE_NO )
		{
			if ( pSquadmate != this )
			{
				if ( PointInSpread( pSquadmate, sourcePos, targetPos, pSquadmate->GetAbsOrigin(), flSpread, maxDistOffCenter ) )
					return true;
			}
		}
		pSquadmate = m_pSquad->GetNextMember( &iter );
	}
	return false;
}


//-----------------------------------------------------------------------------

void CAI_BaseNPC::AddToSquad( string_t name )
{
	g_AI_SquadManager.FindCreateSquad( this, name );
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::SetSquad( CAI_Squad *pSquad )	
{ 
	if ( m_pSquad == pSquad )
	{
		return;
	}

	if ( m_pSquad && m_iMySquadSlot != SQUAD_SLOT_NONE)
	{
		VacateStrategySlot();
	}

	m_pSquad = pSquad; 	
}

//-----------------------------------------------------------------------------

void CAI_BaseNPC::RemoveFromSquad()
{
	if ( m_pSquad )
	{
		m_pSquad->RemoveFromSquad( this, false );
		m_pSquad = NULL;
	}
}

//-----------------------------------------------------------------------------
void CAI_BaseNPC::CheckSquad()
{
	if( !IsInSquad() )
		return;

	if( !GetSquad()->IsLeader(this) )
		return;

	if( VPhysicsGetObject() != NULL && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD) )
	{
		// I AM the leader, and I'm currently being held. This will screw up all of my relationship checks
		// if I'm a manhack or a rollermine, so just bomb out and try next time.
		return;
	}

	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		if( IRelationType(pSquadmate) < D_LI )
		{
			bool bWarn = true;

			// Rollermines and manhacks set their Class to NONE when held by the player, which makes all of 
			// their squadmates complain that an enemy is in the squad. Suppress this.
			if( pSquadmate->VPhysicsGetObject() != NULL )
			{
				if (pSquadmate->VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD)
				{
					bWarn = false;
				}
			}	

			if( bWarn )
			{
				Warning( "ERROR: Squad '%s' has enemies in it!\n", GetSquad()->GetName() );
				Warning( "%s doesn't like %s\n\n", GetDebugName(), pSquadmate->GetDebugName() );
			}
		}

		pSquadmate = m_pSquad->GetNextMember( &iter );
	}
}

//-----------------------------------------------------------------------------
// Returns the number of weapons of this type currently owned by squad members.
//-----------------------------------------------------------------------------
int CAI_BaseNPC::NumWeaponsInSquad( const char *pszWeaponClassname )
{
	string_t iszWeaponClassname = FindPooledString( pszWeaponClassname );

	if( !GetSquad() )
	{
		if( GetActiveWeapon() && GetActiveWeapon()->m_iClassname == iszWeaponClassname )
		{
			// I'm alone in my squad, but I do have this weapon.
			return 1;
		}

		return 0;
	}

	int count = 0;
	AISquadIter_t iter;
	CAI_BaseNPC *pSquadmate = m_pSquad->GetFirstMember( &iter );
	while ( pSquadmate )
	{
		if( pSquadmate->GetActiveWeapon() && pSquadmate->GetActiveWeapon()->m_iClassname == iszWeaponClassname )
		{
			count++;
		}
		pSquadmate = m_pSquad->GetNextMember( &iter );
	}

	return count;
}
