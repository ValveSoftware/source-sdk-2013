//========= Copyright Valve Corporation, All rights reserved. ==============================//
//
// Purpose: Gets and sets SendTable/DataMap networked properties and caches results.
//
// Code contributions by and used with the permission of L4D2 modders:
// Neil Rao (neilrao42@gmail.com)
// Raymond Nondorf (rayman1103@aol.com)
//==========================================================================================//

#include "cbase.h"
//#include "../../engine/server.h"
#include "netpropmanager.h"
#include "stdstring.h"
#include "dt_utlvector_common.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


extern void SendProxy_StringT_To_String( const SendProp *pProp, const void *pStruct, const void *pVarData, DVariant *pOut, int iElement, int objectID );
extern ISaveRestoreOps* ActivityDataOps();

const char *ArrayElementNameForIdx( size_t i )
{
	i = Clamp<size_t>( i, 0, MAX_ARRAY_ELEMENTS );
	return DT_ArrayElementNameForIdx( i );
}

//-----------------------------------------------------------------------------
CNetPropManager::~CNetPropManager()
{
	m_PropCache.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
SendProp *CNetPropManager::SearchSendTable( SendTable *pSendTable, const char *pszProperty ) const
{
	// Iterate through the send table and find the prop that we are looking for
	for ( int nPropIdx = 0; nPropIdx < pSendTable->GetNumProps(); nPropIdx++ )
	{
		SendProp *pSendProp = pSendTable->GetProp( nPropIdx );
		const char *pszPropName = pSendProp->GetName();
		// If we found the property, return the prop
		if ( pszPropName && V_strcmp( pszPropName, pszProperty ) == 0 )
		{
			// Skip the prop if it's inside an array, since we want the array itself
			if ( pSendProp->IsInsideArray() )
				continue;
			else
				return pSendProp;
		}

		// Search nested tables
		SendTable *pInternalSendTable = pSendProp->GetDataTable();
		if ( pInternalSendTable && pSendProp->GetOffset() == 0 )
		{
			pSendProp = SearchSendTable( pInternalSendTable, pszProperty );
			if ( pSendProp )
				return pSendProp;
		}
	}

	return NULL;
}


//-----------------------------------------------------------------------------
inline typedescription_t *CNetPropManager::SearchDataMap( datamap_t *pMap, const char *pszProperty ) const
{
	while ( pMap )
	{
		for ( int field = 0; field < pMap->dataNumFields; field++ )
		{
			typedescription_t *pTypeDesc = &pMap->dataDesc[field];
			const char *fieldName = pTypeDesc->fieldName;
			if ( !fieldName )
				continue;
			
			if ( V_strcmp( pszProperty, fieldName ) == 0 )
				return pTypeDesc;
			
			if ( pTypeDesc->td && pTypeDesc->fieldOffset[TD_OFFSET_NORMAL] == 0)
			{
				typedescription_t *td = SearchDataMap( pTypeDesc->td, pszProperty );
				if ( td )
					return td;
			}
		}

		pMap = pMap->baseMap;
	}
	
	return NULL; 
}

const char *s_pszBannedNetProps[]
{
	"EntityQuality",
	"AccountID",
};

//-----------------------------------------------------------------------------
inline CNetPropManager::PropInfo_t CNetPropManager::GetEntityPropInfo( CBaseEntity* pBaseEntity, const char *pszProperty, int element )
{
	for ( int i = 0; i < ARRAYSIZE( s_pszBannedNetProps ); i++ )
	{
		if ( V_stristr( pszProperty, s_pszBannedNetProps[ i ] ) != NULL)
		{
			// Replace any banned properties with a dummy string.
			pszProperty = "Y6WP5EH4I45F2LMKSDY2";
		}
	}

	ServerClass *pServerClass       = pBaseEntity->GetServerClass();
	const char  *pszServerClassName = pServerClass->GetName();
	SendTable   *pSendTable         = pServerClass->m_pTable;
	datamap_t   *pDataMap           = pBaseEntity->GetDataDescMap();

	// First, search the cache and see if the property was looked up before
	int classIdx = m_PropCache.Find( pszServerClassName );
	if ( m_PropCache.IsValidIndex( classIdx ) )
	{
		const PropInfoDict_t &properties = *(m_PropCache[ classIdx ]);
		int propIdx = properties.Find( pszProperty );
		if ( element > 0 )
		{
			char pProperty[256];
			V_snprintf( pProperty, sizeof(pProperty), "%s%d", pszProperty, element );
			propIdx = properties.Find( pProperty );
		}
		if ( properties.IsValidIndex( propIdx ) )
			return properties[ propIdx ];
	}

	CUtlStringList szPropertyList;
	char pProperty[256];
	int offset = 0;
	bool bSendTableOnly = false;
	bool bDataMapOnly = false;
	
	typedescription_t *pTypeDesc = NULL;
	SendProp *pSendProp = NULL;

	V_SplitString( pszProperty, ".", szPropertyList );

	if ( V_strcmp( szPropertyList[0], "SendTable" ) == 0 )
	{
		szPropertyList.Remove(0);
		bSendTableOnly = true;
	}
	else if ( V_strcmp( szPropertyList[0], "DataMap" ) == 0 )
	{
		szPropertyList.Remove(0);
		bDataMapOnly = true;
	}
	
	int nPropertyCount = szPropertyList.Count();
	if ( nPropertyCount )
	{
		// Search the SendTable for the prop, and if not found, search the datamap
		int iProperty = 0;
		int nRootStringLength = ( bSendTableOnly ) ? 10 : 0;

		if ( !bDataMapOnly )
		{
			while ( iProperty < nPropertyCount )
			{
				if ( !pSendTable )
				{
					pSendProp = NULL;
					break;
				}

				char *pszSearchProperty = szPropertyList[ iProperty ];
				pSendProp = SearchSendTable( pSendTable, pszSearchProperty );
				if ( !pSendProp )
				{
					if ( iProperty != nPropertyCount - 1 )
					{
						// Try the full string remainder as a single property name
						const char *pszPropertyRemainder = pszProperty + nRootStringLength;
						pSendProp = SearchSendTable( pSendTable, pszPropertyRemainder );
						if ( pSendProp )
							offset += pSendProp->GetOffset();
					}
					break;
				}

				// Handle nested properties
				++iProperty;
				offset += pSendProp->GetOffset();
				nRootStringLength += V_strlen( pszSearchProperty ) + iProperty;
				pSendTable = pSendProp->GetDataTable();
			}
		}

		if ( !pSendProp && !bSendTableOnly )
		{
			offset = 0;
			iProperty = 0;
			nRootStringLength = ( bDataMapOnly ) ? 8 : 0;

			while ( iProperty < nPropertyCount )
			{
				if ( !pDataMap )
				{
					pTypeDesc = NULL;
					break;
				}

				char *pszSearchProperty = szPropertyList[ iProperty ];
				pTypeDesc = SearchDataMap( pDataMap, pszSearchProperty );
				if ( !pTypeDesc )
				{
					if ( iProperty != nPropertyCount - 1 )
					{
						// Try the full string remainder as a single property name
						const char *pszPropertyRemainder = pszProperty + nRootStringLength;
						pTypeDesc = SearchDataMap( pDataMap, pszPropertyRemainder );
						if ( pTypeDesc )
							offset += pTypeDesc->fieldOffset[TD_OFFSET_NORMAL];
					}
					break;
				}

				// handle nested properties
				++iProperty;
				offset += pTypeDesc->fieldOffset[TD_OFFSET_NORMAL];
				nRootStringLength += V_strlen( pszSearchProperty ) + iProperty;
				pDataMap = pTypeDesc->td;
			}
		}
	}

	PropInfo_t  propInfo;
	if ( pSendProp )
	{
		if ( element < 0 )
		{
			propInfo.m_eType = Type_InvalidOrMax;
			propInfo.m_IsPropValid = false;
			return propInfo;
		}

		propInfo.m_nOffset	= offset;
		propInfo.m_nProps	= 0;
		
		int nElements = pSendProp->GetNumElements();
		NetPropType ePropType = (NetPropType)pSendProp->GetType();
		if ( ePropType == Type_DataTable )
		{
			SendTable pArrayTable = *pSendProp->GetDataTable();
			propInfo.m_nProps = pArrayTable.GetNumProps();

			if ( element >= pArrayTable.GetNumProps() )
			{
				propInfo.m_eType = Type_InvalidOrMax;
				propInfo.m_IsPropValid = false;
				return propInfo;
			}
			pSendProp = pArrayTable.GetProp( element );
			propInfo.m_nOffset += pSendProp->GetOffset();
		}
		else if ( ePropType == Type_Array )
		{
			propInfo.m_nProps = nElements;

			if ( element >= nElements )
			{
				propInfo.m_eType = Type_InvalidOrMax;
				propInfo.m_IsPropValid = false;
				return propInfo;
			}
			propInfo.m_nOffset += ( element * pSendProp->GetElementStride() );
			pSendProp = pSendProp->GetArrayProp();
			propInfo.m_nOffset += pSendProp->GetOffset();
		}

		ePropType = (NetPropType)pSendProp->GetType();
		int nBits = pSendProp->m_nBits;
		if ( ePropType == Type_String )
		{
			if ( pSendProp->GetProxyFn() != NULL )
			{
				if ( pSendProp->GetProxyFn() == &SendProxy_StringT_To_String )
					ePropType = Type_String_t;
			}
		}
		else if ( ePropType == Type_Int )
		{
			if ( nBits == NUM_NETWORKED_EHANDLE_BITS )
				ePropType = Type_EHandle;
			else if ( nBits < 2 )
				ePropType = Type_Bool;
		}

		propInfo.m_bIsSendProp = true;
		propInfo.m_eType       = ePropType;
		propInfo.m_nBitCount   = nBits;
		propInfo.m_nElements   = nElements;
		propInfo.m_nTransFlags = pSendProp->GetFlags();
		propInfo.m_IsPropValid = true;

		if ( propInfo.m_eType == Type_String )
			propInfo.m_nPropLen = DT_MAX_STRING_BUFFERSIZE;
	}
	else if ( (pTypeDesc) && (!(pTypeDesc->flags & FTYPEDESC_INPUT) && !(pTypeDesc->flags & FTYPEDESC_OUTPUT) && !(pTypeDesc->flags & FTYPEDESC_FUNCTIONTABLE)) )
	{
		if ( element < 0 || element >= pTypeDesc->fieldSize )
		{
			propInfo.m_eType = Type_InvalidOrMax;
			propInfo.m_IsPropValid = false;
			return propInfo;
		}

		if ( pTypeDesc->fieldSizeInBytes > 0 )
			offset += ( element * ( pTypeDesc->fieldSizeInBytes / pTypeDesc->fieldSize ) );

		propInfo.m_bIsSendProp		= false;
		propInfo.m_IsPropValid		= true;
		propInfo.m_nOffset			= offset;
		propInfo.m_nElements		= pTypeDesc->fieldSize;
		propInfo.m_nTransFlags		= pTypeDesc->flags;
		propInfo.m_nProps			= propInfo.m_nElements;

		switch (pTypeDesc->fieldType)
		{
		case FIELD_TICK:
		case FIELD_MODELINDEX:
		case FIELD_MATERIALINDEX:
		case FIELD_INTEGER:
		case FIELD_COLOR32:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_Int;
				break;
			}
		case FIELD_VECTOR:
		case FIELD_POSITION_VECTOR:
			{
				propInfo.m_nBitCount = 12;
				propInfo.m_eType = Type_Vector;
				break;
			}
		case FIELD_SHORT:
			{
				propInfo.m_nBitCount = 16;
				propInfo.m_eType = Type_Int;
				break;
			}
		case FIELD_BOOLEAN:
			{
				propInfo.m_nBitCount = 1;
				propInfo.m_eType = Type_Bool;
				break;
			}
		case FIELD_CHARACTER:
			{
				if (pTypeDesc->fieldSize == 1)
				{
					propInfo.m_nBitCount = 8;
					propInfo.m_eType = Type_Int;
				}
				else
				{
					propInfo.m_nBitCount = 8 * pTypeDesc->fieldSize;
					propInfo.m_eType = Type_String;
				}

				break;
			}
		case FIELD_MODELNAME:
		case FIELD_SOUNDNAME:
		case FIELD_STRING:
			{
				propInfo.m_nBitCount = sizeof(string_t);
				propInfo.m_eType = Type_String_t;
				break;
			}
		case FIELD_FLOAT:
		case FIELD_TIME:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_Float;
				break;
			}
		case FIELD_EHANDLE:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_EHandle;
				break;
			}
		case FIELD_CLASSPTR:
			{
				propInfo.m_nBitCount = 32;
				propInfo.m_eType = Type_ClassPtr;
				break;
			}
		case FIELD_CUSTOM:
			{
				if ( pTypeDesc->pSaveRestoreOps != NULL )
				{
					if ( pTypeDesc->pSaveRestoreOps == GetStdStringDataOps() )
					{
						propInfo.m_nBitCount = sizeof(std::string);
						propInfo.m_eType = Type_Std_String;
						break;
					}
					else if ( pTypeDesc->pSaveRestoreOps == ActivityDataOps() )
					{
						propInfo.m_nBitCount = 32;
						propInfo.m_eType = Type_Int;
						break;
					}
				}
				propInfo.m_IsPropValid = false;
				propInfo.m_eType = Type_InvalidOrMax;
				break;
			}
		default:
			{
				propInfo.m_IsPropValid = false;
				propInfo.m_eType = Type_InvalidOrMax;
			}
		}

		propInfo.m_nPropLen = pTypeDesc->fieldSize;
	}
	else
	{
		propInfo.m_eType = Type_InvalidOrMax;
		propInfo.m_IsPropValid = false;
		return propInfo;
	}

	// Cache the property
 	if ( !m_PropCache.IsValidIndex( classIdx ) )
	{
		classIdx = m_PropCache.Insert( pszServerClassName, new PropInfoDict_t );
	}
	PropInfoDict_t &properties = *(m_PropCache[ classIdx ]);
	if ( element > 0 )
	{
		V_snprintf( pProperty, sizeof( pProperty ), "%s%d", pszProperty, element );
		properties.Insert( pProperty, propInfo );
	}
	else
	{
		properties.Insert( pszProperty, propInfo );
	}

	return propInfo;
}


//-----------------------------------------------------------------------------
int CNetPropManager::GetPropInt( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropIntArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
int CNetPropManager::GetPropIntArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	// Get the base entity of the specified index
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return -1;

	// Find the requested property info (this will throw if the entity is
	// invalid, which is exactly what we want)
	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	// Property must be valid
	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Int && propInfo.m_eType != Type_Bool && propInfo.m_eType != Type_EHandle && propInfo.m_eType != Type_ClassPtr) )
		return -1;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return -1;
	}

	// All sendprops store an offset from the pointer to the base entity to
	// where the prop data actually is; the reason is because the engine needs
	// to relay data very quickly to all the clients, so it works with
	// offsets to make the ordeal faster
	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;
	bool bUnsigned = propInfo.m_nTransFlags & SPROP_UNSIGNED;

	// Thanks to SM for figuring out the types to use for bit counts.
	// All we are doing below is looking at how many bits are in the prop.
	// Since some values can be shorts, longs, ints, signed/unsigned,
	// boolean, etc., so we need to decipher exactly what the SendProp info
	// tells us in order to properly retrieve the right number of bytes.
	if (propInfo.m_nBitCount >= 17)
	{
		return *(int32 *)pEntityPropData;
	}
	else if (propInfo.m_nBitCount >= 9)
	{
		if (bUnsigned)
			return *(uint16 *)pEntityPropData;
		else
			return *(int16 *)pEntityPropData;
	}
	else if (propInfo.m_nBitCount >= 2)
	{
		if (bUnsigned)
			return *(uint8 *)pEntityPropData;
		else
			return *(int8 *)pEntityPropData;
	}
	else
	{
		return *(bool *)(pEntityPropData) ? 1 : 0;
	}

	return 0;
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropInt( HSCRIPT hEnt, const char *pszProperty, int value )
{
	SetPropIntArray( hEnt, pszProperty, value, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropIntArray( HSCRIPT hEnt, const char *pszProperty, int value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Int && propInfo.m_eType != Type_Bool && propInfo.m_eType != Type_EHandle && propInfo.m_eType != Type_ClassPtr) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;
	bool bUnsigned = propInfo.m_nTransFlags & SPROP_UNSIGNED;

	if (propInfo.m_nBitCount >= 17)
	{
		*(int32 *)pEntityPropData = (int32)value;
	}
	else if (propInfo.m_nBitCount >= 9)
	{
		if (bUnsigned)
			*(uint16 *)pEntityPropData = (uint16)value;
		else
			*(int16 *)pEntityPropData = (int16)value;
	}
	else if (propInfo.m_nBitCount >= 2)
	{
		if (bUnsigned)
			*(uint8 *)pEntityPropData = (uint8)value;
		else
			*(int8 *)pEntityPropData = (int8)value;
	}
	else
	{
		*(bool *)pEntityPropData = value ? true : false;
	}

	// Network the prop change to connected clients (otherwise the network state won't
	// be updated until the engine re-transmits the entire table)
	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
float CNetPropManager::GetPropFloat( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropFloatArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
float CNetPropManager::GetPropFloatArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return -1.0f;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Float) )
		return -1.0f;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return -1.0f;
	}

	return *(float *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropFloat( HSCRIPT hEnt, const char *pszProperty, float value )
{
	SetPropFloatArray( hEnt, pszProperty, value, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropFloatArray( HSCRIPT hEnt, const char *pszProperty, float value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Float) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	*(float *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset) = value;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
HSCRIPT CNetPropManager::GetPropEntity( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropEntityArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
HSCRIPT CNetPropManager::GetPropEntityArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return NULL;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_EHandle && propInfo.m_eType != Type_ClassPtr) )
		return NULL;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return NULL;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;

	CBaseEntity *pPropEntity = NULL;
	if ( propInfo.m_eType == Type_EHandle )
	{
		CBaseHandle &baseHandle = *(CBaseHandle *)pEntityPropData;
		pPropEntity = CBaseEntity::Instance( baseHandle );
	}
	else
		pPropEntity = *(CBaseEntity **)pEntityPropData;

	return ToHScript( pPropEntity );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropEntity( HSCRIPT hEnt, const char *pszProperty, HSCRIPT hPropEnt )
{
	SetPropEntityArray( hEnt, pszProperty, hPropEnt, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropEntityArray( HSCRIPT hEnt, const char *pszProperty, HSCRIPT hPropEnt, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_EHandle && propInfo.m_eType != Type_ClassPtr) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;

	CBaseEntity *pOtherEntity = ToEnt( hPropEnt );
	if ( propInfo.m_eType == Type_EHandle )
	{
		CBaseHandle &baseHandle = *(CBaseHandle *)pEntityPropData;
		if ( !pOtherEntity )
			baseHandle.Set( NULL );
		else
			baseHandle.Set( (IHandleEntity *)pOtherEntity );
	}
	else
		*(CBaseEntity **)pEntityPropData = pOtherEntity;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
const Vector& CNetPropManager::GetPropVector( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropVectorArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
const Vector& CNetPropManager::GetPropVectorArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	static Vector vAng = Vector(0, 0, 0);
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return vAng;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Vector) )
		return vAng;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return vAng;
	}

	vAng = *(Vector *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	return vAng;
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropVector( HSCRIPT hEnt, const char *pszProperty, Vector value )
{
	SetPropVectorArray( hEnt, pszProperty, value, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropVectorArray( HSCRIPT hEnt, const char *pszProperty, Vector value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Vector) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	Vector *pVec = (Vector *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
	pVec->x = value.x;
	pVec->y = value.y;
	pVec->z = value.z;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
const char *CNetPropManager::GetPropString( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropStringArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
const char *CNetPropManager::GetPropStringArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return "";

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_String && propInfo.m_eType != Type_String_t && propInfo.m_eType != Type_Int && propInfo.m_eType != Type_Std_String) )
		return "";

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return "";
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;

	if ( propInfo.m_eType == Type_Std_String )
	{
		std::string *propString = (std::string *)pEntityPropData;
		return (propString->empty()) ? "" : propString->c_str();
	}
	else if ( propInfo.m_eType == Type_String_t )
	{
		string_t propString = *(string_t *)pEntityPropData;
		return (propString == NULL_STRING) ? "" : STRING(propString);
	}
	else
	{
		return (const char *)pEntityPropData;
	}
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropString( HSCRIPT hEnt, const char *pszProperty, const char *value )
{
	SetPropStringArray( hEnt, pszProperty, value, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropStringArray( HSCRIPT hEnt, const char *pszProperty, const char *value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_String && propInfo.m_eType != Type_String_t && propInfo.m_eType != Type_Std_String) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset;

	if ( propInfo.m_eType == Type_Std_String )
	{
		std::string *propString = (std::string *)pEntityPropData;
		propString->assign(value);
	}
	else if ( propInfo.m_eType == Type_String_t )
	{
		*(string_t *)pEntityPropData = AllocPooledString(value);
	}
	else
	{
		char* strDest = (char *)pEntityPropData;
		V_strncpy( strDest, value, propInfo.m_nPropLen );
	}

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
int CNetPropManager::GetPropArraySize( HSCRIPT hEnt, const char *pszProperty )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if (!pBaseEntity)
		return -1;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

	if ( (!propInfo.m_IsPropValid) )
		return -1;

	return propInfo.m_nProps;
}


//-----------------------------------------------------------------------------
bool CNetPropManager::HasProp( HSCRIPT hEnt, const char *pszProperty )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return false;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

	return ( propInfo.m_IsPropValid ) ? true : false;
}


//-----------------------------------------------------------------------------
const char *CNetPropManager::GetPropType( HSCRIPT hEnt, const char *pszProperty )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return NULL;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, 0 );

	if ( !propInfo.m_IsPropValid )
		return NULL;

	switch (propInfo.m_eType)
	{
	case Type_Int:
			return "integer";
	case Type_Float:
			return "float";
	case Type_Vector:
			return "Vector";
	case Type_VectorXY:
			return "VectorXY";
	case Type_String:
	case Type_String_t:
	case Type_Std_String:
			return "string";
	case Type_Array:
			return "array";
	case Type_DataTable:
			return "table";
	case Type_Bool:
			return "bool";
	case Type_EHandle:
	case Type_ClassPtr:
			return "instance";
	#ifdef SUPPORTS_INT64
		case Type_Int64:
			return "integer64";
	#endif
	}

	return NULL;
}


//-----------------------------------------------------------------------------
bool CNetPropManager::GetPropBool( HSCRIPT hEnt, const char *pszProperty )
{
	return GetPropBoolArray( hEnt, pszProperty, 0 );
}


//-----------------------------------------------------------------------------
bool CNetPropManager::GetPropBoolArray( HSCRIPT hEnt, const char *pszProperty, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return false;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Bool) )
		return false;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return false;
	}

	return *(bool *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset);
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropBool( HSCRIPT hEnt, const char *pszProperty, bool value )
{
	SetPropBoolArray( hEnt, pszProperty, value, 0 );
}


//-----------------------------------------------------------------------------
void CNetPropManager::SetPropBoolArray( HSCRIPT hEnt, const char *pszProperty, bool value, int element )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity )
		return;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( (!propInfo.m_IsPropValid) || (propInfo.m_eType != Type_Bool) )
		return;

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) && propInfo.m_bIsSendProp )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	*(bool *)((uint8 *)pBaseEntityOrGameRules + propInfo.m_nOffset) = value ? true : false;

	if ( propInfo.m_bIsSendProp )
	{
		pBaseEntity->edict()->StateChanged( propInfo.m_nOffset );
	}
}


//-----------------------------------------------------------------------------
bool CNetPropManager::GetPropInfo( HSCRIPT hEnt, const char *pszProperty, int element, HSCRIPT hTable )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity || !hTable )
		return false;

	PropInfo_t propInfo = GetEntityPropInfo( pBaseEntity, pszProperty, element );

	if ( !propInfo.m_IsPropValid )
		return false;

	g_pScriptVM->SetValue( hTable, "is_sendprop", propInfo.m_bIsSendProp );
	g_pScriptVM->SetValue( hTable, "type", propInfo.m_eType );
	g_pScriptVM->SetValue( hTable, "bits", propInfo.m_nBitCount );
	g_pScriptVM->SetValue( hTable, "elements", propInfo.m_nElements );
	g_pScriptVM->SetValue( hTable, "offset", propInfo.m_nOffset );
	g_pScriptVM->SetValue( hTable, "length", (propInfo.m_nPropLen > 0) ? propInfo.m_nPropLen : 1 );
	g_pScriptVM->SetValue( hTable, "array_props", propInfo.m_nProps );
	g_pScriptVM->SetValue( hTable, "flags", propInfo.m_nTransFlags );

	return true;
}


//-----------------------------------------------------------------------------
void CNetPropManager::StoreSendPropValue( SendProp *pSendProp, CBaseEntity *pBaseEntity, int iOffset, int iElement, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	const char *pszPropName = pSendProp->GetName();
	if ( iElement > -1 )
		pszPropName = ArrayElementNameForIdx( iElement );

	void *pBaseEntityOrGameRules = pBaseEntity;
	if ( dynamic_cast<CGameRulesProxy*>(pBaseEntity) )
	{
		pBaseEntityOrGameRules = GameRules();
		if ( !pBaseEntityOrGameRules )
			return;
	}

	uint8 *pEntityPropData = (uint8 *)pBaseEntityOrGameRules + iOffset + pSendProp->GetOffset();

	switch (pSendProp->GetType())
	{
	case DPT_Int:
		{
			int nBits = pSendProp->m_nBits;
			if (nBits == 21)
			{
				CBaseHandle &baseHandle = *(CBaseHandle *)pEntityPropData;
				CBaseEntity *pPropEntity = CBaseEntity::Instance( baseHandle );
				if ( pPropEntity )
					g_pScriptVM->SetValue( hTable, pszPropName, ToHScript( pPropEntity ) );
				else
					g_pScriptVM->SetValue( hTable, pszPropName, NULL );
			}
			else if (nBits >= 17)
			{
				g_pScriptVM->SetValue( hTable, pszPropName, *(int32 *)pEntityPropData );
			}
			else if (nBits >= 9)
			{
				if (!pSendProp->IsSigned())
					g_pScriptVM->SetValue( hTable, pszPropName, *(uint16 *)pEntityPropData );
				else
					g_pScriptVM->SetValue( hTable, pszPropName, *(int16 *)pEntityPropData );
			}
			else if (nBits >= 2)
			{
				if (!pSendProp->IsSigned())
					g_pScriptVM->SetValue( hTable, pszPropName, *(uint8 *)pEntityPropData );
				else
					g_pScriptVM->SetValue( hTable, pszPropName, *(int8 *)pEntityPropData );
			}
			else
			{
				g_pScriptVM->SetValue( hTable, pszPropName, *(bool *)pEntityPropData );
			}
			break;
		}
	case DPT_Float:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(float *)(uint8 *)pEntityPropData );
			break;
		}
	case DPT_Vector:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(Vector *)(uint8 *)pEntityPropData );
			break;
		}
	case DPT_String:
		{
			if ( pSendProp->GetProxyFn() != NULL )
			{
				if ( pSendProp->GetProxyFn() == &SendProxy_StringT_To_String )
				{
					string_t propString = *(string_t *)(uint8 *)pEntityPropData;
					g_pScriptVM->SetValue( hTable, pszPropName, (propString == NULL_STRING) ? "" : STRING(propString) );
					break;
				}
			}

			g_pScriptVM->SetValue( hTable, pszPropName, (const char *)(uint8 *)pEntityPropData );
			break;
		}
	}
}


//-----------------------------------------------------------------------------
void CNetPropManager::StoreDataPropValue( typedescription_t *pTypeDesc, CBaseEntity *pBaseEntity, int iOffset, int iElement, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	const char *pszPropName = pTypeDesc->fieldName;
	if ( iElement > -1 )
		pszPropName = ArrayElementNameForIdx( iElement );

	uint8 *pEntityPropData = (uint8 *)pBaseEntity + iOffset + pTypeDesc->fieldOffset[TD_OFFSET_NORMAL];

	switch (pTypeDesc->fieldType)
	{
	case FIELD_TICK:
	case FIELD_MODELINDEX:
	case FIELD_MATERIALINDEX:
	case FIELD_INTEGER:
	case FIELD_COLOR32:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(int32 *)pEntityPropData );
			break;
		}
	case FIELD_VECTOR:
	case FIELD_POSITION_VECTOR:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(Vector *)(uint8 *)pEntityPropData );
			break;
		}
	case FIELD_SHORT:
		{
			if ( pTypeDesc->flags & SPROP_UNSIGNED )
				g_pScriptVM->SetValue( hTable, pszPropName, *(uint16 *)pEntityPropData );
			else
				g_pScriptVM->SetValue( hTable, pszPropName, *(int16 *)pEntityPropData );
			break;
		}
	case FIELD_BOOLEAN:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(bool *)pEntityPropData );
			break;
		}
	case FIELD_CHARACTER:
		{
			if (pTypeDesc->fieldSize == 1)
			{
				if ( pTypeDesc->flags & SPROP_UNSIGNED )
					g_pScriptVM->SetValue( hTable, pszPropName, *(uint8 *)pEntityPropData );
				else
					g_pScriptVM->SetValue( hTable, pszPropName, *(int8 *)pEntityPropData );
			}
			else
			{
				g_pScriptVM->SetValue( hTable, pszPropName, (const char *)(uint8 *)pEntityPropData );
			}

			break;
		}
	case FIELD_MODELNAME:
	case FIELD_SOUNDNAME:
	case FIELD_STRING:
		{
			string_t propString = *(string_t *)(uint8 *)pEntityPropData;
			g_pScriptVM->SetValue( hTable, pszPropName, (propString == NULL_STRING) ? "" : STRING(propString) );
			break;
		}
	case FIELD_FLOAT:
	case FIELD_TIME:
		{
			g_pScriptVM->SetValue( hTable, pszPropName, *(float *)(uint8 *)pEntityPropData );
			break;
		}
	case FIELD_EHANDLE:
		{
			CBaseHandle &baseHandle = *(CBaseHandle *)pEntityPropData;
			CBaseEntity *pPropEntity = CBaseEntity::Instance( baseHandle );
			if ( pPropEntity )
				g_pScriptVM->SetValue( hTable, pszPropName, ToHScript( pPropEntity ) );
			else
				g_pScriptVM->SetValue( hTable, pszPropName, NULL );
			break;
		}
	case FIELD_CLASSPTR:
		{
			CBaseEntity *pPropEntity = *(CBaseEntity **)pEntityPropData;
			if ( pPropEntity )
				g_pScriptVM->SetValue( hTable, pszPropName, ToHScript( pPropEntity ) );
			else
				g_pScriptVM->SetValue( hTable, pszPropName, NULL );
			break;
		}
	case FIELD_CUSTOM:
		{
			if ( pTypeDesc->pSaveRestoreOps != NULL )
			{
				if ( pTypeDesc->pSaveRestoreOps == GetStdStringDataOps() )
				{
					std::string *propString = (std::string *)pEntityPropData;
					g_pScriptVM->SetValue( hTable, pszPropName, (propString->empty()) ? "" : propString->c_str() );
				}
				else if ( pTypeDesc->pSaveRestoreOps == ActivityDataOps() )
				{
					g_pScriptVM->SetValue( hTable, pszPropName, *(int *)pEntityPropData );
				}
			}
			break;
		}
	}
}


//-----------------------------------------------------------------------------
void CNetPropManager::CollectNestedSendProps( SendTable *pSendTable, CBaseEntity *pBaseEntity, int iOffset, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	for ( int nPropIdx = 0; nPropIdx < pSendTable->GetNumProps(); nPropIdx++ )
	{
		SendProp *pSendProp = pSendTable->GetProp( nPropIdx );
		if ( pSendProp->IsExcludeProp() )
			continue;

		const char *pszPropName = pSendProp->GetName();
		SendTable *pInternalSendTable = pSendProp->GetDataTable();
		if ( pInternalSendTable )
		{
			if ( V_strcmp( pSendProp->GetName(), "baseclass" ) == 0 )
				pszPropName = pInternalSendTable->m_pNetTableName;

			ScriptVariant_t hPropTable;
			g_pScriptVM->CreateTable( hPropTable );
			CollectNestedSendProps( pInternalSendTable, pBaseEntity, (iOffset + pSendProp->GetOffset()), hPropTable );
			g_pScriptVM->SetValue( hTable, pszPropName, hPropTable );
			g_pScriptVM->ReleaseValue( hPropTable );
		}
		else
		{
			if ( pSendProp->GetType() == DPT_Array )
			{
				SendProp *pArrayProp = pSendProp->GetArrayProp();
				ScriptVariant_t hPropTable;
				g_pScriptVM->CreateTable( hPropTable );

				for ( int element = 0; element < pSendProp->GetNumElements(); element++ )
				{
					StoreSendPropValue( pArrayProp, pBaseEntity, iOffset + ( element * pSendProp->GetElementStride() ), element, hPropTable );
				}

				g_pScriptVM->SetValue( hTable, pszPropName, hPropTable );
				g_pScriptVM->ReleaseValue( hPropTable );
			}
			else
				StoreSendPropValue( pSendProp, pBaseEntity, iOffset, -1, hTable );
		}
	}
}


//-----------------------------------------------------------------------------
void CNetPropManager::CollectNestedDataMaps( datamap_t *pMap, CBaseEntity *pBaseEntity, int iOffset, HSCRIPT hTable )
{
	if ( !hTable )
		return;

	while ( pMap )
	{
		for ( int field = 0; field < pMap->dataNumFields; field++ )
		{
			typedescription_t *pTypeDesc = &pMap->dataDesc[field];
			if ( pTypeDesc->flags & FTYPEDESC_INPUT || pTypeDesc->flags & FTYPEDESC_OUTPUT || pTypeDesc->flags & FTYPEDESC_FUNCTIONTABLE )
				continue;

			if ( pTypeDesc->td )
			{
				ScriptVariant_t hPropTable;
				g_pScriptVM->CreateTable( hPropTable );
				CollectNestedDataMaps( pTypeDesc->td, pBaseEntity, (iOffset + pTypeDesc->fieldOffset[TD_OFFSET_NORMAL]), hPropTable);
				g_pScriptVM->SetValue( hTable, pTypeDesc->fieldName, hPropTable );
				g_pScriptVM->ReleaseValue( hPropTable );
			}
			else
			{
				if ( pTypeDesc->fieldSize > 1 )
				{
					ScriptVariant_t hPropTable;
					g_pScriptVM->CreateTable( hPropTable );

					for ( int element = 0; element < pTypeDesc->fieldSize; element++ )
					{
						StoreDataPropValue( pTypeDesc, pBaseEntity, iOffset + ( element * ( pTypeDesc->fieldSizeInBytes / pTypeDesc->fieldSize ) ), element, hPropTable );
					}

					g_pScriptVM->SetValue( hTable, pTypeDesc->fieldName, hPropTable );
					g_pScriptVM->ReleaseValue( hPropTable );
				}
				else
					StoreDataPropValue( pTypeDesc, pBaseEntity, iOffset, -1, hTable );
			}
		}

		pMap = pMap->baseMap;
	}
}


//-----------------------------------------------------------------------------
void CNetPropManager::GetTable( HSCRIPT hEnt, int iPropType, HSCRIPT hTable )
{
	CBaseEntity *pBaseEntity = ToEnt( hEnt );
	if ( !pBaseEntity || !hTable )
		return;

	if ( !iPropType )
	{
		ServerClass *pServerClass = pBaseEntity->GetServerClass();
		SendTable   *pSendTable = pServerClass->m_pTable;
		CollectNestedSendProps( pSendTable, pBaseEntity, 0, hTable );
	}
	else
	{
		datamap_t   *pDataMap = pBaseEntity->GetDataDescMap();
		CollectNestedDataMaps( pDataMap, pBaseEntity, 0, hTable );
	}
}