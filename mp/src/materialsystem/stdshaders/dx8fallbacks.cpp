#include "BaseVSShader.h"

// This one isn't supported on dx8
DEFINE_FALLBACK_SHADER( SDK_DepthWrite, Wireframe )

DEFINE_FALLBACK_SHADER( SDK_EyeRefract, Eyes_dx8 )
DEFINE_FALLBACK_SHADER( SDK_VolumeClouds, UnlitGeneric_DX8 )

// FIXME: These aren't supported on dx8, but need to be.
DEFINE_FALLBACK_SHADER( SDK_EyeGlint, EyeGlint )
DEFINE_FALLBACK_SHADER( SDK_AfterShock, AfterShock )
