#include "cbase.h"
#include "prediction.h"
#include "igamemovement.h"
#include "c_mom_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

// Expose interface to engine
static CPrediction g_Prediction;

EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CPrediction, IPrediction, VCLIENT_PREDICTION_INTERFACE_VERSION, g_Prediction);

CPrediction *prediction = &g_Prediction;