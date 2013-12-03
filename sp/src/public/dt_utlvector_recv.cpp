//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "dt_utlvector_recv.h"

#include "tier0/memdbgon.h"


extern const char *s_ClientElementNames[MAX_ARRAY_ELEMENTS];


class CRecvPropExtra_UtlVector
{
public:
	DataTableRecvVarProxyFn m_DataTableProxyFn;	// If it's a datatable, then this is the proxy they specified.
	RecvVarProxyFn m_ProxyFn;				// If it's a non-datatable, then this is the proxy they specified.
	ResizeUtlVectorFn m_ResizeFn;			// The function used to resize the CUtlVector.
	EnsureCapacityFn m_EnsureCapacityFn;
	int m_ElementStride;					// Distance between each element in the array.
	int m_Offset;							// Offset of the CUtlVector from its parent structure.
	int m_nMaxElements;						// For debugging...
};

void RecvProxy_UtlVectorLength( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pData->m_pRecvProp->GetExtraData();
	pExtra->m_ResizeFn( pStruct, pExtra->m_Offset, pData->m_Value.m_Int );
}

void RecvProxy_UtlVectorElement( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pData->m_pRecvProp->GetExtraData();

	// Kind of lame overloading element stride to hold the element index,
	// but we can easily move it into its SetExtraData stuff if we need to.
	int iElement = pData->m_pRecvProp->GetElementStride();

	// NOTE: this is cheesy, but it does the trick.
	CUtlVector<int> *pUtlVec = (CUtlVector<int>*)((char*)pStruct + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector.
	// Note: there should be space here as long as the element is < the max # elements
	// that we ensured capacity for in DataTableRecvProxy_LengthProxy.
	pExtra->m_ProxyFn( pData, pOut, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride );
}

void RecvProxy_UtlVectorElement_DataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pProp->GetExtraData();

	int iElement = pProp->GetElementStride();
	Assert( iElement < pExtra->m_nMaxElements );

	// NOTE: this is cheesy, but it does the trick.
	CUtlVector<int> *pUtlVec = (CUtlVector<int>*)((char*)pData + pExtra->m_Offset);

	// Call through to the proxy they passed in, making pStruct=the CUtlVector and forcing iElement to 0.
	pExtra->m_DataTableProxyFn( pProp, pOut, (char*)pUtlVec->Base() + iElement*pExtra->m_ElementStride, objectID );
}


void DataTableRecvProxy_LengthProxy( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	// This is VERY important - since it calls all the datatable proxies in the tree first, 
	// particularly BEFORE it calls our array length proxy, we need to make sure we return 
	// valid pointers that aren't going to change when it starts to copy the data into 
	// the datatable elements.
	CRecvPropExtra_UtlVector *pExtra = (CRecvPropExtra_UtlVector*)pProp->GetExtraData();
	pExtra->m_EnsureCapacityFn( pData, pExtra->m_Offset, pExtra->m_nMaxElements );
	
	*pOut = pData;
}


RecvProp RecvPropUtlVector(
	const char *pVarName,		// Use RECVINFO_UTLVECTOR to generate these 4.
	int offset,			// Used to generate pData in the function specified in varProxy.
	int sizeofVar,		// The size of each element in the utlvector.
	ResizeUtlVectorFn fn,
	EnsureCapacityFn ensureFn,
	int nMaxElements,											// Max # of elements in the array. Keep this as low as possible.
	RecvProp pArrayProp
	)
{
	RecvProp ret;

	Assert( nMaxElements <= MAX_ARRAY_ELEMENTS );

	ret.m_RecvType = DPT_DataTable;
	ret.m_pVarName = pVarName;
	ret.SetOffset( 0 );
	ret.SetDataTableProxyFn( DataTableRecvProxy_StaticDataTable );
	
	RecvProp *pProps = new RecvProp[nMaxElements+1]; // TODO free that again

	
	// Extra data bound to each of the properties.
	CRecvPropExtra_UtlVector *pExtraData = new CRecvPropExtra_UtlVector;
	
	pExtraData->m_nMaxElements = nMaxElements;
	pExtraData->m_ElementStride = sizeofVar;
	pExtraData->m_ResizeFn = fn;
	pExtraData->m_EnsureCapacityFn = ensureFn;
	pExtraData->m_Offset = offset;
	
	if ( pArrayProp.m_RecvType == DPT_DataTable )
		pExtraData->m_DataTableProxyFn = pArrayProp.GetDataTableProxyFn();
	else
		pExtraData->m_ProxyFn = pArrayProp.GetProxyFn();


	// The first property is datatable with an int that tells the length of the array.
	// It has to go in a datatable, otherwise if this array holds datatable properties, it will be received last.
	RecvProp *pLengthProp = new RecvProp;
	*pLengthProp = RecvPropInt( AllocateStringHelper( "lengthprop%d", nMaxElements ), 0, 0, 0, RecvProxy_UtlVectorLength );
	pLengthProp->SetExtraData( pExtraData );

	char *pLengthProxyTableName = AllocateUniqueDataTableName( false, "_LPT_%s_%d", pVarName, nMaxElements );
	RecvTable *pLengthTable = new RecvTable( pLengthProp, 1, pLengthProxyTableName );
	pProps[0] = RecvPropDataTable( "lengthproxy", 0, 0, pLengthTable, DataTableRecvProxy_LengthProxy );
	pProps[0].SetExtraData( pExtraData );

	// The first element is a sub-datatable.
	for ( int i = 1; i < nMaxElements+1; i++ )
	{
		pProps[i] = pArrayProp;	// copy array element property setting
		pProps[i].SetOffset( 0 ); // leave offset at 0 so pStructBase is always a pointer to the CUtlVector
		pProps[i].m_pVarName = s_ClientElementNames[i-1];	// give unique name
		pProps[i].SetExtraData( pExtraData );
		pProps[i].SetElementStride( i-1 );	// Kind of lame overloading element stride to hold the element index,
											// but we can easily move it into its SetExtraData stuff if we need to.
		
		// We provide our own proxy here.
		if ( pArrayProp.m_RecvType == DPT_DataTable )
		{
			pProps[i].SetDataTableProxyFn( RecvProxy_UtlVectorElement_DataTable );
		}
		else
		{
			pProps[i].SetProxyFn( RecvProxy_UtlVectorElement );
		}
	}

	RecvTable *pTable = new RecvTable( 
		pProps, 
		nMaxElements+1, 
		AllocateUniqueDataTableName( false, "_ST_%s_%d", pVarName, nMaxElements )
		); // TODO free that again

	ret.SetDataTable( pTable );
	return ret;
}
