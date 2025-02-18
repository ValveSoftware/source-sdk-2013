//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "econ_entity_creation.h"
#include "vgui/ILocalize.h"
#include "tier3/tier3.h"

#if defined( CLIENT_DLL )
#define UTIL_VarArgs VarArgs

#include "econ_item_inventory.h"
#include "model_types.h"
#include "eventlist.h"
#include "networkstringtable_clientdll.h"
#include "cdll_util.h"

#if defined(TF_CLIENT_DLL)
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "c_playerresource.h"
#include "tf_shareddefs.h"
#endif

#else // defined( CLIENT_DLL )

#include "activitylist.h"

#if defined(TF_DLL)
#include "tf_player.h"
#endif
#endif // defined( CLIENT_DLL )

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( EconEntity, DT_EconEntity )
IMPLEMENT_NETWORKCLASS_ALIASED( BaseAttributableItem, DT_BaseAttributableItem )

#if defined( CLIENT_DLL )
bool ParseItemKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue );
#endif

#if defined(_DEBUG)
extern ConVar item_debug;
extern ConVar item_debug_validation;
#endif

#if !defined( CLIENT_DLL )
	#define DEFINE_ECON_ENTITY_NETWORK_TABLE() \
		SendPropDataTable( SENDINFO_DT( m_AttributeManager ), &REFERENCE_SEND_TABLE(DT_AttributeContainer) ),
#else
	#define DEFINE_ECON_ENTITY_NETWORK_TABLE() \
		RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE(DT_AttributeContainer) ),
#endif // CLIENT_DLL

BEGIN_NETWORK_TABLE( CEconEntity , DT_EconEntity )
	DEFINE_ECON_ENTITY_NETWORK_TABLE()

#if defined(TF_DLL)
	SendPropBool( SENDINFO( m_bValidatedAttachedEntity ) ),
#elif defined(TF_CLIENT_DLL)
	RecvPropBool( RECVINFO( m_bValidatedAttachedEntity ) ),
#endif // TF_DLL || TF_CLIENT_DLL

END_NETWORK_TABLE()

BEGIN_DATADESC( CEconEntity )
END_DATADESC()

//
// Duplicating CEconEntity's network table and data description for backwards compat with demos.
// NOTE: NOTE_RENAMED_RECVTABLE() will not work with this class.
//
BEGIN_NETWORK_TABLE( CBaseAttributableItem, DT_BaseAttributableItem )
	DEFINE_ECON_ENTITY_NETWORK_TABLE()
END_NETWORK_TABLE()

BEGIN_DATADESC( CBaseAttributableItem )
END_DATADESC()

#ifdef GAME_DLL
BEGIN_ENT_SCRIPTDESC( CEconEntity, CBaseAnimating, "Econ Entity" )
	DEFINE_SCRIPTFUNC( AddAttribute, "Add an attribute to the entity" )
	DEFINE_SCRIPTFUNC( RemoveAttribute, "Remove an attribute to the entity" )
	DEFINE_SCRIPTFUNC( ReapplyProvision, "Flush any attribute changes we provide onto our owner" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetAttribute, "GetAttribute", "Get an attribute float from the entity" )
END_SCRIPTDESC();
#endif

#ifdef TF_CLIENT_DLL
extern ConVar cl_flipviewmodels;
#endif


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DrawEconEntityAttachedModels( CBaseAnimating *pEnt, CEconEntity *pAttachedModelSource, const ClientModelRenderInfo_t *pInfo, int iMatchDisplayFlags )
{
#ifndef DOTA_DLL
	if ( !pEnt || !pAttachedModelSource || !pInfo )
		return;

	// This flag says we should turn off the material overrides for attachments. 
	IMaterial* pMaterialOverride = NULL;
	OverrideType_t nMaterialOverrideType = OVERRIDE_NORMAL;

	if ( ( pInfo->flags & STUDIO_NO_OVERRIDE_FOR_ATTACH ) != 0 )
	{
		modelrender->GetMaterialOverride( &pMaterialOverride, &nMaterialOverrideType );
		modelrender->ForcedMaterialOverride( NULL, nMaterialOverrideType );
	}

	// Draw our attached models as well
	for ( int i = 0; i < pAttachedModelSource->m_vecAttachedModels.Size(); i++ )
	{
		const AttachedModelData_t& attachedModel = pAttachedModelSource->m_vecAttachedModels[i];

		if ( attachedModel.m_pModel && (attachedModel.m_iModelDisplayFlags & iMatchDisplayFlags) )
		{
			ClientModelRenderInfo_t infoAttached = *pInfo;
			
			infoAttached.pRenderable	= pEnt;
			infoAttached.instance		= MODEL_INSTANCE_INVALID;
			infoAttached.entity_index	= pEnt->index;
			infoAttached.pModel			= attachedModel.m_pModel;

			infoAttached.pModelToWorld  = &infoAttached.modelToWorld;

			// Turns the origin + angles into a matrix
			AngleMatrix( infoAttached.angles, infoAttached.origin, infoAttached.modelToWorld );

			DrawModelState_t state;
			matrix3x4_t *pBoneToWorld;
			bool bMarkAsDrawn = modelrender->DrawModelSetup( infoAttached, &state, NULL, &pBoneToWorld );
			pEnt->DoInternalDrawModel( &infoAttached, ( bMarkAsDrawn && ( infoAttached.flags & STUDIO_RENDER ) ) ? &state : NULL, pBoneToWorld );
		}
	}

	if ( pMaterialOverride != NULL )
		modelrender->ForcedMaterialOverride( pMaterialOverride, nMaterialOverrideType );
#endif
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconEntity::CEconEntity()
{
	m_pAttributes = this;

	// Inform base entity system that we can deal with dynamic models
	EnableDynamicModels();
#ifdef GAME_DLL
	m_iOldOwnerClass = 0;
#endif

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	m_bValidatedAttachedEntity = false;
#endif // TF_DLL || TF_CLIENT_DLL

#ifdef CLIENT_DLL
	m_flFlexDelayTime = 0.0f;
	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;
	m_iNumOwnerValidationRetries = 0;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconEntity::~CEconEntity()
{
#ifdef CLIENT_DLL
	SetParticleSystemsVisible( PARTICLE_SYSTEM_STATE_NOT_VISIBLE );
	delete [] m_flFlexDelayedWeight;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CStudioHdr * CEconEntity::OnNewModel()
{
	CStudioHdr* hdr = BaseClass::OnNewModel();

#ifdef GAME_DLL
	// Adjust class-specific bodygroup after model load if we have a model and a class
	if ( hdr && m_iOldOwnerClass > 0 )
	{
		CEconItemView *pItem = GetAttributeContainer()->GetItem();
		if ( pItem && pItem->IsValid() && pItem->GetStaticData()->UsesPerClassBodygroups( GetTeamNumber() ) )
		{
			// Classes start at 1, bodygroups at 0
			SetBodygroup( 1, m_iOldOwnerClass - 1 );
		}
	}
#endif

#ifdef TF_CLIENT_DLL
	m_bValidatedOwner = false; // require item validation to re-run

	// If we're carried by a player, let him know he should recalc his bodygroups.
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		pPlayer->SetBodygroupsDirty();
	}

	// allocate room for delayed flex weights
	delete [] m_flFlexDelayedWeight;
	m_flFlexDelayedWeight = NULL;
	m_cFlexDelayedWeight = 0;
	if ( hdr && hdr->numflexcontrollers() )
	{
		m_cFlexDelayedWeight = hdr->numflexcontrollers();
		m_flFlexDelayedWeight = new float[ m_cFlexDelayedWeight ];
		memset( m_flFlexDelayedWeight, 0, sizeof( float ) * m_cFlexDelayedWeight );

		C_BaseFlex::LinkToGlobalFlexControllers( hdr );
	}
#endif // TF_CLIENT_DLL

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::InitializeAttributes( void )
{
	m_AttributeManager.InitializeAttributes( this );
	m_AttributeManager.SetProviderType( PROVIDER_WEAPON );

#ifdef CLIENT_DLL
	// Check particle systems
	CUtlVector<const attachedparticlesystem_t *> vecParticles;
	GetEconParticleSystems( &vecParticles );
	m_bHasParticleSystems = vecParticles.Count() > 0;

	if ( !m_bClientside )
		return;
#else
	m_AttributeManager.GetItem()->InitNetworkedDynamicAttributesForDemos();
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::DebugDescribe( void )
{
	CEconItemView *pScriptItem = GetAttributeContainer()->GetItem();

	Msg("============================================\n");
	char tempstr[1024];
// FIXME:	ILocalize::ConvertUnicodeToANSI( pScriptItem->GetItemName(), tempstr, sizeof(tempstr) );
	const char *pszQualityString = EconQuality_GetQualityString( (EEconItemQuality)pScriptItem->GetItemQuality() );
	Msg("%s \"%s\" (level %d)\n", pszQualityString ? pszQualityString : "[unknown]", tempstr, pScriptItem->GetItemLevel() );
	// FIXME: ILocalize::ConvertUnicodeToANSI( pScriptItem->GetAttributeDescription(), tempstr, sizeof(tempstr) );
	Msg("%s", tempstr );
	Msg("\n============================================\n");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateOnRemove( void )
{
	SetOwnerEntity( NULL );
	ReapplyProvision();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::ReapplyProvision( void )
{
#ifdef GAME_DLL
	UpdateModelToClass();
#endif

	CBaseEntity *pNewOwner = GetOwnerEntity();
	if ( pNewOwner == m_hOldProvidee.Get() )
		return;

	// Remove ourselves from the old providee's list
	if ( m_hOldProvidee.Get() )
	{
		GetAttributeManager()->StopProvidingTo( m_hOldProvidee.Get() );
	}

	// Add ourselves to our new owner's provider list
	if ( pNewOwner )
	{
		GetAttributeManager()->ProvideTo( pNewOwner );
	}

	m_hOldProvidee = pNewOwner;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CEconEntity::ScriptGetAttribute( const char *pName, float flFallbackValue )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem )
	{
		CEconItemAttributeDefinition *pDef = GetItemSchema()->GetAttributeDefinitionByName( pName );
		if ( pDef )
		{
			CEconGetAttributeIterator it( pDef->GetDefinitionIndex(), flFallbackValue );
			pItem->IterateAttributes( &it );
			return it.m_flValue;
		}
	}

	return flFallbackValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Activity CEconEntity::TranslateViewmodelHandActivity( Activity actBase )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->IsValid() )
	{
		GameItemDefinition_t *pStaticData = pItem->GetStaticData();
		if ( pStaticData && pStaticData->ShouldAttachToHands() )
		{
			return TranslateViewmodelHandActivityInternal(actBase);
		}
	}

	return actBase;
}

#if !defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnOwnerClassChange( void )
{
#ifdef TF_DLL
	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() != m_iOldOwnerClass )
	{
		UpdateModelToClass();
	}
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEconEntity::CalculateVisibleClassFor( CBaseCombatCharacter *pPlayer )
{
#ifdef TF_DLL
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
	return (pTFPlayer ? pTFPlayer->GetPlayerClass()->GetClassIndex() : 0);
#else
	return 0;
#endif
}
#endif

//-----------------------------------------------------------------------------
// Purpose: double duty function - sets up model for current player class, and
// also sets bodygroups if the correct model is fully loaded.
//-----------------------------------------------------------------------------
void CEconEntity::UpdateModelToClass( void )
{
#ifdef TF_DLL
	MDLCACHE_CRITICAL_SECTION();

	CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	m_iOldOwnerClass = CalculateVisibleClassFor( pPlayer );
	if ( !pPlayer )
		return;

	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( !pItem->IsValid() )
		return;

	const char *pszModel = NULL;

	// If we attach to hands, we need to use the hand models
	if ( pItem->GetStaticData()->ShouldAttachToHands() )
	{
		pszModel = pPlayer->GetPlayerClass()->GetHandModelName( 0 );
	}
	else
	{
		int nTeam = pPlayer->GetTeamNumber();
		CTFWearable *pWearable = dynamic_cast< CTFWearable*>( this );
		if ( pWearable && pWearable->IsDisguiseWearable() )
		{
			nTeam = pPlayer->m_Shared.GetDisguiseTeam();
		}

		pszModel = pItem->GetPlayerDisplayModel( m_iOldOwnerClass, nTeam );
	}
	if ( pszModel && pszModel[0] )
	{
		if ( V_stricmp( STRING( GetModelName() ), pszModel ) != 0 )
		{
			if ( pItem->GetStaticData()->IsContentStreamable() )
			{
				modelinfo->RegisterDynamicModel( pszModel, IsClient() );

				const char *pszModelAlt = pItem->GetStaticData()->GetPlayerDisplayModelAlt( m_iOldOwnerClass );
				if ( pszModelAlt && pszModelAlt[0] )
				{
					modelinfo->RegisterDynamicModel( pszModelAlt, IsClient() );
				}

				if ( pItem->GetVisionFilteredDisplayModel() && pItem->GetVisionFilteredDisplayModel()[ 0 ] != '\0' )
				{
					modelinfo->RegisterDynamicModel( pItem->GetVisionFilteredDisplayModel(), IsClient() );
				}
			}

			SetModel( pszModel );
		}
	}

	if ( GetModelPtr() && pItem->GetStaticData()->UsesPerClassBodygroups( GetTeamNumber() ) )
	{
		// Classes start at 1, bodygroups at 0, so we shift them all back 1.
		SetBodygroup( 1, (m_iOldOwnerClass-1) );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::PlayAnimForPlaybackEvent( wearableanimplayback_t iPlayback )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( !pItem->IsValid() || !GetOwnerEntity() )
		return;

	int iTeamNum = GetOwnerEntity()->GetTeamNumber();
	int iActivities = pItem->GetStaticData()->GetNumPlaybackActivities( iTeamNum );
	for ( int i = 0; i < iActivities; i++ )
	{
		activity_on_wearable_t *pData = pItem->GetStaticData()->GetPlaybackActivityData( iTeamNum, i );
		if ( pData && pData->iPlayback == iPlayback && pData->pszActivity )
		{
			// If this is the first time we've tried to use it, find the activity
			if ( pData->iActivity == kActivityLookup_Unknown )
			{
				pData->iActivity = ActivityList_IndexForName( pData->pszActivity );
			}

			int sequence = SelectWeightedSequence( (Activity)pData->iActivity ); 
			if ( sequence != ACTIVITY_NOT_AVAILABLE )
			{
				ResetSequence( sequence );
				SetCycle( 0 );

#if !defined( CLIENT_DLL )
				if ( IsUsingClientSideAnimation() )
				{
					ResetClientsideFrame();
				}
#endif
			}
			return;
		}
	}
}

#endif // !CLIENT_DLL

#if defined( TF_CLIENT_DLL )
// It's okay to draw attached entities with these models.
const char* g_modelWhiteList[] =
{
	"models/weapons/w_models/w_toolbox.mdl",
	"models/weapons/w_models/w_sapper.mdl",

	// Canteens can change model based on the powerup type... all of these alternates are ok!
	"models/player/items/mvm_loot/all_class/mvm_flask_krit.mdl",
	"models/player/items/mvm_loot/all_class/mvm_flask_uber.mdl",
	"models/player/items/mvm_loot/all_class/mvm_flask_tele.mdl",
	"models/player/items/mvm_loot/all_class/mvm_flask_ammo.mdl",
	"models/player/items/mvm_loot/all_class/mvm_flask_build.mdl",

	TF_WEAPON_TAUNT_FRONTIER_JUSTICE_GUITAR_MODEL,

	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack.mdl",
	"models/workshop/weapons/c_models/c_paratooper_pack/c_paratrooper_pack_open.mdl",

};

#define HALLOWEEN_KART_MODEL	"models/player/items/taunts/bumpercar/parts/bumpercar.mdl"
#define HALLOWEEN_KART_CAGE_MODEL	"models/props_halloween/bumpercar_cage.mdl"
#endif

#if defined( CLIENT_DLL )
//-----------------------------------------------------------------------------
// Purpose: TF prevents drawing of any entity attached to players that aren't items in the inventory of the player.
//			This is to prevent servers creating fake cosmetic items and attaching them to players.
//-----------------------------------------------------------------------------
bool CEconEntity::ValidateEntityAttachedToPlayer( bool &bShouldRetry )
{
	bShouldRetry = false;

	// We only use this variable in debug or on the client.
#if defined( _DEBUG ) || defined( TF_CLIENT_DLL )
	bool bItemDebugValidation = false;
#endif // defined( _DEBUG ) || defined( TF_CLIENT_DLL )

#ifdef _DEBUG
	bItemDebugValidation = item_debug_validation.GetBool();

	// Always valid in debug if item_debug_validation is disabled
	if ( !bItemDebugValidation )
		return true;
#endif // _DEBUG

#if defined( TF_CLIENT_DLL )
	// Always valid in item testing mode
	if ( TFGameRules()->IsInItemTestingMode() )
		return true;

	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	// If we're not carried by a player, we're not valid. This prevents them
	// parenting hats to ents that they then parent to the player.
	if ( !pOwner )
	{
		//Msg( "NO OWNER SET! %i\n", m_iNumOwnerValidationRetries );
		bShouldRetry = ( m_iNumOwnerValidationRetries < 500 );
		m_iNumOwnerValidationRetries++;
		return false;
	}

	C_BaseEntity *pVM = pOwner->GetViewModel();

	// The owner entity must also be a move parent of this entity.
	bool bPlayerIsParented = false;
	C_BaseEntity *pEntity = this;
	while ( (pEntity = pEntity->GetMoveParent()) != NULL )
	{
		if ( pOwner == pEntity || pVM == pEntity )
		{
			bPlayerIsParented = true;
			break;
		}
	}

	if ( !bPlayerIsParented )
	{
		//Msg( "NOT PARENTED! %i\n", m_iNumOwnerValidationRetries );
		bShouldRetry = ( m_iNumOwnerValidationRetries < 500 );
		m_iNumOwnerValidationRetries++;
		return false;
	}

	m_iNumOwnerValidationRetries = 0;

	// We only need this in debug (for item_debug_validation) or PvE mode
	bool bOwnerIsBot = pOwner->IsABot(); // THIS IS INSECURE -- DO NOT USE THIS OUTSIDE OF DEBUG OR PVE MODE

	// Allow bots to use anything in PvE mode
	if ( bOwnerIsBot && TFGameRules()->IsPVEModeActive() )
		return true;

	int iClass = pOwner->GetPlayerClass()->GetClassIndex();
	int iTeam = pOwner->GetTeamNumber();

	// Allow all weapons parented to the local player
	if ( pOwner == C_BasePlayer::GetLocalPlayer() )
	{
		// They can change the owner entity, so we have to keep checking.
		bShouldRetry = true;
		return true;
	}

	// HACK: For now, if our owner is a disguised spy, we assume everything is valid.
	if ( (pOwner->m_Shared.InCond( TF_COND_DISGUISED ) || pOwner->m_Shared.InCond( TF_COND_DISGUISING )) && iClass == TF_CLASS_SPY )
	{
		bShouldRetry = true; // Keep checking in case the player switches class or becomes no longer disguised
		return true;
	}

	// If our owner is a disguised spy, we validate everything based 
	// on the items carried by the person we're disguised as.
	/*if ( pOwner->m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		// This won't work. If our disguise target is a player we've never seen before,
		//		 we won't have a client entity, and hence we don't have their inventory.
		C_TFPlayer *pDisguiseTarget = pOwner->m_Shared.GetDisguiseTarget();
		if ( pDisguiseTarget && pDisguiseTarget != pOwner )
		{
			pOwner = pDisguiseTarget;
			iClass = pOwner->GetPlayerClass()->GetClassIndex();
		}
		else
		{
			// We're not disguised as a specific player. Make sure we lookup base weapons with the disguise class.
			iClass = pOwner->m_Shared.GetDisguiseClass();
		}
	}
	*/

#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	if ( m_bValidatedAttachedEntity )
		return true;
#endif // TF_DLL || TF_CLIENT_DLL

	const char *pszClientModel = modelinfo->GetModelName( GetModel() );
	if ( pszClientModel && g_modelWhiteList[0] )
	{
		// Certain builder models are okay to have.
		for ( int i=0; i<ARRAYSIZE( g_modelWhiteList ); ++i )
		{
			if ( FStrEq( pszClientModel, g_modelWhiteList[i] ) )
				return true;
		}
	}

	// Halloween Karts
	if ( pOwner->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
	{
		if ( FStrEq( pszClientModel, HALLOWEEN_KART_MODEL ) )
			return true;

		if ( FStrEq( pszClientModel, HALLOWEEN_KART_CAGE_MODEL ) )
			return true;
	}

	// If our player doesn't have an inventory, we're not valid. 
	CTFPlayerInventory *pInv = pOwner->Inventory();
	if ( !pInv )
		return false;

	// check if this is a custom taunt prop
	if ( pOwner->m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		const char* pszCustomTauntProp = NULL;
		int iClassTaunt = pOwner->GetPlayerClass()->GetClassIndex();
		CEconItemView *pMiscItemView = pInv->GetItemInLoadout( iClassTaunt, pOwner->GetActiveTauntSlot() );
		if ( pMiscItemView && pMiscItemView->IsValid() )
		{
			if ( pMiscItemView->GetStaticData()->GetTauntData() )
			{
				pszCustomTauntProp = pMiscItemView->GetStaticData()->GetTauntData()->GetProp( iClassTaunt );
				if ( pszCustomTauntProp )
				{
					return true;
				}
			}
		}
	}

	// If we've lost connection to the GC, let's just trust the server to avoid breaking the appearance for everyone.
	bool bSkipInventoryCheck = bItemDebugValidation && bOwnerIsBot; // will always be false in release builds
	if ( ( !pInv->GetSOC() || !pInv->GetSOC()->BIsInitialized() ) && !bSkipInventoryCheck )
	{
		bShouldRetry = true;
		return true;
	}

	CEconItemView *pScriptItem = GetAttributeContainer()->GetItem();

	// If the item isn't valid, we're probably an extra wearable for another item. See if our model is 
	// a model specified as the extra wearable for any of the items we have equipped.
	if ( !pScriptItem->IsValid() )
	{
		// Uninitialized client models return their model as '?'
		if ( pszClientModel && pszClientModel[0] != '?' )
		{
			CSteamID steamIDForPlayer;
			pOwner->GetSteamID( &steamIDForPlayer );

			for ( int i = 0; i < CLASS_LOADOUT_POSITION_COUNT; i++ )
			{
				CEconItemView *pItem = TFInventoryManager()->GetItemInLoadoutForClass( iClass, i, &steamIDForPlayer );
				if ( pItem && pItem->IsValid() )
				{
					const char *pszAttached = pItem->GetExtraWearableModel();
					if ( pszAttached && pszAttached[0] )
					{
						if ( FStrEq( pszClientModel, pszAttached ) )
							return true;
					}

					pszAttached = pItem->GetExtraWearableViewModel();
					if ( pszAttached && pszAttached[0] )
					{
						if ( FStrEq( pszClientModel, pszAttached ) )
							return true;
					}
				}
			}
		}
		else if ( pszClientModel && pszClientModel[0] == '?' )
		{
			bShouldRetry = true;
		}

		return false;
	}

	// Skip this check for bots if item_debug_validation is enabled.
	if ( !pInv->GetInventoryItemByItemID( pScriptItem->GetItemID() ) && !bSkipInventoryCheck )
	{
		// If it's a base item, we allow it.
		CEconItemView *pBaseItem = TFInventoryManager()->GetBaseItemForClass( iClass, pScriptItem->GetStaticData()->GetLoadoutSlot(iClass) );
		if ( *pScriptItem != *pBaseItem )
		{
			const wchar_t *pwzItemName = pScriptItem->GetItemName();

			char szItemName[ MAX_ITEM_NAME_LENGTH ];
			ILocalize::ConvertUnicodeToANSI( pwzItemName, szItemName, sizeof( szItemName ) );

#ifdef _DEBUG
			Warning("Item '%s' attached to %s, but it's not in his inventory.\n", szItemName, pOwner->GetPlayerName() );
#endif
			return false;
		}
	}

	// Our model has to match the model in our script
	const char *pszScriptModel = pScriptItem->GetWorldDisplayModel();
	if ( !pszScriptModel )
	{
		pszScriptModel = pScriptItem->GetPlayerDisplayModel( iClass, iTeam );
	}

	if ( pszClientModel && pszClientModel[0] && pszClientModel[0] != '?' )
	{
		// A model was set on the entity, let's make sure it matches the model in the script.
		if ( !pszScriptModel || !pszScriptModel[0] )
			return false;

		if ( FStrEq( pszClientModel, pszScriptModel ) == false )
		{
			// The regular model didn't work...let's try the Alt version if it exists
			const char *pszScriptModelAlt = pScriptItem->GetStaticData()->GetPlayerDisplayModelAlt( iClass );
			if ( !pszScriptModelAlt || !pszScriptModelAlt[0] || ( FStrEq( pszClientModel, pszScriptModelAlt ) == false ) )
			{
				// The Alt model didn't work... let's try the vision filtered display model if it happens to exist
				pszScriptModel = pScriptItem->GetVisionFilteredDisplayModel();
				if ( !pszScriptModel || !pszScriptModel[0] )
					return false;

				if ( FStrEq( pszClientModel, pszScriptModel ) == false )
				{
					return false;
				}
			}
		}
	}
	else
	{
		// The client model was not set, so check that there isn't a model set in the script either.
		if ( pszScriptModel && pszScriptModel[0] )
		{
			if ( pszClientModel[0] == '?' )
				bShouldRetry = true;

			return false;
		}
	}

	return true;
#else
	return false;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Set a material override for this entity via code
//-----------------------------------------------------------------------------
void CEconEntity::SetMaterialOverride( int team, const char *pszMaterial )
{
	if ( team >= 0 && team < TEAM_VISUAL_SECTIONS )
	{
		m_MaterialOverrides[ team ].Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
}


//-----------------------------------------------------------------------------
void CEconEntity::SetMaterialOverride( int team, CMaterialReference &ref )
{
	if ( team >= 0 && team < TEAM_VISUAL_SECTIONS )
	{
		m_MaterialOverrides[ team ].Init( ref );
	}
}


// Deal with recording
void CEconEntity::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );

	bool bUseOverride = ( GetTeamNumber() >= 0 && GetTeamNumber() < TEAM_VISUAL_SECTIONS ) && m_MaterialOverrides[ GetTeamNumber() ].IsValid();
	if ( bUseOverride )
	{
		msg->SetString( "materialOverride", m_MaterialOverrides[ GetTeamNumber() ]->GetName() );
	}
#endif
}


#ifndef DOTA_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ViewmodelAttachmentModel::SetOuter( CEconEntity *pOuter )
{
	m_hOuter = pOuter;
	SetOwnerEntity( pOuter );

	CEconItemView *pItem = pOuter->GetAttributeContainer()->GetItem();
	if ( pItem->IsValid() )
	{
		m_bAlwaysFlip = pItem->GetStaticData()->ShouldFlipViewmodels();
	}
}

bool C_ViewmodelAttachmentModel::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	if ( !BaseClass::InitializeAsClientEntity( pszModelName, renderGroup ) )
		return false;

	AddEffects( EF_BONEMERGE );
	AddEffects( EF_BONEMERGE_FASTCULL );

	// Invisible by default, and made visible->drawn->made invisible when the viewmodel is drawn
	AddEffects( EF_NODRAW );
	return true;
}

int C_ViewmodelAttachmentModel::InternalDrawModel( int flags )
{
#ifdef TF_CLIENT_DLL
	CMatRenderContextPtr pRenderContext( materials );
	if ( cl_flipviewmodels.GetBool() != m_bAlwaysFlip )
	{
		pRenderContext->CullMode( MATERIAL_CULLMODE_CW );
	}
#endif
	int r = BaseClass::InternalDrawModel( flags );

#ifdef TF_CLIENT_DLL
	pRenderContext->CullMode( MATERIAL_CULLMODE_CCW );
#endif

	return r;
}

bool C_ViewmodelAttachmentModel::OnPostInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnPostInternalDrawModel( pInfo ) )
		return false;

	if ( !m_hOuter )
		return true;
	if ( !m_hOuter->GetAttributeContainer() )
		return true;
	if ( !m_hOuter->GetAttributeContainer()->GetItem() )
		return true;

	DrawEconEntityAttachedModels( this, GetOuter(), pInfo, kAttachedModelDisplayFlag_ViewModel );
	return true;
}

void C_ViewmodelAttachmentModel::StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask )
{
	BaseClass::StandardBlendingRules( hdr, pos, q, currentTime, boneMask );

	// Will it blend?

	if ( !m_hOuter )
		return;

	m_hOuter->ViewModelAttachmentBlending( hdr, pos, q, currentTime, boneMask );
}

void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );
void C_ViewmodelAttachmentModel::FormatViewModelAttachment( int nAttachment, matrix3x4_t &attachmentToWorld )
{
	Vector vecOrigin;
	MatrixPosition( attachmentToWorld, vecOrigin );
	::FormatViewModelAttachment( vecOrigin, false );
	PositionMatrix( vecOrigin, attachmentToWorld );
}
int C_ViewmodelAttachmentModel::GetSkin( void )
{
	if ( m_hOuter != NULL )
	{
		CBaseCombatWeapon *pWeapon = m_hOuter->MyCombatWeaponPointer();

		if ( pWeapon )
		{
			int nSkin = pWeapon->GetSkinOverride();
			if ( nSkin != -1 )
			{
				return nSkin;
			}
		}
		else
		{
			// some models like the Festive Targe don't have combat pointers but they still need to get the correct skin
			if ( m_hOuter->GetAttributeContainer() )
			{
				CEconItemView *pItem = m_hOuter->GetAttributeContainer()->GetItem();
				if ( pItem && pItem->IsValid() && GetOwnerViaInterface() )
				{
					return pItem->GetSkin( GetOwnerViaInterface()->GetTeamNumber(), true );
				}
			}
		}
	}

	return BaseClass::GetSkin();
}

#endif // !defined( DOTA_DLL )

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::Release( void )
{
#ifdef CLIENT_DLL
	SetParticleSystemsVisible( PARTICLE_SYSTEM_STATE_NOT_VISIBLE );

	// Remove all effects associated with this econ entity, not just turn them off
	C_BaseEntity *pEffectOwnerWM = this;
	C_BaseEntity *pEffectOwnerVM = NULL;

	bool bExtraWearable = false;
	bool bExtraWearableVM = false;

	CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( this );
	if ( pWeapon )
	{
		pEffectOwnerVM = pWeapon->GetPlayerOwner() ? pWeapon->GetPlayerOwner()->GetViewModel() : NULL;
		if ( pWeapon->m_hExtraWearable.Get() )
		{
			pEffectOwnerWM = pWeapon->m_hExtraWearable.Get();
			bExtraWearable = true;
		}

		if ( pWeapon->m_hExtraWearableViewModel.Get() )
		{
			pEffectOwnerVM = pWeapon->m_hExtraWearableViewModel.Get();
			bExtraWearableVM = true;
		}
		// always kill all effects for VM
		if ( pEffectOwnerVM )
		{
			pEffectOwnerVM->ParticleProp()->StopEmission( NULL, false, true );
		}
	}

	pEffectOwnerWM->ParticleProp()->StopEmission( NULL, false, true );

#endif
	if ( m_hViewmodelAttachment )
	{
		m_hViewmodelAttachment->Release();
	}

	BaseClass::Release();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::SetDormant( bool bDormant )
{
	// If I'm burning, stop the burning sounds
	if ( !IsDormant() && bDormant && m_nParticleSystemsCreated != PARTICLE_SYSTEM_STATE_NOT_VISIBLE )
	{
		SetParticleSystemsVisible( PARTICLE_SYSTEM_STATE_NOT_VISIBLE );
	}

	BaseClass::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_iOldTeam = m_iTeamNum;
}

IMaterial *CreateTempMaterialForPlayerLogo( int iPlayerIndex, player_info_t *info, char *texname, int nchars );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::OnDataChanged( DataUpdateType_t updateType )
{
	// If we were just created, setup from the script files
	if ( updateType == DATA_UPDATE_CREATED )
	{
		InitializeAttributes();
		m_nParticleSystemsCreated = PARTICLE_SYSTEM_STATE_NOT_VISIBLE;
		m_bAttachmentDirty = true;
	}

	BaseClass::OnDataChanged( updateType );

	GetAttributeContainer()->OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CEconItemView *pItem = m_AttributeManager.GetItem();

#if defined(_DEBUG)
		if ( item_debug.GetBool() )
		{
			DebugDescribe();
		}
#endif

		// if we have paintkit material override, stomp all material override
		const char *pszPaintKitMaterialOverride = GetPaintKitMaterialOverride( pItem );

		// Find & cache for easy leaf code usage
		for ( int team = 0; team < TEAM_VISUAL_SECTIONS; team++ )
		{
			const char *pszMaterial = pszPaintKitMaterialOverride ? pszPaintKitMaterialOverride : pItem->GetStaticData()->GetMaterialOverride( team );
			if ( pszMaterial )
			{
				m_MaterialOverrides[team].Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
			}
		}

#ifdef TF_CLIENT_DLL
		// If we're carried by a player, let him know he should recalc his bodygroups.
		C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
		if ( pPlayer )
		{
			pPlayer->SetBodygroupsDirty();
		}

		//Warning("Forcing recalc of visiblity for %d\n", entindex());
		m_bValidatedOwner = false;
		m_iNumOwnerValidationRetries = 0;
		UpdateVisibility();
#endif // TF_CLIENT_DLL
	}

	UpdateAttachmentModels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateAttachmentModels( void )
{
#ifndef DOTA_DLL
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	GameItemDefinition_t *pItemDef = pItem && pItem->IsValid() ? pItem->GetStaticData() : NULL;

	// Update the state of additional model attachments
	m_vecAttachedModels.Purge();
	if ( pItemDef && AttachmentModelsShouldBeVisible() )
	{
		int iTeamNumber = GetTeamNumber();
		{
			int iAttachedModels = pItemDef->GetNumAttachedModels( iTeamNumber );
			for ( int i = 0; i < iAttachedModels; i++ )
			{
				attachedmodel_t	*pModel = pItemDef->GetAttachedModelData( iTeamNumber, i );

				int iModelIndex = modelinfo->GetModelIndex( pModel->m_pszModelName );
				if ( iModelIndex >= 0 )
				{
					AttachedModelData_t attachedModelData;
					attachedModelData.m_pModel			   = modelinfo->GetModel( iModelIndex );
					attachedModelData.m_iModelDisplayFlags = pModel->m_iModelDisplayFlags;
					m_vecAttachedModels.AddToTail( attachedModelData );
				}
			}
		}

		// Check for Festive attachedmodels for festivized weapons
		{
			int iAttachedModels = pItemDef->GetNumAttachedModelsFestivized( iTeamNumber );
			if ( iAttachedModels )
			{
				int iFestivized = 0;
				CALL_ATTRIB_HOOK_INT( iFestivized, is_festivized );
				if ( iFestivized )
				{
					for ( int i = 0; i < iAttachedModels; i++ )
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

	// Update the state of attachment models for this item
 	bool bItemNeedsAttachment = pItemDef && (pItemDef->ShouldAttachToHands() || pItemDef->ShouldAttachToHandsVMOnly());
	if ( bItemNeedsAttachment )
	{
		bool bShouldShowAttachment = false;
		CBasePlayer *pOwner = ToBasePlayer( GetOwnerEntity() );
		if ( pOwner && !pOwner->ShouldDrawThisPlayer() )
		{
			// Drawing the viewmodel
			bShouldShowAttachment = true;
		}

		if ( bShouldShowAttachment && AttachmentModelsShouldBeVisible() )
		{
			if ( !m_hViewmodelAttachment )
			{
				C_BaseViewModel *vm = pOwner->GetViewModel( 0 );
				if ( vm )
				{
					C_ViewmodelAttachmentModel *pEnt = new class C_ViewmodelAttachmentModel;
					if ( !pEnt )
						return;

					pEnt->SetOuter( this );

					int iClass = 0;
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
					CTFPlayer *pTFPlayer = ToTFPlayer( pOwner );
					if ( pTFPlayer )
					{
						iClass = pTFPlayer->GetPlayerClass()->GetClassIndex();
					}
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )
					if ( pEnt->InitializeAsClientEntity( pItem->GetPlayerDisplayModel( iClass, pOwner->GetTeamNumber() ), RENDER_GROUP_VIEW_MODEL_OPAQUE ) == false )
						return;

					m_hViewmodelAttachment = pEnt;
					m_hViewmodelAttachment->SetParent( vm );
					m_hViewmodelAttachment->SetLocalOrigin( vec3_origin );
					m_hViewmodelAttachment->UpdatePartitionListEntry();
					m_hViewmodelAttachment->CollisionProp()->UpdatePartition();
					m_hViewmodelAttachment->UpdateVisibility();

					m_bAttachmentDirty = true;
				}
			}
			else if ( m_hViewmodelAttachment )
			{
				// If a player changes team, we may need to update the skin on the attachment weapon model
				if ( m_iOldTeam != m_iTeamNum )
				{
					m_bAttachmentDirty = true;
				}
			}

			// We can't pull data from the viewmodel until we're actually the active weapon.
			if ( m_bAttachmentDirty && m_hViewmodelAttachment )
			{
				pOwner = ToBasePlayer( GetOwnerEntity() );
				C_BaseViewModel *vm = pOwner->GetViewModel( 0 );
				if ( vm && vm->GetWeapon() == this )
				{
					m_hViewmodelAttachment->m_nSkin = vm->GetSkin();
					m_bAttachmentDirty = false;
				}
			}
			return;
		}
	}

	// If we get here we shouldn't have an attachment.
	if ( m_hViewmodelAttachment )
	{
		m_hViewmodelAttachment->Release();
	}

#endif // !defined( DOTA_DLL )
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::HasCustomParticleSystems( void ) const
{
	return m_bHasParticleSystems;
}

//-----------------------------------------------------------------------------
// Purpose: Create / Destroy particle systems on this item as appropriate
//-----------------------------------------------------------------------------
void CEconEntity::UpdateParticleSystems( void )
{
	if ( !HasCustomParticleSystems() )
		return;

	ParticleSystemState_t nVisible = PARTICLE_SYSTEM_STATE_NOT_VISIBLE;
	if ( IsEffectActive( EF_NODRAW ) || !ShouldDraw() )
	{
		nVisible = PARTICLE_SYSTEM_STATE_NOT_VISIBLE;
	}
	else if ( !GetOwnerEntity() && !IsDormant() )
	{
		nVisible = PARTICLE_SYSTEM_STATE_VISIBLE;
	}
	else if ( GetOwnerEntity() && !GetOwnerEntity()->IsDormant() && GetOwnerEntity()->IsPlayer() && GetOwnerEntity()->IsAlive() )
	{
		nVisible = PARTICLE_SYSTEM_STATE_VISIBLE;
	}

	if ( nVisible == PARTICLE_SYSTEM_STATE_NOT_VISIBLE )
	{
#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
		// Make sure the entity we're attaching to is being drawn
		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( this );
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer && pLocalPlayer == GetOwnerEntity() && pLocalPlayer->GetViewModel() && pLocalPlayer->GetViewModel()->GetWeapon() == pWeapon && !C_BasePlayer::ShouldDrawLocalPlayer() )
		{
			nVisible = PARTICLE_SYSTEM_STATE_VISIBLE_VM;
		}
#endif
	}

	if ( nVisible != PARTICLE_SYSTEM_STATE_NOT_VISIBLE && !ShouldDrawParticleSystems() )
	{
		nVisible = PARTICLE_SYSTEM_STATE_NOT_VISIBLE;
	}

	SetParticleSystemsVisible( nVisible );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconEntity::ShouldDrawParticleSystems( void )
{
#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer )
	{
		bool bStealthed = pPlayer->m_Shared.IsStealthed();
		if ( bStealthed )
			return false;
		bool bDisguised = pPlayer->m_Shared.InCond( TF_COND_DISGUISED );
		if ( bDisguised )
		{
			CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase*>( this );
			bool bDisguiseWeapon = pWeapon && pWeapon->m_bDisguiseWeapon;
			if ( !bDisguiseWeapon )
			{
				return false;
			}
		}
	}
#endif

	// Make sure the entity we're attaching to is being drawn
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		C_BaseEntity *pEffectOwner = this;
		if ( pLocalPlayer == GetOwnerEntity() && pLocalPlayer->GetViewModel() && !C_BasePlayer::ShouldDrawLocalPlayer() )
		{
			pEffectOwner = pLocalPlayer->GetViewModel();
		}

		if ( !pEffectOwner->ShouldDraw() )
		{
			return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEconEntity::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	if ( !InternalFireEvent( origin, angles, event, options ) )
	{
		BaseClass::FireEvent( origin, angles, event, options );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconEntity::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	return InternalFireEvent( origin, angles, event, options );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEconEntity::InternalFireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	switch( event )
	{
	case AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN:
		if ( m_hViewmodelAttachment )
		{
			// Translate it into a set bodygroup event on our attached weapon
			m_hViewmodelAttachment->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
		}
		return true;
		break;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Does this model use delayed flex weights?
//-----------------------------------------------------------------------------
bool CEconEntity::UsesFlexDelayedWeights()
{
	return m_flFlexDelayedWeight != NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Rendering callback to allow the client to set up all the model specific flex weights
//-----------------------------------------------------------------------------
void CEconEntity::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	if ( GetModelPtr() && GetModelPtr()->numflexcontrollers() )
	{
		if ( IsEffectActive( EF_BONEMERGE ) && GetMoveParent() )
		{
			C_BaseFlex *pParentFlex = dynamic_cast<C_BaseFlex*>( GetMoveParent() );
			if ( pParentFlex )
			{
				// BUGBUG: We have a bug with SetCustomModel that causes a disagreement between the studio header here and the one used in l_studio.cpp CModelRender::DrawModelExecute
				// So when we hit that case, let's not do any work because otherwise we'd crash since the array sizes (m_flFlexDelayedWeight vs pFlexWeights) don't match.
				// Note that this check is duplicated in C_BaseFlex::SetupLocalWeights.
				AssertMsg( nFlexWeightCount == m_cFlexDelayedWeight, "Disagreement between the number of flex weights. Do the studio headers match?" );
				if ( nFlexWeightCount != m_cFlexDelayedWeight )
				{
					return;
				}

				if ( pParentFlex->SetupGlobalWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights ) )
				{
					// convert the flex controllers into actual flex values
					C_BaseFlex::RunFlexRules( GetModelPtr(), pFlexWeights );
					
					// aim the eyes
					// SetViewTarget( hdr ); // FIXME: Not enough info yet
					
					// process local versions of the delay weights
					if ( pFlexDelayedWeights )
					{
						C_BaseFlex::RunFlexDelay( nFlexWeightCount, pFlexWeights, m_flFlexDelayedWeight, m_flFlexDelayTime );
						memcpy( pFlexDelayedWeights, m_flFlexDelayedWeight, sizeof( float ) * nFlexWeightCount );
					}
					return;
				}
			}
		}
	}

	BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	return;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
static void cc_dump_particlemanifest()
{
	Msg("Dumping particle list:\n");
	for ( int i = 0; i < g_pParticleSystemMgr->GetParticleSystemCount(); i++ )
	{
		const char *pParticleSystemName = g_pParticleSystemMgr->GetParticleSystemNameFromIndex(i);
		Msg(" %d: %s\n", i, pParticleSystemName );
	}
}
static ConCommand dump_particlemanifest( "dump_particlemanifest", cc_dump_particlemanifest, "Dump the list of particles loaded.", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::GetEconParticleSystems( CUtlVector<const attachedparticlesystem_t *> *out_pvecParticleSystems ) const
{
	Assert( out_pvecParticleSystems );
	
	const CEconItemView *pEconItemView = m_AttributeManager.GetItem();
	if ( pEconItemView )
	{
		const GameItemDefinition_t *pItemDef = pEconItemView->GetStaticData();

		// Count static particles included in the item definition -- these are things like
		// the kritzkrieg particles or the milk splash particles.
		const int iStaticParticleCount = pItemDef->GetNumAttachedParticles( GetTeamNumber() );
		for ( int i = 0; i < iStaticParticleCount; i++ )
		{
			out_pvecParticleSystems->AddToTail( pItemDef->GetAttachedParticleData( GetTeamNumber(), i ) );
		}

		// Do we have a particle effect that goes along with our specific quality? Self-made
		// and community items have a sparkle, for example.
		const int iQualityParticleType = pEconItemView->GetQualityParticleType();
		if ( iQualityParticleType > 0 )
		{
			out_pvecParticleSystems->AddToTail( GetItemSchema()->GetAttributeControlledParticleSystem( iQualityParticleType ) );
		}
	}

	// Do we have particle systems added on via static attributes (ie., pipe smoke)?
	// Note that this is functionally identical to the dynamic unusual particles. We don't support
	// having multiple attributes of the same type with independent values so we split these out
	// at a higher level, limiting ourself to one of each.
	int iStaticParticleEffect = 0;
	CALL_ATTRIB_HOOK_INT( iStaticParticleEffect, set_attached_particle_static );
	if ( iStaticParticleEffect > 0 )
	{
		out_pvecParticleSystems->AddToTail( GetItemSchema()->GetAttributeControlledParticleSystem( iStaticParticleEffect ) );
	}

	// Do we have particle systems added on dynamically (ie., unusuals?)?
	int iDynamicParticleEffect = 0;
	int iIsThrowableTrail = 0;
	CALL_ATTRIB_HOOK_INT( iDynamicParticleEffect, set_attached_particle );
	CALL_ATTRIB_HOOK_INT( iIsThrowableTrail, throwable_particle_trail_only );

#if defined(TF_CLIENT_DLL)
#endif

	if ( iDynamicParticleEffect > 0 && !iIsThrowableTrail )
	{
		attachedparticlesystem_t *pSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iDynamicParticleEffect );

		if ( pSystem )
		{
#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
			// TF Team Color Particles
			static char pszFullname[256];
			if ( GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pSystem->pszSystemName, "_teamcolor_red" ))
			{
				V_StrSubst( pSystem->pszSystemName, "_teamcolor_red", "_teamcolor_blue", pszFullname, 256 );
				pSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pszFullname );

			}
			else if ( GetTeamNumber() == TF_TEAM_RED && V_stristr( pSystem->pszSystemName, "_teamcolor_blue" ))
			{
				// Guard against accidentally giving out the blue team color (support tool)
				V_StrSubst( pSystem->pszSystemName, "_teamcolor_blue", "_teamcolor_red", pszFullname, 256 );
				pSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pszFullname );
			}
#endif
			if ( pSystem )
			{
				out_pvecParticleSystems->AddToTail( pSystem );
			}
		}
	}

	// Scan the particle system
	// - Clean up our list in case we fed in bad data from the schema or wherever.
	for ( int i = out_pvecParticleSystems->Count() - 1; i >= 0; i-- )
	{
		if ( !(*out_pvecParticleSystems)[i] ||
			 !(*out_pvecParticleSystems)[i]->pszSystemName ||
			 !(*out_pvecParticleSystems)[i]->pszSystemName[0] )
		{
			out_pvecParticleSystems->FastRemove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::SetParticleSystemsVisible( ParticleSystemState_t nState )
{
	if ( nState == m_nParticleSystemsCreated )
	{
		bool bDirty = false;

#if defined(TF_CLIENT_DLL) || defined(TF_DLL)
		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( this );
		if ( pWeapon )
		{
			if ( pWeapon->m_hExtraWearable.Get() )
			{
				bDirty = !( pWeapon->m_hExtraWearable->m_nParticleSystemsCreated == nState );
				pWeapon->m_hExtraWearable->m_nParticleSystemsCreated = nState;
			}

			if ( pWeapon->m_hExtraWearableViewModel.Get() )
			{
				bDirty = !( pWeapon->m_hExtraWearableViewModel->m_nParticleSystemsCreated == nState );
				pWeapon->m_hExtraWearableViewModel->m_nParticleSystemsCreated = nState;
			}
		}
#endif

		if ( !bDirty )
		{
			return;
		}
	}

	CUtlVector<const attachedparticlesystem_t *> vecParticleSystems;
	GetEconParticleSystems( &vecParticleSystems );
	
	FOR_EACH_VEC( vecParticleSystems, i )
	{
		const attachedparticlesystem_t *pSystem = vecParticleSystems[i];
		Assert( pSystem );
		Assert( pSystem->pszSystemName );
		Assert( pSystem->pszSystemName[0] );
		
		// Ignore custom particles. Weapons handle them in custom fashions.
		if ( pSystem->iCustomType )
			continue;

		ParticleSystemState_t nIndividualParticleState = nState;
		if ( nIndividualParticleState == PARTICLE_SYSTEM_STATE_VISIBLE )
		{
			// double check that we don't have a style overriding us to not draw 
			// (e.g. two styles have cig_smoke and the third doesn't)
			const CEconItemView *pItem = GetAttributeContainer()->GetItem();
			if ( pItem )
			{
				GameItemDefinition_t *pDef = pItem->GetStaticData();
				if ( pDef && pDef->GetNumStyles() )
				{
					style_index_t unStyle = pItem->GetItemStyle();
					if ( unStyle != INVALID_STYLE_INDEX )
					{
						const CEconStyleInfo *pStyle = pDef->GetStyleInfo( unStyle );

						// It's possible to get back a NULL pStyle if GetItemStyle() returns INVALID_STYLE_INDEX.
						if ( pStyle )
						{
							if ( !pStyle->UseSmokeParticleEffect() && ( FStrEq( pSystem->pszSystemName, "drg_pipe_smoke" ) ) )
							{
								nIndividualParticleState = PARTICLE_SYSTEM_STATE_NOT_VISIBLE;
							}
						}
					}
				}
			}
		}

		UpdateSingleParticleSystem( nIndividualParticleState != PARTICLE_SYSTEM_STATE_NOT_VISIBLE, pSystem );
	}

	m_nParticleSystemsCreated = nState;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEconEntity::UpdateSingleParticleSystem( bool bVisible, const attachedparticlesystem_t *pSystem )
{
	Assert( pSystem );

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	C_BaseEntity *pEffectOwnerWM = this;
	C_BaseEntity *pEffectOwnerVM = NULL;
	
	bool bExtraWearable = false;
	bool bExtraWearableVM = false;

	CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase* >( this );
	if ( pWeapon )
	{
		pEffectOwnerVM = pWeapon->GetPlayerOwner() ? pWeapon->GetPlayerOwner()->GetViewModel() : NULL;
		if ( pWeapon->m_hExtraWearable.Get() )
		{
			pEffectOwnerWM = pWeapon->m_hExtraWearable.Get();
			bExtraWearable = true;
		}

		if ( pWeapon->m_hExtraWearableViewModel.Get() )
		{
			pEffectOwnerVM = pWeapon->m_hExtraWearableViewModel.Get();
			bExtraWearableVM = true;
		}
	}

	C_BaseEntity *pEffectOwner = pEffectOwnerWM;
	bool bIsVM = false;
	C_BasePlayer *pOwner = ToBasePlayer(GetOwnerEntity());
	bool bDrawThisEffect = true;
	if ( !pOwner->ShouldDrawThisPlayer() )
	{
		// only draw effects designated for this
		if ( !pSystem->bDrawInViewModel )
		{
			bDrawThisEffect = false;
		}

		C_BaseViewModel *pLocalPlayerVM = pLocalPlayer->GetViewModel();
		if ( pLocalPlayerVM && pLocalPlayerVM->GetOwningWeapon() == this )
		{
			bIsVM = true;
			pEffectOwner = pEffectOwnerVM;
		}
	}

	const char *pszAttachmentName = pSystem->pszControlPoints[0];
	if ( bIsVM && bExtraWearableVM )
		pszAttachmentName = "attach_fob_v";
	if ( !bIsVM && bExtraWearable )
		pszAttachmentName = "attach_fob";

	int iAttachment = INVALID_PARTICLE_ATTACHMENT;
	if ( pszAttachmentName && pszAttachmentName[0] && pEffectOwner->GetBaseAnimating() )
	{
		iAttachment = pEffectOwner->GetBaseAnimating()->LookupAttachment( pszAttachmentName );
	}

	// Stop it on both the viewmodel & the world model, because it may be removed due to first/thirdperson switch
	// Get Full name
	const CEconItemView *pEconItemView = m_AttributeManager.GetItem();
	static char pszTempName[256] = { 0 };
	static char pszTempNameVM[256] = { 0 };
	const char* pszSystemName = pSystem->pszSystemName;
	

	// Weapon Remap for a Base Effect to be used on a specific weapon
	if ( pSystem->bUseSuffixName && pEconItemView && pEconItemView->GetItemDefinition()->GetParticleSuffix() )
	{
		V_strcpy_safe( pszTempName, pszSystemName );
		V_strcat_safe( pszTempName, "_" );
		V_strcat_safe( pszTempName, pEconItemView->GetItemDefinition()->GetParticleSuffix() );
		pszSystemName = pszTempName;
	}
	
	bool bHasUniqueVMEffect = true;
	if ( pSystem->bDrawInViewModel )
	{
		V_strcpy_safe( pszTempNameVM, pszSystemName );
		V_strcat_safe( pszTempNameVM, "_vm" );
		
		// VM doesn't exist so fall back to regular
		if ( g_pParticleSystemMgr->FindParticleSystem( pszTempNameVM ) == NULL )
		{
			V_strcpy_safe( pszTempNameVM, pszSystemName );
			bHasUniqueVMEffect = false;
		}

		if ( bIsVM )
		{
			pszSystemName = pszTempNameVM;
		}
	}

	// Check that the effect is valid
	if ( g_pParticleSystemMgr->FindParticleSystem( pszSystemName ) == NULL  )
		return;

	if ( iAttachment != INVALID_PARTICLE_ATTACHMENT )
	{
		pEffectOwnerWM->ParticleProp()->StopParticlesWithNameAndAttachment( pszSystemName, iAttachment, true );

		if ( pEffectOwnerVM )
		{
			if ( bHasUniqueVMEffect )
			{
				pEffectOwnerVM->ParticleProp()->StopParticlesWithNameAndAttachment( pszTempNameVM, iAttachment, true );
			}
			pEffectOwnerVM->ParticleProp()->StopParticlesWithNameAndAttachment( pszSystemName, iAttachment, true );
		}
	}
	else
	{
		pEffectOwnerWM->ParticleProp()->StopParticlesNamed( pszSystemName, true );

		if ( pEffectOwnerVM )
		{
			if ( bHasUniqueVMEffect )
			{
				pEffectOwnerVM->ParticleProp()->StopParticlesNamed( pszTempNameVM, true );
			}
			pEffectOwnerVM->ParticleProp()->StopParticlesNamed( pszSystemName, true );
		}
	}

	if ( !bDrawThisEffect )
		return;

	// do not generate a viewmodel effect if there is no weapon else it is in your face
	if ( !pWeapon && bIsVM )
	{
		Assert( 0 );
		Warning( "Cannot create a Viewmodel Particle Effect [%s] when there is no Viewmodel Weapon", pszSystemName );
		return;
	}

	if ( bVisible && pEffectOwner )
	{
		HPARTICLEFFECT pEffect = NULL;
		// We can't have fastcull on if we want particles attached to us
		//if ( !bIsVM )
		{
			RemoveEffects( EF_BONEMERGE_FASTCULL );
		}

		if ( iAttachment != INVALID_PARTICLE_ATTACHMENT )
		{
			pEffect = pEffectOwner->ParticleProp()->Create( pszSystemName, PATTACH_POINT_FOLLOW, pszAttachmentName );
		}
		else
		{
			// Attachments can fall back to following root bones if the attachment point wasn't found
			if ( pSystem->bFollowRootBone )
			{
				pEffect = pEffectOwner->ParticleProp()->Create( pszSystemName, PATTACH_ROOTBONE_FOLLOW );
			}
			else
			{
				pEffect = pEffectOwner->ParticleProp()->Create( pszSystemName, PATTACH_ABSORIGIN_FOLLOW );
			}
		}

		if ( pEffect )
		{
			// update the control points if necessary
			for ( int i=1; i<ARRAYSIZE( pSystem->pszControlPoints ); ++i )
			{
				const char *pszControlPointName = pSystem->pszControlPoints[i];
				if ( pszControlPointName && pszControlPointName[0] != '\0' )
				{
					pEffectOwner->ParticleProp()->AddControlPoint( pEffect, i, this, PATTACH_POINT_FOLLOW, pszControlPointName );
				}
			}

			if ( bIsVM )
			{
				pEffect->SetIsViewModelEffect( true );
				ClientLeafSystem()->SetRenderGroup( pEffect->RenderHandle(), RENDER_GROUP_VIEW_MODEL_TRANSLUCENT );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::InitializeAsClientEntity( const char *pszModelName, RenderGroup_t renderGroup )
{
	m_bClientside = true;
	return BaseClass::InitializeAsClientEntity( pszModelName, renderGroup );
}

//-----------------------------------------------------------------------------
// Purpose: Get an econ material override for the given team.
// Returns: NULL if there is no override. 
//-----------------------------------------------------------------------------
IMaterial* CEconEntity::GetEconWeaponMaterialOverride( int iTeam ) 
{
	if ( iTeam >= 0 && iTeam < TEAM_VISUAL_SECTIONS && m_MaterialOverrides[ iTeam ].IsValid() )
		return m_MaterialOverrides[ iTeam ];

	return NULL;
}


bool CEconEntity::ShouldDraw()
{
	if ( ShouldHideForVisionFilterFlags() )
	{
		return false;
	}

	return BaseClass::ShouldDraw();
}

bool CEconEntity::ShouldHideForVisionFilterFlags( void )
{
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	if ( pItem && pItem->IsValid() )
	{
		CEconItemDefinition *pData = pItem->GetStaticData();
		if ( pData )
		{
			int nVisionFilterFlags = pData->GetVisionFilterFlags();
			if ( nVisionFilterFlags != 0 )
			{
				// Only visible if the local player has an item that allows them to see it (Pyro Goggles)
				if ( !IsLocalPlayerUsingVisionFilterFlags( nVisionFilterFlags, true ) )
				{
					// They didn't have the correct vision flags
					return true;
				}
			}
		}
	}

	return false;
}

bool CEconEntity::IsTransparent( void )
{
#ifdef TF_CLIENT_DLL
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );
	if ( pPlayer && pPlayer->IsTransparent() )
	{
		return true;
	}
#endif // TF_CLIENT_DLL

	return BaseClass::IsTransparent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::ViewModel_IsTransparent( void )
{
	if ( m_hViewmodelAttachment != NULL && m_hViewmodelAttachment->IsTransparent() )
	{
		return true;
	}
	return IsTransparent();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::ViewModel_IsUsingFBTexture( void )
{
	if ( m_hViewmodelAttachment != NULL && m_hViewmodelAttachment->UsesPowerOfTwoFrameBufferTexture() )
	{
		return true;
	}
	return UsesPowerOfTwoFrameBufferTexture();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::IsOverridingViewmodel( void )
{
	bool bUseOverride = (GetTeamNumber() >= 0 && GetTeamNumber() < TEAM_VISUAL_SECTIONS) && m_MaterialOverrides[GetTeamNumber()].IsValid();
	bUseOverride = bUseOverride || (m_hViewmodelAttachment != NULL) || ( m_AttributeManager.GetItem()->GetStaticData()->GetNumAttachedModels( GetTeamNumber() ) > 0 );
	return bUseOverride;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CEconEntity::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int ret = 0;
#ifndef DOTA_DLL
	bool bIsAttachmentTranslucent = m_hViewmodelAttachment.Get() ? m_hViewmodelAttachment->IsTransparent() : false;
	bool bUseOverride = false;
	
	CEconItemView *pItem = GetAttributeContainer()->GetItem();
	bool bAttachesToHands = ( pItem->IsValid() && (pItem->GetStaticData()->ShouldAttachToHands() || pItem->GetStaticData()->ShouldAttachToHandsVMOnly()));

	// If the attachment is translucent, we need to render the viewmodel first
	if ( bIsAttachmentTranslucent )
	{
		ret = pViewmodel->DrawOverriddenViewmodel( flags );
	}

	if ( flags & STUDIO_RENDER )
	{
		// If there is some other material override, it's probably the client asking for us to render invuln or the 
		// spy cloaking. Those are way more important than ours, so do them instead.
		IMaterial* pOverrideMaterial = NULL;
		OverrideType_t nDontcare = OVERRIDE_NORMAL;
		modelrender->GetMaterialOverride( &pOverrideMaterial, &nDontcare );
		bool bIgnoreOverride = pOverrideMaterial != NULL;

		bUseOverride = !bIgnoreOverride && (GetTeamNumber() >= 0 && GetTeamNumber() < TEAM_VISUAL_SECTIONS) && m_MaterialOverrides[GetTeamNumber()].IsValid();
		if ( bUseOverride )
		{
			modelrender->ForcedMaterialOverride( m_MaterialOverrides[GetTeamNumber()] );
			flags |= STUDIO_NO_OVERRIDE_FOR_ATTACH;
		}
	
		if ( m_hViewmodelAttachment )
		{
			m_hViewmodelAttachment->RemoveEffects( EF_NODRAW );
			m_hViewmodelAttachment->DrawModel( flags );
			m_hViewmodelAttachment->AddEffects( EF_NODRAW );
		}

		// if we are attached to the hands, then we DO NOT want have an override material when we draw our view model
		if ( bAttachesToHands && bUseOverride )
		{
			modelrender->ForcedMaterialOverride( NULL );
			bUseOverride = false;
		}
	}

	if ( !bIsAttachmentTranslucent )
	{
		ret = pViewmodel->DrawOverriddenViewmodel( flags );
	}

	if ( bUseOverride )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}
#endif // !defined( DOTA_DLL )
	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	if ( !BaseClass::OnInternalDrawModel( pInfo ) )
		return false;

	// Correct the ambient lighting position to match our owner entity
	if ( GetOwnerEntity() && pInfo )
	{
		pInfo->pLightingOrigin = &( GetOwnerEntity()->WorldSpaceCenter() );
	}

	DrawEconEntityAttachedModels( this, this, pInfo, kAttachedModelDisplayFlag_WorldModel );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CEconEntity::LookupAttachment( const char *pAttachmentName )
{
	if ( m_hViewmodelAttachment )
		return m_hViewmodelAttachment->LookupAttachment( pAttachmentName );

	return BaseClass::LookupAttachment( pAttachmentName );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::GetAttachment( int number, matrix3x4_t &matrix )
{
	if ( m_hViewmodelAttachment )
		return m_hViewmodelAttachment->GetAttachment( number, matrix );

	return BaseClass::GetAttachment( number, matrix );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::GetAttachment( int number, Vector &origin )
{
	if ( m_hViewmodelAttachment )
		return m_hViewmodelAttachment->GetAttachment( number, origin );

	return BaseClass::GetAttachment( number, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::GetAttachment( int number, Vector &origin, QAngle &angles )
{
	if ( m_hViewmodelAttachment )
		return m_hViewmodelAttachment->GetAttachment( number, origin, angles );

	return BaseClass::GetAttachment( number, origin, angles );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CEconEntity::GetAttachmentVelocity( int number, Vector &originVel, Quaternion &angleVel )
{
	if ( m_hViewmodelAttachment )
		return m_hViewmodelAttachment->GetAttachmentVelocity( number, originVel, angleVel );

	return BaseClass::GetAttachmentVelocity( number, originVel, angleVel );
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Hides or shows masked bodygroups associated with this item.
//-----------------------------------------------------------------------------
bool CEconEntity::UpdateBodygroups( CBaseCombatCharacter* pOwner, int iState )
{
	if ( !pOwner )
		return false;

	CAttributeContainer *pCont = GetAttributeContainer();
	if ( !pCont )
		return false;

	CEconItemView *pItem = pCont->GetItem();
	if ( !pItem )
		return false;

	const CEconItemDefinition *pItemDef = pItem->GetItemDefinition();
	if ( !pItemDef )
		return false;

	int iNumBodyGroups = pItemDef->GetNumModifiedBodyGroups( 0 );
	for ( int i=0; i<iNumBodyGroups; ++i )
	{
		int iBody = 0;
		const char *pszBodyGroup = pItemDef->GetModifiedBodyGroup( 0, i, iBody );
		if ( iBody != iState )
			continue;

		int iBodyGroup = pOwner->FindBodygroupByName( pszBodyGroup );

		if ( iBodyGroup == -1 )
			continue;

		pOwner->SetBodygroup( iBodyGroup, iState );
	}

	// Handle per-style bodygroup hiding
	const CEconStyleInfo *pStyle = pItemDef->GetStyleInfo( pItem->GetStyle() );
	if ( pStyle )
	{
		FOR_EACH_VEC( pStyle->GetAdditionalHideBodygroups(), i )
		{
			int iBodyGroup = pOwner->FindBodygroupByName( pStyle->GetAdditionalHideBodygroups()[i] );

			if ( iBodyGroup == -1 )
				continue;

			pOwner->SetBodygroup( iBodyGroup, iState );
		}

		// should we override this model bodygroup
		if ( pStyle->GetBodygroupName() != NULL )
		{
			int iBodyGroup = pOwner->FindBodygroupByName( pStyle->GetBodygroupName() );
			if ( iBodyGroup != -1 )
			{
				SetBodygroup( iBodyGroup, pStyle->GetBodygroupSubmodelIndex() );
			}
		}
	}

	// Handle world model bodygroup overrides
	int iBodyOverride = pItemDef->GetWorldmodelBodygroupOverride( pOwner->GetTeamNumber() );
	int iBodyStateOverride = pItemDef->GetWorldmodelBodygroupStateOverride( pOwner->GetTeamNumber() );
	if ( iBodyOverride > -1 && iBodyStateOverride > -1 )
	{
		pOwner->SetBodygroup( iBodyOverride, iBodyStateOverride );
	}

	// Handle view model bodygroup overrides
	iBodyOverride = pItemDef->GetViewmodelBodygroupOverride( pOwner->GetTeamNumber() );
	iBodyStateOverride = pItemDef->GetViewmodelBodygroupStateOverride( pOwner->GetTeamNumber() );
	if ( iBodyOverride > -1 && iBodyStateOverride > -1 )
	{
		CBasePlayer *pPlayer = ToBasePlayer( pOwner );
		if ( pPlayer )
		{
			CBaseViewModel *pVM = pPlayer->GetViewModel();
			if ( pVM && pVM->GetModelPtr() )
			{
				pVM->SetBodygroup( iBodyOverride, iBodyStateOverride );
			}
		}
	}
	 
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CBaseAttributableItem::CBaseAttributableItem()
{
}
