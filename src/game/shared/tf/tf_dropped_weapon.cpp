//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tf_dropped_weapon.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "model_types.h"
#endif // CLIENT_DLL

#ifdef GAME_DLL
#include "tf_player.h"
#include "tf_weaponbase.h"
#include "tf_weapon_medigun.h"
#include "tf_gamerules.h"
#include "tf_weapon_bottle.h"
#endif // GAME_DLL


#ifdef GAME_DLL
ConVar tf_dropped_weapon_lifetime( "tf_dropped_weapon_lifetime", "30", FCVAR_CHEAT ); 

EXTERN_SEND_TABLE( DT_ScriptCreatedItem );

LINK_ENTITY_TO_CLASS( tf_dropped_weapon, CTFDroppedWeapon );

PRECACHE_REGISTER( tf_dropped_weapon );
#else
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFDroppedWeapon, DT_TFDroppedWeapon );

BEGIN_NETWORK_TABLE( CTFDroppedWeapon, DT_TFDroppedWeapon )
#if !defined( CLIENT_DLL )
	SendPropDataTable( SENDINFO_DT(m_Item), &REFERENCE_SEND_TABLE(DT_ScriptCreatedItem) ),
	SendPropFloat( SENDINFO( m_flChargeLevel ) ),
#else
	RecvPropDataTable( RECVINFO_DT(m_Item), 0, &REFERENCE_RECV_TABLE(DT_ScriptCreatedItem) ),
	RecvPropFloat( RECVINFO( m_flChargeLevel ) ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_AUTO_LIST( IDroppedWeaponAutoList );

//-----------------------------------------------------------------------------
CTFDroppedWeapon::CTFDroppedWeapon()
#ifdef GAME_DLL
	: m_nClip( 0 )
	, m_nAmmo( 0 )
	, m_nDetonated( 0 )
	, m_flEnergy( 0.f )
	, m_flEffectBarRegenTime( 0.f )
	, m_flNextPrimaryAttack ( 0.f )
	, m_flNextSecondaryAttack( 0.f )
	, m_bBroken( false )
	, m_flMeter( 0.f )
#endif
{
#ifdef CLIENT_DLL
	m_pGlowEffect = NULL;
	m_bShouldGlowForLocalPlayer = false;
	m_flOldChargeLevel = 0.f;
#endif // CLIENT_DLL

	m_flChargeLevel.Set( 0.f );
}

//-----------------------------------------------------------------------------
CTFDroppedWeapon::~CTFDroppedWeapon()
{
#ifdef CLIENT_DLL
	if ( m_worldmodelStatTrakAddon )
	{
		m_worldmodelStatTrakAddon->Remove();
	}

	if ( m_effect )
	{
		ParticleProp()->StopEmission( m_effect );
		m_effect = NULL;
	}

	DestroyGlowEffect();
#endif // CLIENT_DLL
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::Spawn()
{
#ifdef GAME_DLL
	SetModel( STRING( GetModelName() ) );

	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetSolid( SOLID_BBOX );
	SetBlocksLOS( false );
	AddEFlags( EFL_NO_ROTORWASH_PUSH );

	// This will make them not collide with the player, but will collide
	// against other items + weapons
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	CollisionProp()->UseTriggerBounds( true, ITEM_PICKUP_BOX_BLOAT );

	// Create the object in the physics system
	int nSolidFlags = GetSolidFlags() | FSOLID_NOT_STANDABLE;

	if ( VPhysicsInitNormal( SOLID_VPHYSICS, nSolidFlags, false ) == NULL )
	{
		SetSolid( SOLID_BBOX );
		AddSolidFlags( nSolidFlags );

		// If it's not physical, drop it to the floor
		if ( UTIL_DropToFloor( this, MASK_SOLID ) == 0 )
		{
			Warning( "Item %s fell out of level at %f,%f,%f\n", GetClassname(), GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z);
			UTIL_Remove( this );
			return;
		}
	}

#endif // GAME_DLL
	BaseClass::Spawn();

#ifdef GAME_DLL
	SetContextThink( &CTFDroppedWeapon::SUB_Remove, gpGlobals->curtime + tf_dropped_weapon_lifetime.GetFloat(), "RemoveThink" );
#endif // GAME_DLL
}


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
void CTFDroppedWeapon::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_flOldChargeLevel = m_flChargeLevel;
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// if its startrak attach a model to it
		if ( m_Item.GetItemID() != INVALID_ITEM_ID )
		{
			int iStrangeType = -1;
			for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
			{
				if ( m_Item.FindAttribute( GetKillEaterAttr_Score( i ) ) )
				{
					iStrangeType = i;
					break;
				}
			}

			// It's strange, does it have module as well?
			if ( iStrangeType != -1 )
			{
				CAttribute_String attrModule;
				if ( GetStattrak( &m_Item, &attrModule ) )
				{
					static CSchemaAttributeDefHandle pAttr_moduleScale( "weapon_stattrak_module_scale" );
					// Does it have a stat track module
					float flScale = 1.0f;
					uint32 unFloatAsUint32 = 1;
					if ( m_Item.FindAttribute( pAttr_moduleScale, &unFloatAsUint32 ) )
					{
						flScale = (float&)unFloatAsUint32;
					}

					C_BaseAnimating *pStatTrakEnt = new class C_BaseAnimating;
					if ( pStatTrakEnt && pStatTrakEnt->InitializeAsClientEntity( attrModule.value().c_str(), RENDER_GROUP_OPAQUE_ENTITY ) )
					{
						pStatTrakEnt->AddEffects( EF_BONEMERGE );
						pStatTrakEnt->AddEffects( EF_BONEMERGE_FASTCULL );

						m_worldmodelStatTrakAddon = pStatTrakEnt;
						pStatTrakEnt->SetParent( this );
						pStatTrakEnt->SetLocalOrigin( vec3_origin );
						pStatTrakEnt->UpdatePartitionListEntry();
						pStatTrakEnt->CollisionProp()->MarkPartitionHandleDirty();
						pStatTrakEnt->SetModelScale( flScale );
						pStatTrakEnt->UpdateVisibility();

						pStatTrakEnt->SetBodygroup( 1, 1 );

						pStatTrakEnt->m_nSkin = m_Item.GetTeamNumber();	// Use the "Sad" skin

						//pStatTrakEnt->SetModelScale( 2.0f );
						//	//if ( !cl_righthand.GetBool() )
						//	//{
						//	//	pStatTrakEnt->SetBodygroup( 0, 1 ); // use a special mirror-image stattrak module that appears correct for lefties
						//	//}

						RemoveEffects( EF_NODRAW );
					}
				}
			}

			// Normal Attached models (ie festive lights)
			const CEconItemDefinition *pItemDef = m_Item.GetItemDefinition();
			if ( pItemDef )
			{
				// Update the state of additional model attachments
				m_vecAttachedModels.Purge();
				int iTeamNumber = m_Item.GetTeamNumber();
				int iAttachedModels = pItemDef->GetNumAttachedModels( iTeamNumber );
				for ( int i = 0; i < iAttachedModels; i++ )
				{
					attachedmodel_t	*pModel = pItemDef->GetAttachedModelData( iTeamNumber, i );

					int iModelIndex = modelinfo->GetModelIndex( pModel->m_pszModelName );
					if ( iModelIndex >= 0 )
					{
						AttachedModelData_t attachedModelData;
						attachedModelData.m_pModel = modelinfo->GetModel( iModelIndex );
						attachedModelData.m_iModelDisplayFlags = pModel->m_iModelDisplayFlags;
						m_vecAttachedModels.AddToTail( attachedModelData );
					}
				}

				// Festive
				{
					static CSchemaAttributeDefHandle pAttr_is_festivized( "is_festivized" );
					if ( pAttr_is_festivized && m_Item.FindAttribute( pAttr_is_festivized ) )
					{
						int iAttachedFestiveModels = pItemDef->GetNumAttachedModelsFestivized( iTeamNumber );
						if ( iAttachedFestiveModels )
						{

							for ( int i = 0; i < iAttachedFestiveModels; i++ )
							{
								attachedmodel_t	*pModel = pItemDef->GetAttachedModelDataFestivized( iTeamNumber, i );

								int iModelIndex = modelinfo->GetModelIndex( pModel->m_pszModelName );
								if ( iModelIndex >= 0 )
								{
									AttachedModelData_t attachedModelData;
									attachedModelData.m_pModel = modelinfo->GetModel( iModelIndex );
									attachedModelData.m_iModelDisplayFlags = pModel->m_iModelDisplayFlags;
									m_vecAttachedModels.AddToTail( attachedModelData );
								}

							}
						}
					}
				}
			}

			SetupParticleEffect();
		}

		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	if ( m_flOldChargeLevel != m_flChargeLevel )
	{
		float flRem = fmod( m_flChargeLevel, 0.1f );
		if ( flRem < 0.01f )
		{
			ParticleProp()->Create( "drain_effect", PATTACH_POINT_FOLLOW, LookupAttachment( "muzzle" ) );
			EmitSound( "Medigun.DrainCharge" );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::ModifyEmitSoundParams( EmitSound_t &params )
{
	params.m_nPitch = RemapVal( m_flChargeLevel, 0.f, 1.f, 90, 180 );
	params.m_nFlags |= SND_CHANGE_PITCH;
}

//-----------------------------------------------------------------------------
bool CTFDroppedWeapon::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnInternalDrawModel( pInfo ) )
		return false;

	// Draw Attached Models
	// Draw our attached models as well
	for ( int i = 0; i < m_vecAttachedModels.Size(); i++ )
	{
		const AttachedModelData_t& attachedModel = m_vecAttachedModels[i];

		if ( attachedModel.m_pModel && ( attachedModel.m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
		{
			ClientModelRenderInfo_t infoAttached = *pInfo;

			infoAttached.pRenderable = this;
			infoAttached.instance = MODEL_INSTANCE_INVALID;
			infoAttached.entity_index = this->index;
			infoAttached.pModel = attachedModel.m_pModel;

			infoAttached.pModelToWorld = &infoAttached.modelToWorld;

			// Turns the origin + angles into a matrix
			AngleMatrix( infoAttached.angles, infoAttached.origin, infoAttached.modelToWorld );

			DrawModelState_t state;
			matrix3x4_t *pBoneToWorld;
			bool bMarkAsDrawn = modelrender->DrawModelSetup( infoAttached, &state, NULL, &pBoneToWorld );
			DoInternalDrawModel( &infoAttached, ( bMarkAsDrawn && ( infoAttached.flags & STUDIO_RENDER ) ) ? &state : NULL, pBoneToWorld );
		}
	}

	return true;
}
//-----------------------------------------------------------------------------
// Purpose: Get an econ material override for the given team.
// Returns: NULL if there is no override. 
//-----------------------------------------------------------------------------
IMaterial *CTFDroppedWeapon::GetEconWeaponMaterialOverride( int iTeam )
{
	CEconItemView *pItemView = GetItem();
	if ( !pItemView )
		return NULL;

	return pItemView->GetMaterialOverride( iTeam );
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::SetupParticleEffect()
{
	attachedparticlesystem_t *pParticleSystem = NULL;

	// do community_sparkle effect if this is a community item?
	const int iQualityParticleType = m_Item.GetQualityParticleType();
	if ( iQualityParticleType > 0 )
	{
		pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iQualityParticleType );
	}

	if ( !pParticleSystem )
	{
		// does this hat even have a particle effect
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
		uint32 iValue = 0;
		if ( !m_Item.FindAttribute( pAttrDef_AttachParticleEffect, &iValue ) )
		{
			return;
		}

		const float& value_as_float = (float&)iValue;
		pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( value_as_float );
	}

	// failed to find any particle effect
	if ( !pParticleSystem )
	{
		return;
	}

	// Team Color
	if ( GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_red" ))
	{
		static char pBlue[256];
		V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
		pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pBlue );
		if ( !pParticleSystem )
		{
			return;
		}
	}

	// World model effect
	// Stop it on both the viewmodel & the world model, because it may be removed due to first/thirdperson switch
	static char pszTempName[256];
	const char* pszSystemName = pParticleSystem->pszSystemName;

	// Weapon Remap for a Base Effect to be used on a specific weapon
	if ( pParticleSystem->bUseSuffixName && m_Item.GetItemDefinition()->GetParticleSuffix() )
	{
		V_strcpy_safe( pszTempName, pszSystemName );
		V_strcat_safe( pszTempName, "_" );
		V_strcat_safe( pszTempName, m_Item.GetItemDefinition()->GetParticleSuffix() );
		pszSystemName = pszTempName;
	}

	m_effect = ParticleProp()->Create( pszSystemName, PATTACH_ABSORIGIN_FOLLOW );
	if ( m_effect )
	{
		for ( int i=0; i<ARRAYSIZE( pParticleSystem->pszControlPoints ); ++i )
		{
			const char *pszAttachmentName = pParticleSystem->pszControlPoints[i];
			if ( pszAttachmentName && pszAttachmentName[0] != '\0' )
			{
				ParticleProp()->AddControlPoint( m_effect, i, this, PATTACH_POINT_FOLLOW, pParticleSystem->pszControlPoints[i] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::ClientThink()
{
	C_TFPlayer *pTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	bool bShouldGlowForLocalPlayer = pTFPlayer && pTFPlayer->IsAlive() && pTFPlayer->CanPickupDroppedWeapon( this ) && pTFPlayer->IsLineOfSightClear( this );
	if ( bShouldGlowForLocalPlayer )
	{
		// ignore the item that the player's equipped
		int iLoadoutSlot = 0;
		CTFItemDefinition *pItemDef = m_Item.GetStaticData();
		if ( pItemDef )
		{
			int iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
			iLoadoutSlot = pItemDef->GetLoadoutSlot( iClass );
			CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( pTFPlayer->GetEntityForLoadoutSlot( iLoadoutSlot ) );
			if ( pWeapon && *pWeapon->GetAttributeContainer()->GetItem() == m_Item )
			{
				bShouldGlowForLocalPlayer = false;
			}
		}
	}

	if ( m_bShouldGlowForLocalPlayer != bShouldGlowForLocalPlayer )
	{
		m_bShouldGlowForLocalPlayer = bShouldGlowForLocalPlayer;
		UpdateGlowEffect();
	}
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect if we have a cart
	if ( m_bShouldGlowForLocalPlayer )
	{
		Vector color = Vector( 0.745f, 0.773f, 0.157f );
		m_pGlowEffect = new CGlowObject( this, color, 1.0, true );
	}
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
bool CTFDroppedWeapon::IsVisibleToTargetID( void ) const
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;
	return pLocalPlayer->CanPickupDroppedWeapon( this );
}

#endif // CLIENT_DLL

#ifdef GAME_DLL
#define MAX_DROPPED_WEAPON_COUNT	32
//-----------------------------------------------------------------------------
CTFDroppedWeapon *CTFDroppedWeapon::Create( CTFPlayer *pLastOwner, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszModelName, const CEconItemView *pItem )
{
	// don't drop weapon in MVM
	if ( TFGameRules()->IsMannVsMachineMode() )
		return NULL;

	int nNumRemoved = 0;

	// make sure we clean up the same item that was dropped before dropping a new one
	for ( int i=0; i<CTFDroppedWeapon::AutoList().Count(); ++i )
	{
		CTFDroppedWeapon *pDroppedWeapon = static_cast< CTFDroppedWeapon* >( CTFDroppedWeapon::AutoList()[i] );
		if ( pDroppedWeapon->m_Item.GetItemID() == pItem->GetItemID() && pDroppedWeapon->m_Item.GetItemDefIndex() == pItem->GetItemDefIndex() && pDroppedWeapon->m_hPlayer.Get() == pLastOwner )
		{
			UTIL_Remove( pDroppedWeapon );
			nNumRemoved++;
		}
	}

	// if we're still going over max dropped weapon count, remove more items
	int nNumToRemove = CTFDroppedWeapon::AutoList().Count() - nNumRemoved - MAX_DROPPED_WEAPON_COUNT;
	for ( int i=0; i<CTFDroppedWeapon::AutoList().Count() && nNumToRemove > 0; ++i )
	{
		CTFDroppedWeapon *pDroppedWeapon = static_cast< CTFDroppedWeapon* >( CTFDroppedWeapon::AutoList()[i] );

		// skip item that we already marked for deletion
		if ( pDroppedWeapon->IsMarkedForDeletion() )
			continue;

		UTIL_Remove( pDroppedWeapon );
		nNumToRemove--;
	}

	CTFDroppedWeapon *pDroppedWeapon = static_cast<CTFDroppedWeapon*>( CBaseAnimating::CreateNoSpawn( "tf_dropped_weapon", vecOrigin, vecAngles ) );
	if ( pDroppedWeapon )
	{
		pDroppedWeapon->SetModelName( AllocPooledString( pszModelName ) );
		pDroppedWeapon->SetItem( pItem );
		DispatchSpawn( pDroppedWeapon );
	}

	return pDroppedWeapon;
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::InitDroppedWeapon( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon,  bool bSwap, bool bIsSuicide /*= false*/ )
{
	m_hPlayer = pPlayer;

	// Calculate the initial impulse on the weapon.
	Vector vecImpulse( 0.0f, 0.0f, 0.0f );
	float flImpulseScale = 0.f;
	if ( bSwap && pPlayer )
	{
		Vector vecForward, vecUp;
		AngleVectors( pPlayer->EyeAngles(), &vecForward, NULL, &vecUp );
		vecImpulse += Vector(0,0,1.5); //vecUp * 0.5f;
		vecImpulse += vecForward * 1.0f;
		flImpulseScale = 250.f;
	}
	else
	{
		Vector vecRight, vecUp;
		AngleVectors( EyeAngles(), NULL, &vecRight, &vecUp );
		vecImpulse += vecUp * random->RandomFloat( -0.25, 0.25 );
		vecImpulse += vecRight * random->RandomFloat( -0.25, 0.25 );
		flImpulseScale = random->RandomFloat( 100.f, 150.f );
	}
	VectorNormalize( vecImpulse );
	vecImpulse *= flImpulseScale;
	vecImpulse += GetAbsVelocity();

	if ( VPhysicsGetObject() )
	{
		// We can probably remove this when the mass on the weapons is correct!
		VPhysicsGetObject()->SetMass( 25.0f );
		AngularImpulse angImpulse( 0, random->RandomFloat( 0, 100 ), 0 );
		VPhysicsGetObject()->SetVelocityInstantaneous( &vecImpulse, &angImpulse );
	}

	m_nSkin = pWeapon->GetSkin();
	
	m_nClip = pWeapon->IsEnergyWeapon() ? pWeapon->GetMaxClip1() : pWeapon->Clip1();
	m_nAmmo = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
	m_flEnergy = pWeapon->Energy_GetEnergy();
	m_flNextPrimaryAttack = pWeapon->m_flNextPrimaryAttack;
	m_flNextSecondaryAttack = pWeapon->m_flNextSecondaryAttack;

	m_bBroken = pWeapon->IsBroken();
	m_nBody = pWeapon->m_nBody;

	CEconItemView *pEconItemView = pWeapon->GetAttributeContainer() ? pWeapon->GetAttributeContainer()->GetItem() : NULL;
	if ( pEconItemView )
	{
		CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
		if ( pItemDef )
		{
			loadout_positions_t eLoadoutPosition = ( loadout_positions_t )( pItemDef->GetLoadoutSlot( pPlayer->GetPlayerClass()->GetClassIndex() ) );
			m_flMeter = pPlayer->m_Shared.GetItemChargeMeter( eLoadoutPosition );
		}
	}

	if ( bIsSuicide )
	{
		m_flChargeLevel = 0.f;
	}
	else
	{
		CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pWeapon );
		if ( pMedigun )
		{
			m_flChargeLevel.Set( pMedigun->GetChargeLevel() );
			if ( m_flChargeLevel > 0.f )
			{
				SetContextThink( &CTFDroppedWeapon::ChargeLevelDegradeThink, gpGlobals->curtime + 0.1f, "ChargeLevelDegradeThink" );
			}
		}
	}

	CTFStickBomb *pStickBomb = dynamic_cast< CTFStickBomb* >( pWeapon );
	if ( pStickBomb )
	{
		m_nDetonated = pStickBomb->GetDetonated();
	}

	// Capture bar regen (Jarate, base ball)
	m_flEffectBarRegenTime = pWeapon->m_flEffectBarRegenTime;

	//DevMsg( "Dropped weapon with: clip[%d] ammo[%d] energy[%f]\n", m_nClip, m_nAmmo, m_flEnergy );
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::InitPickedUpWeapon( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon )
{
	// clear the context think
	SetContextThink( NULL, 0, "ChargeLevelDegradeThink" );

	// preserve the ammo
	int nCurrentMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );
	pWeapon->m_iClip1 = m_nClip;
	if ( pWeapon->GetPrimaryAmmoType() != -1 )
	{
		int nMaxTotalAmmo = pPlayer->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() );
		pPlayer->SetAmmoCount( Min( m_nAmmo, nMaxTotalAmmo ), pWeapon->GetPrimaryAmmoType() );
	}
	// SetAmmoCount can override metal for some weapon
	// Make sure engineer don't gain metal by picking up weapon
	pPlayer->SetAmmoCount( nCurrentMetal, TF_AMMO_METAL );
	pWeapon->Energy_SetEnergy( m_flEnergy );

	CWeaponMedigun *pMedigun = dynamic_cast< CWeaponMedigun* >( pWeapon );
	if ( pMedigun )
	{
		pMedigun->SetChargeLevel( m_flChargeLevel );
	}

	CTFStickBomb *pStickBomb = dynamic_cast< CTFStickBomb* >( pWeapon );
	if ( pStickBomb )
	{
		pStickBomb->SetDetonated( m_nDetonated );
	}

	// stomp the team color
	if ( pWeapon->GetAttributeContainer() && pWeapon->GetAttributeContainer()->GetItem() )
	{
		pWeapon->GetAttributeContainer()->GetItem()->SetTeamNumber( GetItem()->GetTeamNumber() );
	}

	pWeapon->m_flEffectBarRegenTime = m_flEffectBarRegenTime;
 	pWeapon->m_flNextPrimaryAttack = m_flNextPrimaryAttack;
 	pWeapon->m_flNextSecondaryAttack = m_flNextSecondaryAttack;

	pWeapon->SetBroken( m_bBroken );
	pWeapon->m_nBody = m_nBody;

	CEconItemView *pEconItemView = pWeapon->GetAttributeContainer() ? pWeapon->GetAttributeContainer()->GetItem() : NULL;
	if ( pEconItemView )
	{
		CTFItemDefinition *pItemDef = pEconItemView->GetStaticData();
		if ( pItemDef )
		{
			loadout_positions_t eLoadoutPosition = ( loadout_positions_t ) ( pItemDef->GetLoadoutSlot( pPlayer->GetPlayerClass()->GetClassIndex() ) );
			pPlayer->m_Shared.SetItemChargeMeter( eLoadoutPosition, m_flMeter );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::ChargeLevelDegradeThink()
{
	m_flChargeLevel.Set( m_flChargeLevel - 0.01f );

	if ( m_flChargeLevel < 0.f )
	{
		m_flChargeLevel.Set( 0.f );
		SetContextThink( NULL, 0, "ChargeLevelDegradeThink" );
		return;
	}

	SetContextThink( &CTFDroppedWeapon::ChargeLevelDegradeThink, gpGlobals->curtime + 0.1f, "ChargeLevelDegradeThink" );
}

//-----------------------------------------------------------------------------
void CTFDroppedWeapon::SetItem( const CEconItemView *pItem )
{
	if ( pItem )
		m_Item.CopyFrom( *pItem );
}
#endif // GAME_DLL
