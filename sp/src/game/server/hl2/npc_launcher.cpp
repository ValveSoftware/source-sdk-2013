//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_senses.h"
#include "ai_squad.h"
#include "grenade_homer.h"
#include "grenade_pathfollower.h"
#include "explode.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	LAUNCHER_REST_TIME	3


//------------------------------------
// Spawnflags
//------------------------------------
#define SF_LAUNCHER_CHECK_LOS	(1 << 16)


//=========================================================
//	>> CNPC_Launcher
//=========================================================
class CNPC_Launcher : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Launcher, CAI_BaseNPC );

public:
	int					m_nStartOn;
	string_t			m_sMissileModel;
	string_t			m_sLaunchSound;
	string_t			m_sFlySound;
	int					m_nSmokeTrail;
	bool				m_bSmokeLaunch;
	int					m_nLaunchDelay;
	float				m_flLaunchSpeed;
	string_t			m_sPathCornerName;		// If following a path
	float				m_flHomingSpeed;
	int					m_nHomingStrength;
	float				m_flHomingDelay;		// How long before homing starts
	float				m_flHomingRampUp;		// How much time to ramp up to full homing
	float				m_flHomingDuration;		// How long does homing last
	float				m_flHomingRampDown;		// How long to ramp down to no homing
	float				m_flMissileGravity;
	float				m_flMinAttackDist;
	float				m_flMaxAttackDist;
	float				m_flSpinMagnitude;
	float				m_flSpinSpeed;
	float				m_flDamage;
	float				m_flDamageRadius;
	
	// ----------------
	//	Outputs
	// ----------------
	COutputEvent		m_OnLaunch;					// Triggered when missile is launched.

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputLOSCheckOn( inputdata_t &inputdata );
	void InputLOSCheckOff( inputdata_t &inputdata );
	void InputSetEnemy( inputdata_t &inputdata );
	void InputClearEnemy( inputdata_t &inputdata );
	void InputFireOnce( inputdata_t &inputdata );

	void LauncherTurnOn(void);
	
	void				Precache( void );
	void				Spawn( void );
	Class_T				Classify( void );
	bool				IsValidEnemy(CBaseEntity *pTarget );
	void				LaunchGrenade(CBaseEntity* pLauncher );
	void				LauncherThink(void );
	bool				FInViewCone( CBaseEntity *pEntity );

	int					DrawDebugTextOverlays(void);

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CNPC_Launcher )

	// Inputs
	DEFINE_KEYFIELD( m_nStartOn,					FIELD_INTEGER,	"StartOn" ),
	DEFINE_KEYFIELD( m_sMissileModel,			FIELD_STRING,	"MissileModel" ),
	DEFINE_KEYFIELD( m_sLaunchSound,				FIELD_STRING,	"LaunchSound" ),
	DEFINE_KEYFIELD( m_sFlySound,				FIELD_STRING,	"FlySound" ),
	DEFINE_KEYFIELD( m_nSmokeTrail,				FIELD_INTEGER,	"SmokeTrail" ),
	DEFINE_KEYFIELD( m_bSmokeLaunch,				FIELD_BOOLEAN,	"LaunchSmoke" ),
	DEFINE_KEYFIELD( m_nLaunchDelay,				FIELD_INTEGER,	"LaunchDelay" ),
	DEFINE_KEYFIELD( m_flLaunchSpeed,			FIELD_FLOAT,	"LaunchSpeed" ),
	DEFINE_KEYFIELD( m_sPathCornerName,			FIELD_STRING,	"PathCornerName" ),
	DEFINE_KEYFIELD( m_flHomingSpeed,			FIELD_FLOAT,	"HomingSpeed" ),
	DEFINE_KEYFIELD( m_nHomingStrength,			FIELD_INTEGER,	"HomingStrength" ),
	DEFINE_KEYFIELD( m_flHomingDelay,			FIELD_FLOAT,	"HomingDelay" ),
	DEFINE_KEYFIELD( m_flHomingRampUp,			FIELD_FLOAT,	"HomingRampUp" ),
	DEFINE_KEYFIELD( m_flHomingDuration,			FIELD_FLOAT,	"HomingDuration" ),
	DEFINE_KEYFIELD( m_flHomingRampDown,			FIELD_FLOAT,	"HomingRampDown" ),
	DEFINE_KEYFIELD( m_flGravity,				FIELD_FLOAT,	"Gravity" ),
	DEFINE_KEYFIELD( m_flMinAttackDist,			FIELD_FLOAT,	"MinRange" ),
	DEFINE_KEYFIELD( m_flMaxAttackDist,			FIELD_FLOAT,	"MaxRange" ),
	DEFINE_KEYFIELD( m_flSpinMagnitude,			FIELD_FLOAT,	"SpinMagnitude" ),
	DEFINE_KEYFIELD( m_flSpinSpeed,				FIELD_FLOAT,	"SpinSpeed" ),
	DEFINE_KEYFIELD( m_flDamage,					FIELD_FLOAT,	"Damage" ),
	DEFINE_KEYFIELD( m_flDamageRadius,			FIELD_FLOAT,	"DamageRadius" ),
	DEFINE_FIELD( m_flMissileGravity, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LOSCheckOn", InputLOSCheckOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "LOSCheckOn", InputLOSCheckOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "FireOnce", InputFireOnce ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "SetEnemyEntity", InputSetEnemy ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearEnemyEntity", InputClearEnemy ),

	DEFINE_OUTPUT( m_OnLaunch, "OnLaunch" ),

	// Function Pointers
	DEFINE_THINKFUNC( LauncherThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_launcher, CNPC_Launcher );


// ===================
//  Input Functions
// ===================
void CNPC_Launcher::InputTurnOn( inputdata_t &inputdata )
{
	LauncherTurnOn();
}

void CNPC_Launcher::InputTurnOff( inputdata_t &inputdata )
{
	SetThink(NULL);
}

void CNPC_Launcher::InputLOSCheckOn( inputdata_t &inputdata )
{
	m_spawnflags |= SF_LAUNCHER_CHECK_LOS;
}

void CNPC_Launcher::InputLOSCheckOff( inputdata_t &inputdata )
{
	m_spawnflags &= ~SF_LAUNCHER_CHECK_LOS;
}

void CNPC_Launcher::InputSetEnemy( inputdata_t &inputdata )
{
	SetEnemy( inputdata.value.Entity().Get() );
}

void CNPC_Launcher::InputClearEnemy( inputdata_t &inputdata )
{
	SetEnemy( NULL );
}

void CNPC_Launcher::InputFireOnce( inputdata_t &inputdata )
{
	m_flNextAttack = 0;

	// If I using path following missiles just launch
	if (m_sPathCornerName != NULL_STRING)
	{
		LaunchGrenade(NULL);
	}
	// Otherwise only launch if I have an enemy
	else
	{
		LauncherThink();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Launcher::Precache( void )
{
	// This is a dummy model that is never used!
	PrecacheModel("models/player.mdl");
	PrecacheModel(STRING(m_sMissileModel));
	PrecacheScriptSound( STRING(m_sLaunchSound));
	PrecacheScriptSound( STRING(m_sFlySound));
	
	UTIL_PrecacheOther( "grenade_homer");
	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Launcher::LauncherTurnOn(void)
{
	SetThink(&CNPC_Launcher::LauncherThink);
	SetNextThink( gpGlobals->curtime );
	m_flNextAttack = 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Launcher::Spawn( void )
{
	Precache();

	// This is a dummy model that is never used!
	SetModel( "models/player.mdl" );

	UTIL_SetSize(this, vec3_origin, vec3_origin);

	m_takedamage		= DAMAGE_NO;

	if (m_nHomingStrength > 100)
	{
		m_nHomingStrength = 100;
		Warning("WARNING: NPC_Launcher Homing Strength must be between 0 and 100\n");
	}
	
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	SetBloodColor( DONT_BLEED );
	AddEffects( EF_NODRAW );

	AddFlag( FL_NPC );

	CapabilitiesAdd( bits_CAP_SQUAD );

	InitRelationshipTable();
	
	if (m_nStartOn)
	{
		LauncherTurnOn();
	}

	// -------------------------------------------------------
	//  If I form squads add me to a squad
	// -------------------------------------------------------
	// @TODO (toml 12-05-02): RECONCILE WITH SAVE/RESTORE	
	if (!m_pSquad)
	{
		m_pSquad = g_AI_SquadManager.FindCreateSquad(this, m_SquadName);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Class_T	CNPC_Launcher::Classify( void )
{
	return	CLASS_NONE;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CNPC_Launcher::LaunchGrenade( CBaseEntity* pEnemy )
{
	// If a path following missile, create a path following missile
	if (m_sPathCornerName != NULL_STRING)
	{
		CGrenadePathfollower *pGrenade = CGrenadePathfollower::CreateGrenadePathfollower( m_sMissileModel, m_sFlySound,  GetAbsOrigin(), vec3_angle, edict() );
		pGrenade->SetDamage(m_flDamage);
		pGrenade->SetDamageRadius(m_flDamageRadius);
		pGrenade->Launch(m_flLaunchSpeed,m_sPathCornerName);
	}
	else
	{
		Vector vUp;
		AngleVectors( GetAbsAngles(), NULL, NULL, &vUp );
		Vector vLaunchVelocity = (vUp * m_flLaunchSpeed);

		CGrenadeHomer *pGrenade = CGrenadeHomer::CreateGrenadeHomer( m_sMissileModel, m_sFlySound,  GetAbsOrigin(), vec3_angle, edict() );
		pGrenade->Spawn( );
		pGrenade->SetSpin(m_flSpinMagnitude,m_flSpinSpeed);
		pGrenade->SetHoming((0.01*m_nHomingStrength),m_flHomingDelay,m_flHomingRampUp,m_flHomingDuration,m_flHomingRampDown);
		pGrenade->SetDamage(m_flDamage);
		pGrenade->SetDamageRadius(m_flDamageRadius);
		pGrenade->Launch(this,pEnemy,vLaunchVelocity,m_flHomingSpeed,GetGravity(),m_nSmokeTrail);
	}

	CPASAttenuationFilter filter( this, 0.3 );

	EmitSound_t ep;
	ep.m_nChannel = CHAN_WEAPON;
	ep.m_pSoundName = STRING(m_sLaunchSound);
	ep.m_SoundLevel = SNDLVL_NORM;

	EmitSound( filter, entindex(), ep );

	if (m_bSmokeLaunch)
	{
		UTIL_Smoke(GetAbsOrigin(), random->RandomInt(20,30), random->RandomInt(10,15));
	}
	m_flNextAttack = gpGlobals->curtime + LAUNCHER_REST_TIME;

}

//------------------------------------------------------------------------------
// Purpose : Launcher sees 360 degrees
//------------------------------------------------------------------------------
bool CNPC_Launcher::FInViewCone( CBaseEntity *pEntity )
{
	return true;
}

//------------------------------------------------------------------------------
// Purpose : Override base class to check range and visibility
//------------------------------------------------------------------------------
bool CNPC_Launcher::IsValidEnemy( CBaseEntity *pTarget )
{
	// ---------------------------------
	//  Check range
	// ---------------------------------
	float flTargetDist = (GetAbsOrigin() - pTarget->GetAbsOrigin()).Length();
	if (flTargetDist < m_flMinAttackDist)
	{
		return false;
	}
	if (flTargetDist > m_flMaxAttackDist)
	{
		return false;
	}

	if (!FBitSet (m_spawnflags, SF_LAUNCHER_CHECK_LOS))
	{
		return true;
	}
	// ------------------------------------------------------
	// Make sure I can see the target from above my position
	// ------------------------------------------------------
	trace_t tr;

	// Trace from launch position to target position.  
	// Use position above actual barral based on vertical launch speed
	Vector vStartPos = GetAbsOrigin() + Vector(0,0,0.2*m_flLaunchSpeed);
	Vector vEndPos	 = pTarget->GetAbsOrigin();
	AI_TraceLine( vStartPos, vEndPos, MASK_SHOT, pTarget, COLLISION_GROUP_NONE, &tr );

	if (tr.fraction == 1.0)
	{
		return true;
	}
	return false;
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_Launcher::LauncherThink( void )
{
	if (gpGlobals->curtime > m_flNextAttack)
	{
		// If enemy was set, fire at enemy
		if (GetEnemy())
		{
			LaunchGrenade(GetEnemy());
			m_OnLaunch.FireOutput(GetEnemy(), this);
			m_flNextAttack = gpGlobals->curtime + m_nLaunchDelay;
		}
		// Otherwise look for enemy to fire at
		else
		{
			GetSenses()->Look(m_flMaxAttackDist);
			CBaseEntity* pBestEnemy = BestEnemy();

			if (pBestEnemy)
			{
				LaunchGrenade(pBestEnemy);
				m_OnLaunch.FireOutput(pBestEnemy, this);
				m_flNextAttack = gpGlobals->curtime + m_nLaunchDelay;
			}
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CNPC_Launcher::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"State: %s", (m_pfnThink) ? "On" : "Off" );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"LOS: %s", (FBitSet (m_spawnflags, SF_LAUNCHER_CHECK_LOS)) ? "On" : "Off" );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}
