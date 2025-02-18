//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_annotationspanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include <vgui_controls/Label.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "c_baseobject.h"
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "tf_gamerules.h"
#include "tf_hud_training.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CTFAnnotationsPanel, 1 );

static const float LIFE_TIME = 1.0f;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFAnnotationsPanel::CTFAnnotationsPanel( const char *pElementName )
	: EditablePanel( NULL, "AnnotationsPanel" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
	m_bShouldBeVisible = false;
	SetScheme( "ClientScheme" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAnnotationsPanel::~CTFAnnotationsPanel()
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::Reset()
{
	RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::Init()
{
	// listen for events
	ListenForGameEvent( "show_annotation" );
	ListenForGameEvent( "hide_annotation" );
	
	RemoveAll();

	CHudElement::Init();
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::FireGameEvent( IGameEvent * event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "hide_annotation", pEventName ) == 0 )
	{
		HideAnnotation( event->GetInt("id") );
	}
	else if ( Q_strcmp( "show_annotation", pEventName ) == 0 )
	{
		AddAnnotation( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAnnotationsPanelCallout *CTFAnnotationsPanel::TestAndAddCallout( int id, Vector &origin, const char *text )
{
	int insertSlot = -1;

	// Find an available slot and also see if this call out already exists in the list.
	for (int i=0; i<m_pCalloutPanels.Count(); ++i)
	{
		if (NULL == m_pCalloutPanels[i])
		{
			insertSlot = i;
			continue;
		}
		// If we already have this annotation, return and don't add it again.
		if (m_pCalloutPanels[i]->GetAnnotationID() == id)
		{
			m_pCalloutPanels[i]->SetText( text );
			m_pCalloutPanels[i]->SetLocation( origin );
			m_pCalloutPanels[i]->SetFollowEntity( NULL );
			m_pCalloutPanels[i]->InvalidateLayout();
			return m_pCalloutPanels[i];
		}
		// if one is available, use it
		if ( m_pCalloutPanels[i]->IsVisible() == false )
		{
			insertSlot = i;
			continue;
		}
	}

	CTFAnnotationsPanelCallout *pCallout = new CTFAnnotationsPanelCallout( g_pClientMode->GetViewport(), "AnnotationsPanelCallout", id, origin, text );

	if (-1 == insertSlot)
	{
		m_pCalloutPanels.AddToTail( vgui::SETUP_PANEL(pCallout) );
	}
	else
	{
		if ( m_pCalloutPanels[insertSlot] != NULL )
		{
			m_pCalloutPanels[insertSlot]->MarkForDeletion();
		}
		m_pCalloutPanels[insertSlot] = vgui::SETUP_PANEL(pCallout);
	}

	return pCallout;
}


void CTFAnnotationsPanel::UpdateAnnotations( void )
{

	//Find an available slot and also see if this call out already exists in the list.
	for ( int i = m_pCalloutPanels.Count()-1; i >= 0; i-- )
	{
		if ( !m_pCalloutPanels[i] ) 
		{
			m_pCalloutPanels.Remove(i);
			continue;
		}

		// Update the callout. If it says it's finished, remove it.
		if ( m_pCalloutPanels[i]->UpdateCallout() )
		{
			m_pCalloutPanels[i]->MarkForDeletion();
			m_pCalloutPanels.Remove(i);
		}
	}

	// If we have no active callouts, hide
	if ( !m_pCalloutPanels.Count() )
	{
		m_bShouldBeVisible = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::AddAnnotation( IGameEvent * event )
{
	const char *text = event->GetString( "text" );
	int id = event->GetInt("id");
	float x = event->GetFloat("worldPosX");
	float y = event->GetFloat("worldPosY");
	float z = event->GetFloat("worldPosZ");
	int iVisibilityBitfield = event->GetInt("visibilityBitfield");
	float flLifetime = event->GetFloat("lifetime");
	int iFollowEntIndex = event->GetInt("follow_entindex");
	bool bShowDistance = event->GetBool("show_distance");
	const char *pSound = event->GetString( "play_sound" );
	bool bShowEffect = event->GetBool( "show_effect" );
					   
	Vector location;

	location.x = x;
	location.y = y;
	location.z = z;

	m_bShouldBeVisible = true;

	// Try and add the callout
	CTFAnnotationsPanelCallout *pCallout = TestAndAddCallout( id, location, text );

	if ( pCallout )
	{
		C_BaseEntity *pFollowEntity = iFollowEntIndex != 0 ? ClientEntityList().GetEnt( iFollowEntIndex) : NULL;
		pCallout->Touch();
		pCallout->SetLifetime( flLifetime );
		pCallout->SetVisibilityBitfield( iVisibilityBitfield );
		pCallout->SetFollowEntity( pFollowEntity );
		pCallout->SetShowDistance( bShowDistance );
		pCallout->UpdateCallout();

		if ( pCallout->IsVisible() )
		{
			if ( pSound )
			{
				vgui::surface()->PlaySound( pSound );
			}

			if ( bShowEffect )
			{
				if ( pFollowEntity && pFollowEntity->ParticleProp())
				{
					pFollowEntity->ParticleProp()->Create( "ping_circle", PATTACH_ABSORIGIN_FOLLOW );
				}
				else
				{
					Vector vecNormal( event->GetFloat("worldNormalX"), event->GetFloat("worldNormalY"), event->GetFloat("worldNormalZ") );
					Vector vecOrigin( x, y, z );
					vecOrigin += vecNormal * 20.0f;

					QAngle vecAngles;
					VectorAngles( vecNormal, vecAngles );
					DispatchParticleEffect( "ping_circle", vecOrigin, vecAngles );
				}				
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::HideAnnotation( int id )
{
	// Delete all our callout panels
	for ( int i = m_pCalloutPanels.Count()-1; i >= 0; i-- )
	{
		if ( m_pCalloutPanels[i]->GetAnnotationID() == id )
		{
			 m_pCalloutPanels[i]->FadeAndRemove();
			 break;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::RemoveAll()
{
	m_bShouldBeVisible = false;

	// Delete all our callout panels
	for ( int i = m_pCalloutPanels.Count()-1; i >= 0; i-- )
	{
		m_pCalloutPanels[i]->MarkForDeletion();
	}
	m_pCalloutPanels.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAnnotationsPanel::ShouldDraw( void )
{
	return m_bShouldBeVisible;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( IsVisible() )
	{
		UpdateAnnotations();
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAnnotationsPanelCallout::~CTFAnnotationsPanelCallout()
{
	if ( m_pArrowImages )
	{
		m_pArrowImages->deleteThis();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFAnnotationsPanelCallout::CTFAnnotationsPanelCallout( Panel *parent, const char *name, int id, Vector &location, const char* text ) : EditablePanel( parent, name )
{
	m_pAnnotationLabel = NULL;
	m_pDistanceLabel = NULL;
	m_pArrowImages = NULL;
	m_ID = id;
	m_Location = location;
	m_Text = text;
	m_DeathTime = 0.0f;
	m_flLerpPercentage = 1.0f;
	m_bWasOffscreen = false;
	m_bShowDistance = false;
	m_flAlpha[0] = m_flAlpha[1] = 0.0f;
	SetWorldPositionCurrentFrame( true );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::ApplySettings( KeyValues *pInResourceData )
{
	BaseClass::ApplySettings( pInResourceData );

	if ( !m_pArrowImages )
	{
		KeyValues *pArrowImagesSubKey = pInResourceData->FindKey( "ArrowIcons" );	AssertMsg( pArrowImagesSubKey, "This must exist!" );
		m_pArrowImages = pArrowImagesSubKey->MakeCopy();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/AnnotationsPanelCallout.res" );

	m_pDistanceLabel = dynamic_cast<Label *>( FindChildByName( "DistanceLabel" ) );

	m_pBackground = FindChildByName( "CalloutBG" );
	if ( m_pBackground )
	{
		m_pAnnotationLabel = dynamic_cast<CExLabel *>( FindChildByName( "CalloutLabel" ) );
		m_pAnnotationLabel->SetParent( m_pBackground );
		m_pDistanceLabel->SetParent( m_pBackground );
	}

	wchar_t outputText[MAX_TRAINING_MSG_LENGTH];
	if ( m_pAnnotationLabel && CTFHudTraining::FormatTrainingText( m_Text, outputText ) )
	{
		m_pAnnotationLabel->SetText(outputText);
	}

	m_pArrow = dynamic_cast<ImagePanel *>( FindChildByName( "ArrowIcon" ) );
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::PerformLayout( void )
{
	if ( !m_pAnnotationLabel || !m_pDistanceLabel )
		return;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer == NULL )
		return;

	// @todo Tom Bui: not sure why we need to do this again, but if we don't
	// we still have the camera lag!
	InvalidateLayout();

	// Reposition the callout based on our target's position
	Vector vecTarget = m_Location;
	if ( m_FollowEntity.Get() )
	{
		vecTarget = m_FollowEntity->GetAbsOrigin();
		if ( m_FollowEntity->CollisionProp() )
		{
			vecTarget.z += m_FollowEntity->CollisionProp()->OBBSize().z;
		}
	}
	Vector vDelta = vecTarget - MainViewOrigin();
	float flDistance = vDelta.Length();
	VectorNormalize( vDelta );	// Only necessary so we can use it as part of our alpha calculation

	// Is the target visible on screen?
	int iX, iY;
	bool bOnscreen = GetVectorInHudSpace( vecTarget, iX, iY ); // Tested - confirmed NOT GetVectorInScreenSpace

	// Calculate the perp dot product
	QAngle angPlayerView = MainViewAngles();
	Vector vView, vRight, vUp;
	AngleVectors( angPlayerView, &vView, &vRight, &vUp );
	const float flPerpDot = vDelta.x * vView.y - vDelta.y * vView.x;

	// Calculate the alpha - the more the user looks away from the target, the greater the alpha
	if ( m_DeathTime > 0.0f && m_DeathTime - LIFE_TIME >= gpGlobals->curtime )
	{
		m_flAlpha[0] = m_flAlpha[1] = 255 * clamp( ( m_DeathTime - gpGlobals->curtime ) / LIFE_TIME, 0.0f, 1.0f );
	}
	else
	{
		m_flAlpha[1] = 255;
		// BUGBUG: the following lines don't do anything because of the clamp range
		//const float flDot = DotProduct( vDelta, vView );	// As the player looks away the target to the target, this will go from -1 to 1
		//m_flAlpha[1] = clamp( -255 * flDot, 255, 255 );	// Set target.
		m_flAlpha[0] = Lerp( gpGlobals->frametime, m_flAlpha[0], m_flAlpha[1] );	 // Move towards target
	}

	const int fade_alpha = m_flAlpha[0];

	SetAlpha( fade_alpha );
	m_pArrow->SetAlpha( fade_alpha );
	m_pBackground->SetAlpha( fade_alpha );

	const int halfWidth = m_pBackground->GetWide() / 2;
	bool bOffscreen = !bOnscreen || iX < halfWidth || iX > ScreenWidth()-halfWidth;
	if ( bOffscreen )
	{
		m_pArrow->SetSize( XRES( 10 ), YRES( 20 ) );

		const int nHorizontalBuffer = XRES( 20 );
		iX = flPerpDot <= 0.0f ? nHorizontalBuffer : ( ScreenWidth() - nHorizontalBuffer - m_pBackground->GetWide() - m_pArrow->GetWide() );
		iY = ( ScreenHeight() - m_pBackground->GetTall() ) / 2;
	}
	else
	{
		// On screen
		// If our target isn't visible, we draw transparently
		trace_t	tr;
		UTIL_TraceLine( vecTarget, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		
		if ( tr.fraction < 1.0f )
		{
			// Not visible ie obstructed by some objects in the world.
			// Do *not* show entities that are not the same team
			if ( m_FollowEntity.Get() &&
				 m_FollowEntity->GetTeamNumber() != TEAM_UNASSIGNED && m_FollowEntity->GetTeamNumber() != TEAM_INVALID &&
				 m_FollowEntity->GetTeamNumber() != pLocalTFPlayer->GetTeamNumber() && 
				 pLocalTFPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			{
				SetAlpha( 0 );
				m_pArrow->SetAlpha( 0 );
			}
		}
		
		iX = iX - m_pBackground->GetWide() / 2;
		iY = iY - m_pBackground->GetTall() - m_pArrow->GetTall();
	}
	
	if ( m_bWasOffscreen != bOffscreen )
	{
		m_flLerpPercentage = 0.0f;
		m_bWasOffscreen = bOffscreen;
	}
	// lerp to the new position if applicable
	// note that this accelerates, cause it uses the current position as the "last position"
	if ( m_flLerpPercentage < 1.0f )
	{
		const float kInvLerpTime = 1.0f / 2.5f;
		m_flLerpPercentage = MIN( m_flLerpPercentage + gpGlobals->frametime * kInvLerpTime, 1.0f );
		int currentX, currentY;
		GetPos( currentX, currentY );
		iX = Lerp( m_flLerpPercentage, currentX, iX );
		iY = Lerp( m_flLerpPercentage, currentY, iY );
	}
	SetPos( iX, iY );

	int wide, tall;
	m_pAnnotationLabel->GetContentSize( wide, tall );

	if ( m_bShowDistance )
	{
		wchar_t *wzFollowEntityName = NULL;
		if ( m_FollowEntity.Get() )
		{
			C_BaseObject *pBuilding = dynamic_cast< C_BaseObject* >( m_FollowEntity.Get() );
			if ( pBuilding && pBuilding->GetType() < OBJ_LAST )
			{
				// Must match resource/tf_objects.txt!!!
				const char *szLocalizedObjectNames[OBJ_LAST] =
				{
					"#TF_Object_Dispenser",
					"#TF_Object_Tele",
					"#TF_Object_Sentry",
					"#TF_object_Sapper"
				};
				wzFollowEntityName = g_pVGuiLocalize->Find( szLocalizedObjectNames[pBuilding->GetType()] );
			}
		}
		const float kInchesToMeters = 0.0254f;
		int distance = RoundFloatToInt( flDistance * kInchesToMeters );
		wchar_t wzValue[32];
		_snwprintf( wzValue, ARRAYSIZE( wzValue ), L"%u", distance );

		wchar_t wzText[256];
		if ( wzFollowEntityName == NULL )
		{
			g_pVGuiLocalize->ConstructString_safe( wzText, g_pVGuiLocalize->Find( "#TR_DistanceTo" ), 1, wzValue );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wzText, g_pVGuiLocalize->Find( "#TR_DistanceToObject" ), 2, wzFollowEntityName, wzValue );
		}

		m_pDistanceLabel->SetText( wzText );
		int distanceWide, distanceTall;
		m_pDistanceLabel->GetContentSize( distanceWide, distanceTall );
		wide = MAX( distanceWide, wide );
		tall += distanceTall;
	}

	wide += XRES(24);
	tall += YRES(18);

	// Set this panel, the label, and the background to contain the text
	const int aArrowBuffers[2] = { (int)(XRES( 20 ) * 2), (int)YRES( 20 ) };	// Leave enough room for arrows
	SetSize( wide + aArrowBuffers[0], tall + aArrowBuffers[1] );
	m_pDistanceLabel->SetSize( wide, m_pDistanceLabel->GetTall() );
	if ( m_pBackground )
	{
		// also adjust the background image
		m_pBackground->SetSize( wide, tall );
		m_pAnnotationLabel->SetSize( m_pBackground->GetWide(), m_pBackground->GetTall() );
		m_pDistanceLabel->SetSize( m_pBackground->GetWide(), m_pDistanceLabel->GetTall() );
	}

	// position background and arrows
	if ( bOffscreen )
	{
		// Set the arrow to left or right, depending on which side of the screen the panel is on
		if ( m_pBackground )
		{
			if ( iX + m_pBackground->GetWide() / 2 < ScreenWidth() / 2 )
			{
				m_pArrow->SetImage( m_pArrowImages->GetString( "left" ) );
				m_pArrow->SetPos( 0, ( m_pBackground->GetTall() - m_pArrow->GetTall() ) / 2 );
				m_pBackground->SetPos( m_pArrow->GetWide() + XRES( 1 ), 0 );
			}
			else
			{
				m_pArrow->SetImage( m_pArrowImages->GetString( "right" ) );
				m_pArrow->SetPos( m_pBackground->GetWide(), ( m_pBackground->GetTall() - m_pArrow->GetTall() ) / 2 );
				m_pBackground->SetPos( 0, 0 );
			}
		}
	}
	else
	{
		if ( m_pBackground )
		{
			// Set the arrow image to the one that points down
			m_pBackground->SetPos( 0, 0 );
			m_pArrow->SetImage( m_pArrowImages->GetString( "down" ) );
			m_pArrow->SetSize( XRES( 20 ), YRES( 10 ) );
			m_pArrow->SetPos( ( m_pBackground->GetWide() - m_pArrow->GetWide() ) / 2, m_pBackground->GetTall() );
		}
	}

	// check that we haven't run off the right side of the screen
	int x,y;
	GetPos( x, y );
	if ( (x + wide) > ScreenWidth() )
	{
		// push ourselves to the left to fit on the screen
		SetPos( ScreenWidth() - wide - XRES(8), y );
	}
}

void CTFAnnotationsPanelCallout::Touch()
{
	SetVisible( true );
	m_DeathTime = gpGlobals->curtime + LIFE_TIME;
	m_bWasOffscreen = true;
	SetPos( ScreenWidth() * 0.125f, ScreenHeight() * 0.125f );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::SetLifetime( float flLifetime )
{
	if ( flLifetime < 0 )
	{
		m_DeathTime = 0.0f;
	}
	else if ( flLifetime == 0 )
	{
		m_DeathTime = gpGlobals->curtime + LIFE_TIME;
	}
	else
	{
		m_DeathTime = gpGlobals->curtime + flLifetime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::SetShowDistance( bool bShowDistance )
{
	m_bShowDistance = bShowDistance;
	if ( m_pDistanceLabel && m_pDistanceLabel->IsVisible() != m_bShowDistance )
	{
		m_pDistanceLabel->SetVisible( bShowDistance );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::SetText( const char *text )
{
	m_Text = text;
	wchar_t outputText[MAX_TRAINING_MSG_LENGTH];
	if ( m_pAnnotationLabel && CTFHudTraining::FormatTrainingText( m_Text, outputText ) )
	{
		m_pAnnotationLabel->SetText(outputText);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFAnnotationsPanelCallout::FadeAndRemove()
{
	m_DeathTime = gpGlobals->curtime + 0.25f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFAnnotationsPanelCallout::UpdateCallout()
{
	if ( m_DeathTime > 0.0f )
	{
		float pct_life = clamp( (m_DeathTime - gpGlobals->curtime) / LIFE_TIME, 0.0f, 1.0f );
		if (pct_life <= 0.0f)
		{
			SetVisible( false );
			return true;
		}
	}

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return true;
	
	if ( m_iVisibilityBitfield != 0 )
	{
		int bit = 1 << pLocalTFPlayer->entindex();
		if (  ( m_iVisibilityBitfield & bit ) == 0 )
		{
			SetVisible( false );
			return true;
		}
	}

	if ( !IsVisible() )
	{
		SetVisible( true );
	}

	InvalidateLayout();

	return false;
}
