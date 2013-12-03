//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Security cameras will track a default target (if they have one)
//			until they either acquire an enemy to track or are told to track
//			an entity via an input. If they lose their target they will
//			revert to tracking their default target. They acquire enemies
//			using the relationship table just like any other NPC.
//
//			Cameras have two zones of awareness, an inner zone formed by the
//			intersection of an inner FOV and an inner radius. The camera is
//			fully aware of entities in the inner zone and will acquire enemies
//			seen there.
//
//			The outer zone of awareness is formed by the intersection of an
//			outer FOV and an outer radius. The camera is only vaguely aware
//			of entities in the outer zone and will flash amber when enemies
//			are there, but will otherwise ignore them.
//
//			They can be made angry via an input, at which time they sound an
//			alarm and snap a few pictures of whatever they are tracking. They
//			can also be set to become angry anytime they acquire an enemy.
//
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_senses.h"
#include "ai_memory.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "Sprite.h"
#include "hl2/hl2_player.h"
#include "soundenvelope.h"
#include "explode.h"
#include "IEffects.h"
#include "animation.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Debug visualization
ConVar	g_debug_combine_camera("g_debug_combine_camera", "0");

#define	COMBINE_CAMERA_MODEL		"models/combine_camera/combine_camera.mdl"
#define COMBINE_CAMERA_GLOW_SPRITE	"sprites/glow1.vmt"
#define COMBINE_CAMERA_FLASH_SPRITE "sprites/light_glow03.vmt"
#define COMBINE_CAMERA_BC_YAW		"aim_yaw"
#define COMBINE_CAMERA_BC_PITCH		"aim_pitch"

#define COMBINE_CAMERA_SPREAD		VECTOR_CONE_2DEGREES
#define	COMBINE_CAMERA_MAX_WAIT		5
#define	COMBINE_CAMERA_PING_TIME	1.0f

// Spawnflags
#define SF_COMBINE_CAMERA_BECOMEANGRY		0x00000020
#define SF_COMBINE_CAMERA_IGNOREENEMIES		0x00000040
#define SF_COMBINE_CAMERA_STARTINACTIVE		0x00000080

// Heights
#define	COMBINE_CAMERA_RETRACT_HEIGHT	24
#define	COMBINE_CAMERA_DEPLOY_HEIGHT	64


// Activities
int ACT_COMBINE_CAMERA_OPEN;
int ACT_COMBINE_CAMERA_CLOSE;
int ACT_COMBINE_CAMERA_OPEN_IDLE;
int ACT_COMBINE_CAMERA_CLOSED_IDLE;
int ACT_COMBINE_CAMERA_FIRE;


const float CAMERA_CLICK_INTERVAL = 0.5f;
const float CAMERA_MOVE_INTERVAL = 1.0f;


//
// The camera has two FOVs - a wide one for becoming slightly aware of someone,
// a narrow one for becoming totally aware of them.
//
const float CAMERA_FOV_WIDE = 0.5;
const float CAMERA_FOV_NARROW = 0.707;


// Camera states
enum cameraState_e
{
	CAMERA_SEARCHING,
	CAMERA_AUTO_SEARCHING,
	CAMERA_ACTIVE,
	CAMERA_DEAD,
};


// Eye states
enum eyeState_t
{
	CAMERA_EYE_IDLE,				// Nothing abnormal in the inner or outer viewcone, dim green.
	CAMERA_EYE_SEEKING_TARGET,		// Something in the outer viewcone, flashes amber as it converges on the target.
	CAMERA_EYE_FOUND_TARGET,		// Something in the inner viewcone, bright amber.
	CAMERA_EYE_ANGRY,				// Found a target that we don't like: angry, bright red.
	CAMERA_EYE_DORMANT,				// Not active
	CAMERA_EYE_DEAD,				// Completely invisible
	CAMERA_EYE_DISABLED,			// Turned off, must be reactivated before it'll deploy again (completely invisible)
	CAMERA_EYE_HAPPY,				// Found a target that we like: go green for a second
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_CombineCamera : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_CombineCamera, CAI_BaseNPC);
public:
	
	CNPC_CombineCamera();
	~CNPC_CombineCamera();

	void Precache();
	void Spawn();
	Vector HeadDirection2D();

	int DrawDebugTextOverlays();

	void Deploy();
	void ActiveThink();
	void SearchThink();
	void DeathThink();

	void InputToggle(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputDisable(inputdata_t &inputdata);
	void InputSetAngry(inputdata_t &inputdata);
	void InputSetIdle(inputdata_t &inputdata);

	void DrawDebugGeometryOverlays(void);
	
	float MaxYawSpeed();

	int OnTakeDamage(const CTakeDamageInfo &inputInfo);

	Class_T Classify() { return (m_bEnabled) ? CLASS_MILITARY : CLASS_NONE; }
	
	bool IsValidEnemy( CBaseEntity *pEnemy );
	bool FVisible(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL);

	Vector EyeOffset(Activity nActivity) 
	{
		Vector vecEyeOffset(0,0,-64);
		GetEyePosition(GetModelPtr(), vecEyeOffset);
		return vecEyeOffset;
	}

	Vector EyePosition()
	{
		return GetAbsOrigin() + EyeOffset(GetActivity());
	}

protected:

	CBaseEntity *GetTarget();
	bool UpdateFacing();
	void TrackTarget(CBaseEntity *pTarget);

	bool PreThink(cameraState_e state);
	void SetEyeState(eyeState_t state);
	void MaintainEye();
	void Ping();	
	void Toggle();
	void Enable();
	void Disable();
	void SetHeight(float height);

	CBaseEntity *MaintainEnemy();
	void SetAngry(bool bAngry);

protected:
	int m_iAmmoType;
	int m_iMinHealthDmg;

	int m_nInnerRadius;	// The camera will only lock onto enemies that are within the inner radius.
	int m_nOuterRadius; // The camera will flash amber when enemies are within the outer radius, but outside the inner radius.

	bool m_bActive;		// The camera is deployed and looking for targets
	bool m_bAngry;		// The camera has gotten angry at someone and sounded an alarm.
	bool m_bBlinkState;
	bool m_bEnabled;		// Denotes whether the camera is able to deploy or not

	string_t m_sDefaultTarget;

	EHANDLE	m_hEnemyTarget;			// Entity we acquired as an enemy.	

	float m_flPingTime;
	float m_flClickTime;			// Time to take next picture while angry.
	int m_nClickCount;				// Counts pictures taken since we last became angry.
	float m_flMoveSoundTime;
	float m_flTurnOffEyeFlashTime;
	float m_flEyeHappyTime;

	QAngle m_vecGoalAngles;

	CSprite *m_pEyeGlow;
	CSprite *m_pEyeFlash;

	DECLARE_DATADESC();
};


BEGIN_DATADESC(CNPC_CombineCamera)

	DEFINE_FIELD(m_iAmmoType, FIELD_INTEGER),
	DEFINE_KEYFIELD(m_iMinHealthDmg, FIELD_INTEGER, "minhealthdmg"),
	DEFINE_KEYFIELD(m_nInnerRadius, FIELD_INTEGER, "innerradius"),
	DEFINE_KEYFIELD(m_nOuterRadius, FIELD_INTEGER, "outerradius"),
	DEFINE_FIELD(m_bActive, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bAngry, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bBlinkState, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bEnabled, FIELD_BOOLEAN),
	DEFINE_KEYFIELD(m_sDefaultTarget, FIELD_STRING, "defaulttarget"),
	DEFINE_FIELD(m_hEnemyTarget, FIELD_EHANDLE),
	DEFINE_FIELD(m_flPingTime, FIELD_TIME),
	DEFINE_FIELD(m_flClickTime, FIELD_TIME),
	DEFINE_FIELD(m_nClickCount, FIELD_INTEGER ),
	DEFINE_FIELD(m_flMoveSoundTime, FIELD_TIME),
	DEFINE_FIELD(m_flTurnOffEyeFlashTime, FIELD_TIME),
	DEFINE_FIELD(m_flEyeHappyTime, FIELD_TIME),
	DEFINE_FIELD(m_vecGoalAngles, FIELD_VECTOR),
	DEFINE_FIELD(m_pEyeGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(m_pEyeFlash, FIELD_CLASSPTR),

	DEFINE_THINKFUNC(Deploy),
	DEFINE_THINKFUNC(ActiveThink),
	DEFINE_THINKFUNC(SearchThink),
	DEFINE_THINKFUNC(DeathThink),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetAngry", InputSetAngry),
	DEFINE_INPUTFUNC(FIELD_VOID, "SetIdle", InputSetIdle),

END_DATADESC()

LINK_ENTITY_TO_CLASS(npc_combine_camera, CNPC_CombineCamera);


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CNPC_CombineCamera::CNPC_CombineCamera()
{
	m_bActive			= false;
	m_pEyeGlow			= NULL;
	m_pEyeFlash			= NULL;
	m_iAmmoType			= -1;
	m_iMinHealthDmg		= 0;
	m_flPingTime		= 0;
	m_bBlinkState		= false;
	m_bEnabled			= false;

	m_vecGoalAngles.Init();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CNPC_CombineCamera::~CNPC_CombineCamera()
{
}


//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Precache()
{
	PrecacheModel(COMBINE_CAMERA_MODEL);
	PrecacheModel(COMBINE_CAMERA_GLOW_SPRITE);
	PrecacheModel(COMBINE_CAMERA_FLASH_SPRITE);

	//  Activities
	ADD_CUSTOM_ACTIVITY(CNPC_CombineCamera, ACT_COMBINE_CAMERA_OPEN);
	ADD_CUSTOM_ACTIVITY(CNPC_CombineCamera, ACT_COMBINE_CAMERA_CLOSE);
	ADD_CUSTOM_ACTIVITY(CNPC_CombineCamera, ACT_COMBINE_CAMERA_CLOSED_IDLE);
	ADD_CUSTOM_ACTIVITY(CNPC_CombineCamera, ACT_COMBINE_CAMERA_OPEN_IDLE);
	ADD_CUSTOM_ACTIVITY(CNPC_CombineCamera, ACT_COMBINE_CAMERA_FIRE);

	PrecacheScriptSound( "NPC_CombineCamera.Move" );
	PrecacheScriptSound( "NPC_CombineCamera.BecomeIdle" );
	PrecacheScriptSound( "NPC_CombineCamera.Active" );
	PrecacheScriptSound( "NPC_CombineCamera.Click" );
	PrecacheScriptSound( "NPC_CombineCamera.Ping" );
	PrecacheScriptSound( "NPC_CombineCamera.Angry" );
	PrecacheScriptSound( "NPC_CombineCamera.Die" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Spawn the entity
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Spawn()
{
	Precache();

	SetModel(COMBINE_CAMERA_MODEL);

	m_pEyeFlash = CSprite::SpriteCreate(COMBINE_CAMERA_FLASH_SPRITE, GetLocalOrigin(), FALSE);
	m_pEyeFlash->SetTransparency(kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation);
	m_pEyeFlash->SetAttachment(this, 2);
	m_pEyeFlash->SetBrightness(0);
	m_pEyeFlash->SetScale(1.0);

	BaseClass::Spawn();

	m_HackedGunPos	= Vector(0, 0, 12.75);
	SetViewOffset(EyeOffset(ACT_IDLE));
	m_flFieldOfView	= CAMERA_FOV_WIDE;
	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 50;
	m_bloodColor	= BLOOD_COLOR_MECH;
	
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);

	SetHeight(COMBINE_CAMERA_RETRACT_HEIGHT);

	AddFlag(FL_AIMTARGET);

	SetPoseParameter(COMBINE_CAMERA_BC_YAW, 0);
	SetPoseParameter(COMBINE_CAMERA_BC_PITCH, 0);

	m_iAmmoType = GetAmmoDef()->Index("Pistol");

	// Create our eye sprite
	m_pEyeGlow = CSprite::SpriteCreate(COMBINE_CAMERA_GLOW_SPRITE, GetLocalOrigin(), false);
	m_pEyeGlow->SetTransparency(kRenderWorldGlow, 255, 0, 0, 128, kRenderFxNoDissipation);
	m_pEyeGlow->SetAttachment(this, 2);

	// Set our enabled state
	m_bEnabled = ((m_spawnflags & SF_COMBINE_CAMERA_STARTINACTIVE) == false);

	// Make sure the radii are sane.
	if (m_nOuterRadius <= 0)
	{
		m_nOuterRadius = 300;
	}

	if (m_nInnerRadius <= 0)
	{
		m_nInnerRadius = 450;
	}

	if (m_nOuterRadius < m_nInnerRadius)
	{
		V_swap(m_nOuterRadius, m_nInnerRadius);
	}

	// Do we start active?
	if (m_bEnabled)
	{
		Deploy();
	}
	else
	{
		SetEyeState(CAMERA_EYE_DISABLED);
	}

	//Adrian: No shadows on these guys.
	AddEffects( EF_NOSHADOW );

	// Stagger our starting times
	SetNextThink( gpGlobals->curtime + random->RandomFloat(0.1f, 0.3f) );

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags( BCF_NO_ANIMATION_SKIP );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_CombineCamera::GetTarget()
{
	return m_hEnemyTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CNPC_CombineCamera::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
	if (!m_takedamage)
		return 0;

	CTakeDamageInfo info = inputInfo;

	if (m_bActive == false)
		info.ScaleDamage(0.1f);

	// If attacker can't do at least the min required damage to us, don't take any damage from them
	if (info.GetDamage() < m_iMinHealthDmg)
		return 0;

	m_iHealth -= info.GetDamage();

	if (m_iHealth <= 0)
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_NO;

		RemoveFlag(FL_NPC); // why are they set in the first place???

		// FIXME: This needs to throw a ragdoll gib or something other than animating the retraction -- jdw

		ExplosionCreate(GetAbsOrigin(), GetLocalAngles(), this, 100, 100, false);
		SetThink(&CNPC_CombineCamera::DeathThink);

		StopSound("Alert");

		m_OnDamaged.FireOutput(info.GetInflictor(), this);

		SetNextThink( gpGlobals->curtime + 0.1f );

		return 0;
	}

	return 1;
}


//-----------------------------------------------------------------------------
// Purpose: Deploy and start searching for targets.
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Deploy()
{
	m_vecGoalAngles = GetAbsAngles();

	SetNextThink( gpGlobals->curtime );

	SetEyeState(CAMERA_EYE_IDLE);
	m_bActive = true;

	SetHeight(COMBINE_CAMERA_DEPLOY_HEIGHT);
	SetIdealActivity((Activity) ACT_COMBINE_CAMERA_OPEN_IDLE);
	m_flPlaybackRate = 0;
	SetThink(&CNPC_CombineCamera::SearchThink);

	EmitSound("NPC_CombineCamera.Move");
}


//-----------------------------------------------------------------------------
// Purpose: Returns the speed at which the camera can face a target
//-----------------------------------------------------------------------------
float CNPC_CombineCamera::MaxYawSpeed()
{
	if (m_hEnemyTarget)
		return 180.0f;

	return 60.0f;
}


//-----------------------------------------------------------------------------
// Purpose: Causes the camera to face its desired angles
//-----------------------------------------------------------------------------
bool CNPC_CombineCamera::UpdateFacing()
{
	bool  bMoved = false;
	matrix3x4_t localToWorld;
	
	GetAttachment(LookupAttachment("eyes"), localToWorld);

	Vector vecGoalDir;
	AngleVectors(m_vecGoalAngles, &vecGoalDir );

	Vector vecGoalLocalDir;
	VectorIRotate(vecGoalDir, localToWorld, vecGoalLocalDir);

	QAngle vecGoalLocalAngles;
	VectorAngles(vecGoalLocalDir, vecGoalLocalAngles);

	// Update pitch
	float flDiff = AngleNormalize(UTIL_ApproachAngle( vecGoalLocalAngles.x, 0.0, 0.1f * MaxYawSpeed()));
	
	int iPose = LookupPoseParameter(COMBINE_CAMERA_BC_PITCH);
	SetPoseParameter(iPose, GetPoseParameter(iPose) + (flDiff / 1.5f));

	if (fabs(flDiff) > 0.1f)
	{
		bMoved = true;
	}

	// Update yaw
	flDiff = AngleNormalize(UTIL_ApproachAngle( vecGoalLocalAngles.y, 0.0, 0.1f * MaxYawSpeed()));

	iPose = LookupPoseParameter(COMBINE_CAMERA_BC_YAW);
	SetPoseParameter(iPose, GetPoseParameter(iPose) + (flDiff / 1.5f));

	if (fabs(flDiff) > 0.1f)
	{
		bMoved = true;
	}

	if (bMoved && (m_flMoveSoundTime < gpGlobals->curtime))
	{
		EmitSound("NPC_CombineCamera.Move");
		m_flMoveSoundTime = gpGlobals->curtime + CAMERA_MOVE_INTERVAL;
	}

	// You're going to make decisions based on this info.  So bump the bone cache after you calculate everything
	InvalidateBoneCache();

	return bMoved;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CNPC_CombineCamera::HeadDirection2D()
{
	Vector	vecMuzzle, vecMuzzleDir;

	GetAttachment("eyes", vecMuzzle, &vecMuzzleDir );
	vecMuzzleDir.z = 0;
	VectorNormalize(vecMuzzleDir);

	return vecMuzzleDir;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_CombineCamera::FVisible(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
	CBaseEntity	*pHitEntity = NULL;
	if ( BaseClass::FVisible( pEntity, traceMask, &pHitEntity ) )
		return true;

	// If we hit something that's okay to hit anyway, still fire
	if ( pHitEntity && pHitEntity->MyCombatCharacterPointer() )
	{
		if (IRelationType(pHitEntity) == D_HT)
			return true;
	}

	if (ppBlocker)
	{
		*ppBlocker = pHitEntity;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Enemies are only valid if they're inside our radius
//-----------------------------------------------------------------------------
bool CNPC_CombineCamera::IsValidEnemy( CBaseEntity *pEnemy )
{
	Vector vecDelta = pEnemy->GetAbsOrigin() - GetAbsOrigin();
	float flDist = vecDelta.Length();
	if ( (flDist > m_nOuterRadius) || !FInViewCone(pEnemy) )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

//-----------------------------------------------------------------------------
// Purpose: Called when we have no scripted target. Looks for new enemies to track.
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_CombineCamera::MaintainEnemy()
{
	if (HasSpawnFlags(SF_COMBINE_CAMERA_IGNOREENEMIES))
		return NULL;

	GetSenses()->Look(m_nOuterRadius);

	CBaseEntity *pEnemy = BestEnemy();
	if (pEnemy)
	{
		// See if our best enemy is too far away to care about.
		Vector vecDelta = pEnemy->GetAbsOrigin() - GetAbsOrigin();
		float flDist = vecDelta.Length();
		if (flDist < m_nOuterRadius)
		{
			if (FInViewCone(pEnemy))
			{
				// dvs: HACK: for checking multiple view cones
				float flSaveFieldOfView = m_flFieldOfView;
				m_flFieldOfView = CAMERA_FOV_NARROW;

				// Is the target visible?
				bool bVisible = FVisible(pEnemy);
				m_flFieldOfView = flSaveFieldOfView;
				if ( bVisible )
					return pEnemy;
			}
		}
	}
	
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Think while actively tracking a target.
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::ActiveThink()
{
	// Allow descended classes a chance to do something before the think function
	if (PreThink(CAMERA_ACTIVE))
		return;

	// No active target, look for suspicious characters.
	CBaseEntity *pTarget = MaintainEnemy();
	if ( !pTarget )
	{
		// Nobody suspicious. Go back to being idle.
		m_hEnemyTarget = NULL;
		EmitSound("NPC_CombineCamera.BecomeIdle");
		SetAngry(false);
		SetThink(&CNPC_CombineCamera::SearchThink);
		SetNextThink( gpGlobals->curtime );
		return;
	}

	// Examine the target until it reaches our inner radius
	if ( pTarget != m_hEnemyTarget )
	{
		Vector vecDelta = pTarget->GetAbsOrigin() - GetAbsOrigin();
		float flDist = vecDelta.Length();
		if ( (flDist < m_nInnerRadius) && FInViewCone(pTarget) )
		{
			m_OnFoundEnemy.Set(pTarget, pTarget, this);

			// If it's a citizen, it's ok. If it's the player, it's not ok.
			if ( pTarget->IsPlayer() )
			{
				SetEyeState(CAMERA_EYE_FOUND_TARGET);

				if (HasSpawnFlags(SF_COMBINE_CAMERA_BECOMEANGRY))
				{
					SetAngry(true);
				}
				else
				{
					EmitSound("NPC_CombineCamera.Active");
				}

				m_OnFoundPlayer.Set(pTarget, pTarget, this);
				m_hEnemyTarget = pTarget;
			}
			else
			{
				SetEyeState(CAMERA_EYE_HAPPY);
				m_flEyeHappyTime = gpGlobals->curtime + 2.0;

				// Now forget about this target forever
				AddEntityRelationship( pTarget, D_NU, 99 );
			}
		}
		else
		{
			// If we get angry automatically, we get un-angry automatically
			if ( HasSpawnFlags(SF_COMBINE_CAMERA_BECOMEANGRY) && m_bAngry )
			{
				SetAngry(false);
			}
			m_hEnemyTarget = NULL;

			// We don't quite see this guy, but we sense him.
			SetEyeState(CAMERA_EYE_SEEKING_TARGET);
		}
	}

	// Update our think time
	SetNextThink( gpGlobals->curtime + 0.1f );

	TrackTarget(pTarget);
	MaintainEye();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pTarget - 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::TrackTarget( CBaseEntity *pTarget )
{
	if (!pTarget)
		return;

	// Calculate direction to target
	Vector vecMid = EyePosition();
	Vector vecMidTarget = pTarget->BodyTarget(vecMid);
	Vector vecDirToTarget = vecMidTarget - vecMid;	

	// We want to look at the target's eyes so we don't jitter
	Vector vecDirToTargetEyes = pTarget->WorldSpaceCenter() - vecMid;
	VectorNormalize(vecDirToTargetEyes);

	QAngle vecAnglesToTarget;
	VectorAngles(vecDirToTargetEyes, vecAnglesToTarget);

	// Draw debug info
	if (g_debug_combine_camera.GetBool())
	{
		NDebugOverlay::Cross3D(vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05);
		NDebugOverlay::Cross3D(pTarget->WorldSpaceCenter(), -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05);
		NDebugOverlay::Line(vecMid, pTarget->WorldSpaceCenter(), 0, 255, 0, false, 0.05);

		NDebugOverlay::Cross3D(vecMid, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05);
		NDebugOverlay::Cross3D(vecMidTarget, -Vector(2,2,2), Vector(2,2,2), 0, 255, 0, false, 0.05);
		NDebugOverlay::Line(vecMid, vecMidTarget, 0, 255, 0, false, 0.05f);
	}

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	
	GetAttachment("eyes", vecMuzzle, &vecMuzzleDir);
	
	SetIdealActivity((Activity) ACT_COMBINE_CAMERA_OPEN_IDLE);

	m_vecGoalAngles.y = vecAnglesToTarget.y;
	m_vecGoalAngles.x = vecAnglesToTarget.x;

	// Turn to face
	UpdateFacing();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::MaintainEye()
{
	// Angry cameras take a few pictures of their target.
	if ((m_bAngry) && (m_nClickCount <= 3))
	{
		if ((m_flClickTime != 0) && (m_flClickTime < gpGlobals->curtime))
		{
			m_pEyeFlash->SetScale(1.0);
			m_pEyeFlash->SetBrightness(255);
			m_pEyeFlash->SetColor(255,255,255);

			EmitSound("NPC_CombineCamera.Click");

			m_flTurnOffEyeFlashTime = gpGlobals->curtime + 0.1;
			m_flClickTime = gpGlobals->curtime + CAMERA_CLICK_INTERVAL;
		}
		else if ((m_flTurnOffEyeFlashTime != 0) && (m_flTurnOffEyeFlashTime < gpGlobals->curtime))
		{
			m_flTurnOffEyeFlashTime = 0;
			m_pEyeFlash->SetBrightness( 0, 0.25f );
			m_nClickCount++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Target doesn't exist or has eluded us, so search for one
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::SearchThink()
{
	// Allow descended classes a chance to do something before the think function
	if (PreThink(CAMERA_SEARCHING))
		return;

	SetNextThink( gpGlobals->curtime + 0.05f );

	SetIdealActivity((Activity) ACT_COMBINE_CAMERA_OPEN_IDLE);

	if ( !GetTarget() )
	{
		// Try to acquire a new target
		if (MaintainEnemy())
		{
			SetThink( &CNPC_CombineCamera::ActiveThink );
			return;
		}
	}

	// Display that we're scanning
	m_vecGoalAngles.x = 15.0f;
	m_vecGoalAngles.y = GetAbsAngles().y + (sin(gpGlobals->curtime * 2.0f) * 45.0f);

	// Turn and ping
	UpdateFacing();
	Ping();

	SetEyeState(CAMERA_EYE_IDLE);
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the camera is currently in
//-----------------------------------------------------------------------------
bool CNPC_CombineCamera::PreThink(cameraState_e state)
{
	CheckPVSCondition();

	MaintainActivity();
	StudioFrameAdvance();

	// If we're disabled, shut down
	if ( !m_bEnabled )
	{
		SetIdealActivity((Activity) ACT_COMBINE_CAMERA_CLOSED_IDLE);
		SetNextThink( gpGlobals->curtime + 0.1f );
		return true;
	}

	// Do not interrupt current think function
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Sets the state of the glowing eye attached to the camera
// Input  : state - state the eye should be in
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::SetEyeState(eyeState_t state)
{
	// Must have a valid eye to affect
	if (m_pEyeGlow == NULL)
		return;

	if (m_bAngry)
	{
		m_pEyeGlow->SetColor(255, 0, 0);
		m_pEyeGlow->SetBrightness(164, 0.1f);
		m_pEyeGlow->SetScale(0.4f, 0.1f);
		return;
	}

	// If we're switching to IDLE, and we're still happy, use happy instead
	if ( state == CAMERA_EYE_IDLE && m_flEyeHappyTime > gpGlobals->curtime )
	{
		state = CAMERA_EYE_HAPPY;
	}

	// Set the state
	switch (state)
	{
		default:
		case CAMERA_EYE_IDLE:
		{
			m_pEyeGlow->SetColor(0, 255, 0);
			m_pEyeGlow->SetBrightness(164, 0.1f);
			m_pEyeGlow->SetScale(0.4f, 0.1f);
			break;
		}

		case CAMERA_EYE_SEEKING_TARGET:
		{
			// Toggle our state
			m_bBlinkState = !m_bBlinkState;

			// Amber
			m_pEyeGlow->SetColor(255, 128, 0);

			if (m_bBlinkState)
			{
				// Fade up and scale up
				m_pEyeGlow->SetScale(0.25f, 0.1f);
				m_pEyeGlow->SetBrightness(164, 0.1f);
			}
			else
			{
				// Fade down and scale down
				m_pEyeGlow->SetScale(0.2f, 0.1f);
				m_pEyeGlow->SetBrightness(64, 0.1f);
			}

			break;
		}

		case CAMERA_EYE_FOUND_TARGET:
		{
			if (!m_bAngry)
			{
				// Amber
				m_pEyeGlow->SetColor(255, 128, 0);

				// Fade up and scale up
				m_pEyeGlow->SetScale(0.45f, 0.1f);
				m_pEyeGlow->SetBrightness(220, 0.1f);
			}
			else
			{
				m_pEyeGlow->SetColor(255, 0, 0);
				m_pEyeGlow->SetBrightness(164, 0.1f);
				m_pEyeGlow->SetScale(0.4f, 0.1f);
			}

			break;
		}

		case CAMERA_EYE_DORMANT: // Fade out and scale down
		{
			m_pEyeGlow->SetColor(0, 255, 0);
			m_pEyeGlow->SetScale(0.1f, 0.5f);
			m_pEyeGlow->SetBrightness(64, 0.5f);
			break;
		}

		case CAMERA_EYE_DEAD: // Fade out slowly
		{
			m_pEyeGlow->SetColor(255, 0, 0);
			m_pEyeGlow->SetScale(0.1f, 3.0f);
			m_pEyeGlow->SetBrightness(0, 3.0f);
			break;
		}

		case CAMERA_EYE_DISABLED:
		{
			m_pEyeGlow->SetColor(0, 255, 0);
			m_pEyeGlow->SetScale(0.1f, 1.0f);
			m_pEyeGlow->SetBrightness(0, 1.0f);
			break;
		}

		case CAMERA_EYE_HAPPY:
		{
			m_pEyeGlow->SetColor(0, 255, 0);
			m_pEyeGlow->SetBrightness(255, 0.1f);
			m_pEyeGlow->SetScale(0.5f, 0.1f);
			break;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Make a pinging noise so the player knows where we are
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Ping()
{
	// See if it's time to ping again
	if (m_flPingTime > gpGlobals->curtime)
		return;

	// Ping!
	EmitSound("NPC_CombineCamera.Ping");
	m_flPingTime = gpGlobals->curtime + COMBINE_CAMERA_PING_TIME;
}


//-----------------------------------------------------------------------------
// Purpose: Toggle the camera's state
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Toggle()
{
	if (m_bEnabled)
	{
		Disable();
	}
	else 
	{
		Enable();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Enable the camera and deploy
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Enable()
{
	m_bEnabled = true;
	SetThink(&CNPC_CombineCamera::Deploy);
	SetNextThink( gpGlobals->curtime + 0.05f );
}


//-----------------------------------------------------------------------------
// Purpose: Retire the camera until enabled again
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::Disable()
{
	m_bEnabled = false;
	m_hEnemyTarget = NULL;
	SetNextThink( gpGlobals->curtime + 0.1f );
}


//-----------------------------------------------------------------------------
// Purpose: Toggle the camera's state via input function
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::InputToggle(inputdata_t &inputdata)
{
	Toggle();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to enable the camera.
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::InputEnable(inputdata_t &inputdata)
{
	Enable();
}


//-----------------------------------------------------------------------------
// Purpose: Input handler to disable the camera.
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::InputDisable(inputdata_t &inputdata)
{
	Disable();
}


//-----------------------------------------------------------------------------
// Purpose: When we become angry, we make an angry sound and start photographing
//			whatever target we are tracking.
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::SetAngry(bool bAngry)
{
	if ((bAngry) && (!m_bAngry))
	{
		m_bAngry = true;
		m_nClickCount = 0;
		m_flClickTime = gpGlobals->curtime + 0.4;
		EmitSound("NPC_CombineCamera.Angry");
		SetEyeState(CAMERA_EYE_ANGRY);
	}
	else if ((!bAngry) && (m_bAngry))
	{
		m_bAngry = false;

		// make sure the flash is off (we might be in mid-flash)
		m_pEyeFlash->SetBrightness(0);
		SetEyeState(GetTarget() ? CAMERA_EYE_SEEKING_TARGET : CAMERA_EYE_IDLE);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::InputSetAngry(inputdata_t &inputdata)
{
	SetAngry(true);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::InputSetIdle(inputdata_t &inputdata)
{
	SetAngry(false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::DeathThink()
{
	if (PreThink(CAMERA_DEAD))
		return;

	// Level out our angles
	m_vecGoalAngles = GetAbsAngles();
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (m_lifeState != LIFE_DEAD)
	{
		m_lifeState = LIFE_DEAD;

		EmitSound("NPC_CombineCamera.Die");

		// lots of smoke
		Vector pos;
		CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &pos );
		
		CBroadcastRecipientFilter filter;
		
		te->Smoke(filter, 0.0, &pos, g_sModelIndexSmoke, 2.5, 10);
		
		g_pEffects->Sparks(pos);

		SetActivity((Activity) ACT_COMBINE_CAMERA_CLOSE);
	}

	StudioFrameAdvance();

	if (IsActivityFinished() && (UpdateFacing() == false))
	{
		SetHeight(COMBINE_CAMERA_RETRACT_HEIGHT);

		m_flPlaybackRate = 0;
		SetThink(NULL);
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : height - 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::SetHeight(float height)
{
	Vector forward, right, up;
	AngleVectors(GetLocalAngles(), &forward, &right, &up);

	Vector mins = (forward * -16.0f) + (right * -16.0f);
	Vector maxs = (forward *  16.0f) + (right *  16.0f) + (up * -height);

	if (mins.x > maxs.x)
	{
		V_swap(mins.x, maxs.x);
	}

	if (mins.y > maxs.y)
	{
		V_swap(mins.y, maxs.y);
	}

	if (mins.z > maxs.z)
	{
		V_swap(mins.z, maxs.z);
	}

	SetCollisionBounds(mins, maxs);

	UTIL_SetSize(this, mins, maxs);
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
//-----------------------------------------------------------------------------
int CNPC_CombineCamera::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ),"Enemy     : %s", m_hEnemyTarget ? m_hEnemyTarget->GetDebugName() : "<none>");
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_CombineCamera::DrawDebugGeometryOverlays(void)
{
	// ------------------------------
	// Draw viewcone if selected
	// ------------------------------
	if ((m_debugOverlays & OVERLAY_NPC_VIEWCONE_BIT))
	{
		float flViewRange	= acos(CAMERA_FOV_NARROW);
		Vector vEyeDir = EyeDirection2D( );
		Vector vLeftDir, vRightDir;
		float fSin, fCos;
		SinCos( flViewRange, &fSin, &fCos );

		vLeftDir.x			= vEyeDir.x * fCos - vEyeDir.y * fSin;
		vLeftDir.y			= vEyeDir.x * fSin + vEyeDir.y * fCos;
		vLeftDir.z			=  vEyeDir.z;
		fSin				= sin(-flViewRange);
		fCos				= cos(-flViewRange);
		vRightDir.x			= vEyeDir.x * fCos - vEyeDir.y * fSin;
		vRightDir.y			= vEyeDir.x * fSin + vEyeDir.y * fCos;
		vRightDir.z			=  vEyeDir.z;

		NDebugOverlay::BoxDirection(EyePosition(), Vector(0,0,-40), Vector(200,0,40), vLeftDir, 255, 255, 0, 50, 0 );
		NDebugOverlay::BoxDirection(EyePosition(), Vector(0,0,-40), Vector(200,0,40), vRightDir, 255, 255, 0, 50, 0 );
		NDebugOverlay::Box(EyePosition(), -Vector(2,2,2), Vector(2,2,2), 255, 255, 0, 128, 0 );
	}

	BaseClass::DrawDebugGeometryOverlays();
}
