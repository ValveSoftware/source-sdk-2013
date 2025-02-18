//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Attributable entities contain one of these, which handles game specific handling:
//				- Save / Restore
//				- Networking
//				- Attribute providers
//				- Application of attribute effects
//
//=============================================================================

#ifndef ATTRIBUTE_MANAGER_H
#define ATTRIBUTE_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "econ_item_view.h"
#include "ihasattributes.h"
#include "tf_gcmessages.h"

// Provider types
enum attributeprovidertypes_t
{
	PROVIDER_GENERIC,
	PROVIDER_WEAPON,
};

float CollateAttributeValues( const CEconItemAttributeDefinition *pAttrDef1, const float flAttribValue1, const CEconItemAttributeDefinition *pAttrDef2, const float flAttribValue2 );

// Retrieve the IHasAttributes pointer from a Base Entity. This function checks for NULL entities
//  and asserts the return value is == to dynamic_cast< IHasAttributes * >( pEntity ).
inline IHasAttributes *GetAttribInterface( CBaseEntity *pEntity )
{
	IHasAttributes *pAttribInterface = pEntity ? pEntity->GetHasAttributesInterfacePtr() : NULL;
	// If this assert hits it most likely means that m_pAttribInterface has not been set
	//  in the leaf class constructor for this object. See CTFPlayer::CTFPlayer() for an
	//  example.
	Assert( pAttribInterface == dynamic_cast< IHasAttributes *>( pEntity ) );
	return pAttribInterface;
}

//-----------------------------------------------------------------------------
// Macros for hooking the application of attributes
#define CALL_ATTRIB_HOOK( vartype, retval, hookName, who, itemlist ) \
	retval = CAttributeManager::AttribHookValue<vartype>( retval, #hookName, static_cast<const CBaseEntity*>( who ), itemlist, true );

#define CALL_ATTRIB_HOOK_INT( retval, hookName )	CALL_ATTRIB_HOOK( int, retval, hookName, this, NULL )
#define CALL_ATTRIB_HOOK_FLOAT( retval, hookName )	CALL_ATTRIB_HOOK( float, retval, hookName, this, NULL )
#define CALL_ATTRIB_HOOK_STRING( retval, hookName )	CALL_ATTRIB_HOOK( CAttribute_String, retval, hookName, this, NULL )
#define CALL_ATTRIB_HOOK_INT_ON_OTHER( other, retval, hookName )	CALL_ATTRIB_HOOK( int, retval, hookName, other, NULL )
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( other, retval, hookName )	CALL_ATTRIB_HOOK( float, retval, hookName, other, NULL )
#define CALL_ATTRIB_HOOK_STRING_ON_OTHER( other, retval, hookName )	CALL_ATTRIB_HOOK( CAttribute_String, retval, hookName, other, NULL )
#define CALL_ATTRIB_HOOK_INT_ON_OTHER_WITH_ITEMS( other, retval, items_array, hookName )	CALL_ATTRIB_HOOK( int, retval, hookName, other, items_array )
#define CALL_ATTRIB_HOOK_FLOAT_ON_OTHER_WITH_ITEMS( other, retval, items_array, hookName )	CALL_ATTRIB_HOOK( float, retval, hookName, other, items_array )
#define CALL_ATTRIB_HOOK_STRING_ON_OTHER_WITH_ITEMS( other, retval, items_array, hookName )	CALL_ATTRIB_HOOK( CAttribute_String, retval, hookName, other, items_array )

template< class T > T AttributeConvertFromFloat( float flValue );
template<> float AttributeConvertFromFloat<float>( float flValue );
template<> int AttributeConvertFromFloat<int>( float flValue );

//-----------------------------------------------------------------------------
// Purpose: Base Attribute manager.
//			This class knows how to apply attribute effects that have been
//			provided to its owner by other entities, but doesn't contain attributes itself.
//-----------------------------------------------------------------------------
class CAttributeManager
{
	DECLARE_CLASS_NOBASE( CAttributeManager );
public:
	DECLARE_DATADESC();
	DECLARE_EMBEDDED_NETWORKVAR();

	CAttributeManager();
	virtual ~CAttributeManager() {}

	// Call this inside your entity's Spawn()
	virtual void InitializeAttributes( CBaseEntity *pEntity );

	CBaseEntity *GetOuter( void ) const { return m_hOuter.Get(); }

	//--------------------------------------------------------
	// Attribute providers.
	// Other entities that are providing attributes to this entity (i.e. weapons being carried by a player)
	void ProvideTo( CBaseEntity *pProvider );
	void StopProvidingTo( CBaseEntity *pProvider );

protected:
	// Not to be called directly. Use ProvideTo() or StopProvidingTo() above.
	void AddProvider( CBaseEntity *pProvider );
	void RemoveProvider( CBaseEntity *pProvider );

public:
	// Return true if this entity is providing attributes to the specified entity
	bool IsProvidingTo( CBaseEntity *pEntity ) const;

	// Return true if this entity is being provided attributes by the specified entity
	bool IsBeingProvidedToBy( CBaseEntity *pEntity ) const;

	// Provider types are used to prevent specified providers supplying to certain initiators
	void SetProviderType( attributeprovidertypes_t tType ) { m_ProviderType = tType; }
	attributeprovidertypes_t GetProviderType( void ) const { return m_ProviderType; }

	//--------------------------------------------------------
	// Attribute hook. Use the CALL_ATTRIB_HOOK macros above.
	template <class T> static T AttribHookValue( T TValue, const char *pszAttribHook, const CBaseEntity *pEntity, CUtlVector<CBaseEntity*> *pItemList = NULL, bool bIsGlobalConstString = false )
	{
		VPROF_BUDGET( "CAttributeManager::AttribHookValue", VPROF_BUDGETGROUP_ATTRIBUTES );

		// Do we have a hook?
		if ( pszAttribHook == NULL || pszAttribHook[0] == '\0' )
			return TValue;

		// Verify that we have an entity, at least as "this"
		if ( pEntity == NULL )
			return TValue;

		IHasAttributes *pAttribInterface = GetAttribInterface( (CBaseEntity*) pEntity );
		AssertMsg( pAttribInterface, "If you hit this, you've probably got a hook incorrectly setup, because the entity it's hooking on doesn't know about attributes." );
		if ( pAttribInterface == NULL )
			return TValue;

		// Hook base attribute.
		T Scratch;
		AttribHookValueInternal( Scratch, TValue, pszAttribHook, pEntity, pAttribInterface, pItemList, bIsGlobalConstString );

		return Scratch;
	}

private:
	template <class T> static void TypedAttribHookValueInternal( T& out, T TValue, string_t iszAttribHook, const CBaseEntity *pEntity, IHasAttributes *pAttribInterface, CUtlVector<CBaseEntity*> *pItemList )
	{
		float flValue = pAttribInterface->GetAttributeManager()->ApplyAttributeFloatWrapper( static_cast<float>( TValue ), const_cast<CBaseEntity *>( pEntity ), iszAttribHook, pItemList );

		out = AttributeConvertFromFloat<T>( flValue );
	}

	static void TypedAttribHookValueInternal( CAttribute_String& out, const CAttribute_String& TValue, string_t iszAttribHook, const CBaseEntity *pEntity, IHasAttributes *pAttribInterface, CUtlVector<CBaseEntity*> *pItemList )
	{
		string_t iszIn = AllocPooledString( TValue.value().c_str() );
		string_t iszOut = pAttribInterface->GetAttributeManager()->ApplyAttributeStringWrapper( iszIn, const_cast<CBaseEntity *>( pEntity ), iszAttribHook, pItemList );
		const char* pszOut = STRING( iszOut );
		// STRING() returns different value for server and client
		// server will return "" for NULL_STRING
		// client will return NULL for NULL_STRING
		if ( pszOut )
		{
			out.set_value( pszOut );
		}
		else
		{
			out.set_value( "" );
		}
	}

	template <class T> static void AttribHookValueInternal( T& out, T TValue, const char *pszAttribHook, const CBaseEntity *pEntity, IHasAttributes *pAttribInterface, CUtlVector<CBaseEntity*> *pItemList, bool bIsGlobalConstString )
	{
		Assert( pszAttribHook );
		Assert( pszAttribHook[0] );
		Assert( pEntity );
		Assert( pAttribInterface );
		Assert( GetAttribInterface( (CBaseEntity*) pEntity ) == pAttribInterface );
		Assert( pAttribInterface->GetAttributeManager() );
		
		string_t iszAttribHook = bIsGlobalConstString ? AllocPooledString_StaticConstantStringPointer( pszAttribHook ) : AllocPooledString( pszAttribHook );
		return TypedAttribHookValueInternal( out, TValue, iszAttribHook, pEntity, pAttribInterface, pItemList );
	}
	int m_nCurrentTick;
	int m_nCalls;

public:
	virtual float	ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL );
	virtual string_t	ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL );

	//--------------------------------------------------------
	// Networking
#ifdef CLIENT_DLL
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
#endif

	//--------------------------------------------------------
	// memory handling
	void *operator new( size_t stAllocateBlock );
	void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );

protected:
	CUtlVector<EHANDLE>							m_Providers;						// entities that we receive attribute data *from*
	CUtlVector<EHANDLE>							m_Receivers;						// entities that we provide attribute data *to*
	CNetworkVarForDerived( int,					m_iReapplyProvisionParity );
	CNetworkVarForDerived( EHANDLE,				m_hOuter );
	bool										m_bPreventLoopback;
	CNetworkVarForDerived( attributeprovidertypes_t,	m_ProviderType );
	int											m_iCacheVersion;					// maps to gamerules counter for global cache flushing

public:
	virtual void OnAttributeValuesChanged()
	{
		ClearCache();
	}

private:
	void	ClearCache();
	int		GetGlobalCacheVersion() const;

	virtual float	ApplyAttributeFloatWrapper( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = NULL );
	virtual string_t ApplyAttributeStringWrapper( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList = NULL );

	// Cached attribute results
	// We cache off requests for data, and wipe the cache whenever our providers change.
	union cached_attribute_types
	{
		float fl;
		string_t isz;
	};

	struct cached_attribute_t
	{
		string_t	iAttribHook;
		cached_attribute_types		in;
		cached_attribute_types		out;
	};
	CUtlVector<cached_attribute_t>	m_CachedResults;

#ifdef CLIENT_DLL
public:
	// Data received from the server
	int							m_iOldReapplyProvisionParity;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: This is an attribute manager that also knows how to contain attributes.
//-----------------------------------------------------------------------------
class CAttributeContainer : public CAttributeManager
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CAttributeContainer, CAttributeManager );
	DECLARE_EMBEDDED_NETWORKVAR();

	virtual void InitializeAttributes( CBaseEntity *pEntity );

	//--------------------------------------------------------
	// Attribute hook. Use the CALL_ATTRIB_HOOK macros above.
	virtual float	ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL ) OVERRIDE;
	virtual string_t	ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL ) OVERRIDE;

	CEconItemView *GetItem( void ) { return &m_Item; }
	const CEconItemView *GetItem( void ) const { return &m_Item; }
	void		SetItem( const CEconItemView *pItem ) { m_Item.CopyFrom( *pItem ); }

	virtual void OnAttributeValuesChanged()
	{
		BaseClass::OnAttributeValuesChanged();

		m_Item.OnAttributeValuesChanged();
	}

private:
	CNetworkVarEmbedded( CEconItemView,	m_Item );
};

//-----------------------------------------------------------------------------
// Purpose: An attribute manager that uses a player's shared attributes.
//-----------------------------------------------------------------------------

#ifndef DOTA_DLL
class CAttributeContainerPlayer : public CAttributeManager
{
public:
	DECLARE_DATADESC();
	DECLARE_CLASS( CAttributeContainerPlayer, CAttributeManager );
	DECLARE_EMBEDDED_NETWORKVAR();

	virtual float	ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL ) OVERRIDE;
	virtual string_t	ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook = NULL_STRING, CUtlVector<CBaseEntity*> *pItemList = NULL ) OVERRIDE;

	CBasePlayer*	GetPlayer( void ) { return m_hPlayer; }
	void			SetPlayer( CBasePlayer *pPlayer ) { m_hPlayer = pPlayer; }

	virtual void OnAttributeValuesChanged()
	{
		BaseClass::OnAttributeValuesChanged();

		m_hPlayer->NetworkStateChanged();
	}

private:
	CNetworkHandle( CBasePlayer, m_hPlayer );
};
#endif

class CEconGetAttributeIterator : public CEconItemSpecificAttributeIterator
{
public:
	CEconGetAttributeIterator( attrib_definition_index_t nDefIndex, float flDefaultValue )
		: m_nDefIndex( nDefIndex ), m_flValue( flDefaultValue ) {}

	bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value ) override
	{
		if ( pAttrDef->GetDefinitionIndex() == m_nDefIndex )
		{
			m_flValue = *reinterpret_cast<float *>( &value );
		}
		return true;
	}

	float m_flValue;
	attrib_definition_index_t m_nDefIndex;
};

#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_AttributeManager );
EXTERN_RECV_TABLE( DT_AttributeContainer );
#else
EXTERN_SEND_TABLE( DT_AttributeManager );
EXTERN_SEND_TABLE( DT_AttributeContainer );
#endif

#endif // ATTRIBUTE_MANAGER_H
