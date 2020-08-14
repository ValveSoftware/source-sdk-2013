//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "datadesc_mod.h"
#include "saverestore.h"


// Sets a field's value to a specific string.
char *Datadesc_SetFieldString( const char *szValue, CBaseEntity *pObject, typedescription_t *pField, fieldtype_t *pFieldType )
{
	// Copied from ::ParseKeyvalue...
	fieldtype_t fieldtype = FIELD_VOID;
	int fieldOffset = pField->fieldOffset[ TD_OFFSET_NORMAL ];
	switch( pField->fieldType )
	{
	case FIELD_MODELNAME:
	case FIELD_SOUNDNAME:
	case FIELD_STRING:
		(*(string_t *)((char *)pObject + fieldOffset)) = AllocPooledString( szValue );
		fieldtype = FIELD_STRING;
		break;

	case FIELD_TIME:
	case FIELD_FLOAT:
		(*(float *)((char *)pObject + fieldOffset)) = atof( szValue );
		fieldtype = FIELD_FLOAT;
		break;

	case FIELD_BOOLEAN:
		(*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi( szValue ) != 0);
		fieldtype = FIELD_BOOLEAN;
		break;

	case FIELD_CHARACTER:
		(*(char *)((char *)pObject + fieldOffset)) = (char)atoi( szValue );
		fieldtype = FIELD_CHARACTER;
		break;

	case FIELD_SHORT:
		(*(short *)((char *)pObject + fieldOffset)) = (short)atoi( szValue );
		fieldtype = FIELD_SHORT;
		break;

	case FIELD_INTEGER:
	case FIELD_TICK:
		(*(int *)((char *)pObject + fieldOffset)) = atoi( szValue );
		fieldtype = FIELD_INTEGER;
		break;

	case FIELD_POSITION_VECTOR:
	case FIELD_VECTOR:
		UTIL_StringToVector( (float *)((char *)pObject + fieldOffset), szValue );
		fieldtype = FIELD_VECTOR;
		break;

	case FIELD_VMATRIX:
	case FIELD_VMATRIX_WORLDSPACE:
		UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
		fieldtype = FIELD_VMATRIX; // ???
		break;

	case FIELD_MATRIX3X4_WORLDSPACE:
		UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
		fieldtype = FIELD_VMATRIX; // ???
		break;

	case FIELD_COLOR32:
		UTIL_StringToColor32( (color32 *) ((char *)pObject + fieldOffset), szValue );
		fieldtype = FIELD_COLOR32;
		break;

	case FIELD_CUSTOM:
	{
		SaveRestoreFieldInfo_t fieldInfo =
		{
			(char *)pObject + fieldOffset,
			pObject,
			pField
		};
		pField->pSaveRestoreOps->Parse( fieldInfo, szValue );
		fieldtype = FIELD_STRING;
		break;
	}

	default:
	case FIELD_INTERVAL:
	case FIELD_CLASSPTR:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_EDICT:
		return NULL;
		//Warning( "%s cannot set field of type %i.\n", GetDebugName(), dmap->dataDesc[i].fieldType );
		break;
	}

	if (pFieldType)
		*pFieldType = fieldtype;

	return ((char*)pObject) + fieldOffset;
}

//-----------------------------------------------------------------------------
// Purpose: ReadUnregisteredKeyfields() was a feeble attempt to obtain non-keyfield keyvalues from KeyValue() with variant_t.
// 
// I didn't know about GetKeyValue() until 9/29/2018.
// I don't remember why I decided to write down the date I found out about it. Maybe I considered that monumental of a discovery.
// 
// However, we still use ReadUnregisteredKeyfields() since GetKeyValue() only supports a string while this function was used for entire variant_ts.
// It now calls GetKeyValue() and returns it as an allocated string.
//-----------------------------------------------------------------------------
bool ReadUnregisteredKeyfields(CBaseEntity *pTarget, const char *szKeyName, variant_t *variant)
{
	if (!pTarget)
		return false;

	char szValue[256];
	if (pTarget->GetKeyValue(szKeyName, szValue, sizeof(szValue)))
	{
		variant->SetString(AllocPooledString(szValue)); // MAKE_STRING causes badness, must pool
		return true;
	}

#if 0
	if ( FStrEq( szKeyName, "targetname" ) )
	{
		variant->SetString(pTarget->GetEntityName());
		return true;
	}

	if( FStrEq( szKeyName, "origin" ) )
	{
		variant->SetPositionVector3D(pTarget->GetAbsOrigin());
		return true;
	}

	if( FStrEq( szKeyName, "angles" ) /*|| FStrEq( szKeyName, "angle" )*/ )
	{
		Vector angles;
		AngleVectors(pTarget->GetAbsAngles(), &angles);
		variant->SetVector3D(angles);
		return true;
	}

	if ( FStrEq( szKeyName, "rendercolor" ) || FStrEq( szKeyName, "rendercolor32" ))
	{
		// Copy it over since we're not going to use the alpha
		color32 theircolor = pTarget->GetRenderColor();
		color32 color;
		color.r = theircolor.r;
		color.g = theircolor.g;
		color.b = theircolor.b;
		variant->SetColor32(color);
		return true;
	}
	
	if ( FStrEq( szKeyName, "renderamt" ) )
	{
		char szAlpha = pTarget->GetRenderColor().a;
		variant->SetString(MAKE_STRING(&szAlpha));
		return true;
	}

	if ( FStrEq( szKeyName, "disableshadows" ))
	{
		variant->SetBool((pTarget->GetEffects() & EF_NOSHADOW) != NULL);
		return true;
	}

	if ( FStrEq( szKeyName, "mins" ))
	{
		variant->SetVector3D(pTarget->CollisionProp()->OBBMinsPreScaled());
		return true;
	}

	if ( FStrEq( szKeyName, "maxs" ))
	{
		variant->SetVector3D(pTarget->CollisionProp()->OBBMaxsPreScaled());
		return true;
	}

	if ( FStrEq( szKeyName, "disablereceiveshadows" ))
	{
		variant->SetBool((pTarget->GetEffects() & EF_NORECEIVESHADOW) != NULL);
		return true;
	}

	if ( FStrEq( szKeyName, "nodamageforces" ))
	{
		variant->SetBool((pTarget->GetEFlags() & EFL_NO_DAMAGE_FORCES) != NULL);
		return true;
	}
#endif

	return false;
}
