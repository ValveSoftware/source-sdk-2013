//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COLLISIONPROPERTY_H
#define COLLISIONPROPERTY_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "engine/ICollideable.h"
#include "mathlib/vector.h"
#include "ispatialpartition.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseEntity;
class IHandleEntity;
class QAngle;
class Vector;
struct Ray_t;
class IPhysicsObject;


//-----------------------------------------------------------------------------
// Force spatial partition updates (to avoid threading problems caused by lazy update)
//-----------------------------------------------------------------------------
void UpdateDirtySpatialPartitionEntities();


//-----------------------------------------------------------------------------
// Specifies how to compute the surrounding box
//-----------------------------------------------------------------------------
enum SurroundingBoundsType_t
{
	USE_OBB_COLLISION_BOUNDS = 0,
	USE_BEST_COLLISION_BOUNDS,		// Always use the best bounds (most expensive)
	USE_HITBOXES,
	USE_SPECIFIED_BOUNDS,
	USE_GAME_CODE,
	USE_ROTATION_EXPANDED_BOUNDS,
	USE_COLLISION_BOUNDS_NEVER_VPHYSICS,

	SURROUNDING_TYPE_BIT_COUNT = 3
};


//-----------------------------------------------------------------------------
// Encapsulates collision representation for an entity
//-----------------------------------------------------------------------------
class CCollisionProperty : public ICollideable
{
	DECLARE_CLASS_NOBASE( CCollisionProperty );
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

public:
	CCollisionProperty();
	~CCollisionProperty();

	void Init( CBaseEntity *pEntity );

	// Methods of ICollideable
	virtual IHandleEntity	*GetEntityHandle();
 	virtual const Vector&	OBBMinsPreScaled() const { return m_vecMinsPreScaled.Get(); }
	virtual const Vector&	OBBMaxsPreScaled() const { return m_vecMaxsPreScaled.Get(); }
	virtual const Vector&	OBBMins() const { return m_vecMins.Get(); }
	virtual const Vector&	OBBMaxs() const { return m_vecMaxs.Get(); }
	virtual void			WorldSpaceTriggerBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs ) const;
	virtual bool			TestCollision( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual bool			TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );
	virtual int				GetCollisionModelIndex();
	virtual const model_t*	GetCollisionModel();
	virtual const Vector&	GetCollisionOrigin() const;
	virtual const QAngle&	GetCollisionAngles() const;
	virtual const matrix3x4_t&	CollisionToWorldTransform() const;
	virtual SolidType_t		GetSolid() const;
	virtual int				GetSolidFlags() const;
	virtual IClientUnknown*	GetIClientUnknown();
	virtual int				GetCollisionGroup() const;
	virtual void			WorldSpaceSurroundingBounds( Vector *pVecMins, Vector *pVecMaxs );
	virtual bool			ShouldTouchTrigger( int triggerSolidFlags ) const;
	virtual const matrix3x4_t *GetRootParentToWorldTransform() const;

public:
	// Spatial partition management
	void			CreatePartitionHandle();
	void			DestroyPartitionHandle();
	unsigned short	GetPartitionHandle() const;

	// Marks the spatial partition dirty
	void			MarkPartitionHandleDirty();

	// Sets the collision bounds + the size (OBB)
	void			SetCollisionBounds( const Vector& mins, const Vector &maxs );

	// Rebuilds the scaled bounds from the pre-scaled bounds after a model's scale has changed
	void			RefreshScaledCollisionBounds( void );

	// Sets special trigger bounds. The bloat amount indicates how much bigger the 
	// trigger bounds should be beyond the bounds set in SetCollisionBounds
	// This method will also set the FSOLID flag FSOLID_USE_TRIGGER_BOUNDS
	void			UseTriggerBounds( bool bEnable, float flBloat = 0.0f );

	// Sets the method by which the surrounding collision bounds is set
	// You must pass in values for mins + maxs if you select the USE_SPECIFIED_BOUNDS type. 
	void			SetSurroundingBoundsType( SurroundingBoundsType_t type, const Vector *pMins = NULL, const Vector *pMaxs = NULL );

	// Sets the solid type (which type of collision representation)
	void			SetSolid( SolidType_t val );

	// Methods related to size. The OBB here is measured in CollisionSpace
	// (specified by GetCollisionToWorld)
	const Vector&	OBBSize( ) const;

	// Returns a radius (or the square of the radius) of a sphere 
	// *centered at the world space center* bounding the collision representation 
	// of the entity. NOTE: The world space center *may* move when the entity rotates.
	float			BoundingRadius() const;
	float			BoundingRadius2D() const;

	// Returns the center of the OBB in collision space
	const Vector &	OBBCenter( ) const;

	// center point of entity measured in world space
	// NOTE: This point *may* move when the entity moves depending on
	// which solid type is being used.
	const Vector &	WorldSpaceCenter( ) const;

	// Methods related to solid flags
	void			ClearSolidFlags( void );	
	void			RemoveSolidFlags( int flags );
	void			AddSolidFlags( int flags );
	bool			IsSolidFlagSet( int flagMask ) const;
	void		 	SetSolidFlags( int flags );
	bool			IsSolid() const;

	// Updates the spatial partition
	void			UpdatePartition( );

	// Are the bounds defined in entity space?
	bool			IsBoundsDefinedInEntitySpace() const;

	// Transforms a point in OBB space to world space
	const Vector &	CollisionToWorldSpace( const Vector &in, Vector *pResult ) const;

	// Transforms a point in world space to OBB space
	const Vector &	WorldToCollisionSpace( const Vector &in, Vector *pResult ) const;

	// Transforms a direction in world space to OBB space
	const Vector &	WorldDirectionToCollisionSpace( const Vector &in, Vector *pResult ) const;

	// Selects a random point in the bounds given the normalized 0-1 bounds 
	void			RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const;

	// Is a worldspace point within the bounds of the OBB?
	bool			IsPointInBounds( const Vector &vecWorldPt ) const;

	// Computes a bounding box in world space surrounding the collision bounds
	void			WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const;

	// Get the collision space mins directly
	const Vector &	CollisionSpaceMins( void ) const;

	// Get the collision space maxs directly
	const Vector &	CollisionSpaceMaxs( void ) const;

	// Computes a "normalized" point (range 0,0,0 - 1,1,1) in collision space
	// Useful for things like getting a point 75% of the way along z on the OBB, for example
	const Vector &	NormalizedToCollisionSpace( const Vector &in, Vector *pResult ) const;

	// Computes a "normalized" point (range 0,0,0 - 1,1,1) in world space
	const Vector &	NormalizedToWorldSpace( const Vector &in, Vector *pResult ) const;

	// Transforms a point in world space to normalized space
	const Vector &	WorldToNormalizedSpace( const Vector &in, Vector *pResult ) const;

	// Transforms a point in collision space to normalized space
	const Vector &	CollisionToNormalizedSpace( const Vector &in, Vector *pResult ) const;

	// Computes the nearest point in the OBB to a point specified in world space
	void			CalcNearestPoint( const Vector &vecWorldPt, Vector *pVecNearestWorldPt ) const;

	// Computes the distance from a point in world space to the OBB
	float			CalcDistanceFromPoint( const Vector &vecWorldPt ) const;

	// Does a rotation make us need to recompute the surrounding box?
	bool			DoesRotationInvalidateSurroundingBox( ) const;

	// Does VPhysicsUpdate make us need to recompute the surrounding box?
	bool			DoesVPhysicsInvalidateSurroundingBox( ) const;

	// Marks the entity has having a dirty surrounding box
	void			MarkSurroundingBoundsDirty();

	// Compute the largest dot product of the OBB and the specified direction vector
	float			ComputeSupportMap( const Vector &vecDirection ) const;

private:
	// Transforms an AABB measured in collision space to a box that surrounds it in world space
	void CollisionAABBToWorldAABB( const Vector &entityMins, const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const;

	// Expand trigger bounds..
	void ComputeVPhysicsSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Expand trigger bounds..
	bool ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );
	bool ComputeEntitySpaceHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Computes the surrounding collision bounds based on whatever algorithm we want...
	void ComputeCollisionSurroundingBox( bool bUseVPhysics, Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Computes the surrounding collision bounds from the the OBB (not vphysics)
	void ComputeRotationExpandedBounds( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Computes the surrounding collision bounds based on whatever algorithm we want...
	void ComputeSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	// Check for untouch
	void CheckForUntouch();

	// Updates the spatial partition
	void UpdateServerPartitionMask( );

	// Outer
	CBaseEntity *GetOuter();
	const CBaseEntity *GetOuter() const;

private:
	CBaseEntity *m_pOuter;

	CNetworkVector( m_vecMinsPreScaled );
	CNetworkVector( m_vecMaxsPreScaled );
	CNetworkVector( m_vecMins );
	CNetworkVector( m_vecMaxs );
	float m_flRadius;

	CNetworkVar( unsigned short, m_usSolidFlags );

	// Spatial partition
	SpatialPartitionHandle_t m_Partition;
	CNetworkVar( unsigned char, m_nSurroundType );

	// One of the SOLID_ defines. Use GetSolid/SetSolid.
	CNetworkVar( unsigned char, m_nSolidType );			
	CNetworkVar( unsigned char , m_triggerBloat );

	// SUCKY: We didn't use to have to store this previously
	// but storing it here means that we can network it + avoid a ton of
	// client-side mismatch problems
	CNetworkVector( m_vecSpecifiedSurroundingMinsPreScaled );
	CNetworkVector( m_vecSpecifiedSurroundingMaxsPreScaled );
	CNetworkVector( m_vecSpecifiedSurroundingMins );
	CNetworkVector( m_vecSpecifiedSurroundingMaxs );

	// Cached off world-aligned surrounding bounds
#if 0
	short	m_surroundingMins[3];
	short	m_surroundingMaxs[3];
#else
	Vector	m_vecSurroundingMins;
	Vector	m_vecSurroundingMaxs;
#endif

	// pointer to the entity's physics object (vphysics.dll)
	//IPhysicsObject	*m_pPhysicsObject;
	
	friend class CBaseEntity;
};


//-----------------------------------------------------------------------------
// For networking this bad boy
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_CollisionProperty );
#else
EXTERN_SEND_TABLE( DT_CollisionProperty );
#endif


//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CBaseEntity *CCollisionProperty::GetOuter()
{
	return m_pOuter;
}

inline const CBaseEntity *CCollisionProperty::GetOuter() const
{
	return m_pOuter;
}


//-----------------------------------------------------------------------------
// Spatial partition
//-----------------------------------------------------------------------------
inline unsigned short CCollisionProperty::GetPartitionHandle() const
{
	return m_Partition;
}


//-----------------------------------------------------------------------------
// Methods related to size
//-----------------------------------------------------------------------------
inline const Vector& CCollisionProperty::OBBSize( ) const
{
	// NOTE: Could precache this, but it's not used that often..
	Vector &temp = AllocTempVector();
	VectorSubtract( m_vecMaxs, m_vecMins, temp );
	return temp;
}


//-----------------------------------------------------------------------------
// Bounding radius size
//-----------------------------------------------------------------------------
inline float CCollisionProperty::BoundingRadius() const
{
	return m_flRadius;
}


//-----------------------------------------------------------------------------
// Methods relating to solid flags
//-----------------------------------------------------------------------------
inline bool CCollisionProperty::IsBoundsDefinedInEntitySpace() const
{
	return (( m_usSolidFlags & FSOLID_FORCE_WORLD_ALIGNED ) == 0 ) &&
			( m_nSolidType != SOLID_BBOX ) && ( m_nSolidType != SOLID_NONE );
}

inline void CCollisionProperty::ClearSolidFlags( void )
{
	SetSolidFlags( 0 );
}

inline void CCollisionProperty::RemoveSolidFlags( int flags )
{
	SetSolidFlags( m_usSolidFlags & ~flags );
}

inline void CCollisionProperty::AddSolidFlags( int flags )
{
	SetSolidFlags( m_usSolidFlags | flags );
}

inline int CCollisionProperty::GetSolidFlags( void ) const
{
	return m_usSolidFlags;
}

inline bool CCollisionProperty::IsSolidFlagSet( int flagMask ) const
{
	return (m_usSolidFlags & flagMask) != 0;
}

inline bool CCollisionProperty::IsSolid() const
{
	return ::IsSolid( (SolidType_t)(unsigned char)m_nSolidType, m_usSolidFlags );
}


//-----------------------------------------------------------------------------
// Returns the center in OBB space
//-----------------------------------------------------------------------------
inline const Vector& CCollisionProperty::OBBCenter( ) const
{
	Vector &vecResult = AllocTempVector();
	VectorLerp( m_vecMins, m_vecMaxs, 0.5f, vecResult );
	return vecResult;
}


//-----------------------------------------------------------------------------
// center point of entity
//-----------------------------------------------------------------------------
inline const Vector &CCollisionProperty::WorldSpaceCenter( ) const 
{
	Vector &vecResult = AllocTempVector();
	CollisionToWorldSpace( OBBCenter(), &vecResult );
	return vecResult;
}


//-----------------------------------------------------------------------------
// Transforms a point in OBB space to world space
//-----------------------------------------------------------------------------
inline const Vector &CCollisionProperty::CollisionToWorldSpace( const Vector &in, Vector *pResult ) const 
{
	// Makes sure we don't re-use the same temp twice
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		VectorAdd( in, GetCollisionOrigin(), *pResult );
	}
	else
	{
		VectorTransform( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}


//-----------------------------------------------------------------------------
// Transforms a point in world space to OBB space
//-----------------------------------------------------------------------------
inline const Vector &CCollisionProperty::WorldToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		VectorSubtract( in, GetCollisionOrigin(), *pResult );
	}
	else
	{
		VectorITransform( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}


//-----------------------------------------------------------------------------
// Transforms a direction in world space to OBB space
//-----------------------------------------------------------------------------
inline const Vector & CCollisionProperty::WorldDirectionToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		*pResult = in;
	}
	else
	{
		VectorIRotate( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}


//-----------------------------------------------------------------------------
// Computes a bounding box in world space surrounding the collision bounds
//-----------------------------------------------------------------------------
inline void CCollisionProperty::WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	CollisionAABBToWorldAABB( m_vecMins, m_vecMaxs, pWorldMins, pWorldMaxs );
}


// Get the collision space mins directly
inline const Vector & CCollisionProperty::CollisionSpaceMins( void ) const
{
	return m_vecMins;
}

// Get the collision space maxs directly
inline const Vector & CCollisionProperty::CollisionSpaceMaxs( void ) const
{
	return m_vecMaxs;
}


//-----------------------------------------------------------------------------
// Does a rotation make us need to recompute the surrounding box?
//-----------------------------------------------------------------------------
inline bool CCollisionProperty::DoesRotationInvalidateSurroundingBox( ) const
{
	if ( IsSolidFlagSet(FSOLID_ROOT_PARENT_ALIGNED) )
		return true;

	switch ( m_nSurroundType )
	{
	case USE_COLLISION_BOUNDS_NEVER_VPHYSICS:
	case USE_OBB_COLLISION_BOUNDS:
	case USE_BEST_COLLISION_BOUNDS:
		return IsBoundsDefinedInEntitySpace();

	// In the case of game code, we don't really know, so we have to assume it does
	case USE_HITBOXES:
	case USE_GAME_CODE:
		return true;

	case USE_ROTATION_EXPANDED_BOUNDS:
	case USE_SPECIFIED_BOUNDS:
		return false;

	default:
		Assert(0);
		return true;
	}
}


#endif // COLLISIONPROPERTY_H
