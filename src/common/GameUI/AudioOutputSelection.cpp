#include "AudioOutputSelection.h"
#include "icvar.h"
#include "cdll_int.h"
#include "tier1/convar.h"
#include "tier1/KeyValues.h"
#include "vgui_controls/ComboBox.h"

#include "tier0/memdbgon.h"

CAudioOutputSelection::CAudioOutputSelection( ICvar *pCvar, IVEngineClient *pEngine,
	AudioOutputChoicesFn pEnumerate, void *pContext ) :
	m_pCvar( pCvar ), m_pEngine( pEngine ), m_pEnumerate( pEnumerate ),
	m_pContext( pContext ), m_bReady( false )
{
}

ConVar *CAudioOutputSelection::FindOverride() const
{
	ConVar *pVar = m_pCvar ? m_pCvar->FindVar( "sound_device_override" ) : NULL;
	// Require the known persistent, disconnected-only contract. Fail closed on
	// incompatible builds rather than defining a replacement ConVar.
	if ( !pVar || !pVar->IsFlagSet( FCVAR_ARCHIVE ) || !pVar->IsFlagSet( FCVAR_NOT_CONNECTED ) ||
		pVar->IsFlagSet( FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN |
			FCVAR_REPLICATED | FCVAR_NEVER_AS_STRING | FCVAR_MATERIAL_THREAD_MASK ) )
		return NULL;
	return pVar;
}

bool CAudioOutputSelection::IsConnectionBlocked() const
{
	// IsInGame alone misses loading/connected clients. Also conservatively block
	// loading; do not disconnect a player or queue a change for later execution.
	return !m_pEngine || m_pEngine->IsConnected() || m_pEngine->IsInGame() ||
		m_pEngine->IsDrawingLoadingImage();
}

bool CAudioOutputSelection::Enumerate( CUtlVector<AudioOutputChoice_t> &choices ) const
{
	choices.RemoveAll();
	if ( !m_pEnumerate || !m_pEnumerate( m_pContext, choices ) )
		return false;
	FOR_EACH_VEC( choices, i )
	{
		if ( !choices[i].m_DisplayName.Length() )
			return false;
		for ( int j = 0; j < i; ++j )
		{
			if ( !V_strcmp( choices[i].m_OverrideValue.Get(), choices[j].m_OverrideValue.Get() ) )
				return false;
		}
	}
	return true;
}

bool CAudioOutputSelection::Reset( vgui::ComboBox &combo )
{
	m_bReady = false;
	combo.RemoveAll();
	combo.SetEnabled( false );
	ConVar *pVar = FindOverride();
	CUtlVector<AudioOutputChoice_t> choices;
	if ( !m_pEngine || !pVar || !Enumerate( choices ) )
		return false;

	m_OriginalValue = pVar->GetString();
	int selected = -1;
	bool selectedAvailable = false;
	FOR_EACH_VEC( choices, i )
	{
		KeyValues::AutoDelete data( "AudioOutput" );
		data->SetString( "override", choices[i].m_OverrideValue.Get() );
		data->SetBool( "available", choices[i].m_bAvailable );
		// UTF-8 needs no more wide code units than input bytes. Avoid fixed-size
		// truncation and keep names separate from opaque, case-sensitive IDs.
		CUtlVector<wchar_t> label;
		label.SetCount( choices[i].m_DisplayName.Length() + 1 );
		V_UTF8ToUnicode( choices[i].m_DisplayName.Get(), label.Base(), label.Count() * sizeof( wchar_t ) );
		// The verified empty override is a policy, not an endpoint name. Its
		// warning is localized separately; endpoint labels remain literal text.
		int item = choices[i].m_OverrideValue.Length() ? combo.AddItem( label.Base(), data ) :
			combo.AddItem( "#GameUI_AudioOutputDefault", data );
		combo.SetItemEnabled( item, choices[i].m_bAvailable );
		if ( !V_strcmp( choices[i].m_OverrideValue.Get(), m_OriginalValue.Get() ) )
		{
			selected = item;
			selectedAvailable = choices[i].m_bAvailable;
		}
	}
	if ( selected == -1 )
	{
		// Preserve an unknown/missing saved value; opening and accepting options
		// must not replace it with the first available device.
		KeyValues::AutoDelete data( "AudioOutput" );
		data->SetString( "override", m_OriginalValue.Get() );
		data->SetBool( "available", false );
		selected = combo.AddItem( "#GameUI_AudioOutputSavedUnavailable", data );
		combo.SetItemEnabled( selected, false );
	}
	// Menu::SilentActivateItem ignores disabled entries. Activate the saved row
	// before disabling it, so Apply without edits can preserve an unavailable ID.
	combo.SetItemEnabled( selected, true );
	combo.SilentActivateItem( selected );
	combo.SetItemEnabled( selected, selectedAvailable );
	m_bReady = true;
	UpdateEnabled( combo );
	return true;
}

void CAudioOutputSelection::UpdateEnabled( vgui::ComboBox &combo ) const
{
	combo.SetEnabled( m_bReady && FindOverride() && !IsConnectionBlocked() );
}

CAudioOutputSelection::ApplyResult_t CAudioOutputSelection::Apply( vgui::ComboBox &combo )
{
	ConVar *pVar = FindOverride();
	if ( !m_bReady || !pVar || !m_pEngine )
		return UNSUPPORTED;
	KeyValues *pData = combo.GetActiveItemUserData();
	if ( !pData )
		return DEVICE_UNAVAILABLE;
	const CUtlString requested( pData->GetString( "override" ) );
	if ( V_strcmp( pVar->GetString(), m_OriginalValue.Get() ) )
		return EXTERNALLY_CHANGED;
	if ( !V_strcmp( requested.Get(), m_OriginalValue.Get() ) )
		return UNCHANGED;
	if ( IsConnectionBlocked() )
		return CONNECTED;

	// Refresh at Apply; never trust the old row index or display name.
	CUtlVector<AudioOutputChoice_t> choices;
	if ( !Enumerate( choices ) )
		return UNSUPPORTED;
	bool found = false;
	FOR_EACH_VEC( choices, i )
	{
		if ( choices[i].m_bAvailable && !V_strcmp( choices[i].m_OverrideValue.Get(), requested.Get() ) )
			found = true;
	}
	if ( !found )
		return DEVICE_UNAVAILABLE;
	// Recheck after provider execution. Generic ConVar::SetValue does not itself
	// enforce NOT_CONNECTED. Main-thread host integration is mandatory.
	pVar = FindOverride();
	if ( !pVar )
		return UNSUPPORTED;
	if ( IsConnectionBlocked() )
		return CONNECTED;
	if ( V_strcmp( pVar->GetString(), m_OriginalValue.Get() ) )
		return EXTERNALLY_CHANGED;

	pVar->SetValue( requested.Get() ); // Preserve existing engine change callback.
	// No extra snd_restart, replacement callback, config file or console command.
	pVar = FindOverride();
	if ( !pVar || V_strcmp( pVar->GetString(), requested.Get() ) )
		return PREFERENCE_REJECTED;
	m_OriginalValue = requested;
	return PREFERENCE_ACCEPTED;
}
