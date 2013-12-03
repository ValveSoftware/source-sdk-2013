//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef VIEWPOSTPROCESS_H
#define VIEWPOSTPROCESS_H

#if defined( _WIN32 )
#pragma once
#endif

void DoEnginePostProcessing( int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui = false );
void DoImageSpaceMotionBlur( const CViewSetup &view, int x, int y, int w, int h );
void DumpTGAofRenderTarget( const int width, const int height, const char *pFilename );

#endif // VIEWPOSTPROCESS_H
