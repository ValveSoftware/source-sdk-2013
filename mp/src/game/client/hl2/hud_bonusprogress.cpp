//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// BonusProgress.cpp
//
// implementation of CHudBonusProgress class
//
#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "view.h"

#include "iclientmode.h"

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>

#include <vgui/ILocalize.h>

using namespace vgui;

#include "hudelement.h"
#include "hud_numericdisplay.h"

#include "convar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define INIT_BONUS_PROGRESS -1


//-----------------------------------------------------------------------------
// Purpose: BonusProgress panel
//-----------------------------------------------------------------------------
class CHudBonusProgress : public CHudElement, public CHudNumericDisplay
{
	DECLARE_CLASS_SIMPLE( CHudBonusProgress, CHudNumericDisplay );

public:
	CHudBonusProgress( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual void OnThink();

private:
	void SetChallengeLabel( void );

private:
	// old variables
	int		m_iBonusProgress;

	int		m_iLastChallenge;
};	

DECLARE_HUDELEMENT( CHudBonusProgress );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudBonusProgress::CHudBonusProgress( const char *pElementName ) : CHudElement( pElementName ), CHudNumericDisplay(NULL, "HudBonusProgress")
{
	SetHiddenBits( HIDEHUD_BONUS_PROGRESS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBonusProgress::Init()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBonusProgress::Reset()
{
	m_iBonusProgress = INIT_BONUS_PROGRESS;

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( local )
		m_iLastChallenge = local->GetBonusChallenge();

	SetChallengeLabel();

	SetDisplayValue(m_iBonusProgress);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBonusProgress::VidInit()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBonusProgress::OnThink()
{
	C_GameRules *pGameRules = GameRules();

	if ( !pGameRules )
	{
		// Not ready to init!
		return;
	}

	int newBonusProgress = 0;
	int iBonusChallenge = 0;

	C_BasePlayer *local = C_BasePlayer::GetLocalPlayer();
	if ( !local )
	{
		// Not ready to init!
		return;
	}

	// Never below zero
	newBonusProgress = MAX( local->GetBonusProgress(), 0 );
	iBonusChallenge = local->GetBonusChallenge();

	// Only update the fade if we've changed bonusProgress
	if ( newBonusProgress == m_iBonusProgress && m_iLastChallenge == iBonusChallenge )
	{
		return;
	}

	m_iBonusProgress = newBonusProgress;

	if ( m_iLastChallenge != iBonusChallenge )
	{
		m_iLastChallenge = iBonusChallenge;
		SetChallengeLabel();
	}

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("BonusProgressFlash");

	if ( pGameRules->IsBonusChallengeTimeBased() )
	{
		SetIsTime( true );
		SetIndent( false );
	}
	else
	{
		SetIsTime( false );
		SetIndent( true );
	}

	SetDisplayValue(m_iBonusProgress);
}

void CHudBonusProgress::SetChallengeLabel( void )
{
	// Blank for no challenge
	if ( m_iLastChallenge == 0 )
	{
		SetLabelText(L"");
		return;
	}

	char szBonusTextName[] = "#Valve_Hud_BONUS_PROGRESS00";

	int iStringLength = Q_strlen( szBonusTextName );

	szBonusTextName[ iStringLength - 2 ] = ( m_iLastChallenge / 10 ) + '0';
	szBonusTextName[ iStringLength - 1 ] = ( m_iLastChallenge % 10 ) + '0';

	wchar_t *tempString = g_pVGuiLocalize->Find(szBonusTextName);

	if (tempString)
	{
		SetLabelText(tempString);
		return;
	}

	// Couldn't find a special string for this challenge
	tempString = g_pVGuiLocalize->Find("#Valve_Hud_BONUS_PROGRESS");
	if (tempString)
	{
		SetLabelText(tempString);
		return;
	}

	// Couldn't find any localizable string
	SetLabelText(L"BONUS");
}