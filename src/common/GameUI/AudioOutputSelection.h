// Audio output selection UI support. Engine enumeration is supplied by the host.
#ifndef AUDIO_OUTPUT_SELECTION_H
#define AUDIO_OUTPUT_SELECTION_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlstring.h"
#include "tier1/utlvector.h"

class ICvar;
class IVEngineClient;
class ConVar;
namespace vgui { class ComboBox; }

// Values must come from a verified engine or platform list, in exactly the encoding
// accepted by sound_device_override. Labels are UTF-8 display text.
struct AudioOutputChoice_t
{
	CUtlString m_OverrideValue;
	CUtlString m_DisplayName;
	bool m_bAvailable;
	AudioOutputChoice_t() : m_bAvailable( false ) {}
};

// Local UI integration seam, NOT an exported engine interface. The host must
// supply structured enumeration with verified override IDs, never console parsing.
// Return false when unsupported or refresh fails. Return owned copies, with
// unique override values. Include a default-policy row only when its exact
// override value and semantics have been verified. No default ID is invented.
typedef bool (*AudioOutputChoicesFn)( void *pContext, CUtlVector<AudioOutputChoice_t> &choices );

class CAudioOutputSelection
{
public:
	enum ApplyResult_t
	{
		UNCHANGED,
		PREFERENCE_ACCEPTED, // ConVar readback only; not proof of audible output.
		UNSUPPORTED,
		CONNECTED,
		EXTERNALLY_CHANGED,
		DEVICE_UNAVAILABLE,
		PREFERENCE_REJECTED
	};

	// All operations and the provider run on the UI/main thread. Dependencies
	// must outlive this helper; the helper never retains engine device pointers.
	CAudioOutputSelection( ICvar *pCvar, IVEngineClient *pEngine,
		AudioOutputChoicesFn pEnumerate, void *pContext );
	bool Reset( vgui::ComboBox &combo ); // Open/reset/cancel; never changes sound.
	void UpdateEnabled( vgui::ComboBox &combo ) const;
	ApplyResult_t Apply( vgui::ComboBox &combo );

private:
	ConVar *FindOverride() const;
	bool IsConnectionBlocked() const;
	bool Enumerate( CUtlVector<AudioOutputChoice_t> &choices ) const;
	ICvar *m_pCvar;
	IVEngineClient *m_pEngine;
	AudioOutputChoicesFn m_pEnumerate;
	void *m_pContext;
	CUtlString m_OriginalValue;
	bool m_bReady;
};

#endif
