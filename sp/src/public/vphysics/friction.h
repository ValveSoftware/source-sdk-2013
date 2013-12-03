//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FRICTION_H
#define FRICTION_H
#ifdef _WIN32
#pragma once
#endif

// NOTE: This is an iterator for the contact points on an object
// NOTE: This should only be used temporarily.  Holding one of these
// NOTE: across collision callbacks or calls into simulation will cause errors!
// NOTE: VPHYSICS may choose to make the data contained within this object invalid 
// NOTE: any time simulation is run.
class IPhysicsFrictionSnapshot
{
public:
	virtual ~IPhysicsFrictionSnapshot() {}

	virtual bool IsValid() = 0;

	// Object 0 is this object, Object 1 is the other object
	virtual IPhysicsObject *GetObject( int index ) = 0;
	virtual int GetMaterial( int index ) = 0;

	virtual void GetContactPoint( Vector &out ) = 0;
	
	// points away from source object
	virtual void GetSurfaceNormal( Vector &out ) = 0;
	virtual float GetNormalForce() = 0;
	virtual float GetEnergyAbsorbed() = 0;

	// recompute friction (useful if dynamically altering materials/mass)
	virtual void RecomputeFriction() = 0;
	// clear all friction force at this contact point
	virtual void ClearFrictionForce() = 0;

	virtual void MarkContactForDelete() = 0;
	virtual void DeleteAllMarkedContacts( bool wakeObjects ) = 0;

	// Move to the next friction data for this object
	virtual void NextFrictionData() = 0;
	virtual float GetFrictionCoefficient() = 0;
};



#endif // FRICTION_H
