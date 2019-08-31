//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

char *Datadesc_SetFieldString( const char *szValue, CBaseEntity *pObject, typedescription_t *pField, fieldtype_t *pFieldType = NULL );

bool ReadUnregisteredKeyfields( CBaseEntity *pTarget, const char *szKeyName, variant_t *variant );
