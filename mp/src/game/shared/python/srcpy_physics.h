//====== Copyright © Sandern Corporation, All rights reserved. ===========//
//
// Purpose: Two safe wrappers for IPhysicObject in python
//			One bound to the lifetime to an entity (used with VGetPhysicObject())
//			And one that can be created directly through the physic interface.
//
// $NoKeywords: $
//=============================================================================//

#ifndef SRCPY_PHYSICS_H
#define SRCPY_PHYSICS_H

#ifdef _WIN32
#pragma once
#endif

#include "ehandle.h"
#include "vcollide_parse.h"
#include "vphysics_interface.h"
#include "physics_shared.h"
//#include "src_python_wrappers_base.h"
#include "srcpy_util.h"

#ifndef CLIENT_DLL
	#include "physics.h"
#endif // CLIENT_DLL

class PyPhysicsObjectBase;

//-----------------------------------------------------------------------------
// Purpose: IPhysicsShadowController wrapper for python.
//-----------------------------------------------------------------------------
class PyPhysicsShadowController 
{
public:
	PyPhysicsShadowController( boost::python::object refPyPhysObj );

	void CheckValid();

	// Comparing
	bool operator==(const PyPhysicsShadowController& v) const;
	bool operator!=(const PyPhysicsShadowController& v) const;	

	bool Cmp( boost::python::object other );
	bool NonZero();

public:
	void Update( const Vector &position, const QAngle &angles, float timeOffset );
	void MaxSpeed( float maxSpeed, float maxAngularSpeed );
	void StepUp( float height );

	// If the teleport distance is non-zero, the object will be teleported to 
	// the target location when the error exceeds this quantity.
	void SetTeleportDistance( float teleportDistance );
	bool AllowsTranslation();
	bool AllowsRotation();

	// There are two classes of shadow objects:
	// 1) Game physics controlled, shadow follows game physics (this is the default)
	// 2) Physically controlled - shadow position is a target, but the game hasn't guaranteed that the space can be occupied by this object
	void SetPhysicallyControlled( bool isPhysicallyControlled );
	bool IsPhysicallyControlled();
	void GetLastImpulse( Vector *pOut );
	void UseShadowMaterial( bool bUseShadowMaterial );
	void ObjectMaterialChanged( int materialIndex );


	//Basically get the last inputs to IPhysicsShadowController::Update(), returns last input to timeOffset in Update()
	float GetTargetPosition( Vector *pPositionOut, QAngle *pAnglesOut );

	float GetTeleportDistance( void );
	void GetMaxSpeed( float *pMaxSpeedOut, float *pMaxAngularSpeedOut );

private:
	boost::python::object m_refPyPhysObj;
	PyPhysicsObjectBase *m_pPyPhysObj;
	IPhysicsShadowController *m_pShadCont;
};

//-----------------------------------------------------------------------------
// Inlines
//-----------------------------------------------------------------------------
inline bool PyPhysicsShadowController::operator==( const PyPhysicsShadowController& src ) const
{
	return m_pShadCont == src.m_pShadCont;
}

inline bool PyPhysicsShadowController::operator!=( const PyPhysicsShadowController& src ) const
{
	return m_pShadCont != src.m_pShadCont;
}


//-----------------------------------------------------------------------------
// Purpose: IPhysicsObject base wrapper for python.
//-----------------------------------------------------------------------------
class PyPhysicsObjectBase 
{
public:
	friend class PyPhysicsShadowController;
	friend class PyPhysicsCollision;

	PyPhysicsObjectBase() : m_pPhysObj(NULL) {}

	virtual PyObject *GetPySelf() const { return NULL; }

	virtual void CheckValid()
	{
		PyErr_SetString(PyExc_ValueError, "PhysicsObject invalid" );
		throw boost::python::error_already_set();
	}

	// Comparing
	bool operator==(const PyPhysicsObjectBase& v) const;
	bool operator!=(const PyPhysicsObjectBase& v) const;	

	bool Cmp( boost::python::object other );
	bool NonZero();

public:
	// IPhysicsObject methods
	// Methods commented out: don't care now or don't care at all

	// returns true if this object is static/unmoveable
	// NOTE: returns false for objects that are not created static, but set EnableMotion(false);
	// Call IsMoveable() to find if the object is static OR has motion disabled
	bool			IsStatic();
	bool			IsAsleep();
	bool			IsTrigger();
	bool			IsFluid();		// fluids are special triggers with fluid controllers attached, they return true to IsTrigger() as well!
	bool			IsHinged();
	bool			IsCollisionEnabled();
	bool			IsGravityEnabled();
	bool			IsDragEnabled();
	bool			IsMotionEnabled();
	bool			IsMoveable();	 // legacy: IsMotionEnabled() && !IsStatic()
	bool			IsAttachedToConstraint(bool bExternalOnly);

	// Enable / disable collisions for this object
	void			EnableCollisions( bool enable );
	// Enable / disable gravity for this object
	void			EnableGravity( bool enable );
	// Enable / disable air friction / drag for this object
	void			EnableDrag( bool enable );
	// Enable / disable motion (pin / unpin the object)
	void			EnableMotion( bool enable );


	// Game can store data in each object (link back to game object)
	//void			SetGameData( void *pGameData );
	//void			*GetGameData( void );
	// This flags word can be defined by the game as well
	void			SetGameFlags( unsigned short userFlags );
	unsigned short	GetGameFlags( void );
	void			SetGameIndex( unsigned short gameIndex );
	unsigned short	GetGameIndex( void );

	// setup various callbacks for this object
	void			SetCallbackFlags( unsigned short callbackflags );
	// get the current callback state for this object
	unsigned short	GetCallbackFlags( void );

	// "wakes up" an object
	// NOTE: ALL OBJECTS ARE "Asleep" WHEN CREATED
	void			Wake( void );
	void			Sleep( void );
	// call this when the collision filter conditions change due to this 
	// object's state (e.g. changing solid type or collision group)
	void			RecheckCollisionFilter();
	// NOTE: Contact points aren't updated when collision rules change, call this to force an update
	// UNDONE: Force this in RecheckCollisionFilter() ?
	void			RecheckContactPoints();

	// mass accessors
	void			SetMass( float mass );
	float			GetMass( void );
	// get 1/mass (it's cached)
	float			GetInvMass( void );
	Vector			GetInertia( void );
	Vector			GetInvInertia( void );
	void			SetInertia( const Vector &inertia );

	void			SetDamping( float speed, float rot );
	boost::python::tuple GetDamping();

	// coefficients are optional, pass either
	void			SetDragCoefficient( float *pDrag, float *pAngularDrag );
	void			SetBuoyancyRatio( float ratio );			// Override bouyancy

	// material index
	int				GetMaterialIndex();
	void			SetMaterialIndex( int materialIndex );

	// contents bits
	unsigned int	GetContents();
	void			SetContents( unsigned int contents );

	// Get the radius if this is a sphere object (zero if this is a polygonal mesh)
	float			GetSphereRadius();
	float			GetEnergy();
	Vector			GetMassCenterLocalSpace();

	// NOTE: This will teleport the object
	void			SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport );
	void			SetPositionMatrix( const matrix3x4_t&matrix, bool isTeleport );

	void			GetPosition( Vector *worldPosition, QAngle *angles );
	void			GetPositionMatrix( matrix3x4_t *positionMatrix );
	// force the velocity to a new value
	// NOTE: velocity is in worldspace, angularVelocity is relative to the object's 
	// local axes (just like pev->velocity, pev->avelocity)
	void			SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity );

	// like the above, but force the change into the simulator immediately
	void			SetVelocityInstantaneous( const Vector *velocity, const AngularImpulse *angularVelocity );

	// NOTE: velocity is in worldspace, angularVelocity is relative to the object's 
	// local axes (just like pev->velocity, pev->avelocity)
	void			GetVelocity( Vector *velocity, AngularImpulse *angularVelocity );

	// NOTE: These are velocities, not forces.  i.e. They will have the same effect regardless of
	// the object's mass or inertia
	void			AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity );
	// gets a velocity in the object's local frame of reference at a specific point
	void			GetVelocityAtPoint( const Vector &worldPosition, Vector *pVelocity );
	// gets the velocity actually moved by the object in the last simulation update
	void			GetImplicitVelocity( Vector *velocity, AngularImpulse *angularVelocity );
	// NOTE:	These are here for convenience, but you can do them yourself by using the matrix
	//			returned from GetPositionMatrix()
	// convenient coordinate system transformations (params - dest, src)
	void			LocalToWorld( Vector *worldPosition, const Vector &localPosition );
	void			WorldToLocal( Vector *localPosition, const Vector &worldPosition );

	// transforms a vector (no translation) from object-local to world space
	void			LocalToWorldVector( Vector *worldVector, const Vector &localVector );
	// transforms a vector (no translation) from world to object-local space
	void			WorldToLocalVector( Vector *localVector, const Vector &worldVector );

	// push on an object
	// force vector is direction & magnitude of impulse kg in / s
	void			ApplyForceCenter( const Vector &forceVector );
	void			ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition );
	// apply torque impulse.  This will change the angular velocity on the object.
	// HL Axes, kg degrees / s
	void			ApplyTorqueCenter( const AngularImpulse &torque );

	// Calculates the force/torque on the center of mass for an offset force impulse (pass output to ApplyForceCenter / ApplyTorqueCenter)
	void			CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerForce, AngularImpulse *centerTorque );
	// Calculates the linear/angular velocities on the center of mass for an offset force impulse (pass output to AddVelocity)
	void			CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerVelocity, AngularImpulse *centerAngularVelocity );
	// calculate drag scale
	float			CalculateLinearDrag( const Vector &unitDirection );
	float			CalculateAngularDrag( const Vector &objectSpaceRotationAxis );

	// returns true if the object is in contact with another object
	// if true, puts a point on the contact surface in contactPoint, and
	// a pointer to the object in contactObject
	// NOTE: You can pass NULL for either to avoid computations
	// BUGBUG: Use CreateFrictionSnapshot instead of this - this is a simple hack
	// bool			GetContactPoint( Vector *contactPoint, IPhysicsObject **contactObject );

	// refactor this a bit - move some of this to IPhysicsShadowController
	void			SetShadow( float maxSpeed, float maxAngularSpeed, bool allowPhysicsMovement, bool allowPhysicsRotation );
	void			UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset );

	// returns number of ticks since last Update() call
	int				GetShadowPosition( Vector *position, QAngle *angles );
	PyPhysicsShadowController GetShadowController( void );
	void			RemoveShadowController();
	// applies the math of the shadow controller to this object.
	// for use in your own controllers
	// returns the new value of secondsToArrival with dt time elapsed
	// float			ComputeShadowControl( const hlshadowcontrol_params_t &params, float secondsToArrival, float dt );


	// const CPhysCollide	*GetCollide( void );
	const char			*GetName();

	void			BecomeTrigger();
	void			RemoveTrigger();

	// sets the object to be hinged.  Fixed it place, but able to rotate around one axis.
	void			BecomeHinged( int localAxis );
	// resets the object to original state
	void			RemoveHinged();

	// used to iterate the contact points of an object
	// IPhysicsFrictionSnapshot *CreateFrictionSnapshot();
	// void DestroyFrictionSnapshot( IPhysicsFrictionSnapshot *pSnapshot );

	// dumps info about the object to Msg()
	void			OutputDebugInfo();

protected:
	IPhysicsObject *m_pPhysObj;
	bool			m_bOwnsObject;
};

inline bool PyPhysicsObjectBase::operator==( const PyPhysicsObjectBase& src ) const
{
	return m_pPhysObj == src.m_pPhysObj;
}

inline bool PyPhysicsObjectBase::operator!=( const PyPhysicsObjectBase& src ) const 
{
	return m_pPhysObj != src.m_pPhysObj;
}

//-----------------------------------------------------------------------------
// Purpose: IPhysicsObject python wrapper 
//			TODO: Merge with base object, no longer needed.
//-----------------------------------------------------------------------------
class PyPhysicsObject : public PyPhysicsObjectBase
{
public:
	PyPhysicsObject();
	//PyPhysicsObject( IPhysicsObject *pPhysObj );
	PyPhysicsObject( CBaseEntity *pEnt );

	~PyPhysicsObject();

	virtual void CheckValid() 
	{
		if( m_bValid == false ) {
			PyErr_SetString(PyExc_ValueError, "PhysicsObject invalid" );
			throw boost::python::error_already_set();
		}
	}

	bool HasEntity() { return m_hEnt != NULL; }

	// Functions for modifying. Not for python.
	void SetEntity( CBaseEntity *pEnt ) { m_hEnt = pEnt; }
	void SetInvalid() { m_bValid = false; m_hEnt = NULL; m_pPhysObj = NULL; }
	bool IsValid() { return m_bValid; }
	IPhysicsObject *GetVPhysicsObject() { return m_pPhysObj; }
	void InitFromPhysicsObject( IPhysicsObject *pPhysObj );

	void Destroy();

private:
	EHANDLE m_hEnt;
	bool m_bValid;
	bool m_bOwnsPhysObject;
};

boost::python::object PyCreateEmptyPhysicsObject();
boost::python::object PyCreatePhysicsObject( IPhysicsObject *pPhysObj );
boost::python::object PyCreatePhysicsObject( CBaseEntity *pEnt );
void PyPhysDestroyObject( PyPhysicsObject *pPyPhysObj, CBaseEntity *pEntity = NULL );

//-----------------------------------------------------------------------------
// Purpose: Physic collision interface
//-----------------------------------------------------------------------------
class PyPhysicsCollision 
{
public:
	boost::python::tuple CollideGetAABB( PyPhysicsObject *pPhysObj, const Vector &collideOrigin, const QAngle &collideAngles );

	void TraceBox( PyRay_t &ray, PyPhysicsObject &physObj, const Vector &collideOrigin, const QAngle &collideAngles, trace_t &ptr );
};
extern PyPhysicsCollision *pyphyscollision;

//-----------------------------------------------------------------------------
// Purpose: Surface Props
//-----------------------------------------------------------------------------
class PyPhysicsSurfaceProps
{
public:
	// parses a text file containing surface prop keys
	int		ParseSurfaceData( const char *pFilename, const char *pTextfile );
	// current number of entries in the database
	int		SurfacePropCount( void ) const;

	int		GetSurfaceIndex( const char *pSurfacePropName ) const;
	//void	GetPhysicsProperties( int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity ) const;

	surfacedata_t	GetSurfaceData( int surfaceDataIndex );
	const char		*GetString( unsigned short stringTableIndex ) const;


	const char		*GetPropName( int surfaceDataIndex ) const;

	// sets the global index table for world materials
	// UNDONE: Make this per-CPhysCollide
	// TODO: Needed?
	//void	SetWorldMaterialIndexTable( int *pMapArray, int mapSize ) = 0;

	// NOTE: Same as GetPhysicsProperties, but maybe more convenient
	void	GetPhysicsParameters( int surfaceDataIndex, surfacephysicsparams_t &paramsout ) const;
};
extern PyPhysicsSurfaceProps *pyphysprops;

//-----------------------------------------------------------------------------
// Purpose: Wrapper functions, to check arguments
//-----------------------------------------------------------------------------
inline boost::python::object PyPhysModelCreateBox( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, bool isStatic )
{
	if( pEntity ) 
		return PyCreatePhysicsObject( PhysModelCreateBox( pEntity, mins, maxs, origin, isStatic ) );
	return boost::python::object();
}

inline boost::python::object PyPhysModelCreateOBB( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, const QAngle &angle, bool isStatic )
{
	if( pEntity ) 
		return PyCreatePhysicsObject( PhysModelCreateOBB( pEntity, mins, maxs, origin, angle, isStatic ) );
	return boost::python::object();
}

#if 0 // TODO
inline boost::python::object PyPhysModelCreateSphere( CBaseEntity *pEntity, float radius, const Vector &origin, bool isStatic )
{
	if( pEntity ) 
		return PyCreatePhysicsObject( PhysModelCreateSphere( pEntity, radius, origin, isStatic ) );
	return boost::python::object();
}
#endif // 0

#ifndef CLIENT_DLL
// Callbacks
void PyPhysCallbackImpulse( PyPhysicsObject &pyPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque );
void PyPhysCallbackSetVelocity( PyPhysicsObject &pyPhysicsObject, const Vector &vecVelocity );
void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );
void PyPhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info );
void PyPhysCallbackRemove(CBaseEntity *pRemove);

// Impact damage
float PyCalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageTypeOut, const char *iszDamageTableName = NULL, bool bDamageFromHeldObjects = false );

// For forcing an extra think (testing purposes)
void Physics_RunThinkFunctions( bool simulating );
#endif // CLIENT_DLL

void PyForcePhysicsSimulate();

#endif // SRCPY_PHYSICS_H