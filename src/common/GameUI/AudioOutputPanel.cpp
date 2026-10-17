#ifdef _WIN64
#include "AudioOutputPanel.h"
#include "AudioOutputDevicesWin.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Controls.h"
#include "tier0/memdbgon.h"

CAudioOutputPanel::CAudioOutputPanel( vgui::Panel *parent, ICvar *cvar, IVEngineClient *engine ) :
	BaseClass( parent, "AudioOutputPanel" ),
	m_Selection( cvar, engine, EnumerateAudioOutputDevicesWin, NULL ), m_bReady( false )
{
	// This prototype ships English tokens only. Load that fallback explicitly
	// instead of requesting a missing per-language resource.
	g_pVGuiLocalize->AddFile( "resource/audio_output_english.txt" );
	m_pLabel = new vgui::Label( this, "AudioOutputLabel", "#GameUI_AudioOutput" );
	m_pDevices = new vgui::ComboBox( this, "AudioOutputDevices", 8, false );
	m_pRefresh = new vgui::Button( this, "AudioOutputRefresh", "#GameUI_AudioOutputRefresh", this, "RefreshAudio" );
	m_pStatus = new vgui::Label( this, "AudioOutputStatus", "" );
	m_pStatus->SetWrap( true );
	Reset();
}

void CAudioOutputPanel::Reset()
{
	// Explicit Reset / Refresh discards only this control's staged selection.
	m_bReady = m_Selection.Reset( *m_pDevices );
	m_pStatus->SetText( m_bReady ? "#GameUI_AudioOutputDisconnected" : "#GameUI_AudioOutputUnavailable" );
}

void CAudioOutputPanel::OnCommand( const char *command )
{
	if ( !V_stricmp( command, "RefreshAudio" ) ) { Reset(); return; }
	BaseClass::OnCommand( command );
}

void CAudioOutputPanel::OnThink()
{
	BaseClass::OnThink();
	m_Selection.UpdateEnabled( *m_pDevices );
}

void CAudioOutputPanel::PerformLayout()
{
	BaseClass::PerformLayout();
	const int gap = 5;
	const int line = GetTall() / 4;
	const int width = GetWide() - 2 * gap;
	m_pLabel->SetBounds( gap, 0, width, line );
	m_pDevices->SetBounds( gap, line, width, line );
	m_pRefresh->SetBounds( gap, 2 * line, width / 3, line );
	m_pStatus->SetBounds( gap, 3 * line, width, line );
}

bool CAudioOutputPanel::Apply()
{
	// A failed initial enumeration must not prevent saving unrelated settings.
	if ( !m_bReady ) return true;
	CAudioOutputSelection::ApplyResult_t result = m_Selection.Apply( *m_pDevices );
	if ( result == CAudioOutputSelection::UNCHANGED || result == CAudioOutputSelection::PREFERENCE_ACCEPTED )
		return true;
	const char *message = "#GameUI_AudioOutputRejected";
	switch ( result )
	{
	case CAudioOutputSelection::CONNECTED: message = "#GameUI_AudioOutputDisconnected"; break;
	case CAudioOutputSelection::EXTERNALLY_CHANGED: message = "#GameUI_AudioOutputConflict"; break;
	case CAudioOutputSelection::DEVICE_UNAVAILABLE: message = "#GameUI_AudioOutputMissing"; break;
	case CAudioOutputSelection::UNSUPPORTED: message = "#GameUI_AudioOutputUnavailable"; break;
	default: break;
	}
	m_pStatus->SetText( message );
	return false;
}
#endif
