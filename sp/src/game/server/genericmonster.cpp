//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//=========================================================
// Generic NPC - purely for scripted sequence work.
//=========================================================
#include "cbase.h"
#include "npcevent.h"
#include "ai_basenpc.h"
#include "ai_hull.h"
#include "KeyValues.h"
#include "engine/IEngineSound.h"
#include "physics_bone_follower.h"
#include "ai_baseactor.h"
#include "ai_senses.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICNPC_NOTSOLID					(1 << 16) 

//=========================================================
// NPC's Anim Events Go Here
//=========================================================

class CGenericNPC : public CAI_BaseNPC
{
public:
	DECLARE_CLASS( CGenericNPC, CAI_BaseNPC );

	void	Spawn( void );
	void	Precache( void );
	float	MaxYawSpeed( void );
	Class_T Classify ( void );
	void	HandleAnimEvent( animevent_t *pEvent );
	int		GetSoundInterests ( void );

	void	TempGunEffect( void );
};

LINK_ENTITY_TO_CLASS( monster_generic, CGenericNPC );

//=========================================================
// Classify - indicates this NPC's place in the 
// relationship table.
//=========================================================
Class_T	CGenericNPC::Classify ( void )
{
	return	CLASS_NONE;
}


//=========================================================
// MaxYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
float CGenericNPC::MaxYawSpeed ( void )
{
	return 90;
}

//---------------------------------------------------------
// !!!TEMP
// !!!TEMP
// !!!TEMP
// !!!TEMP
//
// (sjb)
//---------------------------------------------------------
void CGenericNPC::TempGunEffect( void )
{
	QAngle vecAngle;
	Vector vecDir, vecShot;
	Vector vecMuzzle, vecButt;

	GetAttachment( 2, vecMuzzle, vecAngle );
	GetAttachment( 3, vecButt, vecAngle );

	vecDir = vecMuzzle - vecButt;
	VectorNormalize( vecDir );

	// CPVSFilter filter( GetAbsOrigin() );
	//te->ShowLine( filter, 0.0, vecSpot, vecSpot + vecForward );
	//UTIL_Sparks( vecMuzzle );

	bool fSound = false;
	
	if( random->RandomInt( 0, 3 ) == 0 )
	{
		fSound = true;
	}

	Vector start = vecMuzzle + vecDir * 64;
	Vector end = vecMuzzle + vecDir * 4096;
	UTIL_Tracer( start, end, 0, TRACER_DONT_USE_ATTACHMENT, 5500, fSound );
	CPASAttenuationFilter filter2( this, "GenericNPC.GunSound" );
	EmitSound( filter2, entindex(), "GenericNPC.GunSound" );
}


//=========================================================
// HandleAnimEvent - catches the NPC-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericNPC::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		// TEMPORARLY. Makes the May 2001 sniper demo work (sjb)
		TempGunEffect();
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// GetSoundInterests - generic NPC can't hear.
//=========================================================
int CGenericNPC::GetSoundInterests ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGenericNPC::Spawn()
{
	Precache();

	SetModel( STRING( GetModelName() ) );

/*
	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) )
		UTIL_SetSize(this, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING( GetModelName() ), "models/player.mdl" ) || FStrEq( STRING( GetModelName() ), "models/holo.mdl" ) )
		UTIL_SetSize(this, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(this, NAI_Hull::Mins(HULL_HUMAN), NAI_Hull::Maxs(HULL_HUMAN));

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	m_bloodColor		= BLOOD_COLOR_RED;
	m_iHealth			= 8;
	m_flFieldOfView		= 0.5;// indicates the width of this NPC's forward view cone ( as a dotproduct result )
	m_NPCState			= NPC_STATE_NONE;
	
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_OPEN_DOORS );

	NPCInit();
	if ( !HasSpawnFlags(SF_GENERICNPC_NOTSOLID) )
	{
		trace_t tr;
		UTIL_TraceEntity( this, GetAbsOrigin(), GetAbsOrigin(), MASK_SOLID, &tr );
		if ( tr.startsolid )
		{
			Msg("Placed npc_generic in solid!!! (%s)\n", STRING(GetModelName()) );
			m_spawnflags |= SF_GENERICNPC_NOTSOLID;
		}
	}

	if ( HasSpawnFlags(SF_GENERICNPC_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		m_takedamage = DAMAGE_NO;
		VPhysicsDestroyObject();
	}
}

//-----------------------------------------------------------------------------
// Purpose: precaches all resources this NPC needs
//-----------------------------------------------------------------------------
void CGenericNPC::Precache()
{
	BaseClass::Precache();

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( "GenericNPC.GunSound" );
}	

// a really large health is set to make sure these never die.
const int TOO_MUCH_HEALTH_TO_DIE = 1000;
//=======================================================================================
// Furniture: A dumb "NPC" that is uses in scripted sequences
//			  where an NPC needs to be frame locked with a prop.
//=======================================================================================
class CNPC_Furniture : public CAI_BaseActor
{
	DECLARE_CLASS( CNPC_Furniture, CAI_BaseActor );
	DECLARE_DATADESC();
public:
	void	Spawn( void );
	void	Precache( void );
	void	Die( void );
	void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	Class_T	Classify ( void );
	float	MaxYawSpeed( void ){ return 0; }
	virtual int	ObjectCaps( void );
	bool	CreateVPhysics( void );
	void	NPCThink( void );
	void	UpdateOnRemove( void );
	int		SelectSchedule( void );
	void	OnRestore( void );
	int		OnTakeDamage( const CTakeDamageInfo &info )
	{
		if ( m_iHealth <= info.GetDamage() )
			m_iHealth = info.GetDamage() + TOO_MUCH_HEALTH_TO_DIE;
		return BaseClass::OnTakeDamage(info);
	}

	void DrawDebugGeometryOverlays(void);

	void SetPlayerAvoidState( void );
	void InputDisablePlayerCollision( inputdata_t &inputdata );
	void InputEnablePlayerCollision( inputdata_t &inputdata );
	void UpdateBoneFollowerState( void );

private:
	// Contained Bone Follower manager
	CBoneFollowerManager	m_BoneFollowerManager;
};

LINK_ENTITY_TO_CLASS( monster_furniture, CNPC_Furniture );
LINK_ENTITY_TO_CLASS( npc_furniture, CNPC_Furniture );

//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CNPC_Furniture )
	DEFINE_EMBEDDED( m_BoneFollowerManager ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisablePlayerCollision", InputDisablePlayerCollision ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnablePlayerCollision", InputEnablePlayerCollision ),
	
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: This used to have something to do with bees flying, but 
//			now it only initializes moving furniture in scripted sequences
//-----------------------------------------------------------------------------
void CNPC_Furniture::Spawn( )
{
	Precache();
	
	SetModel( STRING(GetModelName()) );

	SetMoveType( MOVETYPE_STEP );
	SetSolid( SOLID_BBOX );

	// Our collision, if needed, will be done through bone followers
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetBloodColor( DONT_BLEED );
	m_iHealth = TOO_MUCH_HEALTH_TO_DIE; //wow
	m_takedamage = DAMAGE_AIM;
	SetSequence( 0 );
	SetCycle( 0 );
	SetNavType( NAV_FLY );
	AddFlag( FL_FLY );

	CapabilitiesAdd( bits_CAP_MOVE_FLY | bits_CAP_TURN_HEAD | bits_CAP_ANIMATEDFACE );

	AddEFlags( EFL_NO_MEGAPHYSCANNON_RAGDOLL );

//	pev->nextthink += 1.0;
//	SetThink (WalkMonsterDelay);

	ResetSequenceInfo( );
	SetCycle( 0 );
	NPCInit();

	// Furniture needs to block LOS
	SetBlocksLOS( true );

	// Furniture just wastes CPU doing sensing code, since all they do is idle and play scripts
	GetSenses()->AddSensingFlags( SENSING_FLAGS_DONT_LOOK | SENSING_FLAGS_DONT_LISTEN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Furniture::Precache( void )
{
	PrecacheModel( STRING( GetModelName() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CNPC_Furniture::ObjectCaps( void ) 
{ 
	// HL2 furniture transitions
#ifdef HL2_DLL
	return CAI_BaseNPC::ObjectCaps(); 
#else
	return (CAI_BaseNPC::ObjectCaps() & ~FCAP_ACROSS_TRANSITION); 
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Furniture is killed
//-----------------------------------------------------------------------------
void CNPC_Furniture::Die( void )
{
	SetThink ( &CNPC_Furniture::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: ID's Furniture as neutral (noone will attack it)
//-----------------------------------------------------------------------------
Class_T CNPC_Furniture::Classify ( void )
{
	return CLASS_NONE;
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CNPC_Furniture::CreateVPhysics( void )
{
#ifndef HL2_DLL
	return false;
#endif

	if ( !m_BoneFollowerManager.GetNumBoneFollowers() )
	{
		KeyValues *modelKeyValues = new KeyValues("");
		if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
		{
			// Do we have a bone follower section?
			KeyValues *pkvBoneFollowers = modelKeyValues->FindKey("bone_followers");
			if ( pkvBoneFollowers )
			{
				// Loop through the list and create the bone followers
				KeyValues *pBone = pkvBoneFollowers->GetFirstSubKey();
				while ( pBone )
				{
					// Add it to the list
					const char *pBoneName = pBone->GetString();
					m_BoneFollowerManager.AddBoneFollower( this, pBoneName );

					pBone = pBone->GetNextKey();
				}
			}
		}
		modelKeyValues->deleteThis();
	}

	return true;
}

void CNPC_Furniture::InputDisablePlayerCollision( inputdata_t &inputdata )
{
	SetCollisionGroup( COLLISION_GROUP_NPC_ACTOR );
	UpdateBoneFollowerState();
}

void CNPC_Furniture::InputEnablePlayerCollision( inputdata_t &inputdata )
{
	SetCollisionGroup( COLLISION_GROUP_NPC );
	UpdateBoneFollowerState();
}

void CNPC_Furniture::UpdateBoneFollowerState( void )
{
	if ( m_BoneFollowerManager.GetNumBoneFollowers() )
	{
		physfollower_t* pBone = m_BoneFollowerManager.GetBoneFollower( 0 );

		if ( pBone && pBone->hFollower && pBone->hFollower->GetCollisionGroup() != GetCollisionGroup() )
		{
			for ( int i = 0; i < m_BoneFollowerManager.GetNumBoneFollowers(); i++ )
			{
				pBone = m_BoneFollowerManager.GetBoneFollower( i );

				if ( pBone && pBone->hFollower )
				{
					pBone->hFollower->SetCollisionGroup( GetCollisionGroup() );
				}
			}
		}
	}
}

void CNPC_Furniture::SetPlayerAvoidState( void )
{

}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Furniture::NPCThink( void )
{
	BaseClass::NPCThink();
	
	// Update follower bones
	m_BoneFollowerManager.UpdateBoneFollowers(this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Furniture::UpdateOnRemove( void )
{
	m_BoneFollowerManager.DestroyBoneFollowers();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CNPC_Furniture::SelectSchedule( void )
{
	switch( m_NPCState )
	{
	case NPC_STATE_NONE:
	case NPC_STATE_PRONE:
	case NPC_STATE_IDLE:
	case NPC_STATE_ALERT:
	case NPC_STATE_COMBAT:
	case NPC_STATE_DEAD:
		return SCHED_WAIT_FOR_SCRIPT;

	case NPC_STATE_SCRIPT:
		return BaseClass::SelectSchedule();

	default:
		DevWarning( 2, "Invalid State for SelectSchedule!\n" );
		break;
	}

	return SCHED_FAIL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Furniture::OnRestore( void )
{
	// Recreate any bone followers we have
	CreateVPhysics();

	BaseClass::OnRestore();
}
void CNPC_Furniture::DrawDebugGeometryOverlays( void )
{
	//ugh
	if ( m_debugOverlays & OVERLAY_NPC_ZAP_BIT )
	{
		m_debugOverlays &= ~OVERLAY_NPC_ZAP_BIT;
	}

	BaseClass::DrawDebugGeometryOverlays();
}
