//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_baseplayer.h"
#include "voice_status.h"
#include "clientmode_shared.h"
#include "c_playerresource.h"
#include "voice_common.h"
#include "vgui_avatarimage.h"

ConVar *sv_alltalk = NULL;

//=============================================================================
// Icon for the local player using voice
//=============================================================================
class CHudVoiceSelfStatus : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudVoiceSelfStatus, vgui::Panel );

	CHudVoiceSelfStatus( const char *name );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void VidInit();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	CHudTexture *m_pVoiceIcon;

	Color	m_clrIcon;
};


DECLARE_HUDELEMENT( CHudVoiceSelfStatus );


CHudVoiceSelfStatus::CHudVoiceSelfStatus( const char *pName ) :
	vgui::Panel( NULL, "HudVoiceSelfStatus" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );

	m_pVoiceIcon = NULL;

	SetHiddenBits( 0 );

	m_clrIcon = Color(255,255,255,255);
}

void CHudVoiceSelfStatus::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

#ifdef HL2MP
	SetBgColor( Color( 0, 0, 0, 0 ) );
#endif
}

void CHudVoiceSelfStatus::VidInit( void )
{
	m_pVoiceIcon = gHUD.GetIcon( "voice_self" );
}

bool CHudVoiceSelfStatus::ShouldDraw()
{
	return GetClientVoiceMgr()->IsLocalPlayerSpeaking();
}

void CHudVoiceSelfStatus::Paint()
{
   if( !m_pVoiceIcon )
		return;
	
	int x, y, w, h;
	GetBounds( x, y, w, h );

	m_pVoiceIcon->DrawSelf( 0, 0, w, h, m_clrIcon );
}


//=============================================================================
// Icons for other players using voice
//=============================================================================
class CHudVoiceStatus : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CHudVoiceStatus, vgui::Panel );

	CHudVoiceStatus( const char *name );
	~CHudVoiceStatus( void );

	virtual bool ShouldDraw();	
	virtual void Paint();
	virtual void VidInit();
	virtual void Init();
	virtual void OnThink();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

protected:
	void ClearActiveList();
	int FindActiveSpeaker( int playerId );

private:
	CHudTexture *m_pVoiceIcon;
	int m_iDeadImageID;

	Color	m_clrIcon;

	struct ActiveSpeaker
	{
		int					playerId;
		CAvatarImage*		pAvatar;
		bool				bSpeaking;
		float				fAlpha;
	};

	CUtlLinkedList< ActiveSpeaker > m_SpeakingList;
	// CUtlLinkedList< CAvatarImagePanel* > m_SpeakingListAvatar;

	CPanelAnimationVar( vgui::HFont, m_NameFont, "Default", "Default" );

	CPanelAnimationVarAliasType( float, item_tall, "item_tall", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, item_wide, "item_wide", "160", "proportional_float" );
	CPanelAnimationVarAliasType( float, item_spacing, "item_spacing", "2", "proportional_float" );

	CPanelAnimationVarAliasType( bool, show_avatar, "show_avatar", "0", "bool" );
	CPanelAnimationVarAliasType( bool, show_friend, "show_friend", "1", "bool" );
	CPanelAnimationVarAliasType( float, avatar_ypos, "avatar_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, avatar_xpos, "avatar_xpos", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, avatar_tall, "avatar_tall", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, avatar_wide, "avatar_wide", "16", "proportional_float" );

	CPanelAnimationVarAliasType( bool, show_voice_icon, "show_voice_icon", "1", "bool" );
	CPanelAnimationVarAliasType( float, voice_icon_ypos, "icon_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, voice_icon_xpos, "icon_xpos", "24", "proportional_float" );
	CPanelAnimationVarAliasType( float, voice_icon_tall, "icon_tall", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, voice_icon_wide, "icon_wide", "16", "proportional_float" );

	CPanelAnimationVarAliasType( bool, show_dead_icon, "show_dead_icon", "1", "bool" );
	CPanelAnimationVarAliasType( float, dead_icon_ypos, "dead_ypos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, dead_icon_xpos, "dead_xpos", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, dead_icon_tall, "dead_tall", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, dead_icon_wide, "dead_wide", "16", "proportional_float" );

	CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "40", "proportional_float" );

	CPanelAnimationVarAliasType( float, fade_in_time, "fade_in_time", "0.0", "float" );
	CPanelAnimationVarAliasType( float, fade_out_time, "fade_out_time", "0.0", "float" );
};


DECLARE_HUDELEMENT( CHudVoiceStatus );


CHudVoiceStatus::CHudVoiceStatus( const char *pName ) :
	vgui::Panel( NULL, "HudVoiceStatus" ), CHudElement( pName )
{
	SetParent( g_pClientMode->GetViewport() );

	m_pVoiceIcon = NULL;

	SetHiddenBits( 0 );

	m_clrIcon = Color(255,255,255,255);

	m_iDeadImageID = surface()->DrawGetTextureId( "hud/leaderboard_dead" );
	if ( m_iDeadImageID == -1 ) // we didn't find it, so create a new one
	{
		m_iDeadImageID = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( m_iDeadImageID, "hud/leaderboard_dead", true, false );
	}
}

CHudVoiceStatus::~CHudVoiceStatus()
{
	ClearActiveList();
}

void CHudVoiceStatus::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

#ifdef HL2MP
	SetBgColor( Color( 0, 0, 0, 0 ) );
#endif
}

void CHudVoiceStatus::Init( void )
{
	ClearActiveList();
}

void CHudVoiceStatus::VidInit( void )
{
	m_pVoiceIcon = gHUD.GetIcon( "voice_player" );
}

void CHudVoiceStatus::OnThink( void )
{
	for ( int iPlayerIndex=1; iPlayerIndex<=gpGlobals->maxClients; iPlayerIndex++ )
	{
		int activeSpeakerIndex = FindActiveSpeaker(iPlayerIndex);
		bool bSpeaking = GetClientVoiceMgr()->IsPlayerSpeaking(iPlayerIndex);

		if (activeSpeakerIndex != m_SpeakingList.InvalidIndex() )
		{
			// update their speaking status
			m_SpeakingList[activeSpeakerIndex].bSpeaking = bSpeaking;
		}
		else
		{
			//=============================================================================
			// HPE_BEGIN:
			// [Forrest] Don't use UTIL_PlayerByIndex here.  It may be null for some players when
			// a match starts because the server only passes full player info as it affects
			// the client.
			//=============================================================================
			// if they are talking and not in the list, add them to the end
			if( bSpeaking )
			{
				ActiveSpeaker activeSpeaker;
				activeSpeaker.playerId = iPlayerIndex;
				activeSpeaker.bSpeaking = true;
				activeSpeaker.fAlpha = 0.0f;
				activeSpeaker.pAvatar = NULL;

				//=============================================================================
				// HPE_BEGIN:
				// [pfreese] If a player is now talking set up their avatar
				//=============================================================================

				activeSpeaker.pAvatar = new CAvatarImage();
#ifdef CSTRIKE_DLL
				// [jpaquin] this allows counter strike to display default avatars for bots.  It can't be a virtual function on
				// C_BasePlayer because there would be no way to get a game specific default image if the player is null.
				extern vgui::IImage* GetDefaultAvatarImage( C_BasePlayer *pPlayer );
				activeSpeaker.pAvatar->SetDefaultImage( GetDefaultAvatarImage( UTIL_PlayerByIndex( activeSpeaker.playerId ) ) );
#endif
				activeSpeaker.pAvatar->SetDrawFriend(show_friend);
				player_info_t pi;
				if ( engine->GetPlayerInfo( iPlayerIndex, &pi ) )
				{
					if ( steamapicontext != NULL && steamapicontext->SteamUtils() != NULL )
					{
						CSteamID steamIDForPlayer( pi.friendsID, 1, steamapicontext->SteamUtils()->GetConnectedUniverse(), k_EAccountTypeIndividual );
						activeSpeaker.pAvatar->SetAvatarSteamID(steamIDForPlayer, k_EAvatarSize32x32);
					}
				}

				activeSpeaker.pAvatar->SetAvatarSize( avatar_wide, avatar_tall);

				//=============================================================================
				// HPE_END
				//=============================================================================

				m_SpeakingList.AddToTail(activeSpeaker);
			}
			//=============================================================================
			// HPE_END
			//=============================================================================
		}
	}

	float fTime = gpGlobals->frametime;

	for ( int i = m_SpeakingList.Head(); i != m_SpeakingList.InvalidIndex(); )
	{
		ActiveSpeaker& activeSpeaker = m_SpeakingList[i];

		if (activeSpeaker.bSpeaking)
		{
			if ( fade_in_time > 0.0f )
			{
				activeSpeaker.fAlpha += fTime / fade_in_time;
				if ( activeSpeaker.fAlpha > 1.0f )
					activeSpeaker.fAlpha = 1.0f;
			}
			else
			{
				activeSpeaker.fAlpha = 1.0f;
			}
		}
		else
		{
			if ( fade_out_time > 0.0f )
			{
				activeSpeaker.fAlpha -= fTime / fade_out_time;
			}
			else
			{
				activeSpeaker.fAlpha = 0.0f;
			}

			if ( activeSpeaker.fAlpha <= 0.0f )
			{
				// completely faded, remove them them from the list
				delete activeSpeaker.pAvatar;
				int iNext = m_SpeakingList.Next(i);
				m_SpeakingList.Remove(i);
				i = iNext;
				continue;
			}
		}
		i = m_SpeakingList.Next(i);
	}
}

bool CHudVoiceStatus::ShouldDraw()
{
	if ( IsInFreezeCam() == true )
		return false;

	return true;
}

void CHudVoiceStatus::Paint()
{
   	if( !m_pVoiceIcon )
		return;
	
	int x, y, w, h;
	GetBounds( x, y, w, h );

	// Heights to draw the current voice item at
	int ypos = h - item_tall;

	int length = m_SpeakingList.Count();

	int iFontHeight = 0;

	if( length > 0 )
	{
		surface()->DrawSetTextFont( m_NameFont );
		surface()->DrawSetTextColor( Color(255,255,255,255) );
		iFontHeight = surface()->GetFontTall( m_NameFont );
	}

	if ( !sv_alltalk )
		sv_alltalk = cvar->FindVar( "sv_alltalk" );

	//draw everyone in the list!
	FOR_EACH_LL(m_SpeakingList, i)
	{
		int playerId = m_SpeakingList[i].playerId;
		bool bIsAlive = g_PR->IsAlive( playerId );

		float oldAlphaMultiplier = surface()->DrawGetAlphaMultiplier();
		surface()->DrawSetAlphaMultiplier(oldAlphaMultiplier * m_SpeakingList[i].fAlpha);

		Color c = g_PR->GetTeamColor( g_PR ? g_PR->GetTeam(playerId) : TEAM_UNASSIGNED );

		c[3] = 128;

		const char *pName = g_PR ? g_PR->GetPlayerName(playerId) : "unknown";
		wchar_t szconverted[ 64 ];

		// Add the location, if any
		bool usedLocation = false;
		if ( sv_alltalk && !sv_alltalk->GetBool() )
		{
			C_BasePlayer *pPlayer = UTIL_PlayerByIndex( playerId );
			if ( pPlayer )
			{
				const char *asciiLocation = pPlayer->GetLastKnownPlaceName();
				if ( asciiLocation && *asciiLocation )
				{
					const wchar_t *unicodeLocation = g_pVGuiLocalize->Find( asciiLocation );
					if ( unicodeLocation && *unicodeLocation )
					{
						wchar_t *formatStr = g_pVGuiLocalize->Find( "#Voice_UseLocation" );
						if ( formatStr )
						{
							wchar_t unicodeName[ 64 ];
							g_pVGuiLocalize->ConvertANSIToUnicode( pName, unicodeName, sizeof( unicodeName ) );

							g_pVGuiLocalize->ConstructString( szconverted, sizeof( szconverted ),
								formatStr, 2, unicodeName, unicodeLocation );

							usedLocation = true;
						}
					}
				}
			}
		}

		if ( !usedLocation )
		{
			g_pVGuiLocalize->ConvertANSIToUnicode( pName, szconverted, sizeof(szconverted)  );
		}

		// Draw the item background
		surface()->DrawSetColor( c );
		surface()->DrawFilledRect( 0, ypos, item_wide, ypos + item_tall );

		if ( show_dead_icon && bIsAlive == false && m_iDeadImageID != -1 )
		{
			// draw background for dead icon
			// surface()->DrawFilledRect(dead_icon_xpos, ypos, 0, ypos + dead_icon_tall);

			Vertex_t vert[4];
			float uv1 = 0.0f;
			float uv2 = 1.0f;

			// Draw the dead material
			surface()->DrawSetTexture( m_iDeadImageID );

			vert[0].Init( Vector2D( dead_icon_xpos, ypos + dead_icon_ypos ), Vector2D( uv1, uv1 ) );
			vert[1].Init( Vector2D( dead_icon_xpos + dead_icon_wide, ypos + dead_icon_ypos ), Vector2D( uv2, uv1 ) );
			vert[2].Init( Vector2D( dead_icon_xpos + dead_icon_wide, ypos + dead_icon_ypos + dead_icon_tall ), Vector2D( uv2, uv2 ) );
			vert[3].Init( Vector2D( dead_icon_xpos, ypos + dead_icon_ypos + dead_icon_tall ), Vector2D( uv1, uv2 ) );

			surface()->DrawSetColor(COLOR_WHITE);
			surface()->DrawTexturedPolygon( 4, vert );
		}

		//=============================================================================
		// HPE_BEGIN:
		// [pfreese] Draw the avatar for the given player
		//=============================================================================

		// Draw the players icon
		if (show_avatar && m_SpeakingList[i].pAvatar)
		{
			m_SpeakingList[i].pAvatar->SetPos( avatar_xpos, ypos + avatar_ypos );
			m_SpeakingList[i].pAvatar->Paint();
		}

		//=============================================================================
		// HPE_END
		//=============================================================================

		// Draw the voice icon
		if (show_voice_icon)
			m_pVoiceIcon->DrawSelf( voice_icon_xpos, ypos + voice_icon_ypos, voice_icon_wide, voice_icon_tall, m_clrIcon );

		// Draw the player's name
		surface()->DrawSetTextColor(COLOR_WHITE);
		surface()->DrawSetTextPos( text_xpos, ypos + ( item_tall / 2 ) - ( iFontHeight / 2 ) );

		int iTextSpace = item_wide - text_xpos;

		// write as much of the name as will fit, truncate the rest and add ellipses
		int iNameLength = wcslen(szconverted);
		const wchar_t *pszconverted = szconverted;
		int iTextWidthCounter = 0;
		for( int j=0;j<iNameLength;j++ )
		{
			iTextWidthCounter += surface()->GetCharacterWidth( m_NameFont, pszconverted[j] );

			if( iTextWidthCounter > iTextSpace )
			{	
				if( j > 3 )
				{
					szconverted[j-2] = '.';
					szconverted[j-1] = '.';
					szconverted[j] = '\0';
				}
				break;
			}
		}

		surface()->DrawPrintText( szconverted, wcslen(szconverted) );
			
		ypos -= ( item_spacing + item_tall );

		surface()->DrawSetAlphaMultiplier(oldAlphaMultiplier);
	}
}

int CHudVoiceStatus::FindActiveSpeaker( int playerId )
{
	FOR_EACH_LL(m_SpeakingList, i)
	{
		if (m_SpeakingList[i].playerId == playerId)
			return i;
	}
	return m_SpeakingList.InvalidIndex();
}

void CHudVoiceStatus::ClearActiveList()
{
	FOR_EACH_LL(m_SpeakingList, i)
	{
		delete m_SpeakingList[i].pAvatar;
	}

	m_SpeakingList.RemoveAll();
}