//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "utlrbtree.h"
#include "saverestore_utlvector.h"
#include "ai_goalentity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// CAI_GoalEntity implementation
//

BEGIN_DATADESC( CAI_GoalEntity )

	DEFINE_KEYFIELD(	m_iszActor,				FIELD_STRING, 	"Actor"					),
	DEFINE_KEYFIELD(	m_iszGoal,				FIELD_STRING, 	"Goal"					),
	DEFINE_KEYFIELD(	m_fStartActive,			FIELD_BOOLEAN,  "StartActive"			),
	DEFINE_KEYFIELD(	m_iszConceptModifiers,	FIELD_STRING, 	"BaseConceptModifiers"	),
	DEFINE_KEYFIELD(	m_SearchType,			FIELD_INTEGER, 	"SearchType"			),
	DEFINE_UTLVECTOR(	m_actors, 				FIELD_EHANDLE 							),
	DEFINE_FIELD(		m_hGoalEntity, 			FIELD_EHANDLE 							),
	DEFINE_FIELD(		m_flags, 				FIELD_INTEGER 							),

	DEFINE_THINKFUNC( DelayedRefresh ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", 		InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "UpdateActors",	InputUpdateActors ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate",		InputDeactivate ),

END_DATADESC()


//-------------------------------------

void CAI_GoalEntity::Spawn()
{
	SetThink( &CAI_GoalEntity::DelayedRefresh );
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-------------------------------------

void CAI_GoalEntity::OnRestore()
{
	BaseClass::OnRestore();

	ExitDormant();

	if ( ( m_flags & ACTIVE ) )
		gEntList.AddListenerEntity( this );
}

//-------------------------------------

void CAI_GoalEntity::DelayedRefresh()
{
	inputdata_t ignored;
	if ( m_fStartActive )
	{
		Assert( !(m_flags & ACTIVE) );
		InputActivate( ignored );
		m_fStartActive = false;
	}
	else
		InputUpdateActors( ignored );
	
	SetThink( NULL );
}

//-------------------------------------

void CAI_GoalEntity::PruneActors()
{
	for ( int i = m_actors.Count() - 1; i >= 0; i-- )
	{
		if ( m_actors[i] == NULL || m_actors[i]->IsMarkedForDeletion() || m_actors[i]->GetState() == NPC_STATE_DEAD )
			m_actors.FastRemove( i );
	}
}

//-------------------------------------

void CAI_GoalEntity::ResolveNames()
{
	m_actors.SetCount( 0 );
	
	CBaseEntity *pEntity = NULL;
	for (;;)
	{
		switch ( m_SearchType )
		{
			case ST_ENTNAME:
			{
				pEntity = gEntList.FindEntityByName( pEntity, m_iszActor );
				break;
			}
			
			case ST_CLASSNAME:
			{
				pEntity = gEntList.FindEntityByClassname( pEntity, STRING( m_iszActor ) );
				break;
			}
		}
		
		if ( !pEntity )
			break;
			
		CAI_BaseNPC *pActor = pEntity->MyNPCPointer();
		
		if ( pActor  && pActor->GetState() != NPC_STATE_DEAD )
		{
			AIHANDLE temp;
			temp = pActor;
			m_actors.AddToTail( temp );
		}
	}
		
	m_hGoalEntity = gEntList.FindEntityByName( NULL, m_iszGoal );
}

//-------------------------------------

void CAI_GoalEntity::InputActivate( inputdata_t &inputdata )
{
	if ( !( m_flags & ACTIVE ) )
	{
		gEntList.AddListenerEntity( this );
		
		UpdateActors();
		m_flags |= ACTIVE;
		
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			EnableGoal( m_actors[i] );
		}
	}
}

//-------------------------------------

void CAI_GoalEntity::InputUpdateActors( inputdata_t &inputdata )
{
	int i;
	CUtlRBTree<CAI_BaseNPC *> prevActors;
	CUtlRBTree<CAI_BaseNPC *>::IndexType_t index;

	SetDefLessFunc( prevActors );
	
	PruneActors();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		prevActors.Insert( m_actors[i] );
	}
	
	ResolveNames();
	
	for ( i = 0; i < m_actors.Count(); i++ )
	{
		index = prevActors.Find( m_actors[i] );
		if ( index == prevActors.InvalidIndex() )
		{
			if ( m_flags & ACTIVE )
				EnableGoal( m_actors[i] );
		}
		else
			prevActors.Remove( m_actors[i] );
	}
	
	for ( index = prevActors.FirstInorder(); index != prevActors.InvalidIndex(); index = prevActors.NextInorder( index ) )
	{
		if ( m_flags & ACTIVE )
			DisableGoal( prevActors[ index ] );
	}
}

//-------------------------------------

void CAI_GoalEntity::InputDeactivate( inputdata_t &inputdata ) 	
{
	if ( m_flags & ACTIVE )
	{
		gEntList.RemoveListenerEntity( this );
		UpdateActors();
		m_flags &= ~ACTIVE;

		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			DisableGoal( m_actors[i] );
		}		
	}
}

//-------------------------------------

void CAI_GoalEntity::EnterDormant( void )
{
	if ( m_flags & ACTIVE )
	{
		m_flags |= DORMANT;
		for ( int i = 0; i < m_actors.Count(); i++ )
		{
			DisableGoal( m_actors[i] );
		}
	}
}

//-------------------------------------

void CAI_GoalEntity::ExitDormant( void )
{
	if ( m_flags & DORMANT )
	{
		m_flags &= ~DORMANT;

		inputdata_t ignored;
		InputUpdateActors( ignored );
	}
}

//-------------------------------------

void CAI_GoalEntity::UpdateOnRemove()
{
	if ( m_flags & ACTIVE )
	{
		inputdata_t inputdata;
		InputDeactivate( inputdata );
	}
	BaseClass::UpdateOnRemove();
}

//-------------------------------------

void CAI_GoalEntity::OnEntityCreated( CBaseEntity *pEntity )
{
	Assert( m_flags & ACTIVE );
	
	if ( pEntity->MyNPCPointer() )
	{
		SetThink( &CAI_GoalEntity::DelayedRefresh );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	
}

//-------------------------------------

void CAI_GoalEntity::OnEntityDeleted( CBaseEntity *pEntity )
{
	Assert( pEntity != this );
}

//-----------------------------------------------------------------------------

int CAI_GoalEntity::DrawDebugTextOverlays()
{
	char tempstr[512];
	int offset = BaseClass::DrawDebugTextOverlays();

	Q_snprintf( tempstr, sizeof(tempstr), "Active: %s", IsActive() ? "yes" : "no" );
	EntityText( offset, tempstr, 0 );
	offset++;
		
	return offset;
}


