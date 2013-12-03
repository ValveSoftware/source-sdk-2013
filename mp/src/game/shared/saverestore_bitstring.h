//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef SAVERESTORE_BITSTRING_H
#define SAVERESTORE_BITSTRING_H

#include "isaverestore.h"

#if defined( _WIN32 )
#pragma once
#endif

//-------------------------------------

template <class BITSTRING>
class CVarBitVecSaveRestoreOps : public CDefSaveRestoreOps
{
public:
	CVarBitVecSaveRestoreOps()
	{
	}

	// save data type interface
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		BITSTRING *pBitString = (BITSTRING *)fieldInfo.pField;
		int numBits = pBitString->GetNumBits();
		pSave->WriteInt( &numBits );
		pSave->WriteInt( pBitString->Base(), pBitString->GetNumDWords() );
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		BITSTRING *pBitString = (BITSTRING *)fieldInfo.pField;
		int numBits = pRestore->ReadInt();
		if ( !pBitString->IsFixedSize() )
			pBitString->Resize( numBits );
		else
		{
			Assert( pBitString->GetNumBits() >= numBits );
			pBitString->ClearAll();
		}
		int numIntsInStream = CalcNumIntsForBits( numBits );
		int readSize = MIN( pBitString->GetNumDWords(), numIntsInStream );
		pRestore->ReadInt( pBitString->Base(), numIntsInStream );

		numIntsInStream -= readSize;
		while ( numIntsInStream-- > 0 )
		{
			int ignored;
			pRestore->ReadInt( &ignored, 1 );
		}
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		BITSTRING *pBitString = (BITSTRING *)fieldInfo.pField;
		pBitString->ClearAll();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		BITSTRING *pBitString = (BITSTRING *)fieldInfo.pField;
		return pBitString->IsAllClear();
	}
};

//-------------------------------------

template <class BITSTRING>
ISaveRestoreOps *GetBitstringDataOps(BITSTRING *)
{
	static CVarBitVecSaveRestoreOps<BITSTRING> ops;
	return &ops;
}

//-------------------------------------

#define SaveBitString( pSave, pBitString, fieldtype) \
	CDataopsInstantiator<fieldtype>::GetDataOps( pBitString )->Save( pBitString, pSave );

#define RestoreBitString( pRestore, pBitString, fieldtype) \
	CDataopsInstantiator<fieldtype>::GetDataOps( pBitString )->Restore( pBitString, pRestore );

//-------------------------------------

#define DEFINE_BITSTRING(name) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, GetBitstringDataOps(&(((classNameTypedef *)0)->name)), NULL }

#endif // SAVERESTORE_BITSTRING_H

