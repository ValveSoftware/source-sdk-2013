//========= Copyright Valve Corporation, All rights reserved. ============//
// NVNT haptics for Team Fortress 2
#ifndef C_TF_HAPTICS_H
#define C_TF_HAPTICS_H

class C_TFPlayer;

#include "haptics/haptic_utils.h"

class C_TFHaptics {
protected:
	C_TFHaptics();
public:
	bool wasCloaked : 1;
	bool wasFullyCloaked : 1;
	bool wasUber : 1;
	bool wasBurning :1;
	bool wasHealing : 1;
	bool isBeingHealed : 1;
	bool wasBeingHealed : 1;
	bool wasBeingHealedMedic : 1;
	bool wasBeingTeleported :1;
	bool skippedFirstCloak:1;
	bool readyForCloak:1;
	unsigned int healingDispenserCount:16;//short
	void Revert();
	// should only be local player!
	void HapticsThink(C_TFPlayer *player);
};

extern C_TFHaptics &tfHaptics;


#endif // C_TF_HAPTICS_H
