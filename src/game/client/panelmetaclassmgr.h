//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A panel "metaclass" is a name given to a particular type of 
// panel with particular instance data. Such panels tend to be dynamically
// added and removed from their parent panels.
//
// $Workfile: $
// $NoKeywords: $
//=============================================================================//

#if !defined( PANELMETACLASSMGR_H )
#define PANELMETACLASSMGR_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "basetypes.h"
#include <vgui/VGUI.h>

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class KeyValues;
class Color;

namespace vgui
{
	class Panel;
}


//-----------------------------------------------------------------------------
// Class factory interface for metaclasses
//-----------------------------------------------------------------------------
abstract_class IPanelFactory
{
public:
	// Creation, destruction methods
	virtual vgui::Panel *Create( const char *pMetaClassName, KeyValues* pKeyValues, void *pInitData, vgui::Panel *pParent ) = 0;
};


//-----------------------------------------------------------------------------
// Purpose: Singleton class responsible for managing vgui panel metaclasses
// A metaclass is simply an association of panel implementation class with
// various initialization data
//-----------------------------------------------------------------------------
abstract_class IPanelMetaClassMgr
{
public:
	// Call this to load up a file containing metaclass definitions
	virtual void LoadMetaClassDefinitionFile( const char *pFileName ) = 0;

	// Call this to install a new panel type
	// MetaClasses will refer to the panel type to create along with
	// various initialization data
	virtual void InstallPanelType( const char *pPanelName, IPanelFactory *pFactory ) = 0;

	// Creates a metaclass panel with the specified parent panel.
	// Chain name is used as a filter of the metaclass data; if specified,
	// it recursively iterates through the keyvalue sections and calls
	// chainKeyValue on sections whose name matches the chain name
	virtual vgui::Panel *CreatePanelMetaClass( const char *pMetaClassType, 
		int sortorder, void *pInitData, vgui::Panel *pParent, const char *pChainName = NULL ) = 0;

	// removes a particular panel meta class
	virtual void DestroyPanelMetaClass( vgui::Panel *pPanel ) = 0;

protected:
	// Don't delete me!
	virtual ~IPanelMetaClassMgr() {}
};


//-----------------------------------------------------------------------------
// Returns the panel meta class manager
//-----------------------------------------------------------------------------
IPanelMetaClassMgr *PanelMetaClassMgr();


//-----------------------------------------------------------------------------
// Helper class for simple construction of planel class factories
// This class is expected to be a singleton
// Note the panel must have a constructor of the following form:
//		CPanel( vgui::Panel* );
// and it must have the following member function:
//		bool CPanel::Init( KeyValues* pInitData )
// which returns true if the panel initialized successfully
//-----------------------------------------------------------------------------

#include "tier0/memdbgon.h"

template< class CPanel, class CInitData >
class CPanelFactory : public IPanelFactory
{
public:
	CPanelFactory( const char *pTypeName )
	{
		// Hook us up baby
		Assert( pTypeName );
		PanelMetaClassMgr()->InstallPanelType( pTypeName, this );
	}

	// Creation, destruction methods
	virtual vgui::Panel *Create( const char *pMetaClassName, KeyValues* pKeyValues, void *pVoidInitData, vgui::Panel *pParent )
	{
		// NOTE: make sure this matches the panel allocation pattern;
		// it will break if panels are deleted differently
		CPanel* pPanel = new CPanel( pParent, pMetaClassName ); 
		if (pPanel)
		{
			// Set parent before Init; it may be needed there...
			CInitData* pInitData = (CInitData*)(pVoidInitData); 
			if (!pPanel->Init( pKeyValues, pInitData ))
			{
				delete pPanel;
				pPanel = NULL;
			}
		}
		return pPanel;
	}
};

#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// Helper macro to make panel factories one line of code. Use like this:
//	DECLARE_PANEL_FACTORY( CEntityImagePanel, CInitData, "image" );
// The type string is used in a panel script file to specify the type.
// CInitData is the type of the data to pass to the init function
//-----------------------------------------------------------------------------
#define DECLARE_PANEL_FACTORY( _PanelClass, _InitData, _nameString )	\
	CPanelFactory< _PanelClass, _InitData > g_ ## _PanelClass ## Factory( _nameString )



//-----------------------------------------------------------------------------
// Helper class to make meta class panels
//-----------------------------------------------------------------------------
class CPanelWrapper
{
public:
	CPanelWrapper();
	~CPanelWrapper();
	void Activate( char const* pMetaClassName, vgui::Panel *pParent, int sortorder, void *pVoidInitData );
	void Deactivate( void );
	vgui::Panel *GetPanel( );

private:
	vgui::Panel *m_pPanel;
};


//-----------------------------------------------------------------------------
// Macros for help with simple registration of panel metaclass
// Put DECLARE_METACLASS_PANEL() in your class definition
// and CONSTRUCT_METACLASS_PANEL() in your class constructor
//-----------------------------------------------------------------------------
#define DECLARE_METACLASS_PANEL( _memberName )	CPanelWrapper _memberName
#define CONSTRUCT_METACLASS_PANEL( _memberName, _metaClassName, _parentPanel, _sortorder, _initData )	\
	_memberName.Activate( _metaClassName, _parentPanel, _sortorder, _initData )
#define DESTRUCT_METACLASS_PANEL( _memberName ) \
	_memberName.Deactivate()


//-----------------------------------------------------------------------------
// Helper KeyValues parsing methods
//-----------------------------------------------------------------------------
bool ParseRGBA( KeyValues* pValues, const char* pFieldName, int& r, int& g, int& b, int& a );
bool ParseRGBA( KeyValues* pValues, const char* pFieldName, Color& c );
bool ParseCoord( KeyValues* pValues, const char* pFieldName, int& x, int& y );
bool ParseRect( KeyValues* pValues, const char* pFieldName, int& x, int& y, int& w, int& h );


/* FIXME: Why do we have KeyValues too!?!??! Bleah
bool ParseRGBA( KeyValues *pValues, const char* pFieldName, int& r, int& g, int& b, int& a );
bool ParseRGBA( KeyValues* pValues, const char* pFieldName, vgui::Color& c ); */

#endif // PANELMETACLASSMGR_H
