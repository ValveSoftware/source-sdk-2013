//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_MONSTER_RESOURCE
#define C_MONSTER_RESOURCE
#ifdef _WIN32
#pragma once
#endif


class C_MonsterResource : public C_BaseEntity
{
	DECLARE_CLASS( C_MonsterResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_MonsterResource();
	virtual ~C_MonsterResource();

	float GetBossHealthPercentage( void );
	float GetBossStunPercentage( void );

	int GetSkillShotCompleteCount( void ){ return m_iSkillShotCompleteCount; }
	float GetSkillShotComboEndTime( void ){ return m_fSkillShotComboEndTime; }

	int GetBossState() const { return m_iBossState; }

private:
	int m_iBossHealthPercentageByte;
	int m_iBossStunPercentageByte;

	int m_iSkillShotCompleteCount;		// the number of consecutive skill shots that have been completed. 0 = don't show combo HUD
	float m_fSkillShotComboEndTime;		// the time when the current skill shot combo window closes

	int m_iBossState;
};

extern C_MonsterResource *g_pMonsterResource;


#endif // C_MONSTER_RESOURCE
