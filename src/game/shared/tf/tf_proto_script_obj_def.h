//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds WarData
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PROTO_SCRIPT_OBJ_DEF_H
#define TF_PROTO_SCRIPT_OBJ_DEF_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_proto_def_messages.h"
#include <google/protobuf/text_format.h>

#if defined (CLIENT_DLL) || defined (GAME_DLL)
	#include "gc_clientsystem.h"
#endif


// Forward declaration
class IProtoBufScriptObjectFactory;
class IProtoBufScriptObjectDefinition;
class CProtoBufScriptObjectDefinitionManager;

// Global singleton accessor
CProtoBufScriptObjectDefinitionManager* GetProtoScriptObjDefManager();

CUtlString GetProtoDefLocTokenForField( const IProtoBufScriptObjectDefinition* pDef, const CMsgFieldID& fieldID );
bool BMessagesTypesAreTheSame( const google::protobuf::Descriptor* pType1, const google::protobuf::Descriptor* pType2 );
CMsgProtoDefHeader& GetMutableHeaderFromMessage( google::protobuf::Message* pMsg );
const CMsgProtoDefHeader& GetHeaderFromMessage( const google::protobuf::Message* pMsg );
bool BMessagesAreEqual( const google::protobuf::Message* pMsg1, const google::protobuf::Message* pMsg2 );
void SpewProtobufMessage( const google::protobuf::Message* pMsg );
bool UniversalFieldIDLess( const CMsgFieldID &lhs, const CMsgFieldID &rhs );
void GetMessageFromFieldID( const CMsgFieldID& startingFieldID,  const google::protobuf::Message* pStartingMsg,
							CMsgFieldID_CMsgField& outerFieldID, const google::protobuf::Message** pOuterMsg );
void GetMutableMessageFromFieldID( const CMsgFieldID& startingFieldID,  google::protobuf::Message* pStartingMsg,
								   CMsgFieldID_CMsgField& outerFieldID, google::protobuf::Message** pOuterMsg );
void SpewFieldID( const CMsgFieldID& fieldID, const google::protobuf::Message* pMsg );
void SpewUniversalFieldID( const CMsgUniversalFieldID& universalFieldID );
const CMsgFieldID_CMsgField& GetFieldIDFromFieldID( const CMsgFieldID& fieldID );

const google::protobuf::FieldDescriptor* GetMergingKeyField( const google::protobuf::Descriptor* pMsgToSearch );

inline bool operator ==( const CMsgProtoDefID& lhs, const CMsgProtoDefID& rhs )
{
	return lhs.type() == rhs.type() && lhs.defindex() == rhs.defindex();
}
struct ProtoMessageHierarchy_t
{
	const google::protobuf::Message* m_pMsg;
	const ::google::protobuf::FieldDescriptor* m_pFieldDesc;
	CMsgFieldID_CMsgField m_msgField;
};
typedef CUtlVector< ProtoMessageHierarchy_t > ProtoMessageHierarchyVector_t;
void GetMessageHierarchyFromFieldID( const CMsgFieldID& startingFieldID, const google::protobuf::Message* pStartingMsg, ProtoMessageHierarchyVector_t& root );

struct ProtoDefID_t
{
	ProtoDefID_t() {}

	ProtoDefID_t( const CMsgProtoDefID& id )
		: m_ID( id )
	{}

	ProtoDefID_t( ProtoDefTypes eType, uint32 nDefIndex ) { SetDef( eType, nDefIndex ); }

	void SetDef( ProtoDefTypes eType, uint32 nDefIndex )
	{
		m_ID.set_type( eType );
		m_ID.set_defindex( nDefIndex );
	}

	void SetDef( const CMsgProtoDefID& id ) { m_ID = id; }
	void SetDef( const ProtoDefID_t& id ) { m_ID = id.m_ID; }

	const IProtoBufScriptObjectDefinition* operator()() const;
	bool operator ==( const ProtoDefID_t& rhs ) const { return m_ID == rhs.m_ID; }
	bool operator !=( const ProtoDefID_t& rhs ) const { return !( *this == rhs ); }
	bool operator < ( const ProtoDefID_t& rhs ) const 
	{ 
		if ( GetType() < rhs.GetType() )
			return true;

		if ( GetType() == rhs.GetType() )
		{
			return GetDefindex() < rhs.GetDefindex();
		}

		return false;
	}

	ProtoDefTypes	GetType() const		{ return m_ID.type(); }
	uint32			GetDefindex() const { return m_ID.defindex(); }

private:
	CMsgProtoDefID	m_ID;
};


typedef CUtlMap< CMsgFieldID, CMsgUniversalFieldID* > MapFieldLocator_t;

struct VariableValueLookup_t
{
	CMsgVariableDefinition m_msgVarDef;
	CMsgUniversalFieldID m_msgUID;
};


//
// Abstract base class for definitions of objects where the definition is defined
// by a protobuf structure.  This makes the definition, serialization, and editing
// of these definitions much more crisp compared to using KeyValues.
//
class IProtoBufScriptObjectDefinition
{
	friend class CProtoBufScriptObjectDefinitionManager;
	// Editor friends
	friend class IProtoEditorPanel;
	friend class CProtoDefEditorPanel;
	friend class CProtoEditingPanel;
	friend class CMessageEditorPanel;
public:

	IProtoBufScriptObjectDefinition();
	virtual ~IProtoBufScriptObjectDefinition() {}

	// The main accessors to the definition data
	virtual const google::protobuf::Message* GetMsg() const = 0;
	virtual ProtoDefTypes GetDefType() const = 0;
	uint32 GetDefIndex() const;
	const char* GetName() const;
	CMsgProtoDefID GetID() const;
	const CMsgProtoDefHeader& GetHeader() const;
	virtual const char* GetDisplayName() const { return GetName(); }

	virtual const google::protobuf::Message* GetTopLayerMessage() const = 0;

protected:

	void SerializeToBuffer( CUtlBuffer& bufOut ) const;
	bool BParseFromBuffer( CUtlBuffer& bufIn, CUtlVector<CUtlString> *pVecErrors );
	bool BParseFromString( std::string& strIn, CUtlVector<CUtlString> *pVecErrors );


	// Occurs after all objects of this type have been created and their data set.  Useful
	// to setup any pointers or references between objects.
	virtual bool BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors );
	// Occurs right after the data has been set for this object.  Do any data verification here.
	virtual bool BOnDataLoaded( CUtlVector<CUtlString> *pVecErrors) { return true; }


protected:

	// This is the message that has the data specific to THIS definition, without any prefab values.
	virtual google::protobuf::Message* GetMutableTopLayerMsg() = 0;
	// This is the message that has all the data inherited by this definition, without definition specific values.
	virtual google::protobuf::Message* GetMutableMsg() = 0;	

	bool m_bCompiledMessageDirty = false;
	bool m_bSourceMessagesLoaded = false;
	bool m_bCompiledMessageLoaded = false;
	bool m_bUnsavedChanges = false;
	MapFieldLocator_t m_mapFieldValueSources;
};

template< class MsgType, ProtoDefTypes DefType >
class CTypedProtoBufScriptObjectDefinition : public IProtoBufScriptObjectDefinition
{
public:
	CTypedProtoBufScriptObjectDefinition()
	{
		// If you hit this, your Protobuf Message doesn't have a CMsgProtoDefHeader field named "header".  Add one!
		COMPILE_TIME_ASSERT( MsgType::kHeaderFieldNumber );
	}

	const google::protobuf::Message* GetTopLayerMessage() const OVERRIDE
	{ 
		Assert( m_bSourceMessagesLoaded );
		return &m_msgTopLayerData;
	}

	const google::protobuf::Message* GetMsg() const OVERRIDE
	{
		Assert( m_bCompiledMessageLoaded );
		Assert( !m_bCompiledMessageDirty );
		return &m_msgData;
	}

	const MsgType& GetTypedMsg() const
	{
		Assert( m_bCompiledMessageLoaded );
		Assert( !m_bCompiledMessageDirty );
		return m_msgData;
	}

	ProtoDefTypes GetDefType() const OVERRIDE { return DefType; }

	const static ProtoDefTypes k_nDefType = DefType;
protected:

	google::protobuf::Message* GetMutableTopLayerMsg() OVERRIDE { return &m_msgTopLayerData; }	
	google::protobuf::Message* GetMutableMsg() OVERRIDE { return &m_msgData; }

	MsgType m_msgTopLayerData;	// The fields set explicitly by THIS definition.
	MsgType m_msgData;			// The composite msg of prefabs + this defintiion
};

#ifdef CLIENT_DLL

class CProtoDefReferenceFinder : public CAutoGameSystemPerFrame
{
public:
	CProtoDefReferenceFinder();
	virtual ~CProtoDefReferenceFinder() {}

	void SetDef( const ProtoDefID_t& defID );

	bool BDone() const { return m_bDone; }
	ProtoDefID_t GetDefID() const { return m_defID; }
	const CUtlVector< ProtoDefID_t >& GetReferences() const { return m_vecReferences; }
	const CUtlVector< ProtoDefID_t >& GetDerivedDefs() const { return m_vecPrefabReferences; }
protected:
	void SearchForReferencesForDef( const ProtoDefID_t& defID );

	virtual void OnDone() {}
	virtual void OnRefRound( const ProtoDefID_t& defID ) {}
	virtual void OnDerivedFound( const ProtoDefID_t& defID ) {}

	virtual void Update( float frametime ) OVERRIDE;

	int m_nCurrentReferenceSearchType = 0;
	int m_nCurrentReferenceSearchDefindex = 0;
	int m_nReferencesToThis = 0;
	bool m_bSearchingOriginalDef = true;
	CUtlVector< ProtoDefID_t > m_vecReferences;

private:
	CUtlVector< ProtoDefID_t > m_vecPrefabReferences;
	int m_nPrefabIterator = 0;
	ProtoDefID_t m_defID;
	bool m_bDone = true;
	static double m_sflTimeTaken;
};

#endif

#ifdef CLIENT_DLL
typedef bool protoDefPostSolveFunc( google::protobuf::Message* pMsgSource, const google::protobuf::Message* pMsgCompiled );

struct ProtoDefTypeDesc_t
{
	const char* m_pszDisplayName;
	bool m_bSourceDirty;
	protoDefPostSolveFunc* m_pfnPostSolve;
};

extern ProtoDefTypeDesc_t g_ProtoDefTypeDescs[];

class CConversionFuncRegister
{
public:
	CConversionFuncRegister( protoDefPostSolveFunc func, ProtoDefTypes eType ) { g_ProtoDefTypeDescs[ eType ].m_pfnPostSolve = func; }
};
#define REGISTER_PROTODEF_TYPE_POST_SOLVE_FUNCTION( func, type ) CConversionFuncRegister g_##type##ConvFunc( func, type );
#endif

//
// Class that manages the creation, deletion, and storage of all Proto Def objects.
// Globally accessed via GetProtoScriptObjDefManager(), which is commonly used to lookup
// a definition of something.
//
typedef CUtlMap< uint32, IProtoBufScriptObjectDefinition* > DefinitionMap_t;
class CProtoBufScriptObjectDefinitionManager
	: public CAutoGameSystemPerFrame
{
public:
	CProtoBufScriptObjectDefinitionManager();

	virtual bool Init() OVERRIDE;
	virtual void PostInit();

	bool BInitDefinitions();
	bool BDefinitionsLoaded() const { return m_bDefinitionsLoaded && m_bDefinitionsPostDataLoadedCalled; }
	bool BPostDefinitionsLoaded();
	bool BPostDefinitionsCalled() const { return m_bDefinitionsPostDataLoadedCalled; }

	template< typename T >
	const T* GetTypedDefinition( const ProtoDefID_t& defID ) const
	{
		return assert_cast< const T* >( GetDefinition( defID ) );
	}

	template< typename T >
	const T* GetTypedDefinition( uint32 nDefIndex ) const
	{
		return assert_cast< const T* >( GetDefinition( ProtoDefID_t( T::k_nDefType, nDefIndex ) ) );
	}

	const IProtoBufScriptObjectDefinition* GetDefinition( const ProtoDefID_t& defID ) const;
	IProtoBufScriptObjectDefinition* GetMutableDefinition( const ProtoDefID_t& defID ) const;
	const DefinitionMap_t& GetDefinitionMapForType( ProtoDefTypes eType ) const { return m_arDefinitionsMaps[ eType ]; }

	void RegisterTypeFactory( const IProtoBufScriptObjectFactory* pFactory );


private:

	void InsertObjectIntoMap( IProtoBufScriptObjectDefinition* pObject );

	DefinitionMap_t m_arDefinitionsMaps[ ProtoDefTypes_ARRAYSIZE ];
	CUtlMap< ProtoDefTypes, const IProtoBufScriptObjectFactory* > m_mapFactories;
	CUtlMap< ProtoDefID_t, uint32 > m_mapCopyCount;

	bool m_bDefinitionsLoaded = false;
	bool m_bDefinitionsPostDataLoadedCalled = false;
	bool m_bSourceFilesLoaded = false;
};

//
// Utility class for registering a factory for creating protobuf script objects
//
class IProtoBufScriptObjectFactory
{
public:
	IProtoBufScriptObjectFactory( const char* pszName, ProtoDefTypes eType ) 
		: m_strName( pszName )
		, m_eType( eType )
	{
		GetProtoScriptObjDefManager()->RegisterTypeFactory( this );
	}
	virtual IProtoBufScriptObjectDefinition* CreateNewObject() const = 0;
	ProtoDefTypes GetType() const { return m_eType; }
	const char* GetName() const { return m_strName; }
private:
	CUtlString m_strName;
	ProtoDefTypes m_eType;
};

template< class Type >
class CProtoBufScriptObjectFactory : public IProtoBufScriptObjectFactory
{
public:
	CProtoBufScriptObjectFactory( const char* pszName, ProtoDefTypes eType ) 
		: IProtoBufScriptObjectFactory( pszName, eType )
	{}

	virtual IProtoBufScriptObjectDefinition* CreateNewObject() const OVERRIDE
	{
		IProtoBufScriptObjectDefinition* pNewDef = new Type();
		return pNewDef;
	}
};

#define REGISTER_PROTO_DEF_FACTORY( classtype, deftype ) CProtoBufScriptObjectFactory< classtype > g_##classtype##Factory( #deftype, deftype );

//
// Utility function for iterating over all fields and sub fields of a message.
//
template< class F >
void ForEachProtoField( google::protobuf::Message* pMsg, F &&operation, bool bRecurse, bool bShowUnset, CMsgFieldID fieldID = CMsgFieldID::default_instance() )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	CMsgFieldID_CMsgField& currentField = *fieldID.add_field();

	for ( int i = 0; i < pMsg->GetDescriptor()->field_count(); ++i )
	{
		const google::protobuf::FieldDescriptor* pField = pMsg->GetDescriptor()->field( i );
		currentField.set_field_number( pField->number() );
		currentField.clear_repeated_index();

		// Handle Oneofs
		const ::google::protobuf::OneofDescriptor* pOneOfDesc = pField->containing_oneof();
		if ( pOneOfDesc )
		{
			bool bHasThisFieldInOneOf = false;
			for ( int j = 0; j < pOneOfDesc->field_count(); ++j )
			{
				const ::google::protobuf::FieldDescriptor* pOneOfField = pOneOfDesc->field( j );
				if ( pMsg->GetReflection()->HasField( *pMsg, pOneOfField ) && pOneOfField == pField )
				{
					bHasThisFieldInOneOf = true;
					break;
				}
			}

			if ( !bHasThisFieldInOneOf )
				continue;
		}


		if ( pField->is_repeated() )
		{
			for ( int j = 0; j < pMsg->GetReflection()->FieldSize( *pMsg, pField ); ++j )
			{
				currentField.set_repeated_index( j );

				// We only care about messages for recursion
				if ( bRecurse && pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
				{
					google::protobuf::Message* pSubMsg = pMsg->GetReflection()->MutableRepeatedMessage( pMsg, pField, j );
					ForEachProtoField( pSubMsg, operation, bRecurse, bShowUnset, fieldID );
				}

				// Has to happen AFTER the logic above because we might be deleting the repeated field index
				if ( !operation( pMsg, pField, fieldID ) )
				{
					return;
				}
			}
		}
		else if ( bShowUnset || pMsg->GetReflection()->HasField( *pMsg, pField ) )
		{
			if ( !operation( pMsg, pField, fieldID ) )
			{
				return;
			}

			// We only care about messages for recursion
			if ( bRecurse && pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
			{
				ForEachProtoField( pMsg->GetReflection()->MutableMessage( pMsg, pField ), operation, bRecurse, bShowUnset, fieldID );
			}
		}
	}
}

template< class F >
void ForEachConstProtoField( const google::protobuf::Message* pMsg, F &&operation, bool bRecurse, bool bShowUnset, CMsgFieldID fieldID = CMsgFieldID::default_instance() )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	CMsgFieldID_CMsgField& currentField = *fieldID.add_field();

	for ( int i = 0; i < pMsg->GetDescriptor()->field_count(); ++i )
	{
		const google::protobuf::FieldDescriptor* pField = pMsg->GetDescriptor()->field( i );
		currentField.set_field_number( pField->number() );
		currentField.clear_repeated_index();

		// Handle Oneofs
		const ::google::protobuf::OneofDescriptor* pOneOfDesc = pField->containing_oneof();
		if ( pOneOfDesc )
		{
			bool bHasThisFieldInOneOf = false;
			for ( int j = 0; j < pOneOfDesc->field_count(); ++j )
			{
				const ::google::protobuf::FieldDescriptor* pOneOfField = pOneOfDesc->field( j );
				if ( pMsg->GetReflection()->HasField( *pMsg, pOneOfField ) && pOneOfField == pField )
				{
					bHasThisFieldInOneOf = true;
					break;
				}
			}

			if ( !bHasThisFieldInOneOf )
				continue;
		}


		if ( pField->is_repeated() )
		{
			for ( int j = 0; j < pMsg->GetReflection()->FieldSize( *pMsg, pField ); ++j )
			{
				currentField.set_repeated_index( j );

				// We only care about messages for recursion
				if ( bRecurse && pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
				{
					const google::protobuf::Message* pSubMsg = &pMsg->GetReflection()->GetRepeatedMessage( *pMsg, pField, j );
					ForEachConstProtoField( pSubMsg, operation, bRecurse, bShowUnset, fieldID );
				}

				// Has to happen AFTER the logic above because we might be deleting the repeated field index
				if ( !operation( pMsg, pField, fieldID ) )
				{
					return;
				}
			}
		}
		else if ( bShowUnset || pMsg->GetReflection()->HasField( *pMsg, pField ) )
		{
			if ( !operation( pMsg, pField, fieldID ) )
			{
				return;
			}

			// We only care about messages for recursion
			if ( bRecurse && pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
			{
				ForEachConstProtoField( &pMsg->GetReflection()->GetMessage( *pMsg, pField ), operation, bRecurse, bShowUnset, fieldID );
			}
		}
	}
}

const ::google::protobuf::Message* GetSubMessage( const ::google::protobuf::Message* pMsg,
	const ::google::protobuf::FieldDescriptor *pField,
	int nIndex );

::google::protobuf::Message* GetMutableSubMessage( ::google::protobuf::Message* pMsg,
	const ::google::protobuf::FieldDescriptor *pField,
	int nIndex );

::google::protobuf::Message* AddSubMessage( ::google::protobuf::Message* pMsg, const ::google::protobuf::FieldDescriptor *pField );

#endif // TF_PROTO_SCRIPT_OBJ_DEF_H
