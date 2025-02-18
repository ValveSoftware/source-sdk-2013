// NextBotCombatCharacter.h
// Next generation bot system
// Author: Michael Booth, April 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_H_
#define _NEXT_BOT_H_

#include "NextBotInterface.h"
#include "NextBotManager.h"

#ifdef TERROR
#include "player_lagcompensation.h"
#endif

class NextBotCombatCharacter;
struct animevent_t;

extern ConVar NextBotStop;


//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
/**
 * A Next Bot derived from CBaseCombatCharacter
 */
class NextBotCombatCharacter : public CBaseCombatCharacter, public INextBot
{
public:
	DECLARE_CLASS( NextBotCombatCharacter, CBaseCombatCharacter );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();
	
	NextBotCombatCharacter( void );
	virtual ~NextBotCombatCharacter() { }

	virtual void Spawn( void );

	virtual Vector EyePosition( void );

	virtual INextBot *MyNextBotPointer( void ) { return this; }

	// Event hooks into NextBot system ---------------------------------------
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void HandleAnimEvent( animevent_t *event );
	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );	// invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)
	virtual void Touch( CBaseEntity *other );
	virtual void SetModel( const char *szModelName );
	virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	virtual void Ignite( float flFlameLifetime, CBaseEntity *pAttacker );
	//------------------------------------------------------------------------

	virtual bool IsUseableEntity( CBaseEntity *entity, unsigned int requiredCaps = 0 );
	void UseEntity( CBaseEntity *entity, USE_TYPE useType = USE_TOGGLE );

	// Implement this if you use MOVETYPE_CUSTOM
	virtual void PerformCustomPhysics( Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity );

	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	
	// hook to INextBot update
	void DoThink( void );

	// expose to public
	int	GetLastHitGroup( void ) const;								// where on our body were we injured last

	virtual bool IsAreaTraversable( const CNavArea *area ) const;							// return true if we can use the given area 

	virtual CBaseCombatCharacter *GetLastAttacker( void ) const;	// return the character who last attacked me

	// begin INextBot public interface ----------------------------------------------------------------
	virtual NextBotCombatCharacter *GetEntity( void ) const			{ return const_cast< NextBotCombatCharacter * >( this ); }
	virtual NextBotCombatCharacter *GetNextBotCombatCharacter( void ) const	{ return const_cast< NextBotCombatCharacter * >( this ); }

private:
	EHANDLE m_lastAttacker;
	
	bool m_didModelChange;
};


inline CBaseCombatCharacter *NextBotCombatCharacter::GetLastAttacker( void ) const
{
	return ( m_lastAttacker.Get() == NULL ) ? NULL : m_lastAttacker->MyCombatCharacterPointer();
}

inline int NextBotCombatCharacter::GetLastHitGroup( void ) const
{
	return LastHitGroup();
}

//-----------------------------------------------------------------------------------------------------
class NextBotDestroyer
{
public:
	NextBotDestroyer( int team );
	bool operator() ( INextBot *bot );
	int m_team;			// the team to delete bots from, or TEAM_ANY for any team
};

#endif // _NEXT_BOT_H_
