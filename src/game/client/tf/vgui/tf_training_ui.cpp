//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <filesystem.h>
#include "ienginevgui.h"
#include "tf_gcmessages.h"
#include "tf_mouseforwardingpanel.h"
#include "gc_clientsystem.h"
#include "c_tf_gamestats.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_gamerules.h"
#include "econ/confirm_dialog.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//------------------------------------------------------------------------------------------------------

#define TRAINING_DIALOG_NAME	"TrainingDialog"
#define TRAINING_PROGRESS_FILE	"trainingprogress.txt"

//------------------------------------------------------------------------------------------------------

#ifdef _DEBUG
#define PRINT_KEY_VALUES( kv_ )	{ CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER); kv_->RecursiveSaveToFile( buf, 0 ); ConMsg( "---\n%s\n---\n", buf.String() ); }
#else
#define PRINT_KEY_VALUES( kv_ )	{ }
#endif

//------------------------------------------------------------------------------------------------------

enum GameMode_t	// Supported game modes for offline practice
{
	MODE_INVALID = -1,

	MODE_CP,
	MODE_KOTH,
	MODE_PL,

	NUM_GAME_MODES
};

static const char *gs_pGameModeTokens[ NUM_GAME_MODES ] = {
	"#Gametype_CP",
	"#Gametype_Koth",
	"#Gametype_Escort",
};

//------------------------------------------------------------------------------------------------------

ConVar cl_training_completed_with_classes( "cl_training_completed_with_classes", "0", FCVAR_ARCHIVE, "Bitfield representing what classes have been used to complete training." );

bool Training_TrainingProgressFileExists()
{
	const char *pFilename = TRAINING_PROGRESS_FILE;
	return g_pFullFileSystem->FileExists( pFilename, NULL );
}

//------------------------------------------------------------------------------------------------------

static int Training_GetClassProgress( int iClass )	// Returns a percent, in the range [0,100]
{
	Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS );

	const int nDefaultResult = iClass == TF_CLASS_SOLDIER ? 0 : -1;

	KeyValuesAD pTrainingProgressData( "TrainingProgress" );
	if ( !pTrainingProgressData )
	{
		Warning( "Failed to save training progress!\n" );
		AssertMsg( 0, "Failed to save training progress!\n" );
		return nDefaultResult;
	}

	const char *pFilename = TRAINING_PROGRESS_FILE;
	if ( !pTrainingProgressData->LoadFromFile( g_pFullFileSystem, pFilename, "MOD" ) )
		return nDefaultResult;

	const char *pClassName = g_aPlayerClassNames_NonLocalized[ iClass ];
	KeyValues *pClassSubKey = pTrainingProgressData->FindKey( pClassName );
	if ( !pClassSubKey )
		return nDefaultResult;

	return pClassSubKey->GetInt( "progress", nDefaultResult );
}

static void Training_GetProgress( int pClass[TF_CLASS_COUNT] )	// Returns a percent, in the range [0,100]
{
	KeyValuesAD pTrainingProgressData( "TrainingProgress" );
	const char *pFilename = TRAINING_PROGRESS_FILE;

	bool bLoadedFileOk = pTrainingProgressData->LoadFromFile( g_pFullFileSystem, pFilename, "MOD" );

	for ( int i = 0; i < TF_CLASS_COUNT; ++i )
	{
		const int iClass = i;
		const int nDefaultResult = ( bLoadedFileOk && iClass == TF_CLASS_SOLDIER ) ? 0 : -1;

		const char *pClassName = g_aPlayerClassNames_NonLocalized[ iClass ];
		KeyValues *pClassSubKey = pTrainingProgressData->FindKey( pClassName );
		if ( !pClassSubKey )
		{
			pClass[ i ] = nDefaultResult;
			continue;
		}

		pClass[ i ] = pClassSubKey->GetInt( "progress", nDefaultResult );
	}
}

KeyValues *Training_LoadProgressFile()
{
	KeyValues *pTrainingProgressData = new KeyValues( "TrainingProgress" );
	if ( !pTrainingProgressData )
	{
		Warning( "Failed to save training progress!\n" );
		AssertMsg( 0, "Failed to save training progress!\n" );
		return NULL;
	}

	// Attempt to load any existing progress from disk
	const char *pFilename = TRAINING_PROGRESS_FILE;
	if ( !pTrainingProgressData->LoadFromFile( g_pFullFileSystem, pFilename, "MOD" ) )
	{
		// File didn't exist - create from defaults
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
		{
			KeyValues *pClassSubKey = new KeyValues( g_aPlayerClassNames_NonLocalized[ i ] );
			if ( !pClassSubKey )
				continue;

			pClassSubKey->SetInt( "progress", -1 );	// -1 means they haven't beat anything
			pTrainingProgressData->AddSubKey( pClassSubKey );
		}
	}

	return pTrainingProgressData;
}

void Training_SaveProgress( KeyValues *pTrainingProgressData )
{
	const char *pFilename = TRAINING_PROGRESS_FILE;
	if ( !pTrainingProgressData->SaveToFile( g_pFullFileSystem, pFilename, "MOD" ) )
	{
		Warning( "Failed to save progress!\n" );
		AssertMsg( 0, "Failed to save progress!" );
	}
}

KeyValues *Training_FindClassData( KeyValues *pTrainingProgressData, int iClass )
{
	const char *pClassName = g_aPlayerClassNames_NonLocalized[ iClass ];
	return pTrainingProgressData->FindKey( pClassName );
}

void Training_SaveProgress( int pProgress[ TF_CLASS_COUNT ] )
{
	KeyValues *pTrainingProgressData = Training_LoadProgressFile();
	if ( !pTrainingProgressData )
		return;

	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		KeyValues *pClassSubKey = Training_FindClassData( pTrainingProgressData, i );
		if ( !pClassSubKey )
		{
			AssertMsg( 0, "All classes should have been created on load if they didn't exist - this should not happen!" );
			continue;
		}

		Assert( pProgress[ i ] >= -1 );
		pClassSubKey->SetInt( "progress", pProgress[ i ] );
	}

	Training_SaveProgress( pTrainingProgressData );
}

void Training_MarkClassComplete( int iClass, int iStage )
{
	Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_LAST_NORMAL_CLASS );
	Assert( iStage >= 0 );

	KeyValues *pTrainingProgressData = Training_LoadProgressFile();
	if ( !pTrainingProgressData )
		return;

	// Find the data for the corresponding class
	KeyValues *pClassSubKey = Training_FindClassData( pTrainingProgressData, iClass );
	if ( pClassSubKey )
	{
		pClassSubKey->SetInt( "progress", iStage );
	}
	else
	{
		Warning( "Failed to load data for class %s!\n", g_aPlayerClassNames_NonLocalized[ iClass ] );
	}

	// Unlock next class if necessary
	const int iLastTrainingClass = TF_CLASS_ENGINEER;
	if ( iClass != iLastTrainingClass )
	{
		const int aNextClasses[ TF_CLASS_COUNT ] = {
			-1,						// TF_CLASS_UNDEFINED
			-1,						// TF_CLASS_SCOUT
		    -1,						// TF_CLASS_SNIPER
		    TF_CLASS_DEMOMAN,		// TF_CLASS_SOLDIER
			TF_CLASS_SPY,			// TF_CLASS_DEMOMAN
			-1,						// TF_CLASS_MEDIC
			-1,						// TF_CLASS_HEAVYWEAPONS
			-1,						// TF_CLASS_PYRO
			TF_CLASS_ENGINEER,		// TF_CLASS_SPY
			-1,						// TF_CLASS_ENGINEER		
		};
		const int aUnlockRequirements[ TF_CLASS_COUNT ] = {
			-1,						// TF_CLASS_UNDEFINED
			-1,						// TF_CLASS_SCOUT
		    -1,						// TF_CLASS_SNIPER
		     2,						// TF_CLASS_SOLDIER	- must beat 2 stages to complete soldier training
			 1,						// TF_CLASS_DEMOMAN
			-1,						// TF_CLASS_MEDIC
			-1,						// TF_CLASS_HEAVYWEAPONS
			-1,						// TF_CLASS_PYRO
			 1,						// TF_CLASS_SPY
			-1,						// TF_CLASS_ENGINEER		
		};
		const int iNextClass = aNextClasses[ iClass ];
		const bool bCurrentClassCompleted = iStage >= aUnlockRequirements[ iClass ];
		if ( iNextClass >= TF_FIRST_NORMAL_CLASS && bCurrentClassCompleted )
		{
			// Find the data for the given class and unlock it
			KeyValues *pNextClassData = pTrainingProgressData->FindKey( g_aPlayerClassNames_NonLocalized[ iNextClass ] );
			if ( pNextClassData )
			{
				pNextClassData->SetInt( "progress", 0 );
			}
			else
			{
				AssertMsg( 0, "This class data should have been filled out above" );
			}
		}
	}

	// Attempt to save
	Training_SaveProgress( pTrainingProgressData );

	// Free
	pTrainingProgressData->deleteThis();
}

static ConVar training_map_video( "training_map_video", "", 0, "Video to show for training" );

void CL_Training_LevelShutdown()
{
	training_map_video.Revert();
}

int Training_GetNumCoursesForClass( int iClass )
{
	static int s_aClassCourses[ TF_CLASS_COUNT ] = {
		0,		// TF_CLASS_UNDEFINED
		0,		// TF_CLASS_SCOUT
	    0,		// TF_CLASS_SNIPER
	    2,		// TF_CLASS_SOLDIER
		1,		// TF_CLASS_DEMOMAN
		0,		// TF_CLASS_MEDIC
		0,		// TF_CLASS_HEAVYWEAPONS
		0,		// TF_CLASS_PYRO
		1,		// TF_CLASS_SPY
		1,		// TF_CLASS_ENGINEER		
	};

	AssertMsg( iClass >= 0 && iClass < TF_CLASS_COUNT, "Training_GetNumCoursesForClass(): Class out of range!" );
	return s_aClassCourses[ iClass ];
}

int Training_GetNumCourses()
{
	static bool s_bComputed = false;
	static int s_nTotal = 0;

	if ( !s_bComputed )
	{
		for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
		{
			s_nTotal += Training_GetNumCoursesForClass( i );
		}
		s_bComputed = true;
	}

	AssertMsg( s_nTotal == 5, "Number of total courses is incorrect - should be soldier (2) + demo (1) + spy (1) + engy (1)" );

	return s_nTotal;
}

int Training_GetProgressCount()
{
	int aProgress[ TF_CLASS_COUNT ];
	Training_GetProgress( aProgress );

	int nTotalProgress = 0;
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		int nClassProgress = Training_GetClassProgress( i );
		if ( nClassProgress > 0 )
		{
			nTotalProgress += nClassProgress;
		}
	}
	
	return nTotalProgress;
}

bool Training_IsComplete()
{
	return Training_GetProgressCount() == Training_GetNumCourses();
}

void Training_Init()
{
	// If the progress file already exists, early out as we only do conversation from the old system to the new here.
	if ( Training_TrainingProgressFileExists() )
		return;

	int aProgress[ TF_CLASS_COUNT ];

	int fProgressOld = cl_training_completed_with_classes.GetInt();
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		if ( ( fProgressOld & ( 1 << i ) ) != 0 )
		{
			aProgress[ i ] = 1;
		}
		else
		{
			aProgress[ i ] = -1;
		}
	}

	const int TRAINING_CLASS_ATTACK_DEFEND = 15;

	// Add an explicit check for attack/defend
	if ( ( fProgressOld & ( 1 << TRAINING_CLASS_ATTACK_DEFEND ) ) != 0 )
	{
		aProgress[ TF_CLASS_SOLDIER ] = 2;
	}

	// Soldier should always be at least 0
	aProgress[ TF_CLASS_SOLDIER ] = MAX( aProgress[ TF_CLASS_SOLDIER ], 0 );

	// Save a file with the given progress settings
	Training_SaveProgress( aProgress );
}

//------------------------------------------------------------------------------------------------------

Panel *FindAncestorByName( Panel *pChild, const char *pName )
{
	if ( !pChild )
		return NULL;

	Panel *pCurrent = pChild->GetParent();
	while ( pCurrent )
	{
		if ( FStrEq( pCurrent->GetName(), pName ) )
			return pCurrent;

		pCurrent = pCurrent->GetParent();
	}

	return NULL;
}

CExButton *SetupButtonActionSignalTarget( Panel *pParent, const char *pButtonName, const char *pCommand = NULL )
{
	CExButton *pButton = dynamic_cast< CExButton * >( pParent->FindChildByName( pButtonName ) );
	EditablePanel *pTrainingDialog = static_cast< EditablePanel * >( FindAncestorByName( pParent, TRAINING_DIALOG_NAME ) );		Assert( pTrainingDialog );

	if ( pButton && pTrainingDialog )
	{
		if ( pCommand )
		{
			pButton->SetCommand( pCommand );
		}

		pButton->AddActionSignalTarget( pTrainingDialog );
	}

	return pButton;
}

//------------------------------------------------------------------------------------------------------

//
// Sets dialog title/subtitle and sets up cancel/back buttons
//
class CTrainingBasePanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTrainingBasePanel, EditablePanel );
public:
	CTrainingBasePanel( Panel *pParent, const char *pName )
	:	EditablePanel( pParent, pName ),
		m_pPrevPagePanel( NULL )
	{
	}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );

		m_strTitleToken = pInResourceData->GetString( "TrainingTitle", NULL );
		m_strSubTitleToken = pInResourceData->GetString( "TrainingSubTitle", NULL );
	}

	inline bool FindCharInWideString( const wchar_t *pStr, wchar_t c )
	{
		if ( !pStr )
			return false;

		const int nLen = V_wcslen( pStr );
		for ( int i = 0; i < nLen; ++i )
		{
			if ( pStr[ i ] == c )
				return true;
		}

		return false;
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		EditablePanel *pTrainingDialog = static_cast< EditablePanel * >( FindAncestorByName( this, TRAINING_DIALOG_NAME ) );		Assert( pTrainingDialog );
		if ( pTrainingDialog )
		{
			const wchar_t *pTitleString = g_pVGuiLocalize->Find( m_strTitleToken.Get() );
			if ( FindCharInWideString( pTitleString, L'%' ) )
			{
				KeyValues *pTitleFormatData = GetTitleFormatData();		AssertMsg( pTitleFormatData, "Should get valid data here." );
				if ( pTitleFormatData )
				{
					wchar_t wszTitle[ 1024 ];
					g_pVGuiLocalize->ConstructString_safe( wszTitle, m_strTitleToken.Get(), pTitleFormatData );
					pTitleFormatData->deleteThis();

					pTrainingDialog->SetDialogVariable( "title", wszTitle );
				}
			}
			else
			{
				pTrainingDialog->SetDialogVariable( "title", g_pVGuiLocalize->Find( m_strTitleToken.Get() ) );
			}
		
			pTrainingDialog->SetDialogVariable( "subtitle", g_pVGuiLocalize->Find( m_strSubTitleToken ) );
		}
	}

	virtual void OnCommand( const char *pCommand )
	{
		if ( FStrEq( pCommand, "goprev" ) )
		{
			GoPrev();
		}
		else if ( FStrEq( pCommand, "gonext" ) )
		{
			GoNext();
		}
		else
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	virtual void OnKeyCodePressed( KeyCode nCode )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( nCode );

		if ( nCode == KEY_SPACE || nCode == KEY_ENTER || nCode == KEY_XBUTTON_A || nCode == STEAMCONTROLLER_A )
		{
			Go();
		}
		else if ( nButtonCode == KEY_XBUTTON_LEFT || 
				  nButtonCode == KEY_XSTICK1_LEFT ||
				  nButtonCode == KEY_XSTICK2_LEFT ||
				  nButtonCode == STEAMCONTROLLER_DPAD_LEFT ||
				  nButtonCode == KEY_LEFT )
		{
			GoPrev();
		}
		else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
				  nButtonCode == KEY_XSTICK1_RIGHT ||
				  nButtonCode == KEY_XSTICK2_RIGHT || 
				  nButtonCode == STEAMCONTROLLER_DPAD_RIGHT ||
				  nButtonCode == KEY_RIGHT )
		{
			GoNext();
		}
		else
		{
			BaseClass::OnKeyCodePressed( nCode );
		}
	}

	void Go()
	{
		EditablePanel *pTrainingDialog = static_cast< EditablePanel * >( FindAncestorByName( this, TRAINING_DIALOG_NAME ) );		Assert( pTrainingDialog );
		if ( pTrainingDialog )
		{
			const char *pGoCommand = GetGoCommand();
			if ( pGoCommand )
			{
				pTrainingDialog->OnCommand( pGoCommand );
			}
		}
	}

	virtual void GoPrev()
	{
	}

	virtual void GoNext()
	{
	}
	
	virtual void OnBackPressed()
	{
	}

	virtual KeyValues *GetTitleFormatData() const
	{
		return NULL;
	}

	virtual const char *GetGoCommand() const
	{
		return NULL;
	}

	void SetPrevPage( CTrainingBasePanel *pPanel )
	{
		m_pPrevPagePanel = pPanel;
	}

	CTrainingBasePanel *GetPrevPage()
	{
		return m_pPrevPagePanel;
	}

	virtual bool IsFirstPage() const
	{
		return false;
	}

	virtual bool ShouldShowGradient() const
	{
		return false;
	}

protected:
	CUtlString			m_strTitleToken;
	CUtlString			m_strSubTitleToken;
	CTrainingBasePanel	*m_pPrevPagePanel;
};

//------------------------------------------------------------------------------------------------------

class CTrainingBaseCarouselPanel : public CTrainingBasePanel
{
	DECLARE_CLASS_SIMPLE( CTrainingBaseCarouselPanel, CTrainingBasePanel );
public:
	CTrainingBaseCarouselPanel( Panel *pParent, const char *pName )
	:	CTrainingBasePanel( pParent, pName ),
		m_iPage( 0 )
	{
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		CFmtStr fmtCurPageLabelText( "%i/%i", m_iPage + 1, GetNumPages() );
		SetDialogVariable( "curpage", fmtCurPageLabelText.Access() );

		const int nNumPages = GetNumPages();

		// Set visibility on buttons and current page based on the number of pages.
		CExLabel *pCurPageLabel = dynamic_cast< CExLabel * >( FindChildByName( "CurPageLabel" ) );
		if ( pCurPageLabel )
		{
			pCurPageLabel->SetVisible( nNumPages > 1 );
		}

		const char *pNavButtonNames[2] = { "PrevButton", "NextButton" };
		for ( int i = 0; i < 2; ++i )
		{
			CExButton *pCurButton = dynamic_cast< CExButton * >( FindChildByName( pNavButtonNames[ i ] ) );
			if ( !pCurButton )
				continue;
			pCurButton->SetVisible( nNumPages > 1 );
		}
	}

	virtual void OnCommand( const char *pCommand )
	{
		if ( FStrEq( pCommand, "goprev" ) )
		{
			GoPrev();
		}
		else if ( FStrEq( pCommand, "gonext" ) )
		{
			GoNext();
		}
		else
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	virtual void OnKeyCodePressed( KeyCode nCode )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( nCode );

		if ( nButtonCode == KEY_XBUTTON_LEFT || 
			 nButtonCode == KEY_XSTICK1_LEFT ||
			 nButtonCode == KEY_XSTICK2_LEFT || 
			 nButtonCode == KEY_LEFT )
		{
			GoPrev();
		}
		else if ( nButtonCode == KEY_XBUTTON_RIGHT || 
				  nButtonCode == KEY_XSTICK1_RIGHT ||
				  nButtonCode == KEY_XSTICK2_RIGHT || 
				  nButtonCode == KEY_RIGHT )
		{
			GoNext();
		}
		else
		{
			BaseClass::OnKeyCodePressed( nCode );
		}
	}

	void GoPrev()
	{
		--m_iPage;

		if ( m_iPage < 0 )
		{
			m_iPage += GetNumPages();
		}

		InvalidateLayout( false, true );
	}

	void GoNext()
	{
		m_iPage = ( m_iPage + 1 ) % GetNumPages();

		InvalidateLayout( false, true );
	}

	virtual int GetNumPages() const = 0;

protected:
	int		m_iPage;
};

//------------------------------------------------------------------------------------------------------

class CModePanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CModePanel, EditablePanel );
public:
	CModePanel( Panel *pParent, const char *pName )
	:	EditablePanel( pParent, pName ) 
	{
		HScheme hScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
		SetScheme( hScheme );
		SetProportional( true );
	}

	~CModePanel()
	{
	}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );
		
		m_strModeNameToken = pInResourceData->GetString( "modename", NULL );
		m_strDescriptionToken = pInResourceData->GetString( "description", NULL );
		m_strImageToken = pInResourceData->GetString( "image", NULL );
		m_strStartCommand = pInResourceData->GetString( "startcommand", NULL );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/modeselection/modepanel.res" );

		EditablePanel *pContainer = static_cast< EditablePanel * >( FindChildByName( "ModeInfoContainer" ) );
		if ( pContainer )
		{
			pContainer->SetDialogVariable( "modename", g_pVGuiLocalize->Find( m_strModeNameToken.Get() ) );
			pContainer->SetDialogVariable( "description", g_pVGuiLocalize->Find( m_strDescriptionToken.Get() ) );

			EditablePanel *pImageFrame = static_cast< EditablePanel * >( pContainer->FindChildByName( "ImageFrame" ) );
			if ( pImageFrame )
			{
				ImagePanel *pImage = dynamic_cast< ImagePanel * >( pContainer->FindChildByName( "Image" ) );
				if ( pImage )
				{
					pImage->SetImage( m_strImageToken );
					pImage->SetParent( pImageFrame );
				}
			}
		}

		SetupButtonActionSignalTarget( this, "StartButton", m_strStartCommand.Get() );
	}

	virtual void PerformLayout( void )
	{
		BaseClass::PerformLayout();

		GetParent()->NavigateTo();
	}

private:
	CUtlString	m_strModeNameToken;
	CUtlString	m_strDescriptionToken;
	CUtlString	m_strImageToken;
	CUtlString	m_strStartCommand;
};

DECLARE_BUILD_FACTORY( CModePanel );

//------------------------------------------------------------------------------------------------------

class CModeSelectionPanel : public CTrainingBasePanel
{
	DECLARE_CLASS_SIMPLE( CModeSelectionPanel, CTrainingBasePanel );
public:
	CModeSelectionPanel( Panel *pParent, const char *pName )
	:	CTrainingBasePanel( pParent, pName )
	{
		SetProportional( true );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/modeselection/modeselection.res" );
	}

	virtual bool IsFirstPage() const
	{
		return true;
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		Panel *pPanel = FindChildByName( "BasicTrainingPanel" );
		if ( pPanel )
		{
			pPanel->SetNavToRelay( "StartButton" );
			pPanel->SetNavRight( "<<OfflinePracticePanel" );
			pPanel->InvalidateLayout();
		}

		pPanel = FindChildByName( "OfflinePracticePanel" );
		if ( pPanel )
		{
			pPanel->SetNavToRelay( "StartButton" );
			pPanel->SetNavLeft( "<<BasicTrainingPanel" );
			pPanel->InvalidateLayout();
		}

		SetNavToRelay( "BasicTrainingPanel" );
	}
};

DECLARE_BUILD_FACTORY( CModeSelectionPanel );

//------------------------------------------------------------------------------------------------------

class CBasicTraining_ClassPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBasicTraining_ClassPanel, EditablePanel );
public:
	CBasicTraining_ClassPanel( Panel *pParent, const char *pName )
	:	EditablePanel( pParent, pName ),
		m_pImagePanel( NULL ),
		m_pSelectButton( NULL )
	{
		SetProportional( true );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/basictraining/classpanel.res" );

		m_pImagePanel = dynamic_cast< ImagePanel * >( FindChildByName( "Image" ) );		Assert( m_pImagePanel );
		m_pSelectButton = SetupButtonActionSignalTarget( this, "SelectButton" );				Assert( m_pSelectButton );

		if ( m_pSelectButton )
		{
			m_pSelectButton->SetDefaultBorder( pScheme->GetBorder( m_pSelectButton->IsEnabled() ? "MainMenuButtonDefault" : "MainMenuButtonDisabled" ) );
		}

		m_pProgressLabel = dynamic_cast< CExLabel * >( FindChildByName( "ProgressLabel" ) );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_pImagePanel && m_pSelectButton )
		{
			const int nMargin = XRES( 10 );
			const int nButtonStartY = YRES( 215 );

			m_pImagePanel->SetBounds( nMargin, YRES( 20 ), GetWide() - 2 * nMargin, nButtonStartY - YRES( 40 ) );

			int aButtonBounds[4] = {
				nMargin, nButtonStartY, GetWide() - nMargin * 2, (int)YRES( 25 )
			};

			m_pSelectButton->SetBounds( aButtonBounds[0], aButtonBounds[1], aButtonBounds[2], aButtonBounds[3] );

			CExLabel *pProgressLabel = dynamic_cast< CExLabel * >( FindChildByName( "ProgressLabel" ) );
			if ( pProgressLabel )
			{
				int aPos[2];
				pProgressLabel->GetPos( aPos[0], aPos[1] );
				pProgressLabel->SetPos( aButtonBounds[0], aPos[1] );
				pProgressLabel->SetWide( aButtonBounds[2] );
			}
		}

		GetParent()->NavigateTo();
	}

	void SetClassData( int iClass, int nProgress, const char *pImageBase )
	{
		const bool bLocked = nProgress < 0;

		if ( m_pImagePanel )
		{
			CFmtStr fmtImagePath( "%s_%s", pImageBase, bLocked ? "off" : "on" );
			m_pImagePanel->SetImage( fmtImagePath.Access() );
		}

		if ( m_pSelectButton )
		{
			m_pSelectButton->SetEnabled( !bLocked );
		}

		if ( m_pProgressLabel )
		{
			const int nPercent = (int)( 100.0f * nProgress / Training_GetNumCoursesForClass( iClass ) );
			wchar_t wszLocalized[256];
			if ( nPercent > 0 )
			{
				if ( nPercent < 100 )
				{
					wchar_t wszNum[16] = L"";
					V_snwprintf( wszNum, ARRAYSIZE( wszNum ), L"%i", nPercent );
					g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TR_Progress" ), 1, wszNum );
				}
				else
				{
					V_wcsncpy( wszLocalized, g_pVGuiLocalize->Find( "#TR_ProgressDone" ), sizeof( wszLocalized ) );
				}

				m_pProgressLabel->SetText( wszLocalized );
				m_pProgressLabel->SetVisible( true );
			}
			else
			{
				m_pProgressLabel->SetVisible( false );
			}
		}

		InvalidateLayout( true, false );
	}

	void SetSelectCommand( const char *pCommand )
	{
		if ( m_pSelectButton )
		{
			m_pSelectButton->SetCommand( pCommand );
		}
	}

private:
	ImagePanel		*m_pImagePanel;
	CExButton		*m_pSelectButton;
	CExLabel		*m_pProgressLabel;
};

DECLARE_BUILD_FACTORY( CBasicTraining_ClassPanel );

//------------------------------------------------------------------------------------------------------

enum Consts_t
{
	NUM_CLASS_PANELS = 4,
};

const char *g_pClassPanelNames[ NUM_CLASS_PANELS ] =
{
	"SoldierPanel",
	"DemoPanel",
	"SpyPanel",
	"EngineerPanel"
};

class CBasicTraining_ClassSelectionPanel : public CTrainingBasePanel
{
	DECLARE_CLASS_SIMPLE( CBasicTraining_ClassSelectionPanel, CTrainingBasePanel );
public:
	CBasicTraining_ClassSelectionPanel( Panel *pParent, const char *pName )
	:	CTrainingBasePanel( pParent, pName )
	{
		SetProportional( true );

		for ( int i = 0; i < NUM_CLASS_PANELS; ++i )
		{
			m_PanelInfos[ i ].m_pPanel = new CBasicTraining_ClassPanel( this, g_pClassPanelNames[ i ] );
		}
	}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );

		for ( int i = 0; i < NUM_CLASS_PANELS; ++i )
		{
			CFmtStr fmtToken( "Class%iToken", i );
			m_PanelInfos[ i ].m_strSelectButtonToken = pInResourceData->GetString( fmtToken.Access(), NULL );

			CFmtStr fmtImage( "Class%iImage", i );
			m_PanelInfos[ i ].m_strClassImage = pInResourceData->GetString( fmtImage.Access(), NULL );

			CFmtStr fmtCommand( "Class%iCommand", i );
			m_PanelInfos[ i ].m_strCommand = pInResourceData->GetString( fmtCommand.Access(), NULL );
		}

		PRINT_KEY_VALUES( pInResourceData );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/basictraining/classselection.res" );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		const int nWidth = GetWide();
		const int nClassPanelW = nWidth / NUM_CLASS_PANELS;
		const int nClassPanelH = YRES( 260 );

		const int aTrainingClasses[ NUM_CLASS_PANELS ] = {
			TF_CLASS_SOLDIER, TF_CLASS_DEMOMAN, TF_CLASS_SPY, TF_CLASS_ENGINEER
		};


		for ( int i = 0; i < NUM_CLASS_PANELS; ++i )
		{
			CBasicTraining_ClassPanel *pCurClassPanel = m_PanelInfos[ i ].m_pPanel;

			pCurClassPanel->SetBounds(
				i * nClassPanelW,
				0,
				nClassPanelW,
				nClassPanelH
			);

			pCurClassPanel->SetDialogVariable( "selectbuttontext", g_pVGuiLocalize->Find( m_PanelInfos[ i ].m_strSelectButtonToken.Get() ) );

			const int nProgress = Training_GetClassProgress( aTrainingClasses[ i ] );
			pCurClassPanel->SetClassData( aTrainingClasses[ i ], nProgress, m_PanelInfos[ i ].m_strClassImage.Get() );

			pCurClassPanel->SetSelectCommand( m_PanelInfos[ i ].m_strCommand.Get() );

			pCurClassPanel->SetNavToRelay( "SelectButton" );

			char szName[ 64 ];
			if ( i > 0 )
			{
				Panel *pPrevPanel = m_PanelInfos[ i - 1 ].m_pPanel;
				if ( pPrevPanel )
				{
					V_snprintf( szName, sizeof( szName ), "<%s", pPrevPanel->GetName() );
					pCurClassPanel->SetNavLeft( szName );

					V_snprintf( szName, sizeof( szName ), "<%s", pCurClassPanel->GetName() );
					pPrevPanel->SetNavRight( szName );
				}
			}

			pCurClassPanel->InvalidateLayout();
		}

		SetNavToRelay( g_pClassPanelNames[ 0 ] );
	}

	virtual bool ShouldShowGradient() const
	{
		return true;
	}

private:
	struct ClassPanelInfo_t
	{
		CBasicTraining_ClassPanel	*m_pPanel;
		CUtlString					m_strSelectButtonToken;
		CUtlString					m_strClassImage;
		CUtlString					m_strCommand;
	}
	m_PanelInfos[ NUM_CLASS_PANELS ];
};

DECLARE_BUILD_FACTORY( CBasicTraining_ClassSelectionPanel );

//------------------------------------------------------------------------------------------------------

class CBasicTraining_ClassDetailsPanel : public CTrainingBasePanel
{
	DECLARE_CLASS_SIMPLE( CBasicTraining_ClassDetailsPanel, CTrainingBasePanel );
public:
	CBasicTraining_ClassDetailsPanel( Panel *pParent, const char *pName )
	:	CTrainingBasePanel( pParent, pName ),
		m_iClass( TF_CLASS_UNDEFINED ),
		m_pStartTrainingButton( NULL )
	{
		SetProportional( true );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/basictraining/classdetails.res" );

		EditablePanel *pOverlayPanel = dynamic_cast< EditablePanel * >( FindChildByName( "OverlayPanel" ) );
		if ( pOverlayPanel && m_iClass >= TF_FIRST_NORMAL_CLASS && m_iClass < TF_LAST_NORMAL_CLASS )
		{
			pOverlayPanel->SetDialogVariable( "classname", g_pVGuiLocalize->Find( g_aPlayerClassNames[ m_iClass ] ) );

			CFmtStr fmtDescToken( "TR_ClassInfo_%s", m_szClassName );
			pOverlayPanel->SetDialogVariable( "description", g_pVGuiLocalize->Find( fmtDescToken.Access() ) );

			for ( int i = 0; i < 3; ++i )
			{
				CFmtStr fmtWeaponImageName( "WeaponImage%i", i );
				ImagePanel *pCurImage = dynamic_cast< ImagePanel * >( pOverlayPanel->FindChildByName( fmtWeaponImageName.Access() ) );

				if ( pCurImage )
				{
					CFmtStr fmtWeaponImagePath;
					GetWeaponPath( m_iClass, i, fmtWeaponImagePath );
					pCurImage->SetImage( fmtWeaponImagePath.Access() );
				}
			}
		}

		ImagePanel *pClassImage = dynamic_cast< ImagePanel * >( FindChildByName( "ClassImage" ) );
		if ( pClassImage )
		{
			CFmtStr fmtImageName( "training/class_%s_on", m_szClassName );
			pClassImage->SetImage( fmtImageName.Access() );
		}

		ImagePanel *pClassIconImage = dynamic_cast< ImagePanel * >( FindChildByName( "ClassIconImage" ) );
		if ( pClassIconImage )
		{
			CFmtStr fmtImageName( "training/class_icon_%s", m_szClassName );
			pClassIconImage->SetImage( fmtImageName.Access() );
		}

		m_pStartTrainingButton = SetupButtonActionSignalTarget( this, "StartTrainingButton" );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		SetNavToRelay( "StartTrainingButton" );
		NavigateTo();
	}

	void GetWeaponPath( int iClass, int iWeapon, CFmtStr &fmtOut )	// iWeapon is in [0,2]
	{
		static const char *s_pWeaponNames[ TF_CLASS_COUNT ][ 3 ] = {
			{ NULL, NULL, NULL },	// TF_CLASS_UNDEFINED
			{ NULL, NULL, NULL },	// TF_CLASS_SCOUT
			{ NULL, NULL, NULL },	// TF_CLASS_SNIPER
			{ "rocketlauncher", "shotgun", "shovel" },		// TF_CLASS_SOLDIER
			{ "grenadelauncher", "stickybomb_launcher", "bottle" },	// TF_CLASS_DEMOMAN,
			{ NULL, NULL, NULL },	// TF_CLASS_MEDIC
			{ NULL, NULL, NULL },	// TF_CLASS_HEAVYWEAPONS
			{ NULL, NULL, NULL },	// TF_CLASS_PYRO
			{ "revolver", "c_spy_watch", "knife", },		// TF_CLASS_SPY,
			{ "shotgun", "pistol", "wrench" },				// TF_CLASS_ENGINEER,		
		};

		Assert( iClass >= TF_FIRST_NORMAL_CLASS && iClass < TF_CLASS_COUNT );
		Assert( iWeapon >= 0 && iWeapon < 3 );

		if ( iClass == TF_CLASS_SPY && iWeapon == 1 )
		{
			fmtOut.sprintf( "../backpack/weapons/c_models/c_spy_watch/parts/c_spy_watch" );
		}
		else
		{
			fmtOut.sprintf( "../backpack/weapons/w_models/w_%s", s_pWeaponNames[ iClass ][ iWeapon ] );
		}
	}

	virtual bool ShouldShowGradient() const
	{
		return true;
	}

	virtual const char *GetGoCommand() const
	{
		if ( !m_pStartTrainingButton )
			return NULL;
		
		KeyValues *pCommand = m_pStartTrainingButton->GetCommand();
		if ( !pCommand )
			return NULL;

		return pCommand->GetString( "command", NULL );
	}

	void SetClass( const char *pClassName )
	{
		V_strcpy_safe( m_szClassName, pClassName );

		// Setup class details panel
		if ( FStrEq( pClassName, "soldier" ) )
		{
			m_iClass = TF_CLASS_SOLDIER;
		}
		else if ( FStrEq( pClassName, "demoman" ) )
		{
			m_iClass = TF_CLASS_DEMOMAN;
		}
		else if ( FStrEq( pClassName, "spy" ) )
		{
			m_iClass = TF_CLASS_SPY;
		}
		else if ( FStrEq( pClassName, "engineer" ) )
		{
			m_iClass = TF_CLASS_ENGINEER;
		}
		else
		{
			AssertMsg( 0, "Bad class name." );
		}
	}

private:
	char		m_szClassName[16];
	int			m_iClass;
	CExButton	*m_pStartTrainingButton;
};

DECLARE_BUILD_FACTORY( CBasicTraining_ClassDetailsPanel );

//------------------------------------------------------------------------------------------------------

class COfflinePractice_ModeSelectionPanel : public CTrainingBaseCarouselPanel
{
	DECLARE_CLASS_SIMPLE( COfflinePractice_ModeSelectionPanel, CTrainingBaseCarouselPanel );
public:
	COfflinePractice_ModeSelectionPanel( Panel *pParent, const char *pName )
	:	CTrainingBaseCarouselPanel( pParent, pName ),
		m_pGameModeImagePanel( NULL )
	{
		SetProportional( true );
	}

	virtual void ApplySettings( KeyValues *pInResourceData )
	{
		BaseClass::ApplySettings( pInResourceData );

		for ( int i = 0; i < NUM_PRACTICE_MODES; ++i )
		{
			CFmtStr fmtModeToken( "Mode%iToken", i );
			m_ModeInfos[ i ].m_strModeToken = pInResourceData->GetString( fmtModeToken.Access(), NULL );

			CFmtStr fmtDescToken( "Desc%iToken", i );
			m_ModeInfos[ i ].m_strDescToken = pInResourceData->GetString( fmtDescToken.Access(), NULL );

			CFmtStr fmtImagePath( "Image%iPath", i );
			m_ModeInfos[ i ].m_strImage = pInResourceData->GetString( fmtImagePath.Access(), NULL );

			CFmtStr fmtModeId( "Mode%iId", i );
			m_ModeInfos[ i ].m_nId = ( GameMode_t )pInResourceData->GetInt( fmtModeId.Access(), MODE_INVALID );	Assert( m_ModeInfos[ i ].m_nId != MODE_INVALID );
		}

		PRINT_KEY_VALUES( pInResourceData );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		LoadControlSettings( "resource/ui/training/offlinepractice/practicemodeselection.res" );

		m_pGameModeImagePanel = dynamic_cast< ImagePanel * >( FindChildByName( "GameModeImagePanel" ) );
		if ( m_pGameModeImagePanel )
		{
			Assert( m_iPage >= 0 && m_iPage < NUM_PRACTICE_MODES );
			m_pGameModeImagePanel->SetImage( m_ModeInfos[ m_iPage ].m_strImage.Get() );
		}

		SetupButtonActionSignalTarget( this, "SelectCurrentGameModeButton" );

		SetDialogVariable( "description", g_pVGuiLocalize->Find( m_ModeInfos[ m_iPage ].m_strDescToken.Get() ) );
		SetDialogVariable( "gamemode", g_pVGuiLocalize->Find( m_ModeInfos[ m_iPage ].m_strModeToken.Get() ) );

		CFmtStr fmtCurPageLabelText( "%i/%i", m_iPage + 1, NUM_PRACTICE_MODES );
		SetDialogVariable( "curpage", fmtCurPageLabelText.Access() );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_pGameModeImagePanel )
		{
			// Use .res file ypos
			int aPos[2];
			m_pGameModeImagePanel->GetPos( aPos[0], aPos[1] );

			// Center
			m_pGameModeImagePanel->SetPos( ( GetWide() - m_pGameModeImagePanel->GetWide() ) / 2, aPos[1] );
		}

		SetNavToRelay( "SelectCurrentGameModeButton" );
		NavigateTo();
	}

	virtual int GetNumPages() const
	{
		return NUM_PRACTICE_MODES;
	}

	GameMode_t GetMode() const
	{
		return m_ModeInfos[ m_iPage ].m_nId;
	}

private:
	enum Consts_t
	{
		NUM_PRACTICE_MODES = 3,
	};

	struct PracticeModeInfo_t
	{
		CUtlString		m_strModeToken;
		CUtlString		m_strDescToken;
		CUtlString		m_strImage;
		GameMode_t		m_nId;
	}
	m_ModeInfos[ NUM_PRACTICE_MODES ];

	ImagePanel			*m_pGameModeImagePanel;
};

DECLARE_BUILD_FACTORY( COfflinePractice_ModeSelectionPanel );


const char *g_pDifficultyModes[ 4 ] = { "Easy", "Normal", "Hard", "Expert" };

//------------------------------------------------------------------------------------------------------

class COfflinePractice_MapSelectionPanel : public CTrainingBaseCarouselPanel
{
	DECLARE_CLASS_SIMPLE( COfflinePractice_MapSelectionPanel, CTrainingBaseCarouselPanel );

	struct MapInfo_t
	{
		CUtlString	m_strDisplayName;
		CUtlString	m_strName;
		int			m_aPlayerRange[2];
	};

public:
	COfflinePractice_MapSelectionPanel( Panel *pParent, const char *pName )
	:	CTrainingBaseCarouselPanel( pParent, pName ),
		m_pMapImagePanel( NULL ),
		m_pDefaultsData( NULL ),
		m_pDifficultyComboBox( NULL ),
		m_pSavedData( NULL ),
		m_iGameMode( MODE_INVALID )
	{
		SetProportional( true );
		LoadMapData();
	}
	
	~COfflinePractice_MapSelectionPanel()
	{
		for ( int i = 0; i < NUM_GAME_MODES; ++i )
		{
			m_vecMapData[i].PurgeAndDeleteElements();
		}

		if ( m_pDefaultsData )
		{
			m_pDefaultsData->deleteThis();
		}
	}

	void SetGameMode( int iGameMode )
	{
		m_iGameMode = iGameMode;
		m_iPage = 0;
		InvalidateLayout( false, true );
	}

	const MapInfo_t *GetSelectedMapInfo() const
	{
		return m_iGameMode < 0 ? NULL : m_vecMapData[ m_iGameMode ][ m_iPage ];
	}

	int GetMaxPlayers() const
	{
		return GetSelectedMapInfo()->m_aPlayerRange[1];
	}

	const char *GetMapName() const
	{
		return GetSelectedMapInfo()->m_strName.Get();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		LoadControlSettings( "resource/ui/training/offlinepractice/mapselection.res" );

		BaseClass::ApplySchemeSettings( pScheme );

		const MapInfo_t *pCurMapInfo = GetSelectedMapInfo();
		if ( !pCurMapInfo )
			return;

		m_pMapImagePanel = dynamic_cast< ImagePanel * >( FindChildByName( "MapImagePanel" ) );
		if ( m_pMapImagePanel )
		{
			Assert( m_iPage >= 0 && m_iPage < GetMapCount() );

			CFmtStr fmtMapImageBasePath( "training/screenshots/%s.vmt", pCurMapInfo->m_strName.Get() );
			m_pMapImagePanel->SetImage( fmtMapImageBasePath.Access() );
		}

		// Send the 'select' button's command to the actual dialog
		SetupButtonActionSignalTarget( this, "SelectCurrentMapButton" );

		// update recommended number of players
		CExLabel *pSuggestedPlayerCountLabel = dynamic_cast< CExLabel * >( FindChildByName( "SuggestedPlayerCountLabel" ) );
		if ( pSuggestedPlayerCountLabel )
		{
			wchar_t wszLocalized[256];
			wchar_t wszNum1[16]=L"";
			wchar_t wszNum2[16]=L"";
			V_snwprintf( wszNum1, ARRAYSIZE( wszNum1 ), L"%i", pCurMapInfo->m_aPlayerRange[0] );
			V_snwprintf( wszNum2, ARRAYSIZE( wszNum2 ), L"%i", pCurMapInfo->m_aPlayerRange[1] );
			g_pVGuiLocalize->ConstructString_safe( wszLocalized, g_pVGuiLocalize->Find( "#TF_OfflinePractice_NumPlayers" ), 2, wszNum1, wszNum2 );
			pSuggestedPlayerCountLabel->SetText( wszLocalized );
		}

		m_pDifficultyComboBox = dynamic_cast< ComboBox * >( FindChildByName( "DifficultyComboBox" ) );
		if ( m_pDifficultyComboBox )
		{
			for ( int i = 0; i < ARRAYSIZE( g_pDifficultyModes ); ++i )
			{
				m_pDifficultyComboBox->AddItem( g_pDifficultyModes[i], NULL );  
			}
		}

		TextEntry *pNumPlayersTextEntry = dynamic_cast< TextEntry * >( FindChildByName( "NumPlayersTextEntry" ) );
		if ( pNumPlayersTextEntry )
		{
			pNumPlayersTextEntry->SetBorder( pScheme->GetBorder( "ComboBoxBorder" ) );
		}

		SetupButtonActionSignalTarget( this, "StartOfflinePracticeButton" );

		SetDialogVariable( "mapname", pCurMapInfo->m_strDisplayName.Get() );

		UpdateControlsFromSavedData( m_pDifficultyComboBox, pNumPlayersTextEntry );
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_pMapImagePanel )
		{
			// Use .res file ypos
			int aPos[2];
			m_pMapImagePanel->GetPos( aPos[0], aPos[1] );

			// Center
			m_pMapImagePanel->SetPos( ( GetWide() - m_pMapImagePanel->GetWide() ) / 2, aPos[1] );
		}

		SetNavToRelay( "StartOfflinePracticeButton" );
		NavigateTo();
	}

	virtual void OnKeyCodePressed( KeyCode nCode )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( nCode );

		if ( nButtonCode == KEY_XBUTTON_X )
		{
			if ( m_pDifficultyComboBox )
			{
				m_pDifficultyComboBox->SilentActivateItemByRow( ( m_pDifficultyComboBox->GetActiveItem() + 1 ) % ARRAYSIZE( g_pDifficultyModes ) );
			}
		}
		else if ( nButtonCode == KEY_XBUTTON_UP || 
				  nButtonCode == KEY_XSTICK1_UP ||
				  nButtonCode == KEY_XSTICK2_UP || 
				  nButtonCode == KEY_UP )
		{
			SetControlInt( "NumPlayersTextEntry", clamp( GetControlInt( "NumPlayersTextEntry", 0 ) + 1, 1, 31 ) );
		}
		else if ( nButtonCode == KEY_XBUTTON_DOWN || 
				  nButtonCode == KEY_XSTICK1_DOWN ||
				  nButtonCode == KEY_XSTICK2_DOWN || 
				  nButtonCode == KEY_RIGHT )
		{
			SetControlInt( "NumPlayersTextEntry", clamp( GetControlInt( "NumPlayersTextEntry", 0 ) - 1, 1, 31 ) );
		}
		else
		{
			BaseClass::OnKeyCodePressed( nCode );
		}
	}
	
	virtual int GetNumPages() const
	{
		return GetMapCount();
	}

	int GetMapCount() const
	{
		return m_vecMapData[ m_iGameMode ].Count();
	}

	void GetControlValues( int *pOutNumPlayers, int *pOutDiff, CUtlString *pOutMap = NULL )
	{
		const MapInfo_t *pSelectedMapInfo = GetSelectedMapInfo();
		if ( !pSelectedMapInfo )
			return;

		*pOutNumPlayers = clamp( GetControlInt( "NumPlayersTextEntry", 0 ), 1, 31 );

		*pOutDiff = clamp( GetBotDifficulty(), 0, 3 );

		if ( pOutMap )
		{
			*pOutMap = pSelectedMapInfo->m_strName;
		}
	}

	bool DoSetup()
	{
		// @note Tom Bui: if you add any other convars that get set, please revert them
		// in CTFBotManager::RevertOfflinePracticeConvars()

		const MapInfo_t *pSelectedMapInfo = GetSelectedMapInfo();
		if ( !pSelectedMapInfo )
			return false;

		int nQuota = 1;
		int iDifficulty = 0;
		GetControlValues( &nQuota, &iDifficulty );

		// the player count in the dialog includes the human player, so decrease the bot count by one
		ConVarRef tf_bot_quota( "tf_bot_quota" );
		tf_bot_quota.SetValue( nQuota - 1 );

		ConVarRef tf_bot_quota_mode( "tf_bot_quota_mode" );
		tf_bot_quota_mode.SetValue( "normal" );

		ConVarRef tf_bot_auto_vacate( "tf_bot_auto_vacate" );
		tf_bot_auto_vacate.SetValue( 0 );

		ConVarRef tf_bot_difficulty( "tf_bot_difficulty" );
		tf_bot_difficulty.SetValue( iDifficulty );

		ConVarRef tf_bot_offline_practice( "tf_bot_offline_practice" );
		tf_bot_offline_practice.SetValue( 1 );

		tf_training_client_message.SetValue( TRAINING_CLIENT_MESSAGE_WATCHING_INTRO_MOVIE );

		SaveSettings();

		return true;
	}

private:
	virtual KeyValues *GetTitleFormatData() const
	{
		KeyValues *pResult = new KeyValues( "data" );
		if ( pResult )
		{
			const char *pGameModeToken = ( m_iGameMode >= 0 && m_iGameMode < NUM_GAME_MODES ) ? gs_pGameModeTokens[ m_iGameMode ] : "";
			pResult->SetWString( "gametype", g_pVGuiLocalize->Find( pGameModeToken ) );
		}
		return pResult;
	}

	virtual void OnBackPressed()
	{
		SaveSettings();
	}

	void SaveSettings()
	{
		// Save settings
		if ( m_pSavedData )
		{
			int nNumPlayers = 1;
			int iDifficulty = 0;
			CUtlString strMap;
			GetControlValues( &nNumPlayers, &iDifficulty, &strMap );

			m_pSavedData->SetInt( "tf_bot_quota", nNumPlayers );
			m_pSavedData->SetInt( "tf_bot_difficulty", iDifficulty );
			m_pSavedData->SetString( "map", strMap.Get() );
		}

		if ( !m_pSavedData->SaveToFile( g_pFullFileSystem, "OfflinePracticeConfig.vdf", "MOD" ) )
		{
			Warning( "Failed to write save data to OfflinePracticeConfig.vdf!\n" );
		}
	}

	int GetBotDifficulty() const
	{
		if ( m_pDifficultyComboBox )
		{
			return m_pDifficultyComboBox->GetActiveItem();
		}

		AssertMsg( 0, "Shouldn't get here." );
		return 0;
	}

	void UpdateControlsFromSavedData( ComboBox *pDifficultyComboBox, TextEntry *pNumPlayersTextEntry )
	{
		if ( !pDifficultyComboBox )
			return;

		if ( !pNumPlayersTextEntry )
			return;

		int iDifficulty = -1;
		int nQuota = 0;
		const char *defaultMap = "";		

		if ( m_pSavedData )
		{
			m_pSavedData->deleteThis();
			m_pSavedData = NULL;
		}

		m_pSavedData = new KeyValues( "OfflinePracticeConfig" );
	
		// load the config data
		if ( m_pSavedData )
		{
			// this is game-specific data, so it should live in GAME, not CONFIG
			if ( m_pSavedData->LoadFromFile( g_pFullFileSystem, "OfflinePracticeConfig.vdf", "MOD" ) )
			{
				iDifficulty = m_pSavedData->GetInt( "tf_bot_difficulty", -1 );
				nQuota = m_pSavedData->GetInt( "tf_bot_quota", 0 );
				defaultMap = m_pSavedData->GetString( "map", "" );
			}
		}

		if ( m_pDefaultsData )
		{
			const int nMaxPlayers = m_pDefaultsData->GetInt( "max_players" );

			if ( FStrEq( defaultMap, "" ) )
			{
				defaultMap = m_pDefaultsData->GetString( "map", "" );
			}

			if ( nQuota == 0 )
			{
				nQuota = m_pDefaultsData->GetInt( "suggested_players", nMaxPlayers );
			}

			if ( iDifficulty == -1 )
			{
				const char *pDifficultyString = m_pDefaultsData->GetString( "difficulty" );
				if ( pDifficultyString )
				{
					static const char* difficulties [] = { "easy", "normal", "hard", "expert" };
					for ( int i = 0, n = ARRAYSIZE(difficulties); i < n; ++i )
					{
						if ( Q_strcmp( difficulties[i], pDifficultyString ) == 0)
						{
							iDifficulty = i;
							break;
						}
					}
				}
			}
		}

		// Set values in controls
		m_pDifficultyComboBox->SilentActivateItemByRow( iDifficulty );

		CFmtStr fmtNumPlayers( "%i", nQuota );
		pNumPlayersTextEntry->SetText( fmtNumPlayers.Access() );
	}

	void LoadMapData()
	{
		m_pOfflinePracticeData = new KeyValues( "offline_practice.res" );
		const char *pFilename = "resource/offline_practice.res";
		if ( !m_pOfflinePracticeData->LoadFromFile( g_pFullFileSystem, pFilename, "MOD" ) )
		{
			Warning( "Could not load %s!\n", pFilename );
			return;
		}

		// Save defaults
		KeyValues *pDefaultsData = m_pOfflinePracticeData->FindKey( "defaults" );
		if ( pDefaultsData )
		{
			m_pDefaultsData = pDefaultsData->MakeCopy();
		}

		KeyValues *pMapData = m_pOfflinePracticeData->FindKey( "maps" );
		if ( pMapData )
		{
			FOR_EACH_TRUE_SUBKEY( pMapData, pCurMap )
			{
				MapInfo_t *pMapInfo = new MapInfo_t;

				pMapInfo->m_strName = pCurMap->GetName();
				pMapInfo->m_strDisplayName = pCurMap->GetString( "name" );
				pMapInfo->m_aPlayerRange[0] = pCurMap->GetInt( "min_players" );
				pMapInfo->m_aPlayerRange[1] = pCurMap->GetInt( "max_players" );

				// Figure out which bucket to add to
				const GameMode_t iGameMode = GetGameModeFromMapName( pMapInfo->m_strName.Get() );
				if ( iGameMode != MODE_INVALID )
				{
					AddMapInfo( pMapInfo, iGameMode );
				}
			}
		}

		pMapData->deleteThis();
	}

	GameMode_t GetGameModeFromMapName( const char *pMapName )
	{
		if ( !V_strnicmp( pMapName, "cp", 2 ) )
		{
			return MODE_CP;
		}
		else if ( !V_strnicmp( pMapName, "koth", 4 ) )
		{
			return MODE_KOTH;
		}
		else if ( !V_strnicmp( pMapName, "pl", 2 ) )
		{
			return MODE_PL;
		}

		AssertMsg( 0, "Should never get here!" );

		return MODE_INVALID;
	}

	void AddMapInfo( MapInfo_t *pMapInfo, GameMode_t iGameMode )
	{
		m_vecMapData[ iGameMode ].AddToTail( pMapInfo );
	}

	int							m_iGameMode;
	KeyValues					*m_pSavedData;
	KeyValues					*m_pDefaultsData;
	ImagePanel					*m_pMapImagePanel;
	ComboBox					*m_pDifficultyComboBox;
	KeyValues					*m_pOfflinePracticeData;
	CUtlVector< MapInfo_t * >	m_vecMapData[ NUM_GAME_MODES ];
};

DECLARE_BUILD_FACTORY( COfflinePractice_MapSelectionPanel );

//------------------------------------------------------------------------------------------------------

class CTrainingDialog : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTrainingDialog, EditablePanel );
public:
	CTrainingDialog( Panel *parent ) 
	:	EditablePanel( parent, TRAINING_DIALOG_NAME ),
		m_pBackButton( NULL ),
		m_pCancelButton( NULL ),
		m_pGradientBgPanel( NULL ),
		m_pModeSelectionPanel( NULL ),
		m_pCurrentPagePanel( NULL ),
		m_pBasicTraining_ClassSelectionPanel( NULL ),
		m_pBasicTraining_ClassDetailsPanel( NULL ),
		m_pOfflinePractice_ModeSelectionPanel( NULL ),
		m_pOfflinePractice_MapSelectionPanel( NULL ),
		m_pTrainingData( NULL ),
		m_bStartTraining( false ),
		m_bContinue( false )
	{
		HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
		SetScheme(scheme);
		SetProportional( true );
		m_pContainer = new EditablePanel( this, "Container" );

		// load configuration
		const char *filename = "resource/training.res";
		m_pTrainingData = new KeyValues( "training.res" );
		Assert( m_pTrainingData );
		if ( !m_pTrainingData->LoadFromFile( g_pFullFileSystem, filename, "MOD" ) )
		{
			Warning( "Unable to load '%s'\n", filename );
			AssertMsg( 0, "Couldn't load training data!" );
		}

		C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_open", "training" );
	}

	virtual ~CTrainingDialog()
	{
		C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_close", "training" );

		ivgui()->RemoveTickSignal( GetVPanel() );
	}

	virtual void SetDialogVariable( const char *pVarName, const char *pValue )
	{
		m_pContainer->SetDialogVariable( pVarName, pValue );
	}

	virtual void SetDialogVariable( const char *pVarName, const wchar_t *pValue )
	{
		m_pContainer->SetDialogVariable( pVarName, pValue );
	}

	virtual void SetDialogVariable( const char *pVarName, int nValue )
	{
		m_pContainer->SetDialogVariable( pVarName, nValue );
	}

	virtual void SetDialogVariable( const char *pVarName, float flValue )
	{
		m_pContainer->SetDialogVariable( pVarName, flValue );
	}

	void SetupButton( const char *pPanelName, CExButton **ppOut = NULL )
	{
		Panel *pPanel = m_pContainer->FindChildByName( pPanelName );
		if ( pPanel )
		{
			pPanel->AddActionSignalTarget( this );
		}

		if ( ppOut )
		{
			*ppOut = static_cast< CExButton * >( pPanel );
		}
	}

	virtual void Show()
	{
		SetVisible( true );
		MakePopup();
		MoveToFront();
		SetKeyBoardInputEnabled( true );
		SetMouseInputEnabled( true );
		TFModalStack()->PushModal( this );
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();
	}

	virtual void OnCommand( const char *pCommand )
	{
		C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(training)", pCommand );

		if ( FStrEq( pCommand, "prevpage" ) )
		{
			ShowPrevPage();
		}
		else if ( FStrEq( pCommand, "cancel" ) )
		{
			Close();
		}
		else if ( FStrEq( pCommand, "basictrainingselected" ) )
		{
			BasicTraining_ShowClassSelection();
		}
		else if ( FStrEq( pCommand, "offlinepracticeselected" ) )
		{
			OfflinePractice_ShowPracticeMode();
		}
		else if ( FStrEq( pCommand, "startbasictraining" ) )
		{
			BasicTraining_Start();
		}
		else if ( !V_strnicmp( pCommand, "basictraining_classselection_", 29 ) )
		{
			BasicTraining_ShowClassDetailsPage( pCommand + 29 );
		}
		else if ( FStrEq( pCommand, "selectcurrentgamemode" ) )
		{
			OfflinePractice_ShowMapSelection();
		}
		else if ( FStrEq( pCommand, "startofflinepractice" ) )
		{
			OfflinePractice_Start();
		}
		else 
		{
			BaseClass::OnCommand( pCommand );
		}
	}

	void SetCurrentPage( CTrainingBasePanel *pPanel, bool bGoingBack = false )
	{
		AssertMsg( pPanel, "Setting current page to NULL!" );

		pPanel->SetVisible( true );

		if ( !bGoingBack )
		{
			pPanel->SetPrevPage( m_pCurrentPagePanel );
		}

		m_pCurrentPagePanel = pPanel;
		m_pCurrentPagePanel->InvalidateLayout( false, true );

		InvalidateLayout( true, false );
	}

	void ShowPrevPage()
	{
		CTrainingBasePanel *pPrevPagePanel = m_pCurrentPagePanel->GetPrevPage();
		if ( pPrevPagePanel )
		{
			if ( m_pCurrentPagePanel == pPrevPagePanel )
				return;

			m_pCurrentPagePanel->SetVisible( false );
			m_pCurrentPagePanel->OnBackPressed();
			SetCurrentPage( pPrevPagePanel, true );
		}
		else
		{
			OnCommand( "cancel" );
		}
	}

	void HideCurrentPage()
	{
		m_pCurrentPagePanel->SetVisible( false );
	}

	void BasicTraining_ShowClassSelection()
	{
		if ( m_pCurrentPagePanel == m_pBasicTraining_ClassSelectionPanel )
			return;

		HideCurrentPage();
		SetCurrentPage( m_pBasicTraining_ClassSelectionPanel );
	}

	int GetClassFromData( KeyValues *pClassData )
	{
		const int iClass = pClassData->GetInt( "class", TF_CLASS_SOLDIER );
		if ( iClass < TF_FIRST_NORMAL_CLASS || iClass >= TF_LAST_NORMAL_CLASS )
		{
			return TF_CLASS_SOLDIER;
		}
		return iClass;
	}

	static void ConfirmDialogCallback( bool bConfirmed, void *pContext )
	{
		CTrainingDialog *pDialog = ( CTrainingDialog * )pContext;
		if ( pDialog )
		{
			pDialog->m_bContinue = bConfirmed;
			pDialog->m_bStartTraining = true;
		}
	}

	void ConfirmContinue()
	{
		ShowConfirmDialog( "#TR_ContinueTitle", "#TR_ContinueMsg", "#TR_Continue", "#TR_StartOver", &ConfirmDialogCallback, NULL, this ); 
	}

	void BasicTraining_Start()
	{
		if ( !m_pTrainingData )
			return;

		KeyValues *pData = m_pTrainingData->FindKey( m_strBasicTrainingClassName.Get() );
		if ( !pData )
			return;

		// Override for soldier - if target practice is complete, start from
		const int iClass = GetClassFromData( pData );
		int nProgress = Training_GetClassProgress( iClass );
		if ( iClass == TF_CLASS_SOLDIER && nProgress >= 1 )
		{
			ConfirmContinue();
			return;
		}

		m_bStartTraining = true;
		m_bContinue = false;
	}

	virtual void Think()
	{
		if ( !m_bStartTraining )
			return;

		KeyValues *pData = m_pTrainingData->FindKey( m_strBasicTrainingClassName.Get() );
		if ( !pData )
			return;

		// Override map if user has selected to continue
		const char *pMapName = pData->GetString( "map", NULL );
		if ( m_bContinue )
		{
			pMapName = "tr_dustbowl";
		}

		if ( pMapName )
		{
			const int iClass = GetClassFromData( pData );

			ConVarRef training_class( "training_class" );
			training_class.SetValue( iClass );

			const char* pMapVideo = pData->GetString( "video", "" );
			training_map_video.SetValue( pMapVideo );

			// create the command to execute
			CFmtStr fmtMapCommand( "disconnect\nwait\nwait\n\nprogress_enable\nmap %s\n", pMapName );

			// exec
			engine->ClientCmd_Unrestricted( fmtMapCommand.Access() );
		}

		Close();
	}

	void BasicTraining_ShowClassDetailsPage( const char *pClassName )
	{
		if ( m_pCurrentPagePanel == m_pBasicTraining_ClassDetailsPanel )
			return;

		HideCurrentPage();

		m_pBasicTraining_ClassDetailsPanel->SetClass( pClassName );
		m_pBasicTraining_ClassDetailsPanel->InvalidateLayout( true, true );

		SetCurrentPage( m_pBasicTraining_ClassDetailsPanel );

		// Cache class
		m_strBasicTrainingClassName = pClassName;
	}

	void OfflinePractice_ShowPracticeMode()
	{
		if ( m_pCurrentPagePanel == m_pOfflinePractice_ModeSelectionPanel )
			return;

		HideCurrentPage();
		SetCurrentPage( m_pOfflinePractice_ModeSelectionPanel );
	}

	void OfflinePractice_ShowMapSelection()
	{
		if ( m_pCurrentPagePanel == m_pOfflinePractice_MapSelectionPanel )
			return;

		// Pass on the game mode to the map selection panel so it will only show corresponding maps
		const GameMode_t nGameMode = m_pOfflinePractice_ModeSelectionPanel->GetMode();
		m_pOfflinePractice_MapSelectionPanel->SetGameMode( nGameMode );

		HideCurrentPage();
		SetCurrentPage( m_pOfflinePractice_MapSelectionPanel );
	}

	void OfflinePractice_Start()
	{
		// reset server enforced cvars
		g_pCVar->RevertFlaggedConVars( FCVAR_REPLICATED );	
		
		// Cheats were disabled; revert all cheat cvars to their default values.
		// This must be done heading into multiplayer games because people can play
		// demos etc and set cheat cvars with sv_cheats 0.
		g_pCVar->RevertFlaggedConVars( FCVAR_CHEAT );
		
		DevMsg( "FCVAR_CHEAT cvars reverted to defaults.\n" );
		
		if ( m_pOfflinePractice_MapSelectionPanel->DoSetup() )
		{
			// create the command to execute
			CFmtStr1024 fmtMapCommand(
				"disconnect\nwait\nwait\nmaxplayers %i\n\nprogress_enable\nmap %s\n",
				m_pOfflinePractice_MapSelectionPanel->GetMaxPlayers(),
				m_pOfflinePractice_MapSelectionPanel->GetMapName()
			);

			// exec
			engine->ClientCmd_Unrestricted( fmtMapCommand.Access() );
		}

		Close();
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );
		
		LoadControlSettings( "Resource/ui/training/main.res" );
		
		SetupButton( "CancelButton", &m_pCancelButton );
		SetupButton( "BackButton", &m_pBackButton );

		m_pModeSelectionPanel = dynamic_cast< CModeSelectionPanel * >( m_pContainer->FindChildByName( "ModeSelectionPanel" ) );	Assert( m_pModeSelectionPanel );
		m_pBasicTraining_ClassSelectionPanel = dynamic_cast< CBasicTraining_ClassSelectionPanel * >( m_pContainer->FindChildByName( "BasicTraining_ClassSelectionPanel" ) );	Assert( m_pBasicTraining_ClassSelectionPanel );
		m_pBasicTraining_ClassDetailsPanel = dynamic_cast< CBasicTraining_ClassDetailsPanel * >( m_pContainer->FindChildByName( "BasicTraining_ClassDetailsPanel" ) );	Assert( m_pBasicTraining_ClassDetailsPanel );
		m_pOfflinePractice_ModeSelectionPanel = dynamic_cast< COfflinePractice_ModeSelectionPanel * >( m_pContainer->FindChildByName( "OfflinePractice_ModeSelectionPanel" ) );	Assert( m_pOfflinePractice_ModeSelectionPanel );
		m_pOfflinePractice_MapSelectionPanel = dynamic_cast< COfflinePractice_MapSelectionPanel * >( m_pContainer->FindChildByName( "OfflinePractice_MapSelectionPanel" ) );	Assert( m_pOfflinePractice_MapSelectionPanel );
		m_pGradientBgPanel = dynamic_cast< ImagePanel * >( m_pContainer->FindChildByName( "GradientBgPanel" ) );

		m_pCurrentPagePanel = m_pModeSelectionPanel;
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		if ( m_pCurrentPagePanel )
		{
			m_pCurrentPagePanel->SetVisible( true );

			const bool bFirstPage = m_pCurrentPagePanel->IsFirstPage();
			if ( m_pBackButton && m_pCancelButton )
			{
				m_pBackButton->SetVisible( !bFirstPage );

				int w = m_pContainer->GetWide();
				const int nBuffer = XRES( 5 );

				int cbx, cby;
				m_pCancelButton->GetPos( cbx, cby );

				if ( bFirstPage )
				{
					m_pCancelButton->SetPos( ( w - m_pCancelButton->GetWide() ) / 2, cby );
				}
				else
				{
					m_pBackButton->SetPos( w/2 - m_pBackButton->GetWide() - nBuffer, cby );
					m_pCancelButton->SetPos( w/2 + nBuffer, cby );
				}

				if ( m_pGradientBgPanel )
				{
					m_pGradientBgPanel->SetVisible( m_pCurrentPagePanel->ShouldShowGradient() );
				}
			}
		}
	}

	virtual void OnKeyCodePressed( KeyCode code )
	{
		ButtonCode_t nButtonCode = GetBaseButtonCode( code );
		if ( code == KEY_ESCAPE )
		{
			OnCommand( "cancel" );
		}
		else if ( nButtonCode == KEY_XBUTTON_B || nButtonCode == STEAMCONTROLLER_B )
		{
			OnCommand( "prevpage" );
		}
		else if ( code == KEY_ENTER || code == KEY_SPACE || nButtonCode == KEY_XBUTTON_A || nButtonCode == STEAMCONTROLLER_A )
		{
			if ( m_pCurrentPagePanel )
			{
				m_pCurrentPagePanel->Go();
			}
		}
		else
		{
			BaseClass::OnKeyCodePressed( code );
		}
	}

protected:
	void Close()
	{
		SetVisible( false );
		TFModalStack()->PopModal( this );
		MarkForDeletion();
	}

private:
	EditablePanel		*m_pContainer;
	CModeSelectionPanel	*m_pModeSelectionPanel;
	CBasicTraining_ClassSelectionPanel	*m_pBasicTraining_ClassSelectionPanel;
	CBasicTraining_ClassDetailsPanel	*m_pBasicTraining_ClassDetailsPanel;
	COfflinePractice_ModeSelectionPanel	*m_pOfflinePractice_ModeSelectionPanel;
	COfflinePractice_MapSelectionPanel	*m_pOfflinePractice_MapSelectionPanel;
	CTrainingBasePanel	*m_pCurrentPagePanel;
	CTrainingBasePanel	*m_pPrevPagePanel;
	CExButton			*m_pCancelButton;
	CExButton			*m_pBackButton;
	ImagePanel			*m_pGradientBgPanel;
	KeyValues			*m_pTrainingData;
	CUtlString			m_strBasicTrainingClassName;
	bool				m_bStartTraining;
	bool				m_bContinue;
};

static DHANDLE<CTrainingDialog> g_pTrainingDialog;

//------------------------------------------------------------------------------------------------------

void CL_ShowTrainingDialog( const CCommand &args )
{
	if ( g_pTrainingDialog.Get() == NULL )
	{
		IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
		g_pTrainingDialog = new CTrainingDialog( (CHudMainMenuOverride*)pMMOverride );
		g_pTrainingDialog->InvalidateLayout( true, true );
	}
	g_pTrainingDialog->Show();
}

//------------------------------------------------------------------------------------------------------

CON_COMMAND( cl_training_class_unlock_all, "Unlock all training" )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		Training_MarkClassComplete( i, 100 );
	}
}

//------------------------------------------------------------------------------------------------------

#ifdef _DEBUG
CON_COMMAND( training_set, 0 )
{
	if ( args.ArgC() != 3 )
	{
		Warning( "Not enough arguments\n" );
		return;
	}

	Training_MarkClassComplete( atoi( args[1] ), atoi( args[2] ) );
}
#endif

//------------------------------------------------------------------------------------------------------

static ConCommand training_showdlg( "training_showdlg", &CL_ShowTrainingDialog, "Displays the training dialog." );