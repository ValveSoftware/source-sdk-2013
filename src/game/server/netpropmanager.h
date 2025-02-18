//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Gets and sets SendTable/DataMap networked properties and caches results.
//
// Code contributions by and used with the permission of L4D2 modders:
// Neil Rao (neilrao42@gmail.com)
// Raymond Nondorf (rayman1103@aol.com)
//==========================================================================================//

#ifndef NETPROPMANAGER_H
#define NETPROPMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "dt_send.h"
#include "datamap.h"

// Gets and sets SendTable/DataMap netprops and caches results
class CNetPropManager
{
public:
	~CNetPropManager();

private:

	// This class manages both SendProps and DataMap props,
	// so we will need an enum to hold type info
	enum NetPropType
	{
		Type_Int = 0,
		Type_Float,
		Type_Vector,
		Type_VectorXY,
		Type_String,
		Type_Array,
		Type_DataTable,

	#ifdef SUPPORTS_INT64
		Type_Int64,
	#endif

		Type_String_t,
		Type_Bool,
		Type_EHandle,
		Type_ClassPtr,
		Type_Std_String,
		Type_InvalidOrMax
	};

	// Holds prop information that is valid throughout the life of the engine
	struct PropInfo_t
	{
	public:
		int m_nOffset;			/**< A SendProp holds the offset from the "this" pointer of an entity */
		int m_nBitCount;		/**< Usually the number of bits to transmit. */
		int m_nTransFlags;		/**< Entity transmission flags */
		NetPropType m_eType;	/**< The type of prop this offset belongs to */
		int m_nElements;		/**< The number of elements */
		bool m_bIsSendProp;		/**< Is this a SendProp (if false, it is a DataMap) */
		bool m_IsPropValid;		/**< Is the prop data in the struct valid? */
		int m_nPropLen;			/**< The length of the prop (applies to strings) */
		int m_nProps;			/**< The number of props in an array */
	};

	// Searches the specified SendTable and returns the SendProp or NULL if it DNE
	inline SendProp *SearchSendTable( SendTable *pSendTable, const char *pstrProperty ) const;

	// Searches the data map and returns the offset
	inline typedescription_t *SearchDataMap( datamap_t *pMap, const char *pstrProperty ) const;

	// Searches a ServerClass's SendTable and datamap and returns pertinent prop info
	inline PropInfo_t GetEntityPropInfo( CBaseEntity *pBaseEntity, const char *pstrProperty, int element );

	// Gets the value of a SendProp and stores it in a table
	inline void StoreSendPropValue( SendProp *pSendProp, CBaseEntity *pBaseEntity, int iOffset, int iElement, HSCRIPT hTable );

	// Gets the value of a DataMap prop and stores it in a table
	inline void StoreDataPropValue( typedescription_t *pTypeDesc, CBaseEntity *pBaseEntity, int iOffset, int iElement, HSCRIPT hTable );

	// Iterates through the SendTable and stores prop names in a table
	inline void CollectNestedSendProps( SendTable *pSendTable, CBaseEntity *pBaseEntity, int iOffset, HSCRIPT hTable );

	// Iterates through the DataMap and stores prop names in a table
	inline void CollectNestedDataMaps( datamap_t *pMap, CBaseEntity *pBaseEntity, int iOffset, HSCRIPT hTable );


private:

	// Prop/offset dictionary
	typedef CUtlDict< PropInfo_t, int > PropInfoDict_t;
	CUtlDict< PropInfoDict_t*, int > m_PropCache;

public:

	// Gets an integer netprop value for the provided entity
	int GetPropInt( HSCRIPT hEnt, const char *pstrProperty );

	// Gets an integer netprop array value for the provided entity
	int GetPropIntArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets an integer netprop value for the provided entity
	void SetPropInt( HSCRIPT hEnt, const char *pstrProperty, int value );

	// Sets an integer netprop array value for the provided entity
	void SetPropIntArray( HSCRIPT hEnt, const char *pstrProperty, int value, int element );

	// Gets a floating point netprop value for the provided entity
	float GetPropFloat( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a floating point netprop array value for the provided entity
	float GetPropFloatArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets a floating point netprop value for the provided entity
	void SetPropFloat( HSCRIPT hEnt, const char *pstrProperty, float value );

	// Sets a floating point netprop array value for the provided entity
	void SetPropFloatArray( HSCRIPT hEnt, const char *pstrProperty, float value, int element );

	// Gets an entity index pointed to by the netprop value for the provided index
	// @TODO Needs more unit tests
	HSCRIPT GetPropEntity( HSCRIPT hEnt, const char *pstrProperty );

	// Gets an entity pointed to by the netprop array value for the provided entity
	// @TODO Needs more unit tests
	HSCRIPT GetPropEntityArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets an entity pointed to by the netprop value for the provided entity
	// @TODO Needs more unit tests
	void SetPropEntity( HSCRIPT hEnt, const char *pstrProperty, HSCRIPT hPropEnt );

	// Sets an entity pointed to by the netprop array value for the provided entity
	// @TODO Needs more unit tests
	void SetPropEntityArray( HSCRIPT hEnt, const char *pstrProperty, HSCRIPT hPropEnt, int element );

	// Gets a Vector netprop value for the provided entity
	const Vector& GetPropVector( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a Vector netprop array value for the provided entity
	const Vector& GetPropVectorArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets a Vector netprop value for the provided entity
	void SetPropVector( HSCRIPT hEnt, const char *pstrProperty, Vector value );

	// Sets a Vector netprop array value for the provided entity
	void SetPropVectorArray( HSCRIPT hEnt, const char *pstrProperty, Vector value, int element );

	// Gets a string netprop value for the provided entity
	const char *GetPropString( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a string netprop array value for the provided entity
	const char *GetPropStringArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets a string netprop value for the provided entity
	void SetPropString( HSCRIPT hEnt, const char *pstrProperty, const char *value );

	// Sets a string netprop array value for the provided entity
	void SetPropStringArray( HSCRIPT hEnt, const char *pstrProperty, const char *value, int element );

	// Gets the size of a netprop array value for the provided entity
	int GetPropArraySize( HSCRIPT hEnt, const char *pstrProperty );

	// Returns true if the netprop exists for the provided entity
	bool HasProp( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a string of the type of netprop value for the provided entity
	const char *GetPropType( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a boolean netprop value for the provided entity
	bool GetPropBool( HSCRIPT hEnt, const char *pstrProperty );

	// Gets a boolean netprop array value for the provided entity
	bool GetPropBoolArray( HSCRIPT hEnt, const char *pstrProperty, int element );

	// Sets a boolean netprop value for the provided entity
	void SetPropBool( HSCRIPT hEnt, const char *pstrProperty, bool value );

	// Sets a boolean netprop array value for the provided entity
	void SetPropBoolArray( HSCRIPT hEnt, const char *pstrProperty, bool value, int element );

	// Fills in a passed table with all SendProps or DataMaps for the provided entity
	void GetTable( HSCRIPT hEnt, int iPropType, HSCRIPT hTable );

	// Fills in a passed table with property info for the provided entity
	bool GetPropInfo( HSCRIPT hEnt, const char *pstrProperty, int element, HSCRIPT hTable );
};


#endif // NETPROPMANAGER_H
