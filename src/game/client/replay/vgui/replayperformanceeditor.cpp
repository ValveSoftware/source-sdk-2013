//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"

#if defined( REPLAY_ENABLED )

#include "replayperformanceeditor.h"
#include "replay/replay.h"
#include "replay/ireplayperformanceeditor.h"
#include "replay/ireplayperformancecontroller.h"
#include "replay/performance.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Slider.h"
#include "vgui_controls/Menu.h"
#include "vgui/ILocalize.h"
#include "vgui/IImage.h"
#include "c_team.h"
#include "vgui_avatarimage.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "replay/replaycamera.h"
#include "replay/ireplaymanager.h"
#include "replay/iclientreplaycontext.h"
#include "confirm_dialog.h"
#include "replayperformancesavedlg.h"
#include "replay/irecordingsessionmanager.h"
#include "achievementmgr.h"
#include "c_playerresource.h"
#include "replay/gamedefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

extern CAchievementMgr g_AchievementMgrTF;

//-----------------------------------------------------------------------------

using namespace vgui;

//-----------------------------------------------------------------------------

extern IReplayPerformanceController *g_pReplayPerformanceController;

//-----------------------------------------------------------------------------

// Hack-y global bool to communicate when we are rewinding for map load screens.
// Order of operations issues preclude the use of engine->IsPlayingDemo().
bool g_bIsReplayRewinding = false;

//-----------------------------------------------------------------------------

// TODO: Make these archive?  Right now, the tips are reset every time the game starts
ConVar replay_perftip_count_enter( "replay_perftip_count_enter", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "", true, 0, false, 0 );
ConVar replay_perftip_count_exit( "replay_perftip_count_exit", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "", true, 0, false, 0 );
ConVar replay_perftip_count_freecam_enter( "replay_perftip_count_freecam_enter", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "", true, 0, false, 0 );
ConVar replay_perftip_count_freecam_exit( "replay_perftip_count_freecam_exit", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "", true, 0, false, 0 );
ConVar replay_perftip_count_freecam_exit2( "replay_perftip_count_freecam_exit2", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "", true, 0, false, 0 );

ConVar replay_editor_fov_mousewheel_multiplier( "replay_editor_fov_mousewheel_multiplier", "5", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "The multiplier on mousewheel input for adjusting camera FOV in the replay editor." );
ConVar replay_editor_fov_mousewheel_invert( "replay_editor_fov_mousewheel_invert", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_DONTRECORD, "Invert FOV zoom/unzoom on mousewheel in the replay editor." );

ConVar replay_replayeditor_rewindmsgcounter( "replay_replayeditor_rewindmsgcounter", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_HIDDEN, "" );

//-----------------------------------------------------------------------------

#define MAX_TIP_DISPLAYS		1

//-----------------------------------------------------------------------------

#define TIMESCALE_MIN			0.01f
#define TIMESCALE_MAX			3.0f

//-----------------------------------------------------------------------------

#define SLIDER_RANGE_MAX		10000.0f

//-----------------------------------------------------------------------------

#define REPLAY_SOUND_DIALOG_POPUP		"replay\\replaydialog_warn.wav"

//-----------------------------------------------------------------------------

static const char *gs_pCamNames[ NCAMS ] =
{
	"free",
	"third",
	"first",
	"timescale",
};

static const char *gs_pBaseComponentNames[ NCAMS ] =
{
	"replay/replay_camera_%s%s",
	"replay/replay_camera_%s%s",
	"replay/replay_camera_%s%s",
	"replay/replay_%s%s",
};

//-----------------------------------------------------------------------------

void PlayDemo()
{
	engine->ClientCmd_Unrestricted( "demo_resume" );
}

void PauseDemo()
{
	engine->ClientCmd_Unrestricted( "demo_pause" );
}

//-----------------------------------------------------------------------------

inline float SCurve( float t )
{
	t = clamp( t, 0.0f, 1.0f );
	return t * t * (3 - 2*t);
}

inline float CubicEaseIn( float t )
{
	t = clamp( t, 0.0f, 1.0f );
	return t * t * t;
}

inline float LerpScale( float flIn, float flInMin, float flInMax, float flOutMin, float flOutMax )
{
	float flDenom = flInMax - flInMin;
	if ( flDenom == 0.0f )
		return 0.0f;

	float t = clamp( ( flIn - flInMin ) / flDenom, 0.0f, 1.0f );
	return Lerp( t, flOutMin, flOutMax );
}

//-----------------------------------------------------------------------------

void HighlightTipWords( Label *pLabel )
{
	// Setup coloring - get # of words that should be highlighted
	wchar_t *pwNumWords = g_pVGuiLocalize->Find( "#Replay_PerfTip_Highlight_NumWords" );
	if ( !pwNumWords )
		return;

	// Get the current label text
	wchar_t wszLabelText[512];
	pLabel->GetText( wszLabelText, sizeof( wszLabelText ) );

	pLabel->GetTextImage()->ClearColorChangeStream();
	pLabel->GetTextImage()->AddColorChange( pLabel->GetFgColor(), 0 );

	int nNumWords = _wtoi( pwNumWords );
	for ( int i = 0; i < nNumWords; ++i )
	{
		char szWordFindStr[64];
		V_snprintf( szWordFindStr, sizeof( szWordFindStr ), "#Replay_PerfTip_Highlight_Word%i", i );
		wchar_t *pwWord = g_pVGuiLocalize->Find( szWordFindStr );
		if ( !pwWord )
			continue;

		const int nWordLen = wcslen( pwWord );

		// Find any instance of the word in the label text and highlight it in red
		const wchar_t *p = wszLabelText;
		do 
		{
			const wchar_t *pInst = wcsstr( p, pwWord );
			if ( !pInst )
				break;

			// Highlight the text
			int nStartPos = pInst - wszLabelText;
			int nEndPos = nStartPos + nWordLen;

			// If start pos is non-zero, clear color changes
			bool bChangeColor = true;
			if ( nStartPos == 0 )
			{
				pLabel->GetTextImage()->ClearColorChangeStream();
			}
			else if ( iswalpha( wszLabelText[ nStartPos - 1 ] ) )
			{
				// If this is not the beginning of the string, check the previous character.  If it's
				// not whitespace, etc, we found an instance of a keyword within another word.  Skip.
				bChangeColor = false;
			}

			if ( bChangeColor )
			{
				pLabel->GetTextImage()->AddColorChange( Color(200,80,60,255), nStartPos );
				pLabel->GetTextImage()->AddColorChange( pLabel->GetFgColor(), nEndPos );
			}

			p = pInst + nWordLen;
		} while ( 1 );
	}
}

//-----------------------------------------------------------------------------

class CSavingDialog : public CGenericWaitingDialog
{
	DECLARE_CLASS_SIMPLE( CSavingDialog, CGenericWaitingDialog );
public:
	CSavingDialog( CReplayPerformanceEditorPanel *pEditorPanel ) 
	:	CGenericWaitingDialog( pEditorPanel )
	{
		m_pEditorPanel = pEditorPanel;
	}

	virtual void OnTick()
	{
		BaseClass::OnTick();

		if ( !g_pReplayPerformanceController )
			return;

		// Update async save
		if ( g_pReplayPerformanceController->IsSaving() )
		{
			g_pReplayPerformanceController->SaveThink();
		}
		else
		{
			if ( m_pEditorPanel.Get() )
			{
				m_pEditorPanel->OnSaveComplete();
			}

			Close();
		}
	}

private:
	CConfirmDialog *m_pLoginDialog;
	vgui::DHANDLE< CReplayPerformanceEditorPanel > m_pEditorPanel;
};

//-----------------------------------------------------------------------------

class CReplayTipLabel : public Label
{
	DECLARE_CLASS_SIMPLE( CReplayTipLabel, Label );
public:
	CReplayTipLabel( Panel *pParent, const char *pName, const char *pText )
	:	BaseClass( pParent, pName, pText )
	{
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		HighlightTipWords( this );
	}
};

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CReplayTipLabel, Label );

//-----------------------------------------------------------------------------

class CPerformanceTip : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CPerformanceTip, EditablePanel );
public:
	static DHANDLE< CPerformanceTip > s_pTip;

	static CPerformanceTip *CreateInstance( const char *pText )
	{
		if ( s_pTip )
		{
			s_pTip->SetVisible( false );
			s_pTip->MarkForDeletion();
			s_pTip = NULL;
		}

		s_pTip = SETUP_PANEL( new CPerformanceTip( pText ) );

		return s_pTip;
	}

	CPerformanceTip( const char *pText )
	:	BaseClass( g_pClientMode->GetViewport(), "Tip" ),
		m_flBornTime( gpGlobals->realtime ),
		m_flAge( 0.0f ),
		m_flShowDuration( 15.0f )
	{
		m_pTextLabel = new CReplayTipLabel( this, "TextLabel", pText );
	}
	
	virtual void OnThink()
	{
		// Delete the panel if life exceeded
		const float flEndTime = m_flBornTime + m_flShowDuration;
		if ( gpGlobals->realtime >= flEndTime )
		{
			SetVisible( false );
			MarkForDeletion();
			s_pTip = NULL;
			return;
		}

		SetVisible( true );

		const float flFadeDuration = .4f;
		float flAlpha;

		// Fade out?
		if ( gpGlobals->realtime >= flEndTime - flFadeDuration )
		{
			flAlpha = LerpScale( gpGlobals->realtime, flEndTime - flFadeDuration, flEndTime, 1.0f, 0.0f );
		}

		// Fade in?
		else if ( gpGlobals->realtime <= m_flBornTime + flFadeDuration )
		{
			flAlpha = LerpScale( gpGlobals->realtime, m_flBornTime, m_flBornTime + flFadeDuration, 0.0f, 1.0f );
		}

		// Otherwise, we must be in between fade in/fade out
		else
		{
			flAlpha = 1.0f;
		}

		SetAlpha( 255 * SCurve( flAlpha ) );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/replayperformanceeditor/tip.res", "GAME" );

		// Center relative to parent
		const int nScreenW = ScreenWidth();
		const int nScreenH = ScreenHeight();
		int aContentSize[2];
		m_pTextLabel->GetContentSize( aContentSize[0], aContentSize[1] );
		const int nLabelHeight = aContentSize[1];
		SetBounds( 
			0,
			3 * nScreenH / 4 - nLabelHeight / 2,
			nScreenW,
			nLabelHeight + 2 * m_nTopBottomMargin
		);
		m_pTextLabel->SetBounds(
			m_nLeftRightMarginWidth,
			m_nTopBottomMargin,
			nScreenW - 2 * m_nLeftRightMarginWidth,
			nLabelHeight
		);
	}

	static void Cleanup()
	{
		if ( s_pTip )
		{
			s_pTip->MarkForDeletion();
			s_pTip = NULL;
		}
	}

	CPanelAnimationVarAliasType( int, m_nLeftRightMarginWidth, "left_right_margin", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nTopBottomMargin , "top_bottom_margin", "0", "proportional_ypos" );

	CReplayTipLabel	*m_pTextLabel;
	float			m_flBornTime;
	float			m_flAge;
	float			m_flShowDuration;
};

DHANDLE< CPerformanceTip > CPerformanceTip::s_pTip;

// Display the performance tip if we haven't already displayed it nMaxTimesToDisplay times or more
inline void DisplayPerformanceTip( const char *pText, ConVar* pCountCv = NULL, int nMaxTimesToDisplay = -1 )
{
	// Already displayed too many times?  Get out.
	if ( pCountCv && nMaxTimesToDisplay >= 0 )
	{
		int nCount = pCountCv->GetInt();
		if ( nCount >= nMaxTimesToDisplay )
			return;

		// Incremement count cvar
		pCountCv->SetValue( nCount + 1 );
	}

	// Display the tip
	CPerformanceTip::CreateInstance( pText );
}

//-----------------------------------------------------------------------------

inline float GetPlaybackTime()
{
	CReplay *pPlayingReplay = g_pReplayManager->GetPlayingReplay();
	return gpGlobals->curtime - TICKS_TO_TIME( pPlayingReplay->m_nSpawnTick );
}

//-----------------------------------------------------------------------------

class CPlayerCell : public CExImageButton
{
	DECLARE_CLASS_SIMPLE( CPlayerCell, CExImageButton );
public:
	CPlayerCell( Panel *pParent, const char *pName, int *pCurTargetPlayerIndex )
	:	CExImageButton( pParent, pName, "" ),
		m_iPlayerIndex( -1 ),
		m_pCurTargetPlayerIndex( pCurTargetPlayerIndex )
	{
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		GetImage()->SetImage( "" );
		SetFont( pScheme->GetFont( "ReplaySmall" ) );
		SetContentAlignment( Label::a_center );
	}

	MESSAGE_FUNC( DoClick, "PressButton" )
	{
		ReplayCamera()->SetPrimaryTarget( m_iPlayerIndex );
		*m_pCurTargetPlayerIndex = m_iPlayerIndex;

		float flCurTime = GetPlaybackTime();

		extern IReplayPerformanceController *g_pReplayPerformanceController;
		g_pReplayPerformanceController->AddEvent_Camera_ChangePlayer( flCurTime, m_iPlayerIndex );
	}

	int		m_iPlayerIndex;
	int		*m_pCurTargetPlayerIndex;	// Allow the button to write current target in outer class when pressed
};

//-----------------------------------------------------------------------------

/*
class CReplayEditorSlider : public Slider
{
	DECLARE_CLASS_SIMPLE( CReplayEditorSlider, Slider );
public:
	CReplayEditorSlider( Panel *pParent, const char *pName )
	:	Slider( pParent, pName )
	{
	}

	virtual void SetDefault( float flDefault ) { m_flDefault = flDefault; }

	ON_MESSAGE( Reset, OnReset )
	{
		SetValue( 
	}

private:
	float	m_flDefault;
};
*/

//-----------------------------------------------------------------------------

class CCameraOptionsPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCameraOptionsPanel, EditablePanel );
public:
	CCameraOptionsPanel( Panel *pParent, const char *pName, const char *pTitle )
	:	EditablePanel( pParent, pName ),
		m_bControlsAdded( false )
	{
		m_pTitleLabel = new CExLabel( this, "TitleLabel", pTitle );

		AddControlToLayout( m_pTitleLabel );
	}

	~CCameraOptionsPanel()
	{
		m_lstSliderInfos.PurgeAndDeleteElements();
	}

	void AddControlToLayout( Panel *pControl )
	{
		if ( pControl )
		{
			m_lstControls.AddToTail( pControl );
			pControl->SetMouseInputEnabled( true );
		}
	}

	// NOTE: Default value is assumed to be stored in flOut
	void AddSliderToLayout( int nId, Slider *pSlider, const char *pLabelText,
							float flMinValue, float flMaxValue, float &flOut )
	{
		SliderInfo_t *pNewSliderInfo = new SliderInfo_t;

		pNewSliderInfo->m_nId = nId;
		pNewSliderInfo->m_pSlider = pSlider;
		pNewSliderInfo->m_flRange[ 0 ] = flMinValue;
		pNewSliderInfo->m_flRange[ 1 ] = flMaxValue;
		pNewSliderInfo->m_flDefault = flOut;
		pNewSliderInfo->m_pValueOut = &flOut;

		m_lstSliderInfos.AddToTail( pNewSliderInfo );

		AddControlToLayout( new EditablePanel( this, "Buffer" ) );
		AddControlToLayout( NewLabel( pLabelText ) );
		AddControlToLayout( NewSetDefaultButton( nId ) );
		AddControlToLayout( pSlider );

		pSlider->AddActionSignalTarget( this );
	}

	void ResetSlider( int nId )
	{
		const SliderInfo_t *pSliderInfo = FindSliderInfoFromId( nId );
		if ( !pSliderInfo )
			return;

		SetValue( pSliderInfo, pSliderInfo->m_flDefault );
	}

	void SetValue( int nId, float flValue )
	{
		const SliderInfo_t *pSliderInfo = FindSliderInfoFromId( nId );
		if ( !pSliderInfo )
			return;

		SetValue( pSliderInfo, flValue );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		// Setup border
		SetBorder( pScheme->GetBorder( "ButtonBorder" ) );

		HFont hFont = pScheme->GetFont( "ReplayBrowserSmallest", true );
		m_pTitleLabel->SetFont( hFont );
		m_pTitleLabel->SizeToContents();
		m_pTitleLabel->SetTall( YRES( 20 ) );
		m_pTitleLabel->SetColorStr( "235 235 235 255" );

		if ( !m_bControlsAdded )
		{
            const char *pResFile = GetResFile();
            if ( pResFile )
            {
				LoadControlSettings( pResFile, "GAME" );
			}

			AddControls();
			m_bControlsAdded = true;
		}

		FOR_EACH_LL( m_lstSliderInfos, it )
		{
			SliderInfo_t *pInfo = m_lstSliderInfos[ it ];
			Slider *pSlider = pInfo->m_pSlider;
			pSlider->SetRange( 0, SLIDER_RANGE_MAX );
			pSlider->SetNumTicks( 10 );
			float flDenom = fabs( pInfo->m_flRange[1] - pInfo->m_flRange[0] );
			pSlider->SetValue( SLIDER_RANGE_MAX * fabs( pInfo->m_flDefault - pInfo->m_flRange[0] ) / flDenom );
		}
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int nWidth = XRES( 140 );
		int nMargins[2] = { (int)XRES( 5 ), (int)YRES( 5 ) };
		int nVBuf = YRES( 0 );
		int nLastY = -1;
		int nY = nMargins[1];
		Panel *pPrevPanel = NULL;
		int nLastCtrlHeight = 0;

		FOR_EACH_LL( m_lstControls, i )
		{
			Panel *pPanel = m_lstControls[ i ];
			if ( !pPanel->IsVisible() )
				continue;

			int aPos[2];
			pPanel->GetPos( aPos[0], aPos[1] );

			if ( pPrevPanel && aPos[1] >= 0 )
			{
				nY += pPrevPanel->GetTall() + nVBuf;
			}

			// Gross hack to see if the control is a default button
			if ( dynamic_cast< CExButton * >( pPanel ) )
			{
				pPanel->SetWide( XRES( 36 ) );
				pPanel->SetPos( pPrevPanel ? ( GetWide() - nMargins[0] - pPanel->GetWide() ) : 0, nLastY );
			}
			else
			{
				pPanel->SetWide( nWidth - 2 * nMargins[0] );
				pPanel->SetPos( nMargins[0], nY );
			}

			nLastY = nY;
			pPrevPanel = pPanel;
			nLastCtrlHeight = MAX( nLastCtrlHeight, pPanel->GetTall() );
		}

		SetSize( nWidth, nY + nLastCtrlHeight + 2 * YRES( 3 ) );
	}

	virtual void OnCommand( const char *pCommand )
	{
		if ( !V_strnicmp( pCommand, "reset_", 6 ) )
		{
			const int nSliderInfoId = atoi( pCommand + 6 );
			ResetSlider( nSliderInfoId );
		}
		else
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	Label *NewLabel( const char *pText )
	{
		Label *pLabel = new Label( this, "Label", pText );
		pLabel->SetTall( YRES( 9 ) );
		pLabel->SetPos( -1, 0 );	// Use default x and accumulated y

		// Set font
		IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		HFont hFont = pScheme->GetFont( "DefaultVerySmall", true );
		pLabel->SetFont( hFont );

		return pLabel;
	}

	CExButton *NewSetDefaultButton( int nSliderInfoId )
	{
		CExButton *pButton = new CExButton( this, "DefaultButton", "#Replay_SetDefaultSetting" );
		pButton->SetTall( YRES( 11 ) );
		pButton->SetPos( XRES( 30 ), -1 );	// Set y to -1 so it will stay on the same line
		pButton->SetContentAlignment( Label::a_center );
		CFmtStr fmtResetCommand( "reset_%i", nSliderInfoId );
		pButton->SetCommand( fmtResetCommand.Access() );
		pButton->AddActionSignalTarget( this );

		// Set font
		IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
		HFont hFont = pScheme->GetFont( "DefaultVerySmall", true );
		pButton->SetFont( hFont );

		return pButton;
	}

protected:
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", pParams )
	{
		Panel *pSlider = (Panel *)pParams->GetPtr( "panel" );
		float flPercent = pParams->GetInt( "position" ) / SLIDER_RANGE_MAX;

		FOR_EACH_LL( m_lstSliderInfos, it )
		{
			SliderInfo_t *pInfo = m_lstSliderInfos[ it ];
			if ( pSlider == pInfo->m_pSlider )
			{
				*pInfo->m_pValueOut = Lerp( flPercent, pInfo->m_flRange[0], pInfo->m_flRange[1] );
			}
		}
	}

	virtual const char *GetResFile() { return NULL; }

	virtual void AddControls()
	{
	}

	struct SliderInfo_t
	{
		Slider	*m_pSlider;
		float	m_flRange[2];
		float	m_flDefault;
		int		m_nId;
		float	*m_pValueOut;
	};

	const SliderInfo_t *FindSliderInfoFromId( int nId )
	{
		FOR_EACH_LL( m_lstSliderInfos, it )
		{
			SliderInfo_t *pInfo = m_lstSliderInfos[ it ];
			if ( pInfo->m_nId == nId )
				return pInfo;
		}

		AssertMsg( 0, "Should always find a slider here." );

		return NULL;
	}

	void SetValue( const SliderInfo_t *pSliderInfo, float flValue )
	{
		if ( !pSliderInfo )
		{
			AssertMsg( 0, "This should not happen." );
			return;
		}

		// Calculate the range
		const float flRange = fabs( pSliderInfo->m_flRange[1] - pSliderInfo->m_flRange[0] );
		AssertMsg( flRange > 0, "Bad slider range!" );

		// Calculate the percentile based on the specified value and the range.
		const float flPercent = fabs( flValue - pSliderInfo->m_flRange[0] ) / flRange;
		pSliderInfo->m_pSlider->SetValue( flPercent * SLIDER_RANGE_MAX, true );
	}

	CUtlLinkedList< Panel * >				m_lstControls;
	CUtlLinkedList< SliderInfo_t *, int >	m_lstSliderInfos;
	CExLabel								*m_pTitleLabel;
	bool									m_bControlsAdded;
};

//-----------------------------------------------------------------------------

class CTimeScaleOptionsPanel : public CCameraOptionsPanel
{
	DECLARE_CLASS_SIMPLE( CTimeScaleOptionsPanel, CCameraOptionsPanel );
public:
	CTimeScaleOptionsPanel( Panel *pParent, float *pTimeScaleProxy )
	:	BaseClass( pParent, "TimeScaleSettings", "#Replay_TimeScale" ),
		m_pTimeScaleSlider( NULL ),
		m_pTimeScaleProxy( pTimeScaleProxy )
	{
	}

	virtual const char *GetResFile()
	{
		return "resource/ui/replayperformanceeditor/settings_timescale.res";
	}

	virtual void AddControls()
	{
		m_pTimeScaleSlider = dynamic_cast< Slider * >( FindChildByName( "TimeScaleSlider" ) );

		AddSliderToLayout( SLIDER_TIMESCALE, m_pTimeScaleSlider, "#Replay_Scale", TIMESCALE_MIN, TIMESCALE_MAX, *m_pTimeScaleProxy );
	}

	enum FreeCamSliders_t
	{
		SLIDER_TIMESCALE,
	};

	Slider		*m_pTimeScaleSlider;
	float		*m_pTimeScaleProxy;
};

//-----------------------------------------------------------------------------
class CCameraOptionsPanel_Free : public CCameraOptionsPanel
{
	DECLARE_CLASS_SIMPLE( CCameraOptionsPanel_Free, CCameraOptionsPanel );
public:
	CCameraOptionsPanel_Free( Panel *pParent )
	:	BaseClass( pParent, "FreeCameraSettings", "#Replay_FreeCam" ),
		m_pAccelSlider( NULL ),
		m_pSpeedSlider( NULL ),
		m_pFovSlider( NULL ),
		m_pRotFilterSlider( NULL ),
		m_pShakeSpeedSlider( NULL ),
		m_pShakeAmountSlider( NULL )
	{
	}

	virtual const char *GetResFile()
	{
		return "resource/ui/replayperformanceeditor/camsettings_free.res";
	}

	virtual void AddControls()
	{
		m_pAccelSlider = dynamic_cast< Slider * >( FindChildByName( "AccelSlider" ) );
		m_pSpeedSlider = dynamic_cast< Slider * >( FindChildByName( "SpeedSlider" ) );
		m_pFovSlider = dynamic_cast< Slider * >( FindChildByName( "FovSlider" ) );
		m_pRotFilterSlider = dynamic_cast< Slider * >( FindChildByName( "RotFilterSlider" ) );
		m_pShakeSpeedSlider = dynamic_cast< Slider * >( FindChildByName( "ShakeSpeedSlider" ) );
		m_pShakeAmountSlider = dynamic_cast< Slider * >( FindChildByName( "ShakeAmountSlider" ) );
		m_pShakeDirSlider = dynamic_cast< Slider * >( FindChildByName( "ShakeDirSlider" ) );

		AddSliderToLayout( SLIDER_ACCEL, m_pAccelSlider, "#Replay_Accel", FREE_CAM_ACCEL_MIN, FREE_CAM_ACCEL_MAX, ReplayCamera()->m_flRoamingAccel );
		AddSliderToLayout( SLIDER_SPEED, m_pSpeedSlider, "#Replay_Speed", FREE_CAM_SPEED_MIN, FREE_CAM_SPEED_MAX, ReplayCamera()->m_flRoamingSpeed );
		AddSliderToLayout( SLIDER_FOV, m_pFovSlider, "#Replay_Fov", FREE_CAM_FOV_MIN, FREE_CAM_FOV_MAX, ReplayCamera()->m_flRoamingFov[1] );
		AddSliderToLayout( SLIDER_ROTFILTER, m_pRotFilterSlider, "#Replay_RotFilter", FREE_CAM_ROT_FILTER_MIN, FREE_CAM_ROT_FILTER_MAX, ReplayCamera()->m_flRoamingRotFilterFactor );
		AddSliderToLayout( SLIDER_SHAKE_SPEED, m_pShakeSpeedSlider, "#Replay_ShakeSpeed", FREE_CAM_SHAKE_SPEED_MIN, FREE_CAM_SHAKE_SPEED_MAX, ReplayCamera()->m_flRoamingShakeSpeed );
		AddSliderToLayout( SLIDER_SHAKE_AMOUNT, m_pShakeAmountSlider, "#Replay_ShakeAmount", FREE_CAM_SHAKE_AMOUNT_MIN, FREE_CAM_SHAKE_AMOUNT_MAX, ReplayCamera()->m_flRoamingShakeAmount );
		AddSliderToLayout( SLIDER_SHAKE_DIR, m_pShakeDirSlider, "#Replay_ShakeDir", FREE_CAM_SHAKE_DIR_MIN, FREE_CAM_SHAKE_DIR_MAX, ReplayCamera()->m_flRoamingShakeDir );
	}

	enum FreeCamSliders_t
	{
		SLIDER_ACCEL,
		SLIDER_SPEED,
		SLIDER_FOV,
		SLIDER_ROTFILTER,
		SLIDER_SHAKE_SPEED,
		SLIDER_SHAKE_AMOUNT,
		SLIDER_SHAKE_DIR,
	};

	Slider		*m_pAccelSlider;
	Slider		*m_pSpeedSlider;
	Slider		*m_pFovSlider;
	Slider		*m_pRotFilterSlider;
	Slider		*m_pShakeSpeedSlider;
	Slider		*m_pShakeAmountSlider;
	Slider		*m_pShakeDirSlider;
};

//-----------------------------------------------------------------------------

class CReplayButton : public CExImageButton
{
	DECLARE_CLASS_SIMPLE( CReplayButton, CExImageButton );
public:
	CReplayButton( Panel *pParent, const char *pName, const char *pText )
	:	BaseClass( pParent, pName, pText ),
		m_pTipText( NULL )
	{
	}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );

		const char *pTipName = pInResourceData->GetString( "tipname" );
		if ( pTipName && pTipName[0] )
		{
			const wchar_t *pTipText = g_pVGuiLocalize->Find( pTipName );
			if ( pTipText && pTipText[0] )
			{
				const int nTipLength = V_wcslen( pTipText );
				m_pTipText = new wchar_t[ nTipLength + 1 ];
				V_wcsncpy( m_pTipText, pTipText, sizeof(wchar_t) * ( nTipLength + 1 ) );
				m_pTipText[ nTipLength ] = L'\0';
			}
		}
	}

	virtual void OnCursorEntered()
	{
		BaseClass::OnCursorEntered();

		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( pEditor && m_pTipText )
		{
			pEditor->SetButtonTip( m_pTipText, this );
			pEditor->ShowButtonTip( true );
		}
	}

	virtual void OnCursorExited()
	{
		BaseClass::OnCursorExited();

		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( pEditor && m_pTipText )
		{
			pEditor->ShowButtonTip( false );
		}
	}

private:
	wchar_t	*m_pTipText;
};

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CReplayButton, CExImageButton );

//-----------------------------------------------------------------------------

#define MAX_FF_RAMP_TIME		8.0f	// The amount of time until we ramp to max scale value.

class CReplayEditorFastForwardButton : public CReplayButton
{
	DECLARE_CLASS_SIMPLE( CReplayEditorFastForwardButton, CReplayButton );
public:
	CReplayEditorFastForwardButton( Panel *pParent, const char *pName, const char *pText )
	:	BaseClass( pParent, pName, pText ),
		m_flPressTime( 0.0f )
	{
		m_pHostTimescale = cvar->FindVar( "host_timescale" );
		AssertMsg( m_pHostTimescale, "host_timescale lookup failed!" );

		ivgui()->AddTickSignal( GetVPanel(), 10 );
	}

	~CReplayEditorFastForwardButton()
	{
		ivgui()->RemoveTickSignal( GetVPanel() );

		// Avoid a non-1.0 host_timescale after replay edit, which can happen if
		// the user is still holding downt he FF button at the end of the replay.
		if ( m_pHostTimescale )
		{
			m_pHostTimescale->SetValue( 1.0f );
		}

		// Resume demo playback so that any demo played later won't start paused.
		PlayDemo();
	}

	virtual void OnMousePressed( MouseCode code )
	{
		m_flPressTime = gpGlobals->realtime;
		PlayDemo();

		BaseClass::OnMousePressed( code );
	}

	virtual void OnMouseReleased( MouseCode code )
	{
		m_flPressTime = 0.0f;
		PauseDemo();

		BaseClass::OnMouseReleased( code );
	}

	void OnTick()
	{
		float flScale;
		
		if ( m_flPressTime == 0.0f )
		{
			flScale = 1.0f;
		}
		else
		{
			const float flElapsed = clamp( gpGlobals->realtime - m_flPressTime, 0.0f, MAX_FF_RAMP_TIME );
			const float t = CubicEaseIn( flElapsed / MAX_FF_RAMP_TIME );

			// If a shift key is down...
			if ( input()->IsKeyDown( KEY_LSHIFT ) || input()->IsKeyDown( KEY_RSHIFT ) )
			{
				// ...slow down host_timescale.
				flScale = .1f + .4f * t;
			}
			// If alt key down...
			else if ( input()->IsKeyDown( KEY_LALT ) || input()->IsKeyDown( KEY_RALT ) )
			{
				// ...FF very quickly, ramp from 5 to 10.
				flScale = 5.0f + 5.0f * t;
			}
			else
			{
				// Otherwise, start at 1.5 and ramp upwards over time.
				flScale = 1.5f + 3.5f * t;
			}
		}

		// Set host_timescale.
		if ( m_pHostTimescale )
		{
			m_pHostTimescale->SetValue( flScale );
		}
	}

private:
	float	m_flPressTime;
	ConVar *m_pHostTimescale;
};

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CReplayEditorFastForwardButton, CExImageButton );

//-----------------------------------------------------------------------------

class CRecLightPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CRecLightPanel, vgui::EditablePanel );
public:
	CRecLightPanel( Panel *pParent )
	:	EditablePanel( pParent, "RecLightPanel" ),
		m_flPlayPauseTime( 0.0f ),
		m_bPaused( false ),
		m_bPerforming( false )
	{
		m_pRecLights[ 0 ] = NULL;
		m_pRecLights[ 1 ] = NULL;
		m_pPlayPause[ 0 ] = NULL;
		m_pPlayPause[ 1 ] = NULL;
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/replayperformanceeditor/reclight.res", "GAME" );

		m_pRecLights[ 0 ] = dynamic_cast< ImagePanel * >( FindChildByName( "RecLightOffImg" ) );
		m_pRecLights[ 1 ] = dynamic_cast< ImagePanel * >( FindChildByName( "RecLightOnImg" ) );

		m_pPlayPause[ 0 ] = dynamic_cast< ImagePanel * >( FindChildByName( "PlayImg" ) );
		m_pPlayPause[ 1 ] = dynamic_cast< ImagePanel * >( FindChildByName( "PauseImg" ) );

		m_pCameraFringe = dynamic_cast< ImagePanel *>( FindChildByName( "CameraFringe" ) );
		m_pCameraCrosshair = dynamic_cast< ImagePanel *>( FindChildByName( "CameraCrosshair" ) );
	}
	
	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		SetVisible( m_bPerforming );

		const int nScreenWidth = ScreenWidth();
		const int nRecLightW = m_pRecLights[ 0 ]->GetWide();
		int nXPos = nScreenWidth - nRecLightW + XRES( 6 );
		int nYPos = -YRES( 8 );
		m_pRecLights[ 0 ]->SetPos( nXPos, nYPos );
		m_pRecLights[ 1 ]->SetPos( nXPos, nYPos );

		const int nWidth = GetWide();
		const int nHeight = GetTall();

		// Setup camera fringe height
		if ( m_pCameraFringe )
		{
			m_pCameraFringe->SetSize( nWidth, nHeight );
			m_pCameraFringe->InstallMouseHandler( this );
		}

		// Setup camera cross hair height
		if ( m_pCameraCrosshair )
		{
			int aImageSize[2];
			IImage *pImage = m_pCameraCrosshair->GetImage();
			pImage->GetSize( aImageSize[0], aImageSize[1] );

			aImageSize[0] = m_pCameraCrosshair->GetWide();
			aImageSize[1] = m_pCameraCrosshair->GetTall();

			const int nStartY = YRES( 13 );

			m_pCameraCrosshair->SetBounds(
				nStartY + ( nWidth - aImageSize[0] ) / 2,
				nStartY + ( nHeight - aImageSize[1] ) / 2,
				aImageSize[0] - 2 * nStartY,
				aImageSize[1] - 2 * nStartY
			);

			m_pCameraCrosshair->InstallMouseHandler( this );
		}
	}

	void UpdateBackgroundVisibility()
	{
		m_pCameraCrosshair->SetVisible( m_bPaused );
		m_pCameraFringe->SetVisible( m_bPaused );
	}

	virtual void OnThink()
	{
		const float flTime = gpGlobals->realtime;
		bool bPauseAnimating = m_flPlayPauseTime > 0.0f &&
			                   flTime >= m_flPlayPauseTime &&
							   flTime < ( m_flPlayPauseTime + m_flAnimTime );

		// Setup light visibility
		int nOnOff = fmod( flTime * 2.0f, 2.0f );
		bool bOnLightVisible = (bool)nOnOff;
		bool bRecording = g_pReplayPerformanceController->IsRecording();
		m_pRecLights[ 0 ]->SetVisible( m_bPaused || ( bRecording && !bOnLightVisible ) );
		m_pRecLights[ 1 ]->SetVisible( bRecording && ( !m_bPaused && bOnLightVisible ) );

		// Deal with fringe and crosshair vis
		UpdateBackgroundVisibility();

		int iPlayPauseActive = (int)m_bPaused;

		// Animate the pause icon
		if ( bPauseAnimating )
		{
			const float t = clamp( ( flTime - m_flPlayPauseTime ) / m_flAnimTime, 0.0f, 1.0f );
			const float s = SCurve( t );
			const int nSize = (int)Lerp( s, 60.0f, 60.0f * m_nAnimScale );
			int aCrossHairPos[2];
			m_pCameraCrosshair->GetPos( aCrossHairPos[0], aCrossHairPos[1] );
			const int nScreenXCenter = aCrossHairPos[0] + m_pCameraCrosshair->GetWide() / 2;
			const int nScreenYCenter = aCrossHairPos[1] + m_pCameraCrosshair->GetTall() / 2;

			m_pPlayPause[ iPlayPauseActive ]->SetBounds( 
				nScreenXCenter - nSize / 2,
				nScreenYCenter - nSize / 2,
				nSize,
				nSize
			);

			m_pPlayPause[ iPlayPauseActive ]->SetAlpha( (int)( MIN( 0.5f, 1.0f - s ) * 255) );
		}

		m_pPlayPause[  iPlayPauseActive ]->SetVisible( bPauseAnimating );
		m_pPlayPause[ !iPlayPauseActive ]->SetVisible( false );
	}

	void UpdatePauseState( bool bPaused )
	{
		if ( bPaused == m_bPaused )
			return;

		m_bPaused = bPaused;

		m_flPlayPauseTime = gpGlobals->realtime;
	}

	void SetPerforming( bool bPerforming )
	{
		if ( bPerforming == m_bPerforming )
			return;

		m_bPerforming = bPerforming;
		InvalidateLayout( true, false );
	}

	float		m_flPlayPauseTime;
	bool		m_bPaused;
	bool		m_bPerforming;
	ImagePanel	*m_pPlayPause[2];	// 0=play, 1=pause
	ImagePanel	*m_pRecLights[2];	// 0=off, 1=on
	ImagePanel	*m_pCameraFringe;
	ImagePanel	*m_pCameraCrosshair;

	CPanelAnimationVar( int, m_nAnimScale, "anim_scale", "4" );
	CPanelAnimationVar( float, m_flAnimTime, "anim_time", "1.5" );
};

//-----------------------------------------------------------------------------

CReplayPerformanceEditorPanel::CReplayPerformanceEditorPanel( Panel *parent, ReplayHandle_t hReplay )
:	EditablePanel( parent, "ReplayPerformanceEditor" ),
	m_hReplay( hReplay ),
	m_flLastTime( -1 ),
	m_nRedBlueLabelRightX( 0 ),
	m_nBottomPanelStartY( 0 ),
	m_nBottomPanelHeight( 0 ),
	m_nLastRoundedTime( -1 ),
	m_flSpaceDownStart( 0.0f ),
	m_flOldFps( -1.0f ),
	m_flLastTimeSpaceBarPressed( 0.0f ),
	m_flActiveTimeInEditor( 0.0f ),
	m_flTimeScaleProxy( 1.0f ),
	m_iCameraSelection( CAM_FIRST ),
	m_bMousePressed( false ),
	m_bMouseDown( false ),
	m_nMouseClickedOverCameraSettingsPanel( CAM_INVALID ),
	m_bShownAtLeastOnce( false ),
	m_bAchievementAwarded( false ),
	m_pImageList( NULL ),
	m_pCurTimeLabel( NULL ),
	m_pTotalTimeLabel( NULL ),
	m_pPlayerNameLabel( NULL ),
	m_pMouseTargetPanel( NULL ),
	m_pSlowMoButton( NULL ),
	m_pRecLightPanel( NULL ),
	m_pPlayerCellData( NULL ),
	m_pBottom( NULL ),
	m_pMenuButton( NULL ),
	m_pMenu( NULL ),
	m_pPlayerCellsPanel( NULL ),
	m_pButtonTip( NULL ),
	m_pSavingDlg( NULL )
{
	V_memset( m_pCameraButtons, 0, sizeof( m_pCameraButtons ) );
	V_memset( m_pCtrlButtons, 0, sizeof( m_pCtrlButtons ) );
	V_memset( m_pCameraOptionsPanels, NULL, sizeof( m_pCameraOptionsPanels ) );

	m_pCameraOptionsPanels[ CAM_FREE ] = new CCameraOptionsPanel_Free( this );
	m_pCameraOptionsPanels[ COMPONENT_TIMESCALE ] = new CTimeScaleOptionsPanel( this, &m_flTimeScaleProxy );

	m_nRedBlueSigns[0] = -1;
	m_nRedBlueSigns[1] = 1;
	m_iCurPlayerTarget = -1;
	m_bCurrentTargetNeedsVisibilityUpdate = false;

	m_pImageList = new ImageList( false );

	SetParent( g_pClientMode->GetViewport() );

	HScheme hScheme = scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme( hScheme );

	ivgui()->AddTickSignal( GetVPanel(), 16 );	// Roughly 60hz

	MakePopup( true );
	SetMouseInputEnabled( true );

	// Create bottom
	m_pBottom = new EditablePanel( this, "BottomPanel" );

	// Add player cells
	m_pPlayerCellsPanel = new EditablePanel( m_pBottom, "PlayerCellsPanel" );
	for ( int i = 0; i < 2; ++i )
	{
		for ( int j = 0; j <= MAX_PLAYERS; ++j )
		{
			m_pPlayerCells[i][j] = new CPlayerCell( m_pPlayerCellsPanel, "PlayerCell", &m_iCurPlayerTarget );
			m_pPlayerCells[i][j]->SetVisible( false );
			AddPanelKeyboardInputDisableList( m_pPlayerCells[i][j] );
		}
	}

	// Create rec light panel
	m_pRecLightPanel = SETUP_PANEL( new CRecLightPanel( g_pClientMode->GetViewport() ) );

	// Display "enter performance mode" tip
	DisplayPerformanceTip( "#Replay_PerfTip_EnterPerfMode", &replay_perftip_count_enter, MAX_TIP_DISPLAYS );

	// Create menu
	m_pMenu = new Menu( this, "Menu" );
	m_aMenuItemIds[ MENU_SAVE   ] = m_pMenu->AddMenuItem( "#Replay_Save", "menu_save", this );
	m_aMenuItemIds[ MENU_SAVEAS ] = m_pMenu->AddMenuItem( "#Replay_SaveAs", "menu_saveas", this );
	m_pMenu->AddSeparator();
	m_aMenuItemIds[ MENU_EXIT   ] = m_pMenu->AddMenuItem( "#Replay_Exit", "menu_exit", this );

	m_pMenu->EnableUseMenuManager( false );	// The menu manager doesn't play nice with the menu button
}

CReplayPerformanceEditorPanel::~CReplayPerformanceEditorPanel()
{
	m_pRecLightPanel->MarkForDeletion();
	m_pRecLightPanel = NULL;

	m_pButtonTip->MarkForDeletion();
	m_pButtonTip = NULL;

	g_bIsReplayRewinding = false;

	surface()->PlaySound( "replay\\performanceeditorclosed.wav" );

	CPerformanceTip::Cleanup();

	ClearPlayerCellData();
}

void CReplayPerformanceEditorPanel::ClearPlayerCellData()
{
	if ( m_pPlayerCellData )
	{
		m_pPlayerCellData->deleteThis();
		m_pPlayerCellData = NULL;
	}
}

void CReplayPerformanceEditorPanel::AddPanelKeyboardInputDisableList( Panel *pPanel )
{
	m_lstDisableKeyboardInputPanels.AddToTail( pPanel );
}

void CReplayPerformanceEditorPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/ui/replayperformanceeditor/main.res", "GAME" );

	m_lstDisableKeyboardInputPanels.RemoveAll();

	int nParentWidth = GetParent()->GetWide();
	int nParentHeight = GetParent()->GetTall();

	// Set size of this panel
	SetSize( nParentWidth, nParentHeight );

	// Layout bottom
	if ( m_pBottom )
	{
		m_nBottomPanelHeight = m_pBottom->GetTall();	// Get from .res
		m_nBottomPanelStartY = nParentHeight - m_nBottomPanelHeight;
		m_pBottom->SetBounds( 0, m_nBottomPanelStartY, nParentWidth, m_nBottomPanelHeight );
	}

	// Layout rec light panel - don't overlap bottom panel
	m_pRecLightPanel->SetBounds( 0, 0, ScreenWidth(), m_nBottomPanelStartY );

	// Setup camera buttons
	const int nNumCameraButtons = NCAMS;
	const char *pCameraButtonNames[nNumCameraButtons] = { "CameraFree", "CameraThird", "CameraFirst", "TimeScaleButton" };
	int nCurButtonX = nParentWidth - m_nRightMarginWidth;
	int nLeftmostCameraButtonX = 0;
	for ( int i = 0; i < nNumCameraButtons; ++i )
	{
		m_pCameraButtons[i] = dynamic_cast< CExImageButton * >( FindChildByName( pCameraButtonNames[ i ] ) );
		if ( m_pCameraButtons[i] )
		{
			CExImageButton *pCurButton = m_pCameraButtons[ i ];
			if ( !pCurButton )
				continue;

			nCurButtonX -= pCurButton->GetWide();

			int nX, nY;
			pCurButton->GetPos( nX, nY );
			pCurButton->SetPos( nCurButtonX, nY );

			pCurButton->SetParent( m_pBottom );
			pCurButton->AddActionSignalTarget( this );

#if !defined( TF_CLIENT_DLL )
			pCurButton->SetPaintBorderEnabled( false );
#endif

			AddPanelKeyboardInputDisableList( pCurButton );
		}
	}
	nLeftmostCameraButtonX = nCurButtonX;

	static const char *s_pControlButtonNames[NUM_CTRLBUTTONS] = {
		"InButton", "GotoBeginningButton", "RewindButton",
		"PlayButton",
		"FastForwardButton", "GotoEndButton", "OutButton"
	};
	for ( int i = 0; i < NUM_CTRLBUTTONS; ++i )
	{
		CExImageButton *pCurButton = dynamic_cast< CExImageButton * >( FindChildByName( s_pControlButtonNames[ i ] ) );	Assert( pCurButton );
		if ( !pCurButton )
			continue;

		pCurButton->SetParent( m_pBottom );
		pCurButton->AddActionSignalTarget( this );

		AddPanelKeyboardInputDisableList( pCurButton );

#if !defined( TF_CLIENT_DLL )
		pCurButton->SetPaintBorderEnabled( false );
#endif

		m_pCtrlButtons[ i ] = pCurButton;
	}

	// If the performance in tick is set, highlight the in point button
	{
		CReplayPerformance *pSavedPerformance = GetSavedPerformance();
		m_pCtrlButtons[ CTRLBUTTON_IN  ]->SetSelected( pSavedPerformance && pSavedPerformance->HasInTick() );
		m_pCtrlButtons[ CTRLBUTTON_OUT ]->SetSelected( pSavedPerformance && pSavedPerformance->HasOutTick() );
	}

	// Select first-person camera by default.
	UpdateCameraSelectionPosition( CAM_FIRST );

	// Position time label
	m_pCurTimeLabel = dynamic_cast< CExLabel * >( FindChildByName( "CurTimeLabel" ) );
	m_pTotalTimeLabel = dynamic_cast< CExLabel * >( FindChildByName( "TotalTimeLabel" ) );

	m_pCurTimeLabel->SetParent( m_pBottom );
	m_pTotalTimeLabel->SetParent( m_pBottom );

	// Get player name label
	m_pPlayerNameLabel = dynamic_cast< CExLabel * >( FindChildByName( "PlayerNameLabel" ) );

	// Get mouse target panel
	m_pMouseTargetPanel = dynamic_cast< EditablePanel * >( FindChildByName( "MouseTargetPanel" ) );

	for ( int i = 0; i < 2; ++i )
	{
		for ( int j = 0; j <= MAX_PLAYERS; ++j )
		{
			m_pPlayerCells[i][j]->SetMouseInputEnabled( true );
		}
	}

	// Get menu button
	m_pMenuButton = dynamic_cast< CExImageButton * >( FindChildByName( "MenuButton" ) );
	AddPanelKeyboardInputDisableList( m_pMenuButton );
	m_pMenuButton->SetMouseInputEnabled( true );
#if !defined( TF_CLIENT_DLL )
	m_pMenuButton->SetPaintBorderEnabled( false );
#endif

	// Get button tip
	m_pButtonTip = dynamic_cast< CReplayTipLabel * >( FindChildByName( "ButtonTip" ) );
	m_pButtonTip->SetParent( g_pClientMode->GetViewport() );
}

static void Replay_GotoTick( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		int nGotoTick = (int)(intp)pContext;
		CFmtStr fmtCmd( "demo_gototick %i\ndemo_pause\n", nGotoTick );
		engine->ClientCmd_Unrestricted( fmtCmd.Access() );
	}
}

void CReplayPerformanceEditorPanel::OnSliderMoved( KeyValues *pParams )
{
}

void CReplayPerformanceEditorPanel::OnInGameMouseWheelEvent( int nDelta )
{
	HandleMouseWheel( nDelta );
}

void CReplayPerformanceEditorPanel::HandleMouseWheel( int nDelta )
{
	if ( ReplayCamera()->GetMode() == OBS_MODE_ROAMING )
	{
		// Invert mousewheel input if necessary
		if ( replay_editor_fov_mousewheel_invert.GetBool() )
		{
			nDelta *= -1;
		}

		float &flFov = ReplayCamera()->m_flRoamingFov[1];
		flFov = clamp( flFov - nDelta * replay_editor_fov_mousewheel_multiplier.GetFloat(), FREE_CAM_FOV_MIN, FREE_CAM_FOV_MAX );

		// Update FOV slider in free camera settings
		CCameraOptionsPanel_Free *pFreeCamOptions = static_cast< CCameraOptionsPanel_Free * >( m_pCameraOptionsPanels[ CAM_FREE ] );
		pFreeCamOptions->m_pFovSlider->SetValue( flFov - FREE_CAM_FOV_MIN, false );
	}
}

void CReplayPerformanceEditorPanel::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	ClearPlayerCellData();

	KeyValues *pPlayerCellData = pInResourceData->FindKey( "PlayerCell" );
	if ( pPlayerCellData )
	{
		m_pPlayerCellData = new KeyValues( "PlayerCell" );
		pPlayerCellData->CopySubkeys( m_pPlayerCellData );
	}
}

CameraMode_t CReplayPerformanceEditorPanel::IsMouseOverActiveCameraOptionsPanel( int nMouseX, int nMouseY )
{
	// In one of the camera options panels?
	for ( int i = 0; i < NCAMS; ++i )
	{
		CCameraOptionsPanel *pCurPanel = m_pCameraOptionsPanels[ i ];
		if ( pCurPanel && pCurPanel->IsVisible() && pCurPanel->IsWithin( nMouseX, nMouseY ) )
			return (CameraMode_t)i;
	}

	return CAM_INVALID;
}

void CReplayPerformanceEditorPanel::OnMouseWheeled( int nDelta )
{
	HandleMouseWheel( nDelta );
}

void CReplayPerformanceEditorPanel::OnTick()
{
	BaseClass::OnTick();

//	engine->Con_NPrintf( 0, "timescale: %f", g_pReplayPerformanceController->GetPlaybackTimeScale() );

	C_ReplayCamera *pCamera = ReplayCamera();
	if ( !pCamera )
		return;

	// Calc elapsed time
	float flElapsed = gpGlobals->realtime - m_flLastTime;
	m_flLastTime = gpGlobals->realtime;

	// If this is the first time we're running and camera is valid, get primary target
	if ( m_iCurPlayerTarget < 0 )
	{
		m_iCurPlayerTarget = pCamera->GetPrimaryTargetIndex();
	}

	// NOTE: Third-person is not "controllable" yet
	int nCameraMode = pCamera->GetMode();
	bool bInAControllableCameraMode = nCameraMode == OBS_MODE_ROAMING || nCameraMode == OBS_MODE_CHASE;

	// Get mouse cursor pos
	int nMouseX, nMouseY;
	input()->GetCursorPos( nMouseX, nMouseY );

	// Toggle in and out of camera control if appropriate
	// Mouse pressed?
	bool bMouseDown = input()->IsMouseDown( MOUSE_LEFT );
	m_bMousePressed = bMouseDown && !m_bMouseDown;
	m_bMouseDown = bMouseDown;

	// Reset this flag if mouse is no longer down
	if ( !m_bMouseDown )
	{
		m_nMouseClickedOverCameraSettingsPanel = CAM_INVALID;
	}

	bool bNoDialogsUp = TFModalStack()->IsEmpty();
	bool bMouseCursorOverPerfEditor = nMouseY >= m_nBottomPanelStartY;
	bool bMouseOverMenuButton = m_pMenuButton->IsWithin( nMouseX, nMouseY );
	bool bMouseOverMenu = m_pMenu->IsWithin( nMouseX, nMouseY );
	bool bRecording = g_pReplayPerformanceController->IsRecording();
	if ( IsVisible() && m_bMousePressed )
	{
		CameraMode_t nActiveOptionsPanel = IsMouseOverActiveCameraOptionsPanel( nMouseX, nMouseY );
		if ( nActiveOptionsPanel != CAM_INVALID )
		{
			m_nMouseClickedOverCameraSettingsPanel = nActiveOptionsPanel;
		}
		else if ( m_pMenu->IsVisible() && !m_pMenu->IsWithin( nMouseX, nMouseY ) )
		{
			ToggleMenu();
		}
		else if ( bInAControllableCameraMode && !bMouseCursorOverPerfEditor && !bMouseOverMenuButton &&
			!bMouseOverMenu && bNoDialogsUp )
		{
			if ( bRecording )
			{
				bool bMouseInputEnabled = IsMouseInputEnabled();

				// Already in a controllable camera mode?
				if ( bMouseInputEnabled )
				{
					DisplayPerformanceTip( "#Replay_PerfTip_ExitFreeCam", &replay_perftip_count_freecam_exit, MAX_TIP_DISPLAYS );	
					surface()->PlaySound( "replay\\cameracontrolmodeentered.wav" );
				}
				else
				{
					DisplayPerformanceTip( "#Replay_PerfTip_EnterFreeCam", &replay_perftip_count_freecam_enter, MAX_TIP_DISPLAYS );
					surface()->PlaySound( "replay\\cameracontrolmodeexited.wav" );
				}

				SetMouseInputEnabled( !bMouseInputEnabled );
			}
			else
			{
				// Play an error sound
				surface()->PlaySound( "replay\\cameracontrolerror.wav" );
			}
		}
	}

	// Show panel if space key bar is down
	bool bSpaceDown = bNoDialogsUp && !enginevgui->IsGameUIVisible() && input()->IsKeyDown( KEY_SPACE );
	m_bSpacePressed = bSpaceDown && !m_bSpaceDown;
	m_bSpaceDown = bSpaceDown;

	// Modify visibility?
	bool bShow = IsVisible();
	if ( m_bSpacePressed )
	{
		bShow = !IsVisible();
	}

	// Set visibility?
	if ( IsVisible() != bShow )
	{
		ShowPanel( bShow );
		m_bShownAtLeastOnce = true;

		// For achievements:
		Achievements_OnSpaceBarPressed();
	}

	// Factor in host_timescale.
	float flScaledElapsed = flElapsed;
	ConVarRef host_timescale( "host_timescale" );
	if ( host_timescale.GetFloat() > 0 )
	{
		flScaledElapsed *= host_timescale.GetFloat();
	}

	// Do FOV smoothing
	ReplayCamera()->SmoothFov( flScaledElapsed );

	// Don't do any more processing if not needed
	if ( !m_bShownAtLeastOnce )
		return;

	// Update time text if necessary
	UpdateTimeLabels();

	// Make all player cells invisible
	int nTeamCounts[2] = {0,0};
	int nCurTeam = 0;
	for ( int i = 0; i < 2; ++i )
	for ( int j = 0; j <= MAX_PLAYERS; ++j )
	{
		m_pPlayerCells[i][j]->SetVisible( false );
	}

	int iMouseOverPlayerIndex = -1;
	CPlayerCell *pMouseOverCell = NULL;

	// Update player cells
	bool bLayoutPlayerCells = true;	// TODO: only layout when necessary
	C_ReplayGame_PlayerResource_t *pGamePlayerResource = dynamic_cast< C_ReplayGame_PlayerResource_t * >( g_PR );
	for ( int iPlayer = 1; iPlayer <= MAX_PLAYERS; ++iPlayer )
	{
		IGameResources *pGR = GameResources();

		if ( !pGR || !pGR->IsConnected( iPlayer ) )
			continue;

		// Which team?
		int iTeam = pGR->GetTeam( iPlayer );
		switch ( iTeam )
		{
		case REPLAY_TEAM_TEAM0:
			++nTeamCounts[0];
			nCurTeam = 0;
			break;
		case REPLAY_TEAM_TEAM1:
			++nTeamCounts[1];
			nCurTeam = 1;
			break;
		default:
			nCurTeam = -1;
			break;
		}

		if ( nCurTeam < 0 )
			continue;

#if !defined( CSTRIKE_DLL )
		int iPlayerClass = pGamePlayerResource->GetPlayerClass( iPlayer );
		if ( iPlayerClass == REPLAY_CLASS_UNDEFINED )
			continue;
#endif
					 
		int nCurTeamCount = nTeamCounts[ nCurTeam ];
		CPlayerCell* pCell = m_pPlayerCells[ nCurTeam ][ nCurTeamCount-1 ];

		// Cache the player index
		pCell->m_iPlayerIndex = iPlayer;

		// Make visible
		pCell->SetVisible( true );

		// Show leaderboard icon
#if defined( TF_CLIENT_DLL )
		char szClassImg[64];
		extern const char *g_aPlayerClassNames_NonLocalized[ REPLAY_NUM_CLASSES ];
		char const *pClassName = iPlayerClass == TF_CLASS_DEMOMAN
			? "demo"
			: g_aPlayerClassNames_NonLocalized[ iPlayerClass ];
		V_snprintf( szClassImg, sizeof( szClassImg ), "../HUD/leaderboard_class_%s", pClassName );

		// Show dead icon instead?
		if ( !pGamePlayerResource->IsAlive( iPlayer ) )
		{
			V_strcat( szClassImg, "_d", sizeof( szClassImg ) );
		}

		IImage *pImage = scheme()->GetImage( szClassImg, true );
		if ( pImage )
		{
			pImage->SetSize( 32, 32 );
			pCell->GetImage()->SetImage( pImage );
		}

#elif defined( CSTRIKE_DLL )
		// TODO - create and use class icons
		char szText[16];
		V_snprintf( szText, sizeof( szText ), "%i", nTeamCounts[ nCurTeam ] );
		pCell->SetText( szText );
#endif

		// Display player name if mouse is over the current cell
		if ( pCell->IsWithin( nMouseX, nMouseY ) )
		{
			iMouseOverPlayerIndex = iPlayer;
			pMouseOverCell = pCell;
		}
	}

	// Check to see if we're hovering over a camera-mode, and if so, display its options panel if it has one
	if ( bRecording )
	{
		for ( int i = 0; i < NCAMS; ++i )
		{
			CCameraOptionsPanel *pCurOptionsPanel = m_pCameraOptionsPanels[ i ];
			if ( !pCurOptionsPanel )
				continue;

			bool bMouseOverButton = m_pCameraButtons[ i ]->IsWithin( nMouseX, nMouseY );
			bool bMouseOverOptionsPanel = pCurOptionsPanel->IsWithin( nMouseX, nMouseY );
			bool bInCameraModeThatMouseIsOver = ReplayCamera()->GetMode() == GetCameraModeFromButtonIndex( (CameraMode_t)i );
			bool bDontCareAboutCameraMode = i == COMPONENT_TIMESCALE;
			bool bActivate = ( i == m_nMouseClickedOverCameraSettingsPanel ) ||
				( ( ( bInCameraModeThatMouseIsOver || bDontCareAboutCameraMode ) && bMouseOverButton ) || ( bMouseOverOptionsPanel && pCurOptionsPanel->IsVisible() ) );
			pCurOptionsPanel->SetVisible( bActivate );
		}
	}

	if ( bLayoutPlayerCells )
	{
		LayoutPlayerCells();
	}

	// Setup player name label and temporary camera view
	if ( m_pPlayerNameLabel && pGamePlayerResource && pMouseOverCell )
	{
		m_pPlayerNameLabel->SetText( pGamePlayerResource->GetPlayerName( iMouseOverPlayerIndex ) );
		m_pPlayerNameLabel->SizeToContents();

		int nCellPos[2];
		pMouseOverCell->GetPos( nCellPos[0], nCellPos[1] );

		int nLabelX = MAX(
			nCellPos[0],
			m_nRedBlueLabelRightX
		);
		int nLabelY = m_nBottomPanelStartY + ( m_nBottomPanelHeight - m_pPlayerNameLabel->GetTall() ) / 2;
		m_pPlayerNameLabel->SetPos( nLabelX, nLabelY );

		m_pPlayerNameLabel->SetVisible( true );

		// Setup camera
		pCamera->SetPrimaryTarget( iMouseOverPlayerIndex );
	}
	else
	{
		m_pPlayerNameLabel->SetVisible( false );

		// Set camera to last valid target
		Assert( m_iCurPlayerTarget >= 0 );
		pCamera->SetPrimaryTarget( m_iCurPlayerTarget );
	}

	// If user clicked, assume it was the selected cell and set primary target in camera
	if ( iMouseOverPlayerIndex >= 0 )
	{
		pCamera->SetPrimaryTarget( iMouseOverPlayerIndex );
	}
	else
	{
		pCamera->SetPrimaryTarget( m_iCurPlayerTarget );
	}

	// fixes a case where the replay would be paused and the player would cycle cameras but the 
	// target's visibility wouldn't be updated until the replay was unpaused (they would be invisible)
	if ( m_bCurrentTargetNeedsVisibilityUpdate )
	{
		C_BaseEntity *pTarget = ClientEntityList().GetEnt( pCamera->GetPrimaryTargetIndex() );
		if ( pTarget )
		{
			pTarget->UpdateVisibility();
		}

		m_bCurrentTargetNeedsVisibilityUpdate = false;
	}

	// If in free-cam mode, add set view event if we're not paused
	if ( bInAControllableCameraMode && m_bShownAtLeastOnce && bRecording )
	{
		AddSetViewEvent();
		AddTimeScaleEvent( m_flTimeScaleProxy );
	}

	// Set paused state in rec light
	const bool bPaused = IsPaused();
	m_pRecLightPanel->UpdatePauseState( bPaused );

	Achievements_Think( flElapsed );
}

void CReplayPerformanceEditorPanel::Achievements_OnSpaceBarPressed()
{
	m_flLastTimeSpaceBarPressed = gpGlobals->realtime;
}

void CReplayPerformanceEditorPanel::Achievements_Think( float flElapsed )
{
//	engine->Con_NPrintf( 10, "total time: %f", m_flActiveTimeInEditor );
//	engine->Con_NPrintf( 11, "last time space bar pressed: %f", m_flLastTimeSpaceBarPressed );

	// Already awarded one this editing session?
	if ( m_bAchievementAwarded )
		return;
	
	// Too much idle time since last activity?
	if ( gpGlobals->realtime - m_flLastTimeSpaceBarPressed > 60.0f )
	{
		m_flActiveTimeInEditor = 0.0f;
		return;
	}

	// Accumulate active time
	m_flActiveTimeInEditor += flElapsed;

	// Award now if three-minutes of non-idle time has passed
	const float flMinutes = 60.0f * 3.0f;
	if ( m_flActiveTimeInEditor < flMinutes )
		return;

	Achievements_Grant();
}

void CReplayPerformanceEditorPanel::Achievements_Grant()
{
#if defined( TF_CLIENT_DLL )
	g_AchievementMgrTF.AwardAchievement( ACHIEVEMENT_TF_REPLAY_EDIT_TIME );
#endif

	// Awarded
	m_bAchievementAwarded = true;
}

bool CReplayPerformanceEditorPanel::IsPaused()
{
	return IsVisible();
}

CReplayPerformance *CReplayPerformanceEditorPanel::GetPerformance() const
{
	return g_pReplayPerformanceController->GetPerformance();
}

CReplayPerformance *CReplayPerformanceEditorPanel::GetSavedPerformance() const
{
	return g_pReplayPerformanceController->GetSavedPerformance();
}

int CReplayPerformanceEditorPanel::GetCameraModeFromButtonIndex( CameraMode_t iCamera )
{
	switch ( iCamera )
	{
	case CAM_FREE:	return OBS_MODE_ROAMING;
	case CAM_THIRD:	return OBS_MODE_CHASE;
	case CAM_FIRST:	return OBS_MODE_IN_EYE;
	}
	return CAM_INVALID;
}

void CReplayPerformanceEditorPanel::UpdateTimeLabels()
{
	CReplay *pPlayingReplay = g_pReplayManager->GetPlayingReplay();

	if ( !pPlayingReplay || !m_pCurTimeLabel || !m_pTotalTimeLabel )
		return;

	float flCurTime, flTotalTime;
	g_pClientReplayContext->GetPlaybackTimes( flCurTime, flTotalTime, pPlayingReplay, GetPerformance() );

	int nCurRoundedTime = (int)flCurTime;	// Essentially floor'd
	if ( nCurRoundedTime == m_nLastRoundedTime )
		return;

	m_nLastRoundedTime = nCurRoundedTime;

	// Set current time text
	char szTimeText[64];
	V_snprintf( szTimeText, sizeof( szTimeText ), "%s", CReplayTime::FormatTimeString( nCurRoundedTime ) );
	m_pCurTimeLabel->SetText( szTimeText );

	// Set total time text
	V_snprintf( szTimeText, sizeof( szTimeText ), "%s", CReplayTime::FormatTimeString( (int)flTotalTime ) );
	m_pTotalTimeLabel->SetText( szTimeText );

	// Center between left-most camera button and play/pause button
	m_pCurTimeLabel->SizeToContents();
	m_pTotalTimeLabel->SizeToContents();
}

void CReplayPerformanceEditorPanel::UpdateCameraSelectionPosition( CameraMode_t nCameraMode )
{
	Assert( nCameraMode >= 0 && nCameraMode < NCAMS );
	m_iCameraSelection = nCameraMode;

	UpdateCameraButtonImages();
}

void CReplayPerformanceEditorPanel::UpdateFreeCamSettings( const SetViewParams_t &params )
{
	CCameraOptionsPanel_Free *pSettingsPanel = dynamic_cast< CCameraOptionsPanel_Free * >( m_pCameraOptionsPanels[ CAM_FREE ] );
	if ( !pSettingsPanel )
		return;

	pSettingsPanel->SetValue( CCameraOptionsPanel_Free::SLIDER_ACCEL, params.m_flAccel );
	pSettingsPanel->SetValue( CCameraOptionsPanel_Free::SLIDER_SPEED, params.m_flSpeed );
	pSettingsPanel->SetValue( CCameraOptionsPanel_Free::SLIDER_FOV, params.m_flFov );
	pSettingsPanel->SetValue( CCameraOptionsPanel_Free::SLIDER_ROTFILTER, params.m_flRotationFilter );
}

void CReplayPerformanceEditorPanel::UpdateTimeScale( float flScale )
{
	CTimeScaleOptionsPanel *pSettingsPanel = dynamic_cast< CTimeScaleOptionsPanel * >( m_pCameraOptionsPanels[ COMPONENT_TIMESCALE ] );
	if ( !pSettingsPanel )
		return;

	pSettingsPanel->SetValue( CTimeScaleOptionsPanel::SLIDER_TIMESCALE, flScale );
}

void CReplayPerformanceEditorPanel::LayoutPlayerCells()
{
	int nPanelHeight = m_pPlayerCellsPanel->GetTall();
	int nCellBuffer = XRES(1);
	for ( int i = 0; i < 2; ++i )
	{
		int nCurX = m_nRedBlueLabelRightX;

		for ( int j = 0; j <= MAX_PLAYERS; ++j )
		{
			CPlayerCell *pCurCell = m_pPlayerCells[i][j];
			if ( !pCurCell->IsVisible() )
				continue;

			// Apply cached settings from .res file
			if ( m_pPlayerCellData )
			{
				pCurCell->ApplySettings( m_pPlayerCellData );
			}

			const int nY = nPanelHeight/2 + m_nRedBlueSigns[i] * nPanelHeight/4 - pCurCell->GetTall()/2;
			pCurCell->SetPos(
				nCurX,
				nY
			);

			nCurX += pCurCell->GetWide() + nCellBuffer;
		}
	}
}

void CReplayPerformanceEditorPanel::PerformLayout() 
{
	int w = ScreenWidth(), h = ScreenHeight();
	SetBounds(0,0,w,h);

	// Layout camera options panels
	for ( int i = 0; i < NCAMS; ++i )
	{
		CCameraOptionsPanel *pCurOptionsPanel = m_pCameraOptionsPanels[ i ];
		if ( !pCurOptionsPanel )
			continue;

		CExImageButton *pCurCameraButton = m_pCameraButtons[ i ];	
		if ( !pCurCameraButton )
			continue;

		// Get camera button position
		int aCameraButtonPos[2];
		int aBottomPos[2];
		pCurCameraButton->GetPos( aCameraButtonPos[ 0 ], aCameraButtonPos[ 1 ] );
		m_pBottom->GetPos( aBottomPos[ 0 ], aBottomPos[ 1 ] );

		// Layout the panel now - it should set its own size, which we need to know to position it properly
		pCurOptionsPanel->InvalidateLayout( true, true );

		// Position it
		pCurOptionsPanel->SetPos(
			aBottomPos[ 0 ] + aCameraButtonPos[ 0 ] + pCurCameraButton->GetWide() - pCurOptionsPanel->GetWide() - XRES( 3 ),
			aBottomPos[ 1 ] + aCameraButtonPos[ 1 ] - pCurOptionsPanel->GetTall()
		);
	}

	// Setup menu position relative to menu button
	int aMenuButtonPos[2];
	m_pMenuButton->GetPos( aMenuButtonPos[0], aMenuButtonPos[1] );
	m_pMenu->SetPos( aMenuButtonPos[0], aMenuButtonPos[1] + m_pMenuButton->GetTall() );

	// Set player cell panel to be the size of half the bottom panel
	int aBottomSize[2];
	m_pBottom->GetSize( aBottomSize[0], aBottomSize[1] );
	m_pPlayerCellsPanel->SetBounds( 0, 0, aBottomSize[0] / 2, m_pPlayerCellsPanel->GetTall() );

	CExLabel *pRedBlueLabels[2] = {
		dynamic_cast< CExLabel * >( m_pPlayerCellsPanel->FindChildByName( "RedLabel" ) ),
		dynamic_cast< CExLabel * >( m_pPlayerCellsPanel->FindChildByName( "BlueLabel" ) )
	};
	int nMargins[2] = { (int)XRES( 5 ), (int)YRES( 2 ) };
	for ( int i = 0; i < 2; ++i )
	{
		pRedBlueLabels[i]->SizeToContents();

		const int nY = m_pPlayerCellsPanel->GetTall()/2 + m_nRedBlueSigns[i] * m_pPlayerCellsPanel->GetTall()/4 - pRedBlueLabels[i]->GetTall()/2;
		pRedBlueLabels[i]->SetPos( nMargins[0], nY );

		m_nRedBlueLabelRightX = MAX( m_nRedBlueLabelRightX, nMargins[0] + pRedBlueLabels[i]->GetWide() + nMargins[0] );
	}

	// Position player cells
	LayoutPlayerCells();

	BaseClass::PerformLayout();
}

bool CReplayPerformanceEditorPanel::OnStateChangeRequested( const char *pEventStr )
{
	// If we're already recording, allow the change.
	if ( g_pReplayPerformanceController->IsRecording() )
		return true;

	// If we aren't recording and there is no forthcoming data in the playback stream, allow the change.
	if ( !g_pReplayPerformanceController->IsPlaybackDataLeft() )
		return true;

	// Otherwise, record the event string and show a dialog asking the user if they're sure they want to nuke.
	V_strncpy( m_szSuspendedEvent, pEventStr, sizeof( m_szSuspendedEvent ) );
	ShowConfirmDialog( "#Replay_Warning", "#Replay_NukePerformanceChanges", "#GameUI_Confirm", "#GameUI_CancelBold", OnConfirmDestroyChanges, this, this, REPLAY_SOUND_DIALOG_POPUP );

	return false;
}

void CReplayPerformanceEditorPanel::SetButtonTip( wchar_t *pTipText, Panel *pContextPanel )
{
	// Set the text
	m_pButtonTip->SetText( pTipText );
	m_pButtonTip->InvalidateLayout( true, true );

	// Center relative to context panel
	int aPos[2];
	ipanel()->GetAbsPos( pContextPanel->GetVPanel(), aPos[0], aPos[1] );
	const int nX = clamp(
		aPos[0] - m_pButtonTip->GetWide() / 2,
		0,
		ScreenWidth() - m_pButtonTip->GetWide() - (int) XRES( 40 )
	);
	const int nY = m_nBottomPanelStartY - m_pButtonTip->GetTall() - (int) YRES( 2 );
	m_pButtonTip->SetPos( nX, nY );
}

void CReplayPerformanceEditorPanel::ShowButtonTip( bool bShow )
{
	m_pButtonTip->SetVisible( bShow );
}

void CReplayPerformanceEditorPanel::ShowSavingDialog()
{
	Assert( !m_pSavingDlg );
	m_pSavingDlg = new CSavingDialog( ReplayUI_GetPerformanceEditor() );
	ShowWaitingDialog( m_pSavingDlg, "#Replay_Saving", true, false, -1 );
}

void CReplayPerformanceEditorPanel::ShowPanel( bool bShow )
{
	if ( bShow == IsVisible() )
		return;

	if ( bShow )
	{
		// We are now performing.
		m_pRecLightPanel->SetPerforming( true );

		// Disable keyboard input on all panels added to the list
		FOR_EACH_LL( m_lstDisableKeyboardInputPanels, it )
		{
			m_lstDisableKeyboardInputPanels[ it ]->SetKeyBoardInputEnabled( false );
		}

		DisplayPerformanceTip( "#Replay_PerfTip_ExitPerfMode", &replay_perftip_count_exit, MAX_TIP_DISPLAYS );

		// Fire a message the game DLL can intercept (for achievements, etc).
		IGameEvent *event = gameeventmanager->CreateEvent( "entered_performance_mode" );
		if ( event )
		{
			gameeventmanager->FireEventClientSide( event );
		}

		// Play a sound
		surface()->PlaySound( "replay\\enterperformancemode.wav" );
	}
	else
	{
		// Display a tip
		DisplayPerformanceTip( "#Replay_PerfTip_EnterPerfMode", &replay_perftip_count_enter, MAX_TIP_DISPLAYS );

		// Play a sound
		surface()->PlaySound( "replay\\exitperformancemode.wav" );
	}

	// Show mouse cursor
	SetMouseInputEnabled( bShow );
	SetVisible( bShow );
	MakePopup( bShow );

	// Avoid waiting for next OnThink() to hide background images
	m_pRecLightPanel->UpdatePauseState( bShow );
	m_pRecLightPanel->UpdateBackgroundVisibility();

	// Play or pause
	if ( bShow )
	{
		PauseDemo();
	}
	else
	{
		PlayDemo();
	}

	// Keep controller informed about pause state so that it can throw away unimportant events during pause if it's recording.
	g_pReplayPerformanceController->NotifyPauseState( bShow );
}

bool CReplayPerformanceEditorPanel::OnEndOfReplayReached()
{
	if ( m_bShownAtLeastOnce )
	{
		ShowPanel( true );
		DisplayPerformanceTip( "#Replay_PerfTip_EndOfReplayReached" );

		// Don't end demo playback yet.
		return true;
	}

	// Let the demo player end demo playback
	return false;
}

void CReplayPerformanceEditorPanel::AddSetViewEvent()
{
	if ( !g_pReplayManager->GetPlayingReplay() )
		return;

	if ( !g_pReplayPerformanceController )
		return;

	Vector pos;
	QAngle angles;
	float fov;
	ReplayCamera()->GetCachedView( pos, angles, fov );

	SetViewParams_t params;
	params.m_flTime = GetPlaybackTime();
	params.m_flFov = fov;
	params.m_pOrigin = &pos;
	params.m_pAngles = &angles;

	params.m_flAccel = ReplayCamera()->m_flRoamingAccel;
	params.m_flSpeed = ReplayCamera()->m_flRoamingSpeed;
	params.m_flRotationFilter = ReplayCamera()->m_flRoamingRotFilterFactor;

	g_pReplayPerformanceController->AddEvent_Camera_SetView( params );
}

// Input should be in [0,1]
void CReplayPerformanceEditorPanel::AddTimeScaleEvent( float flTimeScale )
{
	if ( !g_pReplayManager->GetPlayingReplay() )
		return;

	if ( !g_pReplayPerformanceController )
		return;

	g_pReplayPerformanceController->AddEvent_TimeScale( GetPlaybackTime(), flTimeScale );
}

void CReplayPerformanceEditorPanel::UpdateCameraButtonImages( bool bForceUnselected/*=false*/ )
{
	CReplayPerformance *pPerformance = GetPerformance();
	for ( int i = 0; i < NCAMS; ++i )
	{
		CFmtStr fmtFile(
			gs_pBaseComponentNames[i],
			gs_pCamNames[i],
			( !bForceUnselected && ( !pPerformance || g_pReplayPerformanceController->IsRecording() ) && i == m_iCameraSelection ) ? "_selected" : ""
		);

		if ( m_pCameraButtons[ i ] )
		{
			m_pCameraButtons[ i ]->SetSubImage( fmtFile.Access() );
		}
	}
}

void CReplayPerformanceEditorPanel::EnsureRecording( bool bShouldSnip )
{
	// Not recording?
	if ( !g_pReplayPerformanceController->IsRecording() )
	{
		// Start recording - snip if needed.
		g_pReplayPerformanceController->StartRecording( GetReplay(), bShouldSnip );
	}
}

void CReplayPerformanceEditorPanel::ToggleMenu()
{
	if ( !m_pMenu )
		return;

	// Show/hide
	const bool bShow = !m_pMenu->IsVisible();
	m_pMenu->SetVisible( bShow );
}

void CReplayPerformanceEditorPanel::SaveAs( const wchar_t *pTitle )
{
	if ( !g_pReplayPerformanceController->SaveAsAsync( pTitle ) )
	{
		DisplaySavedTip( false );
	}

	ShowSavingDialog();
}

/*static*/ void CReplayPerformanceEditorPanel::OnConfirmSaveAs( bool bShouldSave, wchar_t *pTitle, void *pContext )
{
	// NOTE: Assumes that overwriting has already been confirmed by the user.

	if ( !bShouldSave )
		return;

	CReplayPerformanceEditorPanel *pThis = (CReplayPerformanceEditorPanel *)pContext;
	pThis->SaveAs( pTitle );

	surface()->PlaySound( "replay\\saved_take.wav" );
}

void CReplayPerformanceEditorPanel::ShowRewindConfirmMessage()
{
	ShowMessageBox( "#Replay_RewindWarningTitle", "#Replay_RewindWarningMsg", "#GameUI_OK", OnConfirmRewind, NULL, (void *)this );
	surface()->PlaySound( "replay\\replaydialog_warn.wav" );
}

/*static*/ void	CReplayPerformanceEditorPanel::OnConfirmRewind( bool bConfirmed, void *pContext )
{
	if ( bConfirmed )
	{
		if ( pContext )
		{
			CReplayPerformanceEditorPanel *pEditor = (CReplayPerformanceEditorPanel *)pContext;
			pEditor->OnCommand( "goto_back" );
		}
	}
}

void CReplayPerformanceEditorPanel::OnMenuCommand_Save( bool bExitEditorWhenDone/*=false*/ )
{
	// If this is the first time we're saving this performance, do a save-as.
	if ( !g_pReplayPerformanceController->HasSavedPerformance() )
	{
		OnMenuCommand_SaveAs( bExitEditorWhenDone );
		return;
	}

	// Regular save
	if ( !g_pReplayPerformanceController->SaveAsync() )
	{
		DisplaySavedTip( false );
	}

	// Show saving dialog
	ShowSavingDialog();

	// Exit editor?
	if ( bExitEditorWhenDone )
	{
		OnMenuCommand_Exit();
	}
}

void CReplayPerformanceEditorPanel::OnMenuCommand_SaveAs( bool bExitEditorWhenDone/*=false*/ )
{
	ReplayUI_ShowPerformanceSaveDlg( OnConfirmSaveAs, this, GetReplay(), bExitEditorWhenDone );
}

void CReplayPerformanceEditorPanel::DisplaySavedTip( bool bSucceess )
{
	DisplayPerformanceTip( bSucceess ? "#Replay_PerfTip_Saved" : "#Replay_PerfTip_SaveFailed" );
}

void CReplayPerformanceEditorPanel::OnSaveComplete()
{
	DisplaySavedTip( g_pReplayPerformanceController->GetLastSaveStatus() );	

	m_pSavingDlg = NULL;
}

void CReplayPerformanceEditorPanel::HandleUiToggle()
{
	if ( !TFModalStack()->IsEmpty() )
		return;

	PauseDemo();
	Exit_ShowDialogs();
}

void CReplayPerformanceEditorPanel::Exit()
{
	engine->ClientCmd_Unrestricted( "disconnect" );
}

void CReplayPerformanceEditorPanel::Exit_ShowDialogs()
{
	if ( g_pReplayPerformanceController->IsDirty() )
	{
		ShowConfirmDialog( "#Replay_DiscardTitle", "#Replay_DiscardChanges", "#Replay_Discard", "#Replay_Cancel", OnConfirmDiscard, NULL, this, REPLAY_SOUND_DIALOG_POPUP );
	}
	else
	{
		ShowConfirmDialog( "#Replay_ExitEditorTitle", "#Replay_BackToReplays", "#GameUI_Confirm", "#Replay_Cancel", OnConfirmExit, NULL, this, REPLAY_SOUND_DIALOG_POPUP );
	}
}

void CReplayPerformanceEditorPanel::OnMenuCommand_Exit()
{
	Exit_ShowDialogs();
}

void CReplayPerformanceEditorPanel::OnCommand( const char *command )
{
	float flCurTime = GetPlaybackTime();

	g_bIsReplayRewinding = false;

	if ( !V_stricmp( command, "toggle_menu" ) )
	{
		ToggleMenu();
	}
	else if ( !V_strnicmp( command, "menu_", 5 ) )
	{
		const char *pMenuCommand = command + 5;

		if ( !V_stricmp( pMenuCommand, "save" ) )
		{
			OnMenuCommand_Save();
		}
		else if ( !V_stricmp( pMenuCommand, "saveas" ) )
		{
			OnMenuCommand_SaveAs();
		}
		else if ( !V_stricmp( pMenuCommand, "exit" ) )
		{
			OnMenuCommand_Exit();
		}
	}
	else if ( !V_stricmp( command, "close" ) )
	{
		ShowPanel( false );
		MarkForDeletion();
		return;
	}
	else if ( !V_stricmp( command, "play" ) )
	{
		ShowPanel( false );
		return;
	}
	else if ( !V_stricmp( command, "pause" ) )
	{
		ShowPanel( true );
		return;
	}
	else if ( !V_strnicmp( command, "timescale_", 10 ) )
	{
		const char *pTimeScaleCmd = command + 10;
		if ( !V_stricmp( pTimeScaleCmd, "showpanel" ) )
		{
			// If we're playing back, pop up a dialog asking if the user is sure they want to nuke the
			// rest of whatever is playing back.
			if ( !OnStateChangeRequested( command ) )
				return;

			EnsureRecording();
		}
	}
	else if ( !V_strnicmp( command, "settick_", 8 ) )
	{
		const char *pSetType = command + 8;
		const int nCurTick = engine->GetDemoPlaybackTick();

		if ( !V_stricmp( pSetType, "in" ) )
		{
			SetOrRemoveInTick( nCurTick, true );
		}
		else if ( !V_stricmp( pSetType, "out" ) )
		{
			SetOrRemoveOutTick( nCurTick, true );
		}

		// Save the replay
		CReplay *pReplay = GetReplay();
		if ( pReplay )
		{
			g_pReplayManager->FlagReplayForFlush( pReplay, true );
		}

		return;
	}
	else if ( !V_strnicmp( command, "goto_", 5 ) )
	{
		const char *pGotoType = command + 5;
		CReplay *pReplay = GetReplay();
		if ( pReplay )
		{
			const CReplayPerformance *pScratchPerformance = g_pReplayPerformanceController->GetPerformance();
			const CReplayPerformance *pSavedPerformance = g_pReplayPerformanceController->GetSavedPerformance();
			const CReplayPerformance *pPerformance = pScratchPerformance ? pScratchPerformance : pSavedPerformance;

			const int nCurTick = engine->GetDemoPlaybackTick();

			// If in or out ticks are set in the performance, use those for the 'full' rewind/fast-forward
			const int nStartTick = MAX( 0, ( pPerformance && pPerformance->HasInTick() ) ? pPerformance->m_nTickIn : pReplay->m_nSpawnTick );
			const int nEndTick = MAX(	// The MAX() here will keep us from going back in time if we're already past the "end" tick
				nCurTick,
				( ( pPerformance && pPerformance->HasOutTick() ) ?
					pPerformance->m_nTickOut :
					( nStartTick + TIME_TO_TICKS( pReplay->m_flLength ) ) )
				- TIME_TO_TICKS( 0.1f )
			);

			int nGotoTick = 0;
			bool bGoingBack = false;

			if ( !V_stricmp( pGotoType, "start" ) )
			{
				bGoingBack = true;
				nGotoTick = nStartTick;
			}
			else if ( !V_stricmp( pGotoType, "back" ) )
			{
				// If this is the first time rewinding, display a message
				if ( !replay_replayeditor_rewindmsgcounter.GetBool() )
				{
					replay_replayeditor_rewindmsgcounter.SetValue( 1 );
					ShowRewindConfirmMessage();
					return;
				}

				bGoingBack = true;
				nGotoTick = nCurTick - TIME_TO_TICKS( 10.0f );
			}
			else if ( !V_stricmp( pGotoType, "end" ) )
			{
				nGotoTick = nEndTick;	// Don't go back in time
			}

			// Clamp it
			nGotoTick = clamp( nGotoTick, nStartTick, nEndTick );

			// If going back...
			if ( bGoingBack )
			{
				// ...and notify the recorder that we're skipping, which we only need to do if we're going backwards
				g_pReplayPerformanceController->NotifyRewinding();
				g_bIsReplayRewinding = true;
			}

			// Go to the given tick and pause
			CFmtStr fmtCmd( "demo_gototick %i\ndemo_pause\n", nGotoTick );
			engine->ClientCmd_Unrestricted( fmtCmd.Access() );
		}
		return;
	}
	else if ( !V_strnicmp( command, "setcamera_", 10 ) )
	{
		const char *pCamType = command + 10;
		int nEntIndex = ReplayCamera()->GetPrimaryTargetIndex();

		// If we're playing back, pop up a dialog asking if the user is sure they want to nuke the
		// rest of whatever is playing back.
		if ( !OnStateChangeRequested( command ) )
			return;

		EnsureRecording();

		if ( !V_stricmp( pCamType, "first" ) )
		{
			ReplayCamera()->SetMode( OBS_MODE_IN_EYE );
			UpdateCameraSelectionPosition( CAM_FIRST );
			m_bCurrentTargetNeedsVisibilityUpdate = true;
			g_pReplayPerformanceController->AddEvent_Camera_Change_FirstPerson( flCurTime, nEntIndex );
		}
		else if ( !V_stricmp( pCamType, "third" ) )
		{
			ReplayCamera()->SetMode( OBS_MODE_CHASE );
			UpdateCameraSelectionPosition( CAM_THIRD );
			m_bCurrentTargetNeedsVisibilityUpdate = true;
			g_pReplayPerformanceController->AddEvent_Camera_Change_ThirdPerson( flCurTime, nEntIndex );
			AddSetViewEvent();
		}
		else if ( !V_stricmp( pCamType, "free" ) )
		{
			ReplayCamera()->SetMode( OBS_MODE_ROAMING );
			UpdateCameraSelectionPosition( CAM_FREE );
			m_bCurrentTargetNeedsVisibilityUpdate = true;
			g_pReplayPerformanceController->AddEvent_Camera_Change_Free( flCurTime );
			AddSetViewEvent();
			DisplayPerformanceTip( "#Replay_PerfTip_EnterFreeCam", &replay_perftip_count_freecam_enter, MAX_TIP_DISPLAYS );
		}

		return;
	}
	else
	{
		engine->ClientCmd( const_cast<char *>( command ) );
		return;
	}

	BaseClass::OnCommand( command );
}

void CReplayPerformanceEditorPanel::OnConfirmDestroyChanges( bool bConfirmed, void *pContext )
{
	AssertMsg( pContext, "Should have a context!  Fix me!" );
	if ( pContext && bConfirmed )
	{
		CReplayPerformanceEditorPanel *pEditorPanel = (CReplayPerformanceEditorPanel *)pContext;
		if ( bConfirmed )
		{
			CReplay *pReplay = pEditorPanel->GetReplay();
			g_pReplayPerformanceController->StartRecording( pReplay, true );

			// Reissue the command.
			pEditorPanel->OnCommand( pEditorPanel->m_szSuspendedEvent );

			// Play a sound
			surface()->PlaySound( "replay\\snip.wav" );
		}

		// Clear suspended event
		pEditorPanel->m_szSuspendedEvent[ 0 ] = '\0';

		// Make sure mouse is free
		pEditorPanel->SetMouseInputEnabled( true );

		DisplayPerformanceTip( "#Replay_PerfTip_Snip" );
	}
}

/*static*/ void CReplayPerformanceEditorPanel::OnConfirmDiscard( bool bConfirmed, void *pContext )
{
	CReplayPerformanceEditorPanel *pEditor = (CReplayPerformanceEditorPanel *)pContext;
	if ( bConfirmed )
	{
		pEditor->Exit();
	}
	else
	{
		if ( !pEditor->IsVisible() )
		{
			PlayDemo();
		}
	}
}

/*static*/ void CReplayPerformanceEditorPanel::OnConfirmExit( bool bConfirmed, void *pContext )
{
	CReplayPerformanceEditorPanel *pEditor = (CReplayPerformanceEditorPanel *)pContext;
	if ( bConfirmed )
	{
		pEditor->Exit();
	}
	else
	{
		if ( !pEditor->IsVisible() )
		{
			PlayDemo();
		}
	}
}

void CReplayPerformanceEditorPanel::SetOrRemoveInTick( int nTick, bool bRemoveIfSet )
{
	SetOrRemoveTick( nTick, true, bRemoveIfSet );
}

void CReplayPerformanceEditorPanel::SetOrRemoveOutTick( int nTick, bool bRemoveIfSet )
{
	SetOrRemoveTick( nTick, false, bRemoveIfSet );
}

void CReplayPerformanceEditorPanel::SetOrRemoveTick( int nTick, bool bUseInTick, bool bRemoveIfSet )
{
	CReplayPerformance *pPerformance = GetPerformance();
	AssertMsg( pPerformance, "Performance should always be valid by this point." );

	ControlButtons_t iButton;
	int *pResultTick;
	const char *pSetTickKey;
	const char *pUnsetTickKey;
	if ( bUseInTick )
	{
		pResultTick = &pPerformance->m_nTickIn;
		iButton = CTRLBUTTON_IN;
		pSetTickKey = "#Replay_PerfTip_InPointSet";
		pUnsetTickKey = "#Replay_PerfTip_InPointRemoved";
	}
	else
	{
		pResultTick = &pPerformance->m_nTickOut;
		iButton = CTRLBUTTON_OUT;
		pSetTickKey = "#Replay_PerfTip_OutPointSet";
		pUnsetTickKey = "#Replay_PerfTip_OutPointRemoved";
	}

	// Tick explicitly being removed?  Caller passing in -1?
	const bool bRemoving = nTick < 0;

	// If tick already exists and we want to remove, remove it
	bool bSetting;
	if ( ( *pResultTick >= 0 && bRemoveIfSet ) || bRemoving )
	{
		*pResultTick = -1;
		bSetting = false;
	}
	else
	{
		*pResultTick = nTick;
		bSetting = true;
	}

	// Display the appropriate tip
	DisplayPerformanceTip( bSetting ? pSetTickKey : pUnsetTickKey );

	// Select/unselect button
	CExImageButton *pButton = m_pCtrlButtons[ iButton ];
	pButton->SetSelected( bSetting );
	pButton->InvalidateLayout( true, true );	// Without this, buttons don't update immediately

	// Mark the performance as dirty
	g_pReplayPerformanceController->NotifyDirty();
}

CReplay *CReplayPerformanceEditorPanel::GetReplay()
{
	return g_pReplayManager->GetReplay( m_hReplay );
}

void CReplayPerformanceEditorPanel::OnRewindComplete()
{
	// Get rid of any "selected" icon - this will happen as soon as we actually start playing back
	// events, but if we aren't playing back events yet we need to explicitly tell the icons not
	// to display their "selected" versions.
	UpdateCameraButtonImages( true );
}

//-----------------------------------------------------------------------------

static DHANDLE<CReplayPerformanceEditorPanel> g_ReplayPerformanceEditorPanel;

//-----------------------------------------------------------------------------

CReplayPerformanceEditorPanel *ReplayUI_InitPerformanceEditor( ReplayHandle_t hReplay )
{
	if ( !g_ReplayPerformanceEditorPanel.Get() )
	{
		g_ReplayPerformanceEditorPanel = SETUP_PANEL( new CReplayPerformanceEditorPanel( NULL, hReplay ) );
		g_ReplayPerformanceEditorPanel->InvalidateLayout( false, true );
	}

	// Notify recorder of editor
	g_pReplayPerformanceController->SetEditor( g_ReplayPerformanceEditorPanel.Get() );

	return g_ReplayPerformanceEditorPanel;
}

void ReplayUI_ClosePerformanceEditor()
{
	if ( g_ReplayPerformanceEditorPanel )
	{
		g_ReplayPerformanceEditorPanel->MarkForDeletion();
		g_ReplayPerformanceEditorPanel = NULL;
	}
}

CReplayPerformanceEditorPanel *ReplayUI_GetPerformanceEditor()
{
	return g_ReplayPerformanceEditorPanel;
}

#if _DEBUG
CON_COMMAND_F( replay_showperfeditor, "Show performance editor", FCVAR_CLIENTDLL )
{
	ReplayUI_ClosePerformanceEditor();
	ReplayUI_InitPerformanceEditor( REPLAY_HANDLE_INVALID );
}

CON_COMMAND_F( replay_tiptest, "", FCVAR_CLIENTDLL )
{
	DisplayPerformanceTip( "#Replay_PerfTip_EnterFreeCam" );
}
#endif

#endif
