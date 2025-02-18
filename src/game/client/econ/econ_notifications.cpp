//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include "econ_notifications.h"

#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include "vgui_avatarimage.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/TextImage.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IVGui.h"
#include "rtime.h"
#include "econ_controls.h"
#include "hud_basechat.h"
#include "hud_vote.h"
#include "inputsystem/iinputsystem.h"
#include "iinput.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------

ConVar cl_notifications_show_ingame( "cl_notifications_show_ingame", "1", FCVAR_ARCHIVE, "Whether notifications should show up in-game." );
ConVar cl_notifications_max_num_visible( "cl_notifications_max_num_visible", "3", FCVAR_ARCHIVE, "How many notifications are visible in-game." );
ConVar cl_notifications_move_time( "cl_notifications_move_time", "0.5", FCVAR_ARCHIVE, "How long it takes for a notification to move." );

// notification queue holds all the notifications
class CEconNotificationQueue
{
public:
	CEconNotificationQueue();
	~CEconNotificationQueue();

	int AddNotification( CEconNotification *pNotification );
	void RemoveAllNotifications();
	void RemoveNotification( int iID );
	void RemoveNotification( CEconNotification *pNotification );
	void RemoveNotifications( NotificationFilterFunc func );
	int CountNotifications( NotificationFilterFunc func );
	void VisitNotifications( CEconNotificationVisitor &visitor );
	CEconNotification *GetNotification( int iID );
	CEconNotification *GetNotificationByIndex( int idx );
	void Update();
	bool HasItems() { return m_vecNotifications.Count() != 0; }
	const CUtlVector< CEconNotification *> &GetItems() { return m_vecNotifications; }

private:
	int m_iIDGenerator;
	CUtlVector< CEconNotification *> m_vecNotifications;
};
static CEconNotificationQueue g_notificationQueue;

CEconNotificationQueue::CEconNotificationQueue()
	: m_iIDGenerator(0)
{
}

CEconNotificationQueue::~CEconNotificationQueue()
{
}

int CEconNotificationQueue::AddNotification( CEconNotification *pNotification )
{
	int iID = ++m_iIDGenerator;
	pNotification->m_iID = iID;
	m_vecNotifications.AddToTail( pNotification );
	return iID;
}

void CEconNotificationQueue::RemoveAllNotifications()
{
	m_vecNotifications.PurgeAndDeleteElements();
}

void CEconNotificationQueue::RemoveNotification( int iID )
{
	FOR_EACH_VEC( m_vecNotifications, i )
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		if ( pNotification->GetID() == iID )
		{
			delete pNotification;
			m_vecNotifications.Remove( i );
			return;
		}
	}
}

void CEconNotificationQueue::RemoveNotification( CEconNotification *pNotification )
{
	if ( pNotification )
	{
		RemoveNotification( pNotification->GetID() );
	}
}

void CEconNotificationQueue::RemoveNotifications( NotificationFilterFunc func )
{
	for ( int i = 0; i < m_vecNotifications.Count(); ++i)
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		if ( func( pNotification ) )
		{
			pNotification->MarkForDeletion();
		}
	}
}

int CEconNotificationQueue::CountNotifications( NotificationFilterFunc func )
{
	int nResult = 0;
	for ( int i = 0; i < m_vecNotifications.Count(); ++i)
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		if ( func( pNotification ) )
		{
			++nResult;
		}
	}

	return nResult;
}

void CEconNotificationQueue::VisitNotifications( CEconNotificationVisitor &visitor )
{
	for ( int i = 0; i < m_vecNotifications.Count(); ++i )
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		visitor.Visit( *pNotification );
	}
}

CEconNotification *CEconNotificationQueue::GetNotification( int iID )
{
	FOR_EACH_VEC( m_vecNotifications, i )
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		if ( pNotification->GetID() == iID )
		{
			return pNotification;
		}
	}
	return NULL;
}

CEconNotification *CEconNotificationQueue::GetNotificationByIndex( int idx )
{
	if ( idx < 0 || idx >= m_vecNotifications.Count() )
	{
		Assert( !"Invalid index passed to GetNotificationByIndex" );
		return NULL;
	}
	return m_vecNotifications[idx];
}

void CEconNotificationQueue::Update()
{
	float flNowTime = engine->Time();
	for ( int i = 0; i < m_vecNotifications.Count(); )
	{
		CEconNotification *pNotification = m_vecNotifications[i];
		if ( pNotification->GetIsInUse() == false && pNotification->GetExpireTime() >= 0 && pNotification->GetExpireTime() < flNowTime )
		{
			pNotification->Expired();
			delete pNotification;
			m_vecNotifications.Remove( i );
			continue;
		}
		pNotification->UpdateTick();
		++i;
	}
}

//-----------------------------------------------------------------------------

static void ColorizeText( CEconNotification *pNotification, CExLabel *pControl, const wchar_t* wszText )
{
	static wchar_t wszStrippedText[2048];

	if ( pControl == NULL )
		return;

	pControl->GetTextImage()->ClearColorChangeStream();

	if ( wszText == NULL )
	{
		pControl->SetText( L"" );
		return;
	}

	Color newColor = pControl->GetFgColor();
	int endIdx = 0;
	int insertIdx = 0;
	bool bContinue = true;
	while ( bContinue )
	{
		bool bSetColor = false;
		switch ( wszText[endIdx] )
		{
			case 0:
				bContinue = false;
				break;
			case COLOR_NORMAL:
			case COLOR_USEOLDCOLORS:
				newColor = pControl->GetFgColor();
				bSetColor = true;
				break;
			case COLOR_PLAYERNAME:
				newColor = g_ColorYellow;
				bSetColor = true;
				break;
			case COLOR_LOCATION:
				newColor = g_ColorDarkGreen;
				bSetColor = true;
				break;
			case COLOR_ACHIEVEMENT:
				{
					vgui::IScheme *pSourceScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "SourceScheme" ) ); 
					if ( pSourceScheme )
					{
						newColor = pSourceScheme->GetColor( "SteamLightGreen", pControl->GetBgColor() );
					}
					else
					{
						newColor = pControl->GetFgColor();
					}
					bSetColor = true;
				}
				break;
			case COLOR_CUSTOM:
				newColor = pControl->GetFgColor();
				KeyValues *pKeyValues = pNotification->GetKeyValues();
				if ( pKeyValues )
				{
					KeyValues* pColor = pKeyValues->FindKey( "custom_color" );
					if ( pColor )
					{
						newColor = pColor->GetColor();
					}
				}
				bSetColor = true;
				break;
		}
		if ( bSetColor )
		{
			pControl->GetTextImage()->AddColorChange( newColor, insertIdx );
		}
		else
		{
			wszStrippedText[insertIdx++] = wszText[endIdx];
		}
		++endIdx;
	}
	pControl->SetText( wszStrippedText );
}

// generic "toast" for notifications
class CGenericNotificationToast : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CGenericNotificationToast, vgui::EditablePanel );
public:
	CGenericNotificationToast( vgui::Panel *parent, int iNotificationID, bool bMainMenu );
	virtual ~CGenericNotificationToast();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnThink() OVERRIDE;
protected:

	void UpdateKVs( CEconNotification* pNotification );

	int 				m_iNotificationID;
	vgui::Panel			*m_pAvatarBG;
	CAvatarImagePanel	*m_pAvatar;
	bool				m_bMainMenu;
	int					m_iKVVersion = 0;
};

CGenericNotificationToast::CGenericNotificationToast( vgui::Panel *parent, int iNotificationID, bool bMainMenu )
	: BaseClass( parent, "GenericNotificationToast" )
	, m_iNotificationID( iNotificationID )
	, m_pAvatar( NULL )
	, m_pAvatarBG( NULL )
	, m_bMainMenu( bMainMenu )
{
}

CGenericNotificationToast::~CGenericNotificationToast()
{
}

void CGenericNotificationToast::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	CEconNotification *pNotification = NotificationQueue_Get( m_iNotificationID );
	bool bHighPriority = pNotification && pNotification->BHighPriority();
	KeyValues *pConditions = NULL;

	if ( bHighPriority )
	{
		pConditions = new KeyValues( "conditions" );
		if ( bHighPriority )
		{
			KeyValues *pSubKey = new KeyValues( "if_high_priority" );
			pConditions->AddSubKey( pSubKey );
		}
	}

	if ( m_bMainMenu )
	{
		LoadControlSettings( "Resource/UI/Econ/GenericNotificationToastMainMenu.res", NULL, NULL, pConditions );
	}
	else
	{
		LoadControlSettings( "Resource/UI/Econ/GenericNotificationToast.res", NULL, NULL, pConditions );
	}

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pAvatar = dynamic_cast< CAvatarImagePanel *>( FindChildByName("AvatarImage") );
	m_pAvatarBG = FindChildByName("AvatarBGPanel");

	UpdateKVs( pNotification );
}

void CGenericNotificationToast::PerformLayout()
{
	BaseClass::PerformLayout();

	CSteamID steamID;
	CEconNotification *pNotification = NotificationQueue_Get( m_iNotificationID );
	if ( pNotification )
	{
		steamID = pNotification->GetSteamID();
	}

	int iMinHeight = 0;
	if ( m_pAvatar )
	{
		if ( steamID != CSteamID() )
		{
			m_pAvatar->SetVisible( true );
			m_pAvatar->SetShouldDrawFriendIcon( false );
			m_pAvatar->SetPlayer( steamID, k_EAvatarSize64x64 );
			// make sure there's a minimum height
			// note we use iY to ensure that there's a buffer below too
			int iX, iY, iWidth, iHeight;
			m_pAvatar->GetBounds( iX, iY, iWidth, iHeight );
			iMinHeight = 2 * iY + iHeight;
		}
		else
		{
			m_pAvatar->SetVisible( false );
			m_pAvatar->ClearAvatar();
		}
	}
	if ( m_pAvatarBG )
	{
		m_pAvatarBG->SetVisible( m_pAvatar != NULL && m_pAvatar->IsVisible() );
	}

	const char *pTextLabelName = steamID != CSteamID() ? "AvatarTextLabel" : "TextLabel";
	CExLabel* pText = dynamic_cast< CExLabel *>( FindChildByName( pTextLabelName ) );
	if ( pText )
	{
		pText->SetVisible( true );
		pText->InvalidateLayout( true, false );
		int iWidth, iHeight;
		pText->GetSize( iWidth, iHeight );
		int iContentWidth, iContentHeight;
		pText->GetContentSize( iContentWidth, iContentHeight );
		pText->SetSize( iWidth, iContentHeight );
		int iDelta = iContentHeight - iHeight;
		// resize ourselves to fit
		int iContainerWidth, iContainerHeight;
		GetSize( iContainerWidth, iContainerHeight );
		SetSize( iContainerWidth, MAX( iContainerHeight + iDelta, iMinHeight ) );
	}
}

void CGenericNotificationToast::OnThink()
{
	BaseClass::OnThink();

	CEconNotification *pNotification = NotificationQueue_Get( m_iNotificationID );
	// Keep our KVs in sync with the notification's KVs
	if ( pNotification && m_iKVVersion != pNotification->GetKVVersion() )
	{
		UpdateKVs( pNotification );
		InvalidateLayout( true, false );
	}
}

void CGenericNotificationToast::UpdateKVs( CEconNotification* pNotification )
{
	if ( pNotification )
	{
		if ( pNotification->GetSteamID() == CSteamID() )
		{
			ColorizeText( pNotification, dynamic_cast< CExLabel* >( FindChildByName( "TextLabel" ) ), pNotification->GetText() );
		}
		else
		{
			ColorizeText( pNotification, dynamic_cast< CExLabel* >( FindChildByName( "AvatarTextLabel" ) ), pNotification->GetText() );
		}

		m_iKVVersion = pNotification->GetKVVersion();
	}
}

//-----------------------------------------------------------------------------

class CNotificationToastControl : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CNotificationToastControl, vgui::EditablePanel );	
public:
	CNotificationToastControl( vgui::EditablePanel *pParent, vgui::EditablePanel *pNotificationToast, int iNotificationID, bool bAddControls )
		: BaseClass( pParent, bAddControls ? "NotificationToastControl" : "NotificationToastContainer" )
		, m_pChild( pNotificationToast )
		, m_iNotificationID( iNotificationID )
		, m_bAddControls( bAddControls )
		, m_pTriggerButton( NULL )
		, m_pAcceptButton( NULL )
		, m_pDeclineButton( NULL )
		, m_iOverrideHeight( 0 )
	{
		m_pChild->SetParent( this );
	}

	virtual ~CNotificationToastControl()
	{
	}

	virtual void ApplySchemeSettings( vgui::IScheme *scheme )
	{
		CEconNotification *pNotification = g_notificationQueue.GetNotification( m_iNotificationID );

		// It is not entirely clear why pNotification is allowed to be NULL. Weapon switching
		// with pyro was causing crashes because of pNotification being NULL, and there were
		// previously existing checks, but it's not clear why.
		CEconNotification::EType eNotificationType = pNotification ? pNotification->NotificationType() \
		                                                           : CEconNotification::eType_Basic;

		bool bHighPriority = pNotification && pNotification->BHighPriority();
		bool bCanDelete = false;
		bool bCanAcceptDecline = false;
		bool bCanTrigger = false;
		bool bOneButton = false;

		switch ( eNotificationType )
		{
			case CEconNotification::eType_AcceptDecline:
				bCanAcceptDecline = true;
				break;
			case CEconNotification::eType_Basic:
				bCanDelete = true;
				bOneButton = true;
				break;
			case CEconNotification::eType_MustTrigger:
				bCanTrigger = true;
				bOneButton = true;
				break;
			case CEconNotification::eType_Trigger:
				bCanTrigger = true;
				bCanDelete = true;
				break;
			default:
				Assert( !"Unhandled enum type" );
		}

		KeyValues *pConditions = NULL;

		if ( bOneButton || bHighPriority )
		{
			pConditions = new KeyValues( "conditions" );
			if ( bOneButton )
			{
				KeyValues *pSubKey = new KeyValues( "if_one_button" );
				pConditions->AddSubKey( pSubKey );
			}
			if ( bHighPriority )
			{
				KeyValues *pSubKey = new KeyValues( "if_high_priority" );
				pConditions->AddSubKey( pSubKey );
			}
		}

		if ( m_bAddControls )
		{
			LoadControlSettings( "Resource/UI/Econ/NotificationToastControl.res", NULL, NULL, pConditions );
		}
		else
		{
			LoadControlSettings( "Resource/UI/Econ/NotificationToastContainer.res", NULL, NULL, pConditions );
		}

		if ( pConditions )
		{
			pConditions->deleteThis();
		}

		BaseClass::ApplySchemeSettings( scheme );

		m_pTriggerButton = NULL;

		GetSize( m_iOriginalWidth, m_iOriginalHeight );

		CExButton *pDeleteButton = dynamic_cast< CExButton *>( FindChildByName( "DeleteButton" ) );

		if ( pDeleteButton && bCanDelete )
		{
			pDeleteButton->AddActionSignalTarget( this );
			pDeleteButton->SetVisible ( pNotification != NULL );
		}

		if ( pNotification == NULL )
			return;

		if ( bCanAcceptDecline )
		{
			m_pAcceptButton = dynamic_cast< CExButton * >( FindChildByName( "AcceptButton" ) );
			m_pDeclineButton = dynamic_cast< CExButton * >( FindChildByName( "DeclineButton" ) );
			if ( m_pAcceptButton && m_pDeclineButton )
			{
				m_pAcceptButton->AddActionSignalTarget( this );
				m_pDeclineButton->AddActionSignalTarget( this );
				m_pAcceptButton->SetVisible( true );
				m_pDeclineButton->SetVisible( true );
				int posX, posY;
				m_pAcceptButton->GetPos( posX, posY );
				m_iButtonOffsetY = GetTall() - posY;
			}
		}

		if ( bCanTrigger )
		{
			m_pTriggerButton = dynamic_cast< CExButton *>( FindChildByName( "TriggerButton" ) );
			if ( m_pTriggerButton )
			{
				m_pTriggerButton->AddActionSignalTarget( this );
				m_pTriggerButton->SetVisible( true );
				int posX, posY;
				m_pTriggerButton->GetPos( posX, posY );
				m_iButtonOffsetY = GetTall() - posY;
			}
		}
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		m_pChild->PerformLayout();
		int iWidth, iHeight;
		m_pChild->GetSize( iWidth, iHeight );

		// position control buttons
		if ( iHeight + m_iButtonOffsetY > m_iOriginalHeight )
		{
			if ( m_pAcceptButton && m_pDeclineButton )
			{
				int posX, posY;
				m_pAcceptButton->GetPos( posX, posY );
//				int newPosY = iHeight;
//				iHeight += m_iButtonOffsetY;
//				m_pAcceptButton->SetPos( posX, newPosY );
//				m_pDeclineButton->GetPos( posX, posY );
//				m_pDeclineButton->SetPos( posX, newPosY );
			}
			else if ( m_pTriggerButton )
			{
				int posX, posY;
				m_pTriggerButton->GetPos( posX, posY );
//				posY = iHeight;
//				iHeight += m_iButtonOffsetY;
//				m_pTriggerButton->SetPos( posX, posY );
			}
		}

		// position help label
		CEconNotification *pNotification = g_notificationQueue.GetNotification( m_iNotificationID );
		CExLabel *pHelpLabel = dynamic_cast< CExLabel* >( FindChildByName( "HelpTextLabel" ) );
		if ( pHelpLabel )
		{
			if ( pNotification )
			{
				const wchar_t *pszText = NULL;
				const char *pszTextKey = pNotification->GetUnlocalizedHelpText();
				if ( pszTextKey )
				{
					pszText = g_pVGuiLocalize->Find( pszTextKey );
				}
				if ( pszText )
				{					
					wchar_t wzFinal[512] = L"";
					if ( ::input->IsSteamControllerActive() )
					{
						UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ), GAME_ACTION_SET_FPSCONTROLS );
					}
					else
					{
						UTIL_ReplaceKeyBindings( pszText, 0, wzFinal, sizeof( wzFinal ) );
					}
					ColorizeText( pNotification, pHelpLabel, wzFinal );
				}
			}

			pHelpLabel->InvalidateLayout( true, false );
			int posX, posY;
			pHelpLabel->GetPos( posX, posY );
			int iContentWidth, iContentHeight;
			pHelpLabel->GetContentSize( iContentWidth, iContentHeight );
			int iLabelWidth, iLabelHeight;
			pHelpLabel->GetSize( iLabelWidth, iLabelHeight );
			int iTextInsetX, iTextInsetY;
			pHelpLabel->GetTextInset( &iTextInsetX, &iTextInsetY );
			pHelpLabel->SetSize( iLabelWidth, iContentHeight + iTextInsetY );
			posY = iHeight;
			pHelpLabel->SetPos( posX, posY - iTextInsetY );
			iHeight += iContentHeight + iTextInsetY;
		}

		// resize ourselves to fit the child height wise
		int iContainerHeight = MAX( m_iOriginalHeight, iHeight );
		SetSize( m_iOriginalWidth, m_iOverrideHeight != 0 ? m_iOverrideHeight : iContainerHeight );
	}

	virtual void OnCommand( const char *command )
	{
		CEconNotification *pNotification = g_notificationQueue.GetNotification( m_iNotificationID );
		
		if ( pNotification != NULL )
		{
			if ( !Q_strncmp( command, "delete", ARRAYSIZE( "delete" ) ) )
			{
				pNotification->Deleted();
				g_notificationQueue.RemoveNotification( m_iNotificationID );
				return;
			}
			else if ( !Q_strncmp( command, "trigger", ARRAYSIZE( "trigger" ) ) )
			{
				pNotification->Trigger();
			}
			else if ( !Q_strncmp( command, "accept", ARRAYSIZE( "accept" ) ) )
			{
				pNotification->Accept();
			}
			else if ( !Q_strncmp( command, "decline", ARRAYSIZE( "decline" ) ) )
			{
				pNotification->Decline();
			}
			else
			{
				BaseClass::OnCommand( command );
			}
		}
		else
		{
			BaseClass::OnCommand( command );
		}
	}

	int GetOverrideHeight() const 
	{
		return m_iOverrideHeight; 
	}

	void SetOverrideHeight( int iHeight )
	{
		m_iOverrideHeight = iHeight;
	}

private:
	vgui::EditablePanel* m_pChild;
	vgui::Panel* m_pTriggerButton;
	vgui::Panel* m_pAcceptButton;
	vgui::Panel* m_pDeclineButton;
	int m_iNotificationID;
	int m_iOriginalWidth;
	int m_iOriginalHeight;
	int m_iButtonOffsetY;
	int m_iOverrideHeight;
	bool m_bAddControls;
};

//-----------------------------------------------------------------------------

struct NotificationUIInfo_t
{
	CNotificationToastControl *m_pPanel;
	int m_iStartPosX;
	int m_iStartPosY;
};

// notification queue panel that is a HUD element
// this is the visualization of the notifications while in game
class CNotificationQueuePanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CNotificationQueuePanel, vgui::EditablePanel );
public:
	CNotificationQueuePanel( const char *pElementName ) 
		: CHudElement( pElementName )
		, BaseClass( NULL, "NotificationQueuePanel" )
		, m_mapNotificationPanels( DefLessFunc(int) )
		, m_flInvalidateTime( 0.0f )
		, m_bInvalidated( false )
	{
		vgui::Panel *pParent = g_pClientMode->GetViewport();
		SetParent( pParent );

		SetHiddenBits( HIDEHUD_MISCSTATUS );
	}

	virtual ~CNotificationQueuePanel()
	{
	}

	virtual bool ShouldDraw( void )
	{
		if ( !CHudElement::ShouldDraw() )
		{
			return false;
		}

		if ( engine->IsPlayingDemo() )
		{
			return false;
		}

		if ( cl_notifications_show_ingame.GetInt() == 0 )
		{
			return false;
		}

		CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
		if ( pHudVote && pHudVote->IsVoteUIActive() )
		{
			return false;
		}
		
		return m_mapNotificationPanels.Count() > 0 || g_notificationQueue.HasItems();
	}

	virtual void PerformLayout( void )
	{
		BaseClass::PerformLayout();

		// Get filtered list of only the notifications that show some in-game content
		CUtlVector< CEconNotification *> notifications;
		GetNotifications( notifications );

		const float flMoveTime = cl_notifications_move_time.GetFloat();
		float lerpPercentage = flMoveTime > 0 ? clamp( ( flMoveTime - m_flInvalidateTime ) / flMoveTime, 0.0f, 1.0f ) : 1.0f;
		float flCurrTime = engine->Time();

		// move the notifications around
		const int kMaxVisibleNotifications = cl_notifications_max_num_visible.GetInt();

		int iPosY = MIN( notifications.Count() - 1, kMaxVisibleNotifications - 1 ) * m_iOverlapOffset_Y;
		int iPosX = MIN( notifications.Count() - 1, kMaxVisibleNotifications - 1 ) * m_iOverlapOffset_X;
		int zpos = 100;
		int iPreviousHeight = 0;
		for ( int i = 0; i < notifications.Count(); ++i )
		{
			CEconNotification *pNotification = notifications[i];
			int mapIdx = m_mapNotificationPanels.Find( pNotification->GetID() );
			if ( m_mapNotificationPanels.IsValidIndex( mapIdx ) == false || pNotification->GetInGameLifeTime() < flCurrTime )
			{
				continue;
			}
			NotificationUIInfo_t &info = m_mapNotificationPanels[mapIdx];
			CNotificationToastControl *pPanel = info.m_pPanel;
			if ( pPanel )
			{
				if ( pPanel->IsVisible() == false )
				{
					pPanel->SetVisible( true );
				}
				if ( i == 0 && pPanel->GetOverrideHeight() != 0 )
				{
					pPanel->SetOverrideHeight( 0 );
					pPanel->InvalidateLayout( true, false );
				}
				int iPanelX;
				int iPanelY;
				pPanel->GetPos( iPanelX, iPanelY );
				if ( m_bInvalidated )
				{
					info.m_iStartPosX = iPanelX;
					info.m_iStartPosY = iPanelY;
				}
				int iNewPosX = iPosX;
				int iNewPosY = iPosY;
				iNewPosX = Lerp( lerpPercentage, info.m_iStartPosX, iNewPosX );
				iNewPosY = Lerp( lerpPercentage, info.m_iStartPosY, iNewPosY );
				pPanel->SetPos( iNewPosX, iNewPosY );
				pPanel->SetZPos( --zpos );
				bool bStoppedMoving = iNewPosX == iPanelX && iNewPosY == iPosY;
				// only show panels that are more than we want visible if they are moving
				if ( i > kMaxVisibleNotifications - 1 )
				{
					if ( bStoppedMoving )
					{
						pPanel->SetVisible( false );
					}
					continue;
				}
				// don't poke out underneath if we are visible and stopped moving
				if ( i != 0 && pPanel->GetOverrideHeight() == 0 && bStoppedMoving )
				{
					pPanel->SetOverrideHeight( MIN( pPanel->GetTall(), iPreviousHeight ) );
					pPanel->InvalidateLayout( true, false );
				}
				iPreviousHeight = pPanel->GetTall();

				iPosY = MAX( 0, iPosY - m_iOverlapOffset_Y );
				iPosX = MAX( 0, iPosX - m_iOverlapOffset_X );
			}
		}
		m_bInvalidated = false;
		SetTall( ScreenHeight() - m_iOriginalY );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *scheme )
	{
		LoadControlSettings( "Resource/UI/Econ/NotificationQueuePanel.res" );
		GetBounds( m_iOriginalX, m_iOriginalY, m_iOriginalWidth, m_iOriginalHeight );

		BaseClass::ApplySchemeSettings( scheme );
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();
		
		if ( IsVisible() == false )
		{
			return;
		}

		// Get filtered list of only the notifications that show some in-game content
		CUtlVector< CEconNotification *> notifications;
		GetNotifications( notifications );

		float flCurrTime = engine->Time();

		// check to see if we have a panel for each notification
		int i = 0;
		for ( i = 0; i < notifications.Count(); ++i )
		{
			CEconNotification *pNotification = notifications[i];
			int mapIdx = m_mapNotificationPanels.Find( pNotification->GetID() );
			if ( m_mapNotificationPanels.IsValidIndex( mapIdx ) == false || pNotification->GetInGameLifeTime() < flCurrTime )
			{
				m_bInvalidated = true;
				// create the panel and add it to the UI
				// have it slide from the bottom
				CNotificationToastControl *pControl = NULL;
				int iPosX = 0, iPosY = 0;
				vgui::EditablePanel *pPanel = pNotification->CreateUIElement( false );
				if ( pPanel )
				{
					pControl = new CNotificationToastControl( this, pPanel, pNotification->GetID(), false );
					pControl->GetPos( iPosX, iPosY );
					pControl->SetPos( iPosX, ScreenHeight() );
					iPosY = ScreenHeight();
				}
				NotificationUIInfo_t info = { pControl, iPosX, iPosY };
				m_mapNotificationPanels.Insert( pNotification->GetID(), info );
			}
		}

		// now check to see if we have panels and there is no matching notification
		i = m_mapNotificationPanels.FirstInorder();
		while ( m_mapNotificationPanels.IsValidIndex( i ) )
		{
			int idx = i;
			i = m_mapNotificationPanels.NextInorder( i );
			int iID = m_mapNotificationPanels.Key( idx );

			CEconNotification *pNotification = g_notificationQueue.GetNotification( iID );
			if ( pNotification == NULL || pNotification->GetInGameLifeTime() < flCurrTime )
			{
				// fade here, cause we don't really want to re-layout
				NotificationUIInfo_t &info = m_mapNotificationPanels[idx];
				vgui::EditablePanel *pPanel = info.m_pPanel;
				if ( pPanel )
				{
					pPanel->MarkForDeletion();
				}
				m_mapNotificationPanels.RemoveAt( idx );
				m_bInvalidated = true;
			}
		}

		if ( m_bInvalidated )
		{
			m_flInvalidateTime = MAX( cl_notifications_move_time.GetFloat(), 0.0f );
		}
		if ( m_flInvalidateTime > 0 )
		{
			m_flInvalidateTime -= gpGlobals->frametime;
			InvalidateLayout( true, false );
		}
	}

protected:
	typedef CUtlMap< int, NotificationUIInfo_t > tNotificationPanels;
	tNotificationPanels m_mapNotificationPanels;
	int m_iOriginalX;
	int m_iOriginalY;
	int m_iOriginalWidth;
	int m_iOriginalHeight;
	float m_flInvalidateTime;
	bool m_bInvalidated;

	CPanelAnimationVar( int, m_iVisibleBuffer, "buffer_between_visible", "5" );
	CPanelAnimationVar( int, m_iOverlapOffset_X, "overlap_offset_x", "10" );
	CPanelAnimationVar( int, m_iOverlapOffset_Y, "overlap_offset_y", "10" );

	void GetNotifications(CUtlVector< CEconNotification *> &notifications )
	{
		const CUtlVector< CEconNotification *> &allNotifications = g_notificationQueue.GetItems();

		for (int i = 0 ; i < allNotifications.Count() ; ++i )
		{
			CEconNotification *pNotification = allNotifications[i];
			if ( pNotification->BShowInGameElements() )
			{
				notifications.AddToTail( pNotification );
			}
		}
	}
};

DECLARE_HUDELEMENT( CNotificationQueuePanel );

//-----------------------------------------------------------------------------

CEconNotification::CEconNotification()
	: m_pText("")
	, m_pSoundFilename( NULL )
	, m_flExpireTime( engine->Time() + 10.0f )
	, m_pKeyValues( NULL )
	, m_bInUse( false )
	, m_steamID()
{
}

CEconNotification::~CEconNotification()
{
	if ( m_pKeyValues )
	{
		m_pKeyValues->deleteThis();
	}
}

void CEconNotification::SetText( const char *pText )
{
	m_pText = pText;
}

void CEconNotification::AddStringToken( const char* pToken, const wchar_t* pValue )
{
	if ( m_pKeyValues == NULL )
	{
		m_pKeyValues = new KeyValues( "CEconNotification" );
	}
	m_pKeyValues->SetWString( pToken, pValue );
}

void CEconNotification::SetKeyValues( KeyValues *pKeyValues )
{
	if ( m_pKeyValues != NULL )
	{
		m_pKeyValues->deleteThis();
	}
	m_pKeyValues = pKeyValues->MakeCopy();
	++m_iKVVersion;
}

KeyValues *CEconNotification::GetKeyValues() const
{
	return m_pKeyValues;
}

const wchar_t *CEconNotification::GetText()
{
	g_pVGuiLocalize->ConstructString_safe( m_wszBuffer, m_pText, m_pKeyValues );
	return m_wszBuffer;
}

int CEconNotification::GetID() const
{
	return m_iID;
}

void CEconNotification::SetLifetime( float flSeconds )
{
	m_flExpireTime = engine->Time() + flSeconds;
}

float CEconNotification::GetExpireTime() const
{
	return m_flExpireTime;
}

float CEconNotification::GetInGameLifeTime() const
{
	return m_flExpireTime;	// default's to passed in time unless otherwise set (for derived classes)
}

void CEconNotification::SetIsInUse( bool bInUse)
{
	m_bInUse = bInUse;
}

bool CEconNotification::GetIsInUse() const
{
	return m_bInUse;
}

void CEconNotification::SetSteamID( const CSteamID &steamID )
{
	m_steamID = steamID;
}

const CSteamID &CEconNotification::GetSteamID() const
{
	return m_steamID;
}

void CEconNotification::MarkForDeletion()
{
	// to be deleted ASAP
	m_flExpireTime = 0.0f;
}

CEconNotification::EType CEconNotification::NotificationType()
{
	return eType_Basic;
}

bool CEconNotification::BHighPriority()
{
	return false;
}

void CEconNotification::Trigger()
{
}

void CEconNotification::Accept()
{
}

void CEconNotification::Decline()
{
}

void CEconNotification::Deleted()
{
}

void CEconNotification::Expired()
{
}
vgui::EditablePanel *CEconNotification::CreateUIElement( bool bMainMenu ) const
{
	CGenericNotificationToast *pToast = new CGenericNotificationToast( NULL, m_iID, bMainMenu );
	return pToast;
}

const char *CEconNotification::GetUnlocalizedHelpText()
{
	switch ( NotificationType() )
	{
		case eType_AcceptDecline:
			return "#Notification_AcceptOrDecline_Help";
		case eType_MustTrigger:
		case eType_Trigger:
			return "#Notification_CanTrigger_Help";
		default:
			Assert( !"Unhandled enum value" );
			// ---v
		case eType_Basic:
			return "#Notification_Remove_Help";
	}

}

//-----------------------------------------------------------------------------

class CMainMenuNotificationsControl : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMainMenuNotificationsControl, vgui::EditablePanel );
public:
	CMainMenuNotificationsControl( vgui::EditablePanel *pParent, const char *pElementName ) 
		: BaseClass( pParent, pElementName )
		, m_mapNotificationPanels( DefLessFunc(int) )
		, m_iNumItems( 0 )
	{
		vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
	}

	virtual ~CMainMenuNotificationsControl()
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	virtual void PerformLayout( void )
	{
		BaseClass::PerformLayout();

		const CUtlVector< CEconNotification *> &notifications = g_notificationQueue.GetItems();

		// position the notifications around
		// grow down
		int iTotalHeight = 0;
		const int kBuffer = 5;
		for ( int i = 0; i < notifications.Count(); ++i )
		{
			CEconNotification *pNotification = notifications[i];
			if ( !pNotification->BCreateMainMenuPanel() )
			{
				continue;
			}

			int mapIdx = m_mapNotificationPanels.Find( pNotification->GetID() );
			if ( m_mapNotificationPanels.IsValidIndex( mapIdx ) == false )
			{
				continue;
			}
			NotificationUIInfo_t &info = m_mapNotificationPanels[mapIdx];
			vgui::EditablePanel *pPanel = info.m_pPanel;
			if ( pPanel )
			{
				int iPanelX;
				int iPanelY;
				int iWidth;
				int iHeight;				
				pPanel->GetBounds( iPanelX, iPanelY, iWidth, iHeight );
				int iNewPosX = iPanelX;
				int iNewPosY = iTotalHeight;
				pPanel->SetPos( iNewPosX, iNewPosY );
				iTotalHeight += iHeight + kBuffer;
			}
		}
		int iWidth, iHeight;
		GetSize( iWidth, iHeight );
		SetSize( iWidth, iTotalHeight );
		if ( iTotalHeight != iHeight )
		{
			GetParent()->InvalidateLayout( false, false );
		}
	}

	virtual void OnTick()
	{
		if ( m_iNumItems != g_notificationQueue.GetItems().Count() )
		{
			m_iNumItems = g_notificationQueue.GetItems().Count();
			PostActionSignal( new KeyValues("Command", "command", "notifications_update" ) );
		}
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();
		
		if ( IsVisible() == false )
		{
			return;
		}

		const CUtlVector< CEconNotification *> &notifications = g_notificationQueue.GetItems();
		bool bInvalidated = false;

		// check to see if we have a panel for each notification
		int i = 0;
		for ( i = 0; i < notifications.Count(); ++i )
		{
			CEconNotification *pNotification = notifications[i];
			int mapIdx = m_mapNotificationPanels.Find( pNotification->GetID() );
			if ( m_mapNotificationPanels.IsValidIndex( mapIdx ) == false )
			{
				bInvalidated = true;
				CNotificationToastControl *pControl = NULL;
				vgui::EditablePanel *pPanel = pNotification->CreateUIElement( true );
				if ( pPanel )
				{
					pControl = new CNotificationToastControl( this, pPanel, pNotification->GetID(), true );
				}
				NotificationUIInfo_t info = { pControl, 0, 0 };
				m_mapNotificationPanels.Insert( pNotification->GetID(), info );
			}
		}

		// now check to see if we have panels and there is no matching notification
		i = m_mapNotificationPanels.FirstInorder();
		while ( m_mapNotificationPanels.IsValidIndex( i ) )
		{
			int idx = i;
			i = m_mapNotificationPanels.NextInorder( i );
			int iID = m_mapNotificationPanels.Key( idx );
			if ( g_notificationQueue.GetNotification( iID ) == NULL )
			{
				NotificationUIInfo_t &info = m_mapNotificationPanels[idx];
				vgui::EditablePanel *pPanel = info.m_pPanel;
				if ( pPanel )
				{
					pPanel->MarkForDeletion();
				}
				m_mapNotificationPanels.RemoveAt( idx );
				bInvalidated = true;
			}
		}

		if ( bInvalidated )
		{
			InvalidateLayout( true, false );
		}
	}

protected:
	typedef CUtlMap< int, NotificationUIInfo_t > tNotificationPanels;
	tNotificationPanels m_mapNotificationPanels;
	int m_iNumItems;
};

// Show in UI
class CNotificationsPresentPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CNotificationsPresentPanel, vgui::EditablePanel );
public:
	CNotificationsPresentPanel( vgui::Panel *pParent, const char* pElementName ) : vgui::EditablePanel( pParent, "NotificationsPresentPanel" )
	{
		SetMouseInputEnabled( true );
	}

	virtual ~CNotificationsPresentPanel()
	{
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		
		LoadControlSettings( "Resource/UI/Econ/NotificationsPresentPanel.res" );

		for ( int i = 0; i < GetChildCount(); i++ )
		{
			vgui::Panel *pChild = GetChild( i );
			pChild->SetMouseInputEnabled( false );
		}
	}
	
	virtual void OnMousePressed(vgui::MouseCode code)
	{
		if ( code != MOUSE_LEFT )
			return;

		PostActionSignal( new KeyValues("Close") );

		// audible feedback
		const char *soundFilename = "ui/buttonclick.wav";

		vgui::surface()->PlaySound( soundFilename );
	}
};

DECLARE_BUILD_FACTORY( CNotificationsPresentPanel );

//-----------------------------------------------------------------------------
// External interface for the notification queue

int NotificationQueue_Add( CEconNotification *pNotification )
{
	return 0;

	if ( ( !engine->IsInGame() && pNotification->BCreateMainMenuPanel() ) ||
		 ( engine->IsInGame() && cl_notifications_show_ingame.GetBool() && pNotification->BShowInGameElements()) )
	{
		vgui::surface()->PlaySound( pNotification->GetSoundFilename() );
	}

	return g_notificationQueue.AddNotification( pNotification );
}

CEconNotification *NotificationQueue_Get( int iID )
{
	return g_notificationQueue.GetNotification( iID );
}

CEconNotification *NotificationQueue_GetByIndex( int idx )
{
	return g_notificationQueue.GetNotificationByIndex( idx );
}

void NotificationQueue_RemoveAll()
{
	g_notificationQueue.RemoveAllNotifications();
}

void NotificationQueue_Remove( int iID )
{
	g_notificationQueue.RemoveNotification( iID );
}

void NotificationQueue_Remove( CEconNotification *pNotification )
{
	g_notificationQueue.RemoveNotification( pNotification );
}

void NotificationQueue_Remove( NotificationFilterFunc func )
{
	g_notificationQueue.RemoveNotifications( func );
}

int NotificationQueue_Count( NotificationFilterFunc func )
{
	return g_notificationQueue.CountNotifications( func );
}

void NotificationQueue_Visit( CEconNotificationVisitor &visitor )
{
	g_notificationQueue.VisitNotifications( visitor );
}

void NotificationQueue_Update()
{
	g_notificationQueue.Update();
}

int NotificationQueue_GetNumMainMenuNotifications()
{
	auto& vecNotification = g_notificationQueue.GetItems();
	int nCount = 0;
	FOR_EACH_VEC( vecNotification, i )
	{
		if ( vecNotification[ i ]->BCreateMainMenuPanel() )
			++nCount;
	}

	return nCount;
}

int NotificationQueue_GetNumNotifications()
{
	return g_notificationQueue.GetItems().Count();
}

vgui::EditablePanel* NotificationQueue_CreateMainMenuUIElement( vgui::EditablePanel *pParent, const char *pElementName )
{
	CMainMenuNotificationsControl *pControl = new CMainMenuNotificationsControl( pParent, pElementName );
	pControl->AddActionSignalTarget( pParent );
	return pControl;
}

//-----------------------------------------------------------------------------

CON_COMMAND( cl_trigger_first_notification, "Tries to accept/trigger the first notification" )
{
	const CUtlVector< CEconNotification *> &notifications = g_notificationQueue.GetItems();
	if ( notifications.Count() > 0 )
	{
		CEconNotification *pNotification = notifications[0];
		switch ( pNotification->NotificationType() )
		{
			case CEconNotification::eType_AcceptDecline:
				pNotification->Accept();
				break;
			case CEconNotification::eType_MustTrigger:
			case CEconNotification::eType_Trigger:
				pNotification->Trigger();
			case CEconNotification::eType_Basic:
				break;
			default:
				Assert( !"Unhandled enum value" );
		}
	}
}

CON_COMMAND( cl_decline_first_notification, "Tries to decline/remove the first notification" )
{
	const CUtlVector< CEconNotification *> &notifications = g_notificationQueue.GetItems();
	if ( notifications.Count() > 0 )
	{
		CEconNotification *pNotification = notifications[0];
		switch ( pNotification->NotificationType() )
		{
			case CEconNotification::eType_AcceptDecline:
				pNotification->Decline();
				break;
			case CEconNotification::eType_MustTrigger:
				break; // YOU MUUUSSTTTTT
			case CEconNotification::eType_Trigger:
			case CEconNotification::eType_Basic:
				pNotification->Deleted();
				pNotification->MarkForDeletion();
				break;
			default:
				Assert( !"Unhandled enum value" );
		}
	}
}

#ifdef _DEBUG

#include "confirm_dialog.h"

class CTFTestNotification : public CEconNotification
{
public:
	CTFTestNotification( const char* pText, EType eType )
		: CEconNotification()
		, m_pText( pText )
		, m_eType( eType )
	{
	}

	virtual EType NotificationType() OVERRIDE { return m_eType; }

	virtual void Trigger() OVERRIDE
	{
		ShowMessageBox( "", m_pText, "#GameUI_OK" );
		MarkForDeletion();
	}
	virtual void Accept() OVERRIDE
	{
		ShowMessageBox( "Accept", m_pText, "#GameUI_OK" );
		MarkForDeletion();
	}
	virtual void Decline() OVERRIDE
	{
		ShowMessageBox( "Decline", m_pText, "#GameUI_OK" );
		MarkForDeletion();
	}
private:
	const char *m_pText;
	EType m_eType;
};

CON_COMMAND( cl_add_notification, "Adds a notification" )
{
	if ( args.ArgC() >= 2 )
	{
		CEconNotification::EType eType = CEconNotification::eType_Basic;
		if ( args.ArgC() >= 5 )
		{
			eType = (CEconNotification::EType)atoi( args[4] );
		}

		CEconNotification *pNotification = new CTFTestNotification( args[1], eType );

		pNotification->SetText( args[1] );
		if ( args.ArgC() >= 3 )
		{
			int iLifetime = atoi( args[2] );
			pNotification->SetLifetime( iLifetime );
		}
		if ( args.ArgC() >= 4 )
		{
			if ( steamapicontext && steamapicontext->SteamUser() )
			{
				pNotification->SetSteamID( steamapicontext->SteamUser()->GetSteamID() );
			}
		}
		int id = NotificationQueue_Add( pNotification );
		Msg( "Added notification %d\n", id);
	}
}

#endif
