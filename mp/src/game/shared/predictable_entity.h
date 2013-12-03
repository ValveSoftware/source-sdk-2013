//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PREDICTABLE_ENTITY_H
#define PREDICTABLE_ENTITY_H
#ifdef _WIN32
#pragma once
#endif

// For introspection
#include "tier0/platform.h"
#include "predictioncopy.h"
#include "shared_classnames.h"

#ifndef NO_ENTITY_PREDICTION
#define UsePrediction() 1
#else
#define UsePrediction() 0
#endif

// CLIENT DLL includes
#if defined( CLIENT_DLL )

#include "iclassmap.h"
#include "recvproxy.h"

class SendTable;

// Game DLL includes
#else

#include "sendproxy.h"

#endif  // !CLIENT_DLL

#if defined( CLIENT_DLL )

#define DECLARE_NETWORKCLASS()											\
		DECLARE_CLIENTCLASS()

#define DECLARE_NETWORKCLASS_NOBASE()									\
		DECLARE_CLIENTCLASS_NOBASE()							

#else

#define DECLARE_NETWORKCLASS()											\
		DECLARE_SERVERCLASS()

#define DECLARE_NETWORKCLASS_NOBASE()									\
		DECLARE_SERVERCLASS_NOBASE()	

#endif

#if defined( CLIENT_DLL )

#ifndef NO_ENTITY_PREDICTION
#define DECLARE_PREDICTABLE()											\
	public:																\
		static typedescription_t m_PredDesc[];							\
		static datamap_t m_PredMap;										\
		virtual datamap_t *GetPredDescMap( void );						\
		template <typename T> friend datamap_t *PredMapInit(T *)
#else
#define DECLARE_PREDICTABLE()	template <typename T> friend datamap_t *PredMapInit(T *)
#endif

#ifndef NO_ENTITY_PREDICTION
#define BEGIN_PREDICTION_DATA( className ) \
	datamap_t className::m_PredMap = { 0, 0, #className, &BaseClass::m_PredMap }; \
	datamap_t *className::GetPredDescMap( void ) { return &m_PredMap; } \
	BEGIN_PREDICTION_DATA_GUTS( className )

#define BEGIN_PREDICTION_DATA_NO_BASE( className ) \
	datamap_t className::m_PredMap = { 0, 0, #className, NULL }; \
	datamap_t *className::GetPredDescMap( void ) { return &m_PredMap; } \
	BEGIN_PREDICTION_DATA_GUTS( className )

#define BEGIN_PREDICTION_DATA_GUTS( className ) \
	template <typename T> datamap_t *PredMapInit(T *); \
	template <> datamap_t *PredMapInit<className>( className * ); \
	namespace className##_PredDataDescInit \
	{ \
		datamap_t *g_PredMapHolder = PredMapInit( (className *)NULL ); /* This can/will be used for some clean up duties later */ \
	} \
	\
	template <> datamap_t *PredMapInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static typedescription_t predDesc[] = \
		{ \
		{ FIELD_VOID,0, {0,0},0,0,0,0,0,0}, /* so you can define "empty" tables */

#define END_PREDICTION_DATA() \
		}; \
		\
		if ( sizeof( predDesc ) > sizeof( predDesc[0] ) ) \
		{ \
			classNameTypedef::m_PredMap.dataNumFields = ARRAYSIZE( predDesc ) - 1; \
			classNameTypedef::m_PredMap.dataDesc 	  = &predDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_PredMap.dataNumFields = 1; \
			classNameTypedef::m_PredMap.dataDesc 	  = predDesc; \
		} \
		return &classNameTypedef::m_PredMap; \
	}
#else
#define BEGIN_PREDICTION_DATA( className ) \
	template <> inline datamap_t *PredMapInit<className>( className * ) \
	{ \
		if ( 0 ) \
		{ \
			typedef className classNameTypedef; \
			typedescription_t predDesc[] = \
			{ \
				{ FIELD_VOID,0, {0,0},0,0,0,0,0,0},

#define BEGIN_PREDICTION_DATA_NO_BASE( className ) BEGIN_PREDICTION_DATA( className )

#define END_PREDICTION_DATA() \
			}; \
			predDesc[0].flags = 0; /* avoid compiler warning of unused data */ \
		} \
	}
#endif

#else

	// nothing, only client has a prediction system
	#define DECLARE_PREDICTABLE()	
	#define BEGIN_PREDICTION_DATA( className ) 
	#define END_PREDICTION_DATA() 

#endif

#if defined( CLIENT_DLL )

// On the client .dll this creates a mapping between a classname and
//  a client side class.  Probably could be templatized at some point.

#define LINK_ENTITY_TO_CLASS( localName, className )						\
	static C_BaseEntity *C##className##Factory( void )						\
	{																		\
		return static_cast< C_BaseEntity * >( new className );				\
	};																		\
	class C##localName##Foo													\
	{																		\
	public:																	\
		C##localName##Foo( void )											\
		{																	\
			GetClassMap().Add( #localName, #className, sizeof( className ),	\
				&C##className##Factory );									\
		}																	\
	};																		\
	static C##localName##Foo g_C##localName##Foo;

#define BEGIN_NETWORK_TABLE( className, tableName ) BEGIN_RECV_TABLE( className, tableName )
#define BEGIN_NETWORK_TABLE_NOBASE( className, tableName ) BEGIN_RECV_TABLE_NOBASE( className, tableName )

#define END_NETWORK_TABLE	END_RECV_TABLE

#define IMPLEMENT_NETWORKCLASS_ALIASED(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS( C_##className, dataTable, C##className )
#define IMPLEMENT_NETWORKCLASS(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS(className, dataTable, className)
#define IMPLEMENT_NETWORKCLASS_DT(className, dataTable)			\
	IMPLEMENT_CLIENTCLASS_DT(className, dataTable, className)

#else

#define BEGIN_NETWORK_TABLE( className, tableName ) BEGIN_SEND_TABLE( className, tableName )
#define BEGIN_NETWORK_TABLE_NOBASE( className, tableName ) BEGIN_SEND_TABLE_NOBASE( className, tableName )

#define END_NETWORK_TABLE	END_SEND_TABLE

#define IMPLEMENT_NETWORKCLASS_ALIASED(className, dataTable)			\
	IMPLEMENT_SERVERCLASS( C##className, dataTable )
#define IMPLEMENT_NETWORKCLASS(className, dataTable)			\
	IMPLEMENT_SERVERCLASS(className, dataTable)
#define IMPLEMENT_NETWORKCLASS_DT(className, dataTable)			\
	IMPLEMENT_SERVERCLASS_ST(className, dataTable)

#endif																	

// Interface used by client and server to track predictable entities
abstract_class IPredictableList
{
public:
	// Get predictables by index
	virtual CBaseEntity		*GetPredictable( int slot ) = 0;
	// Get count of predictables
	virtual int				GetPredictableCount( void ) = 0;
};

// Expose interface to rest of .dll
extern IPredictableList *predictables;

#endif // PREDICTABLE_ENTITY_H
