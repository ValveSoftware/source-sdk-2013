//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Public interfaces to vphysics DLL
//
// $NoKeywords: $
//=============================================================================

#ifndef VPHYSICS_INTERFACE_V30_H
#define VPHYSICS_INTERFACE_V30_H
#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include "mathlib/vector.h"
#include "mathlib/vector4d.h"
#include "vcollide.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IPhysicsObjectPairHash;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsFluidController;
class IPhysicsSpring;
class IPhysicsVehicleController;
class IPhysicsCollisionSet;
class IPhysicsPlayerController;
class IPhysicsFrictionSnapshot;
struct Ray_t;
struct constraint_ragdollparams_t;
struct constraint_hingeparams_t;
struct constraint_fixedparams_t;
struct constraint_ballsocketparams_t;
struct constraint_slidingparams_t;
struct constraint_pulleyparams_t;
struct constraint_lengthparams_t;
struct constraint_groupparams_t;
struct vehicleparams_t;
struct matrix3x4_t;
struct fluidparams_t;
struct springparams_t;
struct objectparams_t;
struct debugcollide_t;
class CGameTrace;
typedef CGameTrace trace_t;
struct physics_stats_t;
struct physics_performanceparams_t;
struct physsaveparams_t;
struct physrestoreparams_t;
struct physprerestoreparams_t;

namespace VPhysicsInterfaceV30
{

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IPhysicsObject;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
class IConvexInfo;

enum PhysInterfaceId_t 
{
	PIID_UNKNOWN,
	PIID_IPHYSICSOBJECT,
	PIID_IPHYSICSFLUIDCONTROLLER,
	PIID_IPHYSICSSPRING,
	PIID_IPHYSICSCONSTRAINTGROUP,
	PIID_IPHYSICSCONSTRAINT,
	PIID_IPHYSICSSHADOWCONTROLLER,
	PIID_IPHYSICSPLAYERCONTROLLER,
	PIID_IPHYSICSMOTIONCONTROLLER,
	PIID_IPHYSICSVEHICLECONTROLLER,
	PIID_IPHYSICSGAMETRACE,

	PIID_NUM_TYPES
};


class ISave;
class IRestore;


#define VPHYSICS_DEBUG_OVERLAY_INTERFACE_VERSION_1	"VPhysicsDebugOverlay001"

abstract_class IVPhysicsDebugOverlay
{
public:
	virtual void AddEntityTextOverlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void AddBoxOverlay(const Vector& origin, const Vector& mins, const Vector& max, QAngle const& orientation, int r, int g, int b, int a, float duration) = 0;
	virtual void AddTriangleOverlay(const Vector& p1, const Vector& p2, const Vector& p3, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
	virtual void AddLineOverlay(const Vector& origin, const Vector& dest, int r, int g, int b,bool noDepthTest, float duration) = 0;
	virtual void AddTextOverlay(const Vector& origin, float duration, PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void AddTextOverlay(const Vector& origin, int line_offset, float duration, PRINTF_FORMAT_STRING const char *format, ...) = 0;
	virtual void AddScreenTextOverlay(float flXPos, float flYPos,float flDuration, int r, int g, int b, int a, const char *text) = 0;
	virtual void AddSweptBoxOverlay(const Vector& start, const Vector& end, const Vector& mins, const Vector& max, const QAngle & angles, int r, int g, int b, int a, float flDuration) = 0;
	virtual void AddTextOverlayRGB(const Vector& origin, int line_offset, float duration, float r, float g, float b, float alpha, PRINTF_FORMAT_STRING const char *format, ...) = 0;
};

#define VPHYSICS_INTERFACE_VERSION_30	"VPhysics030"

abstract_class IPhysics
{
public:
	virtual	IPhysicsEnvironment		*CreateEnvironment( void ) = 0;
	virtual void DestroyEnvironment( IPhysicsEnvironment * ) = 0;
	virtual IPhysicsEnvironment		*GetActiveEnvironmentByIndex( int index ) = 0;

	// Creates a fast hash of pairs of objects
	// Useful for maintaining a table of object relationships like pairs that do not collide.
	virtual IPhysicsObjectPairHash		*CreateObjectPairHash() = 0;
	virtual void						DestroyObjectPairHash( IPhysicsObjectPairHash *pHash ) = 0;

	// holds a cache of these by id.  So you can get by id to search for the previously created set
	// UNDONE: Sets are currently limited to 32 elements.  More elements will return NULL in create.
	// NOTE: id is not allowed to be zero.
	virtual IPhysicsCollisionSet		*FindOrCreateCollisionSet( unsigned int id, int maxElementCount ) = 0;
	virtual IPhysicsCollisionSet		*FindCollisionSet( unsigned int id ) = 0;
	virtual void						DestroyAllCollisionSets() = 0;
};


// CPhysConvex is a single convex solid
class CPhysConvex;
// CPhysPolysoup is an abstract triangle soup mesh
class CPhysPolysoup;
class ICollisionQuery;
class IVPhysicsKeyParser;
struct convertconvexparams_t;

// UNDONE: Find a better place for this?  Should be in collisionutils, but it's needs VPHYSICS' solver.
struct truncatedcone_t
{
	Vector	origin;
	Vector	normal;
	float	h;			// height of the cone (hl units)
	float	theta;		// cone angle (degrees)
};


#define VPHYSICS_COLLISION_INTERFACE_VERSION_7	"VPhysicsCollision007"

abstract_class IPhysicsCollision
{
public:
	virtual ~IPhysicsCollision( void ) {}

	// produce a convex element from verts (convex hull around verts)
	virtual CPhysConvex		*ConvexFromVerts( Vector **pVerts, int vertCount ) = 0;
	// produce a convex element from planes (csg of planes)
	virtual CPhysConvex		*ConvexFromPlanes( float *pPlanes, int planeCount, float mergeDistance ) = 0;
	// calculate volume of a convex element
	virtual float			ConvexVolume( CPhysConvex *pConvex ) = 0;

	// Convert an array of convex elements to a compiled collision model (this deletes the convex elements)
	virtual CPhysCollide	*ConvertConvexToCollide( CPhysConvex **pConvex, int convexCount ) = 0;

	virtual float			ConvexSurfaceArea( CPhysConvex *pConvex ) = 0;
	// store game-specific data in a convex solid
	virtual void			SetConvexGameData( CPhysConvex *pConvex, unsigned int gameData ) = 0;
	// If not converted, free the convex elements with this call
	virtual void			ConvexFree( CPhysConvex *pConvex ) = 0;

	// concave objects
	// create a triangle soup
	virtual CPhysPolysoup	*PolysoupCreate( void ) = 0;
	// destroy the container and memory
	virtual void			PolysoupDestroy( CPhysPolysoup *pSoup ) = 0;
	// add a triangle to the soup
	virtual void			PolysoupAddTriangle( CPhysPolysoup *pSoup, const Vector &a, const Vector &b, const Vector &c, int materialIndex7bits ) = 0;
	// convert the convex into a compiled collision model
	virtual CPhysCollide *ConvertPolysoupToCollide( CPhysPolysoup *pSoup, bool useMOPP ) = 0;
	
	// Get the memory size in bytes of the collision model for serialization
	virtual int				CollideSize( CPhysCollide *pCollide ) = 0;
	// serialize the collide to a block of memory
	virtual int				CollideWrite( char *pDest, CPhysCollide *pCollide ) = 0;

	// Free a collide that was created with ConvertConvexToCollide()
	// UNDONE: Move this up near the other Collide routines when the version is changed
	virtual void			DestroyCollide( CPhysCollide *pCollide ) = 0;
	// compute the volume of a collide
	virtual float			CollideVolume( CPhysCollide *pCollide ) = 0;
	// compute surface area for tools
	virtual float			CollideSurfaceArea( CPhysCollide *pCollide ) = 0;

	// Get the support map for a collide in the given direction
	virtual Vector			CollideGetExtent( const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction ) = 0;

	// Get an AABB for an oriented collision model
	virtual void CollideGetAABB( Vector &mins, Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles ) = 0;

	// Convert a bbox to a collide
	virtual CPhysCollide	*BBoxToCollide( const Vector &mins, const Vector &maxs ) = 0;


	// Trace an AABB against a collide
	virtual void TraceBox( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr ) = 0;
	virtual void TraceBox( const Ray_t &ray, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr ) = 0;

	// Trace one collide against another
	virtual void TraceCollide( const Vector &start, const Vector &end, const CPhysCollide *pSweepCollide, const QAngle &sweepAngles, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr ) = 0;

	// loads a set of solids into a vcollide_t
	virtual void			VCollideLoad( vcollide_t *pOutput, int solidCount, const char *pBuffer, int size ) = 0;
	// destroyts the set of solids created by VCollideLoad
	virtual void			VCollideUnload( vcollide_t *pVCollide ) = 0;

	// begins parsing a vcollide.  NOTE: This keeps pointers to the text
	// If you free the text and call members of IVPhysicsKeyParser, it will crash
	virtual IVPhysicsKeyParser	*VPhysicsKeyParserCreate( const char *pKeyData ) = 0;
	// Free the parser created by VPhysicsKeyParserCreate
	virtual void			VPhysicsKeyParserDestroy( IVPhysicsKeyParser *pParser ) = 0;

	// creates a list of verts from a collision mesh
	virtual int				CreateDebugMesh( CPhysCollide const *pCollisionModel, Vector **outVerts ) = 0;
	// destroy the list of verts created by CreateDebugMesh
	virtual void			DestroyDebugMesh( int vertCount, Vector *outVerts ) = 0;

	// create a queryable version of the collision model
	virtual ICollisionQuery *CreateQueryModel( CPhysCollide *pCollide ) = 0;
	// destroy the queryable version
	virtual void			DestroyQueryModel( ICollisionQuery *pQuery ) = 0;

	virtual IPhysicsCollision *ThreadContextCreate( void ) = 0;
	virtual void			ThreadContextDestroy( IPhysicsCollision *pThreadContex ) = 0;

	virtual unsigned int	ReadStat( int statID ) = 0;

	// UNDONE: Move this up when changing the interface version
	virtual void TraceBox( const Ray_t &ray, unsigned int contentsMask, IConvexInfo *pConvexInfo, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr ) = 0;
	virtual void			CollideGetMassCenter( CPhysCollide *pCollide, Vector *pOutMassCenter ) = 0;
	virtual void			CollideSetMassCenter( CPhysCollide *pCollide, const Vector &massCenter ) = 0;

	// query the collide index in the physics model for the instance
	virtual int				CollideIndex( const CPhysCollide *pCollide ) = 0;

	virtual CPhysCollide	*ConvertConvexToCollideParams( CPhysConvex **pConvex, int convexCount, const convertconvexparams_t &convertParams ) = 0;
	virtual CPhysConvex		*BBoxToConvex( const Vector &mins, const Vector &maxs ) = 0;

	// get the approximate cross-sectional area projected orthographically on the bbox of the collide
	// NOTE: These are fractional areas - unitless.  Basically this is the fraction of the OBB on each axis that
	// would be visible if the object were rendered orthographically.
	// NOTE: This has been precomputed when the collide was built or this function will return 1,1,1
	virtual Vector			CollideGetOrthographicAreas( const CPhysCollide *pCollide ) = 0;

	// dumps info about the collide to Msg()
	virtual void			OutputDebugInfo( const CPhysCollide *pCollide ) = 0;

	// relatively slow test for box vs. truncated cone
	virtual bool			IsBoxIntersectingCone( const Vector &boxAbsMins, const Vector &boxAbsMaxs, const truncatedcone_t &cone ) = 0;
};

// this can be used to post-process a collision model
abstract_class ICollisionQuery
{
public:
	virtual ~ICollisionQuery() {}
	// number of convex pieces in the whole solid
	virtual int		ConvexCount( void ) = 0;
	// triangle count for this convex piece
	virtual int		TriangleCount( int convexIndex ) = 0;
	// get the stored game data
	virtual unsigned int GetGameData( int convexIndex ) = 0;
	// Gets the triangle's verts to an array
	virtual void	GetTriangleVerts( int convexIndex, int triangleIndex, Vector *verts ) = 0;
	
	// UNDONE: This doesn't work!!!
	virtual void	SetTriangleVerts( int convexIndex, int triangleIndex, const Vector *verts ) = 0;
	
	// returns the 7-bit material index
	virtual int		GetTriangleMaterialIndex( int convexIndex, int triangleIndex ) = 0;
	// sets a 7-bit material index for this triangle
	virtual void	SetTriangleMaterialIndex( int convexIndex, int triangleIndex, int index7bits ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Ray traces from game engine.
//-----------------------------------------------------------------------------
abstract_class IPhysicsGameTrace
{
public:
	virtual void VehicleTraceRay( const Ray_t &ray, void *pVehicle, trace_t *pTrace ) = 0;
	virtual	void VehicleTraceRayWithWater( const Ray_t &ray, void *pVehicle, trace_t *pTrace ) = 0;
	virtual bool VehiclePointInWater( const Vector &vecPoint ) = 0;
};

// The caller should implement this to return contents masks per convex on a collide
abstract_class IConvexInfo
{
public:
	virtual unsigned int GetContents( int convexGameData ) = 0;
};

class CPhysicsEventHandler;
abstract_class IPhysicsCollisionData
{
public:
	virtual void GetSurfaceNormal( Vector &out ) = 0;		// normal points toward second object (object index 1)
	virtual void GetContactPoint( Vector &out ) = 0;		// contact point of collision (in world space)
	virtual void GetContactSpeed( Vector &out ) = 0;		// speed of surface 1 relative to surface 0 (in world space)
};


struct vcollisionevent_t
{
	IPhysicsObject	*pObjects[2];
	int				surfaceProps[2];
	bool			isCollision;
	bool			isShadowCollision;
	float			deltaCollisionTime;

	float			collisionSpeed;				// only valid at postCollision
	IPhysicsCollisionData *pInternalData;		// may change pre/post collision
};

abstract_class IPhysicsCollisionEvent
{
public:
	// returns the two objects that collided, time between last collision of these objects
	// and an opaque data block of collision information
	// NOTE: PreCollision/PostCollision ALWAYS come in matched pairs!!!
	virtual void PreCollision( vcollisionevent_t *pEvent ) = 0;
	virtual void PostCollision( vcollisionevent_t *pEvent ) = 0;

	// This is a scrape event.  The object has scraped across another object consuming the indicated energy
	virtual void Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData ) = 0;

	virtual void StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData ) = 0;
	virtual void EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData ) = 0;

	virtual void FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) = 0;
	virtual void FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) = 0;

	virtual void PostSimulationFrame() = 0;

	virtual void ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}
	virtual void ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}
};


abstract_class IPhysicsObjectEvent
{
public:
	// these can be used to optimize out queries on sleeping objects
	// Called when an object is woken after sleeping
	virtual void ObjectWake( IPhysicsObject *pObject ) = 0;
	// called when an object goes to sleep (no longer simulating)
	virtual void ObjectSleep( IPhysicsObject *pObject ) = 0;
};

class IPhysicsConstraintEvent
{
public:
	// the constraint is now inactive, the game code is required to delete it or re-activate it.
	virtual void ConstraintBroken( IPhysicsConstraint * ) = 0;
};

struct hlshadowcontrol_params_t
{
	Vector			targetPosition;
	QAngle			targetRotation;
	float			maxAngular;
	float			maxDampAngular;
	float			maxSpeed;
	float			maxDampSpeed;
	float			dampFactor;
	float			teleportDistance;
};

// UNDONE: At some point allow this to be parameterized using hlshadowcontrol_params_t.
// All of the infrastructure is in place to do that.
abstract_class IPhysicsShadowController
{
public:
	virtual ~IPhysicsShadowController( void ) {}

	virtual void Update( const Vector &position, const QAngle &angles, float timeOffset ) = 0;
	virtual void MaxSpeed( float maxSpeed, float maxAngularSpeed ) = 0;
	virtual void StepUp( float height ) = 0;
	
	// If the teleport distance is non-zero, the object will be teleported to 
	// the target location when the error exceeds this quantity.
	virtual void SetTeleportDistance( float teleportDistance ) = 0;
	virtual bool AllowsTranslation() = 0;
	virtual bool AllowsRotation() = 0;

	// There are two classes of shadow objects:
	// 1) Game physics controlled, shadow follows game physics (this is the default)
	// 2) Physically controlled - shadow position is a target, but the game hasn't guaranteed that the space can be occupied by this object
	virtual void SetPhysicallyControlled( bool isPhysicallyControlled ) = 0;
	virtual bool IsPhysicallyControlled() = 0;
	virtual void GetLastImpulse( Vector *pOut ) = 0;
	virtual void UseShadowMaterial( bool bUseShadowMaterial ) = 0;
	virtual void ObjectMaterialChanged( int materialIndex ) = 0;
};

class CPhysicsSimObject;
class IPhysicsMotionController;

// Callback for simulation
class IMotionEvent
{
public:
	// These constants instruct the simulator as to how to apply the values copied to linear & angular
	// GLOBAL/LOCAL refer to the coordinate system of the values, whereas acceleration/force determine whether or not
	// mass is divided out (forces must be divided by mass to compute acceleration)
	enum simresult_e { SIM_NOTHING = 0, SIM_LOCAL_ACCELERATION, SIM_LOCAL_FORCE, SIM_GLOBAL_ACCELERATION, SIM_GLOBAL_FORCE };
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular ) = 0;
};



abstract_class IPhysicsMotionController
{
public:
	virtual ~IPhysicsMotionController( void ) {}
	virtual void SetEventHandler( IMotionEvent *handler ) = 0;
	virtual void AttachObject( IPhysicsObject *pObject, bool checkIfAlreadyAttached ) = 0;
	virtual void DetachObject( IPhysicsObject *pObject ) = 0;

	// returns the number of objects currently attached to the controller
	virtual int CountObjects( void ) = 0;
	// NOTE: pObjectList is an array with at least CountObjects() allocated
	virtual void GetObjects( IPhysicsObject **pObjectList ) = 0;
	// detaches all attached objects
	virtual void ClearObjects( void ) = 0;

	// wakes up all attached objects
	virtual void WakeObjects( void ) = 0;
	enum priority_t
	{
		LOW_PRIORITY = 0,
		MEDIUM_PRIORITY = 1,
		HIGH_PRIORITY = 2,
	};
	virtual void SetPriority( priority_t priority ) = 0;
};

// -------------------
// Collision filter function.  Return 0 if objects should not be tested for collisions, nonzero otherwise
// Install with IPhysicsEnvironment::SetCollisionFilter()
// -------------------
abstract_class IPhysicsCollisionSolver
{
public:
	virtual int ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 ) = 0;
	virtual int ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt ) = 0;
	
	// pObject has already done the max number of collisions this tick, should we freeze it to save CPU?
	virtual bool ShouldFreezeObject( IPhysicsObject *pObject ) = 0;

	// The system has already done too many collision checks, performance will suffer.
	// How many more should it do?
	virtual int AdditionalCollisionChecksThisTick( int currentChecksDone ) = 0;
};

enum PhysicsTraceType_t
{
	VPHYSICS_TRACE_EVERYTHING = 0,
	VPHYSICS_TRACE_STATIC_ONLY,
	VPHYSICS_TRACE_MOVING_ONLY,
	VPHYSICS_TRACE_TRIGGERS_ONLY,
	VPHYSICS_TRACE_STATIC_AND_MOVING,
};

abstract_class IPhysicsTraceFilter
{
public:
	virtual bool ShouldHitObject( IPhysicsObject *pObject, int contentsMask ) = 0;
	virtual PhysicsTraceType_t	GetTraceType() const = 0;
};

abstract_class IPhysicsEnvironment
{
public:
	virtual ~IPhysicsEnvironment( void ) {}

	virtual void SetDebugOverlay( CreateInterfaceFn debugOverlayFactory ) = 0;
	virtual IVPhysicsDebugOverlay *GetDebugOverlay( void ) = 0;

	// gravity is a 3-vector in in/s^2
	virtual void			SetGravity( const Vector &gravityVector ) = 0;
	virtual void			GetGravity( Vector &gravityVector ) = 0;

	// air density is in kg / m^3 (water is 1000)
	// This controls drag, air that is more dense has more drag.
	virtual void			SetAirDensity( float density ) = 0;
	virtual float			GetAirDensity( void ) = 0;
	
	// object creation
	// create a polygonal object.  pCollisionModel was created by the physics builder DLL in a pre-process.
	virtual IPhysicsObject	*CreatePolyObject( const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams ) = 0;
	// same as above, but this one cannot move or rotate (infinite mass/inertia)
	virtual IPhysicsObject	*CreatePolyObjectStatic( const CPhysCollide *pCollisionModel, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams ) = 0;
	// Create a perfectly spherical object
	virtual IPhysicsObject *CreateSphereObject( float radius, int materialIndex, const Vector &position, const QAngle &angles, objectparams_t *pParams, bool isStatic ) = 0;
	// Create a polygonal fluid body out of the specified collision model
	// This object will affect any other objects that collide with the collision model
	virtual IPhysicsFluidController	*CreateFluidController( IPhysicsObject *pFluidObject, fluidparams_t *pParams ) = 0;

	// Create a simulated spring that connects 2 objects
	virtual IPhysicsSpring	*CreateSpring( IPhysicsObject *pObjectStart, IPhysicsObject *pObjectEnd, springparams_t *pParams ) = 0;

	// Create a constraint in the space of pReferenceObject which is attached by the constraint to pAttachedObject
	virtual IPhysicsConstraint	*CreateRagdollConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ragdollparams_t &ragdoll ) = 0;
	virtual IPhysicsConstraint	*CreateHingeConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_hingeparams_t &hinge ) = 0;
	virtual IPhysicsConstraint	*CreateFixedConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_fixedparams_t &fixed ) = 0;
	virtual IPhysicsConstraint	*CreateSlidingConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_slidingparams_t &sliding ) = 0;
	virtual IPhysicsConstraint	*CreateBallsocketConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_ballsocketparams_t &ballsocket ) = 0;
	virtual IPhysicsConstraint *CreatePulleyConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_pulleyparams_t &pulley ) = 0;
	virtual IPhysicsConstraint *CreateLengthConstraint( IPhysicsObject *pReferenceObject, IPhysicsObject *pAttachedObject, IPhysicsConstraintGroup *pGroup, const constraint_lengthparams_t &length ) = 0;

	virtual IPhysicsConstraintGroup *CreateConstraintGroup( const constraint_groupparams_t &groupParams ) = 0;

	// destroy an object created with CreatePolyObject() or CreatePolyObjectStatic()
	virtual void DestroyObject( IPhysicsObject * ) = 0;
	virtual void DestroySpring( IPhysicsSpring * ) = 0;
	// Destroy an object created with CreateFluidController()
	virtual void DestroyFluidController( IPhysicsFluidController * ) = 0;
	virtual void DestroyConstraint( IPhysicsConstraint * ) = 0;
	virtual void DestroyConstraintGroup( IPhysicsConstraintGroup *pGroup ) = 0;

	// install a function to filter collisions/penentration
	virtual void			SetCollisionSolver( IPhysicsCollisionSolver *pSolver ) = 0;

	// run the simulator for deltaTime seconds
	virtual void			Simulate( float deltaTime ) = 0;
	// true if currently running the simulator (i.e. in a callback during physenv->Simulate())
	virtual bool			IsInSimulation( void ) const = 0;

	// Manage the timestep (period) of the simulator.  The main functions are all integrated with
	// this period as dt.
	virtual float			GetSimulationTimestep( void ) = 0;
	virtual void			SetSimulationTimestep( float timestep ) = 0;

	// returns the current simulation clock's value.  This is an absolute time.
	virtual float			GetSimulationTime( void ) = 0;
	virtual void			ResetSimulationClock( void ) = 0;

	// Collision callbacks (game code collision response)
	virtual void			SetCollisionEventHandler( IPhysicsCollisionEvent *pCollisionEvents ) = 0;
	virtual void			SetObjectEventHandler( IPhysicsObjectEvent *pObjectEvents ) = 0;
	virtual	void			SetConstraintEventHandler( IPhysicsConstraintEvent *pConstraintEvents ) = 0;

	virtual IPhysicsShadowController *CreateShadowController( IPhysicsObject *pObject, bool allowTranslation, bool allowRotation ) = 0;
	virtual void						DestroyShadowController( IPhysicsShadowController * ) = 0;

	virtual IPhysicsPlayerController	*CreatePlayerController( IPhysicsObject *pObject ) = 0;
	virtual void						DestroyPlayerController( IPhysicsPlayerController * ) = 0;

	virtual IPhysicsMotionController	*CreateMotionController( IMotionEvent *pHandler ) = 0;
	virtual void						DestroyMotionController( IPhysicsMotionController *pController ) = 0;

	virtual IPhysicsVehicleController	*CreateVehicleController( IPhysicsObject *pVehicleBodyObject, const vehicleparams_t &params, unsigned int nVehicleType, IPhysicsGameTrace *pGameTrace ) = 0;
	virtual void						DestroyVehicleController( IPhysicsVehicleController * ) = 0;

	virtual void					SetQuickDelete( bool bQuick ) = 0;

	virtual int						GetActiveObjectCount( void ) = 0;
	virtual void					GetActiveObjects( IPhysicsObject **pOutputObjectList ) = 0;

	virtual void				CleanupDeleteList( void ) = 0;
	virtual void				EnableDeleteQueue( bool enable ) = 0;

	// Save/Restore methods
	virtual bool Save( const physsaveparams_t &params ) = 0;
	virtual void PreRestore( const physprerestoreparams_t &params ) = 0;
	virtual bool Restore( const physrestoreparams_t &params ) = 0;
	virtual void PostRestore() = 0;

	// Debugging:
	virtual bool IsCollisionModelUsed( CPhysCollide *pCollide ) = 0;
	
	// Physics world version of the enginetrace API:
	virtual void TraceRay( const Ray_t &ray, unsigned int fMask, IPhysicsTraceFilter *pTraceFilter, trace_t *pTrace ) = 0;
	virtual void SweepCollideable( const CPhysCollide *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
		const QAngle &vecAngles, unsigned int fMask, IPhysicsTraceFilter *pTraceFilter, trace_t *pTrace ) = 0;

	// performance tuning
	virtual void GetPerformanceSettings( physics_performanceparams_t *pOutput ) = 0;
	virtual void SetPerformanceSettings( const physics_performanceparams_t *pSettings ) = 0;

	// perf/cost statistics
	virtual void ReadStats( physics_stats_t *pOutput ) = 0;
	virtual void ClearStats() = 0;
};

enum callbackflags
{
	CALLBACK_GLOBAL_COLLISION	= 0x0001,
	CALLBACK_GLOBAL_FRICTION	= 0x0002,
	CALLBACK_GLOBAL_TOUCH		= 0x0004,
	CALLBACK_GLOBAL_TOUCH_STATIC = 0x0008,
	CALLBACK_SHADOW_COLLISION	= 0x0010,
	CALLBACK_GLOBAL_COLLIDE_STATIC = 0x0020,
	CALLBACK_IS_VEHICLE_WHEEL	= 0x0040,
	CALLBACK_FLUID_TOUCH		= 0x0100,
	CALLBACK_NEVER_DELETED		= 0x0200,	// HACKHACK: This means this object will never be deleted (set on the world)
	CALLBACK_MARKED_FOR_DELETE	= 0x0400,	// This allows vphysics to skip some work for this object since it will be
											// deleted later this frame. (Set automatically by destroy calls)
	CALLBACK_ENABLING_COLLISION = 0x0800,	// This is active during the time an object is enabling collisions
											// allows us to skip collisions between "new" objects and objects marked for delete
	CALLBACK_DO_FLUID_SIMULATION = 0x1000,  // remove this to opt out of fluid simulations
	CALLBACK_IS_PLAYER_CONTROLLER= 0x2000,	// HACKHACK: Set this on players until player cotrollers are unified with shadow controllers
	CALLBACK_CHECK_COLLISION_DISABLE = 0x4000,
	CALLBACK_MARKED_FOR_TEST	= 0x8000,	// debug -- marked object is being debugged
};

abstract_class IPhysicsObject
{
public:
	virtual ~IPhysicsObject( void ) {}

	// returns true if this object is static/unmoveable
	// NOTE: returns false for objects that are not created static, but set EnableMotion(false);
	// Call IsMoveable() to find if the object is static OR has motion disabled
	virtual bool			IsStatic( void ) = 0;

	// "wakes up" an object
	// NOTE: ALL OBJECTS ARE "Asleep" WHEN CREATED
	virtual void			Wake( void ) = 0;
	virtual void			Sleep( void ) = 0;
	virtual bool			IsAsleep( void ) = 0;

	// Game can store data in each object (link back to game object)
	virtual void			SetGameData( void *pGameData ) = 0;
	virtual void			*GetGameData( void ) const = 0;
	// This flags word can be defined by the game as well
	virtual void			SetGameFlags( unsigned short userFlags ) = 0;
	virtual unsigned short	GetGameFlags( void ) const = 0;
	virtual void			SetGameIndex( unsigned short gameIndex ) = 0;
	virtual unsigned short	GetGameIndex( void ) const = 0;

	// setup various callbacks for this object
	virtual void			SetCallbackFlags( unsigned short callbackflags ) = 0;
	// get the current callback state for this object
	virtual unsigned short	GetCallbackFlags( void ) = 0;

	// mass accessors
	virtual void			SetMass( float mass ) = 0;
	virtual float			GetMass( void ) const = 0;
	// get 1/mass (it's cached)
	virtual float			GetInvMass( void ) const = 0;
	virtual Vector			GetInertia( void ) const = 0;
	virtual Vector			GetInvInertia( void ) const = 0;
	virtual void			SetInertia( const Vector &inertia ) = 0;

	virtual void			SetDamping( const float *speed, const float *rot ) = 0;
	virtual void			GetDamping( float *speed, float *rot ) = 0;

	// material index
	virtual int				GetMaterialIndex() const = 0;
	virtual void			SetMaterialIndex( int materialIndex ) = 0;

	// Enable / disable collisions for this object
	virtual void			EnableCollisions( bool enable ) = 0;
	// Enable / disable gravity for this object
	virtual void			EnableGravity( bool enable ) = 0;
	// Enable / disable air friction / drag for this object
	virtual void			EnableDrag( bool enable ) = 0;
	// Enable / disable motion (pin / unpin the object)
	virtual void			EnableMotion( bool enable ) = 0;

	// call this when the collision filter conditions change due to this 
	// object's state (e.g. changing solid type or collision group)
	virtual void			RecheckCollisionFilter() = 0;

	// NOTE:	These are here for convenience, but you can do them yourself by using the matrix
	//			returned from GetPositionMatrix()
	// convenient coordinate system transformations (params - dest, src)
	virtual void			LocalToWorld( Vector *worldPosition, const Vector &localPosition ) = 0;
	virtual void			WorldToLocal( Vector *localPosition, const Vector &worldPosition ) = 0;

	// transforms a vector (no translation) from object-local to world space
	virtual void			LocalToWorldVector( Vector *worldVector, const Vector &localVector ) = 0;
	// transforms a vector (no translation) from world to object-local space
	virtual void			WorldToLocalVector( Vector *localVector, const Vector &worldVector ) = 0;
	
	// push on an object
	// force vector is direction & magnitude of impulse kg in / s
	virtual void			ApplyForceCenter( const Vector &forceVector ) = 0;
	virtual void			ApplyForceOffset( const Vector &forceVector, const Vector &worldPosition ) = 0;

	// Calculates the force/torque on the center of mass for an offset force impulse (pass output to ApplyForceCenter / ApplyTorqueCenter)
	virtual void			CalculateForceOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerForce, AngularImpulse *centerTorque ) = 0;
	// Calculates the linear/angular velocities on the center of mass for an offset force impulse (pass output to AddVelocity)
	virtual void			CalculateVelocityOffset( const Vector &forceVector, const Vector &worldPosition, Vector *centerVelocity, AngularImpulse *centerAngularVelocity ) = 0;

	// apply torque impulse.  This will change the angular velocity on the object.
	// HL Axes, kg degrees / s
	virtual void			ApplyTorqueCenter( const AngularImpulse &torque ) = 0;

	// NOTE: This will teleport the object
	virtual void			SetPosition( const Vector &worldPosition, const QAngle &angles, bool isTeleport ) = 0;
	virtual void			SetPositionMatrix( const matrix3x4_t&matrix, bool isTeleport ) = 0;

	virtual void			GetPosition( Vector *worldPosition, QAngle *angles ) = 0;
	virtual void			GetPositionMatrix( matrix3x4_t *positionMatrix ) = 0;
	// force the velocity to a new value
	// NOTE: velocity is in worldspace, angularVelocity is relative to the object's 
	// local axes (just like pev->velocity, pev->avelocity)
	virtual void			SetVelocity( const Vector *velocity, const AngularImpulse *angularVelocity ) = 0;

	// like the above, but force the change into the simulator immediately
	virtual void			SetVelocityInstantaneous( const Vector *velocity, const AngularImpulse *angularVelocity ) = 0;

	// NOTE: velocity is in worldspace, angularVelocity is relative to the object's 
	// local axes (just like pev->velocity, pev->avelocity)
	virtual void			GetVelocity( Vector *velocity, AngularImpulse *angularVelocity ) = 0;

	// NOTE: These are velocities, not forces.  i.e. They will have the same effect regardless of
	// the object's mass or inertia
	virtual void			AddVelocity( const Vector *velocity, const AngularImpulse *angularVelocity ) = 0;
	virtual void			GetVelocityAtPoint( const Vector &worldPosition, Vector &velocity ) = 0;
	
	virtual float			GetEnergy() = 0;

	// returns true if the object is in contact with another object
	// if true, puts a point on the contact surface in contactPoint, and
	// a pointer to the object in contactObject
	// NOTE: You can pass NULL for either to avoid computations
	// JAY: This is still an experiment
	virtual bool			GetContactPoint( Vector *contactPoint, IPhysicsObject **contactObject ) = 0;

	// refactor this a bit - move some of this to IPhysicsShadowController
	virtual void			SetShadow( float maxSpeed, float maxAngularSpeed, bool allowPhysicsMovement, bool allowPhysicsRotation ) = 0;
	virtual void			UpdateShadow( const Vector &targetPosition, const QAngle &targetAngles, bool tempDisableGravity, float timeOffset ) = 0;
	
	// returns number of ticks since last Update() call
	virtual int				GetShadowPosition( Vector *position, QAngle *angles ) = 0;
	virtual IPhysicsShadowController *GetShadowController( void ) const = 0;


	virtual const CPhysCollide			*GetCollide( void ) const = 0;
	virtual const char					*GetName() = 0;
	virtual void			RemoveShadowController() = 0;
	virtual bool			IsMoveable() = 0;

	// applies the math of the shadow controller to this object.
	// for use in your own controllers
	// returns the new value of secondsToArrival with dt time elapsed
	virtual float			ComputeShadowControl( const hlshadowcontrol_params_t &params, float secondsToArrival, float dt ) = 0;

	// coefficients are optional, pass either
	virtual void			SetDragCoefficient( float *pDrag, float *pAngularDrag ) = 0;

	// Get the radius if this is a sphere object (zero if this is a polygonal mesh)
	virtual float			GetSphereRadius() = 0;

	virtual float			CalculateLinearDrag( const Vector &unitDirection ) const = 0;
	virtual float			CalculateAngularDrag( const Vector &objectSpaceRotationAxis ) const = 0;
	virtual void			SetBuoyancyRatio( float ratio ) = 0;			// Override bouyancy

	virtual void			BecomeTrigger() = 0;
	virtual void			RemoveTrigger() = 0;
	virtual bool			IsTrigger() = 0;
	virtual bool			IsFluid() = 0;		// fluids are special triggers with fluid controllers attached, they return true to IsTrigger() as well!

	// sets the object to be hinged.  Fixed it place, but able to rotate around one axis.
	virtual void			BecomeHinged( int localAxis ) = 0;
	// resets the object to original state
	virtual void			RemoveHinged() = 0;
	virtual bool			IsHinged() = 0;

	virtual unsigned int	GetContents() = 0;
	virtual void			SetContents( unsigned int contents ) = 0;
	virtual Vector			GetMassCenterLocalSpace() = 0;
	
	// used to iterate the contact points of an object
	virtual IPhysicsFrictionSnapshot *CreateFrictionSnapshot() = 0;
	virtual void DestroyFrictionSnapshot( IPhysicsFrictionSnapshot *pSnapshot ) = 0;

	// dumps info about the object to Msg()
	virtual void			OutputDebugInfo() = 0;
	virtual void			GetImplicitVelocity( Vector *velocity, AngularImpulse *angularVelocity ) = 0;
	// this is a hack to recheck current contacts
	// some of them may not be valid if the object's collision rules have recently changed
	// UNDONE: Force this in RecheckCollisionFilter() ?
	virtual void			RecheckContactPoints() = 0;
};


abstract_class IPhysicsSpring
{
public:
	virtual ~IPhysicsSpring( void ) {}
	virtual void			GetEndpoints( Vector *worldPositionStart, Vector *worldPositionEnd ) = 0;
	virtual void			SetSpringConstant( float flSpringContant) = 0;
	virtual void			SetSpringDamping( float flSpringDamping) = 0;
	virtual void			SetSpringLength( float flSpringLenght) = 0;

	// Get the starting object
	virtual IPhysicsObject *GetStartObject( void ) = 0;

	// Get the end object
	virtual IPhysicsObject *GetEndObject( void ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: These properties are defined per-material.  This is accessible at 
//			each triangle in a collision mesh
//-----------------------------------------------------------------------------
struct surfacephysicsparams_t
{
// vphysics physical properties
	float			friction;
	float			elasticity;				// collision elasticity - used to compute coefficient of restitution
	float			density;				// physical density (in kg / m^3)
	float			thickness;				// material thickness if not solid (sheet materials) in inches
	float			dampening;
};

struct surfaceaudioparams_t
{
// sounds / audio data
	float			reflectivity;		// like elasticity, but how much sound should be reflected by this surface
	float			hardnessFactor;	// like elasticity, but only affects impact sound choices
	float			roughnessFactor;	// like friction, but only affects scrape sound choices

// audio thresholds
	float			roughThreshold;	// surface roughness > this causes "rough" scrapes, < this causes "smooth" scrapes
	float			hardThreshold;	// surface hardness > this causes "hard" impacts, < this causes "soft" impacts
	float			hardVelocityThreshold;	// collision velocity > this causes "hard" impacts, < this causes "soft" impacts
									// NOTE: Hard impacts must meet both hardnessFactor AND velocity thresholds
};

struct surfacesoundnames_t
{
	unsigned short	stepleft;
	unsigned short	stepright;

	unsigned short	impactSoft;
	unsigned short	impactHard;

	unsigned short	scrapeSmooth;
	unsigned short	scrapeRough;

	unsigned short	bulletImpact;
	unsigned short	rolling;

	unsigned short	breakSound;
	unsigned short	strainSound;
};

struct surfacegameprops_t
{
// game movement data
	float			maxSpeedFactor;			// Modulates player max speed when walking on this surface
	float			jumpFactor;				// Indicates how much higher the player should jump when on the surface
// Game-specific data
	unsigned short	material;
	// Indicates whether or not the player is on a ladder.
	unsigned char	climbable;
	unsigned char	pad;
};

//-----------------------------------------------------------------------------
// Purpose: Each different material has an entry like this
//-----------------------------------------------------------------------------
struct surfacedata_t
{
	surfacephysicsparams_t	physics;	// physics parameters
	surfaceaudioparams_t	audio;		// audio parameters
	surfacesoundnames_t		sounds;		// names of linked sounds
	surfacegameprops_t		game;		// Game data / properties


};

#define VPHYSICS_SURFACEPROPS_INTERFACE_VERSION_1	"VPhysicsSurfaceProps001"
abstract_class IPhysicsSurfaceProps
{
public:
	virtual ~IPhysicsSurfaceProps( void ) {}

	// parses a text file containing surface prop keys
	virtual int		ParseSurfaceData( const char *pFilename, const char *pTextfile ) = 0;
	// current number of entries in the database
	virtual int		SurfacePropCount( void ) = 0;

	virtual int		GetSurfaceIndex( const char *pSurfacePropName ) = 0;
	virtual void	GetPhysicsProperties( int surfaceDataIndex, float *density, float *thickness, float *friction, float *elasticity ) = 0;

	virtual surfacedata_t	*GetSurfaceData( int surfaceDataIndex ) = 0;
	virtual const char		*GetString( unsigned short stringTableIndex ) = 0;


	virtual const char		*GetPropName( int surfaceDataIndex ) = 0;

	// sets the global index table for world materials
	virtual void	SetWorldMaterialIndexTable( int *pMapArray, int mapSize ) = 0;

	// NOTE: Same as GetPhysicsProperties, but maybe more convenient
	virtual void	GetPhysicsParameters( int surfaceDataIndex, surfacephysicsparams_t *pParamsOut ) = 0;
};

abstract_class IPhysicsFluidController
{
public:
	virtual ~IPhysicsFluidController( void ) {}

	virtual void	SetGameData( void *pGameData ) = 0;
	virtual void	*GetGameData( void ) const = 0;

	virtual void	GetSurfacePlane( Vector *pNormal, float *pDist ) = 0;
	virtual float	GetDensity() = 0;
	virtual void	WakeAllSleepingObjects() = 0;
	virtual int		GetContents() const = 0;
};


//-----------------------------------------------------------------------------
// Purpose: parameter block for creating fluid dynamic motion
// UNDONE: Expose additional fluid model paramters?
//-----------------------------------------------------------------------------
struct fluidparams_t
{
	Vector4D	surfacePlane;	// x,y,z normal, dist (plane constant) fluid surface
	Vector		currentVelocity; // velocity of the current in inches/second
	float		damping;		// damping factor for buoyancy (tweak)
	float		torqueFactor;
	float		viscosityFactor;
	void		*pGameData;
	bool		useAerodynamics;// true if this controller should calculate surface pressure
	int			contents;

	fluidparams_t() {}
	fluidparams_t( fluidparams_t const& src )
	{
		Vector4DCopy( src.surfacePlane, surfacePlane );
		VectorCopy( src.currentVelocity, currentVelocity );
		damping = src.damping;
		torqueFactor = src.torqueFactor;
		viscosityFactor = src.viscosityFactor;
		contents = src.contents;
	}
};

//-----------------------------------------------------------------------------
// Purpose: parameter block for creating linear springs
// UNDONE: Expose additional spring model paramters?
//-----------------------------------------------------------------------------
struct springparams_t
{
	springparams_t()
	{
		memset( this, 0, sizeof(*this) );
	}
	float	constant;		// spring constant
	float	naturalLength;// relaxed length
	float	damping;		// damping factor
	float	relativeDamping;	// relative damping (damping proportional to the change in the relative position of the objects)
	Vector	startPosition;
	Vector	endPosition;
	bool	useLocalPositions;	// start & end Position are in local space to start and end objects if this is true
	bool	onlyStretch;		// only apply forces when the length is greater than the natural length
};

//-----------------------------------------------------------------------------
// Purpose: parameter block for creating polygonal objects
//-----------------------------------------------------------------------------
struct objectparams_t
{
	Vector		*massCenterOverride;
	float		mass;
	float		inertia;
	float		damping;
	float		rotdamping;
	float		rotInertiaLimit;
	const char	*pName;				// used only for debugging
	void		*pGameData;
	float		volume;
	float		dragCoefficient;
	bool		enableCollisions;
};

struct convertconvexparams_t
{
	bool		buildOuterConvexHull;
	bool		buildDragAxisAreas;
	bool		buildOptimizedTraceTables;
	float		dragAreaEpsilon;
	CPhysConvex *pForcedOuterHull;

	void Defaults()
	{
		dragAreaEpsilon = 0.25f; // 0.5in x 0.5in square
		buildOuterConvexHull = false;
		buildDragAxisAreas = false;
		buildOptimizedTraceTables = false;
		pForcedOuterHull = NULL;
	}
};

//-----------------------------------------------------------------------------
// Physics interface IDs
//
// Note that right now the order of the enum also defines the order of save/load


//-----------------------------------------------------------------------------
// Purpose: parameter blocks for save and load operations
//-----------------------------------------------------------------------------
struct physsaveparams_t
{
	ISave 				*pSave;
	void 				*pObject;
	PhysInterfaceId_t 	type;
};

struct physrestoreparams_t
{
	IRestore 			*pRestore;
	void 				**ppObject;
	PhysInterfaceId_t 	type;
	void 				*pGameData;
	const char			*pName;				// used only for debugging
	const CPhysCollide 	*pCollisionModel;
	IPhysicsEnvironment *pEnvironment;
	IPhysicsGameTrace	*pGameTrace;
};

struct physrecreateparams_t
{
	void *pOldObject;
	void *pNewObject;
};

struct physprerestoreparams_t
{
	int recreatedObjectCount;
	physrecreateparams_t recreatedObjectList[1];
};


//-------------------------------------

#define DEFINE_PIID( type, enumval ) \
	template <> inline PhysInterfaceId_t GetPhysIID<type>( type ** ) { return enumval; }

template <class PHYSPTR> inline PhysInterfaceId_t GetPhysIID(PHYSPTR **); // will get link error if no match

DEFINE_PIID( IPhysicsObject, 			PIID_IPHYSICSOBJECT );
DEFINE_PIID( IPhysicsFluidController, 	PIID_IPHYSICSFLUIDCONTROLLER );
DEFINE_PIID( IPhysicsSpring, 			PIID_IPHYSICSSPRING );
DEFINE_PIID( IPhysicsConstraintGroup, 	PIID_IPHYSICSCONSTRAINTGROUP );
DEFINE_PIID( IPhysicsConstraint, 		PIID_IPHYSICSCONSTRAINT );
DEFINE_PIID( IPhysicsShadowController, 	PIID_IPHYSICSSHADOWCONTROLLER );
DEFINE_PIID( IPhysicsPlayerController,	PIID_IPHYSICSPLAYERCONTROLLER );
DEFINE_PIID( IPhysicsMotionController,	PIID_IPHYSICSMOTIONCONTROLLER );
DEFINE_PIID( IPhysicsVehicleController,	PIID_IPHYSICSVEHICLECONTROLLER );
DEFINE_PIID( IPhysicsGameTrace,			PIID_IPHYSICSGAMETRACE );

//-----------------------------------------------------------------------------

} // end namespace VPhysicsInterfaceV30

#endif // VPHYSICS_INTERFACE_V30_H
