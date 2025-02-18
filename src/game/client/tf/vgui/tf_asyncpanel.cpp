#include "cbase.h"
#include "tf_asyncpanel.h"
#include "econ_controls.h"

static const float k_flRequestInterval = 5.f;
static const float k_flNeverRequestUpdate = -1.f;


CBaseASyncPanel::CBaseASyncPanel( Panel *pParent, const char *pszPanelName )
	: EditablePanel( pParent, pszPanelName )
	, m_flLastRequestTime( 0.f )
	, m_flLastUpdatedTime( 0.f )
	, m_bDataInitialized( false )
	, m_bSettingsApplied( false )
{
	ivgui()->AddTickSignal( GetVPanel(), 1.f );
}

void CBaseASyncPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bSettingsApplied = true;
	m_flLastUpdatedTime = 0.f; // Force a refresh
}


void CBaseASyncPanel::LoadControlSettings(const char *dialogResourceName, const char *pathID, KeyValues *pPreloadedKeyValues, KeyValues *pConditions )
{
	m_vecLoadingPanels.Purge();
	m_vecPanelsToShow.Purge();

	BaseClass::LoadControlSettings( dialogResourceName, pathID, pPreloadedKeyValues, pConditions );
}

void CBaseASyncPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	FOR_EACH_VEC( m_vecLoadingPanels, i )
	{
		m_vecLoadingPanels[ i ]->SetVisible( true );
	}

	FOR_EACH_VEC( m_vecPanelsToShow, i )
	{
		m_vecPanelsToShow[ i ]->SetVisible( false );
	}
}


void CBaseASyncPanel::OnChildSettingsApplied( KeyValues *pInResourceData, Panel *pChild )
{
	const char *pszAsync = pInResourceData->GetString( "asynchandling", NULL );

	if ( pszAsync == NULL )
		return;

	if ( FStrEq( pszAsync, "content" ) )
	{
		m_vecPanelsToShow[ m_vecPanelsToShow.AddToTail() ].Set( pChild );
	}
	else if ( FStrEq( pszAsync, "loading" ) )
	{
		m_vecLoadingPanels[ m_vecLoadingPanels.AddToTail() ].Set( pChild );
	}
}

bool CBaseASyncPanel::IsInitialized() const
{
	return m_bDataInitialized;
}

void CBaseASyncPanel::PresentDataIfReady()
{
	if ( m_bDataInitialized && m_bSettingsApplied )
	{
		FOR_EACH_VEC( m_vecLoadingPanels, i )
		{
			m_vecLoadingPanels[ i ]->SetVisible( false );
		}

		FOR_EACH_VEC( m_vecPanelsToShow, i )
		{
			m_vecPanelsToShow[ i ]->SetVisible( true );
		}

		// stop ticking. job is done.
		ivgui()->RemoveTickSignal( GetVPanel() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Checks if data is ready.  If so, mark the time and hide the loading image
//-----------------------------------------------------------------------------
void CBaseASyncPanel::CheckForData()
{
	m_flLastRequestTime = Plat_FloatTime();
	if ( CheckForData_Internal() )
	{
		m_flLastUpdatedTime = Plat_FloatTime();
		m_bDataInitialized = true;
		PresentDataIfReady();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if we need to check for data
//-----------------------------------------------------------------------------
void CBaseASyncPanel::OnTick()
{
	const float flTimeSinceUpdate = Plat_FloatTime() - m_flLastUpdatedTime;
	const float flTimeSinceRequest = Plat_FloatTime() - m_flLastRequestTime;
	// Need an update if we're beyodn the refresh delay, and our refresh delay isn't k_flNeverRequestUpdate, or if we're just not initialized
	const bool bNeedsUpdate = ( ( flTimeSinceUpdate > m_flRefreshDelay ) && ( m_flRefreshDelay != k_flNeverRequestUpdate ) ) || m_flLastUpdatedTime == 0.f;

	if ( m_bSettingsApplied && bNeedsUpdate && flTimeSinceRequest > k_flRequestInterval )
	{
		CheckForData();
	}
}
