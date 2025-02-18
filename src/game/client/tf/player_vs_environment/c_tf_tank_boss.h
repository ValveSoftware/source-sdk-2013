//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_TF_TANK_BOSS_H
#define C_TF_TANK_BOSS_H

#include "c_tf_base_boss.h"
#include "NextBot/C_NextBot.h"

class C_TFTankBoss : public C_TFBaseBoss
{
public:
	DECLARE_CLASS( C_TFTankBoss, C_TFBaseBoss );
	DECLARE_CLIENTCLASS();

	C_TFTankBoss();

	virtual void GetGlowEffectColor( float *r, float *g, float *b );

	// ITFMvMBossProgressUser
	virtual const char* GetBossProgressImageName() const OVERRIDE { return "tank"; }
};

#endif // C_TF_TANK_BOSS_H
