#ifndef AUDIO_OUTPUT_DEVICES_WIN_H
#define AUDIO_OUTPUT_DEVICES_WIN_H
#ifdef _WIN64
#include "AudioOutputSelection.h"

// Synchronous, read-only enumeration on the calling thread. No COM pointers
// escape this call. Context is unused; IDs are complete UTF-8 MMDevice IDs.
bool EnumerateAudioOutputDevicesWin( void *pContext, CUtlVector<AudioOutputChoice_t> &choices );
#endif
#endif
