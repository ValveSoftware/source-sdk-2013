//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// nav.h
// Data structures and constants for the Navigation Mesh system
// Author: Michael S. Booth (mike@turtlerockstudios.com), January 2003

#ifndef _NAV_H_
#define _NAV_H_

#include "modelentities.h"		// for CFuncBrush
#include "doors.h"

/**
 * Below are several constants used by the navigation system.
 * @todo Move these into TheNavMesh singleton.
 */
const float GenerationStepSize = 25.0f;			// (30) was 20, but bots can't fit always fit
const float JumpHeight = 41.8f;					// if delta Z is less than this, we can jump up on it

#if defined(CSTRIKE_DLL)
const float JumpCrouchHeight = 58.0f;			// (48) if delta Z is less than or equal to this, we can jumpcrouch up on it
#else
const float JumpCrouchHeight = 64.0f;			// (48) if delta Z is less than or equal to this, we can jumpcrouch up on it
#endif

// There are 3 different definitions of StepHeight throughout the code, waiting to produce bugs if the 18.0 is ever changed.
const float StepHeight = 18.0f;					// if delta Z is greater than this, we have to jump to get up

// TERROR: Increased DeathDrop from 200, since zombies don't take falling damage
#if defined(CSTRIKE_DLL)
const float DeathDrop = 200.0f;					// (300) distance at which we will die if we fall - should be about 600, and pay attention to fall damage during pathfind
#else
const float DeathDrop = 400.0f;					// (300) distance at which we will die if we fall - should be about 600, and pay attention to fall damage during pathfind
#endif

#if defined(CSTRIKE_DLL)
const float ClimbUpHeight = JumpCrouchHeight;	// CSBots assume all jump up links are reachable
#else
const float ClimbUpHeight = 200.0f;				// height to check for climbing up
#endif

const float CliffHeight = 300.0f;				// height which we consider a significant cliff which we would not want to fall off of

// TERROR: Converted these values to use the same numbers as the player bounding boxes etc
#define HalfHumanWidth			16
#define HalfHumanHeight			35.5
#define HumanHeight				71
#define HumanEyeHeight			62
#define HumanCrouchHeight		55
#define HumanCrouchEyeHeight	37


#define NAV_MAGIC_NUMBER 0xFEEDFACE				// to help identify nav files

/**
 * A place is a named group of navigation areas
 */
typedef unsigned int Place;
#define UNDEFINED_PLACE 0				// ie: "no place"
#define ANY_PLACE 0xFFFF

enum NavErrorType
{
	NAV_OK,
	NAV_CANT_ACCESS_FILE,
	NAV_INVALID_FILE,
	NAV_BAD_FILE_VERSION,
	NAV_FILE_OUT_OF_DATE,
	NAV_CORRUPT_DATA,
	NAV_OUT_OF_MEMORY,
};

enum NavAttributeType
{
	NAV_MESH_INVALID		= 0,
	NAV_MESH_CROUCH			= 0x00000001,				// must crouch to use this node/area
	NAV_MESH_JUMP			= 0x00000002,				// must jump to traverse this area (only used during generation)
	NAV_MESH_PRECISE		= 0x00000004,				// do not adjust for obstacles, just move along area
	NAV_MESH_NO_JUMP		= 0x00000008,				// inhibit discontinuity jumping
	NAV_MESH_STOP			= 0x00000010,				// must stop when entering this area
	NAV_MESH_RUN			= 0x00000020,				// must run to traverse this area
	NAV_MESH_WALK			= 0x00000040,				// must walk to traverse this area
	NAV_MESH_AVOID			= 0x00000080,				// avoid this area unless alternatives are too dangerous
	NAV_MESH_TRANSIENT		= 0x00000100,				// area may become blocked, and should be periodically checked
	NAV_MESH_DONT_HIDE		= 0x00000200,				// area should not be considered for hiding spot generation
	NAV_MESH_STAND			= 0x00000400,				// bots hiding in this area should stand
	NAV_MESH_NO_HOSTAGES	= 0x00000800,				// hostages shouldn't use this area
	NAV_MESH_STAIRS			= 0x00001000,				// this area represents stairs, do not attempt to climb or jump them - just walk up
	NAV_MESH_NO_MERGE		= 0x00002000,				// don't merge this area with adjacent areas
	NAV_MESH_OBSTACLE_TOP	= 0x00004000,				// this nav area is the climb point on the tip of an obstacle
	NAV_MESH_CLIFF			= 0x00008000,				// this nav area is adjacent to a drop of at least CliffHeight

	NAV_MESH_FIRST_CUSTOM	= 0x00010000,				// apps may define custom app-specific bits starting with this value
	NAV_MESH_LAST_CUSTOM	= 0x04000000,				// apps must not define custom app-specific bits higher than with this value

	NAV_MESH_FUNC_COST		= 0x20000000,				// area has designer specified cost controlled by func_nav_cost entities
	NAV_MESH_HAS_ELEVATOR	= 0x40000000,				// area is in an elevator's path
	NAV_MESH_NAV_BLOCKER	= 0x80000000				// area is blocked by nav blocker ( Alas, needed to hijack a bit in the attributes to get within a cache line [7/24/2008 tom])
};

extern NavAttributeType NameToNavAttribute( const char *name );

enum NavDirType
{
	NORTH = 0,
	EAST = 1,
	SOUTH = 2,
	WEST = 3,

	NUM_DIRECTIONS
};

/**
 * Defines possible ways to move from one area to another
 */
enum NavTraverseType
{
	// NOTE: First 4 directions MUST match NavDirType
	GO_NORTH = 0,
	GO_EAST,
	GO_SOUTH,
	GO_WEST,

	GO_LADDER_UP,
	GO_LADDER_DOWN,
	GO_JUMP,
	GO_ELEVATOR_UP,
	GO_ELEVATOR_DOWN,

	NUM_TRAVERSE_TYPES
};

enum NavCornerType
{
	NORTH_WEST = 0,
	NORTH_EAST = 1,
	SOUTH_EAST = 2,
	SOUTH_WEST = 3,

	NUM_CORNERS
};

enum NavRelativeDirType
{
	FORWARD = 0,
	RIGHT,
	BACKWARD,
	LEFT,
	UP,
	DOWN,

	NUM_RELATIVE_DIRECTIONS
};

struct Extent
{
	Vector lo, hi;

	void Init( void )
	{
		lo.Init();
		hi.Init();
	}

	void Init( CBaseEntity *entity )
	{
		entity->CollisionProp()->WorldSpaceSurroundingBounds( &lo, &hi );
	}

	float SizeX( void ) const	{ return hi.x - lo.x; }
	float SizeY( void ) const	{ return hi.y - lo.y; }
	float SizeZ( void ) const	{ return hi.z - lo.z; }
	float Area( void ) const	{ return SizeX() * SizeY(); }

	// Increase bounds to contain the given point
	void Encompass( const Vector &pos )
	{
		for ( int i=0; i<3; ++i )
		{
			if ( pos[i] < lo[i] )
			{
				lo[i] = pos[i];
			}
			else if ( pos[i] > hi[i] )
			{
				hi[i] = pos[i];
			}
		}
	}

	// Increase bounds to contain the given extent
	void Encompass( const Extent &extent )
	{
		Encompass( extent.lo );
		Encompass( extent.hi );
	}

	// return true if 'pos' is inside of this extent
	bool Contains( const Vector &pos ) const
	{
		return (pos.x >= lo.x && pos.x <= hi.x &&
				pos.y >= lo.y && pos.y <= hi.y &&
				pos.z >= lo.z && pos.z <= hi.z);
	}
	
	// return true if this extent overlaps the given one
	bool IsOverlapping( const Extent &other ) const
	{
		return (lo.x <= other.hi.x && hi.x >= other.lo.x &&
				lo.y <= other.hi.y && hi.y >= other.lo.y &&
				lo.z <= other.hi.z && hi.z >= other.lo.z);
	}

	// return true if this extent completely contains the given one
	bool IsEncompassing( const Extent &other, float tolerance = 0.0f ) const
	{
		return (lo.x <= other.lo.x + tolerance && hi.x >= other.hi.x - tolerance &&
				lo.y <= other.lo.y + tolerance && hi.y >= other.hi.y - tolerance &&
				lo.z <= other.lo.z + tolerance && hi.z >= other.hi.z - tolerance);
	}
};

struct Ray
{
	Vector from, to;
};


class CNavArea;
class CNavNode;


//--------------------------------------------------------------------------------------------------------------
inline NavDirType OppositeDirection( NavDirType dir )
{
	switch( dir )
	{
		case NORTH: return SOUTH;
		case SOUTH: return NORTH;
		case EAST:	return WEST;
		case WEST:	return EAST;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType DirectionLeft( NavDirType dir )
{
	switch( dir )
	{
		case NORTH: return WEST;
		case SOUTH: return EAST;
		case EAST:	return NORTH;
		case WEST:	return SOUTH;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType DirectionRight( NavDirType dir )
{
	switch( dir )
	{
		case NORTH: return EAST;
		case SOUTH: return WEST;
		case EAST:	return SOUTH;
		case WEST:	return NORTH;
		default: break;
	}

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void AddDirectionVector( Vector *v, NavDirType dir, float amount )
{
	switch( dir )
	{
		case NORTH: v->y -= amount; return;
		case SOUTH: v->y += amount; return;
		case EAST:  v->x += amount; return;
		case WEST:  v->x -= amount; return;
		default: break;
	}
}

//--------------------------------------------------------------------------------------------------------------
inline float DirectionToAngle( NavDirType dir )
{
	switch( dir )
	{
		case NORTH:	return 270.0f;
		case SOUTH:	return 90.0f;
		case EAST:	return 0.0f;
		case WEST:	return 180.0f;
		default: break;
	}

	return 0.0f;
}

//--------------------------------------------------------------------------------------------------------------
inline NavDirType AngleToDirection( float angle )
{
	while( angle < 0.0f )
		angle += 360.0f;

	while( angle > 360.0f )
		angle -= 360.0f;

	if (angle < 45 || angle > 315)
		return EAST;

	if (angle >= 45 && angle < 135)
		return SOUTH;

	if (angle >= 135 && angle < 225)
		return WEST;

	return NORTH;
}

//--------------------------------------------------------------------------------------------------------------
inline void DirectionToVector2D( NavDirType dir, Vector2D *v )
{
	switch( dir )
	{
		case NORTH: v->x =  0.0f; v->y = -1.0f; break;
		case SOUTH: v->x =  0.0f; v->y =  1.0f; break;
		case EAST:  v->x =  1.0f; v->y =  0.0f; break;
		case WEST:  v->x = -1.0f; v->y =  0.0f; break;
		default: break;
	}
}


//--------------------------------------------------------------------------------------------------------------
inline void CornerToVector2D( NavCornerType dir, Vector2D *v )
{
	switch( dir )
	{
		case NORTH_WEST: v->x = -1.0f; v->y = -1.0f; break;
		case NORTH_EAST: v->x =  1.0f; v->y = -1.0f; break;
		case SOUTH_EAST: v->x =  1.0f; v->y =  1.0f; break;
		case SOUTH_WEST: v->x = -1.0f; v->y =  1.0f; break;
		default: break;
	}

	v->NormalizeInPlace();
}


//--------------------------------------------------------------------------------------------------------------
// Gets the corner types that surround the given direction
inline void GetCornerTypesInDirection( NavDirType dir, NavCornerType *first, NavCornerType *second )
{
	switch ( dir )
	{
	case NORTH:
		*first = NORTH_WEST;
		*second = NORTH_EAST;
		break;
	case SOUTH:
		*first = SOUTH_WEST;
		*second = SOUTH_EAST;
		break;
	case EAST:
		*first = NORTH_EAST;
		*second = SOUTH_EAST;
		break;
	case WEST:
		*first = NORTH_WEST;
		*second = SOUTH_WEST;
		break;
	default:
		break;
	}
}


//--------------------------------------------------------------------------------------------------------------
inline float RoundToUnits( float val, float unit )
{
	val = val + ((val < 0.0f) ? -unit*0.5f : unit*0.5f);
	return (float)( unit * ( ((int)val) / (int)unit ) );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Return true if given entity can be ignored when moving
 */
#define WALK_THRU_PROP_DOORS		0x01
#define WALK_THRU_FUNC_DOORS		0x02
#define WALK_THRU_DOORS				(WALK_THRU_PROP_DOORS | WALK_THRU_FUNC_DOORS)
#define WALK_THRU_BREAKABLES		0x04
#define WALK_THRU_TOGGLE_BRUSHES	0x08
#define WALK_THRU_EVERYTHING		(WALK_THRU_DOORS | WALK_THRU_BREAKABLES | WALK_THRU_TOGGLE_BRUSHES)
extern ConVar nav_solid_props;
inline bool IsEntityWalkable( CBaseEntity *entity, unsigned int flags )
{
	if (FClassnameIs( entity, "worldspawn" ))
		return false;

	if (FClassnameIs( entity, "player" ))
		return false;

	// if we hit a door, assume its walkable because it will open when we touch it
	if (FClassnameIs( entity, "func_door*" ))
	{
#ifdef PROBLEMATIC	// cp_dustbowl doors dont open by touch - they use surrounding triggers
		if ( !entity->HasSpawnFlags( SF_DOOR_PTOUCH ) )
		{
			// this door is not opened by touching it, if it is closed, the area is blocked
			CBaseDoor *door = (CBaseDoor *)entity;
			return door->m_toggle_state == TS_AT_TOP;
		}
#endif // _DEBUG

		return (flags & WALK_THRU_FUNC_DOORS) ? true : false;
	}

	if (FClassnameIs( entity, "prop_door*" ))
	{
		return (flags & WALK_THRU_PROP_DOORS) ? true : false;
	}

	// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
	if (FClassnameIs( entity, "func_brush" ))
	{
		CFuncBrush *brush = (CFuncBrush *)entity;
		switch ( brush->m_iSolidity )
		{
		case CFuncBrush::BRUSHSOLID_ALWAYS:
			return false;
		case CFuncBrush::BRUSHSOLID_NEVER:
			return true;
		case CFuncBrush::BRUSHSOLID_TOGGLE:
			return (flags & WALK_THRU_TOGGLE_BRUSHES) ? true : false;
		}
	}

	// if we hit a breakable object, assume its walkable because we will shoot it when we touch it
	if (FClassnameIs( entity, "func_breakable" ) && entity->GetHealth() && entity->m_takedamage == DAMAGE_YES)
		return (flags & WALK_THRU_BREAKABLES) ? true : false;

	if (FClassnameIs( entity, "func_breakable_surf" ) && entity->m_takedamage == DAMAGE_YES)
		return (flags & WALK_THRU_BREAKABLES) ? true : false;

	if ( FClassnameIs( entity, "func_playerinfected_clip" ) == true )
		return true;

	if ( nav_solid_props.GetBool() && FClassnameIs( entity, "prop_*" ) )
		return true;

	return false;
}


//--------------------------------------------------------------------------------------------------------------
/**
 *  Trace filter that ignores players, NPCs, and objects that can be walked through
 */
class CTraceFilterWalkableEntities : public CTraceFilterNoNPCsOrPlayer
{
public:
	CTraceFilterWalkableEntities( const IHandleEntity *passentity, int collisionGroup, unsigned int flags )
		: CTraceFilterNoNPCsOrPlayer( passentity, collisionGroup ), m_flags( flags )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		if ( CTraceFilterNoNPCsOrPlayer::ShouldHitEntity(pServerEntity, contentsMask) )
		{
			CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );
			return ( !IsEntityWalkable( pEntity, m_flags ) );
		}
		return false;
	}

private:
	unsigned int m_flags;
};


extern bool IsWalkableTraceLineClear( const Vector &from, const Vector &to, unsigned int flags = 0 );

#endif // _NAV_H_
