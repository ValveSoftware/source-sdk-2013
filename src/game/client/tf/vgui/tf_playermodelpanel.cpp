//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_playermodelpanel.h"
#include "tf_classdata.h"
#include "tf_item_inventory.h"
#include "vgui/IVGui.h"
#include "game_item_schema.h"
#include "econ_item_system.h"
#include "animation.h"
#include "choreoscene.h"
#include "choreoevent.h"
#include "choreoactor.h"
#include "choreochannel.h"
#include "scenefilecache/ISceneFileCache.h"
#include "c_sceneentity.h"
#include "c_baseflex.h"
#include "sentence.h"
#include "engine/IEngineSound.h"
#include "c_tf_player.h"
#include "tier2/renderutils.h"
#include "bone_setup.h"
#include "halloween/tf_weapon_spellbook.h"
#include "matsys_controls/matsyscontrols.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DECLARE_BUILD_FACTORY( CTFPlayerModelPanel );

char g_szSceneTmpName[256];

static bool IsTauntItem( GameItemDefinition_t *pItemDef, const int iTeam, const int iClass, const char **ppSequence = NULL, const char **ppRequiredItem = NULL, const char **ppScene = NULL )
{
	if ( !IsTauntSlot( pItemDef->GetLoadoutSlot( iClass ) ) )
	{
		return false;
	}

	CTFTauntInfo *pTauntData = pItemDef->GetTauntData();
	if ( pTauntData )
	{
		if ( ppScene )
		{
			if ( pItemDef->GetDefinitionIndex() == 1183 )
			{
				*ppScene = "scenes/player/items/taunts/yeti_taunt.vcd";
			}
			else
			{
				int iTauntIndex = RandomInt( 0, pTauntData->GetIntroSceneCount( iClass ) - 1 );
				*ppScene = pTauntData->GetIntroScene( iClass, iTauntIndex );
			}
		}

		if ( ppRequiredItem )
		{
			*ppRequiredItem = pTauntData->GetProp( iClass );
		}

		return true;
	}

	for ( int i=0; i<pItemDef->GetNumAnimations( iTeam ); ++i )
	{
		animation_on_wearable_t* pAnim = pItemDef->GetAnimationData( iTeam, i );
		if ( pAnim && pAnim->pszActivity &&	!Q_stricmp( pAnim->pszActivity, "taunt_concept" ) )
		{
			// If we have a scene, use it first
			const char *pszScene = pAnim->pszScene;
			if ( pszScene && (iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS) )
			{
				if ( ppScene )
				{
					Q_snprintf( g_szSceneTmpName, sizeof(g_szSceneTmpName), "scenes/player/%s/low/%s", g_aPlayerClassNames_NonLocalized[iClass], pszScene );
					*ppScene = g_szSceneTmpName;
				}
			}

			const char *pszSequence = pAnim->pszSequence;
			if ( pszSequence )
			{
				if ( ppSequence )
				{
					*ppSequence = pszSequence;
				}
				if ( ppRequiredItem )
				{
					*ppRequiredItem = pAnim->pszRequiredItem;
				}
			}

			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerModelPanel::CTFPlayerModelPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName ),
	m_LocalToGlobal( 0, 0, FlexSettingLessFunc )
{
	m_iCurrentClassIndex = TF_CLASS_UNDEFINED;
	m_iCurrentSlotIndex = -1;

	m_nBody = 0;
	m_pHeldItem = NULL;
	m_iTeam = TF_TEAM_RED;
	m_bZoomedToHead = false;
	m_pszVCD = NULL;
	m_pszWeaponEntityRequired = NULL;
	m_bLoopVCD = true;
	m_bVCDFileNameOnly = true;

	InitPhonemeMappings();

	m_pScene = NULL;
	ClearScene();
	memset( m_flexWeight, 0, sizeof( m_flexWeight ) );

	SetIgnoreDoubleClick( true );

	for ( int i = 0; i < ARRAYSIZE( m_aParticleSystems ); i++ )
	{
		m_aParticleSystems[i] = NULL;
	}

	m_bPlaySparks = false;
	m_pszEyeGlowParticleName[0] = '\0';
	m_bDrawActionSlotEffects = false;
	m_bDrawTauntParticles = false;
	m_strPlayerModelOverride = "";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerModelPanel::~CTFPlayerModelPanel( void )
{
	m_vecItemsLoaded.PurgeAndDeleteElements();
	m_ItemsToCarry.PurgeAndDeleteElements();

	for ( int i = 0; i < ARRAYSIZE( m_aParticleSystems ); i++ )
	{
		SafeDeleteParticleData( &m_aParticleSystems[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_angPlayerOrg = m_angPlayer;

	static ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		inResourceData->ProcessResolutionKeys( "_minmode" );
	}

	// custom class data
	m_customClassData.Purge();
	KeyValues *pCustomData = inResourceData->FindKey( "customclassdata" );
	if ( pCustomData )
	{
		for ( KeyValues *pData = pCustomData->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
		{
			CustomClassData_t data;
			data.m_flFOV = pData->GetFloat( "fov" );
			data.m_vPosition.Init( pData->GetFloat( "origin_x" ), pData->GetFloat( "origin_y" ), pData->GetFloat( "origin_z" ) );
			data.m_vAngles.Init( pData->GetFloat( "angles_x" ), pData->GetFloat( "angles_y" ), pData->GetFloat( "angles_z" ) );
			m_customClassData.AddToTail( data );
		}

		Assert( m_customClassData.Count() == TF_LAST_NORMAL_CLASS );
	}

	// always allow particle for this panel
	m_bUseParticle = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetToPlayerClass( int iClass, bool bForceRefresh /*= false*/, const char *pszPlayerModelOverride /*= NULL*/ )
{
	if ( !m_strPlayerModelOverride.IsEqual_CaseInsensitive( pszPlayerModelOverride ) )
	{
		bForceRefresh = true;
		m_strPlayerModelOverride = pszPlayerModelOverride ? pszPlayerModelOverride : "";
	}

	if ( m_iCurrentClassIndex == iClass && !bForceRefresh )
		return;

	if ( m_bZoomedToHead )
	{
		ToggleZoom();
	}

	m_iCurrentClassIndex = iClass;
	ClearScene();

	if ( IsValidTFPlayerClass( m_iCurrentClassIndex ) )
	{
		if ( !m_strPlayerModelOverride.IsEmpty() )
		{
			SetMDL( m_strPlayerModelOverride.Get() );
		}
		else
		{
			TFPlayerClassData_t *pData = GetPlayerClassData( m_iCurrentClassIndex );
			SetMDL( pData->GetModelName() );
			HoldFirstValidItem();
		}

		// set custom class data
		if ( m_customClassData.IsValidIndex( m_iCurrentClassIndex ) )
		{
			SetCameraFOV( m_customClassData[m_iCurrentClassIndex].m_flFOV );
			m_vecPlayerPos = m_customClassData[m_iCurrentClassIndex].m_vPosition;
			m_angPlayer = m_customClassData[m_iCurrentClassIndex].m_vAngles;
		}
		else
		{
			m_angPlayer = m_angPlayerOrg;
		}
	}
	else
	{
		SetMDL( MDLHANDLE_INVALID );
		RemoveAdditionalModels();
	}

	InitPhonemeMappings();

	SetTeam( TF_TEAM_RED );

	m_nBody = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::HoldFirstValidItem( void )
{
	RemoveAdditionalModels();

	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return;

	int iDesiredSlot = -1;

	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		bool bIsTauntItem = IsTauntItem( pItem->GetStaticData(), GetTeam(), m_iCurrentClassIndex );
		if ( !bIsTauntItem )
		{
			if ( pItem->GetStaticData()->IsAWearable() )
				continue;
			if ( pItem->GetAnimationSlot() == -2 )
				continue;
		}

		// Found a weapon. Wield it.
		iDesiredSlot = pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex );
		break;
	}

	if ( iDesiredSlot != -1 )
	{
		UpdateHeldItem( iDesiredSlot );
		return;
	}

	// If we didn't find a weapon to wield, we wield the class's base primary weapon
	CEconItemView *pItem = TFInventoryManager()->GetBaseItemForClass( m_iCurrentClassIndex, LOADOUT_POSITION_PRIMARY );
	if ( !pItem || !pItem->IsValid() )
	{
		// Some classes only have secondary weapons. Fall back to that.
		pItem = TFInventoryManager()->GetBaseItemForClass( m_iCurrentClassIndex, LOADOUT_POSITION_SECONDARY );
	}

	if ( pItem && pItem->IsValid() )
	{
		SwitchHeldItemTo( pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::HoldItemInSlot( int iSlot )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return false;

	return UpdateHeldItem( iSlot );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::HoldItem( int iItemNumber )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return false;

	if ( iItemNumber >= m_ItemsToCarry.Count() )
		return false;

	CEconItemView *pItem = m_ItemsToCarry[iItemNumber];

	bool bIsTauntitem = IsTauntItem( pItem->GetStaticData(), GetTeam(), m_iCurrentClassIndex );

	// Ignore requests to equip wearables, because they're always equipped
	// Also ignore requests to equip non-wearables that are never actively equipped
	if ( bIsTauntitem || ( !pItem->GetStaticData()->IsAWearable() && pItem->GetAnimationSlot() != -2 ) )
	{
		SwitchHeldItemTo( pItem );
		return true;
	}

	// If we were trying to switch to a new item, and it's not valid, stick to our current
	if ( pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex ) != m_iCurrentSlotIndex )
	{
		UpdateHeldItem( m_iCurrentSlotIndex );
		return false;
	}

	// We were trying to stay on the current weapon, and it's not valid. Find anything.
	HoldFirstValidItem();

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::UpdateHeldItem( int iDesiredSlot )
{
	m_pHeldItem = NULL;

	CEconItemView *pItem = GetItemInSlot( iDesiredSlot );
	if ( pItem )
	{
		bool bIsTauntItem = IsTauntItem( pItem->GetStaticData(), GetTeam(), m_iCurrentClassIndex );
		// Ignore requests to equip wearables, because they're always equipped
		// Also ignore requests to equip non-wearables that are never actively equipped
		if ( bIsTauntItem || ( !pItem->GetStaticData()->IsAWearable() && pItem->GetAnimationSlot() != -2 ) )
		{
			SwitchHeldItemTo( pItem );
			return true;
		}
	}

	// If we were trying to switch to a new item, and it's not valid, stick to our current
	if ( iDesiredSlot != m_iCurrentSlotIndex )
	{
		UpdateHeldItem( m_iCurrentSlotIndex );
		return false;
	}

	// We were trying to stay on the current weapon, and it's not valid. Find anything.
	HoldFirstValidItem();
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ClearScene( void )
{
	if ( m_pScene )
	{
		delete m_pScene;
	}

	m_pScene = NULL;
	m_flSceneTime = 0;
	m_flSceneEndTime = 0;
	m_flLastTickTime = 0;
	m_bLoopScene = true;
	//memset( m_flexWeight, 0, sizeof( m_flexWeight ) );
}

extern CChoreoStringPool g_ChoreoStringPool;
CChoreoScene *LoadSceneForModel( const char *filename, IChoreoEventCallback *pCallback, float *flSceneEndTime )
{
	char loadfile[ 512 ];
	V_strcpy_safe( loadfile, filename );
	V_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	V_FixSlashes( loadfile );

	char *pBuffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	pBuffer = new char[ bufsize ];
	if ( !scenefilecache->GetSceneData( filename, (byte *)pBuffer, bufsize ) )
	{
		delete[] pBuffer;
		return NULL;
	}

	CChoreoScene *pScene;
	if ( IsBufferBinaryVCD( pBuffer, bufsize ) )
	{
		pScene = new CChoreoScene( pCallback );
		CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
		if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
		{
			Warning( "Unable to restore binary scene '%s'\n", loadfile );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( Scene_Printf );
			pScene->SetEventCallbackInterface( pCallback );
		}
	}
	else
	{
		g_TokenProcessor.SetBuffer( pBuffer );
		pScene = ChoreoLoadScene( loadfile, pCallback, &g_TokenProcessor, Scene_Printf );
	}

	delete[] pBuffer;

	if ( flSceneEndTime != NULL )
	{
		// find the scene length
		// The scene is as long as the end point for the last event unless one of the events is a loop
		*flSceneEndTime = 0.0f;
		bool bSetEndTime = false;
		for ( int i = 0; i < pScene->GetNumEvents(); i++ )
		{
			CChoreoEvent *pEvent = pScene->GetEvent( i );
			if ( pEvent->GetType() == CChoreoEvent::LOOP )
			{
				*flSceneEndTime = -1.0f;
				bSetEndTime = false;
				break;
			}

			if ( pEvent->GetEndTime() > *flSceneEndTime )
			{
				*flSceneEndTime = pEvent->GetEndTime();
				bSetEndTime = true;
			}
		}

		if ( bSetEndTime )
		{
			*flSceneEndTime += 0.1f; // give time for lerp to idle pose
		}
	}

	return pScene;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::PlayVCD( const char *pszVCD, const char *pszWeaponEntityRequired /*= NULL*/, bool bLoopVCD /*= true*/, bool bFileNameOnly /*= true*/ )
{
	m_pszVCD = pszVCD;
	m_pszWeaponEntityRequired = pszWeaponEntityRequired;
	m_bLoopVCD = bLoopVCD;
	m_bVCDFileNameOnly = bFileNameOnly;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::FireEvent( const char *pszEventName, const char *pszEventOptions )
{
	//Plat_DebugString( CFmtStr( "********* ANIM EVENT: %s\n", pszEventName ) );

	if ( V_strcmp( pszEventName, "AE_WPN_HIDE" ) == 0 )
	{
		int nWeaponIndex = GetMergeMDLIndex( static_cast<IClientRenderable*>(m_pHeldItem) );
		if ( nWeaponIndex >= 0 )
		{
			m_aMergeMDLs[nWeaponIndex].m_bDisabled = true;
		}
	}
	else if ( V_strcmp( pszEventName, "AE_WPN_UNHIDE" ) == 0 )
	{
		int nWeaponIndex = GetMergeMDLIndex( static_cast<IClientRenderable*>(m_pHeldItem) );
		if ( nWeaponIndex >= 0 )
		{
			m_aMergeMDLs[nWeaponIndex].m_bDisabled = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SwitchHeldItemTo( CEconItemView *pItem )
{
	m_nBody = 0;

	ClearScene();

	m_pHeldItem = pItem;
	// force yeti model for yeti taunt item
	bool bYeti = false;
	if ( m_pHeldItem )
	{
		if ( m_pHeldItem->GetItemDefinition()->GetDefinitionIndex() == 1183 )
		{
			SetToPlayerClass( m_iCurrentClassIndex, false, "models/player/items/taunts/yeti/yeti.mdl" );
			bYeti = true;
		}
		else
		{
			SetToPlayerClass( m_iCurrentClassIndex );
		}
	}

	// Clear out visible items, and re-equip out wearables
	RemoveAdditionalModels();
	if ( !bYeti )
	{
		EquipAllWearables( pItem );
	}

	// Then equip the held item
	EquipItem( pItem );
	m_iCurrentSlotIndex = pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex );

	m_StatTrackModel.m_bDisabled = true;
	m_StatTrackModel.m_MDL.SetMDL( MDLHANDLE_INVALID );

	CAttribute_String attrModule;
	if ( GetStattrak( m_pHeldItem, &attrModule ) )
	{
		// Allow for already strange items
		bool bIsStrange = false;
		if ( m_pHeldItem->GetQuality() == AE_STRANGE )
		{
			bIsStrange = true;
		}

		if ( !bIsStrange )
		{
			// Go over the attributes of the item, if it has any strange attributes the item is strange and don't apply
			for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
			{
				if ( m_pHeldItem->FindAttribute( GetKillEaterAttr_Score( i ) ) )
				{
					bIsStrange = true;
					break;
				}
			}
		}

		if ( bIsStrange )
		{
			static CSchemaAttributeDefHandle pAttr_moduleScale( "weapon_stattrak_module_scale" );
			// Does it have a stat track module
			m_flStatTrackScale = 1.0f;
			uint32 unFloatAsUint32 = 1;
			if ( m_pHeldItem->FindAttribute( pAttr_moduleScale, &unFloatAsUint32 ) )
			{
				m_flStatTrackScale = (float&)unFloatAsUint32;
			}

			MDLHandle_t hStatTrackMDL = mdlcache->FindMDL( attrModule.value().c_str() );
			if ( mdlcache->IsErrorModel( hStatTrackMDL ) )
			{
				hStatTrackMDL = MDLHANDLE_INVALID;
			}
			m_StatTrackModel.m_MDL.SetMDL( hStatTrackMDL );
			mdlcache->Release( hStatTrackMDL ); // counterbalance addref from within FindMDL

			m_StatTrackModel.m_MDL.m_pProxyData = static_cast<IClientRenderable*>(pItem);
			m_StatTrackModel.m_bDisabled = false;
			m_StatTrackModel.m_MDL.m_nSequence = ACT_IDLE;
			SetIdentityMatrix( m_StatTrackModel.m_MDLToWorld );
		}
	}

	SetSequenceLayers( NULL, 0 );

	// See if our VCD is overridden
	if ( m_pszVCD )
	{
		// Make sure we're holding the weapon, if it's required
		bool bCanRunScene = true;
		if ( m_pszWeaponEntityRequired && *m_pszWeaponEntityRequired )
		{
			bCanRunScene = false;

			if ( pItem && pItem->IsValid() )
			{
				const char *pszClassName = pItem->GetStaticData()->GetItemClass();
				if ( pszClassName && *pszClassName )
				{
					bCanRunScene = V_stricmp( pszClassName, m_pszWeaponEntityRequired ) == 0;
				}
			}
		}

		if ( bCanRunScene )
		{
			if ( m_bVCDFileNameOnly )
			{
				// auto complete relative path for the vcd file
				V_sprintf_safe( g_szSceneTmpName, "scenes/player/%s/low/%s", g_aPlayerClassNames_NonLocalized[m_iCurrentClassIndex], m_pszVCD );
			}
			else
			{
				// m_pszVCD should be a valid relative path
				V_strcpy_safe( g_szSceneTmpName, m_pszVCD );
			}
			
			m_pScene = LoadSceneForModel( g_szSceneTmpName, this, &m_flSceneEndTime );
			m_bLoopScene = m_bLoopVCD;

			return;
		}
	}

	const char *pScene = NULL;
	const char *pSequence = NULL;
	const char *pRequiredItem = NULL;
	bool bRemoveTauntParticles = true;

	if ( IsTauntItem( pItem->GetStaticData(), GetTeam(), m_iCurrentClassIndex, &pSequence, &pRequiredItem, &pScene ) )
	{	
		MDLCACHE_CRITICAL_SECTION();

		if ( pScene )
		{
			m_pScene = LoadSceneForModel( pScene, this, &m_flSceneEndTime );
			
			// load custom prop for taunt
			const char *pszProp = pItem->GetStaticData()->GetTauntData()->GetProp( m_iCurrentClassIndex );
			if ( pszProp )
			{
				LoadAndAttachAdditionalModel( pszProp, pItem );
			}

			// force taunt to equip certain slot
			static CSchemaAttributeDefHandle pAttrDef_TauntForceWeaponSlot( "taunt force weapon slot" );
			const char* pszTauntForceWeaponSlotName = NULL;
			if ( FindAttribute_UnsafeBitwiseCast<CAttribute_String>( pItem, pAttrDef_TauntForceWeaponSlot, &pszTauntForceWeaponSlotName ) )
			{
				int iForceWeaponSlot = StringFieldToInt( pszTauntForceWeaponSlotName, GetItemSchema()->GetWeaponTypeSubstrings() );
				EquipRequiredLoadoutSlot( iForceWeaponSlot );
			}
		}
		else
		{
			ClearScene();

			CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
			int iSequence = LookupSequence( &studioHdr, pSequence );
			if ( iSequence >= 0 )
			{
				// does a weapon need to be equipped?
				loadout_positions_t requiredLoadoutItem = LOADOUT_POSITION_INVALID;
				if ( pRequiredItem )
				{
					requiredLoadoutItem = (loadout_positions_t)StringFieldToInt( pRequiredItem, ItemSystem()->GetItemSchema()->GetLoadoutStrings( pItem->GetItemDefinition()->GetEquipType() ) );
				}
				
				EquipRequiredLoadoutSlot( requiredLoadoutItem );

				// finally, set the sequence layers
				MDLSquenceLayer_t	tmpSequenceLayers[1];
				tmpSequenceLayers[0].m_nSequenceIndex = iSequence;
				tmpSequenceLayers[0].m_flWeight = 1.0;
				tmpSequenceLayers[0].m_bNoLoop = false;
				tmpSequenceLayers[0].m_flCycleBeganAt = m_RootMDL.m_MDL.m_flTime;
				SetSequenceLayers( tmpSequenceLayers, 1 );
			}
		}

		// Taunt Particles
		static CSchemaAttributeDefHandle pAttrDef_TauntAttachParticleIndex( "taunt attach particle index" );
		uint32 unUnusualEffectIndex = 0;
		if ( pItem->FindAttribute( pAttrDef_TauntAttachParticleIndex, &unUnusualEffectIndex ) && unUnusualEffectIndex > 0 )
		{
			const attachedparticlesystem_t *pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( unUnusualEffectIndex );
			if ( pParticleSystem )
			{
				SafeDeleteParticleData( &m_aParticleSystems[ SYSTEM_TAUNT ] );
				m_aParticleSystems[ SYSTEM_TAUNT ] = CreateParticleData( pParticleSystem->pszSystemName );
				m_flTauntParticleRefireTime = gpGlobals->curtime + pParticleSystem->fRefireTime;
				m_flTauntParticleRefireRate = pParticleSystem->fRefireTime;
				m_bDrawTauntParticles = true;
				bRemoveTauntParticles = false;
			}
		}
	}

	// update poseparam
	if ( pItem->GetStaticData()->GetNumPlayerPoseParameters( m_iTeam ) > 0 )
	{
		for ( int iPlayerPoseParam=0; iPlayerPoseParam < pItem->GetStaticData()->GetNumPlayerPoseParameters( m_iTeam ); ++iPlayerPoseParam )
		{
			poseparamtable_t *pPoseParam = pItem->GetStaticData()->GetPlayerPoseParameters( m_iTeam, iPlayerPoseParam );
			SetPoseParameterByName( pPoseParam->strName, pPoseParam->flValue );
		}
	}
	else
	{
		SetPoseParameterByName( "r_hand_grip", 0.f );
	}

	// Clear out taunt particles
	if ( bRemoveTauntParticles && m_aParticleSystems[SYSTEM_TAUNT] )
	{
		m_bDrawTauntParticles = false;
		SafeDeleteParticleData( &m_aParticleSystems[SYSTEM_TAUNT] );
	}

	// Check for hand particles (spell book)
	// always nuke
	if ( m_aParticleSystems[ SYSTEM_ACTIONSLOT ] )
	{
		SafeDeleteParticleData( &m_aParticleSystems[ SYSTEM_ACTIONSLOT ] );
	}
	m_bDrawActionSlotEffects = false;
	if ( pItem->GetStaticData()->GetItemClass() )
	{
		m_bDrawActionSlotEffects = FStrEq( pItem->GetStaticData()->GetItemClass(), "tf_weapon_spellbook" );
	}

	// update eyeglows
	m_bUpdateEyeGlows = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EquipRequiredLoadoutSlot( int iRequiredLoadoutSlot )
{
	if ( iRequiredLoadoutSlot != LOADOUT_POSITION_INVALID )
	{
		int iDesiredSlot = -1;
		FOR_EACH_VEC( m_ItemsToCarry, i )
		{
			CEconItemView *pItem = m_ItemsToCarry[i];
			if ( pItem->GetStaticData()->IsAWearable() )
				continue;
			if ( pItem->GetAnimationSlot() == -2 )
				continue;

			// Found a weapon. Wield it.
			if ( iRequiredLoadoutSlot == pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex ) )
			{
				iDesiredSlot = i;
				break;
			}					
		}

		if ( iDesiredSlot >= 0 )
		{
			EquipItem( m_ItemsToCarry[iDesiredSlot] );
		}
		else
		{
			// If we didn't find a weapon in the appropriate slot, get the base item
			CEconItemView *pWeapon = TFInventoryManager()->GetBaseItemForClass( m_iCurrentClassIndex, iRequiredLoadoutSlot );
			if ( pWeapon && pWeapon->IsValid() )
			{
				EquipItem( pWeapon );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::UpdateWeaponBodygroups( bool bModifyDeployedOnlyBodygroups )
{
	for ( int i=0; i<MAX_WEAPON_SLOTS; i++ )
	{
		CEconItemView *pItem = GetItemInSlot( i );
		if ( !pItem )
			continue;

		if ( pItem->GetStaticData()->GetHideBodyGroupsDeployedOnly() != bModifyDeployedOnlyBodygroups )
			continue;

		if ( !(m_pHeldItem == pItem || !pItem->GetStaticData()->GetHideBodyGroupsDeployedOnly()) )
			continue;

		UpdateHiddenBodyGroups( pItem );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::UpdateHiddenBodyGroups( CEconItemView* pItem )
{
	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	int iNumBodyGroups = pItem->GetStaticData()->GetNumModifiedBodyGroups( 0 );
	for ( int i=0; i<iNumBodyGroups; ++i )
	{
		int iState = 0;
		const char *pszBodyGroup = pItem->GetStaticData()->GetModifiedBodyGroup( 0, i, iState );
		int iBodyGroup = FindBodygroupByName( &studioHdr, pszBodyGroup );

		if ( iBodyGroup == -1 )
			continue;

		::SetBodygroup( &studioHdr, m_nBody, iBodyGroup, iState );
		SetBody( m_nBody );
	}

	// Handle style-based bodygroups
	const CEconItemDefinition *pItemDef = pItem->GetItemDefinition();
	const CEconStyleInfo *pStyle = pItemDef ? pItemDef->GetStyleInfo( pItem->GetStyle() ) : NULL;
	if ( pStyle )
	{
		FOR_EACH_VEC( pStyle->GetAdditionalHideBodygroups(), i )
		{
			int iBodyGroup = FindBodygroupByName( &studioHdr, pStyle->GetAdditionalHideBodygroups()[i] );

			if ( iBodyGroup == -1 )
				continue;

			::SetBodygroup( &studioHdr, m_nBody, iBodyGroup, 1 );			// force state to '1' here to mean hidden
			SetBody( m_nBody );
		}
	}

	// Handle world model bodygroup overrides
	int iBodyOverride = pItem->GetStaticData()->GetWorldmodelBodygroupOverride( m_iTeam  );
	int iBodyStateOverride = pItem->GetStaticData()->GetWorldmodelBodygroupStateOverride( m_iTeam  );
	if ( iBodyOverride > -1 && iBodyStateOverride > -1 )
	{
		::SetBodygroup( &studioHdr, m_nBody, iBodyOverride, iBodyStateOverride );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEconItemView *CTFPlayerModelPanel::GetItemInSlot( int iSlot )
{
	CEconItemView *pOwnedItemInSlot = TFInventoryManager()->GetItemInLoadoutForClass( m_iCurrentClassIndex, iSlot );

	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		int iLoadoutSlot = pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex );
		if ( iSlot == iLoadoutSlot )
			return pItem;

		// GetLoadoutSlot will not work for misc2, taunt2-8 because it will always return misc/taunt
		if ( pOwnedItemInSlot && pOwnedItemInSlot->GetItemDefIndex() == pItem->GetItemDefIndex() )
			return pItem;
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EquipAllWearables( CEconItemView *pHeldItem )
{
	// First, reset all our bodygroups
	MDLCACHE_CRITICAL_SECTION();
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	const CEconItemSchema::BodygroupStateMap_t& mapBodygroupState = GetItemSchema()->GetDefaultBodygroupStateMap();

	FOR_EACH_DICT_FAST( mapBodygroupState, i )
	{
		const char *pszBodygroupName = mapBodygroupState.GetElementName(i);
		int iBodyGroup = FindBodygroupByName( &studioHdr, pszBodygroupName );
		if ( iBodyGroup > -1 )
		{
			int iState = mapBodygroupState[i];
			::SetBodygroup( &studioHdr, m_nBody, iBodyGroup, iState );
		}
	}

	SetBody( m_nBody );

	UpdateWeaponBodygroups( false );

	// Now equip each of our wearables
	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		// If it's a wearable item, we put it on.
		if ( pItem->GetStaticData()->IsAWearable() )
		{
			EquipItem( pItem );
		}

		// Then see if there's an extra wearable we need to attach for this item
		const char *pszAttached = pItem->GetExtraWearableModel();
		if ( pszAttached && pszAttached[ 0 ] )
		{
			const char *pszViewModelAttached = pItem->GetExtraWearableViewModel();
			if ( pHeldItem == pItem || pszViewModelAttached == NULL || pszViewModelAttached[ 0 ] == '\0' || pszViewModelAttached[ 0 ] == '?' )
			{
				LoadAndAttachAdditionalModel( pszAttached, pItem );
			}
		}
	}

	UpdateWeaponBodygroups( true );

	SetBody( m_nBody );

	UpdatePreviewVisuals();
}

static const char *s_pszDefaultAnimForWpnSlot[] =
{
	"ACT_MP_STAND_PRIMARY",			// TF_WPN_TYPE_PRIMARY
	"ACT_MP_STAND_SECONDARY",		// TF_WPN_TYPE_SECONDARY
	"ACT_MP_STAND_MELEE",			// TF_WPN_TYPE_MELEE
	NULL,							// TF_WPN_TYPE_GRENADE
	"ACT_MP_STAND_BUILDING",		// TF_WPN_TYPE_BUILDING
	"ACT_MP_STAND_PDA",				// TF_WPN_TYPE_PDA
	"ACT_MP_STAND_ITEM1",			// TF_WPN_TYPE_ITEM1
	"ACT_MP_STAND_ITEM2",			// TF_WPN_TYPE_ITEM2
	NULL,							// TF_WPN_TYPE_HEAD
	NULL,							// TF_WPN_TYPE_MISC
	"ACT_MP_STAND_MELEE_ALLCLASS",	// TF_WPN_TYPE_MELEE_ALLCLASS
	"ACT_MP_STAND_SECONDARY2",		// TF_WPN_TYPE_SECONDARY2
	"ACT_MP_STAND_PRIMARY",			// TF_WPN_TYPE_PRIMARY2
	"ACT_MP_STAND_ITEM3",			// TF_WPN_TYPE_ITEM3
	"ACT_MP_STAND_ITEM4",			// TF_WPN_TYPE_ITEM4
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszDefaultAnimForWpnSlot ) == TF_WPN_TYPE_COUNT );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EquipItem( CEconItemView *pItem )
{
	if ( m_iCurrentClassIndex == TF_CLASS_UNDEFINED )
		return;

	const GameItemDefinition_t *pItemDef = pItem->GetItemDefinition();
	Assert( pItemDef );

	// Change team number so skins composite correctly
	pItem->SetTeamNumber( m_iTeam );

	// Non wearables can modify the animation
	if ( !pItemDef->IsAWearable() )
	{
		int iAnimSlot = pItem->GetAnimationSlot();

		// Ignore items that don't want to control player animation
		if ( iAnimSlot == -2 )
			return;

		if ( iAnimSlot == -1 )
		{
			iAnimSlot = pItemDef->GetLoadoutSlot( m_iCurrentClassIndex );
		}

		const CUtlVector<const char *>& vecWeaponTypeSubstrings = GetItemSchema()->GetWeaponTypeSubstrings();
		if ( vecWeaponTypeSubstrings.IsValidIndex( iAnimSlot ) )
		{
			MDLCACHE_CRITICAL_SECTION();

			// Get the studio header of the root model.
			if ( !m_RootMDL.m_pStudioHdr )
				return;

			CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
			int iSequence = FindSequenceFromActivity( &studioHdr, s_pszDefaultAnimForWpnSlot[ iAnimSlot ] );
			if ( iSequence != ACT_INVALID )
			{
				SetSequence( iSequence, true );
			}
		}
	}

	// Attach the models for the item
	const char *pszAttached = pItem->GetWorldDisplayModel();
	if ( !pszAttached )
	{
		pszAttached = pItem->GetPlayerDisplayModel( m_iCurrentClassIndex, m_iTeam );
	}

	if ( pszAttached && pszAttached[0] )
	{
		LoadAndAttachAdditionalModel( pszAttached, pItem );
		
		int iTeam = pItemDef->GetBestVisualTeamData( m_iTeam );
		// Set attached models if viewable third-person.
		{
			const int iNumAttachedModels = pItemDef->GetNumAttachedModels( iTeam );
			for ( int i = 0; i < iNumAttachedModels; ++i )
			{
				attachedmodel_t	*pModel = pItemDef->GetAttachedModelData( iTeam, i );

				if ( !( pModel->m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
					continue;

				if ( !pModel->m_pszModelName )
				{
					Warning( "econ item definition '%s' attachment (team %d idx %d) has no model\n", pItemDef->GetDefinitionName(), iTeam, i );
					continue;
				}

				LoadAndAttachAdditionalModel( pModel->m_pszModelName, pItem );
			}
		}

		// Festive
		// Set attached models if viewable third-person.
		static CSchemaAttributeDefHandle pAttr_is_festivized( "is_festivized" );
		if ( pAttr_is_festivized && pItem->FindAttribute( pAttr_is_festivized ) )
		{
			const int iNumAttachedModels = pItemDef->GetNumAttachedModelsFestivized( iTeam );
			for ( int i = 0; i < iNumAttachedModels; ++i )
			{
				attachedmodel_t	*pModel = pItemDef->GetAttachedModelDataFestivized( iTeam, i );

				if ( !( pModel->m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
					continue;

				if ( !pModel->m_pszModelName )
				{
					Warning( "econ item definition '%s' attachment (team %d idx %d) has no model\n", pItemDef->GetDefinitionName(), iTeam, i );
					continue;
				}

				LoadAndAttachAdditionalModel( pModel->m_pszModelName, pItem );
			}
		}
	}

	// Hide any item associated groups.
	UpdateHiddenBodyGroups( pItem );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerModelPanel::AddCarriedItem( CEconItemView *pItem )
{
	CEconItemView *pNewItem = new CEconItemView;
	*pNewItem = *pItem;
	int iIdx = m_ItemsToCarry.AddToTail( pNewItem );

	// This is a terrible hack. If we have team paint, we need an entity to find out what team
	// we're on, but in this panel we don't have one. Instead, we force a flag all the way through
	// the system on the CEconItemView so that the low-level paint code can pull from it if necessary.
	if ( GetTeam() == TF_TEAM_BLUE )
	{
		pNewItem->SetClientItemFlags( kEconItemFlagClient_ForceBlueTeam );
	}

	return iIdx;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ClearCarriedItems( void )
{
	RemoveAdditionalModels();
	m_ItemsToCarry.PurgeAndDeleteElements();
	m_pHeldItem = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::RemoveAdditionalModels( void )
{
	ClearMergeMDLs();

	// Unregister for all callbacks
	modelinfo->UnregisterModelLoadCallback( -1, this );
	m_vecDynamicAssetsLoaded.Purge();
	m_vecItemsLoaded.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::LoadAndAttachAdditionalModel( const char *pMDLName, CEconItemView *pItem )
{
	int nModelIndex = -1;
	
	if ( pItem->GetStaticData()->IsContentStreamable() )
	{
		// Get the client-only dynamic model index. The auto-addref
		// of vecDynamicAssetsLoaded will actually trigger the load.
		nModelIndex = modelinfo->RegisterDynamicModel( pMDLName, true );
		// Dynamic models never fail to register in this engine.
		Assert( nModelIndex != -1 );
	}
	else
	{
		// Is the (non-streamable) model already precached? If so, use it.
		nModelIndex = modelinfo->GetModelIndex( pMDLName );
	}

	if ( nModelIndex == -1 )
	{
		MDLHandle_t hMDL = vgui::MDLCache()->FindMDL( pMDLName );
		Assert( hMDL != MDLHANDLE_INVALID );
		if ( hMDL != MDLHANDLE_INVALID )
		{
			// Model not loaded, not dynamic. Hard load and exit out.
			SetMergeMDL( hMDL, static_cast<IClientRenderable*>(pItem), pItem->GetSkin( m_iTeam ) );
		}
		m_MergeMDL = hMDL;
		return;
	}

	CEconItemView *pClone = new CEconItemView;
	*pClone = *pItem;
	m_vecDynamicAssetsLoaded[ m_vecDynamicAssetsLoaded.AddToTail() ] = nModelIndex;
	m_vecItemsLoaded.AddToTail( pClone );

	// callback triggers immediately if not dynamic
	modelinfo->RegisterModelLoadCallback( nModelIndex, this, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static void SetMDLSkinForTeam( CMDL *pMDL, const CEconItemView *pItem, int iTeam )
{
	Assert( pItem );

	if ( !pMDL )
		return;

	// Ask the item for a skin...
	int nSkin = pItem->GetSkin( iTeam );

	if ( nSkin == -1 )
	{
		// ... if not, use the team skin.
		nSkin = iTeam == TF_TEAM_RED ? 0 : 1;
	}

	pMDL->m_nSkin = nSkin;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::OnModelLoadComplete( const model_t *pModel )
{
	CEconItemView *pItem = NULL;
	FOR_EACH_VEC_BACK( m_vecDynamicAssetsLoaded, i )
	{
		if ( modelinfo->GetModel( m_vecDynamicAssetsLoaded[ i ] ) == pModel )
		{
			pItem = GetPreviewItem( m_vecItemsLoaded[ i ] );
			break;
		}
	}

	Assert( pItem );
	if ( pItem )
	{
		MDLHandle_t hMDL = modelinfo->GetCacheHandle( pModel );
		Assert( hMDL != MDLHANDLE_INVALID );
		if ( hMDL != MDLHANDLE_INVALID )
		{
			SetMergeMDL( hMDL, static_cast<IClientRenderable*>(pItem) );

			int nBody = 0;
			if ( pItem->GetStaticData()->UsesPerClassBodygroups( m_iTeam ) )
			{
				CMDL *pMDL = GetMergeMDL(hMDL);
				if ( pMDL )
				{
					// Classes start at 1, bodygroups at 0, so we shift them all back 1.
					MDLCACHE_CRITICAL_SECTION();
					::SetBodygroup( GetMergeMDLStudioHdr( hMDL ), nBody, 1, m_iCurrentClassIndex-1 );
					pMDL->m_nBody = nBody;
				}
			}

			// Set the custom skin.
			SetMDLSkinForTeam( GetMergeMDL( hMDL ), pItem, m_iTeam );
		}
	}
}

void CTFPlayerModelPanel::SetTeam( int iTeam )
{
	m_iTeam = iTeam;

	UpdatePreviewVisuals();
}

void CTFPlayerModelPanel::UpdatePreviewVisuals()
{
	// Assume skin will be chosen based only on the preview team
	int iSkin = m_iTeam == TF_TEAM_RED ? 0 : 1;

	// Check if any of the items we're carrying should override this
	static CSchemaAttributeDefHandle pAttrDef_PlayerSkinOverride( "player skin override" );
	Assert( pAttrDef_PlayerSkinOverride );
	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		if ( !pItem )
			continue;
		float fSkinOverride = 0.0f;
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( pItem, pAttrDef_PlayerSkinOverride, &fSkinOverride ) && fSkinOverride == 1.0f )
		{
			C_TFPlayer::AdjustSkinIndexForZombie( m_iCurrentClassIndex, iSkin );
			break;
		}
		Assert( fSkinOverride == 0.0f );
	}

	// Set the player model skin.
	SetSkin( iSkin );

	// Set the weapon's skin.
	if ( m_MergeMDL && m_pHeldItem )
	{
		SetMDLSkinForTeam( GetMergeMDL( m_MergeMDL ), GetPreviewItem( m_pHeldItem ), m_iTeam );
	}

	// Set the skin for all other equipped items (wearables, etc).
	for ( int i=0; i<m_vecDynamicAssetsLoaded.Count(); i++ )
	{
		const model_t *pModel = modelinfo->GetModel( m_vecDynamicAssetsLoaded[i] );
		if ( pModel )
		{
			MDLHandle_t hMDL = modelinfo->GetCacheHandle( pModel );

			// We're iterating over a list of the dynamic assets that we've completed streaming in, but
			// we want to set the style based on the "preview item" definition if possible.
			SetMDLSkinForTeam( GetMergeMDL( hMDL ), GetPreviewItem( m_vecItemsLoaded[i] ), m_iTeam );
		}
	}
}

CEconItemView *CTFPlayerModelPanel::GetPreviewItem( CEconItemView *pMatchItem )
{
	Assert( pMatchItem );
	if ( !pMatchItem )
		return NULL;

	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		if ( *pMatchItem == *pItem )
			return pItem;
	}

	return pMatchItem;
}

int ClassZoomZ[] =
{
	0,
	20, // TF_CLASS_SCOUT,
	25, // TF_CLASS_SNIPER,
	20, // TF_CLASS_SOLDIER,
	22, // TF_CLASS_DEMOMAN,
	30, // TF_CLASS_MEDIC,
	30, // TF_CLASS_HEAVYWEAPONS,
	22, // TF_CLASS_PYRO,
	27, // TF_CLASS_SPY,
	20, // TF_CLASS_ENGINEER,		
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ToggleZoom()
{
	m_bZoomedToHead = !m_bZoomedToHead;

	// NOTE: GetZoomOffset() relies on m_bZoomedToHead being up to date
	m_vecPlayerPos += GetZoomOffset();

	SetModelAnglesAndPosition( m_angPlayer, m_vecPlayerPos );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CTFPlayerModelPanel::GetZoomOffset()
{
	const Vector vecOffset( 100, 0, ClassZoomZ[m_iCurrentClassIndex] );
	return m_bZoomedToHead ? -vecOffset : vecOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::PrePaint3D( IMatRenderContext *pRenderContext )
{
	if ( g_PlayerPreviewEffect.GetEffect() == C_TFPlayerPreviewEffect::PREVIEW_EFFECT_UBER )
	{
		modelrender->ForcedMaterialOverride( *g_PlayerPreviewEffect.GetInvulnMaterialRef() );
	}

	BaseClass::PrePaint3D( pRenderContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::PostPaint3D( IMatRenderContext *pRenderContext )
{
	if ( g_PlayerPreviewEffect.GetEffect() == C_TFPlayerPreviewEffect::PREVIEW_EFFECT_UBER )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	static bool bAlternate = false;
	Vector vColor = bAlternate ? m_vEyeGlowColor1 : m_vEyeGlowColor2;
	bAlternate = !bAlternate;
	// Eye glows
	if ( m_aParticleSystems[ SYSTEM_EYEGLOW_RIGHT ] )
	{
		m_aParticleSystems[ SYSTEM_EYEGLOW_RIGHT ]->m_pParticleSystem->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
	}
	if ( m_aParticleSystems[ SYSTEM_EYESPARK_RIGHT ] )
	{
		m_aParticleSystems[ SYSTEM_EYESPARK_RIGHT ]->m_pParticleSystem->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
	}

	if ( m_aParticleSystems[ SYSTEM_EYEGLOW_LEFT ] )
	{
		m_aParticleSystems[ SYSTEM_EYEGLOW_LEFT ]->m_pParticleSystem->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
	}
	if ( m_aParticleSystems[ SYSTEM_EYESPARK_LEFT ])
	{
		m_aParticleSystems[ SYSTEM_EYESPARK_LEFT ]->m_pParticleSystem->SetControlPoint( CUSTOM_COLOR_CP1, vColor );
	}

	m_bUpdateEyeGlows = false;
	m_bPlaySparks = false;

	// remove all particles that are not up-to-date before simulating the updated ones in the base
	for ( int i = 0; i < ARRAYSIZE( m_aParticleSystems ); i++ )
	{
		if ( m_aParticleSystems[i] && !m_aParticleSystems[i]->m_bIsUpdateToDate )
		{
			SafeDeleteParticleData( &m_aParticleSystems[i] );
		}
	}

	BaseClass::PostPaint3D( pRenderContext );
}

//-----------------------------------------------------------------------------
// Purpose : Called by base Mdlpanel when a merged mdl has been drawn
// For TF we use this as a way to render effects on top of model as appropriate (ie Unusual effects)
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::RenderingRootModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix )
{
	if ( !m_bUseParticle )
		return;

	// Eye Glows
	UpdateEyeGlows( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix, true );
	UpdateEyeGlows( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix, false );

	// Right hand
	UpdateActionSlotEffects( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix );
	
	// Taunt Effects
	UpdateTauntEffects( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix );
}

CEconItemView *CTFPlayerModelPanel::GetLoadoutItemFromMDLHandle( loadout_positions_t iPosition, MDLHandle_t mdlHandle )
{
	// Check if we have a particle hat, if not ignore
	CEconItemView *pEconItem = NULL;

	const char *pszModelName = vgui::MDLCache()->GetModelName( mdlHandle );

	// Find this item
	FOR_EACH_VEC( m_ItemsToCarry, i )
	{
		CEconItemView *pItem = m_ItemsToCarry[i];
		int iLoadoutSlot = pItem->GetStaticData()->GetLoadoutSlot( m_iCurrentClassIndex );
		if ( ( IsMiscSlot( iLoadoutSlot ) && IsMiscSlot( iPosition ) ) ||
			 ( IsValidPickupWeaponSlot( iLoadoutSlot ) && iLoadoutSlot == iPosition ) )
		{
			const char * pDisplayModel = pItem->GetPlayerDisplayModel( m_iCurrentClassIndex, m_iTeam );
			if ( pDisplayModel )
			{
				// compare the model to make sure that this is the same item
				if ( !V_strcmp( pszModelName, pDisplayModel )  )
				{
					pEconItem = pItem;
					break;
				}
			}
		}
	}

	return pEconItem;
}

//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::RenderingMergedModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix )
{
	if ( !m_bUseParticle )
		return;
	
	static struct MergeModelSlot_t
	{
		loadout_positions_t iPosition;
		modelpanel_particle_system_t iSystem;
	} s_mergeModelSlot[] =
	{
		{ LOADOUT_POSITION_HEAD, SYSTEM_HEAD },
		{ LOADOUT_POSITION_MISC, SYSTEM_MISC1 },
		{ LOADOUT_POSITION_MISC2, SYSTEM_MISC2 },
		{ LOADOUT_POSITION_PRIMARY, SYSTEM_WEAPON },
		{ LOADOUT_POSITION_SECONDARY, SYSTEM_WEAPON },
		{ LOADOUT_POSITION_MELEE, SYSTEM_WEAPON },
	};

	modelpanel_particle_system_t iSystem = SYSTEM_HEAD;
	loadout_positions_t iPosition = LOADOUT_POSITION_INVALID;
	CEconItemView *pEconItem = NULL;
	int count = ARRAYSIZE( s_mergeModelSlot );
	for ( int i=0; i<count; ++i )
	{
		// find the item for this model
		pEconItem = GetLoadoutItemFromMDLHandle( s_mergeModelSlot[i].iPosition, mdlHandle );
		if ( pEconItem )
		{
			iPosition = s_mergeModelSlot[i].iPosition;
			iSystem = s_mergeModelSlot[i].iSystem;
			
			// this is horrible but this fixes multiple unusual cosmetics with same default loadout to update their particles
			if ( m_aParticleSystems[ iSystem ] && m_aParticleSystems[ iSystem ]->m_bIsUpdateToDate )
				continue;

			break;
		}
	}

	// couldn't find matching item for this model, do nothing
	if ( !pEconItem )
		return;

	// Unusual Particles
	// Update Misc Particles 1 by 1, Unfortunately the equip location is generic (MISC_SLOT) and not the specific slot
	// so we have to test each slot individually
	UpdateCosmeticParticles( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix, iSystem, pEconItem );

	if ( m_iCurrentSlotIndex == iPosition )
	{
		RenderStatTrack( pStudioHdr, pWorldMatrix );
	}
}

IMaterial* CTFPlayerModelPanel::GetOverrideMaterial( MDLHandle_t mdlHandle ) 
{
	loadout_positions_t s_iPosition[] = {
		LOADOUT_POSITION_HEAD,
		LOADOUT_POSITION_MISC,
		LOADOUT_POSITION_MISC2,
		LOADOUT_POSITION_PRIMARY,
		LOADOUT_POSITION_SECONDARY,
		LOADOUT_POSITION_MELEE
	};

	int count = ARRAYSIZE( s_iPosition );
	for ( int i = 0; i < count; ++i ) 
	{
		CEconItemView *pEconItem = GetLoadoutItemFromMDLHandle( s_iPosition[ i ], mdlHandle );
		if ( pEconItem )
			return pEconItem->GetMaterialOverride( m_iTeam );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::RenderStatTrack( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix )
{
	// Draw the merge MDLs.
	if ( !m_StatTrackModel.m_bDisabled )
	{
		matrix3x4_t matMergeBoneToWorld[MAXSTUDIOBONES];

		// Get the merge studio header.
		CStudioHdr *pStatTrackStudioHdr = m_StatTrackModel.m_pStudioHdr;
		matrix3x4_t *pMergeBoneToWorld = &matMergeBoneToWorld[0];

		// If we have a valid mesh, bonemerge it. If we have an invalid mesh we can't bonemerge because
		// it'll crash trying to pull data from the missing header.
		if ( pStatTrackStudioHdr != NULL )
		{
			CStudioHdr &mergeHdr = *m_RootMDL.m_pStudioHdr;
			m_StatTrackModel.m_MDL.SetupBonesWithBoneMerge( &mergeHdr, pMergeBoneToWorld, pStudioHdr, pWorldMatrix, m_StatTrackModel.m_MDLToWorld );
			for ( int i=0; i<mergeHdr.numbones(); ++i )
			{
				MatrixScaleBy( m_flStatTrackScale, pMergeBoneToWorld[i] );
			}
			m_StatTrackModel.m_MDL.Draw( m_StatTrackModel.m_MDLToWorld, pMergeBoneToWorld );
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::UpdateCosmeticParticles( 
	IMatRenderContext				*pRenderContext, 
	CStudioHdr						*pStudioHdr, 
	MDLHandle_t						mdlHandle, 
	matrix3x4_t						*pWorldMatrix,
	modelpanel_particle_system_t	iSystem,
	CEconItemView *pEconItem
)
{
	if ( m_aParticleSystems[ iSystem ] && m_aParticleSystems[ iSystem ]->m_bIsUpdateToDate )
		return false;

	attachedparticlesystem_t *pParticleSystem = NULL;

	static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
	uint32 iValue = 0;
	if ( pEconItem->FindAttribute( pAttrDef_AttachParticleEffect, &iValue ) )
	{
		const float& value_as_float = (float&)iValue;
		pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( value_as_float );
	}

	if ( !pParticleSystem )
	{
		// do community_sparkle effect if this is a community item?
		const int iQualityParticleType = pEconItem->GetQualityParticleType();
		if ( iQualityParticleType > 0 )
		{
			pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iQualityParticleType );
		}
	}

	// failed to find any particle effect
	if ( !pParticleSystem )
	{
		return false;
	}

	// Team Color
	if ( GetTeam() == TF_TEAM_BLUE && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_red" ))
	{
		static char pBlue[256];
		V_StrSubst( pParticleSystem->pszSystemName, "_teamcolor_red", "_teamcolor_blue", pBlue, 256 );
		pParticleSystem = GetItemSchema()->FindAttributeControlledParticleSystem( pBlue );
		if ( !pParticleSystem )
		{
			return false;
		}
	}

	// if this thing has a bip_head or prp_helmet (aka a hat)
	int iBone = Studio_BoneIndexByName( pStudioHdr, "bip_head" );
	if ( iBone < 0 )
	{
		iBone = Studio_BoneIndexByName( pStudioHdr, "prp_helmet" );
		if ( iBone < 0 )
		{
			iBone = Studio_BoneIndexByName( pStudioHdr, "prp_hat" );
		}
	}
	
	// default to root
	if ( iBone < 0 ) 
	{
		iBone = 0;
	}

	// Get Use Head Origin
	CUtlVector< int > vecAttachments;
	static CSchemaAttributeDefHandle pAttrDef_UseHead( "particle effect use head origin" );
	uint32 iUseHead = 0;
	if ( !pEconItem->FindAttribute( pAttrDef_UseHead, &iUseHead ) || !iUseHead == 0 )
	{
		// not using head? try searching for attachment points
		for ( int i=0; i<ARRAYSIZE( pParticleSystem->pszControlPoints ); ++i )
		{
			const char *pszAttachmentName = pParticleSystem->pszControlPoints[i];
			if ( pszAttachmentName && pszAttachmentName[0] )
			{
				int iAttachment = Studio_FindAttachment( pStudioHdr, pszAttachmentName );
				if ( iAttachment < 0 )
					continue;

				vecAttachments.AddToTail( iAttachment );
			}
		}
	}

	static char pszFullname[256];
	const char* pszSystemName = pParticleSystem->pszSystemName;
	// Weapon Remap for a Base Effect to be used on a specific weapon
	if ( pParticleSystem->bUseSuffixName && pEconItem && pEconItem->GetItemDefinition()->GetParticleSuffix() )
	{
		V_strcpy_safe( pszFullname, pParticleSystem->pszSystemName );
		V_strcat_safe( pszFullname, "_" );
		V_strcat_safe( pszFullname, pEconItem->GetItemDefinition()->GetParticleSuffix() );
		pszSystemName = pszFullname;
	}

	// Update the Particles and render them
	if ( m_aParticleSystems[ iSystem ] )
	{
		// Check if its a new particle system
		if ( V_strcmp( m_aParticleSystems[ iSystem ]->m_pParticleSystem->GetName(), pszSystemName ) )
		{
			SafeDeleteParticleData( &m_aParticleSystems[ iSystem ] );
			m_aParticleSystems[ iSystem ] = CreateParticleData( pszSystemName );
		}
	}
	else
	{
		// create
		m_aParticleSystems[ iSystem ] = CreateParticleData( pszSystemName );
	}

	// Particle system does not exist
	if ( !m_aParticleSystems[ iSystem ] )
		return false;

	// Get offset if it exists (and if we're using head offset)
	static CSchemaAttributeDefHandle pAttrDef_VerticalOffset( "particle effect vertical offset" );
	uint32 iOffset = 0;
	Vector vecParticleOffset( 0, 0, 0 );
	if ( iUseHead > 0 && pEconItem->FindAttribute( pAttrDef_VerticalOffset, &iOffset ) )
	{
		vecParticleOffset.z = (float&)iOffset;
	}

	m_aParticleSystems[ iSystem ]->UpdateControlPoints( pStudioHdr, pWorldMatrix, vecAttachments, iBone, vecParticleOffset );
	return true;
}


//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::UpdateEyeGlows( 
	IMatRenderContext *pRenderContext, 
	CStudioHdr *pStudioHdr, 
	MDLHandle_t mdlHandle, 
	matrix3x4_t *pWorldMatrix, 
	bool bIsRightEye
) {
	float flOffset = 0;
	modelpanel_particle_system_t eyeSystem = bIsRightEye ? SYSTEM_EYEGLOW_RIGHT : SYSTEM_EYEGLOW_LEFT;
	modelpanel_particle_system_t sparkSystem = bIsRightEye ? SYSTEM_EYESPARK_RIGHT : SYSTEM_EYESPARK_LEFT;
	const char* pszAttach = bIsRightEye ? "eyeglow_R" : "eyeglow_L";

	// is this a model we care about?
	int iAttachment = Studio_FindAttachment( pStudioHdr, pszAttach );
	if ( iAttachment == INVALID_PARTICLE_ATTACHMENT || iAttachment == -1 )
		return;

	if ( m_bUpdateEyeGlows )
	{
		const char* pszGlowEffectName = m_pszEyeGlowParticleName;

		// kill old effects
		SafeDeleteParticleData( &m_aParticleSystems[ eyeSystem ] );

		if ( !bIsRightEye && GetPlayerClass() == TF_CLASS_DEMOMAN )
		{
			// demo man has a green eyeglow for eyelander if applicable
			C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pPlayer )
			{
				int iDecaps = pPlayer->m_Shared.GetDecapitations();
				pszGlowEffectName = pPlayer->GetDemomanEyeEffectName( iDecaps );
			}
		}

		if ( pszGlowEffectName && pszGlowEffectName[0] != '\0' )
		{
			m_aParticleSystems[ eyeSystem ] = CreateParticleData( pszGlowEffectName );
		}
	}

	if ( m_bPlaySparks && !m_vEyeGlowColor1.IsZero() )
	{
		SafeDeleteParticleData( &m_aParticleSystems[ sparkSystem ] );

		// Generate an eye spark as well not for demo
		m_aParticleSystems[ sparkSystem ] = CreateParticleData( "killstreak_t0_lvl1_flash" );
	}

	// Tick Update on position
	if ( m_aParticleSystems[ eyeSystem ] || m_aParticleSystems[ sparkSystem ] )
	{
		// Figure out where our attach point is
		matrix3x4_t matAttachToWorld;

		CUtlVector< int > vecAttachments;
		vecAttachments.AddToTail( iAttachment );

		// Update control points which is updating the position of the particles
		Vector vecForward;
		MatrixGetColumn( matAttachToWorld, 0, vecForward );

		Vector vecParticleOffset = vecForward * flOffset;
		if ( m_aParticleSystems[ eyeSystem ] )
		{
			m_aParticleSystems[ eyeSystem ]->UpdateControlPoints( pStudioHdr, pWorldMatrix, vecAttachments, 0, vecParticleOffset );
		}
		
		if ( m_aParticleSystems[ sparkSystem ] )
		{
			m_aParticleSystems[ sparkSystem ]->UpdateControlPoints( pStudioHdr, pWorldMatrix, vecAttachments, 0, vecParticleOffset );
		}
	}
}

//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::UpdateActionSlotEffects( 
	IMatRenderContext *pRenderContext, 
	CStudioHdr *pStudioHdr, 
	MDLHandle_t mdlHandle, 
	matrix3x4_t *pWorldMatrix
) {
	// is this a model we care about?
	int iAttachment = Studio_FindAttachment( pStudioHdr, "effect_hand_R" );
	if ( iAttachment == INVALID_PARTICLE_ATTACHMENT || iAttachment == -1 )
		return;

	if ( !m_bDrawActionSlotEffects )
		return;

	if ( !m_aParticleSystems[ SYSTEM_ACTIONSLOT ] )
	{
		m_aParticleSystems[ SYSTEM_ACTIONSLOT ] = CreateParticleData( CTFSpellBook::GetHandEffect( m_pHeldItem, 0 ) );
	}

	if ( !m_aParticleSystems[ SYSTEM_ACTIONSLOT ] )
		return;

	CUtlVector< int > vecAttachments;
	vecAttachments.AddToTail( iAttachment );

	m_aParticleSystems[ SYSTEM_ACTIONSLOT ]->UpdateControlPoints( pStudioHdr, pWorldMatrix, vecAttachments );
}

//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::UpdateTauntEffects(
	IMatRenderContext *pRenderContext,
	CStudioHdr *pStudioHdr,
	MDLHandle_t mdlHandle,
	matrix3x4_t *pWorldMatrix
	) {
	if ( !m_bDrawTauntParticles )
		return;

	if ( !m_aParticleSystems[SYSTEM_TAUNT] )
		return;

	// Check if refire is needed
	if ( m_flTauntParticleRefireRate > 0 && m_flTauntParticleRefireTime < gpGlobals->curtime )
	{
		m_flTauntParticleRefireTime = gpGlobals->curtime + m_flTauntParticleRefireRate;

		// safe off current particle name
		CUtlString strParticleName = m_aParticleSystems[SYSTEM_TAUNT]->m_pParticleSystem->GetName();

		// remove old particle
		SafeDeleteParticleData( &m_aParticleSystems[SYSTEM_TAUNT] );

		// create new particle
		m_aParticleSystems[SYSTEM_TAUNT] = CreateParticleData( strParticleName.String() );
	}

	matrix3x4_t matAttachToWorld;
	SetIdentityMatrix( matAttachToWorld );

	CUtlVector< int > vecAttachments;
	m_aParticleSystems[SYSTEM_TAUNT]->UpdateControlPoints( pStudioHdr, &matAttachToWorld, vecAttachments, 0, m_vecPlayerPos );
}

//-----------------------------------------------------------------------------
// Called Externally
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetEyeGlowEffect( const char *pEffectName, Vector vColor1, Vector vColor2, bool bForceUpdate, bool bPlaySparks )
{
	m_vEyeGlowColor1 = vColor1;
	m_vEyeGlowColor2 = vColor2;
	m_bPlaySparks = bPlaySparks;

	if ( bForceUpdate )
	{
		m_bUpdateEyeGlows = true;
	}

	if ( !pEffectName )
	{
		if ( m_pszEyeGlowParticleName[0] != '\0' )
		{
			m_bUpdateEyeGlows = true;
		}
		m_pszEyeGlowParticleName[0] = '\0';
	}
	else if ( !FStrEq( m_pszEyeGlowParticleName, pEffectName) )
	{
		V_strcpy_safe( m_pszEyeGlowParticleName, pEffectName );
		m_bUpdateEyeGlows = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: clear all particles
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::InvalidateParticleEffects()
{
	for ( int i=0; i<ARRAYSIZE(m_aParticleSystems); ++i )
	{
		if ( m_aParticleSystems[i] )
		{
			SafeDeleteParticleData( &m_aParticleSystems[i] );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg( "Got STARTEVENT at %.2f\n", currenttime );
	//Msg( "%8.4f:  start %s\n", currenttime, event->GetDescription() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SEQUENCE:
		ProcessSequence( scene, event );
		break;

	case CChoreoEvent::SPEAK:
		{
			if ( m_bDisableSpeakEvent )
				return;

			// FIXME: dB hack.  soundlevel needs to be moved into inside of wav?
			soundlevel_t iSoundlevel = SNDLVL_TALKING;
			if ( event->GetParameters2() )
			{
				iSoundlevel = (soundlevel_t)atoi( event->GetParameters2() );
				if ( iSoundlevel == SNDLVL_NONE )
				{
					iSoundlevel = SNDLVL_TALKING;
				}
			}

			float time_in_past = currenttime - event->GetStartTime() ;
			float soundtime = gpGlobals->curtime - time_in_past;

			EmitSound_t es;
			es.m_nChannel = CHAN_VOICE;
			es.m_flVolume = 1;
			es.m_SoundLevel = iSoundlevel;
			es.m_flSoundTime = soundtime;
			es.m_bEmitCloseCaption = false;
			es.m_pSoundName = event->GetParameters();

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, es );
		}
		break;

	case CChoreoEvent::STOPPOINT:
		{
			// Nothing, this is a symbolic event for keeping the vcd alive for ramping out after the last true event
			//ClearScene();
		}
		break;

	case CChoreoEvent::LOOP:
		ProcessLoop( scene, event );
		break;

	// Not supported in TF2's model previews
	case CChoreoEvent::SUBSCENE:
	case CChoreoEvent::SECTION:
		{
			Assert(0);
		}
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg( "Got ENDEVENT at %.2f\n", currenttime );
	//Msg( "%8.4f:  end %s %i\n", currenttime, event->GetDescription(), event->GetType() );

	switch ( event->GetType() )
	{
	case CChoreoEvent::SUBSCENE:
		{
			// Not supported in TF2's model previews
			Assert(0);
		}
		break;
	case CChoreoEvent::SPEAK:
		{
		}
		break;
	case CChoreoEvent::STOPPOINT:
		{
			//SetSequenceLayers( NULL, 0 );
			//ClearScene();
		}
		break;
	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event || !event->GetActive() )
		return;

	CChoreoActor *actor = event->GetActor();
	if ( actor && !actor->GetActive() )
		return;

	CChoreoChannel *channel = event->GetChannel();
	if ( channel && !channel->GetActive() )
		return;

	//Msg("PROCESSEVENT at %.2f\n", currenttime );

	switch( event->GetType() )
	{
	case CChoreoEvent::EXPRESSION:
		if ( !m_bShouldRunFlexEvents )
		{
			ProcessFlexSettingSceneEvent( scene, event );
		}
		break;

	case CChoreoEvent::FLEXANIMATION:
		if ( m_bShouldRunFlexEvents )
		{
			ProcessFlexAnimation( scene, event );
		}
		break;

	case CChoreoEvent::SEQUENCE:
	case CChoreoEvent::SPEAK:
	case CChoreoEvent::STOPPOINT:
		// Nothing
		break;

	// Not supported in TF2's model previews
	case CChoreoEvent::LOOKAT:
	case CChoreoEvent::FACE:
	case CChoreoEvent::SUBSCENE:
	case CChoreoEvent::MOVETO:
	case CChoreoEvent::INTERRUPT:
	case CChoreoEvent::PERMIT_RESPONSES:
	case CChoreoEvent::GESTURE:
		Assert(0);
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Apply a sequence
// Input  : *event - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessSequence( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::SEQUENCE );

	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	if ( !event->GetActor() )
		return;

	int iSequence = LookupSequence( &studioHdr, event->GetParameters() );
	if (iSequence < 0)
		return;

	// making sure the mdl has correct playback rate
	mstudioseqdesc_t &seqdesc = studioHdr.pSeqdesc( iSequence );
	mstudioanimdesc_t &animdesc = studioHdr.pAnimdesc( studioHdr.iRelativeAnim( iSequence, seqdesc.anim(0,0) ) );
	m_RootMDL.m_MDL.m_flPlaybackRate = animdesc.fps;

	MDLSquenceLayer_t	tmpSequenceLayers[1];
	tmpSequenceLayers[0].m_nSequenceIndex = iSequence;
	tmpSequenceLayers[0].m_flWeight = 1.0;
	tmpSequenceLayers[0].m_bNoLoop = true;
	tmpSequenceLayers[0].m_flCycleBeganAt = m_RootMDL.m_MDL.m_flTime;
	SetSequenceLayers( tmpSequenceLayers, 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *scene - 
//			*event - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOP );

	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	//Msg("LOOP: %.2f (%.2f)\n", m_flSceneTime, scene->GetTime() );

	float flPrevTime = m_flSceneTime;
	scene->LoopToTime( backtime );
	m_flSceneTime = backtime;

	//Msg("   -> %.2f (%.2f)\n", m_flSceneTime, scene->GetTime() );

	float flDelta = flPrevTime - backtime;

	//Msg("	-> Delta %.2f\n", flDelta );

	// If we're running noloop sequences, we need to push out their begin time, so they keep playing
	for ( int i = 0; i < m_nNumSequenceLayers; i++ )
	{
		if ( m_SequenceLayers[i].m_bNoLoop )
		{
			m_SequenceLayers[i].m_flCycleBeganAt += flDelta;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LocalFlexController_t CTFPlayerModelPanel::GetNumFlexControllers( void )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	return studioHdr.numflexcontrollers();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexDescFacs( int iFlexDesc )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexdesc_t *pflexdesc = studioHdr.pFlexdesc( iFlexDesc );

	return pflexdesc->pszFACS( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFPlayerModelPanel::GetFlexControllerType( LocalFlexController_t iFlexController )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

	mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( iFlexController );

	return pflexcontroller->pszType( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
LocalFlexController_t CTFPlayerModelPanel::FindFlexController( const char *szName )
{
	for (LocalFlexController_t i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		if (stricmp( GetFlexControllerName( i ), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t(-1);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetFlexWeight( LocalFlexController_t index, float value )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

		mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			value = (value - pflexcontroller->min) / (pflexcontroller->max - pflexcontroller->min);
			value = clamp( value, 0.0f, 1.0f );
		}

		m_flexWeight[ index ] = value;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerModelPanel::GetFlexWeight( LocalFlexController_t index )
{
	if (index >= 0 && index < GetNumFlexControllers())
	{
		CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;

		mstudioflexcontroller_t *pflexcontroller = studioHdr.pFlexcontroller( index );

		if (pflexcontroller->max != pflexcontroller->min)
		{
			return m_flexWeight[index] * (pflexcontroller->max - pflexcontroller->min) + pflexcontroller->min;
		}

		return m_flexWeight[index];
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: During paint, apply the flex weights to the model
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetupFlexWeights( void )
{
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	// initialize the models local to global flex controller mappings
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	if (studioHdr.pFlexcontroller( LocalFlexController_t(0) )->localToGlobal == -1)
	{
		for ( LocalFlexController_t i = LocalFlexController_t(0); i < studioHdr.numflexcontrollers(); i++)
		{
			int j = C_BaseFlex::AddGlobalFlexController( studioHdr.pFlexcontroller( i )->pszName() );
			studioHdr.pFlexcontroller( i )->localToGlobal = j;
		}
	}

	int iControllers = GetNumFlexControllers();
	for ( int j = 0; j < iControllers; j++ )
	{
		m_RootMDL.m_MDL.m_pFlexControls[j] = 0;
	}

	LocalFlexController_t i;

	// Decay to neutral
	for ( i = LocalFlexController_t(0); i < GetNumFlexControllers(); i++)
	{
		SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
	}

	// Run scene
	if ( m_pScene )
	{
		m_bShouldRunFlexEvents = true;
		m_pScene->Think( m_flSceneTime );
	}

	// get the networked flexweights and convert them from 0..1 to real dynamic range
	for (i = LocalFlexController_t(0); i < studioHdr.numflexcontrollers(); i++)
	{
		mstudioflexcontroller_t *pflex = studioHdr.pFlexcontroller( i );

		m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] = m_flexWeight[i];
		// rescale
		m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] = m_RootMDL.m_MDL.m_pFlexControls[pflex->localToGlobal] * (pflex->max - pflex->min) + pflex->min;
	}

	if ( m_pScene )
	{
		m_bShouldRunFlexEvents = false;
		m_pScene->Think( m_flSceneTime );
	}

	ProcessVisemes( m_PhonemeClasses );

	if ( m_pScene )
	{
		// Advance time
		if ( m_flLastTickTime < FLT_EPSILON )
		{
			m_flLastTickTime = m_RootMDL.m_MDL.m_flTime - 0.1;
		}

		m_flSceneTime += (m_RootMDL.m_MDL.m_flTime - m_flLastTickTime);
		m_flLastTickTime = m_RootMDL.m_MDL.m_flTime;

		if ( m_flSceneEndTime > FLT_EPSILON && m_flSceneTime > m_flSceneEndTime )
		{
			bool bLoopScene = m_bLoopScene;
			char filename[MAX_PATH];
			V_strcpy_safe( filename, m_pScene->GetFilename() );

			SetSequenceLayers( NULL, 0 );
			ClearScene();

			if ( bLoopScene )
			{
				m_pScene = LoadSceneForModel( filename, this, &m_flSceneEndTime );
			}
			else
			{
				m_pszVCD = NULL;
			}
		}
	}
}

extern CFlexSceneFileManager g_FlexSceneFileManager;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessExpression( CChoreoScene *scene, CChoreoEvent *event )
{
	// Flexanimations have to have an end time!!!
	if ( !event->HasEndTime() )
		return;

	// Look up the actual strings
	const char *scenefile	= event->GetParameters();
	const char *name		= event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = ( const flexsettinghdr_t * )g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();

			float flIntensity = event->GetIntensity( scenetime );

			int i;
			const flexsetting_t *pSetting = NULL;

			// Find the named setting in the base
			for ( i = 0; i < pExpHdr->numflexsettings; i++ )
			{
				pSetting = pExpHdr->pSetting( i );
				if ( !pSetting )
					continue;

				if ( !V_stricmp( pSetting->pszName(), name ) )
					break;
			}

			if ( i>=pExpHdr->numflexsettings )
				return;

			flexweight_t *pWeights = NULL;
			int truecount = pSetting->psetting( (byte *)pExpHdr, 0, &pWeights );
			if ( !pWeights )
				return;

			for (i = 0; i < truecount; i++, pWeights++)
			{
				int j = FlexControllerLocalToGlobal( pExpHdr, pWeights->key );

				float s = clamp( pWeights->influence * flIntensity, 0.0f, 1.0f );
				m_RootMDL.m_MDL.m_pFlexControls[ j ] = m_RootMDL.m_MDL.m_pFlexControls[j] * (1.0f - s) + pWeights->weight * s;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event )
{
	// Flexanimations have to have an end time!!!
	if ( !event->HasEndTime() )
		return;

	VPROF( "C_BaseFlex::ProcessFlexSettingSceneEvent" );

	// Look up the actual strings
	const char *scenefile	= event->GetParameters();
	const char *name		= event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = ( const flexsettinghdr_t * )g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			float scenetime = scene->GetTime();

			float scale = event->GetIntensity( scenetime );

			// Add the named expression
			AddFlexSetting( name, scale, pExpHdr );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		const char *name = pSetting->pszName();

		if ( !V_stricmp( name, expr ) )
			break;
	}

	if ( i>=pSettinghdr->numflexsettings )
	{
		return;
	}

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for (i = 0; i < truecount; i++, pWeights++)
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

		// blend scaled weighting in to total (post networking g_flexweight!!!!)
		float s = clamp( scale * pWeights->influence, 0.0f, 1.0f );
		m_RootMDL.m_MDL.m_pFlexControls[index] = m_RootMDL.m_MDL.m_pFlexControls[index] * (1.0f - s) + pWeights->weight * s;

		for ( int iMergeMDL=0; iMergeMDL<m_aMergeMDLs.Count(); ++iMergeMDL )
		{
			m_aMergeMDLs[iMergeMDL].m_MDL.m_pFlexControls[index] = m_aMergeMDLs[iMergeMDL].m_MDL.m_pFlexControls[index] * (1.0f - s) + pWeights->weight * s;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Apply flexanimation to actor's face
// Input  : *event - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::FLEXANIMATION );

	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	CStudioHdr *hdr = &studioHdr;
	if ( !hdr )
		return;

	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[ 512 ];
				Q_strncpy( name, "right_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 0 );

				Q_strncpy( name, "left_" ,sizeof(name));
				Q_strncat( name, track->GetFlexControllerName(),sizeof(name), COPY_ALL_CHARACTERS );

				track->SetFlexControllerIndex( MAX( FindFlexController( name ), LocalFlexController_t(0) ), 0, 1 );
			}
			else
			{
				track->SetFlexControllerIndex( MAX( FindFlexController( (char *)track->GetFlexControllerName() ), LocalFlexController_t(0)), 0 );
			}
		}

		event->SetTrackLookupSet( true );
	}

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				LocalFlexController_t controller = track->GetRawFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= LocalFlexController_t(0) )
				{
					float orig = GetFlexWeight( controller );
					float value = orig * (1 - weight) + flIntensity * weight;
					SetFlexWeight( controller, value );
				}
			}
		}
		else
		{
			LocalFlexController_t controller = track->GetRawFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= LocalFlexController_t(0) )
			{
				float orig = GetFlexWeight( controller );
				float value = orig * (1 - weight) + flIntensity * weight;
				SetFlexWeight( controller, value );
			}
		}
	}
}

extern ConVar g_CV_PhonemeDelay;
extern ConVar g_CV_PhonemeFilter;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	if ( !MouthInfo().IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0 ; source < MouthInfo().GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = MouthInfo().GetVoiceSource( source );
		if ( !vd || vd->ShouldIgnorePhonemes() )
			continue;

		CSentence *sentence = engine->GetSentence( vd->GetSource() );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->GetSource() );
		float	timesincestart = vd->GetElapsedTime();

		// This sound should be done...why hasn't it been removed yet???
		if ( timesincestart >= ( sentence_length + 2.0f ) )
			continue;

		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();

		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Streaming sounds get an additional delay...
		/*
		// Tracker 20534:  Probably not needed any more with the async sound stuff that
		//  we now have (we don't have a disk i/o hitch on startup which might have been
		//  messing up the startup timing a bit )
		bool streaming = engine->IsStreaming( vd->m_pAudioSource );
		if ( streaming )
		{
			t -= g_CV_PhonemeDelayStreaming.GetFloat();
		}
		*/

		// Assume sound has been playing for a while...
		bool juststarted = false;

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	int pcount = sentence->GetRuntimePhonemeCount();
	for ( int k = 0; k < pcount; k++ )
	{
		const CBasePhonemeTag *phoneme = sentence->GetRuntimePhoneme( k );

		if (t > phoneme->GetStartTime() && t < phoneme->GetEndTime())
		{
			bool bCrossfade = true;
			if (bCrossfade)
			{
				if (k < pcount-1)
				{
					const CBasePhonemeTag *next = sentence->GetRuntimePhoneme( k + 1 );
					// if I have a neighbor
					if ( next )
					{
						//  and they're touching
						if (next->GetStartTime() == phoneme->GetEndTime() )
						{
							// no gap, so increase the blend length to the end of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetEndTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
						else
						{
							// dead space, so increase the blend length to the start of the next phoneme, as long as it's not longer than the current phoneme
							dt = MAX( dt, MIN( next->GetStartTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
						}
					}
					else
					{
						// last phoneme in list, increase the blend length to the length of the current phoneme
						dt = MAX( dt, phoneme->GetEndTime() - phoneme->GetStartTime() );
					}
				}
			}
		}

		float t1 = ( phoneme->GetStartTime() - t) / dt;
		float t2 = ( phoneme->GetEndTime() - t) / dt;

		if (t1 < 1.0 && t2 > 0)
		{
			float scale;

			// clamp
			if (t2 > 1)
				t2 = 1;
			if (t1 < 0)
				t1 = 0;

			// FIXME: simple box filter.  Should use something fancier
			scale = (t2 - t1);

			AddViseme( classes, emphasis_intensity, phoneme->GetPhonemeCode(), scale, juststarted );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	CStudioHdr &studioHdr = *m_RootMDL.m_pStudioHdr;
	CStudioHdr *hdr = &studioHdr;
	if ( !hdr )
		return;

	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );

	phoneme = 230;
	scale = 1.0;

	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}

	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[ type ];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		const flexsettinghdr_t *actual_flexsetting_header = info->base;
		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if (!pSetting)
		{
			continue;
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for ( int i = 0; i < truecount; i++)
			{
				// Translate to global controller number
				int j = FlexControllerLocalToGlobal( actual_flexsetting_header, pWeights->key );
				// Add scaled weighting in
				if ( pWeights->weight > 0 )
				{
					m_RootMDL.m_MDL.m_pFlexControls[j] += info->amount * scale * pWeights->weight;
				}
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[ i ];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, info->classname, false );
		}
		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ('V' << 16) + ('F' << 8) + ('E') );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}
	}

	return skip;
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which blends are available for the current phoneme
	bool has_weak	= classes[ PHONEME_CLASS_WEAK ].valid;
	bool has_strong = classes[ PHONEME_CLASS_STRONG ].valid;

	// Better have phonemes in general
	Assert( classes[ PHONEME_CLASS_NORMAL ].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (frac) * 2.0f * STRONG_CROSSFADE_START;
			classes[ PHONEME_CLASS_STRONG ].amount = 1.0f - frac; 
		}
		else
		{
			emphasis_intensity = MIN( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[ PHONEME_CLASS_NORMAL ].amount = (1.0f - frac) * 2.0f * WEAK_CROSSFADE_START;
			classes[ PHONEME_CLASS_WEAK ].amount = frac; 
		}
		else
		{
			emphasis_intensity = MAX( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[ PHONEME_CLASS_NORMAL ].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::InitPhonemeMappings( void )
{
	CStudioHdr *pStudioHdr = m_RootMDL.m_pStudioHdr;
	if ( pStudioHdr && pStudioHdr->IsValid() )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudioHdr->pszName(), szBasename, sizeof( szBasename ) );

		char szExpressionName[MAX_PATH];
		Q_snprintf( szExpressionName, sizeof( szExpressionName ), "%s/phonemes/phonemes", szBasename );
		if ( g_FlexSceneFileManager.FindSceneFile( this, szExpressionName, false ) )
		{
			SetupMappings( szExpressionName );	
			return;
		}
	}

	SetupMappings( "phonemes" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetupMappings( char const *pchFileRoot )
{
	// Fill in phoneme class lookup
	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

	Emphasized_Phoneme *normal = &m_PhonemeClasses[ PHONEME_CLASS_NORMAL ];
	Q_snprintf( normal->classname, sizeof( normal->classname ), "%s", pchFileRoot );
	normal->required = true;

	Emphasized_Phoneme *weak = &m_PhonemeClasses[ PHONEME_CLASS_WEAK ];
	Q_snprintf( weak->classname, sizeof( weak->classname ), "%s_weak", pchFileRoot );
	Emphasized_Phoneme *strong = &m_PhonemeClasses[ PHONEME_CLASS_STRONG ];
	Q_snprintf( strong->classname, sizeof( strong->classname ), "%s_strong", pchFileRoot );
}

//-----------------------------------------------------------------------------
// Purpose: Since everyone shared a pSettinghdr now, we need to set up the localtoglobal mapping per entity, but 
//  we just do this in memory with an array of integers (could be shorts, I suppose)
// Input  : *pSettinghdr - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EnsureTranslations( const flexsettinghdr_t *pSettinghdr )
{
	Assert( pSettinghdr );

	FS_LocalToGlobal_t entry( pSettinghdr );

	unsigned short idx = m_LocalToGlobal.Find( entry );
	if ( idx != m_LocalToGlobal.InvalidIndex() )
		return;

	entry.SetCount( pSettinghdr->numkeys );

	for ( int i = 0; i < pSettinghdr->numkeys; ++i )
	{
		entry.m_Mapping[ i ] = C_BaseFlex::AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	m_LocalToGlobal.Insert( entry );
}

//-----------------------------------------------------------------------------
// Purpose: Look up instance specific mapping
// Input  : *pSettinghdr - 
//			key - 
// Output : int
//-----------------------------------------------------------------------------
int CTFPlayerModelPanel::FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key )
{
	FS_LocalToGlobal_t entry( pSettinghdr );

	int idx = m_LocalToGlobal.Find( entry );
	if ( idx == m_LocalToGlobal.InvalidIndex() )
	{
		// This should never happen!!!
		Assert( 0 );
		Warning( "Unable to find mapping for flexcontroller %i, settings %p on CTFPlayerModelPanel\n", key, pSettinghdr );
		EnsureTranslations( pSettinghdr );
		idx = m_LocalToGlobal.Find( entry );
		if ( idx == m_LocalToGlobal.InvalidIndex() )
		{
			Error( "CTFPlayerModelPanel::FlexControllerLocalToGlobal failed!\n" );
		}
	}

	FS_LocalToGlobal_t& result = m_LocalToGlobal[ idx ];
	// Validate lookup
	Assert( result.m_nCount != 0 && key < result.m_nCount );
	int index = result.m_Mapping[ key ];
	return index;
}

