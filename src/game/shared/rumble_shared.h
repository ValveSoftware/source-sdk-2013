//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared code for XBox Rumble Effects
//
// $NoKeywords: $
//
//=============================================================================//
#pragma once
#ifndef RUMBLE_SHARED_H
#define RUMBLE_SHARED_H

#define RUMBLE_FLAGS_NONE				0x0000
#define RUMBLE_FLAG_STOP				0x0001 // Stop any instance of this type of effect that's already playing.
#define RUMBLE_FLAG_LOOP				0x0002 // Make this effect loop.
#define RUMBLE_FLAG_RESTART				0x0004 // If this effect is already playing, restart it.
#define RUMBLE_FLAG_UPDATE_SCALE		0x0008 // Apply DATA to this effect if already playing, but don't restart.
#define RUMBLE_FLAG_ONLYONE				0x0010 // Don't play this effect if it is already playing.
#define RUMBLE_FLAG_RANDOM_AMPLITUDE	0x0020 // Amplitude scale will be randomly chosen. Between 10% and 100%
#define RUMBLE_FLAG_INITIAL_SCALE		0x0040 // Data is the initial scale to start this effect ( * 100 )

enum
{
// DO NOT CHANGE THE ORDER OF ANY OF THESE ENUMS
// DO NOT INSERT ANY ITEMS
	RUMBLE_INVALID = -1,

	RUMBLE_STOP_ALL	= 0,		// Cease all current rumbling effects.

	// Weapons
	RUMBLE_PISTOL,
	RUMBLE_357,
	RUMBLE_SMG1,
	RUMBLE_AR2,
	RUMBLE_SHOTGUN_SINGLE,
	RUMBLE_SHOTGUN_DOUBLE,
	RUMBLE_AR2_ALT_FIRE,

// YOU MAY INSERT/REARRANGE ITEMS FROM HERE DOWN, AS YOU SEE FIT
	RUMBLE_RPG_MISSILE,

	RUMBLE_CROWBAR_SWING,

	// Vehicles
	RUMBLE_AIRBOAT_GUN,
	RUMBLE_JEEP_ENGINE_LOOP,
	
	RUMBLE_FLAT_LEFT,
	RUMBLE_FLAT_RIGHT,
	RUMBLE_FLAT_BOTH,

	// Damage
	RUMBLE_DMG_LOW,
	RUMBLE_DMG_MED,
	RUMBLE_DMG_HIGH,

	// Fall damage
	RUMBLE_FALL_LONG,
	RUMBLE_FALL_SHORT,

	RUMBLE_PHYSCANNON_OPEN,
	RUMBLE_PHYSCANNON_PUNT,
	RUMBLE_PHYSCANNON_LOW,
	RUMBLE_PHYSCANNON_MEDIUM,
	RUMBLE_PHYSCANNON_HIGH,

	RUMBLE_PORTALGUN_LEFT,
	RUMBLE_PORTALGUN_RIGHT,
	RUMBLE_PORTAL_PLACEMENT_FAILURE,

	NUM_RUMBLE_EFFECTS, // THIS MUST BE LAST!!!
};

#endif//RUMBLE_SHARED_H
