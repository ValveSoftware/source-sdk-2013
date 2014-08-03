//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A panel "metaclass" is a name given to a particular type of 
// panel with particular instance data. Such panels tend to be dynamically
// added and removed from their parent panels.
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "panelmetaclassmgr.h"
#include <KeyValues.h>
#include <vgui_controls/Panel.h>
#include "utldict.h"
#include "filesystem.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Helper KeyValue parsing methods
//-----------------------------------------------------------------------------
bool ParseRGBA( KeyValues *pValues, const char* pFieldName, int& r, int& g, int& b, int& a )
{
	r = g = b = a = 255;
	const char *pColorString = pValues->GetString( pFieldName, "255 255 255 255" );
	if ( !pColorString || !pColorString[ 0 ] )
		return false;

	// Try and scan them in
	int scanned;
	scanned = sscanf( pColorString, "%i %i %i %i", &r, &g, &b, &a );
	if ( scanned != 4 )
	{
		Warning( "Couldn't scan four color values from %s\n", pColorString );
		return false;
	}

	return true;
}

bool ParseRGBA( KeyValues* pValues, const char* pFieldName, Color& c )
{
	int r, g, b, a;
	if (!ParseRGBA( pValues, pFieldName, r, g, b, a ))
		return false;

	c.SetColor( r, g, b, a );
	return true;
}

//-----------------------------------------------------------------------------
// FIXME: Why do we have vgui::KeyValues too!?!??! Bleah
/*-----------------------------------------------------------------------------
bool ParseRGBA( KeyValues *pValues, const char* pFieldName, int& r, int& g, int& b, int& a )
{
	r = g = b = a = 255;
	const char *pColorString = pValues->GetString( pFieldName, "255 255 255 255" );
	if ( !pColorString || !pColorString[ 0 ] )
		return false;

	// Try and scan them in
	int scanned;
	scanned = sscanf( pColorString, "%i %i %i %i", &r, &g, &b, &a );
	if ( scanned != 4 )
	{
		Warning( "Couldn't scan four color values from %s\n", pColorString );
		return false;
	}

	return true;
}

bool ParseRGBA( KeyValues* pValues, const char* pFieldName, Color& c )
{
	int r, g, b, a;
	if (!ParseRGBA( pValues, pFieldName, r, g, b, a ))
		return false;

	c.SetColor( r, g, b, a );
	return true;
} */


bool ParseCoord( KeyValues *pValues, const char* pFieldName, int& x, int& y )
{
	x = y = 0;
	const char *pCoordString = pValues->GetString( pFieldName, "0 0" );
	if ( !pCoordString || !pCoordString[ 0 ] )
		return false;

	// Try and scan them in
	int scanned;
	scanned = sscanf( pCoordString, "%i %i", &x, &y );
	if ( scanned != 2 )
	{
		Warning( "Couldn't scan 2d coordinate values from %s\n", pCoordString );
		return false;
	}

	// coords are within 640x480 screen space
	x = ( x * ( ( float )ScreenWidth() / 640.0 ) );
	y = ( y * ( ( float )ScreenHeight() / 480.0 ) );

	return true;
}

bool ParseRect( KeyValues *pValues, const char* pFieldName, int& x, int& y, int& w, int& h )
{
	x = y = w = h = 0;
	const char *pRectString = pValues->GetString( pFieldName, "0 0 0 0" );
	if ( !pRectString || !pRectString[ 0 ] )
		return false;

	// Try and scan them in
	int scanned;
	scanned = sscanf( pRectString, "%i %i %i %i", &x, &y, &w, &h );
	if ( scanned != 4 )
	{
		Warning( "Couldn't scan rectangle values from %s\n", pRectString );
		return false;
	}

	// coords are within 640x480 screen space
	x = ( x * ( ( float )ScreenWidth() / 640.0 ) );
	y = ( y * ( ( float )ScreenHeight() / 480.0 ) );
	w = ( w * ( ( float )ScreenWidth() / 640.0 ) );
	h = ( h * ( ( float )ScreenHeight() / 480.0 ) );

	return true;
}


//-----------------------------------------------------------------------------
// Helper class to make meta class panels (for use in entities, so they autocleanup)
//-----------------------------------------------------------------------------
CPanelWrapper::CPanelWrapper() : m_pPanel(NULL)
{
}

CPanelWrapper::~CPanelWrapper()
{
	Deactivate();
}

void CPanelWrapper::Activate( char const* pMetaClassName, vgui::Panel *pParent, int sortorder, void *pVoidInitData )
{
	if ( m_pPanel )
	{
		Deactivate();
	}

	m_pPanel = PanelMetaClassMgr()->CreatePanelMetaClass( pMetaClassName, sortorder, pVoidInitData, pParent );
}

void CPanelWrapper::Deactivate( void )
{
	if ( m_pPanel )
	{
		PanelMetaClassMgr()->DestroyPanelMetaClass( m_pPanel );
		m_pPanel = NULL;
	}
}

vgui::Panel *CPanelWrapper::GetPanel( )
{
	return m_pPanel;
}


//-----------------------------------------------------------------------------
// Purpose: Singleton class responsible for managing metaclass panels 
//-----------------------------------------------------------------------------
class CPanelMetaClassMgrImp : public IPanelMetaClassMgr
{
public:
	// constructor, destructor
	CPanelMetaClassMgrImp();
	virtual ~CPanelMetaClassMgrImp();

	// Members of IPanelMetaClassMgr
	virtual void LoadMetaClassDefinitionFile( const char* pLevelName );
	virtual void InstallPanelType( const char* pPanelName, IPanelFactory* pFactory );
	virtual vgui::Panel *CreatePanelMetaClass( const char* pMetaClassName,
		int sortorder, void *pInitData, vgui::Panel *pParent, const char *pChainName );
	virtual void DestroyPanelMetaClass( vgui::Panel *pPanel );

private:
	struct MetaClassDict_t
	{
		unsigned short	m_KeyValueIndex;
		unsigned short	m_TypeIndex;
		KeyValues*		m_pKeyValues;
	};

	// various parsing helper methods
	bool ParseSingleMetaClass( const char* pFileName, const char* pInstanceName,
		KeyValues* pMetaClass, int keyValueIndex );
	bool ParseMetaClassList( const char* pFileName, KeyValues* pKeyValues, int keyValueIndex );

	// No copy constructor
	CPanelMetaClassMgrImp( const CPanelMetaClassMgrImp & );

	// List of panel types...
	CUtlDict< IPanelFactory*, unsigned short > m_PanelTypeDict;

	// List of metaclass types
	CUtlDict< MetaClassDict_t, unsigned short > m_MetaClassDict;

	// Create key value accesor
	CUtlDict< KeyValues*, unsigned short >	m_MetaClassKeyValues;
};


//-----------------------------------------------------------------------------
// Returns the singleton panel metaclass mgr interface
//-----------------------------------------------------------------------------
IPanelMetaClassMgr* PanelMetaClassMgr()
{
	// NOTE: the CPanelFactory implementation requires the local static here
	// even though it means an extra check every time PanelMetaClassMgr is accessed
	static CPanelMetaClassMgrImp s_MetaClassMgrImp;
	return &s_MetaClassMgrImp;
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
CPanelMetaClassMgrImp::CPanelMetaClassMgrImp() : m_PanelTypeDict( true, 0, 32 )
{
}

CPanelMetaClassMgrImp::~CPanelMetaClassMgrImp()
{
}


//-----------------------------------------------------------------------------
// Call this to install a new panel type
//-----------------------------------------------------------------------------
void CPanelMetaClassMgrImp::InstallPanelType( const char* pPanelName, IPanelFactory* pFactory )
{
	Assert( pPanelName && pFactory );
	
	// convert to lowercase
	int len = Q_strlen(pPanelName) + 1;
	char* pTemp = (char*)stackalloc( len );
	Q_strncpy( pTemp, pPanelName, len );
	Q_strnlwr( pTemp, len );

	m_PanelTypeDict.Insert( pTemp, pFactory );

	stackfree( pTemp );
}


//-----------------------------------------------------------------------------
// Parse a single metaclass
//-----------------------------------------------------------------------------
bool CPanelMetaClassMgrImp::ParseSingleMetaClass( const char* pFileName,
	const char* pMetaClassName, KeyValues* pMetaClassValues, int keyValueIndex )
{
	// Complain about duplicately defined metaclass names...
	if ( m_MetaClassDict.Find( pMetaClassName ) != m_MetaClassDict.InvalidIndex() )
	{
		Warning( "Meta class %s duplicately defined (file %s)\n", pMetaClassName, pFileName );
		return false;
	}

	// find the type...
	const char* pPanelType = pMetaClassValues->GetString( "type" );
	if (!pPanelType || !pPanelType[0])
	{
		Warning( "Unable to find type of meta class %s in file %s\n", pMetaClassName, pFileName );
		return false;
	}

	unsigned short i = m_PanelTypeDict.Find( pPanelType );
	if (i == m_PanelTypeDict.InvalidIndex())
	{
		Warning( "Type %s of meta class %s undefined!\n", pPanelType, pMetaClassName );
		stackfree(pLwrMetaClass);
		return false;
	}

	// Add it to the metaclass dictionary
	MetaClassDict_t element;
	element.m_TypeIndex = i;
	element.m_KeyValueIndex = keyValueIndex;
	element.m_pKeyValues = pMetaClassValues;
	m_MetaClassDict.Insert( pMetaClassName, element );

	return true;
}


//-----------------------------------------------------------------------------
// Parse the metaclass list
//-----------------------------------------------------------------------------
bool CPanelMetaClassMgrImp::ParseMetaClassList( const char* pFileName, 
												  KeyValues* pKeyValues, int keyValueIdx )
{
	// Iterate over all metaclasses...
	KeyValues* pIter = pKeyValues->GetFirstSubKey();
	while( pIter )
	{
		if (!ParseSingleMetaClass( pFileName, pIter->GetName(), pIter, keyValueIdx ))
		{
		//	return false;
			Warning( "MetaClass missing for %s\n", pIter->GetName() );
		}
		pIter = pIter->GetNextKey();
	}

	return true;
}


//-----------------------------------------------------------------------------
// Loads up a file containing metaclass definitions
//-----------------------------------------------------------------------------
void CPanelMetaClassMgrImp::LoadMetaClassDefinitionFile( const char *pFileName )
{
	MEM_ALLOC_CREDIT();

	// Blat out previous metaclass definitions read in from this file...
	int i = m_MetaClassKeyValues.Find( pFileName );
	if (i != m_MetaClassKeyValues.InvalidIndex() )
	{
		// Blow away the previous keyvalues	from that file
		unsigned short j = m_MetaClassDict.First();
		while ( j != m_MetaClassDict.InvalidIndex() )
		{
			unsigned short next = m_MetaClassDict.Next(j);
			if ( m_MetaClassDict[j].m_KeyValueIndex == i)
			{
				m_MetaClassDict.RemoveAt(j);
			}

			j = next;
		}

		m_MetaClassKeyValues[i]->deleteThis();
		m_MetaClassKeyValues.RemoveAt(i); 
	}

	// Create a new keyvalues entry
	KeyValues* pKeyValues = new KeyValues(pFileName);
	int idx = m_MetaClassKeyValues.Insert( pFileName, pKeyValues );

	// Read in all metaclass definitions...

	// Load the file
	if ( !pKeyValues->LoadFromFile( filesystem, pFileName ) )
	{
		Warning( "Couldn't find metaclass definition file %s\n", pFileName );
		pKeyValues->deleteThis();
		m_MetaClassKeyValues.RemoveAt(idx);
		return;
	}
	else
	{
		// Go ahead and parse the data now
		if ( !ParseMetaClassList( pFileName, pKeyValues, idx ) )
		{
			Warning( "Detected one or more errors parsing %s\n", pFileName );
		}
	}
}


//-----------------------------------------------------------------------------
// Performs key value chaining
//-----------------------------------------------------------------------------
static void KeyValueChainRecursive( KeyValues* pKeyValues, const char *pSectionName )
{
	KeyValues* pSection = pKeyValues->FindKey( pSectionName );

	if (pSection)
	{
		pKeyValues->ChainKeyValue( pSection );
	}

	KeyValues* pIter = pKeyValues->GetFirstSubKey();
	while (pIter)
	{
		// Don't both setting up links on a keyvalue that has no children
		if (pIter->GetFirstSubKey())
			KeyValueChainRecursive( pIter, pSectionName );

		pIter = pIter->GetNextKey();
	}
}


//-----------------------------------------------------------------------------
// Create, destroy panel...
//-----------------------------------------------------------------------------
vgui::Panel *CPanelMetaClassMgrImp::CreatePanelMetaClass( const char* pMetaClassName,
	int sortorder, void *pInitData, vgui::Panel *pParent, const char *pChainName )
{
	// Search for the metaclass name
	int i = m_MetaClassDict.Find( pMetaClassName );
	if (i == m_MetaClassDict.InvalidIndex())
		return NULL; 

	// Now that we've got the metaclass, we can figure out what kind of
	// panel to instantiate...
	MetaClassDict_t &metaClass = m_MetaClassDict[i];
	IPanelFactory* pFactory = m_PanelTypeDict[metaClass.m_TypeIndex];

	// Set up the key values for use in initialization
	if (pChainName)
	{
		KeyValueChainRecursive( metaClass.m_pKeyValues, pChainName );
	}

	// Create and initialize the panel
	vgui::Panel *pPanel = pFactory->Create( pMetaClassName, metaClass.m_pKeyValues, pInitData, pParent );
	if ( pPanel )
	{
		pPanel->SetZPos( sortorder );
	}

	return pPanel;
}

void CPanelMetaClassMgrImp::DestroyPanelMetaClass( vgui::Panel *pPanel )
{
//	if ( pPanel )
//		pPanel->MarkForDeletion();
	delete pPanel;
}


