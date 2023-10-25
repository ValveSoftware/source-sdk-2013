//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Custom weapon classes for Half-Life 2 based weapons.
// 
// Author: Peter Covington (petercov@outlook.com)
//
//==================================================================================//

#include "cbase.h"
#include "custom_weapon_factory.h"
#include "basebludgeonweapon.h"
#include "ai_basenpc.h"
#include "player.h"
#include "npcevent.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Acttables
extern acttable_t* GetSMG1Acttable();
extern int GetSMG1ActtableCount();
extern acttable_t* GetPistolActtable();
extern int GetPistolActtableCount();
extern acttable_t* GetShotgunActtable();
extern int GetShotgunActtableCount();
extern acttable_t* Get357Acttable();
extern int Get357ActtableCount();
extern acttable_t* GetAR2Acttable();
extern int GetAR2ActtableCount();
extern acttable_t* GetCrossbowActtable();
extern int GetCrossbowActtableCount();
extern acttable_t* GetAnnabelleActtable();
extern int GetAnnabelleActtableCount();



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

typedef struct HL2CustomMeleeData_s
{
	float m_flMeleeRange;
	float m_flRefireRate;
	float m_flDamage;
	float m_flNPCDamage;
	float m_flHitDelay;
	byte m_nDamageClass;
	bool m_bHitUsesMissAnim;

	bool Parse(KeyValues*);
} HL2CustomMeleeData_t;

class CHLCustomWeaponMelee : public CBaseHLBludgeonWeapon, public ICustomWeapon
{
public:
	DECLARE_CLASS(CHLCustomWeaponMelee, CBaseHLBludgeonWeapon);

	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	CHLCustomWeaponMelee();

	float		GetRange(void) { return	m_CustomData.m_flMeleeRange; }
	float		GetFireRate(void) { return	m_CustomData.m_flRefireRate; }
	float		GetHitDelay() { return m_CustomData.m_flHitDelay; }

	void		AddViewKick(void);
	float		GetDamageForActivity(Activity hitActivity);

	virtual int WeaponMeleeAttack1Condition(float flDot, float flDist);
	void		SecondaryAttack(void) { return; }

	// Animation event
	virtual void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

	// Don't use backup activities
	acttable_t*		GetBackupActivityList() { return NULL; }
	int				GetBackupActivityListCount() { return 0; }

	//Functions to select animation sequences 
	virtual Activity	GetPrimaryAttackActivity(void) { return m_CustomData.m_bHitUsesMissAnim ? ACT_VM_MISSCENTER : BaseClass::GetPrimaryAttackActivity(); }

	const char* GetWeaponScriptName() { return m_iszWeaponScriptName.Get(); }
	virtual int		GetDamageType() { return g_nDamageClassTypeBits[m_CustomData.m_nDamageClass]; }

	virtual void InitCustomWeaponFromData(const void* pData, const char* pszWeaponScript);

private:
	// Animation event handlers
	void HandleAnimEventMeleeHit(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

private:
	HL2CustomMeleeData_t m_CustomData;

	CNetworkString(m_iszWeaponScriptName, 128);
};

IMPLEMENT_SERVERCLASS_ST(CHLCustomWeaponMelee, DT_HLCustomWeaponMelee)
SendPropString(SENDINFO(m_iszWeaponScriptName)),
END_SEND_TABLE();

DEFINE_CUSTOM_WEAPON_FACTORY(hl2_melee, CHLCustomWeaponMelee, HL2CustomMeleeData_t);

bool HL2CustomMeleeData_s::Parse(KeyValues* pKVWeapon)
{
	KeyValues* pkvData = pKVWeapon->FindKey("CustomData");
	if (pkvData)
	{
		m_flDamage = pkvData->GetFloat("damage");
		m_flNPCDamage = pkvData->GetFloat("damage_npc", m_flDamage);
		m_flMeleeRange = pkvData->GetFloat("range", 70.f);
		m_flRefireRate = pkvData->GetFloat("rate", 0.7f);
		m_flHitDelay = pkvData->GetFloat("hitdelay");
		m_bHitUsesMissAnim = pkvData->GetBool("hit_uses_miss_anim");

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
		return true;
	}

	return false;
}

void CHLCustomWeaponMelee::InitCustomWeaponFromData(const void* pData, const char* pszWeaponScript)
{
	Q_FileBase(pszWeaponScript, m_iszWeaponScriptName.GetForModify(), 128);
	V_memcpy(&m_CustomData, pData, sizeof(HL2CustomMeleeData_t));
}

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

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_MELEE,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_MELEE,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_MELEE,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_MELEE,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_MELEE,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_MELEE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_MELEE,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_MELEE,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_MELEE,				false },//always aims

	// Readiness activities (aiming)
		{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_MELEE,			false },//never aims	
		{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_ANGRY_MELEE,	false },
		{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_MELEE,			false },//always aims

		{ ACT_WALK_AIM_RELAXED,			ACT_WALK_MELEE,			false },//never aims
		{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_MELEE,	false },
		{ ACT_WALK_AIM_AGITATED,		ACT_WALK_MELEE,				false },//always aims

		{ ACT_RUN_AIM_RELAXED,			ACT_RUN_MELEE,			false },//never aims
		{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_MELEE,	false },
		{ ACT_RUN_AIM_AGITATED,			ACT_RUN_MELEE,				false },//always aims
	//End readiness activities
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
}

//-----------------------------------------------------------------------------
// Purpose: Get the damage amount for the animation we're doing
// Input  : hitActivity - currently played activity
// Output : Damage amount
//-----------------------------------------------------------------------------
float CHLCustomWeaponMelee::GetDamageForActivity(Activity hitActivity)
{
	if ((GetOwner() != NULL) && (GetOwner()->IsPlayer()))
		return m_CustomData.m_flDamage;

	return m_CustomData.m_flNPCDamage;
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
		Vector(-16, -16, -16), Vector(36, 36, 36), m_CustomData.m_flNPCDamage, GetDamageType(), 0.75);

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

//--------------------------------------------------------------------------
//
//	Custom ranged weapon
//
//--------------------------------------------------------------------------

class CHLCustomWeaponGun : public CBaseHLCombatWeapon, public ICustomWeapon
{
public:
	DECLARE_CLASS(CHLCustomWeaponGun, CBaseHLCombatWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CHLCustomWeaponGun();
	virtual void InitCustomWeaponFromData(const void* pData, const char* pszWeaponScript);
	const char* GetWeaponScriptName() { return m_iszWeaponScriptName.Get(); }

	// Weapon behaviour
	virtual void			ItemPostFrame(void);				// called each frame by the player PostThink
	virtual void			ItemBusyFrame(void);				// called each frame by the player PostThink, if the player's not ready to attack yet
	virtual bool			ReloadOrSwitchWeapons(void);
	virtual bool			Holster(CBaseCombatWeapon* pSwitchingTo = NULL);

	// Bullet launch information
	virtual const Vector&	GetBulletSpread(void);
	virtual float			GetFireRate(void) { return m_CustomData.m_flFireRate; }
	virtual int				GetMinBurst() { return m_CustomData.m_nMinBurst; }
	virtual int				GetMaxBurst() { return m_CustomData.m_nMaxBurst; }
	virtual float			GetMinRestTime() { return m_CustomData.m_RestInterval.start; }
	virtual float			GetMaxRestTime() { return m_CustomData.m_RestInterval.start + m_CustomData.m_RestInterval.range; }

	// Autoaim
	virtual float			GetMaxAutoAimDeflection() { return 0.99f; }
	virtual float			WeaponAutoAimScale() { return m_CustomData.m_flAutoAimScale; } // allows a weapon to influence the perceived size of the target's autoaim radius.

	virtual void			AddViewKick(void);
	int						WeaponSoundRealtime(WeaponSound_t shoot_type);

	bool					StartReload(void);
	bool					Reload(void);
	void					FillClip(void);
	void					FinishReload(void);
	void					Pump(void);

	void					PrimaryAttack();

	void					FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles);
	void					FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles);
	void					Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary);
	void					Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);
	int						CapabilitiesGet(void) { return bits_CAP_WEAPON_RANGE_ATTACK1; }

	Activity				GetPrimaryAttackActivity(void);

	virtual acttable_t*		ActivityList(void);
	virtual int				ActivityListCount(void);

	virtual acttable_t*		GetBackupActivityList();
	virtual int				GetBackupActivityListCount();
private:
	void	CheckZoomToggle(void);
	void	ToggleZoom(void);

public:
	typedef struct Data_s
	{
		float m_flFireRate;
		int m_nMinBurst;
		int m_nMaxBurst;
		interval_t m_RestInterval;

		float m_flAutoAimScale;

		Vector m_vPlayerSpread;
		Vector m_vAllySpread;
		Vector m_vNPCSpread;
		int m_nBulletsPerShot; // For shotguns

		// Viewkick
		float m_flMaxVerticalKick;
		float m_flSlideLimit;
		interval_t m_VerticalPunchRange;

		int	m_nActTableIndex;

		bool m_bUseRecoilAnims;
		bool m_bFullAuto; // True for machine gun, false for semi-auto
		bool m_bNextAttackFromSequence;
		bool m_bUsePumpAnimation;
		bool m_bHasSecondaryFire;
		bool m_bHasZoom;
		bool m_bZoomDuringReload;
	} Data_t;

	struct Cache_s : public Data_s
	{
		bool					m_bFiresUnderwater;		// true if this weapon can fire underwater
		bool					m_bAltFiresUnderwater;		// true if this weapon can fire underwater
		float					m_fMinRange1;			// What's the closest this weapon can be used?
		float					m_fMinRange2;			// What's the closest this weapon can be used?
		float					m_fMaxRange1;			// What's the furthest this weapon can be used?
		float					m_fMaxRange2;			// What's the furthest this weapon can be used?
		bool					m_bReloadsSingly;		// True if this weapon reloads 1 round at a time

		bool Parse(KeyValues*);
	};

private:
	CNetworkString(m_iszWeaponScriptName, 128);

	Data_t m_CustomData;

	bool	m_bNeedPump;		// When emptied completely
	bool	m_bDelayedFire1;	// Fire primary when finished reloading
	bool	m_bDelayedFire2;	// Fire secondary when finished reloading
	bool	m_bInZoom;
	bool	m_bMustReload;

	int	m_nShotsFired;	// Number of consecutive shots fired
	float	m_flNextSoundTime;	// real-time clock of when to make next sound
public:
	enum WeaponActTable_e
	{
		ACTTABLE_SMG1 = 0,
		ACTTABLE_PISTOL,
		ACTTABLE_REVOLVER,
		ACTTABLE_SHOTGUN,
		ACTTABLE_AR2,
		ACTTABLE_CROSSBOW,
		ACTTABLE_ANNABELLE,

		NUM_GUN_ACT_TABLES
	};
};

IMPLEMENT_SERVERCLASS_ST(CHLCustomWeaponGun, DT_HLCustomWeaponGun)
SendPropString(SENDINFO(m_iszWeaponScriptName)),
END_SEND_TABLE();

BEGIN_DATADESC(CHLCustomWeaponGun)
DEFINE_FIELD(m_nShotsFired, FIELD_INTEGER),
DEFINE_FIELD(m_flNextSoundTime, FIELD_TIME),
DEFINE_FIELD(m_bNeedPump, FIELD_BOOLEAN),
DEFINE_FIELD(m_bDelayedFire1, FIELD_BOOLEAN),
DEFINE_FIELD(m_bDelayedFire2, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInZoom, FIELD_BOOLEAN),
DEFINE_FIELD(m_bMustReload, FIELD_BOOLEAN),
END_DATADESC();

DEFINE_CUSTOM_WEAPON_FACTORY(hl2_gun, CHLCustomWeaponGun, CHLCustomWeaponGun::Cache_s);

CHLCustomWeaponGun::CHLCustomWeaponGun()
{
	m_bNeedPump = false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;
	m_bInZoom = false;
	m_bMustReload = false;
	m_nShotsFired = 0;
}

acttable_t* CHLCustomWeaponGun::ActivityList(void)
{
	switch (m_CustomData.m_nActTableIndex)
	{
	default:
	case ACTTABLE_SMG1:
		return GetSMG1Acttable();
		break;
	case ACTTABLE_PISTOL:
		return GetPistolActtable();
		break;
	case ACTTABLE_REVOLVER:
		return Get357Acttable();
		break;
	case ACTTABLE_SHOTGUN:
		return GetShotgunActtable();
		break;
	case ACTTABLE_AR2:
		return GetAR2Acttable();
		break;
	case ACTTABLE_CROSSBOW:
		return GetCrossbowActtable();
		break;
	case ACTTABLE_ANNABELLE:
		return GetAnnabelleActtable();
		break;
	}
}

int CHLCustomWeaponGun::ActivityListCount(void)
{
	switch (m_CustomData.m_nActTableIndex)
	{
	default:
	case ACTTABLE_SMG1:
		return GetSMG1ActtableCount();
		break;
	case ACTTABLE_PISTOL:
		return GetPistolActtableCount();
		break;
	case ACTTABLE_REVOLVER:
		return Get357ActtableCount();
		break;
	case ACTTABLE_SHOTGUN:
		return GetShotgunActtableCount();
		break;
	case ACTTABLE_AR2:
		return GetAR2ActtableCount();
		break;
	case ACTTABLE_CROSSBOW:
		return GetCrossbowActtableCount();
		break;
	case ACTTABLE_ANNABELLE:
		return GetAnnabelleActtableCount();
		break;
	}
}

acttable_t* CHLCustomWeaponGun::GetBackupActivityList(void)
{
	switch (m_CustomData.m_nActTableIndex)
	{
	default:
	case ACTTABLE_SMG1:
	case ACTTABLE_CROSSBOW:
	case ACTTABLE_AR2:
		return GetSMG1Acttable();
		break;
	case ACTTABLE_PISTOL:
	case ACTTABLE_REVOLVER:
		return GetPistolActtable();
		break;
	case ACTTABLE_SHOTGUN:
	case ACTTABLE_ANNABELLE:
		return GetShotgunActtable();
		break;
	}
}

int CHLCustomWeaponGun::GetBackupActivityListCount(void)
{
	switch (m_CustomData.m_nActTableIndex)
	{
	default:
	case ACTTABLE_SMG1:
	case ACTTABLE_CROSSBOW:
	case ACTTABLE_AR2:
		return GetSMG1ActtableCount();
		break;
	case ACTTABLE_PISTOL:
	case ACTTABLE_REVOLVER:
		return GetPistolActtableCount();
		break;
	case ACTTABLE_SHOTGUN:
	case ACTTABLE_ANNABELLE:
		return GetShotgunActtableCount();
		break;
	}
}

void ReadIntervalInt(const char* pString, int &iMin, int &iMax)
{
	char tempString[128];
	Q_strncpy(tempString, pString, sizeof(tempString));

	char* token = strtok(tempString, ",");
	if (token)
	{
		iMin = atoi(token);
		token = strtok(NULL, ",");
		if (token)
		{
			iMax = atoi(token);
		}
		else
		{
			iMax = iMin;
		}
	}
}

bool CHLCustomWeaponGun::Cache_s::Parse(KeyValues* pKVWeapon)
{
	static const char* ppszCustomGunAnimTypes[NUM_GUN_ACT_TABLES] = {
		"smg",
		"pistol",
		"revolver",
		"shotgun",
		"ar2",
		"crossbow",
		"annabelle",
	};

	KeyValues* pkvData = pKVWeapon->FindKey("CustomData");
	if (pkvData)
	{
		m_flFireRate = pkvData->GetFloat("fire_rate", 0.5f);
		ReadIntervalInt(pkvData->GetString("npc_burst", "1"), m_nMinBurst, m_nMaxBurst);
		m_RestInterval = ReadInterval(pkvData->GetString("npc_rest_time", "0.3,0.6"));
		m_flAutoAimScale = pkvData->GetFloat("autoaim_scale", 1.f);
		m_bFullAuto = pkvData->GetBool("auto_fire");
		m_nBulletsPerShot = pkvData->GetInt("bullets", 1);
		m_bUseRecoilAnims = pkvData->GetBool("recoil_anims", true);
		m_bReloadsSingly = pkvData->GetBool("reload_singly");
		m_bFiresUnderwater = pkvData->GetBool("fires_underwater");
		m_bHasZoom = pkvData->GetBool("zoom_enable");
		m_bZoomDuringReload = m_bHasZoom && pkvData->GetBool("zoom_in_reload");

		m_fMinRange1 = pkvData->GetFloat("range1_min", 65.f);
		m_fMinRange2 = pkvData->GetFloat("range2_min", 65.f);
		m_fMaxRange1 = pkvData->GetFloat("range1_max", 1024.f);
		m_fMaxRange2 = pkvData->GetFloat("range2_max", 1024.f);

		if (m_bFullAuto)
		{
			m_flMaxVerticalKick = pkvData->GetFloat("viewkick_vertical_max", 1.f);
			m_flSlideLimit = pkvData->GetFloat("viewkick_slide_limit", 2.f);
		}
		else
		{
			m_flSlideLimit = pkvData->GetFloat("viewpunch_side_max", .6f);
			m_VerticalPunchRange = ReadInterval(pkvData->GetString("viewpunch_vertical", "0.25,0.5"));

			m_bNextAttackFromSequence = pkvData->GetBool("next_attack_time_from_sequence");
			m_bUsePumpAnimation = pkvData->GetBool("use_pump_anim");
		}

		// NOTE: The way these are calculated is that each component == sin (degrees/2)
		float flSpread = pkvData->GetFloat("spread", 5.f);
		float flNPCSpread = pkvData->GetFloat("spread_npc", flSpread);
		float flAllySperad = pkvData->GetFloat("spread_ally", flNPCSpread);
		m_vPlayerSpread = Vector(sin(DEG2RAD(flSpread * 0.5f)));
		m_vNPCSpread = Vector(sin(DEG2RAD(flNPCSpread * 0.5f)));
		m_vAllySpread = Vector(sin(DEG2RAD(flAllySperad * 0.5f)));

		const char* pszAnimType = pkvData->GetString("anim_type", nullptr);
		if (pszAnimType)
		{
			for (int i = 0; i < NUM_GUN_ACT_TABLES; i++)
			{
				if (V_stricmp(pszAnimType, ppszCustomGunAnimTypes[i]) == 0)
				{
					m_nActTableIndex = i;
					break;
				}
			}
		}

		return true;
	}

	return false;
}

void CHLCustomWeaponGun::InitCustomWeaponFromData(const void* pData, const char* pszWeaponScript)
{
	Q_FileBase(pszWeaponScript, m_iszWeaponScriptName.GetForModify(), 128);
	const auto* pCache = static_cast<const Cache_s*> (pData);
	m_CustomData = *pCache;
	m_bFiresUnderwater = pCache->m_bFiresUnderwater;
	m_bAltFiresUnderwater = pCache->m_bAltFiresUnderwater;
	m_fMinRange1 = pCache->m_fMinRange1;
	m_fMinRange2 = pCache->m_fMinRange2;
	m_fMaxRange1 = pCache->m_fMaxRange1;
	m_fMaxRange2 = pCache->m_fMaxRange2;
	m_bReloadsSingly = pCache->m_bReloadsSingly;
}

const Vector& CHLCustomWeaponGun::GetBulletSpread()
{
	if (!GetOwner() || !GetOwner()->IsNPC())
		return m_CustomData.m_vPlayerSpread;

	if (GetOwner()->MyNPCPointer()->IsPlayerAlly())
	{
		// 357 allies should be cooler
		return m_CustomData.m_vAllySpread;
	}

	return m_CustomData.m_vNPCSpread;
}

void CHLCustomWeaponGun::AddViewKick(void)
{
	//Get the view kick
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	if (m_CustomData.m_bFullAuto)
	{
		float flDuration = m_fFireDuration;

		if (g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE)
		{
			// On the 360 (or in any configuration using the 360 aiming scheme), don't let the
			// AR2 progressive into the late, highly inaccurate stages of its kick. Just
			// spoof the time to make it look (to the kicking code) like we haven't been
			// firing for very long.
			flDuration = MIN(flDuration, 0.75f);
		}

		CHLMachineGun::DoMachineGunKick(pPlayer, 0.5f, m_CustomData.m_flMaxVerticalKick, flDuration, m_CustomData.m_flSlideLimit);
	}
	else
	{
		QAngle	viewPunch;
		viewPunch.x = RandomInterval(m_CustomData.m_VerticalPunchRange);
		viewPunch.y = RandomFloat(-m_CustomData.m_flSlideLimit, m_CustomData.m_flSlideLimit);
		viewPunch.z = 0.0f;

		//Add it to the view punch
		pPlayer->ViewPunch(viewPunch);
	}
}

bool CHLCustomWeaponGun::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	// Stop zooming
	if (m_bInZoom)
	{
		ToggleZoom();
	}

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::CheckZoomToggle(void)
{
	if (!m_CustomData.m_bHasZoom)
		return;

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	int iButtonsTest = IN_ATTACK3;
	if (!m_CustomData.m_bHasSecondaryFire)
		iButtonsTest |= IN_ATTACK2;

	if (pPlayer->m_afButtonPressed & iButtonsTest)
	{
		ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::ToggleZoom(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	if (m_bInZoom)
	{
		if (pPlayer->SetFOV(this, 0, 0.2f))
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if (pPlayer->SetFOV(this, 20, 0.1f))
		{
			m_bInZoom = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CHLCustomWeaponGun::StartReload(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	// If shotgun totally emptied then a pump animation is needed

	//NOTENOTE: This is kinda lame because the player doesn't get strong feedback on when the reload has finished,
	//			without the pump.  Technically, it's incorrect, but it's good for feedback...

	if (m_CustomData.m_bUsePumpAnimation && m_iClip1 <= 0)
	{
		m_bNeedPump = true;
	}

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	// Make shotgun shell visible
	SetBodygroup(1, 0);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

#ifdef MAPBASE
	if (pOwner->IsPlayer())
	{
		static_cast<CBasePlayer*>(pOwner)->SetAnimation(PLAYER_RELOAD);
	}
#endif

	if (m_bInZoom && !m_CustomData.m_bZoomDuringReload)
	{
		ToggleZoom();
	}

	m_bInReload = true;
	m_bMustReload = false;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Override so only reload one shell at a time
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CHLCustomWeaponGun::Reload(void)
{
	if (m_bReloadsSingly)
	{
		// Check that StartReload was called first
		if (!m_bInReload)
		{
			Warning("ERROR: Shotgun Reload called incorrectly!\n");
		}

		CBaseCombatCharacter* pOwner = GetOwner();

		if (pOwner == NULL)
			return false;

		if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
			return false;

		if (m_iClip1 >= GetMaxClip1())
			return false;

		int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

		if (j <= 0)
			return false;

		FillClip();
		// Play reload on different channel as otherwise steals channel away from fire sound
		WeaponSound(RELOAD);
		SendWeaponAnim(ACT_VM_RELOAD);

		pOwner->m_flNextAttack = gpGlobals->curtime;
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();

		return true;
	}
	else if (BaseClass::Reload())
	{
		if (m_bInZoom && !m_CustomData.m_bZoomDuringReload)
		{
			ToggleZoom();
		}

		m_bMustReload = false;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::FinishReload(void)
{
	if (m_bReloadsSingly)
	{
		// Make shotgun shell invisible
		SetBodygroup(1, 1);

		CBaseCombatCharacter* pOwner = GetOwner();

		if (pOwner == NULL)
			return;

		m_bInReload = false;

		// Finish reload animation
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);

		pOwner->m_flNextAttack = gpGlobals->curtime;
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
	}
	else
	{
		BaseClass::FinishReload();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play finish reload anim and fill clip
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::FillClip(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	// Add them to the clip
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		if (Clip1() < GetMaxClip1())
		{
			m_iClip1++;
			pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Play weapon pump anim
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::Pump(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bNeedPump = false;

	WeaponSound(SPECIAL1);

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_PUMP);

	pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

//-----------------------------------------------------------------------------
// Purpose: If the current weapon has more ammo, reload it. Otherwise, switch 
//			to the next best weapon we've got. Returns true if it took any action.
//-----------------------------------------------------------------------------
bool CHLCustomWeaponGun::ReloadOrSwitchWeapons(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	Assert(pOwner);

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if (!HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime)
	{
		// weapon isn't useable, switch.
		// Ammo might be overridden to 0, in which case we shouldn't do this
		if (((GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false) && !HasSpawnFlags(SF_WEAPON_NO_AUTO_SWITCH_WHEN_EMPTY) && (g_pGameRules->SwitchToNextBestWeapon(pOwner, this)))
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if (UsesClipsForAmmo1() && !AutoFiresFullClip() &&
			(m_iClip1 == 0) &&
			(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) == false &&
			m_flNextPrimaryAttack < gpGlobals->curtime &&
			m_flNextSecondaryAttack < gpGlobals->curtime)
		{
			// if we're successfully reloading, we're done
			if (m_bReloadsSingly)
				return StartReload();
			else
				return Reload();
		}
	}

	return false;
}

void CHLCustomWeaponGun::ItemBusyFrame(void)
{
	BaseClass::ItemBusyFrame();

	if (m_CustomData.m_bZoomDuringReload)
		CheckZoomToggle();
}

void CHLCustomWeaponGun::ItemPostFrame(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	// Debounce the recoiling counter
	if ((pOwner->m_nButtons & IN_ATTACK) == false)
	{
		m_nShotsFired = 0;
	}

	UpdateAutoFire();

	if (m_CustomData.m_bZoomDuringReload || !m_bInReload)
		CheckZoomToggle();

	if (m_bReloadsSingly)
	{
		if (m_bInReload)
		{
			m_fFireDuration = 0.f;

			// If I'm primary firing and have one round stop reloading and fire
			if ((pOwner->m_nButtons & IN_ATTACK) && (m_iClip1 >= 1))
			{
				m_bInReload = false;
				m_bNeedPump = false;
				m_bDelayedFire1 = true;
			}
			// If I'm secondary firing and have one round stop reloading and fire
			else if (m_CustomData.m_bHasSecondaryFire && (pOwner->m_nButtons & IN_ATTACK2) && (m_iClip1 >= 2))
			{
				m_bInReload = false;
				m_bNeedPump = false;
				m_bDelayedFire2 = true;
			}
			else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
			{
				// If out of ammo end reload
				if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
				{
					FinishReload();
					return;
				}
				// If clip not full reload again
				if (m_iClip1 < GetMaxClip1())
				{
					Reload();
					return;
				}
				// Clip full, stop reloading
				else
				{
					FinishReload();
					return;
				}
			}
		}
		else
		{
			// Make shotgun shell invisible
			SetBodygroup(1, 1);
		}
	}
	else if (UsesClipsForAmmo1())
	{
		CheckReload();
	}

	if (m_CustomData.m_bUsePumpAnimation && (m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_fFireDuration = 0.f;
		Pump();
		return;
	}

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = (pOwner->m_nButtons & IN_ATTACK) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

	bool bFired = false;

	// Secondary attack has priority
	if (m_CustomData.m_bHasSecondaryFire && !m_bMustReload && (m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire2 = false;

		if (pOwner->HasSpawnFlags(SF_PLAYER_SUPPRESS_FIRING))
		{
			// Don't do anything, just cancel the whole function
			return;
		}
		else if (UsesSecondaryAmmo() && pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// FIXME: This isn't necessarily true if the weapon doesn't have a secondary fire!
			// For instance, the crossbow doesn't have a 'real' secondary fire, but it still 
			// stops the crossbow from firing on the 360 if the player chooses to hold down their
			// zoom button. (sjb) Orange Box 7/25/2007
#if !defined(CLIENT_DLL)
			if (!IsX360() || !ClassMatches("weapon_crossbow"))
#endif
			{
				bFired = ShouldBlockPrimaryFire();
			}

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if (UsesClipsForAmmo2())
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					pOwner->RemoveAmmo(1, m_iSecondaryAmmoType);
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}

	if (!bFired && !m_bMustReload && (m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire1 = false;

		if (pOwner->HasSpawnFlags(SF_PLAYER_SUPPRESS_FIRING))
		{
			// Don't do anything, just cancel the whole function
			return;
		}
		// Clip empty? Or out of ammo on a no-clip weapon?
		else if ((UsesClipsForAmmo1() && m_iClip1 <= 0) || (!UsesClipsForAmmo1() && pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0))
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw

			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ((pOwner->m_afButtonPressed & IN_ATTACK) || (pOwner->m_afButtonReleased & IN_ATTACK2))
			{
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();

			if (AutoFiresFullClip())
			{
				m_bFiringWholeClip = true;
			}

#ifdef CLIENT_DLL
			pOwner->SetFiredWeapon(true);
#endif
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	// -----------------------
	if ((pOwner->m_nButtons & IN_RELOAD || m_bMustReload) && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		if (m_bReloadsSingly)
			StartReload();
		else
			Reload();

		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	else if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (CanReload() && pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down or reloading
		if (!ReloadOrSwitchWeapons() && (m_bInReload == false))
		{
			WeaponIdle();
		}
	}
}

void CHLCustomWeaponGun::PrimaryAttack()
{
	// Only the player fires this way so we can cast
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	// Abort here to handle burst and auto fire modes
	if ((UsesClipsForAmmo1() && m_iClip1 == 0) || (!UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType)))
		return;

	if (m_CustomData.m_bFullAuto)
	{
		m_nShotsFired++;

		pPlayer->DoMuzzleFlash();

		// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
		// especially if the weapon we're firing has a really fast rate of fire.
		int iBulletsToFire = 0;
		float fireRate = GetFireRate();

		// MUST call sound before removing a round from the clip of a CHLMachineGun
		while (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			WeaponSound(SINGLE, m_flNextPrimaryAttack);
			m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
			iBulletsToFire++;
		}

		// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
		if (UsesClipsForAmmo1())
		{
			if (iBulletsToFire > m_iClip1)
				iBulletsToFire = m_iClip1;
			m_iClip1 -= iBulletsToFire;
		}

		// Fire the bullets
		FireBulletsInfo_t info;
		info.m_iShots = iBulletsToFire * m_CustomData.m_nBulletsPerShot;
		info.m_vecSrc = pPlayer->Weapon_ShootPosition();
		info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
		info.m_vecSpread = pPlayer->GetAttackSpread(this);
		info.m_flDistance = MAX_TRACE_LENGTH;
		info.m_iAmmoType = m_iPrimaryAmmoType;
		info.m_iTracerFreq = 2;
		FireBullets(info);

		SendWeaponAnim(GetPrimaryAttackActivity());
	}
	else
	{
		if (!m_CustomData.m_bNextAttackFromSequence && !m_CustomData.m_bUsePumpAnimation && !(pPlayer->m_afButtonPressed & IN_ATTACK))
			return;

		m_nShotsFired++;

		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE);
		pPlayer->DoMuzzleFlash();
		SendWeaponAnim(GetPrimaryAttackActivity());

		m_flNextPrimaryAttack = gpGlobals->curtime + ((m_CustomData.m_bNextAttackFromSequence || m_CustomData.m_bUsePumpAnimation) ? GetViewModelSequenceDuration() : GetFireRate());
		m_iClip1 -= 1;

		Vector	vecSrc = pPlayer->Weapon_ShootPosition();
		Vector	vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

		pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 1.0);

		// Fire the bullets, and force the first shot to be perfectly accuracy
		pPlayer->FireBullets(m_CustomData.m_nBulletsPerShot, vecSrc, vecAiming, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0, -1, -1, 0, NULL, (m_CustomData.m_nBulletsPerShot > 1), true);

		if (m_CustomData.m_bUsePumpAnimation && m_iClip1)
		{
			// pump so long as some rounds are left.
			m_bNeedPump = true;
		}
	}

	m_iPrimaryAttacks++;

	//Factor in the view kick
	AddViewKick();

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pPlayer);

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	pPlayer->SetAnimation(PLAYER_ATTACK1);

	// Register a muzzleflash for the AI
	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CHLCustomWeaponGun::GetPrimaryAttackActivity(void)
{
	if (!m_CustomData.m_bUseRecoilAnims || m_nShotsFired < 2)
		return ACT_VM_PRIMARYATTACK;

	if (m_nShotsFired < 3)
		return ACT_VM_RECOIL1;

	if (m_nShotsFired < 4)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
// Purpose: Make enough sound events to fill the estimated think interval
// returns: number of shots needed
//-----------------------------------------------------------------------------
int CHLCustomWeaponGun::WeaponSoundRealtime(WeaponSound_t shoot_type)
{
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = Clamp(m_flAnimTime - m_flPrevAnimTime, 0.f, 0.2f);
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound(SINGLE_NPC, m_flNextSoundTime);
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound(SINGLE_NPC, m_flNextSoundTime);
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOperator - 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles)
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC* npc = pOperator->MyNPCPointer();
	int iMuzzle = LookupAttachment("muzzle");

	ASSERT(npc != NULL);

	if (bUseWeaponAngles)
	{
		QAngle	angShootDir;
		GetAttachment(iMuzzle, vecShootOrigin, angShootDir);
		AngleVectors(angShootDir, &vecShootDir);
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);
	}

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2f, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());

	const Vector& vecSpread = (bUseWeaponAngles || m_CustomData.m_nBulletsPerShot > 1) ? GetBulletSpread() : VECTOR_CONE_PRECALCULATED;
	if (m_CustomData.m_bFullAuto)
	{
		int nShots = WeaponSoundRealtime(SINGLE_NPC);
		pOperator->FireBullets(nShots * m_CustomData.m_nBulletsPerShot, vecShootOrigin, vecShootDir, vecSpread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), iMuzzle);
		pOperator->DoMuzzleFlash();
		m_iClip1 = m_iClip1 - nShots;
	}
	else
	{
		WeaponSound(SINGLE_NPC);
		pOperator->FireBullets(m_CustomData.m_nBulletsPerShot, vecShootOrigin, vecShootDir, vecSpread, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), iMuzzle);
		pOperator->DoMuzzleFlash();
		m_iClip1 = m_iClip1 - 1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::FireNPCSecondaryAttack(CBaseCombatCharacter* pOperator, bool bUseWeaponAngles)
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	if (bSecondary)
	{
		FireNPCSecondaryAttack(pOperator, true);
	}
	else
	{
		// Ensure we have enough rounds in the clip
		m_iClip1++;

		FireNPCPrimaryAttack(pOperator, true);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CHLCustomWeaponGun::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	case EVENT_WEAPON_SHOTGUN_FIRE:
	case EVENT_WEAPON_AR1:
	case EVENT_WEAPON_AR2:
	case EVENT_WEAPON_HMG1:
	case EVENT_WEAPON_SMG2:
	case EVENT_WEAPON_SNIPER_RIFLE_FIRE:
	case EVENT_WEAPON_PISTOL_FIRE:
	{
		FireNPCPrimaryAttack(pOperator, false);
	}
	break;

	case EVENT_WEAPON_AR2_ALTFIRE:
	{
		FireNPCSecondaryAttack(pOperator, false);
	}
	break;

	default:
		CBaseCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
