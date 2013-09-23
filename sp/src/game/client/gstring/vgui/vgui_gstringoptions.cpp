
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/gstring_cvars.h"
#include "Gstring/vgui/vgui_gstringOptions.h"
#include "ienginevgui.h"

#include "vgui_controls/panel.h"

#include "vgui_controls/checkbutton.h"
#include "vgui_controls/combobox.h"
#include "vgui_controls/slider.h"
#include "vgui_controls/button.h"

using namespace vgui;


CVGUIGstringOptions::CVGUIGstringOptions( VPANEL parent, const char *pName ) : BaseClass( NULL, pName )
{
	SetParent( parent );

	Activate();

	m_pCheck_CinematicBars = new CheckButton( this, "check_bars", "Enable cinematic bars" );
	m_pCheck_FilmGrain = new CheckButton( this, "check_filmgrain", "Enable film grain" );
	m_pCheck_Vignette = new CheckButton( this, "check_vignette", "Enable vignette" );
	m_pCheck_GodRays = new CheckButton( this, "check_godrays", "Enable god-rays" );
	m_pCheck_WaterEffects = new CheckButton( this, "check_screenwater", "Enable water screen effects" );
	m_pCheck_LensFlare = new CheckButton( this, "check_lensflare", "Enable lens flares" );
	m_pCBox_BloomFlare = new ComboBox( this, "check_bloomflare", 3, false );
	m_pCBox_BloomFlare->AddItem( "Never", NULL );
	m_pCBox_BloomFlare->AddItem( "Map based", NULL );
	m_pCBox_BloomFlare->AddItem( "Always", NULL );
	m_pCheck_MotionBlur = new CheckButton( this, "check_motionblur", "Enable motion blur" );
	m_pCheck_DreamBlur = new CheckButton( this, "check_dreamblur", "Enable dream effects" );
	m_pCheck_ScreenBlur = new CheckButton( this, "check_screenblur", "Enable screen blur" );
	m_pCheck_ExplosionBlur = new CheckButton( this, "check_explosion", "Enable explosion distortion" );

#define CREATE_VGUI_SLIDER( var, name, minRange, maxRange, ticks ) var = new Slider( this, name ); \
	var->SetRange( minRange, maxRange ); \
	var->SetNumTicks( ticks );

	CREATE_VGUI_SLIDER( m_pSlider_CinematicBars_Size, "slider_bars", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_MotionBlur_Strength, "slider_mblur", 1, 10, 9 );
	CREATE_VGUI_SLIDER( m_pSlider_BloomFlare_Strength, "slider_bflare", 1, 10, 9 );
	CREATE_VGUI_SLIDER( m_pSlider_ExplosionBlur_Strength, "slider_expblur", 1, 10, 9 );
	CREATE_VGUI_SLIDER( m_pSlider_Desaturation_Strength, "slider_desat", 0, 10, 10 );
	CREATE_VGUI_SLIDER( m_pSlider_FilmGrain_Strength, "slider_filmgrain", 1, 10, 9 );

	m_pLabel_Value_CinematicBars = new Label( this, "label_bars", "" );
	m_pLabel_Value_MotionBlur = new Label( this, "label_mblur", "" );
	m_pLabel_Value_BloomFlare = new Label( this, "label_bflare", "" );
	m_pLabel_Value_ExplosionBlur = new Label( this, "label_expblur", "" );
	m_pLabel_Value_Desaturation = new Label( this, "label_desat", "" );
	m_pLabel_Value_FilmGrain = new Label( this, "label_filmgrain", "" );

	LoadControlSettings( "resource/gstring_options.res" );

	DoModal();

	SetDeleteSelfOnClose( true );
	SetVisible( true );
	SetSizeable(false);
	SetMoveable(true);

	SetTitle( "G-string options", false );
}

CVGUIGstringOptions::~CVGUIGstringOptions()
{
}

void CVGUIGstringOptions::OnCommand( const char *cmd )
{
	if ( !Q_stricmp( cmd, "save" ) )
	{
#define CVAR_CHECK_INTEGER( x, y ) ( x.SetValue( ( y->IsSelected() ? 1 : 0 ) ) )
#define CVAR_SLIDER_FLOAT( x, y, ratio ) ( x.SetValue( (float)(y->GetValue()/(float)ratio ) ) )

		CVAR_CHECK_INTEGER( cvar_gstring_drawbars, m_pCheck_CinematicBars );
		CVAR_CHECK_INTEGER( cvar_gstring_drawfilmgrain, m_pCheck_FilmGrain );
		CVAR_CHECK_INTEGER( cvar_gstring_drawvignette, m_pCheck_Vignette );
		CVAR_CHECK_INTEGER( cvar_gstring_drawgodrays, m_pCheck_GodRays );
		CVAR_CHECK_INTEGER( cvar_gstring_drawexplosionblur, m_pCheck_ExplosionBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawmotionblur, m_pCheck_MotionBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
		CVAR_CHECK_INTEGER( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
		CVAR_CHECK_INTEGER( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );

		CVAR_SLIDER_FLOAT( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 50 );
		CVAR_SLIDER_FLOAT( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
		CVAR_SLIDER_FLOAT( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
		CVAR_SLIDER_FLOAT( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 50 );

		cvar_gstring_drawbloomflare.SetValue( m_pCBox_BloomFlare->GetActiveItem() );

		engine->ClientCmd( "host_writeconfig" );

		CloseModal();
	}
	else
		BaseClass::OnCommand( cmd );
}

void CVGUIGstringOptions::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

#define CVAR_CHECK_SELECTED( x, y ) ( y->SetSelected( x.GetBool() ) )
#define CVAR_SLIDER_INTEGER( x, y, ratio ) ( y->SetValue( x.GetFloat() * ratio ) )

	CVAR_CHECK_SELECTED( cvar_gstring_drawbars, m_pCheck_CinematicBars );
	CVAR_CHECK_SELECTED( cvar_gstring_drawfilmgrain, m_pCheck_FilmGrain );
	CVAR_CHECK_SELECTED( cvar_gstring_drawvignette, m_pCheck_Vignette );
	CVAR_CHECK_SELECTED( cvar_gstring_drawgodrays, m_pCheck_GodRays );
	CVAR_CHECK_SELECTED( cvar_gstring_drawexplosionblur, m_pCheck_ExplosionBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawmotionblur, m_pCheck_MotionBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawscreenblur, m_pCheck_ScreenBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawdreamblur, m_pCheck_DreamBlur );
	CVAR_CHECK_SELECTED( cvar_gstring_drawlensflare, m_pCheck_LensFlare );
	CVAR_CHECK_SELECTED( cvar_gstring_drawwatereffects, m_pCheck_WaterEffects );

	m_pCBox_BloomFlare->ActivateItem( clamp( cvar_gstring_drawbloomflare.GetInt(),
		0, m_pCBox_BloomFlare->GetItemCount() ) );

	CVAR_SLIDER_INTEGER( cvar_gstring_explosionfx_strength, m_pSlider_ExplosionBlur_Strength, 10 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bars_scale, m_pSlider_CinematicBars_Size, 51 );
	CVAR_SLIDER_INTEGER( cvar_gstring_motionblur_scale, m_pSlider_MotionBlur_Strength, 10 );
	CVAR_SLIDER_INTEGER( cvar_gstring_bloomflare_strength, m_pSlider_BloomFlare_Strength, 2 );
	CVAR_SLIDER_INTEGER( cvar_gstring_desaturation_strength, m_pSlider_Desaturation_Strength, 10 );
	CVAR_SLIDER_INTEGER( cvar_gstring_filmgrain_strength, m_pSlider_FilmGrain_Strength, 51 );
}

void CVGUIGstringOptions::PerformLayout()
{
	BaseClass::PerformLayout();

	MoveToCenterOfScreen();
}

void CVGUIGstringOptions::OnCheckButtonChecked( Panel *panel )
{
}

void CVGUIGstringOptions::OnSliderMoved( KeyValues *pKV )
{
	m_pLabel_Value_CinematicBars->SetText( VarArgs( "%.1f", m_pSlider_CinematicBars_Size->GetValue() / 10.0f ) );
	m_pLabel_Value_MotionBlur->SetText( VarArgs( "%.1f", m_pSlider_MotionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_BloomFlare->SetText( VarArgs( "%.1f", m_pSlider_BloomFlare_Strength->GetValue() / 2.0f ) );
	m_pLabel_Value_ExplosionBlur->SetText( VarArgs( "%.1f", m_pSlider_ExplosionBlur_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_Desaturation->SetText( VarArgs( "%.1f", m_pSlider_Desaturation_Strength->GetValue() / 10.0f ) );
	m_pLabel_Value_FilmGrain->SetText( VarArgs( "%.1f", m_pSlider_FilmGrain_Strength->GetValue() / 10.0f ) );
}

CON_COMMAND( vgui_showGstringOptions, "" )
{
	vgui::VPANEL GameUIRoot = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	new CVGUIGstringOptions( GameUIRoot, "GstringOptions" );
}