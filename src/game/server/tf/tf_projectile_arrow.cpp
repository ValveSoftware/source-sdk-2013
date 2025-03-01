//========= Copyright Valve Corporation, All rights reserved. ============//
//
// TF Arrow
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_arrow.h"
#include "soundent.h"
#include "tf_fx.h"
#include "props.h"
#include "baseobject_shared.h"
#include "SpriteTrail.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "collisionutils.h"
#include "bone_setup.h"
#include "decals.h"
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_pumpkin_bomb.h"
#include "tf_weapon_shovel.h"
#include "player_vs_environment/tf_tank_boss.h"
#include "halloween/halloween_base_boss.h"
#include "halloween/merasmus/merasmus_trick_or_treat_prop.h"
#include "tf_logic_robot_destruction.h"

#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "tf_weapon_medigun.h"
#include "soundenvelope.h"
#include "tf_obj_sentrygun.h"


//=============================================================================
//
// TF Arrow Projectile functions (Server specific).
//
#define ARROW_MODEL_GIB1			"models/weapons/w_models/w_arrow_gib1.mdl"
#define ARROW_MODEL_GIB2			"models/weapons/w_models/w_arrow_gib2.mdl"

#define ARROW_GRAVITY				0.3f

#define ARROW_THINK_CONTEXT			"CTFProjectile_ArrowThink"

#define CLAW_TRAIL_RED				"effects/repair_claw_trail_red.vmt"
#define CLAW_TRAIL_BLU				"effects/repair_claw_trail_blue.vmt"
#define CLAW_GIB1					"models/weapons/w_models/w_repair_claw_gib1.mdl"
#define CLAW_GIB2					"models/weapons/w_models/w_repair_claw_gib2.mdl"

#define CLAW_REPAIR_EFFECT_BLU		"repair_claw_heal_blue"
#define CLAW_REPAIR_EFFECT_RED		"repair_claw_heal_red"
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_arrow, CTFProjectile_Arrow );
PRECACHE_WEAPON_REGISTER( tf_projectile_arrow );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TFProjectile_Arrow )

BEGIN_NETWORK_TABLE( CTFProjectile_Arrow, DT_TFProjectile_Arrow )
	SendPropBool( SENDINFO( m_bArrowAlight ) ),
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropInt( SENDINFO( m_iProjectileType ) ),
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_Arrow )
DEFINE_THINKFUNC( ImpactThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_healing_bolt, CTFProjectile_HealingBolt );
PRECACHE_WEAPON_REGISTER( tf_projectile_healing_bolt );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_HealingBolt, DT_TFProjectile_HealingBolt )

BEGIN_NETWORK_TABLE( CTFProjectile_HealingBolt, DT_TFProjectile_HealingBolt )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_HealingBolt )
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_grapplinghook, CTFProjectile_GrapplingHook );
PRECACHE_WEAPON_REGISTER( tf_projectile_grapplinghook );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_GrapplingHook, DT_TFProjectile_GrapplingHook )

BEGIN_NETWORK_TABLE( CTFProjectile_GrapplingHook, DT_TFProjectile_GrapplingHook )
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_GrapplingHook )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Arrow::CTFProjectile_Arrow()
{
	m_flImpactTime = 0.0f;
	m_flTrailLife = 0.f;
	m_pTrail = NULL;
	m_bStruckEnemy = false;
	m_bArrowAlight = false;
	m_iDeflected = 0;
	m_bCritical = false;
	m_flInitTime = 0;
	m_bPenetrate = false;
	m_iProjectileType = TF_PROJECTILE_ARROW;
	m_iWeaponId = TF_WEAPON_COMPOUND_BOW;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Arrow::~CTFProjectile_Arrow()
{
	m_HitEntities.Purge();
}

static const char* GetArrowEntityName( ProjectileType_t projectileType )
{
	switch ( projectileType )
	{
	case TF_PROJECTILE_HEALING_BOLT:
	case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
		return "tf_projectile_healing_bolt";
	case TF_PROJECTILE_GRAPPLINGHOOK:
		return "tf_projectile_grapplinghook";
	
	default:
		return "tf_projectile_arrow";
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Arrow *CTFProjectile_Arrow::Create( const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	const char* pszArrowEntityName = GetArrowEntityName( projectileType );
	CTFProjectile_Arrow *pArrow = static_cast<CTFProjectile_Arrow*>( CBaseEntity::Create( pszArrowEntityName, vecOrigin, vecAngles, pOwner ) );
	if ( pArrow )
	{
		pArrow->InitArrow( vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer );
	}

	return pArrow;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::InitArrow( const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	// Initialize the owner.
	SetOwnerEntity( pOwner );

	// Set team.
	ChangeTeam( pOwner->GetTeamNumber() );

	// must override projectile type before Spawn for proper model
	m_iProjectileType = projectileType;

	// Spawn.
	Spawn();

	SetGravity( fGravity );

	SetCritical( true );

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = vecForward * fSpeed;
	
	SetAbsVelocity( vecVelocity );	
	SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	SetAbsAngles( angles );

	// Save the scoring player.
	SetScorer( pScorer );

	// Create a trail.
	CreateTrail();

	// Add ourselves to the hit entities list so we dont shoot ourselves
	m_HitEntities.AddToTail( pOwner->entindex() );

	m_flInitTime = gpGlobals->curtime;

	{
		m_bFiredWhileZoomed = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Spawn()
{
	if ( m_iProjectileType == TF_PROJECTILE_BUILDING_REPAIR_BOLT )
	{
		SetModel( g_pszArrowModels[MODEL_ARROW_BUILDING_REPAIR] );
		m_iWeaponId = TF_WEAPON_SHOTGUN_BUILDING_RESCUE;
	}
	else if ( m_iProjectileType == TF_PROJECTILE_FESTIVE_ARROW )
	{
		SetModel( g_pszArrowModels[MODEL_FESTIVE_ARROW_REGULAR] );
	}
	else if ( m_iProjectileType == TF_PROJECTILE_HEALING_BOLT 
	) {
		SetModel( g_pszArrowModels[MODEL_SYRINGE] );
		SetModelScale( 3.0f );
	}
	else if ( m_iProjectileType == TF_PROJECTILE_FESTIVE_HEALING_BOLT )
	{
		SetModel( g_pszArrowModels[MODEL_FESTIVE_HEALING_BOLT] );
		SetModelScale( 3.0f );
	}
	else if ( m_iProjectileType == TF_PROJECTILE_GRAPPLINGHOOK )
	{
		SetModel( g_pszArrowModels[MODEL_GRAPPLINGHOOK] );
	}
	else
	{
		SetModel( g_pszArrowModels[MODEL_ARROW_REGULAR] );
	}

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, Vector( -1.0f, -1.0f, -1.0f ), Vector( 1.0f, 1.0f, 1.0f ) );
	SetSolid( SOLID_BBOX );	

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );
	AddEffects( EF_NOSHADOW );
	AddFlag( FL_GRENADE );

	SetTouch( &CTFProjectile_Arrow::ArrowTouch );

	// Set team.
	m_nSkin = GetArrowSkin();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Precache()
{
	int arrow_model = PrecacheModel( g_pszArrowModels[MODEL_ARROW_REGULAR] );
	int claw_model = PrecacheModel( g_pszArrowModels[MODEL_ARROW_BUILDING_REPAIR] );
	int festive_arrow_model = PrecacheModel( g_pszArrowModels[MODEL_FESTIVE_ARROW_REGULAR] );
	PrecacheModel( g_pszArrowModels[MODEL_FESTIVE_HEALING_BOLT] );

	PrecacheGibsForModel( arrow_model );
	PrecacheGibsForModel( claw_model );
	PrecacheGibsForModel( festive_arrow_model );
	//PrecacheGibsForModel( festive_healing_arrow_model );
	PrecacheModel( "effects/arrowtrail_red.vmt" );
	PrecacheModel( "effects/arrowtrail_blu.vmt" );
	PrecacheModel( "effects/healingtrail_red.vmt" );
	PrecacheModel( "effects/healingtrail_blu.vmt" );
	PrecacheModel( CLAW_TRAIL_RED );
	PrecacheModel( CLAW_TRAIL_BLU );
	PrecacheParticleSystem( CLAW_REPAIR_EFFECT_BLU );
	PrecacheParticleSystem( CLAW_REPAIR_EFFECT_RED );
	PrecacheScriptSound( "Weapon_Arrow.ImpactFlesh" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactMetal" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactWood" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactConcrete" );
	PrecacheScriptSound( "Weapon_Arrow.Nearmiss" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactFleshCrossbowHeal" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBasePlayer *CTFProjectile_Arrow::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CanHeadshot() 
{ 
	CBaseEntity *pOwner = GetScorer();
	if ( pOwner == NULL )
		return false;

	if ( m_iProjectileType == TF_PROJECTILE_BUILDING_REPAIR_BOLT 
		|| m_iProjectileType == TF_PROJECTILE_HEALING_BOLT 
		|| m_iProjectileType == TF_PROJECTILE_FESTIVE_HEALING_BOLT 
	) {
		return false;
	}


	return true; 
}

//-----------------------------------------------------------------------------
// Purpose: Healing bolt damage.
//-----------------------------------------------------------------------------
float CTFProjectile_Arrow::GetDamage()
{
	if ( m_iProjectileType == TF_PROJECTILE_HEALING_BOLT
		|| m_iProjectileType == TF_PROJECTILE_FESTIVE_HEALING_BOLT
	) {
		float lifeTimeScale = RemapValClamped( gpGlobals->curtime - m_flInitTime, 0.0f, 0.6f, 0.5f, 1.0f );	
		return m_flDamage * lifeTimeScale;
	}
	return BaseClass::GetDamage();
}


//-----------------------------------------------------------------------------
// Purpose: Moves the arrow to a particular bbox.
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim )
{
	CStudioHdr *pStudioHdr = pOtherAnim->GetModelPtr();
	if ( !pStudioHdr )
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pOtherAnim->GetHitboxSet() );
	if ( !set )
		return false;
	if ( !set->numhitboxes )			// Target must have hit boxes.
		return false;

	if ( pBox->bone < 0 || pBox->bone >= pStudioHdr->numbones() )	// Bone index must be valid.
		return false;

	CBoneCache *pCache = pOtherAnim->GetBoneCache();
	if ( !pCache )
		return false;

	matrix3x4_t *bone_matrix = pCache->GetCachedBone( pBox->bone );
	if ( !bone_matrix )
		return false;

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	TransformAABB( *bone_matrix, pBox->bbmin, pBox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );

	// Adjust the arrow so it isn't exactly in the center of the box.
	Vector position;
	Vector vecDelta = vecBoxAbsMaxs - vecBoxAbsMins;
	float frand = (float) rand() / VALVE_RAND_MAX;
	position.x = vecBoxAbsMins.x + vecDelta.x*0.6f - vecDelta.x*frand*0.2f;
	frand = (float) rand() / VALVE_RAND_MAX;
	position.y = vecBoxAbsMins.y + vecDelta.y*0.6f - vecDelta.y*frand*0.2f;
	frand = (float) rand() / VALVE_RAND_MAX;
	position.z = vecBoxAbsMins.z + vecDelta.z*0.6f - vecDelta.z*frand*0.2f;
	SetAbsOrigin( position );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This was written after PositionArrowOnBone, but the two might be mergable?
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pOtherAnim, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex )
{
	// Find a bone to stick to.
	matrix3x4_t arrowWorldSpace;
	MatrixCopy( EntityToWorldTransform(), arrowWorldSpace );

	// Get the bone info so we can follow the bone.
	boneIndexAttached = pBox->bone;
	physicsBoneIndex = pOtherAnim->GetPhysicsBone( boneIndexAttached );
	matrix3x4_t boneToWorld;
	pOtherAnim->GetBoneTransform( boneIndexAttached, boneToWorld );

	Vector attachedBonePos;
	QAngle attachedBoneAngles;
	pOtherAnim->GetBonePosition( boneIndexAttached, attachedBonePos, attachedBoneAngles );

	// Transform my current position/orientation into the hit bone's space.
	matrix3x4_t worldToBone, localMatrix;
	MatrixInvert( boneToWorld, worldToBone );
	ConcatTransforms( worldToBone, arrowWorldSpace, localMatrix );
	MatrixAngles( localMatrix, boneAngles, bonePosition );
}

//-----------------------------------------------------------------------------
int	CTFProjectile_Arrow::GetProjectileType ( void ) const	
{ 
	return m_iProjectileType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::StrikeTarget( mstudiobbox_t *pBox, CBaseEntity *pOther )
{
	if ( !pOther )
		return false;

	// Different path for arrows that heal friendly buildings.
	if ( pOther->IsBaseObject() )
	{
		if ( OnArrowImpactObject( pOther ) )
		{
			return false;
		}
	}

	// Block and break on invulnerable players, ignoring teammates under normal rules
	CTFPlayer *pTFPlayerOther = ToTFPlayer( pOther );
	if ( pTFPlayerOther && ( pTFPlayerOther != GetOwnerEntity() ) && pTFPlayerOther->m_Shared.IsInvulnerable() && ( !InSameTeam( pTFPlayerOther ) || CanCollideWithTeammates() ) )
		return false;

	CBaseAnimating *pOtherAnim = dynamic_cast< CBaseAnimating* >(pOther);
	if ( !pOtherAnim )
		return false;

	bool bBreakArrow = IsBreakable() && ( ( dynamic_cast< CTFTankBoss* >( pOther ) != NULL ) || ( dynamic_cast< CHalloweenBaseBoss* >( pOther ) != NULL ) );

	// Position the arrow so its on the bone, within a reasonable region defined by the bbox.
	if ( !m_bPenetrate && !bBreakArrow )
	{
		if ( !PositionArrowOnBone( pBox, pOtherAnim ) )
		{
			return false;
		}
	}

	//
	const Vector &vecOrigin = GetAbsOrigin();
	Vector vecVelocity = GetAbsVelocity();
	int nDamageCustom = 0;
	bool bApplyEffect = true;
	int nDamageType = GetDamageType();

	// Are we a headshot?
	bool bHeadshot = false;
	if ( pBox->group == HITGROUP_HEAD && CanHeadshot() )
	{
		bHeadshot = true;
	}

	// Damage the entity we struck.
	CBaseEntity *pAttacker = GetScorer();
	if ( !pAttacker )
	{
		// likely not launched by a player
		pAttacker = GetOwnerEntity();
	}
	
	if ( pAttacker )
	{
		// Check if we have the penetrate attribute.  We don't want
		// to strike the same target multiple times.
		if ( m_bPenetrate )
		{
			// Don't strike the same target again
			if ( m_HitEntities.Find( pOther->entindex() ) != m_HitEntities.InvalidIndex() )
			{
				bApplyEffect = false;
			}
			else
			{
				m_HitEntities.AddToTail( pOther->entindex() );
			}
		}

		if ( !InSameTeam( pOther ) )
		{
			IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
			if ( pScorerInterface )
			{
				pAttacker = pScorerInterface->GetScorer();
			}

			if ( m_bArrowAlight )
			{
				nDamageType |= DMG_IGNITE;
				nDamageCustom = TF_DMG_CUSTOM_FLYINGBURN;
			}

			if ( bHeadshot )
			{
				nDamageType |= DMG_CRITICAL;
				nDamageCustom = TF_DMG_CUSTOM_HEADSHOT;
			}

			if ( m_bCritical )
			{
				nDamageType |= DMG_CRITICAL;
			}

#ifdef GAME_DLL
			if ( TFGameRules()->IsPVEModeControlled( pAttacker ) )
			{
				// scenario bots cant crit (unless they always do)
				CTFBot *bot = ToTFBot( pAttacker );
				if ( !bot || !bot->HasAttribute( CTFBot::ALWAYS_CRIT ) )
				{
					nDamageType &= ~DMG_CRITICAL;
				}
			}
#endif
			// Damage
			if ( bApplyEffect )
			{
				// Apply Milk First so we can get health from this
				if ( m_bApplyMilkOnHit && pOther->IsPlayer() )
				{
					CTFPlayer *pVictim = ToTFPlayer( pOther );
					if ( pVictim && pVictim->m_Shared.CanBeDebuffed() && pVictim->CanGetWet() )
					{
						// duration is based on damage
						float flDuration = RemapValClamped( GetDamage(), 25.0f, 75.0f, 6.0f, 10.0f );
						pVictim->m_Shared.AddCond( TF_COND_MAD_MILK, flDuration, pAttacker );
						pVictim->m_Shared.SetPeeAttacker( ToTFPlayer( pAttacker ) );
						pVictim->SpeakConceptIfAllowed( MP_CONCEPT_JARATE_HIT );
					}
				}

				CTakeDamageInfo info( this, pAttacker, m_hLauncher, vecVelocity, vecOrigin, GetDamage(), nDamageType, nDamageCustom );
				pOther->TakeDamage( info );

				// Play an impact sound.
				ImpactSound( "Weapon_Arrow.ImpactFlesh", true );
			}
		}
		else if ( pOther->IsPlayer() ) // Hit a team-mate.
		{
			// Heal
			if ( bApplyEffect )
			{
				ImpactTeamPlayer( dynamic_cast<CTFPlayer*>( pOther ) );
			}
		}
	}

	if ( !m_bPenetrate && !bBreakArrow )
	{
		OnArrowImpact( pBox, pOther, pAttacker );
	}

	// Perform a blood mesh decal trace.
	trace_t tr;
	Vector start = vecOrigin - vecVelocity * gpGlobals->frametime;
	Vector end = vecOrigin + vecVelocity * gpGlobals->frametime;
	CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
	UTIL_TraceLine( start, end, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
	UTIL_ImpactTrace( &tr, 0 );

	// Break it?
	if ( bBreakArrow )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::OnArrowImpact( mstudiobbox_t *pBox, CBaseEntity *pOther, CBaseEntity *pAttacker )
{
	CBaseAnimating *pOtherAnim = dynamic_cast< CBaseAnimating* >(pOther);
	if ( !pOtherAnim )
		return;

	const Vector &vecOrigin = GetAbsOrigin();
	Vector vecVelocity = GetAbsVelocity();

	Vector bonePosition = vec3_origin;
	QAngle boneAngles = QAngle(0,0,0);
	int boneIndexAttached = -1;
	int physicsBoneIndex = -1;
	GetBoneAttachmentInfo( pBox, pOtherAnim, bonePosition, boneAngles, boneIndexAttached, physicsBoneIndex );
	bool bSendImpactMessage = true;

	// Did we kill the target?
	if ( !pOther->IsAlive() && pOther->IsPlayer() )
	{
		CTFPlayer *pTFPlayerOther = dynamic_cast<CTFPlayer*>(pOther);
		if ( pTFPlayerOther && pTFPlayerOther->m_hRagdoll )
		{
			VectorNormalize( vecVelocity );
			if ( CheckRagdollPinned( vecOrigin, vecVelocity, boneIndexAttached, physicsBoneIndex, pTFPlayerOther->m_hRagdoll, pBox->group, pTFPlayerOther->entindex() ) )
			{
				pTFPlayerOther->StopRagdollDeathAnim();
				bSendImpactMessage = false;
			}
		}
	}

	// Notify relevant clients of an arrow impact.
	if ( bSendImpactMessage )
	{
		IGameEvent * event = gameeventmanager->CreateEvent( "arrow_impact" );
		if ( event )
		{
			event->SetInt( "attachedEntity", pOther->entindex() );
			event->SetInt( "shooter", pAttacker ? pAttacker->entindex() : 0 );
			event->SetInt( "attachedEntity", pOther->entindex() );
			event->SetInt( "boneIndexAttached", boneIndexAttached );
			event->SetFloat( "bonePositionX", bonePosition.x );
			event->SetFloat( "bonePositionY", bonePosition.y );
			event->SetFloat( "bonePositionZ", bonePosition.z );
			event->SetFloat( "boneAnglesX", boneAngles.x );
			event->SetFloat( "boneAnglesY", boneAngles.y );
			event->SetFloat( "boneAnglesZ", boneAngles.z );
			event->SetInt( "projectileType", GetProjectileType() );
			gameeventmanager->FireEvent( event );
		}
	}

	FadeOut( 3.0 );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::OnArrowImpactObject( CBaseEntity *pOther )
{
	if ( InSameTeam( pOther ) )
	{
		BuildingHealingArrow( pOther );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::ImpactThink( void )
{
}

//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::BuildingHealingArrow( CBaseEntity *pOther )
{
	// This arrow impacted a building
	// If its a building on our team, heal it
	if ( !pOther->IsBaseObject() )
		return;

	CTFPlayer *pTFAttacker = ToTFPlayer( GetScorer() );
	if ( !pTFAttacker )
		return;

	// if not on our team, forget about it
	if ( GetTeamNumber() != pOther->GetTeamNumber() )
		return;

	int iArrowHealAmount = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER( pTFAttacker, iArrowHealAmount, arrow_heals_buildings );
	if ( iArrowHealAmount == 0 )
		return;

	CBaseObject *pBuilding = dynamic_cast< CBaseObject * >( pOther );
	if ( !pBuilding || pBuilding->HasSapper() || pBuilding->IsPlasmaDisabled() || pBuilding->IsBuilding() || pBuilding->IsPlacing() )
		return;

	// if building is shielded, reduce health gain
	if ( pBuilding->GetShieldLevel() == SHIELD_NORMAL )
	{
		iArrowHealAmount *= SHIELD_NORMAL_VALUE;
	}

	int nHealed = pBuilding->Command_Repair( pTFAttacker, iArrowHealAmount, 1.f, 4.f, true );
	if ( nHealed > 0 )
	{
		const char *pParticleName = GetTeamNumber() == TF_TEAM_BLUE ? CLAW_REPAIR_EFFECT_BLU : CLAW_REPAIR_EFFECT_RED;
		CPVSFilter filter( GetAbsOrigin() );
		TE_TFParticleEffect( filter, 0.0, pParticleName, GetAbsOrigin(), vec3_angle );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Arrow::GetArrowSkin() const
{
	int nTeam = GetTeamNumber();
	if ( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pOwner && pOwner->IsPlayerClass( TF_CLASS_SPY ) && pOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			nTeam = pOwner->m_Shared.GetDisguiseTeam();
		}
	}
	return ( nTeam == TF_TEAM_BLUE ) ? 1 : 0;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::OnArrowMissAllPlayers()
{
	CTFPlayer* pOwner = ToTFPlayer( GetOwnerEntity() );
	if( pOwner && pOwner->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		EconEntity_OnOwnerKillEaterEventNoPartner( assert_cast<CEconEntity *>( m_hLauncher.Get() ), pOwner, kKillEaterEvent_NEGATIVE_SniperShotsMissed );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::ArrowTouch( CBaseEntity *pOther )
{
	// Safety net hack:
	// We routinely introduce new entity types, and arrows
	// are repeat-offenders at not getting along with them.
	// If enough time goes by, just remove the arrow.
	float flAliveTime = gpGlobals->curtime - m_flInitTime;
	if ( flAliveTime >= 10.f )
	{
		Warning( "Arrow alive for %f3.2\n seconds", flAliveTime );
		UTIL_Remove( this );
	}

	if ( m_bStruckEnemy || (GetMoveType() == MOVETYPE_NONE) )
		return;

	if ( !pOther )
		return;

	bool bShield = pOther->IsCombatItem() && !InSameTeam( pOther );
	CTFPumpkinBomb *pPumpkinBomb = dynamic_cast< CTFPumpkinBomb * >( pOther );

	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) && !pPumpkinBomb && !bShield )
		return;

	// test against combat characters, which include players, engineer buildings, and NPCs
	CBaseCombatCharacter *pOtherCombatCharacter = dynamic_cast< CBaseCombatCharacter * >( pOther );

	if ( !pOtherCombatCharacter )
	{
		// It might be a track train with boss parented
		pOtherCombatCharacter = dynamic_cast< CBaseCombatCharacter * >( pOther->FirstMoveChild() );
		if ( pOtherCombatCharacter )
		{
			pOther = pOtherCombatCharacter;
		}
	}

	CTFMerasmusTrickOrTreatProp *pMerasmusProp = dynamic_cast< CTFMerasmusTrickOrTreatProp* >( pOther );
	CTFRobotDestruction_Robot *pRobot = dynamic_cast< CTFRobotDestruction_Robot* >( pOther );
	if ( pOther->IsWorld() || ( !pOtherCombatCharacter && !pPumpkinBomb && !pMerasmusProp && !bShield && !pRobot ) )
	{
		// Check to see if we struck the skybox.
		CheckSkyboxImpact( pOther );

		// If we've only got 1 entity in the hit list (the attacker by default) and we've not been deflected
		// then we can consider this arrow to have completely missed all players.
		if( m_HitEntities.Count() == 1 && GetDeflected() == 0 )
		{
			OnArrowMissAllPlayers();
		}

		return;
	}

	CBaseAnimating *pAnimOther = dynamic_cast<CBaseAnimating*>(pOther);
	CStudioHdr *pStudioHdr = NULL;
	mstudiohitboxset_t *set = NULL;
	if ( pAnimOther )
	{
		pStudioHdr = pAnimOther->GetModelPtr();
		if ( pStudioHdr )
		{
			set = pStudioHdr->pHitboxSet( pAnimOther->GetHitboxSet() );
		}
	}

	if ( !pAnimOther || !pStudioHdr || !set )
	{
		// Whatever we hit doesn't have hitboxes. Ignore it.
		UTIL_Remove( this );
		return;
	}

	// We struck the collision box of a player or a buildable object.
	// Trace forward to see if we struck a hitbox.
	CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
	Vector start = GetAbsOrigin();
	Vector vel = GetAbsVelocity();
	trace_t tr;
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );

	// If we hit a hitbox, stop tracing.
	mstudiobbox_t *closest_box = NULL;
	if ( tr.m_pEnt && tr.m_pEnt == pOther && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() )
	{
		// This means the arrow was true and was flying directly at a hitbox on the target.
		// We'll attach to that hitbox.
		closest_box = set->pHitbox( tr.hitbox );
	}

	if ( !closest_box )
	{
		// Locate the hitbox closest to our point of impact on the collision box.
		Vector position, start, forward;
		QAngle angles;
		float closest_dist = 99999;

		// Intense, but extremely accurate:
		AngleVectors( GetAbsAngles(), &forward );
		start = GetAbsOrigin() + forward*16;
		for ( int i = 0; i < set->numhitboxes; i++ )
		{
			mstudiobbox_t *pbox = set->pHitbox( i );

			pAnimOther->GetBonePosition( pbox->bone, position, angles );

			Ray_t ray;
			ray.Init( start, position );
			trace_t tr;
			IntersectRayWithBox( ray, position+pbox->bbmin, position+pbox->bbmax, 0.f, &tr );
			float dist = tr.endpos.DistTo( start );

			if ( dist < closest_dist )
			{
				closest_dist = dist;
				closest_box = pbox;
			}
		}
	}

	if ( closest_box  )
	{
		// See if we're supposed to stick in the target.
		bool bStrike = StrikeTarget( closest_box, pOther );
		if ( bStrike && !m_bPenetrate)
		{
			// If we're here, it means StrikeTarget() called FadeOut( 3.0 )
			SetAbsOrigin( start );
		}

		if ( !bStrike || bShield )
		{
			BreakArrow();
		}

		// Slightly confusing.  If we're here, the arrow stopped at the
		// target and will fade or break.  Setting this prevents the
		// touch code from re-running during the delay.
		if ( !m_bPenetrate  )
		{
			m_bStruckEnemy = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::CheckSkyboxImpact( CBaseEntity *pOther )
{
	trace_t tr;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
	if ( tr.fraction < 1.0 && tr.surface.flags & SURF_SKY )
	{
		// We hit the skybox, go away soon.
		FadeOut( 3.f );
		return;
	}

	if ( !pOther->IsWorld() )
	{
		BreakArrow();
	}
	else
	{
		CEffectData	data;
		data.m_vOrigin = tr.endpos;
		data.m_vNormal = velDir;
		data.m_nEntIndex = 0;/*tr.fraction != 1.0f;*/
		data.m_nAttachmentIndex = 0;
		data.m_nMaterial = 0;
		data.m_fFlags = GetProjectileType();
		data.m_nColor = GetArrowSkin();

		DispatchEffect( "TFBoltImpact", data );

		FadeOut( 3.f );

		// Play an impact sound.
		const char* pszSoundName = "Weapon_Arrow.ImpactMetal";
		surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( psurf )
		{
			switch ( psurf->game.material )
			{
			case CHAR_TEX_GRATE:
			case CHAR_TEX_METAL:
				pszSoundName = "Weapon_Arrow.ImpactMetal";
				break;

			case CHAR_TEX_CONCRETE:
				pszSoundName = "Weapon_Arrow.ImpactConcrete";
				break;

			case CHAR_TEX_WOOD:
				pszSoundName = "Weapon_Arrow.ImpactWood";
				break;
			}
		}
		ImpactSound( pszSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::ImpactSound( const char *pszSoundName, bool bLoudForAttacker )
{
	CTFPlayer *pAttacker = ToTFPlayer( GetScorer() );
	if ( !pAttacker )
		return;

	if ( bLoudForAttacker )
	{
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = pszSoundName;
		params.m_pflSoundDuration = &soundlen;
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( ToTFPlayer(pAttacker) );
		EmitSound( filter, entindex(), params );

		CSingleUserRecipientFilter attackerFilter( ToTFPlayer(pAttacker) );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( pszSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::BreakArrow()
{
	FadeOut( 3.f );
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CheckRagdollPinned( const Vector &start, const Vector &vel, int boneIndexAttached, int physicsBoneIndex, CBaseEntity *pOther, int iHitGroup, int iVictim )
{
	// Pin to the wall.
	trace_t tr;
	UTIL_TraceLine( start, start + vel * 125, MASK_BLOCKLOS, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0f && tr.DidHitWorld() )
	{
		CEffectData	data;

		data.m_vOrigin = tr.endpos;
		data.m_vNormal = vel;
		data.m_nEntIndex = pOther->entindex();
		data.m_nAttachmentIndex = boneIndexAttached;
		data.m_nMaterial = physicsBoneIndex;
		data.m_nDamageType = iHitGroup;
		data.m_nSurfaceProp = iVictim;
		data.m_fFlags = GetProjectileType();
		data.m_nColor = GetArrowSkin();

		if ( GetScorer() )
		{
			data.m_nHitBox = GetScorer()->entindex();
		}

		DispatchEffect( "TFBoltImpact", data );

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::FadeOut( int iTime )
{
	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( vec3_origin	);
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	// Start remove timer.
	SetContextThink( &CTFProjectile_Arrow::RemoveThink, gpGlobals->curtime + iTime, "ARROW_REMOVE_THINK" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::RemoveThink( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
const char *CTFProjectile_Arrow::GetTrailParticleName( void )
{
	if ( m_iProjectileType == TF_PROJECTILE_BUILDING_REPAIR_BOLT )
	{	
		return ( GetTeamNumber() == TF_TEAM_RED ) ? CLAW_TRAIL_RED : CLAW_TRAIL_BLU;
	}
	else if ( m_iProjectileType == TF_PROJECTILE_HEALING_BOLT || m_iProjectileType == TF_PROJECTILE_FESTIVE_HEALING_BOLT )
	{
		return ( GetTeamNumber() == TF_TEAM_RED ) ? "effects/healingtrail_red.vmt" : "effects/healingtrail_blu.vmt";
	}

	return ( GetTeamNumber() == TF_TEAM_RED ) ? "effects/arrowtrail_red.vmt" : "effects/arrowtrail_blu.vmt";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::CreateTrail( void )
{
	if ( IsDormant() )
		return;

	if ( !m_pTrail )
	{
		int width = 3;
		switch ( m_iProjectileType )
		{
			case TF_PROJECTILE_BUILDING_REPAIR_BOLT:
				width = 5;
				break;
			case TF_PROJECTILE_HEALING_BOLT:
			case TF_PROJECTILE_FESTIVE_HEALING_BOLT:
			case TF_PROJECTILE_GRAPPLINGHOOK:
				return; // do not create arrow trail for healing bolt, use particle instead (client only)
		}
		
		const char *pTrailTeamName = GetTrailParticleName();
		CSpriteTrail *pTempTrail = NULL;

		pTempTrail = CSpriteTrail::SpriteTrailCreate( pTrailTeamName, GetAbsOrigin(), true );
		pTempTrail->FollowEntity( this );
		pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
		pTempTrail->SetStartWidth( width );
		pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
		pTempTrail->SetLifeTime( 0.3 );
		pTempTrail->TurnOn();
		pTempTrail->SetAttachment( this, 0 );
		m_pTrail = pTempTrail;
		SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime + 3, "FadeTrail");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fade and kill the trail
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::RemoveTrail( void )
{
	if ( !m_pTrail )
		return;

	if ( m_pTrail )
	{
		if ( m_flTrailLife <= 0 )
		{
			UTIL_Remove( m_pTrail );
			m_flTrailLife = 1.0f;
		}
		else	
		{
			float fAlpha = 128 * m_flTrailLife;

			CSpriteTrail *pTempTrail = dynamic_cast< CSpriteTrail*>( m_pTrail.Get() );

			if ( pTempTrail )
			{
				pTempTrail->SetBrightness( int(fAlpha) );
			}

			m_flTrailLife = m_flTrailLife - 0.1f;
			SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime + 0.05, "FadeTrail");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt )
{
	if ( pEnt )
	{
		dir = info.GetDamagePosition() - info.GetDamageForce() - pEnt->WorldSpaceCenter();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Arrow was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::IncrementDeflected( void )
{
	m_iDeflected++; 

	// Change trail color.
	if ( m_pTrail )
	{
		UTIL_Remove( m_pTrail );
		m_pTrail = NULL;
		m_flTrailLife = 1.0f;
	}
	CreateTrail();
}

//-----------------------------------------------------------------------------
// Purpose: Arrow was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	CTFPlayer *pTFDeflector = ToTFPlayer( pDeflectedBy );
	if ( !pTFDeflector )
		return;

	ChangeTeam( pTFDeflector->GetTeamNumber() );
	SetLauncher( pTFDeflector->GetActiveWeapon() );

	CTFPlayer* pOldOwner = ToTFPlayer( GetOwnerEntity() );
	SetOwnerEntity( pTFDeflector );

	if ( pOldOwner )
	{
		pOldOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
	}

	if ( pTFDeflector->m_Shared.IsCritBoosted() )
	{
		SetCritical( true );
	}

	CTFWeaponBase::SendObjectDeflectedEvent( pTFDeflector, pOldOwner, GetWeaponID(), this );

	IncrementDeflected();
	SetScorer( pTFDeflector );

	// Purge our hit list so we can hit everyone again
	m_HitEntities.Purge();
	// Add ourselves so we dont hit ourselves
	m_HitEntities.AddToTail( pTFDeflector->entindex() );
}


//-----------------------------------------------------------------------------
// Purpose: Setup function.
//-----------------------------------------------------------------------------
void CTFProjectile_HealingBolt::InitArrow( const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	BaseClass::InitArrow( vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer );

	//SetNextThink( gpGlobals->curtime );
}

// ConVar healingbolt_uber_scale( "healingbolt_uber_scale", "1.0", FCVAR_REPLICATED, "" );

//-----------------------------------------------------------------------------
// Purpose: Healing bolt heal.
//-----------------------------------------------------------------------------
void CTFProjectile_HealingBolt::ImpactTeamPlayer( CTFPlayer *pOther )
{
	if ( !pOther )
		return;


	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return;

	// Don't heal players using a weapon that blocks healing
	CTFWeaponBase *pWeapon = pOther->GetActiveTFWeapon();
	if ( pWeapon )
	{
		int iBlockHealing = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER( pWeapon, iBlockHealing, weapon_blocks_healing );
		if ( iBlockHealing )
			return;
	}

	float flHealth = GetDamage() * 2.0f;


	// Scale this if needed
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pOther, flHealth, mult_healing_from_medics );

	CTFWeaponBase *pActiveWeapon = pOther->GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pActiveWeapon, flHealth, mult_health_fromhealers_penalty_active );
	}
	
	int iActualHealed = pOther->TakeHealth( flHealth, DMG_GENERIC );
	if ( iActualHealed <= 0 )
		return;

	// Play an impact sound.
	ImpactSound( "Weapon_Arrow.ImpactFleshCrossbowHeal" );

	CTF_GameStats.Event_PlayerHealedOther( pOwner, flHealth );

	IGameEvent * event = gameeventmanager->CreateEvent( "player_healed" );
	if ( event )
	{
		// HLTV event priority, not transmitted
		event->SetInt( "priority", 1 );	

		// Healed by another player.
		event->SetInt( "patient", pOther->GetUserID() );
		event->SetInt( "healer", pOwner->GetUserID() );
		event->SetInt( "amount", flHealth );
		gameeventmanager->FireEvent( event );
	}

	event = gameeventmanager->CreateEvent( "player_healonhit" );
	if ( event )
	{
		event->SetInt( "amount", flHealth );
		event->SetInt( "entindex", pOther->entindex() );
		item_definition_index_t healingItemDef = INVALID_ITEM_DEF_INDEX;
		if ( pWeapon && pWeapon->GetAttributeContainer() && pWeapon->GetAttributeContainer()->GetItem() )
		{
			healingItemDef = pWeapon->GetAttributeContainer()->GetItem()->GetItemDefIndex();
		}
		event->SetInt( "weapon_def_index", healingItemDef );
		gameeventmanager->FireEvent( event ); 
	}

	event = gameeventmanager->CreateEvent( "crossbow_heal" );
	if ( event )
	{
		event->SetInt( "healer", pOwner->GetUserID() );
		event->SetInt( "target", pOther->GetUserID() );
		event->SetInt( "amount", flHealth );
		gameeventmanager->FireEvent( event ); 
	}

	// Add ubercharge based on amount healed
	CWeaponMedigun *pMedigun = static_cast<CWeaponMedigun *>( pOwner->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN ) );
	if ( pMedigun )
	{
		float flTimeSinceDamage = gpGlobals->curtime - pOther->GetLastDamageReceivedTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10.f, 15.f, 3.f, 1.f ); /*healingbolt_uber_scale.GetFloat()*/
		const float flGainRate = 24.f * flScale;

		// Ubercharge rate is based on the medigun's heal rate, then scaled based on last combat time (same rule as the medigun's heal rate)
		pMedigun->AddCharge( ( iActualHealed / flGainRate ) * gpGlobals->frametime );
	}
	pOther->m_Shared.AddCond( TF_COND_HEALTH_OVERHEALED, 1.2f );

	EconEntity_OnOwnerKillEaterEvent_Batched( dynamic_cast<CEconEntity *>( GetLauncher() ), pOwner, pOther, kKillEaterEvent_AllyHealingDone, flHealth );
}


CTFProjectile_GrapplingHook::CTFProjectile_GrapplingHook()
	: m_pImpactFleshSoundLoop( NULL )
{
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::Spawn()
{
	BaseClass::Spawn();

	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
}


void CTFProjectile_GrapplingHook::Precache()
{
	BaseClass::Precache();

	PrecacheModel( "models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl" );
	PrecacheScriptSound( "WeaponGrapplingHook.ImpactFlesh" );
	PrecacheScriptSound( "WeaponGrapplingHook.ImpactDefault" );
	PrecacheScriptSound( "WeaponGrapplingHook.ImpactFleshLoop" );
}


//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::UpdateOnRemove()
{
	// clear hook target
	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pTFPlayer )
	{
		pTFPlayer->SetGrapplingHookTarget( NULL );
		pTFPlayer->m_Shared.RemoveCond( TF_COND_GRAPPLINGHOOK );
	}

	StopImpactFleshSoundLoop();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Setup function.
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::InitArrow( const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	BaseClass::InitArrow( vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer );

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->m_Shared.AddCond( TF_COND_GRAPPLINGHOOK );
	}
}


//-----------------------------------------------------------------------------
// Purpose: OnArrowImpact
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::OnArrowImpact( mstudiobbox_t *pBox, CBaseEntity *pOther, CBaseEntity *pAttacker )
{
	HookTarget( pOther );
}


//-----------------------------------------------------------------------------
// Purpose: OnArrowImpactObject
//-----------------------------------------------------------------------------
bool CTFProjectile_GrapplingHook::OnArrowImpactObject( CBaseEntity *pOther )
{
	HookTarget( pOther );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: CheckSkyboxImpact
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::CheckSkyboxImpact( CBaseEntity *pOther )
{
	trace_t tr;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
	if ( tr.fraction < 1.0 && tr.surface.flags & SURF_SKY )
	{
		// We hit the skybox, go away soon.
		FadeOut( 1.f );
		return;
	}

	if ( !pOther->IsWorld() )
	{
		HookTarget( pOther );
	}
	else
	{
		HookTarget( pOther );

		// rotate the hook model to be perpendicular to the world surface
		Vector vUp;
		AngleVectors( GetAbsAngles(), NULL, NULL, &vUp );
		QAngle qNewAngles;
		VectorAngles( -tr.plane.normal, vUp, qNewAngles );
		SetAbsAngles( qNewAngles );
		SetAbsOrigin( GetAbsOrigin() + 3.f * tr.plane.normal );
	}
}


//-----------------------------------------------------------------------------
// Purpose: HookTarget
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::HookTarget( CBaseEntity *pOther )
{
	if ( !GetOwnerEntity() || !pOther )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pTFPlayer || pTFPlayer->GetGrapplingHookTarget() )
		return;

	CBaseEntity *pTarget = pOther->IsWorld() ? this : pOther;
	const char *pszSoundName = NULL;
	if ( pTarget->IsPlayer() )
	{
		pszSoundName = "WeaponGrapplingHook.ImpactFlesh";
	}
	else
	{
		pszSoundName = "WeaponGrapplingHook.ImpactDefault";
	}
	ImpactSound( pszSoundName );

	pTFPlayer->SetGrapplingHookTarget( pTarget, true );

	// Stop moving!
	if ( pOther->IsPlayer() )
	{
		FollowEntity( pOther, false );
		StartImpactFleshSoundLoop();
	}
	else
		SetMoveType( MOVETYPE_NONE );

	SetContextThink( &CTFProjectile_GrapplingHook::HookLatchedThink, gpGlobals->curtime + 0.1f, "HookLatchedThink" );
}


//-----------------------------------------------------------------------------
// Purpose: HookLatchedThink
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::HookLatchedThink()
{
	// if owner is dead, remove the hook
	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pTFPlayer || !pTFPlayer->IsAlive() )
	{
		UTIL_Remove( this );
		return;
	}

	// if the target nolonger exist or target player is dead, remove the hook
	CBaseEntity *pHookTarget = pTFPlayer->GetGrapplingHookTarget();
	if ( !pHookTarget || ( pHookTarget->IsPlayer() && !pHookTarget->IsAlive() ) )
	{
		UTIL_Remove( this );
		return;
	}
	
	SetContextThink( &CTFProjectile_GrapplingHook::HookLatchedThink, gpGlobals->curtime + 0.1f, "HookLatchedThink" );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::StartImpactFleshSoundLoop()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	CPASAttenuationFilter filter( this );
	m_pImpactFleshSoundLoop = controller.SoundCreate( filter, entindex(), "WeaponGrapplingHook.ImpactFleshLoop" );
	controller.Play( m_pImpactFleshSoundLoop, 1.0, 100 );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_GrapplingHook::StopImpactFleshSoundLoop()
{
	if ( m_pImpactFleshSoundLoop )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pImpactFleshSoundLoop );
		m_pImpactFleshSoundLoop = NULL;
	}
}
