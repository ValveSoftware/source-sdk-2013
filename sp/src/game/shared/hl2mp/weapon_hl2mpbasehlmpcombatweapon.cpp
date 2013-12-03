//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#include "hl2mp_player_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( basehl2mpcombatweapon, CBaseHL2MPCombatWeapon );

IMPLEMENT_NETWORKCLASS_ALIASED( BaseHL2MPCombatWeapon , DT_BaseHL2MPCombatWeapon )

BEGIN_NETWORK_TABLE( CBaseHL2MPCombatWeapon , DT_BaseHL2MPCombatWeapon )
#if !defined( CLIENT_DLL )
//	SendPropInt( SENDINFO( m_bReflectViewModelAnimations ), 1, SPROP_UNSIGNED ),
#else
//	RecvPropInt( RECVINFO( m_bReflectViewModelAnimations ) ),
#endif
END_NETWORK_TABLE()


#if !defined( CLIENT_DLL )

#include "globalstate.h"

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBaseHL2MPCombatWeapon )

	DEFINE_FIELD( m_bLowered,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),

END_DATADESC()

#endif

BEGIN_PREDICTION_DATA( CBaseHL2MPCombatWeapon )
END_PREDICTION_DATA()

extern ConVar sk_auto_reload_time;

CBaseHL2MPCombatWeapon::CBaseHL2MPCombatWeapon( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseHL2MPCombatWeapon::ItemHolsterFrame( void )
{
	BaseClass::ItemHolsterFrame();

	// Must be player held
	if ( GetOwner() && GetOwner()->IsPlayer() == false )
		return;

	// We can't be active
	if ( GetOwner()->GetActiveWeapon() == this )
		return;

	// If it's been longer than three seconds, reload
	if ( ( gpGlobals->curtime - m_flHolsterTime ) > sk_auto_reload_time.GetFloat() )
	{
		// Just load the clip with no animations
		FinishReload();
		m_flHolsterTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Drops the weapon into a lowered pose
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHL2MPCombatWeapon::Lower( void )
{
	//Don't bother if we don't have the animation
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Brings the weapon up to the ready position
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHL2MPCombatWeapon::Ready( void )
{
	//Don't bother if we don't have the animation
	if ( SelectWeightedSequence( ACT_VM_LOWERED_TO_IDLE ) == ACTIVITY_NOT_AVAILABLE )
		return false;

	m_bLowered = false;	
	m_flRaiseTime = gpGlobals->curtime + 0.5f;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHL2MPCombatWeapon::Deploy( void )
{
	// If we should be lowered, deploy in the lowered position
	// We have to ask the player if the last time it checked, the weapon was lowered
	if ( GetOwner() && GetOwner()->IsPlayer() )
	{
		CHL2MP_Player *pPlayer = assert_cast<CHL2MP_Player*>( GetOwner() );
		if ( pPlayer->IsWeaponLowered() )
		{
			if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) != ACTIVITY_NOT_AVAILABLE )
			{
				if ( DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_VM_IDLE_LOWERED, (char*)GetAnimPrefix() ) )
				{
					m_bLowered = true;

					// Stomp the next attack time to fix the fact that the lower idles are long
					pPlayer->SetNextAttack( gpGlobals->curtime + 1.0 );
					m_flNextPrimaryAttack = gpGlobals->curtime + 1.0;
					m_flNextSecondaryAttack	= gpGlobals->curtime + 1.0;
					return true;
				}
			}
		}
	}

	m_bLowered = false;
	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHL2MPCombatWeapon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
		SetWeaponVisible( false );
		m_flHolsterTime = gpGlobals->curtime;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CBaseHL2MPCombatWeapon::WeaponShouldBeLowered( void )
{
	// Can't be in the middle of another animation
  	if ( GetIdealActivity() != ACT_VM_IDLE_LOWERED && GetIdealActivity() != ACT_VM_IDLE &&
		 GetIdealActivity() != ACT_VM_IDLE_TO_LOWERED && GetIdealActivity() != ACT_VM_LOWERED_TO_IDLE )
  		return false;

	if ( m_bLowered )
		return true;
	
#if !defined( CLIENT_DLL )

	if ( GlobalEntity_GetState( "friendly_encounter" ) == GLOBAL_ON )
		return true;

#endif

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CBaseHL2MPCombatWeapon::WeaponIdle( void )
{
	//See if we should idle high or low
	if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED 
			 && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( m_flRaiseTime < gpGlobals->curtime && GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}

#if defined( CLIENT_DLL )

#define	HL2_BOB_CYCLE_MIN	1.0f
#define	HL2_BOB_CYCLE_MAX	0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f

extern float	g_lateralBob;
extern float	g_verticalBob;

static ConVar	cl_bobcycle( "cl_bobcycle","0.8" );
static ConVar	cl_bob( "cl_bob","0.002" );
static ConVar	cl_bobup( "cl_bobup","0.5" );

// Register these cvars if needed for easy tweaking
static ConVar	v_iyaw_cycle( "v_iyaw_cycle", "2", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_cycle( "v_iroll_cycle", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_cycle( "v_ipitch_cycle", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iyaw_level( "v_iyaw_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_level( "v_iroll_level", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_level( "v_ipitch_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CBaseHL2MPCombatWeapon::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;
	
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp( speed, -320, 320 );

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
	
	bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX)*HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}
	
	g_verticalBob = speed*0.005f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

	g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX*2)*HL2_BOB_CYCLE_MAX*2;
	cycle /= HL2_BOB_CYCLE_MAX*2;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}

	g_lateralBob = speed*0.005f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
	
	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CBaseHL2MPCombatWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	CalcViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA( origin, g_verticalBob * 0.1f, forward, origin );
	
	// Z bob a bit more
	origin[2] += g_verticalBob * 0.1f;
	
	// bob the angles
	angles[ ROLL ]	+= g_verticalBob * 0.5f;
	angles[ PITCH ]	-= g_verticalBob * 0.4f;

	angles[ YAW ]	-= g_lateralBob  * 0.3f;

	VectorMA( origin, g_lateralBob * 0.8f, right, origin );
}

//-----------------------------------------------------------------------------
Vector CBaseHL2MPCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	return BaseClass::GetBulletSpread( proficiency );
}

//-----------------------------------------------------------------------------
float CBaseHL2MPCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	return BaseClass::GetSpreadBias( proficiency );
}
//-----------------------------------------------------------------------------

const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetProficiencyValues()
{
	return NULL;
}

#else

// Server stubs
float CBaseHL2MPCombatWeapon::CalcViewmodelBob( void )
{
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CBaseHL2MPCombatWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}


//-----------------------------------------------------------------------------
Vector CBaseHL2MPCombatWeapon::GetBulletSpread( WeaponProficiency_t proficiency )
{
	Vector baseSpread = BaseClass::GetBulletSpread( proficiency );

	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	float flModifier = (pProficiencyValues)[ proficiency ].spreadscale;
	return ( baseSpread * flModifier );
}

//-----------------------------------------------------------------------------
float CBaseHL2MPCombatWeapon::GetSpreadBias( WeaponProficiency_t proficiency )
{
	const WeaponProficiencyInfo_t *pProficiencyValues = GetProficiencyValues();
	return (pProficiencyValues)[ proficiency ].bias;
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetProficiencyValues()
{
	return GetDefaultProficiencyValues();
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CBaseHL2MPCombatWeapon::GetDefaultProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}

#endif