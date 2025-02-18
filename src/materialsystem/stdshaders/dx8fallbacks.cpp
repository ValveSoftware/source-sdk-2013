//========= Copyright Valve Corporation, All rights reserved. ============//
#include "BaseVSShader.h"

// This one isn't supported on dx8
DEFINE_FALLBACK_SHADER( DepthWrite, Wireframe )

DEFINE_FALLBACK_SHADER( EyeRefract, Eyes_dx8 )
DEFINE_FALLBACK_SHADER( bik, bik_dx81 )
DEFINE_FALLBACK_SHADER( VolumeClouds, UnlitGeneric_DX8 )
DEFINE_FALLBACK_SHADER( DebugTextureView, UnlitGeneric_DX8 )

// FIXME: These aren't supported on dx8, but need to be.
DEFINE_FALLBACK_SHADER( EyeGlint, Wireframe )
DEFINE_FALLBACK_SHADER( AfterShock, Wireframe )
