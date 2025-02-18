// NextBotPlayer.h
// A CBasePlayer bot based on the NextBot technology
// Author: Michael Booth, November 2005
//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef _NEXT_BOT_PLAYER_H_
#define _NEXT_BOT_PLAYER_H_

#include "cbase.h"
#include "gameinterface.h"

#include "NextBot.h"
#include "Path/NextBotPathFollow.h"
//#include "NextBotPlayerBody.h"
#include "NextBotBehavior.h"

#include "in_buttons.h"

extern ConVar NextBotPlayerStop;
extern ConVar NextBotPlayerWalk;
extern ConVar NextBotPlayerCrouch;
extern ConVar NextBotPlayerMove;



//--------------------------------------------------------------------------------------------------
/**
 * Instantiate a NextBot derived from CBasePlayer and spawn it into the environment.
 * Assumes class T is derived from CBasePlayer, and has the following method that
 * creates a new entity of type T and returns it:
 *
 * static CBasePlayer *T::AllocatePlayerEntity( edict_t *pEdict, const char *playerName )
 *
 */
template < typename T > 
T * NextBotCreatePlayerBot( const char *name, bool bReportFakeClient = true )
{
	/*
	if ( UTIL_ClientsInGame() >= gpGlobals->maxClients )
	{
	Msg( "CreatePlayerBot: Failed - server is full (%d/%d clients).\n", UTIL_ClientsInGame(), gpGlobals->maxClients );
	return NULL;
	}
	*/

	// This is a "back door" for allocating a custom player bot entity when
	// the engine calls ClientPutInServer (from CreateFakeClient)
	ClientPutInServerOverride( T::AllocatePlayerEntity );

	// create the bot and spawn it into the environment
	edict_t *botEdict = engine->CreateFakeClientEx( name, bReportFakeClient );

	// close the "back door"
	ClientPutInServerOverride( NULL );

	if ( botEdict == NULL )
	{
		Msg( "CreatePlayerBot: Unable to create bot %s - CreateFakeClient() returned NULL.\n", name );
		return NULL;
	}

	// create an instance of the bot's class and bind it to the edict
	T *bot = dynamic_cast< T * >( CBaseEntity::Instance( botEdict ) );

	if ( bot == NULL )
	{
		Assert( false );
		Error( "CreatePlayerBot: Could not Instance() from the bot edict.\n" );
		return NULL;
	}

	bot->SetPlayerName( name );

	// flag this as a fakeclient (bot)
	bot->ClearFlags();
	bot->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	return bot;
}


//--------------------------------------------------------------------------------------------------
/**
 * Interface to access player input buttons.
 * Unless a duration is given, each button is released at the start of the next frame.
 * The release methods allow releasing a button before its duration has elapsed.
 */
class INextBotPlayerInput
{
public:
	virtual void PressFireButton( float duration = -1.0f ) = 0;
	virtual void ReleaseFireButton( void ) = 0;

	virtual void PressAltFireButton( float duration = -1.0f ) = 0;
	virtual void ReleaseAltFireButton( void ) = 0;

	virtual void PressMeleeButton( float duration = -1.0f ) = 0;
	virtual void ReleaseMeleeButton( void ) = 0;

	virtual void PressSpecialFireButton( float duration = -1.0f ) = 0;
	virtual void ReleaseSpecialFireButton( void ) = 0;

	virtual void PressUseButton( float duration = -1.0f ) = 0;
	virtual void ReleaseUseButton( void ) = 0;

	virtual void PressReloadButton( float duration = -1.0f ) = 0;
	virtual void ReleaseReloadButton( void ) = 0;
	
	virtual void PressForwardButton( float duration = -1.0f ) = 0;
	virtual void ReleaseForwardButton( void ) = 0;

	virtual void PressBackwardButton( float duration = -1.0f ) = 0;
	virtual void ReleaseBackwardButton( void ) = 0;

	virtual void PressLeftButton( float duration = -1.0f ) = 0;
	virtual void ReleaseLeftButton( void ) = 0;

	virtual void PressRightButton( float duration = -1.0f ) = 0;
	virtual void ReleaseRightButton( void ) = 0;

	virtual void PressJumpButton( float duration = -1.0f ) = 0;
	virtual void ReleaseJumpButton( void ) = 0;

	virtual void PressCrouchButton( float duration = -1.0f ) = 0;
	virtual void ReleaseCrouchButton( void ) = 0;

	virtual void PressWalkButton( float duration = -1.0f ) = 0;
	virtual void ReleaseWalkButton( void ) = 0;

	virtual void SetButtonScale( float forward, float right ) = 0;
};


//--------------------------------------------------------------------------------------------------
/**
 * Drive a CBasePlayer-derived entity via NextBot logic
 */
template < typename PlayerType >
class NextBotPlayer : public PlayerType, public INextBot, public INextBotPlayerInput
{
public:
	DECLARE_CLASS( NextBotPlayer, PlayerType );

	NextBotPlayer( void );
	virtual ~NextBotPlayer();

	virtual void Spawn( void );

	virtual void SetSpawnPoint( CBaseEntity *spawnPoint );						// define place in environment where bot will (re)spawn
	virtual CBaseEntity		*EntSelectSpawnPoint( void );

	virtual void PhysicsSimulate( void );

	virtual bool IsNetClient( void ) const { return false; }					// Bots should return FALSE for this, they can't receive NET messages
	virtual bool IsFakeClient( void ) const { return true; }
	virtual bool IsBot( void ) const { return true; }
	virtual INextBot *MyNextBotPointer( void ) { return this; }

	// this is valid because the templatized PlayerType must be derived from CBasePlayer, which is derived from CBaseCombatCharacter
	virtual CBaseCombatCharacter *GetEntity( void ) const { return ( PlayerType * )this; }	

	virtual bool IsRemovedOnReset( void ) const { return false; }				// remove this bot when the NextBot manager calls Reset

	virtual bool IsDormantWhenDead( void ) const	{ return true; }			// should this player-bot continue to update itself when dead (respawn logic, etc)

	// allocate a bot and bind it to the edict
	static CBasePlayer *AllocatePlayerEntity( edict_t *edict, const char *playerName );

	//------------------------------------------------------------------------
	// utility methods
	float GetDistanceBetween( CBaseEntity *other ) const;						// return distance between us and the given entity
	bool IsDistanceBetweenLessThan( CBaseEntity *other, float range ) const;	// return true if distance between is less than the given value
	bool IsDistanceBetweenGreaterThan( CBaseEntity *other, float range ) const;	// return true if distance between is greater than the given value

	float GetDistanceBetween( const Vector &target ) const;						// return distance between us and the given entity
	bool IsDistanceBetweenLessThan( const Vector &target, float range ) const;	// return true if distance between is less than the given value
	bool IsDistanceBetweenGreaterThan( const Vector &target, float range ) const;	// return true if distance between is greater than the given value

	//------------------------------------------------------------------------
	// INextBotPlayerInput
	virtual void PressFireButton( float duration = -1.0f );
	virtual void ReleaseFireButton( void );

	virtual void PressAltFireButton( float duration = -1.0f );
	virtual void ReleaseAltFireButton( void );

	virtual void PressMeleeButton( float duration = -1.0f );
	virtual void ReleaseMeleeButton( void );

	virtual void PressSpecialFireButton( float duration = -1.0f );
	virtual void ReleaseSpecialFireButton( void );

	virtual void PressUseButton( float duration = -1.0f );
	virtual void ReleaseUseButton( void );

	virtual void PressReloadButton( float duration = -1.0f );
	virtual void ReleaseReloadButton( void );

	virtual void PressForwardButton( float duration = -1.0f );
	virtual void ReleaseForwardButton( void );

	virtual void PressBackwardButton( float duration = -1.0f );
	virtual void ReleaseBackwardButton( void );

	virtual void PressLeftButton( float duration = -1.0f );
	virtual void ReleaseLeftButton( void );

	virtual void PressRightButton( float duration = -1.0f );
	virtual void ReleaseRightButton( void );

	virtual void PressJumpButton( float duration = -1.0f );
	virtual void ReleaseJumpButton( void );

	virtual void PressCrouchButton( float duration = -1.0f );
	virtual void ReleaseCrouchButton( void );

	virtual void PressWalkButton( float duration = -1.0f );
	virtual void ReleaseWalkButton( void );

	virtual void SetButtonScale( float forward, float right );

	//------------------------------------------------------------------------
	// Event hooks into NextBot system 
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void HandleAnimEvent( animevent_t *event );
	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );	// invoked (by UpdateLastKnownArea) when we enter a new nav area (or it is reset to NULL)
	virtual void Touch( CBaseEntity *other );
	virtual void Weapon_Equip( CBaseCombatWeapon *weapon );						// for OnPickUp
	virtual	void Weapon_Drop( CBaseCombatWeapon *weapon, const Vector *target, const Vector *velocity );	// for OnDrop
	virtual void OnMainActivityComplete( Activity newActivity, Activity oldActivity );
	virtual void OnMainActivityInterrupted( Activity newActivity, Activity oldActivity );
	//------------------------------------------------------------------------

	bool IsAbleToAutoCenterOnLadders( void ) const;

	virtual void AvoidPlayers( CUserCmd *pCmd ) { }								// some game types allow players to pass through each other, this method pushes them apart

public:
	// begin INextBot ------------------------------------------------------------------------------------------------------------------
	virtual void Update( void );												// (EXTEND) update internal state

protected:
	int m_inputButtons;					// this is still needed to guarantee each button press is captured at least once
	int m_prevInputButtons;
	CountdownTimer m_fireButtonTimer;
	CountdownTimer m_meleeButtonTimer;
	CountdownTimer m_specialFireButtonTimer;
	CountdownTimer m_useButtonTimer;
	CountdownTimer m_reloadButtonTimer;
	CountdownTimer m_forwardButtonTimer;
	CountdownTimer m_backwardButtonTimer;
	CountdownTimer m_leftButtonTimer;
	CountdownTimer m_rightButtonTimer;
	CountdownTimer m_jumpButtonTimer;
	CountdownTimer m_crouchButtonTimer;
	CountdownTimer m_walkButtonTimer;
	CountdownTimer m_buttonScaleTimer;
	IntervalTimer m_burningTimer;		// how long since we were last burning
	float m_forwardScale;
	float m_rightScale;
	CHandle< CBaseEntity > m_spawnPointEntity;
};


template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::SetSpawnPoint( CBaseEntity *spawnPoint )
{
	m_spawnPointEntity = spawnPoint;
}

template < typename PlayerType >
inline CBaseEntity *NextBotPlayer< PlayerType >::EntSelectSpawnPoint( void )
{
	if ( m_spawnPointEntity != NULL )
		return m_spawnPointEntity;

	return BaseClass::EntSelectSpawnPoint();
}

template < typename PlayerType >
inline float NextBotPlayer< PlayerType >::GetDistanceBetween( CBaseEntity *other ) const
{
	return (this->GetAbsOrigin() - other->GetAbsOrigin()).Length();
}

template < typename PlayerType >
inline bool NextBotPlayer< PlayerType >::IsDistanceBetweenLessThan( CBaseEntity *other, float range ) const
{
	return (this->GetAbsOrigin() - other->GetAbsOrigin()).IsLengthLessThan( range );
}

template < typename PlayerType >
inline bool NextBotPlayer< PlayerType >::IsDistanceBetweenGreaterThan( CBaseEntity *other, float range ) const
{
	return (this->GetAbsOrigin() - other->GetAbsOrigin()).IsLengthGreaterThan( range );
}

template < typename PlayerType >
inline float NextBotPlayer< PlayerType >::GetDistanceBetween( const Vector &target ) const
{
	return (this->GetAbsOrigin() - target).Length();
}

template < typename PlayerType >
inline bool NextBotPlayer< PlayerType >::IsDistanceBetweenLessThan( const Vector &target, float range ) const
{
	return (this->GetAbsOrigin() - target).IsLengthLessThan( range );
}

template < typename PlayerType >
inline bool NextBotPlayer< PlayerType >::IsDistanceBetweenGreaterThan( const Vector &target, float range ) const
{
	return (this->GetAbsOrigin() - target).IsLengthGreaterThan( range );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressFireButton( float duration )
{
	m_inputButtons |= IN_ATTACK;
	m_fireButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseFireButton( void )
{
	m_inputButtons &= ~IN_ATTACK;
	m_fireButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressAltFireButton( float duration )
{
	PressMeleeButton( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseAltFireButton( void )
{
	ReleaseMeleeButton();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressMeleeButton( float duration )
{
	m_inputButtons |= IN_ATTACK2;
	m_meleeButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseMeleeButton( void )
{
	m_inputButtons &= ~IN_ATTACK2;
	m_meleeButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressSpecialFireButton( float duration )
{
	m_inputButtons |= IN_ATTACK3;
	m_specialFireButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseSpecialFireButton( void )
{
	m_inputButtons &= ~IN_ATTACK3;
	m_specialFireButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressUseButton( float duration )
{
	m_inputButtons |= IN_USE;
	m_useButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseUseButton( void )
{
	m_inputButtons &= ~IN_USE;
	m_useButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressReloadButton( float duration )
{
	m_inputButtons |= IN_RELOAD;
	m_reloadButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseReloadButton( void )
{
	m_inputButtons &= ~IN_RELOAD;
	m_reloadButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressJumpButton( float duration )
{
	m_inputButtons |= IN_JUMP;
	m_jumpButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseJumpButton( void )
{
	m_inputButtons &= ~IN_JUMP;
	m_jumpButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressCrouchButton( float duration )
{
	m_inputButtons |= IN_DUCK;
	m_crouchButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseCrouchButton( void )
{
	m_inputButtons &= ~IN_DUCK;
	m_crouchButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressWalkButton( float duration )
{
	m_inputButtons |= IN_SPEED;
	m_walkButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseWalkButton( void )
{
	m_inputButtons &= ~IN_SPEED;
	m_walkButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressForwardButton( float duration )
{
	m_inputButtons |= IN_FORWARD;
	m_forwardButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseForwardButton( void )
{
	m_inputButtons &= ~IN_FORWARD;
	m_forwardButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressBackwardButton( float duration )
{
	m_inputButtons |= IN_BACK;
	m_backwardButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseBackwardButton( void )
{
	m_inputButtons &= ~IN_BACK;
	m_backwardButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressLeftButton( float duration )
{
	m_inputButtons |= IN_MOVELEFT;
	m_leftButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseLeftButton( void )
{
	m_inputButtons &= ~IN_MOVELEFT;
	m_leftButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PressRightButton( float duration )
{
	m_inputButtons |= IN_MOVERIGHT;
	m_rightButtonTimer.Start( duration );
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::ReleaseRightButton( void )
{
	m_inputButtons &= ~IN_MOVERIGHT;
	m_rightButtonTimer.Invalidate();
}

template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::SetButtonScale( float forward, float right )
{
	m_forwardScale = forward;
	m_rightScale = right;
	m_buttonScaleTimer.Start( 0.01 );
}



//-----------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline NextBotPlayer< PlayerType >::NextBotPlayer( void )
{
	m_prevInputButtons = 0;
	m_inputButtons = 0;
	m_burningTimer.Invalidate();
	m_spawnPointEntity = NULL;
}


//-----------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline NextBotPlayer< PlayerType >::~NextBotPlayer()
{
}


//-----------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Spawn( void )
{
	engine->SetFakeClientConVarValue( this->edict(), "cl_autohelp", "0" );

	m_prevInputButtons = m_inputButtons = 0;
	m_fireButtonTimer.Invalidate();
	m_meleeButtonTimer.Invalidate();
	m_specialFireButtonTimer.Invalidate();
	m_useButtonTimer.Invalidate();
	m_reloadButtonTimer.Invalidate();
	m_forwardButtonTimer.Invalidate();
	m_backwardButtonTimer.Invalidate();
	m_leftButtonTimer.Invalidate();
	m_rightButtonTimer.Invalidate();
	m_jumpButtonTimer.Invalidate();
	m_crouchButtonTimer.Invalidate();
	m_walkButtonTimer.Invalidate();
	m_buttonScaleTimer.Invalidate();
	m_forwardScale = m_rightScale = 0.04;
	m_burningTimer.Invalidate();

	// reset first, because Spawn() may access various interfaces
	INextBot::Reset();

	BaseClass::Spawn();
}



//-----------------------------------------------------------------------------------------------------
inline void _NextBot_BuildUserCommand( CUserCmd *cmd, const QAngle &viewangles, float forwardmove, float sidemove, float upmove, int buttons, byte impulse )
{
	Q_memset( cmd, 0, sizeof( CUserCmd ) );

	cmd->command_number = gpGlobals->tickcount;
	cmd->forwardmove = forwardmove;
	cmd->sidemove = sidemove;
	cmd->upmove = upmove;
	cmd->buttons = buttons;
	cmd->impulse = impulse;

	VectorCopy( viewangles, cmd->viewangles );

	cmd->random_seed = random->RandomInt( 0, 0x7fffffff );
}


//-----------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::PhysicsSimulate( void )
{
	VPROF( "NextBotPlayer::PhysicsSimulate" );

	// Make sure not to simulate this guy twice per frame
	if ( PlayerType::m_nSimulationTick == gpGlobals->tickcount )
	{
		return;
	}

	if ( engine->IsPaused() )
	{
		// We're paused - don't add new commands
		PlayerType::PhysicsSimulate();
		return;
	}

	if ( ( IsDormantWhenDead() && PlayerType::m_lifeState == LIFE_DEAD ) || NextBotStop.GetBool() )
	{
		// death animation complete - nothing left to do except let PhysicsSimulate run PreThink etc
		PlayerType::PhysicsSimulate();
		return;
	}

	int inputButtons;
	//
	// Update bot behavior
	//
	if ( BeginUpdate() )
	{
		Update();

		// build button bits
		if ( !m_fireButtonTimer.IsElapsed() )
			m_inputButtons |= IN_ATTACK;

		if ( !m_meleeButtonTimer.IsElapsed() )
			m_inputButtons |= IN_ATTACK2;

		if ( !m_specialFireButtonTimer.IsElapsed() )
			m_inputButtons |= IN_ATTACK3;

		if ( !m_useButtonTimer.IsElapsed() )
			m_inputButtons |= IN_USE;

		if ( !m_reloadButtonTimer.IsElapsed() )
			m_inputButtons |= IN_RELOAD;

		if ( !m_forwardButtonTimer.IsElapsed() )
			m_inputButtons |= IN_FORWARD;

		if ( !m_backwardButtonTimer.IsElapsed() )
			m_inputButtons |= IN_BACK;

		if ( !m_leftButtonTimer.IsElapsed() )
			m_inputButtons |= IN_MOVELEFT;

		if ( !m_rightButtonTimer.IsElapsed() )
			m_inputButtons |= IN_MOVERIGHT;

		if ( !m_jumpButtonTimer.IsElapsed() )
			m_inputButtons |= IN_JUMP;

		if ( !m_crouchButtonTimer.IsElapsed() )
			m_inputButtons |= IN_DUCK;

		if ( !m_walkButtonTimer.IsElapsed() )
			m_inputButtons |= IN_SPEED;

		m_prevInputButtons = m_inputButtons;
		inputButtons = m_inputButtons;

		EndUpdate();
	}
	else
	{
		// HACK: Smooth out body animations
		GetBodyInterface()->Update();

		// keep buttons pressed between Update() calls (m_prevInputButtons),
		// and include any button presses that occurred this tick (m_inputButtons).
		inputButtons = m_prevInputButtons | m_inputButtons;
	}

	//
	// Convert NextBot locomotion and posture into
	// player commands
	//
	IBody *body = GetBodyInterface();
	ILocomotion *mover = GetLocomotionInterface();

	if ( body->IsActualPosture( IBody::CROUCH ) )
	{
		inputButtons |= IN_DUCK;
	}

	float forwardSpeed = 0.0f;
	float strafeSpeed = 0.0f;
	float verticalSpeed = ( m_inputButtons & IN_JUMP ) ? mover->GetRunSpeed() : 0.0f;

	if ( inputButtons & IN_FORWARD )
	{
		forwardSpeed = mover->GetRunSpeed();		
	}
	else if ( inputButtons & IN_BACK )
	{
		forwardSpeed = -mover->GetRunSpeed();
	}

	if ( inputButtons & IN_MOVELEFT )
	{
		strafeSpeed = -mover->GetRunSpeed();		
	}
	else if ( inputButtons & IN_MOVERIGHT )
	{
		strafeSpeed = mover->GetRunSpeed();
	}

	if ( NextBotPlayerWalk.GetBool() )
	{
		inputButtons |= IN_SPEED;
	}

	if ( NextBotPlayerCrouch.GetBool() )
	{
		inputButtons |= IN_DUCK;
	}

	if ( !m_buttonScaleTimer.IsElapsed() )
	{
		forwardSpeed = mover->GetRunSpeed() * m_forwardScale;
		strafeSpeed = mover->GetRunSpeed() * m_rightScale;
	}

	if ( !NextBotPlayerMove.GetBool() )
	{
		inputButtons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP );
		forwardSpeed = 0.0f;
		strafeSpeed = 0.0f;
		verticalSpeed = 0.0f;
	}

	QAngle angles = this->EyeAngles();

#ifdef TERROR
	if ( IsStunned() )
	{
		inputButtons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP | IN_DUCK );
	}

	// "Look" in the direction we're climbing/stumbling etc.  We can't do anything anyway, and it
	// keeps motion extraction working.
	if ( IsRenderYawOverridden() && IsMotionControlledXY( GetMainActivity() ) )
	{
		angles[YAW] = GetOverriddenRenderYaw();
	}
#endif

	// construct a "command" to move the player
	CUserCmd userCmd;
	_NextBot_BuildUserCommand( &userCmd, angles, forwardSpeed, strafeSpeed, verticalSpeed, inputButtons, 0 );

	AvoidPlayers( &userCmd );

	// allocate a new command and add it to the player's list of command to process
	this->ProcessUsercmds( &userCmd, 1, 1, 0, false );

	m_inputButtons = 0;

	// actually execute player commands and do player physics
	PlayerType::PhysicsSimulate();
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea )
{
	// propagate into NextBot responders
	INextBotEventResponder::OnNavAreaChanged( enteredArea, leftArea );

	BaseClass::OnNavAreaChanged( enteredArea, leftArea );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Touch( CBaseEntity *other )
{
	if ( ShouldTouch( other ) )
	{
		// propagate touch into NextBot event responders
		trace_t result;
		result = this->GetTouchTrace();
		OnContact( other, &result );
	}

	BaseClass::Touch( other );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Weapon_Equip( CBaseCombatWeapon *weapon )
{
#ifdef TERROR
	// TODO: Reimplement GetDroppingPlayer() into GetLastOwner()
	OnPickUp( weapon, weapon->GetDroppingPlayer() );
#else
	OnPickUp( weapon, NULL );
#endif

	BaseClass::Weapon_Equip( weapon );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Weapon_Drop( CBaseCombatWeapon *weapon, const Vector *target, const Vector *velocity )
{
	OnDrop( weapon );

	BaseClass::Weapon_Drop( weapon, target, velocity );
}


//--------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::OnMainActivityComplete( Activity newActivity, Activity oldActivity )
{
#ifdef TERROR
	BaseClass::OnMainActivityComplete( newActivity, oldActivity );
#endif
	OnAnimationActivityComplete( oldActivity );
}


//--------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::OnMainActivityInterrupted( Activity newActivity, Activity oldActivity )
{
#ifdef TERROR
	BaseClass::OnMainActivityInterrupted( newActivity, oldActivity );
#endif
	OnAnimationActivityInterrupted( oldActivity );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Update( void )
{
	// don't spend CPU updating if this Survivor is dead
	if ( ( this->IsAlive() || !IsDormantWhenDead() ) && !NextBotPlayerStop.GetBool() )
	{
		INextBot::Update();	
	}
}

//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline bool NextBotPlayer< PlayerType >::IsAbleToAutoCenterOnLadders( void ) const
{
	const ILocomotion *locomotion = GetLocomotionInterface();
	return locomotion && locomotion->IsAbleToAutoCenterOnLadder();
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline int NextBotPlayer< PlayerType >::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_BURN )
	{
		if ( !m_burningTimer.HasStarted() || m_burningTimer.IsGreaterThen( 1.0f ) )
		{
			// emit ignite event periodically as long as we are burning
			OnIgnite();
			m_burningTimer.Start();
		}
	}

	// propagate event to components
	OnInjured( info );

	return BaseClass::OnTakeDamage_Alive( info );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline int NextBotPlayer< PlayerType >::OnTakeDamage_Dying( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_BURN )
	{
		if ( !m_burningTimer.HasStarted() || m_burningTimer.IsGreaterThen( 1.0f ) )
		{
			// emit ignite event periodically as long as we are burning
			OnIgnite();
			m_burningTimer.Start();
		}
	}

	// propagate event to components
	OnInjured( info );

	return BaseClass::OnTakeDamage_Dying( info );
}


//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::Event_Killed( const CTakeDamageInfo &info )
{
	// propagate event to my components
	OnKilled( info );

	BaseClass::Event_Killed( info );
}



//----------------------------------------------------------------------------------------------------------
template < typename PlayerType >
inline void NextBotPlayer< PlayerType >::HandleAnimEvent( animevent_t *event )
{
	// propagate event to components
	OnAnimationEvent( event );

	BaseClass::HandleAnimEvent( event );
}


#endif // _NEXT_BOT_PLAYER_H_
