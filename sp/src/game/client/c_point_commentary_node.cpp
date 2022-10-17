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
#include "vgui_controls/TextImage.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/AnimationController.h"
#include "filesystem.h"
#include "scenefilecache/ISceneFileCache.h"
#include "choreoscene.h"
#include "c_sceneentity.h"
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
ConVar commentary_type_force( "commentary_type_force", "-1", FCVAR_NONE, "Forces all commentary nodes to use the specified type." );
ConVar commentary_type_text_endtime( "commentary_type_text_endtime", "120" );
ConVar commentary_type_image_endtime( "commentary_type_image_endtime", "120" );
ConVar commentary_audio_element_below_cc( "commentary_audio_element_below_cc", "1", FCVAR_NONE, "Allows commentary audio elements to display even when CC is enabled (although this is done by inverting their Y axis)" );
ConVar commentary_audio_element_below_cc_margin( "commentary_audio_element_below_cc_margin", "4" );
ConVar commentary_combine_speaker_and_printname( "commentary_combine_speaker_and_printname", "1" );
ConVar commentary_footnote_offset_x( "commentary_footnote_offset_x", "16" );
ConVar commentary_footnote_offset_y( "commentary_footnote_offset_y", "8" );
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
	void StartImageCommentary( C_PointCommentaryNode *pNode, const char *pszImage, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime );
	void StartSceneCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime );
#endif
	void StopCommentary( void );
	bool IsTheActiveNode( C_PointCommentaryNode *pNode ) { return (pNode == m_hActiveNode); }

#ifdef MAPBASE
	void FixupCommentaryLabels( const char *pszPrintName, const char *pszSpeakers, const char *pszFootnote );
	void RepositionAndFollowCloseCaption( int yOffset = 0 );
#endif

	// vgui overrides
	virtual void Paint( void );
	virtual bool ShouldDraw( void );
#ifdef MAPBASE
	virtual void PerformLayout();
	void ResolveBounds( int width, int height );

	virtual void LevelShutdown();
#endif

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
	int		m_iCommentaryType;
	float	m_flPanelScale;
	float	m_flOverrideX;
	float	m_flOverrideY;

	vgui::Label *m_pLabel;
	vgui::ImagePanel *m_pImage;
	vgui::HFont m_hFont;

	vgui::Label *m_pFootnoteLabel;
	vgui::HFont m_hSmallFont;

	// HACKHACK: Needed as a failsafe to prevent desync
	int		m_iCCDefaultY;
	float	m_flCCAnimTime;

	bool	m_bShouldRepositionSubtitles;
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
	CPanelAnimationVarAliasType( int, m_iTypeTextCountYFB, "type_text_count_ypos_from_bottom", "10", "proportional_int" );
	CPanelAnimationVar( Color, m_TextBackgroundColor, "BackgroundColorTextContent", "0 0 0 224" );
	CPanelAnimationVar( Color, m_TypeTextContentColor, "TextContentColor", "255 230 180 255" );
	CPanelAnimationVar( int, m_iTextBorderSpace, "type_text_border_space", "8" );
#endif

	CPanelAnimationVar( bool, m_bUseScriptBGColor, "use_script_bgcolor", "0" );
#ifdef MAPBASE
	CPanelAnimationVar( Color, m_BackgroundColor, "BackgroundColor", "Panel.BgColor" );
	CPanelAnimationVar( Color, m_ForegroundColor, "ForegroundColor", "255 170 0 255" );
#else
	CPanelAnimationVar( Color, m_BackgroundColor, "BackgroundColor", "0 0 0 0" );
#endif
	CPanelAnimationVar( Color, m_BGOverrideColor, "BackgroundOverrideColor", "Panel.BgColor" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_PointCommentaryNode : public C_BaseAnimating, public IChoreoEventCallback
{
	DECLARE_CLASS( C_PointCommentaryNode, C_BaseAnimating );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_DATADESC();
#ifdef MAPBASE_VSCRIPT
	DECLARE_ENT_SCRIPTDESC();
#endif

	virtual void OnPreDataChanged( DataUpdateType_t type );
	virtual void OnDataChanged( DataUpdateType_t type );

	void StartAudioCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );
#ifdef MAPBASE
	void StartTextCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );
	void StartImageCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );
	void StartSceneCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer );

	// From IChoreoEventCallback
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
#else
	virtual void			StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) {}
#endif
	virtual void			EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) {}
	virtual void			ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) {}
	virtual bool			CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event ) { return true; }

	void ClientThink();

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

#ifdef MAPBASE
			// Special commentary localization file (useful for things like text nodes or print names)
			g_pVGuiLocalize->AddFile( "resource/commentary_%language%.txt", "MOD", true );
#endif
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

#ifdef MAPBASE_VSCRIPT // VScript funcs
	bool IsActive() { return m_bActive; }

	int GetCommentaryType() { return m_iCommentaryType; }
	void SetCommentaryType( int iType ) { m_iCommentaryType = iType; }

	const char *GetCommentaryFile() { return m_iszCommentaryFile; }
	void SetCommentaryFile( const char *pszNewFile ) { Q_strncpy( m_iszCommentaryFile, pszNewFile, sizeof( m_iszCommentaryFile ) ); }
	const char *GetSpeakers() { return m_iszSpeakers; }
	void SetSpeakers( const char *pszSpeakers ) { Q_strncpy( m_iszSpeakers, pszSpeakers, sizeof( m_iszSpeakers ) ); }
	const char *GetPrintName() { return m_iszPrintName; }
	void SetPrintName( const char *pszPrintName ) { Q_strncpy( m_iszPrintName, pszPrintName, sizeof( m_iszPrintName ) ); }
	const char *GetFootnote() { return m_iszFootnote; }
	void SetFootnote( const char *pszFootnote ) { Q_strncpy( m_iszFootnote, pszFootnote, sizeof( m_iszFootnote ) ); }
#endif

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
	char		m_iszPrintName[MAX_SPEAKER_NAME];
	char		m_iszFootnote[MAX_SPEAKER_NAME];
	int		m_iCommentaryType;
	float	m_flPanelScale;
	float	m_flPanelX;
	float	m_flPanelY;

	CChoreoScene	*m_pScene;
	//CHandle<C_SceneEntity>	m_hScene;
	EHANDLE			m_hSceneOrigin;
#endif

#ifdef MAPBASE_VSCRIPT
	static ScriptHook_t	g_Hook_PreStartCommentaryClient;
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
	RecvPropString( RECVINFO( m_iszPrintName ) ),
	RecvPropString( RECVINFO( m_iszFootnote ) ),
	RecvPropInt( RECVINFO( m_iCommentaryType ) ),
	RecvPropFloat( RECVINFO( m_flPanelScale ) ),
	RecvPropFloat( RECVINFO( m_flPanelX ) ),
	RecvPropFloat( RECVINFO( m_flPanelY ) ),
#endif
END_RECV_TABLE()

BEGIN_DATADESC( C_PointCommentaryNode )
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWasActive, FIELD_BOOLEAN ),
	DEFINE_SOUNDPATCH( m_sndCommentary ),
END_DATADESC()

#ifdef MAPBASE_VSCRIPT
ScriptHook_t	C_PointCommentaryNode::g_Hook_PreStartCommentaryClient;

BEGIN_ENT_SCRIPTDESC( C_PointCommentaryNode, C_BaseAnimating, "Commentary nodes which play commentary in commentary mode." )

	DEFINE_SCRIPTFUNC( IsActive, "" )
	DEFINE_SCRIPTFUNC( GetCommentaryFile, "" )
	DEFINE_SCRIPTFUNC( SetCommentaryFile, "" )
	DEFINE_SCRIPTFUNC( GetSpeakers, "" )
	DEFINE_SCRIPTFUNC( SetSpeakers, "" )
	DEFINE_SCRIPTFUNC( GetPrintName, "" )
	DEFINE_SCRIPTFUNC( SetPrintName, "" )
	DEFINE_SCRIPTFUNC( GetFootnote, "" )
	DEFINE_SCRIPTFUNC( SetFootnote, "" )
	DEFINE_SCRIPTFUNC( GetCommentaryType, "" )
	DEFINE_SCRIPTFUNC( SetCommentaryType, "" )

	DEFINE_SIMPLE_SCRIPTHOOK( C_PointCommentaryNode::g_Hook_PreStartCommentaryClient, "PreStartCommentaryClient", FIELD_BOOLEAN, "Called just before commentary begins on the client. Use this to modify variables or commentary behavior before it begins. Returning false will prevent the commentary from starting." )

END_SCRIPTDESC();
#endif


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
#ifdef MAPBASE_VSCRIPT
		if (m_ScriptScope.IsInitialized() && g_Hook_PreStartCommentaryClient.CanRunInScope( m_ScriptScope ))
		{
			ScriptVariant_t functionReturn;
			if (g_Hook_PreStartCommentaryClient.Call( m_ScriptScope, &functionReturn, NULL ) && functionReturn.m_type == FIELD_BOOLEAN)
			{
				// Don't play the commentary if it returned false
				if (functionReturn.m_bool == false)
				{
					engine->ServerCmd( "commentary_finishnode\n" );
					return;
				}
			}
		}
#endif

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
		int iCommentaryType = m_iCommentaryType;
		if (commentary_type_force.GetInt() != -1)
			iCommentaryType = commentary_type_force.GetInt();

		switch (iCommentaryType)
		{
			case COMMENTARY_TYPE_TEXT:
				StartTextCommentary( pszCommentaryFile, pPlayer );
				break;

			case COMMENTARY_TYPE_IMAGE:
				StartImageCommentary( pszCommentaryFile, pPlayer );
				break;

			case COMMENTARY_TYPE_SCENE:
				StartSceneCommentary( pszCommentaryFile, pPlayer );
				break;

			default:
			case COMMENTARY_TYPE_AUDIO:
				StartAudioCommentary( pszCommentaryFile, pPlayer );
				break;
		}
#else
		StartAudioCommentary( pszCommentaryFile, pPlayer );
#endif
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
	bool bSubtitlesEnabled = false;

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

			bSubtitlesEnabled = true;
		}
	}

	char *pszSpeakers = m_iszSpeakers;

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartCommentary( this, pszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartTextCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer )
{
	// Get the duration so we know when it finishes
	//float flDuration = enginesound->GetSoundDuration( STRING( CSoundEnvelopeController::GetController().SoundGetName( m_sndCommentary ) ) ) ;

	// TODO: Determine from text length?
	float flDuration = commentary_type_text_endtime.GetFloat();

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartTextCommentary( this, pszCommentaryFile, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartImageCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer )
{
	// Get the duration so we know when it finishes
	//float flDuration = enginesound->GetSoundDuration( STRING( CSoundEnvelopeController::GetController().SoundGetName( m_sndCommentary ) ) ) ;

	float flDuration = commentary_type_image_endtime.GetFloat();

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartImageCommentary( this, pszCommentaryFile, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );
}

extern CChoreoStringPool g_ChoreoStringPool;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartSceneCommentary( const char *pszCommentaryFile, C_BasePlayer *pPlayer )
{
	char loadfile[MAX_PATH];
	Q_strncpy( loadfile, pszCommentaryFile, sizeof( loadfile ) );
	Q_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	Q_FixSlashes( loadfile );

	// 
	// Raw scene file support
	// 
	void *pBuffer = 0;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize > 0 )
	{
		// Definitely in scenes.image
		pBuffer = malloc( bufsize );
		if ( !scenefilecache->GetSceneData( pszCommentaryFile, (byte *)pBuffer, bufsize ) )
		{
			free( pBuffer );
		}

	
		if ( IsBufferBinaryVCD( (char*)pBuffer, bufsize ) )
		{
			m_pScene = new CChoreoScene( NULL );
			CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
			if ( !m_pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
			{
				Warning( "Unable to restore scene '%s'\n", loadfile );
				delete m_pScene;
				m_pScene = NULL;
			}
		}
	}
	else if (filesystem->ReadFileEx( loadfile, "MOD", &pBuffer, true ))
	{
		// Not in scenes.image, but it's a raw file
		g_TokenProcessor.SetBuffer((char*)pBuffer);
		m_pScene = ChoreoLoadScene( loadfile, this, &g_TokenProcessor, Scene_Printf );
	}

	free( pBuffer );

	if( m_pScene )
	{
		m_pScene->SetPrintFunc( Scene_Printf );
		m_pScene->SetEventCallbackInterface( this );
	}
	else
	{
		// Cancel commentary (TODO: clean up?)
		return;
	}

	int types[ 2 ];
	types[ 0 ] =  CChoreoEvent::SPEAK;
	//types[ 1 ] =  CChoreoEvent::GENERIC; // TODO: Support for the game_text event?
	m_pScene->RemoveEventsExceptTypes( types, 1 );

	// Iterate events and precache necessary resources
	for ( int i = 0; i < m_pScene->GetNumEvents(); i++ )
	{
		CChoreoEvent *event = m_pScene->GetEvent( i );
		if ( !event )
			continue;

		// load any necessary data
		switch (event->GetType() )
		{
		default:
			break;
		case CChoreoEvent::SPEAK:
			{
				// Defined in SoundEmitterSystem.cpp
				// NOTE:  The script entries associated with .vcds are forced to preload to avoid
				//  loading hitches during triggering
				CBaseEntity::PrecacheScriptSound( event->GetParameters() );

				if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER && 
					 event->GetNumSlaves() > 0 )
				{
					char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
					if ( event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) ) )
					{
						CBaseEntity::PrecacheScriptSound( tok );
					}
				}
			}
			break;
		}
	}

	PrecacheScriptSound( "AI_BaseNPC.SentenceStop" );

	if ( m_hViewPosition )
	{
		m_hSceneOrigin = m_hViewPosition;
	}
	else if ( render->GetViewEntity() )
	{
		m_hSceneOrigin = cl_entitylist->GetEnt( render->GetViewEntity() );
	}
	else
	{
		m_hSceneOrigin = pPlayer;
	}

	// Get the duration so we know when it finishes
	float flDuration = m_pScene->GetDuration();

	// Add a tiny amount of time at the end to ensure audio doesn't get cut off
	flDuration += 0.5f;

	CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
	if ( pHudCloseCaption )
	{
		// This is where we play the commentary close caption (and lock the other captions out).
		// Also, if close captions are off we force a caption in non-English
		if ( closecaption.GetBool() || ( !closecaption.GetBool() && !english.GetBool() ) )
		{
			// Clear the close caption element in preparation
			pHudCloseCaption->Reset();

			// Find the close caption hud element & lock it
			pHudCloseCaption->Lock();
		}
	}

	// Tell the HUD element
	CHudCommentary *pHudCommentary = (CHudCommentary *)GET_HUDELEMENT( CHudCommentary );
	pHudCommentary->StartSceneCommentary( this, m_iszSpeakers, m_iNodeNumber, m_iNodeNumberMax, m_flStartTime, m_flStartTime + flDuration );

	// Start thinking for the scene
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: All events are leading edge triggered
// Input  : currenttime - 
//			*event - 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !Q_stricmp( event->GetName(), "NULL" ) )
 	{
 		return;
 	}

	//Msg("Starting event \"%s\" (%s)\n", event->GetName(), event->GetParameters());

	// load any necessary data
	switch (event->GetType() )
	{
	default:
		break;
	case CChoreoEvent::SPEAK:
		{
			CSingleUserRecipientFilter filter( C_BasePlayer::GetLocalPlayer() );

			CSoundParameters soundParams;
			bool bSoundscript = (g_pSoundEmitterSystem->GetParametersForSound( event->GetParameters(), soundParams, GENDER_NONE, false ));
			EmitSound_t es( soundParams );
			if (bSoundscript)
			{
			}
			else
			{
				es.m_pSoundName = event->GetParameters();
				es.m_flVolume = 1;
			}

			// TODO: This is supposed to make sure actors don't interrupt each other, but it doesn't seem to work
			es.m_nChannel = CHAN_USER_BASE + scene->FindActorIndex( event->GetActor() );
			es.m_SoundLevel = SNDLVL_GUNFIRE;
			es.m_nFlags = SND_SHOULDPAUSE;

			es.m_bEmitCloseCaption = false;

			// Just in case
			if (!m_hSceneOrigin)
				m_hSceneOrigin = C_BasePlayer::GetLocalPlayer();

			EmitSound( filter, m_hSceneOrigin->entindex(), es );

			// Close captioning only on master token no matter what...
			// Also, if close captions are off we force a caption in non-English
			if ( event->GetCloseCaptionType() == CChoreoEvent::CC_MASTER && closecaption.GetBool() || ( !closecaption.GetBool() && !english.GetBool() ) )
			{
				char tok[ CChoreoEvent::MAX_CCTOKEN_STRING ];
				bool validtoken = event->GetPlaybackCloseCaptionToken( tok, sizeof( tok ) );
				if ( validtoken )
				{
					CRC32_t tokenCRC;
					CRC32_Init( &tokenCRC );

					char lowercase[ 256 ];
					Q_strncpy( lowercase, tok, sizeof( lowercase ) );
					Q_strlower( lowercase );

					CRC32_ProcessBuffer( &tokenCRC, lowercase, Q_strlen( lowercase ) );
					CRC32_Final( &tokenCRC );

					float endtime = event->GetLastSlaveEndTime();
					float durationShort = event->GetDuration();
					float durationLong = endtime - event->GetStartTime();
					float duration = MAX( durationShort, durationLong );

					CHudCloseCaption *hudCloseCaption = GET_HUDELEMENT( CHudCloseCaption );
					if ( hudCloseCaption )
					{
						hudCloseCaption->ProcessCaptionDirect( lowercase, duration );
					}
				}

			}
		}
		break;
		// TODO: Support for the game_text event?
		/*
	case CChoreoEvent::GENERIC:
		{
			
		}
		break;
		*/
	}

	event->m_flPrevTime = currenttime;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PointCommentaryNode::ClientThink()
{
	BaseClass::ClientThink();

#ifdef MAPBASE
	if (m_iCommentaryType == COMMENTARY_TYPE_SCENE && m_pScene)
	{
		m_pScene->Think( gpGlobals->curtime - m_flStartTime );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
#endif
}

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

#ifdef MAPBASE
	if ( m_pScene )
	{
		// Must do this to terminate audio
		if (m_hSceneOrigin)
		{
			CSingleUserRecipientFilter filter( C_BasePlayer::GetLocalPlayer() );

			for (int i = 0; i < m_pScene->GetNumActors(); i++)
			{
				EmitSound_t es;
				es.m_nChannel = CHAN_USER_BASE + i;
				es.m_pSoundName = "common/null.wav";

				EmitSound( filter, m_hSceneOrigin->entindex(), es );
			}
		}

		delete m_pScene;
		m_pScene = NULL;
	}
#endif
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
	m_pLabel = new vgui::Label( this, "HudCommentaryTextLabel", L"Textual commentary" );
	m_pImage = new vgui::ImagePanel( this, "HudCommentaryImagePanel" );
	m_pImage->SetShouldScaleImage( true );

	m_pFootnoteLabel = new vgui::Label( this, "HudCommentaryFootnoteLabel", L"Commentary footnote" );

	m_iCCDefaultY = 0;
	m_flCCAnimTime = 0.0f;
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

	m_pFootnoteLabel->SetPaintBackgroundType( 2 );
	m_pFootnoteLabel->SetSize( 0, GetTall() );
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

#ifdef MAPBASE
				// Reset close caption element if needed
				if (pHudCloseCaption->IsUsingCommentaryDimensions())
				{
					// Run this animation command instead of setting the position directly
					g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", m_iCCDefaultY, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_ACCEL );

					pHudCloseCaption->SetUsingCommentaryDimensions( false );
				}
#endif
			}
		}
	}
	else
	{
		// Detect the end of the commentary
		if ( flPercentage >= 1 && m_hActiveNode )
		{
#ifdef MAPBASE
			// Ensure that the scene is terminated
			if (m_iCommentaryType == COMMENTARY_TYPE_SCENE)
				m_hActiveNode->StopLoopingSounds();

			// Reset close caption element if needed
			CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
			if (pHudCloseCaption && pHudCloseCaption->IsUsingCommentaryDimensions())
			{
				// Run this animation command instead of setting the position directly
				g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", m_iCCDefaultY, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_ACCEL );

				pHudCloseCaption->SetUsingCommentaryDimensions( false );
			}
#endif

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
	Color clr = m_ForegroundColor;

#ifdef MAPBASE
	switch (m_iCommentaryType)
	{
		case COMMENTARY_TYPE_TEXT:
			{
				// Figure out the size before setting bounds
				int lW, lT;
				m_pLabel->GetContentSize( lW, lT );

				lT += (m_iTextBorderSpace * 2);

				vgui::surface()->DrawSetColor( clr );
				vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset + (m_iBarWide * m_flPanelScale), yOffset + (lT /** m_flPanelScale*/) ); //m_iTypeTextT - (yOffset /*+ m_iBarTall*/) );
			} break;

		case COMMENTARY_TYPE_IMAGE:
			{
				// Figure out the size before setting bounds
				int iW, iT;
				m_pImage->GetSize( iW, iT );
				//vgui::surface()->DrawGetTextureSize( m_pImage->GetImage()->GetID(), iW, iT );

				iW += (m_iTextBorderSpace * 2);
				iT += (m_iTextBorderSpace * 2);

				vgui::surface()->DrawSetColor( clr );
				vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset + iW, yOffset + iT ); //m_iTypeTextT - (yOffset /*+ m_iBarTall*/) );
			} break;

		default:
		case COMMENTARY_TYPE_SCENE:
		case COMMENTARY_TYPE_AUDIO:
			{
				// Draw the progress bar
				vgui::surface()->DrawSetColor( clr );
				vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset+(m_iBarWide*m_flPanelScale), yOffset+m_iBarTall );
				vgui::surface()->DrawSetColor( clr );
				vgui::surface()->DrawFilledRect( xOffset+2, yOffset+2, xOffset+(int)(flPercentage*(m_iBarWide*m_flPanelScale))-2, yOffset+m_iBarTall-2 );
			} break;
	}
#else
	// Draw the progress bar
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawOutlinedRect( xOffset, yOffset, xOffset+m_iBarWide, yOffset+m_iBarTall );
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawFilledRect( xOffset+2, yOffset+2, xOffset+(int)(flPercentage*m_iBarWide)-2, yOffset+m_iBarTall-2 );
#endif

	// Draw the speaker names
	vgui::surface()->DrawSetTextFont( m_hFont );
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
			vgui::surface()->GetTextSize( m_hFont, wzFinal, w, h );
			vgui::surface()->DrawSetTextPos( m_iBarX + m_iBarWide - w, iY );
			vgui::surface()->DrawPrintText( wzFinal, wcslen(wzFinal) );
		}
	}

	// Draw the commentary count
	// Determine our text size, and move that far in from the right hand size (plus the offset)
	int iCountWide, iCountTall;
	vgui::surface()->GetTextSize( m_hFont, m_szCount, iCountWide, iCountTall );

#ifdef MAPBASE
	if (m_pFootnoteLabel->IsEnabled())
	{
		// Raise the count's position so that it doesn't get in the way
		//iCountTall *= 2;

		int x, y;
		m_pFootnoteLabel->GetPos(x, y);

		// 
		// Draw a bullet next to each footnote
		// 
		CUtlVector<int> pLineCoords;
		pLineCoords.AddToTail( 0 ); // First line

		m_pFootnoteLabel->GetTextImage()->GetNewlinePositions( &pLineCoords, true );

		int iBulletX = x - commentary_footnote_offset_x.GetInt();
		int iBulletY = y;

		vgui::surface()->DrawSetTextFont( m_hFont );
		vgui::surface()->DrawSetTextColor( clr );

		for (int i = 0; i < pLineCoords.Count(); i++)
		{
			vgui::surface()->DrawSetTextPos( iBulletX, iBulletY + pLineCoords[i] );
			vgui::surface()->DrawUnicodeChar( L'\u2022' );
		}
	}

	if (m_iCommentaryType != COMMENTARY_TYPE_AUDIO && m_iCommentaryType != COMMENTARY_TYPE_SCENE)
		vgui::surface()->DrawSetTextPos( wide - m_iTypeTextCountXFR - iCountWide, tall - m_iTypeTextCountYFB - iCountTall );
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

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::PerformLayout()
{
	BaseClass::PerformLayout();

	// Don't do anything if we shouldn't draw
	if (!m_hActiveNode) // !ShouldDraw()
		return;

	int extraWidth = 0, extraHeight = 0;

	// The dimensions of a progress bar, text card, etc.
	int contentWidth = 0, contentHeight = 0;

	int xOffset = m_iBarX;
	int yOffset = m_iBarY;

	// Footnotes can add more space to the bottom if they have newlines.
	if (m_pFootnoteLabel->IsEnabled())
	{
		m_pFootnoteLabel->SetBounds( xOffset, yOffset, (float)(m_iBarWide * m_flPanelScale), GetTall() );

		int iNoteWide, iNoteTall;
		m_pFootnoteLabel->GetContentSize( iNoteWide, iNoteTall );

		m_pFootnoteLabel->SetTall( iNoteTall );

		extraHeight += iNoteTall;
	}

	switch (m_iCommentaryType)
	{
		case COMMENTARY_TYPE_TEXT:
			{
				m_pLabel->SetBounds(
					xOffset + m_iTextBorderSpace, yOffset + m_iTextBorderSpace,
					(float)(m_iBarWide * m_flPanelScale) - m_iTextBorderSpace, GetTall() );

				// Figure out the size before setting bounds
				int lW, lT;
				m_pLabel->GetContentSize( lW, lT );

				//lT = (float)lT * m_flPanelScale; // Don't affect height when scaling

				m_pLabel->SetTall( lT );

				lW += (m_iTextBorderSpace * 2);
				lT += (m_iTextBorderSpace * 2);

				contentWidth = lW, contentHeight = lT;

				lW += (xOffset * 2);
				lT += (yOffset * 2);

				ResolveBounds( lW + extraWidth, lT + extraHeight );
			} break;

		case COMMENTARY_TYPE_IMAGE:
			{
				// Figure out the size before setting bounds
				int iW, iT;
				//m_pImage->GetImage()->GetSize( iW, iT );
				vgui::surface()->DrawGetTextureSize( m_pImage->GetImage()->GetID(), iW, iT );
				if (iW <= 0)
					iW = 1;

				int iTargetSize = (m_iBarWide - m_iTextBorderSpace);
				iT *= (iTargetSize / iW);
				iW = iTargetSize;

				iW = (float)iW * m_flPanelScale;
				iT = (float)iT * m_flPanelScale;

				m_pImage->SetBounds(
					xOffset + m_iTextBorderSpace,
					yOffset + m_iTextBorderSpace,
					iW, iT );

				iW += (m_iTextBorderSpace * 2);
				iT += (m_iTextBorderSpace * 2);

				contentWidth = iW, contentHeight = iT;

				iW += (xOffset * 2);
				iT += (yOffset * 2);

				ResolveBounds( iW + extraWidth, iT + extraHeight );
			} break;

		default:
		case COMMENTARY_TYPE_SCENE:
		case COMMENTARY_TYPE_AUDIO:

			// Keep the box centered
			SetBounds( m_iTypeAudioX, m_iTypeAudioY - extraHeight, m_iTypeAudioW + extraWidth, m_iTypeAudioT + extraHeight );
			
			// Reposition the subtitles to be above the commentary dialog
			if (m_bShouldRepositionSubtitles)
			{
				RepositionAndFollowCloseCaption( extraHeight );
			}

			contentWidth = (m_iBarWide * m_flPanelScale), contentHeight = m_iBarTall;

			break;
	}

	// Move the footnote to be at the bottom
	if (m_pFootnoteLabel->IsEnabled())
	{
		m_pFootnoteLabel->SetPos( m_iSpeakersX + commentary_footnote_offset_x.GetInt(), yOffset+contentHeight+ commentary_footnote_offset_y.GetInt() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resolves position on screen; Heavily borrows from CHudMessage::XPosition/YPosition
//-----------------------------------------------------------------------------
void CHudCommentary::ResolveBounds( int width, int height )
{
	int xPos;
	int yPos;

	// ====== X ======
	if ( m_flOverrideX == -1 )
	{
		xPos = (ScreenWidth() - width) * 0.5f;
	}
	else
	{
		if ( m_flOverrideX < 0 )
			xPos = (1.0 + m_flOverrideX) * ScreenWidth() - width;	// Align to right
		else
			xPos = m_flOverrideX * (ScreenWidth() - width);
	}

	// Clamp to edge of screen
	if ( xPos + width > ScreenWidth() )
		xPos = ScreenWidth() - width;
	else if ( xPos < 0 )
		xPos = 0;

	// ====== Y ======
	if ( m_flOverrideY == -1 )
	{
		yPos = (ScreenHeight() - height) * 0.5f;
	}
	else
	{
		if ( m_flOverrideY < 0 )
			yPos = (1.0 + m_flOverrideY) * ScreenHeight() - height;	// Align to bottom
		else
			yPos = m_flOverrideY * (ScreenHeight() - height);
	}

	// Clamp to edge of screen
	if ( yPos + height > ScreenHeight() )
		yPos = ScreenHeight() - height;
	else if ( yPos < 0 )
		yPos = 0;

	SetBounds( xPos, yPos, width, height );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::LevelShutdown( void )
{
	if (m_iCCDefaultY != 0)
	{
		CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
		if (pHudCloseCaption && pHudCloseCaption->IsUsingCommentaryDimensions())
		{
			int ccX, ccY;
			pHudCloseCaption->GetPos( ccX, ccY );

			if (m_iCCDefaultY != ccY)
			{
				DevMsg( "CHudCommentary had to reset misaligned CC element Y (%i) to default Y (%i)\n", ccY, m_iCCDefaultY );
				pHudCloseCaption->SetPos( ccX, m_iCCDefaultY );
			}

			pHudCloseCaption->SetUsingCommentaryDimensions( false );
		}
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::Init( void )
{ 
	m_matIcon.Init( "vgui/hud/icon_commentary", TEXTURE_GROUP_VGUI );

#ifdef MAPBASE
	SetProportional( true );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::VidInit( void )
{ 
	SetAlpha(0);
	StopCommentary();
#ifdef MAPBASE
	m_iCCDefaultY = 0;
#endif
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
	m_iCommentaryType = COMMENTARY_TYPE_AUDIO;
	m_flPanelScale = pNode->m_flPanelScale;
	m_flOverrideX = pNode->m_flPanelX;
	m_flOverrideY = pNode->m_flPanelY;
#endif
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );

#ifdef MAPBASE
	SetBounds( m_iTypeAudioX, m_iTypeAudioY, m_iTypeAudioW, m_iTypeAudioT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_BackgroundColor );

	m_pLabel->SetPaintEnabled( false );
	m_pImage->SetPaintEnabled( false );
	m_pImage->EvictImage();

	m_pFootnoteLabel->SetEnabled( false );

	// Get our scheme and font information
	vgui::HScheme scheme = GetScheme();
	m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault" );
	if ( !m_hFont )
	{
		m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
	}

	m_hSmallFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentarySmall" );
	if ( !m_hSmallFont)
	{
		m_hSmallFont = m_hFont;
	}
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

#ifdef MAPBASE
	if (!m_bShouldPaint && commentary_audio_element_below_cc.GetBool())
	{
		m_bShouldPaint = true;
		m_bShouldRepositionSubtitles = true;

		// Ensure we perform layout later
		InvalidateLayout();
	}
	else
		m_bShouldRepositionSubtitles = false;

	FixupCommentaryLabels( pNode->m_iszPrintName, pNode->m_iszSpeakers, pNode->m_iszFootnote );
#endif

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
	m_iCommentaryType = COMMENTARY_TYPE_TEXT;
	m_flPanelScale = pNode->m_flPanelScale;
	m_flOverrideX = pNode->m_flPanelX;
	m_flOverrideY = pNode->m_flPanelY;
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );

	SetBounds( m_iTypeTextX, m_iTypeTextY, m_iTypeTextW, m_iTypeTextT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_TextBackgroundColor );

	// Get our scheme and font information
	vgui::HScheme scheme = GetScheme();
	m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault" );
	if ( !m_hFont )
	{
		m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
	}

	m_hSmallFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentarySmall" );
	if ( !m_hSmallFont)
	{
		m_hSmallFont = m_hFont;
	}

	m_pLabel->SetText( pszText );
	m_pLabel->SetFont( m_hFont );
	m_pLabel->SetWrap( true );
	m_pLabel->SetPaintEnabled( true );
	m_pLabel->SetPaintBackgroundEnabled( false );
	m_pLabel->SetPaintBorderEnabled( false );
	//m_pLabel->SizeToContents();
	m_pLabel->SetContentAlignment( vgui::Label::a_northwest );
	m_pLabel->SetFgColor( m_TypeTextContentColor );

	m_pImage->SetPaintEnabled( false );
	m_pImage->EvictImage();

	m_pFootnoteLabel->SetEnabled( false );

	m_bShouldPaint = true;

	FixupCommentaryLabels( pNode->m_iszPrintName, pNode->m_iszSpeakers, pNode->m_iszFootnote );

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StartImageCommentary( C_PointCommentaryNode *pNode, const char *pszImage, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime )
{
	if ( (flEndTime - flStartTime) <= 0 )
		return;

	m_hActiveNode = pNode;
	m_flStartTime = flStartTime;
	m_flEndTime = flEndTime;
	m_bHiding = false;
	m_iCommentaryType = COMMENTARY_TYPE_IMAGE;
	m_flPanelScale = pNode->m_flPanelScale;
	m_flOverrideX = pNode->m_flPanelX;
	m_flOverrideY = pNode->m_flPanelY;
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );
	
	SetBounds( m_iTypeTextX, m_iTypeTextY, m_iTypeTextW, m_iTypeTextT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_TextBackgroundColor );

	m_pLabel->SetPaintEnabled( false );

	m_pImage->SetPaintEnabled( true );
	m_pImage->SetImage( pszImage );
	m_pImage->SetWide( m_iBarWide - m_iTextBorderSpace );

	m_pFootnoteLabel->SetEnabled( false );

	// Get our scheme and font information
	vgui::HScheme scheme = GetScheme();
	m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault" );
	if ( !m_hFont )
	{
		m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
	}

	m_hSmallFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentarySmall" );
	if ( !m_hSmallFont)
	{
		m_hSmallFont = m_hFont;
	}

	m_bShouldPaint = true;

	FixupCommentaryLabels( pNode->m_iszPrintName, pNode->m_iszSpeakers, pNode->m_iszFootnote );

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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::StartSceneCommentary( C_PointCommentaryNode *pNode, char *pszSpeakers, int iNode, int iNodeMax, float flStartTime, float flEndTime )
{
	if ( (flEndTime - flStartTime) <= 0 )
		return;

	m_hActiveNode = pNode;
	m_flStartTime = flStartTime;
	m_flEndTime = flEndTime;
	m_bHiding = false;
	m_iCommentaryType = COMMENTARY_TYPE_SCENE;
	m_flPanelScale = pNode->m_flPanelScale;
	m_flOverrideX = pNode->m_flPanelX;
	m_flOverrideY = pNode->m_flPanelY;
	g_pVGuiLocalize->ConvertANSIToUnicode( pszSpeakers, m_szSpeakers, sizeof( m_szSpeakers ) );

	SetBounds( m_iTypeAudioX, m_iTypeAudioY, m_iTypeAudioW, m_iTypeAudioT );
	SetBgColor( m_bUseScriptBGColor ? m_BGOverrideColor : m_BackgroundColor );

	m_pLabel->SetPaintEnabled( false );
	m_pImage->SetPaintEnabled( false );
	m_pImage->EvictImage();

	m_pFootnoteLabel->SetEnabled( false );

	// Get our scheme and font information
	vgui::HScheme scheme = GetScheme();
	m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentaryDefault" );
	if ( !m_hFont )
	{
		m_hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "Default" );
	}

	m_hSmallFont = vgui::scheme()->GetIScheme(scheme)->GetFont( "CommentarySmall" );
	if ( !m_hSmallFont)
	{
		m_hSmallFont = m_hFont;
	}

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

	if (!m_bShouldPaint && commentary_audio_element_below_cc.GetBool())
	{
		m_bShouldPaint = true;
		m_bShouldRepositionSubtitles = true;

		// Ensure we perform layout later
		InvalidateLayout();
	}
	else
		m_bShouldRepositionSubtitles = false;

	FixupCommentaryLabels( pNode->m_iszPrintName, pNode->m_iszSpeakers, pNode->m_iszFootnote );

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

#ifdef MAPBASE
	// Reset close caption element if needed
	CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
	if (pHudCloseCaption && pHudCloseCaption->IsUsingCommentaryDimensions())
	{
		// Run this animation command instead of setting the position directly
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", m_iCCDefaultY, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_ACCEL );

		pHudCloseCaption->SetUsingCommentaryDimensions( false );
	}
#endif
}

#ifdef MAPBASE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::FixupCommentaryLabels( const char *pszPrintName, const char *pszSpeakers, const char *pszFootnote )
{
	if (commentary_combine_speaker_and_printname.GetBool() && pszPrintName[0] != '\0')
	{
		wchar_t *pszLocal = g_pVGuiLocalize->Find( pszPrintName );
		if (m_szSpeakers[0] == '\0' || !m_bShouldPaint) // Use m_bShouldPaint as an indicator of whether or not we use subtitles
		{
			if (pszPrintName[0] == '#' && pszLocal)
				wcsncpy( m_szSpeakers, pszLocal, sizeof( m_szSpeakers ) / sizeof( wchar_t ) );
			else
				g_pVGuiLocalize->ConvertANSIToUnicode( pszPrintName, m_szSpeakers, sizeof( m_szSpeakers ) );
		}
		else
		{
			static wchar_t iszSpeakersLocalized[MAX_SPEAKER_NAME] = { 0 };
			static wchar_t iszPrintNameLocalized[MAX_SPEAKER_NAME] = { 0 };
			
			wcsncpy( iszSpeakersLocalized, m_szSpeakers, sizeof( iszSpeakersLocalized ) / sizeof( wchar_t ) );

			if (m_szSpeakers[0] == '#')
			{
				wchar_t *pwszSpeakers = g_pVGuiLocalize->Find( pszSpeakers );
				if (pwszSpeakers)
					wcsncpy( iszSpeakersLocalized, pwszSpeakers, sizeof( iszSpeakersLocalized ) / sizeof( wchar_t ) );
			}

			if (pszPrintName[0] == '#' && pszLocal)
				wcsncpy( iszPrintNameLocalized, pszLocal, sizeof( iszPrintNameLocalized ) / sizeof( wchar_t ) );
			else
				g_pVGuiLocalize->ConvertANSIToUnicode( pszPrintName, iszPrintNameLocalized, sizeof( iszPrintNameLocalized ) );

			V_snwprintf( m_szSpeakers, sizeof( m_szSpeakers ), L"%ls ~ %ls", iszSpeakersLocalized, iszPrintNameLocalized );
		}
	}

	if (pszFootnote[0] != '\0' && m_bShouldPaint)
	{
		m_pFootnoteLabel->SetText( pszFootnote );
		m_pFootnoteLabel->SetFont( m_hSmallFont );
		m_pFootnoteLabel->SetWrap( true );
		m_pFootnoteLabel->SetEnabled( true );
		m_pFootnoteLabel->SetPaintEnabled( true );
		m_pFootnoteLabel->SetPaintBackgroundEnabled( false );
		m_pFootnoteLabel->SetPaintBorderEnabled( false );
		//m_pFootnoteLabel->SizeToContents();
		m_pFootnoteLabel->SetContentAlignment( vgui::Label::a_northwest );
		m_pFootnoteLabel->SetFgColor( m_ForegroundColor );
	}
	else
	{
		m_pFootnoteLabel->SetPaintEnabled( false );
		m_pFootnoteLabel->SetEnabled( false );
	}

	// Reset close caption element if it's still using commentary dimensions
	// (fixes problems with switching from node to node)
	CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
	if (pHudCloseCaption && pHudCloseCaption->IsUsingCommentaryDimensions())
	{
		// Run this animation command instead of setting the position directly
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", m_iCCDefaultY, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_ACCEL );

		pHudCloseCaption->SetUsingCommentaryDimensions( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudCommentary::RepositionAndFollowCloseCaption( int yOffset )
{
	// Invert the Y axis
	//SetPos( m_iTypeAudioX, ScreenHeight() - m_iTypeAudioY );

	// Place underneath the close caption element
	CHudCloseCaption *pHudCloseCaption = (CHudCloseCaption *)GET_HUDELEMENT( CHudCloseCaption );
	if (pHudCloseCaption /*&& !pHudCloseCaption->IsUsingCommentaryDimensions()*/)
	{
		int ccX, ccY;
		pHudCloseCaption->GetPos( ccX, ccY );

		// Save the default position in case we need to do a hard reset
		// (this usually happens when players begin commentary before the CC element's return animation command is finished)
		if (m_iCCDefaultY == 0)
		{
			m_iCCDefaultY = ccY;
		}

		if (!pHudCloseCaption->IsUsingCommentaryDimensions())
		{
			if (m_iCCDefaultY != ccY /*&& !pHudCloseCaption->IsUsingCommentaryDimensions()*/)
			{
				DevMsg( "CHudCommentary had to reset misaligned CC element Y (%i) to default Y (%i)\n", ccY, m_iCCDefaultY );
				ccY = m_iCCDefaultY;
			}

			ccY -= m_iTypeAudioT;

			// Run this animation command instead of setting the position directly
			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", ccY - yOffset, 0.0f, 0.2f, vgui::AnimationController::INTERPOLATOR_DEACCEL );
			//pHudCloseCaption->SetPos( ccX, ccY );
			m_flCCAnimTime = gpGlobals->curtime + 0.2f;

			pHudCloseCaption->SetUsingCommentaryDimensions( true );
		}
		else if (gpGlobals->curtime > m_flCCAnimTime && ccY != m_iCCDefaultY - m_iTypeAudioT - yOffset)
		{
			DevMsg( "CHudCommentary had to correct misaligned CC element offset (%i != %i)\n", m_iCCDefaultY - ccY, yOffset );

			g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( pHudCloseCaption, "YPos", m_iCCDefaultY - m_iTypeAudioT - yOffset, 0.0f, 0.2f, vgui::AnimationController::INTERPOLATOR_DEACCEL );
			m_flCCAnimTime = gpGlobals->curtime + 0.2f;
		}

		SetPos( ccX, ccY + pHudCloseCaption->GetTall() + commentary_audio_element_below_cc_margin.GetInt() );

		m_flPanelScale = (float)pHudCloseCaption->GetWide() / (float)GetWide();
		SetWide( pHudCloseCaption->GetWide() );
	}
}
#endif

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