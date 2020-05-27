//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VARIANT_T_H
#define VARIANT_T_H
#ifdef _WIN32
#pragma once
#endif


#include "ehandle.h"
#include "mathlib/vmatrix.h"

class CBaseEntity;

#ifdef MAPBASE_VSCRIPT
struct ScriptVariant_t;
#endif


//
// A variant class for passing data in entity input/output connections.
//
class variant_t
{
	union
	{
		bool bVal;
		string_t iszVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	CHandle<CBaseEntity> eVal; // this can't be in the union because it has a constructor.

	fieldtype_t fieldType;

public:

	// constructor
	variant_t() : fieldType(FIELD_VOID), iVal(0) {}

	inline bool Bool( void ) const						{ return( fieldType == FIELD_BOOLEAN ) ? bVal : false; }
	inline const char *String( void ) const				{ return( fieldType == FIELD_STRING ) ? STRING(iszVal) : ToString(); }
	inline string_t StringID( void ) const				{ return( fieldType == FIELD_STRING ) ? iszVal : NULL_STRING; }
	inline int Int( void ) const						{ return( fieldType == FIELD_INTEGER ) ? iVal : 0; }
	inline float Float( void ) const					{ return( fieldType == FIELD_FLOAT ) ? flVal : 0; }
	inline const CHandle<CBaseEntity> &Entity(void) const;
	inline color32 Color32(void) const					{ return rgbaVal; }
	inline void Vector3D(Vector &vec) const;
#ifdef MAPBASE
	// Gets angles from a vector
	inline void Angle3D(QAngle &ang) const;
#endif

	fieldtype_t FieldType( void ) { return fieldType; }

	void SetBool( bool b ) { bVal = b; fieldType = FIELD_BOOLEAN; }
	void SetString( string_t str ) { iszVal = str, fieldType = FIELD_STRING; }
	void SetInt( int val ) { iVal = val, fieldType = FIELD_INTEGER; }
	void SetFloat( float val ) { flVal = val, fieldType = FIELD_FLOAT; }
	void SetEntity( CBaseEntity *val );
	void SetVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_VECTOR; }
	void SetPositionVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_POSITION_VECTOR; }
#ifdef MAPBASE
	// Passes in angles as a vector
	void SetAngle3D( const QAngle &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_VECTOR; }
#endif
	void SetColor32( color32 val ) { rgbaVal = val; fieldType = FIELD_COLOR32; }
	void SetColor32( int r, int g, int b, int a ) { rgbaVal.r = r; rgbaVal.g = g; rgbaVal.b = b; rgbaVal.a = a; fieldType = FIELD_COLOR32; }
	void Set( fieldtype_t ftype, void *data );
	void SetOther( void *data );
	bool Convert( fieldtype_t newType );
#ifdef MAPBASE
	// Special conversion specifically for FIELD_EHANDLE with !activator, etc.
	bool Convert( fieldtype_t newType, CBaseEntity *pSelf, CBaseEntity *pActivator, CBaseEntity *pCaller );
	// Hands over the value + the field type.
	// ex: "Otis (String)", "3 (Integer)", or "npc_combine_s (Entity)"
	const char *GetDebug();
#endif

#ifdef MAPBASE_VSCRIPT
	void SetScriptVariant( ScriptVariant_t &var );
#endif

	static typedescription_t m_SaveBool[];
	static typedescription_t m_SaveInt[];
	static typedescription_t m_SaveFloat[];
	static typedescription_t m_SaveEHandle[];
	static typedescription_t m_SaveString[];
	static typedescription_t m_SaveColor[];
	static typedescription_t m_SaveVector[];
	static typedescription_t m_SavePositionVector[];
	static typedescription_t m_SaveVMatrix[];
	static typedescription_t m_SaveVMatrixWorldspace[];
	static typedescription_t m_SaveMatrix3x4Worldspace[];

protected:

	//
	// Returns a string representation of the value without modifying the variant.
	//
	const char *ToString( void ) const;

	friend class CVariantSaveDataOps;
};


//-----------------------------------------------------------------------------
// Purpose: Returns this variant as a vector.
//-----------------------------------------------------------------------------
inline void variant_t::Vector3D(Vector &vec) const
{
	if (( fieldType == FIELD_VECTOR ) || ( fieldType == FIELD_POSITION_VECTOR ))
	{
		vec[0] =  vecVal[0];
		vec[1] =  vecVal[1];
		vec[2] =  vecVal[2];
	}
	else
	{
		vec = vec3_origin;
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: Returns this variant as angles.
//-----------------------------------------------------------------------------
inline void variant_t::Angle3D(QAngle &ang) const
{
	if (( fieldType == FIELD_VECTOR ) || ( fieldType == FIELD_POSITION_VECTOR ))
	{
		ang[0] =  vecVal[0];
		ang[1] =  vecVal[1];
		ang[2] =  vecVal[2];
	}
	else
	{
		ang = vec3_angle;
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns this variant as an EHANDLE.
//-----------------------------------------------------------------------------
inline const CHandle<CBaseEntity> &variant_t::Entity(void) const
{
	if ( fieldType == FIELD_EHANDLE )
		return eVal;

	static CHandle<CBaseEntity> hNull;
	hNull.Set(NULL);
	return(hNull);
}


#endif // VARIANT_T_H
