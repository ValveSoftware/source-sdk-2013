//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Point entity used to create templates out of other entities or groups of entities
//
//=============================================================================//

#ifndef POINT_TEMPLATE_H
#define POINT_TEMPLATE_H
#ifdef _WIN32
#pragma once
#endif

#define MAX_NUM_TEMPLATES		16

struct template_t
{
	int			iTemplateIndex;
	VMatrix		matEntityToTemplate;

	DECLARE_SIMPLE_DATADESC();
};

void ScriptInstallPreSpawnHook();
bool ScriptPreInstanceSpawn( CScriptScope *pScriptScope, CBaseEntity *pChild, string_t iszKeyValueData );
void ScriptPostSpawn( CScriptScope *pScriptScope, CBaseEntity **ppEntities, int nEntities );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointTemplate : public CLogicalEntity
{
	DECLARE_CLASS( CPointTemplate, CLogicalEntity );
public:
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual void	Precache();

	// Template initialization
	void			StartBuildingTemplates( void );
	void			FinishBuildingTemplates( void );

	// Template Entity accessors
	int				GetNumTemplateEntities( void );
	CBaseEntity		*GetTemplateEntity( int iTemplateNumber );
	void			AddTemplate( CBaseEntity *pEntity, const char *pszMapData, int nLen );
	bool			ShouldRemoveTemplateEntities( void );
	bool			AllowNameFixup();

	// Templates accessors
	int				GetNumTemplates( void );
	int				GetTemplateIndexForTemplate( int iTemplate );

	// Template instancing
	bool			CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CBaseEntity*> *pEntities );
	void			CreationComplete( const CUtlVector<CBaseEntity*> &entities );

	// Inputs
	void			InputForceSpawn( inputdata_t &inputdata );

	virtual void	PerformPrecache();

private:
	string_t						m_iszTemplateEntityNames[MAX_NUM_TEMPLATES];

	// List of map entities this template targets. Built inside our Spawn().
	// It's only valid between Spawn() & Activate(), because the map entity parsing
	// code removes all the entities in it once it finishes turning them into templates.
	CUtlVector< CBaseEntity * >		m_hTemplateEntities;

	// List of templates, generated from our template entities.
	CUtlVector< template_t >		m_hTemplates;

	COutputEvent					m_pOutputOnSpawned;
};


//-----------------------------------------------------------------------------
// Entity for template spawning entities from script
//-----------------------------------------------------------------------------
struct scriptTemplate_t
{
	string_t	szClassname;
	HSCRIPT		hSpawnTable;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointScriptTemplate : public CPointTemplate
{
	DECLARE_CLASS( CPointScriptTemplate, CLogicalEntity );
public:
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC();

	virtual ~CPointScriptTemplate();
	virtual void	Spawn( void );

	void			AddTemplate( const char *pClassname, HSCRIPT spawnTable );
	void			SetGroupSpawnTables( HSCRIPT templateSpawnTable, HSCRIPT spawnTables );
	virtual bool	CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CBaseEntity*> *pEntities, CBaseEntity *pEntityMaker = NULL, bool bCreateTime = false );
	void			CreationComplete( const CUtlVector<CBaseEntity*> &entities );

	// Inputs
	void			InputForceSpawn( inputdata_t &inputdata );

private:
	string_t						m_iszTemplateEntityNames[MAX_NUM_TEMPLATES];

	// List of templates, generated from our template entities.
	CUtlVector< scriptTemplate_t >	m_hTemplates;
	COutputEvent	m_pOutputOnSpawned;
	HSCRIPT			m_hTemplateSpawnTable;
	HSCRIPT			m_hGroupSpawnTables;
};

#endif // POINT_TEMPLATE_H
