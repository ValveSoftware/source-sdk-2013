//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Physics constraint entities
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "physics.h"
#include "entityoutput.h"
#include "engine/IEngineSound.h"
#include "igamesystem.h"
#include "physics_saverestore.h"
#include "vcollide_parse.h"
#include "positionwatcher.h"
#include "fmtstr.h"
#include "physics_prop_ragdoll.h"

#define HINGE_NOTIFY HL2_EPISODIC
#if HINGE_NOTIFY
#include "physconstraint_sounds.h"
#endif

#include "physconstraint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_CONSTRAINT_DISABLE_COLLISION			0x0001
#define SF_SLIDE_LIMIT_ENDS						0x0002
#define SF_PULLEY_RIGID							0x0002
#define SF_LENGTH_RIGID							0x0002
#define SF_RAGDOLL_FREEMOVEMENT					0x0002
#define SF_CONSTRAINT_START_INACTIVE			0x0004
#define SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY		0x0008
#define SF_CONSTRAINT_NO_CONNECT_UNTIL_ACTIVATED	0x0010	// Will only check the two attached entities at activation


ConVar    g_debug_constraint_sounds	  ( "g_debug_constraint_sounds", "0", FCVAR_CHEAT, "Enable debug printing about constraint sounds.");

struct constraint_anchor_t
{
	Vector		localOrigin;
	EHANDLE		hEntity;
	int			parentAttachment;
	string_t	name;
	float		massScale;
};

class CAnchorList : public CAutoGameSystem
{
public:
	CAnchorList( char const *name ) : CAutoGameSystem( name )
	{
	}
	void LevelShutdownPostEntity() 
	{
		m_list.Purge();
	}

	void AddToList( CBaseEntity *pEntity, float massScale )
	{
		int index = m_list.AddToTail();
		constraint_anchor_t *pAnchor = &m_list[index];

		pAnchor->hEntity = pEntity->GetParent();
		pAnchor->parentAttachment = pEntity->GetParentAttachment();
		pAnchor->name = pEntity->GetEntityName();
		pAnchor->localOrigin = pEntity->GetLocalOrigin();
		pAnchor->massScale = massScale;
	}

	constraint_anchor_t *Find( string_t name )
	{
		for ( int i = m_list.Count()-1; i >=0; i-- )
		{
			if ( FStrEq( STRING(m_list[i].name), STRING(name) ) )
			{
				return &m_list[i];
			}
		}
		return NULL;
	}

private:
	CUtlVector<constraint_anchor_t>	m_list;
};

static CAnchorList g_AnchorList( "CAnchorList" );

class CConstraintAnchor : public CPointEntity
{
	DECLARE_CLASS( CConstraintAnchor, CPointEntity );
public:
	CConstraintAnchor()
	{
		m_massScale = 1.0f;
	}
	void Spawn( void )
	{
		if ( GetParent() )
		{
			g_AnchorList.AddToList( this, m_massScale );
			UTIL_Remove( this );
		}
	}
	DECLARE_DATADESC();

	float m_massScale;
};

BEGIN_DATADESC( CConstraintAnchor )
	DEFINE_KEYFIELD( m_massScale, FIELD_FLOAT, "massScale" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_constraint_anchor, CConstraintAnchor );

class CPhysConstraintSystem : public CLogicalEntity
{
	DECLARE_CLASS( CPhysConstraintSystem, CLogicalEntity );
public:

	void Spawn();
	IPhysicsConstraintGroup *GetVPhysicsGroup() { return m_pMachine; }

	DECLARE_DATADESC();
private:
	IPhysicsConstraintGroup *m_pMachine;
	int						m_additionalIterations;
};

BEGIN_DATADESC( CPhysConstraintSystem )
	DEFINE_PHYSPTR( m_pMachine ),
	DEFINE_KEYFIELD( m_additionalIterations, FIELD_INTEGER, "additionaliterations" ),
	
END_DATADESC()


void CPhysConstraintSystem::Spawn()
{
	constraint_groupparams_t group;
	group.Defaults();
	group.additionalIterations = m_additionalIterations;
	m_pMachine = physenv->CreateConstraintGroup( group );
}

LINK_ENTITY_TO_CLASS( phys_constraintsystem, CPhysConstraintSystem );

void PhysTeleportConstrainedEntity( CBaseEntity *pTeleportSource, IPhysicsObject *pObject0, IPhysicsObject *pObject1, const Vector &prevPosition, const QAngle &prevAngles, bool physicsRotate )
{
	// teleport the other object
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *> (pObject0->GetGameData());
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *> (pObject1->GetGameData());
	if ( !pEntity0 || !pEntity1 )
		return;

	// figure out which entity needs to be fixed up (the one that isn't pTeleportSource)
	CBaseEntity *pFixup = pEntity1;
	// teleport the other object
	if ( pTeleportSource != pEntity0 )
	{
		if ( pTeleportSource != pEntity1 )
		{
			Msg("Bogus teleport notification!!\n");
			return;
		}
		pFixup = pEntity0;
	}

	// constraint doesn't move this entity
	if ( pFixup->GetMoveType() != MOVETYPE_VPHYSICS )
		return;

	if ( !pFixup->VPhysicsGetObject() || !pFixup->VPhysicsGetObject()->IsMoveable() )
		return;

	QAngle oldAngles = prevAngles;

	if ( !physicsRotate )
	{
		oldAngles = pTeleportSource->GetAbsAngles();
	}

	matrix3x4_t startCoord, startInv, endCoord, xform;
	AngleMatrix( oldAngles, prevPosition, startCoord );
	MatrixInvert( startCoord, startInv );
	ConcatTransforms( pTeleportSource->EntityToWorldTransform(), startInv, xform );
	QAngle fixupAngles;
	Vector fixupPos;

	ConcatTransforms( xform, pFixup->EntityToWorldTransform(), endCoord );
	MatrixAngles( endCoord, fixupAngles, fixupPos );
	pFixup->Teleport( &fixupPos, &fixupAngles, NULL );
}

static void DrawPhysicsBounds( IPhysicsObject *pObject, int r, int g, int b, int a )
{
	const CPhysCollide *pCollide = pObject->GetCollide();
	Vector pos;
	QAngle angles;
	pObject->GetPosition( &pos, &angles );
	Vector mins, maxs;
	physcollision->CollideGetAABB( &mins, &maxs, pCollide, vec3_origin, vec3_angle );
	// don't fight the z-buffer
	mins -= Vector(1,1,1);
	maxs += Vector(1,1,1);
	NDebugOverlay::BoxAngles( pos, mins, maxs, angles, r, g, b, a, 0 );
}

static void DrawConstraintObjectsAxes(CBaseEntity *pConstraintEntity, IPhysicsConstraint *pConstraint)
{
	if ( !pConstraint || !pConstraintEntity )
		return;
	matrix3x4_t xformRef, xformAtt;
	bool bXform = pConstraint->GetConstraintTransform( &xformRef, &xformAtt );
	IPhysicsObject *pRef = pConstraint->GetReferenceObject();

	if ( pRef && !pRef->IsStatic() )
	{
		if ( bXform )
		{
			Vector pos, posWorld;
			QAngle angles;
			MatrixAngles( xformRef, angles, pos );
			pRef->LocalToWorld( &posWorld, pos );
			NDebugOverlay::Axis( posWorld, vec3_angle, 12, false, 0 );
		}
		DrawPhysicsBounds( pRef, 0, 255, 0, 12 );
	}
	IPhysicsObject *pAttach = pConstraint->GetAttachedObject();
	if ( pAttach && !pAttach->IsStatic() )
	{
		if ( bXform )
		{
			Vector pos, posWorld;
			QAngle angles;
			MatrixAngles( xformAtt, angles, pos );
			pAttach->LocalToWorld( &posWorld, pos );
			NDebugOverlay::Axis( posWorld, vec3_angle, 12, false, 0 );
		}
		DrawPhysicsBounds( pAttach, 255, 0, 0, 12 );
	}
}

void CPhysConstraint::ClearStaticFlag( IPhysicsObject *pObj )
{
	if ( !pObj )
		return;
	PhysClearGameFlags( pObj, FVPHYSICS_CONSTRAINT_STATIC );
}

void CPhysConstraint::Deactivate()
{
	if ( !m_pConstraint )
		return;
	m_pConstraint->Deactivate();
	ClearStaticFlag( m_pConstraint->GetReferenceObject() );
	ClearStaticFlag( m_pConstraint->GetAttachedObject() );
	if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
	{
		// constraint may be getting deactivated because an object got deleted, so check them here.
		IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
		IPhysicsObject *pAtt = m_pConstraint->GetAttachedObject();
		if ( pRef && pAtt )
		{
			PhysEnableEntityCollisions( pRef, pAtt );
		}
	}
}

void CPhysConstraint::OnBreak( void )
{
	Deactivate();
	if ( m_breakSound != NULL_STRING )
	{
		CPASAttenuationFilter filter( this, ATTN_STATIC );

		Vector origin = GetAbsOrigin();
		Vector refPos = origin, attachPos = origin;

		IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
		if ( pRef && (pRef != g_PhysWorldObject) )
		{
			pRef->GetPosition( &refPos, NULL );
			attachPos = refPos;
		}
		IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
		if ( pAttach && (pAttach != g_PhysWorldObject) )
		{
			pAttach->GetPosition( &attachPos, NULL );
			if ( !pRef || (pRef == g_PhysWorldObject) )
			{
				refPos = attachPos;
			}
		}
		
		VectorAdd( refPos, attachPos, origin );
		origin *= 0.5f;

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = STRING(m_breakSound);
		ep.m_flVolume = VOL_NORM;
		ep.m_SoundLevel = ATTN_TO_SNDLVL( ATTN_STATIC );
		ep.m_pOrigin = &origin;

		EmitSound( filter, entindex(), ep );
	}
	m_OnBreak.FireOutput( this, this );
	// queue this up to be deleted at the end of physics 
	// The Deactivate() call should make sure we don't get more of these callbacks.
	PhysCallbackRemove( this->NetworkProp() );
}

void CPhysConstraint::InputBreak( inputdata_t &inputdata )
{
	if ( m_pConstraint ) 
		m_pConstraint->Deactivate();
	
	OnBreak();
}

void CPhysConstraint::InputOnBreak( inputdata_t &inputdata )
{
	OnBreak();
}

void CPhysConstraint::InputTurnOn( inputdata_t &inputdata )
{
	if ( HasSpawnFlags( SF_CONSTRAINT_NO_CONNECT_UNTIL_ACTIVATED ) )
	{
		ActivateConstraint();
	}

	if ( !m_pConstraint || !m_pConstraint->GetReferenceObject() || !m_pConstraint->GetAttachedObject() )
		return;

	m_pConstraint->Activate();
	m_pConstraint->GetReferenceObject()->Wake();
	m_pConstraint->GetAttachedObject()->Wake();
}

void CPhysConstraint::InputTurnOff( inputdata_t &inputdata )
{
	Deactivate();
}

int CPhysConstraint::DrawDebugTextOverlays()
{
	int pos = BaseClass::DrawDebugTextOverlays();
	if ( m_pConstraint && (m_debugOverlays & OVERLAY_TEXT_BIT) )
	{
		constraint_breakableparams_t params;
		Q_memset(&params,0,sizeof(params));
		m_pConstraint->GetConstraintParams( &params );
		
		if ( (params.bodyMassScale[0] != 1.0f && params.bodyMassScale[0] != 0.0f) || (params.bodyMassScale[1] != 1.0f && params.bodyMassScale[1] != 0.0f) )
		{
			CFmtStr str("mass ratio %.4f:%.4f\n", params.bodyMassScale[0], params.bodyMassScale[1] );
			NDebugOverlay::EntityTextAtPosition( GetAbsOrigin(), pos, str.Access(), 0, 255, 255, 0, 255 );
		}
		pos++;
	}
	return pos;
}

void CPhysConstraint::DrawDebugGeometryOverlays()
{
	if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
	{
		DrawConstraintObjectsAxes(this, m_pConstraint);
	}
	BaseClass::DrawDebugGeometryOverlays();
}

void CPhysConstraint::GetBreakParams( constraint_breakableparams_t &params, const hl_constraint_info_t &info )
{
	params.Defaults();
	params.forceLimit = lbs2kg(m_forceLimit);
	params.torqueLimit = lbs2kg(m_torqueLimit);
	params.isActive = HasSpawnFlags( SF_CONSTRAINT_START_INACTIVE ) ? false : true;
	params.bodyMassScale[0] = info.massScale[0];
	params.bodyMassScale[1] = info.massScale[1];
}

BEGIN_DATADESC( CPhysConstraint )

	DEFINE_PHYSPTR( m_pConstraint ),

	DEFINE_KEYFIELD( m_nameSystem, FIELD_STRING, "constraintsystem" ),
	DEFINE_KEYFIELD( m_nameAttach1, FIELD_STRING, "attach1" ),
	DEFINE_KEYFIELD( m_nameAttach2, FIELD_STRING, "attach2" ),
	DEFINE_KEYFIELD( m_breakSound, FIELD_SOUNDNAME, "breaksound" ),
	DEFINE_KEYFIELD( m_forceLimit, FIELD_FLOAT, "forcelimit" ),
	DEFINE_KEYFIELD( m_torqueLimit, FIELD_FLOAT, "torquelimit" ),
	DEFINE_KEYFIELD( m_minTeleportDistance, FIELD_FLOAT, "teleportfollowdistance" ),
//	DEFINE_FIELD( m_teleportTick, FIELD_INTEGER ),

	DEFINE_OUTPUT( m_OnBreak, "OnBreak" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ConstraintBroken", InputOnBreak ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),

END_DATADESC()


CPhysConstraint::CPhysConstraint( void )
{
	m_pConstraint = NULL;
	m_nameAttach1 = NULL_STRING;
	m_nameAttach2 = NULL_STRING;
	m_forceLimit = 0;
	m_torqueLimit = 0;
	m_teleportTick = 0xFFFFFFFF;
	m_minTeleportDistance = 0.0f;
}

CPhysConstraint::~CPhysConstraint()
{
	Deactivate();
	physenv->DestroyConstraint( m_pConstraint );
}

void CPhysConstraint::Precache( void )
{
	if ( m_breakSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_breakSound) );
	}
}

void CPhysConstraint::Spawn( void )
{
	BaseClass::Spawn();

	Precache();
}

// debug function - slow, uses dynamic_cast<> - use this to query the attached objects
// physics_debug_entity toggles the constraint system for an object using this
bool GetConstraintAttachments( CBaseEntity *pEntity, CBaseEntity *pAttachOut[2], IPhysicsObject *pAttachVPhysics[2] )
{
	CPhysConstraint *pConstraintEntity = dynamic_cast<CPhysConstraint *>(pEntity);
	if ( pConstraintEntity )
	{
		IPhysicsConstraint *pConstraint = pConstraintEntity->GetPhysConstraint();
		if ( pConstraint )
		{
			IPhysicsObject *pRef = pConstraint->GetReferenceObject();
			pAttachVPhysics[0] = pRef;
			pAttachOut[0] = pRef ? static_cast<CBaseEntity *>(pRef->GetGameData()) : NULL;
			IPhysicsObject *pAttach = pConstraint->GetAttachedObject();
			pAttachVPhysics[1] = pAttach;
			pAttachOut[1] = pAttach ? static_cast<CBaseEntity *>(pAttach->GetGameData()) : NULL;
			return true;
		}
	}
	return false;
}

void DebugConstraint(CBaseEntity *pEntity)
{
	CPhysConstraint *pConstraintEntity = dynamic_cast<CPhysConstraint *>(pEntity);
	if ( pConstraintEntity )
	{
		IPhysicsConstraint *pConstraint = pConstraintEntity->GetPhysConstraint();
		if ( pConstraint )
		{
			pConstraint->OutputDebugInfo();
		}
	}
}


void FindPhysicsAnchor( string_t name, hl_constraint_info_t &info, int index, CBaseEntity *pErrorEntity )
{
	constraint_anchor_t *pAnchor = g_AnchorList.Find( name );
	if ( pAnchor )
	{
		CBaseEntity *pEntity = pAnchor->hEntity;
		if ( pEntity )
		{
			info.massScale[index] = pAnchor->massScale;
			bool bWroteAttachment = false;
			if ( pAnchor->parentAttachment > 0 )
			{
				CBaseAnimating *pAnim = pAnchor->hEntity->GetBaseAnimating();
				if ( pAnim )
				{
					IPhysicsObject *list[VPHYSICS_MAX_OBJECT_LIST_COUNT];
					int listCount = pAnchor->hEntity->VPhysicsGetObjectList( list, ARRAYSIZE(list) );
					int iPhysicsBone = pAnim->GetPhysicsBone( pAnim->GetAttachmentBone( pAnchor->parentAttachment ) );
					if ( iPhysicsBone < listCount )
					{
						Vector pos;
						info.pObjects[index] = list[iPhysicsBone];
						pAnim->GetAttachment( pAnchor->parentAttachment, pos );
						list[iPhysicsBone]->WorldToLocal( &info.anchorPosition[index], pos );
						bWroteAttachment = true;
					}
				}
			}
			if ( !bWroteAttachment )
			{
				info.anchorPosition[index] = pAnchor->localOrigin;
				info.pObjects[index] = pAnchor->hEntity->VPhysicsGetObject();
			}
		}
		else
		{
			pAnchor = NULL;
		}
	}
	if ( !pAnchor )
	{
		info.anchorPosition[index] = vec3_origin;
		info.pObjects[index] = FindPhysicsObjectByName( STRING(name), pErrorEntity );
		info.massScale[index] = 1.0f;
	}
}

void CPhysConstraint::OnConstraintSetup( hl_constraint_info_t &info )
{
	if ( info.pObjects[0] && info.pObjects[1] )
	{
		SetupTeleportationHandling( info );
	}
	if ( m_spawnflags & SF_CONSTRAINT_DISABLE_COLLISION )
	{
		PhysDisableEntityCollisions( info.pObjects[0], info.pObjects[1] );
	}
}

void CPhysConstraint::SetupTeleportationHandling( hl_constraint_info_t &info )
{
	CBaseEntity *pEntity0 = (CBaseEntity *)info.pObjects[0]->GetGameData();
	if ( pEntity0 )
	{
		g_pNotify->AddEntity( this, pEntity0 );
	}

	CBaseEntity *pEntity1 = (CBaseEntity *)info.pObjects[1]->GetGameData();
	if ( pEntity1 )
	{
		g_pNotify->AddEntity( this, pEntity1 );
	}
}

static IPhysicsConstraintGroup *GetRagdollConstraintGroup( IPhysicsObject *pObj )
{
	if ( pObj )
	{
		CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObj->GetGameData());
		ragdoll_t *pRagdoll = Ragdoll_GetRagdoll(pEntity);
		if ( pRagdoll )
			return pRagdoll->pGroup;
	}
	return NULL;
}

void CPhysConstraint::GetConstraintObjects( hl_constraint_info_t &info )
{
	FindPhysicsAnchor( m_nameAttach1, info, 0, this );
	FindPhysicsAnchor( m_nameAttach2, info, 1, this );

	// Missing one object, assume the world instead
	if ( info.pObjects[0] == NULL && info.pObjects[1] )
	{
		if ( Q_strlen(STRING(m_nameAttach1)) )
		{
			Warning("Bogus constraint %s (attaches ENTITY NOT FOUND:%s to %s)\n", GetDebugName(), STRING(m_nameAttach1), STRING(m_nameAttach2));
#ifdef HL2_EPISODIC
			info.pObjects[0] = info.pObjects[1] = NULL;
			return;
#endif	// HL2_EPISODIC
		}
		info.pObjects[0] = g_PhysWorldObject;
		info.massScale[0] = info.massScale[1] = 1.0f; // no mass scale on world constraint
	}
	else if ( info.pObjects[0] && !info.pObjects[1] )
	{
		if ( Q_strlen(STRING(m_nameAttach2)) )
		{
			Warning("Bogus constraint %s (attaches %s to ENTITY NOT FOUND:%s)\n", GetDebugName(), STRING(m_nameAttach1), STRING(m_nameAttach2));
#ifdef HL2_EPISODIC
			info.pObjects[0] = info.pObjects[1] = NULL;
			return;
#endif	// HL2_EPISODIC
		}
		info.pObjects[1] = info.pObjects[0];
		info.pObjects[0] = g_PhysWorldObject;		// Try to make the world object consistently object0 for ease of implementation
		info.massScale[0] = info.massScale[1] = 1.0f; // no mass scale on world constraint
		info.swapped = true;
	}

	info.pGroup = GetRagdollConstraintGroup(info.pObjects[0]);
	if ( !info.pGroup )
	{
		info.pGroup = GetRagdollConstraintGroup(info.pObjects[1]);
	}
}

void CPhysConstraint::Activate( void )
{
	BaseClass::Activate();

	if ( HasSpawnFlags( SF_CONSTRAINT_NO_CONNECT_UNTIL_ACTIVATED ) == false )
	{
		if ( !ActivateConstraint() )
		{
			UTIL_Remove(this);
		}
	}
}

IPhysicsConstraintGroup *GetConstraintGroup( string_t systemName )
{
	CBaseEntity *pMachine = gEntList.FindEntityByName( NULL, systemName );

	if ( pMachine )
	{
		CPhysConstraintSystem *pGroup = dynamic_cast<CPhysConstraintSystem *>(pMachine);
		if ( pGroup )
		{
			return pGroup->GetVPhysicsGroup();
		}
	}
	return NULL;
}

bool CPhysConstraint::ActivateConstraint( void )
{
	// A constraint attaches two objects to each other.
	// The constraint is specified in the coordinate frame of the "reference" object
	// and constrains the "attached" object
	hl_constraint_info_t info;
	if ( m_pConstraint )
	{
		// already have a constraint, don't make a new one
		info.pObjects[0] = m_pConstraint->GetReferenceObject();
		info.pObjects[1] = m_pConstraint->GetAttachedObject();
		OnConstraintSetup(info);
		return true;
	}

	GetConstraintObjects( info );
	if ( !info.pObjects[0] && !info.pObjects[1] )
		return false;

	if ( info.pObjects[0]->IsStatic() && info.pObjects[1]->IsStatic() )
	{
		Warning("Constraint (%s) attached to two static objects (%s and %s)!!!\n", STRING(GetEntityName()), STRING(m_nameAttach1), m_nameAttach2 == NULL_STRING ? "world" : STRING(m_nameAttach2) );
		return false;
	}

	if ( info.pObjects[0]->GetShadowController() && info.pObjects[1]->GetShadowController() )
	{
		Warning("Constraint (%s) attached to two shadow objects (%s and %s)!!!\n", STRING(GetEntityName()), STRING(m_nameAttach1), m_nameAttach2 == NULL_STRING ? "world" : STRING(m_nameAttach2) );
		return false;
	}
	IPhysicsConstraintGroup *pGroup = GetConstraintGroup( m_nameSystem );
	if ( !pGroup )
	{
		pGroup = info.pGroup;
	}
	m_pConstraint = CreateConstraint( pGroup, info );
	if ( !m_pConstraint )
		return false;

	m_pConstraint->SetGameData( (void *)this );

	if ( pGroup )
	{
		pGroup->Activate();
	}

	OnConstraintSetup(info);

	return true;
}

void CPhysConstraint::NotifySystemEvent( CBaseEntity *pNotify, notify_system_event_t eventType, const notify_system_event_params_t &params )
{
	// don't recurse
	if ( eventType != NOTIFY_EVENT_TELEPORT || (unsigned int)gpGlobals->tickcount == m_teleportTick )
		return;

	float distance = (params.pTeleport->prevOrigin - pNotify->GetAbsOrigin()).Length();
	
	// no need to follow a small teleport
	if ( distance <= m_minTeleportDistance )
		return;

	m_teleportTick = gpGlobals->tickcount;

	PhysTeleportConstrainedEntity( pNotify, m_pConstraint->GetReferenceObject(), m_pConstraint->GetAttachedObject(), params.pTeleport->prevOrigin, params.pTeleport->prevAngles, params.pTeleport->physicsRotate );
}

class CPhysHinge : public CPhysConstraint, public IVPhysicsWatcher
{
	DECLARE_CLASS( CPhysHinge, CPhysConstraint );

public:
	void Spawn( void );
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		if ( m_hinge.worldAxisDirection == vec3_origin )
		{
			DevMsg("ERROR: Hinge with bad data!!!\n" );
			return NULL;
		}
		GetBreakParams( m_hinge.constraint, info );
		m_hinge.constraint.strength = 1.0;
		// BUGBUG: These numbers are very hard to edit
		// Scale by 1000 to make things easier
		// CONSIDER: Unify the units of torque around something other 
		// than HL units (kg * in^2 / s ^2)
		m_hinge.hingeAxis.SetAxisFriction( 0, 0, m_hingeFriction * 1000 );

		int hingeAxis;
		if ( IsWorldHinge( info, &hingeAxis ) )
		{
			info.pObjects[1]->BecomeHinged( hingeAxis );
		}
		else
		{
			RemoveSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY );
		}

		return physenv->CreateHingeConstraint( info.pObjects[0], info.pObjects[1], pGroup, m_hinge );
	}

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line(m_hinge.worldPosition, m_hinge.worldPosition + 48 * m_hinge.worldAxisDirection, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	void InputSetVelocity( inputdata_t &inputdata )
	{
		if ( !m_pConstraint || !m_pConstraint->GetReferenceObject() || !m_pConstraint->GetAttachedObject() )
			return;
	
		float speed = inputdata.value.Float();
		float massLoad = 1;
		int numMasses = 0;
		if ( m_pConstraint->GetReferenceObject()->IsMoveable() )
		{
			massLoad = m_pConstraint->GetReferenceObject()->GetInertia().Length();
			numMasses++;
			m_pConstraint->GetReferenceObject()->Wake();
		}
		if ( m_pConstraint->GetAttachedObject()->IsMoveable() )
		{
			massLoad += m_pConstraint->GetAttachedObject()->GetInertia().Length();
			numMasses++;
			m_pConstraint->GetAttachedObject()->Wake();
		}
		if ( numMasses > 0 )
		{
			massLoad /= (float)numMasses;
		}
		
		float loadscale = m_systemLoadScale != 0 ? m_systemLoadScale : 1;
		m_pConstraint->SetAngularMotor( speed, speed * loadscale * massLoad * loadscale * (1.0/TICK_INTERVAL) );
	}

	void InputSetHingeFriction( inputdata_t &inputdata )
	{
		m_hingeFriction = inputdata.value.Float();
		Msg("Setting hinge friction to %f\n", m_hingeFriction );
		m_hinge.hingeAxis.SetAxisFriction( 0, 0, m_hingeFriction * 1000 );
	}

	virtual void Deactivate()
	{
		if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) )
		{
			if ( m_pConstraint && m_pConstraint->GetAttachedObject() )
			{
				// NOTE: RemoveHinged() is always safe
				m_pConstraint->GetAttachedObject()->RemoveHinged();
			}
		}

		BaseClass::Deactivate();
	}
	
	void NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake )
	{
#if HINGE_NOTIFY
		Assert(m_pConstraint);
		if (!m_pConstraint) 
			return;

		// if something woke up, start thinking. If everything is asleep, stop thinking.
		if ( bAwake )
		{
			// Did something wake up when I was not thinking?
			if ( GetNextThink() == TICK_NEVER_THINK )
			{
				m_soundInfo.StartThinking(this, 
					VelocitySampler::GetRelativeAngularVelocity(m_pConstraint->GetAttachedObject(), m_pConstraint->GetReferenceObject()) ,
					m_hinge.worldAxisDirection
					);

				SetThink(&CPhysHinge::SoundThink);
				SetNextThink(gpGlobals->curtime + m_soundInfo.getThinkRate());
			}
		}
		else
		{
			// Is everything asleep? If so, stop thinking.
			if ( GetNextThink() != TICK_NEVER_THINK				&&
				m_pConstraint->GetAttachedObject()->IsAsleep() &&
				m_pConstraint->GetReferenceObject()->IsAsleep() )
			{
				m_soundInfo.StopThinking(this);
				SetNextThink(TICK_NEVER_THINK);
			}
		}
#endif
	}


#if HINGE_NOTIFY
	virtual void OnConstraintSetup( hl_constraint_info_t &info )
	{
		CBaseEntity *pEntity0 = info.pObjects[0] ? static_cast<CBaseEntity *>(info.pObjects[0]->GetGameData()) : NULL;
		if ( pEntity0 && !info.pObjects[0]->IsStatic()  )
		{
			WatchVPhysicsStateChanges( this, pEntity0 );
		}
		CBaseEntity *pEntity1 = info.pObjects[1] ? static_cast<CBaseEntity *>(info.pObjects[1]->GetGameData()) : NULL;
		if ( pEntity1 && !info.pObjects[1]->IsStatic()  )
		{
			WatchVPhysicsStateChanges( this, pEntity1 );
		}
		BaseClass::OnConstraintSetup(info);
	}

	void SoundThink( void );
	// void Spawn( void );
	void Activate( void );
	void Precache( void );
#endif

	DECLARE_DATADESC();


#if HINGE_NOTIFY
protected:
	ConstraintSoundInfo m_soundInfo;
#endif

private:
	constraint_hingeparams_t m_hinge;
	float m_hingeFriction;
	float	m_systemLoadScale;
	bool IsWorldHinge( const hl_constraint_info_t &info, int *pAxisOut );
};

BEGIN_DATADESC( CPhysHinge )

// Quiet down classcheck
//	DEFINE_FIELD( m_hinge, FIELD_??? ),

	DEFINE_KEYFIELD( m_hingeFriction, FIELD_FLOAT, "hingefriction" ),
	DEFINE_FIELD( m_hinge.worldPosition, FIELD_POSITION_VECTOR ),
	DEFINE_KEYFIELD( m_hinge.worldAxisDirection, FIELD_VECTOR, "hingeaxis" ),
	DEFINE_KEYFIELD( m_systemLoadScale, FIELD_FLOAT, "systemloadscale" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAngularVelocity", InputSetVelocity ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetHingeFriction", InputSetHingeFriction ),

#if HINGE_NOTIFY
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_THRESHOLD] , FIELD_FLOAT, "minSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_FULL] , FIELD_FLOAT, "maxSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundFwd, FIELD_SOUNDNAME, "slidesoundfwd" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundBack, FIELD_SOUNDNAME, "slidesoundback" ),

	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[0], FIELD_SOUNDNAME, "reversalsoundSmall" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[1], FIELD_SOUNDNAME, "reversalsoundMedium" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[2], FIELD_SOUNDNAME, "reversalsoundLarge" ),

	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[0] , FIELD_FLOAT, "reversalsoundthresholdSmall" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[1], FIELD_FLOAT, "reversalsoundthresholdMedium" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[2] , FIELD_FLOAT, "reversalsoundthresholdLarge" ),

	DEFINE_THINKFUNC( SoundThink ),
#endif

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_hinge, CPhysHinge );


void CPhysHinge::Spawn( void )
{
	m_hinge.worldPosition = GetLocalOrigin();
	m_hinge.worldAxisDirection -= GetLocalOrigin();
	VectorNormalize(m_hinge.worldAxisDirection);
	UTIL_SnapDirectionToAxis( m_hinge.worldAxisDirection );

	m_hinge.hingeAxis.SetAxisFriction( 0, 0, 0 );

	if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) )
	{
		masscenteroverride_t params;
		if ( m_nameAttach1 == NULL_STRING )
		{
			params.SnapToAxis( m_nameAttach2, m_hinge.worldPosition, m_hinge.worldAxisDirection );
			PhysSetMassCenterOverride( params );
		}
		else if ( m_nameAttach2 == NULL_STRING )
		{
			params.SnapToAxis( m_nameAttach1, m_hinge.worldPosition, m_hinge.worldAxisDirection );
			PhysSetMassCenterOverride( params );
		}
		else
		{
			RemoveSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY );
		}
	}

	Precache();
}

#if HINGE_NOTIFY
void CPhysHinge::Activate( void )
{
	BaseClass::Activate();

	m_soundInfo.OnActivate(this);
	if (m_pConstraint)
	{
		m_soundInfo.StartThinking(this, 
			VelocitySampler::GetRelativeAngularVelocity(m_pConstraint->GetAttachedObject(), m_pConstraint->GetReferenceObject()) ,
			m_hinge.worldAxisDirection
			);

		SetThink(&CPhysHinge::SoundThink);
		SetNextThink( gpGlobals->curtime + m_soundInfo.getThinkRate() );
	}
}

void CPhysHinge::Precache( void )
{
	BaseClass::Precache();
	return m_soundInfo.OnPrecache(this);
}

#endif


static int GetUnitAxisIndex( const Vector &axis )
{
	bool valid = false;
	int index = -1;

	for ( int i = 0; i < 3; i++ )
	{
		if ( axis[i] != 0 )
		{
			if ( fabs(axis[i]) == 1 )
			{
				if ( index  < 0 )
				{
					index = i;
					valid = true;
					continue;
				}
			}
			valid = false;
		}
	}
	return valid ? index : -1;
}

bool CPhysHinge::IsWorldHinge( const hl_constraint_info_t &info, int *pAxisOut )
{
	if ( HasSpawnFlags( SF_CONSTRAINT_ASSUME_WORLD_GEOMETRY ) && info.pObjects[0] == g_PhysWorldObject )
	{
		Vector localHinge;
		info.pObjects[1]->WorldToLocalVector( &localHinge, m_hinge.worldAxisDirection );
		UTIL_SnapDirectionToAxis( localHinge );
		int hingeAxis = GetUnitAxisIndex( localHinge );
		if ( hingeAxis >= 0 )
		{
			*pAxisOut = hingeAxis;
			return true;
		}
	}
	return false;
}


#if HINGE_NOTIFY
void CPhysHinge::SoundThink( void )
{
	Assert(m_pConstraint);
	if (!m_pConstraint)
		return;

	IPhysicsObject * pAttached = m_pConstraint->GetAttachedObject(), *pReference = m_pConstraint->GetReferenceObject();
	Assert( pAttached && pReference );
	if (pAttached && pReference)
	{
		Vector relativeVel = VelocitySampler::GetRelativeAngularVelocity(pAttached,pReference);
		if (g_debug_constraint_sounds.GetBool())
		{
			NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + (relativeVel), 255, 255, 0, true, 0.1f );
		}
		m_soundInfo.OnThink( this, relativeVel );

		SetNextThink(gpGlobals->curtime + m_soundInfo.getThinkRate());
	}
}
#endif

class CPhysBallSocket : public CPhysConstraint
{
public:
	DECLARE_CLASS( CPhysBallSocket, CPhysConstraint );

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
	{
		constraint_ballsocketparams_t ballsocket;
	
		ballsocket.Defaults();
		
		for ( int i = 0; i < 2; i++ )
		{
			info.pObjects[i]->WorldToLocal( &ballsocket.constraintPosition[i], GetAbsOrigin() );
		}
		GetBreakParams( ballsocket.constraint, info );
		ballsocket.constraint.torqueLimit = 0;

		return physenv->CreateBallsocketConstraint( info.pObjects[0], info.pObjects[1], pGroup, ballsocket );
	}
};

LINK_ENTITY_TO_CLASS( phys_ballsocket, CPhysBallSocket );

class CPhysSlideConstraint : public CPhysConstraint, public IVPhysicsWatcher
{
public:
	DECLARE_CLASS( CPhysSlideConstraint, CPhysConstraint );

	DECLARE_DATADESC();
	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );
	void InputSetVelocity( inputdata_t &inputdata )
	{
		if ( !m_pConstraint || !m_pConstraint->GetReferenceObject() || !m_pConstraint->GetAttachedObject() )
			return;

		float speed = inputdata.value.Float();
		float massLoad = 1;
		int numMasses = 0;
		if ( m_pConstraint->GetReferenceObject()->IsMoveable() )
		{
			massLoad = m_pConstraint->GetReferenceObject()->GetMass();
			numMasses++;
			m_pConstraint->GetReferenceObject()->Wake();
		}
		if ( m_pConstraint->GetAttachedObject()->IsMoveable() )
		{
			massLoad += m_pConstraint->GetAttachedObject()->GetMass();
			numMasses++;
			m_pConstraint->GetAttachedObject()->Wake();
		}
		if ( numMasses > 0 )
		{
			massLoad /= (float)numMasses;
		}
		float loadscale = m_systemLoadScale != 0 ? m_systemLoadScale : 1;
		m_pConstraint->SetLinearMotor( speed, speed * loadscale * massLoad * (1.0/TICK_INTERVAL) );
	}

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Box( GetAbsOrigin(), -Vector(8,8,8), Vector(8,8,8), 0, 255, 0, 0, 0 );
			NDebugOverlay::Box( m_axisEnd, -Vector(4,4,4), Vector(4,4,4), 0, 0, 255, 0, 0 );
			NDebugOverlay::Line( GetAbsOrigin(), m_axisEnd, 255, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	void NotifyVPhysicsStateChanged( IPhysicsObject *pPhysics, CBaseEntity *pEntity, bool bAwake )
	{
#if HINGE_NOTIFY
		Assert(m_pConstraint);
		if (!m_pConstraint) 
			return;

		// if something woke up, start thinking. If everything is asleep, stop thinking.
		if ( bAwake )
		{
			// Did something wake up when I was not thinking?
			if ( GetNextThink() == TICK_NEVER_THINK )
			{
				Vector axisDirection = m_axisEnd - GetAbsOrigin();
				VectorNormalize( axisDirection );
				UTIL_SnapDirectionToAxis( axisDirection );

				m_soundInfo.StartThinking(this, 
					VelocitySampler::GetRelativeVelocity(m_pConstraint->GetAttachedObject(), m_pConstraint->GetReferenceObject()),
					axisDirection
					);
				SetThink(&CPhysSlideConstraint::SoundThink);
				SetNextThink(gpGlobals->curtime + m_soundInfo.getThinkRate());
			}
		}
		else
		{
			// Is everything asleep? If so, stop thinking.
			if ( GetNextThink() != TICK_NEVER_THINK				&&
				 m_pConstraint->GetAttachedObject()->IsAsleep() &&
				 m_pConstraint->GetReferenceObject()->IsAsleep() )
			{
				m_soundInfo.StopThinking(this);
				SetNextThink(TICK_NEVER_THINK);
			}
		}
#endif
	}


#if HINGE_NOTIFY
	virtual void OnConstraintSetup( hl_constraint_info_t &info )
	{
		CBaseEntity *pEntity0 = info.pObjects[0] ? static_cast<CBaseEntity *>(info.pObjects[0]->GetGameData()) : NULL;
		if ( pEntity0 && !info.pObjects[0]->IsStatic()  )
		{
			WatchVPhysicsStateChanges( this, pEntity0 );
		}
		CBaseEntity *pEntity1 = info.pObjects[1] ? static_cast<CBaseEntity *>(info.pObjects[1]->GetGameData()) : NULL;
		if ( pEntity1 && !info.pObjects[1]->IsStatic()  )
		{
			WatchVPhysicsStateChanges( this, pEntity1 );
		}
		BaseClass::OnConstraintSetup(info);
	}


	void SoundThink( void );
	// void Spawn( void );
	void Activate( void );
	void Precache( void );
#endif

	Vector	m_axisEnd;
	float	m_slideFriction;
	float	m_systemLoadScale;

#if HINGE_NOTIFY
protected:
	ConstraintSoundInfo m_soundInfo;
#endif
};

LINK_ENTITY_TO_CLASS( phys_slideconstraint, CPhysSlideConstraint );

BEGIN_DATADESC( CPhysSlideConstraint )

	DEFINE_KEYFIELD( m_axisEnd, FIELD_POSITION_VECTOR, "slideaxis" ),
	DEFINE_KEYFIELD( m_slideFriction, FIELD_FLOAT, "slidefriction" ),
	DEFINE_KEYFIELD( m_systemLoadScale, FIELD_FLOAT, "systemloadscale" ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVelocity", InputSetVelocity ),
#if HINGE_NOTIFY
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_THRESHOLD] , FIELD_FLOAT, "minSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_keyPoints[SimpleConstraintSoundProfile::kMIN_FULL] , FIELD_FLOAT, "maxSoundThreshold" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundFwd, FIELD_SOUNDNAME, "slidesoundfwd" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszTravelSoundBack, FIELD_SOUNDNAME, "slidesoundback" ),

	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[0], FIELD_SOUNDNAME, "reversalsoundSmall" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[1], FIELD_SOUNDNAME, "reversalsoundMedium" ),
	DEFINE_KEYFIELD( m_soundInfo.m_iszReversalSounds[2], FIELD_SOUNDNAME, "reversalsoundLarge" ),

	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[0] , FIELD_FLOAT, "reversalsoundthresholdSmall" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[1], FIELD_FLOAT, "reversalsoundthresholdMedium" ),
	DEFINE_KEYFIELD( m_soundInfo.m_soundProfile.m_reversalSoundThresholds[2] , FIELD_FLOAT, "reversalsoundthresholdLarge" ),


	DEFINE_THINKFUNC( SoundThink ),
#endif

END_DATADESC()



IPhysicsConstraint *CPhysSlideConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_slidingparams_t sliding;
	sliding.Defaults();
	GetBreakParams( sliding.constraint, info );
	sliding.constraint.strength = 1.0;

	Vector axisDirection = m_axisEnd - GetAbsOrigin();
	VectorNormalize( axisDirection );
	UTIL_SnapDirectionToAxis( axisDirection );

	sliding.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1], axisDirection );
	sliding.friction = m_slideFriction;
	if ( m_spawnflags & SF_SLIDE_LIMIT_ENDS )
	{
		Vector position;
		info.pObjects[1]->GetPosition( &position, NULL );

		sliding.limitMin = DotProduct( axisDirection, GetAbsOrigin() );
		sliding.limitMax = DotProduct( axisDirection, m_axisEnd );
		if ( sliding.limitMax < sliding.limitMin )
		{
			::V_swap( sliding.limitMin, sliding.limitMax );
		}

		// expand limits to make initial position of the attached object valid
		float limit = DotProduct( position, axisDirection );
		if ( limit < sliding.limitMin )
		{
			sliding.limitMin = limit;
		}
		else if ( limit > sliding.limitMax )
		{
			sliding.limitMax = limit;
		}
		// offset so that the current position is 0
		sliding.limitMin -= limit;
		sliding.limitMax -= limit;
	}

	return physenv->CreateSlidingConstraint( info.pObjects[0], info.pObjects[1], pGroup, sliding );
}


#if HINGE_NOTIFY
void CPhysSlideConstraint::SoundThink( void )
{
	Assert(m_pConstraint);
	if (!m_pConstraint)
		return;

	IPhysicsObject * pAttached = m_pConstraint->GetAttachedObject(), *pReference = m_pConstraint->GetReferenceObject();
	Assert( pAttached && pReference );
	if (pAttached && pReference)
	{
		Vector relativeVel = VelocitySampler::GetRelativeVelocity(pAttached,pReference);
		// project velocity onto my primary axis.:

		Vector axisDirection = m_axisEnd - GetAbsOrigin();
		relativeVel = m_axisEnd * relativeVel.Dot(m_axisEnd)/m_axisEnd.Dot(m_axisEnd);

		m_soundInfo.OnThink( this, relativeVel );

		SetNextThink(gpGlobals->curtime + m_soundInfo.getThinkRate());
	}

}

void CPhysSlideConstraint::Activate( void )
{
	BaseClass::Activate();

	m_soundInfo.OnActivate(this);

	Vector axisDirection = m_axisEnd - GetAbsOrigin();
	VectorNormalize( axisDirection );
	UTIL_SnapDirectionToAxis( axisDirection );
	m_soundInfo.StartThinking(this, 
		VelocitySampler::GetRelativeVelocity(m_pConstraint->GetAttachedObject(), m_pConstraint->GetReferenceObject()),
		axisDirection
		);

	SetThink(&CPhysSlideConstraint::SoundThink);
	SetNextThink(gpGlobals->curtime + m_soundInfo.getThinkRate());
}

void CPhysSlideConstraint::Precache()
{
	m_soundInfo.OnPrecache(this);
}

#endif



LINK_ENTITY_TO_CLASS( phys_constraint, CPhysFixed );

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysFixed::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_fixedparams_t fixed;
	fixed.Defaults();
	fixed.InitWithCurrentObjectState( info.pObjects[0], info.pObjects[1] );
	GetBreakParams( fixed.constraint, info );

	// constraining to the world means object 1 is fixed
	if ( info.pObjects[0] == g_PhysWorldObject )
	{
		PhysSetGameFlags( info.pObjects[1], FVPHYSICS_CONSTRAINT_STATIC );
	}

	return physenv->CreateFixedConstraint( info.pObjects[0], info.pObjects[1], pGroup, fixed );
}


//-----------------------------------------------------------------------------
// Purpose: Breakable pulley w/ropes constraint
//-----------------------------------------------------------------------------
class CPhysPulley : public CPhysConstraint
{
	DECLARE_CLASS( CPhysPulley, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			NDebugOverlay::Line( refPos, origin, 0, 255, 0, false, 0 );
			NDebugOverlay::Line( origin, m_position2, 128, 128, 128, false, 0 );
			NDebugOverlay::Line( m_position2, attachPos, 0, 255, 0, false, 0 );
			NDebugOverlay::Box( origin, -Vector(8,8,8), Vector(8,8,8), 128, 255, 128, 32, 0 );
			NDebugOverlay::Box( m_position2, -Vector(8,8,8), Vector(8,8,8), 255, 128, 128, 32, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_position2;
	Vector		m_offset[2];
	float		m_addLength;
	float		m_gearRatio;
};

BEGIN_DATADESC( CPhysPulley )

	DEFINE_KEYFIELD( m_position2, FIELD_POSITION_VECTOR, "position2" ),
	DEFINE_AUTO_ARRAY( m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( m_gearRatio, FIELD_FLOAT, "gearratio" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_pulleyconstraint, CPhysPulley );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysPulley::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_pulleyparams_t pulley;
	pulley.Defaults();
	pulley.pulleyPosition[0] = GetAbsOrigin();
	pulley.pulleyPosition[1] = m_position2;

	matrix3x4_t matrix;
	Vector world[2];

	info.pObjects[0]->GetPositionMatrix( &matrix );
	VectorTransform( info.anchorPosition[0], matrix, world[0] );
	info.pObjects[1]->GetPositionMatrix( &matrix );
	VectorTransform( info.anchorPosition[1], matrix, world[1] );

	for ( int i = 0; i < 2; i++ )
	{
		pulley.objectPosition[i] = info.anchorPosition[i];
		m_offset[i] = info.anchorPosition[i];
	}
	
	pulley.totalLength = m_addLength + 
		(world[0] - pulley.pulleyPosition[0]).Length() + 
		((world[1] - pulley.pulleyPosition[1]).Length() * m_gearRatio);

	if ( m_gearRatio != 0 )
	{
		pulley.gearRatio = m_gearRatio;
	}
	GetBreakParams( pulley.constraint, info );
	if ( m_spawnflags & SF_PULLEY_RIGID )
	{
		pulley.isRigid = true;
	}

	return physenv->CreatePulleyConstraint( info.pObjects[0], info.pObjects[1], pGroup, pulley );
}

//-----------------------------------------------------------------------------
// Purpose: Breakable rope/length constraint
//-----------------------------------------------------------------------------
class CPhysLength : public CPhysConstraint
{
	DECLARE_CLASS( CPhysLength, CPhysConstraint );
public:
	DECLARE_DATADESC();

	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			Vector origin = GetAbsOrigin();
			Vector refPos = origin, attachPos = origin;
			IPhysicsObject *pRef = m_pConstraint->GetReferenceObject();
			if ( pRef )
			{
				matrix3x4_t matrix;
				pRef->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[0], matrix, refPos );
			}
			IPhysicsObject *pAttach = m_pConstraint->GetAttachedObject();
			if ( pAttach )
			{
				matrix3x4_t matrix;
				pAttach->GetPositionMatrix( &matrix );
				VectorTransform( m_offset[1], matrix, attachPos );
			}
			Vector dir = attachPos - refPos;

			float len = VectorNormalize(dir);
			if ( len > m_totalLength )
			{
				Vector mid = refPos + dir * m_totalLength;
				NDebugOverlay::Line( refPos, mid, 0, 255, 0, false, 0 );
				NDebugOverlay::Line( mid, attachPos, 255, 0, 0, false, 0 );
			}
			else
			{
				NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
			}
		}
		BaseClass::DrawDebugGeometryOverlays();
	}

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	Vector		m_offset[2];
	Vector		m_vecAttach;
	float		m_addLength;
	float		m_minLength;
	float		m_totalLength;
};

BEGIN_DATADESC( CPhysLength )

	DEFINE_AUTO_ARRAY( m_offset, FIELD_VECTOR ),
	DEFINE_KEYFIELD( m_addLength, FIELD_FLOAT, "addlength" ),
	DEFINE_KEYFIELD( m_minLength, FIELD_FLOAT, "minlength" ),
	DEFINE_KEYFIELD( m_vecAttach, FIELD_POSITION_VECTOR, "attachpoint" ),
	DEFINE_FIELD( m_totalLength, FIELD_FLOAT ),
END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_lengthconstraint, CPhysLength );


//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CPhysLength::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_lengthparams_t length;
	length.Defaults();
	Vector position[2];
	position[0] = GetAbsOrigin();
	position[1] = m_vecAttach;
	int index = info.swapped ? 1 : 0;
	length.InitWorldspace( info.pObjects[0], info.pObjects[1], position[index], position[!index] );
	length.totalLength += m_addLength;
	length.minLength = m_minLength;
	m_totalLength = length.totalLength;
	if ( HasSpawnFlags(SF_LENGTH_RIGID) )
	{
		length.minLength = length.totalLength;
	}

	for ( int i = 0; i < 2; i++ )
	{
		m_offset[i] = length.objectPosition[i];
	}
	GetBreakParams( length.constraint, info );

	return physenv->CreateLengthConstraint( info.pObjects[0], info.pObjects[1], pGroup, length );
}

//-----------------------------------------------------------------------------
// Purpose: Limited ballsocket constraint with toggle-able translation constraints
//-----------------------------------------------------------------------------
class CRagdollConstraint : public CPhysConstraint
{
	DECLARE_CLASS( CRagdollConstraint, CPhysConstraint );
public:
	DECLARE_DATADESC();
#if 0
	void DrawDebugGeometryOverlays()
	{
		if ( m_debugOverlays & (OVERLAY_BBOX_BIT|OVERLAY_PIVOT_BIT|OVERLAY_ABSBOX_BIT) )
		{
			NDebugOverlay::Line( refPos, attachPos, 0, 255, 0, false, 0 );
		}
		BaseClass::DrawDebugGeometryOverlays();
	}
#endif

	IPhysicsConstraint *CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info );

private:
	float		m_xmin;	// constraint limits in degrees
	float		m_xmax;
	float		m_ymin;
	float		m_ymax;
	float		m_zmin;
	float		m_zmax;

	float		m_xfriction;
	float		m_yfriction;
	float		m_zfriction;
};

BEGIN_DATADESC( CRagdollConstraint )

	DEFINE_KEYFIELD( m_xmin, FIELD_FLOAT, "xmin" ),
	DEFINE_KEYFIELD( m_xmax, FIELD_FLOAT, "xmax" ),
	DEFINE_KEYFIELD( m_ymin, FIELD_FLOAT, "ymin" ),
	DEFINE_KEYFIELD( m_ymax, FIELD_FLOAT, "ymax" ),
	DEFINE_KEYFIELD( m_zmin, FIELD_FLOAT, "zmin" ),
	DEFINE_KEYFIELD( m_zmax, FIELD_FLOAT, "zmax" ),
	DEFINE_KEYFIELD( m_xfriction, FIELD_FLOAT, "xfriction" ),
	DEFINE_KEYFIELD( m_yfriction, FIELD_FLOAT, "yfriction" ),
	DEFINE_KEYFIELD( m_zfriction, FIELD_FLOAT, "zfriction" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( phys_ragdollconstraint, CRagdollConstraint );

//-----------------------------------------------------------------------------
// Purpose: Activate/create the constraint
//-----------------------------------------------------------------------------
IPhysicsConstraint *CRagdollConstraint::CreateConstraint( IPhysicsConstraintGroup *pGroup, const hl_constraint_info_t &info )
{
	constraint_ragdollparams_t ragdoll;
	ragdoll.Defaults();

	matrix3x4_t entityToWorld, worldToEntity;
	info.pObjects[0]->GetPositionMatrix( &entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToReference );

	info.pObjects[1]->GetPositionMatrix( &entityToWorld );
	MatrixInvert( entityToWorld, worldToEntity );
	ConcatTransforms( worldToEntity, EntityToWorldTransform(), ragdoll.constraintToAttached );

	ragdoll.onlyAngularLimits = HasSpawnFlags( SF_RAGDOLL_FREEMOVEMENT ) ? true : false;

	// FIXME: Why are these friction numbers in different units from what the hinge uses?
	ragdoll.axes[0].SetAxisFriction( m_xmin, m_xmax, m_xfriction );
	ragdoll.axes[1].SetAxisFriction( m_ymin, m_ymax, m_yfriction );
	ragdoll.axes[2].SetAxisFriction( m_zmin, m_zmax, m_zfriction );

	if ( HasSpawnFlags( SF_CONSTRAINT_START_INACTIVE ) )
	{
		ragdoll.isActive = false;
	}
	return physenv->CreateRagdollConstraint( info.pObjects[0], info.pObjects[1], pGroup, ragdoll );
}



class CPhysConstraintEvents : public IPhysicsConstraintEvent
{
	void ConstraintBroken( IPhysicsConstraint *pConstraint )
	{
		CBaseEntity *pEntity = (CBaseEntity *)pConstraint->GetGameData();
		if ( pEntity )
		{
			IPhysicsConstraintEvent *pConstraintEvent = dynamic_cast<IPhysicsConstraintEvent*>( pEntity );
			//Msg("Constraint broken %s\n", pEntity->GetDebugName() );
			if ( pConstraintEvent )
			{
				pConstraintEvent->ConstraintBroken( pConstraint );
			}
			else
			{
				variant_t emptyVariant;
				pEntity->AcceptInput( "ConstraintBroken", NULL, NULL, emptyVariant, 0 );
			}
		}
	}
};

static CPhysConstraintEvents constraintevents;
// registered in physics.cpp
IPhysicsConstraintEvent *g_pConstraintEvents = &constraintevents;





#if HINGE_NOTIFY
//-----------------------------------------------------------------------------
// Code for sampler
//-----------------------------------------------------------------------------


/// Call this in spawn(). (Not a constructor because those are difficult to use in entities.)
void VelocitySampler::Initialize(float samplerate)
{
	m_fIdealSampleRate = samplerate;
}

// This is an old style approach to reversal sounds, from when there was only one.
#if 0
bool VelocitySampler::HasReversed(const Vector &relativeVelocity, float thresholdAcceleration)
{
	// first, make sure the velocity has reversed (is more than 90deg off) from last time, or is zero now.
	// float rVsq = relativeVelocity.LengthSqr();
	float vDot = relativeVelocity.Dot(m_prevSample);
	if (vDot <= 0) // there is a reversal in direction. compute the magnitude of acceleration.
	{
		// find the scalar projection of the relative acceleration this fame onto the previous frame's
		// velocity, and compare that to the threshold. 
		Vector accel = relativeVelocity - m_prevSample;

		float prevSampleLength = m_prevSample.Length();
		float projection = 0;
		// divide through by dt to get the accel per sec
		if (prevSampleLength)
		{
			projection = -(accel.Dot(m_prevSample) / prevSampleLength) / (gpGlobals->curtime - m_fPrevSampleTime);
		}
		else
		{
			projection = accel.Length() / (gpGlobals->curtime - m_fPrevSampleTime);
		}

		if (g_debug_constraint_sounds.GetBool())
		{
			Msg("Reversal accel is %f/%f\n",projection,thresholdAcceleration);
		}
		return ((projection) > thresholdAcceleration); // the scalar projection is negative because the acceleration is against vel
	}
	else
	{
		return false;
	}
}
#endif

/// Looks at the force of reversal and compares it to a ladder of thresholds.
/// Returns the index of the highest threshold exceeded by the reversal velocity. 
int VelocitySampler::HasReversed(const Vector &relativeVelocity, const float thresholdAcceleration[], const unsigned short numThresholds)
{
	// first, make sure the velocity has reversed (is more than 90deg off) from last time, or is zero now.
	// float rVsq = relativeVelocity.LengthSqr();
	float vDot = relativeVelocity.Dot(m_prevSample);
	if (vDot <= 0) // there is a reversal in direction. compute the magnitude of acceleration.
	{
		// find the scalar projection of the relative acceleration this fame onto the previous frame's
		// velocity, and compare that to the threshold. 
		Vector accel = relativeVelocity - m_prevSample;

		float prevSampleLength = m_prevSample.Length();
		float projection = 0;
		// divide through by dt to get the accel per sec
		if (prevSampleLength)
		{
			// the scalar projection is negative because the acceleration is against vel
			projection = -(accel.Dot(m_prevSample) / prevSampleLength) / (gpGlobals->curtime - m_fPrevSampleTime);
		}
		else
		{
			projection = accel.Length() / (gpGlobals->curtime - m_fPrevSampleTime);
		}

		if (g_debug_constraint_sounds.GetBool())
		{
			Msg("Reversal accel is %f/%f\n", projection, thresholdAcceleration[0]);
		}


		// now find the threshold crossed.
		int retval;
		for (retval = numThresholds - 1; retval >= 0 ; --retval)
		{
			if (projection > thresholdAcceleration[retval])
				break;
		}

		return retval; 
	}
	else
	{
		return -1;
	}
}

/// small helper function used just below (technique copy-pasted  from sound.cpp)
inline static bool IsEmpty (const string_t &str)
{
	return (!str || strlen(str.ToCStr()) < 1 );
}

void ConstraintSoundInfo::OnActivate( CPhysConstraint *pOuter )
{
	m_pTravelSound = NULL;
	m_vSampler.Initialize( getThinkRate() );


	ValidateInternals( pOuter );

	// make sure sound filenames are not empty 
	m_bPlayTravelSound   = !IsEmpty(m_iszTravelSoundFwd) || !IsEmpty(m_iszTravelSoundBack);
	m_bPlayReversalSound = false;
	for (int i = 0; i < SimpleConstraintSoundProfile::kREVERSAL_SOUND_ARRAY_SIZE ; ++i)
	{
		if ( !IsEmpty(m_iszReversalSounds[i]) )
		{
			// if there is at least one filled sound field, we should try
			// to play reversals
			m_bPlayReversalSound = true;
			break;
		}
	}


	/*
	SetThink(&CPhysSlideConstraint::SoundThink);
	SetNextThink(gpGlobals->curtime + m_vSampler.getSampleRate());
	*/
}

/// Maintain consistency of internal datastructures on start
void ConstraintSoundInfo::ValidateInternals( CPhysConstraint *pOuter )
{
	// Make sure the reversal sound thresholds are strictly increasing.
	for (int i = 1 ; i < SimpleConstraintSoundProfile::kREVERSAL_SOUND_ARRAY_SIZE ; ++i)
	{
		// if decreases from small to medium, promote small to medium and warn.
		if (m_soundProfile.m_reversalSoundThresholds[i] < m_soundProfile.m_reversalSoundThresholds[i-1])
		{
			Warning("Constraint reversal sounds for %s are out of order!", pOuter->GetDebugName() );
			m_soundProfile.m_reversalSoundThresholds[i] = m_soundProfile.m_reversalSoundThresholds[i-1];
			m_iszReversalSounds[i] = m_iszReversalSounds[i-1];
		}
	}
}

void ConstraintSoundInfo::OnPrecache( CPhysConstraint *pOuter )
{
	pOuter->PrecacheScriptSound( m_iszTravelSoundFwd.ToCStr() ); 
	pOuter->PrecacheScriptSound( m_iszTravelSoundBack.ToCStr() ); 
	for (int i = 0 ; i < SimpleConstraintSoundProfile::kREVERSAL_SOUND_ARRAY_SIZE; ++i )
	{
		pOuter->PrecacheScriptSound( m_iszReversalSounds[i].ToCStr() );
	}
}

void ConstraintSoundInfo::OnThink( CPhysConstraint *pOuter, const Vector &relativeVelocity )
{
	// have we had a hard reversal?
	int playReversal = m_vSampler.HasReversed( relativeVelocity, m_soundProfile.m_reversalSoundThresholds, SimpleConstraintSoundProfile::kREVERSAL_SOUND_ARRAY_SIZE );
	float relativeVelMag = relativeVelocity.Length(); //< magnitude of relative velocity

	CBaseEntity *pChildEntity = static_cast<CBaseEntity *>(pOuter->GetPhysConstraint()->GetAttachedObject()->GetGameData());

	// compute sound level
	float soundVol = this->m_soundProfile.GetVolume(relativeVelMag);

	if (g_debug_constraint_sounds.GetBool())
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr),"Velocity: %.3f", relativeVelMag );
		pChildEntity->EntityText( 0, tempstr, m_vSampler.getSampleRate() );

		Q_snprintf(tempstr,sizeof(tempstr),"Sound volume: %.3f", soundVol );
		pChildEntity->EntityText( 1, tempstr, m_vSampler.getSampleRate() );

		if (playReversal >= 0)
		{
			Q_snprintf(tempstr,sizeof(tempstr),"Reversal [%d]", playReversal );
			pChildEntity->EntityText(2,tempstr,m_vSampler.getSampleRate());
		}
	}

	// if we loaded a travel sound
	if (m_bPlayTravelSound)
	{
		if (soundVol > 0)
		{
			// if we want to play a sound...
			if ( m_pTravelSound )
			{	// if a sound exists, modify it
				CSoundEnvelopeController::GetController().SoundChangeVolume( m_pTravelSound, soundVol, 0.1f );
			}
			else
			{	// if a sound does not exist, create it
				bool travellingForward = relativeVelocity.Dot(m_forwardAxis) > 0;

				CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
				CPASAttenuationFilter filter( pChildEntity );
				m_pTravelSound = controller.SoundCreate( filter, pChildEntity->entindex(), 
					(travellingForward ? m_iszTravelSoundFwd : m_iszTravelSoundBack).ToCStr() );
				controller.Play( m_pTravelSound, soundVol, 100 );
			}
		}
		else
		{
			// if we want to not play sound
			if ( m_pTravelSound )
			{	// and it exists, kill it
				CSoundEnvelopeController::GetController().SoundDestroy( m_pTravelSound );
				m_pTravelSound = NULL;
			}
		}
	}

	if (m_bPlayReversalSound && (playReversal >= 0))
	{
		pChildEntity->EmitSound(m_iszReversalSounds[playReversal].ToCStr());
	}

	m_vSampler.AddSample( relativeVelocity );
	
}


void ConstraintSoundInfo::StartThinking( CPhysConstraint *pOuter, const Vector &relativeVelocity, const Vector &forwardVector )
{
	m_forwardAxis = forwardVector;
	m_vSampler.BeginSampling( relativeVelocity );

	/*
	IPhysicsConstraint *pConstraint = pOuter->GetPhysConstraint();
	Assert(pConstraint);
	if (pConstraint)
	{
		IPhysicsObject * pAttached = pConstraint->GetAttachedObject(), *pReference = pConstraint->GetReferenceObject();
		m_vSampler.BeginSampling( VelocitySampler::GetRelativeVelocity(pAttached,pReference) );
	}
	*/
}

void ConstraintSoundInfo::StopThinking( CPhysConstraint *pOuter )
{
	DeleteAllSounds();
}


ConstraintSoundInfo::~ConstraintSoundInfo()
{
	DeleteAllSounds();
}

// Any sounds envelopes that are active, kill.
void ConstraintSoundInfo::DeleteAllSounds()
{
	if ( m_pTravelSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pTravelSound );
		m_pTravelSound = NULL;
	}
}

#endif
