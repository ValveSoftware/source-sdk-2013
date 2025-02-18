//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "c_tf_tank_boss.h"

#include "teamplayroundbased_gamerules.h"


IMPLEMENT_CLIENTCLASS_DT(C_TFTankBoss, DT_TFTankBoss, CTFTankBoss)
	//RecvPropVector(RECVINFO(m_shadowDirection)),
END_RECV_TABLE()

LINK_ENTITY_TO_CLASS( tank_boss, C_TFTankBoss );


C_TFTankBoss::C_TFTankBoss()
{
}

void C_TFTankBoss::GetGlowEffectColor( float *r, float *g, float *b )
{
	TeamplayRoundBasedRules()->GetTeamGlowColor( GetTeamNumber(), *r, *g, *b );
}

