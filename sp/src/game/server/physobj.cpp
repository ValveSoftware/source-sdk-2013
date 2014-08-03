//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "player.h"
#include "vphysics_interface.h"
#include "physics.h"
#include "vcollide_parse.h"
#include "entitylist.h"
#include "physobj.h"
#include "hierarchy.h"
#include "game.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include "model_types.h"
#include "props.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "vphysics/constraints.h"
#include "collisionutils.h"
#include "decals.h"
#include "bone_setup.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar debug_physimpact("debug_physimpact", "0" );

const char *GetMassEquivalent(float flMass);

// This is a physically simulated spring, used to join objects together and create spring forces
//
// NOTE: Springs are not physical in the sense that they only create force, they do not collide with
// anything or have any REAL constraints.  They can be stretched infinitely (though this will create
// and infinite force), they can penetrate any other object (or spring). They do not occupy any space.
// 

#define SF_SPRING_ONLYSTRETCH		0x0001

class CPhysicsSpring : public CBaseEntity
{
	DECLARE_CLASS( CPhysicsSpring, CBaseEntity );
public:
	CPhysicsSpring();
	~CPhysicsSpring();

	void	Spawn( void );
	void	Activate( void );

	// Inputs
	void InputSetSpringConstant( inputdata_t &inputdata );
	void InputSetSpringDamping( inputdata_t &inputdata );
	void InputSetSpringLength( inputdata_t &inputdata );

	// Debug
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	void GetSpringObjectConnections( string_t nameStart, string_t nameEnd, IPhysicsObject **pStart, IPhysicsObject **pEnd );
	void NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params );
	IPhysicsObject *GetStartObject() { return m_pSpring ? m_pSpring->GetStartObject() : NULL; }
	IPhysicsObject *GetEndObject() { return m_pSpring ? m_pSpring->GetEndObject() : NULL; }

	DECLARE_DATADESC();

private:	
	IPhysicsSpring		*m_pSpring;
	bool			m_isLocal;

	// These are "template" values used to construct the spring.  After creation, they are not needed
	float			m_tempConstant;
	float			m_tempLength;	// This is the "ideal" length of the spring, not the length it is currently stretched to.
	float			m_tempDamping;
	float			m_tempRelativeDamping;

	string_t		m_nameAttachStart;
	string_t		m_nameAttachEnd;
	Vector			m_start;
	Vector			m_end;
	unsigned int 	m_teleportTick;
};

LINK_ENTITY_TO_CLASS( phys_spring, CPhysicsSpring );

BEGIN_DATADESC( CPhysicsSpring )

	DEFINE_PHYSPTR( m_pSpring ),

	DEFINE_KEYFIELD( m_tempConstant, FIELD_FLOAT, "constant" ),
	DEFINE_KEYFIELD( m_tempLength, FIELD_FLOAT, "length" ),
	DEFINE_KEYFIELD( m_tempDamping, FIELD_FLOAT, "damping" ),
	DEFINE_KEYFIELD( m_tempRelativeDamping, FIELD_FLOAT, "relativedamping" ),

	DEFINE_KEYFIELD( m_nameAttachStart, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( m_nameAttachEnd, FIELD_STRING, "attach2" ),

	DEFINE_FIELD( m_start, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( m_end, FIELD_POSITION_VECTOR, "springaxis" ),
	DEFINE_FIELD( m_isLocal, FIELD_BOOLEAN ),

	// Not necessary to save... it's only there to make sure 
//	DEFINE_FIELD( m_teleportTick, FIELD_INTEGER ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpringConstant", InputSetSpringConstant ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpringLength", InputSetSpringLength ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpringDamping", InputSetSpringDamping ),

END_DATADESC()

// debug function - slow, uses dynamic_cast<> - use this to query the attached objects
// physics_debug_entity toggles the constraint system for an object using this
bool GetSpringAttachments( CBaseEntity *pEntity, CBaseEntity *pAttachOut[2], IPhysicsObject *pAttachVPhysics[2] )
{
	CPhysicsSpring *pSpringEntity = dynamic_cast<CPhysicsSpring *>(pEntity);
	if ( pSpringEntity )
	{
		IPhysicsObject *pRef = pSpringEntity->GetStartObject();
		pAttachOut[0] = pRef ? static_cast<CBaseEntity *>(pRef->GetGameData()) : NULL;
		pAttachVPhysics[0] = pRef;
		IPhysicsObject *pAttach = pSpringEntity->GetEndObject();
		pAttachOut[1] = pAttach ? static_cast<CBaseEntity *>(pAttach->GetGameData()) : NULL;
		pAttachVPhysics[1] = pAttach;
		return true;
	}
	return false;
}


CPhysicsSpring::CPhysicsSpring( void )
{
#ifdef _DEBUG
	m_start.Init();
	m_end.Init();
#endif
	m_pSpring = NULL;
	m_tempConstant = 150;
	m_tempLength = 0;
	m_tempDamping = 2.0;
	m_tempRelativeDamping = 0.01;
	m_isLocal = false;
	m_teleportTick = 0xFFFFFFFF;
}

CPhysicsSpring::~CPhysicsSpring( void )
{
	if ( m_pSpring )
	{
		physenv->DestroySpring( m_pSpring );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringConstant( inputdata_t &inputdata )
{
	m_tempConstant = inputdata.value.Float();
	m_pSpring->SetSpringConstant(inputdata.value.Float());
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringDamping( inputdata_t &inputdata )
{
	m_tempDamping = inputdata.value.Float();
	m_pSpring->SetSpringDamping(inputdata.value.Float());
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPhysicsSpring::InputSetSpringLength( inputdata_t &inputdata )
{
	m_tempLength = inputdata.value.Float();
	m_pSpring->SetSpringLength(inputdata.value.Float());
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysicsSpring::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Constant: %3.2f",m_tempConstant);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Length: %3.2f",m_tempLength);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf(tempstr,sizeof(tempstr),"Damping: %3.2f",m_tempDamping);
		EntityText(text_offset,tempstr,0);
		text_offset++;

	}
	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Override base class to add display of fly direction
// Input  :
// Output : 
//-----------------------------------------------------------------------------
void CPhysicsSpring::DrawDebugGeometryOverlays(void) 
{
	if ( !m_pSpring )
		return;

	// ------------------------------
	// Draw if BBOX is on
	// ------------------------------
	if (m_debugOverlays & OVERLAY_BBOX_BIT)
	{
		Vector vStartPos;
		Vector vEndPos;
		m_pSpring->GetEndpoints( &vStartPos, &vEndPos );

		Vector vSpringDir = vEndPos - vStartPos;
		VectorNormalize(vSpringDir);

		Vector vLength = vStartPos + (vSpringDir*m_tempLength);

		NDebugOverlay::Line(vStartPos, vLength, 0,0,255, false, 0);
		NDebugOverlay::Line(vLength, vEndPos, 255,0,0, false, 0);
	}
	BaseClass::DrawDebugGeometryOverlays();
}

bool PointIsNearer( IPhysicsObject *pObject1, const Vector &point1, const Vector &point2 )
{
	Vector center;
	
	pObject1->GetPosition( &center, 0 );

	float dist1 = (center - point1).LengthSqr();
	float dist2 = (center - point2).LengthSqr();

	if ( dist1 < dist2 )
		return true;

	return false;
}

void CPhysicsSpring::GetSpringObjectConnections( string_t nameStart, string_t nameEnd, IPhysicsObject **pStart, IPhysicsObject **pEnd )
{
	IPhysicsObject *pStartObject = FindPhysicsObjectByName( STRING(nameStart), this );
	IPhysicsObject *pEndObject = FindPhysicsObjectByName( STRING(nameEnd), this );

	// Assume the world for missing objects
	if ( !pStartObject )
	{
		pStartObject = g_PhysWorldObject;
	}
	else if ( !pEndObject )
	{
		// try to sort so that the world is always the start object
		pEndObject = pStartObject;
		pStartObject = g_PhysWorldObject;
	}
	else
	{
		CBaseEntity *pEntity0 = (CBaseEntity *) (pStartObject->GetGameData());
		if ( pEntity0 )
		{
			g_pNotify->AddEntity( this, pEntity0 );
		}

		CBaseEntity *pEntity1 = (CBaseEntity *) pEndObject->GetGameData();
		if ( pEntity1 )
		{
			g_pNotify->AddEntity( this, pEntity1 );
		}
	}

	*pStart = pStartObject;
	*pEnd = pEndObject;
}


void CPhysicsSpring::Activate( void )
{
	BaseClass::Activate();

	// UNDONE: save/restore all data, and only create the spring here

	if ( !m_pSpring )
	{
		IPhysicsObject *pStart, *pEnd;

		GetSpringObjectConnections( m_nameAttachStart, m_nameAttachEnd, &pStart, &pEnd );

		// Needs to connect to real, different objects
		if ( (!pStart || !pEnd) || (pStart == pEnd) )
		{
			DevMsg("ERROR: Can't init spring %s from \"%s\" to \"%s\"\n", GetDebugName(), STRING(m_nameAttachStart), STRING(m_nameAttachEnd) );
			UTIL_Remove( this );
			return;
		}

		// if m_end is not closer to pEnd than m_start, swap
		if ( !PointIsNearer( pEnd, m_end, m_start ) )
		{
			Vector tmpVec = m_start;
			m_start = m_end;
			m_end = tmpVec;
		}

		// create the spring
		springparams_t spring;
		spring.constant = m_tempConstant;
		spring.damping = m_tempDamping;
		spring.naturalLength = m_tempLength;
		spring.relativeDamping = m_tempRelativeDamping;
		spring.startPosition = m_start;
		spring.endPosition = m_end;
		spring.useLocalPositions = false;
		spring.onlyStretch = HasSpawnFlags( SF_SPRING_ONLYSTRETCH );
		m_pSpring = physenv->CreateSpring( pStart, pEnd, &spring );
	}
}


void CPhysicsSpring::Spawn( void )
{
	SetSolid( SOLID_NONE );
	m_start = GetAbsOrigin();
	if ( m_tempLength <= 0 )
	{
		m_tempLength = (m_end - m_start).Length();
	}
}

void CPhysicsSpring::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// don't recurse
	if ( eventType != NOTIFY_EVENT_TELEPORT || (unsigned int)gpGlobals->tickcount == m_teleportTick )
		return;

	m_teleportTick = gpGlobals->tickcount;
	PhysTeleportConstrainedEntity( pNotify, m_pSpring->GetStartObject(), m_pSpring->GetEndObject(), params.pTeleport->prevOrigin, params.pTeleport->prevAngles, params.pTeleport->physicsRotate );
}


// ---------------------------------------------------------------------
//
// CPhysBox -- physically simulated brush
//
// ---------------------------------------------------------------------

// SendTable stuff.
IMPLEMENT_SERVERCLASS_ST(CPhysBox, DT_PhysBox)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( func_physbox, CPhysBox );

BEGIN_DATADESC( CPhysBox )

	DEFINE_FIELD( m_hCarryingPlayer, FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_massScale, FIELD_FLOAT, "massScale" ),
	DEFINE_KEYFIELD( m_damageType, FIELD_INTEGER, "Damagetype" ),
	DEFINE_KEYFIELD( m_iszOverrideScript, FIELD_STRING, "overridescript" ),
	DEFINE_KEYFIELD( m_damageToEnableMotion, FIELD_INTEGER, "damagetoenablemotion" ),
	DEFINE_KEYFIELD( m_flForceToEnableMotion, FIELD_FLOAT, "forcetoenablemotion" ), 
	DEFINE_KEYFIELD( m_angPreferredCarryAngles, FIELD_VECTOR, "preferredcarryangles" ),
	DEFINE_KEYFIELD( m_bNotSolidToWorld, FIELD_BOOLEAN, "notsolid" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Wake", InputWake ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Sleep", InputSleep ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMotion", InputEnableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMotion", InputDisableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableFloating", InputDisableFloating ),

	// Function pointers
	DEFINE_ENTITYFUNC( BreakTouch ),

	// Outputs
	DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
	DEFINE_OUTPUT( m_OnAwakened, "OnAwakened" ),
	DEFINE_OUTPUT( m_OnMotionEnabled, "OnMotionEnabled" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunPunt, "OnPhysGunPunt" ),
	DEFINE_OUTPUT( m_OnPhysGunOnlyPickup, "OnPhysGunOnlyPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),

END_DATADESC()

// UNDONE: Save/Restore needs to take the physics object's properties into account
// UNDONE: Acceleration, velocity, angular velocity, etc. must be preserved
// UNDONE: Many of these quantities are relative to a coordinate frame
// UNDONE: Translate when going across transitions?
// UNDONE: Build transition transformation, and transform data in save/restore for IPhysicsObject
// UNDONE: Angles are saved in the entity, but not propagated back to the IPhysicsObject on restore

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::Spawn( void )
{
	// Initialize damage modifiers. Must be done before baseclass spawn.
	m_flDmgModBullet = func_breakdmg_bullet.GetFloat();
	m_flDmgModClub = func_breakdmg_club.GetFloat();
	m_flDmgModExplosive = func_breakdmg_explosive.GetFloat();

	ParsePropData();

	Precache();

	m_iMaxHealth = ( m_iHealth > 0 ) ? m_iHealth : 1;

	if ( HasSpawnFlags( SF_BREAK_TRIGGER_ONLY ) )
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
		AddSpawnFlags( SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE );
	}
	else if ( m_iHealth == 0 )
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
		AddSpawnFlags( SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE );
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}
  
	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( vec3_origin );
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_VPHYSICS );
	if ( HasSpawnFlags( SF_PHYSBOX_DEBRIS ) )
	{
		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}

	if ( HasSpawnFlags( SF_PHYSBOX_NO_ROTORWASH_PUSH ) )
	{
		AddEFlags( EFL_NO_ROTORWASH_PUSH );
	}

	if ( m_bNotSolidToWorld )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}
	CreateVPhysics();

	m_hCarryingPlayer = NULL;

	SetTouch( &CPhysBox::BreakTouch );
	if ( HasSpawnFlags( SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
	{
		SetTouch( NULL );
	}

	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 1.0;
	}
}

// shared from studiomdl, checks for long, thin objects and adds some damping 
// to prevent endless rolling due to low inertia
static bool ShouldDampRotation( const CPhysCollide *pCollide )
{
	Vector mins, maxs;
	physcollision->CollideGetAABB( &mins, &maxs, pCollide, vec3_origin, vec3_angle );
	Vector size = maxs-mins;
	int largest = 0;
	float largeSize = size[0];
	for ( int i = 1; i < 3; i++ )
	{
		if ( size[i] > largeSize )
		{
			largeSize = size[i];
			largest = i;
		}
	}
	size[largest] = 0;
	float len = size.Length();
	if ( len > 0 )
	{
		float sizeRatio = largeSize / len;
		// HACKHACK: Hardcoded size ratio to induce damping
		// This prevents long skinny objects from rolling endlessly
		if ( sizeRatio > 9 )
			return true;
	}
	return false;
}


bool CPhysBox::CreateVPhysics()
{
	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	if ( m_massScale > 0 )
	{
		tmpSolid.params.mass *= m_massScale;
	}

	vcollide_t *pVCollide = modelinfo->GetVCollide( GetModelIndex() );
	PhysGetMassCenterOverride( this, pVCollide, tmpSolid );
	PhysSolidOverride( tmpSolid, m_iszOverrideScript );
	if ( tmpSolid.params.rotdamping < 1.0f && ShouldDampRotation(pVCollide->solids[0]) )
	{
		tmpSolid.params.rotdamping = 1.0f;
	}
	IPhysicsObject *pPhysics = VPhysicsInitNormal( GetSolid(), GetSolidFlags(), true, &tmpSolid );

	if ( m_damageType == 1 )
	{
		PhysSetGameFlags( pPhysics, FVPHYSICS_DMG_SLICE );
	}

	// Wake it up if not asleep
	if ( !HasSpawnFlags(SF_PHYSBOX_ASLEEP) )
	{
		pPhysics->Wake();
	}

	if ( HasSpawnFlags(SF_PHYSBOX_MOTIONDISABLED) || m_damageToEnableMotion > 0 || m_flForceToEnableMotion > 0 )
	{
		pPhysics->EnableMotion( false );
	}

	return true;
}

       
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysBox::ObjectCaps() 
{ 
	int caps = BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION;
	if ( HasSpawnFlags( SF_PHYSBOX_ENABLE_PICKUP_OUTPUT ) )
	{
		caps |= FCAP_IMPULSE_USE;
	}
	else if ( !HasSpawnFlags( SF_PHYSBOX_IGNOREUSE ) )
	{
		if ( CBasePlayer::CanPickupObject( this, 35, 128 ) )
		{
			caps |= FCAP_IMPULSE_USE;
		}
	}

	return caps;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( pPlayer )
	{
		if ( HasSpawnFlags( SF_PHYSBOX_ENABLE_PICKUP_OUTPUT ) )
		{
			m_OnPlayerUse.FireOutput( this, this );
		}

		if ( !HasSpawnFlags( SF_PHYSBOX_IGNOREUSE ) )
		{
			pPlayer->PickupObject( this );
		}
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CPhysBox::CanBePickedUpByPhyscannon()
{
	if ( HasSpawnFlags( SF_PHYSBOX_NEVER_PICK_UP ) )
		return false;

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( !pPhysicsObject )
		return false;
		
	if ( !pPhysicsObject->IsMotionEnabled() && !HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON ) )
		return false;		

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysBox::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr),"Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			EntityText( text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that breaks the physics object away from its parent
//			and starts it simulating.
//-----------------------------------------------------------------------------
void CPhysBox::InputWake( inputdata_t &inputdata )
{
	VPhysicsGetObject()->Wake();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler that breaks the physics object away from its parent
//			and stops it simulating.
//-----------------------------------------------------------------------------
void CPhysBox::InputSleep( inputdata_t &inputdata )
{
	VPhysicsGetObject()->Sleep();
}

//-----------------------------------------------------------------------------
// Purpose: Enable physics motion and collision response (on by default)
//-----------------------------------------------------------------------------
void CPhysBox::InputEnableMotion( inputdata_t &inputdata )
{
	EnableMotion();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::EnableMotion( void )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->Wake();
	}

	m_damageToEnableMotion = 0;
	m_flForceToEnableMotion = 0;

	m_OnMotionEnabled.FireOutput( this, this, 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Disable any physics motion or collision response
//-----------------------------------------------------------------------------
void CPhysBox::InputDisableMotion( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( false );
	}
}

// Turn off floating simulation (and cost)
void CPhysBox::InputDisableFloating( inputdata_t &inputdata )
{
	PhysEnableFloating( VPhysicsGetObject(), false );
}

//-----------------------------------------------------------------------------
// Purpose: If we're being held by the player's hand/physgun, force it to drop us
//-----------------------------------------------------------------------------
void CPhysBox::InputForceDrop( inputdata_t &inputdata )
{
	if ( m_hCarryingPlayer )
	{
		m_hCarryingPlayer->ForceDropOfCarriedPhysObjects();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::Move( const Vector &direction )
{
	VPhysicsGetObject()->ApplyForceCenter( direction );
}

// Update the visible representation of the physic system's representation of this object
void CPhysBox::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	// if this is the first time we have moved, fire our target
	if ( HasSpawnFlags( SF_PHYSBOX_ASLEEP ) )
	{
		if ( !pPhysics->IsAsleep() )
		{
			m_OnAwakened.FireOutput(this, this);
			FireTargets( STRING(m_target), this, this, USE_TOGGLE, 0 );
			RemoveSpawnFlags( SF_PHYSBOX_ASLEEP );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	if ( reason == PUNTED_BY_CANNON )
	{
		m_OnPhysGunPunt.FireOutput( pPhysGunUser, this );
	}

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject && !pPhysicsObject->IsMoveable() )
	{
		if ( !HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON ) )
			return;
		EnableMotion();
	}

	m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );

	// Are we just being punted?
	if ( reason == PUNTED_BY_CANNON )
		return;

	if( reason == PICKED_UP_BY_CANNON )
	{
		m_OnPhysGunOnlyPickup.FireOutput( pPhysGunUser, this );
	}

	m_hCarryingPlayer = pPhysGunUser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	BaseClass::OnPhysGunDrop( pPhysGunUser, Reason );

	m_hCarryingPlayer = NULL;
	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysBox::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	IPhysicsObject *pPhysObj = pEvent->pObjects[!index];

	// If we have a force to enable motion, and we're still disabled, check to see if this should enable us
	if ( m_flForceToEnableMotion )
	{
		CBaseEntity *pOther = static_cast<CBaseEntity *>(pPhysObj->GetGameData());

		// Don't allow the player to bump an object active if we've requested not to
		if ( ( pOther && pOther->IsPlayer() && HasSpawnFlags( SF_PHYSBOX_PREVENT_PLAYER_TOUCH_ENABLE ) ) == false )
		{
			// Large enough to enable motion?
			float flForce = pEvent->collisionSpeed * pEvent->pObjects[!index]->GetMass();
			if ( flForce >= m_flForceToEnableMotion )
			{
				EnableMotion();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysBox::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( IsMarkedForDeletion() )
		return 0;

	// note: if motion is disabled, OnTakeDamage can't apply physics force
	int ret = BaseClass::OnTakeDamage( info );

	if ( info.GetInflictor() )
	{
		m_OnDamaged.FireOutput( info.GetAttacker(), this );
	}

	// Have we been broken? If so, abort
	if ( GetHealth() <= 0 )
		return ret;

	// If we have a force to enable motion, and we're still disabled, check to see if this should enable us
	if ( m_flForceToEnableMotion )
	{
		// Large enough to enable motion?
		float flForce = info.GetDamageForce().Length();
		if ( flForce >= m_flForceToEnableMotion )
		{
			IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				EnableMotion();
			}
		}
	}

	// Check our health against the threshold:
	if( m_damageToEnableMotion > 0 && GetHealth() < m_damageToEnableMotion )
	{
		EnableMotion();
		VPhysicsTakeDamage( info );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this physbox has preferred carry angles
//-----------------------------------------------------------------------------
bool CPhysBox::HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer )
{
	return HasSpawnFlags( SF_PHYSBOX_USEPREFERRED );
}


// ---------------------------------------------------------------------
//
// CPhysExplosion -- physically simulated explosion
//
// ---------------------------------------------------------------------
#define SF_PHYSEXPLOSION_NODAMAGE			0x0001
#define SF_PHYSEXPLOSION_PUSH_PLAYER		0x0002
#define SF_PHYSEXPLOSION_RADIAL				0x0004
#define	SF_PHYSEXPLOSION_TEST_LOS			0x0008
#define SF_PHYSEXPLOSION_DISORIENT_PLAYER	0x0010

LINK_ENTITY_TO_CLASS( env_physexplosion, CPhysExplosion );

BEGIN_DATADESC( CPhysExplosion )

	DEFINE_KEYFIELD( m_damage, FIELD_FLOAT, "magnitude" ),
	DEFINE_KEYFIELD( m_radius, FIELD_FLOAT, "radius" ),
	DEFINE_KEYFIELD( m_targetEntityName, FIELD_STRING, "targetentityname" ),
	DEFINE_KEYFIELD( m_flInnerRadius, FIELD_FLOAT, "inner_radius" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode ),

	// Outputs 
	DEFINE_OUTPUT( m_OnPushedPlayer, "OnPushedPlayer" ),

END_DATADESC()


void CPhysExplosion::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetModelName( NULL_STRING );
}

float CPhysExplosion::GetRadius( void )
{
	float radius = m_radius;
	if ( radius <= 0 )
	{
		// Use the same radius as combat
		radius = m_damage * 2.5;
	}

	return radius;
}

CBaseEntity *CPhysExplosion::FindEntity( CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	// Filter by name or classname
	if ( m_targetEntityName != NULL_STRING )
	{
		// Try an explicit name first
		CBaseEntity *pTarget = gEntList.FindEntityByName( pEntity, m_targetEntityName, NULL, pActivator, pCaller );
		if ( pTarget != NULL )
			return pTarget;

		// Failing that, try a classname
		return gEntList.FindEntityByClassnameWithin( pEntity, STRING(m_targetEntityName), GetAbsOrigin(), GetRadius() );
	}

	// Just find anything in the radius
	return gEntList.FindEntityInSphere( pEntity, GetAbsOrigin(), GetRadius() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysExplosion::InputExplode( inputdata_t &inputdata )
{
	Explode( inputdata.pActivator, inputdata.pCaller );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysExplosion::Explode( CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *pEntity = NULL;
	float		adjustedDamage, falloff, flDist;
	Vector		vecSpot, vecOrigin;

	falloff = 1.0 / 2.5;

	// iterate on all entities in the vicinity.
	// I've removed the traceline heuristic from phys explosions. SO right now they will
	// affect entities through walls. (sjb)
	// UNDONE: Try tracing world-only?
	while ((pEntity = FindEntity( pEntity, pActivator, pCaller )) != NULL)
	{
		// UNDONE: Ask the object if it should get force if it's not MOVETYPE_VPHYSICS?
		if ( pEntity->m_takedamage != DAMAGE_NO && (pEntity->GetMoveType() == MOVETYPE_VPHYSICS || (pEntity->VPhysicsGetObject() /*&& !pEntity->IsPlayer()*/)) )
		{
			vecOrigin = GetAbsOrigin();
			
			vecSpot = pEntity->BodyTarget( vecOrigin );
			// Squash this down to a circle
			if ( HasSpawnFlags( SF_PHYSEXPLOSION_RADIAL ) )
			{
				vecOrigin[2] = vecSpot[2];
			}
			
			// decrease damage for an ent that's farther from the bomb.
			flDist = ( vecOrigin - vecSpot ).Length();

			if( m_radius == 0 || flDist <= m_radius )
			{
				if ( HasSpawnFlags( SF_PHYSEXPLOSION_TEST_LOS ) )
				{
					Vector vecStartPos = GetAbsOrigin();
					Vector vecEndPos = pEntity->BodyTarget( vecStartPos, false );

					if ( m_flInnerRadius != 0.0f )
					{
						// Find a point on our inner radius sphere to begin from
						Vector vecDirToTarget = ( vecEndPos - vecStartPos );
						VectorNormalize( vecDirToTarget );
						vecStartPos = GetAbsOrigin() + ( vecDirToTarget * m_flInnerRadius );
					}

					trace_t tr;
					UTIL_TraceLine( vecStartPos, 
						pEntity->BodyTarget( vecStartPos, false ), 
						MASK_SOLID_BRUSHONLY, 
						this, 
						COLLISION_GROUP_NONE, 
						&tr );

					// Shielded
					if ( tr.fraction < 1.0f && tr.m_pEnt != pEntity )
						continue;
				}

				adjustedDamage =  flDist * falloff;
				adjustedDamage = m_damage - adjustedDamage;
		
				if ( adjustedDamage < 1 )
				{
					adjustedDamage = 1;
				}

				CTakeDamageInfo info( this, this, adjustedDamage, DMG_BLAST );
				CalculateExplosiveDamageForce( &info, (vecSpot - vecOrigin), vecOrigin );
	
				if ( HasSpawnFlags( SF_PHYSEXPLOSION_PUSH_PLAYER ) )
				{
					if ( pEntity->IsPlayer() )
					{
						Vector vecPushDir = ( pEntity->BodyTarget( GetAbsOrigin(), false ) - GetAbsOrigin() );
						float dist = VectorNormalize( vecPushDir );

						float flFalloff = RemapValClamped( dist, m_radius, m_radius*0.75f, 0.0f, 1.0f );

						if ( HasSpawnFlags( SF_PHYSEXPLOSION_DISORIENT_PLAYER ) )
						{
							//Disorient the player
							QAngle vecDeltaAngles;

							vecDeltaAngles.x = random->RandomInt( -30, 30 );
							vecDeltaAngles.y = random->RandomInt( -30, 30 );
							vecDeltaAngles.z = 0.0f;

							CBasePlayer *pPlayer = ToBasePlayer( pEntity );
							pPlayer->SnapEyeAngles( GetLocalAngles() + vecDeltaAngles );
							pEntity->ViewPunch( vecDeltaAngles );
						}

						Vector vecPush = (vecPushDir*m_damage*flFalloff*2.0f);
						if ( pEntity->GetFlags() & FL_BASEVELOCITY )
						{
							vecPush = vecPush + pEntity->GetBaseVelocity();
						}
						if ( vecPush.z > 0 && (pEntity->GetFlags() & FL_ONGROUND) )
						{
							pEntity->SetGroundEntity( NULL );
							Vector origin = pEntity->GetAbsOrigin();
							origin.z += 1.0f;
							pEntity->SetAbsOrigin( origin );
						}

						pEntity->SetBaseVelocity( vecPush );
						pEntity->AddFlag( FL_BASEVELOCITY );

						// Fire an output that the player has been pushed
						m_OnPushedPlayer.FireOutput( this, this );
						continue;
					}
				}
	
				if ( HasSpawnFlags( SF_PHYSEXPLOSION_NODAMAGE ) )
				{
					pEntity->VPhysicsTakeDamage( info );
				}
				else
				{
					pEntity->TakeDamage( info );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysExplosion::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		// print magnitude
		Q_snprintf(tempstr,sizeof(tempstr),"    magnitude: %f", m_damage);
		EntityText(text_offset,tempstr,0);
		text_offset++;

		// print target entity
		Q_snprintf(tempstr,sizeof(tempstr),"    limit to: %s", STRING(m_targetEntityName));
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//==================================================
// CPhysImpact
//==================================================

#define	bitsPHYSIMPACT_NOFALLOFF		0x00000001
#define	bitsPHYSIMPACT_INFINITE_LENGTH	0x00000002
#define	bitsPHYSIMPACT_IGNORE_MASS		0x00000004
#define bitsPHYSIMPACT_IGNORE_NORMAL	0x00000008

#define	DEFAULT_EXPLODE_DISTANCE	256
LINK_ENTITY_TO_CLASS( env_physimpact, CPhysImpact );

BEGIN_DATADESC( CPhysImpact )

	DEFINE_KEYFIELD( m_damage,				FIELD_FLOAT,	"magnitude" ),
	DEFINE_KEYFIELD( m_distance,			FIELD_FLOAT,	"distance" ),
	DEFINE_KEYFIELD( m_directionEntityName,FIELD_STRING,	"directionentityname" ),

	// Function pointers
	DEFINE_FUNCTION( PointAtEntity ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Impact", InputImpact ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::Activate( void )
{
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::Spawn( void )
{
	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_NONE );
	SetModelName( NULL_STRING );

	//If not targetted, and no distance is set, give it a default value
	if ( m_distance == 0 )
	{
		m_distance = DEFAULT_EXPLODE_DISTANCE;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysImpact::PointAtEntity( void )
{
	//If we're not targetted at anything, don't bother
	if ( m_directionEntityName == NULL_STRING )
		return;

	UTIL_PointAtNamedEntity( this, m_directionEntityName );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CPhysImpact::InputImpact( inputdata_t &inputdata )
{
	Vector	dir;
	trace_t	trace;

	//If we have a direction target, setup to point at it
	if ( m_directionEntityName != NULL_STRING )
	{
		PointAtEntity();
	}

	AngleVectors( GetAbsAngles(), &dir );
	
	//Setup our trace information
	float	dist	= HasSpawnFlags( bitsPHYSIMPACT_INFINITE_LENGTH ) ? MAX_TRACE_LENGTH : m_distance;
	Vector	start	= GetAbsOrigin();
	Vector	end		= start + ( dir * dist );

	//Trace out
	UTIL_TraceLine( start, end, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );
	if ( trace.startsolid )
	{
		// ep1_citadel_04 has a phys_impact just behind another entity, so if we startsolid then
		// bump out just a little and retry the trace
		Vector startOffset = start +  ( dir * 0.1 );
		UTIL_TraceLine( startOffset , end, MASK_SHOT, this, COLLISION_GROUP_NONE, &trace );
	}

	if( debug_physimpact.GetBool() )
	{
		NDebugOverlay::Cross3D( start, 24, 255, 255, 255, false, 30 );
		NDebugOverlay::Line( trace.startpos, trace.endpos, 0, 255, 0, false, 30 );
	}

	if ( trace.fraction != 1.0 )
	{
		// if inside the object, just go opposite the direction
		if ( trace.startsolid )
		{
			trace.plane.normal = -dir;
		}
		CBaseEntity	*pEnt = trace.m_pEnt;
	
		IPhysicsObject *pPhysics = pEnt->VPhysicsGetObject();
		//If the entity is valid, hit it
		if ( ( pEnt != NULL  ) && ( pPhysics != NULL ) )
		{
			CTakeDamageInfo info;
			info.SetAttacker( this);
			info.SetInflictor( this );
			info.SetDamage( 0 );
			info.SetDamageForce( vec3_origin );
			info.SetDamageType( DMG_GENERIC );

			pEnt->DispatchTraceAttack( info, dir, &trace );
			ApplyMultiDamage();

			//Damage falls off unless specified or the ray's length is infinite
			float	damage = HasSpawnFlags( bitsPHYSIMPACT_NOFALLOFF | bitsPHYSIMPACT_INFINITE_LENGTH ) ? 
								m_damage : (m_damage * (1.0f-trace.fraction));
			
			if ( HasSpawnFlags( bitsPHYSIMPACT_IGNORE_MASS ) )
			{
				damage *= pPhysics->GetMass();
			}

			if( debug_physimpact.GetBool() )
			{
				NDebugOverlay::Line( trace.endpos, trace.endpos + trace.plane.normal * -128, 255, 0, 0, false, 30 );
			}

			// Legacy entities applied the force along the impact normal, which yielded unpredictable results.
			if ( !HasSpawnFlags( bitsPHYSIMPACT_IGNORE_NORMAL ) )
			{
				dir = -trace.plane.normal;
			}				

			pPhysics->ApplyForceOffset( damage * dir * phys_pushscale.GetFloat(), trace.endpos );
		}
	}
}


class CSimplePhysicsBrush : public CBaseEntity
{
	DECLARE_CLASS( CSimplePhysicsBrush, CBaseEntity );
public:
	void Spawn()
	{
		SetModel( STRING( GetModelName() ) );
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
};

LINK_ENTITY_TO_CLASS( simple_physics_brush, CSimplePhysicsBrush );

class CSimplePhysicsProp : public CBaseProp
{
	DECLARE_CLASS( CSimplePhysicsProp, CBaseProp );

public:
	void Spawn()
	{
		BaseClass::Spawn();
		SetMoveType( MOVETYPE_VPHYSICS );
		SetSolid( SOLID_VPHYSICS );
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}

	int ObjectCaps()
	{ 
		int caps = BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION;

		if ( CBasePlayer::CanPickupObject( this, 35, 128 ) )
		{
			caps |= FCAP_IMPULSE_USE;
		}

		return caps;
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pActivator );
		if ( pPlayer )
		{
			pPlayer->PickupObject( this );
		}
	}
};

LINK_ENTITY_TO_CLASS( simple_physics_prop, CSimplePhysicsProp );

// UNDONE: Is this worth it?, just recreate the object instead? (that happens when this returns false anyway)
// recreating works, but is more expensive and won't inherit properties (velocity, constraints, etc)
bool TransferPhysicsObject( CBaseEntity *pFrom, CBaseEntity *pTo, bool wakeUp )
{
	IPhysicsObject *pVPhysics = pFrom->VPhysicsGetObject();
	if ( !pVPhysics || pVPhysics->IsStatic() )
		return false;

	// clear out the pointer so it won't get deleted
	pFrom->VPhysicsSwapObject( NULL );
	// remove any AI behavior bound to it
	pVPhysics->RemoveShadowController();
	// transfer to the new owner
	pTo->VPhysicsSetObject( pVPhysics );
	pVPhysics->SetGameData( (void *)pTo );
	pTo->VPhysicsUpdate( pVPhysics );
	
	// may have been temporarily disabled by the old object
	pVPhysics->EnableMotion( true );
	pVPhysics->EnableGravity( true );
	
	// Update for the new entity solid type
	pVPhysics->RecheckCollisionFilter();
	if ( wakeUp )
	{
		pVPhysics->Wake();
	}

	return true;
}

// UNDONE: Move/rename this function
static CBaseEntity *CreateSimplePhysicsObject( CBaseEntity *pEntity, bool createAsleep, bool createAsDebris )
{
	CBaseEntity *pPhysEntity = NULL;
	int modelindex = pEntity->GetModelIndex();
	const model_t *model = modelinfo->GetModel( modelindex );
	if ( model && modelinfo->GetModelType(model) == mod_brush )
	{
		pPhysEntity = CreateEntityByName( "simple_physics_brush" );
	}
	else
	{
		pPhysEntity = CreateEntityByName( "simple_physics_prop" );
	}

	pPhysEntity->KeyValue( "model", STRING(pEntity->GetModelName()) );
	pPhysEntity->SetAbsOrigin( pEntity->GetAbsOrigin() );
	pPhysEntity->SetAbsAngles( pEntity->GetAbsAngles() );
	pPhysEntity->Spawn();
	if ( !TransferPhysicsObject( pEntity, pPhysEntity, !createAsleep ) )
	{
		pPhysEntity->VPhysicsInitNormal( SOLID_VPHYSICS, 0, createAsleep );
		if ( createAsDebris )
			pPhysEntity->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}
	return pPhysEntity;
}

#define SF_CONVERT_ASLEEP		0x0001
#define SF_CONVERT_AS_DEBRIS	0x0002

class CPhysConvert : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConvert, CLogicalEntity );

public:
	CPhysConvert( void ) : m_flMassOverride( 0.0f ) {};
	COutputEvent m_OnConvert;	

	// Input handlers
	void InputConvertTarget( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	string_t		m_swapModel;
	float			m_flMassOverride;
};

LINK_ENTITY_TO_CLASS( phys_convert, CPhysConvert );

BEGIN_DATADESC( CPhysConvert )

	DEFINE_KEYFIELD( m_swapModel,		FIELD_STRING,	"swapmodel" ),
	DEFINE_KEYFIELD( m_flMassOverride,	FIELD_FLOAT,	"massoverride" ),
	
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "ConvertTarget", InputConvertTarget ),

	// Outputs
	DEFINE_OUTPUT( m_OnConvert, "OnConvert"),

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Input handler that converts our target to a physics object.
//-----------------------------------------------------------------------------
void CPhysConvert::InputConvertTarget( inputdata_t &inputdata )
{
	bool createAsleep = HasSpawnFlags(SF_CONVERT_ASLEEP);
	bool createAsDebris = HasSpawnFlags(SF_CONVERT_AS_DEBRIS);
	// Fire output
	m_OnConvert.FireOutput( inputdata.pActivator, this );

	CBaseEntity *entlist[512];
	CBaseEntity *pSwap = gEntList.FindEntityByName( NULL, m_swapModel, NULL, inputdata.pActivator, inputdata.pCaller );
	CBaseEntity *pEntity = NULL;
	
	int count = 0;
	while ( (pEntity = gEntList.FindEntityByName( pEntity, m_target, NULL, inputdata.pActivator, inputdata.pCaller )) != NULL )
	{
		entlist[count++] = pEntity;
		if ( count >= ARRAYSIZE(entlist) )
			break;
	}

	// if we're swapping to model out, don't loop over more than one object
	// multiple objects with the same brush model will render, but the dynamic lights
	// and decals will be shared between the two instances...
	if ( pSwap && count > 0 )
	{
		count = 1;
	}

	for ( int i = 0; i < count; i++ )
	{
		pEntity = entlist[i];

		// don't convert something that is already physics based
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			Msg( "ERROR phys_convert %s ! Already MOVETYPE_VPHYSICS\n", STRING(pEntity->m_iClassname) );
			continue;
		}

		UnlinkFromParent( pEntity );

		if ( pSwap )
		{
			// we can't reuse this physics object, so kill it
			pEntity->VPhysicsDestroyObject();
			pEntity->SetModel( STRING(pSwap->GetModelName()) );
		}

		// created phys object, now move hierarchy over
		CBaseEntity *pPhys = CreateSimplePhysicsObject( pEntity, createAsleep, createAsDebris );
		if ( pPhys )
		{
			// Override the mass if specified
			if ( m_flMassOverride > 0 )
			{
				IPhysicsObject *pPhysObj = pPhys->VPhysicsGetObject();
				if ( pPhysObj )
				{
					pPhysObj->SetMass( m_flMassOverride );
				}
			}

			pPhys->SetName( pEntity->GetEntityName() );
			UTIL_TransferPoseParameters( pEntity, pPhys );
			TransferChildren( pEntity, pPhys );
			pEntity->AddSolidFlags( FSOLID_NOT_SOLID );
			pEntity->AddEffects( EF_NODRAW );
			UTIL_Remove( pEntity );
		}
	}
}

//============================================================================================================
// PHYS MAGNET
//============================================================================================================
#define SF_MAGNET_ASLEEP			0x0001
#define SF_MAGNET_MOTIONDISABLED	0x0002
#define SF_MAGNET_SUCK				0x0004
#define SF_MAGNET_ALLOWROTATION		0x0008
#define SF_MAGNET_COAST_HACK		0x0010

LINK_ENTITY_TO_CLASS( phys_magnet, CPhysMagnet );

// BUGBUG: This won't work!  Right now you can't save physics pointers inside an embedded type!
BEGIN_SIMPLE_DATADESC( magnetted_objects_t )

	DEFINE_PHYSPTR( pConstraint ),
	DEFINE_FIELD( hEntity,	FIELD_EHANDLE	),

END_DATADESC()

BEGIN_DATADESC( CPhysMagnet )
	// Outputs
	DEFINE_OUTPUT( m_OnMagnetAttach, "OnAttach" ),
	DEFINE_OUTPUT( m_OnMagnetDetach, "OnDetach" ),

	// Keys
	DEFINE_KEYFIELD( m_massScale, FIELD_FLOAT, "massScale" ),
	DEFINE_KEYFIELD( m_iszOverrideScript, FIELD_STRING, "overridescript" ),
	DEFINE_KEYFIELD( m_iMaxObjectsAttached, FIELD_INTEGER, "maxobjects" ),
	DEFINE_KEYFIELD( m_forceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( m_torqueLimit, FIELD_FLOAT, "torquelimit" ),

	DEFINE_UTLVECTOR( m_MagnettedEntities, FIELD_EMBEDDED ),
	DEFINE_PHYSPTR( m_pConstraintGroup ),

	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bHasHitSomething, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTotalMass, FIELD_FLOAT ),
	DEFINE_FIELD( m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextSuckTime, FIELD_FLOAT ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: SendProxy that converts the magnet's attached object UtlVector to entindexes
//-----------------------------------------------------------------------------
void SendProxy_MagnetAttachedObjectList( const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID )
{
	CPhysMagnet *pMagnet = (CPhysMagnet*)pData;

	// If this assertion fails, then SendProxyArrayLength_MagnetAttachedArray must have failed.
	Assert( iElement < pMagnet->GetNumAttachedObjects() );

	pOut->m_Int = pMagnet->GetAttachedObject(iElement)->entindex();
}


int SendProxyArrayLength_MagnetAttachedArray( const void *pStruct, int objectID )
{
	CPhysMagnet *pMagnet = (CPhysMagnet*)pStruct;
	return pMagnet->GetNumAttachedObjects();
}

IMPLEMENT_SERVERCLASS_ST( CPhysMagnet, DT_PhysMagnet )

	// ROBIN: Disabled because we don't need it anymore
	/*
	SendPropArray2( 
		SendProxyArrayLength_MagnetAttachedArray,
		SendPropInt("magnetattached_array_element", 0, 4, 10, SPROP_UNSIGNED, SendProxy_MagnetAttachedObjectList), 
		128, 
		0, 
		"magnetattached_array"
		)
	*/

END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhysMagnet::CPhysMagnet( void )
{
	m_forceLimit = 0;
	m_torqueLimit = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPhysMagnet::~CPhysMagnet( void )
{
	DetachAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Spawn( void )
{
	Precache();

	SetMoveType( MOVETYPE_NONE );
	SetSolid( SOLID_VPHYSICS );
	SetModel( STRING( GetModelName() ) );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	if ( m_massScale > 0 )
	{
		tmpSolid.params.mass *= m_massScale;
	}
	PhysSolidOverride( tmpSolid, m_iszOverrideScript );
	VPhysicsInitNormal( GetSolid(), GetSolidFlags(), true, &tmpSolid );

	// Wake it up if not asleep
	if ( !HasSpawnFlags(SF_MAGNET_ASLEEP) )
	{
		VPhysicsGetObject()->Wake();
	}

	if ( HasSpawnFlags(SF_MAGNET_MOTIONDISABLED) )
	{
		VPhysicsGetObject()->EnableMotion( false );
	}

	m_bActive = true;
	m_pConstraintGroup = NULL;
	m_flTotalMass = 0;
	m_flNextSuckTime = 0;

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Precache( void )
{
	PrecacheModel( STRING( GetModelName() ) );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::Touch( CBaseEntity *pOther )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	int otherIndex = !index;
	CBaseEntity *pOther = pEvent->pEntities[otherIndex];

	// Ignore triggers
	if ( pOther->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
		return;

	m_bHasHitSomething = true;
	DoMagnetSuck( pEvent->pEntities[!index] );

	// Don't pickup if we're not active
	if ( !m_bActive )
		return;

	// Hit our maximum?
	if ( m_iMaxObjectsAttached && m_iMaxObjectsAttached <= GetNumAttachedObjects() )
		return;

	// This is a hack to solve players (Erik) stacking stuff on their jeeps in coast_01 
	// and being screwed when the crane can't pick them up. We need to get rid of the object.
	if ( HasSpawnFlags( SF_MAGNET_COAST_HACK ) )
	{
		// If the other isn't the jeep, we need to get rid of it
		if ( !FClassnameIs( pOther, "prop_vehicle_jeep" ) )
		{
			// If it takes damage, destroy it
			if ( pOther->m_takedamage != DAMAGE_NO && pOther->m_takedamage != DAMAGE_EVENTS_ONLY )
			{
				CTakeDamageInfo info( this, this, pOther->GetHealth(), DMG_GENERIC | DMG_PREVENT_PHYSICS_FORCE );
				pOther->TakeDamage( info );
			}
			else if ( pEvent->pObjects[ otherIndex ]->IsMoveable() )
			{
				// Otherwise, we're screwed, so just remove it
				UTIL_Remove( pOther );
			}
			else
			{
				Warning( "CPhysMagnet %s:%d blocking magnet\n",
					pOther->GetClassname(), pOther->entindex() );
			}
			return;
		}
	}

	// Make sure it's made of metal
	const surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	char cTexType = phit->game.material;
	if ( cTexType != CHAR_TEX_METAL && cTexType != CHAR_TEX_COMPUTER )
	{
		// If we don't have a model, we're done. The texture we hit wasn't metal.
		if ( !pOther->GetBaseAnimating() )
			return;

		// If we have a model that wants to be metal, even though we hit a non-metal texture, we'll stick to it
		if ( Q_strncmp( Studio_GetDefaultSurfaceProps( pOther->GetBaseAnimating()->GetModelPtr() ), "metal", 5 ) )
			return;
	}

	IPhysicsObject *pPhysics = pOther->VPhysicsGetObject();
	if ( pPhysics && pOther->GetMoveType() == MOVETYPE_VPHYSICS && pPhysics->IsMoveable() )
	{
		// Make sure we haven't already got this sucker on the magnet
		int iCount = m_MagnettedEntities.Count();
		for ( int i = 0; i < iCount; i++ )
		{
			if ( m_MagnettedEntities[i].hEntity == pOther )
				return;
		}

		// We want to cast a long way to ensure our shadow shows up
		pOther->SetShadowCastDistance( 2048 );

		// Create a constraint between the magnet and this sucker
		IPhysicsObject *pMagnetPhysObject = VPhysicsGetObject();
		Assert( pMagnetPhysObject );

		magnetted_objects_t newEntityOnMagnet;
		newEntityOnMagnet.hEntity = pOther;

		// Use the right constraint
		if ( HasSpawnFlags( SF_MAGNET_ALLOWROTATION ) )
		{
			constraint_ballsocketparams_t ballsocket;
			ballsocket.Defaults();
			ballsocket.constraint.Defaults();
			ballsocket.constraint.forceLimit = lbs2kg(m_forceLimit);
			ballsocket.constraint.torqueLimit = lbs2kg(m_torqueLimit);

			Vector vecCollisionPoint;
			pEvent->pInternalData->GetContactPoint( vecCollisionPoint );

			pMagnetPhysObject->WorldToLocal( &ballsocket.constraintPosition[0], vecCollisionPoint );
			pPhysics->WorldToLocal( &ballsocket.constraintPosition[1], vecCollisionPoint );

			//newEntityOnMagnet.pConstraint = physenv->CreateBallsocketConstraint( pMagnetPhysObject, pPhysics, m_pConstraintGroup, ballsocket );
			newEntityOnMagnet.pConstraint = physenv->CreateBallsocketConstraint( pMagnetPhysObject, pPhysics, NULL, ballsocket );
		}
		else
		{
			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState( pMagnetPhysObject, pPhysics );
			fixed.constraint.Defaults();
			fixed.constraint.forceLimit = lbs2kg(m_forceLimit);
			fixed.constraint.torqueLimit = lbs2kg(m_torqueLimit);

			// FIXME: Use the magnet's constraint group.
			//newEntityOnMagnet.pConstraint = physenv->CreateFixedConstraint( pMagnetPhysObject, pPhysics, m_pConstraintGroup, fixed );
			newEntityOnMagnet.pConstraint = physenv->CreateFixedConstraint( pMagnetPhysObject, pPhysics, NULL, fixed );
		}

		newEntityOnMagnet.pConstraint->SetGameData( (void *) this );
		m_MagnettedEntities.AddToTail( newEntityOnMagnet );

		m_flTotalMass += pPhysics->GetMass();
	}

	DoMagnetSuck( pOther );

	m_OnMagnetAttach.FireOutput( this, this );

	BaseClass::VPhysicsCollision( index, pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::DoMagnetSuck( CBaseEntity *pOther )
{
	if ( !HasSpawnFlags( SF_MAGNET_SUCK ) )
		return;

	if ( !m_bActive )
		return;

	// Don't repeatedly suck
	if ( m_flNextSuckTime > gpGlobals->curtime )
		return;
	
	// Look for physics objects underneath the magnet and suck them onto it
	Vector vecCheckPos, vecSuckPoint;
	VectorTransform( Vector(0,0,-96), EntityToWorldTransform(), vecCheckPos );
	VectorTransform( Vector(0,0,-64), EntityToWorldTransform(), vecSuckPoint );

	CBaseEntity *pEntities[20];
	int iNumEntities = UTIL_EntitiesInSphere( pEntities, 20, vecCheckPos, 80.0, 0 );
	for ( int i = 0; i < iNumEntities; i++ )
	{
		CBaseEntity *pEntity = pEntities[i];
		if ( !pEntity || pEntity == pOther )
			continue;

		IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
		if ( pPhys && pEntity->GetMoveType() == MOVETYPE_VPHYSICS && pPhys->GetMass() < 5000 )
		{
			// Do we have line of sight to it?
			trace_t tr;
			UTIL_TraceLine( GetAbsOrigin(), pEntity->GetAbsOrigin(), MASK_SHOT, this, 0, &tr );
			if ( tr.fraction == 1.0 || tr.m_pEnt == pEntity )
			{
				// Pull it towards the magnet
				Vector vecVelocity = (vecSuckPoint - pEntity->GetAbsOrigin());
				VectorNormalize(vecVelocity);
				vecVelocity *= 5 * pPhys->GetMass();
				pPhys->AddVelocity( &vecVelocity, NULL );
			}
		}
	}

	m_flNextSuckTime = gpGlobals->curtime + 2.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::SetConstraintGroup( IPhysicsConstraintGroup *pGroup )
{
	m_pConstraintGroup = pGroup;
}

//-----------------------------------------------------------------------------
// Purpose: Make the magnet active
//-----------------------------------------------------------------------------
void CPhysMagnet::InputTurnOn( inputdata_t &inputdata )
{
	m_bActive = true;
}

//-----------------------------------------------------------------------------
// Purpose: Make the magnet inactive. Drop everything it's got hooked on.
//-----------------------------------------------------------------------------
void CPhysMagnet::InputTurnOff( inputdata_t &inputdata )
{
	m_bActive = false;
	DetachAll();
}

//-----------------------------------------------------------------------------
// Purpose: Toggle the magnet's active state
//-----------------------------------------------------------------------------
void CPhysMagnet::InputToggle( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		InputTurnOff( inputdata );
	}
	else
	{
		InputTurnOn( inputdata );
	}
}

//-----------------------------------------------------------------------------
// Purpose: One of our magnet constraints broke
//-----------------------------------------------------------------------------
void CPhysMagnet::ConstraintBroken( IPhysicsConstraint *pConstraint )
{
	// Find the entity that was constrained and release it
	int iCount = m_MagnettedEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		if ( m_MagnettedEntities[i].hEntity.Get() != NULL && m_MagnettedEntities[i].pConstraint == pConstraint )
		{
			IPhysicsObject *pPhysObject = m_MagnettedEntities[i].hEntity->VPhysicsGetObject();

			if( pPhysObject != NULL )
			{
				m_flTotalMass -= pPhysObject->GetMass();
			}

			m_MagnettedEntities.Remove(i);
			break;
		}
	}

	m_OnMagnetDetach.FireOutput( this, this );

	physenv->DestroyConstraint( pConstraint  );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysMagnet::DetachAll( void )
{
	// Make sure we haven't already got this sucker on the magnet
	int iCount = m_MagnettedEntities.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		// Delay a couple seconds to reset to the default shadow cast behavior
		if ( m_MagnettedEntities[i].hEntity )
		{
			m_MagnettedEntities[i].hEntity->SetShadowCastDistance( 0, 2.0f );
		}

		physenv->DestroyConstraint( m_MagnettedEntities[i].pConstraint  );
	}

	m_MagnettedEntities.Purge();
	m_flTotalMass = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysMagnet::GetNumAttachedObjects( void )
{
	return m_MagnettedEntities.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPhysMagnet::GetTotalMassAttachedObjects( void )
{
	return m_flTotalMass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CPhysMagnet::GetAttachedObject( int iIndex )
{
	Assert( iIndex < GetNumAttachedObjects() );

	return m_MagnettedEntities[iIndex].hEntity;
}

class CInfoMassCenter : public CPointEntity
{
	DECLARE_CLASS( CInfoMassCenter, CPointEntity );
public:
	void Spawn( void )
	{
		if ( m_target != NULL_STRING )
		{
			masscenteroverride_t params;
			params.SnapToPoint( m_target, GetAbsOrigin() );
			PhysSetMassCenterOverride( params );
			UTIL_Remove( this );
		}
	}
};
LINK_ENTITY_TO_CLASS( info_mass_center, CInfoMassCenter );

// =============================================================
// point_push
// =============================================================

class CPointPush : public CPointEntity
{
public:
	DECLARE_CLASS( CPointPush, CPointEntity );

	virtual void Activate( void );
	void PushThink( void );
	
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	DECLARE_DATADESC();

private:
	inline void PushEntity( CBaseEntity *pTarget );

	bool	m_bEnabled;
	float	m_flMagnitude;
	float	m_flRadius;
	float	m_flInnerRadius;	// Inner radius where the push eminates from (on a sphere)
};

LINK_ENTITY_TO_CLASS( point_push, CPointPush );

BEGIN_DATADESC( CPointPush )

	DEFINE_THINKFUNC( PushThink ),
	
	DEFINE_KEYFIELD( m_bEnabled,	FIELD_BOOLEAN,	"enabled" ),
	DEFINE_KEYFIELD( m_flMagnitude, FIELD_FLOAT,	"magnitude" ),
	DEFINE_KEYFIELD( m_flRadius,	FIELD_FLOAT,	"radius" ),
	DEFINE_KEYFIELD( m_flInnerRadius,FIELD_FLOAT,	"inner_radius" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

END_DATADESC();

// Spawnflags
#define	SF_PUSH_TEST_LOS			0x0001
#define SF_PUSH_DIRECTIONAL			0x0002
#define SF_PUSH_NO_FALLOFF			0x0004
#define	SF_PUSH_PLAYER				0x0008
#define SF_PUSH_PHYSICS				0x0010

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointPush::Activate( void )
{
	if ( m_bEnabled )
	{
		SetThink( &CPointPush::PushThink );
		SetNextThink( gpGlobals->curtime + 0.05f );
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CPointPush::PushEntity( CBaseEntity *pTarget )
{
	Vector vecPushDir;
	
	if ( HasSpawnFlags( SF_PUSH_DIRECTIONAL ) )
	{
		GetVectors( &vecPushDir, NULL, NULL );
	}
	else
	{
		vecPushDir = ( pTarget->BodyTarget( GetAbsOrigin(), false ) - GetAbsOrigin() );
	}

	float dist = VectorNormalize( vecPushDir );
	
	float flFalloff = ( HasSpawnFlags( SF_PUSH_NO_FALLOFF ) ) ? 1.0f : RemapValClamped( dist, m_flRadius, m_flRadius*0.25f, 0.0f, 1.0f );
	
	switch( pTarget->GetMoveType() )
	{
	case MOVETYPE_NONE:
	case MOVETYPE_PUSH:
	case MOVETYPE_NOCLIP:
		break;

	case MOVETYPE_VPHYSICS:
		{
			IPhysicsObject *pPhys = pTarget->VPhysicsGetObject();
			if ( pPhys )
			{
				// UNDONE: Assume the velocity is for a 100kg object, scale with mass
				pPhys->ApplyForceCenter( m_flMagnitude * flFalloff * 100.0f * vecPushDir * pPhys->GetMass() * gpGlobals->frametime );
				return;
			}
		}
		break;

	case MOVETYPE_STEP:
		{
			// NPCs cannot be lifted up properly, they need to move in 2D
			vecPushDir.z = 0.0f;
			
			// NOTE: Falls through!
		}

	default:
		{
			Vector vecPush = (m_flMagnitude * vecPushDir * flFalloff);
			if ( pTarget->GetFlags() & FL_BASEVELOCITY )
			{
				vecPush = vecPush + pTarget->GetBaseVelocity();
			}
			if ( vecPush.z > 0 && (pTarget->GetFlags() & FL_ONGROUND) )
			{
				pTarget->SetGroundEntity( NULL );
				Vector origin = pTarget->GetAbsOrigin();
				origin.z += 1.0f;
				pTarget->SetAbsOrigin( origin );
			}

			pTarget->SetBaseVelocity( vecPush );
			pTarget->AddFlag( FL_BASEVELOCITY );
		}
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointPush::PushThink( void )
{
	// Get a collection of entities in a radius around us
	CBaseEntity *pEnts[256];
	int numEnts = UTIL_EntitiesInSphere( pEnts, 256, GetAbsOrigin(), m_flRadius, 0 );
	for ( int i = 0; i < numEnts; i++ )
	{
		// Must be solid
		if ( pEnts[i]->IsSolid() == false )
			continue;

		// Cannot be parented (only push parents)
		if ( pEnts[i]->GetMoveParent() != NULL )
			continue;

		// Must be moveable
		if ( pEnts[i]->GetMoveType() != MOVETYPE_VPHYSICS && 
			 pEnts[i]->GetMoveType() != MOVETYPE_WALK && 
			 pEnts[i]->GetMoveType() != MOVETYPE_STEP )
			continue; 

		// If we don't want to push players, don't
		if ( pEnts[i]->IsPlayer() && HasSpawnFlags( SF_PUSH_PLAYER ) == false )
			continue;

		// If we don't want to push physics, don't
		if ( pEnts[i]->GetMoveType() == MOVETYPE_VPHYSICS && HasSpawnFlags( SF_PUSH_PHYSICS ) == false )
			continue;

		// Test for LOS if asked to
		if ( HasSpawnFlags( SF_PUSH_TEST_LOS ) )
		{
			Vector vecStartPos = GetAbsOrigin();
			Vector vecEndPos = pEnts[i]->BodyTarget( vecStartPos, false );

			if ( m_flInnerRadius != 0.0f )
			{
				// Find a point on our inner radius sphere to begin from
				Vector vecDirToTarget = ( vecEndPos - vecStartPos );
				VectorNormalize( vecDirToTarget );
				vecStartPos = GetAbsOrigin() + ( vecDirToTarget * m_flInnerRadius );
			}

			trace_t tr;
			UTIL_TraceLine( vecStartPos, 
							pEnts[i]->BodyTarget( vecStartPos, false ), 
							MASK_SOLID_BRUSHONLY, 
							this, 
							COLLISION_GROUP_NONE, 
							&tr );

			// Shielded
			if ( tr.fraction < 1.0f && tr.m_pEnt != pEnts[i] )
				continue;
		}

		// Push it along
		PushEntity( pEnts[i] );
	}

	// Set us up for the next think
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointPush::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
	SetThink( &CPointPush::PushThink );
	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointPush::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	SetThink( NULL );
	SetNextThink( gpGlobals->curtime );
}
