// NextBotEventResponderInterface.h
// Interface for propagating and responding to events
// Author: Michael Booth, May 2006
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_EVENT_RESPONDER_INTERFACE_H_
#define _NEXT_BOT_EVENT_RESPONDER_INTERFACE_H_

class Path;
class CTakeDamageInfo;
class CBaseEntity;
class CDOTABaseAbility;

struct CSoundParameters;
struct animevent_t;

#include "ai_speech.h"


//--------------------------------------------------------------------------------------------------------------------------
enum MoveToFailureType
{
	FAIL_NO_PATH_EXISTS,
	FAIL_STUCK,
	FAIL_FELL_OFF,
};

//--------------------------------------------------------------------------------------------------------------------------
/**
 * Events propagated to/between components.
 * To add an event, add its signature here and implement its propagation
 * to derived classes via FirstContainedResponder() and NextContainedResponder().
 * NOTE: Also add a translator to the Action class in NextBotBehavior.h.
 */
class INextBotEventResponder
{
public:
	DECLARE_CLASS_NOBASE( INextBotEventResponder );
	
	virtual ~INextBotEventResponder() { }
	
	// these methods are used by derived classes to define how events propagate
	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return NULL; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }
	
	//
	// Events.  All events must be 'extended' by calling the derived class explicitly to ensure propagation.
	// Each event must implement its propagation in this interface class.
	//
	virtual void OnLeaveGround( CBaseEntity *ground );		// invoked when bot leaves ground for any reason
	virtual void OnLandOnGround( CBaseEntity *ground );		// invoked when bot lands on the ground after being in the air

	virtual void OnContact( CBaseEntity *other, CGameTrace *result = NULL );	// invoked when bot touches 'other'

	virtual void OnMoveToSuccess( const Path *path );		// invoked when a bot reaches the end of the given Path
	virtual void OnMoveToFailure( const Path *path, MoveToFailureType reason );	// invoked when a bot fails to reach the end of the given Path
	virtual void OnStuck( void );							// invoked when bot becomes stuck while trying to move
	virtual void OnUnStuck( void );							// invoked when a previously stuck bot becomes un-stuck and can again move

	virtual void OnPostureChanged( void );					// when bot has assumed new posture (query IBody for posture)

	virtual void OnAnimationActivityComplete( int activity );	// when animation activity has finished playing
	virtual void OnAnimationActivityInterrupted( int activity );// when animation activity was replaced by another animation
	virtual void OnAnimationEvent( animevent_t *event );	// when a QC-file animation event is triggered by the current animation sequence

	virtual void OnIgnite( void );							// when bot starts to burn
	virtual void OnInjured( const CTakeDamageInfo &info );	// when bot is damaged by something
	virtual void OnKilled( const CTakeDamageInfo &info );	// when the bot's health reaches zero
	virtual void OnOtherKilled( CBaseCombatCharacter *victim, const CTakeDamageInfo &info );	// when someone else dies

	virtual void OnSight( CBaseEntity *subject );			// when subject initially enters bot's visual awareness
	virtual void OnLostSight( CBaseEntity *subject );		// when subject leaves enters bot's visual awareness

	virtual void OnSound( CBaseEntity *source, const Vector &pos, KeyValues *keys );				// when an entity emits a sound. "pos" is world coordinates of sound. "keys" are from sound's GameData
	virtual void OnSpokeConcept( CBaseCombatCharacter *who, AIConcept_t concept, AI_Response *response );	// when an Actor speaks a concept
	virtual void OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon );		// when someone fires a weapon

	virtual void OnNavAreaChanged( CNavArea *newArea, CNavArea *oldArea );	// when bot enters a new navigation area

	virtual void OnModelChanged( void );					// when the entity's model has been changed	

	virtual void OnPickUp( CBaseEntity *item, CBaseCombatCharacter *giver );	// when something is added to our inventory
	virtual void OnDrop( CBaseEntity *item );									// when something is removed from our inventory
	virtual void OnActorEmoted( CBaseCombatCharacter *emoter, int emote );			// when "emoter" does an "emote" (ie: manual voice command, etc)

	virtual void OnCommandAttack( CBaseEntity *victim );	// attack the given entity
	virtual void OnCommandApproach( const Vector &pos, float range = 0.0f );	// move to within range of the given position
	virtual void OnCommandApproach( CBaseEntity *goal );	// follow the given leader
	virtual void OnCommandRetreat( CBaseEntity *threat, float range = 0.0f );	// retreat from the threat at least range units away (0 == infinite)
	virtual void OnCommandPause( float duration = 0.0f );	// pause for the given duration (0 == forever)
	virtual void OnCommandResume( void );					// resume after a pause

	virtual void OnCommandString( const char *command );	// for debugging: respond to an arbitrary string representing a generalized command

	virtual void OnShoved( CBaseEntity *pusher );			// 'pusher' has shoved me
	virtual void OnBlinded( CBaseEntity *blinder );			// 'blinder' has blinded me with a flash of light

	virtual void OnTerritoryContested( int territoryID );	// territory has been invaded and is changing ownership
	virtual void OnTerritoryCaptured( int territoryID );	// we have captured enemy territory
	virtual void OnTerritoryLost( int territoryID );		// we have lost territory to the enemy

	virtual void OnWin( void );
	virtual void OnLose( void );

#ifdef DOTA_SERVER_DLL
	virtual void OnCommandMoveTo( const Vector &pos );
	virtual void OnCommandMoveToAggressive( const Vector &pos );
	virtual void OnCommandAttack( CBaseEntity *victim, bool bDeny );
	virtual void OnCastAbilityNoTarget( CDOTABaseAbility *ability );
	virtual void OnCastAbilityOnPosition( CDOTABaseAbility *ability, const Vector &pos );
	virtual void OnCastAbilityOnTarget( CDOTABaseAbility *ability, CBaseEntity *target );
	virtual void OnDropItem( const Vector &pos, CBaseEntity *item );
	virtual void OnPickupItem( CBaseEntity *item );
	virtual void OnPickupRune( CBaseEntity *item );
	virtual void OnStop();
	virtual void OnFriendThreatened( CBaseEntity *friendly, CBaseEntity *threat );
	virtual void OnCancelAttack( CBaseEntity *pTarget );
	virtual void OnDominated();
	virtual void OnWarped( Vector vStartPos );
#endif
};


inline void INextBotEventResponder::OnLeaveGround( CBaseEntity *ground )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnLeaveGround( ground );
	}	
}

inline void INextBotEventResponder::OnLandOnGround( CBaseEntity *ground )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnLandOnGround( ground );
	}	
}

inline void INextBotEventResponder::OnContact( CBaseEntity *other, CGameTrace *result )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnContact( other, result );
	}	
}

inline void INextBotEventResponder::OnMoveToSuccess( const Path *path )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnMoveToSuccess( path );
	}	
}

inline void INextBotEventResponder::OnMoveToFailure( const Path *path, MoveToFailureType reason )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnMoveToFailure( path, reason );
	}	
}

inline void INextBotEventResponder::OnStuck( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnStuck();
	}	
}

inline void INextBotEventResponder::OnUnStuck( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnUnStuck();
	}	
}

inline void INextBotEventResponder::OnPostureChanged( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnPostureChanged();
	}	
}

inline void INextBotEventResponder::OnAnimationActivityComplete( int activity )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnAnimationActivityComplete( activity );
	}	
}

inline void INextBotEventResponder::OnAnimationActivityInterrupted( int activity )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnAnimationActivityInterrupted( activity );
	}	
}

inline void INextBotEventResponder::OnAnimationEvent( animevent_t *event )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnAnimationEvent( event );
	}	
}

inline void INextBotEventResponder::OnIgnite( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnIgnite();
	}	
}

inline void INextBotEventResponder::OnInjured( const CTakeDamageInfo &info )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnInjured( info );
	}	
}

inline void INextBotEventResponder::OnKilled( const CTakeDamageInfo &info )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnKilled( info );
	}	
}

inline void INextBotEventResponder::OnOtherKilled( CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnOtherKilled( victim, info );
	}	
}

inline void INextBotEventResponder::OnSight( CBaseEntity *subject )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnSight( subject );
	}	
}

inline void INextBotEventResponder::OnLostSight( CBaseEntity *subject )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnLostSight( subject );
	}	
}

inline void INextBotEventResponder::OnSound( CBaseEntity *source, const Vector &pos, KeyValues *keys )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnSound( source, pos, keys );
	}	
}

inline void INextBotEventResponder::OnSpokeConcept( CBaseCombatCharacter *who, AIConcept_t concept, AI_Response *response )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnSpokeConcept( who, concept, response );
	}	
}

inline void INextBotEventResponder::OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnWeaponFired( whoFired, weapon );
	}	
}

inline void INextBotEventResponder::OnNavAreaChanged( CNavArea *newArea, CNavArea *oldArea )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnNavAreaChanged( newArea, oldArea );
	}	
}

inline void INextBotEventResponder::OnModelChanged( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnModelChanged();
	}	
}

inline void INextBotEventResponder::OnPickUp( CBaseEntity *item, CBaseCombatCharacter *giver )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnPickUp( item, giver );
	}	
}

inline void INextBotEventResponder::OnDrop( CBaseEntity *item )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnDrop( item );
	}	
}

inline void INextBotEventResponder::OnActorEmoted( CBaseCombatCharacter *emoter, int emote )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnActorEmoted( emoter, emote );
	}	
}

inline void INextBotEventResponder::OnShoved( CBaseEntity *pusher )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnShoved( pusher );
	}	
}

inline void INextBotEventResponder::OnBlinded( CBaseEntity *blinder )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnBlinded( blinder );
	}	
}

inline void INextBotEventResponder::OnCommandAttack( CBaseEntity *victim )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandAttack( victim );
	}	
}

inline void INextBotEventResponder::OnCommandApproach( const Vector &pos, float range )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandApproach( pos, range );
	}	
}

inline void INextBotEventResponder::OnCommandApproach( CBaseEntity *goal )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandApproach( goal );
	}	
}

inline void INextBotEventResponder::OnCommandRetreat( CBaseEntity *threat, float range )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandRetreat( threat, range );
	}	
}

inline void INextBotEventResponder::OnCommandPause( float duration )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandPause( duration );
	}	
}

inline void INextBotEventResponder::OnCommandResume( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandResume();
	}	
}

inline void INextBotEventResponder::OnCommandString( const char *command )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandString( command );
	}	
}

inline void INextBotEventResponder::OnTerritoryContested( int territoryID )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnTerritoryContested( territoryID );
	}	
}

inline void INextBotEventResponder::OnTerritoryCaptured( int territoryID )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnTerritoryCaptured( territoryID );
	}	
}

inline void INextBotEventResponder::OnTerritoryLost( int territoryID )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnTerritoryLost( territoryID );
	}	
}

inline void INextBotEventResponder::OnWin( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnWin();
	}	
}

inline void INextBotEventResponder::OnLose( void )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnLose();
	}	
}

#ifdef DOTA_SERVER_DLL
inline void INextBotEventResponder::OnCommandMoveTo( const Vector &pos )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandMoveTo( pos );
	}
}

inline void INextBotEventResponder::OnCommandMoveToAggressive( const Vector &pos )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandMoveToAggressive( pos );
	}
}

inline void INextBotEventResponder::OnCommandAttack( CBaseEntity *victim, bool bDeny )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCommandAttack( victim, bDeny );
	}	
}

inline void INextBotEventResponder::OnCastAbilityNoTarget( CDOTABaseAbility *ability )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCastAbilityNoTarget( ability );
	}	
}

inline void INextBotEventResponder::OnCastAbilityOnPosition( CDOTABaseAbility *ability, const Vector &pos )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCastAbilityOnPosition( ability, pos );
	}	
}

inline void INextBotEventResponder::OnCastAbilityOnTarget( CDOTABaseAbility *ability, CBaseEntity *target )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCastAbilityOnTarget( ability, target );
	}	
}

inline void INextBotEventResponder::OnDropItem( const Vector &pos, CBaseEntity *item )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnDropItem( pos, item );
	}	
}

inline void INextBotEventResponder::OnPickupItem( CBaseEntity *item )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnPickupItem( item );
	}	
}

inline void INextBotEventResponder::OnPickupRune( CBaseEntity *item )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnPickupRune( item );
	}	
}

inline void INextBotEventResponder::OnStop()
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnStop();
	}	
}

inline void INextBotEventResponder::OnFriendThreatened( CBaseEntity *friendly, CBaseEntity *threat )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnFriendThreatened( friendly, threat );
	}	
}

inline void INextBotEventResponder::OnCancelAttack( CBaseEntity *pTarget )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnCancelAttack( pTarget );
	}	
}

inline void INextBotEventResponder::OnDominated()
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnDominated();
	}	
}

inline void INextBotEventResponder::OnWarped( Vector vStartPos )
{
	for ( INextBotEventResponder *sub = FirstContainedResponder(); sub; sub = NextContainedResponder( sub ) )
	{
		sub->OnWarped( vStartPos );
	}	
}
#endif

#endif // _NEXT_BOT_EVENT_RESPONDER_INTERFACE_H_
