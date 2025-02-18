//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// TODO: 
//		Take advantage of new NPC fields like GetEnemy() and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest NPC code
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "Sprite.h"
#include "basecombatweapon.h"
#include "ai_basenpc.h"
#include "AI_Senses.h"
#include "AI_Memory.h"
#include "gamerules.h"
#include "ammodef.h"
#include "ndebugoverlay.h"
#include "IEffects.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"

class CSprite;

#define TURRET_RANGE	(100 * 12)
#define TURRET_SPREAD	VECTOR_CONE_5DEGREES
#define TURRET_TURNRATE	360		// max angles per second
#define TURRET_MAXWAIT	15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN	5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME	0.5

#define TURRET_BC_YAW	"aim_yaw"
#define TURRET_BC_PITCH	"aim_pitch"

#define TURRET_ORIENTATION_FLOOR 0
#define TURRET_ORIENTATION_CEILING 1

//=========================================================
// private activities
//=========================================================
int ACT_TURRET_OPEN;
int ACT_TURRET_CLOSE;
int ACT_TURRET_OPEN_IDLE;
int ACT_TURRET_CLOSED_IDLE;
int ACT_TURRET_FIRE;
int ACT_TURRET_RELOAD;

// ===============================================
//  Private spawn flags  (must be above (1<<15))
// ===============================================
#define SF_NPC_TURRET_AUTOACTIVATE		0x00000020
#define SF_NPC_TURRET_STARTINACTIVE		0x00000040

extern short		g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

ConVar	sk_miniturret_health( "sk_miniturret_health","0");
ConVar	sk_sentry_health( "sk_sentry_health","0");
ConVar	sk_turret_health( "sk_turret_health","0");

class CBaseTurret : public CAI_BaseNPC
{
	DECLARE_CLASS( CBaseTurret, CAI_BaseNPC );
public:
	void Spawn(void);
	virtual void Precache(void);
	bool KeyValue( const char *szKeyName, const char *szValue );
	//void TurretUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual Class_T		Classify(void);

	int BloodColor( void ) { return DONT_BLEED; }
	bool Event_Gibbed( void ) { return FALSE; }	// UNDONE: Throw turret gibs?

	Vector	EyeOffset( Activity nActivity );
	Vector	EyePosition( void );

	// Inputs
	void InputToggle( inputdata_t &inputdata );

	// Think functions

	void ActiveThink(void);
	void SearchThink(void);
	void AutoSearchThink(void);
	void TurretDeath(void);

	void Deploy(void);
	void Retire(void);
	
	void Initialize(void);

	virtual void Ping(void);
	virtual void EyeOn(void);
	virtual void EyeOff(void);

	DECLARE_DATADESC();

	// other functions
	int MoveTurret(void);
	virtual void Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy) { };

	CSprite *m_pEyeGlow;
	int		m_eyeBrightness;

	int	m_iDeployHeight;
	int	m_iRetractHeight;
	int m_iMinPitch;

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int	m_iOn;
	int m_fBeserk;			// Sometimes this will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
							// enters its range

	Vector	m_vecLastSight;	// Last seen position
	float	m_flLastSight;	// Last time we saw a target
	float	m_flMaxWait;	// Max time to search w/o a target

	// movement
	float	m_flStartYaw;
	QAngle	m_vecGoalAngles;

	int		m_iAmmoType;

	float	m_flPingTime;	// Time until the next ping, used when searching
	float	m_flDamageTime;	// Time we last took damage.

	COutputEvent m_OnDeploy;
	COutputEvent m_OnRetire;

	// external
	//COutputEvent		m_OnDamaged;
	//COutputEvent		m_OnDeath;
	//COutputEvent		m_OnHalfHealth;
	//COutputEHANDLE		m_OnFoundEnemy; 
	//COutputEvent		m_OnLostEnemyLOS; 
	//COutputEvent		m_OnLostEnemy; 
	//COutputEHANDLE		m_OnFoundPlayer;
	//COutputEvent		m_OnLostPlayerLOS;
	//COutputEvent		m_OnLostPlayer; 
	//COutputEvent		m_OnHearWorld;
	//COutputEvent		m_OnHearPlayer;
	//COutputEvent		m_OnHearCombat;

};


BEGIN_DATADESC( CBaseTurret )

	DEFINE_FIELD( m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( m_iDeployHeight, FIELD_INTEGER ),
	DEFINE_FIELD( m_iRetractHeight, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMinPitch, FIELD_INTEGER ),

	DEFINE_FIELD( m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( m_fTurnRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_iOn, FIELD_INTEGER ),
	DEFINE_FIELD( m_fBeserk, FIELD_INTEGER ),
	DEFINE_FIELD( m_iAutoStart, FIELD_INTEGER ),

	DEFINE_FIELD( m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( m_flMaxWait, FIELD_FLOAT ),

	DEFINE_FIELD( m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecGoalAngles, FIELD_VECTOR ),

	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),

	DEFINE_FIELD( m_flPingTime, FIELD_TIME ),
	DEFINE_FIELD( m_flDamageTime, FIELD_TIME ),

	// Function pointers
	//DEFINE_USEFUNC( TurretUse ),
	DEFINE_THINKFUNC( ActiveThink ),
	DEFINE_THINKFUNC( SearchThink ),
	DEFINE_THINKFUNC( AutoSearchThink ),
	DEFINE_THINKFUNC( TurretDeath ),
	DEFINE_THINKFUNC( Deploy ),
	DEFINE_THINKFUNC( Retire ),
	DEFINE_THINKFUNC( Initialize ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	
	// Outputs
	DEFINE_OUTPUT(m_OnDeploy, "OnDeploy"),
	DEFINE_OUTPUT(m_OnRetire, "OnRetire"),

END_DATADESC()



//------------------------------------------------------------------------------
// Purpose : 
// Input   :
// Output  :
//------------------------------------------------------------------------------
Vector	CBaseTurret::EyeOffset( Activity nActivity )
{
	return Vector( 0, 0, -20 );
}


Vector	CBaseTurret::EyePosition( void )
{
	Vector vecOrigin;
	QAngle vecAngles;

	GetAttachment( "eyes", vecOrigin, vecAngles );
	return vecOrigin;
}

bool CBaseTurret::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "maxsleep"))
	{
		m_flMaxWait = atof(szValue);
	}
	else if (FStrEq(szKeyName, "turnrate"))
	{
		m_iBaseTurnRate = atoi(szValue);
	}
	else if (FStrEq(szKeyName, "style") ||
			 FStrEq(szKeyName, "height") ||
			 FStrEq(szKeyName, "value1") ||
			 FStrEq(szKeyName, "value2") ||
			 FStrEq(szKeyName, "value3"))
	{
	}
	else
		return BaseClass::KeyValue( szKeyName, szValue );

	return true;
}


void CBaseTurret::Spawn()
{ 
	Precache( );
	SetNextThink( gpGlobals->curtime + 1 );
	SetMoveType( MOVETYPE_FLY );
	m_nSequence		= 0;
	m_flCycle		= 0;
	SetSolid( SOLID_SLIDEBOX );
	m_takedamage		= DAMAGE_YES;
	AddFlag( FL_AIMTARGET );

	m_iAmmoType = g_pGameRules->GetAmmoDef()->Index("SMG1");

	AddFlag( FL_NPC );

	if (( m_spawnflags & SF_NPC_TURRET_AUTOACTIVATE ) && !( m_spawnflags & SF_NPC_TURRET_STARTINACTIVE ))
	{
		m_iAutoStart = true;
	}

	ResetSequenceInfo( );

	SetPoseParameter( TURRET_BC_YAW, 0 );
	SetPoseParameter( TURRET_BC_PITCH, 0 );

	// Activities
	ADD_CUSTOM_ACTIVITY( CBaseTurret, ACT_TURRET_OPEN );
	ADD_CUSTOM_ACTIVITY( CBaseTurret, ACT_TURRET_CLOSE );
	ADD_CUSTOM_ACTIVITY( CBaseTurret, ACT_TURRET_CLOSED_IDLE );
	ADD_CUSTOM_ACTIVITY( CBase`matTurret, ACT_TURRET_OPEN_IDLE );
	ADD_CUSTOM_ACTIVITY( CBaseTurret, ACT_TURRET_FIRE );
	ADD_CUSTOM_ACTIVITY( CBaseTurret, ACT_TURRET_RELOAD );
}


void CBaseTurret::Precache( )
{
	BaseClass::Precache();

	PrecacheScriptSound( "NPC_Turret.Ping" );
	PrecacheScriptSound( "NPC_Turret.Deploy" );
	PrecacheScriptSound( "NPC_Turret.Retire" );
	PrecacheScriptSound( "NPC_Turret.Alert" );
	PrecacheScriptSound( "NPC_Turret.Die" );
}

void CBaseTurret::Initialize(void)
{
	m_iOn = 0;
	m_fBeserk = 0;

	SetPoseParameter( TURRET_BC_YAW, 0 );
	SetPoseParameter( TURRET_BC_PITCH, 0 );

	if (m_iBaseTurnRate == 0) m_iBaseTurnRate = TURRET_TURNRATE;
	if (m_flMaxWait == 0) m_flMaxWait = TURRET_MAXWAIT;

	m_vecGoalAngles = GetAngles();

	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->curtime + m_flMaxWait;
		SetThink(AutoSearchThink);		
		SetNextThink( gpGlobals->curtime + .1 );
	}
	else
		SetThink(SUB_DoNothing);
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the turret on/off.
//-----------------------------------------------------------------------------
void CBaseTurret::InputToggle( inputdata_t &inputdata )
{
	//if ( !ShouldToggle( useType, m_iOn ) )
	//	return;

	if (m_iOn)
	{
		SetEnemy( NULL );
		SetNextThink( gpGlobals->curtime + 0.1f );
		m_iAutoStart = FALSE;// switching off a turret disables autostart
		//!!!! this should spin down first!!BUGBUG
		SetThink(Retire);
	}
	else 
	{
		SetNextThink( gpGlobals->curtime + 0.1f ); // turn on delay

		// if the turret is flagged as an autoactivate turret, re-enable its ability open self.
		if ( m_spawnflags & SF_NPC_TURRET_AUTOACTIVATE )
		{
			m_iAutoStart = TRUE;
		}
		
		SetThink(Deploy);
	}
}


void CBaseTurret::Ping( void )
{
	// make the pinging noise every second while searching
	if (m_flPingTime == 0)
		m_flPingTime = gpGlobals->curtime + 1;
	else if (m_flPingTime <= gpGlobals->curtime)
	{
		m_flPingTime = gpGlobals->curtime + 1;
		EmitSound( "NPC_Turret.Ping" );
		EyeOn( );
	}
	else if (m_eyeBrightness > 0)
	{
		EyeOff( );
	}
}


void CBaseTurret::EyeOn( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness != 255)
		{
			m_eyeBrightness = 255;
		}
		m_pEyeGlow->SetBrightness( m_eyeBrightness );
	}
}


void CBaseTurret::EyeOff( )
{
	if (m_pEyeGlow)
	{
		if (m_eyeBrightness > 0)
		{
			m_eyeBrightness = MAX( 0, m_eyeBrightness - 30 );
			m_pEyeGlow->SetBrightness( m_eyeBrightness );
		}
	}
}


void CBaseTurret::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	SetNextThink( gpGlobals->curtime + 0.1f );
	StudioFrameAdvance( );

	if ((!m_iOn) || (GetEnemy() == NULL))
	{
		SetEnemy( NULL );
		m_flLastSight = gpGlobals->curtime + m_flMaxWait;
		SetThink(SearchThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !GetEnemy()->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->curtime + 0.5; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->curtime > m_flLastSight)
			{	
				SetEnemy( NULL );
				m_flLastSight = gpGlobals->curtime + m_flMaxWait;
				SetThink(SearchThink);
				return;
			}
		}
	}

	Vector vecMid = EyePosition( );
	Vector vecMidEnemy = GetEnemy()->BodyTarget(vecMid);

	// g_pEffects->Sparks( vecMid );
	// g_pEffects->Sparks( vecMidEnemy );

	// Look for our current enemy
	//int fEnemyVisible = FBoxVisible( this, GetEnemy(), vecMidEnemy );	
	int fEnemyVisible = FInViewCone( GetEnemy() ) && FVisible( GetEnemy() );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	// NDebugOverlay::Line( vecMid, vecMidEnemy, 0, 255, 0, false, 0.1 );
	
	float flDistToEnemy = vecDirToEnemy.Length();

	QAngle vecAnglesToEnemy;
	VectorNormalize( vecDirToEnemy );
	VectorAngles( vecDirToEnemy, vecAnglesToEnemy );

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > TURRET_RANGE))
	{
		// DevMsg( "lost you\n" );

		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->curtime + 0.5;
		}
		else
		{
			// Should we look for a new target?
			if (gpGlobals->curtime > m_flLastSight)
			{
				ClearEnemyMemory();
				SetEnemy( NULL );
				m_flLastSight = gpGlobals->curtime + m_flMaxWait;
				SetThink(SearchThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	VectorNormalize( vecLOS );

	Vector vecMuzzle, vecMuzzleDir;
	QAngle vecMuzzleAng;
	GetAttachment( "eyes", vecMuzzle, vecMuzzleAng );

	AngleVectors( vecMuzzleAng, &vecMuzzleDir );
	
	// Is the Gun looking at the target
	if (DotProduct(vecLOS, vecMuzzleDir) <= 0.9848) // 10 degree slop
	{
		fAttack = FALSE;
	}
	else
	{
		fAttack = TRUE;
	}

	// fire the gun
	if (fAttack || m_fBeserk)
	{
		m_Activity = ACT_RESET;
		SetActivity( (Activity)ACT_TURRET_FIRE );
		Shoot(vecMuzzle, vecMuzzleDir );
	} 
	else
	{
		SetActivity( (Activity)ACT_TURRET_OPEN_IDLE );
	}

	//move the gun
	if (m_fBeserk)
	{
		// DevMsg( "berserk" );

		if (random->RandomInt(0,9) == 0)
		{
			m_vecGoalAngles.y = random->RandomFloat(-180,180);
			m_vecGoalAngles.x = random->RandomFloat(-90,90);
			OnTakeDamage( CTakeDamageInfo( this, this, 1, DMG_GENERIC ) ); // don't beserk forever
			return;
		}
	} 
	else if (fEnemyVisible)
	{
		// DevMsg( "->[%.2f]\n", vec.x);
		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;

	}

	MoveTurret();
}


void CBaseTurret::Deploy(void)
{
	SetNextThink( gpGlobals->curtime + 0.1f );
	StudioFrameAdvance( );

	if ( m_Activity != ACT_TURRET_OPEN )
	{
		m_iOn = 1;
		SetActivity( (Activity)ACT_TURRET_OPEN );
		EmitSound( "NPC_Turret.Deploy" );

		m_OnDeploy.FireOutput(NULL, this);
	}

	if (m_fSequenceFinished)
	{
		Vector curmins, curmaxs;
		curmins = WorldAlignMins();
		curmaxs = WorldAlignMaxs();

		curmaxs.z = m_iDeployHeight;
		curmins.z = -m_iDeployHeight;

		SetCollisionBounds( curmins, curmaxs );

		Relink();

		SetActivity( (Activity)ACT_TURRET_OPEN_IDLE );

		m_flPlaybackRate = 0;
		SetThink(SearchThink);
	}

	m_flLastSight = gpGlobals->curtime + m_flMaxWait;
}

void CBaseTurret::Retire(void)
{
	// make the turret level
	m_vecGoalAngles = GetAngles( );

	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance( );

	EyeOff( );

	if ( m_Activity != ACT_TURRET_CLOSE )
	{
		SetActivity( (Activity)ACT_TURRET_OPEN_IDLE );
		
		if (!MoveTurret())
		{
			SetActivity( (Activity)ACT_TURRET_CLOSE );
			EmitSound( "NPC_Turret.Retire" );

			m_OnRetire.FireOutput(NULL, this);
		}
	}
	else if (m_fSequenceFinished) 
	{	
		m_iOn = 0;
		m_flLastSight = 0;

		SetActivity( (Activity)ACT_TURRET_CLOSED_IDLE );

		Vector curmins, curmaxs;
		curmins = WorldAlignMins();
		curmaxs = WorldAlignMaxs();

		curmaxs.z = m_iRetractHeight;
		curmins.z = -m_iRetractHeight;

		SetCollisionBounds( curmins, curmaxs );
		Relink();

		if (m_iAutoStart)
		{
			SetThink(AutoSearchThink);		
			SetNextThink( gpGlobals->curtime + .1 );
		}
		else
		{
			SetThink(SUB_DoNothing);
		}
	}
}


//
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret::SearchThink(void)
{
	// ensure rethink
	SetActivity( (Activity)ACT_TURRET_OPEN_IDLE );

	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	Ping( );

	// If we have a target and we're still healthy
	if (GetEnemy() != NULL)
	{
		if (!GetEnemy()->IsAlive() )
			SetEnemy( NULL );// Dead enemy forces a search for new one
	}

	// Acquire Target
	if (GetEnemy() == NULL)
	{
		GetSenses()->Look(TURRET_RANGE);
		SetEnemy( BestEnemy() );
	}

	// If we've found a target, spin up the barrel and start to attack
	if (GetEnemy() != NULL)
	{
		m_flLastSight = 0;
		SetThink(ActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
 		if (gpGlobals->curtime > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			SetThink(Retire);
		}
		
		// generic hunt for new victims
		m_vecGoalAngles.y = (m_vecGoalAngles.y + 0.1 * m_iBaseTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;

		MoveTurret();
	}
}


// 
// This think function will deploy the turret when something comes into range. This is for
// automatically activated turrets.
//
void CBaseTurret::AutoSearchThink(void)
{
	// ensure rethink
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + random->RandomFloat( 0.2, 0.3 ) );

	// If we have a target and we're still healthy

	if (GetEnemy() != NULL)
	{
		if (!GetEnemy()->IsAlive() )
			SetEnemy( NULL );// Dead enemy forces a search for new one
	}

	// Acquire Target

	if (GetEnemy() == NULL)
	{
		GetSenses()->Look( TURRET_RANGE );
		SetEnemy( BestEnemy() );
	}

	if (GetEnemy() != NULL)
	{
		SetThink(Deploy);
		EmitSound( "NPC_Turret.Alert" );
	}
}


void CBaseTurret ::	TurretDeath( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (m_lifeState != LIFE_DEAD)
	{
		m_lifeState = LIFE_DEAD;

		EmitSound( "NPC_Turret.Die" );

		SetActivity( (Activity)ACT_TURRET_CLOSE );

		EyeOn( );	
	}

	EyeOff( );

	if (m_flDamageTime + random->RandomFloat( 0, 2 ) > gpGlobals->curtime)
	{
		// lots of smoke
		Vector pos;
		CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &pos );
		pos.z = CollisionProp()->GetCollisionOrigin().z;
		
		CBroadcastRecipientFilter filter;
		te->Smoke( filter, 0.0, &pos,
			g_sModelIndexSmoke,
			2.5,
			10 );
	}
	
	if (m_flDamageTime + random->RandomFloat( 0, 5 ) > gpGlobals->curtime)
	{
		Vector vecSrc;
		CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecSrc );
		g_pEffects->Sparks( vecSrc );
	}

	if (m_fSequenceFinished && !MoveTurret( ) && m_flDamageTime + 5 < gpGlobals->curtime)
	{
		m_flPlaybackRate = 0;
		SetThink( NULL );
	}
}



void CBaseTurret::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;

	if ( ptr->hitgroup == 10 )
	{
		// hit armor
		if ( m_flDamageTime != gpGlobals->curtime || (random->RandomInt(0,10) < 1) )
		{
			g_pEffects->Ricochet( ptr->endpos, (vecDir*-1.0f) );
			m_flDamageTime = gpGlobals->curtime;
		}

		info.SetDamage( 0.1 );// don't hurt the NPC much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}

	if ( !m_takedamage )
		return;

	AddMultiDamage( info, this );
}


int CBaseTurret::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( !m_takedamage )
		return 0;

	CTakeDamageInfo info = inputInfo;

	if (!m_iOn)
		info.ScaleDamage( 0.1f );

	m_iHealth -= info.GetDamage();
	if (m_iHealth <= 0)
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_NO;
		m_flDamageTime = gpGlobals->curtime;

		RemoveFlag( FL_NPC ); // why are they set in the first place???

		SetThink(TurretDeath);

		m_OnDamaged.FireOutput( info.GetInflictor(), this );

		SetNextThink( gpGlobals->curtime + 0.1f );

		return 0;
	}

	if (m_iHealth <= 10)
	{
		if (m_iOn && (1 || random->RandomInt(0, 0x7FFF) > 800))
		{
			m_fBeserk = 1;
			SetThink(SearchThink);
		}
	}

	return 1;
}


int CBaseTurret::MoveTurret(void)
{
	bool bDidMove = false;
	int iPose;

	matrix3x4_t localToWorld;
	
	GetAttachment( LookupAttachment( "eyes" ), localToWorld );

	Vector vecGoalDir;
	AngleVectors( m_vecGoalAngles, &vecGoalDir );

	Vector vecGoalLocalDir;
	VectorIRotate( vecGoalDir, localToWorld, vecGoalLocalDir );

	QAngle vecGoalLocalAngles;
	VectorAngles( vecGoalLocalDir, vecGoalLocalAngles );

	float flDiff;
	QAngle vecNewAngles;

  // update pitch
	flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.x, 0.0, 0.1 * m_iBaseTurnRate ) );
	iPose = LookupPoseParameter( TURRET_BC_PITCH );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + flDiff / 1.5 );

	if (fabs(flDiff) > 0.1)
	{
		bDidMove = true;
	}

	// update yaw, with acceleration
#if 0
	float flDist = AngleNormalize( vecGoalLocalAngles.y );
	float flNewDist;
	float flNewTurnRate;

	ChangeDistance( 0.1, flDist, 0.0, m_fTurnRate, m_iBaseTurnRate, m_iBaseTurnRate * 4, flNewDist, flNewTurnRate );
	m_fTurnRate = flNewTurnRate;
	flDiff = flDist - flNewDist;
#else
	flDiff = AngleNormalize( UTIL_ApproachAngle(  vecGoalLocalAngles.y, 0.0, 0.1 * m_iBaseTurnRate ) );
#endif

	iPose = LookupPoseParameter( TURRET_BC_YAW );
	SetPoseParameter( iPose, GetPoseParameter( iPose ) + flDiff / 1.5 );
	if (fabs(flDiff) > 0.1)
	{
		bDidMove = true;
	}

	if (bDidMove)
	{
		// DevMsg( "(%.2f, %.2f)\n", AngleNormalize( vecGoalLocalAngles.x ), AngleNormalize( vecGoalLocalAngles.y ) );
	}
	return bDidMove;
}

//
// ID as a machine
//
Class_T	CBaseTurret::Classify ( void )

{
	if (m_iOn || m_iAutoStart)
	{
		return	CLASS_MILITARY;
	}

	return CLASS_MILITARY;
}




//////////////////////////////////////////////////////////////////////////////////////////////////////


class CCeilingTurret : public CBaseTurret
{
	DECLARE_CLASS( CCeilingTurret, CBaseTurret );
public:
	void Spawn(void);
	void Precache(void);

	// other functions
	void Shoot( const Vector &vecSrc, const Vector &vecDirToEnemy );
};

#define TURRET_GLOW_SPRITE "sprites/glow01.vmt"

LINK_ENTITY_TO_CLASS( npc_turret_ceiling, CCeilingTurret );

void CCeilingTurret::Spawn()
{ 
	Precache( );

	SetModel( "models/combine_turrets/ceiling_turret.mdl" );
	
	BaseClass::Spawn( );

	m_iHealth			= sk_turret_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 12.75 );

	AngleVectors( GetAngles(), NULL, NULL, &m_vecViewOffset );
	m_vecViewOffset		= m_vecViewOffset * Vector( 0, 0, -64 );

	m_flFieldOfView		= VIEW_FIELD_FULL;

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -45;
	UTIL_SetSize(this, Vector(-32, -32, -m_iRetractHeight), Vector(32, 32, m_iRetractHeight));
	
	SetThink(Initialize);	

	m_pEyeGlow = CSprite::SpriteCreate( TURRET_GLOW_SPRITE, GetOrigin(), FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( this, 2 );
	m_eyeBrightness = 0;

	SetNextThink( gpGlobals->curtime + 0.3; );
}

void CCeilingTurret::Precache()
{
	PrecacheModel( "models/combine_turrets/ceiling_turret.mdl");	
	PrecacheModel( TURRET_GLOW_SPRITE );

	PrecacheScriptSound( "CeilingTurret.Shoot" );
	BaseClass::Precache();
}

void CCeilingTurret::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy)
{
	//NDebugOverlay::Line( vecSrc, vecSrc + vecDirToEnemy * 512, 0, 255, 255, false, 0.1 );
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );
	EmitSound( "CeilingTurret.Shoot" );
	
	DoMuzzleFlash();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0

class CMiniTurret : public CBaseTurret
{
	DECLARE_CLASS( CMiniTurret, CBaseTurret );
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy);
};

LINK_ENTITY_TO_CLASS( npc_miniturret, CMiniTurret );


void CMiniTurret::Spawn()
{ 
	Precache( );
	SetModel( "models/miniturret.mdl" );
	m_iHealth			= sk_miniturret_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 12.75 );
	m_vecViewOffset.z = 12.75;
	m_flFieldOfView		= VIEW_FIELD_NARROW;

	CBaseTurret::Spawn( );

	m_iAmmoType = g_pGameRules->GetAmmoDef()->Index("Pistol");

	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch	= -45;
	UTIL_SetSize(this, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetThink(Initialize);	
	SetNextThink( gpGlobals->curtime + 0.3; );
}


void CMiniTurret::Precache()
{
	PrecacheModel ("models/miniturret.mdl");	

	PrecacheScriptSound( "MiniTurret.Shoot" );

	BaseClass::Precache();
}



void CMiniTurret::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy)
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );

	EmitSound( "MiniTurret.Shoot" );

	DoMuzzleFlash();
}


#endif


//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
	DECLARE_CLASS( CSentry, CBaseTurret );
public:
	void Spawn( );
	void Precache(void);
	// other functions
	void Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy);
	int OnTakeDamage( const CTakeDamageInfo &info );
	void SentryTouch( CBaseEntity *pOther );
	void SentryDeath( void );

protected:

	DECLARE_DATADESC();
};


BEGIN_DATADESC( CSentry )

	// Function pointers
	DEFINE_ENTITYFUNC( SentryTouch ),
	DEFINE_THINKFUNC( SentryDeath ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( NPC_sentry, CSentry );

void CSentry::Precache()
{
	PrecacheModel ("models/sentry.mdl");	

	PrecacheScriptSound( "Sentry.Shoot" );
	PrecacheScriptSound( "Sentry.Die" );

	BaseClass::Precache();
}

void CSentry::Spawn()
{ 
	Precache( );
	SetModel( "models/sentry.mdl" );
	m_iHealth			= sk_sentry_health.GetFloat();
	m_HackedGunPos		= Vector( 0, 0, 48 );
	m_vecViewOffset.z		= 48;
	m_flMaxWait = 1E6;

	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch	= -60;
	UTIL_SetSize(this, Vector(-16, -16, -m_iRetractHeight), Vector(16, 16, m_iRetractHeight));

	SetTouch(SentryTouch);
	SetThink(Initialize);	
	SetNextThink( gpGlobals->curtime + 0.3; );
}

void CSentry::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy)
{
	FireBullets( 1, vecSrc, vecDirToEnemy, TURRET_SPREAD, TURRET_RANGE, m_iAmmoType, 1 );

	EmitSound( "Sentry.Shoot" );

	DoMuzzleFlash();
}


int CSentry::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_takedamage )
		return 0;

	if (!m_iOn)
	{
		SetThink( Deploy );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	m_iHealth -= info.GetDamage();
	if (m_iHealth <= 0)
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_NO;
		m_flDamageTime = gpGlobals->curtime;

		RemoveFlag( FL_NPC ); // why are they set in the first place???

		SetThink(SentryDeath);
		m_OnDamaged.FireOutput( info.GetInflictor(), this );
		SetNextThink( gpGlobals->curtime + 0.1f );

		return 0;
	}

	return 1;
}


void CSentry::SentryTouch( CBaseEntity *pOther )
{
	if ( pOther && (pOther->IsPlayer() || (pOther->GetFlags() & FL_NPC)) )
	{
		OnTakeDamage( CTakeDamageInfo( pOther, pOther, 0, 0 ) );
	}
}


void CSentry ::	SentryDeath( void )
{
	StudioFrameAdvance( );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if (m_lifeState != LIFE_DEAD)
	{
		m_lifeState = LIFE_DEAD;

		EmitSound( "Sentry.Die" );

		SetPoseParameter( TURRET_BC_YAW, 0 );
		SetPoseParameter( TURRET_BC_PITCH, 0 );

		SetActivity( (Activity)ACT_TURRET_CLOSE );

		SetSolid( SOLID_NOT );
		QAngle angles = GetAngles();
		angles.y = UTIL_AngleMod( GetAngles().y + random->RandomInt( 0, 2 ) * 120 );
		SetAngles( angles );

		EyeOn( );
	}

	EyeOff( );

	Vector vecSrc;
	QAngle vecAng;
	GetAttachment( "eyes", vecSrc, vecAng );

	if (m_flDamageTime + random->RandomFloat( 0, 2 ) > gpGlobals->curtime)
	{
		// lots of smoke
		Vector pos = vecSrc + Vector( random->RandomFloat( -16, 16 ), 
				random->RandomFloat( -16, 16 ),
				 -32 );

		CBroadcastRecipientFilter filter;
		te->Smoke( filter, 0.0, &pos,
			g_sModelIndexSmoke,
			1.5,
			8 );
	}
	
	if (m_flDamageTime + random->RandomFloat( 0, 8 ) > gpGlobals->curtime)
	{
		g_pEffects->Sparks( vecSrc );
	}

	if (m_fSequenceFinished && m_flDamageTime + 5 < gpGlobals->curtime)
	{
		m_flPlaybackRate = 0;
		SetThink( NULL );
	}
}

