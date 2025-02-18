//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "attribute_manager.h"
#include "gamestringpool.h"
#include "saverestore.h"
#include "saverestore_utlvector.h"
#include "fmtstr.h"
#include "KeyValues.h"
#include "econ_item_system.h"

#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	#include "tf_gamerules.h"								// attribute cache flushing; can be generalized if/when Dota needs similar functionality
#endif // defined( TF_DLL ) || defined( TF_CLIENT_DLL )

#define PROVIDER_PARITY_BITS		6
#define PROVIDER_PARITY_MASK		((1<<PROVIDER_PARITY_BITS)-1)

//==================================================================================================================
// ATTRIBUTE MANAGER SAVE/LOAD & NETWORKING
//===================================================================================================================
BEGIN_DATADESC_NO_BASE( CAttributeManager )
	DEFINE_UTLVECTOR( m_Providers,				FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_Receivers,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_iReapplyProvisionParity,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hOuter,						FIELD_EHANDLE ),
	// DEFINE_FIELD( m_bPreventLoopback,		FIELD_BOOLEAN ),		// Don't need to save
	DEFINE_FIELD( m_ProviderType,				FIELD_INTEGER ),
END_DATADESC()

BEGIN_DATADESC( CAttributeContainer )
	DEFINE_EMBEDDED( m_Item ),
END_DATADESC()

#ifndef DOTA_DLL
BEGIN_DATADESC( CAttributeContainerPlayer )
END_DATADESC()
#endif

#ifndef CLIENT_DLL
EXTERN_SEND_TABLE( DT_ScriptCreatedItem );
#else
EXTERN_RECV_TABLE( DT_ScriptCreatedItem );
#endif

BEGIN_NETWORK_TABLE_NOBASE( CAttributeManager, DT_AttributeManager )
#ifndef CLIENT_DLL
	SendPropEHandle( SENDINFO(m_hOuter) ),
	SendPropInt( SENDINFO(m_ProviderType), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iReapplyProvisionParity), PROVIDER_PARITY_BITS, SPROP_UNSIGNED ),
#else
	RecvPropEHandle( RECVINFO(m_hOuter) ),
	RecvPropInt( RECVINFO(m_ProviderType) ),
	RecvPropInt( RECVINFO(m_iReapplyProvisionParity) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainer, DT_AttributeContainer )
#ifndef CLIENT_DLL
	SendPropEHandle( SENDINFO(m_hOuter) ),
	SendPropInt( SENDINFO(m_ProviderType), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iReapplyProvisionParity), PROVIDER_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropDataTable(SENDINFO_DT(m_Item), &REFERENCE_SEND_TABLE(DT_ScriptCreatedItem)),
#else
	RecvPropEHandle( RECVINFO(m_hOuter) ),
	RecvPropInt( RECVINFO(m_ProviderType) ),
	RecvPropInt( RECVINFO(m_iReapplyProvisionParity) ),
	RecvPropDataTable(RECVINFO_DT(m_Item), 0, &REFERENCE_RECV_TABLE(DT_ScriptCreatedItem)),
#endif
END_NETWORK_TABLE()

#ifndef DOTA_DLL
BEGIN_NETWORK_TABLE_NOBASE( CAttributeContainerPlayer, DT_AttributeContainerPlayer )
#ifndef CLIENT_DLL
	SendPropEHandle( SENDINFO(m_hOuter) ),
	SendPropInt( SENDINFO(m_ProviderType), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iReapplyProvisionParity), PROVIDER_PARITY_BITS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO(m_hPlayer) ),
#else
	RecvPropEHandle( RECVINFO(m_hOuter) ),
	RecvPropInt( RECVINFO(m_ProviderType) ),
	RecvPropInt( RECVINFO(m_iReapplyProvisionParity) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
#endif
END_NETWORK_TABLE()
#endif

template< class T > T AttributeConvertFromFloat( float flValue )
{
	return static_cast<T>( flValue );
}

template<> float AttributeConvertFromFloat<float>( float flValue )
{
	return flValue;
}

template<> int AttributeConvertFromFloat<int>( float flValue )
{
	return RoundFloatToInt( flValue );
}

//-----------------------------------------------------------------------------
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *CAttributeManager::operator new( size_t stAllocateBlock )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	void *pMem = malloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;
};

void *CAttributeManager::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );				
	void *pMem = malloc( stAllocateBlock );
	memset( pMem, 0, stAllocateBlock );
	return pMem;
}

CAttributeManager::CAttributeManager()
{
	m_nCalls = 0;
	m_nCurrentTick = 0;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::OnPreDataChanged( DataUpdateType_t updateType )
{
	m_iOldReapplyProvisionParity = m_iReapplyProvisionParity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::OnDataChanged( DataUpdateType_t updateType )
{
	if ( m_iReapplyProvisionParity != m_iOldReapplyProvisionParity )
	{
		// We've changed who we're providing to in some way. Reapply it.
		IHasAttributes *pAttribInterface = GetAttribInterface( GetOuter() );
		if ( pAttribInterface )
		{
			pAttribInterface->ReapplyProvision();
		}

		ClearCache();

		m_iOldReapplyProvisionParity = m_iReapplyProvisionParity.Get();
	}
}
#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Call this inside your entity's Spawn()
//-----------------------------------------------------------------------------
void CAttributeManager::InitializeAttributes( CBaseEntity *pEntity )
{
	Assert( GetAttribInterface( pEntity ) );
	m_hOuter = pEntity;
	m_bPreventLoopback = false;
}

//=====================================================================================================
// ATTRIBUTE PROVIDERS
//=====================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::ProvideTo( CBaseEntity *pProvider )
{
	IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pProvider );
	if ( pOwnerAttribInterface )
	{
		pOwnerAttribInterface->GetAttributeManager()->AddProvider( m_hOuter.Get() );

#ifndef CLIENT_DLL
		m_iReapplyProvisionParity = (m_iReapplyProvisionParity + 1) & PROVIDER_PARITY_MASK;
		NetworkStateChanged();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::StopProvidingTo( CBaseEntity *pProvider )
{
	IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pProvider );
	if ( pOwnerAttribInterface )
	{
		pOwnerAttribInterface->GetAttributeManager()->RemoveProvider( m_hOuter.Get() );
#ifndef CLIENT_DLL
		m_iReapplyProvisionParity = (m_iReapplyProvisionParity + 1) & PROVIDER_PARITY_MASK;
		NetworkStateChanged();
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::AddProvider( CBaseEntity *pProvider )
{
	// Make sure he's not already in our list, and prevent circular provision
	Assert( !IsBeingProvidedToBy(pProvider) );
	Assert( !IsProvidingTo(pProvider) );

	// Ensure he's allowed to provide
	IHasAttributes *pProviderAttrInterface = GetAttribInterface( pProvider );
	Assert( pProviderAttrInterface );

	m_Providers.AddToTail( pProvider );
	pProviderAttrInterface->GetAttributeManager()->m_Receivers.AddToTail( GetOuter() );

	ClearCache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::RemoveProvider( CBaseEntity *pProvider )
{
	Assert( pProvider );

	IHasAttributes *pProviderAttrInterface = GetAttribInterface( pProvider );
	Assert( pProviderAttrInterface );

	if ( !IsBeingProvidedToBy( pProvider ) )
		return;
	
	Assert( pProviderAttrInterface->GetAttributeManager()->IsProvidingTo( GetOuter() ) );
	Assert( pProviderAttrInterface->GetAttributeManager()->m_Receivers.Find( GetOuter() ) != pProviderAttrInterface->GetAttributeManager()->m_Receivers.InvalidIndex() );

	m_Providers.FindAndFastRemove( pProvider );
	pProviderAttrInterface->GetAttributeManager()->m_Receivers.FindAndFastRemove( GetOuter() );

	ClearCache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAttributeManager::ClearCache( void )
{
	if ( m_bPreventLoopback )
		return;

	m_CachedResults.Purge();

	m_bPreventLoopback = true;

	// Tell all providers relying on me that they need to wipe their cache too
	FOR_EACH_VEC( m_Receivers, i )
	{
		IHasAttributes *pAttribInterface = GetAttribInterface( m_Receivers[i].Get() );
		if ( pAttribInterface )
		{
			pAttribInterface->GetAttributeManager()->ClearCache();
		}
	}

	// Tell our owner that he needs to clear his too, in case he has attributes affecting him
	IHasAttributes *pMyAttribInterface = GetAttribInterface( m_hOuter.Get().Get() );
	if ( pMyAttribInterface )
	{
		pMyAttribInterface->GetAttributeManager()->ClearCache();
	}

	m_bPreventLoopback = false;

#ifndef CLIENT_DLL
	// Force out client to clear their cache as well
	m_iReapplyProvisionParity = (m_iReapplyProvisionParity + 1) & PROVIDER_PARITY_MASK;
	NetworkStateChanged();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CAttributeManager::GetGlobalCacheVersion() const
{
#if defined( TF_DLL ) || defined( TF_CLIENT_DLL )
	return TFGameRules() ? TFGameRules()->GetGlobalAttributeCacheVersion() : 0;
#else
	return 0;
#endif 
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this entity is providing attributes to the specified entity
//-----------------------------------------------------------------------------
bool CAttributeManager::IsProvidingTo( CBaseEntity *pEntity ) const
{
	IHasAttributes *pAttribInterface = GetAttribInterface( pEntity );
	if ( pAttribInterface )
	{
		if ( pAttribInterface->GetAttributeManager()->IsBeingProvidedToBy( GetOuter() ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this entity is being provided attributes by the specified entity
//-----------------------------------------------------------------------------
bool CAttributeManager::IsBeingProvidedToBy( CBaseEntity *pEntity ) const
{
	return ( m_Providers.Find( pEntity ) != m_Providers.InvalidIndex() );
}

//=====================================================================================================
// ATTRIBUTE HOOKS
//=====================================================================================================

//-----------------------------------------------------------------------------
// Purpose: Wrapper that checks to see if we've already got the result in our cache
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloatWrapper( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList )
{
	VPROF_BUDGET( "CAttributeManager::ApplyAttributeFloatWrapper", VPROF_BUDGETGROUP_ATTRIBUTES );

#ifdef DEBUG
	AssertMsg1( m_nCalls != 5000, "%d calls for attributes in a single tick.  This is slow and bad.", m_nCalls );

	if( m_nCurrentTick != gpGlobals->tickcount )
	{	
		m_nCalls = 0;
		m_nCurrentTick = gpGlobals->tickcount;
	}

	++m_nCalls;
#endif

	// Have we requested a global attribute cache flush?
	const int iGlobalCacheVersion = GetGlobalCacheVersion();
	if ( m_iCacheVersion != iGlobalCacheVersion )
	{
		ClearCache();
		m_iCacheVersion = iGlobalCacheVersion;
	}

	// We can't cache off item references so if we asked for them we need to execute the whole slow path.
	if ( !pItemList )
	{
		int iCount = m_CachedResults.Count();
		for ( int i = iCount-1; i >= 0; i-- )
		{
			if ( m_CachedResults[i].iAttribHook == iszAttribHook )
			{
				if ( m_CachedResults[i].in.fl == flValue )
					return m_CachedResults[i].out.fl;

				// We've got a cached result for a different flIn value. Remove the cached result to
				// prevent stacking up entries for different requests (i.e. crit chance)
				m_CachedResults.Remove(i);
				break;
			}
		}
	}

	// Wasn't in cache, or we need item references. Do the work.
	float flResult = ApplyAttributeFloat( flValue, pInitiator, iszAttribHook, pItemList );

	// Add it to our cache if we didn't ask for item references. We could add the result value here
	// even if we did but we'd need to walk the cache to search for an old entry to overwrite first.
	if ( !pItemList )
	{
		int iIndex = m_CachedResults.AddToTail();
		m_CachedResults[iIndex].in.fl = flValue;
		m_CachedResults[iIndex].out.fl = flResult;
		m_CachedResults[iIndex].iAttribHook = iszAttribHook;
	}

	return flResult;
}

//-----------------------------------------------------------------------------
// Purpose: Wrapper that checks to see if we've already got the result in our cache
//-----------------------------------------------------------------------------
string_t CAttributeManager::ApplyAttributeStringWrapper( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList /*= NULL*/ )
{
	// Have we requested a global attribute cache flush?
	const int iGlobalCacheVersion = GetGlobalCacheVersion();
	if ( m_iCacheVersion != iGlobalCacheVersion )
	{
		ClearCache();
		m_iCacheVersion = iGlobalCacheVersion;
	}

	// We can't cache off item references so if we asked for them we need to execute the whole slow path.
	if ( !pItemList )
	{
		int iCount = m_CachedResults.Count();
		for ( int i = iCount-1; i >= 0; i-- )
		{
			if ( m_CachedResults[i].iAttribHook == iszAttribHook )
			{
				if ( m_CachedResults[i].in.isz == iszValue )
				{
					return m_CachedResults[i].out.isz;
				}

				// We've got a cached result for a different flIn value. Remove the cached result to
				// prevent stacking up entries for different requests (i.e. crit chance)
				m_CachedResults.Remove(i);
				break;
			}
		}
	}

	// Wasn't in cache, or we need item references. Do the work.
	string_t iszOut = ApplyAttributeString( iszValue, pInitiator, iszAttribHook, pItemList );

	// Add it to our cache if we didn't ask for item references. We could add the result value here
	// even if we did but we'd need to walk the cache to search for an old entry to overwrite first.
	if ( !pItemList )
	{
		int iIndex = m_CachedResults.AddToTail();
		m_CachedResults[iIndex].in.isz = iszValue;
		m_CachedResults[iIndex].out.isz = iszOut;
		m_CachedResults[iIndex].iAttribHook = iszAttribHook;
	}

	return iszOut;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CAttributeManager::ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList )
{
	VPROF_BUDGET( "CAttributeManager::ApplyAttributeFloat", VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bPreventLoopback || !GetOuter() )
		return flValue;

	// We need to prevent loopback between two items both providing to the same entity.
	m_bPreventLoopback = true;

	IHasAttributes *pInitiatorAttribInterface = GetAttribInterface( pInitiator );

	// See if we have any providers. If we do, tell them to apply.
	FOR_EACH_VEC( m_Providers, iHook )
	{
		CBaseEntity *pProvider = m_Providers[iHook].Get();
		
		if ( !pProvider )
			continue;
			
		if ( pProvider == pInitiator )
			continue;
			
		IHasAttributes *pAttribInterface = GetAttribInterface( pProvider );
		Assert( pAttribInterface );

		// Don't allow weapons to provide to other weapons being carried by the same person
		if ( pInitiatorAttribInterface &&
		     pAttribInterface->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
			 pInitiatorAttribInterface->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
		{
			continue;
		}

		flValue = pAttribInterface->GetAttributeManager()->ApplyAttributeFloat( flValue, pInitiator, iszAttribHook, pItemList );
	}

	// Then see if our owner has any attributes he wants to apply as well.
	// i.e. An aura is providing attributes to this weapon's carrier.
	IHasAttributes *pMyAttribInterface = GetAttribInterface( m_hOuter.Get().Get() );
	Assert( pMyAttribInterface );

	if ( pMyAttribInterface && pMyAttribInterface->GetAttributeOwner() )
	{
		IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pMyAttribInterface->GetAttributeOwner() );
		if ( pOwnerAttribInterface )
		{
			flValue = pOwnerAttribInterface->GetAttributeManager()->ApplyAttributeFloat( flValue, pInitiator, iszAttribHook, pItemList );
		}
	}

	m_bPreventLoopback = false;

	return flValue;
}

string_t CAttributeManager::ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook /*= NULL_STRING*/, CUtlVector<CBaseEntity*> *pItemList /*= NULL*/ )
{
	VPROF_BUDGET( "CAttributeManager::ApplyAttributeString", VPROF_BUDGETGROUP_ATTRIBUTES );

	if ( m_bPreventLoopback || !GetOuter() )
		return iszValue;

	// We need to prevent loopback between two items both providing to the same entity.
	m_bPreventLoopback = true;

	IHasAttributes *pInitiatorAttribInterface = GetAttribInterface( pInitiator );

	// See if we have any providers. If we do, tell them to apply.
	FOR_EACH_VEC( m_Providers, iHook )
	{
		CBaseEntity *pProvider = m_Providers[iHook].Get();

		if ( !pProvider )
			continue;

		if ( pProvider == pInitiator )
			continue;

		IHasAttributes *pAttribInterface = GetAttribInterface( pProvider );
		Assert( pAttribInterface );

		// Don't allow weapons to provide to other weapons being carried by the same person
		if ( pInitiatorAttribInterface &&
			pAttribInterface->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON &&
			pInitiatorAttribInterface->GetAttributeManager()->GetProviderType() == PROVIDER_WEAPON )
		{
			continue;
		}

		iszValue = pAttribInterface->GetAttributeManager()->ApplyAttributeString( iszValue, pInitiator, iszAttribHook, pItemList );
	}

	// Then see if our owner has any attributes he wants to apply as well.
	// i.e. An aura is providing attributes to this weapon's carrier.
	IHasAttributes *pMyAttribInterface = GetAttribInterface( m_hOuter.Get().Get() );
	Assert( pMyAttribInterface );

	if ( pMyAttribInterface->GetAttributeOwner() )
	{
		IHasAttributes *pOwnerAttribInterface = GetAttribInterface( pMyAttribInterface->GetAttributeOwner() );
		if ( pOwnerAttribInterface )
		{
			iszValue = pOwnerAttribInterface->GetAttributeManager()->ApplyAttributeString( iszValue, pInitiator, iszAttribHook, pItemList );
		}
	}

	m_bPreventLoopback = false;

	return iszValue;
}

//=====================================================================================================
// ATTRIBUTE CONTAINER
//=====================================================================================================

//-----------------------------------------------------------------------------
// Purpose: Call this inside your entity's Spawn()
//-----------------------------------------------------------------------------
void CAttributeContainer::InitializeAttributes( CBaseEntity *pEntity )
{
	BaseClass::InitializeAttributes( pEntity );

#ifndef CLIENT_DLL
	/*
	if ( !m_Item.IsValid() )
	{
		Warning("Item '%s' not setup correctly. Attempting to create attributes on an unitialized item.\n", m_hOuter.Get()->GetDebugName() );
	}
	*/
#endif

	m_Item.GetAttributeList()->SetManager( this );

	OnAttributeValuesChanged();
}

static void ApplyAttribute( const CEconItemAttributeDefinition *pAttributeDef, float& flValue, const float flValueModifier )
{
	Assert( pAttributeDef );
	Assert( pAttributeDef->GetAttributeType() );
	AssertMsg1( pAttributeDef->GetAttributeType()->BSupportsGameplayModificationAndNetworking(), "Attempt to hook the value of attribute '%s' which doesn't support hooking! Pull the value of the attribute directly using FindAttribute()!", pAttributeDef->GetDefinitionName() );

	const int iAttrDescFormat = pAttributeDef->GetDescriptionFormat();

	switch ( iAttrDescFormat )
	{
	case ATTDESCFORM_VALUE_IS_PERCENTAGE:
	case ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE:
		{
			flValue *= flValueModifier;
		}
		break;

	case ATTDESCFORM_VALUE_IS_ADDITIVE:
	case ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE:
	case ATTDESCFORM_VALUE_IS_PARTICLE_INDEX:
		{
			flValue += flValueModifier;
		}
		break;

	case ATTDESCFORM_VALUE_IS_KILLSTREAK_IDLEEFFECT_INDEX:
	case ATTDESCFORM_VALUE_IS_KILLSTREAKEFFECT_INDEX:
	case ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE:
		{
			flValue = flValueModifier;
		}
		break;

	case ATTDESCFORM_VALUE_IS_OR:
		{
			int iTmp = flValue;
			iTmp |= (int)flValueModifier;
			flValue = iTmp;
		}
		break;

	case ATTDESCFORM_VALUE_IS_DATE:
		Assert( !"Attempt to apply date attribute in ApplyAttribute()." ); // No-one should be hooking date descriptions
		break;

	default:
		// Unknown value format. 
		AssertMsg1( false, "Unknown attribute value type %i in ApplyAttribute().", iAttrDescFormat );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Given two attributes, return a collated value.
//-----------------------------------------------------------------------------
float CollateAttributeValues( const CEconItemAttributeDefinition *pAttrDef1, const float flAttribValue1, const CEconItemAttributeDefinition *pAttrDef2, const float flAttribValue2 )
{
	Assert( pAttrDef1 );
	Assert( pAttrDef2 );
	AssertMsg2( !Q_stricmp( pAttrDef1->GetAttributeClass(), pAttrDef2->GetAttributeClass() ), "We can only collate attributes of matching definitions: mismatch between '%s' / '%s'!", pAttrDef1->GetAttributeClass(), pAttrDef2->GetAttributeClass() );
	AssertMsg2( pAttrDef1->GetDescriptionFormat() == pAttrDef2->GetDescriptionFormat(), "We can only collate attributes of matching description format: mismatch between '%u' / '%u'!", pAttrDef1->GetDescriptionFormat(), pAttrDef2->GetDescriptionFormat() );

	const int iAttrDescFormat = pAttrDef1->GetDescriptionFormat();

	float flValue = 0;
	switch ( iAttrDescFormat )
	{
	case ATTDESCFORM_VALUE_IS_PERCENTAGE:
	case ATTDESCFORM_VALUE_IS_INVERTED_PERCENTAGE:
		{
			flValue = 1.0;
		}
		break;

	case ATTDESCFORM_VALUE_IS_ADDITIVE:
	case ATTDESCFORM_VALUE_IS_ADDITIVE_PERCENTAGE:
	case ATTDESCFORM_VALUE_IS_FROM_LOOKUP_TABLE:
	case ATTDESCFORM_VALUE_IS_OR:
		{
			flValue = 0;
		}
		break;

	case ATTDESCFORM_VALUE_IS_DATE:
		Assert( !"Attempt to apply date attribute in CollateAttributeValues()." ); // No-one should be hooking date descriptions
		break;

	default:
		// Unknown value format. 
		AssertMsg1( false, "Unknown attribute value type %i in ApplyAttribute().", iAttrDescFormat );
		break;
	}

	ApplyAttribute( pAttrDef1, flValue, flAttribValue1 );
	ApplyAttribute( pAttrDef2, flValue, flAttribValue2 );

	return flValue;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CEconItemAttributeIterator_ApplyAttributeFloat : public CEconItemSpecificAttributeIterator
{
public:
	CEconItemAttributeIterator_ApplyAttributeFloat( CBaseEntity *pOuter, float flInitialValue, string_t iszAttribHook, CUtlVector<CBaseEntity *> *pItemList )
		: m_pOuter( pOuter )
		, m_flValue( flInitialValue )
		, m_iszAttribHook( iszAttribHook )
		, m_pItemList( pItemList )
	{
		Assert( pOuter );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
	{
		COMPILE_TIME_ASSERT( sizeof( value ) == sizeof( float ) );

		Assert( pAttrDef );

		if ( pAttrDef->GetCachedClass() != m_iszAttribHook )
			return true;

		if ( m_pItemList && !m_pItemList->HasElement( m_pOuter ) )
		{
			m_pItemList->AddToTail( m_pOuter );
		}

		ApplyAttribute( pAttrDef, m_flValue, *reinterpret_cast<float *>( &value ) );

		// We assume that each attribute can only be in the attribute list for a single item once, but we're
		// iterating over attribute *classes* here, not unique attribute types, so we carry on looking.
		return true;
	}

	float GetResultValue() const
	{
		return m_flValue;
	}

private:
	CBaseEntity *m_pOuter;
	float m_flValue;
	string_t m_iszAttribHook;
	CUtlVector<CBaseEntity *> *m_pItemList;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CAttributeContainer::ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList )
{
	if ( m_bPreventLoopback || !GetOuter() )
		return flValue;

	// We need to prevent loopback between two items both providing to the same entity.
	m_bPreventLoopback = true;

	// ...
	CEconItemAttributeIterator_ApplyAttributeFloat it( GetOuter(), flValue, iszAttribHook, pItemList );
	m_Item.IterateAttributes( &it );

	m_bPreventLoopback = false;

	return BaseClass::ApplyAttributeFloat( it.GetResultValue(), pInitiator, iszAttribHook, pItemList );
}

#ifndef DOTA_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CAttributeContainerPlayer::ApplyAttributeFloat( float flValue, CBaseEntity *pInitiator, string_t iszAttribHook, CUtlVector<CBaseEntity*> *pItemList )
{
	if ( m_bPreventLoopback || !GetOuter() )
		return flValue;

	m_bPreventLoopback = true;

	CEconItemAttributeIterator_ApplyAttributeFloat it( GetOuter(), flValue, iszAttribHook, pItemList );

	CBasePlayer *pPlayer = GetPlayer();
	if ( pPlayer )
	{
		pPlayer->m_AttributeList.IterateAttributes( &it );
	}

	m_bPreventLoopback = false;

	return BaseClass::ApplyAttributeFloat( it.GetResultValue(), pInitiator, iszAttribHook, pItemList );
}


#endif


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CEconItemAttributeIterator_ApplyAttributeString : public CEconItemSpecificAttributeIterator
{
public:
	CEconItemAttributeIterator_ApplyAttributeString( CBaseEntity *pOuter, string_t iszInitialValue, string_t iszAttribHook, CUtlVector<CBaseEntity *> *pItemList )
		: m_pOuter( pOuter )
		, m_iszValue( iszInitialValue )
		, m_iszAttribHook( iszAttribHook )
		, m_pItemList( pItemList )
		, m_bFoundString( false )
	{
		Assert( pOuter );
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, attrib_value_t value )
	{
		COMPILE_TIME_ASSERT( sizeof( value ) == sizeof( float ) );

		// Do we want to process attribute of this type?
		Assert( pAttrDef );
		Assert( pAttrDef->GetCachedClass() != m_iszAttribHook );
		//AssertMsg( 0, "OnIterateAttributeValue of type CAttribute_String, we shouldn't get here." );

		return true;
	}

	virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_String& value )
	{
		Assert( pAttrDef );
		
		if ( pAttrDef->GetCachedClass() != m_iszAttribHook )
			return true;

		if ( FoundString() )
			return true;

		m_iszValue = AllocPooledString( value.value().c_str() );

		m_bFoundString = true;

		return true;
	}

	string_t GetResultValue()
	{
		return m_iszValue;
	}

private:
	bool FoundString()
	{
		// Implement something for the case where there's more than one of the same attribute
		AssertMsg( !m_bFoundString, "Already found a string attribute with %s class, return the first attribute found.", STRING( m_iszAttribHook ) );

		return m_bFoundString;
	}

	CBaseEntity *m_pOuter;
	string_t m_iszValue;
	string_t m_iszAttribHook;
	CUtlVector<CBaseEntity *> *m_pItemList;
	bool m_bFoundString;
};


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
string_t CAttributeContainer::ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook /*= NULL_STRING*/, CUtlVector<CBaseEntity*> *pItemList /*= NULL*/ )
{
	if ( m_bPreventLoopback || !GetOuter() )
		return iszValue;

	// We need to prevent loopback between two items both providing to the same entity.
	m_bPreventLoopback = true;

	// ...
	CEconItemAttributeIterator_ApplyAttributeString it( GetOuter(), iszValue, iszAttribHook, pItemList );
	m_Item.IterateAttributes( &it );

	m_bPreventLoopback = false;

	return BaseClass::ApplyAttributeString( it.GetResultValue(), pInitiator, iszAttribHook, pItemList );
}


string_t CAttributeContainerPlayer::ApplyAttributeString( string_t iszValue, CBaseEntity *pInitiator, string_t iszAttribHook /*= NULL_STRING*/, CUtlVector<CBaseEntity*> *pItemList /*= NULL*/ )
{
	if ( m_bPreventLoopback || !GetOuter() )
		return iszValue;

	m_bPreventLoopback = true;

	CEconItemAttributeIterator_ApplyAttributeString it( GetOuter(), iszValue, iszAttribHook, pItemList );

	CBasePlayer *pPlayer = GetPlayer();
	if ( pPlayer )
	{
		pPlayer->m_AttributeList.IterateAttributes( &it );
	}

	m_bPreventLoopback = false;

	return BaseClass::ApplyAttributeString( it.GetResultValue(), pInitiator, iszAttribHook, pItemList );
}
