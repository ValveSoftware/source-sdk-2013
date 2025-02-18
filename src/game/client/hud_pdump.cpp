//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hud_pdump.h"
#include "iclientmode.h"
#include "predictioncopy.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CPDumpPanel *g_pPDumpPanel = NULL;


// OKAY, so typeinfo.h somewhere re-enables a bunch of warnings about float to int conversion, etc., that
//  we pragma'd away in platform.h, so this little compiler specific hack will eliminate those warnings while
//  retaining our own warning setup...ywb
#if defined( WIN32 ) && _MSC_VER <= 1920
#pragma warning( push )
#include <typeinfo.h>
#pragma warning( pop )
#endif

using namespace vgui;

CPDumpPanel *GetPDumpPanel()
{
	return g_pPDumpPanel;
}

DECLARE_HUDELEMENT( CPDumpPanel );

CPDumpPanel::CPDumpPanel( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudPredictionDump" )
{
	g_pPDumpPanel = this;

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetProportional( false );
}

CPDumpPanel::~CPDumpPanel()
{
	g_pPDumpPanel = NULL;
}

void CPDumpPanel::ApplySettings( KeyValues *inResourceData )
{
	SetProportional( false );

	BaseClass::ApplySettings( inResourceData );
}

void CPDumpPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetProportional( false );
	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPDumpPanel::ShouldDraw()
{
	if ( m_DumpEntityInfo.Count() == 0 )
		return false;

	return CHudElement::ShouldDraw();
}

void CPDumpPanel::DumpComparision( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char *value )
{
	if ( fieldname == NULL )
		return;

	int idx = m_DumpEntityInfo.AddToTail();

	DumpInfo *slot = &m_DumpEntityInfo[ idx ];

	Q_snprintf( slot->classname, sizeof( slot->classname ), "%s", classname );
	slot->networked = networked;
	Q_snprintf( slot->fieldstring, sizeof( slot->fieldstring ), "%s %s",
		fieldname,
		value );

	slot->differs = differs;
	slot->withintolerance = withintolerance;
	slot->noterrorchecked = noterrorchecked;
}

//-----------------------------------------------------------------------------
// Purpose: Callback function for dumping entity info to screen
// Input  : *classname - 
//			*fieldname - 
//			*fieldtype - 
//			networked - 
//			noterrorchecked - 
//			differs - 
//			withintolerance - 
//			*value - 
// Output : static void
//-----------------------------------------------------------------------------
static void DumpComparision( const char *classname, const char *fieldname, const char *fieldtype,
	bool networked, bool noterrorchecked, bool differs, bool withintolerance, const char *value )
{
	if ( !g_pPDumpPanel )
		return;

	g_pPDumpPanel->DumpComparision( classname, fieldname, fieldtype, networked, noterrorchecked, differs, withintolerance, value );
}

//-----------------------------------------------------------------------------
// Purpose: Lookup color to use for data
// Input  : networked - 
//			errorchecked - 
//			differs - 
//			withintolerance - 
//			r - 
//			g - 
//			b - 
//			a - 
// Output : static void
//-----------------------------------------------------------------------------
void CPDumpPanel::PredictionDumpColor( bool networked, bool errorchecked, bool differs, bool withintolerance,
	int& r, int& g, int& b, int& a )
{
	r = 255;
	g = 255;
	b = 255;
	a = 255;

	if ( networked )
	{
		if ( errorchecked )
		{
			r = 180;
			g = 180;
			b = 225;
		}
		else
		{
			r = 150;
			g = 180;
			b = 150;
		}
	}

	if ( differs )
	{
		if ( withintolerance )
		{
			r = 255;
			g = 255;
			b = 0;
			a = 255;
		}
		else
		{
			if ( !networked )
			{
				r = 180;
				g = 180;
				b = 100;
				a = 255;
			}
			else
			{
				r = 255;
				g = 0;
				b = 0;
				a = 255;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Dump entity data to screen
// Input  : *ent - 
//			last_predicted - 
//-----------------------------------------------------------------------------
void CPDumpPanel::DumpEntity( C_BaseEntity *ent, int commands_acknowledged )
{
	if ( IsXbox() )
	{
		return;
	}

#ifdef NO_ENTITY_PREDICTION
	return;
#else
	Assert( ent );

	void *original_state_data = NULL;	
	const void *predicted_state_data	= NULL;
	
	bool data_type_original		= PC_DATA_PACKED;
	bool data_type_predicted	= PC_DATA_PACKED;

	if ( ent->GetPredictable() )
	{
		original_state_data		= ent->GetOriginalNetworkDataObject();	
		predicted_state_data	= ent->GetPredictedFrame( commands_acknowledged - 1 );	
	}
	else
	{
		// Compare against self so that we're just dumping data to screen
		original_state_data = ( void * )ent;
		data_type_original = PC_DATA_NORMAL;
		predicted_state_data = original_state_data;
		data_type_predicted = data_type_original;
	}

	Assert( original_state_data );
	Assert( predicted_state_data );

	Clear();

	CPredictionCopy datacompare( PC_EVERYTHING, 
		original_state_data, data_type_original, 
		predicted_state_data, data_type_predicted, 
		true,  // counterrors
		true,  // reporterrors
		false, // copy data
		true,   // describe fields
		::DumpComparision );
	// Don't spew debugging info
	datacompare.TransferData( "", -1, ent->GetPredDescMap() );

	m_hDumpEntity = ent;
#endif
}

void CPDumpPanel::Clear()
{
	m_DumpEntityInfo.RemoveAll();
}

void CPDumpPanel::Paint()
{
	C_BaseEntity *ent = m_hDumpEntity;
	if ( !ent )
	{
		Clear();
		return;
	}

	// Now output the strings
	int x[5];
	x[0] = 20;
	int columnwidth = 640;
	int numcols = GetWide() / columnwidth;
	int i;

	numcols = clamp( numcols, 1, 5 );

	for ( i = 0; i < numcols; i++ )
	{
		if ( i == 0 )
		{
			x[i] = 20;
		}
		else
		{
			x[i] = x[ i-1 ] + columnwidth - 20;
		}
	}

	int c = m_DumpEntityInfo.Size();
	int fonttall = vgui::surface()->GetFontTall( m_FontSmall ) - 3;
	int fonttallMedium = vgui::surface()->GetFontTall( m_FontMedium );
	int fonttallBig = vgui::surface()->GetFontTall( m_FontBig );

	char currentclass[ 128 ];
	currentclass[ 0 ] = 0;

	int starty = 60;
	int y = starty;

	int col = 0;

	int r = 255;
	int g = 255;
	int b = 255;
	int a = 255;

	char classextra[ 32 ];
	classextra[ 0 ] = 0;
	char classprefix[ 32 ];
	Q_strncpy( classprefix, "class ", sizeof( classprefix ) );
	const char *classname = ent->GetClassname();
	if ( !classname[ 0 ] )
	{
		classname = typeid( *ent ).name();
		Q_strncpy( classextra, " (classmap missing)", sizeof( classextra ) );
		classprefix[ 0 ] = 0;
	}

	char sz[ 512 ];
	wchar_t szconverted[ 1024 ];

	surface()->DrawSetTextFont( m_FontBig );
	surface()->DrawSetTextColor( Color( 255, 255, 255, 255 ) );
	surface()->DrawSetTextPos( x[ col ] - 10, y - fonttallBig - 2 );
	Q_snprintf( sz, sizeof( sz ), "entity # %i: %s%s%s", ent->entindex(), classprefix, classname, classextra );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	for ( i = 0; i < c; i++ )
	{
		DumpInfo *slot = &m_DumpEntityInfo[ i ];

		if ( stricmp( slot->classname, currentclass ) )
		{
			y += 2;

			surface()->DrawSetTextFont( m_FontMedium );
			surface()->DrawSetTextColor( Color( 0, 255, 100, 255 ) );
			surface()->DrawSetTextPos( x[ col ] - 10, y );
			Q_snprintf( sz, sizeof( sz ), "%s", slot->classname );
			g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
			surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

			y += fonttallMedium-1;
			Q_strncpy( currentclass, slot->classname, sizeof( currentclass ) );
		}

	
		PredictionDumpColor( slot->networked, !slot->noterrorchecked, slot->differs, slot->withintolerance,
			r, g, b, a );

		surface()->DrawSetTextFont( m_FontSmall );
		surface()->DrawSetTextColor( Color( r, g, b, a ) );
		surface()->DrawSetTextPos( x[ col ], y );
		Q_snprintf( sz, sizeof( sz ), "%s", slot->fieldstring );
		g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
		surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

		y += fonttall;

		if ( y >= GetTall() - fonttall - 60 )
		{
			y = starty;
			col++;
			if ( col >= numcols )
				break;
		}
	}

	surface()->DrawSetTextFont( m_FontSmall );


	// Figure how far over the legend needs to be.
	const char *pFirstAndLongestString = "Not networked, no differences";
	g_pVGuiLocalize->ConvertANSIToUnicode( pFirstAndLongestString, szconverted, sizeof(szconverted)  );
	int textSizeWide, textSizeTall;
	surface()->GetTextSize( m_FontSmall, szconverted, textSizeWide, textSizeTall );


	// Draw a legend now
	int xpos = GetWide() - textSizeWide - 5;
	y = GetTall() - 7 * fonttall - 80;

	// Not networked, no differences
	PredictionDumpColor( false, false, false, false, r, g, b, a );


	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, pFirstAndLongestString, sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;

	// Networked, no error check
	PredictionDumpColor( true, false, false, false, r, g, b, a );

	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, "Networked, not checked", sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;

	// Networked, with error check
	PredictionDumpColor( true, true, false, false, r, g, b, a );

	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, "Networked, error checked", sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;

	// Differs, but within tolerance
	PredictionDumpColor( true, true, true, true, r, g, b, a );

	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, "Differs, but within tolerance", sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;

	// Differs, not within tolerance, but not networked
	PredictionDumpColor( false, true, true, false, r, g, b, a );

	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, "Differs, but not networked", sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;

	// Differs, networked, not within tolerance
	PredictionDumpColor( true, true, true, false, r, g, b, a );

	surface()->DrawSetTextColor( Color( r, g, b, a ) );
	surface()->DrawSetTextPos( xpos, y );
	Q_strncpy( sz, "Differs, networked", sizeof( sz ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, szconverted, sizeof(szconverted)  );
	surface()->DrawPrintText( szconverted, wcslen( szconverted ) );

	y += fonttall;
}