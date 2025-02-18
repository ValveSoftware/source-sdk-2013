//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_HUD_TEAMGOAL_TOURNAMENT_H
#define TF_HUD_TEAMGOAL_TOURNAMENT_H

#include "hud.h"
#include "hudelement.h"
#include "tf_controls.h"
#include "hud_basechat.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudTeamGoalTournament : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTeamGoalTournament, EditablePanel );

public:
	CHudTeamGoalTournament( const char *pElementName );

	virtual void	LevelInit( void ) OVERRIDE;
	virtual void	ApplySchemeSettings( IScheme *scheme ) OVERRIDE;
	virtual bool	ShouldDraw( void ) OVERRIDE;
	virtual void	SetVisible( bool bState ) OVERRIDE;
	int				HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	void			SetupStopWatchLabel( void );
	void			PrepareStopWatchString( wchar_t *pszString, CExRichText	*pText );
	void			FireGameEvent( IGameEvent * event );

private:
	float			m_flShowAt;

	EditablePanel	*m_pStopWatchGoal;
	CExRichText		*m_pStopWatchGoalText;
	CExRichText		*m_pStopWatchGoalText2;

	Panel			*m_pStopWatchGoalBGLarge;
	Panel			*m_pStopWatchGoalBGSmall;
	Panel			*m_pStopWatchGoalDivider;
	Panel			*m_pStopWatchGoalArrow;

	CUtlVector< TextRange > m_textRanges;
	wchar_t					*m_text;

	Color			m_cRegularColor;
	Color			m_cHighlightColor;
};

#endif // TF_HUD_TEAMGOAL_TOURNAMENT_H
