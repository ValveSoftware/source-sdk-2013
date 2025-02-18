//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Holds XP source data
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SURVEY_QUESTIONS_H
#define TF_SURVEY_QUESTIONS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_gcmessages.h"
#ifdef CLIENT_DLL
	#include "vgui_controls/EditablePanel.h"
#endif


#ifdef CLIENT_DLL
	using namespace vgui;
#endif

//-----------------------------------------------------------------------------
// Purpose: The definition of a survey question.
//
//	m_eType : The type of the survey question
//	m_pszSurveyQuestionName: Name of the survey type.  Mostly for debugging
//	m_flWeight: The weight used when randomly choosing which survey question 
//				to ask a user
//	m_pFnSurveyValidForPlayer: Survey specific function to determine if player
//							   meets criteria to receive survey
//	m_bIsActive: Is the survey currently active (asked of players) 
//-----------------------------------------------------------------------------
struct SurveyQuestion_t
{
	SurveyQuestionType	m_eType;
	const char*			m_pszSurveyQuestionName;
	float				m_flWeight;
	bool ( *m_pFnSurveyValidForPlayer ) ( const CMsgGC_Match_Result& msgMatchResult , uint32 nPlayerIndex );
	bool				m_bIsActive;
};

#define UNASWERED_SURVEY_QUESTION ( (int16) -1 )
#define SEEN_BUT_UNANSWERED_SURVEY_QUESTION ( (int16) -2 )
#define SEEN_AND_DISMISSED_SURVEY_QUESTION ( (int16) -3 )
extern const SurveyQuestion_t g_SurveyQuestions[ SurveyQuestionType_ARRAYSIZE ];

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: Use CreateSurveyQuestionPanel to create the panel you want
//-----------------------------------------------------------------------------
class CSurveyQuestionPanel* CreateSurveyQuestionPanel( Panel* pParent, const CMsgGCSurveyRequest& msgSurveyQuestion );

//-----------------------------------------------------------------------------
// Purpose: Base, abstract survey panel to handle common functionality
//-----------------------------------------------------------------------------
class CSurveyQuestionPanel : public EditablePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CSurveyQuestionPanel, EditablePanel );
	CSurveyQuestionPanel( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion );
	~CSurveyQuestionPanel();

	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;

private:
	virtual void Submit() = 0;
	virtual const char* GetResFile() const = 0;
	
	bool m_bResponded;
	CMsgGCSurveyRequest m_msgRequest;
};

//-----------------------------------------------------------------------------
// Purpose: Base class for multiple choice surveys
//-----------------------------------------------------------------------------
class CMultipleChoiceSurveyQuestionPanel : public CSurveyQuestionPanel
{
public:
	DECLARE_CLASS_SIMPLE( CMultipleChoiceSurveyQuestionPanel, CSurveyQuestionPanel );
	CMultipleChoiceSurveyQuestionPanel( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion, uint16 nSurveyResponses );

private:

	virtual void Think() OVERRIDE;
	virtual void Submit() OVERRIDE;

	uint16 m_nSurveyResponses;
};

//-----------------------------------------------------------------------------
// Purpose: Match quality survey.  Users can rate the quality of their match
//			with a score of 0 - 4.  Score is marked through radio buttons.
//-----------------------------------------------------------------------------
class CMatchQualitySurvey : public CMultipleChoiceSurveyQuestionPanel
{
public:
	CMatchQualitySurvey( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion ) : CMultipleChoiceSurveyQuestionPanel( pParent, msgSurveyQuestion, 5 ) {}
	virtual const char* GetResFile() const OVERRIDE
	{
		return  "resource/ui/SurveyPanel_MatchQuality.res";
	}
};

//-----------------------------------------------------------------------------
// Purpose: Map quality survey.  Users can rate the quality of the map played
//			with a score of 0 - 4.  Score is marked through radio buttons.
//-----------------------------------------------------------------------------
class CMapQualitySurvey : public CMultipleChoiceSurveyQuestionPanel
{
public:
	CMapQualitySurvey( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion ) : CMultipleChoiceSurveyQuestionPanel( pParent, msgSurveyQuestion, 5 ) {}

	virtual const char* GetResFile() const OVERRIDE
	{
		return  "resource/ui/SurveyPanel_MapQuality.res";
	}

	virtual void PerformLayout() OVERRIDE;
};

//-----------------------------------------------------------------------------
// Purpose: Survey casual mode players to see why they're not playing competitive
//-----------------------------------------------------------------------------
class CCompInquirySurvey : public CMultipleChoiceSurveyQuestionPanel
{
public:
	CCompInquirySurvey( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion ) : CMultipleChoiceSurveyQuestionPanel( pParent, msgSurveyQuestion, 6 ) {}

	virtual const char* GetResFile() const OVERRIDE
	{
		return  "resource/ui/SurveyPanel_CompInquiry.res";
	}
};

//-----------------------------------------------------------------------------
// Purpose: Survey competitive mode players to see why they're not playing casual
//-----------------------------------------------------------------------------
class CCasualInquirySurvey : public CMultipleChoiceSurveyQuestionPanel
{
public:
	CCasualInquirySurvey( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion ) : CMultipleChoiceSurveyQuestionPanel( pParent, msgSurveyQuestion, 6 ) {}

	virtual const char* GetResFile() const OVERRIDE
	{
		return  "resource/ui/SurveyPanel_CasualInquiry.res";
	}
};

//-----------------------------------------------------------------------------
// Purpose: Survey players to see how they feel about random crits
//-----------------------------------------------------------------------------
class CRandomCritSurvey : public CMultipleChoiceSurveyQuestionPanel
{
public:
	CRandomCritSurvey( Panel* pParent, CMsgGCSurveyRequest msgSurveyQuestion ) : CMultipleChoiceSurveyQuestionPanel( pParent, msgSurveyQuestion, 6 ) {}

	virtual const char* GetResFile() const OVERRIDE
	{
		return  "resource/ui/SurveyPanel_RandomCrit.res";
	}
};

#endif

#endif // TF_SURVEY_QUESTIONS_H
