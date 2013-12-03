//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// fish.h
// Simple fish behavior
// Author: Michael S. Booth, April 2005

#ifndef _FISH_H_
#define _FISH_H_

#include "baseanimating.h"
#include "GameEventListener.h"

class CFishPool;

//----------------------------------------------------------------------------------------------
/**
 * Simple ambient fish
 */
class CFish : public CBaseAnimating
{
public:
	DECLARE_CLASS( CFish, CBaseAnimating );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CFish( void );
	virtual ~CFish();

	void Initialize( CFishPool *pool, unsigned int id );
	
	virtual void Spawn( void );

	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void Touch( CBaseEntity *other );			///< in contact with "other"

	void Update( float deltaT );						///< invoked each server tick

	void FlockTo( CFish *other, float amount );			///< influence my motion to flock with other nearby fish
	float Avoid( void );
	void Panic( void );									///< panic for awhile

	void ResetVisible( void );							///< zero the visible vector
	void AddVisible( CFish *fish );						///< add this fish to our visible vector

private:
	friend void SendProxy_FishOriginX( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	friend void SendProxy_FishOriginY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

	CHandle<CFishPool> m_pool;							///< the pool we are in
	unsigned int m_id;									///< our unique ID

	CNetworkVar( float, m_x );							///< have to send position coordinates separately since Z is unused
	CNetworkVar( float, m_y );							///< have to send position coordinates separately since Z is unused
	CNetworkVar( float, m_z );							///< only sent once since fish always swim at the same depth

	CNetworkVar( float, m_angle );						///< only yaw changes
	float m_angleChange;
	Vector m_forward;
	Vector m_perp;

	CNetworkVar( Vector, m_poolOrigin );				///< used to efficiently network our relative position
	CNetworkVar( float, m_waterLevel );

	float m_speed;
	float m_desiredSpeed;

	float m_calmSpeed;									///< speed the fish moves when calm
	float m_panicSpeed;									///< speed the fish moves when panicked

	float m_avoidRange;									///< range to avoid obstacles

	CountdownTimer m_turnTimer;							///< every so often our turn preference changes
	bool m_turnClockwise;								///< if true this fish prefers to turn clockwise, else CCW
	
	CountdownTimer m_goTimer;							///< start the fish moving when timer elapses
	CountdownTimer m_moveTimer;							///< dont decay speed while we are moving
	CountdownTimer m_panicTimer;						///< if active, fish is panicked
	CountdownTimer m_disperseTimer;						///< initial non-flocking time

	CUtlVector< CFish * > m_visible;					///< vector of fish that we can see
};


//----------------------------------------------------------------------------------------------
/**
 * This class defines a volume of water where a number of CFish swim
 */
class CFishPool : public CBaseEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( CFishPool, CBaseEntity );
	DECLARE_DATADESC();

	CFishPool( void );

	virtual void Spawn();

	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	virtual void FireGameEvent( IGameEvent *event );

	void Update( void );					///< invoked each server tick

	float GetWaterLevel( void ) const;		///< return Z coordinate of water in world coords
	float GetMaxRange( void ) const;		///< return how far a fish is allowed to wander

private:
	int m_fishCount;						///< number of fish in the pool
	float m_maxRange;						///< how far a fish is allowed to wander
	float m_swimDepth;						///< the depth the fish swim below the water surface

	float m_waterLevel;						///< Z of water surface

	bool m_isDormant;

	CUtlVector< CHandle<CFish> > m_fishes;	///< vector of all fish in this pool

	CountdownTimer m_visTimer;				///< for throttling line of sight checks between all fish
};


inline float CFishPool::GetMaxRange( void ) const
{
	return m_maxRange;
}


inline float CFishPool::GetWaterLevel( void ) const
{
	return m_waterLevel;
}


#endif // _FISH_H_

