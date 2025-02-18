//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ITEM_MODEL_PANEL_H
#define ITEM_MODEL_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Tooltip.h>
#include "basemodel_panel.h"
#include "basemodelpanel.h"
#include "econ_controls.h"

#define ITEM_MODEL_IMAGE_CACHE_SIZE		3

#define ATTRIB_LABEL_INDENT				XRES(5)

class CEconItemView;
class CItemModelPanel;
class CItemMaterialCustomizationIconPanel;
class CIconRenderReceiver;
class CEmbeddedItemModelPanel;

enum itempanel_tooltippos_t
{
	IPTTP_LEFT,
	IPTTP_RIGHT,
	IPTTP_LEFT_CENTERED,
	IPTTP_RIGHT_CENTERED,
	IPTTP_ABOVE,
	IPTTP_BELOW,

	NUM_POSITIONS_PER_STRATEGY
};


enum itempanel_tooltip_strategies_t
{
	IPTTP_LEFT_SIDE,				
	IPTTP_RIGHT_SIDE,				
	IPTTP_TOP_SIDE,
	IPTTP_BOTTOM_SIDE,

	NUM_IPTTP_STRATEGIES,
};
extern itempanel_tooltippos_t g_iTooltipStrategies[NUM_IPTTP_STRATEGIES][NUM_POSITIONS_PER_STRATEGY];

struct item_model_cache_t
{
	item_model_cache_t()
	{
		Clear();
	}
	void Clear()
	{
		iItemID = INVALID_ITEM_ID;
		iItemDefinitionIndex = INVALID_ITEM_DEF_INDEX;
		iWidth = 0;
		iHeight = 0;
		m_hModelPanelLock = NULL;
	}

	itemid_t	iItemID;
	item_definition_index_t iItemDefinitionIndex;
	int			iWidth;
	int			iHeight;
	vgui::DHANDLE<CEmbeddedItemModelPanel>	m_hModelPanelLock;
};

class CItemMaterialCustomizationIconPanel : public vgui::Panel 
{
	DECLARE_CLASS_SIMPLE( CItemMaterialCustomizationIconPanel, vgui::Panel );
public:
	CItemMaterialCustomizationIconPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CItemMaterialCustomizationIconPanel();

	// Custom painting
	virtual void PaintBackground( void );
	void DrawQuad( int iSubtileIndex, int iSubtileCount );

	int m_iPaintSplat;

	// UGC file of custom texture we are using.  0 in the more common case of none.
	uint64 m_hUGCId;

	// Paint color. 
	CUtlVector<Color> m_colPaintColors;
};


//-----------------------------------------------------------------------------
// Purpose: The model panel that's embedded inside the CItemModelPanel vgui panel
//-----------------------------------------------------------------------------
class CEmbeddedItemModelPanel : public CBaseModelPanel
{
	DECLARE_CLASS_SIMPLE( CEmbeddedItemModelPanel, CBaseModelPanel );

public:
	CEmbeddedItemModelPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CEmbeddedItemModelPanel();

	virtual void	PerformLayout( void ) OVERRIDE;

	virtual void	Paint( void );
	virtual void	RenderingRootModel( IMatRenderContext *pRenderContext, CStudioHdr *pStudioHdr, MDLHandle_t mdlHandle, matrix3x4_t *pWorldMatrix ) OVERRIDE;
	virtual IMaterial *GetOverrideMaterial( MDLHandle_t mdlHandle ) OVERRIDE;
	
	CEconItemView*	GetItem() const { return m_pItem; }
	void			SetItem( CEconItemView *pItem );
	bool			IsForcingModelUsage( void ) { return m_bForceUseModel; }
	void			SetForceModelUsage( bool bUseModel ) { m_bForceUseModel = bUseModel; }
	bool			IsImageNotLoaded( void ) const;
	bool			IsLoadingWeaponSkin( void ) const;

	enum InventoryImageType_t
	{
		IMAGETYPE_SMALL,
		IMAGETYPE_LARGE,
		IMAGETYPE_DETAILED,				// show in detailed view if present -- defaults to "large" image if not
	};

	InventoryImageType_t	GetInventoryImageType() const								{ return static_cast<InventoryImageType_t>( m_iInventoryImageType ); }
	void					SetInventoryImageType( InventoryImageType_t eNewImageType ) { m_iInventoryImageType = (int)eNewImageType; }
	void					LoadInventoryImage();

	void					SetGreyedOut( bool bGreyedOut ) { m_bGreyedOut = bGreyedOut; }
	void					SetModelHidden( bool bModelHidden ) { m_bModelIsHidden = bModelHidden; }

	ITexture				*GetCachedGeneratedIcon();
	bool					m_bOfflineIconGeneration;

private:
	bool					ShouldUseRenderTargetAsIcon() const;
	bool					UseRenderTargetAsIcon() const { return m_bUseRenderTargetAsIcon || m_bUseItemRenderTarget; }

	CEconItemView			*m_pItem;			// For directly specifying the item associated with this panel.
	int						m_iTextureID;
	CUtlMap<int, int>		m_iOverlayTextureIDs;
	const char*				m_pszToolTargetItemImage;
	int						m_iToolTargetItemTextureID;
	Vector					m_vecToolTargetItemImageOffset;

	bool					m_bImageNotLoaded;

	bool					m_bGreyedOut;
	bool					m_bModelIsHidden;

	bool					m_bIsFestivized;
	bool					m_bIsPaintKitItem;
	bool					m_bUseRenderTargetAsIcon; // same as m_bUseItemRenderTarget but set by attribute instead of res file

	void					CleanUpCachedWeaponIcon();
	bool					m_bWeaponAllowInspect;
	int						m_iCachedTextureID;
	CIconRenderReceiver		*m_pCachedWeaponIcon;
	IMaterial				*m_pCachedWeaponMaterial;

	MDLData_t				m_ItemModel;
	int						m_iPedestalAttachment;

	void					UpdateCameraForIcon();
	int						m_iCameraAttachment;

	bool					RenderStatTrack( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix );
	MDLData_t				m_StatTrackModel;

	bool					RenderAttachedModels( CStudioHdr *pStudioHdr, matrix3x4_t *pWorldMatrix );
	void					LoadAttachedModel( attachedmodel_t *pModel );

	CUtlVector< MDLData_t > m_AttachedModels;

	float					m_flStatTrackScale;

	CPanelAnimationVar( bool, m_bForceUseModel, "force_use_model", "0" );
	CPanelAnimationVar( bool, m_bUseItemRenderTarget, "use_item_rendertarget", "0" );
	CPanelAnimationVar( int, m_iInventoryImageType, "inventory_image_type", "0" );
	CPanelAnimationVar( bool, m_bForceSquareImage, "force_square_image", "0" );
	CPanelAnimationVar( float,  m_flModelRotateYawSpeed, "model_rotate_yaw_speed", "0" );
	CPanelAnimationVar( bool, m_bUsePedestal, "use_pedestal", "0" );

	bool					UpdateParticle(
											IMatRenderContext				*pRenderContext, 
											CStudioHdr						*pStudioHdr, 
											MDLHandle_t						mdlHandle, 
											matrix3x4_t						*pWorldMatrix
											);

	particle_data_t			*m_pItemParticle;

};

IMaterial* GetMaterialForImage( CEmbeddedItemModelPanel::InventoryImageType_t eImageType, const char* pszBaseName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CItemModelPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CItemModelPanel, vgui::EditablePanel );
public:
	CItemModelPanel( vgui::Panel *parent, const char *name );
	virtual ~CItemModelPanel( void );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void	ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void	PerformLayout( void ) OVERRIDE;
	virtual void	PaintTraverse( bool forceRepaint, bool allowForce ) OVERRIDE;
	virtual void	OnSizeChanged( int newWide, int newTall );

	void	ResizeLabels( void );
	virtual void	SetItem( const CEconItemView *pItem );
	void	SetEconItem( CEconItem* pItem );
	bool	HasItem( void );
	CEconItemView *GetItem( void ) { return ( m_ItemData.IsValid() ? &m_ItemData : NULL ); }
	bool	ModelIsHidden( void ) const { return m_bHideModel; }
	void	SetModelIsHidden( bool bHideModel );
	virtual void OnTick() OVERRIDE;
	void	SetNeedsToLoad();
	bool	LoadData();
	void	LoadDataCompleted();

	CExLabel	*GetNameLabel( void ) { return m_pItemNameLabel; }

	// Functionality to allow custom blobs of text to be displayed
	void	SetNoItemText( const char *pszText );
	void	SetNoItemText( const wchar_t *pwszTitleOverride, const wchar_t *pwszAttribs = NULL, int iNegAttribsBegin = 0 );

	// Button functionality
	void	SetActAsButton( bool bClickable, bool bMouseOver );
	virtual void NavigateTo();
	virtual void NavigateFrom();
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseDoublePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );

	void	SetSelected( bool bSelected ) { m_bSelected = bSelected; }
	bool	IsSelected( void ) { return m_bSelected; }

	void	SetAttribOnly( bool bAttribs ) { m_bAttribOnly = bAttribs; }
	void	SetTextYPos( int iPos ) { m_iTextYPos = iPos; }
	bool	IsEquipped( void );
	void	SetShowEquipped( bool bShow ) { m_bShowEquipped = bShow; UpdateEquippedLabel(); }
	void	SetForceShowEquipped( bool bForce ) { m_bForceShowEquipped = bForce; UpdateEquippedLabel(); }
	void	SetGreyedOut( const char *pszGreyedOutReason );			// pass in NULL for "not greyed out"
	void	SetShowGreyedOutTooltip( bool bShow ) { m_bShowGreyedOutTooltip = bShow; }
	bool	IsGreyedOut( void ) const { return m_pszGreyedOutReason != NULL; }
	const char *GetGreyedOutReason( void ) const { return m_pszGreyedOutReason; }
	bool	ShouldShowTooltip( void ) { return (!IsGreyedOut() || m_bShowGreyedOutTooltip); }
	void	SetShowQuantity( bool bShow ) { m_bShowQuantity = bShow; }

	void	UpdatePanels( void );

	void	SendPanelEnterExits( bool bSend ) { m_bShouldSendPanelEnterExits = bSend; }
	
	void	SetShouldShowOthersGiftWrappedItems( bool bShow ) { m_bShowOthersGiftWrappedItems = bShow; }
	void    Dragged( bool bDragging );
	void	HideContainedItemPanel();
	void	ShowContainedItemPanel( const CEconItemView *pItem );

	bool	IsContainedItem() { return m_bContainedItem; }
	void	SetContainedItem( bool bVal ) { m_bContainedItem = bVal; }

	void	SetSkin( int iSkin );
	void	SetItemStyle( style_index_t unStyle ) { m_ItemData.SetItemStyleOverride( unStyle ); }
	void	SetNameOnly( bool bNameOnly ) { m_bNameOnly = bNameOnly; }
	void	SetSpecialAttributesOnly( bool bSpecialOnly ) { m_bSpecialAttributesOnly = bSpecialOnly; }

	CEmbeddedItemModelPanel::InventoryImageType_t	GetInventoryImageType() /*const*/													 { return m_pModelPanel->GetInventoryImageType(); }
	void											SetInventoryImageType( CEmbeddedItemModelPanel::InventoryImageType_t eNewImageType ) { m_pModelPanel->SetInventoryImageType( eNewImageType ); }

	void	UpdateDescription( bool bIsToolTip = false );
	void	DirtyDescription();

	virtual void OnCommand( const char *command ) OVERRIDE;

	void	MakeFakeButton() { m_bFakeButton = true; }
	
private:
	void	UpdateEquippedLabel( void );
	void	CleanupNoItemWChars( void );

	bool	UpdateSeriesLabel();
	bool	UpdateMatchesLabel();
	bool	UpdateQuantityLabel();

	bool	CheckRecipeMatches();

	int		GetAttribWide( int iMaxWide ) { return (m_iTextWide ? m_iTextWide : (iMaxWide - (ATTRIB_LABEL_INDENT * 2))); }

	void	LoadResFileForCurrentItem( bool bForceLoad );

	void	HideAllModifierIcons();

	enum eLoadingType_t
	{
		LOADING_ICONS = 0,
		LOADING_DESCRIPTIONS,
		LOADING_RECIPE_MATCHES,
		
		NUM_LOADING_TYPES
	};

	enum eLoadedCollectionType_t
	{
		LOADED_COLLECTION_NONE,
		LOADED_COLLECTION_WEAPON,
		LOADED_COLLECTION_COSMETIC
	};

	vgui::DHANDLE<CEmbeddedItemModelPanel>	m_pModelPanel;
	CExLabel			*m_pItemNameLabel;
	vgui::Label			*m_pItemAttribLabel;
	CExLabel			*m_pItemCollectionNameLabel;
	vgui::Label			*m_pItemCollectionListLabel;
	vgui::EditablePanel *m_pItemCollectionHighlight;
	eLoadedCollectionType_t	m_nCollectionItemLoaded;
	vgui::Label			*m_pItemEquippedLabel;
	vgui::Label			*m_pItemQuantityLabel;
	vgui::ImagePanel	*m_pVisionRestrictionImage;
	vgui::ImagePanel	*m_pIsStrangeImage;
	vgui::ImagePanel	*m_pIsUnusualImage;
	vgui::ImagePanel	*m_pIsLoanerImage;
	vgui::Label			*m_pSeriesLabel;
	vgui::Label			*m_pMatchesLabel;
	vgui::EditablePanel *m_pMainContentContainer;
	vgui::ImagePanel	*m_pLoadingSpinner;
	CEconItemView		m_ItemData;
	vgui::HFont			m_pFontNameSmallest;
	vgui::HFont			m_pFontNameSmall;
	vgui::HFont			m_pFontNameLarge;
	vgui::HFont			m_pFontNameLarger;
	vgui::HFont			m_pFontAttribSmallest;
	vgui::HFont			m_pFontAttribSmall;
	vgui::HFont			m_pFontAttribLarge;
	vgui::HFont			m_pFontAttribLarger;
	const char			*m_pszNoItemText;
	const wchar_t		*m_pwcNoItemText;
	const wchar_t		*m_pwcNoItemAttrib;
	Color				m_NoItemTextColor;
	Color				m_OrgItemTextColor;
	bool				m_bClickable;
	bool				m_bMouseOver;
	bool				m_bSelected;
	bool				m_bShowEquipped;
	bool				m_bForceShowEquipped;
	const char			*m_pszGreyedOutReason;
	bool				m_bShowGreyedOutTooltip;
	bool				m_bShouldSendPanelEnterExits;
	bool				m_bShowQuantity;
	bool			    m_bContainedItem;
	bool				m_bShowOthersGiftWrappedItems;
	bool				m_bDescriptionDirty;
	int					m_nRecipeMatchingIndex;
	static float		sm_flLoadingTimeThisFrame;
	static int			sm_nCurrentDecriptionUpdateFrame;
	static int			sai_NumLoadingRequests[NUM_LOADING_TYPES];
	static eLoadingType_t se_CurrentLoadingTask;
	CUtlMap< attrib_definition_index_t, int > m_mapMatchingAttributes;
	CItemMaterialCustomizationIconPanel	*m_pPaintIcon;
	vgui::ScalableImagePanel	*m_pTF2Icon;

	CItemModelPanel *m_pContainedItemPanel;

	CPanelAnimationVar( bool, m_bSpecialAttributesOnly, "special_attributes_only", "0" );

	CPanelAnimationVarAliasType( int, m_iModelXPos, "model_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelYPos, "model_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelWide, "model_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelTall, "model_tall", "0", "proportional_int" );
	CPanelAnimationVar( bool, m_bModelCenterX, "model_center_x", "0" );
	CPanelAnimationVar( bool, m_bModelCenterY, "model_center_y", "0" );
	CPanelAnimationVarAliasType( int, m_iTF2IconOffsetX, "tf2_icon_offset_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTF2IconOffsetY, "tf2_icon_offset_y", "0", "proportional_int" );

	CPanelAnimationVar( bool, m_bNoItemFullPanel, "noitem_use_fullpanel", "0" );
	CPanelAnimationVar( bool, m_bTextCenter, "text_center", "0" );
	CPanelAnimationVar( bool, m_bTextCenterX, "text_center_x", "0" );
	CPanelAnimationVar( bool, m_bUseItemSounds, "use_item_sounds", "0" );
	CPanelAnimationVar( bool, m_bNameOnly, "name_only", "0" );
	CPanelAnimationVar( bool, m_bAttribOnly, "attrib_only", "0" );
	CPanelAnimationVar( bool, m_bModelOnly, "model_only", "0" );
	CPanelAnimationVar( bool, m_bHideModelDefault, "model_hide", "0" );
	bool m_bHideModel;
	CPanelAnimationVar( bool, m_bHidePaintIcon, "paint_icon_hide", "0" );
	CPanelAnimationVar( bool, m_bResizeToText, "resize_to_text", "0" );
	CPanelAnimationVar( int,  m_iNameLabelAlignment, "name_label_alignment", "4" /*a_center*/ );
	CPanelAnimationVarAliasType( int, m_iTextXPos, "text_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextYPos, "text_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextWide, "text_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextYOffset, "text_yoffset", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iHPadding, "padding_height", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMaxTextHeight, "max_text_height", "0", "proportional_int" );
	CPanelAnimationVar( int, m_iForceTextSize, "text_forcesize", "0"  );

	CPanelAnimationVarAliasType( int, m_iEquippedInsetX, "inset_eq_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEquippedInsetY, "inset_eq_y", "0", "proportional_int" );

	CPanelAnimationVar( bool, m_bStandardTextColor, "standard_text_color", "0" );

	CPanelAnimationVar( bool, m_bIsMouseOverPanel, "is_mouseover", "0" );
	CPanelAnimationVarAliasType( int, m_iBaseWide, "wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBaseTall, "tall", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCollectionListXPos, "collection_list_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextXPosCollection, "text_xpos_collection", "0", "proportional_int" );
	CPanelAnimationVar( bool, m_bHideCollectionPanel, "hide_collection_panel", "0" );
	CPanelAnimationVar( bool, m_bHideModifierIcons, "hide_modifier_icons", "0" );

	bool m_bFakeButton;

	bool m_bInitializedAsContainedItem = false;
};

//-----------------------------------------------------------------------------
// Purpose: Item model panel tooltip. Calls setvisible on the controlled panel
//			and positions it below/above the current panel.
//-----------------------------------------------------------------------------
class CItemModelPanelToolTip : public vgui::BaseTooltip 
{
	DECLARE_CLASS_SIMPLE( CItemModelPanelToolTip, vgui::BaseTooltip );
public:
	CItemModelPanelToolTip(vgui::Panel *parent, const char *text = NULL);

	void SetText(const char *text) { return; }
	const char *GetText() { return NULL; }

	virtual void PerformLayout();
	virtual void ShowTooltip( vgui::Panel *currentPanel );
	virtual void HideTooltip();

	void SetupPanels( vgui::Panel *pParentPanel, CItemModelPanel *pMouseOverItemPanel ) { m_pParentPanel = pParentPanel; m_pMouseOverItemPanel = pMouseOverItemPanel; }
	void SetPositioningStrategy( itempanel_tooltip_strategies_t iStrat ) { m_iPositioningStrategy = iStrat; }

private:
	void GetPosition( itempanel_tooltippos_t iTooltipPosition, CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );
	bool ValidatePosition( CItemModelPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );

private:
	CItemModelPanel	*m_pMouseOverItemPanel;	// This is the tooltip panel we make visible. Must be a CItemModelPanel.
	vgui::Panel		*m_pParentPanel;		// This is the panel that we send item entered/exited messages to
	vgui::DHANDLE<CItemModelPanel> m_hCurrentPanel;

	itempanel_tooltip_strategies_t	m_iPositioningStrategy;
	bool							m_bHorizontalPreferLeft;
};

//-----------------------------------------------------------------------------
// Purpose: Generate a list of items that can be equipped for a given set of
//			restrictions (class/slot/region/etc.) and other data (count of dupes, etc.).
//			Used for crafting, loadout selection, quickswitch, and elsewhere.
//-----------------------------------------------------------------------------
class CEquippableItemsForSlotGenerator
{
public:
	// Flags that control how the data is generated.
	enum
	{
		kSlotGenerator_None						= 0x00,
		kSlotGenerator_ShowDuplicates			= 0x01,
		kSlotGenerator_EquippedSpecialHandling	= 0x02,
	};

	enum EItemDisplayType
	{
		kSlotDisplay_Normal,
		kSlotDisplay_Disabled_EquipRegionConflict,
		kSlotDisplay_Invalid,							// just used to tag unitialized instances
	};

	struct CEquippableResult
	{
		CEquippableResult()
			: m_pEconItemView( NULL )
			, m_eDisplayType( kSlotDisplay_Invalid )
		{
			//
		}

		CEquippableResult( CEconItemView *pEconItemView, EItemDisplayType eDisplayType = kSlotDisplay_Normal )
			: m_pEconItemView( pEconItemView )
			, m_eDisplayType( eDisplayType )
		{
			//
		}

		// This only exists to support Find() and ignores display state.
		bool operator==( const CEquippableResult& other ) const
		{
			return m_pEconItemView == other.m_pEconItemView;
		}

		CEconItemView	*m_pEconItemView;
		EItemDisplayType m_eDisplayType;
	};
	
	typedef CUtlMap<struct item_stack_type_t, int>	DuplicateCountMap_t;
	typedef CUtlVector<CEquippableResult>			EquippableResultsVec_t;

	CEquippableItemsForSlotGenerator( int iClass, int iSlot, equip_region_mask_t unUsedEquipRegionMask, unsigned int unFlags );

	const EquippableResultsVec_t&	GetDisplayItems() const { return m_vecDisplayItems; }
	CEconItemView				   *GetEquippedItem() const { return m_pEquippedItemView; }
	const DuplicateCountMap_t&	    GetDuplicateCountMap() const { return m_DuplicateCountsMap; }

private:
	EquippableResultsVec_t	 m_vecDisplayItems;
	DuplicateCountMap_t		 m_DuplicateCountsMap;
	CEconItemView			*m_pEquippedItemView;			// if kSlotGenerator_EquippedSpecialHandling is passed in, this will store our equipped item; otherwise NULL
};

#endif // ITEM_MODEL_PANEL_H
