//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "prediction.h"
#include "hl_movedata.h"
#include "c_basehlplayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CHLMoveData g_HLMoveData;
CMoveData *g_pMoveData = &g_HLMoveData;

// Expose interface to engine
static CPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction );

CPrediction *prediction = &g_Prediction;

