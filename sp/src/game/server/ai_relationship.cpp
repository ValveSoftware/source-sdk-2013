//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Entity which alters the relationships between entities via entity I/O
//
//=====================================================================================//

#include "cbase.h"
#include "ndebugoverlay.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_RELATIONSHIP_NOTIFY_SUBJECT	(1<<0)	// Alert the subject of the change and give them a memory of the target entity
#define SF_RELATIONSHIP_NOTIFY_TARGET	(1<<1)	// Alert the target of the change and give them a memory of the subject entity

enum
{
	NOT_REVERTING,
	REVERTING_TO_PREV,
	REVERTING_TO_DEFAULT,
};

//=========================================================
//=========================================================
class CAI_Relationship : public CBaseEntity, public IEntityListener
{
	DECLARE_CLASS( CAI_Relationship, CBaseEntity );

public:
	CAI_Relationship() : m_iPreviousDisposition( -1 )  { }

	void Spawn();
	void Activate();

	void SetActive( bool bActive );
	void ChangeRelationships( int disposition, int iReverting, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	void ApplyRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void RevertRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void RevertToDefaultRelationship( CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );

	void UpdateOnRemove();
	void OnRestore();

	bool IsASubject( CBaseEntity *pEntity );
	bool IsATarget( CBaseEntity *pEntity );

	void OnEntitySpawned( CBaseEntity *pEntity );
	void OnEntityDeleted( CBaseEntity *pEntity );

private:

	void	ApplyRelationshipThink( void );
	CBaseEntity *FindEntityForProceduralName( string_t iszName, CBaseEntity *pActivator, CBaseEntity *pCaller );
	void	DiscloseNPCLocation( CBaseCombatCharacter *pSubject, CBaseCombatCharacter *pTarget );

	string_t	m_iszSubject;
	int			m_iDisposition;
	int			m_iRank;
	bool		m_fStartActive;
	bool		m_bIsActive;
	int			m_iPreviousDisposition;
	float		m_flRadius;
	int			m_iPreviousRank;
	bool		m_bReciprocal;

public:
	// Input functions
	void InputApplyRelationship( inputdata_t &inputdata );
	void InputRevertRelationship( inputdata_t &inputdata );
	void InputRevertToDefaultRelationship( inputdata_t &inputdata );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( ai_relationship, CAI_Relationship );

BEGIN_DATADESC( CAI_Relationship )
	DEFINE_THINKFUNC( ApplyRelationshipThink ),
	
	DEFINE_KEYFIELD( m_iszSubject, FIELD_STRING, "subject" ),
	DEFINE_KEYFIELD( m_iDisposition, FIELD_INTEGER, "disposition" ),
	DEFINE_KEYFIELD( m_iRank, FIELD_INTEGER, "rank" ),
	DEFINE_KEYFIELD( m_fStartActive, FIELD_BOOLEAN, "StartActive" ),
	DEFINE_FIELD( m_bIsActive, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "radius" ),
	DEFINE_FIELD( m_iPreviousDisposition, FIELD_INTEGER ),
	DEFINE_FIELD( m_iPreviousRank, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_bReciprocal, FIELD_BOOLEAN, "reciprocal" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "ApplyRelationship", InputApplyRelationship ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RevertRelationship", InputRevertRelationship ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RevertToDefaultRelationship", InputRevertToDefaultRelationship ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_Relationship::Spawn()
{
	m_bIsActive = false;

	if (m_iszSubject == NULL_STRING)
	{
		DevWarning("ai_relationship '%s' with no subject specified, removing.\n", GetDebugName());
		UTIL_Remove(this);
	}
	else if (m_target == NULL_STRING)
	{
		DevWarning("ai_relationship '%s' with no target specified, removing.\n", GetDebugName());
		UTIL_Remove(this);
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::Activate()
{
	if ( m_fStartActive )
	{
		ApplyRelationship();

		// Clear this flag so that nothing happens when the level is loaded (which calls activate again)
		m_fStartActive = false;
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bActive - 
//-----------------------------------------------------------------------------
void CAI_Relationship::SetActive( bool bActive )
{
	if ( bActive && !m_bIsActive )
	{
		// Start getting entity updates!
		gEntList.AddListenerEntity( this );
	}
	else if ( !bActive && m_bIsActive )
	{
		// Stop getting entity updates!
		gEntList.RemoveListenerEntity( this );
	}

	m_bIsActive = bActive;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::InputApplyRelationship( inputdata_t &inputdata )
{
	ApplyRelationship( inputdata.pActivator, inputdata.pCaller );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::InputRevertRelationship( inputdata_t &inputdata )
{
	RevertRelationship( inputdata.pActivator, inputdata.pCaller );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::InputRevertToDefaultRelationship( inputdata_t &inputdata )
{
	RevertToDefaultRelationship( inputdata.pActivator, inputdata.pCaller );
}

//-----------------------------------------------------------------------------
// Purpose: This think function is used to wait until the player has properly
//			spawned, after all the NPCs have spawned.  Once that occurs, this
//			function terminates.
//-----------------------------------------------------------------------------
void CAI_Relationship::ApplyRelationshipThink( void )
{
	// Call down to the base until the player has properly spawned
	ApplyRelationship();
}

//---------------------------------------------------------
// Purpose: Applies the desired relationships to an entity
//---------------------------------------------------------
void CAI_Relationship::ApplyRelationship( CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	// @TODO (toml 10-22-04): sort out MP relationships 
	
	// The player spawns slightly after the NPCs, meaning that if we don't wait, the
	// player will miss any relationships placed on them.
	if ( AI_IsSinglePlayer() && !UTIL_GetLocalPlayer() )
	{
		SetThink( &CAI_Relationship::ApplyRelationshipThink );
		SetNextThink( gpGlobals->curtime );
	}

	if ( !m_bIsActive )
	{
		SetActive( true );
	}

	ChangeRelationships( m_iDisposition, NOT_REVERTING, pActivator, pCaller );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::RevertRelationship( CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if ( m_bIsActive )
	{
		ChangeRelationships( m_iPreviousDisposition, REVERTING_TO_PREV, pActivator, pCaller );
		SetActive( false );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::RevertToDefaultRelationship( CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	if ( m_bIsActive )
	{
		ChangeRelationships( -1, REVERTING_TO_DEFAULT, pActivator, pCaller );
		SetActive( false );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::UpdateOnRemove()
{
	gEntList.RemoveListenerEntity( this );
	// @TODO (toml 07-21-04): Should this actually revert on kill?
	// RevertRelationship();
	BaseClass::UpdateOnRemove();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::OnRestore()
{
	BaseClass::OnRestore();
	if ( m_bIsActive )
	{
		gEntList.AddListenerEntity( this );
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_Relationship::IsASubject( CBaseEntity *pEntity )
{
	if( pEntity->NameMatches( m_iszSubject ) )
		return true;

	if( pEntity->ClassMatches( m_iszSubject ) )
		return true;

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_Relationship::IsATarget( CBaseEntity *pEntity )
{
	if( pEntity->NameMatches( m_target ) )
		return true;

	if( pEntity->ClassMatches( m_target ) )
		return true;

	return false;
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::OnEntitySpawned( CBaseEntity *pEntity )
{
	// NOTE: This cannot use the procedural entity finding code since that only occurs on
	//		 inputs and not passively.

	if ( IsATarget( pEntity ) || IsASubject( pEntity ) )
	{
		ApplyRelationship();
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::OnEntityDeleted( CBaseEntity *pEntity )
{
}

//-----------------------------------------------------------------------------
// Purpose: Translate special tokens for inputs
// Input  : iszName - Name to check
//			*pActivator - Activator
//			*pCaller - Caller 
// Output : CBaseEntity - Entity that matches (NULL if none)
//-----------------------------------------------------------------------------
#define ACTIVATOR_KEYNAME "!activator"
#define CALLER_KEYNAME "!caller"

CBaseEntity *CAI_Relationship::FindEntityForProceduralName( string_t iszName, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	// Handle the activator token
	if ( iszName == AllocPooledString( ACTIVATOR_KEYNAME ) )
		return pActivator;

	// Handle the caller token
	if ( iszName == AllocPooledString( CALLER_KEYNAME ) )
		return pCaller;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Disclose the location of the target entity to the subject via a memory
// Input  : *pSubject - Entity to gain the memory of the target's location
//			*pTarget - Entity who's location will be disclosed
//-----------------------------------------------------------------------------
void CAI_Relationship::DiscloseNPCLocation( CBaseCombatCharacter *pSubject, CBaseCombatCharacter *pTarget )
{
	if ( pSubject == NULL || pTarget == NULL )
		return;

	CAI_BaseNPC *pNPC = pSubject->MyNPCPointer();
	if ( pNPC != NULL )
	{
		pNPC->UpdateEnemyMemory( pTarget, pTarget->GetAbsOrigin() );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_Relationship::ChangeRelationships( int disposition, int iReverting, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
 	if( iReverting != NOT_REVERTING && m_iPreviousDisposition == -1 )
	{
		// Trying to revert without having ever set the relationships!
		DevMsg( 2, "ai_relationship cannot revert changes before they are applied!\n");
		return;
	}

	const int MAX_HANDLED = 512;
	CUtlVectorFixed<CBaseCombatCharacter *, MAX_HANDLED> subjectList;
	CUtlVectorFixed<CBaseCombatCharacter *, MAX_HANDLED> targetList;

	// Add any special subjects we found
	CBaseEntity *pSpecialSubject = FindEntityForProceduralName( m_iszSubject, pActivator, pCaller );
	if ( pSpecialSubject && pSpecialSubject->MyCombatCharacterPointer() )
	{
		subjectList.AddToTail( pSpecialSubject->MyCombatCharacterPointer() );
	}

	// Add any special targets we found
	CBaseEntity *pSpecialTarget = FindEntityForProceduralName( m_target, pActivator, pCaller );
	if ( pSpecialTarget && pSpecialTarget->MyCombatCharacterPointer() )
	{
		targetList.AddToTail( pSpecialTarget->MyCombatCharacterPointer() );
	}

	// -------------------------------
	// Search for targets and subjects
	// -------------------------------

	float radiusSq = Square( m_flRadius );
	
	// Search players first
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( subjectList.Count() == MAX_HANDLED || targetList.Count() == MAX_HANDLED )
		{
			DevMsg( "Too many entities handled by ai_relationship %s\n", GetDebugName() );
			break;
		}

		CBasePlayer	*pPlayer = UTIL_PlayerByIndex( i );
		if ( pPlayer )
		{
			if( IsASubject( pPlayer ) )
			{
				if ( m_flRadius == 0.0 || GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() ) <= radiusSq )
					subjectList.AddToTail( pPlayer );
			}
			else if( IsATarget( pPlayer ) )
			{
				targetList.AddToTail( pPlayer );
			}
		}
	}

	// Search NPCs
	for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
	{
		if ( subjectList.Count() == MAX_HANDLED || targetList.Count() == MAX_HANDLED )
		{
			DevMsg( "Too many entities handled by ai_relationship %s\n", GetDebugName() );
			break;
		}

		CAI_BaseNPC *pNPC = (g_AI_Manager.AccessAIs())[i];
		if ( pNPC )
		{
			if( IsASubject( pNPC ) )
			{
				if ( m_flRadius == 0.0 || GetAbsOrigin().DistToSqr( pNPC->GetAbsOrigin() ) <= radiusSq )
					subjectList.AddToTail( pNPC );
			}
			else if( IsATarget( pNPC ) )
			{
				targetList.AddToTail( pNPC );
			}
		}
	}

	// If either list is still empty, we have a problem.
	if( subjectList.Count() == 0 )
	{
		DevMsg( 2, "ai_relationship '%s' finds no subject(s) called: %s\n", GetDebugName(), STRING( m_iszSubject ) );
		return;
	}
	else if ( targetList.Count() == 0 )
	{
		DevMsg( 2, "ai_relationship '%s' finds no target(s) called: %s\n", GetDebugName(), STRING( m_target ) );
		return;
	}

	// Ok, lists are populated. Apply all relationships.
	for ( int i = 0 ; i < subjectList.Count(); i++ )
	{
		CBaseCombatCharacter *pSubject = subjectList[ i ];

		for ( int j = 0 ; j < targetList.Count(); j++ )
		{
			CBaseCombatCharacter *pTarget = targetList[ j ];

			if ( m_iPreviousDisposition == -1 && iReverting == NOT_REVERTING )
			{
				// Set previous disposition.
				m_iPreviousDisposition = pSubject->IRelationType( pTarget );
				m_iPreviousRank = pSubject->IRelationPriority( pTarget );
			}

			if ( iReverting == REVERTING_TO_PREV )
			{
				pSubject->AddEntityRelationship( pTarget, (Disposition_t)m_iPreviousDisposition, m_iPreviousRank );

				if( m_bReciprocal )
				{
					pTarget->AddEntityRelationship( pSubject, (Disposition_t)m_iPreviousDisposition, m_iPreviousRank );
				}
			}
			else if ( iReverting == REVERTING_TO_DEFAULT )
			{
				pSubject->RemoveEntityRelationship( pTarget );

				if( m_bReciprocal )
				{
					pTarget->RemoveEntityRelationship( pSubject );
				}
			}
			else if( pSubject->IRelationType(pTarget) != disposition || 
				     pSubject->IRelationPriority(pTarget) != m_iRank || 
					 HasSpawnFlags( SF_RELATIONSHIP_NOTIFY_SUBJECT ) ||
					 HasSpawnFlags( SF_RELATIONSHIP_NOTIFY_TARGET ) )
			{
				// Apply the relationship to the subject
				pSubject->AddEntityRelationship( pTarget, (Disposition_t)disposition, m_iRank );

				// Make the subject aware of the target
				if ( HasSpawnFlags( SF_RELATIONSHIP_NOTIFY_SUBJECT ) )
				{
					DiscloseNPCLocation( pSubject, pTarget );
				}

				// Make the target aware of the subject
				if ( HasSpawnFlags( SF_RELATIONSHIP_NOTIFY_TARGET ) )
				{
					DiscloseNPCLocation( pTarget, pSubject );
				}

				// This relationship is applied to target and subject alike
				if ( m_bReciprocal )
				{
					// Apply the relationship to the target
					pTarget->AddEntityRelationship( pSubject, (Disposition_t)disposition, m_iRank );
				}
			}
		}
	}
}

