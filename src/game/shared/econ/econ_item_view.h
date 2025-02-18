//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef ECON_ITEM_CONSTANTS_H
#define ECON_ITEM_CONSTANTS_H
#ifdef _WIN32
#pragma once
#endif

#include "game_item_schema.h"
#include "econ_item_constants.h"
#include "localization_provider.h"
#include "econ_item_interface.h"
#include "econ_item.h"

#if defined(CLIENT_DLL)
#include "iclientrenderable.h"
#endif

#if defined(TF_DLL)
#include "tf_item_schema.h"
#endif

#if defined(CLIENT_DLL) 
#define CEconItemView C_EconItemView
#endif


#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	#define ENABLE_ATTRIBUTE_CURRENCY_TRACKING	1
#else
	#define ENABLE_ATTRIBUTE_CURRENCY_TRACKING	0
#endif

class CEconItemAttribute;
class CAttributeManager;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CAttributeList
{
	friend class CEconItemView;
	friend class CTFPlayer;

	DECLARE_CLASS_NOBASE( CAttributeList );
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

	CAttributeList();
	void operator=( const CAttributeList &src );

	void					Init();
	void					SetManager( CAttributeManager *pManager );

	void					IterateAttributes( class IEconItemAttributeIterator *pIterator ) const;

	// Remove all attributes on this item
	void					DestroyAllAttributes( void );

	void					AddAttribute( CEconItemAttribute *pAttribute );

	// Remove an attribute by name
	void					RemoveAttribute( const CEconItemAttributeDefinition *pAttrDef );
	void					RemoveAttributeByIndex( int iIndex );

public:
	// Returns the attribute that matches the attribute defname
	const CEconItemAttribute	*GetAttributeByName( const char *pszAttribDefName ) const;

	// Returns the attribute that matches the attribute id
	const CEconItemAttribute	*GetAttributeByID( int iAttributeID ) const;

	// The only way to set the value of an attribute after its creation is through the attribute list 
	// that contains it. This way the matching attribute manager is told one of its attributes has changed.
	void					SetRuntimeAttributeValue( const CEconItemAttributeDefinition *pAttrDef, float flValue );
#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	void					SetRuntimeAttributeRefundableCurrency( const CEconItemAttributeDefinition *pAttrDef, int iRefundableCurrency );
	int						GetRuntimeAttributeRefundableCurrency( const CEconItemAttributeDefinition *pAttrDef ) const;

	void					AdjustRuntimeAttributeRefundableCurrency( const CEconItemAttributeDefinition *pAttrDef, int iRefundableCurrencyAdjustment )
	{
		SetRuntimeAttributeRefundableCurrency( pAttrDef, GetRuntimeAttributeRefundableCurrency( pAttrDef ) + iRefundableCurrencyAdjustment );
	}
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING

private:
	void					NotifyManagerOfAttributeValueChanges();

	// Attribute accessing
	int						GetNumAttributes( void ) const { return m_Attributes.Count(); }
	CEconItemAttribute		*GetAttribute( int iIndex ) { Assert( iIndex >= 0 && iIndex < m_Attributes.Count()); return &m_Attributes[iIndex]; }
	const CEconItemAttribute *GetAttribute( int iIndex ) const { Assert( iIndex >= 0 && iIndex < m_Attributes.Count()); return &m_Attributes[iIndex]; }

	// Our list of attributes
	CUtlVector<CEconItemAttribute>		m_Attributes;

	CAttributeManager		*m_pManager;
};

//-----------------------------------------------------------------------------
// Purpose: An attribute that knows how to read itself from a datafile, describe itself to the user,
//			and serialize itself between Servers, Clients, and Steam.
//			Unlike the attributes created in the Game DLL, this attribute doesn't know how to actually
//			do anything in the game, it just knows how to describe itself.
//-----------------------------------------------------------------------------
class CEconItemAttribute
{
	DECLARE_CLASS_NOBASE( CEconItemAttribute );
public:
	DECLARE_EMBEDDED_NETWORKVAR();

	CEconItemAttribute();
	CEconItemAttribute( const attrib_definition_index_t iAttributeIndex, float flValue );
	CEconItemAttribute( const attrib_definition_index_t iAttributeIndex, uint32 unValue );

	void operator=( const CEconItemAttribute &val );

	// Get the index of this attribute's definition inside the script file
	attrib_definition_index_t			GetAttribIndex( void ) const { return m_iAttributeDefinitionIndex; }
	void			SetAttribIndex( attrib_definition_index_t iIndex ) { m_iAttributeDefinitionIndex = iIndex; }

	// Get the static data contained in this attribute's definition
	const CEconItemAttributeDefinition *GetStaticData( void ) const;

	// Get the float value of this attribute.
	//float			GetValue( void ) const;

#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	int				GetRefundableCurrency( void ) const { return m_nRefundableCurrency; }
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING

private:
	// The only way to set the value of an attribute after its creation is through the attribute list 
	// that contains it. This way the matching attribute manager is told one of its attributes has changed.

	// Set the float value of this attribute.
	// Note that the value must be stored as a float!
	void			SetValue( float flValue );

	// Set the value of this attribute as an unsigned integer.
	// Note that the value must be stored as an integer!
	// See CEconItemAttributeDefinition
	void			SetIntValue( uint32 unValue );

	friend class CAttributeList;

	void			Init( void );

	//--------------------------------------------------------
private:
	// This is the index of the attribute into the attributes read from the data files
	CNetworkVar( attrib_definition_index_t, m_iAttributeDefinitionIndex );

	// This is the value of the attribute. Used to modify the item's variables.
	CNetworkVar( float,	m_flValue );

#if ENABLE_ATTRIBUTE_CURRENCY_TRACKING
	// This is the value that the attribute was first set to by an item definition
	CNetworkVar( int, m_nRefundableCurrency );
#endif // ENABLE_ATTRIBUTE_CURRENCY_TRACKING
};

//-----------------------------------------------------------------------------
// Purpose: An item that knows how to read itself from a datafile, describe itself to the user,
//			and serialize itself between Servers, Clients, and Steam.
//
//			In the client DLL, we derive it from CDefaultClientRenderable so that
//			it can be passed in the pProxyData parameter of material proxies.
//-----------------------------------------------------------------------------
#if defined(CLIENT_DLL)
class CEconItemView : public CDefaultClientRenderable, public CMaterialOverrideContainer< IEconItemInterface >
#else
class CEconItemView : public CMaterialOverrideContainer< IEconItemInterface >
#endif
{
	DECLARE_CLASS_NOBASE( CEconItemView );
public:
	DECLARE_EMBEDDED_NETWORKVAR();
	DECLARE_DATADESC();

public:
	CEconItemView();
	CEconItemView( const CEconItemView &src );
	~CEconItemView();
	CEconItemView&	operator=( const CEconItemView &src );
	bool 				operator==( const CEconItemView &other ) const;
	bool				operator!=( const CEconItemView &other ) const	{ return !operator==( other ); }

	virtual const GameItemDefinition_t *GetItemDefinition() const
	{
		return GetStaticData();
	}

public:

	// IEconItemInterface implementation.
	virtual itemid_t		GetID() const { return GetItemID(); }
	virtual int32			GetQuality() const;
	virtual style_index_t	GetStyle() const;
	virtual uint8			GetFlags() const;
	virtual eEconItemOrigin GetOrigin() const;
	virtual int				GetQuantity() const;
	uint64					GetOriginalID() const { return GetSOCData() ? GetSOCData()->GetOriginalID() : 0; }

	virtual const char	   *GetCustomName() const;
	virtual const char	   *GetCustomDesc() const;

	virtual bool			GetInUse() const { return GetSOCData() ? GetSOCData()->GetInUse() : false; }

	virtual void			IterateAttributes( class IEconItemAttributeIterator *pIterator ) const OVERRIDE;

	bool					IsValid( void ) const { return m_bInitialized; }
	void					Invalidate( void ) { m_bInitialized = false; m_iItemDefinitionIndex = INVALID_ITEM_DEF_INDEX; m_iItemID = INVALID_ITEM_ID; }
	void					InvalidateColor() { m_bColorInit = false; }
	void					InvalidateOverrideColor() { m_bPaintOverrideInit = false; }

	bool					IsUndefined() const { return !IsValid() || m_iItemDefinitionIndex == INVALID_ITEM_DEF_INDEX; }

	// Initialize from the specified data
	// client will load SO cache as needed
	void					Init( int iDefIndex, int iQuality, int iLevel, uint32 iAccountID = 0 );
	void					SetInitialized( bool bInit ) { m_bInitialized = bInit; }

	// Get the static data contained in this item's definition
	GameItemDefinition_t	*GetStaticData( void ) const;

	void					SetNonSOEconItem( CEconItem* pItem ) { m_pNonSOEconItem.SetItem( pItem ); }

	void					OnAttributeValuesChanged()
	{
		NetworkStateChanged();
		MarkDescriptionDirty();
	}

private:
	void					EnsureDescriptionIsBuilt( void ) const;
	void					MarkDescriptionDirty( void );
public:
	void					SetGrayedOutReason( const char *pszGrayedOutReason );

	// Set & Get the index of this item's definition inside the script file
	void					SetItemDefIndex( item_definition_index_t iIndex ) { m_iItemDefinitionIndex = iIndex; MarkDescriptionDirty(); }
	virtual					item_definition_index_t	GetItemDefIndex( void ) const { return m_iItemDefinitionIndex; }

	// Set & Get the quality & level of this item.
	void					SetItemQuality( int iQuality ) { m_iEntityQuality = iQuality; MarkDescriptionDirty(); }
	int						GetItemQuality( void ) const { return m_iEntityQuality; }
	void					SetItemLevel( uint32 unLevel ) { m_iEntityLevel = unLevel; MarkDescriptionDirty(); }
	uint32					GetItemLevel( void ) const { return m_iEntityLevel; }

	int						GetItemQuantity() const;
#ifdef CLIENT_DLL
	void					SetIsTradeItem( bool bIsTradeItem ) { m_bIsTradeItem = bIsTradeItem; MarkDescriptionDirty(); }
	void					SetItemQuantity( int iQuantity ) { m_iEntityQuantity = iQuantity; MarkDescriptionDirty(); }
	void					SetClientItemFlags( uint8 unFlags );

	void					SetItemStyleOverride( style_index_t unNewStyleOverride );
	void					SetItemOriginOverride( eEconItemOrigin unNewOriginOverride );
#endif
	style_index_t			GetItemStyle() const;

	// Access the worldwide global index of this item
	void					SetItemID( itemid_t iIdx ) { m_iItemID = iIdx; m_iItemIDHigh = (m_iItemID >> 32); m_iItemIDLow = (m_iItemID & 0xFFFFFFFF); }
#ifdef CLIENT_DLL
	// On the client, we need to rebuild it from the high & low networked pieces
	itemid_t			GetItemID( void ) const { uint64 iTmp = ((((int64)m_iItemIDHigh)<<32) | m_iItemIDLow); return (itemid_t)iTmp; }
#else
	itemid_t			GetItemID( void ) const { return m_iItemID; }
#endif

	uint32					GetAccountID( void ) const { return m_iAccountID; }
	void					SetOverrideAccountID( uint32 nAccountID ) { m_iAccountID = nAccountID; }

	// Access the inventory position of this item
	void					SetInventoryPosition( uint32 iPosition ) { m_iInventoryPosition = iPosition; }
	const uint32			GetInventoryPosition( void ) const { return m_iInventoryPosition; } 

	// Return the model to use for model panels containing this item
	const char				*GetInventoryModel( void );
	// Return the image to use for model panels containing this item
	const char				*GetInventoryImage( void );
	bool					GetInventoryImageData( int *iPosition, int *iSize );
	const char				*GetInventoryOverlayImage( int idx );
	int						GetInventoryOverlayImageCount( void );

	// Return the model to use when displaying this model on the player character model, if any
	const char				*GetPlayerDisplayModel( int iClass, int iTeam ) const;

	// Return the model to use when displaying this model in the world. See the notes on this in econ_item_schema.h
	const char				*GetWorldDisplayModel() const;
	const char				*GetExtraWearableModel() const;
	const char				*GetExtraWearableViewModel() const;
	const char				*GetVisionFilteredDisplayModel() const;

	// Return the load-out slot that this item must be placed into
	int						GetAnimationSlot( void ) const;
	
	// Return an int that indicates whether the item should be dropped from a dead owner.
	int						GetDropType( void );

	// Remove all attributes on this item
	void					DestroyAllAttributes( void );

	void					InitNetworkedDynamicAttributesForDemos( void );

	// Items that have attributes that modify their RGB values
	int						GetModifiedRGBValue( bool bAltColor=false );

	// Returns the UGC file ID of the custom texture assigned to this item.  If non-zero, then it has a custom texture.
	uint64					GetCustomUserTextureID();

	CEconItem				*GetSOCData( void ) const;

	bool					IsEquipped( void ) const { return GetSOCData() && GetSOCData()->IsEquipped(); }
	bool					IsEquippedForClass( equipped_class_t unClass ) const { return GetSOCData() && GetSOCData()->IsEquippedForClass( unClass ); }
	equipped_slot_t			GetEquippedPositionForClass( equipped_class_t unClass ) const { return GetSOCData() ? GetSOCData()->GetEquippedPositionForClass( unClass ) : INVALID_EQUIPPED_SLOT; }

	// Attached particle systems
	int						GetQualityParticleType() const;

	int						GetSkin( int iTeam, bool bViewmodel = false ) const;

public:
	// ...
	CAttributeList			 *GetAttributeList() { return &m_AttributeList; }
	const CAttributeList	 *GetAttributeList() const { return &m_AttributeList; }
	
public:

#ifdef CLIENT_DLL
	void						SetWeaponSkinBase( ITexture* pBaseTex );
	void						SetWeaponSkinBaseCompositor( ITextureCompositor * pTexCompositor );
	inline void					SetWeaponSkinGeneration( RTime32 nGeneration ) { m_nWeaponSkinGeneration = nGeneration; }
	inline void					SetWeaponSkinGenerationTeam( int iTeam ) { m_iLastGeneratedTeamSkin = iTeam; }
	inline void					SetWeaponSkinBaseCreateFlags( uint32 flags ) { m_unWeaponSkinBaseCreateFlags = flags; }
	void						CancelWeaponSkinComposite( );
	inline void					SetWeaponSkinUseHighRes( bool bUseHighRes ) { m_bWeaponSkinUseHighRes = bUseHighRes; }
	inline void					SetWeaponSkinUseLowRes( bool bUseLowRes ) { m_bWeaponSkinUseLowRes = bUseLowRes; }

	inline ITexture				*GetWeaponSkinBase() const { return m_pWeaponSkinBase; }
	inline ITextureCompositor	*GetWeaponSkinBaseCompositor() const { return m_pWeaponSkinBaseCompositor; }
	inline uint32				GetWeaponSkinBaseCreateFlags() const { return m_unWeaponSkinBaseCreateFlags; }

	inline RTime32				GetWeaponSkinGeneration() const { return m_nWeaponSkinGeneration; }
	inline int					GetWeaponSkinGenerationTeam() const { return m_iLastGeneratedTeamSkin; }

	inline bool					ShouldWeaponSkinUseHighRes() const { return m_bWeaponSkinUseHighRes; }
	inline bool					ShouldWeaponSkinUseLowRes() const { return m_bWeaponSkinUseLowRes; }
#endif // CLIENT_DLL

	inline int					GetTeamNumber() const { return m_iTeamNumber; }
	inline void					SetTeamNumber( int iTeamNumber ) { m_iTeamNumber = iTeamNumber; }

protected:
	// Index of the item definition in the item script file.
	CNetworkVar( item_definition_index_t,	m_iItemDefinitionIndex );	

	// The quality of this item.
	CNetworkVar( int,		m_iEntityQuality );

	// The level of this item.
	CNetworkVar( uint32,	m_iEntityLevel );

	// The global index of this item, worldwide.
	itemid_t			m_iItemID;
	CNetworkVar( uint32,	m_iItemIDHigh );
	CNetworkVar( uint32,	m_iItemIDLow );

	// Account ID of the person who has this in their inventory
	CNetworkVar( uint32,	m_iAccountID );

	// Position inside the player's inventory
	CNetworkVar( uint32,	m_iInventoryPosition );

	// This is an alternate source of data, if this item models something that isn't in the SO cache.
	CEconItemHandle			m_pNonSOEconItem;

#if defined( CLIENT_DLL )
	// exist on the client only
	bool					m_bIsTradeItem;
	int						m_iEntityQuantity;
	uint8					m_unClientFlags;
	
	// clients have the ability to force a style on an item view -- this is used for store previews,
	// character panels, etc.
	style_index_t			m_unOverrideStyle;
	// clients can also force an origin on an item view -- this is used for crafting item previews
	eEconItemOrigin			m_unOverrideOrigin;
#endif

	bool	m_bColorInit;
	bool	m_bPaintOverrideInit;
	bool	m_bHasPaintOverride;
	float	m_flOverrideIndex;
	uint32	m_unRGB;
	uint32	m_unAltRGB;

#ifdef CLIENT_DLL
	ITexture* m_pWeaponSkinBase;
	ITextureCompositor* m_pWeaponSkinBaseCompositor;
	RTime32 m_nWeaponSkinGeneration;
	uint32	m_unWeaponSkinBaseCreateFlags;
	int		m_iLastGeneratedTeamSkin;
	bool	m_bWeaponSkinUseHighRes;
	bool	m_bWeaponSkinUseLowRes;
#endif // CLIENT_DLL

	CNetworkVar( int,		m_iTeamNumber );

	CNetworkVar( bool,		m_bInitialized );

#ifdef CLIENT_DLL		// we avoid using "BUILD_ITEM_NAME_AND_DESC" to prevent everything depending on the CEconItemDescription
public:
	// Return the single-line name of this item.
	const wchar_t			*GetItemName( void ) const;

	// Return the full structure with all of our description lines.
	const class CEconItemDescription *GetDescription( bool bIsToolTip = false ) const { m_bIsToolTip = bIsToolTip; EnsureDescriptionIsBuilt(); return m_pDescription; }

private:
	mutable class CEconItemDescription	*m_pDescription;
	mutable char *m_pszGrayedOutReason;
	mutable bool m_bIsToolTip = false;

	// IClientRenderable
	virtual const Vector&	GetRenderOrigin( void ) { return vec3_origin; }
	virtual const QAngle&	GetRenderAngles( void ) { return vec3_angle; }
	virtual bool			ShouldDraw( void ) { return false; }
	virtual bool			IsTransparent( void ) { return false;}
	virtual const matrix3x4_t &RenderableToWorldTransform() { static matrix3x4_t mat; SetIdentityMatrix( mat ); return mat; }
	virtual void			GetRenderBounds( Vector& mins, Vector& maxs );
#endif

private:
	CNetworkVarEmbedded( CAttributeList,	m_AttributeList );
	CNetworkVarEmbedded( CAttributeList,	m_NetworkedDynamicAttributesForDemos );

	// Some custom gamemodes are using server plugins to modify weapon attributes.
	// This variable allows them to completely set their own attributes on a weapon
	// and have the client and server ignore the static attributes.
	CNetworkVar( bool,		m_bOnlyIterateItemViewAttributes );
};

#ifdef CLIENT_DLL
bool DoesItemPassSearchFilter( const class IEconItemDescription *pDescription, const wchar_t* wszFilter );
CBasePlayer *GetPlayerByAccountID( uint32 unAccountID );
#endif // CLIENT_DLL

#endif // ECON_ITEM_CONSTANTS_H
