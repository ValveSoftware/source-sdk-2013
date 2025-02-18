//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//			
//=============================================================================

#ifndef TF_PROGRESSION_DESCRIPTION_CASUAL_H
#define TF_PROGRESSION_DESCRIPTION_CASUAL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_progression_description.h"

class CCasualProgressionDesc : public IProgressionDesc
{
public:

	CCasualProgressionDesc();

#ifdef CLIENT_DLL
	virtual void SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const OVERRIDE;
	virtual void GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const OVERRIDE;
	virtual const char* GetRankUnitsLocToken() const OVERRIDE { return "#TF_Units_XP"; };
#endif // CLIENT_DLL


private:

	uint32 GetLevelsPerPrestige() const { return m_nSteps * m_nLevelsPerStep; }

	const uint32 m_nLevelsPerStep = 25;
	const uint32 m_nSteps = 6;
	const uint32 m_nPrestigeLevels = 8; // How many times you can roll over 150
	const uint32 m_nAverageXPPerGame = 500;
};

#endif //TF_PROGRESSION_DESCRIPTION_CASUAL_H
