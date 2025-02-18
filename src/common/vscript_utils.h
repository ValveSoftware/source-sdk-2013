//===== Copyright � 1996-2011, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef COMMON_VSCRIPT_UTILS_H
#define COMMON_VSCRIPT_UTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"
#include "fmtstr.h"

#define KV_VARIANT_SCRATCH_BUF_SIZE 128

inline KeyValues * ScriptTableToKeyValues( IScriptVM *pVM, char const *szName, HSCRIPT hTable );
inline HSCRIPT ScriptTableFromKeyValues( IScriptVM *pVM, KeyValues *kv, HSCRIPT hTable = INVALID_HSCRIPT );

inline bool ScriptVmKeyValueToVariant( IScriptVM *pVM, KeyValues *val, ScriptVariant_t &varValue, char chScratchBuffer[KV_VARIANT_SCRATCH_BUF_SIZE] )
{
	switch ( val->GetDataType() )
	{
	case KeyValues::TYPE_STRING:
		varValue = val->GetString();
		return true;
	case KeyValues::TYPE_INT:
		varValue = val->GetInt();
		return true;
	case KeyValues::TYPE_FLOAT:
		varValue = val->GetFloat();
		return true;
	case KeyValues::TYPE_UINT64:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%llu", val->GetUint64() );
		varValue = chScratchBuffer;
		return true;
	case KeyValues::TYPE_NONE:
		varValue = ScriptTableFromKeyValues( pVM, val );
		return true;
	default:
		Warning( "ScriptVmKeyValueToVariant failed to package parameter %s (type %d)\n", val->GetName(), val->GetDataType() );
		return false;
	}
}

inline bool ScriptVmStringFromVariant( ScriptVariant_t &varValue, char chScratchBuffer[KV_VARIANT_SCRATCH_BUF_SIZE] )
{
	switch ( varValue.GetType() )
	{
	case FIELD_INTEGER:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%d", ( int ) varValue );
		return true;
	case FIELD_FLOAT:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%f", ( float ) varValue );
		return true;
	case FIELD_BOOLEAN:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%d", ( bool ) varValue );
		return true;
	case FIELD_CHARACTER:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%c", ( char ) varValue );
		return true;
	case FIELD_CSTRING:
		V_snprintf( chScratchBuffer, KV_VARIANT_SCRATCH_BUF_SIZE, "%s", ( const char * ) varValue );
		return true;
	default:
		Warning( "ScriptVmStringFromVariant failed to unpack parameter variant type %d\n", varValue.GetType() );
		return false;
	}
}

inline KeyValues * ScriptVmKeyValueFromVariant( IScriptVM *pVM, ScriptVariant_t &varValue )
{
	KeyValues *val = NULL;

	switch ( varValue.GetType() )
	{
	case FIELD_INTEGER:
		val = new KeyValues( "" );
		val->SetInt( NULL, ( int ) varValue );
		return val;
	case FIELD_FLOAT:
		val = new KeyValues( "" );
		val->SetFloat( NULL, ( float ) varValue );
		return val;
	case FIELD_BOOLEAN:
		val = new KeyValues( "" );
		val->SetInt( NULL, ( ( bool ) varValue ) ? 1 : 0 );
		return val;
	case FIELD_CHARACTER:
		val = new KeyValues( "" );
		val->SetString( NULL, CFmtStr( "%c", ( char ) varValue ) );
		return val;
	case FIELD_CSTRING:
		val = new KeyValues( "" );
		val->SetString( NULL, ( char const * ) varValue );
		return val;
	case FIELD_HSCRIPT:
		return ScriptTableToKeyValues( pVM, "", ( HSCRIPT ) varValue );
	default:
		Warning( "ScriptVmKeyValueFromVariant failed to unpack parameter variant type %d\n", varValue.GetType() );
		return NULL;
	}
}

inline KeyValues * ScriptTableToKeyValues( IScriptVM *pVM, char const *szName, HSCRIPT hTable )
{
	if ( !szName )
		szName = "";
	
	KeyValues *kv = new KeyValues( szName );

	if ( hTable && pVM )
	{
		ScriptVariant_t varKey, varValue;
		for ( int nIterator = 0;
			( nIterator = pVM->GetKeyValue( hTable, nIterator, &varKey, &varValue ) ) != -1;
			pVM->ReleaseValue( varKey ), pVM->ReleaseValue( varValue ) )
		{
			char chScratchBuffer[ KV_VARIANT_SCRATCH_BUF_SIZE ] = {0};
			ScriptVmStringFromVariant( varKey, chScratchBuffer );

			if ( !chScratchBuffer[0] )
			{
				Assert( 0 );
				continue;
			}

			KeyValues *sub = ScriptVmKeyValueFromVariant( pVM, varValue );
			if ( !sub )
			{
				Assert( 0 );
				// sub->deleteThis();
				// continue;
				// still proceed - it will be a key with no data
				sub = new KeyValues( "" );
			}
			sub->SetName( chScratchBuffer );
			
			kv->AddSubKey( sub );
		}
	}

	return kv;
}

inline HSCRIPT ScriptTableFromKeyValues( IScriptVM *pVM, KeyValues *kv, HSCRIPT hTable )
{
	if ( !kv || !pVM )
		return NULL;

	if ( hTable == INVALID_HSCRIPT )
	{
		ScriptVariant_t varTable;
		pVM->CreateTable( varTable );
		hTable = varTable;
	}

	for ( KeyValues *val = kv->GetFirstSubKey(); val; val = val->GetNextKey() )
	{
		ScriptVariant_t varValue;
		char chScratchBuffer[ KV_VARIANT_SCRATCH_BUF_SIZE ];
		if ( !ScriptVmKeyValueToVariant( pVM, val, varValue, chScratchBuffer ) )
			continue;

#ifdef SCRIPTTABLE_SCRIPT_LOWERCASE_ALL_TABLE_KEYS
		char chNameBuffer[ KV_VARIANT_SCRATCH_BUF_SIZE ];
		V_strncpy( chNameBuffer, val->GetName(), KV_VARIANT_SCRATCH_BUF_SIZE );
		V_strlower( chNameBuffer );
#else
		char const *chNameBuffer = val->GetName();
#endif

		pVM->SetValue( hTable, chNameBuffer, varValue );
	}

	return hTable;
}



#endif // COMMON_VSCRIPT_UTILS_H
