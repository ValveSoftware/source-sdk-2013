//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H
#ifdef _WIN32
#pragma once
#endif

class IPhysicsPlayerControllerEvent
{
public:
	virtual int	ShouldMoveTo( IPhysicsObject *pObject, const Vector &position ) = 0;
};

class IPhysicsPlayerController
{
public:
	virtual ~IPhysicsPlayerController( void ) {}

	virtual void Update( const Vector &position, const Vector &velocity, float secondsToArrival, bool onground, IPhysicsObject *ground ) = 0;
	virtual void SetEventHandler( IPhysicsPlayerControllerEvent *handler ) = 0;
	virtual bool IsInContact( void ) = 0;
	virtual void MaxSpeed( const Vector &maxVelocity ) = 0;

	// allows game code to change collision models
	virtual void SetObject( IPhysicsObject *pObject ) = 0;
	// UNDONE: Refactor this and shadow controllers into a single class/interface through IPhysicsObject
	virtual int GetShadowPosition( Vector *position, QAngle *angles ) = 0;
	virtual void StepUp( float height ) = 0;
	virtual void Jump() = 0;
	virtual void GetShadowVelocity( Vector *velocity ) = 0;
	virtual IPhysicsObject *GetObject() = 0;
	virtual void GetLastImpulse( Vector *pOut ) = 0;

	virtual void SetPushMassLimit( float maxPushMass ) = 0;
	virtual void SetPushSpeedLimit( float maxPushSpeed ) = 0;
	
	virtual float GetPushMassLimit() = 0;
	virtual float GetPushSpeedLimit() = 0;
	virtual bool WasFrozen() = 0;
};

#endif // PLAYER_CONTROLLER_H
