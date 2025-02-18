//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include "item_model_panel.h"
#include "iclientmode.h"
#include "baseviewport.h"
#include "econ_entity.h"
#include "gamestringpool.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "econ_item_system.h"
#include "ienginevgui.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "renderparm.h"
#include "vgui_controls/ScalableImagePanel.h"
#include "engine/IEngineSound.h"
#include "econ/tool_items/tool_items.h"
#include "econ_item_description.h"
#include "econ_item_tools.h"
#include "tool_items/custom_texture_cache.h"
#include "econ_dynamic_recipe.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexturecompositor.h"
#include "bone_setup.h"
#include "animation.h"
#include "iconrenderreceiver.h"

#ifdef TF_CLIENT_DLL
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#endif // TF_CLIENT_DLL
#include "KeyValues.h"

ConVar tf_time_loading_item_panels( "tf_time_loading_item_panels", "0.0005", FCVAR_ARCHIVE, "The time to spend per frame loading data for item panels" );

const char* g_ItemModelPanelRenderTargetNames[] =
{
	"_rt_ItemModelPanel0",
	"_rt_ItemModelPanel1",
	"_rt_ItemModelPanel2"
};
COMPILE_TIME_ASSERT( ITEM_MODEL_IMAGE_CACHE_SIZE == ARRAYSIZE( g_ItemModelPanelRenderTargetNames ) );

CItemMaterialCustomizationIconPanel::CItemMaterialCustomizationIconPanel( vgui::Panel *pParent, const char *pName ) 
	: BaseClass( pParent, pName )
{
	m_iPaintSplat = -1;
}

CItemMaterialCustomizationIconPanel::~CItemMaterialCustomizationIconPanel()
{
	if ( vgui::surface() )
	{
		if ( m_iPaintSplat != -1 )
		{
			vgui::surface()->DestroyTextureID( m_iPaintSplat );
			m_iPaintSplat = -1;
		}
	}
}

// Custom painting
void CItemMaterialCustomizationIconPanel::PaintBackground( void )
{
	// Draw custom texture, if we have one
	if ( m_hUGCId != 0 )
	{
		// Request it from the cache, and get filename, if it's downloaded
		// and ready
		int iCustomTexture = GetCustomTextureGuiHandle( m_hUGCId );
		if ( iCustomTexture != 0 )
		{
			surface()->DrawSetTexture( iCustomTexture );
			DrawQuad( 0, 1 );
			surface()->DrawSetColor(COLOR_WHITE);
		}
	}

	for ( int i = 0; i < m_colPaintColors.Size(); i++ )
	{
		const Color& c = m_colPaintColors[i];

		if ( m_iPaintSplat == -1 )
		{
			m_iPaintSplat = surface()->CreateNewTextureID();
			surface()->DrawSetTextureFile( m_iPaintSplat, "vgui/backpack_jewel_paint_splatter", true, false);
		}
		surface()->DrawSetTexture( m_iPaintSplat );
		surface()->DrawSetColor( c.r(), c.g(), c.b(), GetAlpha() );
		DrawQuad( i, m_colPaintColors.Size() );
		surface()->DrawSetColor(COLOR_WHITE);
	}

	// Clean up
	vgui::surface()->DrawSetTexture(0);
}

// Draw a quad that fills our extents
void CItemMaterialCustomizationIconPanel::DrawQuad( int iSubtileIndex, int iSubtileCount )
{
	int iWide, iTall;
	GetSize( iWide, iTall );

	// All of this math is to accomplish the following: allow us to split our single "icon"
	// into some number of equivalent columns. Then take each column and angle the divider so
	// it goes from the left image to the right image:
	//
	//			+-----+-----+				+------+----+
	//			|	  |     |				|     /		|
	//			|	  |	    |				|     |		|
	//			|	  |	    |				|	  /		|
	//			+-----+-----+				+--- +------+
	//
	// ...because the angle is prettier than a straight vertical cut.
	//
	// My hope is that this code is so awful I'm never allowed to write UI code again.
	float fXScale   = 1.0f / (float)iSubtileCount,
			fXOffsetL = (float)iSubtileIndex * fXScale,
			fXOffsetR = (float)(iSubtileIndex + 1) * fXScale,
			fXUpperLowerOffset = fXScale * 0.65f;

	// We shift our coordinates on the top slightly to the right (by fXUpperLowerOffset) and on
	// the bottom slightly to the left (also by fXUpperLowerOffset). The far left side can't move
	// away from 0 and the far right side can't move away from 1, so the edge case handling makes
	// this look uglier than it really is.
	float fXUL = iSubtileIndex == 0 ? fXOffsetL : fXOffsetL + fXUpperLowerOffset,
			fXUR = iSubtileIndex == iSubtileCount - 1 ? fXOffsetR : fXOffsetR + fXUpperLowerOffset,
			fXBL = iSubtileIndex == 0 ? fXOffsetL : fXOffsetL - fXUpperLowerOffset,
			fXBR = iSubtileIndex == iSubtileCount - 1 ? fXOffsetR : fXOffsetR - fXUpperLowerOffset;

	Vector2D uv11( fXUL, 0.0f );
	Vector2D uv21( fXUR, 0.0f );
	Vector2D uv22( fXBR, 1.0f );
	Vector2D uv12( fXBL, 1.0f );

	vgui::Vertex_t verts[4];
	verts[0].Init( Vector2D( iWide * fXUL,	0 ),	 uv11 );
	verts[1].Init( Vector2D( iWide * fXUR, 0 ),	 uv21 );
	verts[2].Init( Vector2D( iWide * fXBR, iTall ), uv22 );
	verts[3].Init( Vector2D( iWide * fXBL,	iTall ), uv12  );

	vgui::surface()->DrawTexturedPolygon( 4, verts );	
}

DECLARE_BUILD_FACTORY( CItemModelPanel );
DECLARE_BUILD_FACTORY( CEmbeddedItemModelPanel );
DECLARE_BUILD_FACTORY( CItemMaterialCustomizationIconPanel );

item_model_cache_t g_ItemModelImageCache[ITEM_MODEL_IMAGE_CACHE_SIZE];

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEmbeddedItemModelPanel::CEmbeddedItemModelPanel( vgui::Panel *pParent, const char *pName ) : BaseClass( pParent, pName )
{
	m_bUseItemRenderTarget = false;
	m_bForceUseModel = false;
	m_pItem = NULL;
	m_pszToolTargetItemImage = NULL;
	m_iTextureID = -1;
	m_iToolTargetItemTextureID = -1;
	m_iOverlayTextureIDs.SetLessFunc( DefLessFunc(int) );
	m_iOverlayTextureIDs.Purge();
	m_bImageNotLoaded = false;
	m_bGreyedOut = false;
	m_bModelIsHidden = false;
	m_bUseRenderTargetAsIcon = false;

	m_bWeaponAllowInspect = false;
	m_pCachedWeaponIcon = NULL;
	m_pCachedWeaponMaterial = NULL;
	m_iCachedTextureID = -1;

	m_flModelRotateYawSpeed = 0;

	m_bUsePedestal = false;
	m_bOfflineIconGeneration = false;

	m_pItemParticle = NULL;

}


CEmbeddedItemModelPanel::~CEmbeddedItemModelPanel()
{
	CleanUpCachedWeaponIcon();

	SafeDeleteParticleData( &m_pItemParticle );
}


void CEmbeddedItemModelPanel::CleanUpCachedWeaponIcon()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	SafeRelease( &m_pCachedWeaponIcon );
	SafeRelease( &m_pCachedWeaponMaterial );

	if ( m_iCachedTextureID != -1 )
	{
		surface()->DeleteTextureByID( m_iCachedTextureID );
		m_iCachedTextureID = -1;
	}

	// If we match a cache here, clear it so we redraw once when we appear.
	for ( int i = 0; i < ITEM_MODEL_IMAGE_CACHE_SIZE; i++ )
	{
		bool bMatch = g_ItemModelImageCache[i].m_hModelPanelLock.Get() == this;
		if ( bMatch )
		{
			g_ItemModelImageCache[i].Clear();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::UpdateCameraForIcon()
{
	if ( m_iCameraAttachment == -1 )
		return;

	studiohdr_t *pItemStudioHdr = m_RootMDL.m_MDL.GetStudioHdr();
	if ( pItemStudioHdr )
	{
		matrix3x4_t matBoneToWorld[MAXSTUDIOBONES];
		m_RootMDL.m_MDL.SetUpBones( m_RootMDL.m_MDLToWorld, MAXSTUDIOBONES, matBoneToWorld );

		// Get attachment transform
		mstudioattachment_t attach = pItemStudioHdr->pAttachment( m_iCameraAttachment );
		matrix3x4_t matLocalToWorld;
		ConcatTransforms( matBoneToWorld[ attach.localbone ], attach.local, matLocalToWorld );

		QAngle angCameraAngles;
		Vector vecCameraPos;
		MatrixAngles( matLocalToWorld, angCameraAngles, vecCameraPos );
		SetCameraOffset( vec3_origin );
		SetCameraPositionAndAngles( vecCameraPos, angCameraAngles );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::SetItem( CEconItemView *pItem ) 
{ 
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	m_iTextureID = -1; 
	m_iToolTargetItemTextureID = -1;
	m_iOverlayTextureIDs.Purge();
	CleanUpCachedWeaponIcon();
	SafeDeleteParticleData( &m_pItemParticle );

	// reset all models
	SetMDL( MDLHANDLE_INVALID );
	m_ItemModel.m_bDisabled = true;
	m_ItemModel.m_MDL.SetMDL( MDLHANDLE_INVALID );
	m_StatTrackModel.m_bDisabled = true;
	m_StatTrackModel.m_MDL.SetMDL( MDLHANDLE_INVALID );

	m_AttachedModels.Purge();

	m_iCameraAttachment = -1;

	m_pItem = pItem;

	if ( !m_pItem )
		return;

	const char* pszInventoryImage = m_pItem->IsValid() ? m_pItem->GetInventoryImage() : NULL;
	if ( ( pszInventoryImage && pszInventoryImage[0] && !g_pMaterialSystem->IsMaterialLoaded( pszInventoryImage ) )
		)
	{
		m_bImageNotLoaded = true;
	}

	if ( !m_pItem->IsValid() )
		return;

	float flValue;
	static CSchemaAttributeDefHandle pAttrib_ToolTarget( "tool target item" );
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( m_pItem, pAttrib_ToolTarget, &flValue ) )
	{
		const CEconItemDefinition *pTargetDef = GetItemSchema()->GetItemDefinition( flValue );

		m_pszToolTargetItemImage = pTargetDef->GetInventoryImage();
	}
	else
	{
		m_pszToolTargetItemImage = NULL;
	}
   
	float flInspect = 0;
	static CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( m_pItem, pAttrib_WeaponAllowInspect, &flInspect )
		)
	{
		m_bWeaponAllowInspect = flInspect != 0;

	}
	else
	{
		m_bWeaponAllowInspect = false;
			
	}

	static CSchemaAttributeDefHandle pAttr_is_festivized( "is_festivized" );
	m_bIsFestivized = pAttr_is_festivized && m_pItem->FindAttribute( pAttr_is_festivized );

	m_bIsPaintKitItem = GetPaintKitDefIndex( m_pItem );

	m_bUseRenderTargetAsIcon = ShouldUseRenderTargetAsIcon();

	if ( !m_bModelIsHidden )
	{
		if ( !m_pItem->GetInventoryImage() || IsForcingModelUsage() || m_bWeaponAllowInspect || UseRenderTargetAsIcon() )
		{
			int nClass = 0;
			if ( m_pItem->GetItemDefinition() && m_pItem->GetItemDefinition()->GetClassUsability() )
			{
				for ( int i = 0; i < m_pItem->GetItemDefinition()->GetClassUsability()->GetNumBits(); i++ )
				{
					if ( m_pItem->GetItemDefinition()->GetClassUsability()->IsBitSet( i ) )
					{
						nClass = i;
						break;
					}
				}
			}

			const char *pszModelName = m_pItem->GetPlayerDisplayModel( nClass, 0 );
			if ( pszModelName )
			{
				CMDL *pMDL = NULL;
#ifndef PORTAL2 // DOTA COME BACK
				if ( m_bUsePedestal )
				{
					MDLHandle_t hPedestalMDL = mdlcache->FindMDL( "models/weapons/pedestal/pedestal.mdl" ); 
					SetMDL( hPedestalMDL, NULL );
					mdlcache->Release( hPedestalMDL ); // counterbalance addref from within FindMDL

					MDLHandle_t hItemMDL = mdlcache->FindMDL( pszModelName );
					if ( mdlcache->IsErrorModel( hItemMDL ) )
					{
						hItemMDL = MDLHANDLE_INVALID;
					}
					m_ItemModel.m_MDL.SetMDL( hItemMDL );
					mdlcache->Release( hItemMDL ); // counterbalance addref from within FindMDL

					pMDL = &m_ItemModel.m_MDL;
				}
				else
				{
					MDLHandle_t hMDL = mdlcache->FindMDL( pszModelName );
					SetMDL( hMDL, static_cast<IClientRenderable*>( m_pItem ) );
					mdlcache->Release( hMDL ); // counterbalance addref from within FindMDL

					pMDL = &m_RootMDL.m_MDL;
				}
#endif

				if ( pMDL )
				{
					studiohdr_t *pItemStudioHdr = pMDL->GetStudioHdr();
					if ( pItemStudioHdr )
					{
						// Get the appropriate attachment
						CStudioHdr HDR( pItemStudioHdr, g_pMDLCache );
						if ( m_bUsePedestal )
						{
							m_iPedestalAttachment = Studio_FindAttachment( &HDR, "pedestal_0" );
							if ( m_iPedestalAttachment != -1 )
							{
								m_ItemModel.m_MDL.m_pProxyData = static_cast<IClientRenderable*>(m_pItem);
								m_ItemModel.m_bDisabled = false;
								m_ItemModel.m_MDL.m_nSequence = ACT_IDLE;
								SetIdentityMatrix( m_ItemModel.m_MDLToWorld );
							}
						}
						else
						{
							m_iCameraAttachment = Studio_FindAttachment( &HDR, "icon_camera" );
							UpdateCameraForIcon();
						}

						// should we override this model bodygroup
						const CEconStyleInfo *pStyle = m_pItem->GetItemDefinition()->GetStyleInfo( m_pItem->GetStyle() );
						if ( pStyle && pStyle->GetBodygroupName() != NULL )
						{
							int iBodyGroup = ::FindBodygroupByName( &HDR, pStyle->GetBodygroupName() );
							if ( iBodyGroup != -1 )
							{
								::SetBodygroup( &HDR, pMDL->m_nBody, iBodyGroup, pStyle->GetBodygroupSubmodelIndex() );
							}
						}
					}
				}

				// Attach Models
				// Attach the models for the item
				{
					int iTeam = m_pItem->GetItemDefinition()->GetBestVisualTeamData( m_pItem->GetTeamNumber() );
					{
						// Set attached models if viewable third-person.
						const int iNumAttachedModels = m_pItem->GetItemDefinition()->GetNumAttachedModels( iTeam );
						for ( int i = 0; i < iNumAttachedModels; ++i )
						{
							attachedmodel_t	*pModel = m_pItem->GetItemDefinition()->GetAttachedModelData( iTeam, i );
							LoadAttachedModel( pModel );
						}
					}

					// Festive
					if ( m_bIsFestivized )
					{
						const int iNumAttachedModels = m_pItem->GetItemDefinition()->GetNumAttachedModelsFestivized( iTeam );
						for ( int i = 0; i < iNumAttachedModels; ++i )
						{
							attachedmodel_t	*pModel = m_pItem->GetItemDefinition()->GetAttachedModelDataFestivized( iTeam, i );
							LoadAttachedModel( pModel );
						}
					}
				}

				// Stattrak
				CAttribute_String attrModule;
				if ( GetStattrak( m_pItem, &attrModule ) )
				{
					// Allow for already strange items
					bool bIsStrange = false;
					if ( m_pItem->GetQuality() == AE_STRANGE || m_pItem->GetItemQuality() == AE_STRANGE )
					{
						bIsStrange = true;
					}

					if ( !bIsStrange )
					{
						// Go over the attributes of the item, if it has any strange attributes the item is strange and don't apply
						for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
						{
							if ( m_pItem->FindAttribute( GetKillEaterAttr_Score( i ) ) )
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
						if ( m_pItem->FindAttribute( pAttr_moduleScale, &unFloatAsUint32 ) )
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

				int iTeam = GetLocalPlayerTeam(),
					iSkin = iTeam;

#ifdef TF_CLIENT_DLL
				// If we aren't in a game we default to previewing the red team skin.
				if ( iTeam == TEAM_UNASSIGNED )
				{
					iTeam = TF_TEAM_RED;
				}
#endif // TF_CLIENT_DLL

				if ( iSkin != TEAM_UNASSIGNED )
				{
					// Use the first skin for the first team, and the second skin for the other (but default to 0)
					iSkin = (iSkin == (FIRST_GAME_TEAM+1)) ? 1 : 0;
				}

				// Handle styles/visuals overriding the skin.
				int iOverrideSkin = m_pItem->GetSkin( iTeam );
				if ( iOverrideSkin != -1 )
				{
					iSkin = iOverrideSkin;
				}
					
				SetSkin( iSkin );

				if ( m_bUsePedestal )
				{
					m_ItemModel.m_MDL.m_nSkin = iSkin;
				}
			}
		}
	}
}

void CEmbeddedItemModelPanel::LoadAttachedModel( attachedmodel_t *pModel )
{
	if ( !( pModel->m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
		return;

	if ( !pModel->m_pszModelName )
	{
		Warning( "econ item definition '%s' attachment has no model\n", m_pItem->GetItemDefinition()->GetDefinitionName() );
		return;
	}

	int iIndex = m_AttachedModels.AddToTail();
	MDLHandle_t hMDL = mdlcache->FindMDL( pModel->m_pszModelName );
	if ( mdlcache->IsErrorModel( hMDL ) )
	{
		hMDL = MDLHANDLE_INVALID;
	}
	m_AttachedModels[iIndex].m_MDL.SetMDL( hMDL );
	mdlcache->Release( hMDL ); // counterbalance addref from within FindMDL

	m_AttachedModels[iIndex].m_MDL.m_pProxyData = static_cast<IClientRenderable*>( m_pItem );
	m_AttachedModels[iIndex].m_bDisabled = false;
	m_AttachedModels[iIndex].m_MDL.m_nSequence = ACT_IDLE;
	SetIdentityMatrix( m_AttachedModels[iIndex].m_MDLToWorld );
}

bool CEmbeddedItemModelPanel::IsLoadingWeaponSkin( void ) const
{
	static ConVarRef mat_dxlevel( "mat_dxlevel" );
	if ( mat_dxlevel.GetInt() < 90 )
		return false;

	if ( m_bForceUseModel )
		return false;

	if ( m_pItem && m_pItem->IsValid() )
	{
		if ( m_bWeaponAllowInspect && m_bIsPaintKitItem )
		{
			return m_pItem->GetWeaponSkinBaseCompositor() != NULL || !m_pCachedWeaponIcon || !m_pCachedWeaponIcon->GetTexture();
		}
		else if ( UseRenderTargetAsIcon() )
		{
			return !m_pCachedWeaponIcon || !m_pCachedWeaponIcon->GetTexture();
		}
	}
	
	return false;
}


bool CEmbeddedItemModelPanel::IsImageNotLoaded( void ) const
{
	if ( m_bForceUseModel )
		return false;

	if ( m_bImageNotLoaded && m_pItem && m_pItem->IsValid() )
		return true;

	return false;
}


IMaterial* GetMaterialForImage( CEmbeddedItemModelPanel::InventoryImageType_t eImageType, const char* pszBaseName )
{
	IMaterial *pMaterial = NULL;
	
	Assert( pszBaseName );
	if ( !pszBaseName )
		return NULL;


	switch ( eImageType )
	{
	case CEmbeddedItemModelPanel::IMAGETYPE_SMALL:
		pMaterial = g_pMaterialSystem->FindMaterial( pszBaseName, TEXTURE_GROUP_VGUI );
		break;
	case CEmbeddedItemModelPanel::IMAGETYPE_DETAILED:
		pMaterial = g_pMaterialSystem->FindMaterial( CFmtStr("%s_detail",pszBaseName).Access(), TEXTURE_GROUP_VGUI, false );
		break;
	case CEmbeddedItemModelPanel::IMAGETYPE_LARGE:
		pMaterial = g_pMaterialSystem->FindMaterial( CFmtStr("%s_large",pszBaseName).Access(), TEXTURE_GROUP_VGUI );
		break;
	default:
		Assert(0);
	}
	
	Assert( pMaterial && !IsErrorMaterial( pMaterial ) );

	return pMaterial;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::LoadInventoryImage()
{
	InventoryImageType_t type = (InventoryImageType_t)m_iInventoryImageType;
	if ( m_iInventoryImageType == IMAGETYPE_DETAILED && !m_pItem->GetStaticData()->HasDetailedIcon() )
	{
		type = IMAGETYPE_LARGE;
	}
	GetMaterialForImage( type, m_pItem->GetInventoryImage() );
	m_bImageNotLoaded = false;
	m_iTextureID = -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	CleanUpCachedWeaponIcon();

	// Nive the "player pos" to the defined distance
	if ( m_pItem && m_pItem->IsValid() )
	{
		if ( m_bUsePedestal )
		{
			Vector vecOffset = m_BMPResData.m_vecOriginOffset;
			vecOffset.x = m_pItem->GetItemDefinition()->GetInspectPanelDistance();
			// reset model angle and pos to initial values
			SetModelAnglesAndPosition( m_BMPResData.m_angModelPoseRot, vecOffset );
		}
		else if ( m_iCameraAttachment != -1 )
		{
			UpdateCameraForIcon();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::Paint( void )
{
	if ( !m_pItem || !m_pItem->IsValid() )
		return;

	if ( m_bModelIsHidden )
	{
		BaseClass::Paint();
		return;
	}

	// Don't even try to render backpack icon if we're not loaded
	if ( m_bImageNotLoaded && !m_bWeaponAllowInspect && !UseRenderTargetAsIcon() )
		return;

	const char *pszInventoryImage = m_pItem->GetInventoryImage();

	CMatRenderContextPtr pRenderContext( materials );

	int iWidth = GetWide();
	int iHeight = GetTall();
	float flTexW = 1.0;
	float flTexH = 1.0;
	float flTexX = 0.0;
	float flTexY = 0.0;
	int x = 0;
	int y = 0;

	// First, try and use the inventory image instead of the model.
	bool bIsLoadingWeaponSkin = IsLoadingWeaponSkin();

	int iTexture = -1;
	if ( !bIsLoadingWeaponSkin && !m_bForceUseModel )
	{
		// should we override material with cache texture
		if ( m_pCachedWeaponIcon && m_pCachedWeaponIcon->GetTexture() )
		{
			// Clear out the composited texture--we're finished with it.
			m_pItem->SetWeaponSkinBase( NULL );
			// The compositor should have been cleaned up by the material proxy.
			Assert( m_pItem->GetWeaponSkinBaseCompositor() == NULL );

			if ( !m_pCachedWeaponMaterial && g_pMaterialSystem )
			{
				const char *pszTextureName = m_pCachedWeaponIcon->GetTexture()->GetName();
				KeyValues *pVMTKeyValues = new KeyValues( "UnlitGeneric" );
				pVMTKeyValues->SetString( "$basetexture", pszTextureName );
				pVMTKeyValues->SetInt( "$translucent", 1 );
				pVMTKeyValues->SetInt( "$vertexcolor", 1 );
				IMaterial *pMaterial = g_pMaterialSystem->FindProceduralMaterial( pszTextureName, TEXTURE_GROUP_VGUI, pVMTKeyValues );
				SafeAssign( &m_pCachedWeaponMaterial, pMaterial );

				bool bFound = false;
				IMaterialVar *pVar = m_pCachedWeaponMaterial->FindVar( "$basetexture", &bFound );
				if ( bFound && pVar )
				{
					pVar->SetTextureValue( m_pCachedWeaponIcon->GetTexture() );
					m_pCachedWeaponMaterial->RefreshPreservingMaterialVars();
				}
			}

			if ( m_iCachedTextureID == -1 )
			{
				//m_pCachedWeaponIcon->GetTexture()->SaveToFile( CFmtStr( "%d_weapon_skin_cache.tga", m_pItem->GetItemDefIndex() ) );
				m_iCachedTextureID = g_pMatSystemSurface->DrawGetTextureId( m_pCachedWeaponIcon->GetTexture() );
				g_pMatSystemSurface->DrawSetTextureMaterial( m_iCachedTextureID, m_pCachedWeaponMaterial );
			}
			iTexture = m_iCachedTextureID;
			x = 0;
			y = 0;
			flTexX = flTexY = 0.f;

			int iMappingWidth = m_pCachedWeaponMaterial->GetMappingWidth();
			int iMappingHeight = m_pCachedWeaponMaterial->GetMappingHeight();
			if ( iWidth > iMappingWidth || iHeight > iMappingHeight )
			{
				flTexW = 1.f;
				flTexH = 1.f;
			}
			else
			{
				flTexW = (float)iWidth / iMappingWidth;
				flTexH = (float)iHeight / iMappingHeight;
			}

		}
		else if ( pszInventoryImage )
		{
			// Look up the material (use the large one if we've been told to)
			IMaterial *pMaterial = GetMaterialForImage( (CEmbeddedItemModelPanel::InventoryImageType_t)m_iInventoryImageType, pszInventoryImage );
		
			int iCenter[2];
			iCenter[0] = x + (iWidth * 0.5);
			iCenter[1] = y + (iHeight * 0.5);

			// Maintain image aspect ratios. Fit to height.
			int iPosition[2] = {0,0};
			int iSize[2] = {0,0};
			m_pItem->GetInventoryImageData( iPosition, iSize );
		
			if ( m_bForceSquareImage )
			{
				iSize[0] = MAX( iSize[0], iSize[1] );
				iSize[1] = iSize[0];
			}
			if ( !iSize[0] && !iSize[1] )
			{
				iSize[0] = pMaterial->GetMappingWidth();
				iSize[1] = pMaterial->GetMappingHeight();
			}
			else
			{
				bool bForceHighRes = false;
				if ( m_iInventoryImageType != IMAGETYPE_SMALL || bForceHighRes )
				{
					// Normal is 128*128, large is 512x512
					iSize[0] *= 4;
					iSize[1] *= 4;
				}

				flTexW = ((float)iSize[0] / (float)pMaterial->GetMappingWidth());
				flTexH = ((float)iSize[1] / (float)pMaterial->GetMappingHeight());
				flTexX = ( 1.0 - flTexW ) * 0.5;
				flTexY = ( 1.0 - flTexH ) * 0.5;
			}
			if ( iPosition[0] || iPosition[1] )
			{
				x += XRES(iPosition[0]);
				y += YRES(iPosition[1]);
			}

			float flRatio = ((float)iSize[0] / (float)iSize[1]);
			if ( flRatio != ((float)iWidth / (float)iHeight) )
			{
				// Fit to the height
				int iCenterX = x + (iWidth * 0.5);
				iWidth = iHeight * flRatio;
				x = iCenterX - (iWidth * 0.5);
			}

			// Reload our texture, if we need to
			if ( m_iTextureID == -1 ) 
			{
				m_iTextureID = vgui::surface()->DrawGetTextureId( pMaterial->GetName() );

				// If we didn't find it, create a new one
				if ( m_iTextureID == -1 )
				{
					m_iTextureID = vgui::surface()->CreateNewTextureID();	
					g_pMatSystemSurface->DrawSetTextureMaterial( m_iTextureID, pMaterial );
				}
			}

			iTexture = m_iTextureID;
		}
	}

	// draw texture if we have a valid texture
	if ( iTexture != -1 )
	{
		surface()->DrawSetTexture( iTexture );

		if ( m_bGreyedOut )
		{ 
			surface()->DrawSetColor( 96, 96, 96, 255 );
		}
		else
		{
			surface()->DrawSetColor( 255, 255, 255, 255 );
		}
		surface()->DrawTexturedSubRect( x, y, x + iWidth, y + iHeight, flTexX, flTexY, flTexX + flTexW, flTexY + flTexH );

		// Draw the overlay image now, and tint it by the tint attribute (if we have one)
		for ( int i=0; i<m_pItem->GetInventoryOverlayImageCount(); i++ )
		{
			const char *pszInventoryOverlayImage = m_pItem->GetInventoryOverlayImage( i );
			IMaterial *pOverlayMaterial = GetMaterialForImage( (CEmbeddedItemModelPanel::InventoryImageType_t)m_iInventoryImageType, pszInventoryOverlayImage );

			if ( !pOverlayMaterial )
				continue;

			int iTextureIDIdx = m_iOverlayTextureIDs.Find(i);
			if ( (iTextureIDIdx == m_iOverlayTextureIDs.InvalidIndex()
				|| m_iOverlayTextureIDs[iTextureIDIdx] == -1 ) )
			{
				int iTextureID = vgui::surface()->DrawGetTextureId( pOverlayMaterial->GetName() );

				// If we didn't find it, create a new one
				if ( iTextureID == -1 )
				{
					iTextureID = vgui::surface()->CreateNewTextureID();	
					g_pMatSystemSurface->DrawSetTextureMaterial( iTextureID, pOverlayMaterial );
				}

				m_iOverlayTextureIDs.Insert( i, iTextureID );
			}

			surface()->DrawSetTexture( m_iOverlayTextureIDs[m_iOverlayTextureIDs.Find( i )] );

			int iRGB = m_pItem->GetModifiedRGBValue( i == 0 );
			Color col;
			col.SetColor( clamp( (iRGB & 0xFF0000) >> 16, 0, 255 ), clamp( (iRGB & 0xFF00) >> 8, 0, 255 ), clamp( (iRGB & 0xFF), 0, 255 ), 255 );
			// Dim this color if the item is currently greyed out
			float flColorScale = m_bGreyedOut ? 96.f / 255.f : 1.f;
			col.SetColor( col.r() * flColorScale, col.g() * flColorScale, col.b() * flColorScale, col.a() );

			surface()->DrawSetColor( col );

			surface()->DrawTexturedSubRect( x, y, x + iWidth, y + iHeight, flTexX, flTexY, flTexX + flTexW, flTexY + flTexH );
		}

		// Draw strangifier item on top of strangifier bottles
		if ( m_pszToolTargetItemImage && m_pszToolTargetItemImage[0] )
		{
			IMaterial* pToolTargetItemMaterial = GetMaterialForImage( (CEmbeddedItemModelPanel::InventoryImageType_t)m_iInventoryImageType, m_pszToolTargetItemImage );
			
			if ( m_iToolTargetItemTextureID == -1 ) 
			{
				m_iToolTargetItemTextureID = vgui::surface()->DrawGetTextureId( pToolTargetItemMaterial->GetName() );

				// If we didn't find it, create a new one
				if ( m_iToolTargetItemTextureID == -1 )
				{
					m_iToolTargetItemTextureID = vgui::surface()->CreateNewTextureID();	
					g_pMatSystemSurface->DrawSetTextureMaterial( m_iToolTargetItemTextureID, pToolTargetItemMaterial );
				}

				CAttribute_String attrToolTargetItemIconOffset;
				static CSchemaAttributeDefHandle pAttrDef_ToolTargetItemIconOffset( "tool_target_item_icon_offset" );
				if ( m_pItem->FindAttribute( pAttrDef_ToolTargetItemIconOffset, &attrToolTargetItemIconOffset ) && attrToolTargetItemIconOffset.has_value() )
				{
					UTIL_StringToVector( m_vecToolTargetItemImageOffset.Base(), attrToolTargetItemIconOffset.value().c_str() );
				}
			}

			surface()->DrawSetTexture( m_iToolTargetItemTextureID );
			
			int iStrangeX = x + ( iWidth  * m_vecToolTargetItemImageOffset.x );
			int iStrangeY = y + ( iHeight * m_vecToolTargetItemImageOffset.y );
			float flScale = m_vecToolTargetItemImageOffset.z;

			surface()->DrawTexturedSubRect( iStrangeX,
											iStrangeY,
											iStrangeX + (iWidth * flScale),
											iStrangeY + (iHeight * flScale),
											flTexX,
											flTexY,
											flTexX + (flTexW ),
											flTexY + (flTexH ) );
		}
		
		return;
	}

	item_model_cache_t *pCacheRenderTarget = NULL;
	const char *pszCacheRenderTargetName = NULL;
	// find available render target
	for ( int i=0; i<ITEM_MODEL_IMAGE_CACHE_SIZE; ++i )
	{
		CEmbeddedItemModelPanel *pLockPanel = g_ItemModelImageCache[i].m_hModelPanelLock.Get();

		// found available render target?
		if ( pLockPanel == NULL )
		{
			pszCacheRenderTargetName = m_bOfflineIconGeneration ? "offline_icon_generation" : g_ItemModelPanelRenderTargetNames[i];
			pCacheRenderTarget = &g_ItemModelImageCache[i];
			break;
		}
		else
		{
			// waiting for async copy to finish
			if ( pLockPanel->m_pCachedWeaponIcon && pLockPanel->m_pCachedWeaponIcon->GetTexture() )
			{
				g_ItemModelImageCache[i].Clear();

				pszCacheRenderTargetName = m_bOfflineIconGeneration ? "offline_icon_generation" : g_ItemModelPanelRenderTargetNames[i];
				pCacheRenderTarget = &g_ItemModelImageCache[i];
				break;
			}
		}
	}

	// can't find available cache render target, don't do anything
	if ( !pszCacheRenderTargetName || !pCacheRenderTarget )
	{
		BaseClass::Paint();
		return;
	}

	// Turn off depth-write to dest alpha so that we get white there instead.  The code that uses
	// the render target needs a mask of where stuff was rendered.
	pRenderContext->SetIntRenderingParameter( INT_RENDERPARM_WRITE_DEPTH_TO_DESTALPHA, false );

	bool bUseRenderTarget = !m_bForceUseModel && ( UseRenderTargetAsIcon() || bIsLoadingWeaponSkin );
	bool bRenderToTexture = m_bRenderToTexture;
	if ( bUseRenderTarget )
	{
		g_pMatSystemSurface->Set3DPaintTempRenderTarget( pszCacheRenderTargetName );
	}
	else if ( m_bUseParticle )
	{
		// we want to render particle with this model. don't render to texture
		m_bRenderToTexture = false;
	}

	// make sure the weapon skin is ready before we render the model
	bool bDrawWeaponWithSkin = bIsLoadingWeaponSkin && m_pCachedWeaponIcon == NULL && m_pItem->GetWeaponSkinBase();

	m_pItem->SetWeaponSkinBaseCreateFlags( TEX_COMPOSITE_CREATE_FLAGS_NO_COMPRESSION | TEX_COMPOSITE_CREATE_FLAGS_NO_MIPMAPS );

	BaseClass::Paint();

	m_bRenderToTexture = bRenderToTexture;

	// check if we should cache rt from this frame to a texture
	bool bShouldCacheToTexture = !m_pCachedWeaponIcon && !m_bForceUseModel;
	if ( m_bIsPaintKitItem )
	{
		bShouldCacheToTexture &= bDrawWeaponWithSkin;
	}
	else
	{
		bShouldCacheToTexture &= UseRenderTargetAsIcon();
	}

	// copy the rendered weapon skin from the render target
	if ( bShouldCacheToTexture )
	{
		uint64 nPaintKitDef = 0; m_pItem->GetID();

		// Include our paintkit defindex, incase we don't have a SO-backed item (meaning GetID() will
		// return the same thing for all instances).
		attrib_value_t val;
		if ( GetPaintKitDefIndex( m_pItem, &val ) )
		{
			nPaintKitDef = val;
		}

		char buffer[_MAX_PATH];
		V_sprintf_safe( buffer, "proc/icon/item%d_id%lld%lld_w%d_h%d", m_pItem->GetItemDefIndex(), m_pItem->GetID(), nPaintKitDef, iWidth, iHeight );
		SafeAssign( &m_pCachedWeaponIcon, new CIconRenderReceiver() );

		// If the icon still exists in the material system, don't bother regenerating it.
		if ( materials->IsTextureLoaded( buffer ) )
		{
			ITexture* resTexture = materials->FindTexture( buffer, TEXTURE_GROUP_RUNTIME_COMPOSITE, false, 0 );
			if ( resTexture && resTexture->IsError() == false )
			{
				m_pCachedWeaponIcon->OnAsyncCreateComplete( resTexture, NULL );
			}
		}
		else
		{
			// No icon available yet, need to create it.
			ITexture *pRenderTarget = g_pMaterialSystem->FindTexture( pszCacheRenderTargetName, TEXTURE_GROUP_RENDER_TARGET );
			if ( pRenderTarget )
			{
				pRenderContext->AsyncCreateTextureFromRenderTarget( pRenderTarget, buffer, IMAGE_FORMAT_RGBA8888, false, 0, m_pCachedWeaponIcon, NULL );
			
				pCacheRenderTarget->iItemID = m_pItem->GetItemID();
				pCacheRenderTarget->iItemDefinitionIndex = m_pItem->GetItemDefIndex();
				pCacheRenderTarget->iWidth = iWidth;
				pCacheRenderTarget->iHeight = iHeight;
				pCacheRenderTarget->m_hModelPanelLock = this;
			}
		}
	}

	if ( bUseRenderTarget )
	{
		g_pMatSystemSurface->Reset3DPaintTempRenderTarget();
	}

	if ( m_flModelRotateYawSpeed != 0 )
	{
		m_angPlayer[YAW] += m_flModelRotateYawSpeed * gpGlobals->frametime;
		SetModelAnglesAndPosition( m_angPlayer, m_vecPlayerPos );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ITexture *CEmbeddedItemModelPanel::GetCachedGeneratedIcon() 
{
	if ( m_iCachedTextureID == -1 )
		return NULL;
	return m_pCachedWeaponIcon ? m_pCachedWeaponIcon->GetTexture() : NULL;
}


bool CEmbeddedItemModelPanel::ShouldUseRenderTargetAsIcon() const
{
	if ( m_bIsFestivized )
		return true;

	float flUseCacheIcon = 0.f;
	static CSchemaAttributeDefHandle pAttrib_UseModelCacheIcon( "use_model_cache_icon" );
	if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( m_pItem, pAttrib_UseModelCacheIcon, &flUseCacheIcon ) && flUseCacheIcon != 0.f )
	{
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEmbeddedItemModelPanel::UpdateParticle(
	IMatRenderContext				*pRenderContext, 
	CStudioHdr						*pStudioHdr, 
	MDLHandle_t						mdlHandle, 
	matrix3x4_t						*pWorldMatrix
	)
{
	if ( !m_bUseParticle )
		return false;

	if ( m_pItemParticle && m_pItemParticle->m_bIsUpdateToDate )
		return false;

	if ( !m_pItem || !m_pItem->IsValid() )
		return false;

	attachedparticlesystem_t *pParticleSystem = NULL;

	// do community_sparkle effect if this is a community item?
	const int iQualityParticleType = m_pItem->GetQualityParticleType();
	if ( iQualityParticleType > 0 )
	{
		pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( iQualityParticleType );
	}

	if ( !pParticleSystem )
	{
		// does this hat even have a particle effect
		static CSchemaAttributeDefHandle pAttrDef_AttachParticleEffect( "attach particle effect" );
		uint32 iValue = 0;
		if ( !m_pItem->FindAttribute( pAttrDef_AttachParticleEffect, &iValue ) )
		{
			return false;
		}

		const float& value_as_float = (float&)iValue;
		pParticleSystem = GetItemSchema()->GetAttributeControlledParticleSystem( value_as_float );
	}

	// failed to find any particle effect
	if ( !pParticleSystem )
	{
		return false;
	}

	// Team Color
	if ( m_pItem->GetTeamNumber() == TF_TEAM_BLUE && V_stristr( pParticleSystem->pszSystemName, "_teamcolor_red" ))
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
	if ( !m_pItem->FindAttribute( pAttrDef_UseHead, &iUseHead ) || !iUseHead == 0 )
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
	if ( pParticleSystem->bUseSuffixName && m_pItem && m_pItem->GetItemDefinition()->GetParticleSuffix() )
	{
		V_strcpy_safe( pszFullname, pParticleSystem->pszSystemName );
		V_strcat_safe( pszFullname, "_" );
		V_strcat_safe( pszFullname, m_pItem->GetItemDefinition()->GetParticleSuffix() );
		pszSystemName = pszFullname;
	}

	// Update the Particles and render them
	if ( m_pItemParticle )
	{
		// Check if its a new particle system
		if ( V_strcmp( m_pItemParticle->m_pParticleSystem->GetName(), pszSystemName ) )
		{
			SafeDeleteParticleData( &m_pItemParticle );
			m_pItemParticle = CreateParticleData( pszSystemName );
		}
	}
	else
	{
		// create
		m_pItemParticle = CreateParticleData( pszSystemName );
	}

	// Particle system does not exist
	if ( !m_pItemParticle )
		return false;

	// Get offset if it exists (and if we're using head offset)
	static CSchemaAttributeDefHandle pAttrDef_VerticalOffset( "particle effect vertical offset" );
	uint32 iOffset = 0;
	Vector vecParticleOffset( 0, 0, 0 );
	if ( iUseHead > 0 && m_pItem->FindAttribute( pAttrDef_VerticalOffset, &iOffset ) )
	{
		vecParticleOffset.z = (float&)iOffset;
	}

	m_pItemParticle->UpdateControlPoints( pStudioHdr, pWorldMatrix, vecAttachments, 0, vecParticleOffset );
	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CEmbeddedItemModelPanel::RenderStatTrack( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix )
{
	// Draw the merge MDLs.
	if ( !m_StatTrackModel.m_bDisabled )
	{
		matrix3x4_t matMergeBoneToWorld[MAXSTUDIOBONES];

		// Get the merge studio header.
		studiohdr_t *pStatTrackStudioHdr = m_StatTrackModel.m_MDL.GetStudioHdr();
		matrix3x4_t *pMergeBoneToWorld = &matMergeBoneToWorld[0];

		// If we have a valid mesh, bonemerge it. If we have an invalid mesh we can't bonemerge because
		// it'll crash trying to pull data from the missing header.
		if ( pStatTrackStudioHdr != NULL )
		{
			CStudioHdr mergeHdr( pStatTrackStudioHdr, g_pMDLCache );
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

bool CEmbeddedItemModelPanel::RenderAttachedModels( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix )
{
	// Draw the merge MDLs.
	FOR_EACH_VEC( m_AttachedModels, iModel )
	{
		matrix3x4_t matMergeBoneToWorld[MAXSTUDIOBONES];

		// Get the merge studio header.
		studiohdr_t *pAttachedStudioHdr = m_AttachedModels[iModel].m_MDL.GetStudioHdr();
		matrix3x4_t *pMergeBoneToWorld = &matMergeBoneToWorld[0];

		// If we have a valid mesh, bonemerge it. If we have an invalid mesh we can't bonemerge because
		// it'll crash trying to pull data from the missing header.
		if ( pAttachedStudioHdr != NULL )
		{
			CStudioHdr mergeHdr( pAttachedStudioHdr, g_pMDLCache );
			m_AttachedModels[iModel].m_MDL.SetupBonesWithBoneMerge( &mergeHdr, pMergeBoneToWorld, pStudioHdr, pWorldMatrix, m_AttachedModels[iModel].m_MDLToWorld );
			m_AttachedModels[iModel].m_MDL.Draw( m_AttachedModels[iModel].m_MDLToWorld, pMergeBoneToWorld );
		}
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEmbeddedItemModelPanel::RenderingRootModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix )
{
	// No model?  Bail
	if ( m_ItemModel.m_bDisabled )
	{
		// no model means not using pedestal. just use pStudioHdr to find the attachment points
		UpdateParticle( pRenderContext, pStudioHdr, mdlHandle, pWorldMatrix );
		RenderStatTrack( pStudioHdr, pWorldMatrix );
		RenderAttachedModels( pStudioHdr, pWorldMatrix );
		return;
	}

	studiohdr_t *pItemStudioHdr = m_ItemModel.m_MDL.GetStudioHdr();
	
	if ( pItemStudioHdr != NULL )
	{
		matrix3x4_t matIdentity;
		SetIdentityMatrix( matIdentity );

		matrix3x4_t *pBoneToWorld = g_pStudioRender->LockBoneMatrices( pItemStudioHdr->numbones );
		m_ItemModel.m_MDL.SetUpBones( matIdentity, pItemStudioHdr->numbones, pBoneToWorld );

		// Get attachment transform
		mstudioattachment_t attach = pItemStudioHdr->pAttachment( m_iPedestalAttachment );
		matrix3x4_t matLocalToWorld;
		matrix3x4_t matWorldToLocal;
		matrix3x4_t matTransform;

		ConcatTransforms( pBoneToWorld[ attach.localbone ], attach.local, matLocalToWorld );
		MatrixInvert( matLocalToWorld, matWorldToLocal );
		ConcatTransforms( m_RootMDL.m_MDLToWorld, matWorldToLocal, matTransform );

		m_ItemModel.m_MDL.SetUpBones( matTransform, pItemStudioHdr->numbones, pBoneToWorld );

		g_pStudioRender->UnlockBoneMatrices();

		IMaterial* pOverrideMaterial = GetOverrideMaterial( m_ItemModel.m_MDL.GetMDL() );
		if ( pOverrideMaterial != NULL )
			g_pStudioRender->ForcedMaterialOverride( pOverrideMaterial );

		m_ItemModel.m_MDL.Draw( m_ItemModel.m_MDLToWorld, pBoneToWorld );

		if ( pOverrideMaterial != NULL )
			g_pStudioRender->ForcedMaterialOverride( NULL );

		CStudioHdr HDR( pItemStudioHdr, g_pMDLCache );

		// update particle with the actual item model pItemStudioHdr
		UpdateParticle( pRenderContext, &HDR, mdlHandle, pBoneToWorld );
		RenderStatTrack( &HDR, pBoneToWorld );
		RenderAttachedModels( &HDR, pBoneToWorld );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
IMaterial *CEmbeddedItemModelPanel::GetOverrideMaterial( MDLHandle_t mdlHandle )
{
	// This matches the check in RenderingRootModel, if we're not on a pedestal
	// then we expect mdlHandle to not match m_ItemModel and that's fine--we should
	// just get the override from the m_pItem
	if ( !m_ItemModel.m_bDisabled && m_ItemModel.m_MDL.GetMDL() != mdlHandle )
		return NULL;

	if ( !m_pItem ) 
		return NULL;

	int iTeam = GetLocalPlayerTeam();

#ifdef TF_CLIENT_DLL
	// If we aren't in a game we default to previewing the red team skin.
	if ( iTeam == TEAM_UNASSIGNED )
	{
		iTeam = TF_TEAM_RED;
	}
#endif

	return m_pItem->GetMaterialOverride( iTeam );
}


float CItemModelPanel::sm_flLoadingTimeThisFrame = 0.0f;
int   CItemModelPanel::sm_nCurrentDecriptionUpdateFrame = 0;
CItemModelPanel::eLoadingType_t CItemModelPanel::se_CurrentLoadingTask = LOADING_ICONS;
int CItemModelPanel::sai_NumLoadingRequests[NUM_LOADING_TYPES] = {0,0,0};
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemModelPanel::CItemModelPanel( vgui::Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
{
	m_pModelPanel = NULL;
	m_pItemNameLabel = NULL;
	m_pPaintIcon = NULL;
	m_pTF2Icon = NULL;
	m_pItemAttribLabel = NULL;
	m_pItemCollectionNameLabel = NULL;
	m_pItemCollectionListLabel = NULL;
	m_pItemCollectionHighlight = NULL;
	m_pItemEquippedLabel = NULL;
	m_pItemQuantityLabel = NULL;
	m_pVisionRestrictionImage = NULL;
	m_pIsStrangeImage = NULL;
	m_pIsUnusualImage = NULL;
	m_pIsLoanerImage = NULL;
	m_pSeriesLabel = NULL;
	m_pMainContentContainer = NULL;
	m_pLoadingSpinner = NULL;
//	m_ItemData = NULL;
	m_nCollectionItemLoaded = LOADED_COLLECTION_NONE;
	m_pFontNameSmallest = vgui::INVALID_FONT;
	m_pFontNameSmall = vgui::INVALID_FONT;
	m_pFontNameLarge = vgui::INVALID_FONT;
	m_pFontAttribSmallest = vgui::INVALID_FONT;
	m_pFontAttribSmall = vgui::INVALID_FONT;
	m_pFontAttribLarge = vgui::INVALID_FONT;

	m_pszNoItemText = NULL;
	m_pwcNoItemText = NULL;
	m_pwcNoItemAttrib = NULL;
	REGISTER_COLOR_AS_OVERRIDABLE( m_NoItemTextColor, "noitem_textcolor" );

	m_bClickable = false;
	m_bMouseOver = false;
	m_bSelected = false;
	m_bShowEquipped = false;
	m_bForceShowEquipped = false;
	m_bShowQuantity = false;
	m_pszGreyedOutReason = NULL;
	m_bShowGreyedOutTooltip = false;
	m_bShouldSendPanelEnterExits = false;
	m_bContainedItem = false;
	m_bShowOthersGiftWrappedItems = false;
	m_bDescriptionDirty = false;
	m_nRecipeMatchingIndex = 0;

	m_pContainedItemPanel = NULL;

	m_bFakeButton = false;

	m_mapMatchingAttributes.SetLessFunc( DefLessFunc( attrib_definition_index_t ) );

	SetActAsButton( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CItemModelPanel::~CItemModelPanel( void )
{
	CleanupNoItemWChars();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	// These pointers must be zeroed here because their memory may be freed
	// by LoadControlSettings *and* if they remain non-zero they may be dereferenced
	// before LoadControlSettings returns. This causes reliable crashes if you
	// go to the store, shop, change resolutions, then return to the store -- and
	// if you use pageheap/AppVerifier.
	// This set of pointers is simply the set of pointers that are initialized after
	// LoadControlSettings.
	m_pModelPanel = NULL;
	m_pItemNameLabel = NULL;
	m_pItemAttribLabel = NULL;
	m_pItemCollectionNameLabel = NULL;
	m_pItemCollectionListLabel = NULL;
	m_pItemEquippedLabel = NULL;
	m_pItemQuantityLabel = NULL;
	m_pVisionRestrictionImage = NULL;
	m_pIsStrangeImage = NULL;
	m_pIsUnusualImage = NULL;
	m_pIsLoanerImage = NULL;
	m_pSeriesLabel = NULL;
	m_pMatchesLabel = NULL;
	m_pPaintIcon = NULL;
	m_pTF2Icon = NULL;
	m_pFontNameSmallest = NULL;
	m_pFontNameSmall = NULL;
	m_pFontNameLarge = NULL;
	m_pFontNameLarger = NULL;
	m_pFontAttribSmallest = NULL;
	m_pFontAttribSmall = NULL;
	m_pFontAttribLarge = NULL;
	m_pFontAttribLarger = NULL;
	m_pContainedItemPanel = NULL;
	m_pMainContentContainer = NULL;
	m_pLoadingSpinner = NULL;
	m_nCollectionItemLoaded = LOADED_COLLECTION_NONE;
	LoadResFileForCurrentItem( true );

	m_pFontNameSmallest = pScheme->GetFont( "ItemFontNameSmallest", true );
	m_pFontNameSmall = pScheme->GetFont( "ItemFontNameSmall", true );
	m_pFontNameLarge = pScheme->GetFont( "ItemFontNameLarge", true );
	m_pFontNameLarger = pScheme->GetFont( "ItemFontNameLarger", true );
	m_pFontAttribSmallest = pScheme->GetFont( "ItemFontAttribSmallest", true );
	m_pFontAttribSmall = pScheme->GetFont( "ItemFontAttribSmallv2", true );
	m_pFontAttribLarge = pScheme->GetFont( "ItemFontAttribLarge", true );
	m_pFontAttribLarger = pScheme->GetFont( "ItemFontAttribLarger", true );

	if ( m_bContainedItem )
	{
		// SetBorder( pScheme->GetBorder("TFThinLineBorder") );
	}
	else
	{
		SetBorder( pScheme->GetBorder( "TFFatLineBorder" ) );
	}

	if ( m_pModelPanel )
	{
		m_pModelPanel->SetBorder( pScheme->GetBorder( "TFFatLineBorder" ) );
	}
}

void CItemModelPanel::ApplySettings( KeyValues *inResourceData )
{
	if ( !inResourceData )
		return;

	BaseClass::ApplySettings( inResourceData );

	// Pass the itemmodelpanel KVs to the actual model panel
	KeyValues* pItemModelPanelKVs = inResourceData->FindKey( "itemmodelpanel" );
	if ( m_pModelPanel && pItemModelPanelKVs )
	{
		m_pModelPanel->ApplySettings( pItemModelPanelKVs );
	}

	// We can get our settings applied AFTER we've already setup our
	// panel, so re-update.
	UpdatePanels();
}

void CItemModelPanel::LoadResFileForCurrentItem( bool bForceLoad )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );
	const CEconItemView *pItem = GetItem();

	bool bCollectionMouseover = false;
	if ( m_bIsMouseOverPanel && pItem )
	{
		const CEconItemCollectionDefinition *pCollection = pItem->GetItemDefinition()->GetItemCollectionDefinition();
		if ( !pCollection )
		{
			// see if this is part of paintkit collection
			pCollection = GetItemSchema()->GetPaintKitCollectionFromItem( pItem );
		}

		bCollectionMouseover = pCollection != NULL;
	}

	if ( bCollectionMouseover )
	{
		float flInspect = 0;
		static CSchemaAttributeDefHandle pAttrib_WeaponAllowInspect( "weapon_allow_inspect" );
		if ( FindAttribute_UnsafeBitwiseCast<attrib_value_t>( GetItem(), pAttrib_WeaponAllowInspect, &flInspect ) && flInspect != 0.f )
		{
			if ( bForceLoad || m_nCollectionItemLoaded != LOADED_COLLECTION_WEAPON )
			{
				tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s ItemModelPanelCollectionItem", __FUNCTION__ );
				LoadControlSettings( "Resource/UI/econ/ItemModelPanelCollectionItem.res" );
				m_nCollectionItemLoaded = LOADED_COLLECTION_WEAPON;
			}
		}
		else
		{
			if ( bForceLoad || m_nCollectionItemLoaded != LOADED_COLLECTION_COSMETIC )
			{
				tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s ItemModelPanelCollectionCosmeticItem", __FUNCTION__ );
				LoadControlSettings( "Resource/UI/econ/ItemModelPanelCollectionCosmeticItem.res" );
				m_nCollectionItemLoaded = LOADED_COLLECTION_COSMETIC;
			}
		}
		m_bHideModel = false; // Hack
	}
	else
	{
		if ( bForceLoad || m_nCollectionItemLoaded != LOADED_COLLECTION_NONE )
		{
			tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s ItemModelPanel", __FUNCTION__ );
			LoadControlSettings( "Resource/UI/econ/ItemModelPanel.res" );
		}
		m_bHideModel = m_bHideModelDefault;
		m_nCollectionItemLoaded = LOADED_COLLECTION_NONE;
	}
	
	m_pModelPanel = dynamic_cast<CEmbeddedItemModelPanel*>( FindChildByName( "itemmodelpanel", true ) );
	SetModelIsHidden( m_bHideModel );
	if ( m_bIsMouseOverPanel && m_pModelPanel )
	{
		m_pModelPanel->SetInventoryImageType( CEmbeddedItemModelPanel::IMAGETYPE_LARGE );
	}

	m_pItemNameLabel = dynamic_cast<CExLabel*>( FindChildByName( "namelabel", true ) );
	m_pItemAttribLabel = dynamic_cast<vgui::Label*>( FindChildByName( "attriblabel", true ) );
	m_pItemCollectionNameLabel = dynamic_cast<CExLabel*>( FindChildByName( "collectionnamelabel", true ) );
	m_pItemCollectionListLabel = dynamic_cast<vgui::Label*>( FindChildByName( "collectionlistlabel", true ) );
	m_pItemCollectionHighlight = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "collectionhighlight", true ) );
	m_pItemEquippedLabel = dynamic_cast<vgui::Label*>( FindChildByName( "equippedlabel", true ) );
	m_pItemQuantityLabel = dynamic_cast<vgui::Label*>( FindChildByName( "quantitylabel", true ) );
	m_pVisionRestrictionImage = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "vision_restriction_icon", true ) );

	m_pIsStrangeImage = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "is_strange_icon", true ) );
	m_pIsUnusualImage = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "is_unusual_icon", true ) );
	m_pIsLoanerImage = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "is_loaner_icon", true ) );

	m_pSeriesLabel = dynamic_cast<vgui::Label*>( FindChildByName( "serieslabel", true ) );
	m_pMatchesLabel = dynamic_cast<vgui::Label*>( FindChildByName( "matcheslabel", true ) );
	m_pMainContentContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "MainContentsContainer" ) );
	m_pLoadingSpinner = dynamic_cast<vgui::ImagePanel*>( FindChildByName( "LoadingSpinner" ) );

	if ( m_pItemEquippedLabel )
	{
		m_pItemEquippedLabel->SetKeyBoardInputEnabled( false );
		m_pItemEquippedLabel->SetMouseInputEnabled( false );
	}
	if ( m_pItemQuantityLabel )
	{
		m_pItemQuantityLabel->SetKeyBoardInputEnabled( false );
		m_pItemQuantityLabel->SetMouseInputEnabled( false );
	}
	if ( m_pVisionRestrictionImage )
	{
		m_pVisionRestrictionImage->SetKeyBoardInputEnabled( false );
		m_pVisionRestrictionImage->SetMouseInputEnabled( false );
	}
	if ( m_pIsStrangeImage )
	{
		m_pIsStrangeImage->SetKeyBoardInputEnabled( false );
		m_pIsStrangeImage->SetMouseInputEnabled( false );
	}
	if ( m_pIsUnusualImage )
	{
		m_pIsUnusualImage->SetKeyBoardInputEnabled( false );
		m_pIsUnusualImage->SetMouseInputEnabled( false );
	}
	if ( m_pIsLoanerImage )
	{
		m_pIsLoanerImage->SetKeyBoardInputEnabled( false );
		m_pIsLoanerImage->SetMouseInputEnabled( false );
	}

	if ( m_pSeriesLabel )
	{
		m_pSeriesLabel->SetKeyBoardInputEnabled( false );
		m_pSeriesLabel->SetMouseInputEnabled( false );
	}
	if ( m_pMatchesLabel )
	{
		m_pMatchesLabel->SetKeyBoardInputEnabled( false );
		m_pMatchesLabel->SetMouseInputEnabled( false );
	}

	m_pPaintIcon = dynamic_cast<CItemMaterialCustomizationIconPanel*>( FindChildByName( "paint_icon", true ) );
	if ( m_pPaintIcon )
	{
		m_pPaintIcon->SetMouseInputEnabled( false );
	}
	m_pTF2Icon = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName( "tf2_icon", true ) );
	if ( m_pTF2Icon )
	{
		m_pTF2Icon->SetMouseInputEnabled( false );
	}

	if ( m_bContainedItem )
	{
		SetPaintBackgroundEnabled( true );
	}
	else
	{
		SetPaintBackgroundEnabled( false );
	}

	if ( m_pModelPanel )
	{
		m_pModelPanel->SetBgColor( Color( 0, 0, 0, 255 ) );
		m_pModelPanel->AddActionSignalTarget( this );
		m_pModelPanel->SetMouseInputEnabled( false );
	}

	if ( m_pItemNameLabel )
	{
		m_OrgItemTextColor = m_pItemNameLabel->GetFgColor();
		m_pItemNameLabel->SetMouseInputEnabled( false );
		m_pItemNameLabel->AddActionSignalTarget( this );
		m_pItemNameLabel->InvalidateLayout( true, true );
	}

	if ( m_pItemAttribLabel )
	{
		m_pItemAttribLabel->SetMouseInputEnabled( false );
		m_pItemAttribLabel->AddActionSignalTarget( this );
		m_pItemAttribLabel->InvalidateLayout( true, true );
	}

	if ( m_pItemCollectionNameLabel )
	{
		m_pItemCollectionNameLabel->SetMouseInputEnabled( false );
		m_pItemCollectionNameLabel->AddActionSignalTarget( this );
		m_pItemCollectionNameLabel->InvalidateLayout( true, true );
	}

	if ( m_pItemCollectionListLabel )
	{
		m_pItemCollectionListLabel->SetMouseInputEnabled( false );
		m_pItemCollectionListLabel->AddActionSignalTarget( this );
		m_pItemCollectionListLabel->InvalidateLayout( true, true );
	}

	if ( m_pItemCollectionHighlight )
	{
		m_pItemCollectionHighlight->SetMouseInputEnabled( false );
		m_pItemCollectionHighlight->AddActionSignalTarget( this );
		m_pItemCollectionHighlight->InvalidateLayout( true, true );
	}

	m_pContainedItemPanel = dynamic_cast<CItemModelPanel*>( FindChildByName( "contained_item_panel", true ) );

	// Josh: Avoid infinitely creating contained item panels whenever layout
	// gets invalidated.
	if ( m_pContainedItemPanel )
	{
		m_pContainedItemPanel->m_bInitializedAsContainedItem = true;

		// If we are initialized a contained item, kill our child.
		// SetContainedItem doesn't happen until later (when it is chosen to be shown)
		if ( m_bInitializedAsContainedItem )
		{
			m_pContainedItemPanel->MarkForDeletion();
			m_pContainedItemPanel = NULL;
		}
	}

	// Dont eat mouse input
	if ( m_pMainContentContainer )
	{
		m_pMainContentContainer->SetMouseInputEnabled( false );
	}

	UpdatePanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::PerformLayout( void ) 
{
	int w,h;
	GetSize( w, h );
	w = m_iBaseWide ? m_iBaseWide : w;
	h = m_iBaseTall ? m_iBaseTall : h;

	int iTextW = GetAttribWide(w);
	int iModelW = m_iModelWide && m_iModelWide < w ? m_iModelWide : w;
	int iModelT = m_iModelTall && m_iModelTall < h ? m_iModelTall : h;
	int iModelX = m_bModelCenterX ? ( ( w - iModelW ) * 0.5 ) : m_iModelXPos;
	int iModelY = m_bModelCenterY ? ( ( h - iModelT ) * 0.5 ) : m_iModelYPos;

	ResizeLabels();

	if ( m_pModelPanel )
	{
		m_pModelPanel->SetBounds( iModelX, iModelY, iModelW, iModelT );
	}
	if ( m_pLoadingSpinner )
	{
		int nWidthHeight = Max( iModelW, iModelT );
		int xOffset = int( w / 2.f ) - int( nWidthHeight / 2.f );
		int yOffset = int( h / 2.f ) - int( nWidthHeight / 2.f );
		m_pLoadingSpinner->SetBounds( xOffset, yOffset, nWidthHeight, nWidthHeight );
	}

	if ( m_bNoItemFullPanel )
	{
		// We want the "no item" text to use the entire panel, and hide the attribs entirely
		if ( m_pItemNameLabel && m_pItemAttribLabel )
		{
			m_pItemNameLabel->SetBounds( XRES(4), 0, GetWide() - XRES(8), GetTall() );
		}
	}
	else if ( m_pItemNameLabel && m_pItemAttribLabel && !m_bModelOnly )
	{
		// Force the labels to layout now, and get their height.
		m_pItemNameLabel->InvalidateLayout( true );
		m_pItemAttribLabel->InvalidateLayout( true );
		m_pItemNameLabel->SizeToContents();
		m_pItemAttribLabel->SizeToContents();

		if ( m_pItemCollectionNameLabel )
		{
			m_pItemCollectionNameLabel->InvalidateLayout( true );
			m_pItemCollectionNameLabel->SizeToContents();
		}
			
		if ( m_pItemCollectionListLabel )
		{
			m_pItemCollectionListLabel->InvalidateLayout( true );
			m_pItemCollectionListLabel->SizeToContents();
		}

		// "" strings still size themselves as one font-heighth tall, but 0 wide. If there's no
		// text in the attribute, we want 0 tall as well, so we don't get blank lines.
		int iCollectionTall = m_pItemCollectionListLabel ? m_pItemCollectionListLabel->GetTall() : 0;
		int iAttribTall = (m_pItemAttribLabel->GetWide() ? m_pItemAttribLabel->GetTall() : 0);
		iAttribTall = Max( iAttribTall, iCollectionTall );

		int iNameTall = m_pItemNameLabel->GetTall();

		int iCollectionNameTall = m_pItemCollectionNameLabel ? m_pItemCollectionNameLabel->GetTall() : 0;

		if ( m_bAttribOnly )
		{
			iNameTall = 0;
		}

		if ( m_bTextCenterX )
		{
			m_pItemNameLabel->SetSize( iTextW, iNameTall );
			m_pItemAttribLabel->SetSize( iTextW, iAttribTall );
		}
		else if ( m_iTextYPos )
		{
			m_pItemNameLabel->SetSize( iTextW, iNameTall );
			m_pItemAttribLabel->SetSize( iTextW, (m_pItemAttribLabel->GetWide() ? m_pItemAttribLabel->GetTall() : 0) );
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetSize( iTextW, iCollectionNameTall );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetSize( iTextW, iCollectionTall );
		}
		else if ( m_bTextCenter )
		{
			m_pItemNameLabel->SetSize( iTextW, iNameTall );
			m_pItemAttribLabel->SetSize( iTextW, iAttribTall );
		}
		else
		{
			m_pItemNameLabel->SetSize( iTextW, iNameTall );
			m_pItemAttribLabel->SetSize( iTextW, iAttribTall );
		}

		m_pItemNameLabel->InvalidateLayout( true );

		// Force attrib layout to update now in its new size.
		m_pItemAttribLabel->InvalidateLayout( true );
		m_pItemAttribLabel->SizeToContents();
		
		// Reget sizes, wtf
		iCollectionTall = m_pItemCollectionListLabel ? m_pItemCollectionListLabel->GetTall() : 0;
		iAttribTall = ( m_pItemAttribLabel->GetWide() ? m_pItemAttribLabel->GetTall() : 0 );
		// HACK: Now we resize it again. Sets our height properly. Ridiculous. 
		m_pItemAttribLabel->SetSize( iTextW, iAttribTall );
		m_pItemNameLabel->SetSize( iTextW, iNameTall );

		// Ignore attributes if we're only showing the name
		if ( m_bNameOnly || (!HasItem() && m_pszNoItemText && m_pszNoItemText[0]) )
		{
			iAttribTall = 0;
		}

		int iLabelOffset = 0;
		if ( m_bResizeToText )
		{
			h = m_iTextYPos + iNameTall + iAttribTall + m_iHPadding;

			// Must be at least tall enough to fit the image (if visible)
			if ( !m_bHideModel )
			{
				//h = MAX( h, (iModelT + (iModelY * 2)) );
				h = Max( h + iModelT + iModelY, m_iTextYPos + iCollectionNameTall + iCollectionTall + m_iHPadding);
				iLabelOffset = iModelT + iModelY;
			}
		}

		// If we don't have a specific X pos, or attrib width, indent ourselves
		int iTextXPos = (m_iTextXPos || m_iTextWide) ? m_iTextXPos : ATTRIB_LABEL_INDENT;

		if ( iCollectionNameTall && iCollectionTall && m_iTextXPosCollection )
		{
			iTextXPos = m_iTextXPosCollection;
		}

		// Position the name label now we know where our attrib label is
		// If we've got a Y pos, use it. Otherwise, stack up from the bottom of the panel.
		if ( m_bTextCenterX )
		{
			m_pItemNameLabel->SizeToContents();
			m_pItemAttribLabel->SizeToContents();

			m_pItemNameLabel->SetPos( ( w - m_pItemNameLabel->GetWide() ) * 0.5f, m_iTextYPos + iLabelOffset );
			m_pItemAttribLabel->SetPos( ( w - m_pItemAttribLabel->GetWide() ) * 0.5f, m_iTextYPos + iNameTall + iLabelOffset );
		}
		else if ( m_iTextYPos )
		{
			m_pItemNameLabel->SetPos( iTextXPos, m_iTextYPos + iLabelOffset );
			m_pItemAttribLabel->SetPos( iTextXPos, m_iTextYPos + iNameTall + iLabelOffset);
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetPos( m_iCollectionListXPos, m_iTextYPos );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetPos( m_iCollectionListXPos, m_iTextYPos + iCollectionNameTall );
		}
		else if ( m_bTextCenter )
		{
			int iYTop = (h - (iNameTall + iAttribTall)) * 0.5;
			if ( iYTop < 0 )
			{
				iYTop = 0;
			}
			m_pItemNameLabel->SetPos( iTextXPos, iYTop );
			m_pItemAttribLabel->SetPos( iTextXPos, iYTop + iNameTall );
			//m_pItemCollectionLabel->SetPos( iTextXPos + iTextW, iYTop );
		}
		else
		{
			int iOffsetY = (m_iTextYOffset != 0) ? m_iTextYOffset : YRES(8);
			m_pItemNameLabel->SetPos( iTextXPos, h - iAttribTall - iNameTall - iOffsetY + m_iHPadding );
			m_pItemAttribLabel->SetPos( iTextXPos, h - iAttribTall - iOffsetY + m_iHPadding );
			//m_pItemCollectionLabel->SetPos( iTextXPos + iTextW, h - iAttribTall - iNameTall - iOffsetY + m_iHPadding );
		}

		if ( m_bResizeToText )
		{
			const CEconItemView *pItem = GetItem();
			if ( m_bIsMouseOverPanel && pItem && !m_bHideCollectionPanel )
			{
				const CEconItemCollectionDefinition *pCollection = pItem->GetItemDefinition()->GetItemCollectionDefinition();
				if ( !pCollection )
				{
					pCollection = GetItemSchema()->GetPaintKitCollectionFromItem( pItem );
				}

				if ( pCollection && m_pItemCollectionListLabel && m_pItemCollectionNameLabel && m_pItemCollectionHighlight )
				{
					m_pItemCollectionListLabel->SizeToContents();
					m_pItemCollectionNameLabel->SizeToContents();
					int iContentW = Max( m_pItemCollectionNameLabel->GetWide(), m_pItemCollectionListLabel->GetWide() );
					w = iContentW + m_iCollectionListXPos + m_iTextXPosCollection;
					m_pItemCollectionHighlight->SetWide( iContentW );
				}
			}
			SetSize( w, h );
		}
	}

	// pin icons to top right corner
	int xpos = m_iBaseWide - XRES(1);
	int ypos = YRES(1);

	if ( m_pPaintIcon && m_pPaintIcon->IsVisible() )
	{
		m_pPaintIcon->SetPos( xpos - m_pPaintIcon->GetWide(), ypos );
		ypos += m_pPaintIcon->GetTall() * 0.9;
	}
	if ( m_pTF2Icon && m_pTF2Icon->IsVisible() )
	{
		m_pTF2Icon->SetPos( xpos - m_pTF2Icon->GetWide() + m_iTF2IconOffsetX, ypos + m_iTF2IconOffsetY );
		ypos += m_pTF2Icon->GetTall() * 0.9;
	}
	if ( m_pVisionRestrictionImage && m_pVisionRestrictionImage->IsVisible() )
	{
		m_pVisionRestrictionImage->SetPos( xpos - m_pVisionRestrictionImage->GetWide(), ypos );
		ypos += m_pVisionRestrictionImage->GetTall() * 0.9;
	}
	if ( m_pIsUnusualImage && m_pIsUnusualImage->IsVisible() )
	{
		m_pIsUnusualImage->SetPos( xpos - m_pIsUnusualImage->GetWide(), ypos );
		ypos += m_pIsUnusualImage->GetTall() * 0.9;
	}
	if ( m_pIsStrangeImage && m_pIsStrangeImage->IsVisible() )
	{
		m_pIsStrangeImage->SetPos( xpos - m_pIsStrangeImage->GetWide(), ypos );
		ypos += m_pIsStrangeImage->GetTall() * 0.9;
	}
	if ( m_pIsLoanerImage && m_pIsLoanerImage->IsVisible() )
	{
		m_pIsLoanerImage->SetPos( xpos - m_pIsLoanerImage->GetWide(), ypos );
		ypos += m_pIsLoanerImage->GetTall() * 0.9;
	}
	

	if ( m_pItemNameLabel )
	{
		//m_pItemNameLabel->SetContentAlignment( (vgui::Label::Alignment) m_iNameLabelAlignment );
	}

	if ( m_bModelOnly )
	{
		if ( m_pItemNameLabel )
		{
			m_pItemNameLabel->SetVisible( false );
		}
		if ( m_pItemAttribLabel )
		{
			m_pItemAttribLabel->SetVisible( false );
		}
		if ( m_pItemCollectionNameLabel )
		{
			m_pItemCollectionNameLabel->SetVisible( false );
		}
		if ( m_pItemCollectionListLabel )
		{
			m_pItemCollectionListLabel->SetVisible( false );
		}
		if ( m_pItemCollectionHighlight )
		{
			m_pItemCollectionHighlight->SetVisible( false );
		}
	}

	BaseClass::PerformLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::PaintTraverse( bool forceRepaint, bool allowForce )
{
	if ( m_bFakeButton )
		return;
	
	BaseClass::PaintTraverse( forceRepaint, allowForce );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::OnSizeChanged( int newWide, int newTall )
{
	BaseClass::OnSizeChanged( newWide, newTall );
	InvalidateLayout( true );

	if ( m_bModelOnly )
		return;

	if ( m_pItemNameLabel && m_pItemNameLabel->GetTextImage() )
	{
		m_pItemNameLabel->GetTextImage()->RecalculateNewLinePositions();
	}
	if ( m_pItemAttribLabel && m_pItemAttribLabel->GetTextImage() )
	{
		m_pItemAttribLabel->GetTextImage()->RecalculateNewLinePositions();
	}
	if ( m_pItemCollectionNameLabel && m_pItemCollectionNameLabel->GetTextImage() )
	{
		m_pItemCollectionNameLabel->GetTextImage()->RecalculateNewLinePositions();
	}
	if ( m_pItemCollectionListLabel && m_pItemCollectionListLabel->GetTextImage() )
	{
		m_pItemCollectionListLabel->GetTextImage()->RecalculateNewLinePositions();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::ResizeLabels( void )
{
	if ( !m_pItemNameLabel || !m_pItemAttribLabel || m_bModelOnly )
		return;

	int w,h;
	GetSize( w, h );
	int iTextW = GetAttribWide(w);

	if ( m_iMaxTextHeight )
	{
		h = m_iMaxTextHeight;
	}

	// HACK to get the item model panel on the main menu to have its fonts.
	vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	if ( !m_pFontNameSmallest )
		m_pFontNameSmallest = pScheme->GetFont( "ItemFontNameSmallest", true );
	if ( !m_pFontNameSmall )
		m_pFontNameSmall = pScheme->GetFont( "ItemFontNameSmall", true );
	if ( !m_pFontNameLarge )
		m_pFontNameLarge = pScheme->GetFont( "ItemFontNameLarge", true );
	if ( !m_pFontNameLarger )
		m_pFontNameLarger = pScheme->GetFont( "ItemFontNameLarger", true );
	if ( !m_pFontAttribSmallest )
		m_pFontAttribSmallest = pScheme->GetFont( "ItemFontAttribSmallest", true );
	if ( !m_pFontAttribSmall )
		m_pFontAttribSmall = pScheme->GetFont( "ItemFontAttribSmallv2", true );
	if ( !m_pFontAttribLarge )
		m_pFontAttribLarge = pScheme->GetFont( "ItemFontAttribLarge", true );
	if ( !m_pFontAttribLarger )
		m_pFontAttribLarger = pScheme->GetFont( "ItemFontAttribLarger", true );

	if ( m_iForceTextSize && m_iForceTextSize <= 4 )
	{
		// Leave center wrap on if the noitem text is getting to use the whole panel
		if ( !m_bNoItemFullPanel )
		{
			m_pItemNameLabel->SetCenterWrap( false );
		}
		m_pItemNameLabel->InvalidateLayout( true, true );
		m_pItemAttribLabel->InvalidateLayout( true, true );
		if ( m_pItemCollectionNameLabel )
			m_pItemCollectionNameLabel->InvalidateLayout( true, true );
		if ( m_pItemCollectionListLabel )
			m_pItemCollectionListLabel->InvalidateLayout( true, true );

		switch ( m_iForceTextSize )
		{
		case 1:
			m_pItemNameLabel->SetFont( m_pFontNameLarge );
			m_pItemAttribLabel->SetFont( m_pFontAttribLarge );
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetFont( m_pFontNameLarge );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetFont( m_pFontAttribLarge );
			break;

		case 2:
			m_pItemNameLabel->SetFont( m_pFontNameSmall );
			m_pItemAttribLabel->SetFont( m_pFontAttribSmall );
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetFont( m_pFontNameSmall );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetFont( m_pFontAttribSmall );
			break;

		case 3:
			m_pItemNameLabel->SetFont( m_pFontNameSmallest );
			m_pItemAttribLabel->SetFont( m_pFontAttribSmallest );
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetFont( m_pFontNameSmallest );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetFont( m_pFontAttribSmallest );
			break;

		case 4:
			m_pItemNameLabel->SetFont( m_pFontNameLarger );
			m_pItemAttribLabel->SetFont( m_pFontAttribLarger );
			if ( m_pItemCollectionNameLabel )
				m_pItemCollectionNameLabel->SetFont( m_pFontNameLarger );
			if ( m_pItemCollectionListLabel )
				m_pItemCollectionListLabel->SetFont( m_pFontAttribLarger );
			break;
		}

		m_pItemNameLabel->SizeToContents();
		m_pItemAttribLabel->SizeToContents();
		if ( m_pItemCollectionNameLabel )
			m_pItemCollectionNameLabel->SizeToContents();
		if ( m_pItemCollectionListLabel )
			m_pItemCollectionListLabel->SizeToContents();
	}
	else
	{
		m_pItemNameLabel->SetFont( m_pFontNameLarge );

		bool bCenterWrap = false;
		if ( m_ItemData.IsValid() )
		{
			static CSchemaAttributeDefHandle pAttrDef_ForceCenterWrap( "force center wrap" );
			if ( m_ItemData.FindAttribute( pAttrDef_ForceCenterWrap ) )
			{
				bCenterWrap = true;
			}
		}

		m_pItemNameLabel->SetCenterWrap( bCenterWrap );
		m_pItemNameLabel->SizeToContents();
		m_pItemAttribLabel->SetFont( m_pFontAttribLarge );
		m_pItemAttribLabel->SizeToContents();
		if ( m_pItemCollectionNameLabel )
		{
			m_pItemCollectionNameLabel->SetFont( m_pFontNameLarge );
			m_pItemCollectionNameLabel->SetCenterWrap( false );
			m_pItemCollectionNameLabel->SizeToContents();	
		}
		if ( m_pItemCollectionListLabel )
		{
			m_pItemCollectionListLabel->SetFont( m_pFontAttribLarge );
			m_pItemCollectionListLabel->SizeToContents();
		}

		if ( !m_bResizeToText )
		{
			int iAttribTall = m_pItemAttribLabel->GetWide() ? m_pItemAttribLabel->GetTall() : 0;
			int iNameTall = m_bAttribOnly ? 0 : m_pItemNameLabel->GetTall();
			int iTotalH = iAttribTall + iNameTall;

			// If these fonts won't fit, use the smaller ones
			if ( m_pItemNameLabel->GetWide() > iTextW || (!m_bNameOnly && m_pItemAttribLabel->GetWide() > iTextW) || (iTotalH > h) )
			{
				m_pItemNameLabel->SetFont( m_pFontNameSmall );
				m_pItemAttribLabel->SetFont( m_pFontAttribSmall );

				m_pItemNameLabel->InvalidateLayout( true );
				m_pItemNameLabel->SizeToContents();

				iAttribTall = m_pItemAttribLabel->GetWide() ? m_pItemAttribLabel->GetTall() : 0;
				iNameTall = m_pItemNameLabel->GetTall();
				iTotalH = iAttribTall + iNameTall;

				// If they don't fit, go to the smallest
				if ( m_pItemNameLabel->GetWide() > iTextW || (!m_bNameOnly && m_pItemAttribLabel->GetWide() > iTextW) || (iTotalH > h) )
				{
					m_pItemNameLabel->SetFont( m_pFontNameSmallest );
					m_pItemAttribLabel->SetFont( m_pFontAttribSmallest );

					m_pItemNameLabel->InvalidateLayout( true );
					m_pItemNameLabel->SizeToContents();
				}
			}
		}
	}

	// If it still doesn't fit, turn on wrap and pray
	if ( m_pItemNameLabel->GetWide() > iTextW )
	{
		m_pItemNameLabel->SetCenterWrap( true );
	}

	if ( m_pItemAttribLabel->GetWide() > iTextW )
	{
		m_pItemAttribLabel->SetWrap( true );
	}

	// Now restore the sizes
	m_pItemNameLabel->SetSize( iTextW, m_pItemNameLabel->GetTall() );
	m_pItemAttribLabel->SetSize( iTextW, m_pItemAttribLabel->GetTall() );

	if ( m_pItemEquippedLabel )
	{
		m_pItemEquippedLabel->SetPos( GetWide() - m_pItemEquippedLabel->GetWide() - m_iEquippedInsetX, GetTall() - m_pItemEquippedLabel->GetTall() - m_iEquippedInsetY );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetItem( const CEconItemView *pItem )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	HideContainedItemPanel();

	bool bMatch = false;

	if ( pItem && pItem->IsValid() )
	{
		// Items with kill eater attributes never match the previous version of themselves. This stops
		// the code from otherwise being intelligent and preventing the complicated update of the item
		// description, but in this case our kill count is part of that description and we want it to
		// get updated.
		static CSchemaFieldHandle<CEconItemAttributeDefinition> pAttrib_KillEater( "kill eater" );
		const bool bCanMatch = !pItem->FindAttribute( pAttrib_KillEater );

		if ( bCanMatch && m_ItemData.IsValid() )
		{
			if ( m_ItemData.GetItemID() != INVALID_ITEM_ID )
			{
				bool bUseIndexCompare = false;
					
#ifdef TF_CLIENT_DLL
				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
				{
					if ( ( m_ItemData.GetItemID() == 1 ) && ( m_ItemData.GetItemID() == pItem->GetItemID() ) )
					{
						// Items the bots carry in MvM all have itemID of 1, so we need to compare the item index
						bUseIndexCompare = true;
					}
				}
#endif
				 
				if ( bUseIndexCompare )
				{
					bMatch = m_ItemData.GetItemDefIndex() == pItem->GetItemDefIndex();
				}
				else
				{
					// Our current item is non-base. We need to match global indices.
					bMatch = ( m_ItemData.GetItemID() == pItem->GetItemID() );
				}
			}
			else if ( pItem->GetItemID() == INVALID_ITEM_ID )
			{
				static CSchemaFieldHandle<CEconItemAttributeDefinition> pAttrib_ToolTarget( "tool target item" );
				bMatch &= pItem->FindAttribute( pAttrib_ToolTarget );

				// Our current item is a base item. Our new item needs to be base too, and match item indices and quality
				bMatch &= ( m_ItemData.GetItemDefIndex() == pItem->GetItemDefIndex() ) &&
						  ( m_ItemData.GetItemQuality() == pItem->GetItemQuality() ) &&
						  ( m_ItemData.GetSOCData() == pItem->GetSOCData() );
			}
		}
		if ( !bMatch )
		{
			// cancel weapon skin composition for old item
			if ( m_ItemData.IsValid() )
			{
				m_ItemData.CancelWeaponSkinComposite();
			}

			m_ItemData = *pItem;

			if ( m_bIsMouseOverPanel )
			{
				LoadResFileForCurrentItem( false );
			}

			// If the item hasn't built its attribute string, go ahead and do that.
			m_ItemData.SetGrayedOutReason( GetGreyedOutReason() );
		}
		else
		{
			// The rest of the data may match, but we still need the inventory position updates
			m_ItemData.SetInventoryPosition( pItem->GetInventoryPosition() );
		}

		ShowContainedItemPanel( pItem );
	}
	else
	{
		// cancel weapon skin composition for old item
		if ( m_ItemData.IsValid() )
		{
			m_ItemData.CancelWeaponSkinComposite();
		}
		else
		{
			bMatch = true;
		}

		m_ItemData.GetAttributeList()->DestroyAllAttributes();
		m_ItemData.Invalidate();
	}

	// only update panels when item is not matched
	if ( !bMatch )
	{
		UpdatePanels();
	}

	// TODO: Update only description for strange item in the same panel
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::Dragged( bool bDragging )
{
	if ( m_pContainedItemPanel )
	{
		if ( bDragging )
		{
			m_pContainedItemPanel->SetActAsButton( false, false );
			m_pContainedItemPanel->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::ShowContainedItemPanel( const CEconItemView *pItem )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// If this item contains another item, create an interior item model panel.
	if ( pItem->GetSOCData() && pItem->GetSOCData()->GetInteriorItem() && m_pContainedItemPanel )
	{
		if ( !m_pContainedItemPanel->IsContainedItem() )
		{
			m_pContainedItemPanel->SetContainedItem( true );
			m_pContainedItemPanel->InvalidateLayout( false, true );
		}

		CEconItem *pInteriorItem = pItem->GetSOCData()->GetInteriorItem();
		if ( !pInteriorItem )
			return;

		const IEconTool *pEconTool = pItem->GetItemDefinition()
								   ? pItem->GetItemDefinition()->GetEconTool()
								   : NULL;
		if ( !pEconTool )
			return;

		if ( !pEconTool->ShouldShowContainedItemPanel( pItem ) )
		{
			// Only show this to non-local, non-wrapping players if we've been told to (usually in trading panel)
			if ( !m_bShowOthersGiftWrappedItems )
				return;
		}

		SetNeedsToLoad();

		m_pContainedItemPanel->SetEconItem( pInteriorItem );
		m_pContainedItemPanel->SetVisible( true );
		m_pContainedItemPanel->SetActAsButton( false, true );
		m_pContainedItemPanel->SetTooltip( GetTooltip(), "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::HideContainedItemPanel()
{
	if ( m_pContainedItemPanel && m_pContainedItemPanel->IsVisible() )
	{
		m_pContainedItemPanel->SetActAsButton( false, false );
		m_pContainedItemPanel->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetEconItem( CEconItem* pItem )
{
	m_ItemData.SetItemDefIndex( pItem->GetDefinitionIndex() );
	m_ItemData.SetItemQuality( pItem->GetQuality() );
	m_ItemData.SetItemLevel( pItem->GetItemLevel() );
	m_ItemData.SetItemID( pItem->GetItemID() );
	m_ItemData.SetNonSOEconItem( pItem );
	m_ItemData.SetInitialized( true );

#ifdef CLIENT_DLL
	m_ItemData.SetIsTradeItem( false );
	m_ItemData.SetItemQuantity( pItem->GetQuantity() );
#endif

	m_ItemData.GetAttributeList()->DestroyAllAttributes();

	if ( m_pModelPanel )
	{
		m_pModelPanel->SetItem( &m_ItemData );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetNoItemText( const char *pszText ) 
{
	m_pszNoItemText = pszText;
	CleanupNoItemWChars();

	if ( !HasItem() ) 
	{
		UpdatePanels(); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::CleanupNoItemWChars( void )
{
	if ( m_pwcNoItemText )
	{
		delete m_pwcNoItemText;
		m_pwcNoItemText = NULL;
	}
	if ( m_pwcNoItemAttrib )
	{
		delete m_pwcNoItemAttrib;
		m_pwcNoItemAttrib = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemModelPanel::UpdateSeriesLabel()
{
	// hijacking m_bShowQuantity here to indicate "show things where the quantity counter goes", in this case meaning "show crate series indicator"
	if ( m_pSeriesLabel && m_bShowQuantity )
	{
		static CSchemaAttributeDefHandle pAttrDef_CrateSeries( "set supply crate series" );
		static CSchemaAttributeDefHandle pAttrDef_HideSeries( "hide crate series number" );

		float fCrateSeries;
		if ( pAttrDef_CrateSeries && FindAttribute_UnsafeBitwiseCast<attrib_value_t>( &m_ItemData, pAttrDef_CrateSeries, &fCrateSeries ) && pAttrDef_HideSeries && !m_ItemData.FindAttribute( pAttrDef_HideSeries ) )
		{
			wchar_t wszSeries[16]=L"";
			_snwprintf( wszSeries, ARRAYSIZE( wszSeries ), L"#%i", (int)fCrateSeries );
			m_pSeriesLabel->SetVisible( true );
			m_pSeriesLabel->SetText( wszSeries );
			
			return true;
		}
		else
		{
			m_pSeriesLabel->SetVisible( false );

			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Read through a few items and see if they match the recipe's criteria
//			Show elipses while still tallying.  Remove our tick once all items
//			are tallied.
//-----------------------------------------------------------------------------
bool CItemModelPanel::CheckRecipeMatches()
{
	// Don't do this if either we or our parent are invisible
	if( !IsVisible() || ( GetParent() && !GetParent()->IsVisible() ) )
		return false;

	const IEconTool* pTool = m_ItemData.GetStaticData()->GetEconTool();

	// If this isnt a dynamic recipe tool, dont show or do any of this
	if( !pTool 
		|| V_stricmp( m_ItemData.GetStaticData()->GetEconTool()->GetTypeName() , "dynamic_recipe")
		|| m_ItemData.GetStaticData()->GetDefaultLoadoutSlot() != INVALID_EQUIPPED_SLOT )
	{
		if( m_pMatchesLabel )
		{
			m_pMatchesLabel->SetVisible( false );
		}

		return false;
	}

	bool bStillWorking = true;
	if( m_pMatchesLabel && m_bShowQuantity )
	{
		CPlayerInventory *pLocalInv = TFInventoryManager()->GetLocalInventory();
		if ( pLocalInv == NULL )
			return false;

		// We still need to match recipe components
		if ( m_nRecipeMatchingIndex < pLocalInv->GetItemCount() )
			sai_NumLoadingRequests[LOADING_RECIPE_MATCHES]++;

		if ( se_CurrentLoadingTask == LOADING_RECIPE_MATCHES )
		{
			// Go through our entire backpack and check for matches, but only go through a few at a time
			while ( m_nRecipeMatchingIndex < pLocalInv->GetItemCount() && sm_flLoadingTimeThisFrame < tf_time_loading_item_panels.GetFloat() )
			{
				// Mark this time
				float flTime = Plat_FloatTime();

				CEconItemView *pItem = pLocalInv->GetItem( m_nRecipeMatchingIndex );
				Assert( pItem );

				// Check each item
				CRecipeComponentMatchingIterator matchingIterator( &m_ItemData, pItem );
				m_ItemData.IterateAttributes( &matchingIterator );
				const CUtlVector< const CEconItemAttributeDefinition* >& matchingAttribs = matchingIterator.GetMatchingComponentInputs();
				Assert( matchingAttribs.Count() <= 1 );
				FOR_EACH_VEC( matchingAttribs, j )
				{
					CAttribute_DynamicRecipeComponent value;
					const CEconItemAttributeDefinition* pAttrib = matchingAttribs[j];
					attrib_definition_index_t nIndex = pAttrib->GetDefinitionIndex();
					m_ItemData.FindAttribute( pAttrib, &value );

					// Add this entry if it doesnt exist in out map yet
					if( m_mapMatchingAttributes.Find( nIndex ) == m_mapMatchingAttributes.InvalidIndex() )
					{
						m_mapMatchingAttributes.Insert( nIndex );
						m_mapMatchingAttributes[ m_mapMatchingAttributes.Find( nIndex ) ] = 0;
					}

					// Increment this value if it's less than the max needed
					int &nCount = m_mapMatchingAttributes[ m_mapMatchingAttributes.Find( nIndex ) ];
					if( (unsigned)nCount < ( value.num_required() - value.num_fulfilled() ) )
					{
						++nCount;
					}
				}

				m_nRecipeMatchingIndex++;
				// Accumulate time
				sm_flLoadingTimeThisFrame += ( Plat_FloatTime() - flTime );
			}
		}

		bStillWorking = m_nRecipeMatchingIndex != pLocalInv->GetItemCount();
		wchar_t wszMatches[16]=L"...";
		
		if( !bStillWorking )
		{
			CRecipeComponentMatchingIterator matchingIterator( &m_ItemData, NULL );
			m_ItemData.IterateAttributes( &matchingIterator );
			int nTotalAttribs = matchingIterator.GetTotalInputs() - matchingIterator.GetInputsFulfilled();
			int nMatchingAttribs = 0;

			unsigned short index = m_mapMatchingAttributes.FirstInorder();
			while( index != m_mapMatchingAttributes.InvalidIndex() )
			{
				nMatchingAttribs += m_mapMatchingAttributes[ index ];
				index = m_mapMatchingAttributes.NextInorder( index );
			}

			// Fill out the actual number of matches
			_snwprintf( wszMatches, ARRAYSIZE( wszMatches ), L"%i/%i", nMatchingAttribs, nTotalAttribs );
		}
	
		m_pMatchesLabel->SetVisible( true );
		m_pMatchesLabel->SetText( wszMatches );
	}

	return bStillWorking;
}

void CItemModelPanel::UpdateDescription( bool bIsToolTip /* = false */ )
{
	if ( !m_bDescriptionDirty )
		return;

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	m_bDescriptionDirty = false;
	
	enum { kAttribBufferSize = 4 * 1024 };
	wchar_t wszAttribBuffer[ kAttribBufferSize ] = L"";

	enum { kCollectionBufferSize = 4 * 1024 };
	wchar_t wszCollectionListBuffer[kCollectionBufferSize] = L"";

	wchar_t wszCollectionNameBuffer[512] = L"";

	if ( !m_bNameOnly )
	{
		const CEconItemDescription *pDescription = m_ItemData.GetDescription( bIsToolTip );
		if ( pDescription )
		{
			unsigned int unWrittenLines = 0;
			unsigned int unWrittenCollectionLines = 0;
			for ( unsigned int i = 0; i < pDescription->GetLineCount(); i++ )
			{
				const econ_item_description_line_t& line = pDescription->GetLine(i);

				// skip the bonus content for mouse over panel
				if ( m_bIsMouseOverPanel && line.unMetaType & kDescLineFlag_CaseBonusContent )
					continue;

				// skip mouse over panel only line
				if ( !m_bIsMouseOverPanel && line.unMetaType & kDescLineFlag_MouseOverPanel )
					continue;

				// m_bSpecialAttributesOnly, only show purple and orange text, ignore rest
				if ( m_bSpecialAttributesOnly )
				{
					if ( line.eColor == ATTRIB_COL_UNUSUAL || line.eColor == ATTRIB_COL_STRANGE )
					{
						V_wcscat_safe( wszAttribBuffer, unWrittenLines++ == 0 ? L"" : L"\n" );					// add empty lines everywhere except before the first line
						V_wcscat_safe( wszAttribBuffer, line.sText.Get() );
					}	
				}
				else if ( ( line.unMetaType & kDescLineFlag_CollectionName ) != 0 )
				{
					// Ignore name spacers
					if ( !( line.unMetaType & kDescLineFlag_Empty) )
					{
						V_wcscat_safe( wszCollectionNameBuffer, line.sText.Get() );
					}
				}
				else if ( ( line.unMetaType & kDescLineFlag_Collection ) != 0 )
				{		
					V_wcscat_safe( wszCollectionListBuffer, unWrittenCollectionLines++ == 0 ? L"" : L"\n" );					// add empty lines everywhere except before the first line
					V_wcscat_safe( wszCollectionListBuffer, line.sText.Get() );
				}
				else if ( (line.unMetaType & kDescLineFlag_Name ) == 0 )
				{
					V_wcscat_safe( wszAttribBuffer, unWrittenLines++ == 0 ? L"" : L"\n" );					// add empty lines everywhere except before the first line
					V_wcscat_safe( wszAttribBuffer, line.sText.Get() );
				}
			}

			// If we have an unknown name, we should try to rebuild for "awhile"
			m_bDescriptionDirty |= pDescription->HasUnknownPlayer();
		}
	}
		
	if( m_pMainContentContainer )
	{
		m_pMainContentContainer->SetDialogVariable( "attriblist", wszAttribBuffer );
		m_pMainContentContainer->SetDialogVariable( "collectionname", wszCollectionNameBuffer );
		m_pMainContentContainer->SetDialogVariable( "collectionlist", wszCollectionListBuffer );
		m_pMainContentContainer->SetDialogVariable( "itemname", m_ItemData.GetItemName() );
	}

	if ( m_pItemNameLabel )
	{
		uint8 nRarity = m_ItemData.GetRarity();
		const char* pszRarityColor = GetItemSchema()->GetRarityColor( nRarity );

		// Set the name to the quality color
		// Rarity Econ Colorization
		EEconItemQuality eQuality = (EEconItemQuality)m_ItemData.GetItemQuality();
		if ( pszRarityColor && ( eQuality != AE_SELFMADE ) && ( eQuality != AE_UNUSUAL ) )
		{
			m_pItemNameLabel->SetColorStr( pszRarityColor );
		}
		else 
		{
			const char *pszQualityColorString = EconQuality_GetColorString( eQuality );
			if ( m_ItemData.IsValid() && !m_bStandardTextColor && pszQualityColorString )
			{
				m_pItemNameLabel->SetColorStr( pszQualityColorString );
			}
			else
			{
				m_pItemNameLabel->SetColorStr( m_OrgItemTextColor );
			}
		}
		m_pItemNameLabel->SetVisible( !m_bAttribOnly );
	}

	if ( m_pItemAttribLabel )
	{
		m_pItemAttribLabel->SetVisible( !m_bNameOnly );
	}

	bool bCollectionVisible = m_bHideCollectionPanel ? false : !m_bNameOnly;

	if ( m_pItemCollectionNameLabel )
	{
		m_pItemCollectionNameLabel->SetVisible( bCollectionVisible );
	}
	if ( m_pItemCollectionListLabel )
	{
		m_pItemCollectionListLabel->SetVisible( bCollectionVisible );
	}
	if ( m_pItemCollectionHighlight )
	{
		m_pItemCollectionHighlight->SetVisible( bCollectionVisible );
	}

	InvalidateLayout( true );

	// Now that we've built the attribute description, give the attribute colors to our label
	if ( m_pItemAttribLabel && !m_bNameOnly && m_pItemAttribLabel->GetTextImage() && m_ItemData.GetDescription() )
	{
		const CEconItemDescription *pDescription = m_ItemData.GetDescription();
		vgui::TextImage *pAttrTextImage = m_pItemAttribLabel->GetTextImage();
		pAttrTextImage->ClearColorChangeStream();

		vgui::TextImage *pCollectionNameTextImage = m_pItemCollectionNameLabel ? m_pItemCollectionNameLabel->GetTextImage() : NULL;
		if ( pCollectionNameTextImage )
			pCollectionNameTextImage->ClearColorChangeStream();

		vgui::TextImage *pCollectionListTextImage = m_pItemCollectionListLabel ? m_pItemCollectionListLabel->GetTextImage() : NULL;
		if ( pCollectionListTextImage )
			pCollectionListTextImage->ClearColorChangeStream();

		vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

		Color prevAttrColor(0,0,0);
		Color prevCollectionColor(0,0,0);
		unsigned int unCurrentAttrTextStreamIndex = 0;
		unsigned int unCurrentCollectionNameTextStreamIndex = 0;
		unsigned int unCurrentCollectionListTextStreamIndex = 0;
		int iCollectionLineCount = 0;

		if ( m_pItemCollectionHighlight )
		{
			m_pItemCollectionHighlight->SetVisible( false );
		}

		for ( unsigned int i = 0; i < pDescription->GetLineCount(); i++ )
		{
			const econ_item_description_line_t& line = pDescription->GetLine(i);

			// skip the bonus content for mouse over panel
			if ( m_bIsMouseOverPanel && line.unMetaType & kDescLineFlag_CaseBonusContent )
				continue;

			// skip mouse over panel only line
			if ( !m_bIsMouseOverPanel && line.unMetaType & kDescLineFlag_MouseOverPanel )
				continue;

			// Ignore the name line, it was added above
			if ( ( line.unMetaType & kDescLineFlag_Name ) != 0 )
				continue;

			// collection
			int fontHeight = surface()->GetFontTall( m_pFontAttribSmall );
			if ( ( line.unMetaType & (kDescLineFlag_Collection | kDescLineFlag_CollectionName | kDescLineFlag_CollectionCurrentItem ) ) != 0 && pCollectionNameTextImage && pCollectionListTextImage )
			{
				bool bIsCollectionName = ( line.unMetaType & kDescLineFlag_CollectionName ) != 0;
				vgui::TextImage *pTextImage = bIsCollectionName ? pCollectionNameTextImage : pCollectionListTextImage;
				unsigned int &unCurrentCollectionTextStreamIndex = bIsCollectionName ? unCurrentCollectionNameTextStreamIndex : unCurrentCollectionListTextStreamIndex;

				bool bIsCurrentItem = ( line.unMetaType & kDescLineFlag_CollectionCurrentItem ) != 0;
				// use bg color as text color for current item for a better highlight
				Color col = bIsCurrentItem ? Color( 0, 0, 0, 255 ) : pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );
				// Output a color change if necessary.
				if ( i == 0 || prevCollectionColor != col )
				{
					pTextImage->AddColorChange( col, unCurrentCollectionTextStreamIndex );
					prevCollectionColor = col;
				}

				unCurrentCollectionTextStreamIndex += StringFuncs<locchar_t>::Length( line.sText.Get() ) + 1;	// add one character to deal with newlines

				if ( bIsCollectionName )
				{
					continue;
				}

				// Current line highlight
				if ( bIsCurrentItem && m_pItemCollectionHighlight )
				{
					// use text color as bg color for the current item for a better highlight
					Color bgColor = pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );

					// Get the current ypos
					int x, y;
					m_pItemCollectionListLabel->GetPos( x, y );
					m_pItemCollectionHighlight->SetPos( x, y + iCollectionLineCount * fontHeight );
					m_pItemCollectionHighlight->SetBgColor( bgColor );
					m_pItemCollectionHighlight->SetVisible( bCollectionVisible );
				}
				iCollectionLineCount++;
			}
			else 
			{
				Color col = pScheme->GetColor( GetColorNameForAttribColor( line.eColor ), Color( 255, 255, 255, 255 ) );

				// m_bSpecialAttributesOnly, only show purple and orange text, ignore rest
				if ( m_bSpecialAttributesOnly )
				{
					if ( ( line.eColor != ATTRIB_COL_UNUSUAL && line.eColor != ATTRIB_COL_STRANGE ) )
					{
						continue;
					}
				}

				// Output a color change if necessary.
				if ( i == 0 || prevAttrColor != col )
				{
					pAttrTextImage->AddColorChange( col, unCurrentAttrTextStreamIndex );
					prevAttrColor = col;
				}
				unCurrentAttrTextStreamIndex += StringFuncs<locchar_t>::Length( line.sText.Get() ) + 1;	// add one character to deal with newlines
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::DirtyDescription()
{
	m_bDescriptionDirty = true;

	if ( HasItem() )
		GetItem()->OnAttributeValuesChanged();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemModelPanel::UpdateMatchesLabel()
{
	const IEconTool* pTool = m_ItemData.GetStaticData()->GetEconTool();

	if( !pTool || Q_stricmp( m_ItemData.GetStaticData()->GetEconTool()->GetTypeName() , "dynamic_recipe") )
	{
		return false;
	}

	m_nRecipeMatchingIndex = 0;
	m_mapMatchingAttributes.Purge();
	SetNeedsToLoad();
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemModelPanel::UpdateQuantityLabel()
{
	if ( m_pItemQuantityLabel )
	{
		bool bVisible = m_bShowQuantity && m_ItemData.GetStaticData() != NULL;
		if ( bVisible )
		{
			const IEconTool *pEconTool = m_ItemData.GetStaticData()->GetEconTool();
			if ( pEconTool && pEconTool->ShouldDisplayQuantity( &m_ItemData ) )
			{
				wchar_t wszQuantity[16]=L"";
				_snwprintf( wszQuantity, ARRAYSIZE( wszQuantity ), L"%i", m_ItemData.GetQuantity() );
				m_pItemQuantityLabel->SetVisible( true );
				m_pItemQuantityLabel->SetText( wszQuantity );
			}
			else
			{
				bVisible = false;
			}
		}
		m_pItemQuantityLabel->SetVisible( bVisible );

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Simple utility function to allocate memory and duplicate a wide string
 */
inline wchar_t *CloneWString( const wchar_t *str )
{
	const int nLen = V_wcslen(str)+1;
	wchar_t *cloneStr = new wchar_t [ nLen ];
	const int nSize = nLen * sizeof( wchar_t );
	V_wcsncpy( cloneStr, str, nSize );
	return cloneStr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetNoItemText( const wchar_t *pwszTitleOverride, const wchar_t *pwszAttribs, int iNegAttribsBegin )
{ 
	static CSchemaColorDefHandle pColorDef_DescAttribPositive( "desc_attrib_positive" );
	static CSchemaColorDefHandle pColorDef_DescAttribNegative( "ItemAttribNegative" );

	CleanupNoItemWChars();

	m_pwcNoItemText = CloneWString( pwszTitleOverride );
	m_pszNoItemText = NULL;

	if ( pwszAttribs )
	{
		m_pwcNoItemAttrib = CloneWString( pwszAttribs ); 

		if ( m_pItemAttribLabel && m_pItemAttribLabel->GetTextImage() )
		{
			m_pItemAttribLabel->GetTextImage()->ClearColorChangeStream();

			if ( iNegAttribsBegin )
			{
				vgui::IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

				if ( pColorDef_DescAttribPositive )
				{
					Color col = pScheme->GetColor( pColorDef_DescAttribPositive->GetColorName(), Color(255,255,255,255) );
					m_pItemAttribLabel->GetTextImage()->AddColorChange( col, 0 );
				}

				if ( pColorDef_DescAttribNegative )
				{
					Color col = pScheme->GetColor( pColorDef_DescAttribNegative->GetColorName(), Color(255,255,255,255) );
					m_pItemAttribLabel->GetTextImage()->AddColorChange( col, iNegAttribsBegin );
				}
			}
		}
	}

	if ( !HasItem() ) 
	{
		UpdatePanels(); 
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::HideAllModifierIcons()
{
	if ( m_pPaintIcon )
	{
		m_pPaintIcon->SetVisible( false );
	}
	if ( m_pTF2Icon )
	{
		m_pTF2Icon->SetVisible( false );
	}
	if ( m_pItemEquippedLabel )
	{
		m_pItemEquippedLabel->SetVisible( false );
	}
	if ( m_pItemQuantityLabel )
	{
		m_pItemQuantityLabel->SetVisible( false );
	}
	if ( m_pVisionRestrictionImage )
	{
		m_pVisionRestrictionImage->SetVisible( false );
	}
	if ( m_pIsStrangeImage )
	{
		m_pIsStrangeImage->SetVisible( false );
	}
	if ( m_pIsUnusualImage )
	{
		m_pIsUnusualImage->SetVisible( false );
	}
	if ( m_pIsLoanerImage )
	{
		m_pIsLoanerImage->SetVisible( false );
	}
	if ( m_pSeriesLabel )
	{
		m_pSeriesLabel->SetVisible( false );
	}
	if ( m_pMatchesLabel )
	{
		m_pMatchesLabel->SetVisible( false );
	}
	if ( m_pItemEquippedLabel && m_bForceShowEquipped )
	{
		m_pItemEquippedLabel->SetVisible( m_bForceShowEquipped );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::UpdatePanels( void )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !m_pModelPanel )
		return;

	m_pModelPanel->SetModelHidden( m_bHideModel );

	// By default we dont need a tick.
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );

	// Default to loading icons
	se_CurrentLoadingTask = LOADING_ICONS;

	m_pModelPanel->SetItem( &m_ItemData );
	// We need to load if our image isn't in memory already.
	if ( !m_bHideModel && ( m_pModelPanel->IsImageNotLoaded() || m_pModelPanel->IsLoadingWeaponSkin() ) )
	{
		if ( m_pMainContentContainer )
		{
			m_pMainContentContainer->SetVisible( false );
		}

		// Show the spinner
		if ( m_pLoadingSpinner )
		{
			m_pLoadingSpinner->SetVisible( true );
		}

		SetNeedsToLoad();
	}
	else
	{
		// hide the spinner
		if ( m_pLoadingSpinner )
		{
			m_pLoadingSpinner->SetVisible( false );
		}
	}

	if ( !HasItem() )
	{
		if ( m_bModelOnly )
		{
			if ( m_pItemNameLabel )
			{
				m_pItemNameLabel->SetVisible( false );
			}
			if ( m_pItemAttribLabel )
			{
				m_pItemAttribLabel->SetVisible( false );
			}
		}
		else
		{
			if ( m_pItemNameLabel )
			{
				const wchar_t *wcNOText = NULL;
				if ( m_pszNoItemText && m_pszNoItemText[0] )
				{
					wcNOText = g_pVGuiLocalize->Find( m_pszNoItemText ); 
				}
				else if ( m_pwcNoItemText )
				{
					wcNOText = m_pwcNoItemText;
				}

				if ( wcNOText && wcNOText[0] )
				{
					if ( m_pMainContentContainer )
						m_pMainContentContainer->SetDialogVariable( "itemname", wcNOText );
					m_pItemNameLabel->SetVisible( true );
					m_pItemNameLabel->SetColorStr( m_NoItemTextColor );

					m_pItemNameLabel->InvalidateLayout(true,true);
					if ( m_iForceTextSize && m_iForceTextSize <= 3 )
					{
						// Leave center wrap on if the noitem text is getting to use the whole panel
						if ( !m_bNoItemFullPanel )
						{
							m_pItemNameLabel->SetCenterWrap( false );
						}

						switch ( m_iForceTextSize )
						{
						case 1:
							m_pItemNameLabel->SetFont( m_pFontNameLarge );
							break;

						case 2:
							m_pItemNameLabel->SetFont( m_pFontNameSmall );
							break;

						case 3:
							m_pItemNameLabel->SetFont( m_pFontNameSmallest );
							break;
						}
					}
				}
				else
				{
					m_pItemNameLabel->SetVisible( false );
				}
			}
			
			if ( !m_bNameOnly && m_pwcNoItemAttrib )
			{
				if ( m_pMainContentContainer )
					m_pMainContentContainer->SetDialogVariable( "attriblist", m_pwcNoItemAttrib );
				if ( m_pItemAttribLabel && !m_pItemAttribLabel->IsVisible() )
				{
					m_pItemAttribLabel->SetVisible( true );
				}
			}
			else
			{
				if ( m_pItemAttribLabel && m_pItemAttribLabel->IsVisible() )
				{
					m_pItemAttribLabel->SetVisible( false );
				}
			}
		}

		HideAllModifierIcons();
		return;
	}

	if ( m_bHideModifierIcons )
	{
		HideAllModifierIcons();
		return;
	}

	if ( m_pPaintIcon )
	{
		m_pPaintIcon->SetVisible( false );
		if ( !m_bHideModel && !m_bHidePaintIcon )
		{
			// Empty out our list of paint colors. We may or may not put things back in -- an empty
			// list at the end means "don't draw the paint icon".
			m_pPaintIcon->m_colPaintColors.RemoveAll();

			// Fetch custom texture, if any
			m_pPaintIcon->m_hUGCId = m_ItemData.GetCustomUserTextureID();
			if ( m_pPaintIcon->m_hUGCId != 0 )
				m_pPaintIcon->SetVisible( true );

			// Don't show paint icons on any tools, their icon contains the color
			const bool bIsEconTool = m_ItemData.GetItemDefinition()->IsTool();

			// Has the item been painted?
			int iRGB0 = m_ItemData.GetModifiedRGBValue( false ),
				iRGB1 = m_ItemData.GetModifiedRGBValue( true );

			if ( !bIsEconTool && (iRGB0 != 0 || iRGB1 != 0))
			{
				m_pPaintIcon->SetVisible( true );
				m_pPaintIcon->m_colPaintColors.AddToTail( Color( clamp( (iRGB0 & 0xFF0000) >> 16, 0, 255 ), clamp( (iRGB0 & 0xFF00) >> 8, 0, 255 ), clamp( (iRGB0 & 0xFF), 0, 255 ), 255 ) );
				if ( iRGB0 != iRGB1 )
				{
					m_pPaintIcon->m_colPaintColors.AddToTail( Color( clamp( (iRGB1 & 0xFF0000) >> 16, 0, 255 ), clamp( (iRGB1 & 0xFF00) >> 8, 0, 255 ), clamp( (iRGB1 & 0xFF), 0, 255 ), 255 ) );
				}
			}
		}
	}

	if ( m_pTF2Icon )
	{
		if ( m_bHideModel || m_bHidePaintIcon )
		{
			m_pTF2Icon->SetVisible( false );
		}
		else
		{
			m_pTF2Icon->SetVisible( m_ItemData.GetSOCData() && m_ItemData.GetSOCData()->IsForeign() );
		}
	}

	if ( m_bNoItemFullPanel )
	{
		// If we're a noitem-fullpanel mode, we don't show strings when we have an item.
		m_pItemNameLabel->SetVisible( false );
		m_pItemAttribLabel->SetVisible( false );
	}
	else if ( m_bModelOnly )
	{
		if ( m_pItemNameLabel )
		{
			m_pItemNameLabel->SetVisible( false );
		}
		if ( m_pItemAttribLabel )
		{
			m_pItemAttribLabel->SetVisible( false );
		}
	}
	else
	{
		// deferred description loading
		m_bDescriptionDirty = true;
		SetNeedsToLoad();
		if ( m_pMainContentContainer )
			m_pMainContentContainer->SetDialogVariable( "itemname", "" );
	}

	if ( m_pItemEquippedLabel )
	{
		m_pItemEquippedLabel->SetVisible( m_bForceShowEquipped || (m_bShowEquipped && IsEquipped()) );
	}

	// Hide all of these labels
	if( m_pMatchesLabel )
	{
		m_pMatchesLabel->SetVisible( false );
	}

	if( m_pSeriesLabel )
	{
		m_pSeriesLabel->SetVisible( false );
	}

	if( m_pItemQuantityLabel )
	{
		m_pItemQuantityLabel->SetVisible( false );
	}


	// Update that number in the top right
	if ( !UpdateMatchesLabel() )
	{
		if ( !UpdateSeriesLabel() )
		{
			UpdateQuantityLabel();
		}
	}

	if ( m_pVisionRestrictionImage )
	{
		int nVisionFilterFlags = 0;
		const CEconItemDefinition *pData = m_ItemData.GetItemDefinition();
		if ( !m_bModelOnly && pData )
		{
			nVisionFilterFlags = pData->GetVisionFilterFlags();

			// Add support for all the holidays and "vision" mode restrictions
			if ( pData->GetHolidayRestriction() )
			{
				int iHolidayRestriction = UTIL_GetHolidayForString( pData->GetHolidayRestriction() );
				switch ( iHolidayRestriction )
				{
				default:
				case kHoliday_None:
				case kHoliday_TFBirthday:
				case kHoliday_Christmas:
				case kHoliday_Valentines:
				case kHoliday_MeetThePyro:
				case kHoliday_AprilFools:
				case kHoliday_EOTL:
				case kHoliday_CommunityUpdate:
					break;

				case kHoliday_Halloween:
				case kHoliday_FullMoon:
				case kHoliday_HalloweenOrFullMoon:
				case kHoliday_HalloweenOrFullMoonOrValentines:
					#ifdef TF_CLIENT_DLL
						nVisionFilterFlags |= TF_VISION_FILTER_HALLOWEEN;
					#endif
					break;
				}
			}
		}

		switch ( nVisionFilterFlags )
		{
			default:
				AssertMsg1( false, "Unexpected vision restriction flags %d", nVisionFilterFlags );
			case 0:
				m_pVisionRestrictionImage->SetVisible( false );
				break;
#ifdef TF_CLIENT_DLL
			case 1:
				m_pVisionRestrictionImage->SetImage( "viewmode_pyrovision" );
				m_pVisionRestrictionImage->SetVisible( true );
				break;
			case 2:
				// Check if most players who have not specifically opted in will see the item.
				if ( TFGameRules() ? TFGameRules()->IsHolidayActive( kHoliday_HalloweenOrFullMoon ) : TF_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) )
				{
					m_pVisionRestrictionImage->SetImage( "viewmode_spooky" );
				}
				else
				{
					m_pVisionRestrictionImage->SetImage( "viewmode_spooky_off" );
				}
				m_pVisionRestrictionImage->SetVisible( true );
				break;
			case 4:
				m_pVisionRestrictionImage->SetVisible( false );
				break;
#endif
		}
	}

	// Strange Icon
	if ( m_pIsStrangeImage )
	{
		m_pIsStrangeImage->SetVisible( false );

		if ( !m_bIsMouseOverPanel )
		{
			// Allow for already strange items
			bool bIsStrange = false;
			if ( m_ItemData.GetQuality() == AE_STRANGE )
			{
				bIsStrange = true;
			}

			if ( !bIsStrange )
			{
				// Go over the attributes of the item, if it has any strange attributes the item is strange and don't apply
				for ( int i = 0; i < GetKillEaterAttrCount(); i++ )
				{
					if ( m_ItemData.FindAttribute( GetKillEaterAttr_Score( i ) ) )
					{
						bIsStrange = true;
						break;
					}
				}
			}
			if ( bIsStrange )
			{
				if ( GetStattrak( &m_ItemData ) )
				{
					m_pIsStrangeImage->SetImage( "viewmode_statclock" );
				}
				else
				{
					m_pIsStrangeImage->SetImage( "viewmode_strange" );
				}
				m_pIsStrangeImage->SetVisible( true );
			}
		}
	}
	
	// Unusual Icon
	if ( m_pIsUnusualImage )
	{
		m_pIsUnusualImage->SetVisible( false );
	
		static CSchemaAttributeDefHandle pAttrDef_ParticleEffect( "attach particle effect" );
		static CSchemaAttributeDefHandle pAttrDef_TauntParticle( "taunt attach particle index" );
		if ( pAttrDef_ParticleEffect && pAttrDef_TauntParticle && !m_bIsMouseOverPanel )
		{
			// Cant use quality cause of old legacy items.  Quality is just a quick test
			if ( m_ItemData.FindAttribute( pAttrDef_ParticleEffect ) || m_ItemData.FindAttribute( pAttrDef_TauntParticle ) )
			{
				m_pIsUnusualImage->SetImage( "viewmode_unusual" );
				m_pIsUnusualImage->SetVisible( true );
			}
		}
	}

	if ( m_pIsLoanerImage )
	{
		m_pIsLoanerImage->SetVisible( false );
		if ( !m_bIsMouseOverPanel && GetAssociatedQuestID( &m_ItemData ) != INVALID_ITEM_ID )
		{
			m_pIsLoanerImage->SetImage( "viewmode_loaner" );
			m_pIsLoanerImage->SetVisible( true );
		}
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemModelPanel::IsEquipped( void )
{
	if ( !HasItem() )
		return false;

	return m_ItemData.IsEquipped();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetGreyedOut( const char *pszGreyedOutReason ) 
{ 
	m_pszGreyedOutReason = pszGreyedOutReason;
	if ( m_pModelPanel )
	{
		m_pModelPanel->SetGreyedOut( m_pszGreyedOutReason != NULL );
	}
	UpdateEquippedLabel(); 
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CItemModelPanel::HasItem( void )
{
	return m_ItemData.IsValid();
}

void CItemModelPanel::SetModelIsHidden( bool bHideModel )
{ 
	m_bHideModel = bHideModel;
	if ( m_pModelPanel )
	{
		m_pModelPanel->SetModelHidden( bHideModel );
	}
}

void CItemModelPanel::OnTick()
{
	bool bStillWorking = LoadData();
	if ( m_pContainedItemPanel )
	{
		bStillWorking |= m_pContainedItemPanel->LoadData();
	}

	// If we're done working, we dont need to tick anymore
	if ( !bStillWorking )
	{
		LoadDataCompleted();
	}

	BaseClass::OnTick();
}

void CItemModelPanel::SetNeedsToLoad()
{
	vgui::ivgui()->AddTickSignalToHead( GetVPanel() );
}

bool CItemModelPanel::LoadData()
{
	// Different frame?
	if ( sm_nCurrentDecriptionUpdateFrame != gpGlobals->framecount )
	{
		// Reset
		sm_nCurrentDecriptionUpdateFrame = gpGlobals->framecount;
		sm_flLoadingTimeThisFrame = 0.f;

		// Figure out which loading we're going to do.  We want to load
		// certain things sooner (visual things, ie icons) than we start
		// figuring out recipe matches.
		eLoadingType_t type = NUM_LOADING_TYPES;
		for( int i=0; i < NUM_LOADING_TYPES; ++i )
		{
			if ( sai_NumLoadingRequests[i] > 0 && eLoadingType_t(i) < type )
			{
				type = eLoadingType_t(i);
			}
			sai_NumLoadingRequests[i] = 0;
		}

		se_CurrentLoadingTask = type;
	}

	bool bStillWorking = CheckRecipeMatches();

	if ( !m_bHideModel && m_pModelPanel )
	{
		bool bImageLoaded = true;
		bool bLoadingWeaponSkin = m_pModelPanel->IsLoadingWeaponSkin();
		bool bLoadingBackpackIcon = m_pModelPanel->IsImageNotLoaded();

		if ( bLoadingWeaponSkin || bLoadingBackpackIcon )
		{
			// We still need to load icons
			sai_NumLoadingRequests[LOADING_ICONS]++;
			if ( sm_flLoadingTimeThisFrame < tf_time_loading_item_panels.GetFloat() && se_CurrentLoadingTask == LOADING_ICONS )
			{
				float flTime = Plat_FloatTime();

				// no need to load texture if we're doing composite weapon skin
				if ( bLoadingWeaponSkin )
				{
					g_pMatSystemSurface->BeginSkinCompositionPainting();
					m_pModelPanel->Paint();
					g_pMatSystemSurface->EndSkinCompositionPainting();
				}
			
				if ( bLoadingBackpackIcon )
				{
					m_pModelPanel->LoadInventoryImage();
				}

				// Accumulate time
				sm_flLoadingTimeThisFrame += ( Plat_FloatTime() - flTime );
			}

			bStillWorking = m_pModelPanel->IsLoadingWeaponSkin() || m_pModelPanel->IsImageNotLoaded();
			bImageLoaded = !bStillWorking;
		}

		// Hide the spinner and show the main container
		if ( bImageLoaded )
		{
			if ( m_pMainContentContainer && !m_pMainContentContainer->IsVisible() )
			{
				m_pMainContentContainer->SetVisible( true );
			}

			if ( m_pLoadingSpinner && m_pLoadingSpinner->IsVisible() )
			{
				m_pLoadingSpinner->SetVisible( false );
			}
		}
	}

	if ( m_bDescriptionDirty && !IsContainedItem() )
	{
		// We still need to load our description
		sai_NumLoadingRequests[LOADING_DESCRIPTIONS]++;
		// Check if we're clear to update.  We only want to eat up a little slice of time.
		if ( sm_flLoadingTimeThisFrame < tf_time_loading_item_panels.GetFloat() && se_CurrentLoadingTask == LOADING_DESCRIPTIONS )
		{
			float flTime = Plat_FloatTime();
			// Update!
			UpdateDescription();

			// Accumulate time
			sm_flLoadingTimeThisFrame += ( Plat_FloatTime() - flTime );
		}

		bStillWorking |= m_bDescriptionDirty;
	}

	return bStillWorking;
}

void CItemModelPanel::LoadDataCompleted()
{
	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::SetActAsButton( bool bClickable, bool bMouseOver ) 
{ 
	m_bClickable = bClickable; 
	m_bMouseOver = bMouseOver; 

	SetMouseInputEnabled( m_bClickable || m_bMouseOver );
}

void CItemModelPanel::NavigateTo()
{
	BaseClass::NavigateTo();

	if ( IsPC() )
	{
		RequestFocus( 0 );
	}
}

void CItemModelPanel::NavigateFrom()
{
	BaseClass::NavigateFrom();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::OnCursorEntered( void )
{
	if ( !m_bMouseOver )
		return;

	if ( m_bShouldSendPanelEnterExits )
	{
		PostActionSignal( new KeyValues("ItemPanelEntered") );
	}

	if ( IsEnabled() && !IsSelected() )
	{
		NavigateTo();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::OnCursorExited( void )
{
	if ( !m_bMouseOver )
		return;

	if ( m_bShouldSendPanelEnterExits )
	{
		PostActionSignal( new KeyValues("ItemPanelExited") );
	}

	if ( IsSelected() )
	{
		NavigateFrom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
extern ISoundEmitterSystemBase *soundemitterbase;
void CItemModelPanel::OnMousePressed(vgui::MouseCode code)
{
	if ( code == MOUSE_RIGHT )
	{
		PostActionSignal( new KeyValues("ItemPanelMouseRightRelease") );
	}

	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("ItemPanelMousePressed") );

	// audible feedback
	const char *soundFilename = "ui/buttonclick.wav";

	if ( m_bUseItemSounds )
	{
		CEconItemView *item = GetItem();
		if ( item )
		{
			soundFilename = item->GetDefinitionString( "mouse_pressed_sound", "ui/item_default_pickup.wav" );
		}
	}

	const char *pszSound = UTIL_GetRandomSoundFromEntry( soundFilename );
	if ( pszSound && pszSound[0] )
	{
		vgui::surface()->PlaySound( pszSound );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::OnMouseReleased(vgui::MouseCode code)
{
	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("ItemPanelMouseReleased") );

	// audible feedback
	// we're not using item sounds here because they are better handled by the drag/drop code elsewhere
	if ( !m_bUseItemSounds )
	{
		vgui::surface()->PlaySound( "ui/buttonclickrelease.wav" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItemModelPanel::OnMouseDoublePressed(vgui::MouseCode code)
{
	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("ItemPanelMouseDoublePressed") );

	// audible feedback
	const char *soundFilename = "ui/buttonclickrelease.wav";

	if ( m_bUseItemSounds )
	{
		CEconItemView *item = GetItem();
		if ( item )
		{
			soundFilename = item->GetDefinitionString( "mouse_double_pressed_sound", "ui/item_default_drop.wav" );
		}
	}

	vgui::surface()->PlaySound( soundFilename );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanel::OnCursorMoved( int x, int y )
{
	if ( !m_bClickable )
		return;

	// Add our own xpos/ypos offset
	int iXPos;
	int iYPos;
	GetPos( iXPos, iYPos );
	PostActionSignal( new KeyValues("ItemPanelCursorMoved", "x", x + iXPos, "y", y + iYPos) );
}

void CItemModelPanel::OnKeyCodePressed( vgui::KeyCode code )
{
	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "sellitem" ) )
	{
		if ( HasItem() && steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
		{
			const char *pszPrefix = "";
			if ( GetUniverse() == k_EUniverseBeta )
			{
				pszPrefix = "beta.";
			}
			uint32 nAssetContext = 2; // k_EEconContextBackpack
			char szURL[512];
			V_snprintf( szURL, sizeof(szURL), "http://%ssteamcommunity.com/my/inventory/?sellOnLoad=1#%d_%d_%llu", pszPrefix, engine->GetAppID(), nAssetContext, GetItem()->GetItemID() );
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( szURL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanel::UpdateEquippedLabel( void )
{
	if ( !m_pItemEquippedLabel )
		return;

	if ( IsGreyedOut() )
	{
		m_pItemEquippedLabel->SetFgColor( Color(96,96,96,255) );
	}
	else
	{
		m_pItemEquippedLabel->SetFgColor( Color(200,80,60,255) );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanel::SetSkin( int iSkin )
{
	if ( m_pModelPanel )
	{
		m_pModelPanel->SetSkin( iSkin );
	}
}


itempanel_tooltippos_t g_iTooltipStrategies[NUM_IPTTP_STRATEGIES][NUM_POSITIONS_PER_STRATEGY] =
{
	{ IPTTP_LEFT, IPTTP_LEFT_CENTERED, IPTTP_ABOVE, IPTTP_BELOW, IPTTP_RIGHT_CENTERED, IPTTP_RIGHT },	// IPTTP_LEFT_SIDE
	{ IPTTP_RIGHT, IPTTP_RIGHT_CENTERED, IPTTP_ABOVE, IPTTP_BELOW, IPTTP_LEFT_CENTERED, IPTTP_LEFT },	// IPTTP_RIGHT_SIDE
	{ IPTTP_ABOVE, IPTTP_LEFT_CENTERED, IPTTP_RIGHT_CENTERED, IPTTP_LEFT, IPTTP_RIGHT, IPTTP_ABOVE },	// IPTTP_TOP_SIDE
	{ IPTTP_BELOW, IPTTP_LEFT_CENTERED, IPTTP_RIGHT_CENTERED, IPTTP_LEFT, IPTTP_RIGHT, IPTTP_ABOVE },	// IPTTP_BOTTOM_SIDE
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CItemModelPanelToolTip::CItemModelPanelToolTip( vgui::Panel *parent, const char *text ) 
: vgui::BaseTooltip( parent, text )
, m_pMouseOverItemPanel( NULL )
, m_iPositioningStrategy( IPTTP_BOTTOM_SIDE )
{
	m_hCurrentPanel = NULL;
	SetTooltipDelay( 100 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanelToolTip::GetPosition( itempanel_tooltippos_t iTooltipPosition, CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	switch ( iTooltipPosition )
	{
	case IPTTP_LEFT:
		*iXPos = (iItemX - m_pMouseOverItemPanel->GetWide() + XRES(18));
		*iYPos = iItemY - YRES(7);
		break;
	case IPTTP_RIGHT: 
		*iXPos = (iItemX + pItemPanel->GetWide() - XRES(20));
		*iYPos = iItemY - YRES(7);
		break;
	case IPTTP_LEFT_CENTERED:
		*iXPos = (iItemX - m_pMouseOverItemPanel->GetWide()) - XRES(4);
		*iYPos = (iItemY - (m_pMouseOverItemPanel->GetTall() * 0.5));
		break;
	case IPTTP_RIGHT_CENTERED:
		*iXPos = (iItemX + pItemPanel->GetWide()) + XRES(4);
		*iYPos = (iItemY - (m_pMouseOverItemPanel->GetTall() * 0.5));
		break;
	case IPTTP_ABOVE:
		*iXPos = (iItemX + (pItemPanel->GetWide() * 0.5)) - (m_pMouseOverItemPanel->GetWide() * 0.5);
		*iYPos = (iItemY - m_pMouseOverItemPanel->GetTall() - YRES(4));
		break;
	case IPTTP_BELOW:
		*iXPos = (iItemX + (pItemPanel->GetWide() * 0.5)) - (m_pMouseOverItemPanel->GetWide() * 0.5);
		*iYPos = (iItemY + pItemPanel->GetTall() + YRES(4));
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CItemModelPanelToolTip::ValidatePosition( CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos )
{
	bool bSucceeded = true;

	// Make sure the popup stays onscreen.
	if ( *iXPos < 0 )
	{
		*iXPos = 0;
	}
	else if ( (*iXPos + m_pMouseOverItemPanel->GetWide()) > m_pParentPanel->GetWide() )
	{
		int iXPosNew = m_pParentPanel->GetWide() - m_pMouseOverItemPanel->GetWide();
		// make sure it is still on the screen
		if ( iXPosNew >= 0 )
		{
			*iXPos = iXPosNew;
		}
		else
		{
			bSucceeded = false;
		}
	}

	if ( *iYPos < 0 )
	{
		*iYPos = 0;
	}
	else if ( (*iYPos + m_pMouseOverItemPanel->GetTall() + YRES(32)) > m_pParentPanel->GetTall() )
	{
		// Move it up above our item
		int iYPosNew = iItemY - m_pMouseOverItemPanel->GetTall() - YRES(4);
		// make sure it is still on the screen
		if ( iYPosNew >= 0 )
		{
			*iYPos = iYPosNew;
		}
		else
		{
			bSucceeded = false;
		}
	}

	if ( bSucceeded )
	{
		// We also fail if moving it to keep it on screen moved it over the item panel itself
		Vector2D vecToolTipMin, vecToolTipMax, vecItemMin, vecItemMax;
		vecToolTipMin.x = *iXPos;
		vecToolTipMin.y = *iYPos;
		vecToolTipMax.x = vecToolTipMin.x + m_pMouseOverItemPanel->GetWide();
		vecToolTipMax.y = vecToolTipMin.y + m_pMouseOverItemPanel->GetTall();

		vecItemMin.x = iItemX;
		vecItemMin.y = iItemY;
		vecItemMax.x = vecItemMin.x + m_hCurrentPanel->GetWide();
		vecItemMax.y = vecItemMin.y + m_hCurrentPanel->GetTall();
		
		bSucceeded = !( vecToolTipMin.x < vecItemMax.x && vecToolTipMax.x > vecItemMin.x &&	vecToolTipMin.y < vecItemMax.y && vecToolTipMax.y > vecItemMin.y );
	}

	return bSucceeded;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanelToolTip::PerformLayout() 
{
	BaseClass::PerformLayout();

	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	CItemModelPanel *pItemPanel = m_hCurrentPanel.Get();
	if ( m_pMouseOverItemPanel && pItemPanel ) 
	{		
		CEconItemView *pItem = pItemPanel->GetItem();
		if ( pItem && pItemPanel->ShouldShowTooltip() /*&& !IsIgnoringItemPanelEnters()*/ )
		{
			m_pMouseOverItemPanel->SetGreyedOut( pItemPanel->GetGreyedOutReason() );
			m_pMouseOverItemPanel->SetItem( pItem );
			m_pMouseOverItemPanel->DirtyDescription(); // Force rebuilding the description when we first display
			m_pMouseOverItemPanel->UpdateDescription( true );
			m_pMouseOverItemPanel->HideContainedItemPanel();
			m_pMouseOverItemPanel->InvalidateLayout(true);

			int x,y;

			// If the panel is somewhere in a derived class, we need to get its position in our space
			if ( pItemPanel->GetParent() != m_pMouseOverItemPanel->GetParent() )
			{
				int iItemAbsX, iItemAbsY;
				vgui::ipanel()->GetAbsPos( pItemPanel->GetVPanel(), iItemAbsX, iItemAbsY );
				int iParentAbsX, iParentAbsY;
				vgui::ipanel()->GetAbsPos( m_pMouseOverItemPanel->GetParent()->GetVPanel(), iParentAbsX, iParentAbsY );

				x = (iItemAbsX - iParentAbsX);
				y = (iItemAbsY - iParentAbsY);
			}
			else
			{
				pItemPanel->GetPos( x, y );
			}

			int iXPos = 0;
			int iYPos = 0;

			// Loop through the positions in our strategy, and hope we find a valid spot
			for ( int i = 0; i < NUM_POSITIONS_PER_STRATEGY; i++ )
			{
				itempanel_tooltippos_t iPos = g_iTooltipStrategies[m_iPositioningStrategy][i];
				GetPosition( iPos, pItemPanel, x, y, &iXPos, &iYPos );

				if ( ValidatePosition( pItemPanel, x, y, &iXPos, &iYPos ) )
					break;
			}

			m_pMouseOverItemPanel->SetPos( iXPos, iYPos );
			m_pMouseOverItemPanel->SetVisible( true );			
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanelToolTip::ShowTooltip( Panel *currentPanel ) 
{ 
	if ( m_pMouseOverItemPanel && currentPanel != m_hCurrentPanel.Get() ) 
	{
		CItemModelPanel *pItemPanel = assert_cast<CItemModelPanel *>(currentPanel);
		m_hCurrentPanel.Set( pItemPanel );
		pItemPanel->PostActionSignal( new KeyValues("ItemPanelEntered") );
		vgui::surface()->PlaySound( "ui/item_info_mouseover.wav" );

		m_pMouseOverItemPanel->HideContainedItemPanel();
	}
	BaseClass::ShowTooltip( currentPanel );	
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CItemModelPanelToolTip::HideTooltip() 
{
	if ( m_pMouseOverItemPanel ) 
	{
		m_pMouseOverItemPanel->SetVisible( false ); 
	}

	if ( m_hCurrentPanel )
	{
		m_hCurrentPanel.Get()->PostActionSignal( new KeyValues("ItemPanelExited") );
		m_hCurrentPanel = NULL;
	}
}
