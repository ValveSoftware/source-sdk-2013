//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: modifiedconvarts_t from CommentarySystem.cpp moved to a header file so Mapbase can use it.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"


// Convar restoration save/restore
#define MAX_MODIFIED_CONVAR_STRING		128
struct modifiedconvars_t 
{
	DECLARE_SIMPLE_DATADESC();

	char pszConvar[MAX_MODIFIED_CONVAR_STRING];
	char pszCurrentValue[MAX_MODIFIED_CONVAR_STRING];
	char pszOrgValue[MAX_MODIFIED_CONVAR_STRING];
};

// ---------------------------------------------------------------------

#define SF_CVARMOD_START_ACTIVATED (1 << 0)

class CMapbaseCVarModEntity : public CPointEntity
{
public:
	DECLARE_CLASS( CMapbaseCVarModEntity, CPointEntity );

	void UpdateOnRemove();

	void Precache( void );

	void Spawn( void );
	void CvarModActivate();

	void OnRestore( void );

	void InputActivate(inputdata_t &inputdata);
	void InputDeactivate(inputdata_t &inputdata);

	bool NewCVar( ConVarRef *var, const char *pOldString, CBaseEntity *modent );

	CUtlVector< modifiedconvars_t > m_ModifiedConvars;
	bool m_bUseServer;

	DECLARE_DATADESC();
};
