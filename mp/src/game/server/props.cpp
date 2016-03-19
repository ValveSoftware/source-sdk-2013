//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: static_prop - don't move, don't animate, don't do anything.
//			physics_prop - move, take damage, but don't animate
//
//===========================================================================//


#include "cbase.h"
#include "BasePropDoor.h"
#include "ai_basenpc.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "locksounds.h"
#include "filters.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "entityoutput.h"
#include "vcollide_parse.h"
#include "studio.h"
#include "explode.h"
#include "utlrbtree.h"
#include "tier1/strtools.h"
#include "physics_impact_damage.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "scriptevent.h"
#include "entityblocker.h"
#include "soundent.h"
#include "EntityFlame.h"
#include "game.h"
#include "physics_prop_ragdoll.h"
#include "decals.h"
#include "hierarchy.h"
#include "shareddefs.h"
#include "physobj.h"
#include "physics_npc_solver.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "datacache/imdlcache.h"
#include "doors.h"
#include "physics_collisionevent.h"
#include "gamestats.h"
#include "vehicle_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DOOR_HARDWARE_GROUP 1

// Any barrel farther away than this is ignited rather than exploded.
#define PROP_EXPLOSION_IGNITE_RADIUS	32.0f

// How many times to remind the player that supply crates can be broken
// (displayed when the supply crate is picked up)
#define NUM_SUPPLY_CRATE_HUD_HINTS		3

extern CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );


ConVar g_debug_doors( "g_debug_doors", "0" );
ConVar breakable_disable_gib_limit( "breakable_disable_gib_limit", "0" );
ConVar breakable_multiplayer( "breakable_multiplayer", "1" );

// AI Interaction for being hit by a physics object
int g_interactionHitByPlayerThrownPhysObj = 0;
int	g_interactionPlayerPuntedHeavyObject = 0;

int g_ActiveGibCount = 0;
ConVar prop_active_gib_limit( "prop_active_gib_limit", "999999" );
ConVar prop_active_gib_max_fade_time( "prop_active_gib_max_fade_time", "999999" );
#define ACTIVE_GIB_LIMIT prop_active_gib_limit.GetInt()
#define ACTIVE_GIB_FADE prop_active_gib_max_fade_time.GetInt()


// Damage type modifiers for breakable objects.
ConVar func_breakdmg_bullet( "func_breakdmg_bullet", "0.5" );
ConVar func_breakdmg_club( "func_breakdmg_club", "1.5" );
ConVar func_breakdmg_explosive( "func_breakdmg_explosive", "1.25" );

ConVar sv_turbophysics( "sv_turbophysics", "0", FCVAR_REPLICATED, "Turns on turbo physics" );

#ifdef HL2_EPISODIC
	#define PROP_FLARE_LIFETIME 30.0f
	#define PROP_FLARE_IGNITE_SUBSTRACT 5.0f
	CBaseEntity *CreateFlare( Vector vOrigin, QAngle Angles, CBaseEntity *pOwner, float flDuration );
	void KillFlare( CBaseEntity *pOwnerEntity, CBaseEntity *pEntity, float flKillTime );
#endif


//-----------------------------------------------------------------------------
// Purpose: Breakable objects take different levels of damage based upon the damage type.
//			This isn't contained by CBaseProp, because func_breakables use it as well.
//-----------------------------------------------------------------------------
float GetBreakableDamage( const CTakeDamageInfo &inputInfo, IBreakableWithPropData *pProp )
{
	float flDamage = inputInfo.GetDamage();
	int iDmgType = inputInfo.GetDamageType();

	// Bullet damage?
	if ( iDmgType & DMG_BULLET )
	{
		// Buckshot does double damage to breakables
		if ( iDmgType & DMG_BUCKSHOT )
		{
			if ( pProp )
			{
				flDamage *= (pProp->GetDmgModBullet() * 2);
			}
			else
			{
				// Bullets do little damage to breakables
				flDamage *= (func_breakdmg_bullet.GetFloat() * 2);
			}
		}
		else
		{
			if ( pProp )
			{
				flDamage *= pProp->GetDmgModBullet();
			}
			else
			{
				// Bullets do little damage to breakables
				flDamage *= func_breakdmg_bullet.GetFloat();
			}
		}
	}

	// Club damage?
	if ( iDmgType & DMG_CLUB )
	{
		if ( pProp )
		{
			flDamage *= pProp->GetDmgModClub();
		}
		else
		{
			// Club does extra damage
			flDamage *= func_breakdmg_club.GetFloat();
		}
	}

	// Explosive damage?
	if ( iDmgType & DMG_BLAST )
	{
		if ( pProp )
		{
			flDamage *= pProp->GetDmgModExplosive();
		}
		else
		{
			// Explosions do extra damage
			flDamage *= func_breakdmg_explosive.GetFloat();
		}
	}

	if ( (iDmgType & DMG_SLASH) && (iDmgType & DMG_CRUSH) )
	{
		// Cut by a Ravenholm propeller trap
		flDamage *= 10.0f;
	}

	// Poison & other timebased damage types do no damage
	if ( g_pGameRules->Damage_IsTimeBased( iDmgType ) )
	{
		flDamage = 0;
	}

	return flDamage;
}

//=============================================================================================================
// BASE PROP
//=============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Spawn( void )
{
	char *szModel = (char *)STRING( GetModelName() );
	if (!szModel || !*szModel)
	{
		Warning( "prop at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	PrecacheModel( szModel );
	Precache();
	SetModel( szModel );

	// Load this prop's data from the propdata file
	int iResult = ParsePropData();
	if ( !OverridePropdata() )
	{
		if ( iResult == PARSE_FAILED_BAD_DATA )
		{
			DevWarning( "%s at %.0f %.0f %0.f uses model %s, which has an invalid prop_data type. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
			UTIL_Remove( this );
			return;
		}
		else if ( iResult == PARSE_FAILED_NO_DATA )
		{
			// If we don't have data, but we're a prop_physics, fail
			if ( FClassnameIs( this, "prop_physics" ) )
			{
				DevWarning( "%s at %.0f %.0f %0.f uses model %s, which has no propdata which means it must be used on a prop_static. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
				UTIL_Remove( this );
				return;
			}
		}
		else if ( iResult == PARSE_SUCCEEDED )
		{
			// If we have data, and we're not a physics prop, fail
			if ( !dynamic_cast<CPhysicsProp*>(this) )
			{
				DevWarning( "%s at %.0f %.0f %0.f uses model %s, which has propdata which means that it be used on a prop_physics. DELETED.\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z, szModel );
				UTIL_Remove( this );
				return;
			}
		}
	}

	SetMoveType( MOVETYPE_PUSH );
	m_takedamage = DAMAGE_NO;
	SetNextThink( TICK_NEVER_THINK );

	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Precache( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		Msg( "%s at (%.3f, %.3f, %.3f) has no model name!\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		SetModelName( AllocPooledString( "models/error.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	PrecacheScriptSound( "Metal.SawbladeStick" );
	PrecacheScriptSound( "PropaneTank.Burst" );

#ifdef HL2_EPISODIC
	UTIL_PrecacheOther( "env_flare" );
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::Activate( void )
{
	BaseClass::Activate();
	
	// Make sure mapmakers haven't used the wrong prop type.
	if ( m_takedamage == DAMAGE_NO && m_iHealth != 0 )
	{
		Warning("%s has a health specified in model '%s'. Use prop_physics or prop_dynamic instead.\n", GetClassname(), STRING(GetModelName()) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Handles keyvalues from the BSP. Called before spawning.
//-----------------------------------------------------------------------------
bool CBaseProp::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq(szKeyName, "health") )
	{
		// Only override props are allowed to override health.
		if ( FClassnameIs( this, "prop_physics_override" ) || FClassnameIs( this, "prop_dynamic_override" ) )
			return BaseClass::KeyValue( szKeyName, szValue );

		return true;
	}
	else
	{ 
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate whether this prop should block LOS or not
//-----------------------------------------------------------------------------
void CBaseProp::CalculateBlockLOS( void )
{
	// We block LOS if:
	//		- One of our dimensions is >40
	//		- Our other 2 dimensions are >30
	// By default, entities block LOS, so we only need to detect non-blockage
	bool bFoundLarge = false;
	Vector vecSize = CollisionProp()->OBBMaxs() - CollisionProp()->OBBMins();
	for ( int i = 0; i < 3; i++ )
	{
		if ( vecSize[i] > 40 )
		{
			bFoundLarge = true;
		}
		if ( vecSize[i] > 30 )
			continue;

		// Dimension smaller than 30.
		SetBlocksLOS( false );
		return;
	}

	if ( !bFoundLarge )
	{
		// No dimension larger than 40
		SetBlocksLOS( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Parse this prop's data from the model, if it has a keyvalues section.
//			Returns true only if this prop is using a model that has a prop_data section that's invalid.
//-----------------------------------------------------------------------------
int CBaseProp::ParsePropData( void )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( !modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	// Do we have a props section?
	KeyValues *pkvPropData = modelKeyValues->FindKey("prop_data");
	if ( !pkvPropData )
	{
		modelKeyValues->deleteThis();
		return PARSE_FAILED_NO_DATA;
	}

	int iResult = g_PropDataSystem.ParsePropFromKV( this, pkvPropData, modelKeyValues );
	modelKeyValues->deleteThis();
	return iResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBaseProp::DrawDebugGeometryOverlays( void )
{
	BaseClass::DrawDebugGeometryOverlays();

	if ( m_debugOverlays & OVERLAY_PROP_DEBUG )  
	{
		if ( m_takedamage == DAMAGE_NO )
		{
			NDebugOverlay::EntityBounds(this, 255, 0, 0, 0, 0 );
		}
		else if ( m_takedamage == DAMAGE_EVENTS_ONLY )
		{
			NDebugOverlay::EntityBounds(this, 255, 255, 255, 0, 0 );
		}
		else
		{
			// Remap health to green brightness
			float flG = RemapVal( m_iHealth, 0, 100, 64, 255 );
			flG = clamp( flG, 0.f, 255.f );
			NDebugOverlay::EntityBounds(this, 0, flG, 0, 0, 0 );
		}
	}
}


class CEnableMotionFixup : public CBaseEntity
{
	DECLARE_CLASS( CEnableMotionFixup, CBaseEntity );
};

LINK_ENTITY_TO_CLASS( point_enable_motion_fixup, CEnableMotionFixup );

static const char *s_pFadeScaleThink = "FadeScaleThink";
static const char *s_pPropAnimateThink = "PropAnimateThink";

void CBreakableProp::SetEnableMotionPosition( const Vector &position, const QAngle &angles )
{
	ClearEnableMotionPosition();
	CBaseEntity *pFixup = CBaseEntity::Create( "point_enable_motion_fixup", position, angles, this );
	if ( pFixup )
	{
		pFixup->SetParent( this );
	}
}

CBaseEntity	*CBreakableProp::FindEnableMotionFixup()
{
	CUtlVector<CBaseEntity*> list;
	GetAllChildren( this, list );
	for ( int i = list.Count()-1; i >= 0; --i )
	{
		if ( FClassnameIs( list[i], "point_enable_motion_fixup" ) )
			return list[i];
	}

	return NULL;
}

bool CBreakableProp::GetEnableMotionPosition( Vector *pPosition, QAngle *pAngles )
{
	CBaseEntity *pFixup = FindEnableMotionFixup();
	if ( !pFixup )
		return false;
	*pPosition = pFixup->GetAbsOrigin();
	*pAngles = pFixup->GetAbsAngles();
	return true;
}

void CBreakableProp::ClearEnableMotionPosition()
{
	CBaseEntity *pFixup = FindEnableMotionFixup();
	if ( pFixup )
	{
		UnlinkFromParent( pFixup );
		UTIL_Remove( pFixup );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBreakableProp::Ignite( float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner )
{
	if( IsOnFire() )
		return;

	if( !HasInteraction( PROPINTER_FIRE_FLAMMABLE ) )
		return;

	BaseClass::Ignite( flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner );

	if ( g_pGameRules->ShouldBurningPropsEmitLight() )
	{
		GetEffectEntity()->AddEffects( EF_DIMLIGHT );
	}

	// Frighten AIs, just in case this is an exploding thing.
	CSoundEnt::InsertSound( SOUND_DANGER, GetAbsOrigin(), 128.0f, 1.0f, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CBreakableProp::HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent )
{
	if ( pEvent->pEntities[ !index ]->IsWorld() )
	{
		if ( HasInteraction( PROPINTER_PHYSGUN_WORLD_STICK ) )
		{
			HandleInteractionStick( index, pEvent );
		}
	}

	if( HasInteraction( PROPINTER_PHYSGUN_FIRST_BREAK ) )
	{
		// Looks like it's best to break by having the object damage itself. 
		CTakeDamageInfo info;

		info.SetDamage( m_iHealth );
		info.SetAttacker( this );
		info.SetInflictor( this );
		info.SetDamageType( DMG_GENERIC );

		Vector vecPosition;
		Vector vecVelocity;

		VPhysicsGetObject()->GetVelocity( &vecVelocity, NULL );
		VPhysicsGetObject()->GetPosition( &vecPosition, NULL );

		info.SetDamageForce( vecVelocity );
		info.SetDamagePosition( vecPosition );

		TakeDamage( info );
		return;
	}
	
	if( HasInteraction( PROPINTER_PHYSGUN_FIRST_PAINT ) )
	{
		IPhysicsObject *pObj = VPhysicsGetObject();
 
		Vector vecPos;
		pObj->GetPosition( &vecPos, NULL );
 
		Vector vecVelocity = pEvent->preVelocity[0];
		VectorNormalize(vecVelocity);

		trace_t tr;
		UTIL_TraceLine( vecPos, vecPos + (vecVelocity * 64), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.m_pEnt )
		{
#ifdef HL2_DLL
			// Don't paintsplat friendlies
			int iClassify = tr.m_pEnt->Classify();
			if ( iClassify != CLASS_PLAYER_ALLY_VITAL && iClassify != CLASS_PLAYER_ALLY && 
				 iClassify != CLASS_CITIZEN_PASSIVE && iClassify != CLASS_CITIZEN_REBEL ) 
#endif
			{
				switch( entindex() % 3 )
				{
				case 0:
					UTIL_DecalTrace( &tr, "PaintSplatBlue" );
					break;

				case 1:
					UTIL_DecalTrace( &tr, "PaintSplatGreen" );
					break;

				case 2:
					UTIL_DecalTrace( &tr, "PaintSplatPink" );
					break;
				}
			}
		}
	}

	if ( HasInteraction( PROPINTER_PHYSGUN_NOTIFY_CHILDREN ) )
	{
		CUtlVector<CBaseEntity *> children;
		GetAllChildren( this, children );
		for (int i = 0; i < children.Count(); i++ )
		{
			CBaseEntity *pent = children.Element( i );

			IParentPropInteraction *pPropInter = dynamic_cast<IParentPropInteraction *>( pent );
			if ( pPropInter )
			{
				pPropInter->OnParentCollisionInteraction( COLLISIONINTER_PARENT_FIRST_IMPACT, index, pEvent );
			}
		}
	}
}


void CBreakableProp::CheckRemoveRagdolls()
{
	if ( HasSpawnFlags( SF_PHYSPROP_HAS_ATTACHED_RAGDOLLS ) )
	{
		DetachAttachedRagdollsForEntity( this );
		RemoveSpawnFlags( SF_PHYSPROP_HAS_ATTACHED_RAGDOLLS );
	}
}
//-----------------------------------------------------------------------------
// Purpose: Handle special physgun interactions
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CPhysicsProp::HandleAnyCollisionInteractions( int index, gamevcollisionevent_t *pEvent )
{
	// If we're supposed to impale, and we've hit an NPC, impale it
	if ( HasInteraction( PROPINTER_PHYSGUN_FIRST_IMPALE ) )
	{
		Vector vel = pEvent->preVelocity[index];

		Vector forward;
		QAngle angImpaleForward;
		if ( GetPropDataAngles( "impale_forward", angImpaleForward ) )
		{
			Vector vecImpaleForward;
 			AngleVectors( angImpaleForward, &vecImpaleForward );
			VectorRotate( vecImpaleForward, EntityToWorldTransform(), forward );
		}
		else
		{
			GetVectors( &forward, NULL, NULL );
		}

		float speed = DotProduct( forward, vel );
		if ( speed < 1000.0f )
		{
			// not going to stick, so remove any ragdolls we've got
			CheckRemoveRagdolls();
			return;
		}
		CBaseEntity *pHitEntity = pEvent->pEntities[!index];
		if ( pHitEntity->IsWorld() )
		{
			Vector normal;
			float sign = index ? -1.0f : 1.0f;
			pEvent->pInternalData->GetSurfaceNormal( normal );
			float dot = DotProduct( forward, normal );
			if ( (sign*dot) < DOT_45DEGREE )
				return;
			// Impale sticks to the wall if we hit end on
			HandleInteractionStick( index, pEvent );
		}
		else if ( pHitEntity->MyNPCPointer() )
		{
			CAI_BaseNPC *pNPC = pHitEntity->MyNPCPointer();
			IPhysicsObject *pObj = VPhysicsGetObject();

			// do not impale NPCs if the impaler is friendly
			CBasePlayer *pAttacker = HasPhysicsAttacker( 25.0f );
			if (pAttacker && pNPC->IRelationType( pAttacker ) == D_LI)
			{
				return;
			}

			Vector vecPos;
			pObj->GetPosition( &vecPos, NULL );

			// Find the bone for the hitbox we hit
			trace_t tr;
			UTIL_TraceLine( vecPos, vecPos + pEvent->preVelocity[index] * 1.5, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
			Vector vecImpalePos = tr.endpos;
			int iBone = -1;
			if ( tr.hitbox )
			{
				Vector vecBonePos;
				QAngle vecBoneAngles;
				iBone = pNPC->GetHitboxBone( tr.hitbox );
				pNPC->GetBonePosition( iBone, vecBonePos, vecBoneAngles );

				Teleport( &vecBonePos, NULL, NULL );
				vecImpalePos = vecBonePos;
			}

			// Kill the NPC and make an attached ragdoll
			pEvent->pInternalData->GetContactPoint( vecImpalePos );
			CBaseEntity *pRagdoll = CreateServerRagdollAttached( pNPC, vec3_origin, -1, COLLISION_GROUP_INTERACTIVE_DEBRIS, pObj, this, 0, vecImpalePos, iBone, vec3_origin );
			if ( pRagdoll )
			{
				Vector vecVelocity = pEvent->preVelocity[index] * pObj->GetMass();
				PhysCallbackImpulse( pObj, vecVelocity, vec3_origin );
				UTIL_Remove( pNPC );
				AddSpawnFlags( SF_PHYSPROP_HAS_ATTACHED_RAGDOLLS );
			}
		}
	}
}


void CBreakableProp::StickAtPosition( const Vector &stickPosition, const Vector &savePosition, const QAngle &saveAngles )
{
	if ( !VPhysicsGetObject()->IsMotionEnabled() )
		return;

	EmitSound("Metal.SawbladeStick");
	Teleport( &stickPosition, NULL, NULL );
	SetEnableMotionPosition( savePosition, saveAngles );  // this uses hierarchy, so it must be set after teleport

	VPhysicsGetObject()->EnableMotion( false );
	AddSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
//			*pEvent - 
//-----------------------------------------------------------------------------
void CBreakableProp::HandleInteractionStick( int index, gamevcollisionevent_t *pEvent )
{
	Vector vecDir = pEvent->preVelocity[ index ];
	float speed = VectorNormalize( vecDir );

	// Make sure the object is travelling fast enough to stick.
	if( speed > 1000.0f )
	{
		Vector position;
		QAngle angles;
		VPhysicsGetObject()->GetPosition( &position, &angles );

		Vector vecNormal;
		pEvent->pInternalData->GetSurfaceNormal( vecNormal );

		// we want the normal that points away from this object
		if ( index == 1 )
		{
			vecNormal *= -1.0f;
		}
		float flDot = DotProduct( vecDir, vecNormal );

		// Make sure the object isn't hitting the world at too sharp an angle.
		if( flDot > 0.3 )
		{
			// Finally, inhibit sticking in metal, grates, sky, or anything else that doesn't make a sound.
			const surfacedata_t *psurf = physprops->GetSurfaceData( pEvent->surfaceProps[!index] );

			if (psurf->game.material != CHAR_TEX_METAL && psurf->game.material != CHAR_TEX_GRATE && psurf->game.material != 'X' )
			{
				Vector savePosition = position;

				Vector vecEmbed = pEvent->preVelocity[ index ];
				VectorNormalize( vecEmbed );
				vecEmbed *= 8;

				position += vecEmbed;
				g_PostSimulationQueue.QueueCall( this, &CBreakableProp::StickAtPosition, position, savePosition, angles );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turn on prop debugging mode
//-----------------------------------------------------------------------------
void CC_Prop_Debug( void )
{
	// Toggle the prop debug bit on all props
	for ( CBaseEntity *pEntity = gEntList.FirstEnt(); pEntity != NULL; pEntity = gEntList.NextEnt(pEntity) )
	{
		CBaseProp *pProp = dynamic_cast<CBaseProp*>(pEntity);
		if ( pProp )
		{
			if ( pProp->m_debugOverlays & OVERLAY_PROP_DEBUG )
			{
				pProp->m_debugOverlays &= ~OVERLAY_PROP_DEBUG;
			}
			else
			{
				pProp->m_debugOverlays |= OVERLAY_PROP_DEBUG;
			}
		}
	}
}
static ConCommand prop_debug("prop_debug", CC_Prop_Debug, "Toggle prop debug mode. If on, props will show colorcoded bounding boxes. Red means ignore all damage. White means respond physically to damage but never break. Green maps health in the range of 100 down to 1.", FCVAR_CHEAT);

//=============================================================================================================
// BREAKABLE PROPS
//=============================================================================================================
IMPLEMENT_SERVERCLASS_ST(CBreakableProp, DT_BreakableProp)
END_SEND_TABLE()

BEGIN_DATADESC( CBreakableProp )

	DEFINE_KEYFIELD( m_explodeDamage, FIELD_FLOAT, "ExplodeDamage"),	
	DEFINE_KEYFIELD( m_explodeRadius, FIELD_FLOAT, "ExplodeRadius"),	
	DEFINE_KEYFIELD( m_iMinHealthDmg, FIELD_INTEGER, "minhealthdmg" ),
	DEFINE_FIELD( m_createTick, FIELD_INTEGER ),
	DEFINE_FIELD( m_hBreaker, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_PerformanceMode, FIELD_INTEGER, "PerformanceMode" ),

	DEFINE_KEYFIELD( m_iszBreakModelMessage, FIELD_STRING, "BreakModelMessage" ),

	DEFINE_FIELD( m_flDmgModBullet, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDmgModClub, FIELD_FLOAT ),
	DEFINE_FIELD( m_flDmgModExplosive, FIELD_FLOAT ),
	DEFINE_FIELD( m_iszPhysicsDamageTableName, FIELD_STRING ),
	DEFINE_FIELD( m_iszBreakableModel, FIELD_STRING ),
	DEFINE_FIELD( m_iBreakableSkin, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBreakableCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMaxBreakableSize, FIELD_INTEGER ),
	DEFINE_FIELD( m_iszBasePropData, FIELD_STRING ),
	DEFINE_FIELD( m_iInteractions,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iNumBreakableChunks, FIELD_INTEGER ),
	DEFINE_FIELD( m_nPhysgunState, FIELD_CHARACTER ),
	DEFINE_KEYFIELD( m_iszPuntSound, FIELD_STRING, "puntsound" ),

	DEFINE_KEYFIELD( m_flPressureDelay, FIELD_FLOAT, "PressureDelay" ),
	DEFINE_FIELD( m_preferredCarryAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_flDefaultFadeScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_bUsePuntSound, FIELD_BOOLEAN ),
	// DEFINE_FIELD( m_mpBreakMode, mp_break_t ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Break", InputBreak ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddHealth", InputAddHealth ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveHealth", InputRemoveHealth ),
	DEFINE_INPUT( m_impactEnergyScale, FIELD_FLOAT, "physdamagescale" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePhyscannonPickup", InputEnablePhyscannonPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePhyscannonPickup", InputDisablePhyscannonPickup ),
	DEFINE_INPUTFUNC( FIELD_VOID, "EnablePuntSound", InputEnablePuntSound ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisablePuntSound", InputDisablePuntSound ),

	// Outputs
	DEFINE_OUTPUT( m_OnBreak, "OnBreak" ),
	DEFINE_OUTPUT( m_OnHealthChanged, "OnHealthChanged" ),
	DEFINE_OUTPUT( m_OnTakeDamage, "OnTakeDamage" ),
	DEFINE_OUTPUT( m_OnPhysCannonDetach, "OnPhysCannonDetach" ),
	DEFINE_OUTPUT( m_OnPhysCannonAnimatePreStarted, "OnPhysCannonAnimatePreStarted" ),
	DEFINE_OUTPUT( m_OnPhysCannonAnimatePullStarted, "OnPhysCannonAnimatePullStarted" ),
	DEFINE_OUTPUT( m_OnPhysCannonAnimatePostStarted, "OnPhysCannonAnimatePostStarted" ),
	DEFINE_OUTPUT( m_OnPhysCannonPullAnimFinished, "OnPhysCannonPullAnimFinished" ),

	// Function Pointers
	DEFINE_THINKFUNC( BreakThink ),
	DEFINE_THINKFUNC( AnimateThink ),
	DEFINE_THINKFUNC( RampToDefaultFadeScale ),
	DEFINE_ENTITYFUNC( BreakablePropTouch ),

	// Physics Influence
	DEFINE_FIELD( m_hPhysicsAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flLastPhysicsInfluenceTime, FIELD_TIME ),

	DEFINE_FIELD( m_bOriginalBlockLOS, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBlockLOSSetByPropData, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsWalkableSetByPropData, FIELD_BOOLEAN ),

	// Damage
	DEFINE_FIELD( m_hLastAttacker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hFlareEnt,	FIELD_EHANDLE ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CBreakableProp::CBreakableProp()
{
	m_fadeMinDist = -1;
	m_fadeMaxDist = 0;
	m_flFadeScale = 1;
	m_flDefaultFadeScale = 1;
	m_mpBreakMode = MULTIPLAYER_BREAK_DEFAULT;
	
	// This defaults to on. Most times mapmakers won't specify a punt sound to play.
	m_bUsePuntSound = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::Spawn()
{
	// Starts out as the default fade scale value
	m_flDefaultFadeScale = m_flFadeScale;

	// Initialize damage modifiers. Must be done before baseclass spawn.
	m_flDmgModBullet = 1.0;
	m_flDmgModClub = 1.0;
	m_flDmgModExplosive = 1.0;
	
	//jmd: I am guessing that the call to Spawn will set any flags that should be set anyway; this
	//clears flags we don't want (specifically the FL_ONFIRE for explosive barrels in HL2MP)]
#ifdef HL2MP
	ClearFlags();
#endif 

	BaseClass::Spawn();
	
	if ( IsMarkedForDeletion() )
		return;

	CStudioHdr *pStudioHdr = GetModelPtr( );
	if ( pStudioHdr->flags() & STUDIOHDR_FLAGS_NO_FORCED_FADE )
	{
		DisableAutoFade();
	}
	else
	{
		m_flFadeScale = m_flDefaultFadeScale;
	}

	// If we have no custom breakable chunks, see if we're breaking into generic ones
	if ( !m_iNumBreakableChunks )
	{
		IBreakableWithPropData *pBreakableInterface = assert_cast<IBreakableWithPropData*>(this);
		if ( pBreakableInterface->GetBreakableModel() != NULL_STRING && pBreakableInterface->GetBreakableCount() )
		{
			m_iNumBreakableChunks = pBreakableInterface->GetBreakableCount();
		}
	}

	// Setup takedamage based upon the health we parsed earlier, and our interactions
	if ( ( m_iHealth == 0 ) ||
        ( !m_iNumBreakableChunks && 
		    !HasInteraction( PROPINTER_PHYSGUN_BREAK_EXPLODE ) &&
		    !HasInteraction( PROPINTER_PHYSGUN_FIRST_BREAK ) &&
		    !HasInteraction( PROPINTER_FIRE_FLAMMABLE ) &&
		    !HasInteraction( PROPINTER_FIRE_IGNITE_HALFHEALTH ) &&
		    !HasInteraction( PROPINTER_FIRE_EXPLOSIVE_RESIST ) ) )
	{
		m_iHealth = 0;
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
	else
	{
		m_takedamage = DAMAGE_YES;

		if( g_pGameRules->GetAutoAimMode() == AUTOAIM_ON_CONSOLE )
		{
			if ( HasInteraction( PROPINTER_PHYSGUN_BREAK_EXPLODE ) ||
				HasInteraction( PROPINTER_FIRE_IGNITE_HALFHEALTH ) )
			{
				// Exploding barrels, exploding gas cans
				AddFlag( FL_AIMTARGET );	
			}
		}
	}

	m_iMaxHealth = ( m_iHealth > 0 ) ? m_iHealth : 1;

	m_createTick = gpGlobals->tickcount;
	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 0.1f;
	}

 	m_preferredCarryAngles = QAngle( -5, 0, 0 );

	// The presence of this activity causes us to have to detach it before it can be grabbed.
	if ( SelectWeightedSequence( ACT_PHYSCANNON_ANIMATE ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_nPhysgunState = PHYSGUN_ANIMATE_ON_PULL;
	}
	else if ( SelectWeightedSequence( ACT_PHYSCANNON_DETACH ) != ACTIVITY_NOT_AVAILABLE )
	{
		m_nPhysgunState = PHYSGUN_MUST_BE_DETACHED;
	}
	else
	{
		m_nPhysgunState = PHYSGUN_CAN_BE_GRABBED;
	}

	m_hLastAttacker = NULL;

	m_hBreaker = NULL;

	SetTouch( &CBreakableProp::BreakablePropTouch );
}


//-----------------------------------------------------------------------------
// Disable auto fading under dx7 or when level fades are specified
//-----------------------------------------------------------------------------
void CBreakableProp::DisableAutoFade()
{
	m_flFadeScale = 0;
	m_flDefaultFadeScale = 0;
}

	
//-----------------------------------------------------------------------------
// Copy fade from another breakable.
//-----------------------------------------------------------------------------
void CBreakableProp::CopyFadeFrom( CBreakableProp *pSource )
{
	m_flDefaultFadeScale = pSource->m_flDefaultFadeScale;
	m_flFadeScale = pSource->m_flFadeScale;
	if ( m_flFadeScale != m_flDefaultFadeScale )
	{
		float flNextThink = pSource->GetNextThink( s_pFadeScaleThink );
		if ( flNextThink < gpGlobals->curtime + TICK_INTERVAL )
		{
			flNextThink = gpGlobals->curtime + TICK_INTERVAL;
		}

		SetContextThink( &CBreakableProp::RampToDefaultFadeScale, flNextThink, s_pFadeScaleThink );
	}
}


//-----------------------------------------------------------------------------
// Make physcannonable, or not
//-----------------------------------------------------------------------------
void CBreakableProp::InputEnablePhyscannonPickup( inputdata_t &inputdata )
{
	RemoveEFlags( EFL_NO_PHYSCANNON_INTERACTION );
}

void CBreakableProp::InputDisablePhyscannonPickup( inputdata_t &inputdata )
{
	AddEFlags( EFL_NO_PHYSCANNON_INTERACTION );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBreakableProp::BreakablePropTouch( CBaseEntity *pOther )
{
	if ( HasSpawnFlags( SF_PHYSPROP_TOUCH ) )
	{
		// can be broken when run into 
		float flDamage = pOther->GetSmoothedVelocity().Length() * 0.01;

		if ( flDamage >= m_iHealth )
		{
			// Make sure we can take damage
			m_takedamage = DAMAGE_YES;
			OnTakeDamage( CTakeDamageInfo( pOther, pOther, flDamage, DMG_CRUSH ) );

			// do a little damage to player if we broke glass or computer
			CTakeDamageInfo info( pOther, pOther, flDamage/4, DMG_SLASH );
			CalculateMeleeDamageForce( &info, (pOther->GetAbsOrigin() - GetAbsOrigin()), GetAbsOrigin() );
			pOther->TakeDamage( info );
		}
	}

	if ( HasSpawnFlags( SF_PHYSPROP_PRESSURE ) && pOther->GetGroundEntity() == this )
	{
		// can be broken when stood upon
		// play creaking sound here.
		// DamageSound();

		m_hBreaker = pOther;

		if ( m_pfnThink != (void (CBaseEntity::*)())&CBreakableProp::BreakThink )
		{
			SetThink( &CBreakableProp::BreakThink );
			//SetTouch( NULL );
		
			// Add optional delay 
			SetNextThink( gpGlobals->curtime + m_flPressureDelay );
		}
	}

#ifdef HL2_EPISODIC
	if ( m_hFlareEnt )
	{
		CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

		if ( pNPC && pNPC->AllowedToIgnite() && pNPC->IsOnFire() == false )
		{
			pNPC->Ignite( 25.0f );
			KillFlare( this, m_hFlareEnt, PROP_FLARE_IGNITE_SUBSTRACT );
			IGameEvent *event = gameeventmanager->CreateEvent( "flare_ignite_npc" );
			if ( event )
			{
				event->SetInt( "entindex", pNPC->entindex() );
				gameeventmanager->FireEvent( event );
			}
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// UNDONE: Time stamp the object's creation so that an explosion or something doesn't break the parent object
// and then break the children who spawn afterward ?
// Explosions should use entities in box before they start to do damage.  Make sure nothing traverses the list
// in a way that would hose this.
int CBreakableProp::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	CTakeDamageInfo info = inputInfo;

	// If attacker can't do at least the min required damage to us, don't take any damage from them
 	if ( info.GetDamage() < m_iMinHealthDmg )
		return 0;

	if (!PassesDamageFilter( info ))
	{
		return 1;
	}

	if( info.GetAttacker() && info.GetAttacker()->MyCombatCharacterPointer() )
	{
		m_hLastAttacker.Set( info.GetAttacker() );
	}

	float flPropDamage = GetBreakableDamage( info, assert_cast<IBreakableWithPropData*>(this) );
	info.SetDamage( flPropDamage );

	// UNDONE: Do this?
#if 0
	// Make a shard noise each time func breakable is hit.
	// Don't play shard noise if being burned.
	// Don't play shard noise if cbreakable actually died.
	if ( ( bitsDamageType & DMG_BURN ) == false )
	{
		DamageSound();
	}
#endif

	// don't take damage on the same frame you were created 
	// (avoids a set of explosions progressively vaporizing a compound breakable)
	if ( m_createTick == (unsigned int)gpGlobals->tickcount )
	{
		int saveFlags = m_takedamage;
		m_takedamage = DAMAGE_EVENTS_ONLY;
		int ret = BaseClass::OnTakeDamage( info );
		m_takedamage = saveFlags;

		return ret;
	}

	// Ignore fire damage from other flames if I'm already on fire.
	// (i.e., only let the flames attached to me damage me)
	if( IsOnFire() && (inputInfo.GetDamageType() & DMG_BURN) && !(inputInfo.GetDamageType() & DMG_DIRECT) )
	{
		return 0;
	}

	bool bDeadly = info.GetDamage() >= m_iHealth;

	if( bDeadly && (info.GetDamageType() & DMG_BLAST) && HasInteraction( PROPINTER_FIRE_EXPLOSIVE_RESIST ) && info.GetInflictor() )
	{
		// This explosion would kill me, but I have a special interaction with explosions.

		float flDist = ( WorldSpaceCenter() - info.GetInflictor()->WorldSpaceCenter() ).Length();

		// I'm going to burn for a bit instead of exploding right now.
		float flBurnTime;
		if( flDist >= PROP_EXPLOSION_IGNITE_RADIUS )
		{
			// I'm far from the blast. Ignite and burn for several seconds.
			const float MAX_BLAST_DIST = 256.0f;

			// Just clamp distance.
			if( flDist > MAX_BLAST_DIST )
				flDist = MAX_BLAST_DIST;

			float flFactor;
			flFactor = flDist / MAX_BLAST_DIST;
			const float MAX_BURN_TIME = 5.0f;
			flBurnTime = MAX( 0.5, MAX_BURN_TIME * flFactor );
			flBurnTime += random->RandomFloat( 0, 0.5 );
		}
		else
		{
			// Very near the explosion. explode almost immediately.
			flBurnTime = random->RandomFloat( 0.1, 0.2 );
		}

		// Change my health so that I burn for flBurnTime seconds.
		float flIdealHealth = MIN( m_iHealth, FLAME_DIRECT_DAMAGE_PER_SEC *  flBurnTime );
		float flIdealDamage = m_iHealth - flIdealHealth;

		// Scale the damage to do ideal damage.
		info.ScaleDamage( flIdealDamage / info.GetDamage() );

		// Re-evaluate the deadly
		bDeadly = info.GetDamage() >= m_iHealth;
	}

	if( !bDeadly && (info.GetDamageType() & DMG_BLAST) )
	{
		Ignite( random->RandomFloat( 10, 15 ), false );
	}
	else if( !bDeadly && (info.GetDamageType() & DMG_BURN) )
	{
		// Ignite if burned, and flammable (the Ignite() function takes care of all of this).
		Ignite( random->RandomFloat( 10, 15 ), false );
	}
	else if( !bDeadly && (info.GetDamageType() & DMG_BULLET) )
	{
		if( HasInteraction( PROPINTER_FIRE_IGNITE_HALFHEALTH ) )
		{
			if( (m_iHealth - info.GetDamage()) <= m_iMaxHealth / 2 && !IsOnFire() )
			{
				// Bump back up to full health so it burns longer. Magically getting health back isn't
				// a big problem because if this item takes damage again whilst burning, it will break.
				m_iHealth = m_iMaxHealth;
				Ignite( random->RandomFloat( 10, 15 ), false );
			}
			else if( IsOnFire() )
			{
				// Explode right now!
				info.ScaleDamage( m_iHealth / info.GetDamage() );
			}
		}
	}

	int ret = BaseClass::OnTakeDamage( info );

	// Output the new health as a percentage of max health [0..1]
	float flRatio = clamp( (float)m_iHealth / (float)m_iMaxHealth, 0.f, 1.f );
	m_OnHealthChanged.Set( flRatio, info.GetAttacker(), this );
	m_OnTakeDamage.FireOutput( info.GetAttacker(), this );

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::Event_Killed( const CTakeDamageInfo &info )
{
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( pPhysics && !pPhysics->IsMoveable() )
	{
		pPhysics->EnableMotion( true );
		VPhysicsTakeDamage( info );
	}
	Break( info.GetInflictor(), info );
	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for breaking the breakable immediately.
//-----------------------------------------------------------------------------
void CBreakableProp::InputBreak( inputdata_t &inputdata )
{
	CTakeDamageInfo info;
	info.SetAttacker( this );
	Break( inputdata.pActivator, info );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for adding to the breakable's health.
// Input  : Integer health points to add.
//-----------------------------------------------------------------------------
void CBreakableProp::InputAddHealth( inputdata_t &inputdata )
{
	UpdateHealth( m_iHealth + inputdata.value.Int(), inputdata.pActivator );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for removing health from the breakable.
// Input  : Integer health points to remove.
//-----------------------------------------------------------------------------
void CBreakableProp::InputRemoveHealth( inputdata_t &inputdata )
{
	UpdateHealth( m_iHealth - inputdata.value.Int(), inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the breakable's health.
//-----------------------------------------------------------------------------
void CBreakableProp::InputSetHealth( inputdata_t &inputdata )
{
	UpdateHealth( inputdata.value.Int(), inputdata.pActivator );
}


//-----------------------------------------------------------------------------
// Purpose: Choke point for changes to breakable health. Ensures outputs are fired.
// Input  : iNewHealth - 
//			pActivator - 
// Output : Returns true if the breakable survived, false if it died (broke).
//-----------------------------------------------------------------------------
bool CBreakableProp::UpdateHealth( int iNewHealth, CBaseEntity *pActivator )
{
	if ( iNewHealth != m_iHealth )
	{
		m_iHealth = iNewHealth;

		if ( m_iMaxHealth == 0 )
		{
			Assert( false );
			m_iMaxHealth = 1;
		}

		// Output the new health as a percentage of max health [0..1]
		float flRatio = clamp( (float)m_iHealth / (float)m_iMaxHealth, 0.f, 1.f );
		m_OnHealthChanged.Set( flRatio, pActivator, this );

		if ( m_iHealth <= 0 )
		{
			CTakeDamageInfo info;
			info.SetAttacker( this );
			Break( pActivator, info );

			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Advance a ripped-off-animation frame
//-----------------------------------------------------------------------------
bool CBreakableProp::OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	if ( m_nPhysgunState == PHYSGUN_CAN_BE_GRABBED )
		return true;
	if ( m_nPhysgunState == PHYSGUN_ANIMATE_FINISHED )
		return false;

	if ( m_nPhysgunState == PHYSGUN_MUST_BE_DETACHED )
	{
		// A punt advances 
		ResetSequence( SelectWeightedSequence( ACT_PHYSCANNON_DETACH ) );
		SetPlaybackRate( 0.0f );
		ResetClientsideFrame();
		m_nPhysgunState = PHYSGUN_IS_DETACHING;
		return false;
	}

	if ( m_nPhysgunState == PHYSGUN_ANIMATE_ON_PULL )
	{
		// Animation-requiring detachments ignore punts
		if ( reason == PUNTED_BY_CANNON )
			return false;

		// Do we have a pre sequence?
		int iSequence = SelectWeightedSequence( ACT_PHYSCANNON_ANIMATE_PRE );
		if ( iSequence != ACTIVITY_NOT_AVAILABLE )
		{
			m_nPhysgunState = PHYSGUN_ANIMATE_IS_PRE_ANIMATING;
			SetContextThink( &CBreakableProp::AnimateThink, gpGlobals->curtime + 0.1, s_pPropAnimateThink );

			m_OnPhysCannonAnimatePreStarted.FireOutput( NULL,this );
		}
		else
		{
			// Go straight to the animate sequence
			iSequence = SelectWeightedSequence( ACT_PHYSCANNON_ANIMATE );
			m_nPhysgunState = PHYSGUN_ANIMATE_IS_ANIMATING;

			m_OnPhysCannonAnimatePullStarted.FireOutput( NULL,this );
		}

 		ResetSequence( iSequence );
		SetPlaybackRate( 1.0f );
		ResetClientsideFrame();
	}

	// If we're running PRE or POST ANIMATE sequences, wait for them to be done
	if ( m_nPhysgunState == PHYSGUN_ANIMATE_IS_PRE_ANIMATING ||
		 m_nPhysgunState == PHYSGUN_ANIMATE_IS_POST_ANIMATING )
		return false;

	if ( m_nPhysgunState == PHYSGUN_ANIMATE_IS_ANIMATING )
	{
		// Animation-requiring detachments ignore punts
		if ( reason == PUNTED_BY_CANNON )
			return false;

		StudioFrameAdvanceManual( gpGlobals->frametime );
 		DispatchAnimEvents( this );

		if ( IsActivityFinished() )
		{
			int iSequence = SelectWeightedSequence( ACT_PHYSCANNON_ANIMATE_POST );
			if ( iSequence != ACTIVITY_NOT_AVAILABLE )
			{
				m_nPhysgunState = PHYSGUN_ANIMATE_IS_POST_ANIMATING;
				SetContextThink( &CBreakableProp::AnimateThink, gpGlobals->curtime + 0.1, s_pPropAnimateThink );
				ResetSequence( iSequence );
				SetPlaybackRate( 1.0f );
				ResetClientsideFrame();

				m_OnPhysCannonAnimatePostStarted.FireOutput( NULL,this );
			}
			else
			{
				m_nPhysgunState = PHYSGUN_ANIMATE_FINISHED;
				m_OnPhysCannonPullAnimFinished.FireOutput( NULL,this );
			}
		}
	}
	else
	{
		// Here, we're grabbing it. If we try to punt it, advance frames by quite a bit.
		StudioFrameAdvanceManual( (reason == PICKED_UP_BY_CANNON) ? gpGlobals->frametime : 0.5f );
		ResetClientsideFrame();
		DispatchAnimEvents( this );

		if ( IsActivityFinished() )
		{
			// We're done, reset the playback rate.
			SetPlaybackRate( 1.0f );
			m_nPhysgunState = PHYSGUN_CAN_BE_GRABBED;
			m_OnPhysCannonDetach.FireOutput( NULL,this );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Physics Attacker
//-----------------------------------------------------------------------------
void CBreakableProp::AnimateThink( void )
{
	if ( m_nPhysgunState == PHYSGUN_ANIMATE_IS_PRE_ANIMATING || m_nPhysgunState == PHYSGUN_ANIMATE_IS_POST_ANIMATING )
	{
		StudioFrameAdvanceManual( 0.1 );
		DispatchAnimEvents( this );
		SetNextThink( gpGlobals->curtime + 0.1, s_pPropAnimateThink );

		if ( IsActivityFinished() )
		{
			if ( m_nPhysgunState == PHYSGUN_ANIMATE_IS_PRE_ANIMATING )
			{
				// Start the animate sequence
				m_nPhysgunState = PHYSGUN_ANIMATE_IS_ANIMATING;

				ResetSequence( SelectWeightedSequence( ACT_PHYSCANNON_ANIMATE ) );
				SetPlaybackRate( 1.0f );
				ResetClientsideFrame();

				m_OnPhysCannonAnimatePullStarted.FireOutput( NULL,this );
			}
			else
			{
				m_nPhysgunState = PHYSGUN_ANIMATE_FINISHED;
				m_OnPhysCannonPullAnimFinished.FireOutput( NULL,this );
			}

			SetContextThink( NULL, 0, s_pPropAnimateThink );
		}
	}
}
	
//-----------------------------------------------------------------------------
// Physics Attacker
//-----------------------------------------------------------------------------
void CBreakableProp::SetPhysicsAttacker( CBasePlayer *pEntity, float flTime )
{
	m_hPhysicsAttacker = pEntity;
	m_flLastPhysicsInfluenceTime = flTime;
}

	
//-----------------------------------------------------------------------------
// Prevents fade scale from happening
//-----------------------------------------------------------------------------
void CBreakableProp::ForceFadeScaleToAlwaysVisible()
{
	m_flFadeScale = 0.0f;
	SetContextThink( NULL, gpGlobals->curtime, s_pFadeScaleThink );
}


void CBreakableProp::RampToDefaultFadeScale()
{
	m_flFadeScale += m_flDefaultFadeScale * TICK_INTERVAL / 2.0f;
	if ( m_flFadeScale >= m_flDefaultFadeScale )
	{
		m_flFadeScale = m_flDefaultFadeScale;
		SetContextThink( NULL, gpGlobals->curtime, s_pFadeScaleThink );
	}
	else
	{
		SetContextThink( &CBreakableProp::RampToDefaultFadeScale, gpGlobals->curtime + TICK_INTERVAL, s_pFadeScaleThink );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Keep track of physgun influence
//-----------------------------------------------------------------------------
void CBreakableProp::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	// Make sure held objects are always visible
	if ( reason == PICKED_UP_BY_CANNON )
	{
		ForceFadeScaleToAlwaysVisible();
	}
	else
	{
		SetContextThink( &CBreakableProp::RampToDefaultFadeScale, gpGlobals->curtime + 2.0f, s_pFadeScaleThink );
	}

	if( reason == PUNTED_BY_CANNON )
	{
		PlayPuntSound(); 
	}

	if ( IsX360() )
	{
		if( reason != PUNTED_BY_CANNON && (pPhysGunUser->m_nNumCrateHudHints < NUM_SUPPLY_CRATE_HUD_HINTS) )
		{
			if( FClassnameIs( this, "item_item_crate") )
			{
				pPhysGunUser->m_nNumCrateHudHints++;
				UTIL_HudHintText( pPhysGunUser, "#Valve_Hint_Hold_ItemCrate" );
			}
		}
	}

	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );

	// Store original BlockLOS, and disable BlockLOS
	m_bOriginalBlockLOS = BlocksLOS();
	SetBlocksLOS( false );

#ifdef HL2_EPISODIC
	if ( HasInteraction( PROPINTER_PHYSGUN_CREATE_FLARE ) )
	{
		CreateFlare( PROP_FLARE_LIFETIME );
	}
#endif
}


#ifdef HL2_EPISODIC
//-----------------------------------------------------------------------------
// Purpose: Create a flare at the attachment point
//-----------------------------------------------------------------------------
void CBreakableProp::CreateFlare( float flLifetime )
{
	// Create the flare
	CBaseEntity *pFlare = ::CreateFlare( GetAbsOrigin(), GetAbsAngles(), this, flLifetime );
	if ( pFlare )
	{
		int iAttachment = LookupAttachment( "fuse" );

		Vector vOrigin;
		GetAttachment( iAttachment, vOrigin );

		pFlare->SetMoveType( MOVETYPE_NONE );
		pFlare->SetSolid( SOLID_NONE );
		pFlare->SetRenderMode( kRenderTransAlpha );
		pFlare->SetRenderColorA( 1 );
		pFlare->SetLocalOrigin( vOrigin );
		pFlare->SetParent( this, iAttachment );
		RemoveInteraction( PROPINTER_PHYSGUN_CREATE_FLARE );
		m_hFlareEnt = pFlare;

		SetThink( &CBreakable::SUB_FadeOut );
		SetNextThink( gpGlobals->curtime + flLifetime + 5.0f );

		m_nSkin = 1;

		AddEntityToDarknessCheck( pFlare );

		AddEffects( EF_NOSHADOW );
	}
}
#endif // HL2_EPISODIC

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	SetContextThink( &CBreakableProp::RampToDefaultFadeScale, gpGlobals->curtime + 2.0f, s_pFadeScaleThink );

	SetPhysicsAttacker( pPhysGunUser, gpGlobals->curtime );

	if( (int)Reason == (int)PUNTED_BY_CANNON )
	{
		PlayPuntSound(); 
	}

	// Restore original BlockLOS
	SetBlocksLOS( m_bOriginalBlockLOS );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AngularImpulse CBreakableProp::PhysGunLaunchAngularImpulse()
{
	if( HasInteraction( PROPINTER_PHYSGUN_LAUNCH_SPIN_NONE ) || HasInteraction( PROPINTER_PHYSGUN_LAUNCH_SPIN_Z ) )
	{
		// Don't add in random angular impulse if this object is supposed to spin in a specific way.
		AngularImpulse ang( 0, 0, 0 );
		return ang;
	}

	return CDefaultPlayerPickupVPhysics::PhysGunLaunchAngularImpulse();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CBasePlayer *CBreakableProp::HasPhysicsAttacker( float dt )
{
	if (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime)
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::BreakThink( void )
{
	CTakeDamageInfo info;
	info.SetAttacker( this );
	Break( m_hBreaker, info );
}

//-----------------------------------------------------------------------------
// Purpose: Play the sound (if any) that I'm supposed to play when punted.
//-----------------------------------------------------------------------------
void CBreakableProp::PlayPuntSound()
{
	if( !m_bUsePuntSound )
		return;

	if( m_iszPuntSound == NULL_STRING )
		return;

	EmitSound( STRING(m_iszPuntSound) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBreakableProp::Precache()
{
	m_iNumBreakableChunks = PropBreakablePrecacheAll( GetModelName() );

	if( m_iszPuntSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_iszPuntSound) );
	}

	BaseClass::Precache();
}

// Get the root physics object from which all broken pieces will
// derive their positions and velocities
IPhysicsObject *CBreakableProp::GetRootPhysicsObjectForBreak()
{
	return VPhysicsGetObject();
}

void CBreakableProp::Break( CBaseEntity *pBreaker, const CTakeDamageInfo &info )
{
	const char *pModelName = STRING( GetModelName() );
	if ( pModelName && Q_stristr( pModelName, "crate" ) )
	{
		bool bSmashed = false;
		if ( pBreaker && pBreaker->IsPlayer() )
		{
			bSmashed = true;
		}
		else if ( m_hPhysicsAttacker.Get() && m_hPhysicsAttacker->IsPlayer() )
		{
			bSmashed = true;
		}
		else if ( pBreaker && dynamic_cast< CPropVehicleDriveable * >( pBreaker ) )
		{
			CPropVehicleDriveable *veh = static_cast< CPropVehicleDriveable * >( pBreaker );
			CBaseEntity *driver = veh->GetDriver();
			if ( driver && driver->IsPlayer() )
			{
				bSmashed = true;
			}
		}
		if ( bSmashed )
		{
			gamestats->Event_CrateSmashed();
		}
	}

	IGameEvent * event = gameeventmanager->CreateEvent( "break_prop" );

	if ( event )
	{
		if ( pBreaker && pBreaker->IsPlayer() )
		{
			event->SetInt( "userid", ToBasePlayer( pBreaker )->GetUserID() );
		}
		else
		{
			event->SetInt( "userid", 0 );
		}
		event->SetInt( "entindex", entindex() );
		gameeventmanager->FireEvent( event );
	}

	m_takedamage = DAMAGE_NO;
	m_OnBreak.FireOutput( pBreaker, this );

	Vector velocity;
	AngularImpulse angVelocity;
	IPhysicsObject *pPhysics = GetRootPhysicsObjectForBreak();

	Vector origin;
	QAngle angles;
	AddSolidFlags( FSOLID_NOT_SOLID );
	if ( pPhysics )
	{
		pPhysics->GetVelocity( &velocity, &angVelocity );
		pPhysics->GetPosition( &origin, &angles );
		pPhysics->RecheckCollisionFilter();
	}
	else
	{
		velocity = GetAbsVelocity();
		QAngleToAngularImpulse( GetLocalAngularVelocity(), angVelocity );
		origin = GetAbsOrigin();
		angles = GetAbsAngles();
	}

	PhysBreakSound( this, VPhysicsGetObject(), GetAbsOrigin() );

	bool bExploded = false;

	CBaseEntity *pAttacker = info.GetAttacker();
	if ( m_hLastAttacker )
	{
		// Pass along the person who made this explosive breakable explode.
		// This way the player allies can get immunity from barrels exploded by the player.
		pAttacker = m_hLastAttacker;
	}
	else if( m_hPhysicsAttacker )
	{
		// If I have a physics attacker and was influenced in the last 2 seconds,
		// Make the attacker my physics attacker. This helps protect citizens from dying
		// in the explosion of a physics object that was thrown by the player's physgun
		// and exploded on impact.
		if( gpGlobals->curtime - m_flLastPhysicsInfluenceTime <= 2.0f )
		{
			pAttacker = m_hPhysicsAttacker;
		}
	}

	if ( m_explodeDamage > 0 || m_explodeRadius > 0 )
	{
		if( HasInteraction( PROPINTER_PHYSGUN_BREAK_EXPLODE ) )
		{
			ExplosionCreate( WorldSpaceCenter(), angles, pAttacker, m_explodeDamage, m_explodeRadius, 
				SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_SURFACEONLY | SF_ENVEXPLOSION_NOSOUND,
				0.0f, this );
			EmitSound("PropaneTank.Burst");
		}
		else
		{
			float flScale = GetModelScale();
			ExplosionCreate( WorldSpaceCenter(), angles, pAttacker, m_explodeDamage * flScale, m_explodeRadius * flScale,
				SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_SURFACEONLY,
				0.0f, this );
		}

		bExploded = true;
	}

	// Allow derived classes to emit special things
	OnBreak( velocity, angVelocity, pBreaker );

	breakablepropparams_t params( origin, angles, velocity, angVelocity );
	params.impactEnergyScale = m_impactEnergyScale;
	params.defCollisionGroup = GetCollisionGroup();
	if ( params.defCollisionGroup == COLLISION_GROUP_NONE )
	{
		// don't automatically make anything COLLISION_GROUP_NONE or it will
		// collide with debris being ejected by breaking
		params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;
	}
	params.defBurstScale = 100;

	if ( m_iszBreakModelMessage != NULL_STRING )
	{
		CPVSFilter filter( GetAbsOrigin() );
		UserMessageBegin( filter, STRING( m_iszBreakModelMessage ) );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		MessageEnd();

#ifndef HL2MP
		UTIL_Remove( this );
#endif
		return;
	}

	// in multiplayer spawn break models as clientside temp ents
	if ( gpGlobals->maxClients > 1 && breakable_multiplayer.GetBool() )
	{
		CPASFilter filter( WorldSpaceCenter() );

		Vector velocity; velocity.Init();

		if ( pPhysics )
			pPhysics->GetVelocity( &velocity, NULL );

		switch ( GetMultiplayerBreakMode() )
		{
		case MULTIPLAYER_BREAK_DEFAULT:		// default is to break client-side
		case MULTIPLAYER_BREAK_CLIENTSIDE:
			te->PhysicsProp( filter, -1, GetModelIndex(), m_nSkin, GetAbsOrigin(), GetAbsAngles(), velocity, true, GetEffects() );
			break;
		case MULTIPLAYER_BREAK_SERVERSIDE:	// server-side break
			if ( m_PerformanceMode != PM_NO_GIBS || breakable_disable_gib_limit.GetBool() )
			{
				PropBreakableCreateAll( GetModelIndex(), pPhysics, params, this, -1, ( m_PerformanceMode == PM_FULL_GIBS ), false );
			}
			break;
		case MULTIPLAYER_BREAK_BOTH:	// pieces break from both dlls
			te->PhysicsProp( filter, -1, GetModelIndex(), m_nSkin, GetAbsOrigin(), GetAbsAngles(), velocity, true, GetEffects() );
			if ( m_PerformanceMode != PM_NO_GIBS || breakable_disable_gib_limit.GetBool() )
			{
				PropBreakableCreateAll( GetModelIndex(), pPhysics, params, this, -1, ( m_PerformanceMode == PM_FULL_GIBS ), false );
			}
			break;
		}
	}
	// no damage/damage force? set a burst of 100 for some movement
	else if ( m_PerformanceMode != PM_NO_GIBS || breakable_disable_gib_limit.GetBool() )
	{
		PropBreakableCreateAll( GetModelIndex(), pPhysics, params, this, -1, ( m_PerformanceMode == PM_FULL_GIBS ) );
	}

	if( HasInteraction( PROPINTER_PHYSGUN_BREAK_EXPLODE ) )
	{
		if ( bExploded == false )
		{
			ExplosionCreate( origin, angles, pAttacker, 1, m_explodeRadius, 
				SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, this );			
		}

		// Find and ignite all NPC's within the radius
		CBaseEntity *pEntity = NULL;
		for ( CEntitySphereQuery sphere( origin, m_explodeRadius ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			if( pEntity && pEntity->MyCombatCharacterPointer() )
			{
				// Check damage filters so we don't ignite friendlies
				if ( pEntity->PassesDamageFilter( info ) )
				{
					pEntity->MyCombatCharacterPointer()->Ignite( 30 );
				}
			}
		}
	}

#ifndef HL2MP
	UTIL_Remove( this );
#endif
}


//=============================================================================================================
// DYNAMIC PROPS
//=============================================================================================================
LINK_ENTITY_TO_CLASS( dynamic_prop, CDynamicProp );
LINK_ENTITY_TO_CLASS( prop_dynamic, CDynamicProp );	
LINK_ENTITY_TO_CLASS( prop_dynamic_override, CDynamicProp );	

IMPLEMENT_AUTO_LIST( IPhysicsPropAutoList );

BEGIN_DATADESC( CDynamicProp )

	// Fields
	DEFINE_KEYFIELD( m_iszDefaultAnim, FIELD_STRING, "DefaultAnim"),	
	DEFINE_FIELD(	 m_iGoalSequence, FIELD_INTEGER ),
	DEFINE_FIELD(	 m_iTransitionDirection, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_bRandomAnimator, FIELD_BOOLEAN, "RandomAnimation"),	
	DEFINE_FIELD(	 m_flNextRandAnim, FIELD_TIME ),
	DEFINE_KEYFIELD( m_flMinRandAnimTime, FIELD_FLOAT, "MinAnimTime"),
	DEFINE_KEYFIELD( m_flMaxRandAnimTime, FIELD_FLOAT, "MaxAnimTime"),
	DEFINE_KEYFIELD( m_bStartDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_bDisableBoneFollowers, FIELD_BOOLEAN, "DisableBoneFollowers" ),
	DEFINE_FIELD(	 m_bUseHitboxesForRenderBox, FIELD_BOOLEAN ),
	DEFINE_FIELD(	m_nPendingSequence, FIELD_SHORT ),
		
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetAnimation",	InputSetAnimation ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetDefaultAnimation",	InputSetDefaultAnimation ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOn",		InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"TurnOff",		InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Enable",		InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Disable",		InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"EnableCollision",	InputEnableCollision ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"DisableCollision",	InputDisableCollision ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,		"SetPlaybackRate",	InputSetPlaybackRate ),

	// Outputs
	DEFINE_OUTPUT( m_pOutputAnimBegun, "OnAnimationBegun" ),
	DEFINE_OUTPUT( m_pOutputAnimOver, "OnAnimationDone" ),

	// Function Pointers
	DEFINE_THINKFUNC( AnimThink ),

	DEFINE_EMBEDDED( m_BoneFollowerManager ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CDynamicProp, DT_DynamicProp)
	SendPropBool( SENDINFO( m_bUseHitboxesForRenderBox ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CDynamicProp::CDynamicProp()
{
	m_nPendingSequence = -1;
	if ( g_pGameRules->IsMultiplayer() )
	{
		UseClientSideAnimation();
	}
	m_iGoalSequence = -1;
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::Spawn( )
{
	// Condense classname's to one, except for "prop_dynamic_override"
	if ( FClassnameIs( this, "dynamic_prop" ) )
	{
		SetClassname( "prop_dynamic" );
	}

	// If the prop is not-solid, the bounding box needs to be 
	// OBB to correctly surround the prop as it rotates.
	// Check the classname so we don't mess with doors & other derived classes.
	if ( GetSolid() == SOLID_NONE && FClassnameIs( this, "prop_dynamic" ) )
	{
		SetSolid( SOLID_OBB );
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	BaseClass::Spawn();

	if ( IsMarkedForDeletion() )
		return;

	// Now condense all classnames to one
	if ( FClassnameIs( this, "dynamic_prop" ) || FClassnameIs( this, "prop_dynamic_override" )  )
	{
		SetClassname("prop_dynamic");
	}

	AddFlag( FL_STATICPROP );

	if ( m_bRandomAnimator || ( m_iszDefaultAnim != NULL_STRING ) )
	{
		RemoveFlag( FL_STATICPROP );

		if ( m_bRandomAnimator )
		{
			SetThink( &CDynamicProp::AnimThink );
			m_flNextRandAnim = gpGlobals->curtime + random->RandomFloat( m_flMinRandAnimTime, m_flMaxRandAnimTime );
			SetNextThink( gpGlobals->curtime + m_flNextRandAnim + 0.1 );
		}
		else
		{
			PropSetAnim( STRING( m_iszDefaultAnim ) );
		}
	}

	CreateVPhysics();

	BoneFollowerHierarchyChanged();

	if( m_bStartDisabled )
	{
		AddEffects( EF_NODRAW );
	}

	if ( !PropDataOverrodeBlockLOS() )
	{
		CalculateBlockLOS();
	}

	m_bUseHitboxesForRenderBox = HasSpawnFlags( SF_DYNAMICPROP_USEHITBOX_FOR_RENDERBOX );

	if ( HasSpawnFlags( SF_DYNAMICPROP_DISABLE_COLLISION ) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	//m_debugOverlays |= OVERLAY_ABSBOX_BIT;

#ifdef TF_DLL
	const char *pszModelName = modelinfo->GetModelName( GetModel() );
	if ( pszModelName && pszModelName[0] )
	{
		if ( FStrEq( pszModelName, "models/bots/boss_bot/carrier_parts.mdl" ) )
		{
			SetModelIndexOverride( VISION_MODE_NONE, modelinfo->GetModelIndex( pszModelName ) );
			SetModelIndexOverride( VISION_MODE_ROME, modelinfo->GetModelIndex( "models/bots/tw2/boss_bot/twcarrier_addon.mdl" ) );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicProp::OnRestore( void )
{
	BaseClass::OnRestore();

	BoneFollowerHierarchyChanged();
}

void CDynamicProp::SetParent( CBaseEntity *pNewParent, int iAttachment )
{
	BaseClass::SetParent(pNewParent, iAttachment);
	BoneFollowerHierarchyChanged();
}

// Call this when creating bone followers or changing hierarchy to make sure the bone followers get updated when hierarchy changes
void CDynamicProp::BoneFollowerHierarchyChanged()
{
	// If we have bone followers and we're parented to something, we need to constantly update our bone followers
	if ( m_BoneFollowerManager.GetNumBoneFollowers() && GetParent() )
	{
		WatchPositionChanges(this, this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CDynamicProp::OverridePropdata( void )
{
	return ( FClassnameIs(this, "prop_dynamic_override" ) );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CDynamicProp::CreateVPhysics( void )
{
	if ( GetSolid() == SOLID_NONE || ((GetSolidFlags() & FSOLID_NOT_SOLID) && HasSpawnFlags(SF_DYNAMICPROP_NO_VPHYSICS)))
		return true;

	if ( !m_bDisableBoneFollowers )
	{
		CreateBoneFollowers();
	}

	if ( m_BoneFollowerManager.GetNumBoneFollowers() )
	{
		if ( GetSolidFlags() & FSOLID_NOT_SOLID )
		{
			// Already non-solid?  Must need bone followers for some other reason
			// like needing to attach constraints to this object
			for ( int i = 0; i < m_BoneFollowerManager.GetNumBoneFollowers(); i++ )
			{
				CBaseEntity *pFollower = m_BoneFollowerManager.GetBoneFollower(i)->hFollower;
				if ( pFollower )
				{
					pFollower->AddSolidFlags(FSOLID_NOT_SOLID);
				}
			}

		}
		// If our collision is through bone followers, we want to be non-solid
		AddSolidFlags( FSOLID_NOT_SOLID );
		// add these for the client, FSOLID_NOT_SOLID should keep it out of the testCollision code
		// except in the case of TraceEntity() which the client does for impact effects
		AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
		return true;
	}
	else
	{
		VPhysicsInitStatic();
	}
	return true;
}

void CDynamicProp::CreateBoneFollowers()
{
	// already created bone followers?  Don't do so again.
	if ( m_BoneFollowerManager.GetNumBoneFollowers() )
		return;

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

		modelKeyValues->deleteThis();
	}

	// if we got here, we don't have a bone follower section, but if we have a ragdoll
	// go ahead and create default bone followers for it
	if ( m_BoneFollowerManager.GetNumBoneFollowers() == 0 )
	{
		vcollide_t *pCollide = modelinfo->GetVCollide( GetModelIndex() );
		if ( pCollide && pCollide->solidCount > 1 )
		{
			CreateBoneFollowersFromRagdoll(this, &m_BoneFollowerManager, pCollide);
		}
	}
}


bool CDynamicProp::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( IsSolidFlagSet(FSOLID_NOT_SOLID) )
	{
		// if this entity is marked non-solid and custom test it must have bone followers
		if ( IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) && IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ))
		{
			for ( int i = 0; i < m_BoneFollowerManager.GetNumBoneFollowers(); i++ )
			{
				CBaseEntity *pEntity = m_BoneFollowerManager.GetBoneFollower(i)->hFollower;
				if ( pEntity && pEntity->TestCollision(ray, mask, trace) )
					return true;
			}
		}
	}
	return false;
}


IPhysicsObject *CDynamicProp::GetRootPhysicsObjectForBreak()
{
	if ( m_BoneFollowerManager.GetNumBoneFollowers() )
	{
		physfollower_t *pFollower = m_BoneFollowerManager.GetBoneFollower(0);
		CBaseEntity *pFollowerEntity = pFollower->hFollower;
		if ( pFollowerEntity )
		{
			return pFollowerEntity->VPhysicsGetObject();
		}
	}

	return BaseClass::GetRootPhysicsObjectForBreak();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicProp::UpdateOnRemove( void )
{
	m_BoneFollowerManager.DestroyBoneFollowers();
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CDynamicProp::HandleAnimEvent( animevent_t *pEvent )
{ 
	switch( pEvent->event )
	{
		case SCRIPT_EVENT_FIRE_INPUT:
		{
			variant_t emptyVariant;
			this->AcceptInput( pEvent->options, this, this, emptyVariant, 0 );
			return;
		}
		
		case SCRIPT_EVENT_SOUND:
		{
			EmitSound( pEvent->options );
			break;
		}
		
		default:
		{
			break;
		}
	}

	BaseClass::HandleAnimEvent( pEvent ); 
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicProp::NotifyPositionChanged( CBaseEntity *pEntity )
{
	Assert(pEntity==this);
	m_BoneFollowerManager.UpdateBoneFollowers(this);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::AnimThink( void )
{
	if ( m_nPendingSequence != -1 )
	{
		FinishSetSequence( m_nPendingSequence );
		m_nPendingSequence = -1;
	}

	if ( m_bRandomAnimator && m_flNextRandAnim < gpGlobals->curtime )
	{
		ResetSequence( SelectWeightedSequence( ACT_IDLE ) );
		ResetClientsideFrame();

		// Fire output
		m_pOutputAnimBegun.FireOutput( NULL,this );

		m_flNextRandAnim = gpGlobals->curtime + random->RandomFloat( m_flMinRandAnimTime, m_flMaxRandAnimTime );
	}

	if ( ((m_iTransitionDirection > 0 && GetCycle() >= 0.999f) || (m_iTransitionDirection < 0 && GetCycle() <= 0.0f)) && !SequenceLoops() )
	{
		Assert( m_iGoalSequence >= 0 );
		if (GetSequence() != m_iGoalSequence)
		{
			PropSetSequence( m_iGoalSequence );
		}
		else
		{
			// Fire output
			m_pOutputAnimOver.FireOutput(NULL,this);

			// If I'm a random animator, think again when it's time to change sequence
			if ( m_bRandomAnimator )
			{
				SetNextThink( gpGlobals->curtime + m_flNextRandAnim + 0.1 );
			}
			else 
			{
				if (m_iszDefaultAnim != NULL_STRING)
				{
					PropSetAnim( STRING( m_iszDefaultAnim ) );
				}	
			}
		}
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
	}

	StudioFrameAdvance();
	DispatchAnimEvents(this);
	m_BoneFollowerManager.UpdateBoneFollowers(this);
}


//------------------------------------------------------------------------------
// Purpose: Sets an animation by sequence name or activity name.
//------------------------------------------------------------------------------
void CDynamicProp::PropSetAnim( const char *szAnim )
{
	if ( !szAnim )
		return;

	int nSequence = LookupSequence( szAnim );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		PropSetSequence( nSequence );

		// Fire output
		m_pOutputAnimBegun.FireOutput( NULL,this );
	}
	else
	{
		// Not available try to get default anim
		Warning( "Dynamic prop %s: no sequence named:%s\n", GetDebugName(), szAnim );
		SetSequence( 0 );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::InputSetAnimation( inputdata_t &inputdata )
{
	PropSetAnim( inputdata.value.String() );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CDynamicProp::InputSetDefaultAnimation( inputdata_t &inputdata )
{
	m_iszDefaultAnim = inputdata.value.StringID();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CDynamicProp::InputSetPlaybackRate( inputdata_t &inputdata )
{
	SetPlaybackRate( inputdata.value.Float() );
}

//-----------------------------------------------------------------------------
// Purpose: Helper in case we have to async load the sequence
// Input  : nSequence - 
//-----------------------------------------------------------------------------
void CDynamicProp::FinishSetSequence( int nSequence )
{
	// Msg("%.2f CDynamicProp::FinishSetSequence( %d )\n", gpGlobals->curtime, nSequence );
	SetCycle( 0 );
	m_flAnimTime = gpGlobals->curtime;
	ResetSequence( nSequence );
	ResetClientsideFrame();
	RemoveFlag( FL_STATICPROP );
	SetPlaybackRate( m_iTransitionDirection > 0 ? 1.0f : -1.0f );
	SetCycle( m_iTransitionDirection > 0 ? 0.0f : 0.999f );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the sequence and starts thinking.
// Input  : nSequence - 
//-----------------------------------------------------------------------------
void CDynamicProp::PropSetSequence( int nSequence )
{
	m_iGoalSequence = nSequence;

	// Msg("%.2f CDynamicProp::PropSetSequence( %d (%d:%.1f:%.3f)\n", gpGlobals->curtime, nSequence, GetSequence(), GetPlaybackRate(), GetCycle() );

	int nNextSequence;
	float nextCycle;
	float flInterval = 0.1f;

	if (GotoSequence( GetSequence(), GetCycle(), GetPlaybackRate(), m_iGoalSequence, nNextSequence, nextCycle, m_iTransitionDirection ))
	{
		FinishSetSequence( nNextSequence );
	}

	SetThink( &CDynamicProp::AnimThink );
	if ( GetNextThink() <= gpGlobals->curtime )
		SetNextThink( gpGlobals->curtime + flInterval );
}


// NOTE: To avoid risk, currently these do nothing about collisions, only visually on/off
void CDynamicProp::InputTurnOn( inputdata_t &inputdata )
{
	RemoveEffects( EF_NODRAW );
}

void CDynamicProp::InputTurnOff( inputdata_t &inputdata )
{
	AddEffects( EF_NODRAW );
}

void CDynamicProp::InputDisableCollision( inputdata_t &inputdata )
{
	AddSolidFlags( FSOLID_NOT_SOLID );
}

void CDynamicProp::InputEnableCollision( inputdata_t &inputdata )
{
	RemoveSolidFlags( FSOLID_NOT_SOLID );
}

//-----------------------------------------------------------------------------
// Purpose: Ornamental prop that follows a studio
//-----------------------------------------------------------------------------
class COrnamentProp : public CDynamicProp
{
	DECLARE_CLASS( COrnamentProp, CDynamicProp );
public:
	DECLARE_DATADESC();

	void Spawn();
	void Activate();
	void AttachTo( const char *pAttachEntity, CBaseEntity *pActivator = NULL, CBaseEntity *pCaller = NULL );
	void DetachFromOwner();

	// Input handlers
	void InputSetAttached( inputdata_t &inputdata );
	void InputDetach( inputdata_t &inputdata );

private:
	string_t	m_initialOwner;
};

LINK_ENTITY_TO_CLASS( prop_dynamic_ornament, COrnamentProp );	

BEGIN_DATADESC( COrnamentProp )

	DEFINE_KEYFIELD( m_initialOwner, FIELD_STRING, "InitialOwner" ),
	// Inputs
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetAttached",	InputSetAttached ),
	DEFINE_INPUTFUNC( FIELD_VOID,		"Detach",	InputDetach ),

END_DATADESC()

void COrnamentProp::Spawn()
{
	BaseClass::Spawn();
	DetachFromOwner();
}

void COrnamentProp::DetachFromOwner()
{
	SetOwnerEntity( NULL );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );
}

void COrnamentProp::Activate()
{
	BaseClass::Activate();
	
	if ( m_initialOwner != NULL_STRING )
	{
		AttachTo( STRING(m_initialOwner) );
	}
}

void COrnamentProp::InputSetAttached( inputdata_t &inputdata )
{
	AttachTo( inputdata.value.String(), inputdata.pActivator, inputdata.pCaller );
}

void COrnamentProp::AttachTo( const char *pAttachName, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	// find and notify the new parent
	CBaseEntity *pAttach = gEntList.FindEntityByName( NULL, pAttachName, NULL, pActivator, pCaller );
	if ( pAttach )
	{
		RemoveEffects( EF_NODRAW );
		FollowEntity( pAttach );
	}
}

void COrnamentProp::InputDetach( inputdata_t &inputdata )
{
	DetachFromOwner();
}


//=============================================================================
// PHYSICS PROPS
//=============================================================================
LINK_ENTITY_TO_CLASS( physics_prop, CPhysicsProp );
LINK_ENTITY_TO_CLASS( prop_physics, CPhysicsProp );	
LINK_ENTITY_TO_CLASS( prop_physics_override, CPhysicsProp );	

BEGIN_DATADESC( CPhysicsProp )

	DEFINE_INPUTFUNC( FIELD_VOID, "EnableMotion", InputEnableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableMotion", InputDisableMotion ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Wake", InputWake ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Sleep", InputSleep ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DisableFloating", InputDisableFloating ),

	DEFINE_FIELD( m_bAwake, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_massScale, FIELD_FLOAT, "massscale" ),
	DEFINE_KEYFIELD( m_inertiaScale, FIELD_FLOAT, "inertiascale" ),
	DEFINE_KEYFIELD( m_damageType, FIELD_INTEGER, "Damagetype" ),
	DEFINE_KEYFIELD( m_iszOverrideScript, FIELD_STRING, "overridescript" ),

	DEFINE_KEYFIELD( m_damageToEnableMotion, FIELD_INTEGER, "damagetoenablemotion" ), 
	DEFINE_KEYFIELD( m_flForceToEnableMotion, FIELD_FLOAT, "forcetoenablemotion" ), 
	DEFINE_OUTPUT( m_OnAwakened, "OnAwakened" ),
	DEFINE_OUTPUT( m_MotionEnabled, "OnMotionEnabled" ),
	DEFINE_OUTPUT( m_OnPhysGunPickup, "OnPhysGunPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunOnlyPickup, "OnPhysGunOnlyPickup" ),
	DEFINE_OUTPUT( m_OnPhysGunPunt, "OnPhysGunPunt" ),
	DEFINE_OUTPUT( m_OnPhysGunDrop, "OnPhysGunDrop" ),
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
	DEFINE_OUTPUT( m_OnPlayerPickup, "OnPlayerPickup" ),
	DEFINE_OUTPUT( m_OnOutOfWorld, "OnOutOfWorld" ),

	DEFINE_FIELD( m_bThrownByPlayer, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFirstCollisionAfterLaunch, FIELD_BOOLEAN ),

	DEFINE_THINKFUNC( ClearFlagsThink ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPhysicsProp, DT_PhysicsProp )
	SendPropBool( SENDINFO( m_bAwake ) ),
END_SEND_TABLE()

// external function to tell if this entity is a gib physics prop
bool PropIsGib( CBaseEntity *pEntity )
{
	if ( FClassnameIs(pEntity, "prop_physics") )
	{
		CPhysicsProp *pProp = static_cast<CPhysicsProp *>(pEntity);
		return pProp->IsGib();
	}
	return false;
}

CPhysicsProp::~CPhysicsProp()
{
	if (HasSpawnFlags(SF_PHYSPROP_IS_GIB))
	{
		g_ActiveGibCount--;
	}
}

bool CPhysicsProp::IsGib()
{
	return (m_spawnflags & SF_PHYSPROP_IS_GIB) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: Create a physics object for this prop
//-----------------------------------------------------------------------------
void CPhysicsProp::Spawn( )
{
	if (HasSpawnFlags(SF_PHYSPROP_IS_GIB))
	{
		g_ActiveGibCount++;
	}
	// Condense classname's to one, except for "prop_physics_override"
	if ( FClassnameIs( this, "physics_prop" ) )
	{
		SetClassname( "prop_physics" );
	}

	BaseClass::Spawn();

	if ( IsMarkedForDeletion() )
		return;

	// Now condense all classnames to one
	if ( FClassnameIs( this, "prop_physics_override") )
	{
		SetClassname( "prop_physics" );
	}

	if ( HasSpawnFlags( SF_PHYSPROP_DEBRIS ) || HasInteraction( PROPINTER_PHYSGUN_CREATE_FLARE ) )
	{
		SetCollisionGroup( HasSpawnFlags( SF_PHYSPROP_FORCE_TOUCH_TRIGGERS ) ? COLLISION_GROUP_DEBRIS_TRIGGER : COLLISION_GROUP_DEBRIS );
	}

	if ( HasSpawnFlags( SF_PHYSPROP_NO_ROTORWASH_PUSH ) )
	{
		AddEFlags( EFL_NO_ROTORWASH_PUSH );
	}

	CreateVPhysics();

	if ( !PropDataOverrodeBlockLOS() )
	{
		CalculateBlockLOS();
	}

	//Episode 1 change:
	//Hi, since we're trying to ship this game we'll just go ahead and make all these doors not fade out instead of changing all the levels.
	if ( Q_strcmp( STRING( GetModelName() ), "models/props_c17/door01_left.mdl" ) == 0 )
	{
		SetFadeDistance( -1, 0 );
		DisableAutoFade();
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::Precache( void )
{
	if ( GetModelName() == NULL_STRING )
	{
		Msg( "%s at (%.3f, %.3f, %.3f) has no model name!\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
	}
	else
	{
		PrecacheModel( STRING( GetModelName() ) );
		BaseClass::Precache();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPhysicsProp::CreateVPhysics()
{
	// Create the object in the physics system
	bool asleep = HasSpawnFlags( SF_PHYSPROP_START_ASLEEP ) ? true : false;

	solid_t tmpSolid;
	PhysModelParseSolid( tmpSolid, this, GetModelIndex() );
	
	if ( m_massScale > 0 )
	{
		tmpSolid.params.mass *= m_massScale;
	}

	if ( m_inertiaScale > 0 )
	{
		tmpSolid.params.inertia *= m_inertiaScale;
		if ( tmpSolid.params.inertia < 0.5 )
			tmpSolid.params.inertia = 0.5;
	}

	PhysGetMassCenterOverride( this, modelinfo->GetVCollide( GetModelIndex() ), tmpSolid );
	if ( HasSpawnFlags(SF_PHYSPROP_NO_COLLISIONS) )
	{
		tmpSolid.params.enableCollisions = false;
	}
	PhysSolidOverride( tmpSolid, m_iszOverrideScript );

	IPhysicsObject *pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, asleep, &tmpSolid );

	if ( !pPhysicsObject )
	{
		SetSolid( SOLID_NONE );
		SetMoveType( MOVETYPE_NONE );
		Warning("ERROR!: Can't create physics object for %s\n", STRING( GetModelName() ) );
	}
	else
	{
		if ( m_damageType == 1 )
		{
			PhysSetGameFlags( pPhysicsObject, FVPHYSICS_DMG_SLICE );
		}
		if ( HasSpawnFlags( SF_PHYSPROP_MOTIONDISABLED ) || m_damageToEnableMotion > 0 || m_flForceToEnableMotion > 0 )
		{
			pPhysicsObject->EnableMotion( false );
		}
	}

	// fix up any noncompliant blades.
	if( HasInteraction( PROPINTER_PHYSGUN_LAUNCH_SPIN_Z ) )
	{
		if( !(VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_DMG_SLICE) )
		{
			PhysSetGameFlags( pPhysicsObject, FVPHYSICS_DMG_SLICE );

#if 0
			if( g_pDeveloper->GetInt() )
			{
				// Highlight them in developer mode.
				m_debugOverlays |= (OVERLAY_TEXT_BIT|OVERLAY_BBOX_BIT);
			}
#endif
		}
	}

	if( HasInteraction( PROPINTER_PHYSGUN_DAMAGE_NONE ) )
	{
		PhysSetGameFlags( pPhysicsObject, FVPHYSICS_NO_IMPACT_DMG );
	}

	if ( HasSpawnFlags(SF_PHYSPROP_PREVENT_PICKUP) )
	{
		PhysSetGameFlags(pPhysicsObject, FVPHYSICS_NO_PLAYER_PICKUP);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPhysicsProp::CanBePickedUpByPhyscannon( void )
{
	if ( HasSpawnFlags( SF_PHYSPROP_PREVENT_PICKUP ) )
		return false;

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject && pPhysicsObject->IsMoveable() == false )
	{
		if ( HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON ) == false )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CPhysicsProp::OverridePropdata( void )
{
	return ( FClassnameIs(this, "prop_physics_override" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to start the physics prop simulating.
//-----------------------------------------------------------------------------
void CPhysicsProp::InputWake( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->Wake();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Input handler to stop the physics prop simulating.
//-----------------------------------------------------------------------------
void CPhysicsProp::InputSleep( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->Sleep();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Enable physics motion and collision response (on by default)
//-----------------------------------------------------------------------------
void CPhysicsProp::InputEnableMotion( inputdata_t &inputdata )
{
	EnableMotion();
}

//-----------------------------------------------------------------------------
// Purpose: Disable any physics motion or collision response
//-----------------------------------------------------------------------------
void CPhysicsProp::InputDisableMotion( inputdata_t &inputdata )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject != NULL )
	{
		pPhysicsObject->EnableMotion( false );
	}
}

// Turn off floating simulation (and cost)
void CPhysicsProp::InputDisableFloating( inputdata_t &inputdata )
{
	PhysEnableFloating( VPhysicsGetObject(), false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::EnableMotion( void )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		Vector pos;
		QAngle angles;
	
		if ( GetEnableMotionPosition( &pos, &angles ) )
		{
			ClearEnableMotionPosition();
			//pPhysicsObject->SetPosition( pos, angles, true );
			Teleport( &pos, &angles, NULL );
		}

		pPhysicsObject->EnableMotion( true );
		pPhysicsObject->Wake();

		m_MotionEnabled.FireOutput( this, this, 0 );
	}
	CheckRemoveRagdolls();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject && !pPhysicsObject->IsMoveable() )
	{
		if ( !HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON ) )
			return;

		EnableMotion();

		if( HasInteraction( PROPINTER_PHYSGUN_WORLD_STICK ) )
		{
			SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );
		}
	}

	m_OnPhysGunPickup.FireOutput( pPhysGunUser, this );

	if( reason == PICKED_UP_BY_CANNON )
	{
		m_OnPhysGunOnlyPickup.FireOutput( pPhysGunUser, this );
	}

	if ( reason == PUNTED_BY_CANNON )
	{
		m_OnPhysGunPunt.FireOutput( pPhysGunUser, this );
	}

	if ( reason == PICKED_UP_BY_CANNON || reason == PICKED_UP_BY_PLAYER )
	{
		m_OnPlayerPickup.FireOutput( pPhysGunUser, this );
	}

	CheckRemoveRagdolls();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason )
{
	BaseClass::OnPhysGunDrop( pPhysGunUser, Reason );

	if ( Reason == LAUNCHED_BY_CANNON )
	{
		if ( HasInteraction( PROPINTER_PHYSGUN_LAUNCH_SPIN_Z ) )
		{
			AngularImpulse angVel( 0, 0, 5000.0 );
			VPhysicsGetObject()->AddVelocity( NULL, &angVel );
			
			// no angular drag on this object anymore
			float angDrag = 0.0f;
			VPhysicsGetObject()->SetDragCoefficient( NULL, &angDrag );
		}

		PhysSetGameFlags( VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );
		m_bFirstCollisionAfterLaunch = true;
	}
	else if ( Reason == THROWN_BY_PLAYER )
	{
		// Remember the player threw us for NPC response purposes
		m_bThrownByPlayer = true;
	}

	m_OnPhysGunDrop.FireOutput( pPhysGunUser, this );
	
	if ( HasInteraction( PROPINTER_PHYSGUN_NOTIFY_CHILDREN ) )
	{
		CUtlVector<CBaseEntity *> children;
		GetAllChildren( this, children );
		for (int i = 0; i < children.Count(); i++ )
		{
			CBaseEntity *pent = children.Element( i );

			IParentPropInteraction *pPropInter = dynamic_cast<IParentPropInteraction *>( pent );
			if ( pPropInter )
			{
				pPropInter->OnParentPhysGunDrop( pPhysGunUser, Reason );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get the specified key's angles for this prop from the QC's physgun_interactions
//-----------------------------------------------------------------------------
bool CPhysicsProp::GetPropDataAngles( const char *pKeyName, QAngle &vecAngles )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvPropData = modelKeyValues->FindKey( "physgun_interactions" );
		if ( pkvPropData )
		{
			char const *pszBase = pkvPropData->GetString( pKeyName );
			if ( pszBase && pszBase[0] )
			{
				UTIL_StringToVector( vecAngles.Base(), pszBase );
				modelKeyValues->deleteThis();
				return true;
			}
		}
	}

	modelKeyValues->deleteThis();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CPhysicsProp::GetCarryDistanceOffset( void )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvPropData = modelKeyValues->FindKey( "physgun_interactions" );
		if ( pkvPropData )
		{
			float flDistance = pkvPropData->GetFloat( "carry_distance_offset", 0 );
			modelKeyValues->deleteThis();
			return flDistance;
		}
	}

	modelKeyValues->deleteThis();
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysicsProp::ObjectCaps()
{ 
	int caps = BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION;

	if ( HasSpawnFlags( SF_PHYSPROP_ENABLE_PICKUP_OUTPUT ) )
	{
		caps |= FCAP_IMPULSE_USE;
	}
	else if ( CBasePlayer::CanPickupObject( this, 35, 128 ) )
	{
		caps |= FCAP_IMPULSE_USE;

		if( hl2_episodic.GetBool() && HasInteraction( PROPINTER_PHYSGUN_CREATE_FLARE )  )
		{
			caps |= FCAP_USE_IN_RADIUS;
		}
	}

	if( HasSpawnFlags( SF_PHYSPROP_RADIUS_PICKUP ) )
	{
		caps |= FCAP_USE_IN_RADIUS;
	}

	return caps;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CPhysicsProp::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CBasePlayer *pPlayer = ToBasePlayer( pActivator );
	if ( pPlayer )
	{
		if ( HasSpawnFlags( SF_PHYSPROP_ENABLE_PICKUP_OUTPUT ) )
		{
			m_OnPlayerUse.FireOutput( this, this );
		}

		pPlayer->PickupObject( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPhysics - 
//-----------------------------------------------------------------------------
void CPhysicsProp::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );
	m_bAwake = !pPhysics->IsAsleep();
	NetworkStateChanged();
	if ( HasSpawnFlags( SF_PHYSPROP_START_ASLEEP ) )
	{
		if ( m_bAwake )
		{
			m_OnAwakened.FireOutput(this, this);
			RemoveSpawnFlags( SF_PHYSPROP_START_ASLEEP );
		}
	}

	// If we're asleep, clear the player thrown flag
	if ( m_bThrownByPlayer && !m_bAwake )
	{
		m_bThrownByPlayer = false;
	}

	if ( !IsInWorld() )
	{
		m_OnOutOfWorld.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::ClearFlagsThink( void )
{
	// collision may have destroyed the physics object, recheck
	if ( VPhysicsGetObject() )
	{
		PhysClearGameFlags( VPhysicsGetObject(), FVPHYSICS_WAS_THROWN );
	}
	SetContextThink( NULL, 0, "PROP_CLEARFLAGS" );
}


//-----------------------------------------------------------------------------
// Compute impulse to apply to the enabled entity.
//-----------------------------------------------------------------------------
void CPhysicsProp::ComputeEnablingImpulse( int index, gamevcollisionevent_t *pEvent )
{
	// Surface speed of the object that hit us = v + w x r
	// NOTE: w is specified in local space
	Vector vecContactPoint, vecLocalContactPoint;
	pEvent->pInternalData->GetContactPoint( vecContactPoint );

	// Compute the angular component of velocity
	IPhysicsObject *pImpactObject = pEvent->pObjects[!index];
	pImpactObject->WorldToLocal( &vecLocalContactPoint, vecContactPoint );
	vecLocalContactPoint -= pImpactObject->GetMassCenterLocalSpace();

	Vector vecLocalContactVelocity, vecContactVelocity;

	AngularImpulse vecAngularVelocity = pEvent->preAngularVelocity[!index]; 
	vecAngularVelocity *= M_PI / 180.0f;
	CrossProduct( vecAngularVelocity, vecLocalContactPoint, vecLocalContactVelocity );
	pImpactObject->LocalToWorldVector( &vecContactVelocity, vecLocalContactVelocity );

	// Add in the center-of-mass velocity
	vecContactVelocity += pEvent->preVelocity[!index];

	// Compute the force + torque to apply
	vecContactVelocity *= pImpactObject->GetMass();

	Vector vecForce;
	AngularImpulse vecTorque;
	pEvent->pObjects[index]->CalculateForceOffset( vecContactVelocity, vecContactPoint, &vecForce, &vecTorque );

	PhysCallbackImpulse( pEvent->pObjects[index], vecForce, vecTorque );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPhysicsProp::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	IPhysicsObject *pPhysObj = pEvent->pObjects[!index];

	if ( m_flForceToEnableMotion )
	{
		CBaseEntity *pOther = static_cast<CBaseEntity *>(pPhysObj->GetGameData());

		// Don't allow the player to bump an object active if we've requested not to
		if ( ( pOther && pOther->IsPlayer() && HasSpawnFlags( SF_PHYSPROP_PREVENT_PLAYER_TOUCH_ENABLE ) ) == false )
		{
			// Large enough to enable motion?
			float flForce = pEvent->collisionSpeed * pPhysObj->GetMass();
			
			if ( flForce >= m_flForceToEnableMotion )
			{
				ComputeEnablingImpulse( index, pEvent );
				EnableMotion();
				m_flForceToEnableMotion = 0;
			}
		}
	}

	if( m_bFirstCollisionAfterLaunch )
	{
		HandleFirstCollisionInteractions( index, pEvent );
	}

	if ( HasPhysicsAttacker( 2.0f ) )
	{
		HandleAnyCollisionInteractions( index, pEvent );
	}

	if ( !HasSpawnFlags( SF_PHYSPROP_DONT_TAKE_PHYSICS_DAMAGE ) )
	{
		int damageType = 0;

		IBreakableWithPropData *pBreakableInterface = assert_cast<IBreakableWithPropData*>(this);
		float damage = CalculateDefaultPhysicsDamage( index, pEvent, m_impactEnergyScale, true, damageType, pBreakableInterface->GetPhysicsDamageTable() );
		if ( damage > 0 )
		{
			// Take extra damage after we're punted by the physcannon
			if ( m_bFirstCollisionAfterLaunch && !m_bThrownByPlayer )
			{
				damage *= 10;
			}

			CBaseEntity *pHitEntity = pEvent->pEntities[!index];
			if ( !pHitEntity )
			{
				// hit world
				pHitEntity = GetContainingEntity( INDEXENT(0) );
			}
			Vector damagePos;
			pEvent->pInternalData->GetContactPoint( damagePos );
			Vector damageForce = pEvent->postVelocity[index] * pEvent->pObjects[index]->GetMass();
			if ( damageForce == vec3_origin )
			{
				// This can happen if this entity is motion disabled, and can't move.
				// Use the velocity of the entity that hit us instead.
				damageForce = pEvent->postVelocity[!index] * pEvent->pObjects[!index]->GetMass();
			}

			// FIXME: this doesn't pass in who is responsible if some other entity "caused" this collision
			PhysCallbackDamage( this, CTakeDamageInfo( pHitEntity, pHitEntity, damageForce, damagePos, damage, damageType ), *pEvent, index );
		}
	}

	if ( m_bThrownByPlayer || m_bFirstCollisionAfterLaunch )
	{
		// If we were thrown by a player, and we've hit an NPC, let the NPC know
		CBaseEntity *pHitEntity = pEvent->pEntities[!index];
		if ( pHitEntity && pHitEntity->MyNPCPointer() )
		{
			pHitEntity->MyNPCPointer()->DispatchInteraction( g_interactionHitByPlayerThrownPhysObj, this, NULL );
			m_bThrownByPlayer = false;
		}
	}

	if ( m_bFirstCollisionAfterLaunch )
	{
		m_bFirstCollisionAfterLaunch = false;

		// Setup the think function to remove the flags
		RegisterThinkContext( "PROP_CLEARFLAGS" );
		SetContextThink( &CPhysicsProp::ClearFlagsThink, gpGlobals->curtime, "PROP_CLEARFLAGS" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPhysicsProp::OnTakeDamage( const CTakeDamageInfo &info )
{
	// note: if motion is disabled, OnTakeDamage can't apply physics force
	int ret = BaseClass::OnTakeDamage( info );

	if( IsOnFire() )
	{
		if( (info.GetDamageType() & DMG_BURN) && (info.GetDamageType() & DMG_DIRECT) )
		{
			// Burning! scare things in my path if I'm moving.
			Vector vel;

			if( VPhysicsGetObject() )
			{
				VPhysicsGetObject()->GetVelocity( &vel, NULL );

				int dangerRadius = 256; // generous radius to begin with

				if( hl2_episodic.GetBool() )
				{
					// In Episodic, burning items (such as destroyed APCs) are making very large
					// danger sounds which frighten NPCs. This danger sound was designed to frighten
					// NPCs away from burning objects that are about to explode (barrels, etc). 
					// So if this item has no more health (ie, has died but hasn't exploded), 
					// make a smaller danger sound, just to keep NPCs away from the flames. 
					// I suspect this problem didn't appear in HL2 simply because we didn't have 
					// NPCs in such close proximity to destroyed NPCs. (sjb)
					if( GetHealth() < 1 )
					{
						// This item has no health, but still exists. That means that it may keep
						// burning, but isn't likely to explode, so don't frighten over such a large radius.
						dangerRadius = 120;
					}
				}

				trace_t tr;
				UTIL_TraceLine( WorldSpaceCenter(), WorldSpaceCenter() + vel, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
				CSoundEnt::InsertSound( SOUND_DANGER, tr.endpos, dangerRadius, 1.0, this, SOUNDENT_CHANNEL_REPEATED_DANGER );
			}
		}
	}
	
	// If we have a force to enable motion, and we're still disabled, check to see if this should enable us
	if ( m_flForceToEnableMotion )
	{
		// Large enough to enable motion?
		float flForce = info.GetDamageForce().Length();
		if ( flForce >= m_flForceToEnableMotion )
		{
			EnableMotion();
			m_flForceToEnableMotion = 0;
		}
	}

	// Check our health against the threshold:
	if( m_damageToEnableMotion > 0 && GetHealth() < m_damageToEnableMotion )
	{
		// only do this once
		m_damageToEnableMotion = 0;
		
		// The damage that enables motion may have been enough damage to kill me if I'm breakable
		// in which case my physics object is gone.
		if ( VPhysicsGetObject() != NULL )
		{
			EnableMotion(); 
			VPhysicsTakeDamage( info );
		}
	}
	
	return ret;
}


//-----------------------------------------------------------------------------
// Mass / mass center
//-----------------------------------------------------------------------------
void CPhysicsProp::GetMassCenter( Vector *pMassCenter )
{
	if ( !VPhysicsGetObject() )
	{
		pMassCenter->Init();
		return;
	}

	Vector vecLocal = VPhysicsGetObject()->GetMassCenterLocalSpace();
	VectorTransform( vecLocal, EntityToWorldTransform(), *pMassCenter );
}

float CPhysicsProp::GetMass() const
{
	return VPhysicsGetObject() ? VPhysicsGetObject()->GetMass() : 1.0f;
}

	
//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPhysicsProp::DrawDebugTextOverlays(void) 
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

			{
				vphysics_objectstress_t stressOut;
				float stress = CalculateObjectStress( VPhysicsGetObject(), this, &stressOut );
				Q_snprintf(tempstr, sizeof(tempstr),"Stress: %.2f (%.2f / %.2f)", stress, stressOut.exertedStress, stressOut.receivedStress );
				EntityText( text_offset, tempstr, 0);
				text_offset++;
			}

			if ( !VPhysicsGetObject()->IsMoveable() )
			{
				Q_snprintf(tempstr, sizeof(tempstr),"Motion Disabled" );
				EntityText( text_offset, tempstr, 0);
				text_offset++;
			}

			if ( m_iszBasePropData != NULL_STRING )
			{
				Q_snprintf(tempstr, sizeof(tempstr),"Base PropData: %s", STRING(m_iszBasePropData) );
				EntityText( text_offset, tempstr, 0);
				text_offset++;
			}

			if ( m_iNumBreakableChunks != 0 )
			{
				IBreakableWithPropData *pBreakableInterface = assert_cast<IBreakableWithPropData*>(this);
				Q_snprintf(tempstr, sizeof(tempstr),"Breakable Chunks: %d (Max Size %d)", m_iNumBreakableChunks, pBreakableInterface->GetMaxBreakableSize() );
				EntityText( text_offset, tempstr, 0);
				text_offset++;
			}

			Q_snprintf(tempstr, sizeof(tempstr),"Skin: %d", m_nSkin.Get() );
			EntityText( text_offset, tempstr, 0);
			text_offset++;

			Q_snprintf(tempstr, sizeof(tempstr),"Health: %d, collision group %d", GetHealth(), GetCollisionGroup() );
			EntityText( text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}


static CBreakableProp *BreakModelCreate_Prop( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, const QAngle &angles, const breakablepropparams_t &params )
{
	CBreakableProp *pEntity = (CBreakableProp *)CBaseEntity::CreateNoSpawn( "prop_physics", position, angles, pOwner );
	if ( pEntity )
	{
		// UNDONE: Allow .qc to override spawnflags for child pieces
		if ( pOwner )
		{
			pEntity->AddSpawnFlags( pOwner->GetSpawnFlags() );

			// We never want to be motion disabled
			pEntity->RemoveSpawnFlags( SF_PHYSPROP_MOTIONDISABLED );
		}
		pEntity->m_impactEnergyScale = params.impactEnergyScale;	// assume the same material
		// Inherit the base object's damage modifiers
		CBreakableProp *pBreakableOwner = dynamic_cast<CBreakableProp *>(pOwner);
		if ( pBreakableOwner )
		{
			pEntity->SetDmgModBullet( pBreakableOwner->GetDmgModBullet() );
			pEntity->SetDmgModClub( pBreakableOwner->GetDmgModClub() );
			pEntity->SetDmgModExplosive( pBreakableOwner->GetDmgModExplosive() );

			// Copy over the dx7 fade too
			pEntity->CopyFadeFrom( pBreakableOwner );
		}
		pEntity->SetModelName( AllocPooledString( pModel->modelName ) );
		pEntity->SetModel( STRING(pEntity->GetModelName()) );
		pEntity->SetCollisionGroup( pModel->collisionGroup );

		if ( pModel->fadeMinDist > 0 && pModel->fadeMaxDist >= pModel->fadeMinDist )
		{
			pEntity->SetFadeDistance( pModel->fadeMinDist, pModel->fadeMaxDist );
		}

		if ( pModel->fadeTime != 0 )
		{
			pEntity->AddSpawnFlags( SF_PHYSPROP_IS_GIB );
		}
		pEntity->Spawn();

		// If we're burning, break into burning pieces
		CBaseAnimating *pAnimating = dynamic_cast<CBreakableProp *>(pOwner);
		if ( pAnimating && pAnimating->IsOnFire() )
		{
			CEntityFlame *pOwnerFlame = dynamic_cast<CEntityFlame*>( pAnimating->GetEffectEntity() );

			if ( pOwnerFlame )
			{
				pEntity->Ignite( pOwnerFlame->GetRemainingLife(), false );
				pEntity->IgniteNumHitboxFires( pOwnerFlame->GetNumHitboxFires() );
				pEntity->IgniteHitboxFireScale( pOwnerFlame->GetHitboxFireScale() );
			}
			else
			{
				// This should never happen
				pEntity->Ignite( random->RandomFloat( 5, 10 ), false );
			}
		}
	}

	return pEntity;
}

static CBaseAnimating *BreakModelCreate_Ragdoll( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, const QAngle &angles )
{
	CBaseAnimating *pAnimating = CreateServerRagdollSubmodel( dynamic_cast<CBaseAnimating *>(pOwner), pModel->modelName, position, angles, pModel->collisionGroup );
	return pAnimating;
}

CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
	const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params )
{
	CBaseAnimating *pEntity = NULL;
	// stop creating gibs if too many
	if ( g_ActiveGibCount >= ACTIVE_GIB_LIMIT )
	{
		//DevMsg(1,"Gib limit on %s\n", pModel->modelName );
		return NULL;
	}

	if ( !pModel->isRagdoll )
	{
		pEntity = BreakModelCreate_Prop( pOwner, pModel, position, angles, params );
	}
	else
	{
		pEntity = BreakModelCreate_Ragdoll( pOwner, pModel, position, angles );
	}
	if ( pEntity )
	{
		pEntity->m_nSkin = nSkin;
		pEntity->m_iHealth = pModel->health;
		if ( g_ActiveGibCount >= ACTIVE_GIB_FADE )
		{
			pModel->fadeTime = MIN( 3, pModel->fadeTime );
		}
		if ( pModel->fadeTime )
		{
			pEntity->SUB_StartFadeOut( pModel->fadeTime, false );

			CBreakableProp *pProp = dynamic_cast<CBreakableProp *>(pEntity);
			if ( pProp && !pProp->GetNumBreakableChunks() && pProp->m_takedamage == DAMAGE_YES )
			{
				pProp->m_takedamage = DAMAGE_EVENTS_ONLY;
			}
		}

		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		if ( count )
		{
			for ( int i = 0; i < count; i++ )
			{
				pList[i]->SetVelocity( &velocity, &angVelocity );
			}
		}
		else
		{
			// failed to create a physics object
			UTIL_Remove( pEntity );
			return NULL;
		}
	}

	return pEntity;
}

class CBreakModelsPrecached : public CAutoGameSystem
{
public:
	CBreakModelsPrecached() : CAutoGameSystem( "CBreakModelsPrecached" )
	{
		m_modelList.SetLessFunc( BreakLessFunc );
	}

	struct breakable_precache_t
	{
		string_t	iszModelName;
		int			iBreakableCount;
	};

	static bool BreakLessFunc( breakable_precache_t const &lhs, breakable_precache_t const &rhs )
	{
		return ( lhs.iszModelName.ToCStr() < rhs.iszModelName.ToCStr() );
	}

	bool IsInList( string_t modelName, int *iBreakableCount )
	{
		breakable_precache_t sEntry;
		sEntry.iszModelName = modelName;
		int iEntry = m_modelList.Find(sEntry);
		if ( iEntry != m_modelList.InvalidIndex() )
		{
			*iBreakableCount = m_modelList[iEntry].iBreakableCount;
			return true;
		}

		return false;
	}

	void AddToList( string_t modelName, int iBreakableCount )
	{
		breakable_precache_t sEntry;
		sEntry.iszModelName = modelName;
		sEntry.iBreakableCount = iBreakableCount;
		m_modelList.Insert( sEntry );
	}

	void LevelShutdownPostEntity()
	{
		m_modelList.RemoveAll();
	}

private:
	CUtlRBTree<breakable_precache_t>	m_modelList;
};

static CBreakModelsPrecached g_BreakModelsPrecached;

int PropBreakablePrecacheAll( string_t modelName )
{
	int iBreakables = 0;
	if ( g_BreakModelsPrecached.IsInList( modelName, &iBreakables ) )
		return iBreakables;

	if ( modelName == NULL_STRING )
	{
		Msg("Trying to precache breakable prop, but has no model name\n");
		return iBreakables;
	}

	int modelIndex = CBaseEntity::PrecacheModel( STRING(modelName) );

	CUtlVector<breakmodel_t> list;

	BreakModelList( list, modelIndex, COLLISION_GROUP_NONE, 0 );
	iBreakables = list.Count();

	g_BreakModelsPrecached.AddToList( modelName, iBreakables );

	for ( int i = 0; i < iBreakables; i++ )
	{
		string_t breakModelName = AllocPooledString(list[i].modelName);
		if ( modelIndex <= 0 )
		{
			iBreakables--;
			continue;
		}

		PropBreakablePrecacheAll( breakModelName );
	}
	
	return iBreakables;
}

bool PropBreakableCapEdictsOnCreateAll(int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount = -1 )
{
	// @Note (toml 10-07-03): this is stop-gap to prevent this function from crashing the engine
	const int BREATHING_ROOM = 64;

	CUtlVector<breakmodel_t> list;
	BreakModelList( list, modelindex, params.defBurstScale, params.defCollisionGroup );
	
	int numToCreate = 0;

	if ( iPrecomputedBreakableCount != -1 )
	{
		numToCreate = iPrecomputedBreakableCount;
	}
	else
	{
		if ( list.Count() ) 
		{
			for ( int i = 0; i < list.Count(); i++ )
			{
				int modelIndex = modelinfo->GetModelIndex( list[i].modelName );
				if ( modelIndex <= 0 )
					continue;
				numToCreate++;
			}
		}
		// Then see if the propdata specifies any breakable pieces
		else if ( pEntity )
		{
			IBreakableWithPropData *pBreakableInterface = dynamic_cast<IBreakableWithPropData*>(pEntity);
			if ( pBreakableInterface && pBreakableInterface->GetBreakableModel() != NULL_STRING && pBreakableInterface->GetBreakableCount() )
			{
				numToCreate += pBreakableInterface->GetBreakableCount();
			}
		}
	}

	return ( !numToCreate || ( engine->GetEntityCount() + numToCreate + BREATHING_ROOM < MAX_EDICTS ) );
}


//=============================================================================================================
// BASE PROP DOOR
//=============================================================================================================
//
// Private activities.
//
static int ACT_DOOR_OPEN = 0;
static int ACT_DOOR_LOCKED = 0;

//
// Anim events.
//
enum
{
	AE_DOOR_OPEN = 1,	// The door should start opening.
};


void PlayLockSounds(CBaseEntity *pEdict, locksound_t *pls, int flocked, int fbutton);

BEGIN_DATADESC_NO_BASE(locksound_t)

	DEFINE_FIELD( sLockedSound,	FIELD_STRING),
	DEFINE_FIELD( sLockedSentence,	FIELD_STRING ),
	DEFINE_FIELD( sUnlockedSound,	FIELD_STRING ),
	DEFINE_FIELD( sUnlockedSentence, FIELD_STRING ),
	DEFINE_FIELD( iLockedSentence, FIELD_INTEGER ),
	DEFINE_FIELD( iUnlockedSentence, FIELD_INTEGER ),
	DEFINE_FIELD( flwaitSound,		FIELD_FLOAT ),
	DEFINE_FIELD( flwaitSentence,	FIELD_FLOAT ),
	DEFINE_FIELD( bEOFLocked,		FIELD_CHARACTER ),
	DEFINE_FIELD( bEOFUnlocked,	FIELD_CHARACTER ),

END_DATADESC()

BEGIN_DATADESC(CBasePropDoor)
	//DEFINE_FIELD(m_bLockedSentence, FIELD_CHARACTER),
	//DEFINE_FIELD(m_bUnlockedSentence, FIELD_CHARACTER),	
	DEFINE_KEYFIELD(m_nHardwareType, FIELD_INTEGER, "hardware"),
	DEFINE_KEYFIELD(m_flAutoReturnDelay, FIELD_FLOAT, "returndelay"),
	DEFINE_FIELD( m_hActivator, FIELD_EHANDLE ),
	DEFINE_KEYFIELD(m_SoundMoving, FIELD_SOUNDNAME, "soundmoveoverride"),
	DEFINE_KEYFIELD(m_SoundOpen, FIELD_SOUNDNAME, "soundopenoverride"),
	DEFINE_KEYFIELD(m_SoundClose, FIELD_SOUNDNAME, "soundcloseoverride"),
	DEFINE_KEYFIELD(m_ls.sLockedSound, FIELD_SOUNDNAME, "soundlockedoverride"),
	DEFINE_KEYFIELD(m_ls.sUnlockedSound, FIELD_SOUNDNAME, "soundunlockedoverride"),
	DEFINE_KEYFIELD(m_SlaveName, FIELD_STRING, "slavename" ),
	DEFINE_FIELD(m_bLocked, FIELD_BOOLEAN),
	//DEFINE_KEYFIELD(m_flBlockDamage, FIELD_FLOAT, "dmg"),
	DEFINE_KEYFIELD( m_bForceClosed, FIELD_BOOLEAN, "forceclosed" ),
	DEFINE_FIELD(m_eDoorState, FIELD_INTEGER),
	DEFINE_FIELD( m_hMaster, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bFirstBlocked, FIELD_BOOLEAN ),
	//DEFINE_FIELD(m_hDoorList, FIELD_CLASSPTR),	// Reconstructed
	
	DEFINE_INPUTFUNC(FIELD_VOID, "Open", InputOpen),
	DEFINE_INPUTFUNC(FIELD_STRING, "OpenAwayFrom", InputOpenAwayFrom),
	DEFINE_INPUTFUNC(FIELD_VOID, "Close", InputClose),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
	DEFINE_INPUTFUNC(FIELD_VOID, "Lock", InputLock),
	DEFINE_INPUTFUNC(FIELD_VOID, "Unlock", InputUnlock),

	DEFINE_OUTPUT(m_OnBlockedOpening, "OnBlockedOpening"),
	DEFINE_OUTPUT(m_OnBlockedClosing, "OnBlockedClosing"),
	DEFINE_OUTPUT(m_OnUnblockedOpening, "OnUnblockedOpening"),
	DEFINE_OUTPUT(m_OnUnblockedClosing, "OnUnblockedClosing"),
	DEFINE_OUTPUT(m_OnFullyClosed, "OnFullyClosed"),
	DEFINE_OUTPUT(m_OnFullyOpen, "OnFullyOpen"),
	DEFINE_OUTPUT(m_OnClose, "OnClose"),
	DEFINE_OUTPUT(m_OnOpen, "OnOpen"),
	DEFINE_OUTPUT(m_OnLockedUse, "OnLockedUse" ),
	DEFINE_EMBEDDED( m_ls ),

	// Function Pointers
	DEFINE_THINKFUNC(DoorOpenMoveDone),
	DEFINE_THINKFUNC(DoorCloseMoveDone),
	DEFINE_THINKFUNC(DoorAutoCloseThink),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CBasePropDoor, DT_BasePropDoor)
END_SEND_TABLE()

CBasePropDoor::CBasePropDoor( void )
{
	m_hMaster = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Spawn()
{
	BaseClass::Spawn();

	DisableAutoFade();
	Precache();

	DoorTeleportToSpawnPosition();

	if (HasSpawnFlags(SF_DOOR_LOCKED))
	{
		m_bLocked = true;
	}

	SetMoveType(MOVETYPE_PUSH);
	
	if (m_flSpeed == 0)
	{
		m_flSpeed = 100;
	}
	
	RemoveFlag(FL_STATICPROP);

	SetSolid(SOLID_VPHYSICS);
	VPhysicsInitShadow(false, false);
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );

	SetBodygroup( DOOR_HARDWARE_GROUP, m_nHardwareType );
	if ((m_nHardwareType == 0) && (!HasSpawnFlags(SF_DOOR_LOCKED)))
	{
		// Doors with no hardware must always be locked.
		DevWarning(1, "Unlocked prop_door '%s' at (%.0f %.0f %.0f) has no hardware. All openable doors must have hardware!\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
	}

	if ( !PropDataOverrodeBlockLOS() )
	{
		CalculateBlockLOS();
	}

	SetDoorBlocker( NULL );

	// Fills out the m_Soundxxx members.
	CalcDoorSounds();
}


//-----------------------------------------------------------------------------
// Purpose: Returns our capabilities mask.
//-----------------------------------------------------------------------------
int	CBasePropDoor::ObjectCaps()
{
	return BaseClass::ObjectCaps() | ( HasSpawnFlags( SF_DOOR_IGNORE_USE ) ? 0 : (FCAP_IMPULSE_USE|FCAP_USE_IN_RADIUS) );
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Precache(void)
{
	BaseClass::Precache();

	RegisterPrivateActivities();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::RegisterPrivateActivities(void)
{
	static bool bRegistered = false;

	if (bRegistered)
		return;

	REGISTER_PRIVATE_ACTIVITY( ACT_DOOR_OPEN );
	REGISTER_PRIVATE_ACTIVITY( ACT_DOOR_LOCKED );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::Activate( void )
{
	BaseClass::Activate();
	
	UpdateAreaPortals( !IsDoorClosed() );

	// If we have a name, we may be linked
	if ( GetEntityName() != NULL_STRING )
	{
		CBaseEntity	*pTarget = NULL;

		// Find our slaves.
		// If we have a specified slave name, then use that to find slaves.
		// Otherwise, see if there are any other doors that match our name (Backwards compatability).
		string_t iszSearchName = GetEntityName();
		if ( m_SlaveName != NULL_STRING )
		{
			const char *pSlaveName = STRING(m_SlaveName);
			if ( pSlaveName && pSlaveName[0] )
			{
				iszSearchName = m_SlaveName;
			}
		}

		while ( ( pTarget = gEntList.FindEntityByName( pTarget, iszSearchName ) ) != NULL )
		{
			if ( pTarget != this )
			{
				CBasePropDoor *pDoor = dynamic_cast<CBasePropDoor *>(pTarget);

				if ( pDoor != NULL && pDoor->HasSlaves() == false )
				{
					m_hDoorList.AddToTail( pDoor );
					pDoor->SetMaster( this );
					pDoor->SetOwnerEntity( this );
				}
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::HandleAnimEvent(animevent_t *pEvent)
{
	// Opening is called here via an animation event if the open sequence has one,
	// otherwise it is called immediately when the open sequence is set.
	if (pEvent->event == AE_DOOR_OPEN)
	{
		DoorActivate();
	}
}


// Only overwrite str1 if it's NULL_STRING.
#define ASSIGN_STRING_IF_NULL( str1, str2 ) \
	if ( ( str1 ) == NULL_STRING ) { ( str1 ) = ( str2 ); }

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::CalcDoorSounds()
{
	ErrorIfNot( GetModel() != NULL, ( "prop_door with no model at %.2f %.2f %.2f\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z ) );

	string_t strSoundOpen = NULL_STRING;
	string_t strSoundClose = NULL_STRING;
	string_t strSoundMoving = NULL_STRING;
	string_t strSoundLocked = NULL_STRING;
	string_t strSoundUnlocked = NULL_STRING;

	bool bFoundSkin = false;
	// Otherwise, use the sounds specified by the model keyvalues. These are looked up
	// based on skin and hardware.
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvDoorSounds = modelKeyValues->FindKey("door_options");
		if ( pkvDoorSounds )
		{
			// Open / close / move sounds are looked up by skin index.
			char szSkin[80];
			int skin = m_nSkin;
			Q_snprintf( szSkin, sizeof( szSkin ), "skin%d", skin );
			KeyValues *pkvSkinData = pkvDoorSounds->FindKey( szSkin );
			if ( pkvSkinData )
			{
				strSoundOpen = AllocPooledString( pkvSkinData->GetString( "open" ) );
				strSoundClose = AllocPooledString( pkvSkinData->GetString( "close" ) );
				strSoundMoving = AllocPooledString( pkvSkinData->GetString( "move" ) );
				const char *pSurfaceprop = pkvSkinData->GetString( "surfaceprop" );
				if ( pSurfaceprop && VPhysicsGetObject() )
				{
					bFoundSkin = true;
					VPhysicsGetObject()->SetMaterialIndex( physprops->GetSurfaceIndex( pSurfaceprop ) );
				}
			}

			// Locked / unlocked sounds are looked up by hardware index.
			char szHardware[80];
			Q_snprintf( szHardware, sizeof( szHardware ), "hardware%d", m_nHardwareType );
			KeyValues *pkvHardwareData = pkvDoorSounds->FindKey( szHardware );
			if ( pkvHardwareData )
			{
				strSoundLocked = AllocPooledString( pkvHardwareData->GetString( "locked" ) );
				strSoundUnlocked = AllocPooledString( pkvHardwareData->GetString( "unlocked" ) );
			}

			// If any sounds were missing, try the "defaults" block.
			if ( ( strSoundOpen == NULL_STRING ) || ( strSoundClose == NULL_STRING ) || ( strSoundMoving == NULL_STRING ) ||
				 ( strSoundLocked == NULL_STRING ) || ( strSoundUnlocked == NULL_STRING ) )
			{
				KeyValues *pkvDefaults = pkvDoorSounds->FindKey( "defaults" );
				if ( pkvDefaults )
				{
					ASSIGN_STRING_IF_NULL( strSoundOpen, AllocPooledString( pkvDefaults->GetString( "open" ) ) );
					ASSIGN_STRING_IF_NULL( strSoundClose, AllocPooledString( pkvDefaults->GetString( "close" ) ) );
					ASSIGN_STRING_IF_NULL( strSoundMoving, AllocPooledString( pkvDefaults->GetString( "move" ) ) );
					ASSIGN_STRING_IF_NULL( strSoundLocked, AllocPooledString( pkvDefaults->GetString( "locked" ) ) );
					ASSIGN_STRING_IF_NULL( strSoundUnlocked, AllocPooledString( pkvDefaults->GetString( "unlocked" ) ) );
					// NOTE: No default needed for surfaceprop, it's set by the model
				}
			}
		}
	}
	modelKeyValues->deleteThis();
	modelKeyValues = NULL;
	if ( !bFoundSkin && VPhysicsGetObject() )
	{
		Warning( "%s has Door model (%s) with no door_options! Verify that SKIN is valid, and has a corresponding options block in the model QC file\n", GetDebugName(), modelinfo->GetModelName( GetModel() ) );
		VPhysicsGetObject()->SetMaterialIndex( physprops->GetSurfaceIndex("wood") );
	}

	// Any sound data members that are already filled out were specified as level designer overrides,
	// so they should not be overwritten.
	ASSIGN_STRING_IF_NULL( m_SoundOpen, strSoundOpen );
	ASSIGN_STRING_IF_NULL( m_SoundClose, strSoundClose );
	ASSIGN_STRING_IF_NULL( m_SoundMoving, strSoundMoving );
	ASSIGN_STRING_IF_NULL( m_ls.sLockedSound, strSoundLocked );
	ASSIGN_STRING_IF_NULL( m_ls.sUnlockedSound, strSoundUnlocked );

	// Make sure we have real, precachable sound names in all cases.
	UTIL_ValidateSoundName( m_SoundMoving, "DoorSound.Null" );
	UTIL_ValidateSoundName( m_SoundOpen, "DoorSound.Null" );
	UTIL_ValidateSoundName( m_SoundClose, "DoorSound.Null" );
	UTIL_ValidateSoundName( m_ls.sLockedSound, "DoorSound.Null" );
	UTIL_ValidateSoundName( m_ls.sUnlockedSound, "DoorSound.Null" );

	PrecacheScriptSound( STRING( m_SoundMoving ) );
	PrecacheScriptSound( STRING( m_SoundOpen ) );
	PrecacheScriptSound( STRING( m_SoundClose ) );
	PrecacheScriptSound( STRING( m_ls.sLockedSound ) );
	PrecacheScriptSound( STRING( m_ls.sUnlockedSound ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : isOpen - 
//-----------------------------------------------------------------------------
void CBasePropDoor::UpdateAreaPortals(bool isOpen)
{
	string_t name = GetEntityName();
	if (!name)
		return;
	
	CBaseEntity *pPortal = NULL;
	while ((pPortal = gEntList.FindEntityByClassname(pPortal, "func_areaportal")) != NULL)
	{
		if (pPortal->HasTarget(name))
		{
			// USE_ON means open the portal, off means close it
			pPortal->Use(this, this, isOpen?USE_ON:USE_OFF, 0);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CBasePropDoor::SetDoorBlocker( CBaseEntity *pBlocker )
{ 
	m_hBlocker = pBlocker; 

	if ( m_hBlocker == NULL )
	{
		m_bFirstBlocked = false;
	}
}
//-----------------------------------------------------------------------------
// Purpose: Called when the player uses the door.
// Input  : pActivator - 
//			pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBasePropDoor::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if ( GetMaster() != NULL )
	{
		// Tell our owner we've been used
		GetMaster()->Use( pActivator, pCaller, useType, value );
	}
	else
	{
		// Just let it through
		OnUse( pActivator, pCaller, useType, value );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CBasePropDoor::OnUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// If we're blocked while closing, open away from our blocker. This will
	// liberate whatever bit of detritus is stuck in us.
	if ( IsDoorBlocked() && IsDoorClosing() )
	{
		m_hActivator = pActivator;
		DoorOpen( m_hBlocker );
		return;
	}

	if (IsDoorClosed() || (IsDoorOpen() && HasSpawnFlags(SF_DOOR_USE_CLOSES)))
	{
		// Ready to be opened or closed.
		if (m_bLocked)
		{
			PropSetSequence(SelectWeightedSequence((Activity)ACT_DOOR_LOCKED));
			PlayLockSounds(this, &m_ls, TRUE, FALSE);
			m_OnLockedUse.FireOutput( pActivator, pCaller );
		}
		else
		{
			m_hActivator = pActivator;

			PlayLockSounds(this, &m_ls, FALSE, FALSE);
			int nSequence = SelectWeightedSequence((Activity)ACT_DOOR_OPEN);
			PropSetSequence(nSequence);

			if ((nSequence == -1) || !HasAnimEvent(nSequence, AE_DOOR_OPEN))
			{
				// No open anim event, we need to open the door here.
				DoorActivate();
			}
		}
	}
	else if ( IsDoorOpening() && HasSpawnFlags(SF_DOOR_USE_CLOSES) )
	{
		// We've been used while opening, close.
		m_hActivator = pActivator;
		DoorClose();
	}
	else if ( IsDoorClosing() || IsDoorAjar() )
	{
		m_hActivator = pActivator;
		DoorOpen( m_hActivator );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Closes the door if it is not already closed.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputClose(inputdata_t &inputdata)
{
	if (!IsDoorClosed())
	{	
		m_OnClose.FireOutput(inputdata.pActivator, this);
		DoorClose();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that locks the door.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputLock(inputdata_t &inputdata)
{
	Lock();
}


//-----------------------------------------------------------------------------
// Purpose: Opens the door if it is not already open.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputOpen(inputdata_t &inputdata)
{
	OpenIfUnlocked(inputdata.pActivator, NULL);
}


//-----------------------------------------------------------------------------
// Purpose: Opens the door away from a specified entity if it is not already open.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputOpenAwayFrom(inputdata_t &inputdata)
{
	CBaseEntity *pOpenAwayFrom = gEntList.FindEntityByName( NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );
	OpenIfUnlocked(inputdata.pActivator, pOpenAwayFrom);
}


//-----------------------------------------------------------------------------
// Purpose: 
// 
// FIXME: This function should be combined with DoorOpen, but doing that
//		  could break existing content. Fix after shipping!	
//
// Input  : *pOpenAwayFrom - 
//-----------------------------------------------------------------------------
void CBasePropDoor::OpenIfUnlocked(CBaseEntity *pActivator, CBaseEntity *pOpenAwayFrom)
{
	// I'm locked, can't open
	if (m_bLocked)
		return; 

	if (!IsDoorOpen() && !IsDoorOpening())
	{	
		// Play door unlock sounds.
		PlayLockSounds(this, &m_ls, false, false);
		m_OnOpen.FireOutput(pActivator, this);
		DoorOpen(pOpenAwayFrom);
	}
}


//-----------------------------------------------------------------------------
// Purpose: Opens the door if it is not already open.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputToggle(inputdata_t &inputdata)
{
	if (IsDoorClosed())
	{	
		// I'm locked, can't open
		if (m_bLocked)
			return; 

		DoorOpen(NULL);
	}
	else if (IsDoorOpen())
	{
		DoorClose();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Input handler that unlocks the door.
//-----------------------------------------------------------------------------
void CBasePropDoor::InputUnlock(inputdata_t &inputdata)
{
	Unlock();
}


//-----------------------------------------------------------------------------
// Purpose: Locks the door so that it cannot be opened.
//-----------------------------------------------------------------------------
void CBasePropDoor::Lock(void)
{
	m_bLocked = true;
}


//-----------------------------------------------------------------------------
// Purpose: Unlocks the door so that it can be opened.
//-----------------------------------------------------------------------------
void CBasePropDoor::Unlock(void)
{
	if (!m_nHardwareType)
	{
		// Doors with no hardware must always be locked.
		DevWarning(1, "Unlocking prop_door '%s' at (%.0f %.0f %.0f) with no hardware. All openable doors must have hardware!\n", GetDebugName(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
	}

	m_bLocked = false;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the door to "do its thing", i.e. start moving, and cascade activation.
//-----------------------------------------------------------------------------
bool CBasePropDoor::DoorActivate( void )
{
	if ( IsDoorOpen() && DoorCanClose( false ) )
	{
		DoorClose();
	}
	else
	{
		DoorOpen( m_hActivator );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Starts the door opening.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorOpen(CBaseEntity *pOpenAwayFrom)
{
	// Don't bother if we're already doing this
	if ( IsDoorOpen() || IsDoorOpening() )
		return;

	UpdateAreaPortals(true);

	// It could be going-down, if blocked.
	ASSERT( IsDoorClosed() || IsDoorClosing() || IsDoorAjar() );

	// Emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		EmitSound( STRING( m_SoundMoving ) );

		if ( m_hActivator && m_hActivator->IsPlayer() && !HasSpawnFlags( SF_DOOR_SILENT_TO_NPCS ) )
		{
			CSoundEnt::InsertSound( SOUND_PLAYER, GetAbsOrigin(), 512, 0.5, this );//<<TODO>>//magic number
		}
	}

	SetDoorState( DOOR_STATE_OPENING );
	
	SetMoveDone(&CBasePropDoor::DoorOpenMoveDone);

	// Virtual function that starts the door moving for whatever type of door this is.
	BeginOpening(pOpenAwayFrom);

	m_OnOpen.FireOutput(this, this);

	// Tell all the slaves
	if ( HasSlaves() )
	{
		int	numDoors = m_hDoorList.Count();

		CBasePropDoor *pLinkedDoor = NULL;

		// Open all linked doors
		for ( int i = 0; i < numDoors; i++ )
		{
			pLinkedDoor = m_hDoorList[i];

			if ( pLinkedDoor != NULL )
			{
				// If the door isn't already moving, get it moving
				pLinkedDoor->m_hActivator = m_hActivator;
				pLinkedDoor->DoorOpen( pOpenAwayFrom );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The door has reached the open position. Either close automatically
//			or wait for another activation.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorOpenMoveDone(void)
{
	SetDoorBlocker( NULL );

	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		EmitSound( STRING( m_SoundOpen ) );
	}

	ASSERT(IsDoorOpening());
	SetDoorState( DOOR_STATE_OPEN );
	
	if (WillAutoReturn())
	{
		// In flWait seconds, DoorClose will fire, unless wait is -1, then door stays open
		SetMoveDoneTime(m_flAutoReturnDelay + 0.1);
		SetMoveDone(&CBasePropDoor::DoorAutoCloseThink);

		if (m_flAutoReturnDelay == -1)
		{
			SetNextThink( TICK_NEVER_THINK );
		}
	}

	CAI_BaseNPC *pNPC = dynamic_cast<CAI_BaseNPC *>(m_hActivator.Get());
	if (pNPC)
	{
		// Notify the NPC that opened us.
		pNPC->OnDoorFullyOpen(this);
	}

	m_OnFullyOpen.FireOutput(this, this);

	// Let the leaf class do its thing.
	OnDoorOpened();

	m_hActivator = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Think function that tries to close the door. Used for autoreturn.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorAutoCloseThink(void)
{
	// When autoclosing, we check both sides so that we don't close in the player's
	// face, or in an NPC's face for that matter, because they might be shooting
	// through the doorway.
	if ( !DoorCanClose( true ) )
	{
		if (m_flAutoReturnDelay == -1)
		{
			SetNextThink( TICK_NEVER_THINK );
		}
		else
		{
			// In flWait seconds, DoorClose will fire, unless wait is -1, then door stays open
			SetMoveDoneTime(m_flAutoReturnDelay + 0.1);
			SetMoveDone(&CBasePropDoor::DoorAutoCloseThink);
		}

		return;
	}

	DoorClose();
}


//-----------------------------------------------------------------------------
// Purpose: Starts the door closing.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorClose(void)
{
	// Don't bother if we're already doing this
	if ( IsDoorClosed() || IsDoorClosing() )
		return;

	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		EmitSound( STRING( m_SoundMoving ) );

		if ( m_hActivator && m_hActivator->IsPlayer() )
		{
			CSoundEnt::InsertSound( SOUND_PLAYER, GetAbsOrigin(), 512, 0.5, this );//<<TODO>>//magic number
		}
	}
	
	ASSERT(IsDoorOpen() || IsDoorOpening());
	SetDoorState( DOOR_STATE_CLOSING );

	SetMoveDone(&CBasePropDoor::DoorCloseMoveDone);

	// This will set the movedone time.
	BeginClosing();

	m_OnClose.FireOutput(this, this);

	// Tell all the slaves
	if ( HasSlaves() )
	{
		int	numDoors = m_hDoorList.Count();

		CBasePropDoor *pLinkedDoor = NULL;

		// Open all linked doors
		for ( int i = 0; i < numDoors; i++ )
		{
			pLinkedDoor = m_hDoorList[i];

			if ( pLinkedDoor != NULL )
			{
				// If the door isn't already moving, get it moving
				pLinkedDoor->DoorClose();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: The door has reached the closed position. Return to quiescence.
//-----------------------------------------------------------------------------
void CBasePropDoor::DoorCloseMoveDone(void)
{
	SetDoorBlocker( NULL );

	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		StopSound( STRING( m_SoundMoving ) );
		EmitSound( STRING( m_SoundClose ) );
	}

	ASSERT(IsDoorClosing());
	SetDoorState( DOOR_STATE_CLOSED );

	m_OnFullyClosed.FireOutput(m_hActivator, this);
	UpdateAreaPortals(false);

	// Let the leaf class do its thing.
	OnDoorClosed();

	m_hActivator = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBasePropDoor::MasterStartBlocked( CBaseEntity *pOther )
{
	if ( HasSlaves() )
	{
		int	numDoors = m_hDoorList.Count();

		CBasePropDoor *pLinkedDoor = NULL;

		// Open all linked doors
		for ( int i = 0; i < numDoors; i++ )
		{
			pLinkedDoor = m_hDoorList[i];

			if ( pLinkedDoor != NULL )
			{
				// If the door isn't already moving, get it moving
				pLinkedDoor->OnStartBlocked( pOther );
			}
		}
	}

	// Start ourselves blocked
	OnStartBlocked( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: Called the first frame that the door is blocked while opening or closing.
// Input  : pOther - The blocking entity.
//-----------------------------------------------------------------------------
void CBasePropDoor::StartBlocked( CBaseEntity *pOther )
{
	m_bFirstBlocked = true;

	if ( GetMaster() != NULL )
	{
		GetMaster()->MasterStartBlocked( pOther );
		return;
	}

	// Start ourselves blocked
	OnStartBlocked( pOther );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CBasePropDoor::OnStartBlocked( CBaseEntity *pOther )
{
	if ( m_bFirstBlocked == false )
	{
		DoorStop();
	}

	SetDoorBlocker( pOther );

	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		StopSound( STRING( m_SoundMoving ) );
	}

	//
	// Fire whatever events we need to due to our blocked state.
	//
	if (IsDoorClosing())
	{
		// Closed into an NPC, open.
		if ( pOther->MyNPCPointer() )
		{
			DoorOpen( pOther );
		}
		m_OnBlockedClosing.FireOutput(pOther, this);
	}
	else
	{
		// Opened into an NPC, close.
		if ( pOther->MyNPCPointer() )
		{
			DoorClose();
		}

		CAI_BaseNPC *pNPC = dynamic_cast<CAI_BaseNPC *>(m_hActivator.Get());
		
		if ( pNPC != NULL )
		{
			// Notify the NPC that tried to open us.
			pNPC->OnDoorBlocked( this );
		}

		m_OnBlockedOpening.FireOutput( pOther, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called every frame when the door is blocked while opening or closing.
// Input  : pOther - The blocking entity.
//-----------------------------------------------------------------------------
void CBasePropDoor::Blocked(CBaseEntity *pOther)
{
	// dvs: TODO: will prop_door apply any blocking damage?
	// Hurt the blocker a little.
	//if (m_flBlockDamage)
	//{
	//	pOther->TakeDamage(CTakeDamageInfo(this, this, m_flBlockDamage, DMG_CRUSH));
	//}

	if ( m_bForceClosed && ( pOther->GetMoveType() == MOVETYPE_VPHYSICS ) &&
		 ( pOther->m_takedamage == DAMAGE_NO || pOther->m_takedamage == DAMAGE_EVENTS_ONLY ) )
	{
		EntityPhysics_CreateSolver( this, pOther, true, 4.0f );
	}
	else if ( m_bForceClosed && ( pOther->GetMoveType() == MOVETYPE_VPHYSICS ) && ( pOther->m_takedamage == DAMAGE_YES ) )
	{
		pOther->TakeDamage( CTakeDamageInfo( this, this, pOther->GetHealth(), DMG_CRUSH ) );
	}

	// If we're set to force ourselves closed, keep going
	if ( m_bForceClosed )
		return;

	// If a door has a negative wait, it would never come back if blocked,
	// so let it just squash the object to death real fast.
//	if (m_flAutoReturnDelay >= 0)
//	{
//		if (IsDoorClosing())
//		{
//			DoorOpen();
//		}
//		else
//		{
//			DoorClose();
//		}
//	}

	// Block all door pieces with the same targetname here.
//	if (GetEntityName() != NULL_STRING)
//	{
//		CBaseEntity pTarget = NULL;
//		for (;;)
//		{
//			pTarget = gEntList.FindEntityByName(pTarget, GetEntityName() );
//
//			if (pTarget != this)
//			{
//				if (!pTarget)
//					break;
//
//				if (FClassnameIs(pTarget, "prop_door_rotating"))
//				{
//					CPropDoorRotating *pDoor = (CPropDoorRotating *)pTarget;
//
//					if (pDoor->m_fAutoReturnDelay >= 0)
//					{
//						if (pDoor->GetAbsVelocity() == GetAbsVelocity() && pDoor->GetLocalAngularVelocity() == GetLocalAngularVelocity())
//						{
//							// this is the most hacked, evil, bastardized thing I've ever seen. kjb
//							if (FClassnameIs(pTarget, "prop_door_rotating"))
//							{
//								// set angles to realign rotating doors
//								pDoor->SetLocalAngles(GetLocalAngles());
//								pDoor->SetLocalAngularVelocity(vec3_angle);
//							}
//							else
//							//{
//							//	// set origin to realign normal doors
//							//	pDoor->SetLocalOrigin(GetLocalOrigin());
//							//	pDoor->SetAbsVelocity(vec3_origin);// stop!
//							//}
//						}
//
//						if (IsDoorClosing())
//						{
//							pDoor->DoorOpen();
//						}
//						else
//						{
//							pDoor->DoorClose();
//						}
//					}
//				}
//			}
//		}
//	}
}


//-----------------------------------------------------------------------------
// Purpose: Called the first frame that the door is unblocked while opening or closing.
//-----------------------------------------------------------------------------
void CBasePropDoor::EndBlocked( void )
{
	if ( GetMaster() != NULL )
	{
		GetMaster()->EndBlocked();
		return;
	}

	if ( HasSlaves() )
	{
		int	numDoors = m_hDoorList.Count();

		CBasePropDoor *pLinkedDoor = NULL;

		// Check all links as well
		for ( int i = 0; i < numDoors; i++ )
		{
			pLinkedDoor = m_hDoorList[i];

			if ( pLinkedDoor != NULL )
			{
				// Make sure they can close as well
				pLinkedDoor->OnEndBlocked();
			}
		}
	}

	// Emit door moving and stop sounds on CHAN_STATIC so that the multicast doesn't
	// filter them out and leave a client stuck with looping door sounds!
	if (!HasSpawnFlags(SF_DOOR_SILENT))
	{
		EmitSound( STRING( m_SoundMoving ) );
	}

	//
	// Fire whatever events we need to due to our unblocked state.
	//
	if (IsDoorClosing())
	{
		m_OnUnblockedClosing.FireOutput(this, this);
	}
	else
	{
		m_OnUnblockedOpening.FireOutput(this, this);
	}

	OnEndBlocked();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePropDoor::OnEndBlocked( void )
{
	if ( m_bFirstBlocked )
		return;

	// Restart us going
	DoorResume();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNPC - 
//-----------------------------------------------------------------------------
bool CBasePropDoor::NPCOpenDoor( CAI_BaseNPC *pNPC )
{
	// dvs: TODO: use activator filter here
	// dvs: TODO: outboard entity containing rules for whether door is operable?
	
	if ( IsDoorClosed() )
	{
		// Use the door
		Use( pNPC, pNPC, USE_ON, 0 );
	}

	return true;
}

bool CBasePropDoor::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !VPhysicsGetObject() )
		return false;

	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
		return false;

	if ( !( pStudioHdr->contents() & mask ) )
		return false;

	physcollision->TraceBox( ray, VPhysicsGetObject()->GetCollide(), GetAbsOrigin(), GetAbsAngles(), &trace );

	if ( trace.DidHit() )
	{
		trace.surface.surfaceProps = VPhysicsGetObject()->GetMaterialIndex();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Custom trace filter for doors
// Will only test against entities and rejects physics objects below a mass threshold
//-----------------------------------------------------------------------------

class CTraceFilterDoor : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterDoor );
	
	CTraceFilterDoor( const IHandleEntity *pDoor, const IHandleEntity *passentity, int collisionGroup )
		: m_pDoor(pDoor), m_pPassEnt(passentity), m_collisionGroup(collisionGroup)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pDoor ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_pGameRules->ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			// If objects are small enough and can move, close on them
			if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
				Assert(pPhysics);
				
				// Must either be squashable or very light
				if ( pPhysics->IsMoveable() && pPhysics->GetMass() < 32 )
					return false;
			}
		}

		return true;
	}

private:

	const IHandleEntity *m_pDoor;
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
};

inline void TraceHull_Door( const CBasePropDoor *pDoor, const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const CBaseEntity *ignore, 
					 int collisionGroup, trace_t *ptr )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterDoor traceFilter( pDoor, ignore, collisionGroup );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

// Check directions for door movement
enum doorCheck_e
{
	DOOR_CHECK_FORWARD,		// Door's forward opening direction
	DOOR_CHECK_BACKWARD,	// Door's backward opening direction
	DOOR_CHECK_FULL,		// Door's complete movement volume
};


enum PropDoorRotatingSpawnPos_t
{
	DOOR_SPAWN_CLOSED = 0,
	DOOR_SPAWN_OPEN_FORWARD,
	DOOR_SPAWN_OPEN_BACK,
	DOOR_SPAWN_AJAR,
};

enum PropDoorRotatingOpenDirection_e
{
	DOOR_ROTATING_OPEN_BOTH_WAYS = 0,
	DOOR_ROTATING_OPEN_FORWARD,
	DOOR_ROTATING_OPEN_BACKWARD,
};

//===============================================
// Rotating prop door
//===============================================

class CPropDoorRotating : public CBasePropDoor
{
	DECLARE_CLASS( CPropDoorRotating, CBasePropDoor );

public:

	~CPropDoorRotating();

	int		DrawDebugTextOverlays(void);

	void	Spawn( void );
	void	MoveDone( void );
	void	BeginOpening(CBaseEntity *pOpenAwayFrom);
	void	BeginClosing( void );
	void	OnRestore( void );

	void	DoorTeleportToSpawnPosition();

	void	GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata);

	void	DoorClose( void );
	bool	DoorCanClose( bool bAutoClose );
	void	DoorOpen( CBaseEntity *pOpenAwayFrom );

	void	OnDoorOpened();
	void	OnDoorClosed();

	void	DoorResume( void );
	void	DoorStop( void );

	float	GetOpenInterval();

	bool	OverridePropdata() { return true; }

	void	InputSetSpeed(inputdata_t &inputdata);

	DECLARE_DATADESC();

private:

	bool	IsHingeOnLeft();

	void	AngularMove(const QAngle &vecDestAngle, float flSpeed);
	void	CalculateDoorVolume( QAngle closedAngles, QAngle openAngles, Vector *destMins, Vector *destMaxs );

	bool	CheckDoorClear( doorCheck_e state );
	
	doorCheck_e	GetOpenState( void );

	void	InputSetRotationDistance ( inputdata_t &inputdata );			// Set the degree difference between open and closed

	void	CalcOpenAngles ( void );		// Subroutine to setup the m_angRotation QAngles based on the m_flDistance variable

	Vector	m_vecAxis;					// The axis of rotation.
	float	m_flDistance;				// How many degrees we rotate between open and closed.

	PropDoorRotatingSpawnPos_t m_eSpawnPosition;
	PropDoorRotatingOpenDirection_e m_eOpenDirection;

	QAngle	m_angRotationAjar;			// Angles to spawn at if we are set to spawn ajar.
	QAngle	m_angRotationClosed;		// Our angles when we are fully closed.
	QAngle	m_angRotationOpenForward;	// Our angles when we are fully open towards our forward vector.
	QAngle	m_angRotationOpenBack;		// Our angles when we are fully open away from our forward vector.

	QAngle	m_angGoal;

	Vector	m_vecForwardBoundsMin;
	Vector	m_vecForwardBoundsMax;
	Vector	m_vecBackBoundsMin;
	Vector	m_vecBackBoundsMax;

	CHandle<CEntityBlocker>	m_hDoorBlocker;
};


BEGIN_DATADESC(CPropDoorRotating)
	DEFINE_KEYFIELD(m_eSpawnPosition, FIELD_INTEGER, "spawnpos"),
	DEFINE_KEYFIELD(m_eOpenDirection, FIELD_INTEGER, "opendir" ),
	DEFINE_KEYFIELD(m_vecAxis, FIELD_VECTOR, "axis"),
	DEFINE_KEYFIELD(m_flDistance, FIELD_FLOAT, "distance"),
	DEFINE_KEYFIELD( m_angRotationAjar, FIELD_VECTOR, "ajarangles" ),
	DEFINE_FIELD( m_angRotationClosed, FIELD_VECTOR ),
	DEFINE_FIELD( m_angRotationOpenForward, FIELD_VECTOR ),
	DEFINE_FIELD( m_angRotationOpenBack, FIELD_VECTOR ),
	DEFINE_FIELD( m_angGoal, FIELD_VECTOR ),
	DEFINE_FIELD( m_hDoorBlocker, FIELD_EHANDLE ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRotationDistance", InputSetRotationDistance ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetSpeed", InputSetSpeed ),
	//m_vecForwardBoundsMin
	//m_vecForwardBoundsMax
	//m_vecBackBoundsMin
	//m_vecBackBoundsMax
END_DATADESC()

LINK_ENTITY_TO_CLASS(prop_door_rotating, CPropDoorRotating);

//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CPropDoorRotating::~CPropDoorRotating( void )
{
	// Remove our door blocker entity
	if ( m_hDoorBlocker != NULL )
	{
		UTIL_Remove( m_hDoorBlocker );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &mins1 - 
//			&maxs1 - 
//			&mins2 - 
//			&maxs2 - 
//			*destMins - 
//			*destMaxs - 
//-----------------------------------------------------------------------------
void UTIL_ComputeAABBForBounds( const Vector &mins1, const Vector &maxs1, const Vector &mins2, const Vector &maxs2, Vector *destMins, Vector *destMaxs )
{
	// Find the minimum extents
	(*destMins)[0] = MIN( mins1[0], mins2[0] );
	(*destMins)[1] = MIN( mins1[1], mins2[1] );
	(*destMins)[2] = MIN( mins1[2], mins2[2] );

	// Find the maximum extents
	(*destMaxs)[0] = MAX( maxs1[0], maxs2[0] );
	(*destMaxs)[1] = MAX( maxs1[1], maxs2[1] );
	(*destMaxs)[2] = MAX( maxs1[2], maxs2[2] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::Spawn()
{
	// Doors are built closed, so save the current angles as the closed angles.
	m_angRotationClosed = GetLocalAngles();

	// The axis of rotation must be along the z axis for now.
	// NOTE: If you change this, be sure to change IsHingeOnLeft to account for it!
	m_vecAxis = Vector(0, 0, 1);

	CalcOpenAngles();

	// Call this last! It relies on stuff we calculated above.
	BaseClass::Spawn();

	// We have to call this after we call the base Spawn because it requires
	// that the model already be set.
	if ( IsHingeOnLeft() )
	{
		::V_swap( m_angRotationOpenForward, m_angRotationOpenBack );
	}

	// Figure out our volumes of movement as this door opens
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenForward, &m_vecForwardBoundsMin, &m_vecForwardBoundsMax );
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenBack, &m_vecBackBoundsMin, &m_vecBackBoundsMax );
}

//-----------------------------------------------------------------------------
// Purpose: Setup the m_angRotationOpenForward and m_angRotationOpenBack variables based on
//			the m_flDistance variable. Also restricts m_flDistance > 0.
//-----------------------------------------------------------------------------
void CPropDoorRotating::CalcOpenAngles()
{
	// HACK: convert the axis of rotation to dPitch dYaw dRoll
	Vector vecMoveDir(m_vecAxis.y, m_vecAxis.z, m_vecAxis.x); 

	if (m_flDistance == 0)
	{
		m_flDistance = 90;
	}
	m_flDistance = fabs(m_flDistance);

	// Calculate our orientation when we are fully open.
	m_angRotationOpenForward.x = m_angRotationClosed.x - (vecMoveDir.x * m_flDistance);
	m_angRotationOpenForward.y = m_angRotationClosed.y - (vecMoveDir.y * m_flDistance);
	m_angRotationOpenForward.z = m_angRotationClosed.z - (vecMoveDir.z * m_flDistance);

	m_angRotationOpenBack.x = m_angRotationClosed.x + (vecMoveDir.x * m_flDistance);
	m_angRotationOpenBack.y = m_angRotationClosed.y + (vecMoveDir.y * m_flDistance);
	m_angRotationOpenBack.z = m_angRotationClosed.z + (vecMoveDir.z * m_flDistance);
}

//-----------------------------------------------------------------------------
// Figures out whether the door's hinge is on its left or its right.
// Assumes:
// - that the door is hinged through its origin.
// - that the origin is at one edge of the door (revolving doors will give
//   a random answer)
// - that the hinge axis lies along the z axis
//-----------------------------------------------------------------------------
bool CPropDoorRotating::IsHingeOnLeft()
{
	//
	// Find the point farthest from the hinge in 2D.
	//
	Vector vecMins;
	Vector vecMaxs;
	CollisionProp()->WorldSpaceAABB( &vecMins, &vecMaxs );

	vecMins -= GetAbsOrigin();
	vecMaxs -= GetAbsOrigin();

	// Throw out z -- we only care about 2D distance.
	// NOTE: if we allow for arbitrary hinge axes, this needs to change
	vecMins.z = vecMaxs.z = 0;

	Vector vecPointCheck;
	if ( vecMins.LengthSqr() > vecMaxs.LengthSqr() )
	{
		vecPointCheck = vecMins;
	}
	else
	{
		vecPointCheck = vecMaxs;
	}

	//
	// See if the projection of that point lies along our right vector.
	// If it does, the door is hinged on its left.
	//
	Vector vecRight;
	GetVectors( NULL, &vecRight, NULL );
	float flDot = DotProduct( vecPointCheck, vecRight );

	return ( flDot > 0 );
}	


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
doorCheck_e CPropDoorRotating::GetOpenState( void )
{
	return ( m_angGoal == m_angRotationOpenForward ) ? DOOR_CHECK_FORWARD : DOOR_CHECK_BACKWARD;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::OnDoorOpened( void )
{
	if ( m_hDoorBlocker != NULL )
	{
		// Allow passage through this blocker while open
		m_hDoorBlocker->AddSolidFlags( FSOLID_NOT_SOLID );

		if ( g_debug_doors.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), m_hDoorBlocker->CollisionProp()->OBBMins(), m_hDoorBlocker->CollisionProp()->OBBMaxs(), 0, 255, 0, true, 1.0f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::OnDoorClosed( void )
{
	if ( m_hDoorBlocker != NULL )
	{
		// Destroy the blocker that was preventing NPCs from getting in our way.
		UTIL_Remove( m_hDoorBlocker );
		
		if ( g_debug_doors.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), m_hDoorBlocker->CollisionProp()->OBBMins(), m_hDoorBlocker->CollisionProp()->OBBMaxs(), 0, 255, 0, true, 1.0f );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Returns whether the way is clear for the door to close.
// Input  : state - Which sides to check, forward, backward, or both.
// Output : Returns true if the door can close, false if the way is blocked.
//-----------------------------------------------------------------------------
bool CPropDoorRotating::DoorCanClose( bool bAutoClose )
{
	if ( GetMaster() != NULL )
		return GetMaster()->DoorCanClose( bAutoClose );
	
	// Check all slaves
	if ( HasSlaves() )
	{
		int	numDoors = m_hDoorList.Count();

		CPropDoorRotating *pLinkedDoor = NULL;

		// Check all links as well
		for ( int i = 0; i < numDoors; i++ )
		{
			pLinkedDoor = dynamic_cast<CPropDoorRotating *>((CBasePropDoor *)m_hDoorList[i]);

			if ( pLinkedDoor != NULL )
			{
				if ( !pLinkedDoor->CheckDoorClear( bAutoClose ? DOOR_CHECK_FULL : pLinkedDoor->GetOpenState() ) )
					return false;
			}
		}
	}
	
	// See if our path of movement is clear to allow us to shut
	return CheckDoorClear( bAutoClose ? DOOR_CHECK_FULL : GetOpenState() );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : closedAngles - 
//			openAngles - 
//			*destMins - 
//			*destMaxs - 
//-----------------------------------------------------------------------------
void CPropDoorRotating::CalculateDoorVolume( QAngle closedAngles, QAngle openAngles, Vector *destMins, Vector *destMaxs )
{
	// Save our current angles and move to our start angles
	QAngle	saveAngles = GetLocalAngles();
	SetLocalAngles( closedAngles );

	// Find our AABB at the closed state
	Vector	closedMins, closedMaxs;
	CollisionProp()->WorldSpaceAABB( &closedMins, &closedMaxs );
	
	SetLocalAngles( openAngles );

	// Find our AABB at the open state
	Vector	openMins, openMaxs;
	CollisionProp()->WorldSpaceAABB( &openMins, &openMaxs );

	// Reset our angles to our starting angles
	SetLocalAngles( saveAngles );

	// Find the minimum extents
	UTIL_ComputeAABBForBounds( closedMins, closedMaxs, openMins, openMaxs, destMins, destMaxs );
	
	// Move this back into local space
	*destMins -= GetAbsOrigin();
	*destMaxs -= GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::OnRestore( void )
{
	BaseClass::OnRestore();

	// Figure out our volumes of movement as this door opens
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenForward, &m_vecForwardBoundsMin, &m_vecForwardBoundsMax );
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenBack, &m_vecBackBoundsMin, &m_vecBackBoundsMax );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : forward - 
//			mask - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPropDoorRotating::CheckDoorClear( doorCheck_e state )
{
	Vector moveMins;
	Vector moveMaxs;

	switch ( state )
	{
	case DOOR_CHECK_FORWARD:
		moveMins = m_vecForwardBoundsMin;
		moveMaxs = m_vecForwardBoundsMax;
		break;

	case DOOR_CHECK_BACKWARD:
		moveMins = m_vecBackBoundsMin;
		moveMaxs = m_vecBackBoundsMax;
		break;

	default:
	case DOOR_CHECK_FULL:
		UTIL_ComputeAABBForBounds( m_vecForwardBoundsMin, m_vecForwardBoundsMax, m_vecBackBoundsMin, m_vecBackBoundsMax, &moveMins, &moveMaxs );
		break;
	}

	// Look for blocking entities, ignoring ourselves and the entity that opened us.
	trace_t	tr;
	TraceHull_Door( this, GetAbsOrigin(), GetAbsOrigin(), moveMins, moveMaxs, MASK_SOLID, GetActivator(), COLLISION_GROUP_NONE, &tr );
	if ( tr.allsolid || tr.startsolid )
	{
		if ( g_debug_doors.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), moveMins, moveMaxs, 255, 0, 0, true, 10.0f );

			if ( tr.m_pEnt )
			{
				NDebugOverlay::Box( tr.m_pEnt->GetAbsOrigin(), tr.m_pEnt->CollisionProp()->OBBMins(), tr.m_pEnt->CollisionProp()->OBBMaxs(), 220, 220, 0, true, 10.0f );
			}
		}

		return false;
	}

	if ( g_debug_doors.GetBool() )
	{
		NDebugOverlay::Box( GetAbsOrigin(), moveMins, moveMaxs, 0, 255, 0, true, 10.0f );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Puts the door in its appropriate position for spawning.
//-----------------------------------------------------------------------------
void CPropDoorRotating::DoorTeleportToSpawnPosition()
{
	QAngle angSpawn;

	// The Start Open spawnflag trumps the choices field
	if ( ( HasSpawnFlags( SF_DOOR_START_OPEN_OBSOLETE ) ) || ( m_eSpawnPosition == DOOR_SPAWN_OPEN_FORWARD ) )
	{
		angSpawn = m_angRotationOpenForward;
		SetDoorState( DOOR_STATE_OPEN );
	}
	else if ( m_eSpawnPosition == DOOR_SPAWN_OPEN_BACK )
	{
		angSpawn = m_angRotationOpenBack;
		SetDoorState( DOOR_STATE_OPEN );
	}
	else if ( m_eSpawnPosition == DOOR_SPAWN_CLOSED )
	{
		angSpawn = m_angRotationClosed;
		SetDoorState( DOOR_STATE_CLOSED );
	}
	else if ( m_eSpawnPosition == DOOR_SPAWN_AJAR )
	{
		angSpawn = m_angRotationAjar;
		SetDoorState( DOOR_STATE_AJAR );
	}
	else
	{
		// Bogus spawn position setting!
		Assert( false );
		angSpawn = m_angRotationClosed;
		SetDoorState( DOOR_STATE_CLOSED );
	}

	SetLocalAngles( angSpawn );

	// Doesn't relink; that's done in Spawn.
}


//-----------------------------------------------------------------------------
// Purpose: After rotating, set angle to exact final angle, call "move done" function.
//-----------------------------------------------------------------------------
void CPropDoorRotating::MoveDone()
{
	SetLocalAngles(m_angGoal);
	SetLocalAngularVelocity(vec3_angle);
	SetMoveDoneTime(-1);
	BaseClass::MoveDone();
}


//-----------------------------------------------------------------------------
// Purpose: Calculate m_vecVelocity and m_flNextThink to reach vecDest from
//			GetLocalOrigin() traveling at flSpeed. Just like LinearMove, but rotational.
// Input  : vecDestAngle - 
//			flSpeed - 
//-----------------------------------------------------------------------------
void CPropDoorRotating::AngularMove(const QAngle &vecDestAngle, float flSpeed)
{
	ASSERTSZ(flSpeed != 0, "AngularMove:  no speed is defined!");
	
	m_angGoal = vecDestAngle;

	// Already there?
	if (vecDestAngle == GetLocalAngles())
	{
		MoveDone();
		return;
	}
	
	// Set destdelta to the vector needed to move.
	QAngle vecDestDelta = vecDestAngle - GetLocalAngles();
	
	// Divide by speed to get time to reach dest
	float flTravelTime = vecDestDelta.Length() / flSpeed;

	// Call MoveDone when destination angles are reached.
	SetMoveDoneTime(flTravelTime);

	// Scale the destdelta vector by the time spent traveling to get velocity.
	SetLocalAngularVelocity(vecDestDelta * (1.0 / flTravelTime));
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::BeginOpening(CBaseEntity *pOpenAwayFrom)
{
	// Determine the direction to open.
	QAngle angOpen = m_angRotationOpenForward;
	doorCheck_e eDirCheck = DOOR_CHECK_FORWARD;

	if ( m_eOpenDirection == DOOR_ROTATING_OPEN_FORWARD )
	{
		eDirCheck	= DOOR_CHECK_FORWARD;
		angOpen		= m_angRotationOpenForward;
	}
	else if ( m_eOpenDirection == DOOR_ROTATING_OPEN_BACKWARD )
	{
		eDirCheck	= DOOR_CHECK_BACKWARD;
		angOpen		= m_angRotationOpenBack;
	}
	else // Can open either direction, test to see which is appropriate
	{
		if (pOpenAwayFrom != NULL)
		{
			Vector vecForwardDoor;
			GetVectors(&vecForwardDoor, NULL, NULL);

			if (vecForwardDoor.Dot(pOpenAwayFrom->GetAbsOrigin()) > vecForwardDoor.Dot(GetAbsOrigin()))
			{
				angOpen = m_angRotationOpenBack;
				eDirCheck = DOOR_CHECK_BACKWARD;
			}
		}

		// If player is opening us and we're opening away from them, and we'll be
		// blocked if we open away from them, open toward them.
		if (IsPlayerOpening() && (pOpenAwayFrom && pOpenAwayFrom->IsPlayer()) && !CheckDoorClear(eDirCheck))
		{
			if (eDirCheck == DOOR_CHECK_FORWARD)
			{
				angOpen = m_angRotationOpenBack;
				eDirCheck = DOOR_CHECK_BACKWARD;
			}
			else
			{
				angOpen = m_angRotationOpenForward;
				eDirCheck = DOOR_CHECK_FORWARD;
			}
		}
	}

	// Create the door blocker
	Vector mins, maxs;
	if ( eDirCheck == DOOR_CHECK_FORWARD )
	{
		mins = m_vecForwardBoundsMin;
		maxs = m_vecForwardBoundsMax;
	}
	else
	{
		mins = m_vecBackBoundsMin;
		maxs = m_vecBackBoundsMax;		
	}

	if ( m_hDoorBlocker != NULL )
	{
		UTIL_Remove( m_hDoorBlocker );
	}

	// Create a blocking entity to keep random entities out of our movement path
	m_hDoorBlocker = CEntityBlocker::Create( GetAbsOrigin(), mins, maxs, pOpenAwayFrom, false );
	
	Vector	volumeCenter = ((mins+maxs) * 0.5f) + GetAbsOrigin();

	// Ignoring the Z
	float volumeRadius = MAX( fabs(mins.x), maxs.x );
	volumeRadius = MAX( volumeRadius, MAX( fabs(mins.y), maxs.y ) );

	// Debug
	if ( g_debug_doors.GetBool() )
	{
		NDebugOverlay::Cross3D( volumeCenter, -Vector(volumeRadius,volumeRadius,volumeRadius), Vector(volumeRadius,volumeRadius,volumeRadius), 255, 0, 0, true, 1.0f );
	}

	// Make respectful entities move away from our path
	if( !HasSpawnFlags(SF_DOOR_SILENT_TO_NPCS) )
	{
		CSoundEnt::InsertSound( SOUND_MOVE_AWAY, volumeCenter, volumeRadius, 0.5f, pOpenAwayFrom );
	}

	// Do final setup
	if ( m_hDoorBlocker != NULL )
	{
		// Only block NPCs
		m_hDoorBlocker->SetCollisionGroup( COLLISION_GROUP_DOOR_BLOCKER );

		// If we hit something while opening, just stay unsolid until we try again
		if ( CheckDoorClear( eDirCheck ) == false )
		{
			m_hDoorBlocker->AddSolidFlags( FSOLID_NOT_SOLID );
		}

		if ( g_debug_doors.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), m_hDoorBlocker->CollisionProp()->OBBMins(), m_hDoorBlocker->CollisionProp()->OBBMaxs(), 255, 0, 0, true, 1.0f );
		}
	}

	AngularMove(angOpen, m_flSpeed);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::BeginClosing( void )
{
	if ( m_hDoorBlocker != NULL )
	{
		// Become solid again unless we're already being blocked
		if ( CheckDoorClear( GetOpenState() )  )
		{
			m_hDoorBlocker->RemoveSolidFlags( FSOLID_NOT_SOLID );
		}
		
		if ( g_debug_doors.GetBool() )
		{
			NDebugOverlay::Box( GetAbsOrigin(), m_hDoorBlocker->CollisionProp()->OBBMins(), m_hDoorBlocker->CollisionProp()->OBBMaxs(), 255, 0, 0, true, 1.0f );
		}
	}

	AngularMove(m_angRotationClosed, m_flSpeed);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPropDoorRotating::DoorStop( void )
{
	SetLocalAngularVelocity( vec3_angle );
	SetMoveDoneTime( -1 );
}

//-----------------------------------------------------------------------------
// Purpose: Restart a door moving that was temporarily paused
//-----------------------------------------------------------------------------
void CPropDoorRotating::DoorResume( void )
{
	// Restart our angular movement
	AngularMove( m_angGoal, m_flSpeed );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : vecMoveDir - 
//			opendata - 
//-----------------------------------------------------------------------------
void CPropDoorRotating::GetNPCOpenData(CAI_BaseNPC *pNPC, opendata_t &opendata)
{
	// dvs: TODO: finalize open position, direction, activity
	Vector vecForward;
	Vector vecRight;
	AngleVectors(GetAbsAngles(), &vecForward, &vecRight, NULL);

	//
	// Figure out where the NPC should stand to open this door,
	// and what direction they should face.
	//
	opendata.vecStandPos = GetAbsOrigin() - (vecRight * 24);
	opendata.vecStandPos.z -= 54;

	Vector vecNPCOrigin = pNPC->GetAbsOrigin();

	if (pNPC->GetAbsOrigin().Dot(vecForward) > GetAbsOrigin().Dot(vecForward))
	{
		// In front of the door relative to the door's forward vector.
		opendata.vecStandPos += vecForward * 64;
		opendata.vecFaceDir = -vecForward;
	}
	else
	{
		// Behind the door relative to the door's forward vector.
		opendata.vecStandPos -= vecForward * 64;
		opendata.vecFaceDir = vecForward;
	}

	opendata.eActivity = ACT_OPEN_DOOR;
}


//-----------------------------------------------------------------------------
// Purpose: Returns how long it will take this door to open.
//-----------------------------------------------------------------------------
float CPropDoorRotating::GetOpenInterval()
{
	// set destdelta to the vector needed to move
	QAngle vecDestDelta = m_angRotationOpenForward - GetLocalAngles();
	
	// divide by speed to get time to reach dest
	return vecDestDelta.Length() / m_flSpeed;
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CPropDoorRotating::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr, sizeof(tempstr),"Avelocity: %.2f %.2f %.2f", GetLocalAngularVelocity().x,  GetLocalAngularVelocity().y,  GetLocalAngularVelocity().z);
		EntityText( text_offset, tempstr, 0);
		text_offset++;

		if ( IsDoorOpen() )
		{
			Q_strncpy(tempstr, "DOOR STATE: OPEN", sizeof(tempstr));
		}
		else if ( IsDoorClosed() )
		{
			Q_strncpy(tempstr, "DOOR STATE: CLOSED", sizeof(tempstr));
		}
		else if ( IsDoorOpening() )
		{
			Q_strncpy(tempstr, "DOOR STATE: OPENING", sizeof(tempstr));
		}
		else if ( IsDoorClosing() )
		{
			Q_strncpy(tempstr, "DOOR STATE: CLOSING", sizeof(tempstr));
		}
		else if ( IsDoorAjar() )
		{
			Q_strncpy(tempstr, "DOOR STATE: AJAR", sizeof(tempstr));
		}
		EntityText( text_offset, tempstr, 0);
		text_offset++;
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Change this door's distance (in degrees) between open and closed
//-----------------------------------------------------------------------------
void CPropDoorRotating::InputSetRotationDistance( inputdata_t &inputdata )
{
	m_flDistance = inputdata.value.Float();
	
	// Recalculate our open volume
	CalcOpenAngles();
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenForward, &m_vecForwardBoundsMin, &m_vecForwardBoundsMax );
	CalculateDoorVolume( GetLocalAngles(), m_angRotationOpenBack, &m_vecBackBoundsMin, &m_vecBackBoundsMax );
}

// Debug sphere
class CPhysSphere : public CPhysicsProp
{
	DECLARE_CLASS( CPhysSphere, CPhysicsProp );
public:
	virtual bool OverridePropdata() { return true; }
	bool CreateVPhysics()
	{
		SetSolid( SOLID_BBOX );
		SetCollisionBounds( -Vector(12,12,12), Vector(12,12,12) );
		objectparams_t params = g_PhysDefaultObjectParams;
		params.pGameData = static_cast<void *>(this);
		IPhysicsObject *pPhysicsObject = physenv->CreateSphereObject( 12, 0, GetAbsOrigin(), GetAbsAngles(), &params, false );
		if ( pPhysicsObject )
		{
			VPhysicsSetObject( pPhysicsObject );
			SetMoveType( MOVETYPE_VPHYSICS );
			pPhysicsObject->Wake();
		}
	
		return true;
	}
};

void CPropDoorRotating::InputSetSpeed(inputdata_t &inputdata)
{
	AssertMsg1(inputdata.value.Float() > 0.0f, "InputSetSpeed on %s called with negative parameter!", GetDebugName() );
	m_flSpeed = inputdata.value.Float();
	DoorResume();
}

LINK_ENTITY_TO_CLASS( prop_sphere, CPhysSphere );


// ------------------------------------------------------------------------------------------ //
// Special version of func_physbox.
// ------------------------------------------------------------------------------------------ //
class CPhysBoxMultiplayer : public CPhysBox, public IMultiplayerPhysics
{
public:
	DECLARE_CLASS( CPhysBoxMultiplayer, CPhysBox );

	virtual int GetMultiplayerPhysicsMode()
	{
		return m_iPhysicsMode;
	}

	virtual float GetMass()
	{
		return m_fMass;
	}

	virtual bool IsAsleep()
	{
		return VPhysicsGetObject()->IsAsleep();
	}

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float, m_fMass );


	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Activate()
	{
		BaseClass::Activate();
		SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );
		m_fMass = VPhysicsGetObject()->GetMass();
	}
};

LINK_ENTITY_TO_CLASS( func_physbox_multiplayer, CPhysBoxMultiplayer );

BEGIN_DATADESC( CPhysBoxMultiplayer )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPhysBoxMultiplayer, DT_PhysBoxMultiplayer )
	SendPropInt( SENDINFO( m_iPhysicsMode ), 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_fMass ), 0, SPROP_NOSCALE ),
END_SEND_TABLE()



class CPhysicsPropMultiplayer : public CPhysicsProp, public IMultiplayerPhysics
{
	DECLARE_CLASS( CPhysicsPropMultiplayer, CPhysicsProp );

	CNetworkVar( int, m_iPhysicsMode );	// One of the PHYSICS_MULTIPLAYER_ defines.	
	CNetworkVar( float, m_fMass );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CPhysicsPropMultiplayer()
	{
		m_iPhysicsMode = PHYSICS_MULTIPLAYER_AUTODETECT;
		m_usingCustomCollisionBounds = false;
	}

// IBreakableWithPropData:
	void SetPhysicsMode(int iMode)
	{
		m_iPhysicsMode = iMode;
	}

	int		GetPhysicsMode() { return m_iPhysicsMode; }

// IMultiplayerPhysics:
	int		GetMultiplayerPhysicsMode() { return m_iPhysicsMode; }
	float	GetMass() { return m_fMass; }
	bool	IsAsleep() { return !m_bAwake; }

	bool	IsDebris( void )			{ return ( ( m_spawnflags & SF_PHYSPROP_DEBRIS ) != 0 ); }

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics )
	{
		BaseClass::VPhysicsUpdate( pPhysics );

		if ( sv_turbophysics.GetBool() )
		{
			// If the object is set to debris, don't let turbo physics change it.
			if ( IsDebris() )
				return;

			if ( m_bAwake )
			{
				SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );
			}
			else if ( m_iPhysicsMode == PHYSICS_MULTIPLAYER_NON_SOLID )
			{
				SetCollisionGroup( COLLISION_GROUP_DEBRIS );
			}
			else
			{
				SetCollisionGroup( COLLISION_GROUP_NONE );
			}
		}
	}

	virtual void Spawn( void )
	{
		BaseClass::Spawn();

		// if no physicsmode was defined by .QC or propdata.txt, 
		// use auto detect based on size & mass
		if ( m_iPhysicsMode == PHYSICS_MULTIPLAYER_AUTODETECT )
		{
			if ( VPhysicsGetObject() )
			{
				m_iPhysicsMode = GetAutoMultiplayerPhysicsMode( 
					CollisionProp()->OBBSize(), VPhysicsGetObject()->GetMass() );
			}
			else
			{
				UTIL_Remove( this );
				return;
			}
		}

		// check if map maker overrides physics mode to force a server-side entity
		if ( GetSpawnFlags() & SF_PHYSPROP_FORCE_SERVER_SIDE )
		{
			SetPhysicsMode( PHYSICS_MULTIPLAYER_NON_SOLID );
		}

		if ( m_iPhysicsMode == PHYSICS_MULTIPLAYER_CLIENTSIDE )
		{
			if ( engine->IsInEditMode() )
			{
				// in map edit mode always spawn as server phys prop
				SetPhysicsMode( PHYSICS_MULTIPLAYER_NON_SOLID );
			}
			else
			{
				// don't spawn clientside props on server
				UTIL_Remove( this );
				return;
			}
			
		}

		if ( GetCollisionGroup() == COLLISION_GROUP_NONE )
			SetCollisionGroup( COLLISION_GROUP_PUSHAWAY );

		// Items marked as debris should be set as such.
		if ( IsDebris() )
		{
			SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		}

		m_fMass = VPhysicsGetObject()->GetMass();

		// VPhysicsGetObject() is NULL on the client, which prevents the client from finding a decent
		// AABB surrounding the collision bounds.  If we've got a VPhysicsGetObject()->GetCollide(), we'll
		// grab it's unrotated bounds and use it to calculate our collision surrounding bounds.  This
		// can end up larger than the CollisionProp() would have calculated on its own, but it'll be
		// identical on the client and the server.
		m_usingCustomCollisionBounds = false;
		if ( ( GetSolid() == SOLID_VPHYSICS ) && ( GetMoveType() == MOVETYPE_VPHYSICS ) )
		{
			IPhysicsObject *pPhysics = VPhysicsGetObject();
			if ( pPhysics && pPhysics->GetCollide() )
			{
				physcollision->CollideGetAABB( &m_collisionMins.GetForModify(), &m_collisionMaxs.GetForModify(), pPhysics->GetCollide(), vec3_origin, vec3_angle );
				CollisionProp()->SetSurroundingBoundsType( USE_GAME_CODE );
				m_usingCustomCollisionBounds = true;
			}
		}
	}

	virtual void ComputeWorldSpaceSurroundingBox( Vector *mins, Vector *maxs )
	{
		Assert( m_usingCustomCollisionBounds );
		Assert( mins != NULL && maxs != NULL );
		if ( !mins || !maxs )
			return;

		// Take our saved collision bounds, and transform into world space
		TransformAABB( EntityToWorldTransform(), m_collisionMins, m_collisionMaxs, *mins, *maxs );
	}

private:
	bool m_usingCustomCollisionBounds;
	CNetworkVector( m_collisionMins );
	CNetworkVector( m_collisionMaxs );
};

LINK_ENTITY_TO_CLASS( prop_physics_multiplayer, CPhysicsPropMultiplayer );

BEGIN_DATADESC( CPhysicsPropMultiplayer )
	DEFINE_KEYFIELD( m_iPhysicsMode, FIELD_INTEGER, "physicsmode" ),
	DEFINE_FIELD( m_fMass, FIELD_FLOAT ),
	DEFINE_FIELD( m_usingCustomCollisionBounds, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_collisionMins, FIELD_VECTOR ),
	DEFINE_FIELD( m_collisionMaxs, FIELD_VECTOR ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPhysicsPropMultiplayer, DT_PhysicsPropMultiplayer )
	SendPropInt( SENDINFO( m_iPhysicsMode ), 2, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_fMass ), 0, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_collisionMins ), 0, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_collisionMaxs ), 0, SPROP_NOSCALE ),
END_SEND_TABLE()

#define RESPAWNABLE_PROP_DEFAULT_TIME 60.0f

class CPhysicsPropRespawnable : public CPhysicsProp
{
	DECLARE_CLASS( CPhysicsPropRespawnable, CPhysicsProp );
	DECLARE_DATADESC();

public:

	CPhysicsPropRespawnable();

	virtual void Spawn( void );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	void	Materialize( void );

private:

	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;

	Vector m_vOriginalMins;
	Vector m_vOriginalMaxs;

	float m_flRespawnTime;
};

LINK_ENTITY_TO_CLASS( prop_physics_respawnable, CPhysicsPropRespawnable );

BEGIN_DATADESC( CPhysicsPropRespawnable )
	DEFINE_THINKFUNC( Materialize ),
	DEFINE_KEYFIELD( m_flRespawnTime, FIELD_FLOAT, "RespawnTime" ),
	DEFINE_FIELD( m_vOriginalSpawnOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vOriginalSpawnAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vOriginalMins, FIELD_VECTOR ),
	DEFINE_FIELD( m_vOriginalMaxs, FIELD_VECTOR ),
END_DATADESC()

CPhysicsPropRespawnable::CPhysicsPropRespawnable( void )
{
	m_flRespawnTime = 0.0f;
}

void CPhysicsPropRespawnable::Spawn( void )
{
	BaseClass::Spawn();

	m_vOriginalSpawnOrigin = GetAbsOrigin();
	m_vOriginalSpawnAngles = GetAbsAngles();

	m_vOriginalMins = CollisionProp()->OBBMins();
	m_vOriginalMaxs = CollisionProp()->OBBMaxs();

	if ( m_flRespawnTime == 0.0f )
	{
		m_flRespawnTime = RESPAWNABLE_PROP_DEFAULT_TIME;
	}

	SetOwnerEntity( NULL );
}

void CPhysicsPropRespawnable::Event_Killed( const CTakeDamageInfo &info )
{
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if ( pPhysics && !pPhysics->IsMoveable() )
	{
		pPhysics->EnableMotion( true );
		VPhysicsTakeDamage( info );
	}

	Break( info.GetInflictor(), info );

	PhysCleanupFrictionSounds( this );

	VPhysicsDestroyObject();

	CBaseEntity::PhysicsRemoveTouchedList( this );
	CBaseEntity::PhysicsRemoveGroundList( this );
	DestroyAllDataObjects();

	AddEffects( EF_NODRAW );

	if ( IsOnFire() || IsDissolving() )
	{
		UTIL_Remove( GetEffectEntity() );
	}

	Teleport( &m_vOriginalSpawnOrigin, &m_vOriginalSpawnAngles, NULL );

	SetContextThink( NULL, 0, "PROP_CLEARFLAGS" );

	SetThink( &CPhysicsPropRespawnable::Materialize );
	SetNextThink( gpGlobals->curtime + m_flRespawnTime );
}

void CPhysicsPropRespawnable::Materialize( void )
{
	trace_t tr;
	UTIL_TraceHull( m_vOriginalSpawnOrigin, m_vOriginalSpawnOrigin, m_vOriginalMins, m_vOriginalMaxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if ( tr.startsolid || tr.allsolid )
	{
		//Try again in a second.
		SetNextThink( gpGlobals->curtime + 1.0f );
		return;
	}

	RemoveEffects( EF_NODRAW );
	Spawn();
}


//------------------------------------------------------------------------------
// Purpose: Create a prop of the given type
//------------------------------------------------------------------------------
void CC_Prop_Dynamic_Create( const CCommand &args )
{
	if ( args.ArgC() != 2 )
		return;

	// Figure out where to place it
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	Vector forward;
	pPlayer->EyeVectors( &forward );

	trace_t tr;
	UTIL_TraceLine( pPlayer->EyePosition(),
		pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_NPCSOLID, 
		pPlayer, COLLISION_GROUP_NONE, &tr );

	// No hit? We're done.
	if ( tr.fraction == 1.0 )
		return;

	MDLCACHE_CRITICAL_SECTION();

	char pModelName[512];
	Q_snprintf( pModelName, sizeof(pModelName), "models/%s", args[1] );
	Q_DefaultExtension( pModelName, ".mdl", sizeof(pModelName) );
	MDLHandle_t h = mdlcache->FindMDL( pModelName );
	if ( h == MDLHANDLE_INVALID )
		return;

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	vcollide_t *pVCollide = mdlcache->GetVCollide( h );

	Vector xaxis( 1.0f, 0.0f, 0.0f );
	Vector yaxis;
	CrossProduct( tr.plane.normal, xaxis, yaxis );
	if ( VectorNormalize( yaxis ) < 1e-3 )
	{
		xaxis.Init( 0.0f, 0.0f, 1.0f );
		CrossProduct( tr.plane.normal, xaxis, yaxis );
		VectorNormalize( yaxis );
	}
	CrossProduct( yaxis, tr.plane.normal, xaxis );
	VectorNormalize( xaxis );

	VMatrix entToWorld;
	entToWorld.SetBasisVectors( xaxis, yaxis, tr.plane.normal );

	QAngle angles;
	MatrixToAngles( entToWorld, angles );

	// Try to create entity
	CDynamicProp *pProp = dynamic_cast< CDynamicProp * >( CreateEntityByName( "dynamic_prop" ) );
	if ( pProp )
	{
		char buf[512];
		// Pass in standard key values
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", tr.endpos.x, tr.endpos.y, tr.endpos.z );
		pProp->KeyValue( "origin", buf );
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z );
		pProp->KeyValue( "angles", buf );
		pProp->KeyValue( "model", pModelName );
		pProp->KeyValue( "solid", pVCollide ? "6" : "2" );
		pProp->KeyValue( "fademindist", "-1" );
		pProp->KeyValue( "fademaxdist", "0" );
		pProp->KeyValue( "fadescale", "1" );
		pProp->KeyValue( "MinAnimTime", "5" );
		pProp->KeyValue( "MaxAnimTime", "10" );
		pProp->Precache();
		DispatchSpawn( pProp );
		pProp->Activate();
	}
	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}

static ConCommand prop_dynamic_create("prop_dynamic_create", CC_Prop_Dynamic_Create, "Creates a dynamic prop with a specific .mdl aimed away from where the player is looking.\n\tArguments: {.mdl name}", FCVAR_CHEAT);



//------------------------------------------------------------------------------
// Purpose: Create a prop of the given type
//------------------------------------------------------------------------------
void CC_Prop_Physics_Create( const CCommand &args )
{
	if ( args.ArgC() != 2 )
		return;

	char pModelName[512];
	Q_snprintf( pModelName, sizeof(pModelName), "models/%s", args[1] );
	Q_DefaultExtension( pModelName, ".mdl", sizeof(pModelName) );

	// Figure out where to place it
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	Vector forward;
	pPlayer->EyeVectors( &forward );

	CreatePhysicsProp( pModelName, pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, pPlayer, true );
}

static ConCommand prop_physics_create("prop_physics_create", CC_Prop_Physics_Create, "Creates a physics prop with a specific .mdl aimed away from where the player is looking.\n\tArguments: {.mdl name}", FCVAR_CHEAT);


CPhysicsProp* CreatePhysicsProp( const char *pModelName, const Vector &vTraceStart, const Vector &vTraceEnd, const IHandleEntity *pTraceIgnore, bool bRequireVCollide, const char *pClassName )
{
	MDLCACHE_CRITICAL_SECTION();

	MDLHandle_t h = mdlcache->FindMDL( pModelName );
	if ( h == MDLHANDLE_INVALID )
		return NULL;

	// Must have vphysics to place as a physics prop
	studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( h );
	if ( !pStudioHdr )
		return NULL;

	// Must have vphysics to place as a physics prop
	if ( bRequireVCollide && !mdlcache->GetVCollide( h ) )
		return NULL;

	QAngle angles( 0.0f, 0.0f, 0.0f );
	Vector vecSweepMins = pStudioHdr->hull_min;
	Vector vecSweepMaxs = pStudioHdr->hull_max;

	trace_t tr;
	UTIL_TraceHull( vTraceStart, vTraceEnd,
		vecSweepMins, vecSweepMaxs, MASK_NPCSOLID, pTraceIgnore, COLLISION_GROUP_NONE, &tr );
		    
	// No hit? We're done.
	if ( (tr.fraction == 1.0 && (vTraceEnd-vTraceStart).Length() > 0.01) || tr.allsolid )
		return NULL;
		    
	VectorMA( tr.endpos, 1.0f, tr.plane.normal, tr.endpos );

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );
				  
	// Try to create entity
	CPhysicsProp *pProp = dynamic_cast< CPhysicsProp * >( CreateEntityByName( pClassName ) );
	if ( pProp )
	{
		char buf[512];
		// Pass in standard key values
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", tr.endpos.x, tr.endpos.y, tr.endpos.z );
		pProp->KeyValue( "origin", buf );
		Q_snprintf( buf, sizeof(buf), "%.10f %.10f %.10f", angles.x, angles.y, angles.z );
		pProp->KeyValue( "angles", buf );
		pProp->KeyValue( "model", pModelName );
		pProp->KeyValue( "fademindist", "-1" );
		pProp->KeyValue( "fademaxdist", "0" );
		pProp->KeyValue( "fadescale", "1" );
		pProp->KeyValue( "inertiaScale", "1.0" );
		pProp->KeyValue( "physdamagescale", "0.1" );
		pProp->Precache();
		DispatchSpawn( pProp );
		pProp->Activate();
	}
	CBaseEntity::SetAllowPrecache( bAllowPrecache );

	return pProp;
}

//-----------------------------------------------------------------------------
// Purpose: Scale the object to a new size, taking its render verts and physical verts into account
//-----------------------------------------------------------------------------
bool UTIL_CreateScaledPhysObject( CBaseAnimating *pInstance, float flScale )
{
	// Don't scale NPCs
	if ( pInstance->MyCombatCharacterPointer() )
		return false;

	// FIXME: This needs to work for ragdolls!

	// Get our object
	IPhysicsObject *pObject = pInstance->VPhysicsGetObject();
	if ( pObject == NULL )
	{
		AssertMsg( 0, "UTIL_CreateScaledPhysObject: Failed to scale physics for object-- It has no physics." );
		return false;
	}

	// See if our current physics object is motion disabled
	bool bWasMotionDisabled = ( pObject->IsMotionEnabled() == false );
	bool bWasStatic			= ( pObject->IsStatic() );

	vcollide_t *pCollide = modelinfo->GetVCollide( pInstance->GetModelIndex() );
	if ( pCollide == NULL || pCollide->solidCount == 0 )
		return NULL;

	CPhysCollide *pNewCollide = pCollide->solids[0];	// FIXME: Needs to iterate over the solids

	if ( flScale != 1.0f )
	{
		// Create a query to get more information from the collision object
		ICollisionQuery *pQuery = physcollision->CreateQueryModel( pCollide->solids[0] );	// FIXME: This should iterate over all solids!
		if ( pQuery == NULL )
			return false;

		// Create a container to hold all the convexes we'll create
		const int nNumConvex = pQuery->ConvexCount();
		CPhysConvex **pConvexes = (CPhysConvex **) stackalloc( sizeof(CPhysConvex *) * nNumConvex );

		// For each convex, collect the verts and create a convex from it we'll retain for later
		for ( int i = 0; i < nNumConvex; i++ )
		{
			int nNumTris = pQuery->TriangleCount( i );
			int nNumVerts = nNumTris * 3;
			// FIXME: Really?  stackalloc?
			Vector *pVerts = (Vector *) stackalloc( sizeof(Vector) * nNumVerts );
			Vector **ppVerts = (Vector **) stackalloc( sizeof(Vector *) * nNumVerts );
			for ( int j = 0; j < nNumTris; j++ )
			{
				// Get all the verts for this triangle and scale them up
				pQuery->GetTriangleVerts( i, j, pVerts+(j*3) );
				*(pVerts+(j*3)) *= flScale;
				*(pVerts+(j*3)+1) *= flScale;
				*(pVerts+(j*3)+2) *= flScale;

				// Setup our pointers (blech!)
				*(ppVerts+(j*3)) = pVerts+(j*3);
				*(ppVerts+(j*3)+1) = pVerts+(j*3)+1;
				*(ppVerts+(j*3)+2) = pVerts+(j*3)+2;
			}

			// Convert it back to a convex
			pConvexes[i] = physcollision->ConvexFromVerts( ppVerts, nNumVerts );
			Assert( pConvexes[i] != NULL );
			if ( pConvexes[i] == NULL )
				return false;
		}

		// Clean up
		physcollision->DestroyQueryModel( pQuery );

		// Create a collision model from all the convexes
		pNewCollide = physcollision->ConvertConvexToCollide( pConvexes, nNumConvex );
		if ( pNewCollide == NULL )
			return false;
	}

	// Get our solid info
	solid_t tmpSolid;
	if ( !PhysModelParseSolidByIndex( tmpSolid, pInstance, pInstance->GetModelIndex(), -1 ) )
		return false;

	// Physprops get keyvalues that effect the mass, this block is to respect those fields when we scale
	CPhysicsProp *pPhysInstance = dynamic_cast<CPhysicsProp*>( pInstance );
	if ( pPhysInstance )
	{
		if ( pPhysInstance->GetMassScale() > 0 )
		{
			tmpSolid.params.mass *= pPhysInstance->GetMassScale();
		}

		PhysSolidOverride( tmpSolid, pPhysInstance->GetPhysOverrideScript() );
	}			

	// Scale our mass up as well
	tmpSolid.params.mass *= flScale;
	tmpSolid.params.volume = physcollision->CollideVolume( pNewCollide );

	// Get our surface prop info
	int surfaceProp = -1;
	if ( tmpSolid.surfaceprop[0] )
	{
		surfaceProp = physprops->GetSurfaceIndex( tmpSolid.surfaceprop );
	}

	// Now put it all back (phew!)
	IPhysicsObject *pNewObject = NULL;
	if ( bWasStatic )
	{
		pNewObject = physenv->CreatePolyObjectStatic( pNewCollide, surfaceProp, pInstance->GetAbsOrigin(), pInstance->GetAbsAngles(), &tmpSolid.params );
	}
	else
	{
		pNewObject = physenv->CreatePolyObject( pNewCollide, surfaceProp, pInstance->GetAbsOrigin(), pInstance->GetAbsAngles(), &tmpSolid.params );
	}
	Assert( pNewObject );

	pInstance->VPhysicsDestroyObject();
	pInstance->VPhysicsSetObject( pNewObject );

	// Increase our model bounds
	const model_t *pModel = modelinfo->GetModel( pInstance->GetModelIndex() );
	if ( pModel )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( pModel, mins, maxs );
		pInstance->SetCollisionBounds( mins*flScale, maxs*flScale );
	}

	// Scale the base model as well
	pInstance->SetModelScale( flScale );

	if ( pInstance->GetParent() )
	{
		pNewObject->SetShadow( 1e4, 1e4, false, false );
		pNewObject->UpdateShadow( pInstance->GetAbsOrigin(), pInstance->GetAbsAngles(), false, 0 );
	}

	if ( bWasMotionDisabled )
	{
		pNewObject->EnableMotion( false );
	}
	else
	{
		// Make sure we start awake!
		pNewObject->Wake();
	}

	// Blargh
	pInstance->SetScaledPhysics( ( flScale != 1.0f ) ? pNewObject : NULL );

	return true;
}


//------------------------------------------------------------------------------
// Rotates an entity
//------------------------------------------------------------------------------
void CC_Ent_Rotate( const CCommand &args )
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	CBaseEntity* pEntity = FindPickerEntity( pPlayer );
	if ( !pEntity )
		return;

	QAngle angles = pEntity->GetLocalAngles();
	float flAngle = (args.ArgC() == 2) ? atof( args[1] ) : 7.5f;
	   
	VMatrix entToWorld, rot, newEntToWorld;
	MatrixBuildRotateZ( rot, flAngle );
	MatrixFromAngles( angles, entToWorld );
	MatrixMultiply( entToWorld, rot, newEntToWorld );
	MatrixToAngles( newEntToWorld, angles );
	pEntity->SetLocalAngles( angles );
}

static ConCommand ent_rotate("ent_rotate", CC_Ent_Rotate, "Rotates an entity by a specified # of degrees", FCVAR_CHEAT);

// This is a dummy. The entity is entirely clientside.
LINK_ENTITY_TO_CLASS( func_proprrespawnzone, CBaseEntity );
