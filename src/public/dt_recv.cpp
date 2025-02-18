//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "dt_recv.h"
#include "mathlib/vector.h"
#include "tier1/strtools.h"
#include "dt_utlvector_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)

CStandardRecvProxies::CStandardRecvProxies()
{
	m_Int32ToInt8 = RecvProxy_Int32ToInt8;
	m_Int32ToInt16 = RecvProxy_Int32ToInt16;
	m_Int32ToInt32 = RecvProxy_Int32ToInt32;
#ifdef SUPPORTS_INT64
	m_Int64ToInt64 = RecvProxy_Int64ToInt64;
#endif
	m_FloatToFloat = RecvProxy_FloatToFloat;
	m_VectorToVector = RecvProxy_VectorToVector;
}

CStandardRecvProxies g_StandardRecvProxies;


// ---------------------------------------------------------------------- //
// RecvProp.
// ---------------------------------------------------------------------- //
RecvProp::RecvProp()
{
	m_pExtraData = NULL;
	m_pVarName = NULL;
	m_Offset = 0;
	m_RecvType = DPT_Int;
	m_Flags = 0;
	m_ProxyFn = NULL;
	m_DataTableProxyFn = NULL;
	m_pDataTable = NULL;
	m_nElements = 1;
	m_ElementStride = -1;
	m_pArrayProp = NULL;
	m_ArrayLengthProxy = NULL;
	m_bInsideArray = false;
}

// ---------------------------------------------------------------------- //
// RecvTable.
// ---------------------------------------------------------------------- //
RecvTable::RecvTable()
{
	Construct( NULL, 0, NULL );
}

RecvTable::RecvTable(RecvProp *pProps, int nProps, const char *pNetTableName)
{
	Construct( pProps, nProps, pNetTableName );
}

RecvTable::~RecvTable()
{
}

void RecvTable::Construct( RecvProp *pProps, int nProps, const char *pNetTableName )
{
	m_pProps = pProps;
	m_nProps = nProps;
	m_pDecoder = NULL;
	m_pNetTableName = pNetTableName;
	m_bInitialized = false;
	m_bInMainList = false;
}


// ---------------------------------------------------------------------- //
// Prop setup functions (for building tables).
// ---------------------------------------------------------------------- //

RecvProp RecvPropFloat(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

#ifdef _DEBUG
	if ( varProxy == RecvProxy_FloatToFloat )
	{
		Assert( sizeofVar == 0 || sizeofVar == 4 );
	}
#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Float;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropVector(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

#ifdef _DEBUG
	if ( varProxy == RecvProxy_VectorToVector )
	{
		Assert( sizeofVar == sizeof( Vector ) );
	}
#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Vector;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropVectorXY(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

#ifdef _DEBUG
	if ( varProxy == RecvProxy_VectorToVector )
	{
		Assert( sizeofVar == sizeof( Vector ) );
	}
#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_VectorXY;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!

RecvProp RecvPropQuaternion(
	const char *pVarName, 
	int offset, 
	int sizeofVar,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

#ifdef _DEBUG
	if ( varProxy == RecvProxy_QuaternionToQuaternion )
	{
		Assert( sizeofVar == sizeof( Quaternion ) );
	}
#endif

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_Quaternion;
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}
#endif

RecvProp RecvPropInt(
	const char *pVarName, 
	int offset, 
	int sizeofVar,
	int flags, 
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	// If they didn't specify a proxy, then figure out what type we're writing to.
	if (varProxy == NULL)
	{
		if (sizeofVar == 1)
		{
			varProxy = RecvProxy_Int32ToInt8;
		}
		else if (sizeofVar == 2)
		{
			varProxy = RecvProxy_Int32ToInt16;
		}
		else if (sizeofVar == 4)
		{
			varProxy = RecvProxy_Int32ToInt32;
		}
#ifdef SUPPORTS_INT64		
		else if (sizeofVar == 8)
		{
			varProxy = RecvProxy_Int64ToInt64;
		}
#endif
		else
		{
			Assert(!"RecvPropInt var has invalid size");
			varProxy = RecvProxy_Int32ToInt8;	// safest one...
		}
	}

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
#ifdef SUPPORTS_INT64
	ret.m_RecvType = (sizeofVar == 8) ? DPT_Int64 : DPT_Int;
#else
	ret.m_RecvType = DPT_Int;
#endif
	ret.m_Flags = flags;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropString(
	const char *pVarName,
	int offset,
	int bufferSize,
	int flags,
	RecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_String;
	ret.m_Flags = flags;
	ret.m_StringBufferSize = bufferSize;
	ret.SetProxyFn( varProxy );

	return ret;
}

RecvProp RecvPropDataTable(
	const char *pVarName,
	int offset,
	int flags,
	RecvTable *pTable,
	DataTableRecvVarProxyFn varProxy
	)
{
	RecvProp ret;

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_DataTable;
	ret.m_Flags = flags;
	ret.SetDataTableProxyFn( varProxy );
	ret.SetDataTable( pTable );

	return ret;
}

RecvProp RecvPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	RecvProp pArrayProp,
	DataTableRecvVarProxyFn varProxy						
	)
{
	RecvProp ret;

	Assert( elements <= MAX_ARRAY_ELEMENTS );

	ret.m_pVarName = pVarName;
	ret.SetOffset( offset );
	ret.m_RecvType = DPT_DataTable;
	ret.SetDataTableProxyFn( varProxy );

	RecvProp *pProps = new RecvProp[elements]; // TODO free that again

	const char *pParentArrayPropName = AllocateStringHelper( "%s", pVarName );

	for ( int i=0; i < elements; i++ )
	{
		pProps[i] = pArrayProp; // copy basic property settings 
		pProps[i].SetOffset( i * sizeofVar ); // adjust offset
		pProps[i].m_pVarName = DT_ArrayElementNameForIdx(i); // give unique name
		pProps[i].SetParentArrayPropName( pParentArrayPropName ); // For debugging...
	}

	RecvTable *pTable = new RecvTable( pProps, elements, pVarName ); // TODO free that again

	ret.SetDataTable( pTable );

	return ret;
}

RecvProp InternalRecvPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthRecvProxyFn proxy
	)
{
	RecvProp ret;

	ret.InitArray( elementCount, elementStride );
	ret.m_pVarName = pName;
	ret.SetArrayLengthProxy( proxy );

	return ret;
}


// ---------------------------------------------------------------------- //
// Proxies.
// ---------------------------------------------------------------------- //

void RecvProxy_FloatToFloat( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	Assert( IsFinite( pData->m_Value.m_Float ) );
	*((float*)pOut) = pData->m_Value.m_Float;
}

void RecvProxy_VectorToVector( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) && IsFinite( v[2] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
	((float*)pOut)[2] = v[2];
}

void RecvProxy_VectorXYToVectorXY( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
}

void RecvProxy_QuaternionToQuaternion( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	const float *v = pData->m_Value.m_Vector;
	
	Assert( IsFinite( v[0] ) && IsFinite( v[1] ) && IsFinite( v[2] ) && IsFinite( v[3] ) );
	((float*)pOut)[0] = v[0];
	((float*)pOut)[1] = v[1];
	((float*)pOut)[2] = v[2];
	((float*)pOut)[3] = v[3];
}

void RecvProxy_Int32ToInt8( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned char*)pOut) = (unsigned char)pData->m_Value.m_Int;
}

void RecvProxy_Int32ToInt16( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((unsigned short*)pOut) = (unsigned short)pData->m_Value.m_Int;
}

void RecvProxy_Int32ToInt32( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((uint32*)pOut) = (uint32)pData->m_Value.m_Int;
}

#ifdef SUPPORTS_INT64
void RecvProxy_Int64ToInt64( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	*((int64*)pOut) = (int64)pData->m_Value.m_Int64;
}
#endif

void RecvProxy_StringToString( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	char *pStrOut = (char*)pOut;
	if ( pData->m_pRecvProp->m_StringBufferSize <= 0 )
	{
		return;
	}

	for ( int i=0; i < pData->m_pRecvProp->m_StringBufferSize; i++ )
	{
		pStrOut[i] = pData->m_Value.m_pString[i];
		if ( pStrOut[i] == 0 )
			break;
	}
	
	pStrOut[pData->m_pRecvProp->m_StringBufferSize-1] = 0;
}

void DataTableRecvProxy_StaticDataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	*pOut = pData;
}

void DataTableRecvProxy_PointerDataTable( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	*pOut = *((void**)pData);
}

#endif
