//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "c_baseentity.h"
#include "hud.h"
#include "hudelement.h"
#include "clientmode.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <vgui/IScheme.h>
#include <vgui_controls/AnimationController.h>
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "soundenvelope.h"
#include "convar.h"
#include "hud_closecaption.h"
#include "in_buttons.h"
#ifdef MAPBASE
#include "vgui_controls/Label.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_SPEAKER_NAME	256
#define MAX_COUNT_STRING	64

extern ConVar english;
extern ConVar closecaption;
class C_PointCommentaryNode;

CUtlVector< CHandle<C_PointCommentaryNode> >	g_CommentaryNodes;
bool IsInCommentaryMode( void )
{
	return (g_CommentaryNodes.Count() > 0);
}

static bool g_bTracingVsCommentaryNodes = false;

#ifdef MAPBASE
ConVar commentary_text_force( "commentary_text_force", "0", FCVAR_NONE, "Forces all commentary nodes to use the text type." );
ConVar commentary_text_endtime( "commentary_text_endtime", "120" );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudCommentary : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudCommentary, vgui::Panel );
public:
	CHudCommentary( const char *name );

	virtual void Init( void );
	virtual void VidInit( void );
	virtual void LevelInit( void ) { g_CommentaryNodes.Purge(); }
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void StartCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime );
#ifdef MAPBASE
	void StartTextCommentary( C_PointCommentaryNode *pNode, const char *pszText, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime );
#endif
	void StopCommentary( void );
	bool IsTheActiveNode( C_PointCommentaryNode *pNode ) { return (pNode == m_hActiveNode); }

	// vgui overrides
	virtual void Paint( void );
	virtual bool ShouldDraw( void );

private:
	CHandle<C_PointCommentaryNode> m_hActiveNode;
	bool	m_bShouldPaint;
	float	m_flStartTime;
	float	m_flEndTime;
	wchar_t	m_szSpeakers[MAX_SPEAKER_NAME];
	wchar_t	m_szCount[MAX_COUNT_STRING];
	CMaterialReference m_matIcon;
	bool	m_bHiding;
#ifdef MAPBASE
	bool	m_bTextCommentary; // NOTE: If any more types are needed, use an enum
	wchar_t	*m_pszText;
	vgui::Label *m_pLabel;
#endif

	// Painting
	CPanelAnimationVarAliasType( int, m_iBarX, "bar_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarY, "bar_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarTall, "bar_height", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iBarWide, "bar_width", "16", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpeakersX, "speaker_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iSpeakersY, "speaker_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCountXFR, "count_xpos_from_right", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iCountY, "count_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconX, "icon_xpos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconY, "icon_ypos", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconWide, "icon_width", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iIconTall, "icon_height", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nIconTextureId, "icon_texture", "vgui/hud/icon_commentary", "textureid" );

#ifdef MAPBASE
	CPanelAnimationVarAliasType( int, m_iTypeAudioX, "type_audio_xpos", "190", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeAudioY, "type_audio_ypos", "350", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeAudioW, "type_audio_wide", "380", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeAudioT, "type_audio_tall", "40", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextX, "type_text_xpos", "180", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextY, "type_text_ypos", "150", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextW, "type_text_wide", "400", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextT, "type_text_tall", "200", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextCountXFR, "type_text_count_xpos_from_right", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTypeTextCountY, "type_text_count_ypos", "184", "proportional_int" );
	CPanelAnimationVar( Color, m_TextBackgroundColor, "type_text_bg", "0 0 0 192" );
	CPanelAnimationVar( Color, m_TextColor, "type_text_fg", "255 230 180 255" );
#endif

	CPanelAnimationVar( bool, m_bUseScriptBGColor, "use_script_bgcolor", "0" );
#ifdef MAPBASE
	CPanelAnimationVar( Color, m_BackgroundColor, "BackgroundColor", "Panel.BgColor" );
#else
	CPanelAnimationVar( Color, m_BackgroundColor, "BackgroundColor", "0 0 0 0" );
#endif
	CPanelAnimationVar( Color, m_BGOverrideColor, "BackgroundOverrideColor", "Panel.BgColor" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PointCommentaryNode : public C_BaseAnimating
{
	DECLARE_CLASS( C_PointCommentaryNode, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();

	virtual void OnPreDataChanged( DataUpdateType_t type );
	virtual void OnDataChanged( DataUpdateType_t type );

	void StartAudioCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );
#ifdef MAPBASE
	void StartTextCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );
#endif

	void OnRestore( void )
	{
		BaseClass::OnRestore();

		if ( m_bActive )
		{
			StopLoopingSounds();
			m_bRestartAfterRestore = true;
		}

		AddAndLockCommentaryHudGroup();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	virtual void SetDormant( bool bDormant )
	{
		if ( !IsDormant() && bDormant )
		{
			RemoveAndUnlockCommentaryHudGroup();
		}

		BaseClass::SetDormant( bDormant );
	}

	//-----------------------------------------------------------------------------
	// Cleanup
	//-----------------------------------------------------------------------------
	void UpdateOnRemove( void )
	{
		RemoveAndUnlockCommentaryHudGroup();

		StopLoopingSounds();
		BaseClass::UpdateOnRemove();
	}

	void	StopLoopingSounds( void );

	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	void AddAndLockCommentaryHudGroup( void )
	{
		if ( !g_CommentaryNodes.Count() )
		{
			int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "commentary" );
			gHUD.LockRenderGroup( iRenderGroup );
		}

		if ( g_CommentaryNodes.Find(this) == g_CommentaryNodes.InvalidIndex() )
		{
			g_CommentaryNodes.AddToTail( this );
		}
	}

	void RemoveAndUnlockCommentaryHudGroup( void )
	{
		g_CommentaryNodes.FindAndRemove( this );

		if ( !g_CommentaryNodes.Count() )
		{
			int iRenderGroup = gHUD.LookupRenderGroupIndexByName( "commentary" );
			gHUD.UnlockRenderGroup( iRenderGroup );
		}
	}

public:
	// Data received from the server
	bool		m_bActive;
	bool		m_bWasActive;
	float		m_flStartTime;
	char		m_iszCommentaryFile[MAX_PATH];
	char		m_iszCommentaryFileNoHDR[MAX_PATH];
	char		m_iszSpeakers[MAX_SPEAKER_NAME];
	int			m_iNodeNumber;
	int			m_iNodeNumberMax;
	CSoundPatch *m_sndCommentary;
	EHANDLE		m_hViewPosition;
	bool		m_bRestartAfterRestore;
#ifdef MAPBASE
	bool		m_bTextCommentary;
#endif
};

IMPLEMENT_CLIENTCLASS_DT(C_PointCommentaryNode, DT_PointCommentaryNode, CPointCommentaryNode)
	RecvPropBool( RECVINFO( m_bActive ) ),
	RecvPropTime( RECVINFO( m_flStartTime ) ),
	RecvPropString( RECVINFO(m_iszCommentaryFile) ),
	RecvPropString( RECVINFO(m_iszCommentaryFileNoHDR) ),
	RecvPropString( RECVINFO(m_iszSpeakers) ),
	RecvPropInt( RECVINFO( m_iNodeNumber ) ),
	RecvPropInt( RECVINFO( m_iNodeNumberMax ) ),
	RecvPropEHandle( RECVINFO(m_hViewPosition) ),
#ifdef MAPBASE
	RecvPropBool( RECVINFO( m_bTextCommentary ) ),
#endif
END_RECV_TABLE()

BEGIN_DATADESC( C_PointCommentaryNode )
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWasActive, FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_sndCommentary ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bWasActive = m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		AddAndLockCommentaryHudGroup();
	}

	if ( m_bWasActive == m_bActive && !m_bRestartAfterRestore )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( m_bActive && pPlayer )
	{
		// Use the HDR / Non-HDR version based on whether we're running HDR or not
		char *pszCommentaryFile;
		if ( g_pMaterialSystemHardwareConfig->GetHDRType() == HDR_TYPE_NONE && m_iszCommentaryFileNoHDR && m_iszCommentaryFileNoHDR[0] )
		{
			pszCommentaryFile = m_iszCommentaryFileNoHDR;
		}
		else
		{
			pszCommentaryFile = m_iszCommentaryFile;
		}
		if ( !pszCommentaryFile || !pszCommentaryFile[0] )
		{
			engine->ServerCmd( "commentary_finishnode\n" );
			return;
		}

#ifdef MAPBASE
		if (m_bTextCommentary || commentary_text_force.GetBool())
			StartTextCommentary( pszCommentaryFile, pPlayer );
		else
#endif
			StartAudioCommentary( pszCommentaryFile, pPlayer );
	}
	else if ( m_bWasActive )
	{
		StopLoopingSounds();

 		CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
		if ( pHudCommentary->IsTheActiveNode(this) )
		{
			pHudCommentary->StopCommentary();
		}
	}

	m_bRestartAfterRestore = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartAudioCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer )
{
	EmitSound_t es;
	es.m_nChannel = CHAN_STATIC;
	es.m_pSoundName = pszCommentaryFile;
 	es.m_SoundLevel = SNDLVL_GUNFIRE;
	es.m_nFlags = SND_SHOULDPAUSE;

	CBaseEntity *pSoundEntity;
	if ( m_hViewPosition )
	{
		pSoundEntity = m_hViewPosition;
	}
	else if ( render->GetViewEntity() )
	{
		pSoundEntity = cl_entitylist->GetEnt( render->GetViewEntity() );
		es.m_SoundLevel = SNDLVL_NONE;
	}
	else
	{
		pSoundEntity = pPlayer;
	}
	CSingleUserRecipientFilter filter( pPlayer );
	m_sndCommentary = (CSoundEnvelopeController::GetController()).SoundCreate( filter, pSoundEntity->entindex(), es );
	if ( m_sndCommentary )
	{
		(CSoundEnvelopeController::GetController()).SoundSetCloseCaptionDuration( m_sndCommentary, -1 );
		(CSoundEnvelopeController::GetController()).Play( m_sndCommentary, 1.0f, 100, m_flStartTime );
	}

	// Get the duration so we know when it finishes
	float flDuration = enginesound->GetSoundDuration( STRING( CSoundEnvelopeController::GetController().SoundGetName( m_sndCommentary ) ) ) ;

	CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
	if ( pHudCloseCaption )
	{
		// This is where we play the commentary close caption (and lock the other captions out).
		// Also, if close captions are off we force a caption in non-English
		if ( closecaption.GetBool() || ( !closecaption.GetBool() && !english.GetBool() ) )
		{
			// Clear the close caption element in preparation
			pHudCloseCaption->Reset();

			// Process the commentary caption
			pHudCloseCaption->ProcessCaptionDirect( pszCommentaryFile, flDuration );

			// Find the close caption hud element & lock it
			pHudCloseCaption->Lock();
		}
	}

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartCommentary( this, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartTextCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer )
{
	// Get the duration so we know when it finishes
	//float flDuration = enginesound->GetSoundDuration( STRING( CSoundEnvelopeController::GetController().SoundGetName( m_sndCommentary ) ) ) ;

	// TODO: Determine from text length
	float flDuration = commentary_text_endtime.GetFloat();

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartTextCommentary( this, pszCommentaryFile, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Shut down the commentary
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StopLoopingSounds( void )
{
	if ( m_sndCommentary != NULL )
	{
		(CSoundEnvelopeController::GetController()).SoundDestroy( m_sndCommentary );
		m_sndCommentary = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: No client side trace collisions
//-----------------------------------------------------------------------------
bool C_PointCommentaryNode::TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace )
{
	if ( !g_bTracingVsCommentaryNodes )
		return false;

	return BaseClass::TestCollision( ray, mask, trace );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool IsNodeUnderCrosshair( C_BasePlayer *pPlayer )
{
	// See if the player's looking at a commentary node
	trace_t tr;
	Vector vecSrc = pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors( pPlayer->EyeAngles(), &vecForward );

	g_bTracingVsCommentaryNodes = true;
	UTIL_TraceLine( vecSrc, vecSrc + vecForward * MAX_TRACE_LENGTH, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	g_bTracingVsCommentaryNodes = false;

	if ( !tr.m_pEnt )
		return false;

	return dynamic_cast<C_PointCommentaryNode*>(tr.m_pEnt);
}

//===================================================================================================================
// COMMENTARY HUD ELEMENT
//===================================================================================================================
DECLARE_HUDELEMENT( CHudCommentary );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudCommentary::CHudCommentary( const char *name ) : vgui::Panel( NULL, "HudCommentary" ), CHudElement( name )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetPaintBorderEnabled( false );
	SetHiddenBits( HIDEHUD_PLAYERDEAD );

	m_hActiveNode = NULL;
	m_bShouldPaint = true;

#ifdef MAPBASE
	m_pLabel = new vgui::Label( this, "HudCommentaryTextLabel", "" );
#endif
}

void CHudCommentary::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_bUseScriptBGColor )
	{
		SetBgColor( m_BGOverrideColor );
	}

#ifdef MAPBASE
	m_pLabel->SetPaintBackgroundType( 2 );
	m_pLabel->SetSize( 0, GetTall() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::Paint()
{
	float flDuration = (m_flEndTime - m_flStartTime);
	float flPercentage = clamp( ( gpGlobals->curtime - m_flStartTime ) / flDuration, 0.f, 1.f );

	if ( !m_hActiveNode )
	{
		if ( !m_bHiding )
		{
			m_bHiding = true;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HideCommentary" );

			CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
			if ( pHudCloseCaption )
			{
				pHudCloseCaption->Reset();
			}
		}
	}
	else
	{
		// Detect the end of the commentary
		if ( flPercentage >= 1 && m_hActiveNode )
		{
			m_hActiveNode = NULL;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HideCommentary" );

			engine->ServerCmd( "commentary_finishnode\n" );
		}
	}

	if ( !m_bShouldPaint )
		return;

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );

	int xOffset = m_iBarX;
	int yOffset = m_iBarY;

	// Find our fade based on our time shown
	Color clr = Color( 255, 170, 0, GetAlpha() );

	// Get our scheme and font information
	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::HFont hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault" );
	if ( !hFont )
	{
		hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
	}

#ifdef MAPBASE
	if (m_bTextCommentary)
	{
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset + m_iBarWide, m_iTypeTextT - (yOffset + m_iBarTall) );

		m_pLabel->SetFgColor( m_TextColor );
		m_pLabel->SetBounds( xOffset + 4, yOffset + 4, m_iBarWide - 4, m_iTypeTextT - (m_iBarTall + 4) );
		m_pLabel->SetFont( hFont );

		// Draw the speaker names
		/*vgui::surface()->DrawSetTextFont( hFont );
		vgui::surface()->DrawSetTextColor( Color( 255, 200, 100, GetAlpha() ) );
		vgui::surface()->DrawSetTextPos( xOffset+4, yOffset+4 );
		vgui::surface()->DrawPrintText( m_pszText, wcslen( m_pszText ) );*/
	}
	else
#endif
	{
		// Draw the progress bar
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset+m_iBarWide, yOffset+m_iBarTall );
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawFilledRect( xOffset+2, yOffset+2, xOffset+(int)(flPercentage*m_iBarWide)-2, yOffset+m_iBarTall-2 );
	}

	// Draw the speaker names
	vgui::surface()->DrawSetTextFont( hFont );
	vgui::surface()->DrawSetTextColor( clr ); 
	vgui::surface()->DrawSetTextPos( m_iSpeakersX, m_iSpeakersY );
	vgui::surface()->DrawPrintText( m_szSpeakers, wcslen(m_szSpeakers) );

	if ( COMMENTARY_BUTTONS & IN_ATTACK )
	{
		int iY = m_iBarY + m_iBarTall + YRES(4);
		wchar_t wzFinal[512] = L"";

		wchar_t *pszText = g_pVGuiLocalize->Find( "#Commentary_PrimaryAttack" );
		if ( pszText )
		{
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			vgui::surface()->DrawSetTextPos( m_iSpeakersX, iY );
			vgui::surface()->DrawPrintText( wzFinal, wcslen(wzFinal) );
		}

		pszText = g_pVGuiLocalize->Find( "#Commentary_SecondaryAttack" );
		if ( pszText )
		{
			int w, h;
			UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
			vgui::surface()->GetTextSize( hFont, wzFinal, w, h );
			vgui::surface()->DrawSetTextPos( m_iBarX + m_iBarWide - w, iY );
			vgui::surface()->DrawPrintText( wzFinal, wcslen(wzFinal) );
		}
	}

	// Draw the commentary count
	// Determine our text size, and move that far in from the right hand size (plus the offset)
	int iCountWide, iCountTall;
	vgui::surface()->GetTextSize( hFont, m_szCount, iCountWide, iCountTall );

#ifdef MAPBASE
	if (m_bTextCommentary)
		vgui::surface()->DrawSetTextPos( wide - m_iTypeTextCountXFR - iCountWide, m_iTypeTextCountY );
	else
#endif
	vgui::surface()->DrawSetTextPos( wide - m_iCountXFR - iCountWide, m_iCountY );

	vgui::surface()->DrawPrintText( m_szCount, wcslen( m_szCount ) );

	// Draw the icon
 	vgui::surface()->DrawSetColor( Color(255,170,0,GetAlpha()) );
	vgui::surface()->DrawSetTexture(m_nIconTextureId);
	vgui::surface()->DrawTexturedRect( m_iIconX, m_iIconY, m_iIconWide, m_iIconTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudCommentary::ShouldDraw()
{
	return ( m_hActiveNode || GetAlpha() > 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::Init( void )
{ 
	m_matIcon.Init( "vgui/hud/icon_commentary", TEXTURE_GROUP_VGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::VidInit( void )
{ 
	SetAlpha(0);
	StopCommentary();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StartCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime )
{
	if ( (flEndTime - flStartTime) <= 0 )
		return;

	m_hActiveNode = pNode;
	m_flStartTime = flStartTime;
	m_flEndTime = flEndTime;
	m_bHiding = false;
#ifdef MAPBASE
	m_bTextCommentary = false;
#endif
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );

#ifdef MAPBASE
	SetBounds( m_iTypeAudioX, m_iTypeAudioY, m_iTypeAudioW, m_iTypeAudioT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_BackgroundColor );

	m_pLabel->SetPaintEnabled( false );
#endif

	// Don't draw the element itself if closecaptions are on (and captions are always on in non-english mode)
	ConVarRef pCVar( "closecaption" );
	if ( pCVar.IsValid() )
	{
		m_bShouldPaint = ( !pCVar.GetBool() && english.GetBool() );
	}
	else
	{
		m_bShouldPaint = true;
	}
	SetPaintBackgroundEnabled( m_bShouldPaint );

	char sz[MAX_COUNT_STRING];
	Q_snprintf( sz, sizeof(sz), "%d \\ %d", iNode, iNodeMax );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, m_szCount, sizeof(m_szCount) );

	// If the commentary just started, play the commentary fade in.
	if ( fabs(flStartTime - gpGlobals->curtime) < 1.0 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ShowCommentary" );
	}
	else
	{
		// We're reloading a savegame that has an active commentary going in it. Don't fade in.
		SetAlpha( 255 );
	}
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StartTextCommentary( C_PointCommentaryNode *pNode, const char *pszText, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime )
{
	if ( (flEndTime - flStartTime) <= 0 )
		return;

	m_hActiveNode = pNode;
	m_flStartTime = flStartTime;
	m_flEndTime = flEndTime;
	m_bHiding = false;
	m_bTextCommentary = true;
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );

	SetBounds( m_iTypeTextX, m_iTypeTextY, m_iTypeTextW, m_iTypeTextT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_TextBackgroundColor );

	m_pLabel->SetText( pszText );
	m_pLabel->SetWrap( true );
	m_pLabel->SetPaintEnabled( true );
	m_pLabel->SetPaintBackgroundEnabled( false );
	m_pLabel->SetPaintBorderEnabled( false );
	//m_pLabel->SizeToContents();
	m_pLabel->SetContentAlignment( vgui::Label::a_northwest );

	/*
	// Find a localization token first.
	// If one isn't found, use this static buffer.
	static wchar_t szRawTextBuf[512];
	m_pszText = g_pVGuiLocalize->Find( pszText );
	if (!m_pszText)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode( pszText, szRawTextBuf, sizeof( szRawTextBuf ) );
		m_pszText = szRawTextBuf;
	}
	*/

	m_bShouldPaint = true;
	SetPaintBackgroundEnabled( m_bShouldPaint );

	char sz[MAX_COUNT_STRING];
	Q_snprintf( sz, sizeof(sz), "%d \\ %d", iNode, iNodeMax );
	g_pVGuiLocalize->ConvertANSIToUnicode( sz, m_szCount, sizeof(m_szCount) );

	// If the commentary just started, play the commentary fade in.
	if ( fabs(flStartTime - gpGlobals->curtime) < 1.0 )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ShowCommentary" );
	}
	else
	{
		// We're reloading a savegame that has an active commentary going in it. Don't fade in.
		SetAlpha( 255 );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StopCommentary( void )
{
	m_hActiveNode = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CommentaryModeShouldSwallowInput( C_BasePlayer *pPlayer )
{
	if ( !IsInCommentaryMode() )	
		return false;

	if ( pPlayer->m_nButtons & COMMENTARY_BUTTONS )
	{
		// Always steal the secondary attack
		if ( pPlayer->m_nButtons & IN_ATTACK2 )
			return true;

		// See if there's any nodes ahead of us.
		if ( IsNodeUnderCrosshair( pPlayer ) )
			return true;
	}

	return false;
}