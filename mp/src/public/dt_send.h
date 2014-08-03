//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef DATATABLE_SEND_H
#define DATATABLE_SEND_H

#ifdef _WIN32
#pragma once
#endif

#include "dt_common.h"
#include "tier0/dbg.h"
#include "const.h"
#include "bitvec.h"


// ------------------------------------------------------------------------ //
// Send proxies can be used to convert a variable into a networkable type 
// (a good example is converting an edict pointer into an integer index).

// These allow you to translate data. For example, if you had a user-entered 
// string number like "10" (call the variable pUserStr) and wanted to encode 
// it as an integer, you would use a SendPropInt32 and write a proxy that said:
// pOut->m_Int = atoi(pUserStr);

// pProp       : the SendProp that has the proxy
// pStructBase : the base structure (like CBaseEntity*).
// pData       : the address of the variable to proxy.
// pOut        : where to output the proxied value.
// iElement    : the element index if this data is part of an array (or 0 if not).
// objectID    : entity index for debugging purposes.

// Return false if you don't want the engine to register and send a delta to
// the clients for this property (regardless of whether it actually changed or not).
// ------------------------------------------------------------------------ //
typedef void (*SendVarProxyFn)( const SendProp *pProp, const void *pStructBase, const void *pData, DVariant *pOut, int iElement, int objectID );

// Return the pointer to the data for the datatable.
// If the proxy returns null, it's the same as if pRecipients->ClearAllRecipients() was called.
class CSendProxyRecipients;

typedef void* (*SendTableProxyFn)( 
	const SendProp *pProp, 
	const void *pStructBase, 
	const void *pData, 
	CSendProxyRecipients *pRecipients, 
	int objectID );


class CNonModifiedPointerProxy
{
public:
	CNonModifiedPointerProxy( SendTableProxyFn fn );

public:
	
	SendTableProxyFn m_Fn;
	CNonModifiedPointerProxy *m_pNext;
};


// This tells the engine that the send proxy will not modify the pointer
// - it only plays with the recipients. This must be set on proxies that work
// this way, otherwise the engine can't track which properties changed
// in NetworkStateChanged().
#define REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( sendProxyFn ) static CNonModifiedPointerProxy __proxy_##sendProxyFn( sendProxyFn );


class CStandardSendProxiesV1
{
public:
	CStandardSendProxiesV1();

	SendVarProxyFn m_Int8ToInt32;
	SendVarProxyFn m_Int16ToInt32;
	SendVarProxyFn m_Int32ToInt32;

	SendVarProxyFn m_UInt8ToInt32;
	SendVarProxyFn m_UInt16ToInt32;
	SendVarProxyFn m_UInt32ToInt32;

	SendVarProxyFn m_FloatToFloat;
	SendVarProxyFn m_VectorToVector;

#ifdef SUPPORTS_INT64
	SendVarProxyFn m_Int64ToInt64;
	SendVarProxyFn m_UInt64ToInt64;
#endif
};
	
class CStandardSendProxies : public CStandardSendProxiesV1
{
public:
	CStandardSendProxies();
	
	SendTableProxyFn m_DataTableToDataTable;
	SendTableProxyFn m_SendLocalDataTable;
	CNonModifiedPointerProxy **m_ppNonModifiedPointerProxies;
};

extern CStandardSendProxies g_StandardSendProxies;


// Max # of datatable send proxies you can have in a tree.
#define MAX_DATATABLE_PROXIES	32

// ------------------------------------------------------------------------ //
// Datatable send proxies are used to tell the engine where the datatable's 
// data is and to specify which clients should get the data. 
//
// pRecipients is the object that allows you to specify which clients will
// receive the data.
// ------------------------------------------------------------------------ //
class CSendProxyRecipients
{
public:
	void	SetAllRecipients();					// Note: recipients are all set by default when each proxy is called.
	void	ClearAllRecipients();

	void	SetRecipient( int iClient );		// Note: these are CLIENT indices, not entity indices (so the first player's index is 0).
	void	ClearRecipient( int iClient );

	// Clear all recipients and set only the specified one.
	void	SetOnly( int iClient );

public:
	// Make sure we have enough room for the max possible player count
	CBitVec< ABSOLUTE_PLAYER_LIMIT >	m_Bits;
};

inline void CSendProxyRecipients::SetAllRecipients()
{
	m_Bits.SetAll();
}

inline void CSendProxyRecipients::ClearAllRecipients()
{
	m_Bits.ClearAll();
}

inline void CSendProxyRecipients::SetRecipient( int iClient )
{
	m_Bits.Set( iClient );
}

inline void	CSendProxyRecipients::ClearRecipient( int iClient )
{
	m_Bits.Clear( iClient );
}

inline void CSendProxyRecipients::SetOnly( int iClient )
{
	m_Bits.ClearAll();
	m_Bits.Set( iClient );
}



// ------------------------------------------------------------------------ //
// ArrayLengthSendProxies are used when you want to specify an array's length
// dynamically.
// ------------------------------------------------------------------------ //
typedef int (*ArrayLengthSendProxyFn)( const void *pStruct, int objectID );



class RecvProp;
class SendTable;
class CSendTablePrecalc;


// -------------------------------------------------------------------------------------------------------------- //
// SendProp.
// -------------------------------------------------------------------------------------------------------------- //

// If SendProp::GetDataTableProxyIndex() returns this, then the proxy is one that always sends
// the data to all clients, so we don't need to store the results.
#define DATATABLE_PROXY_INDEX_NOPROXY	255
#define DATATABLE_PROXY_INDEX_INVALID	254

class SendProp
{
public:
						SendProp();
	virtual				~SendProp();

	void				Clear();

	int					GetOffset() const;
	void				SetOffset( int i );

	SendVarProxyFn		GetProxyFn() const;
	void				SetProxyFn( SendVarProxyFn f );
	
	SendTableProxyFn	GetDataTableProxyFn() const;
	void				SetDataTableProxyFn( SendTableProxyFn f );
	
	SendTable*			GetDataTable() const;
	void				SetDataTable( SendTable *pTable );

	char const*			GetExcludeDTName() const;
	
	// If it's one of the numbered "000", "001", etc properties in an array, then
	// these can be used to get its array property name for debugging.
	const char*			GetParentArrayPropName() const;
	void				SetParentArrayPropName( char *pArrayPropName );

	const char*			GetName() const;

	bool				IsSigned() const;
	
	bool				IsExcludeProp() const;
	
	bool				IsInsideArray() const;	// Returns true if SPROP_INSIDEARRAY is set.
	void				SetInsideArray();

	// Arrays only.
	void				SetArrayProp( SendProp *pProp );
	SendProp*			GetArrayProp() const;

	// Arrays only.
	void					SetArrayLengthProxy( ArrayLengthSendProxyFn fn );
	ArrayLengthSendProxyFn	GetArrayLengthProxy() const;

	int					GetNumElements() const;
	void				SetNumElements( int nElements );

	// Return the # of bits to encode an array length (must hold GetNumElements()).
	int					GetNumArrayLengthBits() const;

	int					GetElementStride() const;

	SendPropType		GetType() const;

	int					GetFlags() const;
	void				SetFlags( int flags );	

	// Some property types bind more data to the SendProp in here.
	const void*			GetExtraData() const;
	void				SetExtraData( const void *pData );

public:

	RecvProp		*m_pMatchingRecvProp;	// This is temporary and only used while precalculating
												// data for the decoders.

	SendPropType	m_Type;
	int				m_nBits;
	float			m_fLowValue;
	float			m_fHighValue;
	
	SendProp		*m_pArrayProp;					// If this is an array, this is the property that defines each array element.
	ArrayLengthSendProxyFn	m_ArrayLengthProxy;	// This callback returns the array length.
	
	int				m_nElements;		// Number of elements in the array (or 1 if it's not an array).
	int				m_ElementStride;	// Pointer distance between array elements.

	const char *m_pExcludeDTName;			// If this is an exclude prop, then this is the name of the datatable to exclude a prop from.
	const char *m_pParentArrayPropName;

	const char		*m_pVarName;
	float			m_fHighLowMul;
	
private:

	int					m_Flags;				// SPROP_ flags.

	SendVarProxyFn		m_ProxyFn;				// NULL for DPT_DataTable.
	SendTableProxyFn	m_DataTableProxyFn;		// Valid for DPT_DataTable.
	
	SendTable			*m_pDataTable;
	
	// SENDPROP_VECTORELEM makes this negative to start with so we can detect that and
	// set the SPROP_IS_VECTOR_ELEM flag.
	int					m_Offset;

	// Extra data bound to this property.
	const void			*m_pExtraData;
};


inline int SendProp::GetOffset() const
{
	return m_Offset; 
}

inline void SendProp::SetOffset( int i )
{
	m_Offset = i; 
}

inline SendVarProxyFn SendProp::GetProxyFn() const
{
	Assert( m_Type != DPT_DataTable );
	return m_ProxyFn; 
}

inline void SendProp::SetProxyFn( SendVarProxyFn f )
{
	m_ProxyFn = f; 
}

inline SendTableProxyFn SendProp::GetDataTableProxyFn() const
{
	Assert( m_Type == DPT_DataTable );
	return m_DataTableProxyFn; 
}

inline void SendProp::SetDataTableProxyFn( SendTableProxyFn f )
{
	m_DataTableProxyFn = f; 
}

inline SendTable* SendProp::GetDataTable() const
{
	return m_pDataTable;
}

inline void SendProp::SetDataTable( SendTable *pTable )
{
	m_pDataTable = pTable; 
}

inline char const* SendProp::GetExcludeDTName() const
{
	return m_pExcludeDTName; 
}

inline const char* SendProp::GetParentArrayPropName() const
{
	return m_pParentArrayPropName;
}

inline void	SendProp::SetParentArrayPropName( char *pArrayPropName )
{
	Assert( !m_pParentArrayPropName );
	m_pParentArrayPropName = pArrayPropName;
}

inline const char* SendProp::GetName() const
{
	return m_pVarName; 
}


inline bool SendProp::IsSigned() const
{
	return !(m_Flags & SPROP_UNSIGNED); 
}

inline bool SendProp::IsExcludeProp() const
{
	return (m_Flags & SPROP_EXCLUDE) != 0;
}

inline bool	SendProp::IsInsideArray() const
{
	return (m_Flags & SPROP_INSIDEARRAY) != 0;
}

inline void SendProp::SetInsideArray()
{
	m_Flags |= SPROP_INSIDEARRAY;
}

inline void SendProp::SetArrayProp( SendProp *pProp )
{
	m_pArrayProp = pProp;
}

inline SendProp* SendProp::GetArrayProp() const
{
	return m_pArrayProp;
}
	 
inline void SendProp::SetArrayLengthProxy( ArrayLengthSendProxyFn fn )
{
	m_ArrayLengthProxy = fn;
}

inline ArrayLengthSendProxyFn SendProp::GetArrayLengthProxy() const
{
	return m_ArrayLengthProxy;
}
	 
inline int SendProp::GetNumElements() const
{
	return m_nElements; 
}

inline void SendProp::SetNumElements( int nElements )
{
	m_nElements = nElements;
}

inline int SendProp::GetElementStride() const
{
	return m_ElementStride; 
}

inline SendPropType SendProp::GetType() const
{
	return m_Type; 
}

inline int SendProp::GetFlags() const
{
	return m_Flags;
}

inline void SendProp::SetFlags( int flags )
{
	// Make sure they're using something from the valid set of flags.
	Assert( !( flags & ~((1 << SPROP_NUMFLAGBITS) - 1) ) );
	m_Flags = flags;
}

inline const void* SendProp::GetExtraData() const
{
	return m_pExtraData;
}

inline void SendProp::SetExtraData( const void *pData )
{
	m_pExtraData = pData;
}


// -------------------------------------------------------------------------------------------------------------- //
// SendTable.
// -------------------------------------------------------------------------------------------------------------- //

class SendTable
{
public:

	typedef SendProp PropType;

				SendTable();
				SendTable( SendProp *pProps, int nProps, const char *pNetTableName );
				~SendTable();

	void		Construct( SendProp *pProps, int nProps, const char *pNetTableName );

	const char*	GetName() const;
	
	int			GetNumProps() const;
	SendProp*	GetProp( int i );

	// Used by the engine.
	bool		IsInitialized() const;
	void		SetInitialized( bool bInitialized );

	// Used by the engine while writing info into the signon.
	void		SetWriteFlag(bool bHasBeenWritten);
	bool		GetWriteFlag() const;

	bool		HasPropsEncodedAgainstTickCount() const;
	void		SetHasPropsEncodedAgainstTickcount( bool bState );

public:

	SendProp	*m_pProps;
	int			m_nProps;

	const char	*m_pNetTableName;	// The name matched between client and server.

	// The engine hooks the SendTable here.
	CSendTablePrecalc	*m_pPrecalc;


protected:		
	bool		m_bInitialized : 1;	
	bool		m_bHasBeenWritten : 1;		
	bool		m_bHasPropsEncodedAgainstCurrentTickCount : 1; // m_flSimulationTime and m_flAnimTime, e.g.
};


inline const char* SendTable::GetName() const
{
	return m_pNetTableName;
}


inline int SendTable::GetNumProps() const
{
	return m_nProps;
}


inline SendProp* SendTable::GetProp( int i )
{
	Assert( i >= 0 && i < m_nProps );
	return &m_pProps[i];
}


inline bool SendTable::IsInitialized() const
{
	return m_bInitialized;
}


inline void SendTable::SetInitialized( bool bInitialized )
{
	m_bInitialized = bInitialized;
}


inline bool SendTable::GetWriteFlag() const
{
	return m_bHasBeenWritten;
}


inline void SendTable::SetWriteFlag(bool bHasBeenWritten)
{
	m_bHasBeenWritten = bHasBeenWritten;
}

inline bool SendTable::HasPropsEncodedAgainstTickCount() const
{
	return m_bHasPropsEncodedAgainstCurrentTickCount;
}

inline void SendTable::SetHasPropsEncodedAgainstTickcount( bool bState )
{
	m_bHasPropsEncodedAgainstCurrentTickCount = bState;
}

// ------------------------------------------------------------------------------------------------------ //
// Use BEGIN_SEND_TABLE if you want to declare a SendTable and have it inherit all the properties from
// its base class. There are two requirements for this to work:

// 1. Its base class must have a static SendTable pointer member variable called m_pClassSendTable which
//    points to its send table. The DECLARE_SERVERCLASS and IMPLEMENT_SERVERCLASS macros do this automatically.

// 2. Your class must typedef its base class as BaseClass. So it would look like this:
//    class Derived : public CBaseEntity
//    {
//    typedef CBaseEntity BaseClass;
//    };

// If you don't want to interit a base class's properties, use BEGIN_SEND_TABLE_NOBASE.
// ------------------------------------------------------------------------------------------------------ //
#define BEGIN_SEND_TABLE(className, tableName) \
	BEGIN_SEND_TABLE_NOBASE(className, tableName) \
		SendPropDataTable("baseclass", 0, className::BaseClass::m_pClassSendTable, SendProxy_DataTableToDataTable),

#define BEGIN_SEND_TABLE_NOBASE(className, tableName) \
	template <typename T> int ServerClassInit(T *); \
	namespace tableName { \
		struct ignored; \
	} \
	template <> int ServerClassInit<tableName::ignored>(tableName::ignored *); \
	namespace tableName { \
		SendTable g_SendTable;\
		int g_SendTableInit = ServerClassInit((tableName::ignored *)NULL); \
	} \
	template <> int ServerClassInit<tableName::ignored>(tableName::ignored *) \
	{ \
		typedef className currentSendDTClass; \
		static const char *g_pSendTableName = #tableName; \
		SendTable &sendTable = tableName::g_SendTable; \
		static SendProp g_SendProps[] = { \
			SendPropInt("should_never_see_this", 0, sizeof(int)),		// It adds a dummy property at the start so you can define "empty" SendTables.

#define END_SEND_TABLE() \
		};\
		sendTable.Construct(g_SendProps+1, sizeof(g_SendProps) / sizeof(SendProp) - 1, g_pSendTableName);\
		return 1; \
	} 


// These can simplify creating the variables.
// Note: currentSendDTClass::MakeANetworkVar_##varName equates to currentSendDTClass. It's
// there as a check to make sure all networked variables use the CNetworkXXXX macros in network_var.h.
#define SENDINFO(varName)					#varName, offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName), sizeof(((currentSendDTClass*)0)->varName)
#define SENDINFO_ARRAY(varName)				#varName, offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName), sizeof(((currentSendDTClass*)0)->varName[0])
#define SENDINFO_ARRAY3(varName)			#varName, offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName), sizeof(((currentSendDTClass*)0)->varName[0]), sizeof(((currentSendDTClass*)0)->varName)/sizeof(((currentSendDTClass*)0)->varName[0])
#define SENDINFO_ARRAYELEM(varName, i)		#varName "[" #i "]", offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName[i]), sizeof(((currentSendDTClass*)0)->varName[0])
#define SENDINFO_NETWORKARRAYELEM(varName, i)#varName "[" #i "]", offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName.m_Value[i]), sizeof(((currentSendDTClass*)0)->varName.m_Value[0])

// NOTE: Be VERY careful to specify any other vector elems for the same vector IN ORDER and 
// right after each other, otherwise it might miss the Y or Z component in SP.
//
// Note: this macro specifies a negative offset so the engine can detect it and setup m_pNext
#define SENDINFO_VECTORELEM(varName, i)		#varName "[" #i "]", -(int)offsetof(currentSendDTClass::MakeANetworkVar_##varName, varName.m_Value[i]), sizeof(((currentSendDTClass*)0)->varName.m_Value[0])

#define SENDINFO_STRUCTELEM(varName)		#varName, offsetof(currentSendDTClass, varName), sizeof(((currentSendDTClass*)0)->varName.m_Value)
#define SENDINFO_STRUCTARRAYELEM(varName, i)#varName "[" #i "]", offsetof(currentSendDTClass, varName.m_Value[i]), sizeof(((currentSendDTClass*)0)->varName.m_Value[0])

// Use this when you're not using a CNetworkVar to represent the data you're sending.
#define SENDINFO_NOCHECK(varName)						#varName, offsetof(currentSendDTClass, varName), sizeof(((currentSendDTClass*)0)->varName)
#define SENDINFO_STRING_NOCHECK(varName)				#varName, offsetof(currentSendDTClass, varName)
#define SENDINFO_DT(varName)							#varName, offsetof(currentSendDTClass, varName)
#define SENDINFO_DT_NAME(varName, remoteVarName)		#remoteVarName, offsetof(currentSendDTClass, varName)
#define SENDINFO_NAME(varName,remoteVarName)			#remoteVarName, offsetof(currentSendDTClass, varName), sizeof(((currentSendDTClass*)0)->varName)




// ------------------------------------------------------------------------ //
// Built-in proxy types.
// See the definition of SendVarProxyFn for information about these.
// ------------------------------------------------------------------------ //
void SendProxy_QAngles			( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_AngleToFloat		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_FloatToFloat		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_VectorToVector	( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_VectorXYToVectorXY( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
void SendProxy_QuaternionToQuaternion( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif

void SendProxy_Int8ToInt32		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Int16ToInt32		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
void SendProxy_Int32ToInt32		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#ifdef SUPPORTS_INT64
void SendProxy_Int64ToInt64		( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif
void SendProxy_StringToString	( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// pData is the address of a data table.
void* SendProxy_DataTableToDataTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID );

// pData is the address of a pointer to a data table.
void* SendProxy_DataTablePtrToDataTable( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID );

// Used on player entities - only sends the data to the local player (objectID-1).
void* SendProxy_SendLocalDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID );


// ------------------------------------------------------------------------ //
// Use these functions to setup your data tables.
// ------------------------------------------------------------------------ //
SendProp SendPropFloat(
	const char *pVarName,		// Variable name.
	int offset,					// Offset into container structure.
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,				// Number of bits to use when encoding.
	int flags=0,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_FloatToFloat
	);

SendProp SendPropVector(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	int flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_VectorToVector
	);

SendProp SendPropVectorXY(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	int flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_VectorXYToVectorXY
	);

#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
SendProp SendPropQuaternion(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,					// Number of bits (for each floating-point component) to use when encoding.
	int flags=SPROP_NOSCALE,
	float fLowValue=0.0f,			// For floating point, low and high values.
	float fHighValue=HIGH_DEFAULT,	// High value. If HIGH_DEFAULT, it's (1<<nBits).
	SendVarProxyFn varProxy=SendProxy_QuaternionToQuaternion
	);
#endif

SendProp SendPropAngle(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	int flags=0,
	SendVarProxyFn varProxy=SendProxy_AngleToFloat
	);

SendProp SendPropQAngles(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,
	int nBits=32,
	int flags=0,
	SendVarProxyFn varProxy=SendProxy_QAngles
	);

SendProp SendPropInt(
	const char *pVarName,
	int offset,
	int sizeofVar=SIZEOF_IGNORE,	// Handled by SENDINFO macro.
	int nBits=-1,					// Set to -1 to automatically pick (max) number of bits based on size of element.
	int flags=0,
	SendVarProxyFn varProxy=0
	);

inline SendProp SendPropModelIndex( const char *pVarName, int offset, int sizeofVar=SIZEOF_IGNORE )
{
	return SendPropInt( pVarName, offset, sizeofVar, SP_MODEL_INDEX_BITS, 0 );
}

SendProp SendPropString(
	const char *pVarName,
	int offset,
	int bufferLen,
	int flags=0,
	SendVarProxyFn varProxy=SendProxy_StringToString);

// The data table encoder looks at DVariant::m_pData.
SendProp SendPropDataTable(
	const char *pVarName,
	int offset,
	SendTable *pTable, 
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable
	);

SendProp SendPropArray3(
	const char *pVarName,
	int offset,
	int sizeofVar,
	int elements,
	SendProp pArrayProp,
	SendTableProxyFn varProxy=SendProxy_DataTableToDataTable
	);



// Use the macro to let it automatically generate a table name. You shouldn't 
// ever need to reference the table name. If you want to exclude this array, then
// reference the name of the variable in varTemplate.
SendProp InternalSendPropArray(
	const int elementCount,
	const int elementStride,
	const char *pName,
	ArrayLengthSendProxyFn proxy
	);


// Use this and pass the array name and it will figure out the count and stride automatically.
#define SendPropArray( varTemplate, arrayName )			\
	SendPropVariableLengthArray(						\
		0,												\
		varTemplate,									\
		arrayName )

//
// Use this when you want to send a variable-length array of data but there is no physical array you can point it at.
// You need to provide:
// 1. A proxy function that returns the current length of the array.
// 2. The maximum length the array will ever be.
// 2. A SendProp describing what the elements are comprised of.
// 3. In the SendProp, you'll want to specify a proxy function so you can go grab the data from wherever it is.
// 4. A property name that matches the definition on the client.
//
#define SendPropVirtualArray( arrayLengthSendProxy, maxArrayLength, varTemplate, propertyName )	\
	varTemplate,										\
	InternalSendPropArray(								\
		maxArrayLength,									\
		0,												\
		#propertyName,									\
		arrayLengthSendProxy							\
		)


#define SendPropVariableLengthArray( arrayLengthSendProxy, varTemplate, arrayName )	\
	varTemplate,										\
	InternalSendPropArray(								\
		sizeof(((currentSendDTClass*)0)->arrayName) / PROPSIZEOF(currentSendDTClass, arrayName[0]), \
		PROPSIZEOF(currentSendDTClass, arrayName[0]),	\
		#arrayName,										\
		arrayLengthSendProxy							\
		)

// Use this one to specify the element count and stride manually.
#define SendPropArray2( arrayLengthSendProxy, varTemplate, elementCount, elementStride, arrayName )		\
	varTemplate,																	\
	InternalSendPropArray( elementCount, elementStride, #arrayName, arrayLengthSendProxy )


	

// Use these to create properties that exclude other properties. This is useful if you want to use most of 
// a base class's datatable data, but you want to override some of its variables.
SendProp SendPropExclude(
	const char *pDataTableName,	// Data table name (given to BEGIN_SEND_TABLE and BEGIN_RECV_TABLE).
	const char *pPropName		// Name of the property to exclude.
	);


#endif // DATATABLE_SEND_H
