//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dt_utlvector_send.h"

#include "tier0/memdbgon.h"


extern const char *s_ElementNames[MAX_ARRAY_ELEMENTS];

// This gets associated with SendProps inside a utlvector and stores extra data needed to make it work.
class CSendPropExtra_UtlVector
{
public:
	CSendPropExtra_UtlVector() :
		m_DataTableProxyFn( NULL ),
		m_ProxyFn( NULL ),
		m_EnsureCapacityFn( NULL ),
		m_ElementStride( 0 ),
		m_Offset( 0 ),
		m_nMaxElements( 0 )	
	{
	}

	SendTableProxyFn m_DataTableProxyFn;	// If it's a datatable, then this is the proxy they specified.
	SendVarProxyFn m_ProxyFn;				// If it's a non-datatable, then this is the proxy they specified.
	EnsureCapacityFn m_EnsureCapacityFn;
	int m_ElementStride;					// Distance between each element in the array.
	int m_Offset;							// # bytes from the parent structure to its utlvector.
	int m_nMaxElements;						// For debugging...
};


void SendProxy_UtlVectorElement( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	Assert( pExtra );

	// Kind of lame overloading element stride to hold the element index,
	// but we can easily move it into its SetExtraData stuff if we need to.
	iElement = pProp->GetElementStride();

	// NOTE: this is cheesy, but it does the trick.
	CUtlVector<int> *pUtlVec = (CUtlVector<int>*)((char*)pStruct + pExtra->m_Offset);
	if ( iElement >= pUtlVec->Count() )
	{
		// Pass in zero value.
		memset( pOut, 0, sizeof( *pOut ) );
	}
	else
	{
		// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
		pExtra->m_ProxyFn( pProp, pData, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, pOut, 0, objectID );
	}
}

void* SendProxy_UtlVectorElement_DataTable( 
	const SendProp *pProp,
	const void *pStructBase, 
	const void *pData, 
	CSendProxyRecipients *pRecipients, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();

	int iElement = pProp->m_ElementStride;
	Assert( iElement < pExtra->m_nMaxElements );

	// This should have gotten called in SendProxy_LengthTable before we get here, so 
	// the capacity should be correct.
#ifdef _DEBUG
	pExtra->m_EnsureCapacityFn( (void*)pStructBase, pExtra->m_Offset, pExtra->m_nMaxElements );
#endif

	// NOTE: this is cheesy because we're assuming the type of the template class, but it does the trick.
	CUtlVector<int> *pUtlVec = (CUtlVector<int>*)((char*)pStructBase + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
	return pExtra->m_DataTableProxyFn( pProp, pData, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, pRecipients, objectID );
}

void SendProxy_UtlVectorLength( 
	const SendProp *pProp, 
	const void *pStruct, 
	const void *pData, 
	DVariant *pOut, 
	int iElement, 
	int objectID )
{
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	
	// NOTE: this is cheesy because we're assuming the type of the template class, but it does the trick.
	CUtlVector<int> *pUtlVec = (CUtlVector<int>*)((char*)pStruct + pExtra->m_Offset);
	
	// Don't let them overflow the buffer because they might expect that to get transmitted to the client.
	pOut->m_Int = pUtlVec->Count();
	if ( pOut->m_Int > pExtra->m_nMaxElements )
	{
		Assert( false );
		pOut->m_Int = pExtra->m_nMaxElements;
	}
}


void* SendProxy_LengthTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Make sure the array has space to hold all the elements.
	CSendPropExtra_UtlVector *pExtra = (CSendPropExtra_UtlVector*)pProp->GetExtraData();
	pExtra->m_EnsureCapacityFn( (void*)pStructBase, pExtra->m_Offset, pExtra->m_nMaxElements );
	return (void*)pData;
}


// Note: your pArrayProp will NOT get iElement set to anything other than 0, because this function installs its
// own proxy in front of yours. pStruct will point at the CUtlVector and pData will point at the element in the CUtlVector.It will pass you the direct pointer to the element inside the CUtlVector.
//
// You can skip the first 3 parameters in pArrayProp because they're ignored. So your array specification
// could look like this:
//   	 SendPropUtlVector( 
//	    	SENDINFO_UTLVECTOR( m_FloatArray ),
//			SendPropFloat( NULL, 0, 0, 0 [# bits], SPROP_NOSCALE [flags] ) );
//
// Note: you have to be DILIGENT about calling NetworkStateChanged whenever an element in your CUtlVector changes
// since CUtlVector doesn't do this automatically.
SendProp SendPropUtlVector(
	char *pVarName,		// Use SENDINFO_UTLVECTOR to generate these 4.
	int offset,			// Used to generate pData in the function specified in varProxy.
	int sizeofVar,		// The size of each element in the utlvector.
	EnsureCapacityFn ensureFn,	// This is the value returned for elements out of the array's current range.
	int nMaxElements,			// Max # of elements in the array. Keep this as low as possible.
	SendProp pArrayProp,		// Describe the data inside of each element in the array.
	SendTableProxyFn varProxy	// This can be overridden to control who the array is sent to.
	)
{
	SendProp ret;

	Assert( nMaxElements <= MAX_ARRAY_ELEMENTS );

	ret.m_Type = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( 0 );
	ret.SetDataTableProxyFn( varProxy );
	
	// Handle special proxy types where they always let all clients get the results.
	if ( varProxy == SendProxy_DataTableToDataTable || varProxy == SendProxy_DataTablePtrToDataTable )
	{
		ret.SetFlags( SPROP_PROXY_ALWAYS_YES );
	}

	
	// Extra data bound to each of the properties.
	CSendPropExtra_UtlVector *pExtraData = new CSendPropExtra_UtlVector;
	
	pExtraData->m_nMaxElements = nMaxElements;
	pExtraData->m_ElementStride = sizeofVar;
	pExtraData->m_EnsureCapacityFn = ensureFn;
	pExtraData->m_Offset = offset;

	if ( pArrayProp.m_Type == DPT_DataTable )
		pExtraData->m_DataTableProxyFn = pArrayProp.GetDataTableProxyFn();
	else
		pExtraData->m_ProxyFn = pArrayProp.GetProxyFn();


	SendProp *pProps = new SendProp[nMaxElements+1]; // TODO free that again

	// The first property is datatable with an int that tells the length of the array.
	// It has to go in a datatable, otherwise if this array holds datatable properties, it will be received last.
	SendProp *pLengthProp = new SendProp;
	*pLengthProp = SendPropInt( AllocateStringHelper( "lengthprop%d", nMaxElements ), 0, 0, NumBitsForCount( nMaxElements ), SPROP_UNSIGNED, SendProxy_UtlVectorLength );
	pLengthProp->SetExtraData( pExtraData );

	char *pLengthProxyTableName = AllocateUniqueDataTableName( true, "_LPT_%s_%d", pVarName, nMaxElements );
	SendTable *pLengthTable = new SendTable( pLengthProp, 1, pLengthProxyTableName );
	pProps[0] = SendPropDataTable( "lengthproxy", 0, pLengthTable, SendProxy_LengthTable );
	pProps[0].SetExtraData( pExtraData );

	// TERROR:
	char *pParentArrayPropName = AllocateStringHelper( "%s", pVarName );
	Assert( pParentArrayPropName && *pParentArrayPropName ); // TERROR

	// The first element is a sub-datatable.
	for ( int i = 1; i < nMaxElements+1; i++ )
	{
		pProps[i] = pArrayProp;	// copy array element property setting
		pProps[i].SetOffset( 0 ); // leave offset at 0 so pStructBase is always a pointer to the CUtlVector
		pProps[i].m_pVarName = s_ElementNames[i-1];	// give unique name
		pProps[i].m_pParentArrayPropName = pParentArrayPropName; // TERROR: For debugging...
		pProps[i].SetExtraData( pExtraData );
		pProps[i].m_ElementStride = i-1;	// Kind of lame overloading element stride to hold the element index,
											// but we can easily move it into its SetExtraData stuff if we need to.
		
		// We provide our own proxy here.
		if ( pArrayProp.m_Type == DPT_DataTable )
		{
			pProps[i].SetDataTableProxyFn( SendProxy_UtlVectorElement_DataTable );
			pProps[i].SetFlags( SPROP_PROXY_ALWAYS_YES );
		}
		else
		{
			pProps[i].SetProxyFn( SendProxy_UtlVectorElement );
		}
	}

	SendTable *pTable = new SendTable( 
		pProps, 
		nMaxElements+1, 
		AllocateUniqueDataTableName( true, "_ST_%s_%d", pVarName, nMaxElements )
		);

	ret.SetDataTable( pTable );
	return ret;
}
