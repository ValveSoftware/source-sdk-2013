//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_RECV_H
#define DATATABLE_RECV_H

#ifdef _WIN32
#pragma once
#endif

#include "dt_common.h"
#include "tier0/dbg.h"


#define ADDRESSPROXY_NONE	-1


class RecvTable;
class RecvProp;


// This is passed into RecvProxy functions.
class CRecvProxyData
{
public:
	const RecvProp	*m_pRecvProp;		// The property it's receiving.

	DVariant		m_Value;			// The value given to you to store.

	int				m_iElement;			// Which array element you're getting.

	int				m_ObjectID;			// The object being referred to.
};


//-----------------------------------------------------------------------------
// pStruct = the base structure of the datatable this variable is in (like C_BaseEntity)
// pOut    = the variable that this this proxy represents (like C_BaseEntity::m_SomeValue).
//
// Convert the network-standard-type value in m_Value into your own format in pStruct/pOut.
//-----------------------------------------------------------------------------
typedef void (*RecvVarProxyFn)( const CRecvProxyData *pData, void *pStruct, void *pOut );

// ------------------------------------------------------------------------ //
// ArrayLengthRecvProxies are optionally used to get the length of the 
// incoming array when it changes.
// ------------------------------------------------------------------------ //
typedef void (*ArrayLengthRecvProxyFn)( void *pStruct, int objectID, int currentArrayLength );


// NOTE: DataTable receive proxies work differently than the other proxies.
// pData points at the object + the recv table's offset.
// pOut should be set to the location of the object to unpack the data table into.
// If the parent object just contains the child object, the default proxy just does *pOut = pData.
// If the parent object points at the child object, you need to dereference the pointer here.
// NOTE: don't ever return null from a DataTable receive proxy function. Bad things will happen.
typedef void (*DataTableRecvVarProxyFn)(const RecvProp *pProp, void **pOut, void *pData, int objectID);


// This is used to fork over the standard proxy functions to the engine so it can
// make some optimizations.
class CStandardRecvProxies
{
public:
	CStandardRecvProxies();

	RecvVarProxyFn m_Int32ToInt8;
	RecvVarProxyFn m_Int32ToInt16;
	RecvVarProxyFn m_Int32ToInt32;
	RecvVarProxyFn m_FloatToFloat;
	RecvVarProxyFn m_VectorToVector;
#ifdef SUPPORTS_INT64
	RecvVarProxyFn m_Int64ToInt64;
#endif
};
extern CStandardRecvProxies g_StandardRecvProxies;


class CRecvDecoder;


class RecvProp
{
// This info comes from the receive data table.
public:
							RecvProp();

	void					InitArray( int nElements, int elementStride );

	int						GetNumElements() const;
	void					SetNumElements( int nElements );

	int						GetElementStride() const;
	void					SetElementStride( int stride );

	int						GetFlags() const;

	const char*				GetName() const;
	SendPropType			GetType() const;

	RecvTable*				GetDataTable() const;
	void					SetDataTable( RecvTable *pTable );

	RecvVarProxyFn			GetProxyFn() const;
	void					SetProxyFn( RecvVarProxyFn fn );

	DataTableRecvVarProxyFn	GetDataTableProxyFn() const;
	void					SetDataTableProxyFn( DataTableRecvVarProxyFn fn );

	int						GetOffset() const;
	void					SetOffset( int o );

	// Arrays only.
	RecvProp*				GetArrayProp() const;
	void					SetArrayProp( RecvProp *pProp );

	// Arrays only.
	void					SetArrayLengthProxy( ArrayLengthRecvProxyFn proxy );
	ArrayLengthRecvProxyFn	GetArrayLengthProxy() const;

	bool					IsInsideArray() const;
	void					SetInsideArray();

	// Some property types bind more data to the prop in here.
	const void*			GetExtraData() const;
	void				SetExtraData( const void *pData );

	// If it's one of the numbered "000", "001", etc properties in an array, then
	// these can be used to get its array property name for debugging.
	const char*			GetParentArrayPropName();
	void				SetParentArrayPropName( const char *pArrayPropName );

public:

	const char              *m_pVarName;
	SendPropType			m_RecvType;
	int						m_Flags;
	int						m_StringBufferSize;


private:

	bool					m_bInsideArray;		// Set to true by the engine if this property sits inside an array.

	// Extra data that certain special property types bind to the property here.
	const void *m_pExtraData;

	// If this is an array (DPT_Array).
	RecvProp				*m_pArrayProp;
	ArrayLengthRecvProxyFn	m_ArrayLengthProxy;
	
	RecvVarProxyFn			m_ProxyFn;
	DataTableRecvVarProxyFn	m_DataTableProxyFn;	// For RDT_DataTable.

	RecvTable				*m_pDataTable;		// For RDT_DataTable.
	int						m_Offset;
	
	int						m_ElementStride;
	int						m_nElements;

	// If it's one of the numbered "000", "001", etc properties in an array, then
	// these can be used to get its array property name for debugging.
	const char				*m_pParentArrayPropName;
};


class RecvTable
{
public:

	typedef RecvProp	PropType;

				RecvTable();
				RecvTable( RecvProp *pProps, int nProps, const char *pNetTableName );
				~RecvTable();

	void		Construct( RecvProp *pProps, int nProps, const char *pNetTableName );

	int			GetNumProps();
	RecvProp*	GetProp( int i );

	const char*	GetName();

	// Used by the engine while initializing array props.
	void		SetInitialized( bool bInitialized );
	bool		IsInitialized() const;

	// Used by the engine.
	void		SetInMainList( bool bInList );
	bool		IsInMainList() const;


public:

	// Properties described in a table.
	RecvProp		*m_pProps;
	int				m_nProps;

	// The decoder. NOTE: this covers each RecvTable AND all its children (ie: its children
	// will have their own decoders that include props for all their children).
	CRecvDecoder	*m_pDecoder;

	const char		*m_pNetTableName;	// The name matched between client and server.


private:

	bool			m_bInitialized;
	bool			m_bInMainList;
};


inline int RecvTable::GetNumProps()
{
	return m_nProps;
}

inline RecvProp* RecvTable::GetProp( int i )
{
	Assert( i >= 0 && i < m_nProps ); 
	return &m_pProps[i]; 
}

inline const char* RecvTable::GetName()
{
	return m_pNetTableName; 
}

inline void RecvTable::SetInitialized( bool bInitialized )
{
	m_bInitialized = bInitialized;
}

inline bool RecvTable::IsInitialized() const
{
	return m_bInitialized;
}

inline void RecvTable::SetInMainList( bool bInList )
{
	m_bInMainList = bInList;
}

inline bool RecvTable::IsInMainList() const
{
	return m_bInMainList;
}


// ------------------------------------------------------------------------------------------------------ //
// See notes on BEGIN_SEND_TABLE for a description. These macros work similarly.
// ------------------------------------------------------------------------------------------------------ //
#define BEGIN_RECV_TABLE(className, tableName) \
	BEGIN_RECV_TABLE_NOBASE(className, tableName) \
		RecvPropDataTable("baseclass", 0, 0, className::BaseClass::m_pClassRecvTable, DataTableRecvProxy_StaticDataTable),

#define BEGIN_RECV_TABLE_NOBASE(className, tableName) \
	template <typename T> int ClientClassInit(T *); \
	namespace tableName { \
		struct ignored; \
	} \
	template <> int ClientClassInit<tableName::ignored>(tableName::ignored *); \
	namespace tableName {	\
		RecvTable g_RecvTable; \
		int g_RecvTableInit = ClientClassInit((tableName::ignored *)NULL); \
	} \
	template <> int ClientClassInit<tableName::ignored>(tableName::ignored *) \
	{ \
		typedef className currentRecvDTClass; \
		const char *pRecvTableName = #tableName; \
		RecvTable &RecvTable = tableName::g_RecvTable; \
		static RecvProp RecvProps[] = { \
			RecvPropInt("should_never_see_this", 0, sizeof(int)),		// It adds a dummy property at the start so you can define "empty" SendTables.

#define END_RECV_TABLE() \
			}; \
		RecvTable.Construct(RecvProps+1, sizeof(RecvProps) / sizeof(RecvProp) - 1, pRecvTableName); \
		return 1; \
	}

// Normal offset of is invalid on non-array-types, this is dubious as hell. The rest of the codebase converted to the
// legit offsetof from the C headers, so we'll use the old impl here to avoid exposing temptation to others
#define _hacky_dtrecv_offsetof(s,m)	( (size_t)&(((s *)0x1000000)->m) - 0x1000000u )

#define RECVINFO(varName)						#varName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName), sizeof(((currentRecvDTClass*)0)->varName)
#define RECVINFO_NAME(varName, remoteVarName)	#remoteVarName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName), sizeof(((currentRecvDTClass*)0)->varName)
#define RECVINFO_STRING(varName)				#varName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName), STRINGBUFSIZE(currentRecvDTClass, varName)
#define RECVINFO_BASECLASS(tableName)			RecvPropDataTable("this", 0, 0, &REFERENCE_RECV_TABLE(tableName))
#define RECVINFO_ARRAY(varName)					#varName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName), sizeof(((currentRecvDTClass*)0)->varName[0]), sizeof(((currentRecvDTClass*)0)->varName)/sizeof(((currentRecvDTClass*)0)->varName[0])

// Just specify the name and offset. Used for strings and data tables.
#define RECVINFO_NOSIZE(varName)				#varName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName)
#define RECVINFO_DT(varName)					RECVINFO_NOSIZE(varName)
#define RECVINFO_DTNAME(varName,remoteVarName)	#remoteVarName, _hacky_dtrecv_offsetof(currentRecvDTClass, varName)

void RecvProxy_FloatToFloat  ( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_VectorToVector( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_VectorXYToVectorXY( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_QuaternionToQuaternion( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_Int32ToInt8   ( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_Int32ToInt16  ( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_StringToString( const CRecvProxyData *pData, void *pStruct, void *pOut );
void RecvProxy_Int32ToInt32  ( const CRecvProxyData *pData, void *pStruct, void *pOut );
#ifdef SUPPORTS_INT64
void RecvProxy_Int64ToInt64  ( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif

// StaticDataTable does *pOut = pData.
void DataTableRecvProxy_StaticDataTable(const RecvProp *pProp, void **pOut, void *pData, int objectID);

// PointerDataTable does *pOut = *((void**)pData)   (ie: pData is a pointer to the object to decode into).
void DataTableRecvProxy_PointerDataTable(const RecvProp *pProp, void **pOut, void *pData, int objectID);

	
RecvProp RecvPropFloat(
	const char *pVarName, 
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags=0, 
	RecvVarProxyFn varProxy=RecvProxy_FloatToFloat
	);

RecvProp RecvPropVector(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags=0, 
	RecvVarProxyFn varProxy=RecvProxy_VectorToVector
	);

RecvProp RecvPropVectorXY(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags=0, 
	RecvVarProxyFn varProxy=RecvProxy_VectorXYToVectorXY
	);

// This is here so the RecvTable can look more like the SendTable.
#define RecvPropQAngles RecvPropVector

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!

RecvProp RecvPropQuaternion(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags=0, 
	RecvVarProxyFn varProxy=RecvProxy_QuaternionToQuaternion
	);
#endif

RecvProp RecvPropInt(
	const char *pVarName, 
	int offset, 
	int sizeofVar=SIZEOF_IGNORE,	// Handled by RECVINFO macro, but set to SIZEOF_IGNORE if you don't want to bother.
	int flags=0, 
	RecvVarProxyFn varProxy=0
	);

RecvProp RecvPropString(
	const char *pVarName,
	int offset,
	int bufferSize,
	int flags=0,
	RecvVarProxyFn varProxy=RecvProxy_StringToString
	);

RecvProp RecvPropDataTable(
	const char *pVarName,
	int offset,
	int flags,
	RecvTable *pTable,
	DataTableRecvVarProxyFn varProxy=DataTableRecvProxy_StaticDataTable
	);

RecvProp RecvPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	RecvProp pArrayProp,
	DataTableRecvVarProxyFn varProxy=DataTableRecvProxy_StaticDataTable
	);

// Use the macro to let it automatically generate a table name. You shouldn't 
// ever need to reference the table name. If you want to exclude this array, then
// reference the name of the variable in varTemplate.
RecvProp InternalRecvPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthRecvProxyFn proxy
	);


//
// Use this if you want to completely manage the way the array data is stored.
// You'll need to provide a proxy inside varTemplate that looks for 'iElement'
// to figure out where to store the specified element.
//
#define RecvPropVirtualArray( arrayLengthProxy, maxArrayLength, varTemplate, propertyName ) \
	varTemplate, \
	InternalRecvPropArray( \
		maxArrayLength, \
		0, \
		#propertyName, \
		arrayLengthProxy \
		)


// Use this and pass the array name and it will figure out the count and stride automatically.
#define RecvPropVariableLengthArray( arrayLengthProxy, varTemplate, arrayName )			\
	varTemplate,										\
	InternalRecvPropArray(								\
		sizeof(((currentRecvDTClass*)0)->arrayName) / PROPSIZEOF(currentRecvDTClass, arrayName[0]), \
		PROPSIZEOF(currentRecvDTClass, arrayName[0]),	\
		#arrayName,										\
		arrayLengthProxy								\
		)


// Use this and pass the array name and it will figure out the count and stride automatically.
#define RecvPropArray( varTemplate, arrayName )			\
	RecvPropVariableLengthArray( 0, varTemplate, arrayName )


// Use this one to specify the element count and stride manually.
#define RecvPropArray2( arrayLengthProxy, varTemplate, elementCount, elementStride, arrayName )		\
	varTemplate,																	\
	InternalRecvPropArray( elementCount, elementStride, #arrayName, arrayLengthProxy )


// ---------------------------------------------------------------------------------------- //
// Inlines.
// ---------------------------------------------------------------------------------------- //

inline void RecvProp::InitArray( int nElements, int elementStride )
{
	m_RecvType = DPT_Array;
	m_nElements = nElements;
	m_ElementStride = elementStride;
}

inline int RecvProp::GetNumElements() const
{
	return m_nElements;
}

inline void RecvProp::SetNumElements( int nElements )
{
	m_nElements = nElements;
}

inline int RecvProp::GetElementStride() const
{
	return m_ElementStride;
}

inline void RecvProp::SetElementStride( int stride )
{
	m_ElementStride = stride;
}

inline int RecvProp::GetFlags() const
{
	return m_Flags;
}

inline const char* RecvProp::GetName() const
{
	return m_pVarName; 
}

inline SendPropType RecvProp::GetType() const
{
	return m_RecvType; 
}

inline RecvTable* RecvProp::GetDataTable() const 
{
	return m_pDataTable; 
}

inline void RecvProp::SetDataTable( RecvTable *pTable )
{
	m_pDataTable = pTable; 
}

inline RecvVarProxyFn RecvProp::GetProxyFn() const 
{
	return m_ProxyFn; 
}

inline void RecvProp::SetProxyFn( RecvVarProxyFn fn )
{
	m_ProxyFn = fn; 
}

inline DataTableRecvVarProxyFn RecvProp::GetDataTableProxyFn() const
{
	return m_DataTableProxyFn; 
}

inline void RecvProp::SetDataTableProxyFn( DataTableRecvVarProxyFn fn )
{
	m_DataTableProxyFn = fn; 
}

inline int RecvProp::GetOffset() const	
{
	return m_Offset; 
}

inline void RecvProp::SetOffset( int o )
{
	m_Offset = o; 
}

inline RecvProp* RecvProp::GetArrayProp() const
{
	return m_pArrayProp;
}

inline void RecvProp::SetArrayProp( RecvProp *pProp )
{
	m_pArrayProp = pProp;
}

inline void RecvProp::SetArrayLengthProxy( ArrayLengthRecvProxyFn proxy )
{
	m_ArrayLengthProxy = proxy;
}

inline ArrayLengthRecvProxyFn RecvProp::GetArrayLengthProxy() const
{
	return m_ArrayLengthProxy;
}

inline bool RecvProp::IsInsideArray() const
{
	return m_bInsideArray;
}

inline void RecvProp::SetInsideArray()
{
	m_bInsideArray = true;
}

inline const void* RecvProp::GetExtraData() const
{
	return m_pExtraData;
}

inline void RecvProp::SetExtraData( const void *pData )
{
	m_pExtraData = pData;
}

inline const char* RecvProp::GetParentArrayPropName()
{
	return m_pParentArrayPropName;
}

inline void	RecvProp::SetParentArrayPropName( const char *pArrayPropName )
{
	m_pParentArrayPropName = pArrayPropName;
}

#endif // DATATABLE_RECV_H
