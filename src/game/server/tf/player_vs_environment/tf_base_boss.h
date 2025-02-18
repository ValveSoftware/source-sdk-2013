//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef TF_BASE_BOSS_H
#define TF_BASE_BOSS_H

#include "NextBot/NextBot.h"
#include "NextBot/NextBotGroundLocomotion.h"
#include "pathtrack.h"

class CTFPlayer;

#define TF_BASE_BOSS_CURRENCY	125


//----------------------------------------------------------------------------
class CTFBaseBossLocomotion : public NextBotGroundLocomotion
{
public:
	CTFBaseBossLocomotion( INextBot *bot ) : NextBotGroundLocomotion( bot ) { m_flStepHeight = m_flMaxJumpHeight = 100.0f; }
	virtual ~CTFBaseBossLocomotion() { }

	virtual float GetRunSpeed( void ) const;							// get maximum running speed
	virtual float GetStepHeight( void ) const		{ return m_flStepHeight; }	// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const	{ return m_flMaxJumpHeight; }	// return maximum height of a jump

	virtual void FaceTowards( const Vector &target );	// rotate body to face towards "target"

	void SetStepHeight( float flStepHeight ) { m_flStepHeight = flStepHeight; }
	void SetMaxJumpHeight( float flMaxJumpHeight ) { m_flMaxJumpHeight = flMaxJumpHeight; }

private:
	float m_flStepHeight = 100.0f;
	float m_flMaxJumpHeight = 100.0f;
};


//----------------------------------------------------------------------------
class CTFBaseBoss : public NextBotCombatCharacter
{
public:
	DECLARE_CLASS( CTFBaseBoss, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	CTFBaseBoss();
	virtual ~CTFBaseBoss();

	virtual void Precache( void );
	virtual void Spawn( void );
	int UpdateTransmitState( void );

	virtual void UpdateCollisionBounds( void ) {}

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void UpdateOnRemove();

	virtual bool IsRemovedOnReset( void ) const { return false; }	// remove this bot when the NextBot manager calls Reset

	virtual CTFBaseBossLocomotion *GetLocomotionInterface( void ) const	{ return m_locomotor; }

	virtual void Touch( CBaseEntity *pOther );

	virtual int GetCurrencyValue( void ){ return TF_BASE_BOSS_CURRENCY; }

	void BossThink( void );

	void SetResolvePlayerCollisions( bool bResolve ) { m_bResolvePlayerCollisions = bResolve; }

	void SetMaxSpeed( float value ) { m_speed = value; }
	float GetMaxSpeed( void ) const { return m_speed; }
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetSpeed( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );
	void InputSetMaxHealth( inputdata_t &inputdata );
	void InputAddHealth( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );
	void InputSetStepHeight( inputdata_t &inputdata );
	void InputSetMaxJumpHeight( inputdata_t &inputdata );

	void SetInitialHealth( int value );
	void SetCurrencyValue( int value );				// how much cash do we drop when we die?

	COutputEvent m_outputOnHealthBelow90Percent;
	COutputEvent m_outputOnHealthBelow80Percent;
	COutputEvent m_outputOnHealthBelow70Percent;
	COutputEvent m_outputOnHealthBelow60Percent;
	COutputEvent m_outputOnHealthBelow50Percent;
	COutputEvent m_outputOnHealthBelow40Percent;
	COutputEvent m_outputOnHealthBelow30Percent;
	COutputEvent m_outputOnHealthBelow20Percent;
	COutputEvent m_outputOnHealthBelow10Percent;

	COutputEvent m_outputOnKilled;

protected:
	virtual void ModifyDamage( CTakeDamageInfo *info ) const { }
	int GetInitialHealth( ) const { return m_initialHealth; }

private:
	int m_initialHealth;
	CNetworkVar( float, m_lastHealthPercentage );
	string_t m_modelString;
	float m_speed;
	int m_startDisabled;
	bool m_isEnabled;
	int m_damagePoseParameter;
	int m_currencyValue;

	bool m_bResolvePlayerCollisions;

	CTFBaseBossLocomotion *m_locomotor;

	void ResolvePlayerCollision( CTFPlayer *player );
};


inline void CTFBaseBoss::SetInitialHealth( int value )
{
	m_initialHealth = value;
}

inline void CTFBaseBoss::SetCurrencyValue( int value )
{
	m_currencyValue = value;
}


#endif // TF_BASE_BOSS_H
