//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef TEMPENTITY_H
#define TEMPENTITY_H
#ifdef _WIN32
#pragma once
#endif

#define TE_EXPLFLAG_NONE		0x0	// all flags clear makes default Half-Life explosion
#define TE_EXPLFLAG_NOADDITIVE	0x1	// sprite will be drawn opaque (ensure that the sprite you send is a non-additive sprite)
#define TE_EXPLFLAG_NODLIGHTS	0x2	// do not render dynamic lights
#define TE_EXPLFLAG_NOSOUND		0x4	// do not play client explosion sound
#define TE_EXPLFLAG_NOPARTICLES	0x8	// do not draw particles
#define TE_EXPLFLAG_DRAWALPHA	0x10	// sprite will be drawn alpha
#define TE_EXPLFLAG_ROTATE		0x20	// rotate the sprite randomly
#define TE_EXPLFLAG_NOFIREBALL	0x40	// do not draw a fireball
#define TE_EXPLFLAG_NOFIREBALLSMOKE	0x80	// do not draw smoke with the fireball

#define	TE_BEAMPOINTS		0		// beam effect between two points
#define TE_SPRITE			1	// additive sprite, plays 1 cycle
#define TE_BEAMDISK			2	// disk that expands to max radius over lifetime
#define TE_BEAMCYLINDER		3		// cylinder that expands to max radius over lifetime
#define TE_BEAMFOLLOW		4		// create a line of decaying beam segments until entity stops moving
#define TE_BEAMRING			5		// connect a beam ring to two entities
#define TE_BEAMSPLINE		6		
#define TE_BEAMRINGPOINT	7
#define	TE_BEAMLASER		8		// Fades according to viewpoint
#define TE_BEAMTESLA		9


#endif // TEMPENTITY_H
