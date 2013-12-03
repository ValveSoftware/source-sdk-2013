//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLUGINVARIANT_H
#define PLUGINVARIANT_H
#ifdef _WIN32
#pragma once
#endif

#include "stdstring.h"
#include "mathlib/vmatrix.h"

/*
//Tony; including stdstring at this point (which I need) messes up the offsetof override in linux, so I need to reset it here.
#if defined( POSIX )
#undef offsetof
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif
 */
//
// A modified variant class for that functions almost identically to variant_t, for plugins to pass data back and forth.
//
class pluginvariant
{
	union
	{
		bool bVal;
		int iVal;
		float flVal;
		float vecVal[3];
		color32 rgbaVal;
	};
	//Tony; neither of these can be in the union because of constructors.
	edict_t *eVal;
	char iszVal[1024];

	fieldtype_t fieldType;

public:

	// constructor
	pluginvariant() : fieldType(FIELD_VOID), iVal(0) {}

	inline bool Bool( void ) const						{ return( fieldType == FIELD_BOOLEAN ) ? bVal : false; }
	inline const char *String( void ) const				{ return( ToString() );  }
	inline int Int( void ) const						{ return( fieldType == FIELD_INTEGER ) ? iVal : 0; }
	inline float Float( void ) const					{ return( fieldType == FIELD_FLOAT ) ? flVal : 0; }
	inline const edict_t *Edict(void) const;
	inline color32 Color32(void) const					{ return rgbaVal; }
	inline void Vector3D(Vector &vec) const;

	fieldtype_t FieldType( void ) { return fieldType; }

	void SetBool( bool b ) { bVal = b; fieldType = FIELD_BOOLEAN; }
	void SetString( char *str ) { Q_snprintf(iszVal, 1024, "%s", str); fieldType = FIELD_STRING; }
	void SetInt( int val ) { iVal = val, fieldType = FIELD_INTEGER; }
	void SetFloat( float val ) { flVal = val, fieldType = FIELD_FLOAT; }
	void SetEdict( edict_t *val ) { eVal = val; fieldType = FIELD_EHANDLE; }
	void SetVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_VECTOR; }
	void SetPositionVector3D( const Vector &val ) { vecVal[0] = val[0]; vecVal[1] = val[1]; vecVal[2] = val[2]; fieldType = FIELD_POSITION_VECTOR; }
	void SetColor32( color32 val ) { rgbaVal = val; fieldType = FIELD_COLOR32; }
	void SetColor32( int r, int g, int b, int a ) { rgbaVal.r = r; rgbaVal.g = g; rgbaVal.b = b; rgbaVal.a = a; fieldType = FIELD_COLOR32; }

protected:

	//
	// Returns a string representation of the value without modifying the variant.
	//
	const char *ToString( void ) const
	{
		static char szBuf[512];

		switch (fieldType)
		{
		case FIELD_STRING:
			{
				return (const char *)iszVal;
			}

		case FIELD_BOOLEAN:
			{
				if (bVal == 0)
				{
					Q_strncpy(szBuf, "false",sizeof(szBuf));
				}
				else
				{
					Q_strncpy(szBuf, "true",sizeof(szBuf));
				}
				return(szBuf);
			}

		case FIELD_INTEGER:
			{
				Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
				return(szBuf);
			}

		case FIELD_FLOAT:
			{
				Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
				return(szBuf);
			}

		case FIELD_COLOR32:
			{
				Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
				return(szBuf);
			}

		case FIELD_VECTOR:
			{
				Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
				return(szBuf);
			}

		case FIELD_VOID:
			{
				szBuf[0] = '\0';
				return(szBuf);
			}
		}

		return("No conversion to string");
	}
};


////////////////////////// pluginvariant implementation //////////////////////////


//-----------------------------------------------------------------------------
// Purpose: Returns this variant as a vector.
//-----------------------------------------------------------------------------
inline void pluginvariant::Vector3D(Vector &vec) const
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

//-----------------------------------------------------------------------------
// Purpose: Returns this variant as an edict_t
//-----------------------------------------------------------------------------
inline const edict_t *pluginvariant::Edict(void) const
{
	if ( fieldType == FIELD_EHANDLE )
		return eVal;

	return NULL;
}


#endif // pluginvariant_H
