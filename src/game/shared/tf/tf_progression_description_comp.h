//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//			
//=============================================================================

#ifndef TF_PROGRESSION_DESCRIPTION_COMP_H
#define TF_PROGRESSION_DESCRIPTION_COMP_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_progression_description.h"

const uint32_t kGlicko1MaxRatingValue     = 4000;
const uint32_t kGlickoRanks = 13;

class CDrilloProgressionDesc : public IProgressionDesc
{
public:
	CDrilloProgressionDesc();

	const LevelInfo_t& GetLevelForRating( uint32 nExperience ) const OVERRIDE;

#ifdef CLIENT_DLL
	virtual void GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const OVERRIDE;
	virtual void SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const OVERRIDE;
	virtual const char* GetRankUnitsLocToken() const OVERRIDE { return "#TF_Units_MMR"; };
#endif // CLIENT_DLL

};

class CGlickoProgressionDesc : public IProgressionDesc
{
public:
	CGlickoProgressionDesc();

#ifdef CLIENT_DLL
	virtual void GetLocalizedLevelTitle( const LevelInfo_t& level, wchar_t* wszOutString, int wszOutStringSize ) const OVERRIDE;
	virtual void SetupBadgePanel( CBaseModelPanel *pModelPanel, const LevelInfo_t& level, const CSteamID& steamID, bool bInPlacement ) const OVERRIDE;
	virtual const char* GetRankUnitsLocToken() const OVERRIDE { return "#TF_Units_MMR"; };
#endif // CLIENT_DLL

};

#endif //TF_PROGRESSION_DESCRIPTION_COMP_H
