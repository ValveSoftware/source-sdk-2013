//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "npc_vehicledriver.h"
#include "vehicle_apc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_APCDRIVER_NO_ROCKET_ATTACK	0x10000
#define SF_APCDRIVER_NO_GUN_ATTACK		0x20000

#define NPC_APCDRIVER_REMEMBER_TIME		4


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_APCDriver : public CNPC_VehicleDriver
{
	DECLARE_CLASS( CNPC_APCDriver, CNPC_VehicleDriver );
public:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	virtual void Spawn( void );
	virtual void Activate( void );
	
	virtual bool FVisible( CBaseEntity *pTarget, int traceMask, CBaseEntity **ppBlocker );
	virtual bool WeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	virtual Class_T Classify ( void ) { return CLASS_COMBINE; }
	virtual void PrescheduleThink( );
	virtual Disposition_t IRelationType(CBaseEntity *pTarget);

	// AI
	virtual int RangeAttack1Conditions( float flDot, float flDist );
	virtual int RangeAttack2Conditions( float flDot, float flDist );

private:
	// Are we being carried by a dropship?
	bool IsBeingCarried();

	// Enable, disable firing
	void InputEnableFiring( inputdata_t &inputdata );
	void InputDisableFiring( inputdata_t &inputdata );

	CHandle<CPropAPC>	m_hAPC;
	float m_flTimeLastSeenEnemy;
	bool m_bFiringDisabled;
};


BEGIN_DATADESC( CNPC_APCDriver )

	//DEFINE_FIELD( m_hAPC, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bFiringDisabled, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeLastSeenEnemy, FIELD_TIME ),

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableFiring", InputEnableFiring ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableFiring", InputDisableFiring ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_apcdriver, CNPC_APCDriver );


//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CNPC_APCDriver::Spawn( void )
{
	BaseClass::Spawn();

	m_flTimeLastSeenEnemy = -NPC_APCDRIVER_REMEMBER_TIME;
	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK2 );
	m_bFiringDisabled = false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_APCDriver::Activate( void )
{
	BaseClass::Activate();

	m_hAPC = dynamic_cast<CPropAPC*>((CBaseEntity*)m_hVehicleEntity);
	if ( !m_hAPC )
	{
		Warning( "npc_apcdriver %s couldn't find his apc named %s.\n", STRING(GetEntityName()), STRING(m_iszVehicleName) );
		UTIL_Remove( this );
		return;
	}
	SetParent( m_hAPC );
	SetAbsOrigin( m_hAPC->WorldSpaceCenter() );
	SetLocalAngles( vec3_angle );

	m_flDistTooFar = m_hAPC->MaxAttackRange();
	SetDistLook( m_hAPC->MaxAttackRange() );
}


//-----------------------------------------------------------------------------
// Enable, disable firing
//-----------------------------------------------------------------------------
void CNPC_APCDriver::InputEnableFiring( inputdata_t &inputdata )
{
	m_bFiringDisabled = false;
}

void CNPC_APCDriver::InputDisableFiring( inputdata_t &inputdata )
{
	m_bFiringDisabled = true;
}

	
//-----------------------------------------------------------------------------
// Purpose: Let's not hate things the APC makes
//-----------------------------------------------------------------------------
Disposition_t CNPC_APCDriver::IRelationType(CBaseEntity *pTarget)
{
	if ( pTarget == m_hAPC || (pTarget->GetOwnerEntity() == m_hAPC) )
		return D_LI;

	return BaseClass::IRelationType(pTarget);
}


//------------------------------------------------------------------------------
// Are we being carried by a dropship?
//------------------------------------------------------------------------------
bool CNPC_APCDriver::IsBeingCarried()
{
	// Inert if we're carried...
	Vector vecVelocity;
	m_hAPC->GetVelocity( &vecVelocity, NULL );
	return ( m_hAPC->GetMoveParent() != NULL ) || (fabs(vecVelocity.z) >= 15);
}



//------------------------------------------------------------------------------
// Is the enemy visible?
//------------------------------------------------------------------------------
bool CNPC_APCDriver::WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions)
{
	if ( m_hAPC->m_lifeState != LIFE_ALIVE )
		return false;

	if ( IsBeingCarried() || m_bFiringDisabled )
		return false;

	float flTargetDist = ownerPos.DistTo( targetPos );
	if (flTargetDist > m_hAPC->MaxAttackRange())
		return false;

	return true;
}

	
//-----------------------------------------------------------------------------
// Is the enemy visible?
//-----------------------------------------------------------------------------
bool CNPC_APCDriver::FVisible( CBaseEntity *pTarget, int traceMask, CBaseEntity **ppBlocker )
{
	if ( m_hAPC->m_lifeState != LIFE_ALIVE )
		return false;

	if ( IsBeingCarried() || m_bFiringDisabled )
		return false;

	float flTargetDist = GetAbsOrigin().DistTo( pTarget->GetAbsOrigin() );
	if (flTargetDist > m_hAPC->MaxAttackRange())
		return false;

	bool bVisible = m_hAPC->FVisible( pTarget, traceMask, ppBlocker );
	if ( bVisible && (pTarget == GetEnemy()) )
	{
		m_flTimeLastSeenEnemy = gpGlobals->curtime;
	}

	if ( pTarget->IsPlayer() || pTarget->Classify() == CLASS_BULLSEYE )
	{
		if (!bVisible)
		{
			if ( ( gpGlobals->curtime - m_flTimeLastSeenEnemy ) <= NPC_APCDRIVER_REMEMBER_TIME )
				return true;
		}
	}

	return bVisible;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CNPC_APCDriver::RangeAttack1Conditions( float flDot, float flDist )
{
	if ( HasSpawnFlags(SF_APCDRIVER_NO_GUN_ATTACK) )
		return COND_NONE;

	if ( m_hAPC->m_lifeState != LIFE_ALIVE )
		return COND_NONE;

	if ( IsBeingCarried() || m_bFiringDisabled )
		return COND_NONE;

	if ( !HasCondition( COND_SEE_ENEMY ) )
		return COND_NONE;

	if ( !m_hAPC->IsInPrimaryFiringCone() )
		return COND_NONE;

	// Vehicle not ready to fire again yet?
	if ( m_pVehicleInterface->Weapon_PrimaryCanFireAt() > gpGlobals->curtime + 0.1f )
		return COND_NONE;

	float flMinDist, flMaxDist;
	m_pVehicleInterface->Weapon_PrimaryRanges( &flMinDist, &flMaxDist );

	if (flDist < flMinDist)
		return COND_NONE;

	if (flDist > flMaxDist)
		return COND_NONE;

	return COND_CAN_RANGE_ATTACK1;
}

int CNPC_APCDriver::RangeAttack2Conditions( float flDot, float flDist )
{
	if ( HasSpawnFlags(SF_APCDRIVER_NO_ROCKET_ATTACK) )
		return COND_NONE;

	if ( m_hAPC->m_lifeState != LIFE_ALIVE )
		return COND_NONE;

	if ( IsBeingCarried() || m_bFiringDisabled )
		return COND_NONE;

	if ( !HasCondition( COND_SEE_ENEMY ) )
		return COND_NONE;

	// Vehicle not ready to fire again yet?
	if ( m_pVehicleInterface->Weapon_SecondaryCanFireAt() > gpGlobals->curtime + 0.1f )
		return COND_NONE;

	float flMinDist, flMaxDist;
	m_pVehicleInterface->Weapon_SecondaryRanges( &flMinDist, &flMaxDist );

	if (flDist < flMinDist)
		return COND_NONE;

	if (flDist > flMaxDist)
		return COND_NONE;

	return COND_CAN_RANGE_ATTACK2;
}


//-----------------------------------------------------------------------------
// Aim the laser dot!
//-----------------------------------------------------------------------------
void CNPC_APCDriver::PrescheduleThink( )
{
	BaseClass::PrescheduleThink();

	if ( m_hAPC->m_lifeState == LIFE_ALIVE )
	{
		if ( GetEnemy() )
		{
			m_hAPC->AimPrimaryWeapon( GetEnemy()->BodyTarget( GetAbsOrigin(), false ) );
		}
		m_hAPC->AimSecondaryWeaponAt( GetEnemy() );
	}
	else if ( m_hAPC->m_lifeState == LIFE_DEAD )
	{
		UTIL_Remove( this );
	}
}


//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_apcdriver, CNPC_APCDriver )
	
AI_END_CUSTOM_NPC()
