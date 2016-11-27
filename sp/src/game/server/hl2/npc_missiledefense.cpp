
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include	"ai_basenpc.h"
#include	"ai_hull.h"
#include	"ai_senses.h"
#include	"ai_memory.h"
#include	"soundent.h"
#include	"smoke_trail.h"
#include	"weapon_rpg.h"
#include	"gib.h"
#include	"ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "hl2_shareddefs.h"
#include "explode.h" //For the explosion
#include "effect_dispatch_data.h" //Muzzleflash
#include "te_effect_dispatch.h" //Muzzleflash
//#include "ai_basenpc.h" //Ignite
//#include "decals.h" //Scorch effect

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MD_FULLAMMO	50


#define MD_BC_YAW		1
#define MD_BC_PITCH		1
#define MD_AP_LGUN		2
#define MD_AP_RGUN		1
#define MD_GIB_COUNT	4
//#define MD_GIB_MODEL	"models/gibs/missile_defense_gibs.mdl"
#define MD_YAW_SPEED	48
#define MD_PITCH_SPEED  48

//------------------------------------
// Spawnflags
//------------------------------------
#define SF_MISSILEDEFENSE_BULLETDMG			(1 << 16) //Can be damaged with bullets? N on def

//=========================================================
//=========================================================
class CNPC_MissileDefense : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_MissileDefense, CAI_BaseNPC );
	DECLARE_DATADESC();

public:
	CNPC_MissileDefense( void ) { };
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void ) { return CLASS_NONE; }
	int		GetSoundInterests( void ) { return SOUND_NONE; }
	float	MaxYawSpeed( void ) { return 90.f; }

	void	RunAI(void);
	void	FireCannons( void );
	void	AimGun( void );
	void	EnemyShootPosition(CBaseEntity* pEnemy, Vector *vPosition);

	void	Event_Killed( const CTakeDamageInfo &info );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Explode(const Vector &vecExplosionPos);
	void	Gib();
	void	GetGunAim( Vector *vecAim );
	void	TurretTurnOn(void);
	void	DoMuzzleFlash(void);
	~CNPC_MissileDefense();

	Vector		m_vGunAng;
	int			m_iAmmoLoaded;
	float		m_flReloadedTime;

	string_t			m_sTurretModel;
	string_t			m_sGibModel;
	string_t			m_sFireSound;
	string_t			m_sRotateSound;
	string_t			m_sReloadSound;

	int					m_nStartOn;
	int				m_nHealth;

	// ----------------
	//	Inputs
	// ----------------
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
};

LINK_ENTITY_TO_CLASS( npc_missiledefense, CNPC_MissileDefense );


ConVar	sk_missiledefense_health( "sk_missiledefense_health","100");

//=========================================================
//=========================================================
BEGIN_DATADESC( CNPC_MissileDefense )

	DEFINE_FIELD( m_iAmmoLoaded,		FIELD_INTEGER ),
	DEFINE_FIELD( m_flReloadedTime,	FIELD_TIME ),
	DEFINE_FIELD( m_vGunAng,			FIELD_VECTOR ),

	DEFINE_KEYFIELD( m_sTurretModel,			FIELD_STRING,	"TurretModel" ),
	DEFINE_KEYFIELD( m_sGibModel,			FIELD_STRING,	"GibModel" ),
	DEFINE_KEYFIELD( m_sFireSound,				FIELD_STRING,	"FireSound" ),
	DEFINE_KEYFIELD( m_sRotateSound,				FIELD_STRING,	"RotateSound" ),
	DEFINE_KEYFIELD( m_sReloadSound,				FIELD_STRING,	"ReloadSound" ),
	DEFINE_KEYFIELD( m_nHealth,					FIELD_INTEGER,	"Health" ),

	DEFINE_INPUT( m_nStartOn,			FIELD_INTEGER,	"StartOn" ),

	DEFINE_FIELD( m_nStartOn, FIELD_BOOLEAN ), //Starts on?

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

// ===================
//  Input Functions
// ===================

//--------------------------------------------------------------
// Purpose: Enables npc
//--------------------------------------------------------------
void CNPC_MissileDefense::InputTurnOn( inputdata_t &inputdata )
{
	m_nStartOn = 1;
	SetThink( &CNPC_MissileDefense::CallNPCThink );
	SetNextThink( gpGlobals->curtime );
}

//--------------------------------------------------------------
// Purpose: Disables npc
//--------------------------------------------------------------
void CNPC_MissileDefense::InputTurnOff( inputdata_t &inputdata )
{
	m_nStartOn = 0;
	SetThink(NULL);
}


//--------------------------------------------------------------
// Purpose: Precache models and sounds that are going to be used
//--------------------------------------------------------------
void CNPC_MissileDefense::Precache( void )
{
	PrecacheModel(STRING(m_sTurretModel));
	PrecacheModel(STRING(m_sGibModel));

	PrecacheScriptSound( STRING(m_sFireSound));
	PrecacheScriptSound( STRING(m_sRotateSound));
	PrecacheScriptSound( STRING(m_sReloadSound));
}

//---------------------------------------------------------
// Purpose: Spawns the npc
//---------------------------------------------------------
void CNPC_MissileDefense::Spawn( void )
{
	Precache();

	//Gets gib model defined by the mapper
	char *szModel = (char *)STRING( m_sTurretModel );

	SetModel( szModel );	

	UTIL_SetSize( this, Vector( -36, -36 , 0 ), Vector( 36, 36, 64 ) );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_NONE );
	m_takedamage		= DAMAGE_YES;
	SetBloodColor( DONT_BLEED );

	//Gets health defined by the mapper, if not set, get the cvar one
	if(!m_nHealth)
	{
		m_iHealth			= sk_missiledefense_health.GetFloat();
	}
	else
	{
		m_iHealth			= m_nHealth;
	}

	m_flFieldOfView		= VIEW_FIELD_FULL; //0.4f
	m_NPCState			= NPC_STATE_ALERT;
	CapabilitiesClear();
	CapabilitiesAdd ( bits_CAP_INNATE_RANGE_ATTACK1 );

	// Hate missiles	
	AddClassRelationship( CLASS_MISSILE, D_HT, 5 );

	m_spawnflags |= SF_NPC_LONG_RANGE;

	m_flReloadedTime = gpGlobals->curtime;

	InitBoneControllers();

	NPCInit();

	SetBoneController( MD_BC_YAW, 0 ); //10
	SetBoneController( MD_BC_PITCH, 0 );

	SetBodygroup( 1, 0 );
	SetBodygroup( 2, 0 );
	SetBodygroup( 3, 0 );
	SetBodygroup( 4, 0 );
}

//------------------------------------------------------------------------------
// Purpose : Main AI work
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::RunAI( void )
{

	//If Starts On option isn't activated, disable AI
	if (m_nStartOn == 0)
	{
		SetThink(NULL);
	}

	// If my enemy is dead clear the memory and reset m_hEnemy
	if ( ( GetEnemy() != NULL ) && ( GetEnemy()->IsAlive() == false ) )
	{
		ClearEnemyMemory();
		SetEnemy( NULL );
	}

	if (GetEnemy() == NULL )
	{
		// We have to refresh our memories before finding enemies, so
		// dead enemies are cleared out before new ones are added.
		GetEnemies()->RefreshMemories();

		GetSenses()->Look( 4092 );
		SetEnemy( BestEnemy( ) );

		if (GetEnemy() != NULL)
		{
			m_iAmmoLoaded = MD_FULLAMMO;
			m_flReloadedTime = gpGlobals->curtime;
		}
	}

	if( m_iAmmoLoaded < 1 && gpGlobals->curtime > m_flReloadedTime )
	{
		m_iAmmoLoaded = MD_FULLAMMO;
	}

	AimGun();
	FireCannons();
	SetNextThink( gpGlobals->curtime + 0.05 );
}

//---------------------------------------------------------
// Purpose: Starts aiming the gun
//---------------------------------------------------------
void CNPC_MissileDefense::GetGunAim( Vector *vecAim )
{
	Vector vecPos;
	QAngle vecAng;

	GetAttachment( MD_AP_LGUN, vecPos, vecAng );

	vecAng.x = GetLocalAngles().x + GetBoneController( MD_BC_PITCH );
	vecAng.z = 0;
	vecAng.y = GetLocalAngles().y + GetBoneController( MD_BC_YAW );

	Vector vecForward;
	AngleVectors( vecAng, &vecForward );

	*vecAim = vecForward;
}


#define NOISE 0.035f
#define MD_ATTN_CANNON 0.4
//------------------------------------------------------------------------------
// Purpose : Open fire on the rocket
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::FireCannons( void )
{
	// ----------------------------------------------
	//  Make sure I have an enemy
	// ----------------------------------------------
	if (GetEnemy() == NULL)
	{
		return;
	}

	// ----------------------------------------------
	//  Make sure I have ammo
	// ----------------------------------------------
	if( m_iAmmoLoaded < 1 )
	{
		return;
	}
	// ----------------------------------------------
	// Make sure gun it pointing in right direction	
	//
	// Disabled, so it makes work fine the turret
	// ----------------------------------------------
	/*Vector vGunDir;
	GetGunAim( &vGunDir );
	Vector vTargetPos;
	EnemyShootPosition(GetEnemy(),&vTargetPos);

	Vector vTargetDir = vTargetPos - GetAbsOrigin();
	VectorNormalize( vTargetDir );

	float fDotPr = DotProduct( vGunDir, vTargetDir );
	if (fDotPr < 0.95)
	{
		return;
	}*/

	// -----------------------------------------------------------------------------------------
	// Check line of sight
	//
	// For some unknown reason, with two or more turrets it makes them to do not fire. Disabled.
	// -----------------------------------------------------------------------------------------
	/*trace_t tr;
	AI_TraceLine( GetEnemy()->EyePosition(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
	if (tr.fraction < 1.0)
	{
		return;
	}
	*/

	Vector vecRight;
	Vector vecDir;
	Vector vecCenter;
	AngleVectors( GetLocalAngles(), NULL, &vecRight, NULL );

	vecCenter = WorldSpaceCenter();

	if( GetEnemy() == NULL )
	{
		return;
	}

	bool fSound = false;
	if( random->RandomInt( 0, 3 ) == 0 )
	{
		fSound = true;
	}

	//Plays the fire sound
	char *sFireSound = (char *)STRING( m_sFireSound );
	EmitSound(sFireSound);

	DoMuzzleFlash();

	Vector vecGun;
	QAngle vecAng;

	GetAttachment( MD_AP_LGUN, vecGun, vecAng );

	Vector vecTarget;
	EnemyShootPosition(GetEnemy(),&vecTarget);

	vecDir = vecTarget - vecCenter;
	VectorNormalize(vecDir);
	vecDir.x += random->RandomFloat( -NOISE, NOISE );
	vecDir.y += random->RandomFloat( -NOISE, NOISE );

	Vector vecStart = vecGun + vecDir * 110;
	Vector vecEnd	= vecGun + vecDir * 4096;
	UTIL_Tracer( vecStart, vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 3000 + random->RandomFloat( 0, 2000 ), fSound );

	vecDir = vecTarget - vecCenter;
	VectorNormalize(vecDir);
	vecDir.x += random->RandomFloat( -NOISE, NOISE );
	vecDir.y += random->RandomFloat( -NOISE, NOISE );
	vecDir.z += random->RandomFloat( -NOISE, NOISE );

	GetAttachment( MD_AP_RGUN, vecGun, vecAng );
	vecStart = vecGun + vecDir * 110;
	vecEnd = vecGun + vecDir * 4096;
	UTIL_Tracer( vecStart, vecEnd, 0, TRACER_DONT_USE_ATTACHMENT, 3000 + random->RandomFloat( 0, 2000 ) );

	m_iAmmoLoaded -= 2;

	if( m_iAmmoLoaded < 1 )
	{
		// Incite a reload.
		char *sReloadSound = (char *)STRING( m_sReloadSound );
		EmitSound(sReloadSound);

		m_flReloadedTime = gpGlobals->curtime + 0.3;
		return;
	}

	// Do damage to the missile based on distance.
	// if < 1, make damage 0.

	float flDist = (GetEnemy()->GetLocalOrigin() - vecGun).Length();
	float flDamage;

	flDamage = 4000 - flDist;

	flDamage /= 1000.0;

	if( flDamage > 0 )
	{
		if( flDist <= 1500 )
		{
			flDamage *= 2;
		}

		CTakeDamageInfo info( this, this, flDamage, DMG_MISSILEDEFENSE );
		CalculateBulletDamageForce( &info, GetAmmoDef()->Index("SMG1"), vecDir, GetEnemy()->GetAbsOrigin() );
		GetEnemy()->TakeDamage( info );
	}
}

//------------------------------------------------------------------------------
// Purpose : Takes damage only from blasts and bullets
// Input   :
// Output  :
//------------------------------------------------------------------------------
int CNPC_MissileDefense::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{

	// Only take blast damage
	if (info.GetDamageType() & DMG_BLAST )
	{
		return BaseClass::OnTakeDamage_Alive( info );
	}

	//
	// Bullets can damage it if their flag is checked
	//
	else if (info.GetDamageType() & DMG_BULLET)
	{
		if(HasSpawnFlags(SF_MISSILEDEFENSE_BULLETDMG))
		{
			return BaseClass::OnTakeDamage_Alive( info );
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//------------------------------------------------------------------------------
// Purpose : Stuff to do when gets killed
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::Event_Killed( const CTakeDamageInfo &info )
{
	//Set on fire
	//Ignite( 60, false );

	//Paint me black
	//Scorch( 8, 50 );

	char *sRotateSound = (char *)STRING( m_sRotateSound );
	StopSound( sRotateSound );

	CTakeDamageInfo dmgInfo = info;

	Explode( dmgInfo.GetDamagePosition() );

	Gib();
}

//------------------------------------------------------------------------------
// Purpose : Makes an explosion
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::Explode(const Vector &vecExplosionPos)
{
	//Explodes!
	ExplosionCreate( vecExplosionPos, vec3_angle, this, 1000, 500.0f,
		SF_ENVEXPLOSION_NODAMAGE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS	|
		SF_ENVEXPLOSION_NOSMOKE  | SF_ENVEXPLOSION_NOFIREBALLSMOKE, 0 );
	UTIL_ScreenShake( vecExplosionPos, 25.0, 150.0, 1.0, 750.0f, SHAKE_START );
}

//------------------------------------------------------------------------------
// Purpose : Gibs and some more effects when death
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::Gib(void)
{
	//Gets gib model defined by the mapper
	char *szModelGib = (char *)STRING( m_sGibModel );

	// Sparks
	for (int i = 0; i < 4; i++)
	{
		Vector sparkPos = GetAbsOrigin();
		sparkPos.x += random->RandomFloat(-12,12);
		sparkPos.y += random->RandomFloat(-12,12);
		sparkPos.z += random->RandomFloat(-12,12);
		g_pEffects->Sparks(sparkPos);
	}

	// Smoke
	UTIL_Smoke(GetAbsOrigin(), random->RandomInt(10, 15), 10);

	// Light
	CBroadcastRecipientFilter filter;

	te->DynamicLight( filter, 0.0,
			&GetAbsOrigin(), 255, 180, 100, 0, 100, 0.1, 0 );

	// Remove top parts
	SetBodygroup( 1, 1 );
	SetBodygroup( 2, 1 );
	SetBodygroup( 3, 1 );
	SetBodygroup( 4, 1 );
	m_takedamage = 0;
	SetThink(NULL);

	// Throw manhackgibs
	CGib::SpawnSpecificGibs( this, MD_GIB_COUNT, 300, 500, szModelGib);
}

//------------------------------------------------------------------------------
// Purpose : Add a little prediction into my enemy aim position
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::EnemyShootPosition(CBaseEntity* pEnemy, Vector *vPosition)
{
	// This should never happen, but just in case
	if (!pEnemy)
	{
		return;
	}

	*vPosition = pEnemy->GetAbsOrigin();

	// Add prediction but prevents us from flipping around as enemy approaches us
	float	flDist		= (pEnemy->GetAbsOrigin() - GetAbsOrigin()).Length();
	Vector	vPredVel	= pEnemy->GetSmoothedVelocity() * 0.5;
	if ( flDist > vPredVel.Length())
	{
		*vPosition += vPredVel;
	}
}

//------------------------------------------------------------------------------
// Purpose : Moves the turret to aim the rocket
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CNPC_MissileDefense::AimGun( void )
{
	if (GetEnemy() == NULL)
	{
		char *sRotateSound = (char *)STRING( m_sRotateSound );
		StopSound( sRotateSound );
		return;
	}

	Vector forward, right, up;
	AngleVectors( GetLocalAngles(), &forward, &right, &up );

	// Get gun attachment points
	Vector vBasePos;
	QAngle vBaseAng;
	GetAttachment( MD_AP_LGUN, vBasePos, vBaseAng );

	Vector vTargetPos;
	EnemyShootPosition(GetEnemy(),&vTargetPos);

	Vector vTargetDir = vTargetPos - vBasePos;
	VectorNormalize( vTargetDir );

	Vector vecOut;
	vecOut.x = DotProduct( forward, vTargetDir );
	vecOut.y = -DotProduct( right, vTargetDir );
	vecOut.z = DotProduct( up, vTargetDir );

	QAngle angles;
	VectorAngles(vecOut, angles);

	//Pitch for the cannons
	if (angles.x != m_vGunAng.x){
		float flDir = m_vGunAng.x > angles.x ? 1 : -1;
		angles.x += 0.1 * MD_PITCH_SPEED * flDir;

		SetBoneController(1, angles.x - m_vGunAng.x);
	}

	//Yaw for the cannons
	if (angles.y != m_vGunAng.y){
		float flDir = m_vGunAng.y > angles.y ? 1 : -1;
		float flDist = fabs(m_vGunAng.y - angles.y);

		if (flDist > 180){
			flDist = 360 - flDist;
			flDir =- flDir;
		}

		angles.y += 0.1 * MD_YAW_SPEED * flDir;

		if (angles.y < 0)
			angles.y += 360;
		else if (angles.y >= 360)
			angles.y -= 360;

		if(flDist < (0.05 * MD_YAW_SPEED))
			angles.y = m_vGunAng.y;

		SetBoneController(0, angles.y - m_vGunAng.y);
	}

	//Plays movement sound
	if (angles.x != m_vGunAng.x || angles.y != m_vGunAng.y)
	{
		//EmitSound( "NPC_FloorTurret.Alarm" );
		char *sRotateSound = (char *)STRING( m_sRotateSound );
		EmitSound( sRotateSound );
	}
	else
	{
		//StopSound( "NPC_FloorTurret.Alarm" );
		char *sRotateSound = (char *)STRING( m_sRotateSound );
		StopSound( sRotateSound );
	}
}


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
CNPC_MissileDefense::~CNPC_MissileDefense(void)
{
	char *sRotateSound = (char *)STRING( m_sRotateSound );
	StopSound( sRotateSound );
}

//-----------------------------------------------------------------------------
// Purpose: Puts a muzzleflash on the cannons
//-----------------------------------------------------------------------------
void CNPC_MissileDefense::DoMuzzleFlash( void )
{
	CEffectData data, data2;
	data.m_nEntIndex = entindex();
	data2.m_nEntIndex = entindex();
	data.m_nAttachmentIndex = LookupAttachment( "R_Gun_AtchPnt" );
	data2.m_nAttachmentIndex = LookupAttachment( "L_Gun_AtchPnt" );
	data.m_flScale = 1.0f;
	data2.m_flScale = 1.0f;
	DispatchEffect( "ChopperMuzzleFlash", data );
	DispatchEffect( "ChopperMuzzleFlash", data2 );
}