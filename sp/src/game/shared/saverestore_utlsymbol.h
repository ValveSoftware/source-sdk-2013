//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SAVERESTORE_UTLSYMBOL_H
#define SAVERESTORE_UTLSYMBOL_H
#ifdef _WIN32
#pragma once
#endif

#include "utlsymbol.h"

class CUtlSymbolDataOps : public CDefSaveRestoreOps
{
public:
	CUtlSymbolDataOps( CUtlSymbolTable &masterTable ) : m_symbolTable(masterTable) {}

	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		CUtlSymbol *sym = ((CUtlSymbol *)fieldInfo.pField);
		
		pSave->WriteString( m_symbolTable.String( *sym ) );
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		CUtlSymbol *sym = ((CUtlSymbol *)fieldInfo.pField);

		char tmp[1024];
		pRestore->ReadString( tmp, sizeof(tmp), 0 );
		*sym = m_symbolTable.AddString( tmp );
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlSymbol *sym = ((CUtlSymbol *)fieldInfo.pField);
		*sym = UTL_INVAL_SYMBOL;
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		CUtlSymbol *sym = ((CUtlSymbol *)fieldInfo.pField);
		return (*sym).IsValid() ? false : true;
	}

private:
	CUtlSymbolTable &m_symbolTable;
	
};

#endif // SAVERESTORE_UTLSYMBOL_H
