//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef SAVERESTORE_UTLCLASS_H
#define SAVERESTORE_UTLCLASS_H

#if defined( _WIN32 )
#pragma once
#endif

//-------------------------------------

template <int FIELD_TYPE>
class CTypedescDeducer
{
public:
	template <class UTLCLASS>
	static datamap_t *Deduce( UTLCLASS *p )
	{
		return NULL;
	}

};

template<> 
class CTypedescDeducer<FIELD_EMBEDDED>
{
public:
	template <class UTLCLASS>
	static datamap_t *Deduce( UTLCLASS *p )
	{
		return &UTLCLASS::ElemType_t::m_DataMap;
	}

};

#define UTLCLASS_SAVERESTORE_VALIDATE_TYPE( type ) \
	COMPILE_TIME_ASSERT( \
		type == FIELD_FLOAT ||\
		type == FIELD_STRING ||\
		type == FIELD_CLASSPTR ||\
		type == FIELD_EHANDLE ||\
		type == FIELD_EDICT ||\
		type == FIELD_VECTOR ||\
		type == FIELD_QUATERNION ||\
		type == FIELD_POSITION_VECTOR ||\
		type == FIELD_INTEGER ||\
		type == FIELD_BOOLEAN ||\
		type == FIELD_SHORT ||\
		type == FIELD_CHARACTER ||\
		type == FIELD_TIME ||\
		type == FIELD_TICK ||\
		type == FIELD_MODELNAME ||\
		type == FIELD_SOUNDNAME ||\
		type == FIELD_COLOR32 ||\
		type == FIELD_EMBEDDED ||\
		type == FIELD_MODELINDEX ||\
		type == FIELD_MATERIALINDEX\
	)

//-------------------------------------

#endif // SAVERESTORE_UTLCLASS_H
