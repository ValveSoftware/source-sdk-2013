//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "isaverestore.h"

#ifndef STDSTRING_H
#define STDSTRING_H

#if defined( _WIN32 )
#pragma once
#endif

#ifdef _WIN32
#pragma warning(push)
#include <yvals.h>	// warnings get enabled in yvals.h 
#pragma warning(disable:4663)
#pragma warning(disable:4530)
#pragma warning(disable:4245)
#pragma warning(disable:4018)
#pragma warning(disable:4511)
#endif

#include "tier0/valve_minmax_off.h"	// GCC 4.2.2 headers screw up our min/max defs.
#include <string>
#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.

#ifdef _WIN32
#pragma warning(pop)
#endif

class CStdStringSaveRestoreOps : public CDefSaveRestoreOps
{
public:
	enum
	{
		MAX_SAVE_LEN = 4096,
	};

	// save data type interface
	virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		Assert( pString->length() < MAX_SAVE_LEN - 1 );
		if ( pString->length() < MAX_SAVE_LEN - 1 )
			pSave->WriteString( pString->c_str() );
		else
			pSave->WriteString( "<<invalid>>" );
	}
	
	virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		char szString[MAX_SAVE_LEN];
		pRestore->ReadString( szString, sizeof(szString), 0 );
		szString[MAX_SAVE_LEN - 1] = 0;
		pString->assign( szString );
	}
	
	virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		pString->erase();
	}

	virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
	{
		std::string *pString = (std::string *)fieldInfo.pField;
		return pString->empty();
	}
};

//-------------------------------------

inline ISaveRestoreOps *GetStdStringDataOps()
{
	static CStdStringSaveRestoreOps ops;
	return &ops;
}

//-------------------------------------

#define DEFINE_STDSTRING(name) \
	{ FIELD_CUSTOM, #name, { offsetof(classNameTypedef,name), 0 }, 1, FTYPEDESC_SAVE, NULL, GetStdStringDataOps(), NULL }

#endif // STDSTRING_H
