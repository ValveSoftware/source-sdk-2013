//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Model eyeposition node
//
//=============================================================================

#ifndef DMEEYEPOSITION_H
#define DMEEYEPOSITION_H
#ifdef _WIN32
#pragma once
#endif

#include "movieobjects/dmedag.h"


//-----------------------------------------------------------------------------
/*
$eyeposition 0 83.274592 0
$attachment "eyes"     "bip_head"  0         83.274592 1.41954 absolute
$attachment "righteye" "bip_head" -1.425952  83.274592 1.41954 absolute
$attachment "lefteye"  "bip_head"  1.425952  83.274592 1.41954 absolute

$cdmaterials "models/player/hvyweapon/"
$model heavy "parts/dmx/heavy_reference_lo.dmx"{

	eyeball righteye "bip_head" -1.425952 83.274592 1.41954 "eyeball_r" 1.80
		356  -1 "pupil_r" 0.6
		eyeball lefteye  "bip_head"  1.425952 83.274592 1.41954 "eyeball_l" 1.80
		356  1 "pupil_l" 0.6

		localvar %dummy_eyelid_flex 

		flexcontroller eyes range -30 30 eyes_updown
		flexcontroller eyes range -30 30 eyes_rightleft

}

$model eyeball

(name) Name of eyeball, used to match eyelid rules.

(bone name) Name of bone that the eye is parented to, typically the head.

(X) (Y) (Z) World location of the center of the ball of the eye.

(material name) Material name to use when searching for vertices to consider as the “white” of the eye (used in dynamically texture mapping the iris and cornea onto the eye).

(diameter) Diameter of the ball of the eye

(angle) Default yaw offset from “forward” for iris. Humans are typically 2-4 degrees walleyed. Not setting this correctly will result in your either characters appearing cross-eyed, or if you’ve compensated by misplacing the ball of the eye, them not tracking side to side.

(iris material) no longer used but still in the option list.

(pupil scale) World scale of the iris texture
[edit]
Syntax

eyeball (name) (bone name) (X) (Y) (Z) (material name) (diameter) (angle) (iris material) (pupil scale)

*/
//
//-----------------------------------------------------------------------------
class CDmeEyePosition : public CDmeDag
{
	DEFINE_ELEMENT( CDmeEyePosition, CDmeDag );

public:

	void GetWorldPosition( Vector &worldPosition );
};


#endif // DMEEYEPOSITION_H
