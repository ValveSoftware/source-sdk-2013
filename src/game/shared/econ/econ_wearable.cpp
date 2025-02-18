//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_wearable.h"
#include "vcollide_parse.h"

#ifdef CLIENT_DLL
#include "functionproxy.h"
#include "c_te_effect_dispatch.h"
#endif // CLIENT_DLL

#ifdef TF_CLIENT_DLL
#include "c_team.h"
#include "tf_shareddefs.h"
#include "tf_weapon_jar.h"
#include "c_tf_player.h"
#endif // TF_CLIENT_DLL

#ifdef TF_DLL
#include "tf_player.h"
#endif // TF_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( wearable_item, CEconWearable );
IMPLEMENT_NETWORKCLASS_ALIASED( EconWearable, DT_WearableItem )

// Network Table --
BEGIN_NETWORK_TABLE( CEconWearable, DT_WearableItem )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CEconWearable )
END_DATADESC()
// -- Data Desc

PRECACHE_REGISTER( wearable_item );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableItem, DT_TFWearableItem )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWearableItem, DT_TFWearableItem )
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWearableItem )
END_DATADESC()
// -- Data Desc

//-----------------------------------------------------------------------------
// SHARED CODE
//-----------------------------------------------------------------------------

CEconWearable::CEconWearable()
{
	m_bAlwaysAllow = false;
};

void CEconWearable::InternalSetPlayerDisplayModel( void )
{
	int iClass = 0;
	int iTeam = 0;

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pTFPlayer )
	{
		iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
		iTeam = pTFPlayer->GetTeamNumber();
	}
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )

	// Set our model to the player model
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->IsValid() )
	{
		const char *pszPlayerDisplayModel = pItem->GetPlayerDisplayModel( iClass, iTeam );
		if ( pszPlayerDisplayModel )
		{
			if ( pItem->GetStaticData()->IsContentStreamable() )
			{
				modelinfo->RegisterDynamicModel( pszPlayerDisplayModel, IsClient() );

				if ( pItem->GetVisionFilteredDisplayModel() && pItem->GetVisionFilteredDisplayModel()[ 0 ] != '\0' )
				{
					modelinfo->RegisterDynamicModel( pItem->GetVisionFilteredDisplayModel(), IsClient() );
				}
			}
			SetModel( pszPlayerDisplayModel );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set up the item. GC data may not be available here depending on
// where we're called from.
//-----------------------------------------------------------------------------
void CEconWearable::Spawn( void )
{
	InitializeAttributes();

	Precache();

	InternalSetPlayerDisplayModel();

	BaseClass::Spawn();

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );

#if !defined( CLIENT_DLL )
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	SetBlocksLOS( false );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Player touches the item. Currently wearables don't appear in the
// world, so this is only called directly during equipment assignment.
//-----------------------------------------------------------------------------
void CEconWearable::GiveTo( CBaseEntity *pOther )
{
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

#if !defined( CLIENT_DLL )
	pPlayer->EquipWearable( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconWearable::RemoveFrom( CBaseEntity *pOther )
{
	CBasePlayer *pPlayer = ToBasePlayer(pOther);
	if ( !pPlayer )
		return;

#if !defined( CLIENT_DLL )
	pPlayer->RemoveWearable( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconWearable::GetSkin( void )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem(); // Safe. Checked in base class call.
	if ( pItem )
	{
		int iSkin = pItem->GetSkin( GetTeamNumber() );
		if ( iSkin > -1 )
		{
			return iSkin;
		}
	}

	return ( GetTeamNumber() == (LAST_SHARED_TEAM+1) ) ? 0 : 1;
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CEconWearable::Equip( CBasePlayer* pOwner )
{
	if ( !CanEquip( pOwner ) )
	{
		RemoveFrom( pOwner );
		return;
	}

	SetTouch( NULL );
	SetAbsVelocity( vec3_origin );

	CBaseEntity *pFollowEntity = pOwner;

	if ( IsViewModelWearable() )
	{
		pFollowEntity = pOwner->GetViewModel();
	}

	FollowEntity( pFollowEntity, true );

	SetOwnerEntity( pOwner );

	ReapplyProvision();

	ChangeTeam( pOwner->GetTeamNumber() );
	m_nSkin = GetSkin();

#ifdef GAME_DLL
	UpdateModelToClass();
	UpdateBodygroups( pOwner, true );
	PlayAnimForPlaybackEvent( WAP_ON_SPAWN );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Remove item from the player.
//-----------------------------------------------------------------------------
void CEconWearable::UnEquip( CBasePlayer* pOwner )
{
#ifdef CLIENT_DLL
	SetParticleSystemsVisible( PARTICLE_SYSTEM_STATE_NOT_VISIBLE );
#endif

#ifdef GAME_DLL
	UpdateBodygroups( pOwner, false );
#endif

	StopFollowingEntity();
	SetOwnerEntity( NULL );

	ReapplyProvision();
}
/*
//-----------------------------------------------------------------------------
// Purpose: Hides or shows masked bodygroups associated with this item.
//-----------------------------------------------------------------------------
bool CEconWearable::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	if ( !pOwner )
		return false;

	CAttributeContainer *pCont = GetAttributeContainer();
	if ( !pCont )
		return false;

	CEconItemView *pItem = pCont->GetItem();
	if ( !pItem )
		return false;

	int iTeam = pOwner->GetTeamNumber();
	int iNumBodyGroups = pItem->GetNumModifiedBodyGroups( iTeam );
	for ( int i=0; i<iNumBodyGroups; ++i )
	{
		int iBody = 0;
		const char *pszBodyGroup = pItem->GetModifiedBodyGroup( iTeam, i, iBody );
		int iBodyGroup = pOwner->FindBodygroupByName( pszBodyGroup );

		if ( iBodyGroup == -1 )
			continue;

		pOwner->SetBodygroup( iBodyGroup, iState );
	}

	return true;
}
*/
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconWearable::OnWearerDeath( void )
{
#ifdef CLIENT_DLL
	UpdateParticleSystems();

	// make sure we remove all particles on player death
	// some particles could be created from animation
	// UpdateParticleSystems only updates particles on cosmetics/weapons
	ParticleProp()->StopParticlesInvolving( this );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CEconWearable::GetDropType()
{
	CAttributeContainer *pCont = GetAttributeContainer();
	if ( !pCont )
		return 0;

	CEconItemView *pItem = pCont->GetItem();
	if ( pItem )
		return pItem->GetDropType();
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: Ensures that a player's correct body groups are enabled on client respawn.
//-----------------------------------------------------------------------------
void CEconWearable::UpdateWearableBodyGroups( CBasePlayer* pPlayer )
{
	if ( !pPlayer )
		return;

	for ( int i=0; i<pPlayer->GetNumWearables(); ++i )
	{
		CEconWearable* pItem = pPlayer->GetWearable(i);
		if ( !pItem )
			continue;

		// Dynamic models which are not yet rendering do not modify bodygroups
		if ( pItem->IsDynamicModelLoading() )
			continue;

		// On the client, ignore items that aren't valid.
#ifdef TF_CLIENT_DLL
		if ( pItem->EntityDeemedInvalid() )
			continue;
#endif

		int nVisibleState = 1;
#ifdef TF_CLIENT_DLL
		if ( pItem->ShouldHideForVisionFilterFlags() )
		{
			// Items that shouldn't draw (pyro-vision filtered) shouldn't change any body group states
			// unless they have no model (hatless hats)
			nVisibleState = 0;
		}
#endif

		pItem->UpdateBodygroups( pPlayer, nVisibleState );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWearableItem::CTFWearableItem()
{
}

//-----------------------------------------------------------------------------
// SERVER ONLY CODE
//-----------------------------------------------------------------------------

#if defined( GAME_DLL )

#endif

//-----------------------------------------------------------------------------
// CLIENT ONLY CODE
//-----------------------------------------------------------------------------

#if defined( CLIENT_DLL )

//-----------------------------------------------------------------------------
// Purpose: Mirror should draw logic.
//-----------------------------------------------------------------------------
ShadowType_t CEconWearable::ShadowCastType()
{
	if ( ShouldDraw() )
	{
		return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
	}
	
	return SHADOWS_NONE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconWearable::ShouldDraw( void )
{
	CBasePlayer *pPlayerOwner = ToBasePlayer( GetOwnerEntity() );
	if ( !pPlayerOwner )
	{
		return false;
	}

	bool bUseViewModel = !pPlayerOwner->ShouldDrawThisPlayer();

	// Don't show view models if we're drawing the real player, and don't show non view models if using view models.
	if ( bUseViewModel )
	{
		// VM mode.
		if ( !IsViewModelWearable() )
		{
			return false;
		}
	}
	else
	{
		// Non-viewmodel mode.
		if ( IsViewModelWearable() )
		{
			return false;
		}
	}

	if ( !ShouldDrawWhenPlayerIsDead() && !pPlayerOwner->IsAlive() )
	{
		return false;
	}

	if ( pPlayerOwner->GetTeamNumber() == TEAM_SPECTATOR )
	{
		return false;
	}
		
	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconWearable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	// Update our visibility in case our parents' has changed.
	UpdateVisibility();
	UpdateParticleSystems();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconWearable::ClientThink( void )
{
	BaseClass::ClientThink();

	UpdateParticleSystems();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconWearable::ShouldDrawParticleSystems( void )
{
	// Make sure the entity we're attaching to is being drawn
	CBasePlayer *pPlayerOwner = ToBasePlayer( GetOwnerEntity() );
	if ( !pPlayerOwner )
	{
		Assert ( "CEconWearable has no owner?" );		// Not sure what this means - is is visible or not?
		return false;
	}
	if ( pPlayerOwner->ShouldDrawThisPlayer() )
	{
		return true;
	}
	return false;
}

RenderGroup_t CEconWearable::GetRenderGroup()
{
	if ( IsViewModelWearable() )
		return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT;

	return BaseClass::GetRenderGroup();
}

//-----------------------------------------------------------------------------
// Purpose: Wearable tint colors
//-----------------------------------------------------------------------------
class CProxyItemTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );
		Vector vResult = Vector( 0, 0, 0 );

		if ( pC_BaseEntity )
		{
			CEconItemView *pScriptItem = NULL;

			IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
			C_BaseEntity *pEntity = pRend->GetIClientUnknown()->GetBaseEntity();
			if ( pEntity )
			{
				CEconEntity *pItem = dynamic_cast< CEconEntity* >( pEntity );
				if ( pItem )
				{
					pScriptItem = pItem->GetAttributeContainer()->GetItem();
				}
				else if ( pEntity->GetOwnerEntity() )
				{
					// Try the owner (for viewmodels, etc).
					pEntity = pEntity->GetOwnerEntity();
					pItem = dynamic_cast< CEconEntity* >( pEntity );
					if ( pItem )
					{
						pScriptItem = pItem->GetAttributeContainer()->GetItem();
					}
				}
			}
			else
			{
				// Proxy data can be a script created item itself, if we're in a vgui CModelPanel
				pScriptItem = dynamic_cast< CEconItemView* >( pRend );
			}

#ifdef TF_CLIENT_DLL
			if ( !pScriptItem )
			{
				// Might be a throwable
				CTFWeaponBaseGrenadeProj *pProjectile = dynamic_cast< CTFWeaponBaseGrenadeProj* >( pEntity );
				if ( pProjectile )
				{
					CEconEntity *pItem = dynamic_cast< CEconEntity* >( pProjectile->GetLauncher() );
					if ( pItem )
					{
						pScriptItem = pItem->GetAttributeContainer()->GetItem();
					}
				}
			}

			if ( pScriptItem && pScriptItem->IsValid() )
			{
				const bool bAltColor = pEntity && pEntity->GetTeam() != nullptr
									 ? pEntity->GetTeam()->GetTeamNumber() == TF_TEAM_BLUE
									 : pScriptItem->GetFlags() & kEconItemFlagClient_ForceBlueTeam
									 ? true
									 : false;

				int iModifiedRGB = pScriptItem->GetModifiedRGBValue( bAltColor );
				if ( iModifiedRGB )
				{
					// The attrib returns a packed RGB with values between 0 & 255 packed into the bottom 3 bytes.
					Color clr = Color( ((iModifiedRGB & 0xFF0000) >> 16), ((iModifiedRGB & 0xFF00) >> 8), (iModifiedRGB & 0xFF) );

					vResult.x = clamp( clr.r() * (1.f / 255.0f), 0.f, 1.0f );
					vResult.y = clamp( clr.g() * (1.f / 255.0f), 0.f, 1.0f );
					vResult.z = clamp( clr.b() * (1.f / 255.0f), 0.f, 1.0f );
				}
			}
#endif // TF_CLIENT_DLL
		}

		m_pResult->SetVecValue( vResult.x, vResult.y, vResult.z );
	}
};
EXPOSE_INTERFACE( CProxyItemTintColor, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );



//============================================================================================================================
extern ConVar	r_propsmaxdist;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
C_EconWearableGib::C_EconWearableGib()
{
	m_fDeathTime = -1;
	m_iHealth = 0;
	m_bParented = false;
	m_bDelayedInit = false;
}

C_EconWearableGib::~C_EconWearableGib()
{
	PhysCleanupFrictionSounds( this );
	VPhysicsDestroyObject();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconWearableGib::Initialize( bool bWillBeParented )
{
	m_bParented = bWillBeParented;
	return InitializeAsClientEntity( STRING( GetModelName() ), RENDER_GROUP_OPAQUE_ENTITY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr* C_EconWearableGib::OnNewModel()
{
	CStudioHdr* pCStudioHdr = BaseClass::OnNewModel();
	if ( m_bDelayedInit && !IsDynamicModelLoading() )
	{
		FinishModelInitialization();
	}
	return pCStudioHdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconWearableGib::SpawnClientEntity( void )
{
	if ( !IsDynamicModelLoading() )
	{
		FinishModelInitialization();
	}
	else
	{
		m_bDelayedInit = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconWearableGib::FinishModelInitialization( void )
{
	UpdateThinkState();

	const model_t *mod = GetModel();
	if ( mod )
	{
		Vector mins, maxs;
		modelinfo->GetModelBounds( mod, mins, maxs );
		SetCollisionBounds( mins, maxs );
	}

	if ( !m_bParented )
	{
		// Create the object in the physics system
		solid_t tmpSolid;
		if ( !PhysModelParseSolid( tmpSolid, this, GetModelIndex() ) )
		{
			DevMsg("C_EconWearableGib::FinishModelInitialization: PhysModelParseSolid failed for entity %i.\n", GetModelIndex() );
			return false;
		}
		else
		{
			m_pPhysicsObject = VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );

			if ( !m_pPhysicsObject )
			{
				// failed to create a physics object
				DevMsg(" C_EconWearableGib::FinishModelInitialization: VPhysicsInitNormal() failed for %s.\n", STRING(GetModelName()) );
				return false;
			}
		}
	}

	Spawn();

	if ( m_fadeMinDist < 0 )
	{
		// start fading out at 75% of r_propsmaxdist
		m_fadeMaxDist = r_propsmaxdist.GetFloat();
		m_fadeMinDist = r_propsmaxdist.GetFloat() * 0.75f;
	}

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	UpdatePartitionListEntry();

	CollisionProp()->UpdatePartition();

	SetBlocksLOS( false ); // this should be a small object

	// Set up shadows; do it here so that objects can change shadowcasting state
	CreateShadow();

	UpdateVisibility();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconWearableGib::Spawn()
{
	BaseClass::Spawn();
	m_takedamage = DAMAGE_EVENTS_ONLY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_EconWearableGib::ValidateEntityAttachedToPlayer( bool &bShouldRetry )
{
	bShouldRetry = false;

	// Always valid as long as we're not parented to anything
	return (GetMoveParent() == NULL);
}

#define WEARABLE_FADEOUT_TIME	1.0f

//-----------------------------------------------------------------------------
// Purpose: Figure out if we need to think or not
//-----------------------------------------------------------------------------
bool C_EconWearableGib::UpdateThinkState( void )
{
	if ( m_fDeathTime > 0 )
	{
		// If we're in the active fadeout portion, think rapidly. Otherwise, wait for that time.
		if ( (m_fDeathTime - gpGlobals->curtime) > WEARABLE_FADEOUT_TIME )
		{
			SetNextClientThink( m_fDeathTime - WEARABLE_FADEOUT_TIME );
		}
		else
		{
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
		return true;
	}

	SetNextClientThink( CLIENT_THINK_NEVER );
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconWearableGib::ClientThink( void )
{
	if ( (m_fDeathTime > 0) && ((m_fDeathTime - gpGlobals->curtime) <= WEARABLE_FADEOUT_TIME) )
	{
		if ( m_fDeathTime <= gpGlobals->curtime )
		{
			Release(); // Die
			return;
		}

		// fade out 
		float alpha = (m_fDeathTime - gpGlobals->curtime) / WEARABLE_FADEOUT_TIME;
		SetRenderMode( kRenderTransTexture );
		SetRenderColorA( alpha * 256 );
	}

	UpdateThinkState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconWearableGib::StartFadeOut( float fDelay )
{
	m_fDeathTime = gpGlobals->curtime + fDelay + WEARABLE_FADEOUT_TIME;
	UpdateThinkState();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EconWearableGib::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;
	int iDamage = 0;

	if ( iDamageType & DMG_BLAST )
	{
		iDamage = VectorLength( dir );
		dir *= 500;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		// guess avg damage
		if ( iDamageType == DMG_BULLET )
		{
			iDamage = 30;
		}
		else
		{
			iDamage = 50;
		}

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	
	}
}

#if 0
#ifdef _DEBUG
#include "econ_item_system.h"

static CUtlVector< const char * > s_possibleModels;
static CUtlVector< const GameItemDefinition_t * > s_possibleDefinitions;
void Dbg_TestDynamicWearableGibs( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	C_EconWearableGib *pEntity = new C_EconWearableGib();
	if ( !pEntity )
		return;

	Vector forward;
	pLocalPlayer->EyeVectors( &forward );
	trace_t tr;
	UTIL_TraceLine( pLocalPlayer->EyePosition(), pLocalPlayer->EyePosition() + (forward * 256), MASK_NPCSOLID, pLocalPlayer, COLLISION_GROUP_NONE, &tr );

	Vector position = tr.endpos;

	if ( s_possibleModels.Count() == 0 )
	{
		FOR_EACH_MAP( ItemSystem()->GetItemSchema()->GetItemDefinitionMap(), nDefn )
		{
			const GameItemDefinition_t *pDefn = dynamic_cast<GameItemDefinition_t *>( ItemSystem()->GetItemSchema()->GetItemDefinitionMap()[nDefn] );
			if ( !pDefn )
				continue;

			const char *pszModel = pDefn->GetPlayerDisplayModel( 0 );
			if ( pszModel && pszModel[0] && pszModel[0] != '?' && pDefn->BLoadOnDemand() && pDefn->GetDropType() == ITEM_DROP_TYPE_DROP )
			{
				s_possibleModels.AddToTail( pszModel );
				s_possibleDefinitions.AddToTail( pDefn );
			}
		}
	}

	Assert( s_possibleModels.Count() );

	int spawnIndex = random->RandomInt( 0, s_possibleModels.Count() - 1 );
	const char *pszModelName = s_possibleModels[ spawnIndex ];
	const GameItemDefinition_t *pDefn = s_possibleDefinitions[ spawnIndex ];
	Msg( "Spawning: %s\n", pszModelName );
	pEntity->SetModelName( AllocPooledString( pszModelName ) );
	pEntity->SetAbsOrigin( position );
	pEntity->SetAbsAngles( vec3_angle );
	pEntity->SetOwnerEntity( pLocalPlayer );
	pEntity->ChangeTeam( pLocalPlayer->GetTeamNumber() );						// our gibs will match our team; this will probably not be used for anything besides team coloring
	// Copy the script created item data over
	pEntity->GetAttributeContainer()->GetItem()->Init( pDefn->GetDefinitionIndex(), pDefn->GetQuality(), pDefn->GetMinLevel(), true );

	if ( !pEntity->Initialize( false ) )
	{
		pEntity->Release();
		return;
	}

	pEntity->StartFadeOut( 15.0f );
	return;

	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
	if ( !pPhysicsObject )
	{
		pEntity->Release();
		return;
	}

	// randomize velocity by 5%
	Vector rndVel = Vector(0,0,100);
	pPhysicsObject->AddVelocity( &rndVel, &vec3_origin );
}
static ConCommand dbg_testdynamicwearablegib( "dbg_testdynamicwearablegib", Dbg_TestDynamicWearableGibs, "", FCVAR_CHEAT );
#endif // _DEBUG
#endif

#endif // client only
