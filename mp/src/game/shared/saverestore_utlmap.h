//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef SAVERESTORE_UTLMAP_H
#define SAVERESTORE_UTLMAP_H

#include "utlmap.h"
#include "saverestore_utlrbtree.h"

#if defined( _WIN32 )
#pragma once
#endif

template <class UTLMAP, int KEY_TYPE, int FIELD_TYPE>
class CUtlMapDataOps : public CDefSaveRestoreOps
{
public:
	CUtlMapDataOps()
	{
		UTLCLASS_SAVERESTORE_VALIDATE_TYPE( KEY_TYPE );
		UTLCLASS_SAVERESTORE_VALIDATE_TYPE( FIELD_TYPE );
	}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{		
		datamap_t *pKeyDatamap = CTypedescDeducer<KEY_TYPE>::Deduce( (UTLMAP *)NULL );
		datamap_t *pFieldDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLMAP *)NULL );
		typedescription_t dataDesc[] = 
		{
			{
				(fieldtype_t)KEY_TYPE, 
				"K", 
				{ 0, 0 },
				1, 
				FTYPEDESC_SAVE, 
				NULL, 
				NULL, 
				NULL,
				pKeyDatamap,
				sizeof(KEY_TYPE),
			},
			
			{
				(fieldtype_t)FIELD_TYPE, 
				"T", 
				{ offsetof(typename UTLMAP::Node_t, elem), 0 },
				1, 
				FTYPEDESC_SAVE, 
				NULL, 
				NULL, 
				NULL,
				pFieldDatamap,
				sizeof(FIELD_TYPE),
			}
		};
		
		datamap_t dataMap = 
		{
			dataDesc,
			2,
			"um",
			NULL,
			false,
			false,
			0,
#ifdef _DEBUG
			true
#endif
		};

		typename UTLMAP::CTree *pUtlRBTree = ((UTLMAP *)fieldInfo.pField)->AccessTree();

		pSave->StartBlock();
		
		int nElems = pUtlRBTree->Count();
		pSave->WriteInt( &nElems, 1 );

		typename UTLMAP::CTree::IndexType_t i = pUtlRBTree->FirstInorder();
		while ( i != pUtlRBTree->InvalidIndex() )
		{
			typename UTLMAP::CTree::ElemType_t &elem = pUtlRBTree->Element( i );

			pSave->WriteAll( &elem, &dataMap );

			i = pUtlRBTree->NextInorder( i );
		}
		pSave->EndBlock();
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		datamap_t *pKeyDatamap = CTypedescDeducer<KEY_TYPE>::Deduce( (UTLMAP *)NULL );
		datamap_t *pFieldDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLMAP *)NULL );
		typedescription_t dataDesc[] = 
		{
			{
				(fieldtype_t)KEY_TYPE, 
				"K", 
				{ 0, 0 },
				1, 
				FTYPEDESC_SAVE, 
				NULL, 
				NULL, 
				NULL,
				pKeyDatamap,
				sizeof(KEY_TYPE),
			},
			
			{
				(fieldtype_t)FIELD_TYPE, 
				"T", 
				{ offsetof(typename UTLMAP::Node_t, elem), 0 },
				1, 
				FTYPEDESC_SAVE, 
				NULL, 
				NULL, 
				NULL,
				pFieldDatamap,
				sizeof(FIELD_TYPE),
			}
		};
		
		datamap_t dataMap = 
		{
			dataDesc,
			2,
			"um",
			NULL,
			false,
			false,
			0,
#ifdef _DEBUG
			true
#endif
		};

		UTLMAP *pUtlMap = ((UTLMAP *)fieldInfo.pField);

		pRestore->StartBlock();

		int nElems = pRestore->ReadInt();
		typename UTLMAP::CTree::ElemType_t temp;

		while ( nElems-- )
		{
			pRestore->ReadAll( &temp, &dataMap );
			pUtlMap->Insert( temp.key, temp.elem );
		}
		
		pRestore->EndBlock();
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLMAP *pUtlMap = (UTLMAP *)fieldInfo.pField;
		pUtlMap->RemoveAll();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLMAP *pUtlMap = (UTLMAP *)fieldInfo.pField;
		return ( pUtlMap->Count() == 0 );
	}
	
};

//-------------------------------------

template <int KEYTYPE, int FIELD_TYPE>
class CUtlMapDataopsInstantiator
{
public:
	template <class UTLMAP>
	static ISaveRestoreOps *GetDataOps(UTLMAP *)
	{
		static CUtlMapDataOps<UTLMAP, KEYTYPE, FIELD_TYPE> ops;
		return &ops;
	}
};

//-------------------------------------

#define SaveUtlMap( pSave, pUtlMap, fieldtype) \
	CUtlMapDataopsInstantiator<fieldtype>::GetDataOps( pUtlMap )->Save( pUtlMap, pSave );

#define RestoreUtlMap( pRestore, pUtlMap, fieldtype) \
	CUtlMapDataopsInstantiator<fieldtype>::GetDataOps( pUtlMap )->Restore( pUtlMap, pRestore );

//-------------------------------------

#define DEFINE_UTLMAP(name,keyType,fieldtype) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, CUtlMapDataopsInstantiator<keyType, fieldtype>::GetDataOps(&(((classNameTypedef *)0)->name)), NULL }


#endif // SAVERESTORE_UTLMAP_H
