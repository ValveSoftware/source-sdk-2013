//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds the CTFDualSummary
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFDUELSUMMARY_H
#define TFDUELSUMMARY_H
#ifdef _WIN32
#pragma once
#endif

#include "gcsdk/protobufsharedobject.h"
#include "tf_gcmessages.h"


// do not re-order, stored in DB
enum eDuelStatus
{
	kDuelStatus_Loss,
	kDuelStatus_Tie,
	kDuelStatus_Win,
};

// do not re-order, stored in DB
enum eDuelEndReason
{
	kDuelEndReason_DuelOver,
	kDuelEndReason_PlayerDisconnected,
	kDuelEndReason_PlayerSwappedTeams,
	kDuelEndReason_LevelShutdown,
	kDuelEndReason_ScoreTiedAtZero,
	kDuelEndReason_PlayerKicked,
	kDuelEndReason_PlayerForceSwappedTeams,
	kDuelEndReason_ScoreTied,
	kDuelEndReason_Cancelled
};

const char *PchNameFromeDuelEndReason( eDuelEndReason eReason );

const uint32 kWinsPerLevel = 10;

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
class CTFDuelSummary : public GCSDK::CProtoBufSharedObject< CSOTFDuelSummary, k_EEconTypeDuelSummary >
{

public:
};

#endif //TFDUELSUMMARY_H
