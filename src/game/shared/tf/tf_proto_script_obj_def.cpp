//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "tier1/util_misc.h"
#include "tf_proto_script_obj_def.h"
#include "tf_quest_map_node.h"
#include <google/protobuf/text_format.h>
#include "filesystem.h"
#include "tier2/p4helpers.h"
#include "tier2/fileutils.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include "schemainitutils.h"

#ifdef CLIENT_DLL
	#include "icommandline.h"
	#include <vgui/ILocalize.h>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool CheckValveSignature( const void *data, uint32 nDataSize, const void *signature, uint32 nSignatureSize );

const char* g_pszProtoPath = "scripts/protodefs/unencrypted/";
const char* g_pszProtoDefFile = "scripts/protodefs/proto_defs.vpd";
const char* g_pszProtoLocFileName = "resource/tf_proto_obj_defs_english.txt";

#ifdef CLIENT_DLL
ProtoDefTypeDesc_t g_ProtoDefTypeDescs[] = { { "Quest Map Node",		false, NULL },
											 { "Quest Map",				false, NULL },
											 { "Quest Theme",			false, NULL },
											 { "Quest Map Region",		false, NULL },
											 { "Quest",					false, NULL },
											 { "Quest Objective",		false, NULL },
											 { "Paintkit Variable",		false, NULL },
											 { "Paintkit Operation",	false, NULL },
											 { "Paintkit Item Def",		false, NULL },
											 { "Paintkit Def",			false, NULL },
											 { "Header",				false, NULL },
											 { "Quest Map Store Item",	false, NULL },
											 { "Quest Map Star Type",	false, NULL } };

COMPILE_TIME_ASSERT( ARRAYSIZE( g_ProtoDefTypeDescs ) == ProtoDefTypes_ARRAYSIZE );
#endif

const IProtoBufScriptObjectDefinition* ProtoDefID_t::operator()() const
{
	return GetProtoScriptObjDefManager()->GetDefinition( *this );
}

bool BMessagesTypesAreTheSame( const google::protobuf::Descriptor* pType1, const google::protobuf::Descriptor* pType2 )
{
	return pType1->full_name() == pType2->full_name();
}

CUtlString GetProtoDefLocTokenForField( const IProtoBufScriptObjectDefinition* pDef, const CMsgFieldID& fieldID )
{
	// We're going to craft a localization token that is unique for this field, in this definition, among all types.
	// To do that we're going to glue a few things together.
	// - To achieve uniqueness in the message use the field ID as a string
	// - For uniqueness within a type use the defindex
	// - For uniqueness among the types, use the type index

	CUtlString strToken;
	// Type
	strToken.Append( CFmtStr( "%d_", pDef->GetDefType() ) );
	// Defindex
	strToken.Append( CFmtStr( "%d_", pDef->GetDefIndex() ) );
	// Field ID
	strToken.Append( fieldID.ShortDebugString().c_str() );

	return strToken;
}

void SaveLocalizationValueToFile( const char* pszToken, wchar_t* pwszValue, bool bDeleted )
{
#ifdef CLIENT_DLL
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	char szPathAndFileName[MAX_PATH];
	if ( !GenerateFullPath( g_pszProtoLocFileName, "MOD", szPathAndFileName, ARRAYSIZE( szPathAndFileName ) ) )
	{
		Warning( "Failed to GenerateFullPath %s\n", g_pszProtoLocFileName );
		return;
	}

	// The "deleted" file doesn't exist, but if we put the token into that file's map, then save out
	// to tf_proto_obj_defs_english, the token and value won't be saved out.
	const char* pszFile = bDeleted ? "deleted" : szPathAndFileName;
	if ( pszToken[0] == '#' )
	{
		pszToken++;
	}
	g_pVGuiLocalize->AddString( pszToken, pwszValue, pszFile );

	// Get the name of the file and p4 check it out
	char szCorrectCaseFilePath[MAX_PATH];
	g_pFullFileSystem->GetCaseCorrectFullPath( szPathAndFileName, szCorrectCaseFilePath );
	CP4AutoEditFile a( szCorrectCaseFilePath );

	g_pVGuiLocalize->SaveToFile( szPathAndFileName );
#endif
}


const CMsgProtoDefHeader& GetHeaderFromMessage( const google::protobuf::Message* pMsg )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	const google::protobuf::FieldDescriptor* pHeaderField = pMsg->GetDescriptor()->FindFieldByName( "header" );
	const google::protobuf::Message& header = pMsg->GetReflection()->GetMessage( *pMsg, pHeaderField );
	return *(CMsgProtoDefHeader*)(&header);
}

CMsgProtoDefHeader& GetMutableHeaderFromMessage( google::protobuf::Message* pMsg )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	const google::protobuf::FieldDescriptor* pHeaderField = pMsg->GetDescriptor()->FindFieldByName( "header" );
	google::protobuf::Message* pHeader = pMsg->GetReflection()->MutableMessage( pMsg, pHeaderField );
	return *(CMsgProtoDefHeader*)pHeader;
}

bool BMessagesAreEqual( const google::protobuf::Message* pMsg1, const google::protobuf::Message* pMsg2 )
{
	// Quick check.  Need to be the same type!
	if ( pMsg1->GetDescriptor() != pMsg2->GetDescriptor() )
		return false;

	if ( pMsg1->IsInitialized() && pMsg2->IsInitialized() )
	{
		return pMsg1->SerializeAsString() == pMsg2->SerializeAsString();
	}
	else
	{
		return pMsg1->SerializePartialAsString() == pMsg2->SerializePartialAsString();
	}
}

void SpewProtobufMessage( const google::protobuf::Message* pMsg )
{
	std::string strMsg( pMsg->DebugString() );
	for ( size_t i=0; i < strMsg.size(); ++i )
	{
		DevMsg( "%c", strMsg[ i ] );
	}
}

bool UniversalFieldIDLess( const CMsgFieldID &lhs, const CMsgFieldID &rhs )
{
	int nMaxDepth = Min( lhs.field_size(), rhs.field_size() );
	for( int i=0; i < nMaxDepth; ++i )
	{
		const CMsgFieldID_CMsgField& lhsField = lhs.field( i );
		const CMsgFieldID_CMsgField& rhsField = rhs.field( i );

		if ( lhsField.field_number() < rhsField.field_number() )
			return true;

		if ( lhsField.field_number() > rhsField.field_number() )
			return false;

		if ( lhsField.repeated_index() < rhsField.repeated_index() )
			return true;

		if ( lhsField.repeated_index() > rhsField.repeated_index() )
			return false;

		// Same field, same repeated index.  If we have multiple fields to
		// dig into, it means we're going into nested messages.  Check if
		// the subsequent fields differ...
	}

	return lhs.field_size() < rhs.field_size();
}

void GetMessageFromFieldID( const CMsgFieldID& startingFieldID,  const google::protobuf::Message* pStartingMsg,
							CMsgFieldID_CMsgField& outerFieldID, const google::protobuf::Message** pOuterMsg )
{
	// Very possible we're not going deeper
	(*pOuterMsg)  = pStartingMsg;

	for( int i=0; i < startingFieldID.field_size(); ++i )
	{
		outerFieldID = startingFieldID.field( i );

		auto pFieldDesc = (*pOuterMsg)->GetDescriptor()->FindFieldByNumber( outerFieldID.field_number() );

		if ( pFieldDesc->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
		{
			Assert( i != startingFieldID.field_size() );

			// Keep going!
			(*pOuterMsg) = pFieldDesc->is_repeated() ? &(*pOuterMsg)->GetReflection()->GetRepeatedMessage( *(*pOuterMsg), pFieldDesc, outerFieldID.repeated_index() )
													 : &(*pOuterMsg)->GetReflection()->GetMessage( *(*pOuterMsg), pFieldDesc );
		}
	}
}

void SpewFieldID( const CMsgFieldID& fieldID, const google::protobuf::Message* pMsg )
{
	for( int i=0; i < fieldID.field_size(); ++i )
	{
		const CMsgFieldID_CMsgField& field = fieldID.field( i );

		auto pFieldDesc = pMsg->GetDescriptor()->FindFieldByNumber( field.field_number() );

		// Spew the name
		DevMsg( " %d:%s", pFieldDesc->number(), pFieldDesc->name().c_str() );

		if ( pFieldDesc->is_repeated() )
		{
			DevMsg( ":%d", field.repeated_index() );
		}

		if ( pFieldDesc->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
		{
			// Keep going!
			auto pNewMsg = GetSubMessage( pMsg, pFieldDesc, field.repeated_index() );
			if ( !pNewMsg )
			{
				Assert( false );
				pMsg = NULL;
				return;
			}
			pMsg = pNewMsg;
		}
		else
		{
			std::string strFieldVal;
			google::protobuf::TextFormat::PrintFieldValueToString( *pMsg, pFieldDesc, pFieldDesc->is_repeated() ? field.repeated_index() : -1, &strFieldVal );
			DevMsg( " = %s", strFieldVal.c_str() );
		}
	}
}

void GetMutableMessageFromFieldID( const CMsgFieldID& startingFieldID,  google::protobuf::Message* pStartingMsg,
								   CMsgFieldID_CMsgField& outerFieldID, google::protobuf::Message** pOuterMsg )
{
	// Very possible we're not going deeper
	(*pOuterMsg)  = pStartingMsg;

	for( int i=0; i < startingFieldID.field_size(); ++i )
	{
		outerFieldID = startingFieldID.field( i );

		auto pFieldDesc = (*pOuterMsg)->GetDescriptor()->FindFieldByNumber( outerFieldID.field_number() );

		if ( pFieldDesc->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE && i < startingFieldID.field_size() - 1 )
		{
			Assert( i != startingFieldID.field_size() );

			if ( pFieldDesc->is_repeated() && (uint32)(*pOuterMsg)->GetReflection()->FieldSize( *(*pOuterMsg ), pFieldDesc ) <= outerFieldID.repeated_index() )
			{
				SpewFieldID( startingFieldID, pStartingMsg );
				Assert( false );
				return;
			}

			// Keep going!
			(*pOuterMsg) = pFieldDesc->is_repeated() ? (*pOuterMsg)->GetReflection()->MutableRepeatedMessage( (*pOuterMsg), pFieldDesc, outerFieldID.repeated_index() )
													 : (*pOuterMsg)->GetReflection()->MutableMessage( (*pOuterMsg), pFieldDesc );
		}
	}
}

void SpewUniversalFieldID( const CMsgUniversalFieldID& universalFieldID )
{
	auto pDef = GetProtoScriptObjDefManager()->GetDefinition( universalFieldID.defining_obj_id() );
	DevMsg( "\"%s\" (%d): ", pDef->GetName()
					   , pDef->GetDefIndex() );
	SpewFieldID( universalFieldID.field_id(), pDef->GetTopLayerMessage() );
}

const CMsgFieldID_CMsgField& GetFieldIDFromFieldID( const CMsgFieldID& fieldID )
{
	return fieldID.field( fieldID.field_size() - 1 );
}


const google::protobuf::FieldDescriptor* GetMergingKeyField( const google::protobuf::Descriptor* pDesc )
{
	for ( int i=0; i < pDesc->field_count(); ++i )
	{
		const google::protobuf::FieldDescriptor* pField = pDesc->field( i );
		if ( pField->options().GetExtension( merging_key_field ) )
		{
			return pField;
		}
	}

	return NULL;
}

void GetMessageHierarchyFromFieldID( const CMsgFieldID& startingFieldID, const google::protobuf::Message* pStartingMsg, ProtoMessageHierarchyVector_t& root )
{
	root.RemoveAll();

	const google::protobuf::Message *pNextMsg = pStartingMsg;
	for ( int i = 0; i < startingFieldID.field_size(); ++i )
	{
		ProtoMessageHierarchy_t *pMsgNode =  root.AddToTailGetPtr();
		pMsgNode->m_msgField = startingFieldID.field( i );
		pMsgNode->m_pFieldDesc = pNextMsg->GetDescriptor()->FindFieldByNumber( pMsgNode->m_msgField.field_number() );

		if ( pMsgNode->m_pFieldDesc->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
		{
			Assert( i != startingFieldID.field_size() );

			// Keep going!
			pNextMsg = pMsgNode->m_pFieldDesc->is_repeated() ? &pNextMsg->GetReflection()->GetRepeatedMessage( *pNextMsg, pMsgNode->m_pFieldDesc, pMsgNode->m_msgField.repeated_index() )
				: &pNextMsg->GetReflection()->GetMessage( *pNextMsg, pMsgNode->m_pFieldDesc );

			pMsgNode->m_pMsg = pNextMsg;
		}
		else
		{
			pMsgNode->m_pMsg = NULL;
		}
	}
}

void GetDefinitionPath( const IProtoBufScriptObjectDefinition* pDef, CUtlString& strPath )
{
	strPath = CFmtStr( "%s%s/%d_%s.txt", g_pszProtoPath, ProtoDefTypes_Name( (ProtoDefTypes)pDef->GetDefType() ).c_str(), pDef->GetDefIndex(), pDef->GetName() ).Get();
}

bool BDeleteDefinitionFile( const IProtoBufScriptObjectDefinition* pDef )
{
	//
	// Mark the definition source file as deleted with P4
	//
	CUtlString strExistingFile;
	GetDefinitionPath( pDef, strExistingFile );
	strExistingFile.ToLower();

	bool bDeleted = false;

	if ( g_pFullFileSystem->FileExists( strExistingFile.Get(), "MOD" ) )
	{
		char szCorrectCaseFilePath[MAX_PATH];
		if ( !GenerateFullPath( strExistingFile.Get(), "MOD", szCorrectCaseFilePath, ARRAYSIZE( szCorrectCaseFilePath ) ) )
		{
			Warning( "Failed to GenerateFullPath to %s\n", strExistingFile.Get() );
		}

		CP4File p4File( szCorrectCaseFilePath );
		P4FileState_t state = p4File.GetFileState();
		switch ( state )
		{
			case P4FILE_OPENED_FOR_ADD:
			{
				// We're adding it, so just revert it
				bDeleted = p4File.Revert();
				break;
			}

			case P4FILE_OPENED_FOR_EDIT:
			{
				// File is already committed.  Mark for delete
				bDeleted = p4File.Delete();
				break;
			}
		
			case P4FILE_UNOPENED:
			{
				bDeleted = p4File.Delete();
				break;
			}

			default:
				Assert( false );
		}
	}

	if ( bDeleted )
	{
		g_pFullFileSystem->RemoveFile( strExistingFile, "MOD" );
	}

	return false;
}

IProtoBufScriptObjectDefinition::IProtoBufScriptObjectDefinition()
	: m_mapFieldValueSources( UniversalFieldIDLess )
{}

void IProtoBufScriptObjectDefinition::SerializeToBuffer( CUtlBuffer& bufOut ) const
{
	std::string strOut;
	
	int nSize = GetMsg()->ByteSize();
	void *buffer = malloc( nSize );
	if ( nSize == 0 || !GetMsg()->SerializeToArray( buffer, nSize ) )
	{
		DevMsg( "Failed to serialize def %s - %s (%d) to buffer!\n", ProtoDefTypes_Name( GetDefType() ).c_str(), GetName(), GetDefIndex() );
		SpewProtobufMessage( GetMsg() );
		Assert( false );
	}

	// Write the size
	bufOut.PutInt( nSize );
	// Write the message
	bufOut.Put( buffer, nSize );	
	free( buffer );
}


bool IProtoBufScriptObjectDefinition::BParseFromBuffer( CUtlBuffer& bufIn, CUtlVector<CUtlString> *pVecErrors )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	// Read the size
	int nSize = bufIn.GetInt();

	// Parse the message
	GetMutableMsg()->ParsePartialFromArray( bufIn.PeekGet(), nSize );
	bufIn.SeekGet( CUtlBuffer::SEEK_CURRENT, nSize );
	m_bCompiledMessageLoaded = true;
	m_bCompiledMessageDirty = false;

	// Do anything implementation specific
	return BOnDataLoaded( pVecErrors );
}

bool IProtoBufScriptObjectDefinition::BParseFromString( std::string& strIn, CUtlVector<CUtlString> *pVecErrors )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	// Parse the message
	google::protobuf::Message* pMsg = GetMutableTopLayerMsg();
	google::protobuf::io::ArrayInputStream stream( strIn.c_str(), strIn.length() );

	// Parse!
	google::protobuf::TextFormat::Parser().AllowPartialMessage( true );
	google::protobuf::TextFormat::Parse( &stream, pMsg );
	google::protobuf::TextFormat::Parser().AllowPartialMessage( false );
	m_bSourceMessagesLoaded = true;
	m_bCompiledMessageDirty = true;

	// Do anything implementation specific
	SCHEMA_INIT_CHECK( BOnDataLoaded( pVecErrors ), "Definition %s failed to BOnDataLoaded!", GetName() );
	
	return SCHEMA_INIT_SUCCESS();
}


uint32 IProtoBufScriptObjectDefinition::GetDefIndex() const
{
	// Use the header
	return GetHeader().defindex();
}

const char* IProtoBufScriptObjectDefinition::GetName() const
{
	// Use the header
	return GetHeader().name().c_str();
}

CMsgProtoDefID IProtoBufScriptObjectDefinition::GetID() const
{
	CMsgProtoDefID msgID;
	msgID.set_type( GetDefType() );
	msgID.set_defindex( GetDefIndex() );
	return msgID;
}

const CMsgProtoDefHeader& IProtoBufScriptObjectDefinition::GetHeader() const 
{ 
	return GetHeaderFromMessage( m_bCompiledMessageLoaded ? GetMsg() : GetTopLayerMessage() );
}

bool IProtoBufScriptObjectDefinition::BPostDataLoaded( CUtlVector<CUtlString> *pVecErrors )
{
	return true;
}

void ClearNonInheritedFields( ::google::protobuf::Message* pMsgPrefab )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);
	for( int i=0; i < pMsgPrefab->GetDescriptor()->field_count(); ++i )
	{
		const ::google::protobuf::FieldDescriptor* pField = pMsgPrefab->GetDescriptor()->field( i );

		if ( pField->options().GetExtension( do_not_inherit ) )
		{
			pMsgPrefab->GetReflection()->ClearField( pMsgPrefab, pField );
		}
		else if ( pField->type() == ::google::protobuf::FieldDescriptor::TYPE_MESSAGE )
		{
			if ( pField->is_repeated() )
			{
				for( int j=0; j < pMsgPrefab->GetReflection()->FieldSize( *pMsgPrefab, pField ); ++j )
				{
					ClearNonInheritedFields( pMsgPrefab->GetReflection()->MutableRepeatedMessage( pMsgPrefab, pField, j ) );
				}
			}
			else
			{
				if ( pMsgPrefab->GetReflection()->HasField( *pMsgPrefab, pField ) )
				{
					ClearNonInheritedFields( pMsgPrefab->GetReflection()->MutableMessage( pMsgPrefab, pField ) );
				}
			}
		}
	}
}

int GetRepeatedIndexForInheritedMessage( const google::protobuf::Message* pMsgToSearch,
											const google::protobuf::Message* pMsgToInherit,
											const google::protobuf::FieldDescriptor* pField )
{

	const google::protobuf::FieldDescriptor* pKeyField = GetMergingKeyField( pMsgToInherit->GetDescriptor() );
	if ( !pKeyField )
		return -1;

	if ( !pMsgToInherit->GetReflection()->HasField( *pMsgToInherit, pKeyField ) )
		return -1;

	auto pReflection = pMsgToSearch->GetReflection(); 

	for( int i=0; i < pReflection->FieldSize( *pMsgToSearch, pField ); ++i )
	{
		const google::protobuf::Message* pMsgSearchRepeatedMessage = &pReflection->GetRepeatedMessage( *pMsgToSearch, pField, i );

		// If we're both not less than each other, then we're equal.  
		if ( !GCSDK::IsProtoBufFieldLess( *pMsgToInherit, *pMsgSearchRepeatedMessage, pKeyField, pKeyField ) &&
			 !GCSDK::IsProtoBufFieldLess( *pMsgSearchRepeatedMessage, *pMsgToInherit, pKeyField, pKeyField ) )
		{
			// If we're equal, then THIS is the matching message that we want to merge with.
			return i;
		}
	}

	// No match
	return -1;
}


CProtoBufScriptObjectDefinitionManager* GetProtoScriptObjDefManager()
{
	static CProtoBufScriptObjectDefinitionManager* s_pMgr = NULL;
	if ( s_pMgr == NULL )
	{
		s_pMgr = new CProtoBufScriptObjectDefinitionManager();
	}

	return s_pMgr;
}

void ProtoLogHandler( google::protobuf::LogLevel level, const char* filename, int line,
					  const std::string& message )
{
	if ( level == google::protobuf::LOGLEVEL_DFATAL ||
		 level == google::protobuf::LOGLEVEL_ERROR )
	{
		AssertMsg( !"Protobuf Error", "%s", message.c_str() );
	}
	else
	{
		DevMsg( "%s", message.c_str() );
	}
}

CProtoBufScriptObjectDefinitionManager::CProtoBufScriptObjectDefinitionManager()
	: m_mapCopyCount( DefLessFunc( ProtoDefID_t ) )
{
	m_mapFactories.SetLessFunc( DefLessFunc( ProtoDefTypes ) );

	google::protobuf::SetLogHandler( ProtoLogHandler );

	for( int nType=0; nType < ProtoDefTypes_MAX + 1; ++nType ) 
	{
		if ( !ProtoDefTypes_IsValid( nType ) )
			continue;

		m_arDefinitionsMaps[ nType ].SetLessFunc( DefLessFunc( uint32 ) );
	}
}

bool CProtoBufScriptObjectDefinitionManager::Init()
{
#if CLIENT_DLL
	Assert( g_pVGuiLocalize );
	if (g_pVGuiLocalize)
	{
		g_pVGuiLocalize->AddFile( "resource/tf_proto_obj_defs_%language%.txt" );
	}
#endif

	{
		BInitDefinitions();
		return true;
	}
}

void CProtoBufScriptObjectDefinitionManager::PostInit()
{
	// PostInit is called automatically on the client/server due to this
	// deriving from AutoGameSystem.  The schema is also an AutoGameSystem and gets
	// initialized in the Init() round. To make sure we go after the Schema,
	// we need to init our defs on the PostInit() round.
	BPostDefinitionsLoaded();
}

struct SpewOnDestruct
{
	SpewOnDestruct( CUtlVector<CUtlString>& vecErrors ) : m_vecErrors( vecErrors ) {}
	~SpewOnDestruct()
	{
		FOR_EACH_VEC( m_vecErrors, i )
		{
			DevMsg( 0, "%s\n", m_vecErrors[ i ].Get() );
		}
		
		Assert( m_vecErrors.Count() == 0 );
	}
	const CUtlVector<CUtlString>& m_vecErrors;
};

bool CProtoBufScriptObjectDefinitionManager::BInitDefinitions()
{
	m_bDefinitionsLoaded = false;
	m_bDefinitionsPostDataLoadedCalled = false;

	CUtlVector<CUtlString> vecErrors;
	CUtlVector<CUtlString>* pVecErrors = &vecErrors; // Silly, but makes the SCHEMA_INIT_ macros work

	SpewOnDestruct spew( vecErrors );

	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__);

	// Clear out old definitions
	for (int nType = 0; nType < ProtoDefTypes_MAX + 1; ++nType)
	{
		if ( !ProtoDefTypes_IsValid( nType ) )
			continue;
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Purging old definitions", __FUNCTION__ );
		m_arDefinitionsMaps[ nType ].PurgeAndDeleteElements();
	}


	//
	// Parse all the types and definitions from the compiled binary file
	//
	CUtlBuffer buffer;
	bool bReadFileOK = g_pFullFileSystem->ReadFile( g_pszProtoDefFile, "GAME", buffer );
	SCHEMA_INIT_CHECK( bReadFileOK, "Failed to load %s!", g_pszProtoDefFile );

	// Do we need to check the signature?
#if defined(TF_DLL) || defined(TF_CLIENT_DLL)
	{
		// Load up the signature
		CUtlString sSignatureFilename( g_pszProtoDefFile ); sSignatureFilename.Append( ".sig" );
		CUtlBuffer bufSignatureBinary;
		bool bReadSignatureOK = g_pFullFileSystem->ReadFile( sSignatureFilename.String(), "GAME", bufSignatureBinary );
		SCHEMA_INIT_CHECK( bReadSignatureOK, "Cannot load file '%s'", sSignatureFilename.String() );

		// Check it with the Valve public key
		bool bSignatureValid = CheckValveSignature(
			buffer.Base(), buffer.TellPut(),
			bufSignatureBinary.Base(), bufSignatureBinary.TellPut()
		);

		// If they have a signature for a zero-byte file, that's OK, too.
		// That's the secret code that is checked into P4 internally that
		// let's us run with any items_game file
		if ( !bSignatureValid )
		{
			bSignatureValid = CheckValveSignature(
				"", 0,
				bufSignatureBinary.Base(), bufSignatureBinary.TellPut()
			);
		}

		SCHEMA_INIT_CHECK( bSignatureValid, "'%s' is corrupt.  Please verify your local game files.  (https://support.steampowered.com/kb_article.php?ref=2037-QEUH-3335)", g_pszProtoDefFile );
	}
#endif

	uint32 nBytesUsed = 0;

	if ( buffer.TellPut() > buffer.TellGet() )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Parsing definitions", __FUNCTION__ );

		while ( buffer.GetBytesRemaining() > 0 )
		{
			int nType = buffer.GetInt();	// Which type
			int nNumDefs = buffer.GetInt();	// How many definitions there are

			// Make sure there's a factory to create this type
			auto idx = m_mapFactories.Find( (ProtoDefTypes)nType );
			SCHEMA_INIT_CHECK( idx != m_mapFactories.InvalidIndex(), "No factory for type %s!", ProtoDefTypes_Name( (ProtoDefTypes)nType ).c_str() );

			// Parse all the defs
			while ( nNumDefs-- )
			{
				IProtoBufScriptObjectDefinition* pNewDef = m_mapFactories[ idx ]->CreateNewObject();
				SCHEMA_INIT_SUBSTEP( pNewDef->BParseFromBuffer( buffer, &vecErrors ) );
				InsertObjectIntoMap( pNewDef );
				nBytesUsed += pNewDef->GetMsg()->SpaceUsed();
			}
		}
	}
	else
	{
		SCHEMA_INIT_CHECK( false, "Failed to load %s!", g_pszProtoDefFile );
	}

	m_bDefinitionsLoaded = true;

	BPostDefinitionsLoaded();

	Msg( "ProtoDefs loaded. %s used\n", V_pretifymem( nBytesUsed ) );

	return SCHEMA_INIT_SUCCESS();
}


bool CProtoBufScriptObjectDefinitionManager::BPostDefinitionsLoaded()
{
	if ( !m_bDefinitionsLoaded && !m_bSourceFilesLoaded )
	{
		AssertMsg( false, "BPostDefinitionsLoaded called before BInitDefinitions!\n" );
		SCHEMA_INIT_SUBSTEP( BInitDefinitions() );
	}

	CUtlVector<CUtlString> vecErrors;
	CUtlVector<CUtlString>* pVecErrors = &vecErrors; // Silly, buy makes the SCHEMA_INIT_ macros work
	SpewOnDestruct spew( vecErrors );

	// BPostDataLoaded after all defs have been loaded and BOnDataLoaded has
	// been called on them.
	m_bDefinitionsPostDataLoadedCalled = true;
	for( int nType=0; nType < ProtoDefTypes_MAX + 1; ++nType )
	{
		if ( !ProtoDefTypes_IsValid( nType ) )
			continue;

		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - BPostDataLoaded Type: %s", __FUNCTION__, ProtoDefTypes_Name( (ProtoDefTypes)nType ).c_str() );
		FOR_EACH_MAP_FAST( m_arDefinitionsMaps[ nType ], nIndex )
		{
			IProtoBufScriptObjectDefinition* pDef = m_arDefinitionsMaps[ nType ][ nIndex ];
			pDef->BPostDataLoaded( &vecErrors );
		}
	}

	Msg( "ProtoDefs post data loaded.\n" );

	return SCHEMA_INIT_SUCCESS();
}


const IProtoBufScriptObjectDefinition* CProtoBufScriptObjectDefinitionManager::GetDefinition( const ProtoDefID_t& defID ) const
{
	return GetMutableDefinition( defID );
}

IProtoBufScriptObjectDefinition* CProtoBufScriptObjectDefinitionManager::GetMutableDefinition( const ProtoDefID_t& defID ) const
{
	Assert( m_bDefinitionsLoaded );
	Assert( m_bDefinitionsPostDataLoadedCalled );

	auto& mapType = m_arDefinitionsMaps[ defID.GetType() ];
	auto idx = mapType.Find( defID.GetDefindex() );
	if ( idx != mapType.InvalidIndex() )
	{
		return mapType[ idx ];
	}

	return NULL;
}

void CProtoBufScriptObjectDefinitionManager::InsertObjectIntoMap( IProtoBufScriptObjectDefinition* pObject )
{
	// Insert everything into the definitions map
	auto& mapType = m_arDefinitionsMaps[ pObject->GetDefType() ];
	auto defidx = mapType.Find( pObject->GetDefIndex() );
	if ( defidx == mapType.InvalidIndex() )
	{
		mapType.Insert( pObject->GetDefIndex(), pObject );
	}
	else
	{
		AssertMsg2( false, "Attempted to double-insert proto script def %d:%s!", pObject->GetDefIndex(), pObject->GetName() );
		// It had at least be us in that defindex slot
		Assert( mapType[ defidx ] == pObject );
		mapType[ defidx ] = pObject;
	}
}

void CProtoBufScriptObjectDefinitionManager::RegisterTypeFactory( const IProtoBufScriptObjectFactory* pFactory )
{
	auto idx = m_mapFactories.Find( pFactory->GetType() );
	Assert( idx == m_mapFactories.InvalidIndex() );
	if ( idx == m_mapFactories.InvalidIndex() )
	{
		m_mapFactories.Insert( pFactory->GetType(), pFactory );
	}
}

//
// Editing relating functions
//

const ::google::protobuf::Message* GetSubMessage( const ::google::protobuf::Message* pMsg,
	const ::google::protobuf::FieldDescriptor *pField,
	int nIndex )
{
	if ( pMsg )
	{
		if ( pField->is_repeated() )
		{
			if ( nIndex < pMsg->GetReflection()->FieldSize( *pMsg, pField ) )
			{
				return &pMsg->GetReflection()->GetRepeatedMessage( *pMsg, pField, nIndex );
			}
		}
		else if ( pField->is_required() || ( pMsg->GetReflection()->HasField( *pMsg, pField ) && pField->is_optional() ) )
		{
			return &pMsg->GetReflection()->GetMessage( *pMsg, pField );
		}
	}

	return NULL;
}

::google::protobuf::Message* GetMutableSubMessage( ::google::protobuf::Message* pMsg,
	const ::google::protobuf::FieldDescriptor *pField,
	int nIndex )
{
	if ( pMsg )
	{
		if ( !pField->is_repeated() )
		{
			return pMsg->GetReflection()->MutableMessage( pMsg, pField );
		}
		else if ( nIndex < pMsg->GetReflection()->FieldSize( *pMsg, pField ) )
		{
			return pMsg->GetReflection()->MutableRepeatedMessage( pMsg, pField, nIndex );
		}

		Assert( false );
	}

	return NULL;
}

::google::protobuf::Message* AddSubMessage( ::google::protobuf::Message* pMsg, const ::google::protobuf::FieldDescriptor *pField )
{
	::google::protobuf::Message* pSubMsg = pField->is_repeated() ? pMsg->GetReflection()->AddMessage( pMsg, pField )
		: pMsg->GetReflection()->MutableMessage( pMsg, pField );

	return pSubMsg;
}


class CHeaderOnlyDef : public CTypedProtoBufScriptObjectDefinition< CMsgHeaderOnly, DEF_TYPE_HEADER_ONLY >
{
public:
	CHeaderOnlyDef() {}
	virtual ~CHeaderOnlyDef() {}
};
REGISTER_PROTO_DEF_FACTORY( CHeaderOnlyDef, DEF_TYPE_HEADER_ONLY )
