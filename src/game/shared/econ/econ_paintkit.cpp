//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Functions related to paintkits
//
//=============================================================================


#include "cbase.h"
#include "econ_paintkit.h"

#ifdef CLIENT_DLL
#include "filesystem.h"
#include "game_item_schema.h"
#endif // CLIENT_DLL


REGISTER_PROTO_DEF_FACTORY( CPaintKitVariables,			DEF_TYPE_PAINTKIT_VARIABLES )
REGISTER_PROTO_DEF_FACTORY( CPaintKitOperation,			DEF_TYPE_PAINTKIT_OPERATION )
REGISTER_PROTO_DEF_FACTORY( CPaintKitItemDefinition,	DEF_TYPE_PAINTKIT_ITEM_DEFINITION )
REGISTER_PROTO_DEF_FACTORY( CPaintKitDefinition,		DEF_TYPE_PAINTKIT_DEFINITION )

const IProtoBufScriptObjectDefinition *GetProtoObjectDefinitionFromMsg( const CMsgProtoDefID *pDefIDMsg )
{
	return GetProtoScriptObjDefManager()->GetDefinition( ProtoDefID_t( pDefIDMsg->type(), pDefIDMsg->defindex() ) );
}

bool CPaintKitDefinition::CanApplyToItem( item_definition_index_t iDefIndex ) const
{
	GenerateSupportedItems();

	FOR_EACH_VEC( m_vecSupportedItems, i )
	{
		if ( m_vecSupportedItems[i].m_itemDef == iDefIndex )
		{
			return true;
		}
	}

	return false;
}

int CPaintKitDefinition::GetSupportedItems( CUtlVector< item_definition_index_t > *pVecItemIcons /*= NULL*/ ) const
{
	GenerateSupportedItems();

	if ( pVecItemIcons )
	{
		FOR_EACH_VEC( m_vecSupportedItems, i )
		{
			pVecItemIcons->AddToTail( m_vecSupportedItems[i].m_itemDef );
		}
	}

	return m_vecSupportedItems.Count();
}

int CPaintKitDefinition::GetItemsThatCanRenderThisPaintkit( CUtlVector< item_definition_index_t > *pVecItemIcons /*= NULL*/ ) const
{
	int nSupportedItems = GetSupportedItems( pVecItemIcons );
	if ( pVecItemIcons )
	{
		if ( m_bHasPaintKitTool )
		{
			static CSchemaItemDefHandle pPaintkitToolItemDef( "Paintkit" );
			pVecItemIcons->AddToTail( pPaintkitToolItemDef->GetDefinitionIndex() );
			nSupportedItems++;
		}
	}
	return nSupportedItems;
}

const char* CPaintKitDefinition::GetMaterialOverride( item_definition_index_t iDefIndex ) const
{
	GenerateSupportedItems();

	FOR_EACH_VEC( m_vecSupportedItems, i )
	{
		if ( m_vecSupportedItems[i].m_itemDef == iDefIndex )
		{
			return m_vecSupportedItems[i].m_pszMaterialOverride;
		}
	}

	return NULL;
}

void CPaintKitDefinition::GenerateSupportedItems() const
{
	if ( m_vecSupportedItems.IsEmpty() )
	{
		CUtlVector< SupportedItem_t > tempVec;
		const CMsgPaintKit_Definition *pPaintKitDef = static_cast< const CMsgPaintKit_Definition * >( GetMsg() );
		bool bHasPaintKitTool = false;
		auto lambdaGenerateSupportedItems = [ pPaintKitDef, &tempVec, &bHasPaintKitTool ]( const google::protobuf::Message* pMsgVar, const google::protobuf::FieldDescriptor* pField, const CMsgFieldID& fieldID )->bool
		{
			if ( !BMessagesTypesAreTheSame( pMsgVar->GetDescriptor(), CMsgPaintKit_Definition_Item::descriptor() ) )
			{
				return true;
			}

			const CMsgPaintKit_Definition_Item *pItemMsg = static_cast< const CMsgPaintKit_Definition_Item* >( pMsgVar );
			if ( !pItemMsg->has_item_definition_template() )
			{
				return true;
			}

			// check if this definition can be used to render paintkit tool
			if ( !bHasPaintKitTool )
			{
				const CPaintKitItemDefinition *pItemObj = assert_cast< const CPaintKitItemDefinition * >( GetProtoObjectDefinitionFromMsg( &pItemMsg->item_definition_template() ) );
				const CMsgPaintKit_ItemDefinition *pItemDefMsg = assert_cast< const CMsgPaintKit_ItemDefinition* >( pItemObj->GetMsg() );
				static CSchemaItemDefHandle pPaintkitToolItemDef( "Paintkit" );
				if ( pItemDefMsg->item_definition_index() == pPaintkitToolItemDef->GetDefinitionIndex() )
				{
					bHasPaintKitTool = true;
				}
			}

			if ( !pItemMsg->data().can_apply_paintkit() )
			{
				return true;
			}

			const CPaintKitItemDefinition *pItemObj = assert_cast< const CPaintKitItemDefinition * >( GetProtoObjectDefinitionFromMsg( &pItemMsg->item_definition_template() ) );
			const CMsgPaintKit_ItemDefinition *pItemDefMsg = assert_cast< const CMsgPaintKit_ItemDefinition* >( pItemObj->GetMsg() );

			item_definition_index_t iDefIndex = pItemDefMsg->item_definition_index();
			FOR_EACH_VEC( tempVec, i )
			{
				if ( tempVec[i].m_itemDef == iDefIndex )
				{
					// already added this
					return true;
				}
			}

			SupportedItem_t *pSupportedItem = tempVec.AddToTailGetPtr();
			pSupportedItem->m_itemDef = iDefIndex;
			const char *pszMaterialOverride = pItemMsg->data().material_override().c_str();
			pSupportedItem->m_pszMaterialOverride = pszMaterialOverride && *pszMaterialOverride ? pszMaterialOverride : NULL;

			return true;
		};
		ForEachConstProtoField( pPaintKitDef, lambdaGenerateSupportedItems, true, false );

		m_vecSupportedItems.AddVectorToTail( tempVec );
		m_bHasPaintKitTool = bHasPaintKitTool;
	}
}

#ifdef CLIENT_DLL


const google::protobuf::Message *GetMsgFromDefIDMsg( const CMsgProtoDefID *pDefIDMsg, bool bTopLayerMsg )
{
	const IProtoBufScriptObjectDefinition *pObj = GetProtoObjectDefinitionFromMsg( pDefIDMsg );
	Assert( pObj );
	if ( pObj )
	{
		if ( bTopLayerMsg )
		{
			return pObj->GetTopLayerMessage();
		}
		else
		{
			return pObj->GetMsg();
		}
	}

	return NULL;
}

bool BShouldExcludeVar( const CMsgProtoDefHeader& header, const char *pszVarName )
{
	for ( int iHeaderVar = 0; iHeaderVar < header.variables_size(); ++iHeaderVar )
	{
		const CMsgVariableDefinition &headerVar = header.variables( iHeaderVar );
		if ( FStrEq( headerVar.name().c_str(), pszVarName ) )
		{
			return true;
		}
	}

	return false;
}

void UpdateMsgVariables( google::protobuf::Message *pVariableContainerMsg, const CMsgProtoDefHeader& header, bool bKeepOldValues, const CMsgProtoDefHeader* pExcludeHeader = NULL )
{
	// add all variables to msg
	const char *pszVariableFieldName = "variable";
	const google::protobuf::FieldDescriptor *pField = pVariableContainerMsg->GetDescriptor()->FindFieldByName( pszVariableFieldName );
	if ( !pField )
	{
		Assert( pField );
		Warning( "%s doesn't have a field name '%s'\n", pVariableContainerMsg->GetDescriptor()->name().c_str(), pszVariableFieldName );
		return;
	}

	// save the override values
	CUtlDict< CUtlString > overrideDict;
	if ( bKeepOldValues )
	{
		for ( int iOldVar = 0; iOldVar < pVariableContainerMsg->GetReflection()->FieldSize( *pVariableContainerMsg, pField ); ++iOldVar )
		{
			const CMsgVarField *pOldVar = assert_cast< const CMsgVarField * >( &pVariableContainerMsg->GetReflection()->GetRepeatedMessage( *pVariableContainerMsg, pField, iOldVar ) );
			const char *pszVarName = pOldVar->variable().c_str();
			int iIndex = overrideDict.Find( pszVarName );
			if ( iIndex == overrideDict.InvalidIndex() )
			{
				iIndex = overrideDict.Insert( pszVarName );
			}
			overrideDict[iIndex] = pOldVar->string().c_str();
		}
	}

	// clear old variables
	pVariableContainerMsg->GetReflection()->ClearField( pVariableContainerMsg, pField );

	// add new one and override with old values
	for ( int iHeaderVar = 0; iHeaderVar < header.variables_size(); ++iHeaderVar )
	{
		const CMsgVariableDefinition &headerVar = header.variables( iHeaderVar );
		if ( !headerVar.inherit() )
		{
			continue;
		}

		const char *pszName = headerVar.name().c_str();
		const char *pszValue = NULL;

		if ( pExcludeHeader && BShouldExcludeVar( *pExcludeHeader, pszName ) )
		{
			continue;
		}

		int iIndex = overrideDict.Find( pszName );
		if ( iIndex != overrideDict.InvalidIndex() )
		{
			pszValue = overrideDict[iIndex].String();
		}

		CMsgVarField *pNewVar = assert_cast< CMsgVarField* >( pVariableContainerMsg->GetReflection()->AddMessage( pVariableContainerMsg, pField ) );
		pNewVar->set_variable( pszName );
		if ( pszValue && *pszValue )
		{
			pNewVar->set_string( pszValue );
		}
	}
}

static void UpdatePaintKitItemDefinitionVariables( CMsgPaintKit_ItemDefinition *pItemDefMsg, const CMsgProtoDefHeader& header, int iDefIndex, bool bKeepOldValues )
{
	if ( iDefIndex < 0 || iDefIndex >= pItemDefMsg->definition_size() )
		return;

	CMsgPaintKit_ItemDefinition_Definition *pPaintKit = pItemDefMsg->mutable_definition( iDefIndex );
	UpdateMsgVariables( pPaintKit, header, bKeepOldValues );
}

bool BConvertPaintKitItemDefinition( google::protobuf::Message* pMsgSource, 
									 const google::protobuf::Message* pMsgCompiled)
{
	auto &header = GetHeaderFromMessage( pMsgCompiled );
	CMsgPaintKit_ItemDefinition *pItemDefMsg = assert_cast< CMsgPaintKit_ItemDefinition* >( pMsgSource );
	for ( int i = 0; i < pItemDefMsg->definition_size(); ++i )
	{
		UpdatePaintKitItemDefinitionVariables( pItemDefMsg, header, i, true );
	}

	return false;
}
REGISTER_PROTODEF_TYPE_POST_SOLVE_FUNCTION( BConvertPaintKitItemDefinition, DEF_TYPE_PAINTKIT_ITEM_DEFINITION );


void OperationToKV( const google::protobuf::Message *pMsg, KeyValues *pOutKV );

KeyValues *HandleOperationToKVMsg( const google::protobuf::Message* pMsg, const google::protobuf::FieldDescriptor* pField, int nRepeatedIndex, KeyValues *pOutKV )
{
	const google::protobuf::Message* pSubMsg = NULL;
	if ( pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
	{
		pSubMsg = GetSubMessage( pMsg, pField, nRepeatedIndex );
	}

	CUtlString strFieldName = pField->name().c_str();
	strFieldName.ToLower();
	const char *pszFieldName = strFieldName.String();

	if ( pSubMsg && BMessagesTypesAreTheSame( pSubMsg->GetDescriptor(), CMsgProtoDefID::descriptor() ) )
	{
		const CMsgProtoDefID *pDefIDMsg = static_cast< const CMsgProtoDefID* >( pSubMsg );
		OperationToKV( GetMsgFromDefIDMsg( pDefIDMsg, false ), pOutKV );
	}
	else if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_OperationStage::descriptor() ) )
	{
		KeyValues *pSubKV = new KeyValues( pszFieldName );
		pOutKV->AddSubKey( pSubKV );
		return pSubKV;
	}
	else if (	BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_TextureStage::descriptor() ) ||
				BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_CombineStage::descriptor() ) ||
				BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_SelectStage::descriptor()	)  ||
				BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_StickerStage::descriptor() ) ||
				BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_Sticker::descriptor() )
			)
	{
		if ( !pSubMsg )
		{
			return pOutKV;
		}

		if ( BMessagesTypesAreTheSame( pSubMsg->GetDescriptor(), CMsgVarField::descriptor() ) )
		{
			const CMsgVarField *pVarField = static_cast< const CMsgVarField * >( pSubMsg );
			CFmtStr strVarName;
			const char *pszValueOrVariable = pVarField->has_variable() ? ( strVarName.sprintf( "$[%s]", pVarField->variable().c_str() ), strVarName.Get() ) : pVarField->string().c_str();

			// we only allow 'select' field to have multiple entries. warn if something else has multiple entries
			if ( !FStrEq( pszFieldName, "select" ) && pOutKV->FindKey( pszFieldName ) != NULL )
			{
				Warning( "Multiple variable [%s] defined in this operation.\n", pszFieldName );
			}

			KeyValues *pSubKV = new KeyValues( pszFieldName );
			pSubKV->SetStringValue( pszValueOrVariable );
			pOutKV->AddSubKey( pSubKV );
			return pSubKV;
		}
		else if ( BMessagesTypesAreTheSame( pSubMsg->GetDescriptor(), CMsgPaintKit_Operation_Sticker::descriptor() ) )
		{
			KeyValues *pSubKV = new KeyValues( pszFieldName );
			pOutKV->AddSubKey( pSubKV );
			return pSubKV;
		}
	}

	return pOutKV;
}

void OperationToKV( const google::protobuf::Message *pMsg, KeyValues *pOutKV )
{
	for ( int i = 0; i < pMsg->GetDescriptor()->field_count(); ++i )
	{
		const google::protobuf::FieldDescriptor* pField = pMsg->GetDescriptor()->field( i );

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
				KeyValues *pNextKV = HandleOperationToKVMsg( pMsg, pField, j, pOutKV );

				// We only care about messages for recursion
				if ( pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
				{
					const google::protobuf::Message* pSubMsg = &pMsg->GetReflection()->GetRepeatedMessage( *pMsg, pField, j );
					OperationToKV( pSubMsg, pNextKV );
				}
			}
		}
		else if ( pMsg->GetReflection()->HasField( *pMsg, pField ) )
		{
			KeyValues *pNextKV = HandleOperationToKVMsg( pMsg, pField, 0, pOutKV );

			// We only care about messages for recursion
			if ( pField->type() == google::protobuf::FieldDescriptor::TYPE_MESSAGE )
			{
				OperationToKV( &pMsg->GetReflection()->GetMessage( *pMsg, pField ), pNextKV );
			}
		}
	}
}

KeyValues *CreatePaintKitDefinitionKV( const char *pszItemName, const CMsgPaintKit_Operation *pOperationMsg, const VariableDict_t &itemVarDict, int iPaintKitIndex )
{
	KeyValues *pOutKV = new KeyValues( CFmtStr( "wear_level_%d", iPaintKitIndex ) );

	FOR_EACH_DICT_FAST( itemVarDict, i )
	{
		const char *pszOperationVariable = itemVarDict.GetElementName( i );
		const char *pszOperationValue = itemVarDict[i].m_strValue.String();

		// write out to KV
		pOutKV->SetString( CFmtStr( "$%s", pszOperationVariable ), pszOperationValue );
	}

	OperationToKV( pOperationMsg, pOutKV );

	return pOutKV;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
KeyValues *CPaintKitDefinition::GetItemPaintKitDefinitionKV( item_definition_index_t iItemDefIndex, int iPaintKitIndex ) const
{

	PaintKitMapKey_t key( iItemDefIndex, iPaintKitIndex );
	int iIndex = m_mapCacheKV.Find( key );
	if ( iIndex != m_mapCacheKV.InvalidIndex() )
	{
		return m_mapCacheKV[ iIndex ];
	}

	const CMsgPaintKit_Definition *pPaintKitDef = static_cast< const CMsgPaintKit_Definition * >( GetMsg() );

	KeyValues *pPaintKitKV = NULL;
	auto lambdaCreatePaintKitKV = [&pPaintKitKV, pPaintKitDef, key, this]( const google::protobuf::Message* pMsgVar, const google::protobuf::FieldDescriptor* pField, const CMsgFieldID& fieldID )->bool
	{
		if ( !BMessagesTypesAreTheSame( pMsgVar->GetDescriptor(), CMsgPaintKit_Definition_Item::descriptor() ) )
		{
			return true;
		}

		const CMsgPaintKit_Definition_Item *pItemMsg = static_cast< const CMsgPaintKit_Definition_Item* >( pMsgVar );
		if ( !pItemMsg->has_item_definition_template() )
		{
			return true;
		}

		const CPaintKitItemDefinition *pItemObj = assert_cast< const CPaintKitItemDefinition * >( GetProtoObjectDefinitionFromMsg( &pItemMsg->item_definition_template() ) );
		const CMsgPaintKit_ItemDefinition *pItemDefMsg = assert_cast< const CMsgPaintKit_ItemDefinition* >( pItemObj->GetMsg() );
		if ( pItemDefMsg->item_definition_index() == key.first )
		{
			VariableDict_t itemVarDict;

			const CMsgProtoDefHeader *pBaseVarHeader = &pPaintKitDef->header();

			const CMsgPaintKit_Operation *pOperationMsg = NULL;
			if ( pPaintKitDef->has_operation_template() )
			{
				pOperationMsg = assert_cast< const CMsgPaintKit_Operation * >( GetMsgFromDefIDMsg( &pPaintKitDef->operation_template(), false ) );
			}

			// check if we should override operation msg
			int iPaintKitDefIndex = key.second - 1;
			Assert( iPaintKitDefIndex >= 0 && iPaintKitDefIndex < pItemDefMsg->definition_size() );
			iPaintKitDefIndex = clamp( iPaintKitDefIndex, 0, pItemDefMsg->definition_size() - 1 );

			const CMsgPaintKit_ItemDefinition_Definition *pPaintKitItemDef_PaintKit = &pItemDefMsg->definition( iPaintKitDefIndex );
			if ( pPaintKitItemDef_PaintKit->has_operation_template() )
			{
				const CMsgPaintKit_Operation *pOperationMsgOverride = assert_cast< const CMsgPaintKit_Operation * >( GetMsgFromDefIDMsg( &pPaintKitItemDef_PaintKit->operation_template(), false ) );
				if ( pOperationMsgOverride )
				{
					pOperationMsg = pOperationMsgOverride;
					pBaseVarHeader = &pOperationMsgOverride->header();
				}
			}

			for ( int i = 0; i < pBaseVarHeader->variables_size(); ++i )
			{
				auto& var = pBaseVarHeader->variables( i );
				int iIndex = itemVarDict.Insert( var.name().c_str() );
				itemVarDict[iIndex].m_strValue = var.value().c_str();
				itemVarDict[iIndex].m_bCanOverride = var.inherit();
			}

			auto lambdaUpdateVarField = [ &itemVarDict ]( const google::protobuf::Message* pMsgVar, const google::protobuf::FieldDescriptor* pField, const CMsgFieldID& fieldID )->bool
			{
				if ( BMessagesTypesAreTheSame( pMsgVar->GetDescriptor(), CMsgVarField::descriptor() ) )
				{
					const CMsgVarField *pVar = static_cast< const CMsgVarField * >( pMsgVar );
					const char *pszVarName = pVar->variable().c_str();
					const char *pszVarVal = pVar->string().c_str();
					int iIndex = itemVarDict.Find( pszVarName );
					if ( iIndex != itemVarDict.InvalidIndex() && itemVarDict[iIndex].m_bCanOverride && !FStrEq( itemVarDict[iIndex].m_strValue.String(), pszVarVal ) )
					{
						itemVarDict[iIndex].m_strValue = pszVarVal;
					}
				}

				return true;
			};

			auto lambdaUpdateVarDef = [ &itemVarDict ]( const google::protobuf::Message* pMsgVar, const google::protobuf::FieldDescriptor* pField, const CMsgFieldID& fieldID )->bool
			{
				if ( BMessagesTypesAreTheSame( pMsgVar->GetDescriptor(), CMsgVariableDefinition::descriptor() ) )
				{
					const CMsgVariableDefinition *pVar = static_cast< const CMsgVariableDefinition * >( pMsgVar );
					const char *pszVarName = pVar->name().c_str();
					const char *pszVarVal = pVar->value().c_str();
					int iIndex = itemVarDict.Find( pszVarName );
					if ( iIndex != itemVarDict.InvalidIndex() && itemVarDict[iIndex].m_bCanOverride && !FStrEq( itemVarDict[iIndex].m_strValue.String(), pszVarVal ) )
					{
						itemVarDict[iIndex].m_strValue = pszVarVal;
					}
				}

				return true;
			};

			ForEachConstProtoField( &pItemMsg->data(), lambdaUpdateVarField, true, false );

			ForEachConstProtoField( &pItemDefMsg->header(), lambdaUpdateVarDef, true, false );

			ForEachConstProtoField( pPaintKitItemDef_PaintKit, lambdaUpdateVarField, true, false );

			pPaintKitKV = CreatePaintKitDefinitionKV( pItemObj->GetName(), pOperationMsg, itemVarDict, key.second );


			if ( pPaintKitKV )
			{
				m_mapCacheKV.Insert( key, pPaintKitKV );
			}

			// stop the loop once we found the key we're trying to create
			return false;
		}

		return true;
	};
	ForEachConstProtoField( pPaintKitDef, lambdaCreatePaintKitKV, true, false );

	return pPaintKitKV;
}

bool BParseOperationMsg( ::google::protobuf::Message* pMsg, KeyValues* pKV );

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ParseOperationVariable( CMsgVarField* pMsg, KeyValues *pKV )
{
	const char *pszVal = pKV->GetString();
	if ( V_stristr( pszVal, "[$" ) )
	{
		Assert( !"old paintkit def shouldn't have any variables" );
		//pMsg->set_variable_name( pszVal );
	}
	else
	{
		pMsg->set_string( pszVal );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void ParseOperationVariablesFromMsg( ::google::protobuf::Message* pMsg, KeyValues* pKV )
{
	FOR_EACH_VALUE( pKV, pValueKV )
	{
		CUtlString strFieldName = pValueKV->GetName();
		strFieldName.ToLower();
		const ::google::protobuf::FieldDescriptor *pField = pMsg->GetDescriptor()->FindFieldByName( strFieldName.String() );
		::google::protobuf::Message* pVarMsg = AddSubMessage( pMsg, pField );
		BParseOperationMsg( pVarMsg, pValueKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template< class MsgType >
void ParseOperationNodeFromMsg( ::google::protobuf::Message* pMsg, KeyValues* pNodeKV )
{
	MsgType *pTypedMsg = static_cast< MsgType* >( pMsg );
	CUtlString strNodeName = pNodeKV->GetName();
	strNodeName.ToLower();
	CMsgPaintKit_OperationNode *pNode = pTypedMsg->add_operation_node();
	const ::google::protobuf::FieldDescriptor *pField = pNode->mutable_stage()->GetDescriptor()->FindFieldByName( strNodeName.String() );
	::google::protobuf::Message* pTypedStageMsg = AddSubMessage( pNode->mutable_stage(), pField );
	BParseOperationMsg( pTypedStageMsg, pNodeKV );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template< class MsgType >
void ParseAllOperationNodeFromMsg( ::google::protobuf::Message* pMsg, KeyValues* pKV )
{
	FOR_EACH_TRUE_SUBKEY( pKV, pNodeKV )
	{
		ParseOperationNodeFromMsg< MsgType >( pMsg, pNodeKV );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
template<>
void ParseAllOperationNodeFromMsg< CMsgPaintKit_Operation_StickerStage >( ::google::protobuf::Message* pMsg, KeyValues* pKV )
{
	FOR_EACH_TRUE_SUBKEY( pKV, pNodeKV )
	{
		CUtlString strNodeName = pNodeKV->GetName();
		strNodeName.ToLower();
		if ( FStrEq( strNodeName.String(), "sticker" ) )
		{
			CMsgPaintKit_Operation_StickerStage *pStickerStage = static_cast< CMsgPaintKit_Operation_StickerStage* >( pMsg );
			ParseOperationVariablesFromMsg( pStickerStage->add_sticker(), pNodeKV );
		}
		else
		{
			ParseOperationNodeFromMsg< CMsgPaintKit_Operation_StickerStage >( pMsg, pNodeKV );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool BParseOperationMsg( ::google::protobuf::Message* pMsg, KeyValues* pKV )
{
	if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgVarField::descriptor() ) )
	{
		ParseOperationVariable( static_cast< CMsgVarField* >( pMsg ), pKV );
	}
	else if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_TextureStage::descriptor() ) )
	{
		ParseOperationVariablesFromMsg( pMsg, pKV );
	}
	else if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_CombineStage::descriptor() ) )
	{
		ParseOperationVariablesFromMsg( pMsg, pKV );
		ParseAllOperationNodeFromMsg< CMsgPaintKit_Operation_CombineStage >( pMsg, pKV );
	}
	else if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_SelectStage::descriptor() ) )
	{
		ParseOperationVariablesFromMsg( pMsg, pKV );
	}
	else if ( BMessagesTypesAreTheSame( pMsg->GetDescriptor(), CMsgPaintKit_Operation_StickerStage::descriptor() ) )
	{
		ParseOperationVariablesFromMsg( pMsg, pKV );
		ParseAllOperationNodeFromMsg< CMsgPaintKit_Operation_StickerStage >( pMsg, pKV );
	}
	else
	{
		Assert( !"Unhandled message type" );
	}

	return true;
}


const char *GetPaintKitNameFromItem( const char *pszItemName )
{
	const char *pLast = V_strrchr( pszItemName, '_' );
	if ( pLast )
	{
		return pszItemName + ( ( pLast + 1 ) - pszItemName );
	}
	else
	{
		return pszItemName;
	}
}


#endif // CLIENT_DLL
