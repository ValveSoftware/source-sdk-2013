//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ROPE_SHARED_H
#define ROPE_SHARED_H
#ifdef _WIN32
#pragma once
#endif


// Shared definitions for rope.
#define ROPE_MAX_SEGMENTS		10
#define ROPE_TYPE1_NUMSEGMENTS	4
#define ROPE_TYPE2_NUMSEGMENTS	2

// Default rope gravity vector.
#define ROPE_GRAVITY			0, 0, -1500


// Rope flags.
#define ROPE_RESIZE				(1<<0)		// Try to keep the rope dangling the same amount
									// even as the rope length changes.
#define ROPE_BARBED				(1<<1)		// Hack option to draw like a barbed wire.
#define ROPE_COLLIDE			(1<<2)		// Collide with the world?
#define ROPE_SIMULATE			(1<<3)		// Is the rope valid?
#define ROPE_BREAKABLE			(1<<4)		// Can the endpoints detach?
#define ROPE_NO_WIND			(1<<5)		// No wind simulation on this rope.
#define ROPE_INITIAL_HANG		(1<<6)		// By default, ropes will simulate for a bit internally when they 
											// are created so they sag, but dynamically created ropes for things
											// like harpoons don't want this.
#define ROPE_PLAYER_WPN_ATTACH	(1<<7)		// If this flag is set, then the second attachment must be a player.
											// The rope will attach to "buff_attach" on the player's active weapon.
											// (This is a flag because it requires special code on the client to
											// find the weapon).
#define ROPE_NO_GRAVITY			(1<<8)		// Disable gravity on this rope.
#define ROPE_NUMFLAGS			9


// This is added to all rope slacks so when a level designer enters a 
// slack of zero in the entity, it doesn't dangle so low.
#define ROPESLACK_FUDGEFACTOR	-100


// Rope shader IDs.
#define ROPESHADER_BLACKCABLE	0
#define ROPESHADER_ROPE			1
#define ROPESHADER_CHAIN		2


// Rope locked points
enum
{
	ROPE_LOCK_START_POINT = 0x1,
	ROPE_LOCK_END_POINT = 0x2,
	ROPE_LOCK_START_DIRECTION = 0x4,
	ROPE_LOCK_END_DIRECTION = 0x8,
};


// Rope attachment points.
#define ROPE_ATTACHMENT_START_POINT	1
#define ROPE_ATTACHMENT_END_POINT	2


#endif // ROPE_SHARED_H
