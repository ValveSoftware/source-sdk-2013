//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws achievement progress bars on the HUD
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include <game_controls/baseviewport.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/TextImage.h>
#include "vgui/ILocalize.h"
#include "hud_baseachievement_tracker.h"
#include "iachievementmgr.h"
#include "baseachievement.h"
#include "iclientmode.h"
#include "cdll_int.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "fmtstr.h"
#include "engine/IEngineSound.h"

//=============================================================================
// HPE_BEGIN
// [dwenger] Necessary for HUD Achievement display
//=============================================================================

#include "cdll_client_int.h"
#include "steam/isteamuserstats.h"
#include "steam/steam_api.h"

//=============================================================================
// HPE_END
//=============================================================================

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define PROGRESS_BAR_NEEDS_UPDATE -1
#define UNKNOWN_ACHIEVEMENT_ID -1

void TrackerDescriptionChanged( IConVar *var, const char *pOldString, float flOldValue )
{
	static int s_iTimesChanged = 0;
	if ( s_iTimesChanged > 0 )
	{
		engine->ClientCmd_Unrestricted( "hud_reloadscheme" );
	}
	s_iTimesChanged++;
}
ConVar hud_achievement_description("hud_achievement_description", "1", FCVAR_ARCHIVE, "Show full descriptions of achievements on the HUD", TrackerDescriptionChanged );
#ifdef CSTRIKE_DLL
ConVar hud_achievement_count("hud_achievement_count", "5", FCVAR_ARCHIVE, "Max number of achievements that can be shown on the HUD" );
#else
ConVar hud_achievement_count("hud_achievement_count", "8", FCVAR_ARCHIVE, "Max number of achievements that can be shown on the HUD" );
#endif
ConVar hud_achievement_glowtime("hud_achievement_glowtime", "2.5", FCVAR_NONE, "Duration of glow effect around incremented achievements" );
ConVar hud_achievement_tracker("hud_achievement_tracker", "1", FCVAR_NONE, "Show or hide the achievement tracker" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudBaseAchievementTracker::CHudBaseAchievementTracker( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "HudAchievementTracker" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	for ( int i=0; i < GetMaxAchievementsShown(); i++ )
	{
		CAchievementTrackerItem *pNewItem = CreateAchievementPanel();
		pNewItem->SetAchievement( NULL );
		m_AchievementItem.AddToTail( pNewItem );
	}

	m_flNextThink = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseAchievementTracker::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

void CHudBaseAchievementTracker::LevelInit()
{
	// clear out tracker items and floating numbers on level change
	for ( int i = 0; i < GetChildCount(); i++ )
	{
		GetChild( i )->SetVisible( false );
		GetChild( i )->MarkForDeletion();
	}
	m_AchievementItem.Purge();

	for ( int i=0; i < GetMaxAchievementsShown(); i++ )
	{
		CAchievementTrackerItem *pNewItem = CreateAchievementPanel();
		pNewItem->SetAchievement( NULL );
		m_AchievementItem.AddToTail( pNewItem );
	}


	CHudElement::LevelInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseAchievementTracker::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudBaseAchievementTracker::ShouldDraw()
{
	if ( engine->IsPlayingDemo() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudBaseAchievementTracker::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		UpdateAchievementItems();
		m_flNextThink = gpGlobals->curtime + 0.5f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Max number of achievements shown on the HUD
//-----------------------------------------------------------------------------
int CHudBaseAchievementTracker::GetMaxAchievementsShown()
{
	return hud_achievement_count.GetInt();
}

bool CHudBaseAchievementTracker::ShouldShowAchievement( IAchievement *pAchievement )
{
	return ( hud_achievement_tracker.GetBool() && pAchievement && pAchievement->ShouldShowOnHUD() && !pAchievement->IsAchieved() );
}

CAchievementTrackerItem* CHudBaseAchievementTracker::CreateAchievementPanel()
{
	return new CAchievementTrackerItem( this, "HudAchievementTrackerItem" );
}

//-----------------------------------------------------------------------------
// Purpose: create panels for each achievement the player wants shown on the HUD and assign achievements to each one
//-----------------------------------------------------------------------------
void CHudBaseAchievementTracker::UpdateAchievementItems()
{
	IAchievementMgr *pAchievementMgr = engine->GetAchievementMgr();
	if ( !pAchievementMgr )
		return;

	int iCount = pAchievementMgr->GetAchievementCount();
	int iShown = 0;
	for ( int i = 0; i < iCount; ++i )
	{		
		IAchievement* pCur = pAchievementMgr->GetAchievementByIndex( i );
		if ( !ShouldShowAchievement( pCur ) )
		{
			// don't remove achievements that are still glowing (typically a just completed achievement)
			if ( pCur && m_AchievementItem.Count() > iShown && m_AchievementItem[iShown]->GetAchievementID() == pCur->GetAchievementID() 
					&& m_AchievementItem[iShown]->GetGlow() > 0 )
			{
				iShown++;
			}
			continue;
		}

		if ( m_AchievementItem.Count() < iShown+1 )
		{
			CAchievementTrackerItem *pNewItem = CreateAchievementPanel();
			SETUP_PANEL( pNewItem );
			m_AchievementItem.AddToTail( pNewItem );
		}

		m_AchievementItem[iShown]->SetAchievement( pCur );
		m_AchievementItem[iShown]->SetSlot( iShown );
		m_AchievementItem[iShown]->SetVisible( true );
		iShown++;

		if ( iShown >= GetMaxAchievementsShown() )
			break;
	}

	// hide any extra panels we may have created from when the list was longer
	if ( iShown < m_AchievementItem.Count() )
	{
		for ( int i = m_AchievementItem.Count() - 1; i >= iShown ; i-- )
		{
			m_AchievementItem[i]->SetVisible( false );
			m_AchievementItem[i]->SetAchievement( NULL );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Layout all child panels vertically
//-----------------------------------------------------------------------------
void CHudBaseAchievementTracker::PerformLayout()
{
	// make sure all children are laid out first
	for ( int i=0; i < m_AchievementItem.Count(); i++ )
	{
		m_AchievementItem[i]->InvalidateLayout( true );
	}

	int iCurrentY = 0;
	int x, y;
	for ( int i=0; i< m_AchievementItem.Count(); i++ )
	{
		m_AchievementItem[i]->GetPos( x, y );
		m_AchievementItem[i]->SetPos( x, iCurrentY );
		iCurrentY += m_AchievementItem[i]->GetTall() + m_iItemPadding;
	}
}

CAchievementTrackerItem* CHudBaseAchievementTracker::GetAchievementPanel( int i )
{
	if ( i < 0 || i >= m_AchievementItem.Count() )
		return NULL;

	return m_AchievementItem[i];
}

//-----------------------------------------------------------------------------
// Purpose: The child panels
//-----------------------------------------------------------------------------

CAchievementTrackerItem::CAchievementTrackerItem( vgui::Panel* pParent, const char *pElementName ) :
	BaseClass( pParent, pElementName )
{
	m_pAchievementNameGlow = new vgui::Label( this, "AchievementNameGlow", "" );
	m_pAchievementName = new vgui::Label( this, "AchievementName", "" );
	m_pAchievementDesc = new vgui::Label( this, "AchievementDesc", "" );
	m_pProgressBarBackground = SETUP_PANEL( new ImagePanel( this, "ProgressBarBG" ) );
	m_pProgressBar = SETUP_PANEL( new ImagePanel( this, "ProgressBar" ) );

	m_iAchievementID = UNKNOWN_ACHIEVEMENT_ID;
	m_iLastPaintedAchievementID = UNKNOWN_ACHIEVEMENT_ID;
	m_iAccumulatedIncrement = 0;
	m_flShowIncrementsTime = 0;
	m_iLastCount = 0;
	m_iPadding = 1;
}

CAchievementTrackerItem::~CAchievementTrackerItem()
{
	if ( m_pAchievementName )
	{
		m_pAchievementName->MarkForDeletion();
		m_pAchievementName = NULL;
	}

	if ( m_pAchievementNameGlow )
	{
		m_pAchievementNameGlow->MarkForDeletion();
		m_pAchievementNameGlow = NULL;
	}

	if ( m_pAchievementDesc )
	{
		m_pAchievementDesc->MarkForDeletion();
		m_pAchievementDesc = NULL;
	}

	if ( m_pProgressBarBackground )
	{
		m_pProgressBarBackground->MarkForDeletion();
		m_pProgressBarBackground = NULL;
	}

	if ( m_pProgressBar )
	{
		m_pProgressBar->MarkForDeletion();
		m_pProgressBar = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudAchievementTrackerItem.res" );
	m_pAchievementDesc->SetVisible( hud_achievement_description.GetBool() );
	m_iLastPaintedAchievementID = UNKNOWN_ACHIEVEMENT_ID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y, w, t;

    //=============================================================================
    // HPE_BEGIN
    // [dwenger] Necessary for HUD Achievement display
    //=============================================================================

    m_pAchievementName->GetContentSize( w, t );     // needed in order to load up font for the name
    m_pAchievementNameGlow->GetContentSize( w, t ); // needed in order to load up font for the glow

    //=============================================================================
    // HPE_END
    //=============================================================================

    if ( hud_achievement_description.GetBool() )
	{
		m_pAchievementDesc->GetContentSize( w, t );
		m_pAchievementDesc->SetTall( t );
		m_pAchievementDesc->GetBounds( x, y, w, t );
	}
	else
	{
		m_pAchievementName->GetBounds( x, y, w, t );
	}

	if ( m_pProgressBarBackground->IsVisible() )
	{
		// put progress bar after description
		int bx, by;
		m_pProgressBarBackground->GetPos( bx, by );
		m_pProgressBarBackground->SetPos( bx, y + t + m_iPadding );

		SetTall( y + t + m_pProgressBarBackground->GetTall() + m_iPadding * 2 );
	}
	else
	{
		SetTall( y + t + m_iPadding );
	}

	m_iLastProgressBarCount = m_iLastProgressBarGoal = PROGRESS_BAR_NEEDS_UPDATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::SetAchievement( IAchievement* pAchievement )
{
	if ( !pAchievement )
	{
		m_iAchievementID = UNKNOWN_ACHIEVEMENT_ID;
		m_iLastCount = 0;
		return;

	}
	if ( m_iAchievementID != pAchievement->GetAchievementID() )
	{
		m_iAchievementID = pAchievement->GetAchievementID();
		m_iLastCount = pAchievement->GetCount();
		UpdateAchievementDisplay();
	}
}

void CAchievementTrackerItem::OnThink()
{
	UpdateAchievementDisplay();
}

//-----------------------------------------------------------------------------
// Purpose: Make sure our labels and progress bar are up to date
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::UpdateAchievementDisplay()
{
	IAchievementMgr *pAchievementMgr = engine->GetAchievementMgr();
	if ( !pAchievementMgr )
		return;

	CBaseAchievement* pAchievement = pAchievementMgr->GetAchievementByID( m_iAchievementID );
	if ( !pAchievement )
		return;
	
	if ( m_iAchievementID != m_iLastPaintedAchievementID )
	{
		// need to update labels

        //=============================================================================
        // HPE_BEGIN
        // [dwenger] Necessary for HUD Achievement display
        //=============================================================================

        m_pAchievementName->SetText( ACHIEVEMENT_LOCALIZED_NAME( pAchievement ) );
        m_pAchievementNameGlow->SetText( ACHIEVEMENT_LOCALIZED_NAME( pAchievement ) );
        m_pAchievementDesc->SetText( ACHIEVEMENT_LOCALIZED_DESC( pAchievement ) );

        //=============================================================================
        // HPE_END
        //=============================================================================

		m_pProgressBarBackground->SetVisible( pAchievement->GetGoal() > 1 );
		m_pProgressBar->SetVisible( pAchievement->GetGoal() > 1 );
		
		m_iLastPaintedAchievementID = m_iAchievementID;
		m_flGlow = 0.0f;
		m_flGlowTime = 0.0f;
		m_iAccumulatedIncrement = 0;
		//InvalidateLayout( true );
		GetParent()->InvalidateLayout( true );
	}

	if ( m_iAccumulatedIncrement > 0 && gpGlobals->curtime > m_flShowIncrementsTime )
	{
		ShowAccumulatedIncrements();
	}

	if ( pAchievement->GetCount() != m_iLastProgressBarCount || pAchievement->GetGoal() != m_iLastProgressBarGoal )
	{
		// need to update progress bar
		float flProgress = float ( pAchievement->GetCount() ) / float( pAchievement->GetGoal() );
		int x, y, w, t;
		m_pProgressBarBackground->GetBounds( x, y, w, t );
		m_pProgressBar->SetBounds( x, y, w * flProgress, t );

		m_iLastProgressBarCount = pAchievement->GetCount();
		m_iLastProgressBarGoal = pAchievement->GetGoal();

		AchievementIncremented( pAchievement->GetCount() );
	}

	if ( gpGlobals->curtime < m_flGlowTime )
	{
		m_flGlow = MIN( 1.0f, m_flGlow + gpGlobals->frametime * 5.0f );
	}
	else
	{
		m_flGlow = MAX( 0.0f, m_flGlow - gpGlobals->frametime * 5.0f );
	}
	m_pAchievementNameGlow->SetAlpha( m_flGlow * 255.0f );
}

//-----------------------------------------------------------------------------
// Purpose: Achievement count has gone up, make it flash 
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::AchievementIncremented( int iNewCount )
{
	int iIncrement = iNewCount - m_iLastCount;
	m_iLastCount = iNewCount;

	if ( iIncrement <= 0 )
		return;

	if ( m_iLastProgressBarGoal > 1500 )
	{
		// for achievements with very high counts, accumulate increments so we don't have too many +1s on screen
		//  also don't play sounds as these achievements tend to increment constantly

		if ( m_flShowIncrementsTime < gpGlobals->curtime )
		{
			m_flShowIncrementsTime = gpGlobals->curtime + 2.0f;
		}
		m_iAccumulatedIncrement += iIncrement;
	}
	else
	{
		m_flGlowTime = gpGlobals->curtime + hud_achievement_glowtime.GetFloat();

		// create a floating +X to scroll up alongside this achievement
		if ( m_pProgressBarBackground->IsVisible() )
		{
			int px, py;
			GetPos( px, py );
			int x, y, w, t;
			m_pProgressBarBackground->GetBounds( x, y, w, t );
			x += w;
			new CFloatingAchievementNumber( iIncrement, px + x, py + y + ( t * 0.5f ), FN_DIR_RIGHT, GetParent() );
		}

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.AchievementIncremented" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show all the increments we've accumulated 
//-----------------------------------------------------------------------------
void CAchievementTrackerItem::ShowAccumulatedIncrements()
{
	int px, py;
	GetPos( px, py );
	int x, y, w, t;
	m_pProgressBarBackground->GetBounds( x, y, w, t );
	x += w;
	new CFloatingAchievementNumber( m_iAccumulatedIncrement, px + x, py + y + ( t * 0.5f ), FN_DIR_RIGHT, GetParent() );

	m_iAccumulatedIncrement = 0;
	m_flGlowTime = gpGlobals->curtime + hud_achievement_glowtime.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: Floating numbers showing how much achievement progress bars have gone up
//-----------------------------------------------------------------------------

CFloatingAchievementNumber::CFloatingAchievementNumber( int iProgress, int x, int y, floating_number_directions iDir, vgui::Panel* pParent )
		: BaseClass( pParent, "FloatingAchievementNumber" )
{
	m_iStartX = x;
	m_iStartY = y;
	m_iProgress = iProgress;
	m_fStartTime = gpGlobals->curtime;
	m_iDirection = iDir;

	char szLabel[64];
	Q_snprintf( szLabel, sizeof( szLabel ), "+%d", iProgress );
	m_pNumberLabel = new vgui::Label( this, "FloatingNumberLabel", szLabel );
}

CFloatingAchievementNumber::~CFloatingAchievementNumber()
{
	if ( m_pNumberLabel )
	{
		m_pNumberLabel->MarkForDeletion();
		m_pNumberLabel = NULL;
	}
}

void CFloatingAchievementNumber::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	LoadControlSettings( "resource/UI/HudAchievementFloatingNumber.res" );

	int fontHeight = surface()->GetFontTall( m_pNumberLabel->GetFont() );
	SetPos( m_iStartX, m_iStartY - ( fontHeight * 0.5f ) );
	m_pNumberLabel->SetAlpha( 0 );
	GetAnimationController()->RunAnimationCommand( m_pNumberLabel, "alpha", 255, 0, 0.3, vgui::AnimationController::INTERPOLATOR_LINEAR );

	switch ( m_iDirection )
	{
	default:
	case FN_DIR_UP:
		vgui::GetAnimationController()->RunAnimationCommand( this, "ypos", m_iStartY - m_iScrollDistance, 0, 2.0f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		break;
	case FN_DIR_DOWN:
		vgui::GetAnimationController()->RunAnimationCommand( this, "ypos", m_iStartY + m_iScrollDistance, 0, 2.0f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		break;
	case FN_DIR_LEFT:
		vgui::GetAnimationController()->RunAnimationCommand( this, "xpos", m_iStartX - m_iScrollDistance, 0, 2.0f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		break;
	case FN_DIR_RIGHT:
		vgui::GetAnimationController()->RunAnimationCommand( this, "xpos", m_iStartX + m_iScrollDistance, 0, 2.0f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Delete panel when floating number has faded out
//-----------------------------------------------------------------------------
void CFloatingAchievementNumber::OnThink()
{
	if ( gpGlobals->curtime > m_fStartTime + 1.0f )
	{
		if ( m_pNumberLabel->GetAlpha() >=  255 )
		{
			m_pNumberLabel->SetAlpha( 254 );
			GetAnimationController()->RunAnimationCommand( m_pNumberLabel, "alpha", 0, 0.0, 1.0f, vgui::AnimationController::INTERPOLATOR_LINEAR );
		}
		else if ( m_pNumberLabel->GetAlpha() <= 0 )
		{
			MarkForDeletion();
			SetVisible( false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Debug command to make one of the achievement panels flash as though it just went up
//-----------------------------------------------------------------------------
class CHudAchievementTracker;
void cc_TrackerAnim_f( const CCommand &args )
{
	CHudBaseAchievementTracker *pTracker = ( CHudBaseAchievementTracker * )GET_HUDELEMENT( CHudAchievementTracker );
	if ( !pTracker )
		return;

	CAchievementTrackerItem *pItem = pTracker->GetAchievementPanel( atoi(args[1]) );
	if ( !pItem )
		return;

	pItem->AchievementIncremented( pItem->GetLastCount() + RandomInt(1, 1) );
}

ConCommand cc_TrackerAnim( "TrackerAnim", cc_TrackerAnim_f, "Test animation of the achievement tracker. Parameter is achievement number on HUD to flash", FCVAR_NONE );
