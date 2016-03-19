//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef QCGENERATOR_H
#define QCGENERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "tier1/utlstring.h"
#include "vgui_controls/TextEntry.h"

class CQCGenerator;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class Panel;
}

class CBrowseButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CBrowseButton, vgui::Button );

public:
	CBrowseButton( vgui::Panel *pParent );
	~CBrowseButton();
	void InitBrowseInfo( int x, int y, const char *pszName, const char *pszDir, const char *pszFilter, const char *pszField );

private:
	char *pszStartingDirectory;
	char *pszFileFilter;
	char *pszTargetField;

	char **GetStartingDirectory(){ return &pszStartingDirectory; }
	char **GetFileFilter(){ return &pszFileFilter; }
	char **GetTargetField(){ return &pszTargetField; }
	void SetCharVar( char **pVar, const char *pszNewText );
	void SetActionMessage();
};

struct LODInfo
{
	char pszFilename[MAX_PATH];
    int iLOD;	
};

struct QCInfo
{
    CQCGenerator *pQCGenerator;

	char pszSMDPath[MAX_PATH];
	char pszCollisionPath[MAX_PATH];
	char pszSurfaceProperty[MAX_PATH];
	char pszMaterialPath[MAX_PATH];
	char pszSceneName[MAX_PATH];

	bool bStaticProp;
	bool bMostlyOpaque;
	bool bDisableCollision;
	bool bReferenceAsPhys;
	bool bConcave;
	bool bAutomass;
	bool bNoAnimation;

	CUtlVector<LODInfo> LODs;

	float fScale;
	float fMass;
	void Init( CQCGenerator *pPanel )
	{
		pQCGenerator = pPanel;

		V_strcpy_safe( pszSMDPath, "" );
		V_strcpy_safe( pszCollisionPath, "" );
		V_strcpy_safe( pszSurfaceProperty, "default" );
		bStaticProp = false;
		bMostlyOpaque = false;
		bDisableCollision = false;
		bReferenceAsPhys = false;
		bConcave = false;
		bAutomass = false;
		bNoAnimation = true;

		fScale = 1.0;
		fMass = 10.0;
	}
	void SyncToControls();
	void SyncFromControls();
};

//-----------------------------------------------------------------------------
// Purpose: Base class for generating QC files
//-----------------------------------------------------------------------------
class CQCGenerator : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CQCGenerator, vgui::EditablePanel );

public:
	CQCGenerator( vgui::Panel *pParent, const char *pszPath, const char *pszScene );
	~CQCGenerator();

	// overridden frame functions
//	virtual void Activate();

	virtual void OnCommand( const char *command );

	// Purpose: 
//	virtual void OnKeyCodeTyped( vgui::KeyCode code );

	MESSAGE_FUNC( OnNewLODText, "TextNewLine" );	
	MESSAGE_FUNC_PARAMS( OnBrowse, "browse", data );	
	MESSAGE_FUNC_PARAMS( OnFileSelected, "FileSelected", data );	
	MESSAGE_FUNC_PARAMS( OnDirectorySelected, "DirectorySelected", data );	

	bool GenerateQCFile();
//	void BrowseDirectory( KeyValues *data );
	void BrowseFile( KeyValues *data );

	void DeleteLOD( );
	void EditLOD();
	virtual void OnKeyCodeTyped( vgui::KeyCode code);
	void InitializeSMDPaths( const char *pszPath, const char *pszScene );
	
protected:
	// Creates standard controls. Allows the derived class to
	// add these controls to various splitter windows
	void CreateStandardControls( vgui::Panel *pParent );

private:

	CBrowseButton *m_pCollisionBrowseButton;	
	char m_szTargetField[MAX_PATH];
	vgui::ListPanel *m_pLODPanel;

	vgui::TextEntry *m_pLODEdit;
	
	int m_nSelectedSequence;
	int m_nSelectedColumn;

	QCInfo m_QCInfo_t;
};




#endif // QCGENERATOR_H
