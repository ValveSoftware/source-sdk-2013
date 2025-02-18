//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PASSTIME_H
#define TF_HUD_PASSTIME_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "GameEventListener.h"

namespace vgui { class ContinuousProgressBar; }
class CTFHudPasstimePlayerOffscreenArrow;

//-----------------------------------------------------------------------------
class CTFHudPasstimePanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudPasstimePanel, vgui::EditablePanel );
	CTFHudPasstimePanel( vgui::Panel *pParent, const char *name );

	virtual bool IsVisible() OVERRIDE;
};

//-----------------------------------------------------------------------------
class CTFHudTeamScore : public CTFHudPasstimePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudTeamScore, CTFHudPasstimePanel );
	CTFHudTeamScore( vgui::Panel *pParent );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnTick() OVERRIDE;

private:
	int GetTeamScore( int iTeam );

	vgui::EditablePanel *m_pPlayingToCluster;
};

//-----------------------------------------------------------------------------
class CTFHudPasstimePassNotify : public CTFHudPasstimePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudPasstimePassNotify, CTFHudPasstimePanel );
	CTFHudPasstimePassNotify( vgui::Panel *pParent );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
private:
	vgui::EditablePanel *m_pTextBox;
	vgui::Label *m_pTextInPassRange;
	vgui::Label *m_pTextLockedOn;
	vgui::Label *m_pTextPassIncoming;
	vgui::Label *m_pTextPlayerName;
	vgui::ImagePanel *m_pSpeechIndicator;
	vgui::ImagePanel *m_pPassLockIndicator;
	vgui::IBorder *m_pTextBoxBorderNormal;
	vgui::IBorder *m_pTextBoxBorderIncomingRed;
	vgui::IBorder *m_pTextBoxBorderIncomingBlu;
};

//-----------------------------------------------------------------------------
class CTFHudPasstimeEventText
{
public:
	CTFHudPasstimeEventText();
	~CTFHudPasstimeEventText();
	void Tick();
	void Clear();
	void SetControls( vgui::Label *pTitleLabel, vgui::Label *pDetailLabel, vgui::Label *pBonusLabel );
	void EnqueueSteal( C_TFPlayer *pAttacker, C_TFPlayer *pVictim );
	void EnqueuePass( C_TFPlayer *pThrower, C_TFPlayer *pCatcher );
	void EnqueueInterception( C_TFPlayer *pThrower, C_TFPlayer *pCatcher );
	void EnqueueScore( C_TFPlayer *pThrower, C_TFPlayer *pAssister );
	void EnqueueGeneric( const char *pTitle, const char *pDetail, const char *pBonus );

private:
	// this would make more sense as a vgui animation but I need more control than that can reliably give me
	enum class State { Idle, In, Show, Out, Pause };

	struct QueueElement 
	{
		static const size_t STRLEN_MAX = 128;
		QueueElement();
		wchar_t title[STRLEN_MAX];
		wchar_t detail[STRLEN_MAX];
		wchar_t bonus[STRLEN_MAX];
	};

	void SetPlayerName( C_TFPlayer *pPlayer, const char *pKey );
	void SetTeam( C_TFPlayer *pPlayer );
	void EnterState( State state, float duration );
	void SetAlpha( int ia );
	static void SetLabelText( vgui::Label *pLabel, const wchar_t *pText );
	void Enqueue( C_TFPlayer *pSource, C_TFPlayer *pSubject, const char *pTitle, const char *pDetail, const char *pBonus );

	template< int TArraySize >
	void ConstructNewString( const char *pLocTag, wchar_t (&out)[TArraySize] );

	bool m_bValid;
	CountdownTimer m_displayTimer;
	KeyValuesAD m_localizeKeys;
	vgui::Label *m_pTitleLabel;
	vgui::Label *m_pDetailLabel;
	vgui::Label *m_pBonusLabel;
	State m_state;
	typedef CUtlQueue<QueueElement> Queue;
	Queue m_queue;
	wchar_t m_pwcsBuf[32];
};

//-----------------------------------------------------------------------------
class CTFArrowPanel;
class C_TFPlayer;
class C_PasstimeBall;
class CTFHudPasstimeOffscreenArrow;
class C_FuncPasstimeGoal;
class CTFHudPasstimeBallStatus : public CTFHudPasstimePanel, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudPasstimeBallStatus, CTFHudPasstimePanel );
	CTFHudPasstimeBallStatus( vgui::Panel *pParent );
	~CTFHudPasstimeBallStatus();

	void Reset();
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
	virtual void OnTick() OVERRIDE;

private:
	static const int NumGoalIcons = 3;
	bool m_bInitialized;
	bool m_bReset;
	bool m_bGoalsFound;
	int m_iXBlueProgress;
	int m_iXRedProgress;
	int m_iYBlueProgress;
	int m_iYRedProgress;
	vgui::ImagePanel *m_pGoalIconsBlue[NumGoalIcons];
	vgui::ImagePanel *m_pGoalIconsRed[NumGoalIcons];
	CHandle<C_FuncPasstimeGoal> m_hGoalsBlue[NumGoalIcons];
	CHandle<C_FuncPasstimeGoal> m_hGoalsRed[NumGoalIcons];
	vgui::ImagePanel *m_pPlayerIcons[MAX_PLAYERS_ARRAY_SAFE];
	vgui::ImagePanel *m_pProgressBall;
	vgui::Label *m_pProgressBallCarrierName;
	vgui::Panel *m_pProgressLevelBar;
	vgui::ImagePanel *m_pSelfPlayerIcon;
	CTFHudPasstimeEventText *m_pEventText;

	vgui::EditablePanel *m_pPowerCluster;
	vgui::Panel *m_pBallPowerMeterFillContainer;
	vgui::ImagePanel *m_pBallPowerMeterFill;
	vgui::Panel *m_pBallPowerMeterFrame;
	vgui::Panel *m_pBallPowerMeterFinalSection;
	int m_iBallPowerMeterFillWidth;
	int m_iPrevBallPower;

	void OnBallFreeSelf( C_TFPlayer *pOwner, C_TFPlayer *pAttacker );
	void OnBallFreeOther( C_TFPlayer *pOwner, C_TFPlayer *pAttacker );
	void OnBallGetOther( int iPlayer );
	void OnBallGetSelf( int iPlayer );
	void OnBallScore();
	bool TryForceBallFree();
	bool TryForceBallGet();
	void OnBallGet( int getterIndex );
	void UpdateGoalIcon( vgui::ImagePanel *pIcon, C_FuncPasstimeGoal *pGoal );
	bool BShouldDraw() const;
	void OnTickVisible( C_TFPlayer *pLocalPlayer, C_PasstimeBall *pBall);
	void OnTickHidden();
	void HideGoalIcons();
};

//-----------------------------------------------------------------------------
class CTFHudPasstime : public CTFHudPasstimePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudPasstime, CTFHudPasstimePanel );
	CTFHudPasstime( vgui::Panel *pParent );
	~CTFHudPasstime();
	void Reset();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnTick() OVERRIDE;

private:
	CTFHudPasstimeBallStatus *m_pBallStatus;
	CTFHudTeamScore *m_pTeamScore;
	CTFHudPasstimeOffscreenArrow *m_pBallOffscreenArrow;
	CTFHudPasstimePassNotify *m_pPassNotify;
	CTFHudPasstimePlayerOffscreenArrow *m_pPlayerArrows[MAX_PLAYERS_ARRAY_SAFE];
};

#endif // TF_HUD_PASSTIME_H  
