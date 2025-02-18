//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_progression_description.h"

#ifdef CLIENT_DLL
	#include "basemodel_panel.h"
#endif

// Casual XP constants
const float flAverageXPPerGame = 500.f;
const float flAverageMinutesPerGame = 30;

// The target XP per minute
const float flTargetXPPM = (float)flAverageXPPerGame / (float)flAverageMinutesPerGame;

// The target breakdown at the end of a match
const float flScoreXPScale				= 0.4485f;
const float flObjectiveXPScale			= 0.15f;
const float flMatchCompletionXPScale	= 0.3f;

// These come from the first 4 weeks of MyM match data
const float flAvgPPMPM = 27.f;	// Points per minute per match
const float flAvgPPMPP = 1.15f;	// Points per minute per player (above / 24)

const XPSourceDef_t g_XPSourceDefs[] = { { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_Score", flTargetXPPM * flScoreXPScale / flAvgPPMPP					/* 6.5  */ }	// SOURCE_SCORE = 0;
									   , { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_ObjectiveBonus", flTargetXPPM * flObjectiveXPScale / flAvgPPMPM		/* 0.0926  */ }	// SOURCE_OBJECTIVE_BONUS = 1;
									   , { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_CompletedMatch", flTargetXPPM * flMatchCompletionXPScale / flAvgPPMPM /* 0.185 */ }	// SOURCE_COMPLETED_MATCH = 2;
									   , { "MVM.PlayerDied",		"TF_XPSource_NoValueFormat",  "#TF_XPSource_Comp_Abandon", 1.f }																		// SOURCE_COMPETITIVE_ABANDON = 3;
									   , { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_Comp_Win", 1.f }																		// SOURCE_COMPETITIVE_WIN = 4;
									   , { NULL,					"TF_XPSource_NegativeFormat", "#TF_XPSource_Comp_Loss", 1.f }																						// SOURCE_COMPETITIVE_LOSS = 5;
									   , { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_Autobalance_Bonus", 1.f }																// SOURCE_AUTOBALANCE_BONUS = 6;
									   , { "MatchMaking.XPChime",	"TF_XPSource_PositiveFormat", "#TF_XPSource_Prestige_Bonus", 1.f } };																// SOURCE_PRESTIGE_BONUS = 7;

IProgressionDesc::IProgressionDesc( const char* pszBadgeName
								  , const char* pszProgressionResFile 
								  , const char* pszLevelToken )
	: m_strBadgeName( pszBadgeName )
	, m_pszProgressionResFile( pszProgressionResFile )
	, m_pszLevelToken( pszLevelToken )
{}


#ifdef CLIENT_DLL
void IProgressionDesc::EnsureBadgePanelModel( CBaseModelPanel *pModelPanel ) const
{
	studiohdr_t* pHDR = pModelPanel->GetStudioHdr();
	if ( !pHDR || !(CUtlString( pHDR->name ).UnqualifiedFilename() == m_strBadgeName.UnqualifiedFilename()) )
	{
		pModelPanel->SetMDL( m_strBadgeName );
	}
}
#endif // CLIENT_DLL

const LevelInfo_t& IProgressionDesc::GetLevelByNumber( uint32 nNumber ) const
{
	int nIndex = nNumber;
	nIndex = Clamp( nIndex - 1, 0, m_vecLevels.Count() - 1 );
	Assert( nIndex >= 0 && nIndex < m_vecLevels.Count() );
	return m_vecLevels[ nIndex ];
};

const LevelInfo_t& IProgressionDesc::GetLevelForRating( uint32 nExperience ) const
{
	uint32 nNumLevels = (uint32)m_vecLevels.Count();
	// Walk the levels to find where the passed in experience value falls
	for( uint32 i=0; i<nNumLevels; ++i )
	{
		if ( nExperience >= m_vecLevels[ i ].m_nStartXP && ( nExperience < m_vecLevels[ i ].m_nEndXP || (i + 1) == nNumLevels ) )
		{
			return m_vecLevels[ i ];
		}
	}

	Assert( false );
	return m_vecLevels[ 0 ];
}
