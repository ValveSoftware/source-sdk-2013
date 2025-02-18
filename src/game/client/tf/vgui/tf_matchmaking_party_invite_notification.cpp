//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_party_invite_notification.h"
#include "tf_gc_client.h"
#include "tf_partyclient.h"
#include "vgui_avatarimage.h"
#include <vgui_controls/AnimationController.h>
#include "clientmode_tf.h"

using namespace vgui;
using namespace GCSDK;

CInviteNotification::CInviteNotification( CSteamID steamID, CTFParty::EPendingType eType, bool bIncoming )
	: CTFDashboardNotification( CTFDashboardNotification::TYPE_PARTY_INVITE,
								CTFDashboardNotification::LEFT,
								0.f,
								"InviteNotification" )
	, CEconNotification()
	, m_eType( eType )
	, m_bIncoming ( bIncoming )
{
	m_flExpireTime = -1.f;
	m_bCreateMainMenuPanel = false;
	SetSteamID( steamID );

	m_pAvatar = new CAvatarImagePanel( this, "avatar" );
	m_pAvatar->SetShouldDrawFriendIcon( false );

	KeyValuesAD kvConditions( "conditions" );
	if ( m_eType == CTFParty::EPendingType::ePending_JoinRequest )
	{
		kvConditions->AddSubKey( new KeyValues( "if_join" ) );
	}
	if ( m_bIncoming )
	{
		kvConditions->AddSubKey( new KeyValues( "if_incoming" ) );
	}

	LoadControlSettings( "resource/UI/InviteNotification.res", NULL, NULL, kvConditions );
	m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );

	wchar_t wszBuf[ 256 ];
	CUtlString strName = SteamFriends()->GetFriendPersonaName( steamID );
	const char* pszRequestString = NULL;
	if ( m_eType == CTFParty::EPendingType::ePending_JoinRequest )
	{
		if ( bIncoming )
		{
			// Blah has requested to join
			pszRequestString = "#TF_Friends_JoinRequest_Incoming";
		}
		else
		{
			// You've request to join Blah
			pszRequestString = "#TF_Friends_JoinRequest_Outgoing";
		}
	}
	else // Invite Request
	{
		if ( bIncoming )
		{
			// Blah has invited you
			pszRequestString = "#TF_Friends_InviteRequest_Incoming";
		}
		else
		{
			pszRequestString = "#TF_Friends_InviteRequest_Outgoing";
		}
	}

	KeyValues* pKV = new KeyValues( (const char*)NULL, "other", CStrAutoEncode( strName ).ToWString() );
	g_pVGuiLocalize->ConstructString_safe( wszBuf, pszRequestString, pKV );
	pKV->deleteThis();
	SetDialogVariable( "invite", wszBuf );
	SetControlVisible( "AcceptButton", bIncoming );
	SetDialogVariable( "cancel_text", g_pVGuiLocalize->Find( bIncoming ? "#Notifications_Decline" : "#TF_Quest_TurnIn_No" ) );
	SetText( pszRequestString );
	AddStringToken( "other", CStrAutoEncode( strName ).ToWString() );
}

CInviteNotification::~CInviteNotification()
{}

void CInviteNotification::OnExpire()
{
	CEconNotification::MarkForDeletion();
}

float CInviteNotification::GetInGameLifeTime() const
{
	if ( m_eType == CTFParty::EPendingType::ePending_JoinRequest && !m_bIncoming )
	{
		return -1.f;
	}
	else
	{
		return FLT_MAX;
	}
}


void CInviteNotification::OnCommand( const char *command )
{
	if ( FStrEq( "accept", command ) )
	{
		Action( true );
	}
	else if ( FStrEq( "decline", command ) )
	{
		Action( false );
	}
}

void CInviteNotification::Action( bool bConfirmed )
{
	if ( bConfirmed )
	{
		if ( m_eType == CTFParty::EPendingType::ePending_JoinRequest )
		{
			GTFPartyClient()->BInvitePlayerToParty( m_steamID, true );
		}
		else
		{
			GTFPartyClient()->BRequestJoinPlayer( m_steamID, true );
		}
	}
	else
	{
		if ( m_eType == CTFParty::EPendingType::ePending_JoinRequest )
		{
			if ( m_bIncoming )
			{
				// Blah has requested to join
				GTFPartyClient()->CancelOutgoingInviteOrIncomingJoinRequest( m_steamID );
			}
			else
			{
				// You've request to join Blah
				GTFPartyClient()->CancelOutgoingJoinRequestOrIncomingInvite( m_steamID );
			}
		}
		else // Invite Request
		{
			if ( m_bIncoming )
			{
				// Blah has invited you
				GTFPartyClient()->CancelOutgoingJoinRequestOrIncomingInvite( m_steamID );
			}
			else
			{
				// You have invited Blah
				GTFPartyClient()->CancelOutgoingInviteOrIncomingJoinRequest( m_steamID );
			}
		}


		auto pAnim = g_pClientMode->GetViewportAnimationController();
		pAnim->RunAnimationCommand( this, "xpos", GetXPos() - YRES( 30 ), 0.f, 0.2f, AnimationController::INTERPOLATOR_LINEAR, 0, true, false );
		SetToExpire( 0.2f );
	}

	CEconNotification::MarkForDeletion();
}

InviteKey_t CInviteNotification::GetKey() const
{
	return InviteKey_t{ m_steamID, m_eType, m_bIncoming };
}
