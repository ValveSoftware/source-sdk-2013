//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef SCENETOKENPROCESSOR_H
#define SCENETOKENPROCESSOR_H
#ifdef _WIN32
#pragma once
#endif

class ISceneTokenProcessor;

ISceneTokenProcessor *GetTokenProcessor();
void SetTokenProcessorBuffer( const char *buf );

#endif // SCENETOKENPROCESSOR_H
