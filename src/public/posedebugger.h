//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef POSEDEBUGGER_H
#define POSEDEBUGGER_H
#ifdef _WIN32
#pragma once
#endif

class IClientNetworkable;

abstract_class IPoseDebugger
{
public:
	virtual void StartBlending( IClientNetworkable *pEntity, const CStudioHdr *pStudioHdr ) = 0;
	
	virtual void AccumulatePose(
		const CStudioHdr *pStudioHdr,
		CIKContext *pIKContext,
		Vector pos[], 
		Quaternion q[], 
		int sequence, 
		float cycle,
		const float poseParameter[],
		int boneMask,
		float flWeight,
		float flTime
		) = 0;
};

extern IPoseDebugger *g_pPoseDebugger;

#endif // #ifndef POSEDEBUGGER_H
