//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_item_wearable.h"
#include "vcollide_parse.h"
#include "tf_gamerules.h"
#include "animation.h"
#include "basecombatweapon_shared.h"
#ifdef CLIENT_DLL
#include "c_tf_player.h"
#include "model_types.h"
#include "props_shared.h"
#include "tf_mapinfo.h"
#include "usermessages.h"
#else
#include "tf_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable )

// Network Table --
BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
#if defined( GAME_DLL )
	SendPropBool( SENDINFO( m_bDisguiseWearable ) ),
	SendPropEHandle( SENDINFO( m_hWeaponAssociatedWith ) ),
#else
	RecvPropBool( RECVINFO( m_bDisguiseWearable ) ),
	RecvPropEHandle( RECVINFO( m_hWeaponAssociatedWith ) ),
#endif // GAME_DLL
END_NETWORK_TABLE()
// -- Network Table

// Data Desc --
BEGIN_DATADESC( CTFWearable )
END_DATADESC()
// -- Data Desc

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CTFWearable )
	DEFINE_PRED_FIELD(m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
	DEFINE_PRED_FIELD(m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK),
END_PREDICTION_DATA()
#endif // CLIENT_DLL

PRECACHE_REGISTER( tf_wearable );


LINK_ENTITY_TO_CLASS( tf_wearable_vm, CTFWearableVM );
IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableVM, DT_TFWearableVM )

BEGIN_NETWORK_TABLE( CTFWearableVM, DT_TFWearableVM )
END_NETWORK_TABLE()

PRECACHE_REGISTER( tf_wearable_vm );


//-----------------------------------------------------------------------------
// SHARED CODE
//-----------------------------------------------------------------------------

CTFWearable::CTFWearable() : CEconWearable()
{
	m_bDisguiseWearable = false;
	m_hWeaponAssociatedWith = NULL;
#if defined( CLIENT_DLL )
	m_eParticleSystemVisibility = kParticleSystemVisibility_Undetermined;
	m_nWorldModelIndex = 0;
#endif

	UseClientSideAnimation();
};

//-----------------------------------------------------------------------------
// SERVER ONLY CODE
//-----------------------------------------------------------------------------

#if defined( GAME_DLL )
void CTFWearable::Break( void )
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( GetSkin() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWearable::CalculateVisibleClassFor( CBaseCombatCharacter *pPlayer )
{
	if ( m_bDisguiseWearable )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( pTFPlayer )
			return pTFPlayer->m_Shared.GetDisguiseClass();
	}
	return BaseClass::CalculateVisibleClassFor( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWearable::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWearable::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( pInfo->m_pClientEnt && GetOwnerEntity() && CBaseEntity::Instance( pInfo->m_pClientEnt ) == GetOwnerEntity() )
	{
		return FL_EDICT_ALWAYS;
	}

	// We have some entities that have no model (ie., "hatless hats") but we still want
	// to transmit them down to clients so that the clients	can do things like update body
	// groups, etc.
	return FL_EDICT_PVSCHECK;
}

#endif

#ifdef CLIENT_DLL
ConVar tf_test_hat_bodygroup( "tf_test_hat_bodygroup", "0", 0, "For testing bodygroups on hats." );
#endif

static int CalcBodyGroup( CBaseCombatCharacter* pOwner, CEconItemView *pItem, const char *pBodyGroup, codecontrolledbodygroupdata_t &ccbgd )
{
#ifdef CLIENT_DLL
	if ( !Q_strnicmp( ccbgd.pFuncName, "test", ARRAYSIZE( "test" ) ) )
	{
		return tf_test_hat_bodygroup.GetInt();
	}
	else if ( !Q_strnicmp( ccbgd.pFuncName, "map_contributor", ARRAYSIZE( "map_contributor" ) ) )
	{
		int iDonationAmount = MapInfo_GetDonationAmount( pItem->GetAccountID(), engine->GetLevelName() );
		return MIN( iDonationAmount / 25, 4 );
	}
#endif
	return 0;
}

//-----------------------------------------------------------------------------
// CLIENT ONLY CODE
//-----------------------------------------------------------------------------

#if defined( CLIENT_DLL )
extern ConVar tf_playergib_forceup;

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
void HandleBreakModel( bf_read &msg, bool bCheap )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin;
	QAngle vecAngles;
	int nSkin = 0;
	msg.ReadBitVec3Coord( vecOrigin );
	if ( !bCheap )
	{
		msg.ReadBitAngles( vecAngles );
		nSkin = (int)msg.ReadShort();
	}
	else
	{
		vecAngles = vec3_angle;
	}

	// Launch it straight up with some random spread
	Vector vecBreakVelocity = Vector(0,0,200);
	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
	breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;
	breakParams.nDefaultSkin = nSkin;

	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
}

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
USER_MESSAGE( BreakModel )
{
	HandleBreakModel( msg, false );
}

//-----------------------------------------------------------------------------
// Receive the CheapBreakModel user message
//-----------------------------------------------------------------------------
USER_MESSAGE( CheapBreakModel )
{
	HandleBreakModel( msg, true );
}

//-----------------------------------------------------------------------------
// Receive the BreakModel_Pumpkin user message
//-----------------------------------------------------------------------------
USER_MESSAGE( BreakModel_Pumpkin )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin;
	QAngle vecAngles;
	msg.ReadBitVec3Coord( vecOrigin );
	msg.ReadBitAngles( vecAngles );

	// Launch it straight up with some random spread
	Vector vecBreakVelocity = Vector(0,0,0);
	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
	breakablepropparams_t breakParams( vecOrigin /*+ Vector(0,0,20)*/, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;

	for ( int i=0; i<aGibs.Count(); ++i )
	{
		aGibs[i].burstScale = 1000.f;
	}

	CUtlVector<EHANDLE>	hSpawnedGibs;
	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true, &hSpawnedGibs );

	// Make the base stay low to the ground.
	for ( int i=0; i<hSpawnedGibs.Count(); ++i )
	{
		CBaseEntity *pGib = hSpawnedGibs[i];
		if ( pGib )
		{
			IPhysicsObject *pPhysObj = pGib->VPhysicsGetObject();
			if ( pPhysObj )
			{
				Vector vecVel;
				AngularImpulse angImp;
				pPhysObj->GetVelocity( &vecVel, &angImp );
				vecVel *= 3.0;
				if ( i == 3 )
				{
					vecVel.z = 300;
				}
				else
				{
					vecVel.z = 400;
				}
				pPhysObj->SetVelocity( &vecVel, &angImp );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWearable::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
	{
		bool bShouldDraw = false;
		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem )
		{
			econ_tag_handle_t tagHandle = GetItemSchema()->GetHandleForTag( "ghost_wearable" );
			if ( pItem->GetItemDefinition()->HasEconTag( tagHandle ) )
				bShouldDraw = true;
		}

		if ( !bShouldDraw )
			return 0;
	}

	bool bUseInvulnMaterial = ( pOwner && pOwner->m_Shared.IsInvulnerable() && 
							    ( !pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) || gpGlobals->curtime < pOwner->GetLastDamageTimeMvMOnly() + 2.0f ) );

	if ( bUseInvulnMaterial && (flags & STUDIO_RENDER) )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial && (flags & STUDIO_RENDER) )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWearable::ShouldDraw()
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			const CEconItemView *pItem = GetAttributeContainer()->GetItem();
			if ( pItem )
			{
				econ_tag_handle_t tagHandle = GetItemSchema()->GetHandleForTag( "ghost_wearable" );
				if ( pItem->GetItemDefinition()->HasEconTag( tagHandle ) )
					return BaseClass::ShouldDraw();
			}

			return false;
		}

		// don't draw cosmetic while sniper is zoom
		if ( pOwner == C_TFPlayer::GetLocalTFPlayer() && pOwner->m_Shared.InCond( TF_COND_ZOOMED ) )
			return false;
	}

	// Don't draw 3rd person wearables if our owner is disguised.
	if ( pOwner && pOwner->m_Shared.InCond( TF_COND_DISGUISED ) && !IsViewModelWearable() )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( m_bDisguiseWearable && pLocalPlayer )
		{
			int iLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
			if ( pLocalPlayer->m_bIsCoaching && pLocalPlayer->m_hStudent )
			{
				iLocalPlayerTeam = pLocalPlayer->m_hStudent->GetTeamNumber();
			}

			// This wearable is a part of our disguise -- we might want to draw it.
			if ( GetEnemyTeam( pOwner->GetTeamNumber() ) != iLocalPlayerTeam )
			{	
				// The local player is on this spy's team. We don't see the disguise.
				return false;
			}
			else
			{
				if ( pOwner->m_Shared.GetDisguiseClass() == TF_CLASS_SPY &&
					 pOwner->m_Shared.GetDisguiseTeam() == iLocalPlayerTeam )
				{
					// This enemy spy is disguised as a spy on our team, don't draw wearables.
					return false;
				}
				else
				{
					// The local player is an enemy. Show the disguise wearable.
					return BaseClass::ShouldDraw();
				}
			}
		}

		return false;
	}
	else
	{
		// See if the visibility is controlled by a weapon.
		CTFWeaponBase *pWeapon = assert_cast< CTFWeaponBase* >( GetWeaponAssociatedWith() );
		if ( pWeapon )
		{
			// If the weapon isn't active, don't draw
			if ( pOwner && pOwner->GetActiveWeapon() != pWeapon )
			{
				return false;
			}

			if ( !IsViewModelWearable() )
			{
				// If it's the 3rd person wearable, don't draw it when the weapon is hidden
				if ( !pWeapon->ShouldDraw() )
				{
					return false;
				}
			}

			// If the weapon is being repurposed for a taunt dont draw.
			// The Brutal Legend taunt changes your weapon's model to be the guitar,
			// but we dont want things like bot-killer skulls or festive lights
			// to continue to draw
			if( pWeapon->IsBeingRepurposedForTaunt() )
			{
				return false;
			}
		}
		
		return BaseClass::ShouldDraw();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWearable::ShouldDrawParticleSystems( void )
{
	if ( !BaseClass::ShouldDrawParticleSystems() )
		return false;

	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	bool bStealthed = pPlayer->m_Shared.IsStealthed();

	// If we're disguised, this ought to only be getting called on disguise wearables,
	// otherwise we could get two particles showing at once (disguise wearable + real wearable).
	Assert( !pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) || IsDisguiseWearable() );

	if ( bStealthed )
	{
		return false;
	}

	if ( m_eParticleSystemVisibility == kParticleSystemVisibility_Undetermined )
	{
		static CSchemaItemDefHandle pItemDef_MapLoverHat( "World Traveler" );

		m_eParticleSystemVisibility = kParticleSystemVisibility_Shown;

		const CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem && pItem->GetStaticData() == pItemDef_MapLoverHat )
		{
			if ( MapInfo_DidPlayerDonate( pItem->GetAccountID(), engine->GetLevelName() ) == false )
			{
				m_eParticleSystemVisibility = kParticleSystemVisibility_Hidden;
			}
		}
	}

	return m_eParticleSystemVisibility == kParticleSystemVisibility_Shown;
}

int CTFWearable::GetWorldModelIndex( void )
{
	if ( m_nWorldModelIndex == 0 )
		return m_nModelIndex;

	static CSchemaItemDefHandle pItemDef_OculusRiftHeadset( "The TF2VRH" );
	const CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->GetStaticData() == pItemDef_OculusRiftHeadset )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->IsUsingVRHeadset() && pTFPlayer->GetPlayerClass() )
			{
				const char *pszReplacementModel = pItem->GetStaticData()->GetPlayerDisplayModelAlt( pTFPlayer->GetPlayerClass()->GetClassIndex() );
				if ( pszReplacementModel && pszReplacementModel[0] )
				{
					return modelinfo->GetModelIndex( pszReplacementModel );
				}
			}
		}
	}

	//*********************************************************************************
	// Parachute states
	static CSchemaItemDefHandle pItemDef_BaseJumper( "The B.A.S.E. Jumper" );
	const int iParachuteOpen = modelinfo->GetModelIndex( "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl" );
	const int iParachuteClosed = modelinfo->GetModelIndex( "models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl" );
	if ( m_nModelIndex == iParachuteOpen || m_nModelIndex == iParachuteClosed )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->m_Shared.InCond( TF_COND_PARACHUTE_ACTIVE ) )
			{
				return iParachuteOpen;
			}
			else
			{
				return iParachuteClosed;
			}
		}
	}

	if ( GameRules() )
	{
		const char *pBaseName = modelinfo->GetModelName( modelinfo->GetModel( m_nWorldModelIndex ) );
		const char *pTranslatedName = GameRules()->TranslateEffectForVisionFilter( "weapons", pBaseName );

		if ( pTranslatedName != pBaseName )
		{
			return modelinfo->GetModelIndex( pTranslatedName );
		}
	}

	return m_nWorldModelIndex;
}

void CTFWearable::ValidateModelIndex( void )
{
	m_nModelIndex = GetWorldModelIndex();

	BaseClass::ValidateModelIndex();
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Hides or shows masked bodygroups associated with this item.
//-----------------------------------------------------------------------------
bool CTFWearable::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
	if ( !pTFOwner )
		return false;

	bool bBaseUpdate = BaseClass::UpdateBodygroups( pOwner, iState );
	if ( bBaseUpdate && m_bDisguiseWearable )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem(); // Safe. Checked in base class call.

		CTFPlayer *pDisguiseTarget = pTFOwner->m_Shared.GetDisguiseTarget();
		if ( !pDisguiseTarget )
			return false;

		// Update our disguise bodygroup.
		int iDisguiseBody = pTFOwner->m_Shared.GetDisguiseBody();
		int iTeam = pTFOwner->m_Shared.GetDisguiseTeam();
		int iNumBodyGroups = pItem->GetStaticData()->GetNumModifiedBodyGroups( iTeam );
		for ( int i=0; i<iNumBodyGroups; ++i )
		{
			int iBody = 0;
			const char *pszBodyGroup = pItem->GetStaticData()->GetModifiedBodyGroup( iTeam, i, iBody );
			int iBodyGroup = pDisguiseTarget->FindBodygroupByName( pszBodyGroup );

			if ( iBodyGroup == -1 )
				continue;

			::SetBodygroup( pDisguiseTarget->GetModelPtr(), iDisguiseBody, iBodyGroup, iState );
		}

		pTFOwner->m_Shared.SetDisguiseBody( iDisguiseBody );
	}

	CEconItemView *pItem = GetAttributeContainer() ? GetAttributeContainer()->GetItem() : NULL;
	if ( pItem )
	{		
		int iTeam = pTFOwner->GetTeamNumber();
		int iNumBodyGroups = pItem->GetStaticData()->GetNumCodeControlledBodyGroups( iTeam );
		for ( int i=0; i<iNumBodyGroups; ++i )
		{
			codecontrolledbodygroupdata_t ccbgd = { NULL, NULL };
			const char *pszBodyGroup = pItem->GetStaticData()->GetCodeControlledBodyGroup( iTeam, i, ccbgd );
			int iBodyGroup = FindBodygroupByName( pszBodyGroup );
			if ( iBodyGroup != -1 )
			{
				SetBodygroup( iBodyGroup, CalcBodyGroup( pOwner, pItem, pszBodyGroup, ccbgd ) );
			}
		}
	}

	// Additional hidden bodygroups.
	for ( int i=0; i<m_HiddenBodyGroups.Count(); ++i )
	{
		int iBodyGroup = pOwner->FindBodygroupByName( m_HiddenBodyGroups[i] );
		if ( iBodyGroup == -1 )
			continue;
		pOwner->SetBodygroup( iBodyGroup, iState );
	}	

	return true;
}

int CTFWearable::GetSkin()
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( !pPlayer )
		return 0;

	int iTeamNumber = pPlayer->GetTeamNumber();

#if defined( CLIENT_DLL )
	// Run client-only "is the viewer on the same team as the wielder" logic. Assumed to
	// always be false on the server.
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return 0;

	int iLocalTeam = pLocalPlayer->GetTeamNumber();

	// We only show disguise weapon to the enemy team when owner is disguised
	bool bUseDisguiseWeapon = ( iTeamNumber != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

	if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		if ( pLocalPlayer != pPlayer )
		{
			iTeamNumber = pPlayer->m_Shared.GetDisguiseTeam();
		}
	}
#endif // defined( CLIENT_DLL )

	// See if the item wants to override the skin
	int nSkin = -1;

	CBaseCombatWeapon *pWeapon = assert_cast< CBaseCombatWeapon* >( GetWeaponAssociatedWith() );
	if ( pWeapon )
	{
		CEconItemView *pItem = pWeapon->GetAttributeContainer()->GetItem();
		if ( pItem->IsValid() )
		{
			nSkin = pItem->GetSkin( iTeamNumber );			// if we didn't have custom code, fall back to the item definition
		}
	}

	if ( nSkin != -1 )
	{
		return nSkin;
	}

	return BaseClass::GetSkin();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::InternalSetPlayerDisplayModel( void )
{
	// Set our model to the player model
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->IsValid() && pItem->GetStaticData() )
	{
		if ( pItem->GetStaticData()->IsContentStreamable() )
		{
			const char *pszPlayerDisplayModelAlt = pItem->GetStaticData()->GetPlayerDisplayModelAlt();
			if ( pszPlayerDisplayModelAlt && pszPlayerDisplayModelAlt[0] )
			{
				modelinfo->RegisterDynamicModel( pszPlayerDisplayModelAlt, IsClient() );
			}
		}
	}

	BaseClass::InternalSetPlayerDisplayModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::AddHiddenBodyGroup( const char* bodygroup )
{
	m_HiddenBodyGroups.AddToHead( bodygroup );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::ReapplyProvision( void )
{
	// Disguise wearables never provide
	if ( IsDisguiseWearable() )
	{
#ifdef GAME_DLL
		UpdateModelToClass();
#endif
		return;
	}

	BaseClass::ReapplyProvision();
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CTFWearable::Equip( CBasePlayer* pOwner )
{
	BaseClass::Equip( pOwner );

	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( !pTFPlayer )
		return;

	int iTeamNumber = pTFPlayer->GetTeamNumber();
	if ( m_bDisguiseWearable )
	{
		iTeamNumber = pTFPlayer->m_Shared.GetDisguiseTeam();
	}
	ChangeTeam( iTeamNumber );
	m_nSkin = ( iTeamNumber == (LAST_SHARED_TEAM+1) ) ? 0 : 1;

#ifdef CLIENT_DLL
	pTFPlayer->SetBodygroupsDirty();
#endif

#ifdef GAME_DLL
	// Reapply upgrades for wearables upon equip
	CEconItemView *pItem = ( (CTFWearable *)this )->GetAttributeContainer()->GetItem();
	if ( pTFPlayer && pItem->IsValid() )
	{
		pTFPlayer->ReapplyItemUpgrades( pItem );	
	}
#endif // GAME_DLL
}

//-----------------------------------------------------------------------------
// Purpose: Attaches the item to the player.
//-----------------------------------------------------------------------------
void CTFWearable::UnEquip( CBasePlayer* pOwner )
{
	BaseClass::UnEquip( pOwner );

#ifdef CLIENT_DLL
	CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
	if ( pTFPlayer )
	{
		pTFPlayer->SetBodygroupsDirty();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Check for any TF specific restrictions on item use.
//-----------------------------------------------------------------------------
bool CTFWearable::CanEquip( CBaseEntity *pOther )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && TFGameRules() )
	{
		CEconItemDefinition* pData = pItem->GetStaticData();
		if ( pData && pData->GetHolidayRestriction() )
		{
			int iHolidayRestriction = UTIL_GetHolidayForString( pData->GetHolidayRestriction() );
			if ( iHolidayRestriction != kHoliday_None && !TFGameRules()->IsHolidayActive( iHolidayRestriction ) )
				return false;
		}		
	}
	return true;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		ListenForGameEvent( "localplayer_changeteam" );

		m_nWorldModelIndex = m_nModelIndex;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWearable::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( Q_strcmp( pszEventName, "localplayer_changeteam" ) == 0 )
	{
		UpdateVisibility();
	}
}

#endif




//-----------------------------------------------------------------------------
// Purpose:
//		Choose shadow type for VM-wearables.
//-----------------------------------------------------------------------------
#if defined( CLIENT_DLL )
ShadowType_t CTFWearableVM::ShadowCastType( void )
{
	if ( ToTFPlayer(GetMoveParent())->ShouldDrawThisPlayer() )
	{
		// Using the viewmodel.
		return SHADOWS_NONE;
	}

	return SHADOWS_RENDER_TO_TEXTURE;
}
#endif


