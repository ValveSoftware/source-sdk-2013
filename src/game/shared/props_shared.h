//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PROPS_SHARED_H
#define PROPS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include <KeyValues.h>

// Phys prop spawnflags
#define SF_PHYSPROP_START_ASLEEP				0x000001
#define SF_PHYSPROP_DONT_TAKE_PHYSICS_DAMAGE	0x000002		// this prop can't be damaged by physics collisions
#define SF_PHYSPROP_DEBRIS						0x000004
#define SF_PHYSPROP_MOTIONDISABLED				0x000008		// motion disabled at startup (flag only valid in spawn - motion can be enabled via input)
#define	SF_PHYSPROP_TOUCH						0x000010		// can be 'crashed through' by running player (plate glass)
#define SF_PHYSPROP_PRESSURE					0x000020		// can be broken by a player standing on it
#define SF_PHYSPROP_ENABLE_ON_PHYSCANNON		0x000040		// enable motion only if the player grabs it with the physcannon
#define SF_PHYSPROP_NO_ROTORWASH_PUSH			0x000080		// The rotorwash doesn't push these
#define SF_PHYSPROP_ENABLE_PICKUP_OUTPUT		0x000100		// If set, allow the player to +USE this for the purposes of generating an output
#define SF_PHYSPROP_PREVENT_PICKUP				0x000200		// If set, prevent +USE/Physcannon pickup of this prop
#define SF_PHYSPROP_PREVENT_PLAYER_TOUCH_ENABLE	0x000400		// If set, the player will not cause the object to enable its motion when bumped into
#define SF_PHYSPROP_HAS_ATTACHED_RAGDOLLS		0x000800		// Need to remove attached ragdolls on enable motion/etc
#define SF_PHYSPROP_FORCE_TOUCH_TRIGGERS		0x001000		// Override normal debris behavior and respond to triggers anyway
#define SF_PHYSPROP_FORCE_SERVER_SIDE			0x002000		// Force multiplayer physics object to be serverside
#define SF_PHYSPROP_RADIUS_PICKUP				0x004000		// For Xbox, makes small objects easier to pick up by allowing them to be found 
#define SF_PHYSPROP_ALWAYS_PICK_UP				0x100000		// Physcannon can always pick this up, no matter what mass or constraints may apply.
#define SF_PHYSPROP_NO_COLLISIONS				0x200000		// Don't enable collisions on spawn
#define SF_PHYSPROP_IS_GIB						0x400000		// Limit # of active gibs

// Any barrel farther away than this is ignited rather than exploded.
#define PROP_EXPLOSION_IGNITE_RADIUS			32.0f

// ParsePropData returns
enum
{
	PARSE_SUCCEEDED,					// Parsed propdata. Prop must be a prop_physics.
	PARSE_SUCCEEDED_ALLOWED_STATIC,		// Parsed propdata. Prop allowed to be prop_physics or prop_dynamic/prop_static.
	PARSE_FAILED_NO_DATA,				// Parse failed, no propdata in model. Prop must be prop_dynamic/prop_static.
	PARSE_FAILED_BAD_DATA,				// Parse failed, found propdata but it had bad data.
};

// Propdata defined interactions
enum propdata_interactions_t
{
	PROPINTER_PHYSGUN_WORLD_STICK,		// "onworldimpact"	"stick"
	PROPINTER_PHYSGUN_FIRST_BREAK,		// "onfirstimpact"	"break"
	PROPINTER_PHYSGUN_FIRST_PAINT,		// "onfirstimpact"	"paintsplat"
	PROPINTER_PHYSGUN_FIRST_IMPALE,		// "onfirstimpact"	"impale"
	PROPINTER_PHYSGUN_LAUNCH_SPIN_NONE,	// "onlaunch"		"spin_none"
	PROPINTER_PHYSGUN_LAUNCH_SPIN_Z,	// "onlaunch"		"spin_zaxis"
	PROPINTER_PHYSGUN_BREAK_EXPLODE,	// "onbreak"		"explode_fire"
	PROPINTER_PHYSGUN_DAMAGE_NONE,		// "damage"			"none"

	PROPINTER_FIRE_FLAMMABLE,			// "flammable"			"yes"
	PROPINTER_FIRE_EXPLOSIVE_RESIST,	// "explosive_resist"	"yes"
	PROPINTER_FIRE_IGNITE_HALFHEALTH,	// "ignite"				"halfhealth"

	PROPINTER_PHYSGUN_CREATE_FLARE,		// "onpickup"		"create_flare"

	PROPINTER_PHYSGUN_ALLOW_OVERHEAD,	// "allow_overhead"	"yes"

	PROPINTER_WORLD_BLOODSPLAT,			// "onworldimpact", "bloodsplat"
	
	PROPINTER_PHYSGUN_NOTIFY_CHILDREN,	// "onfirstimpact" cause attached flechettes to explode

	// If we get more than 32 of these, we'll need a different system

	PROPINTER_NUM_INTERACTIONS,
};

// Entities using COLLISION_GROUP_SPECIAL_PHYSICS should support this interface.
abstract_class IMultiplayerPhysics
{
public:
	virtual int		GetMultiplayerPhysicsMode() = 0;
	virtual float	GetMass() = 0;
	virtual bool	IsAsleep() = 0;
};

#define PHYSICS_MULTIPLAYER_AUTODETECT	0	// use multiplayer physics mode as defined in model prop data
#define PHYSICS_MULTIPLAYER_SOLID		1	// soild, pushes player away 
#define PHYSICS_MULTIPLAYER_NON_SOLID	2	// nonsolid, but pushed by player
#define PHYSICS_MULTIPLAYER_CLIENTSIDE	3	// Clientside only, nonsolid 	

enum mp_break_t
{
	MULTIPLAYER_BREAK_DEFAULT,
	MULTIPLAYER_BREAK_SERVERSIDE,
	MULTIPLAYER_BREAK_CLIENTSIDE,
	MULTIPLAYER_BREAK_BOTH
};


enum PerformanceMode_t
{
	PM_NORMAL,
	PM_NO_GIBS,
	PM_FULL_GIBS,
	PM_REDUCED_GIBS,
};


//=============================================================================================================
// PROP DATA
//=============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Derive your entity from this if you want your entity to parse propdata
//-----------------------------------------------------------------------------
abstract_class IBreakableWithPropData
{
public:
	// Damage modifiers
	virtual void		SetDmgModBullet( float flDmgMod ) = 0;
	virtual void		SetDmgModClub( float flDmgMod ) = 0;
	virtual void		SetDmgModExplosive( float flDmgMod ) = 0;
	virtual float		GetDmgModBullet( void ) = 0;
	virtual float		GetDmgModClub( void ) = 0;
	virtual float		GetDmgModExplosive( void ) = 0;

	// Explosive
	virtual void		SetExplosiveRadius( float flRadius ) = 0;
	virtual void		SetExplosiveDamage( float flDamage ) = 0;
	virtual float		GetExplosiveRadius( void ) = 0;
	virtual float		GetExplosiveDamage( void ) = 0;

	// Physics damage tables
	virtual void		SetPhysicsDamageTable( string_t iszTableName ) = 0;
	virtual string_t	GetPhysicsDamageTable( void ) = 0;

	// Breakable chunks
	virtual void		SetBreakableModel( string_t iszModel ) = 0;
	virtual string_t 	GetBreakableModel( void ) = 0;
	virtual void		SetBreakableSkin( int iSkin ) = 0;
	virtual int			GetBreakableSkin( void ) = 0;
	virtual void		SetBreakableCount( int iCount ) = 0;
	virtual int			GetBreakableCount( void ) = 0;
	virtual void		SetMaxBreakableSize( int iSize ) = 0;
	virtual int			GetMaxBreakableSize( void ) = 0;

	// LOS blocking
	virtual void		SetPropDataBlocksLOS( bool bBlocksLOS ) = 0;
	virtual void		SetPropDataIsAIWalkable( bool bBlocksLOS ) = 0;

	// Interactions
	virtual void		SetInteraction( propdata_interactions_t Interaction ) = 0;
	virtual bool		HasInteraction( propdata_interactions_t Interaction ) = 0;

	// Multiplayer physics mode
	virtual void		SetPhysicsMode(int iMode) = 0;
	virtual int			GetPhysicsMode() = 0;

	// Multiplayer breakable spawn behavior
	virtual void		SetMultiplayerBreakMode( mp_break_t mode ) = 0;
	virtual mp_break_t	GetMultiplayerBreakMode( void ) const = 0;

	// Used for debugging
	virtual void		SetBasePropData( string_t iszBase ) = 0;
	virtual string_t	GetBasePropData( void ) = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Gamesystem that parses the prop data file
//-----------------------------------------------------------------------------
class CPropData : public CAutoGameSystem
{
public:
	CPropData( void );

	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void );
	virtual void LevelShutdownPostEntity( void );

	// Read in the data from the prop data file
	void ParsePropDataFile( void );

	// Parse a keyvalues section into the prop
	int ParsePropFromKV( CBaseEntity *pProp, KeyValues *pSection, KeyValues *pInteractionSection );

	// Fill out a prop's with base data parsed from the propdata file
	int ParsePropFromBase( CBaseEntity *pProp, const char *pszPropData );

	// Get a random chunk in the specified breakable section
	const char *GetRandomChunkModel( const char *pszBreakableSection, int iMaxSize = -1 );

protected:
	KeyValues	*m_pKVPropData;
	bool		m_bPropDataLoaded;

	struct propdata_breakablechunk_t
	{
		string_t				iszChunkType;
		CUtlVector<string_t>	iszChunkModels;
	};
	CUtlVector<propdata_breakablechunk_t>	m_BreakableChunks;
};

extern CPropData g_PropDataSystem;

struct breakmodel_t
{
	Vector		offset;
	char		modelName[512];
	char		placementName[512];
	float		fadeTime;
	float		fadeMinDist;
	float		fadeMaxDist;
	float		health;
	float		burstScale;
	int			collisionGroup;
	bool		isRagdoll;
	bool		placementIsBone;
	bool		isMotionDisabled;
	mp_break_t	mpBreakMode;
	Vector		velocity;
};

struct breakablepropparams_t
{
	breakablepropparams_t( const Vector &_origin, const QAngle &_angles, const Vector &_velocity, const AngularImpulse &_angularVelocity )
		: origin(_origin), angles(_angles), velocity(_velocity), angularVelocity(_angularVelocity)
	{
		impactEnergyScale = 0;
		defBurstScale = 0;
		defCollisionGroup = COLLISION_GROUP_NONE;
		nDefaultSkin = 0;
	}

	const Vector &origin;
	const QAngle &angles;
	const Vector &velocity;
	const AngularImpulse &angularVelocity;
	float impactEnergyScale;
	float defBurstScale;
	int defCollisionGroup;
	int nDefaultSkin;
};

const char *GetMassEquivalent(float flMass);
int  GetAutoMultiplayerPhysicsMode( Vector size, float mass );
void BuildPropList( const char *pszBlockName, CUtlVector<breakmodel_t> &list, int modelindex, float defBurstScale, int defCollisionGroup );
void BreakModelList( CUtlVector<breakmodel_t> &list, int modelindex, float defBurstScale, int defCollisionGroup );
void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLImit, bool defaultLocation = true );
void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const Vector &origin, const QAngle &angles, const Vector &velocity, const AngularImpulse &angularVelocity, float impactEnergyScale, float burstScale, int collisionGroup, CBaseEntity *pEntity = NULL, bool defaultLocation = true );

// Player gibs.
void PrecachePropsForModel( int iModel, const char *pszBlockName );
void PrecacheGibsForModel( int iModel );
void BuildGibList( CUtlVector<breakmodel_t> &list, int modelindex, float defBurstScale, int defCollisionGroup );
CBaseEntity *CreateGibsFromList( CUtlVector<breakmodel_t> &list, int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLImit, bool defaultLocation = true, CUtlVector<EHANDLE> *pGibList = NULL, bool bBurning = false );

#endif // PROPS_SHARED_H
