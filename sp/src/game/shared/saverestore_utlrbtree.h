//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef SAVERESTORE_UTLRBTREE_H
#define SAVERESTORE_UTLRBTREE_H

#include "utlrbtree.h"
#include "isaverestore.h"
#include "saverestore_utlclass.h"

#if defined( _WIN32 )
#pragma once
#endif

//-------------------------------------

template <class UTLRBTREE, int FIELD_TYPE>
class CUtlRBTreeDataOps : public CDefSaveRestoreOps
{
public:
	CUtlRBTreeDataOps()
	{
		UTLCLASS_SAVERESTORE_VALIDATE_TYPE( FIELD_TYPE );
	}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{		
		datamap_t *pTreeTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLRBTREE *)NULL );
		typedescription_t dataDesc = 
		{
			(fieldtype_t)FIELD_TYPE, 
			"elem", 
			{ 0, 0 },
			1, 
			FTYPEDESC_SAVE, 
			NULL, 
			NULL, 
			NULL,
			pTreeTypeDatamap,
			-1,
		};
		
		datamap_t dataMap = 
		{
			&dataDesc,
			1,
			"urb",
			NULL,
			false,
			false,
			0,
#ifdef _DEBUG
			true
#endif
		};
		
		UTLRBTREE *pUtlRBTree = (UTLRBTREE *)fieldInfo.pField;

		pSave->StartBlock();
		
		int nElems = pUtlRBTree->Count();
		pSave->WriteInt( &nElems, 1 );

		typename UTLRBTREE::IndexType_t i = pUtlRBTree->FirstInorder();
		while ( i != pUtlRBTree->InvalidIndex() )
		{
			typename UTLRBTREE::ElemType_t &elem = pUtlRBTree->Element( i );

			pSave->WriteAll( &elem, &dataMap );

			i = pUtlRBTree->NextInorder( i );
		}
		pSave->EndBlock();
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		datamap_t *pTreeTypeDatamap = CTypedescDeducer<FIELD_TYPE>::Deduce( (UTLRBTREE *)NULL );
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
			pTreeTypeDatamap,
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
		
		UTLRBTREE *pUtlRBTree = (UTLRBTREE *)fieldInfo.pField;

		pRestore->StartBlock();

		int nElems = pRestore->ReadInt();
		typename UTLRBTREE::ElemType_t temp;

		while ( nElems-- )
		{
			pRestore->ReadAll( &temp, &dataMap );
			pUtlRBTree->Insert( temp );
		}
		
		pRestore->EndBlock();
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLRBTREE *pUtlRBTree = (UTLRBTREE *)fieldInfo.pField;
		pUtlRBTree->RemoveAll();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		UTLRBTREE *pUtlRBTree = (UTLRBTREE *)fieldInfo.pField;
		return ( pUtlRBTree->Count() == 0 );
	}
	
};

//-------------------------------------

template <int FIELD_TYPE>
class CUtlRBTreeDataopsInstantiator
{
public:
	template <class UTLRBTREE>
	static ISaveRestoreOps *GetDataOps(UTLRBTREE *)
	{
		static CUtlRBTreeDataOps<UTLRBTREE, FIELD_TYPE> ops;
		return &ops;
	}
};

//-------------------------------------

#define SaveUtlRBTree( pSave, pUtlRBTree, fieldtype) \
	CUtlRBTreeDataopsInstantiator<fieldtype>::GetDataOps( pUtlRBTree )->Save( pUtlRBTree, pSave );

#define RestoreUtlRBTree( pRestore, pUtlRBTree, fieldtype) \
	CUtlRBTreeDataopsInstantiator<fieldtype>::GetDataOps( pUtlRBTree )->Restore( pUtlRBTree, pRestore );

//-------------------------------------

#define DEFINE_UTLRBTREE(name,fieldtype) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, CUtlRBTreeDataopsInstantiator<fieldtype>::GetDataOps(&(((classNameTypedef *)0)->name)), NULL }

#endif // SAVERESTORE_UTLRBTREE_H
