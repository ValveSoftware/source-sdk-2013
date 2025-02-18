//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENGINE_ICOLLIDEABLE_H
#define ENGINE_ICOLLIDEABLE_H
#ifdef _WIN32
#pragma once
#endif


enum SolidType_t;
class IHandleEntity;
struct Ray_t;
struct model_t;
class Vector;
class QAngle;
class CGameTrace;
typedef CGameTrace trace_t;
class IClientUnknown;


abstract_class ICollideable
{
public:
	// Gets at the entity handle associated with the collideable
	virtual IHandleEntity	*GetEntityHandle() = 0;

	// These methods return the bounds of an OBB measured in "collision" space
	// which can be retreived through the CollisionToWorldTransform or
	// GetCollisionOrigin/GetCollisionAngles methods
	virtual const Vector&	OBBMinsPreScaled() const = 0;
	virtual const Vector&	OBBMaxsPreScaled() const = 0;
	virtual const Vector&	OBBMins() const = 0;
	virtual const Vector&	OBBMaxs() const = 0;

	// Returns the bounds of a world-space box used when the collideable is being traced
	// against as a trigger. It's only valid to call these methods if the solid flags
	// have the FSOLID_USE_TRIGGER_BOUNDS flag set.
	virtual void			WorldSpaceTriggerBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs ) const = 0;

	// custom collision test
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) = 0;

	// Perform hitbox test, returns true *if hitboxes were tested at all*!!
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) = 0;

	// Returns the BRUSH model index if this is a brush model. Otherwise, returns -1.
	virtual int				GetCollisionModelIndex() = 0;

	// Return the model, if it's a studio model.
	virtual const model_t*	GetCollisionModel() = 0;

	// Get angles and origin.
	virtual const Vector&	GetCollisionOrigin() const = 0;
	virtual const QAngle&	GetCollisionAngles() const = 0;
	virtual const matrix3x4_t&	CollisionToWorldTransform() const = 0;

	// Return a SOLID_ define.
	virtual SolidType_t		GetSolid() const = 0;
	virtual int				GetSolidFlags() const = 0;

	// Gets at the containing class...
	virtual IClientUnknown*	GetIClientUnknown() = 0;
	
	// We can filter out collisions based on collision group
	virtual int				GetCollisionGroup() const = 0;

	// Returns a world-aligned box guaranteed to surround *everything* in the collision representation
	// Note that this will surround hitboxes, trigger bounds, physics.
	// It may or may not be a tight-fitting box and its volume may suddenly change
	virtual void			WorldSpaceSurroundingBounds( Vector *pVecMins, Vector *pVecMaxs ) = 0;

	virtual bool			ShouldTouchTrigger( int triggerSolidFlags ) const = 0;

	// returns NULL unless this collideable has specified FSOLID_ROOT_PARENT_ALIGNED
	virtual const matrix3x4_t	*GetRootParentToWorldTransform() const = 0;
};


#endif // ENGINE_ICOLLIDEABLE_H
