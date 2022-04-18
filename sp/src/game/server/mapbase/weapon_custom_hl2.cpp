#include "cbase.h"
#include "custom_weapon_factory.h"
#include "basebludgeonweapon.h"
#include "ai_basenpc.h"
#include "player.h"
#include "npcevent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#pragma region Melee
const char* g_ppszDamageClasses[] = {
	"BLUNT",
	"SLASH",
	"STUN",
	"BURN",
};

int g_nDamageClassTypeBits[ARRAYSIZE(g_ppszDamageClasses)] = {
	DMG_CLUB,
	DMG_SLASH,
	DMG_CLUB|DMG_SHOCK,
	DMG_CLUB|DMG_BURN,
};

class CHLCustomWeaponMelee : public CBaseHLBludgeonWeapon, public ICustomWeapon
{
public:
	DECLARE_CLASS(CHLCustomWeaponMelee, CBaseHLBludgeonWeapon);

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CHLCustomWeaponMelee();

	float		GetRange(void) { return	m_flMeleeRange; }
	float		GetFireRate(void) { return	m_flRefireRate; }

	void		AddViewKick(void);
	float		GetDamageForActivity(Activity hitActivity);

	virtual int WeaponMeleeAttack1Condition(float flDot, float flDist);
	void		SecondaryAttack(void) { return; }

	// Animation event
	virtual void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

	// Don't use backup activities
	acttable_t*		GetBackupActivityList() { return NULL; }
	int				GetBackupActivityListCount() { return 0; }

	const char* GetWeaponScriptName() { return m_iszWeaponScriptName.Get(); }
	virtual int		GetDamageType() { return g_nDamageClassTypeBits[m_nDamageClass]; }

	virtual void ParseCustomFromWeaponFile(const char* pFileName);

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

private:
	float m_flMeleeRange;
	float m_flRefireRate;
	float m_flDamage;
	float m_flNPCDamage;
	byte m_nDamageClass;

	CNetworkString(m_iszWeaponScriptName, 128);
};

IMPLEMENT_SERVERCLASS_ST(CHLCustomWeaponMelee, DT_HLCustomWeaponMelee)
SendPropString(SENDINFO(m_iszWeaponScriptName)),
END_SEND_TABLE();

DEFINE_CUSTOM_WEAPON_FACTORY(hl2_melee, CHLCustomWeaponMelee);

acttable_t CHLCustomWeaponMelee::m_acttable[] =
{
	{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
	{ ACT_GESTURE_MELEE_ATTACK1, ACT_GESTURE_MELEE_ATTACK_SWING, false},

	{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },
#if EXPANDED_HL2_WEAPON_ACTIVITIES
	{ ACT_IDLE,				ACT_IDLE_MELEE,		false },
	{ ACT_RUN,				ACT_RUN_MELEE,			false },
	{ ACT_WALK,				ACT_WALK_MELEE,			false },

	{ ACT_ARM,				ACT_ARM_MELEE,			false },
	{ ACT_DISARM,			ACT_DISARM_MELEE,		false },
#else
	{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
#endif

#ifdef MAPBASE
	// HL2:DM activities (for third-person animations in SP)
	{ ACT_RANGE_ATTACK1,                ACT_RANGE_ATTACK_SLAM, true },
	{ ACT_HL2MP_IDLE,                    ACT_HL2MP_IDLE_MELEE,                    false },
	{ ACT_HL2MP_RUN,                    ACT_HL2MP_RUN_MELEE,                    false },
	{ ACT_HL2MP_IDLE_CROUCH,            ACT_HL2MP_IDLE_CROUCH_MELEE,            false },
	{ ACT_HL2MP_WALK_CROUCH,            ACT_HL2MP_WALK_CROUCH_MELEE,            false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,    ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,    false },
	{ ACT_HL2MP_GESTURE_RELOAD,            ACT_HL2MP_GESTURE_RELOAD_MELEE,            false },
	{ ACT_HL2MP_JUMP,                    ACT_HL2MP_JUMP_MELEE,                    false },
#if EXPANDED_HL2DM_ACTIVITIES
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK2,	ACT_HL2MP_GESTURE_RANGE_ATTACK2_MELEE,		false },
	{ ACT_HL2MP_WALK,					ACT_HL2MP_WALK_MELEE,						false },
#endif
#endif
};

IMPLEMENT_ACTTABLE(CHLCustomWeaponMelee);

CHLCustomWeaponMelee::CHLCustomWeaponMelee()
{
	m_nDamageClass = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CHLCustomWeaponMelee::GetDamageForActivity(Activity hitActivity)
{
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return m_flDamage;

	return m_flNPCDamage;
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CHLCustomWeaponMelee::AddViewKick(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat(1.0f, 2.0f);
	punchAng.y = random->RandomFloat(-2.0f, -1.0f);
	punchAng.z = 0.0f;

	pPlayer->ViewPunch(punchAng);
}


//-----------------------------------------------------------------------------
// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
//-----------------------------------------------------------------------------
extern ConVar sk_crowbar_lead_time;

int CHLCustomWeaponMelee::WeaponMeleeAttack1Condition(float flDot, float flDist)
{
	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_BaseNPC* pNPC = GetOwner()->MyNPCPointer();
	CBaseEntity* pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	vecVelocity = pEnemy->GetSmoothedVelocity();

	// Project where the enemy will be in a little while
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += random->RandomFloat(-0.3f, 0.2f);
	if (dt < 0.0f)
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA(pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos);

	Vector vecDelta;
	VectorSubtract(vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta);

	if (fabs(vecDelta.z) > 70)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D();
	vecDelta.z = 0.0f;
	float flExtrapolatedDist = Vector2DNormalize(vecDelta.AsVector2D());
	if ((flDist > 64) && (flExtrapolatedDist > 64))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	float flExtrapolatedDot = DotProduct2D(vecDelta.AsVector2D(), vecForward.AsVector2D());
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CHLCustomWeaponMelee::HandleAnimEventMeleeHit(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors(GetAbsAngles(), &vecDirection);

	CBaseEntity* pEnemy = pOperator->MyNPCPointer() ? pOperator->MyNPCPointer()->GetEnemy() : NULL;
	if (pEnemy)
	{
		Vector vecDelta;
		VectorSubtract(pEnemy->WorldSpaceCenter(), pOperator->Weapon_ShootPosition(), vecDelta);
		VectorNormalize(vecDelta);

		Vector2D vecDelta2D = vecDelta.AsVector2D();
		Vector2DNormalize(vecDelta2D);
		if (DotProduct2D(vecDelta2D, vecDirection.AsVector2D()) > 0.8f)
		{
			vecDirection = vecDelta;
		}
	}

	Vector vecEnd;
	VectorMA(pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd);
	CBaseEntity* pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd,
		Vector(-16, -16, -16), Vector(36, 36, 36), m_flNPCDamage, GetDamageType(), 0.75);

	// did I hit someone?
	if (pHurt)
	{
		// play sound
		WeaponSound(MELEE_HIT);

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine(pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit);
		ImpactEffect(traceHit);
	}
	else
	{
		WeaponSound(MELEE_MISS);
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CHLCustomWeaponMelee::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit(pEvent, pOperator);
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}

void CHLCustomWeaponMelee::ParseCustomFromWeaponFile(const char* pFileName)
{
	Q_FileBase(pFileName, m_iszWeaponScriptName.GetForModify(), 128);
	KeyValuesAD pKVWeapon("WeaponData");
	if (pKVWeapon->LoadFromFile(filesystem, pFileName, "GAME"))
	{
		KeyValues* pkvData = pKVWeapon->FindKey("CustomData");
		if (pkvData)
		{
			m_flDamage = pkvData->GetFloat("damage");
			m_flNPCDamage = pkvData->GetFloat("damage_npc", m_flDamage);
			m_flMeleeRange = pkvData->GetFloat("range", 70.f);
			m_flRefireRate = pkvData->GetFloat("rate", 0.7f);

			const char* pszDamageClass = pkvData->GetString("damage_type", nullptr);
			if (pszDamageClass)
			{
				for (byte i = 0; i < ARRAYSIZE(g_ppszDamageClasses); i++)
				{
					if (V_stricmp(pszDamageClass, g_ppszDamageClasses[i]) == 0)
					{
						m_nDamageClass = i;
						break;
					}
				}
			}
		}
	}
}
#pragma endregion