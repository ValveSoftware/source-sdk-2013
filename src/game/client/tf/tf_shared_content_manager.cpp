//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Generic in-game abuse reporting
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "GameEventListener.h"
#include "tf_shared_content_manager.h"
#include "confirm_dialog.h"
#include "clientmode_tf.h"
#include "tf_gamerules.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Declare singleton object
static C_TFSharedContentManager s_SharedContentManager;
C_TFSharedContentManager *TFSharedContentManager() { return &s_SharedContentManager; }

//-----------------------------------------------------------------------------
// Romevision-specific dialog and callback
//-----------------------------------------------------------------------------
extern ConVar tf_romevision_opt_in;
extern ConVar tf_romevision_skip_prompt;

void PromptAcceptRomevisionSharingCallback( bool bConfirm, void *pContext )
{
	if ( bConfirm )
	{
		tf_romevision_opt_in.SetValue( true );
	}
}

static void PromptAcceptRomevisionSharing()
{
	if ( tf_romevision_opt_in.GetBool() == false )
	{
		ShowConfirmOptOutDialog( "#TF_Prompt_Romevision_Title", "#TF_Prompt_Romevision_Message", 
			"#TF_Prompt_Romevsion_OK", "#TF_Prompt_Romevsion_Cancel", 
			"#TF_Prompt_Romevsion_Opt_Out", "tf_romevision_skip_prompt", 
			PromptAcceptRomevisionSharingCallback, NULL );
	}
}

//-----------------------------------------------------------------------------
bool C_TFSharedContentManager::Init()
{
	m_iSharedVisionFlags = TF_VISION_FILTER_NONE;
	m_PlayersWhoHaveOfferedVision.Purge();
	m_SharedVisionQueue.Purge();

	return true;
}

//-----------------------------------------------------------------------------
void C_TFSharedContentManager::Update( float frametime )
{
	// check our shared vision queue
	if ( m_SharedVisionQueue.Count() > 0 ) 
	{
		int iTeam = GetLocalPlayerTeam();
		bool bPrompt = false;

		if ( iTeam == TEAM_SPECTATOR )
		{
			bPrompt = true;
		}
		else if ( iTeam > LAST_SHARED_TEAM )
		{
			C_TFPlayer *pTFLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pTFLocalPlayer && pTFLocalPlayer->GetPlayerClass() && ( pTFLocalPlayer->GetPlayerClass()->GetClassIndex() > TF_CLASS_UNDEFINED ) )
			{
				bPrompt = true;
			}
		}
	
		if ( bPrompt )
		{
			OfferSharedVision_Internal( m_SharedVisionQueue[0].iFlag, m_SharedVisionQueue[0].unAccountID );
			m_SharedVisionQueue.Remove( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
bool C_TFSharedContentManager::CanOfferVision( int iFlag )
{
	bool bRetVal = false;

	switch ( iFlag )
	{
	case TF_VISION_FILTER_ROME:
		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			bRetVal = true;
		}
		break;
	default: 
		break;
	}

	return bRetVal;
}

//-----------------------------------------------------------------------------
void C_TFSharedContentManager::OfferSharedVision( int iFlag, uint32 unAccountID )
{
	if ( !CanOfferVision( iFlag ) )
		return;

	for ( int i = 0 ; i < m_SharedVisionQueue.Count() ; i++ )
	{
		if ( ( m_SharedVisionQueue[i].iFlag == iFlag ) && ( m_SharedVisionQueue[i].unAccountID == unAccountID ) )
		{
			// we already have this entry in the queue
			return;
		}
	}

	shared_vision_entry_t data = { iFlag, unAccountID };
	m_SharedVisionQueue.AddToTail( data );
}

//-----------------------------------------------------------------------------
void C_TFSharedContentManager::PrintChatText( int iFlag, uint32 unAccountID )
{
	// add some chat text saying who has offered the vision, but only once-per-player to avoid spam
	if ( m_PlayersWhoHaveOfferedVision.Find( unAccountID ) == m_PlayersWhoHaveOfferedVision.InvalidIndex() )
	{
		const char *pszPlayerName = NULL;

		CBasePlayer *pPlayer = GetPlayerByAccountID( unAccountID );
		if ( pPlayer )
		{
			pszPlayerName = pPlayer->GetPlayerName();
		}

		if ( pszPlayerName && pszPlayerName[0] )
		{
			KeyValuesAD pKeyValues( "data" );
			pKeyValues->SetString( "player", pszPlayerName );

			const char *pText = NULL;
			switch ( iFlag )
			{
			case TF_VISION_FILTER_ROME:
				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
				{
					pText = "#TF_Player_OptionalVision"; 
				}
				break;
			default: 
				break;
			}

			if ( pText )
			{
				GetClientModeTFNormal()->PrintTextToChat( pText, pKeyValues );
			}
		}

		m_PlayersWhoHaveOfferedVision.AddToHead( unAccountID );
	}
}

//-----------------------------------------------------------------------------
void C_TFSharedContentManager::OfferSharedVision_Internal( int iFlag, uint32 unAccountID )
{
	if ( !CanOfferVision( iFlag ) )
		return;

	// If we haven't already offered it
	if ( ( m_iSharedVisionFlags & iFlag ) == 0 )
	{
		switch( iFlag )
		{
		case TF_VISION_FILTER_ROME:
			if ( !IsLocalPlayerUsingVisionFilterFlags( TF_VISION_FILTER_ROME ) && ( tf_romevision_skip_prompt.GetBool() == false ) )
			{
				PromptAcceptRomevisionSharing();
			}
			break;
		default:
			break;
		}

		AddSharedVision( iFlag );
	}

	// Display who is offering the shared vision
	PrintChatText( iFlag, unAccountID );
}
