//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SAVERESTORE_UTLVECTOR_H
#define SAVERESTORE_UTLVECTOR_H

#include "utlvector.h"
#include "isaverestore.h"
#include "saverestore_utlclass.h"

#if defined( _WIN32 )
#pragma once
#endif

//-------------------------------------

template <class UTLVECTOR, int FIELD_TYPE>
class CUtlVectorDataOps : public CDefSaveRestoreOps
{
public:
	CUtlVectorDataOps()
	{
		UTLCLASS_SAVERESTORE_VALIDATE_TYPE( FIELD_TYPE );
	}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{		
		datamap_t *pArrayTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLVECTOR *)NULL );
		typedescription_t dataDesc = 
		{
			(fieldtype_t)FIELD_TYPE, 
			"elems", 
			{ 0, 0 },
			1, 
			FTYPEDESC_SAVE, 
			NULL, 
			NULL, 
			NULL,
			pArrayTypeDatamap,
			-1,
		};
		
		datamap_t dataMap = 
		{
			&dataDesc,
			1,
			"uv",
			NULL,
			false,
			false,
			0,
#ifdef _DEBUG
			true
#endif
		};
		
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		int nElems = pUtlVector->Count();
		
		pSave->WriteInt( &nElems, 1 );
		if ( pArrayTypeDatamap == NULL )
		{
			if ( nElems )
			{
				dataDesc.fieldSize = nElems;
				dataDesc.fieldSizeInBytes = nElems * CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
				pSave->WriteFields("elems", &((*pUtlVector)[0]), &dataMap, &dataDesc, 1 );
			}
		}
		else
		{
			// @Note (toml 11-21-02): Save load does not support arrays of user defined types (embedded)
			dataDesc.fieldSizeInBytes = CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
			for ( int i = 0; i < nElems; i++ )
				pSave->WriteAll( &((*pUtlVector)[i]), &dataMap );
		}
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		datamap_t *pArrayTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLVECTOR *)NULL );
		typedescription_t dataDesc = 
		{
			(fieldtype_t)FIELD_TYPE, 
			"elems", 
			{ 0, 0 },
			1, 
			FTYPEDESC_SAVE, 
			NULL, 
			NULL, 
			NULL,
			pArrayTypeDatamap,
			-1,
		};
		
		datamap_t dataMap = 
		{
			&dataDesc,
			1,
			"uv",
			NULL,
			false,
			false,
			0,
#ifdef _DEBUG
			true
#endif
		};
		
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;

		int nElems = pRestore->ReadInt();
		
		pUtlVector->SetCount( nElems );
		if ( pArrayTypeDatamap == NULL )
		{
			if ( nElems )
			{
				dataDesc.fieldSize = nElems;
				dataDesc.fieldSizeInBytes = nElems * CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
				pRestore->ReadFields("elems", &((*pUtlVector)[0]), &dataMap, &dataDesc, 1 );
			}
		}
		else
		{
			// @Note (toml 11-21-02): Save load does not support arrays of user defined types (embedded)
			dataDesc.fieldSizeInBytes = CDatamapFieldSizeDeducer<FIELD_TYPE>::FieldSize();
			for ( int i = 0; i < nElems; i++ )
				pRestore->ReadAll( &((*pUtlVector)[i]), &dataMap );
		}		
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		pUtlVector->SetCount( 0 );
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLVECTOR *pUtlVector = (UTLVECTOR *)fieldInfo.pField;
		return ( pUtlVector->Count() == 0 );
	}
	
};

//-------------------------------------

template <int FIELD_TYPE>
class CUtlVectorDataopsInstantiator
{
public:
	template <class UTLVECTOR>
	static ISaveRestoreOps *GetDataOps(UTLVECTOR *)
	{
		static CUtlVectorDataOps<UTLVECTOR, FIELD_TYPE> ops;
		return &ops;
	}
};

//-------------------------------------

#define SaveUtlVector( pSave, pUtlVector, fieldtype) \
	CUtlVectorDataopsInstantiator<fieldtype>::GetDataOps( pUtlVector )->Save( pUtlVector, pSave );

#define RestoreUtlVector( pRestore, pUtlVector, fieldtype) \
	CUtlVectorDataopsInstantiator<fieldtype>::GetDataOps( pUtlVector )->Restore( pUtlVector, pRestore );

//-------------------------------------

#define DEFINE_UTLVECTOR(name,fieldtype) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, CUtlVectorDataopsInstantiator<fieldtype>::GetDataOps(&(((classNameTypedef *)0)->name)), NULL }

#define DEFINE_GLOBAL_UTLVECTOR(name,fieldtype) \
{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE|FTYPEDESC_GLOBAL, NULL, CUtlVectorDataopsInstantiator<fieldtype>::GetDataOps(&(((classNameTypedef *)0)->name)), NULL }


#endif // SAVERESTORE_UTLVECTOR_H
