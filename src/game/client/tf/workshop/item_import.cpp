//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
//
// TODO:
// * Bring in Jeff Hameluck's translucency support:
//       QC with any $translucent 1 VMT should have $mostlyopaque
// * Add help icons with tooltips on valid asset constraints
//
// Maybe TODO:
// * Add wireframe view to preview
// * Fix issues with gesture support (no animation blending/easing, no weapon state logic)
//
//=============================================================================

#include "cbase.h"

#include "tier2/tier2.h"
#include "tier2/p4helpers.h"
#include "tier2/fileutils.h"
#include "ienginevgui.h"
#include "imageutils.h"
#include "filesystem.h"
#include "vgui_controls/FileOpenDialog.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "vgui_bitmapimage.h"
#include "bitmap/bitmap.h"
#include "itemtest/itemtest.h"

#include "econ_controls.h"
#include "econ_item_system.h"
#include "tf_item_inventory.h"
#include "c_tf_player.h"
#include "confirm_dialog.h"
#include "steampublishedfiles/publish_file_dialog.h"
#include "vgui/store/v2/tf_store_preview_item2.h"
#include "vgui/tf_playermodelpanel.h"
#include "bone_setup.h"

#include "scenefilecache/ISceneFileCache.h"

#include "workshop/item_import.h"

#include "choreoscene.h"
#include "choreoactor.h"
#include "choreochannel.h"

#include "../public/zip_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


static const char *kClassFolders[TF_LAST_NORMAL_CLASS] = {
	"",			// TF_CLASS_UNDEFINED
	"scout",	// TF_CLASS_SCOUT
	"sniper",	// TF_CLASS_SNIPER,
	"soldier",	// TF_CLASS_SOLDIER,
	"demo",		// TF_CLASS_DEMOMAN,
	"medic",	// TF_CLASS_MEDIC,
	"heavy",	//TF_CLASS_HEAVYWEAPONS,
	"pyro",		// TF_CLASS_PYRO,
	"spy",		// TF_CLASS_SPY,
	"engineer",	// TF_CLASS_ENGINEER,
};
static const char *kClassFolderMulticlass = "all_class";

static const int NUM_IMPORT_LODS = 3;
static const int NUM_IMPORT_MATERIALS_PER_TEAM = 2;
static const int MAX_MATERIAL_COUNT = NUM_IMPORT_MATERIALS_PER_TEAM * 2;
static const int MAX_TEXT_EDIT_SIZE = 8192;
static const int MAX_TAUNT_DURATION = 30.f;

static const int kStartWorkshopItemIndex = 30000;

static const char *kBodygroupArray[] =
{
	"backpack",
	"dogtags",
	"grenades",
	"hands",
	"hat",
	"head",
	"headphones",
	"shoes",
	"shoes_socks",
	"bullets"
};

extern IFileSystem *g_pFullFileSystem;

// This should be a constant since it's in the game directory hierarchy now
//static ConVar tf_steam_workshop_dir( "tf_steam_workshop_dir", "workshop", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Directory used to hold import sessions for the Steam Workshop." );
static const char *kSteamWorkshopDir = "%s/import_source";
static const char *kWorkshopSchemaFile = "scripts/items/unencrypted/_items_workshop.txt";
static const char *kWorkshopSoundScriptFile = "scripts/game_sounds_taunt_workshop.txt";

static ConVar tf_steam_workshop_import_icon_path( "tf_steam_workshop_import_icon_path", "", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Default location to load backpack icons from" );
static ConVar tf_steam_workshop_import_model_path( "tf_steam_workshop_import_model_path", "", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Default location to load models from" );
static ConVar tf_steam_workshop_import_material_path( "tf_steam_workshop_import_material_path", "", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Default location to load materials from" );


// Item data tokens
static const char *kItemName = "name";
static const char *kWorkshopName = "import_name";
static const char *kTFEnglishName = "tf_english_name";
static const char *kItemPrefab = "prefab";
static const char *kItemIcon = "icon";
static const char *kItemPaintable = "paintable%d";
static const char *kAnimationLoopable = "loopable";
static const char *kClassQC = "Classes/%s/QC";
static const char *kClassQCI = "Classes/%s/QCI";
static const char *kClassLODN = "Classes/%s/LOD%d";
static const char *kClassLODNFile = "Classes/%s/LOD%d/file";
static const char *kClassAnimation = "Classes/%s/Animation";
static const char *kClassAnimationSourceFile = "Classes/%s/Animation/source_file";
static const char *kClassAnimationVCDFile = "Classes/%s/Animation/vcd_file";
static const char *kClassAnimationDuration = "Classes/%s/Animation/taunt_duration";
static const char *kMaterialN = "Materials/Material%d";
static const char *kMaterialSkinN = "Materials/Skins/%s/Material%d";
static const char *kMaterialSkinNFile = "Materials/Skins/%s/Material%d/%s_texture_file";
static const char *kMaterialSkinNVMT = "Materials/Skins/%s/Material%d/vmt";
static const char *kItemSchema = "ItemSchema";
static const char *kBuildOutput = "output";
static const char *kSkinType = "skin_type";
static const char *kEquipRegion = "equip_region";
static const char *kBodygroup = "bodygroup";
static const char *kPaintDefIndex = "paint_def_index";
static const char *kWorkshopID = "workshop_id";

// The default triangle budget for items
static const int kDefaultTriangleBudget[] = { 1400, 1000, 700 };
COMPILE_TIME_ASSERT( ARRAYSIZE( kDefaultTriangleBudget ) == NUM_IMPORT_LODS );

static const int kDefaultBoneBudget = 5;

static const char *kBuildResultMessages[] = {
	"",
	"#TF_ImportFile_BuildFailedNoSDK",
	"#TF_ImportFile_BuildFailedNoName",
	"#TF_ImportFile_BuildFailedNoType",
	"#TF_ImportFile_BuildFailedNoModels",
	"#TF_ImportFile_BuildFailedNumLODMismatch",
	"#TF_ImportFile_BuildFailedNoMaterials",
	"#TF_ImportFile_BuildFailedNumMaterialMismatch",
	"#TF_ImportFile_BuildFailedNoBackpackIcon",
	"#TF_ImportFile_BuildFailedBadName",
	"TF English Name already exists in item schema.",
	"#TF_ImportFile_BuildFailedBadType",
	"#TF_ImportFile_BuildFailedBadModel",
	"#TF_ImportFile_BuildFailedBadMaterialType",
	"#TF_ImportFile_BuildFailedBadMaterial",
	"#TF_ImportFile_BuildFailedMaterialMissingShader",
	"#TF_ImportFile_BuildFailedMaterialMissingCloak",
	"#TF_ImportFile_BuildFailedMaterialMissingBurning",
	"#TF_ImportFile_BuildFailedMaterialMissingJarate",
	"#TF_ImportFile_BuildFailedMaterialMissingPaintable",
	"#TF_ImportFile_BuildFailedMissingModel",
	"#TF_ImportFile_BuildFailedNeedMoreLOD",
	"#TF_ImportFile_BuildFailedComplexModel",
	"#TF_ImportFile_BuildFailedBadImage",
	"#TF_ImportFile_BuildFailedCompile",
	"#TF_ImportFile_BuildFailedNoWorkshopID",
	"#TF_ImportFile_ImageUnsupportedFileType",
	"#TF_ImportFile_ImageResolutionNotPowerOf2",
	"#TF_ImportFile_ImageResolutionOverLimit",
	"#TF_ImportFile_BuildFailedNoTaunts",
	"#TF_ImportFile_BuildFailedBadVCD",
	"#TF_ImportFile_BuildFailedVCDMissingEventSequence",
	"#TF_ImportFile_BuildFailedVCDEventSequenceTooLong",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kBuildResultMessages ) == CTFFileImportDialog::NUM_BUILD_RESULTS );

static const char *kLoadResultMessages[ CTFFileImportDialog::NUM_LOAD_RESULTS ] = {
	"",
	"#TF_ImportFile_LoadFailedBadFile",
	"#TF_ImportFile_LoadFailedBadFile",
	"#TF_ImportFile_BuildFailedComplexModel",
	"#TF_ImportFile_LoadFailedTooManyBones",
	"#TF_ImportFile_LoadFailedBadFile",
	"#TF_ImportFile_LoadFailedTooManyMaterials",
	"#TF_ImportFile_LoadFailedMaterialCountMismatch",
	"#TF_ImportFile_LoadFailedAnimationTooLong",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kLoadResultMessages ) == CTFFileImportDialog::NUM_LOAD_RESULTS );

static const char *kSaveResultMessages[ CTFFileImportDialog::NUM_SAVE_RESULTS ] = {
	"",
	"#TF_ImportFile_SaveFailedBadFile",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kSaveResultMessages ) == CTFFileImportDialog::NUM_SAVE_RESULTS );

static const char *kWarningMessages[ CTFFileImportDialog::NUM_WARNINGS ] =
{
	"TF_ImportFile_Warning_BaseAlphaMask",
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kWarningMessages ) == CTFFileImportDialog::NUM_WARNINGS );

static const char *kLODLevels[] =
{
	"HIGH LOD",
	"MEDIUM LOD",
	"LOW LOD"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kLODLevels ) == NUM_IMPORT_LODS );

static const char *kPrefabs[] =
{
	"hat",
	"misc",
	"taunt"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( kPrefabs ) == CTFFileImportDialog::PREFAB_COUNT );

static const char* kDefaultPrefab = "hat";
static const char* kDefaultEquipRegion = "hat";

// Poses that your character can do in the item preview
static const char *kModelPoses[] = {
	"REF",
	"STAND",
	"CROUCH",
	"RUN",
};
static const int kDefaultModelPoseIndex = 1;

static const char *kModelActions[] = {
	"taunt_laugh.vcd",
	// The attack and reload animations don't work properly with the Heavy and Pyro
	//"gesture_attack",
	//"gesture_reload",
};
static const int kDefaultModelActionIndex = 0;

static const char* s_pszMaterialFilePrefixes[] =
{
	"base",
	"normal",
	"phongexponent",
	"selfillum"
};
COMPILE_TIME_ASSERT( ARRAYSIZE( s_pszMaterialFilePrefixes ) == NUM_MATERIAL_TEXTURE_FILE_TYPE );

// Each preview needs a unique model name so it actually gets reloaded if it changes
static int s_nLastPreviewModel;

static const char* kDefaultBumpmap = "models/player/shared/shared_normal";

struct LightwarpInfo_t
{
	const char *pszName;
	const char *pszPath;
};

static LightwarpInfo_t kLightwarps[] =
{
	{ "pyro", "models/player/pyro/pyro_lightwarp" },
	{ "weapon", "models/lightwarps/weapon_lightwarp" },
	{ "robot", "models/lightwarps/robot_lightwarp" },
	{ "sentry1", "models/buildables/sentry1/sentry1_lightwarp" },
	{ "sentry2", "models/buildables/sentry2/sentry2_lightwarp" },
	{ "sentry3", "models/buildables/sentry3/sentry3_lightwarp" },
	{ "ambassador", "models/weapons/c_items/c_ambassador_lightwarp" },
	{ "jarate", "models/lightwarps/jarate_lightwarp" },
};
static const char *kLightwarpPath = "lightwarp_path";

struct EnvmapInfo_t
{
	const char *pszName;
	const char *pszPath;
};

static EnvmapInfo_t kEnvmaps[] =
{
	{ "none", "" },
	{ "world cube map", "env_cubemap" },
	{ "saxxy gold", "effects/saxxy/saxxy_gold" },
	{ "pyro land goggles", "effects/pyrocube" }
};
static const char *kEnvmapPath = "envmap_path";

struct EnvmapMaskInfo_t
{
	const char *pszDisplayName;
	const char *pszVarName;
};

static EnvmapMaskInfo_t kEnvmapMasks[] =
{
	{ "none", "" },
	{ "base alpha", "$basealphaenvmapmask" },
	{ "normal map alpha", "$normalmapalphaenvmapmask" }
};
static const char *kEnvmapMaskVarName = "envmapmask_varname";


static float GetMaxTauntDuration()
{
	return MAX_TAUNT_DURATION;
}


static const char *GetWorkshopSoundScriptFile()
{
	static char szCorrectCaseFilePath[MAX_PATH];
	char szItemWorkshopSoundScriptAbsPath[MAX_PATH];
	if ( GenerateFullPath( kWorkshopSoundScriptFile, "MOD", szItemWorkshopSoundScriptAbsPath, ARRAYSIZE( szItemWorkshopSoundScriptAbsPath ) ) )
	{
		g_pFullFileSystem->GetCaseCorrectFullPath( szItemWorkshopSoundScriptAbsPath, szCorrectCaseFilePath );
	}
	else
	{
		Warning( "Failed to GenerateFullPath %s\n", kWorkshopSoundScriptFile );
		return NULL;
	}

	return szCorrectCaseFilePath;
}


//-----------------------------------------------------------------------------
static void SetMessageFileVariable( KeyValues *pData, const char *pszFilePath )
{
	if ( !pData )
		return;

	wchar_t unicodeFile[MAX_PATH];
	g_pVGuiLocalize->ConvertANSIToUnicode( pszFilePath, unicodeFile, sizeof(unicodeFile) );
	pData->SetWString( "file", unicodeFile );
}

//-----------------------------------------------------------------------------
static void ShowMessageBoxWithFile( const char *pTitle, const char *pText, const char *pszFilePath)
{
	KeyValuesAD pData( "data" );
	SetMessageFileVariable( pData, pszFilePath );
	ShowMessageBox( pTitle, pText, pData );
}

static void SaveBrowsePath( ConVar &conVar, const char *pszFilePath )
{
	char pszDirPath[MAX_PATH];
	V_ExtractFilePath( pszFilePath, pszDirPath, sizeof(pszDirPath) );
	conVar.SetValue( pszDirPath );
}

//-----------------------------------------------------------------------------
// Purpose: Save build output for presenting to the user
//-----------------------------------------------------------------------------
class CBuildLog : public CItemLog
{
public:
	CBuildLog() : CItemLog(), m_log( 0, 0, CUtlBuffer::TEXT_BUFFER ) { }

	virtual void Log( ItemtestLogLevel_t nLogLevel, const char *pszMessage ) const
	{
		static const char *pszIgnoredMessages[] = {
			"CDynamicFunction: "
		};

		// See if this is a message we want to ignore in user output
		for ( int i = 0; i < ARRAYSIZE(pszIgnoredMessages); ++i )
		{
			if ( V_strncmp( pszMessage, pszIgnoredMessages[ i ], V_strlen( pszIgnoredMessages[ i ] ) ) == 0 )
				return;
		}

		switch ( nLogLevel )
		{
		case kItemtest_Log_Info:
			::Msg( "%s", pszMessage );
			break;
		case kItemtest_Log_Warning:
			::Warning( "%s", pszMessage );
			break;
		case kItemtest_Log_Error:
			::Error( "%s", pszMessage );
			break;
		}
		m_log.PutString( pszMessage );
	}

	const char *Get() { return (char*)m_log.Base(); }

protected:
	mutable CUtlBuffer m_log;
};


//-----------------------------------------------------------------------------
// Purpose: Import text edit dialog
//-----------------------------------------------------------------------------
class CTFFileImportTextEditDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CTFFileImportTextEditDialog, Frame );

public:
	CTFFileImportTextEditDialog( vgui::Panel *parent, const char *pszTitle, const char *pszCommand = NULL );

	virtual ~CTFFileImportTextEditDialog() { }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnCommand( const char *command );

	void HideCancelButton() { m_bShowCancelButton = false; }

	void SetText( const char *pszText );
	bool GetText( char *pszText, int nMaxSize );

protected:
	bool m_bShowCancelButton;
	CUtlString m_sTitle;
	CUtlString m_sText;
	CUtlString m_sCommand;
	vgui::TextEntry *m_pTextEntry;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportTextEditDialog::CTFFileImportTextEditDialog( vgui::Panel *parent, const char *pszTitle, const char *pszCommand ) : Frame( parent, "ImportFileTextEditDialog" )
,	m_bShowCancelButton( true )
,	m_sTitle( pszTitle )
,	m_sText()
,	m_sCommand( pszCommand )
,	m_pTextEntry( NULL )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme(scheme);
	SetProportional( true );
	SetDeleteSelfOnClose( true );
	SetTitle( "", false );
	SetSizeable( false );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportTextEditDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ImportFileTextEditDialog.res" );

	vgui::Label *pLabel = FindControl<vgui::Label>( "TitleLabel" );
	if ( pLabel )
	{
		pLabel->SetText( m_sTitle );
	}

	m_pTextEntry = FindControl<vgui::TextEntry>( "TextEntry" );
	if ( m_pTextEntry )
	{
		m_pTextEntry->RequestFocus();
		m_pTextEntry->SetMultiline( true );
		m_pTextEntry->SetCatchEnterKey( true );
		m_pTextEntry->SetVerticalScrollbar( true );
		m_pTextEntry->SetText( m_sText );
	}

	if ( !m_bShowCancelButton )
	{
		vgui::Panel *pButton = FindControl<vgui::Panel>( "ButtonClose" );
		if ( pButton )
		{
			pButton->SetVisible( false );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportTextEditDialog::OnCommand( const char *command )
{
	if ( V_stricmp( command, "Done" ) == 0 )
	{
		if ( m_sCommand.IsEmpty() )
		{
			OnCommand( "Close" );
		}
		else
		{
			GetParent()->OnCommand( m_sCommand );
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportTextEditDialog::SetText( const char *pszText )
{
	m_sText = pszText;

	if ( m_pTextEntry )
	{
		m_pTextEntry->SetText( pszText );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportTextEditDialog::GetText( char *pszText, int nMaxSize )
{
	if ( m_pTextEntry )
	{
		m_pTextEntry->GetText( pszText, nMaxSize );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Import material edit dialog
//-----------------------------------------------------------------------------
class CTFImportMaterialEditDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CTFImportMaterialEditDialog, Frame );

public:
	CTFImportMaterialEditDialog( vgui::Panel *parent, int nSkinIndex, int nMaterialIndex, KeyValues *pItemValues );

	virtual ~CTFImportMaterialEditDialog() { }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnCommand( const char *command );

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

	void SetVMTKeyValues( const char *pszRedVMTText, const char *pszBlueVMTText );
	KeyValues* GetVMTKeyValues( int nSkinIndex );

	const char* GetBaseTextureFile( int nSkinIndex ) const { return m_strBaseTextureFile[nSkinIndex].String(); }
	const char* GetNormalTextureFile() const { return m_strNormalTextureFile.String(); }
	const char* GetPhongExponentTextureFile() const { return m_strPhongExponentTextureFile.String(); }
	const char* GetSelfIllumTextureFile() const { return m_bSelfIllumEnabled ? m_strSelfIllumTextureFile.String() : ""; }

private:
	void OnCommandEditSkin( int nSkinIndex );
	void OnCommandBrowseMaterial( MATERIAL_FILE_TYPE fileType );

	void SetColorTintBase( int nSkinIndex, const char* pszColorTintBase );
	void SetSelfIllumTint( int nSkinIndex, const char* pszSelfIllumTint );

	void UpdateBackgroundTexture( int nSkinIndex );

	void UpdateBaseTexture( const char *pszBaseTextureFile );
	void UpdateNormalTexture( const char *pszNormalTextureFile );
	void UpdatePhongExponentTexture( const char* pszPhongExponentTextureFile );
	void UpdateSelfIllumTexture( const char *pszSelfIllumTextureFile );

	void UpdateBaseMapAlphaPhongMask();
	void UpdateRimMask();
	void UpdateRimMaskDisplay( bool bEnable );
	void UpdateHalfLambert();
	void UpdateBlendTintByBaseAlpha();
	void UpdateBlendTintColorOverBaseDisplay( bool bEnable );
	void UpdateColorTintBase( int nSkinIndex, const char* pszColor );
	void UpdateAdditive();
	void UpdateTranslucent();
	void UpdateAlphaTest();
	void UpdateAlphaTestDiaplay( bool bEnable );
	void UpdateSelfIllum();
	void UpdateSelfIllumDisplay( bool bEnable );
	void UpdateSelfIllumTint( int nSkinIndex, const char* pszColor );
	void UpdateEnvmapDisplay( bool bEnable );

	void UpdateUniqueLabels();

	CUtlString m_sCommand;
	KeyValuesAD m_VMTKeyValues;
	int m_nSkinIndex;
	int m_nMaterialIndex;
	KeyValues *m_pItemValues;
	
	// unique values per skin
	CUtlString m_strBaseTextureFile[NUM_IMPORT_MATERIALS_PER_TEAM];
	CUtlString m_strColorTintBase[NUM_IMPORT_MATERIALS_PER_TEAM];
	CUtlString m_strSelfIllumTint[NUM_IMPORT_MATERIALS_PER_TEAM];

	CUtlString m_strNormalTextureFile;
	CUtlString m_strPhongExponentTextureFile;
	bool m_bSelfIllumEnabled;
	CUtlString m_strSelfIllumTextureFile;
	MATERIAL_FILE_TYPE m_browseMaterialFileType;

	vgui::Button *m_pRedTeamButton;
	vgui::Button *m_pBlueTeamButton;

	vgui::Label *m_pBaseTextureFileLabel;
	vgui::Label *m_pNormalTextureFileLabel;
	vgui::Label *m_pPhongExponentTextureFileLabel;
	vgui::Label *m_pSelfIllumTextureFileLabel;

	// Lighting
	vgui::ComboBox *m_pLightwarpComboBox;
	vgui::CheckButton *m_pBaseMapAlphaPhongMaskCheckButton;
	vgui::TextEntry *m_pPhongExponentTextEntry;
	vgui::TextEntry *m_pPhongBoostTextEntry;
	vgui::TextEntry *m_pRimLightExponentTextEntry;
	vgui::TextEntry *m_pRimLightBoostTextEntry;
	vgui::CheckButton *m_pRimMaskCheckButton;
	vgui::CheckButton *m_pHalfLambertCheckButton;

	// Paint
	vgui::CheckButton *m_pBlendTintByBaseAlphaCheckButton;
	vgui::TextEntry *m_pBlendTintColorOverBaseTextEntry;
	vgui::TextEntry *m_pColorTintBaseRedTextEntry;
	vgui::TextEntry *m_pColorTintBaseGreenTextEntry;
	vgui::TextEntry *m_pColorTintBaseBlueTextEntry;

	// Translucent
	vgui::CheckButton *m_pAdditiveCheckButton;
	vgui::CheckButton *m_pTranslucentCheckButton;
	vgui::CheckButton *m_pAlphaTestCheckButton;

	// Cube map
	vgui::ComboBox *m_pEnvmapComboBox;
	vgui::ComboBox *m_pEnvmapAlphaMaskComboBox;
	vgui::TextEntry *m_pCubemapTintRedTextEntry;
	vgui::TextEntry *m_pCubemapTintGreenTextEntry;
	vgui::TextEntry *m_pCubemapTintBlueTextEntry;

	// Self Illum
	vgui::CheckButton *m_pSelfIllumCheckButton;
	vgui::TextEntry *m_pSelfIllumTintRedTextEntry;
	vgui::TextEntry *m_pSelfIllumTintGreenTextEntry;
	vgui::TextEntry *m_pSelfIllumTintBlueTextEntry;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFImportMaterialEditDialog::CTFImportMaterialEditDialog( vgui::Panel *parent, int nSkinIndex, int nMaterialIndex, KeyValues* pItemValues ) : Frame( parent, "ImportMaterialEditDialog" )
	,	m_sCommand( CFmtStr( "EditMaterialDone%d,%d", nSkinIndex, nMaterialIndex ) )
	,	m_VMTKeyValues( "VMT" )
	,	m_nSkinIndex( nSkinIndex )
	,	m_nMaterialIndex( nMaterialIndex )
	,	m_pItemValues( pItemValues ) 
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme(scheme);
	SetProportional( true );
	SetDeleteSelfOnClose( true );
	SetTitle( "", false );
	SetSizeable( false );

	bool bSetSharedTextures = false;
	for ( int nSkin=0; nSkin<CItemUpload::Manifest()->GetNumMaterialSkins(); ++nSkin )
	{
		KeyValues* pMaterialKey = m_pItemValues->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkin ), m_nMaterialIndex ) );
		if ( pMaterialKey )
		{
			m_strBaseTextureFile[nSkin] = pMaterialKey->GetString( CFmtStr( "%s_texture_file", s_pszMaterialFilePrefixes[MATERIAL_FILE_BASETEXTURE] ) );
			if ( !bSetSharedTextures )
			{
				m_strNormalTextureFile = pMaterialKey->GetString( CFmtStr( "%s_texture_file", s_pszMaterialFilePrefixes[MATERIAL_FILE_NORMAL] ) );
				m_strPhongExponentTextureFile = pMaterialKey->GetString( CFmtStr( "%s_texture_file", s_pszMaterialFilePrefixes[MATERIAL_FILE_PHONGEXPONENT] ) );
				m_strSelfIllumTextureFile = pMaterialKey->GetString( CFmtStr( "%s_texture_file", s_pszMaterialFilePrefixes[MATERIAL_FILE_SELFILLUM] ) );
				bSetSharedTextures = true;
			}
		}
	}

	m_pRedTeamButton = NULL;
	m_pBlueTeamButton = NULL;
	m_pBaseTextureFileLabel = NULL;
	m_pNormalTextureFileLabel = NULL;
	m_pPhongExponentTextureFileLabel = NULL;
	m_bSelfIllumEnabled = false;
	m_pSelfIllumTextureFileLabel = NULL;
	m_pLightwarpComboBox = NULL;
	m_pBaseMapAlphaPhongMaskCheckButton = NULL;
	m_pPhongExponentTextEntry = NULL;
	m_pPhongBoostTextEntry = NULL;
	m_pRimLightExponentTextEntry = NULL;
	m_pRimLightBoostTextEntry = NULL;
	m_pRimMaskCheckButton = NULL;
	m_pHalfLambertCheckButton = NULL;
	m_pBlendTintByBaseAlphaCheckButton = NULL;
	m_pBlendTintColorOverBaseTextEntry = NULL;
	m_pColorTintBaseRedTextEntry = NULL;
	m_pColorTintBaseGreenTextEntry = NULL;
	m_pColorTintBaseBlueTextEntry = NULL;
	m_pAdditiveCheckButton = NULL;
	m_pTranslucentCheckButton = NULL;
	m_pAlphaTestCheckButton = NULL;
	m_pEnvmapComboBox = NULL;
	m_pEnvmapAlphaMaskComboBox = NULL;
	m_pCubemapTintRedTextEntry = NULL;
	m_pCubemapTintGreenTextEntry = NULL;
	m_pCubemapTintBlueTextEntry = NULL;
	m_pSelfIllumCheckButton = NULL;
	m_pSelfIllumTintRedTextEntry = NULL;
	m_pSelfIllumTintGreenTextEntry = NULL;
	m_pSelfIllumTintBlueTextEntry = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ImportMaterialEditDialog.res" );

	bool bAllTeam = m_pItemValues->GetInt( kSkinType ) == 0;

	if ( !bAllTeam )
	{
		m_pRedTeamButton = FindControl<vgui::Button>( "RedTeamButton" );
		if ( m_pRedTeamButton )
		{
			m_pRedTeamButton->SetVisible( true );
			m_pRedTeamButton->SetEnabled( true );
		}

		m_pBlueTeamButton = FindControl<vgui::Button>( "BlueTeamButton" );
		if ( m_pBlueTeamButton )
		{
			m_pBlueTeamButton->SetVisible( true );
			m_pBlueTeamButton->SetEnabled( true );
		}

		UpdateUniqueLabels();
	}

	UpdateBackgroundTexture( m_nSkinIndex );

	vgui::Label *pTitleLabel = FindControl<vgui::Label>( "TitleLabel" );
	if ( pTitleLabel )
	{
		pTitleLabel->SetText( CFmtStr( "#TF_ImportFile_EditVMT%d", m_nMaterialIndex ) );
	}

	m_pBaseTextureFileLabel = FindControl<vgui::Label>( "BaseTextureFileLabel" );
	m_pNormalTextureFileLabel = FindControl< vgui::Label >( "NormalTextureFileLabel" );
	m_pPhongExponentTextureFileLabel = FindControl< vgui::Label >( "PhongExponentTextureFileLabel" );
	m_pSelfIllumTextureFileLabel = FindControl< vgui::Label >( "SelfIllumTextureFileLabel" );

	m_pLightwarpComboBox = FindControl<vgui::ComboBox>( "LightwarpComboBox" );
	if ( m_pLightwarpComboBox )
	{
		m_pLightwarpComboBox->RemoveAll();

		KeyValuesAD pKeyValues( "data" );
		for ( int i=0; i<ARRAYSIZE( kLightwarps ); ++i )
		{
			pKeyValues->SetString( kLightwarpPath, kLightwarps[i].pszPath );
			m_pLightwarpComboBox->AddItem( kLightwarps[i].pszName, pKeyValues );
		}

		m_pLightwarpComboBox->AddActionSignalTarget( this );
	}

	// Lighting
	m_pBaseMapAlphaPhongMaskCheckButton = FindControl<vgui::CheckButton>( "BaseMapAlphaPhongMaskCheckButton" );
	if ( m_pBaseMapAlphaPhongMaskCheckButton )
	{
		m_pBaseMapAlphaPhongMaskCheckButton->AddActionSignalTarget( this );
	}

	m_pPhongExponentTextEntry = FindControl<vgui::TextEntry>( "PhongExponentTextEntry" );
	if ( m_pPhongExponentTextEntry )
	{
		m_pPhongExponentTextEntry->AddActionSignalTarget( this );
	}

	m_pPhongBoostTextEntry = FindControl<vgui::TextEntry>( "PhongBoostTextEntry" );
	if ( m_pPhongBoostTextEntry )
	{
		m_pPhongBoostTextEntry->AddActionSignalTarget( this );
	}

	m_pRimLightExponentTextEntry = FindControl<vgui::TextEntry>( "RimLightExponentTextEntry" );
	if ( m_pRimLightExponentTextEntry )
	{
		m_pRimLightExponentTextEntry->AddActionSignalTarget( this );
	}

	m_pRimLightBoostTextEntry = FindControl<vgui::TextEntry>( "RimLightBoostTextEntry" );
	if ( m_pRimLightBoostTextEntry )
	{
		m_pRimLightBoostTextEntry->AddActionSignalTarget( this );
	}

	m_pRimMaskCheckButton = FindControl<vgui::CheckButton>( "RimMaskCheckButton" );
	if ( m_pRimMaskCheckButton )
	{
		m_pRimMaskCheckButton->AddActionSignalTarget( this );
		m_pRimMaskCheckButton->SetCheckButtonCheckable( false );
	}

	m_pHalfLambertCheckButton = FindControl<vgui::CheckButton>( "HalfLambertCheckButton" );
	if ( m_pHalfLambertCheckButton )
	{
		m_pHalfLambertCheckButton->AddActionSignalTarget( this );
	}

	// Paint
	m_pBlendTintByBaseAlphaCheckButton = FindControl<vgui::CheckButton>( "BlendTintByBaseAlphaCheckButton" );
	if ( m_pBlendTintByBaseAlphaCheckButton )
	{
		m_pBlendTintByBaseAlphaCheckButton->AddActionSignalTarget( this );
		m_pBlendTintByBaseAlphaCheckButton->SetCheckButtonCheckable( false );
	}

	m_pBlendTintColorOverBaseTextEntry = FindControl<vgui::TextEntry>( "BlendTintColorOverBaseTextEntry" );
	if ( m_pBlendTintColorOverBaseTextEntry )
	{
		m_pBlendTintColorOverBaseTextEntry->AddActionSignalTarget( this );
	}

	m_pColorTintBaseRedTextEntry = FindControl<vgui::TextEntry>( "ColorTintBaseRedTextEntry" );
	if ( m_pColorTintBaseRedTextEntry )
	{
		m_pColorTintBaseRedTextEntry->AddActionSignalTarget( this );
	}

	m_pColorTintBaseGreenTextEntry = FindControl<vgui::TextEntry>( "ColorTintBaseGreenTextEntry" );
	if ( m_pColorTintBaseGreenTextEntry )
	{
		m_pColorTintBaseGreenTextEntry->AddActionSignalTarget( this );
	}

	m_pColorTintBaseBlueTextEntry = FindControl<vgui::TextEntry>( "ColorTintBaseBlueTextEntry" );
	if ( m_pColorTintBaseBlueTextEntry )
	{
		m_pColorTintBaseBlueTextEntry->AddActionSignalTarget( this );
	}

	// enable paintable UI
	const char* pszPaintable = CFmtStr( kItemPaintable, m_nMaterialIndex );
	if ( m_pItemValues->GetBool( pszPaintable ) )
	{
		m_pBlendTintByBaseAlphaCheckButton->SetEnabled( true );
		m_pBlendTintByBaseAlphaCheckButton->SetCheckButtonCheckable( true );
		m_pBlendTintColorOverBaseTextEntry->SetEnabled( true );
		m_pColorTintBaseRedTextEntry->SetEnabled( true );
		m_pColorTintBaseGreenTextEntry->SetEnabled( true );
		m_pColorTintBaseBlueTextEntry->SetEnabled( true );

		static const char* paintableUILabels[] =
		{
			"BlendTintByBaseAlphaLabel",
			"BlendTintColorOverBaseLabel",
			"ColorTintBaseLabel"
		};

		for ( int i=0; i<ARRAYSIZE( paintableUILabels ); ++i )
		{
			vgui::Label *pLabel = FindControl<vgui::Label>( paintableUILabels[i] );
			if ( pLabel )
			{
				pLabel->SetEnabled( true );
			}
		}
	}

	// Translucent
	m_pAdditiveCheckButton = FindControl<vgui::CheckButton>( "AdditiveCheckButton" );
	if ( m_pAdditiveCheckButton )
	{
		m_pAdditiveCheckButton->AddActionSignalTarget( this );
	}
	m_pTranslucentCheckButton = FindControl<vgui::CheckButton>( "TranslucentCheckButton" );
	if ( m_pTranslucentCheckButton )
	{
		m_pTranslucentCheckButton->AddActionSignalTarget( this );
	}
	m_pAlphaTestCheckButton = FindControl<vgui::CheckButton>( "AlphaTestCheckButton" );
	if ( m_pAlphaTestCheckButton )
	{
		m_pAlphaTestCheckButton->AddActionSignalTarget( this );
		m_pAlphaTestCheckButton->SetCheckButtonCheckable( false );
	}

	// Cube map
	m_pEnvmapComboBox = FindControl<vgui::ComboBox>( "EnvmapComboBox" );
	if ( m_pEnvmapComboBox )
	{
		KeyValuesAD pKeyValues( "data" );
		for ( int i=0; i<ARRAYSIZE(kEnvmaps); ++i )
		{
			pKeyValues->SetString( kEnvmapPath, kEnvmaps[i].pszPath );
			m_pEnvmapComboBox->AddItem( kEnvmaps[i].pszName, pKeyValues );
		}

		m_pEnvmapComboBox->AddActionSignalTarget( this );
	}

	m_pEnvmapAlphaMaskComboBox = FindControl<vgui::ComboBox>( "EnvmapAlphaMaskComboBox" );
	if ( m_pEnvmapAlphaMaskComboBox )
	{
		KeyValuesAD pKeyValues( "data" );
		for ( int i=0; i<ARRAYSIZE(kEnvmapMasks); ++i )
		{
			pKeyValues->SetString( kEnvmapMaskVarName, kEnvmapMasks[i].pszVarName );
			m_pEnvmapAlphaMaskComboBox->AddItem( kEnvmapMasks[i].pszDisplayName, pKeyValues );
		}

		m_pEnvmapAlphaMaskComboBox->AddActionSignalTarget( this );
	}

	m_pCubemapTintRedTextEntry = FindControl<vgui::TextEntry>( "CubemapTintRedTextEntry" );
	if ( m_pCubemapTintRedTextEntry )
	{
		m_pCubemapTintRedTextEntry->AddActionSignalTarget( this );
	}

	m_pCubemapTintGreenTextEntry = FindControl<vgui::TextEntry>( "CubemapTintGreenTextEntry" );
	if ( m_pCubemapTintGreenTextEntry )
	{
		m_pCubemapTintGreenTextEntry->AddActionSignalTarget( this );
	}

	m_pCubemapTintBlueTextEntry = FindControl<vgui::TextEntry>( "CubemapTintBlueTextEntry" );
	if ( m_pCubemapTintBlueTextEntry )
	{
		m_pCubemapTintBlueTextEntry->AddActionSignalTarget( this );
	}

	// Self Illum
	m_pSelfIllumCheckButton = FindControl<vgui::CheckButton>( "SelfIllumCheckButton" );
	if ( m_pSelfIllumCheckButton )
	{
		m_pSelfIllumCheckButton->AddActionSignalTarget( this );
	}

	m_pSelfIllumTintRedTextEntry = FindControl<vgui::TextEntry>( "SelfIllumTintRedTextEntry" );
	if ( m_pSelfIllumTintRedTextEntry )
	{
		m_pSelfIllumTintRedTextEntry->AddActionSignalTarget( this );
	}

	m_pSelfIllumTintGreenTextEntry = FindControl<vgui::TextEntry>( "SelfIllumTintGreenTextEntry" );
	if ( m_pSelfIllumTintGreenTextEntry )
	{
		m_pSelfIllumTintGreenTextEntry->AddActionSignalTarget( this );
	}

	m_pSelfIllumTintBlueTextEntry = FindControl<vgui::TextEntry>( "SelfIllumTintBlueTextEntry" );
	if ( m_pSelfIllumTintBlueTextEntry )
	{
		m_pSelfIllumTintBlueTextEntry->AddActionSignalTarget( this );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::OnCommand( const char *command )
{
	if ( FStrEq( command, "Done" ) )
	{
		if ( m_sCommand.IsEmpty() )
		{
			OnCommand( "Close" );
		}
		else if ( GetParent() )
		{
			GetParent()->OnCommand( m_sCommand );
		}
	}
	else if ( V_strncasecmp( command, "EditSkin", V_strlen( "EditSkin" ) ) == 0 )
	{
		int index = V_atoi( &command[ V_strlen( "EditSkin" ) ] );
		OnCommandEditSkin( index );
	}
	else if ( FStrEq( command, "BrowseMaterial" ) )
	{
		OnCommandBrowseMaterial( MATERIAL_FILE_BASETEXTURE );
	}
	else if ( FStrEq( command, "BrowseNormalTexture" ) )
	{
		OnCommandBrowseMaterial( MATERIAL_FILE_NORMAL );
	}
	else if ( FStrEq( command, "BrowsePhongExponentTexture" ) )
	{
		OnCommandBrowseMaterial( MATERIAL_FILE_PHONGEXPONENT );
	}
	else if ( FStrEq( command, "BrowseSelfIllumTexture" ) )
	{
		OnCommandBrowseMaterial( MATERIAL_FILE_SELFILLUM );
	}
	else if ( FStrEq( command, "ClearNormalTexture" ) )
	{
		UpdateNormalTexture( "" );
	}
	else if ( FStrEq( command, "ClearPhongExponentTexture" ) )
	{
		UpdatePhongExponentTexture( "" );
	}
	else if ( FStrEq( command, "ClearSelfIllumTexture" ) )
	{
		UpdateSelfIllumTexture( "" );
	}
	else if ( FStrEq( command, "UpdateBaseMapAlphaPhongMask" ) )
	{
		UpdateBaseMapAlphaPhongMask();
	}
	else if ( FStrEq( command, "UpdateRimMask" ) )
	{
		UpdateRimMask();
	}
	else if ( FStrEq( command, "UpdateHalfLambert" ) )
	{
		UpdateHalfLambert();
	}
	else if ( FStrEq( command, "UpdateBlendTintByBaseAlpha" ) )
	{
		UpdateBlendTintByBaseAlpha();
	}
	else if ( FStrEq( command, "UpdateAdditive" ) )
	{
		UpdateAdditive();
	}
	else if ( FStrEq( command, "UpdateTranslucent" ) )
	{
		UpdateTranslucent();
	}
	else if ( FStrEq( command, "UpdateAlphaTest" ) )
	{
		UpdateAlphaTest();
	}
	else if ( FStrEq( command, "UpdateSelfIllum" ) )
	{
		UpdateSelfIllum();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::OnCommandEditSkin( int nSkinIndex )
{
	if ( m_nSkinIndex == nSkinIndex )
	{
		return;
	}

	m_nSkinIndex = nSkinIndex;

	UpdateBackgroundTexture( m_nSkinIndex );
	UpdateBaseTexture( m_strBaseTextureFile[m_nSkinIndex] );
	UpdateColorTintBase( m_nSkinIndex, m_strColorTintBase[m_nSkinIndex] );
	UpdateSelfIllumTint( m_nSkinIndex, m_strSelfIllumTint[m_nSkinIndex] );
	UpdateUniqueLabels();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::OnCommandBrowseMaterial( MATERIAL_FILE_TYPE fileType )
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectMaterial", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.tga,*.psd", "#TF_ImportFile_MaterialFileType", true);
	pDialog->AddActionSignalTarget( this );
	const char *pszStartPath = tf_steam_workshop_import_material_path.GetString();
	if ( pszStartPath && *pszStartPath )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();

	m_browseMaterialFileType = fileType;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CheckImageSize( const char *fullpath, MATERIAL_FILE_TYPE materialFileType, KeyValues *pMessageVariables )
{
	CAssetTF asset;
	CTargetTGA image( &asset, NULL );
	if ( !image.SetInputFile( fullpath ) )
	{
		SetMessageFileVariable( pMessageVariables, image.GetInputFile() );
		return CTFFileImportDialog::BUILD_FAILED_IMAGEUNSUPPORTEDFILETYPE;
	}

	int nLimitWidth = 0;
	int nLimitHeight = 0;

	switch ( materialFileType )
	{
	case MATERIAL_FILE_BASETEXTURE:
		{
			// 512x512
			nLimitWidth = nLimitHeight = 512;
		}
		break;
	case MATERIAL_FILE_NORMAL:
		{
			// 512x512
			nLimitWidth = nLimitHeight = 512;
		}
		break;
	case MATERIAL_FILE_PHONGEXPONENT:
		{
			// 256x256
			nLimitWidth = nLimitHeight = 256;
		}
		break;
	case MATERIAL_FILE_SELFILLUM:
		{
			// 256x256
			nLimitWidth = nLimitHeight = 256;
		}
		break;
	}

	if ( image.GetWidth() > nLimitWidth || image.GetHeight() > nLimitHeight )
	{
		SetMessageFileVariable( pMessageVariables, image.GetInputFile() );
		pMessageVariables->SetInt( "width", nLimitWidth );
		pMessageVariables->SetInt( "height", nLimitHeight );
		return CTFFileImportDialog::BUILD_FAILED_IMAGERESOLUTIONOVERLIMIT;
	}

	if ( !IsPowerOfTwo( image.GetWidth() ) || !IsPowerOfTwo( image.GetHeight() ) )
	{
		SetMessageFileVariable( pMessageVariables, image.GetInputFile() );
		return CTFFileImportDialog::BUILD_FAILED_IMAGERESOLUTIONNOTPOWEROF2;
	}

	return CTFFileImportDialog::BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::OnFileSelected( char const *fullpath )
{
	SaveBrowsePath( tf_steam_workshop_import_material_path, fullpath );

	KeyValuesAD pKV("data");
	CTFFileImportDialog::BUILD_RESULT result = CheckImageSize( fullpath, m_browseMaterialFileType, pKV );
	if ( result != CTFFileImportDialog::BUILD_OKAY )
	{
		ShowMessageBox( "#TF_ImportFile_LoadFailed", kBuildResultMessages[result], pKV );
		return;
	}

	switch ( m_browseMaterialFileType )
	{
	case MATERIAL_FILE_BASETEXTURE:
		{
			UpdateBaseTexture( fullpath );
		}
		break;
	case MATERIAL_FILE_NORMAL:
		{
			UpdateNormalTexture( fullpath );
		}
		break;
	case MATERIAL_FILE_PHONGEXPONENT:
		{
			UpdatePhongExponentTexture( fullpath );
		}
		break;
	case MATERIAL_FILE_SELFILLUM:
		{
			UpdateSelfIllumTexture( fullpath );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr( "panel" ) );

	vgui::ComboBox *pComboBox = dynamic_cast< vgui::ComboBox * >( pPanel );
	if ( pComboBox )
	{
		if( pComboBox == m_pLightwarpComboBox )
		{
			KeyValues *pData = m_pLightwarpComboBox->GetActiveItemUserData();
			m_VMTKeyValues->SetString( "$lightwarptexture", pData->GetString( kLightwarpPath ) );
		}
		else if ( pComboBox == m_pEnvmapComboBox )
		{
			KeyValues *pData = m_pEnvmapComboBox->GetActiveItemUserData();
			const char* pszEnvmapPath = pData->GetString( kEnvmapPath );
			m_VMTKeyValues->SetString( "$envmap", pszEnvmapPath );
			UpdateEnvmapDisplay( !FStrEq( pszEnvmapPath, "" ) );
		}
		else if ( pComboBox == m_pEnvmapAlphaMaskComboBox )
		{
			KeyValues *pData = m_pEnvmapAlphaMaskComboBox->GetActiveItemUserData();
			const char* pszEnvmapMaskVarName = pData->GetString( kEnvmapMaskVarName );
			for ( int i=0; i<ARRAYSIZE(kEnvmapMasks); ++i )
			{
				const char* pszCurrentVarName = kEnvmapMasks[i].pszVarName;
				if ( FStrEq( pszCurrentVarName, "" ) )
				{
					continue;
				}

				bool bSet = FStrEq( pszCurrentVarName, pszEnvmapMaskVarName );
				m_VMTKeyValues->SetBool( pszCurrentVarName, bSet );
			}

			// pop a warning
			static bool bWarned = false;
			if  ( FStrEq( pszEnvmapMaskVarName, "$basealphaenvmapmask" ) )
			{
				bool bHasNormal = !m_strNormalTextureFile.IsEmpty();
				if ( !bWarned && bHasNormal )
				{
					bWarned = true;
					ShowMessageBox( "#TF_ImportFile_Warning", kWarningMessages[ CTFFileImportDialog::WARNING_BASEALPHAMASK ] );
				}
			}
			else
			{
				bWarned = false;
			}
		}

		return;
	}

	vgui::TextEntry *pTextEntry = dynamic_cast< vgui::TextEntry * >( pPanel );
	if ( pTextEntry )
	{
		if ( pTextEntry == m_pPhongExponentTextEntry )
		{
			m_VMTKeyValues->SetFloat( "$phongexponent", m_pPhongExponentTextEntry->GetValueAsFloat() );
		}
		else if ( pTextEntry == m_pPhongBoostTextEntry )
		{
			m_VMTKeyValues->SetFloat( "$phongboost", m_pPhongBoostTextEntry->GetValueAsFloat() );
		}
		else if ( pTextEntry == m_pRimLightExponentTextEntry )
		{
			m_VMTKeyValues->SetFloat( "$rimlightexponent", m_pRimLightExponentTextEntry->GetValueAsFloat() );
		}
		else if ( pTextEntry == m_pRimLightBoostTextEntry )
		{
			m_VMTKeyValues->SetFloat( "$rimlightboost", m_pRimLightBoostTextEntry->GetValueAsFloat() );
		}
		else if ( pTextEntry == m_pBlendTintColorOverBaseTextEntry )
		{
			float flClampedValue = clamp( m_pBlendTintColorOverBaseTextEntry->GetValueAsFloat(), 0.f, 1.f );
			m_VMTKeyValues->SetFloat( "$blendtintcoloroverbase", flClampedValue );
		}
		else if (	pTextEntry == m_pColorTintBaseRedTextEntry ||
					pTextEntry == m_pColorTintBaseGreenTextEntry ||
					pTextEntry == m_pColorTintBaseBlueTextEntry )
		{
			const char* pszColor = CFmtStr( "{ %d %d %d }",
											clamp( m_pColorTintBaseRedTextEntry->GetValueAsInt(), 0, 255 ),
											clamp( m_pColorTintBaseGreenTextEntry->GetValueAsInt(), 0, 255 ),
											clamp( m_pColorTintBaseBlueTextEntry->GetValueAsInt(), 0, 255 ) );
			SetColorTintBase( m_nSkinIndex, pszColor );
		}
		else if (	pTextEntry == m_pCubemapTintRedTextEntry ||
					pTextEntry == m_pCubemapTintGreenTextEntry ||
					pTextEntry == m_pCubemapTintBlueTextEntry )
		{
			float flR = (float)clamp( m_pCubemapTintRedTextEntry->GetValueAsInt(), 0, 255 ) / 255.f;
			float flG = (float)clamp( m_pCubemapTintGreenTextEntry->GetValueAsInt(), 0, 255 ) / 255.f;
			float flB = (float)clamp( m_pCubemapTintBlueTextEntry->GetValueAsInt(), 0, 255 ) / 255.f;
			const char* pszColor = CFmtStr( "[%f %f %f]", flR, flG, flB );
			m_VMTKeyValues->SetString( "$envmaptint", pszColor );
		}
		else if (	pTextEntry == m_pSelfIllumTintRedTextEntry ||
					pTextEntry == m_pSelfIllumTintGreenTextEntry ||
					pTextEntry == m_pSelfIllumTintBlueTextEntry )
		{
			float flR = m_pSelfIllumTintRedTextEntry->GetValueAsFloat() / 255.f;
			float flG = m_pSelfIllumTintGreenTextEntry->GetValueAsFloat() / 255.f;
			float flB = m_pSelfIllumTintBlueTextEntry->GetValueAsFloat() / 255.f;
			const char* pszColor = CFmtStr( "[%f %f %f]", flR, flG, flB );
			SetSelfIllumTint( m_nSkinIndex, pszColor );
		}

		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::SetColorTintBase( int nSkinIndex, const char* pszColorTintBase )
{
	m_VMTKeyValues->SetString( "$colortint_base", pszColorTintBase );
	m_VMTKeyValues->SetString( "$color2", pszColorTintBase );

	m_strColorTintBase[nSkinIndex] = pszColorTintBase;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::SetSelfIllumTint( int nSkinIndex, const char* pszSelfIllumTint )
{
	m_VMTKeyValues->SetString( "$selfillumtint", pszSelfIllumTint );

	m_strSelfIllumTint[nSkinIndex] = pszSelfIllumTint;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::SetVMTKeyValues( const char *pszRedVMTText, const char *pszBlueVMTText )
{
	m_VMTKeyValues->Clear();

	// read the unique data
	const char* pszVMTs[NUM_IMPORT_MATERIALS_PER_TEAM] = { pszRedVMTText, pszBlueVMTText };
	for ( int i=0; i<ARRAYSIZE( pszVMTs ); ++i )
	{
		KeyValuesAD pKV( "VMT" );
		if ( pKV->LoadFromBuffer( "VMT", pszVMTs[i] ) )
		{
			m_strColorTintBase[i] = pKV->GetString( "$colortint_base" );
			m_strSelfIllumTint[i] = pKV->GetString( "$selfillumtint" );
		}
	}

	const char* pszVMTText = m_nSkinIndex == 0 ? pszRedVMTText : pszBlueVMTText;
	if ( m_VMTKeyValues->LoadFromBuffer( "VMT", pszVMTText ) )
	{
		// update the UI here
		UpdateBaseTexture( m_strBaseTextureFile[m_nSkinIndex].String() );
		UpdateNormalTexture( m_strNormalTextureFile.String() );
		UpdatePhongExponentTexture( m_strPhongExponentTextureFile.String() );
		UpdateSelfIllumTexture( m_strSelfIllumTextureFile.String() );

		if ( m_pLightwarpComboBox )
		{
			const char* pszCurrentLightwarpPath = m_VMTKeyValues->GetString( "$lightwarptexture" );
			for ( int i=0; i<ARRAYSIZE(kLightwarps); ++i )
			{
				if ( FStrEq( pszCurrentLightwarpPath, kLightwarps[i].pszPath ) )
				{
					m_pLightwarpComboBox->ActivateItemByRow( i );
					break;
				}
			}
		}

		const char* pszFloatFormat = "%.3f";

		// Lighting
		if ( m_pBaseMapAlphaPhongMaskCheckButton )
		{
			m_pBaseMapAlphaPhongMaskCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$basemapalphaphongmask" ) );
		}
		if ( m_pPhongExponentTextEntry )
		{
			m_pPhongExponentTextEntry->SetText( CFmtStr( pszFloatFormat, m_VMTKeyValues->GetFloat( "$phongexponent" ) ) );
		}
		if ( m_pPhongBoostTextEntry )
		{
			m_pPhongBoostTextEntry->SetText( CFmtStr( pszFloatFormat, m_VMTKeyValues->GetFloat( "$phongboost" ) ) );
		}
		if ( m_pRimLightExponentTextEntry )
		{
			m_pRimLightExponentTextEntry->SetText( CFmtStr( pszFloatFormat, m_VMTKeyValues->GetFloat( "$rimlightexponent" ) ) );
		}
		if ( m_pRimLightBoostTextEntry )
		{
			m_pRimLightBoostTextEntry->SetText( CFmtStr( pszFloatFormat, m_VMTKeyValues->GetFloat( "$rimlightboost" ) ) );
		}
		if ( m_pRimMaskCheckButton )
		{
			UpdateRimMaskDisplay( !m_strPhongExponentTextureFile.IsEmpty() );
			m_pRimMaskCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$rimmask" ) );
		}
		if ( m_pHalfLambertCheckButton )
		{
			m_pHalfLambertCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$halflambert" ) );
		}

		// Paint
		if ( m_pBlendTintByBaseAlphaCheckButton )
		{
			m_pBlendTintByBaseAlphaCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$blendtintbybasealpha" ) );
		}
		if ( m_pBlendTintColorOverBaseTextEntry )
		{
			m_pBlendTintColorOverBaseTextEntry->SetText( CFmtStr( pszFloatFormat, m_VMTKeyValues->GetFloat( "$blendtintcoloroverbase" ) ) );
		}

		UpdateColorTintBase( m_nSkinIndex, m_VMTKeyValues->GetString( "$colortint_base" ) );

		// Translucent
		if ( m_pAdditiveCheckButton )
		{
			m_pAdditiveCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$additive" ) );
		}
		if ( m_pTranslucentCheckButton )
		{
			m_pTranslucentCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$translucent" ) );
		}
		if ( m_pAlphaTestCheckButton )
		{
			bool bTranslucent = m_pTranslucentCheckButton->IsSelected();
			UpdateAlphaTestDiaplay( bTranslucent );
			m_pAlphaTestCheckButton->SetSelected( m_VMTKeyValues->GetBool( "$alphatest" ) );
		}

		// Cube map
		if ( m_pEnvmapComboBox )
		{
			const char *pszEnvmap = m_VMTKeyValues->GetString( "$envmap" );
			for ( int i=0; i<m_pEnvmapComboBox->GetItemCount(); ++i )
			{
				KeyValues *pKey = m_pEnvmapComboBox->GetItemUserData( i );
				const char *pszPath = pKey->GetString( kEnvmapPath );
				if ( FStrEq( pszEnvmap, pszPath ) )
				{
					m_pEnvmapComboBox->ActivateItemByRow( i );
					break;
				}
			}
		}

		if ( m_pEnvmapAlphaMaskComboBox )
		{
			int nRow = 0;
			for ( int i=0; i<m_pEnvmapAlphaMaskComboBox->GetItemCount(); ++i )
			{
				KeyValues *pKey = m_pEnvmapAlphaMaskComboBox->GetItemUserData( i );
				const char *pszMask = pKey->GetString( kEnvmapMaskVarName );
				if ( pKey && m_VMTKeyValues->GetBool( pszMask ) )
				{
					nRow = i;
					break;
				}
			}
			m_pEnvmapAlphaMaskComboBox->ActivateItemByRow( nRow );
		}

		const char *pszStrippedEnvmapTintColor = m_VMTKeyValues->GetString( "$envmaptint" ) + 1;
		float entmapTint[3];
		UTIL_StringToFloatArray( entmapTint, ARRAYSIZE( entmapTint ), pszStrippedEnvmapTintColor );
		if ( m_pCubemapTintRedTextEntry )
		{
			m_pCubemapTintRedTextEntry->SetText( CFmtStr( "%d", int( entmapTint[0] * 255.f ) ) );
		}
		if ( m_pCubemapTintGreenTextEntry )
		{
			m_pCubemapTintGreenTextEntry->SetText( CFmtStr( "%d", int( entmapTint[1] * 255.f )  ) );
		}
		if ( m_pCubemapTintBlueTextEntry )
		{
			m_pCubemapTintBlueTextEntry->SetText( CFmtStr( "%d", int( entmapTint[2] * 255.f ) ) );
		}

		// Self Illum
		bool bSelfIllum = m_VMTKeyValues->GetBool( ">=DX90/$selfillum" );
		if ( m_pSelfIllumCheckButton )
		{
			m_pSelfIllumCheckButton->SetSelected( bSelfIllum );
			UpdateSelfIllumDisplay( bSelfIllum );
		}

		UpdateSelfIllumTint( m_nSkinIndex, m_VMTKeyValues->GetString( "$selfillumtint" ) );
	}
	else
	{
		Warning( "CTFImportMaterialEditDialog::SetKeyValues failed to LoadFromBuffer\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues* CTFImportMaterialEditDialog::GetVMTKeyValues( int nSkinIndex )
{
	//set unique values for the skin index here
	m_VMTKeyValues->SetString( "$colortint_base", m_strColorTintBase[nSkinIndex] );
	m_VMTKeyValues->SetString( "$color2", m_strColorTintBase[nSkinIndex] );
	m_VMTKeyValues->SetString( "$selfillumtint", m_strSelfIllumTint[nSkinIndex] );

	return m_VMTKeyValues;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateBackgroundTexture( int nSkinIndex )
{
	static const char* pszBGs[] = { "RedBG", "BlueBG" };
	for ( int i=0; i<ARRAYSIZE(pszBGs); ++i )
	{
		vgui::ImagePanel *pBG = FindControl<vgui::ImagePanel>( pszBGs[i] );
		if ( pBG )
		{
			bool bVisible = nSkinIndex == i;
			pBG->SetVisible( bVisible );
			pBG->SetEnabled( bVisible );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateBaseTexture( const char *pszBaseTextureFile )
{
	if ( m_pBaseTextureFileLabel && pszBaseTextureFile )
	{
		m_pBaseTextureFileLabel->InvalidateLayout( true, true );

		if ( *pszBaseTextureFile )
		{
			char file[MAX_PATH];
			V_FileBase( pszBaseTextureFile, file, sizeof(file) );
			m_pBaseTextureFileLabel->SetText( file );
			m_pBaseTextureFileLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
		}
		else
		{
			m_pBaseTextureFileLabel->SetText( "#TF_PublishFile_NoFileSelected" );
			m_pBaseTextureFileLabel->SetFgColor( Color( 255, 0, 0, 255 ) );
		}

		m_strBaseTextureFile[m_nSkinIndex] = pszBaseTextureFile;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateNormalTexture( const char *pszNormalTextureFile )
{
	if ( m_pNormalTextureFileLabel && pszNormalTextureFile )
	{
		if ( *pszNormalTextureFile )
		{
			char file[MAX_PATH];
			V_FileBase( pszNormalTextureFile, file, sizeof(file) );
			m_pNormalTextureFileLabel->SetText( file );
		}
		else
		{
			m_pNormalTextureFileLabel->SetText( "#TF_PublishFile_Optional" );
		}

		m_strNormalTextureFile = pszNormalTextureFile;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdatePhongExponentTexture( const char* pszPhongExponentTextureFile )
{
	if ( m_pPhongExponentTextureFileLabel && pszPhongExponentTextureFile )
	{
		if ( *pszPhongExponentTextureFile )
		{
			char file[MAX_PATH];
			V_FileBase( pszPhongExponentTextureFile, file, sizeof(file) );
			m_pPhongExponentTextureFileLabel->SetText( file );
		}
		else
		{
			m_pPhongExponentTextureFileLabel->SetText( "#TF_PublishFile_Optional" );
		}

		m_strPhongExponentTextureFile = pszPhongExponentTextureFile;

		UpdateRimMaskDisplay( !m_strPhongExponentTextureFile.IsEmpty() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateSelfIllumTexture( const char *pszSelfIllumTextureFile )
{
	if ( m_pSelfIllumTextureFileLabel && pszSelfIllumTextureFile )
	{
		if ( *pszSelfIllumTextureFile )
		{
			char file[MAX_PATH];
			V_FileBase( pszSelfIllumTextureFile, file, sizeof(file) );
			m_pSelfIllumTextureFileLabel->SetText( file );
		}
		else
		{
			m_pSelfIllumTextureFileLabel->SetText( "#TF_PublishFile_Optional" );
		}

		m_strSelfIllumTextureFile = pszSelfIllumTextureFile;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateBaseMapAlphaPhongMask()
{
	if ( m_pBaseMapAlphaPhongMaskCheckButton )
	{
		m_VMTKeyValues->SetBool( "$basemapalphaphongmask", m_pBaseMapAlphaPhongMaskCheckButton->IsSelected() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateRimMask()
{
	if ( m_pRimMaskCheckButton )
	{
		m_VMTKeyValues->SetBool( "$rimmask", m_pRimMaskCheckButton->IsSelected() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateRimMaskDisplay( bool bEnable )
{
	if ( m_pRimMaskCheckButton )
	{
		m_pRimMaskCheckButton->SetEnabled( bEnable );
		m_pRimMaskCheckButton->SetCheckButtonCheckable( bEnable );
	}

	vgui::Label* pLabel = FindControl< vgui::Label >( "RimMaskLabel" );
	if ( pLabel )
	{
		pLabel->SetEnabled( bEnable );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateHalfLambert()
{
	if ( m_pHalfLambertCheckButton )
	{
		m_VMTKeyValues->SetBool( "$halflambert", m_pHalfLambertCheckButton->IsSelected() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateBlendTintByBaseAlpha()
{
	if ( m_pBlendTintByBaseAlphaCheckButton )
	{
		bool bSelected = m_pBlendTintByBaseAlphaCheckButton->IsSelected();
		m_VMTKeyValues->SetBool( "$blendtintbybasealpha", bSelected );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateBlendTintColorOverBaseDisplay( bool bEnable )
{
	if ( m_pBlendTintColorOverBaseTextEntry )
	{
		if ( !bEnable )
		{
			m_VMTKeyValues->SetFloat( "$blendtintcoloroverbase", 0.f );
		}
		m_pBlendTintColorOverBaseTextEntry->SetText( CFmtStr( "%.3f", m_VMTKeyValues->GetFloat( "$blendtintcoloroverbase" ) ) );
		m_pBlendTintColorOverBaseTextEntry->SetEnabled( bEnable );

		vgui::Label *pLabel = FindControl<vgui::Label>( "BlendTintColorOverBaseLabel" );
		if ( pLabel )
		{
			pLabel->SetEnabled( bEnable );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateColorTintBase( int nSkinIndex, const char* pszColor )
{
	m_strColorTintBase[nSkinIndex] = pszColor;

	int colorTintBase[3];
	// color string +2 to skip "{ " or "[ "
	const char *pszStrippedColorTintBase = pszColor + 2;
	UTIL_StringToIntArray( colorTintBase, 3, pszStrippedColorTintBase );
	if ( m_pColorTintBaseRedTextEntry )
	{
		m_pColorTintBaseRedTextEntry->SetText( CFmtStr( "%d", colorTintBase[0] ) );
	}
	if ( m_pColorTintBaseGreenTextEntry )
	{
		m_pColorTintBaseGreenTextEntry->SetText( CFmtStr( "%d", colorTintBase[1] ) );
	}
	if ( m_pColorTintBaseBlueTextEntry )
	{
		m_pColorTintBaseBlueTextEntry->SetText( CFmtStr( "%d", colorTintBase[2] ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateAdditive()
{
	if ( m_pAdditiveCheckButton )
	{
		m_VMTKeyValues->SetBool( "$additive", m_pAdditiveCheckButton->IsSelected() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateTranslucent()
{
	if ( m_pTranslucentCheckButton )
	{
		bool bSelected = m_pTranslucentCheckButton->IsSelected();
		UpdateAlphaTestDiaplay( bSelected );
		m_VMTKeyValues->SetBool( "$translucent", bSelected );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateAlphaTest()
{
	if ( m_pAlphaTestCheckButton )
	{
		m_VMTKeyValues->SetBool( "$alphatest", m_pAlphaTestCheckButton->IsSelected() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateAlphaTestDiaplay( bool bEnable )
{
	if ( m_pAlphaTestCheckButton )
	{
		m_pAlphaTestCheckButton->SetEnabled( bEnable );
		m_pAlphaTestCheckButton->SetCheckButtonCheckable( bEnable );
	}

	vgui::Label *pLabel = FindControl<vgui::Label>( "AlphaTestLabel" );
	if ( pLabel )
	{
		pLabel->SetEnabled( bEnable );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateSelfIllum()
{
	if ( m_pSelfIllumCheckButton )
	{
		bool bSelected = m_pSelfIllumCheckButton->IsSelected();
		m_VMTKeyValues->SetBool( ">=DX90/$selfillum", bSelected );
		UpdateSelfIllumDisplay( bSelected );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateSelfIllumDisplay( bool bEnable )
{
	m_bSelfIllumEnabled = bEnable;

	static const char* pszSelfIllumLabels[] =
	{
		"SelfIllumTintLabel",
		"SelfIllumTextureLabel"
	};

	for ( int i=0; i<ARRAYSIZE(pszSelfIllumLabels); ++i )
	{
		vgui::Label *pLabel = FindControl<vgui::Label>( pszSelfIllumLabels[i] );
		if ( pLabel )
		{
			pLabel->SetEnabled( bEnable );
		}
	}

	static const char* pszSelfIllumButtons[] =
	{
		"SelfIllumTextureBrowse",
		"SelfIllumTextureClear"
	};

	for ( int i=0; i<ARRAYSIZE(pszSelfIllumButtons); ++i )
	{
		vgui::Button *pButton = FindControl<vgui::Button>( pszSelfIllumButtons[i] );
		if ( pButton )
		{
			pButton->SetEnabled( bEnable );
		}
	}

	m_pSelfIllumTextureFileLabel->SetEnabled( bEnable );
	m_pSelfIllumTintRedTextEntry->SetEnabled( bEnable );
	m_pSelfIllumTintGreenTextEntry->SetEnabled( bEnable );
	m_pSelfIllumTintBlueTextEntry->SetEnabled( bEnable );

	// blend tint color over base is not compatible with selfillum. must turn it off
	UpdateBlendTintColorOverBaseDisplay( !bEnable );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateSelfIllumTint( int nSkinIndex, const char* pszColor )
{
	m_strSelfIllumTint[nSkinIndex] = pszColor;

	// +1 to skip [
	const char *pszStrippedSelfIllumTintColor = pszColor + 1;
	float selfIllumTint[3];
	UTIL_StringToFloatArray( selfIllumTint, ARRAYSIZE( selfIllumTint ), pszStrippedSelfIllumTintColor );
	if ( m_pSelfIllumTintRedTextEntry )
	{
		m_pSelfIllumTintRedTextEntry->SetText( CFmtStr( "%d", int( selfIllumTint[0] * 255.f ) ) );
	}
	if ( m_pSelfIllumTintGreenTextEntry )
	{
		m_pSelfIllumTintGreenTextEntry->SetText( CFmtStr( "%d", int( selfIllumTint[1] * 255.f )  ) );
	}
	if ( m_pSelfIllumTintBlueTextEntry )
	{
		m_pSelfIllumTintBlueTextEntry->SetText( CFmtStr( "%d", int( selfIllumTint[2] * 255.f ) ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateEnvmapDisplay( bool bEnable )
{
	static const char* pszEnvmapLabels[] =
	{
		"EnvmapAlphaMaskLabel",
		"EnvmapTintLabel"
	};

	for ( int i=0; i<ARRAYSIZE(pszEnvmapLabels); ++i )
	{
		vgui::Label *pLabel = FindControl<vgui::Label>( pszEnvmapLabels[i] );
		if ( pLabel )
		{
			pLabel->SetEnabled( bEnable );
		}
	}

	m_pEnvmapAlphaMaskComboBox->SetEnabled( bEnable );
	m_pCubemapTintRedTextEntry->SetEnabled( bEnable );
	m_pCubemapTintGreenTextEntry->SetEnabled( bEnable );
	m_pCubemapTintBlueTextEntry->SetEnabled( bEnable );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImportMaterialEditDialog::UpdateUniqueLabels()
{
	static Color color[NUM_IMPORT_MATERIALS_PER_TEAM] = { Color( 189, 59, 59, 255 ), Color( 57, 92, 120, 255 ) };
	static const char* pszLabelNames[] =
	{
		"BaseTextureLabel",
		"ColorTintBaseLabel",
		"SelfIllumTintLabel"
	};

	for ( int i=0; i<ARRAYSIZE( pszLabelNames ); ++i )
	{
		vgui::Label* pLabel = FindControl<vgui::Label>( pszLabelNames[i] );
		if ( pLabel )
		{
			pLabel->InvalidateLayout( true, true );
			pLabel->SetFgColor( color[m_nSkinIndex] );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Preview an item after building it
//-----------------------------------------------------------------------------
class CImportPreviewItemPanel : public CTFStorePreviewItemPanel2
{
	DECLARE_CLASS_SIMPLE( CImportPreviewItemPanel, CTFStorePreviewItemPanel2 );

public:
	CImportPreviewItemPanel( vgui::Panel *parent, KeyValues *pItemValues, int nSelectedClass );
	virtual ~CImportPreviewItemPanel();

	virtual void	PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry = NULL ) OVERRIDE;

protected:
	void ResetHandles();

	virtual void OnThink();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodeTyped( KeyCode code );
	virtual void PerformLayout( void );
	virtual void OnClose();

	virtual void UpdatePlayerModelButtons();
	void UpdateActivityUI();
	void UpdateActivity();
	void UpdateMaterialButtons();

	virtual bool	AllowUnusualPreview() const { return true; }

	int GetSequence( const char *pszGesture = NULL );
	void StartAction();
	void StartGesture( const char *pszGesture );
	void EndGestures();

	bool ClassHasModels( int nClassIndex )
	{
		for ( int nModelIndex = 0; nModelIndex < NUM_IMPORT_LODS; ++nModelIndex )
		{
			if ( V_stricmp( m_pItemValues->GetString( CFmtStr( kClassLODNFile, kClassFolders[ nClassIndex ], nModelIndex ) ), "" ) != 0 )
				return true;
		}
		return false;
	}

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnClassIconSelected, "ClassIconSelected", data );

	void ResetCamera();

	vgui::ComboBox	*m_pLODComboBox;
	vgui::ComboBox	*m_pLoadoutComboBox;
	vgui::ComboBox	*m_pPoseComboBox;
	vgui::ComboBox	*m_pActionComboBox;
	vgui::ComboBox	*m_pEffectComboBox;
	CUtlVector< vgui::Button* > m_pMaterialButtons;

	KeyValues *m_pItemValues;
	int m_nCurrentLOD;
	loadout_positions_t m_nCurrentLoadout;
	CUtlString m_sCurrentPose;
	CUtlString m_sCurrentAction;
	bool m_bIsVCDFileNameOnly;
	float m_flGestureEndTime;
	int m_nSelectedClass;

	int m_nCacheModelDetail;
	bool m_bIsTaunt;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImportPreviewItemPanel::CImportPreviewItemPanel( vgui::Panel *parent, KeyValues *pItemValues, int nSelectedClass )
: CTFStorePreviewItemPanel2( parent, "resource/ui/ImportPreviewItemPanel.res", "importpreviewitem", NULL )
, m_pItemValues( pItemValues )
, m_nCurrentLOD( 0 )
, m_nCurrentLoadout( LOADOUT_POSITION_PRIMARY )
, m_sCurrentPose( kModelPoses[ kDefaultModelPoseIndex ] )
, m_bIsVCDFileNameOnly( true )
, m_flGestureEndTime( 0.0f )
, m_nSelectedClass( nSelectedClass )
{							   
	ResetHandles();

	static ConVarRef r_rootlod( "r_rootlod" );
	if ( r_rootlod.IsValid() )
	{
		m_nCacheModelDetail = r_rootlod.GetInt();
		r_rootlod.SetValue( 0 );
	}

	m_bIsTaunt = FStrEq( m_pItemValues->GetString( kItemPrefab ), "taunt" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CImportPreviewItemPanel::~CImportPreviewItemPanel()
{
	g_PlayerPreviewEffect.Reset();

	static ConVarRef r_rootlod( "r_rootlod" );
	if ( r_rootlod.IsValid() )
	{
		r_rootlod.SetValue( m_nCacheModelDetail );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::PreviewItem( int iClass, CEconItemView *pItem, const econ_store_entry_t* pEntry )
{
	if ( m_pPlayerModelPanel && m_iState == PS_PLAYER )
	{
		// Preserve state when refreshing the preview
		Vector vecSavedCameraOffset;
		Vector vecSavedCameraPos;
		QAngle angSavedCameraDir;
		m_pPlayerModelPanel->GetCameraOffset( vecSavedCameraOffset );
		m_pPlayerModelPanel->GetCameraPositionAndAngles( vecSavedCameraPos, angSavedCameraDir );

		int iSavedClass = m_iCurrentClass;

		BaseClass::PreviewItem( iClass, pItem, pEntry );

		KeyValuesAD pKeyValues( "data" );
		pKeyValues->SetInt( "class", iSavedClass );
		OnClassIconSelected( pKeyValues );

		m_pPlayerModelPanel->ResetCameraPivot();
		m_pPlayerModelPanel->SetCameraOffset( vecSavedCameraOffset );
		m_pPlayerModelPanel->SetCameraPositionAndAngles( vecSavedCameraPos, angSavedCameraDir );
		m_pPlayerModelPanel->SetForcedCameraPosition( true );

		m_pPlayerModelPanel->SetLOD( m_nCurrentLOD );
	}
	else
	{
		BaseClass::PreviewItem( iClass, pItem, pEntry );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::ResetHandles()
{
	m_pLODComboBox = NULL;
	m_pLoadoutComboBox = NULL;
	m_pPoseComboBox = NULL;
	m_pActionComboBox = NULL;
	m_pEffectComboBox = NULL;
	m_pMaterialButtons.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::OnThink()
{
	// We want to skip CTFStorePreviewItemPanel2::OnThink() because it closes when the mouse is clicked outside the dialog.
	CTFStorePreviewItemPanelBase::OnThink();

	// There's no event when the team color changes, but this is a super cheap call so we'll poll.
	g_PlayerPreviewEffect.SetTeam( GetPreviewTeam() );

	if ( m_flGestureEndTime && gpGlobals->curtime >= m_flGestureEndTime )
	{
		EndGestures();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	ResetHandles();

	BaseClass::ApplySchemeSettings( pScheme );

	// We don't use the scrollbar in this view
	if ( m_pScrollBar )
	{
		m_pScrollBar->MarkForDeletion();
		m_pScrollBar = NULL;
	}

	vgui::Button *pButton = FindControl<vgui::Button>( "AddToCartButton", true );
	if ( pButton )
	{
		pButton->SetVisible( false );
	}

	m_pLODComboBox = FindControl<vgui::ComboBox>( "LODComboBox", true );
	if ( m_pLODComboBox )
	{
		m_pLODComboBox->AddActionSignalTarget( this );
	}

	m_pLoadoutComboBox = FindControl<vgui::ComboBox>( "LoadoutComboBox", true );
	if ( m_pLoadoutComboBox )
	{
		m_pLoadoutComboBox->AddActionSignalTarget( this );
	}

	m_pPoseComboBox = FindControl<vgui::ComboBox>( "PoseComboBox", true );
	if ( m_pPoseComboBox )
	{
		m_pPoseComboBox->AddActionSignalTarget( this );
	}

	m_pActionComboBox = FindControl<vgui::ComboBox>( "ActionComboBox", true );
	if ( m_pActionComboBox )
	{
		m_pActionComboBox->AddActionSignalTarget( this );
	}

	m_pEffectComboBox = FindControl<vgui::ComboBox>( "EffectComboBox", true );
	if ( m_pEffectComboBox )
	{
		m_pEffectComboBox->RemoveAll();

		KeyValuesAD pKeyValues( "data" );
		for ( int iEffect = 0; iEffect < C_TFPlayerPreviewEffect::NUM_PREVIEW_EFFECTS; ++iEffect )
		{
			pKeyValues->SetInt( "effect", iEffect );
			int iRow = m_pEffectComboBox->AddItem( CFmtStr( "#TF_ImportPreview_Effect%d", iEffect ), pKeyValues );
			if ( g_PlayerPreviewEffect.GetEffect() == iEffect )
			{
				m_pEffectComboBox->ActivateItemByRow( iRow );
			}
		}

		m_pEffectComboBox->AddActionSignalTarget( this );
	}

	// Default to the character view, because that's the most valuable for preview here
	const CTFItemDefinition *pItemData = m_item.GetItemDefinition();
	if ( pItemData ) 
	{
		for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_LAST_NORMAL_CLASS; iClass++ )
		{
			if ( !pItemData->CanBeUsedByClass(iClass) )
				continue;

			KeyValuesAD pKeyValues( "data" );
			pKeyValues->SetInt( "class", iClass );
			OnClassIconSelected( pKeyValues );
			break;
		}
	}

	pButton = FindControl<vgui::Button>( "ButtonAction", true );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	pButton = FindControl<vgui::Button>( "ButtonEditQC", true );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	int nSkinType = m_pItemValues->GetInt( kSkinType );
	m_pMaterialButtons.SetCount( MAX_MATERIAL_COUNT );
	for ( int i = 0; i < MAX_MATERIAL_COUNT; ++i )
	{
		m_pMaterialButtons[i] = FindControl<vgui::Button>( CFmtStr( "ButtonEditMaterial%d", i ), true );
		if ( m_pMaterialButtons[i] )
		{
			int nSkinIndex = i / NUM_IMPORT_MATERIALS_PER_TEAM;
			int nMaterialIndex = i % NUM_IMPORT_MATERIALS_PER_TEAM;
			const char* pszTeam;
			if ( nSkinType == 0 )
			{
				pszTeam = "ALL";
			}
			else
			{
				pszTeam = nSkinIndex == 0 ? "RED" : "BLU";
			}
			m_pMaterialButtons[i]->SetText( CFmtStr( "Edit VMT%d %s", nMaterialIndex + 1, pszTeam ) );
			m_pMaterialButtons[i]->AddActionSignalTarget( this );
		}
	}

	// Fix up the fullscreen panel with our desired control set
	if ( m_pFullscreenPanel )
	{
		pButton = m_pFullscreenPanel->FindControl<vgui::Button>( "RotateLeftButton", true );
		if ( pButton )
		{
			pButton->SetVisible( false );
		}

		pButton = m_pFullscreenPanel->FindControl<vgui::Button>( "RotateRightButton", true );
		if ( pButton )
		{
			pButton->SetVisible( false );
		}

		pButton = m_pFullscreenPanel->FindControl<vgui::Button>( "ZoomButton", true );
		if ( pButton )
		{
			pButton->SetVisible( false );
		}
	}

	pButton = FindControl<vgui::Button>( "ButtonEditQCI", true );
	if ( pButton )
	{
		pButton->SetVisible( m_bIsTaunt );
	}

	vgui::Label *pLabel = FindControl<vgui::Label>( "AdvancedLabel", true );
	if ( pLabel )
	{
		pLabel->SetVisible( !m_bIsTaunt );
	}
	vgui::Panel *pPanel = FindControl<vgui::Panel>( "AdvancedFrame", true );
	if ( pPanel )
	{
		pPanel->SetVisible( !m_bIsTaunt );
	}

	UpdateActivityUI();
	UpdateMaterialButtons();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::OnCommand( const char *command )
{
	if ( V_strcasecmp( command, "show_explanations" ) == 0 )
	{
		CExplanationPopup *pPopup = dynamic_cast<CExplanationPopup*>( FindChildByName("StartExplanation") );
		if ( pPopup )
		{
			pPopup->Popup();
		}
	}
	else if ( V_strcasecmp( command, "action" ) == 0 )
	{
		StartAction();
	}
	else if ( V_strcasecmp( command, "BuildPreview" ) == 0 ||
			  V_strcasecmp( command, "EditQC" ) == 0 ||
			  V_strcasecmp( command, "EditQCI" ) == 0 ||
			  V_strncasecmp( command, "EditMaterial", V_strlen( "EditMaterial" ) ) == 0 )
	{
		// Dispatch directly to our parent because the base class tries to run the command through the console interpreter
		GetParent()->OnCommand( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void  CImportPreviewItemPanel::OnKeyCodeTyped( KeyCode code )
{
	if ( code == KEY_F5 )
	{
		OnCommand( "BuildPreview" );
		return;
	}

	BaseClass::OnKeyCodeTyped( code );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::PerformLayout( void )
{
	CTFStorePreviewItemPanel2::PerformLayout();

	PlaceControl( m_pDetailsViewChild, NULL, "UsedByLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "UsedByLabel", "UsedByTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "UsedByLabel", "SlotLabel", m_iSmallVerticalBreakSize, true );
	PlaceControl( m_pDetailsViewChild, "SlotLabel", "SlotTextLabel", m_iHorizontalBreakSize, false );
	PlaceControl( m_pDetailsViewChild, "SlotLabel", "AttributesLabel", m_iBigVerticalBreakSize, true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::OnClose()
{
	GetParent()->OnCommand( "PreviewDone" );
	MarkForDeletion();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::UpdatePlayerModelButtons()
{
	BaseClass::UpdatePlayerModelButtons();

	UpdateActivityUI();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::UpdateActivityUI()
{
	if ( m_pLODComboBox )
	{
		m_pLODComboBox->RemoveAll();
		m_pLODComboBox->SetText( "" );

		Assert( m_nSelectedClass != TF_CLASS_UNDEFINED );
		if ( m_iState == PS_PLAYER )
		{
			KeyValuesAD pKeyValues( "data" );
			int nDefaultRow = -1;
			int nLastValidLOD = 0;
			for ( int nLOD = 0; nLOD < NUM_IMPORT_LODS; ++nLOD )
			{
				KeyValues *pKey = m_pItemValues->FindKey( CFmtStr( kClassLODN, kClassFolders[ m_nSelectedClass ], nLOD ) );
				if ( pKey && *pKey->GetString( "file" ) )
				{
					nLastValidLOD = nLOD;
				}

				pKeyValues->SetInt( "LOD", nLOD );
				int nRow = m_pLODComboBox->AddItem( CFmtStr( "LOD%d", nLOD ), pKeyValues );
				if ( m_nCurrentLOD == nLOD )
				{
					nDefaultRow = nRow;
				}
			}
			Assert( nDefaultRow >= 0 );
			m_pLODComboBox->ActivateItemByRow( nDefaultRow );
		}
	}

	if ( m_pLoadoutComboBox )
	{
		m_pLoadoutComboBox->RemoveAll();
		m_pLoadoutComboBox->SetText( "" );

		if ( m_iState == PS_PLAYER )
		{
			KeyValuesAD pKeyValues( "data" );
			int nDefaultRow = -1;
			loadout_positions_t nFirstLoadout = LOADOUT_POSITION_INVALID;
			for ( int iLoadoutPosition = 0; iLoadoutPosition < CLASS_LOADOUT_POSITION_COUNT; ++iLoadoutPosition )
			{
				CEconItemView *pItem = TFInventoryManager()->GetBaseItemForClass( m_iCurrentClass, iLoadoutPosition );
				if ( pItem && pItem->IsValid() )
				{
					const char *pszLabel = ItemSystem()->GetItemSchema()->GetLoadoutStringsForDisplay( pItem->GetItemDefinition()->GetEquipType() )[ iLoadoutPosition ];
					pKeyValues->SetInt( "loadout", iLoadoutPosition );
					int nRow = m_pLoadoutComboBox->AddItem( pszLabel, pKeyValues );
					if ( iLoadoutPosition == m_nCurrentLoadout )
					{
						nDefaultRow = nRow;
					}
					if ( nFirstLoadout == LOADOUT_POSITION_INVALID )
					{
						nFirstLoadout = (loadout_positions_t)iLoadoutPosition;
					}
				}
			}
			Assert(nFirstLoadout != LOADOUT_POSITION_INVALID);

			if (nDefaultRow < 0)
			{
				// Didn't find our current loadout position, pick the first one
				nDefaultRow = 0;
				m_nCurrentLoadout = nFirstLoadout;
			}
			m_pLoadoutComboBox->ActivateItemByRow( nDefaultRow );
		}
	}

	if ( m_pPoseComboBox )
	{
		m_pPoseComboBox->RemoveAll();
		m_pPoseComboBox->SetText( "" );

		if ( m_iState == PS_PLAYER )
		{
			KeyValuesAD pKeyValues( "data" );
			int nDefaultRow = -1;
			for ( int iPose = 0; iPose < ARRAYSIZE(kModelPoses); ++iPose )
			{
				pKeyValues->SetString( "pose", kModelPoses[ iPose ] );
				int nRow = m_pPoseComboBox->AddItem( CFmtStr( "#TF_ImportPreview_Pose%s", kModelPoses[ iPose ] ), pKeyValues );
				if ( m_sCurrentPose == kModelPoses[ iPose ] )
				{
					nDefaultRow = nRow;
				}
			}
			Assert( nDefaultRow >= 0 );
			m_pPoseComboBox->ActivateItemByRow( nDefaultRow );
		}
	}

	if ( m_pActionComboBox )
	{
		m_pActionComboBox->RemoveAll();
		m_pActionComboBox->SetText( "" );

		if ( m_iState == PS_PLAYER )
		{
			KeyValuesAD pKeyValues("data");
			if ( m_bIsTaunt )
			{
				CTFFileImportDialog *pFileImportDialog = dynamic_cast<CTFFileImportDialog*>(GetParent());
				if (pFileImportDialog)
				{
					KeyValuesAD pItemSchema(pFileImportDialog->BuildItemSchema("item_preview"));

					const CUtlVector<const char *>&vecUsabilityStrings = ItemSystem()->GetItemSchema()->GetClassUsabilityStrings();
					KeyValues *pTauntKey = pItemSchema->FindKey("taunt");
					if (pTauntKey)
					{
						KeyValues *pCustomTauntPerClass = pTauntKey->FindKey("custom_taunt_scene_per_class");
						if (pCustomTauntPerClass)
						{
							const char *pSceneName = pCustomTauntPerClass->GetString(vecUsabilityStrings[m_nSelectedClass]);
							if (pSceneName && *pSceneName)
							{
								pKeyValues->SetString("action", pSceneName);
								pKeyValues->SetBool("file_name_only", false);
								m_pActionComboBox->AddItem("#TF_ImportPreview_Taunt", pKeyValues);
							}
						}
					}
				}
			}
			else
			{
				for (int iAction = 0; iAction < ARRAYSIZE(kModelActions); ++iAction)
				{
					pKeyValues->SetString("action", kModelActions[iAction]);
					pKeyValues->SetBool("file_name_only", true);
					m_pActionComboBox->AddItem(CFmtStr("#TF_ImportPreview_Action%d", iAction), pKeyValues);
				}
			}
			
			Assert( m_pActionComboBox->GetItemCount() >= 0 );
			m_pActionComboBox->ActivateItemByRow( 0 );

			// init default value
			KeyValues *pData = m_pActionComboBox->GetActiveItemUserData();
			m_sCurrentAction = pData->GetString( "action" );
			m_bIsVCDFileNameOnly = pData->GetBool( "file_name_only" );
		}
	}

	UpdateActivity();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::UpdateActivity()
{
	if ( !m_pPlayerModelPanel || !m_pPlayerModelPanel->GetStudioHdr() || m_iState != PS_PLAYER )
	{
		return;
	}

	static CEconItemView tempItem;
	if ( m_bIsTaunt && ClassHasModels( m_nSelectedClass ) )
	{
		// use preview item if specified for taunt
		tempItem.Init( PREVIEW_ITEM_DEFINITION_INDEX, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
	}
	else
	{
		// FIXME: Once we support weapons we'll need to check to see if we want to display the preview item here instead.
		CEconItemView *pItem = TFInventoryManager()->GetBaseItemForClass( m_iCurrentClass, m_nCurrentLoadout );
		Assert( pItem && pItem->IsValid() );
		tempItem = *pItem;
	}
	m_pPlayerModelPanel->SwitchHeldItemTo( &tempItem );

	int iSequence = GetSequence();
	if ( iSequence >= 0 )
	{
		m_pPlayerModelPanel->SetSequence( iSequence );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CImportPreviewItemPanel::GetSequence( const char *pszGesture )
{
	if ( !m_pPlayerModelPanel || !m_pPlayerModelPanel->GetStudioHdr() )
	{
		return -1;
	}

	CEconItemView *pItem = m_pPlayerModelPanel->GetHeldItem();
	if ( !pItem )
	{
		return -1;
	}

	int iAnimSlot = pItem->GetAnimationSlot();
	if ( iAnimSlot < 0 )
	{
		iAnimSlot = pItem->GetItemDefinition()->GetLoadoutSlot( m_iCurrentClass );
	}
	if ( iAnimSlot < 0 )
	{
		return -1;
	}

	CStudioHdr studioHdr( m_pPlayerModelPanel->GetStudioHdr(), g_pMDLCache );

	// Look for the bind pose by label since it's not an activity
	if ( !pszGesture && V_strcasecmp( m_sCurrentPose.Get(), "ref" ) == 0 )
	{
		for ( int iSeq = 0; iSeq < studioHdr.GetNumSeq(); ++iSeq )
		{
			mstudioseqdesc_t &seqDesc = studioHdr.pSeqdesc( iSeq );
			if ( V_strcasecmp( seqDesc.pszLabel(), m_sCurrentPose.Get() ) == 0 )
			{
				return iSeq;
			}
		}
		return -1;
	}

	CUtlString sActivity;
	const char *pszActivityOverride;
	CUtlString sAnimSuffix = GetItemSchema()->GetWeaponTypeSubstrings()[ iAnimSlot ];
	sAnimSuffix.ToUpper();

	if ( pszGesture )
	{
		CUtlString sGesture = pszGesture;
		sGesture.ToUpper();
		sActivity.Format( "ACT_MP_%s_%s_%s", sGesture.Get(), m_sCurrentPose.Get(), sAnimSuffix.Get() );
	}
	else
	{
		sActivity.Format( "ACT_MP_%s_%s", m_sCurrentPose.Get(), sAnimSuffix.Get() );
	}
	pszActivityOverride = pItem->GetItemDefinition()->GetActivityOverride( GetPreviewTeam(), sActivity );
	if ( pszActivityOverride )
	{
		sActivity = pszActivityOverride;
	}
	return m_pPlayerModelPanel->FindSequenceFromActivity( &studioHdr, sActivity.Get() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::UpdateMaterialButtons()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
		return;

	KeyValues* pModelKey = m_pItemValues->FindKey( CFmtStr( kClassLODN, kClassFolders[ m_nSelectedClass ], 0 ) );
	if ( !pModelKey)
		return;

	int nSkinType = m_pItemValues->GetInt( kSkinType );
	int nMaterialCount = pModelKey->GetInt( "materialCount" );
	for ( int i=0; i<m_pMaterialButtons.Count(); ++i )
	{
		int nSkinIndex = i / NUM_IMPORT_MATERIALS_PER_TEAM;
		int nMaterialIndex = i % NUM_IMPORT_MATERIALS_PER_TEAM;
		bool bVisible = nSkinIndex <= nSkinType && nMaterialIndex < nMaterialCount;
		m_pMaterialButtons[i]->SetVisible( bVisible );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::StartAction()
{
	const char *pszAction = m_sCurrentAction.String();
	if ( !pszAction || !*pszAction )
	{
		return;
	}

	if ( !m_pPlayerModelPanel )
	{
		return;
	}

	if ( V_strncasecmp( pszAction, "gesture_", 8 ) == 0 )
	{
		pszAction += 8;
		StartGesture( pszAction );
	}
	else
	{
		m_pPlayerModelPanel->PlayVCD( pszAction, NULL, false, m_bIsVCDFileNameOnly );
		UpdateActivity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::StartGesture( const char *pszGesture )
{
	int iSequence = GetSequence( pszGesture );
	if ( iSequence >= 0 )
	{
		CStudioHdr studioHdr( m_pPlayerModelPanel->GetStudioHdr(), g_pMDLCache );

		MDLSquenceLayer_t	tmpSequenceLayers[1];
		tmpSequenceLayers[0].m_nSequenceIndex = iSequence;
		tmpSequenceLayers[0].m_flWeight = 1.0;
		tmpSequenceLayers[0].m_bNoLoop = true;
		tmpSequenceLayers[0].m_flCycleBeganAt = 0.0f;
		m_pPlayerModelPanel->SetSequenceLayers( tmpSequenceLayers, 1 );

		float flGestureDuration = Studio_Duration( &studioHdr, iSequence, NULL );
		m_flGestureEndTime = gpGlobals->curtime + flGestureDuration;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::EndGestures()
{
	Assert(m_pPlayerModelPanel);
	m_pPlayerModelPanel->SetSequenceLayers( NULL, 0 );
	m_flGestureEndTime = 0.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr( "panel" ) );

	vgui::ComboBox *pComboBox = dynamic_cast< vgui::ComboBox * >( pPanel );
	if ( pComboBox )
	{
		if( pComboBox == m_pLODComboBox )
		{
			KeyValues *pData = pComboBox->GetActiveItemUserData();
			int nLOD = pData->GetInt( "LOD" );
			if ( nLOD != m_nCurrentLOD )
			{
				m_nCurrentLOD = nLOD;
				if ( m_pPlayerModelPanel )
				{
					m_pPlayerModelPanel->SetLOD( m_nCurrentLOD );
				}
			}
		}
		else if( pComboBox == m_pLoadoutComboBox )
		{
			KeyValues *pData = pComboBox->GetActiveItemUserData();
			loadout_positions_t nLoadout = (loadout_positions_t)pData->GetInt( "loadout" );
			if ( nLoadout != m_nCurrentLoadout )
			{
				m_nCurrentLoadout = nLoadout;
				UpdateActivity();
			}
		}
		else if ( pComboBox == m_pPoseComboBox )
		{
			KeyValues *pData = pComboBox->GetActiveItemUserData();
			const char *pszPose = pData->GetString( "pose" );
			if ( V_strcmp( pszPose, m_sCurrentPose ) != 0 )
			{
				m_sCurrentPose = pszPose;
				UpdateActivity();

				// Changing pose resets the camera
				ResetCamera();
			}
		}
		else if ( pComboBox == m_pActionComboBox )
		{
			KeyValues *pData = pComboBox->GetActiveItemUserData();
			m_sCurrentAction = pData->GetString( "action" );
			m_bIsVCDFileNameOnly = pData->GetBool( "file_name_only" );
		}
		else if ( pComboBox == m_pEffectComboBox )
		{
			KeyValues *pData = pComboBox->GetActiveItemUserData();
			g_PlayerPreviewEffect.SetEffect( (C_TFPlayerPreviewEffect::PREVIEW_EFFECT)pData->GetInt("effect") );
		}
		return; 
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::ResetCamera()
{
	if ( m_pPlayerModelPanel )
	{
		m_pPlayerModelPanel->SetForcedCameraPosition( false );
		m_pPlayerModelPanel->InvalidateLayout( true, true );
		m_pPlayerModelPanel->SetForcedCameraPosition( true );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CImportPreviewItemPanel::OnClassIconSelected( KeyValues *data )
{
	BaseClass::OnClassIconSelected( data );

	m_nSelectedClass = data->GetInt( "class" );

	// Clicking on a class icon resets the camera
	ResetCamera();
	UpdateMaterialButtons();
}


//-----------------------------------------------------------------------------
// Purpose: Import file dialog
//-----------------------------------------------------------------------------
CTFFileImportDialog::CTFFileImportDialog( vgui::Panel *parent ) 
	: Frame( parent, "ImportFileDialog" ),
	m_tempQC( 0, 0, CUtlBuffer::TEXT_BUFFER )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	SetScheme(scheme);
	SetProportional( true );

	m_nSelectedClass = TF_CLASS_UNDEFINED;
	m_nFileOpenMode = FILE_OPEN_NONE;

	m_pItemValues = new KeyValues( "ImportSession" );
	m_pItemValues->UsesEscapeSequences( true );

	m_pPreviewSchema = NULL;

	m_pNameTextEntry = NULL;
	m_pTypeComboBox = NULL;
	m_pSwapVMTButton = NULL;
	m_pSkinComboBox = NULL;
	m_pWorkshopIDTextEntry = NULL;
	m_pTFEnglishNameTextEntry = NULL;
	m_pPerforceCheckButton = NULL;
	m_pPartnerCheckButton = NULL;
	
	m_pEquipRegionPanel = NULL;
	m_pEquipRegionComboBox = NULL;

	m_pIconImagePanel = NULL;

	V_memset( m_pClassRadioButtons, 0, sizeof( m_pClassRadioButtons ) );
	V_memset( m_pClassHighlights, 0, sizeof( m_pClassHighlights ) );

	m_pBodygroupsPanel = NULL;

	m_pLODsPanel = NULL;

	m_pSkinsPanel = NULL;

	m_pTauntInputPanel = NULL;
	m_pAnimationSourceFile = NULL;
	m_pAnimationVCDFile = NULL;
	m_pAnimationDurationLabel = NULL;
	m_pAnimationPropLabel = NULL;
	m_pAnimationLoopCheckButton = NULL;
	m_pAnimationLoopStartTextEntry = NULL;

	m_pBuildButton = NULL;

	m_pTextEditDialog = NULL;
	m_pMaterialEditDialog = NULL;
	m_pPreviewDialog = NULL;
	m_pPlayerModelPanel = NULL;

	// force sv_cheats 1 so we can run some client command
	static ConVarRef sv_cheats("sv_cheats");
	if ( sv_cheats.IsValid() )
	{
		m_bWasCheatOn = sv_cheats.GetBool();
		if ( !m_bWasCheatOn )
		{
			engine->ClientCmd_Unrestricted( "sv_cheats 1\n" );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::~CTFFileImportDialog()
{
	m_pItemValues->deleteThis();

	if ( m_pPreviewSchema )
	{
		m_pPreviewSchema->deleteThis();
	}

	if ( !m_bWasCheatOn )
	{
		engine->ClientCmd_Unrestricted( "sv_cheats 0\n" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	vgui::Button *pButton;

	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/ImportFileDialog.res" );

	m_pNameTextEntry = FindControl<vgui::TextEntry>( "Name" );
	if ( m_pNameTextEntry )
	{
		m_pNameTextEntry->AddActionSignalTarget( this );
	}

	m_pTypeComboBox = FindControl<vgui::ComboBox>( "TypeComboBox" );
	if ( m_pTypeComboBox )
	{
		m_pTypeComboBox->RemoveAll();

		KeyValuesAD pKeyValues( "data" );

		// Add all the public prefabs
		const CEconItemSchema::PrefabMap_t &prefabMap = ItemSystem()->GetItemSchema()->GetPrefabMap();
		FOR_EACH_DICT( prefabMap, i )
		{
			KeyValues *pPrefabKeyValues = prefabMap [ i ];
			if ( pPrefabKeyValues->GetBool( "public_prefab" ) )
			{
				for ( int j=0; j<ARRAYSIZE(kPrefabs); ++j )
				{
					if ( V_strcmp( kPrefabs[j], pPrefabKeyValues->GetName() ) == 0 )
					{
						pKeyValues->SetString( kItemPrefab, pPrefabKeyValues->GetName() );
						m_pTypeComboBox->AddItem( CFmtStr( "#TF_ItemPrefab_%s", pPrefabKeyValues->GetName() ), pKeyValues );
					}
				}
			}
		}

		SetItemPrefab( kDefaultPrefab );

		m_pTypeComboBox->AddActionSignalTarget( this );
	}

	m_pEquipRegionPanel = FindControl<vgui::EditablePanel>( "EquipRegionPanel" );
	m_pEquipRegionComboBox = FindControl<vgui::ComboBox>( "EquipRegionComboBox", true );
	if ( m_pEquipRegionComboBox )
	{
		KeyValuesAD pKey( "data" );
		const CEconItemSchema::EquipRegionsList_t& list = ItemSystem()->GetItemSchema()->GetEquipRegionsList();
		CUtlStringList strEquipRegionNameList;

		for ( int j=0; j<list.Count(); ++j )
		{
			const char* pszEquipRegion = list[j].m_sName.Get();
			strEquipRegionNameList.CopyAndAddToTail( pszEquipRegion );
		}
		strEquipRegionNameList.Sort( strEquipRegionNameList.SortFunc );

		for ( int j=0; j<strEquipRegionNameList.Count(); ++j )
		{
			const char* pszEquipRegion = strEquipRegionNameList[j];
			pKey->SetString( kEquipRegion, pszEquipRegion );
			m_pEquipRegionComboBox->AddItem( pszEquipRegion, pKey );
		}

		SetEquipRegion( kDefaultEquipRegion );

		m_pEquipRegionComboBox->AddActionSignalTarget( this );
	}

	if ( vgui::Label* pLabel = FindControl<vgui::Label>( "WorkshopIDLabel" ) )
	{
		pLabel->SetVisible( p4 );
	}

	m_pWorkshopIDTextEntry = FindControl<vgui::TextEntry>( "WorkshopIDTextEntry" );
	if ( m_pWorkshopIDTextEntry )
	{
		m_pWorkshopIDTextEntry->SetVisible( p4 );
	}

	if ( vgui::Label* pLabel = FindControl<vgui::Label>( "TFEnglishNameLabel" ) )
	{
		pLabel->SetVisible( p4 );
	}
	
	m_pTFEnglishNameTextEntry = FindControl<vgui::TextEntry>( "TFEnglishNameTextEntry" );
	if ( m_pTFEnglishNameTextEntry )
	{
		m_pTFEnglishNameTextEntry->SetVisible( p4 );
	}

	m_pPerforceCheckButton = FindControl<vgui::CheckButton>( "PerforceCheckButton" );
	if ( m_pPerforceCheckButton )
	{
		m_pPerforceCheckButton->SetVisible( p4 );
	}

	m_pPartnerCheckButton = FindControl<vgui::CheckButton>( "PartnerCheckButton" );
	if ( m_pPartnerCheckButton )
	{
		m_pPartnerCheckButton->SetVisible( p4 );
	}

	m_pIconImagePanel = FindControl<vgui::ImagePanel>( "Icon", true );
	if ( m_pIconImagePanel )
	{
		// Set black background, and center the 512x512 backpack icon
		m_pIconImagePanel->SetFillColor( Color(0, 0, 0, 255) );
	}

	pButton = FindControl<vgui::Button>( "ButtonIconClear", true );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}
	pButton = FindControl<vgui::Button>( "ButtonIconBrowse", true );
	if ( pButton )
	{
		pButton->AddActionSignalTarget( this );
	}

	m_pPaintableCheckButtons.SetCount( 2 );
	for ( int i=0; i<m_pPaintableCheckButtons.Count(); ++i )
	{
		m_pPaintableCheckButtons[i] = FindControl<vgui::CheckButton>( CFmtStr( "Paintable%dCheckBox", i ), true );
		if ( m_pPaintableCheckButtons[i] )
		{
			m_pPaintableCheckButtons[i]->AddActionSignalTarget( this );
		}	
	}

	for ( int i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		m_pClassHighlights[ i ] = FindControl<vgui::Panel>( CFmtStr( "ClassHighlight%d", i ), true );
		m_pClassRadioButtons[i] = FindControl<vgui::RadioButton>( CFmtStr( "ButtonSelectClass%d", i ), true );
		if ( m_pClassRadioButtons[i] )
			m_pClassRadioButtons[i]->AddActionSignalTarget( this );
	}

	m_pBodygroupsPanel = FindControl<vgui::EditablePanel>( "BodygroupsPanel" );
	m_pBodygroups.SetCount( ARRAYSIZE( kBodygroupArray ) );
	for ( int i=0; i<m_pBodygroups.Count(); ++i )
	{
		m_pBodygroups[i] = FindControl<vgui::CheckButton>( CFmtStr( "Bodygroup%d", i ), true );
		AssertMsg( m_pBodygroups[i], CFmtStr( "Missing Bodygroup%d CheckButton in ImportFileDialog.res", i ) );
		if ( m_pBodygroups[i] )
		{
			m_pBodygroups[i]->AddActionSignalTarget( this );
			m_pBodygroups[i]->SetSelected( true );
			m_pBodygroups[i]->SetCheckButtonCheckable( false );
			m_pBodygroups[i]->SetText( kBodygroupArray[i] );
		}
	}

	m_pLODsPanel = FindControl<vgui::EditablePanel>( "LODsPanel" );
	m_pLODPanels.SetCount( NUM_IMPORT_LODS );
	m_pLODFiles.SetCount( NUM_IMPORT_LODS );
	m_pLODDetails.SetCount( NUM_IMPORT_LODS );
	for ( int i = 0; i < NUM_IMPORT_LODS; ++i )
	{
		m_pLODPanels[ i ] = FindControl<vgui::Panel>( CFmtStr( "LOD%dPanel", i ), true );
		m_pLODFiles[ i ] = FindControl<vgui::Label>( CFmtStr( "LOD%dFile", i ), true );
		m_pLODDetails[ i ] = FindControl<vgui::Label>( CFmtStr( "LOD%dDetails", i ), true );

		pButton = FindControl<vgui::Button>( CFmtStr( "ButtonLOD%dBrowse", i ), true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( CFmtStr( "ButtonLOD%dClear", i ), true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}
	}

	pButton = FindControl<vgui::Button>( "ButtonEditQC", true );
	if (pButton)
	{
		pButton->AddActionSignalTarget(this);
	}

	m_pSkinsPanel = FindControl<vgui::EditablePanel>( "SkinsPanel" );

	m_pSwapVMTButton = FindControl<vgui::Button>( "SwapVMTButton", true );
	if ( m_pSwapVMTButton )
	{
		m_pSwapVMTButton->AddActionSignalTarget( this );
	}

	m_pSkinComboBox = FindControl<vgui::ComboBox>( "SkinComboBox", true );
	if ( m_pSkinComboBox )
	{
		m_pSkinComboBox->RemoveAll();

		KeyValuesAD pKeyValues( "data" );
		for ( int j=0; j<NUM_IMPORT_MATERIALS_PER_TEAM; ++j )
		{
			pKeyValues->SetInt( kSkinType, j );
			m_pSkinComboBox->AddItem( CFmtStr( "#TF_ItemSkinType_%d", j ), pKeyValues );
		}

		m_pSkinComboBox->ActivateItemByRow( 0 );

		m_pSkinComboBox->AddActionSignalTarget( this );
	}

	m_pMaterialPanels.SetCount( MAX_MATERIAL_COUNT );
	m_pMaterialLabels.SetCount( MAX_MATERIAL_COUNT );
	m_pMaterialFiles.SetCount( MAX_MATERIAL_COUNT );
	for ( int i = 0; i < MAX_MATERIAL_COUNT; ++i )
	{
		m_pMaterialPanels[ i ] = FindControl<vgui::Panel>( CFmtStr( "Material%dPanel", i ), true );
		m_pMaterialLabels[ i ] = FindControl<vgui::Label>( CFmtStr( "Material%dLabel", i ), true );
		m_pMaterialFiles[ i ] = FindControl<vgui::Label>( CFmtStr( "Material%dFile", i ), true );

		pButton = FindControl<vgui::Button>( CFmtStr( "ButtonMaterial%dEdit", i ), true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( CFmtStr( "ButtonMaterial%dBrowse", i ), true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( CFmtStr( "ButtonMaterial%dClear", i ), true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}
	}

	m_pTauntInputPanel = FindControl<vgui::EditablePanel>( "TauntInputPanel" );
	if ( m_pTauntInputPanel )
	{
		m_pAnimationSourceFile = FindControl<vgui::Label>( "AnimationSourceFile", true );
		pButton = FindControl<vgui::Button>( "ButtonAnimationSourceBrowse", true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( "ButtonAnimationSourceClear", true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		m_pAnimationVCDFile = FindControl<vgui::Label>( "AnimationVCDFile", true );
		pButton = FindControl<vgui::Button>( "ButtonAnimationVCDBrowse", true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( "ButtonAnimationVCDClear", true );
		if ( pButton )
		{
			pButton->AddActionSignalTarget( this );
		}

		m_pAnimationDurationLabel = FindControl<vgui::Label>( "AnimDurationLabel", true );

		m_pAnimationLoopCheckButton = FindControl<vgui::CheckButton>( "AnimationLoopCheckButton", true );
		if ( m_pAnimationLoopCheckButton )
		{
			m_pAnimationLoopCheckButton->AddActionSignalTarget( this );
		}

		m_pAnimationLoopStartTextEntry = FindControl<vgui::TextEntry>( "AnimationLoopStartTextEntry", true );
		if ( m_pAnimationLoopStartTextEntry )
		{
			m_pAnimationLoopStartTextEntry->AddActionSignalTarget( this );
		}

		pButton = FindControl<vgui::Button>( "ButtonEditQCI", true );
		if (pButton)
		{
			pButton->AddActionSignalTarget(this);
		}
	}
	m_pAnimationPropLabel = FindControl<vgui::Label>( "AnimationPropLabel" );

	m_pBuildButton = FindControl<vgui::Button>( "ButtonBuild" );
	if ( m_pBuildButton )
	{
		m_pBuildButton->SetEnabled( false );
	}

	m_pPlayerModelPanel = dynamic_cast<CTFPlayerModelPanel*>( FindChildByName("classmodelpanel") );
	
	OnOpen();

	UpdateMaterialDisplay();
	UpdateBodygroupsDisplay();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommand( const char *command )
{
	if ( V_stricmp( command, "Load" ) == 0 )
	{
		OnCommandLoad();
	}
	else if ( V_stricmp( command, "Save" ) == 0 )
	{
		OnCommandSave();
	}
	else if ( V_stricmp( command, "ClearIcon" ) == 0 )
	{
		SetItemIcon( "" );
	}
	else if ( V_stricmp( command, "BrowseIcon" ) == 0 )
	{
		OnCommandBrowseIcon();
	}
	else if ( V_stricmp( command, "UpdateBodygroup" ) == 0 )
	{
		OnCommandUpdateBodygroup();
	}
	else if ( V_strncasecmp( command, "UpdatePaintable", V_strlen( "UpdatePaintable" ) ) == 0 )
	{
		int nMaterialIndex = V_atoi( &command[ V_strlen( "UpdatePaintable" ) ] );
		SetPaintable( m_pPaintableCheckButtons[nMaterialIndex]->IsSelected(), nMaterialIndex );
	}
	else if ( V_strncasecmp( command, "ClearLOD", V_strlen( "ClearLOD" ) ) == 0 )
	{
		int index = V_atoi( &command[ V_strlen( "ClearLOD" ) ] );

		SetLOD( m_nSelectedClass, index, "" );
	}
	else if ( V_strncasecmp( command, "BrowseLOD", V_strlen( "BrowseLOD" ) ) == 0 )
	{
		if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
		{
			ShowMessageBox( "#TF_ImportFile_SelectClassTitle", "#TF_ImportFile_SelectClass" );
			return;
		}

		int index = V_atoi( &command[ V_strlen( "BrowseLOD" ) ] );

		OnCommandBrowseLOD( index );
	}
	else if ( V_stricmp( command, "SwapVMT" ) == 0 )
	{
		OnCommandSwapVMT();
	}
	else if ( V_strncasecmp( command, "EditMaterialDone", V_strlen( "EditMaterialDone" ) ) == 0 )
	{
		int nSkinIndex, nMaterialIndex;
		if ( sscanf( command + V_strlen( "EditMaterialDone" ), "%d,%d", &nSkinIndex, &nMaterialIndex ) == 2 )
		{
			OnCommandEditMaterialDone( nSkinIndex, nMaterialIndex );
		}
	}
	else if ( V_strncasecmp( command, "EditMaterial", V_strlen( "EditMaterial" ) ) == 0 )
	{
		int nMaterialPanelIndex;
		if ( sscanf( command + V_strlen( "EditMaterial" ), "%d", &nMaterialPanelIndex ) == 1 )
		{
			int nSkinIndex = nMaterialPanelIndex / NUM_IMPORT_MATERIALS_PER_TEAM;
			int nMaterialIndex = nMaterialPanelIndex % NUM_IMPORT_MATERIALS_PER_TEAM;
			OnCommandEditMaterial( nSkinIndex, nMaterialIndex );
		}
	}
	else if ( V_stricmp( command, "UpdateAnimationLoopable" ) == 0 )
	{
		SetLoopableTaunt( IsLoopableTaunt(), GetAnimationLoopStartTime() );
	}
	else if ( V_stricmp( command, "ClearAnimationSource" ) == 0 )
	{
		SetAnimationSource( m_nSelectedClass, NULL );
	}
	else if ( V_stricmp( command, "ClearAnimationVCD" ) == 0 )
	{
		SetAnimationVCD( m_nSelectedClass, NULL );
	}
	else if ( V_stricmp( command, "BrowseAnimationSource" ) == 0 )
	{
		if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
		{
			ShowMessageBox( "#TF_ImportFile_SelectClassTitle", "#TF_ImportFile_SelectClass" );
			return;
		}

		OnCommandBrowseAnimationSource();
	}
	else if ( V_stricmp( command, "BrowseAnimationVCD" ) == 0 )
	{
		if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
		{
			ShowMessageBox( "#TF_ImportFile_SelectClassTitle", "#TF_ImportFile_SelectClass" );
			return;
		}

		OnCommandBrowseAnimationVCD();
	}
	else if ( V_stricmp( command, "EditQC" ) == 0 )
	{
		OnCommandEditQC();
	}
	else if ( V_stricmp( command, "EditQCI") == 0 )
	{
		OnCommandEditQCI();
	}
	else if ( V_stricmp( command, "EditQCDone" ) == 0 )
	{
		OnCommandEditQCDone();
	}
	else if ( V_stricmp( command, "EditQCIDone" ) == 0 )
	{
		OnCommandEditQCIDone();
	}
	else if ( V_stricmp( command, "BuildPreview" ) == 0 )
	{
		OnCommandBuild( BUILD_PREVIEW );
	}
	else if ( V_stricmp( command, "BuildVerify") == 0 )
	{
		OnCommandBuild( BUILD_VERIFY );
	}
	else if ( V_stricmp( command, "BuildFinal" ) == 0 )
	{
		OnCommandBuild( BUILD_FINAL );
	}
	else if ( V_stricmp( command, "PreviewDone" ) == 0 )
	{
		CleanupPreviewData();
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandLoad()
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectFile", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.txt,*.zip", "#TF_ImportFile_LoadSessionFileType", true);
	pDialog->AddActionSignalTarget( this );
	char pszStartPath[ MAX_PATH ];
	if ( g_pFullFileSystem->RelativePathToFullPath_safe( CFmtStr( kSteamWorkshopDir, GetWorkshopFolder() ), "GAME", pszStartPath ) )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = FILE_OPEN_LOAD;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandSave()
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectFile", vgui::FOD_SAVE );
	pDialog->AddFilter( "*.txt", "#TF_ImportFile_SaveSessionFileType", true);
	pDialog->AddActionSignalTarget( this );
	char pszStartPath[ MAX_PATH ];
	if ( g_pFullFileSystem->RelativePathToFullPath_safe( CFmtStr( kSteamWorkshopDir, GetWorkshopFolder() ), "GAME", pszStartPath ) )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = FILE_OPEN_SAVE;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandBrowseIcon()
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectImage", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.tga,*.psd", "#TF_ImportFile_IconFileType", true);
	pDialog->AddActionSignalTarget( this );
	const char *pszStartPath = tf_steam_workshop_import_icon_path.GetString();
	if ( pszStartPath && *pszStartPath )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = FILE_OPEN_ICON;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandBrowseLOD( int index )
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectModel", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.smd,*.dmx,*.fbx", "#TF_ImportFile_ModelFileType", true);
	pDialog->AddActionSignalTarget( this );
	const char *pszStartPath = tf_steam_workshop_import_model_path.GetString();
	if ( pszStartPath && *pszStartPath )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = static_cast<FileOpenMode>( FILE_OPEN_LOD0 + index );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandBrowseAnimationSource()
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectAnimationSource", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.smd,*.dmx,*.fbx", "#TF_ImportFile_AnimationSourceFileType", true);
	pDialog->AddActionSignalTarget( this );
	const char *pszStartPath = tf_steam_workshop_import_model_path.GetString();
	if ( pszStartPath && *pszStartPath )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = static_cast<FileOpenMode>( FILE_OPEN_ANIMATION_SOURCE );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandBrowseAnimationVCD()
{
	vgui::FileOpenDialog *pDialog = new vgui::FileOpenDialog( NULL, "#TF_ImportFile_SelectAnimationVCD", vgui::FOD_OPEN );
	pDialog->AddFilter( "*.vcd", "#TF_ImportFile_AnimationVCDFileType", true);
	pDialog->AddActionSignalTarget( this );
	const char *pszStartPath = tf_steam_workshop_import_model_path.GetString();
	if ( pszStartPath && *pszStartPath )
	{
		pDialog->SetStartDirectory( pszStartPath );
	}
	pDialog->DoModal();
	pDialog->Activate();
	m_nFileOpenMode = static_cast<FileOpenMode>( FILE_OPEN_ANIMATION_VCD );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandSwapVMT()
{
	bool bSwapPaint = false;
	for ( int nSkin=0; nSkin<CItemUpload::Manifest()->GetNumMaterialSkins(); ++nSkin )
	{
		KeyValues *pVMT0 = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkin ), 0 ) );
		KeyValues *pVMT1 = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkin ), 1 ) );
		if ( pVMT0 && pVMT1 )
		{
			KeyValuesAD pTemp( pVMT0->MakeCopy() );
			pVMT0->Clear();
			pVMT1->CopySubkeys( pVMT0 );
			pVMT1->Clear();
			pTemp->CopySubkeys( pVMT1 );

			if ( !bSwapPaint )
			{
				bool bPaintable0 = IsPaintable( 0 );
				bool bPaintable1 = IsPaintable( 1 );
				SetPaintable( bPaintable1, 0 );
				SetPaintable( bPaintable0, 1 );
				bSwapPaint = true;
			}
		}
	}

	UpdateMaterialDisplay();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditMaterial( int nSkinIndex, int nMaterialIndex )
{
	CTFImportMaterialEditDialog *pDialog = new CTFImportMaterialEditDialog( this, nSkinIndex, nMaterialIndex, GetItemValues() );
	pDialog->InvalidateLayout( true, true );

	CUtlBuffer sRedMaterialText, sBlueMaterialText;
	pDialog->SetVMTKeyValues( GetMaterialText( 0, nMaterialIndex, sRedMaterialText ), GetMaterialText( 1, nMaterialIndex, sBlueMaterialText ) );
	pDialog->DoModal();
	pDialog->Activate();

	m_pMaterialEditDialog = pDialog;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditMaterialDone( int nSkinIndex, int nMaterialIndex )
{
	if ( !m_pMaterialEditDialog )
	{
		return;
	}

	CUtlStringList oldMaterials;
	CUtlStringList oldBaseTextures;
	CUtlStringList oldNormalTextures;
	CUtlStringList oldPhongExponentTextures;
	CUtlStringList oldSelfIllumTextures;

	bool bAllTeam = m_pItemValues->GetInt( kSkinType ) == 0;
	const int nNumSkin = bAllTeam ? 1 : CItemUpload::Manifest()->GetNumMaterialSkins();
	for ( int nSkin=0; nSkin < nNumSkin; ++nSkin )
	{
		KeyValuesAD pKV( m_pMaterialEditDialog->GetVMTKeyValues( nSkin )->MakeCopy() );
		BUILD_RESULT nResult = ValidateMaterialValues( pKV, nMaterialIndex );
		if ( nResult != BUILD_OKAY )
		{
			ShowMessageBoxWithFile( "#TF_SteamWorkshop_Error", kBuildResultMessages[ nResult ], CFmtStr( "VMT%d", 1 + nMaterialIndex ) );
			return;
		}

		CUtlBuffer sOldText;
		GetMaterialText( nSkin, nMaterialIndex, sOldText );
		oldMaterials.CopyAndAddToTail( sOldText.String() );
		oldBaseTextures.CopyAndAddToTail( GetMaterialTextureFile( nSkin, nMaterialIndex, MATERIAL_FILE_BASETEXTURE ) );
		oldNormalTextures.CopyAndAddToTail( GetMaterialTextureFile( nSkin, nMaterialIndex, MATERIAL_FILE_NORMAL ) );
		oldPhongExponentTextures.CopyAndAddToTail( GetMaterialTextureFile( nSkin, nMaterialIndex, MATERIAL_FILE_PHONGEXPONENT ) );
		oldSelfIllumTextures.CopyAndAddToTail( GetMaterialTextureFile( nSkin, nMaterialIndex, MATERIAL_FILE_SELFILLUM ) );

		// set new material text
		CUtlBuffer sNewText;
		sNewText.SetBufferType( true, true );
		pKV->RecursiveSaveToFile( sNewText, 0, false, true );
		sNewText.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
		SetMaterialText( nSkin, nMaterialIndex, sNewText.String() );

		// set new textures
		SetMaterial( nSkin, nMaterialIndex, m_pMaterialEditDialog->GetBaseTextureFile( nSkin ), MATERIAL_FILE_BASETEXTURE );
		SetMaterial( nSkin, nMaterialIndex, m_pMaterialEditDialog->GetNormalTextureFile(), MATERIAL_FILE_NORMAL );
		SetMaterial( nSkin, nMaterialIndex, m_pMaterialEditDialog->GetPhongExponentTextureFile(), MATERIAL_FILE_PHONGEXPONENT );
		SetMaterial( nSkin, nMaterialIndex, m_pMaterialEditDialog->GetSelfIllumTextureFile(), MATERIAL_FILE_SELFILLUM );
	}

	if ( m_pPreviewDialog )
	{
		// Refresh the preview and see if everything builds correctly
		if ( !OnCommandBuild( BUILD_PREVIEW ) )
		{
			for ( int nSkin=0; nSkin<nNumSkin; ++nSkin )
			{
				SetMaterialText( nSkin, nMaterialIndex, oldMaterials[nSkin] );
				SetMaterial( nSkin, nMaterialIndex, oldBaseTextures[nSkin], MATERIAL_FILE_BASETEXTURE );
				SetMaterial( nSkin, nMaterialIndex, oldNormalTextures[nSkin], MATERIAL_FILE_NORMAL );
				SetMaterial( nSkin, nMaterialIndex, oldPhongExponentTextures[nSkin], MATERIAL_FILE_PHONGEXPONENT );
				SetMaterial( nSkin, nMaterialIndex, oldSelfIllumTextures[nSkin], MATERIAL_FILE_SELFILLUM );
			}
			return;
		}
	}

	m_pMaterialEditDialog->OnCommand( "Close" );
	m_pMaterialEditDialog.Set(INVALID_PANEL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditQC()
{
	// The QC template is dependent on the item type, so make sure that's set first
	if ( V_strcmp( GetItemPrefab(), "" ) == 0 )
	{
		ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_BuildFailedNoType" );
		return;
	}

	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		ShowMessageBox( "#TF_ImportFile_SelectClassTitle", "#TF_ImportFile_SelectClass" );
		return;
	}

	CTFFileImportTextEditDialog *pDialog = new CTFFileImportTextEditDialog( this, "#TF_ImportFile_EditQC", "EditQCDone" );
	pDialog->SetText( GetQCTemplate( m_nSelectedClass ) );
	pDialog->DoModal();
	pDialog->Activate();

	m_pTextEditDialog = pDialog;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditQCI()
{
	// The QC template is dependent on the item type, so make sure that's set first
	if ( V_strcmp(GetItemPrefab(), "") == 0 )
	{
		ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_BuildFailedNoType" );
		return;
	}

	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		ShowMessageBox( "#TF_ImportFile_SelectClassTitle", "#TF_ImportFile_SelectClass" );
		return;
	}

	CTFFileImportTextEditDialog *pDialog = new CTFFileImportTextEditDialog(this, "#TF_ImportFile_EditQCI", "EditQCIDone");
	pDialog->SetText( GetQCITemplate( m_nSelectedClass ) );
	pDialog->DoModal();
	pDialog->Activate();

	m_pTextEditDialog = pDialog;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditQCDone()
{
	if ( !m_pTextEditDialog )
	{
		return;
	}
	
	char pszText[MAX_TEXT_EDIT_SIZE];
	m_pTextEditDialog->GetText( pszText, sizeof(pszText) );

	static const char *pszRequiredText[] = {
		"<ITEMTEST_REPLACE_MDLABSPATH>",
		"<ITEMTEST_REPLACE_LOD0>",
		"<ITEMTEST_REPLACE_SKIN_OPTIONALBLOCK>",
	};
	for ( int i = 0; i < ARRAYSIZE(pszRequiredText); ++i )
	{
		if ( !V_strstr( pszText, pszRequiredText[i] ) )
		{
			KeyValuesAD pData( "Data" );
			pData->SetString( "text", pszRequiredText[i] );
			ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_QCMissingText", pData );
			return;
		}
	}

	static const char *pszUnsupportedText[] =
	{
		"$keyvalues",
	};
	for ( int i = 0; i < ARRAYSIZE(pszUnsupportedText); ++i )
	{
		if ( V_strstr( pszText, pszUnsupportedText[i] ) )
		{
			KeyValuesAD pData( "Data" );
			pData->SetString( "text", pszUnsupportedText[i] );
			ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_QCUnsupportedText", pData );
			return;
		}
	}

	const char* pszQCKeyName = CFmtStr( kClassQC, kClassFolders[ m_nSelectedClass ] );
	CUtlString sOldText = GetItemValues()->GetString( pszQCKeyName );
	GetItemValues()->SetString( pszQCKeyName, pszText );

	if ( m_pPreviewDialog )
	{
		// Refresh the preview and see if everything builds correctly
		if ( !OnCommandBuild( BUILD_PREVIEW ) )
		{
			GetItemValues()->SetString( pszQCKeyName, sOldText );
			return;
		}
	}

	m_pTextEditDialog->OnCommand( "Close" );
	m_pTextEditDialog.Set(INVALID_PANEL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandEditQCIDone()
{
	if ( !m_pTextEditDialog )
	{
		return;
	}
	
	char pszText[MAX_TEXT_EDIT_SIZE];
	m_pTextEditDialog->GetText( pszText, sizeof(pszText) );

	const char* pszQCKeyName = CFmtStr( kClassQCI, kClassFolders[ m_nSelectedClass ] );
	CUtlString sOldText = GetItemValues()->GetString( pszQCKeyName );
	GetItemValues()->SetString( pszQCKeyName, pszText );

	if ( m_pPreviewDialog )
	{
		// Refresh the preview and see if everything builds correctly
		if ( !OnCommandBuild( BUILD_PREVIEW ) )
		{
			GetItemValues()->SetString( pszQCKeyName, sOldText );
			return;
		}
	}

	m_pTextEditDialog->OnCommand( "Close" );
	m_pTextEditDialog.Set(INVALID_PANEL);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::OnCommandBuild( BUILD_STAGE buildStage )
{
	KeyValuesAD pBuildMessageVariables( "data" );

	// this can take a while, put up a waiting cursor
	vgui::surface()->SetCursor( vgui::dc_hourglass );

	// Set up Perforce integration
	CItemUpload::SetP4( ShouldP4AddOrEdit() );
	if ( CItemUpload::GetP4() )
	{
		CUtlString sChangeList( "imported - " );
		sChangeList += GetItemName();

		g_p4factory->SetOpenFileChangeList( NULL );
		g_p4factory->SetOpenFileChangeList( sChangeList.String() );
	}

	BUILD_RESULT nResult = Build( buildStage, pBuildMessageVariables );

	if ( CItemUpload::GetP4() )
	{
		g_p4factory->SetOpenFileChangeList( NULL );
	}

	// change the cursor back to normal
	vgui::surface()->SetCursor( vgui::dc_user );

	if ( nResult == BUILD_OKAY )
	{
		switch ( buildStage )
		{
		case BUILD_PREVIEW:
			{
				if ( !SetupPreviewData() )
				{
					ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_PreviewFailed" );
					return false;
				}
				m_pPreviewDialog->SetVisible( true );
			}
			break;
		case BUILD_VERIFY:
			{
				SetDirty( false );
			}
			break;
		case BUILD_FINAL:
			{
				const char *pszBuildOutput = V_GetFileName( GetItemValues()->GetString( kBuildOutput ) );
				ShowMessageBoxWithFile( "#TF_ImportFile_ImportCompleteTitle", "#TF_ImportFile_ImportComplete", pszBuildOutput );
				SetWorkshopData();
				BaseClass::OnCommand( "Close" );
			}
			break;
		default:
			Assert( 0 );
		}
		return true;
	}

	// The build failed.
	SetDirty( true );
	if ( nResult == BUILD_FAILED_COMPILE )
	{
		CTFFileImportTextEditDialog *pDialog = new CTFFileImportTextEditDialog( this, "#TF_ImportFile_BuildFailed" );
		pDialog->HideCancelButton();
		pDialog->SetText( pBuildMessageVariables->GetString( "log" ) );
		pDialog->DoModal();
		pDialog->Activate();
	}
	else
	{
		ShowMessageBox( "#TF_ImportFile_BuildFailed", kBuildResultMessages[ nResult ], pBuildMessageVariables );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnCommandUpdateBodygroup()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	KeyValuesAD pBodygroupKey( kBodygroup );
	for ( int i=0; i<m_pBodygroups.Count(); ++i )
	{
		if ( m_pBodygroups[i] && !m_pBodygroups[i]->IsSelected() )
		{
			pBodygroupKey->SetBool( kBodygroupArray[i], true );
		}
	}
	SetBodygroup( pBodygroupKey );

	SetDirty( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnOpen()
{
	// Make sure our workshop directory exists
	const char *pszWorkshopDir = CFmtStr( kSteamWorkshopDir, GetWorkshopFolder() );
	g_pFullFileSystem->CreateDirHierarchy( pszWorkshopDir, "GAME" );

	GetWorkshopData();

	if ( !CheckSourceSDK() )
	{
		ShowMessageBox( "#TF_SteamWorkshop_Error", "#TF_ImportFile_BuildFailedNoSDK" );
	}

	CUtlString sName;
	if ( CItemUpload::SanitizeName( GetItemName(), sName ) )
	{
		char pszSessionPath[ MAX_PATH ];
		V_ComposeFileName( pszWorkshopDir, CFmtStr( "%s.txt", sName.Get() ), pszSessionPath, sizeof(pszSessionPath) );
		CUtlString sFailedPath;
		LOAD_RESULT nResult = Load( pszSessionPath, "GAME", sFailedPath );

		// We ignore a general load failed since it's okay if the file is missing, but if some of the internal
		// data is missing from a valid session we want to let the user know.
		if ( nResult == LOAD_FAILED_BADMODEL || nResult == LOAD_FAILED_BADMATERIAL )
		{
			ShowMessageBoxWithFile( "#TF_ImportFile_LoadFailed", kLoadResultMessages[ nResult ], sFailedPath );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnClose()
{
	CleanupPreviewData();

	CUtlString sName;
	if ( CItemUpload::SanitizeName( GetItemName(), sName ) )
	{
		char pszSessionPath[ MAX_PATH ];
		V_ComposeFileName( CFmtStr( kSteamWorkshopDir, GetWorkshopFolder() ), CFmtStr( "%s.txt", sName.Get() ), pszSessionPath, sizeof(pszSessionPath) );
		SAVE_RESULT nResult = Save( pszSessionPath, "GAME" ); 
		if ( nResult != SAVE_OKAY )
		{
			ShowMessageBoxWithFile( "#TF_ImportFile_SaveFailed", kSaveResultMessages[ nResult ], pszSessionPath );
		}
	}

	BaseClass::OnClose();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>( data->GetPtr( "panel" ) );

	vgui::ComboBox *pComboBox = dynamic_cast< vgui::ComboBox * >( pPanel );
	if ( pComboBox )
	{
		if( pComboBox == m_pTypeComboBox )
		{
			KeyValues *pData = m_pTypeComboBox->GetActiveItemUserData();
			SetItemPrefab( pData->GetString( kItemPrefab ) );
		}
		else if ( pComboBox == m_pSkinComboBox )
		{
			KeyValues *pData = m_pSkinComboBox->GetActiveItemUserData();
			SetSkinType( pData->GetInt( kSkinType ) );
		}
		else if ( pComboBox == m_pEquipRegionComboBox )
		{
			KeyValues *pData = m_pEquipRegionComboBox->GetActiveItemUserData();
			SetEquipRegion( pData->GetString( kEquipRegion ) );
		}

		return; 
	}

	vgui::TextEntry *pTextEntry = dynamic_cast< vgui::TextEntry * >( pPanel );
	if ( pTextEntry )
	{
		if ( pTextEntry == m_pNameTextEntry )
		{
			char name[256];
			m_pNameTextEntry->GetText( name, sizeof(name) );
			GetItemValues()->SetString( kItemName, name );
		}
		else if ( pTextEntry == m_pWorkshopIDTextEntry )
		{
			char szID[256];
			m_pWorkshopIDTextEntry->GetText( szID, ARRAYSIZE( szID ) );
			GetItemValues()->SetString( kWorkshopID, szID );
		}
		else if ( pTextEntry == m_pTFEnglishNameTextEntry )
		{
			char name[256];
			m_pTFEnglishNameTextEntry->GetText( name, sizeof(name) );
			GetItemValues()->SetString( kTFEnglishName, name );
		}
		else if ( pTextEntry == m_pAnimationLoopStartTextEntry )
		{
			const float flAnimationLoopStartTime =  m_pAnimationLoopStartTextEntry->GetValueAsFloat();
			GetItemValues()->SetFloat( kAnimationLoopable, flAnimationLoopStartTime );
		}
		return;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnFileSelected( char const *fullpath )
{
	switch ( m_nFileOpenMode )
	{
	case FILE_OPEN_LOAD:
		{
			CUtlString sFailedPath;
			LOAD_RESULT nResult = Load( fullpath, NULL, sFailedPath ); 
			if ( nResult != LOAD_OKAY )
			{
				ShowMessageBoxWithFile( "#TF_ImportFile_LoadFailed", kLoadResultMessages[ nResult ], sFailedPath );
			}
		}
		break;
	case FILE_OPEN_SAVE:
		{
			SAVE_RESULT nResult = Save( fullpath, NULL ); 
			if ( nResult != SAVE_OKAY )
			{
				ShowMessageBoxWithFile( "#TF_ImportFile_SaveFailed", kSaveResultMessages[ nResult ], fullpath );
			}
		}
		break;
	case FILE_OPEN_ICON:
		{
			SetItemIcon( fullpath );
			SaveBrowsePath( tf_steam_workshop_import_icon_path, fullpath );
		}
		break;
	case FILE_OPEN_LOD0:
	case FILE_OPEN_LOD1:
	case FILE_OPEN_LOD2:
		{
			KeyValuesAD pKV( "data" );
			LOAD_RESULT result = SetLOD( m_nSelectedClass, (m_nFileOpenMode - FILE_OPEN_LOD0), fullpath, pKV );
			if ( result != LOAD_OKAY )
			{
				ShowMessageBox( "#TF_ImportFile_LoadFailed", kLoadResultMessages[ result ], pKV );
			}
			SaveBrowsePath( tf_steam_workshop_import_model_path, fullpath );
		}
		break;
	case FILE_OPEN_ANIMATION_SOURCE:
		{
			KeyValuesAD pKV( "data" );
			LOAD_RESULT result = SetAnimationSource( m_nSelectedClass, fullpath, pKV );
			if ( result != LOAD_OKAY )
			{
				ShowMessageBox( "#TF_ImportFile_LoadFailed", kLoadResultMessages[ result ], pKV );
			}
			SaveBrowsePath( tf_steam_workshop_import_model_path, fullpath );
		}
		break;
	case FILE_OPEN_ANIMATION_VCD:
		{
			KeyValuesAD pKV( "data" );
			LOAD_RESULT result = SetAnimationVCD( m_nSelectedClass, fullpath, pKV );
			if ( result != LOAD_OKAY )
			{
				ShowMessageBox( "#TF_ImportFile_LoadFailed", kLoadResultMessages[ result ], pKV );
			}
			SaveBrowsePath( tf_steam_workshop_import_model_path, fullpath );
		}
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::OnRadioButtonChecked( Panel *panel )
{
	int i;

	for ( i = TF_FIRST_NORMAL_CLASS; i < TF_LAST_NORMAL_CLASS; ++i )
	{
		if ( panel == m_pClassRadioButtons[i] )
		{
			SelectClass( i );
			return;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetItemName( const char *pszName )
{
	if ( m_pNameTextEntry )
	{
		m_pNameTextEntry->SetText( pszName );
	}
	GetItemValues()->SetString( kItemName, pszName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetItemName()
{
	const char *pszName = GetItemValues()->GetString( kItemName );

	// Skip "The " if someone put it into the item name
	const char *kSkip = "The ";
	if ( V_strncasecmp(pszName, kSkip, V_strlen( kSkip ) ) == 0 )
	{
		pszName += V_strlen( kSkip );
	}
	return pszName;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsValidPrefab( const char *pszPrefab )
{
	KeyValues *pPrefab = ItemSystem()->GetItemSchema()->FindDefinitionPrefabByName( GetItemPrefab() );
	return pPrefab && pPrefab->GetBool( "public_prefab" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetItemPrefab( const char *pszPrefab )
{
	if ( m_pTypeComboBox )
	{
		ImportPrefab_t nOldPrefab = m_nPrefab;
		bool bFound = false;
		for ( int i = 0; i < m_pTypeComboBox->GetItemCount(); ++i )
		{
			KeyValues *pKeyValues = m_pTypeComboBox->GetItemUserData( m_pTypeComboBox->GetItemIDFromRow( i ) );
			if ( V_strcasecmp( pszPrefab, pKeyValues->GetString( kItemPrefab ) ) == 0 )
			{
				bFound = true;
				m_pTypeComboBox->ActivateItemByRow( i );
				m_nPrefab = ImportPrefab_t(i);
				break;
			}
		}

		if ( ( nOldPrefab != PREFAB_TAUNT && m_nPrefab == PREFAB_TAUNT ) ||
			 ( nOldPrefab == PREFAB_TAUNT && m_nPrefab != PREFAB_TAUNT ) )
		{
			ClearLODs();
		}

		pszPrefab = bFound ? pszPrefab : "";
		GetItemValues()->SetString( kItemPrefab, pszPrefab );
		UpdateUIForPrefab( m_nPrefab );
	}

	SetDirty( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateUIForPrefab( ImportPrefab_t nPrefab )
{
	if ( nPrefab == PREFAB_TAUNT )
	{
		if ( m_pEquipRegionPanel )
		{
			m_pEquipRegionPanel->SetVisible( false );
		}
		if ( m_pBodygroupsPanel )
		{
			m_pBodygroupsPanel->SetVisible( false );
		}
		if ( m_pTauntInputPanel )
		{
			m_pTauntInputPanel->SetVisible( true );
		}
		if ( m_pAnimationPropLabel )
		{
			m_pAnimationPropLabel->SetVisible( true );
		}

		UpdateAnimationSourceDisplay();
		UpdateAnimationVCDDisplay();
		UpdateAnimDurationDisplay();
	}
	else
	{
		if ( m_pEquipRegionPanel )
		{
			m_pEquipRegionPanel->SetVisible( true );
		}
		if ( m_pBodygroupsPanel )
		{
			m_pBodygroupsPanel->SetVisible( true );
		}
		if ( m_pTauntInputPanel )
		{
			m_pTauntInputPanel->SetVisible( false );
		}
		if ( m_pAnimationPropLabel )
		{
			m_pAnimationPropLabel->SetVisible( false );
		}

		UpdateLODDisplay();
		UpdateMaterialDisplay();
	}

	InvalidateLayout();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetItemPrefab()
{
	return GetItemValues()->GetString( kItemPrefab );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::GetItemPrefabValue( const char *pszPrefab, const char *pszName, CUtlString& strOutput )
{
	extern void MergeDefinitionPrefab( KeyValues *pKVWriteItem, KeyValues *pKVSourceItem );

	KeyValues *pPrefab = ItemSystem()->GetItemSchema()->FindDefinitionPrefabByName( pszPrefab );
	if ( !pPrefab )
	{
		return false;
	}

	KeyValuesAD pKVUnpackedPrefab( "prefab" );
	MergeDefinitionPrefab( pKVUnpackedPrefab, pPrefab );

	strOutput = pKVUnpackedPrefab->GetString( pszName, NULL );
	return !strOutput.IsEmpty();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SelectClass( int nClassIndex )
{
	m_nSelectedClass = nClassIndex;

	if ( m_nPrefab == PREFAB_TAUNT )
	{
		UpdateAnimationSourceDisplay();
		UpdateAnimationVCDDisplay();
		UpdateAnimDurationDisplay();
		UpdateLODDisplay();
		UpdateMaterialDisplay();
	}
	else
	{
		UpdateBodygroupsDisplay();
		UpdateLODDisplay();
		UpdateMaterialDisplay();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetItemIcon( const char *pszFilePath )
{
	if ( pszFilePath && *pszFilePath && m_pIconImagePanel )
	{
		// clean up old image if there's one
		m_pIconImagePanel->EvictImage();

		Bitmap_t image;
		ConversionErrorType nErrorCode = ImgUtl_LoadBitmap( pszFilePath, image );
		if ( nErrorCode != CE_SUCCESS )
		{
			m_pIconImagePanel->SetImage( (vgui::IImage *)0 );
			ShowMessageBoxWithFile( "#TF_ImportFile_LoadFailed", "#TF_ImportFile_LoadFailedBadFile", pszFilePath );
			return;
		}

		if ( image.Width() != 512 || image.Height() != 512 )
		{
			ShowMessageBoxWithFile( "#TF_ImportFile_LoadFailed", "#TF_ImportFile_LoadFailedBadIconResolution", pszFilePath );
			return;
		}

		if ( image.Format() != IMAGE_FORMAT_RGBA8888 )
		{
			ShowMessageBox( "#TF_ImportFile_LoadFailed", "#TF_ImportFile_LoadFailedImageNot32Bits" );
			return;
		}

		int badPixelCount = 0;
		static const int startSafeZoneY = 91;
		static const int endSafeZoneY = 420;
		for ( int y=0; y<512; ++y )
		{
			bool bSafeZone = y >= startSafeZoneY && y < endSafeZoneY;
			// check if icon is outside the safezone
			for ( int x=0; x<512; ++x )
			{
				RGBA8888_t* pPixel = (RGBA8888_t*)image.GetPixel( x, y );

				if ( bSafeZone )
				{
					// in the safezone, check if background is black
					if ( pPixel->a == 0 && ( pPixel->r || pPixel->g || pPixel->b ) )
					{
						badPixelCount++;
					}
				}
				else
				{
					if ( pPixel->r || pPixel->g || pPixel->b || pPixel->a )
					{
						ShowMessageBox( "#TF_ImportFile_LoadFailed", "#TF_ImportFile_LoadFailedImageDoesNotFitInsideSafeZone" );
						return;
					}
				}
			}
		}

		static const int nTotalSafeZonePixels = 512 * ( endSafeZoneY - startSafeZoneY );
		float flBadPercentage = (float)badPixelCount / nTotalSafeZonePixels;
		if ( flBadPercentage > 0.05f )
		{
			ShowMessageBox( "#TF_ImportFile_LoadFailed", "#TF_ImportFile_LoadFailedBadIconBackground" );
			return;
		}

		int wide, tall;
		BitmapImage *pBitmapImage = new BitmapImage;
		pBitmapImage->SetBitmap( image );
		pBitmapImage->GetSize( wide, tall );

		// The ImagePanel needs to know the scaling factor for the BitmapImage, to
		// center it when it's going to be rendered, and then the BitmapImage needs
		// to know how big it should be rendered.
		float fScaleAmount = static_cast<float>( m_pIconImagePanel->GetWide() ) / wide;
		m_pIconImagePanel->SetShouldCenterImage( true );
		m_pIconImagePanel->SetShouldScaleImage( true );
		m_pIconImagePanel->SetScaleAmount( fScaleAmount );
		pBitmapImage->SetRenderSize( m_pIconImagePanel->GetWide(), static_cast<int>( tall * fScaleAmount ) );

		m_pIconImagePanel->SetImage( pBitmapImage );
	}

	GetItemValues()->SetString( kItemIcon, pszFilePath );
	SetDirty( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetItemIcon()
{
	return GetItemValues()->GetString( kItemIcon );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetPaintable( bool bPaintable, int nMaterialIndex )
{
	const char* pszPaintable = CFmtStr( kItemPaintable, nMaterialIndex );
	GetItemValues()->SetBool( pszPaintable, bPaintable );

	if ( m_pPaintableCheckButtons[nMaterialIndex] )
	{
		m_pPaintableCheckButtons[nMaterialIndex]->SetSelected( bPaintable );
	}

	// You should check out how paints look on the item
	SetDirty( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsPaintable( int nMaterialIndex )
{
	const char* pszPaintable = CFmtStr( kItemPaintable, nMaterialIndex );
	return GetItemValues()->GetBool( pszPaintable );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsAnyVMTPaintable()
{
	for ( int nMaterialIndex=0; nMaterialIndex<NUM_IMPORT_MATERIALS_PER_TEAM; ++nMaterialIndex )
	{
		if ( IsPaintable( nMaterialIndex ) )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetUserAnimationQCTemplate( int nSelectedClass, bool bPerforce /*= false*/ )
{
	Assert( m_nPrefab == PREFAB_TAUNT );

	CUtlString strQCTemplateFile;
	if ( bPerforce )
	{
		// load class_workshop_animations.qc here!
		CAssetTF asset;
		asset.SetName( "temp" );
		asset.SetClass( GetClassFolder() );

		asset.AddModel();
		const char *pszClassFolder = kClassFolders[ nSelectedClass ];
		CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();
		const char *pszCustomRelativeDir = CFmtStr( "%s/player/animations", GetWorkshopFolder() );
		pQC->SetCustomRelativeDir( pszCustomRelativeDir );
		// use class_workshop_animations as shipping QC
		const char *pszCustomQCOutputName = CFmtStr( "%s_workshop_animations", pszClassFolder );
		pQC->SetCustomOutputName( pszCustomQCOutputName );
		pQC->SetQCITemplate( GetQCTemplate( nSelectedClass ) );

		// add temp DMX to get QC path
		const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassAnimationSourceFile, pszClassFolder ) );
		asset.AddTargetDMX( pszFilePath );

		m_tempQC.Clear();
		if ( pQC->GetOutputPath( strQCTemplateFile, 0 ) )
		{
			// if the QC was checked out, revert it first
			char szCorrectCaseFilePath[MAX_PATH];
			g_pFullFileSystem->GetCaseCorrectFullPath( strQCTemplateFile.String(), szCorrectCaseFilePath );
			CP4AutoRevertFile revertFile( szCorrectCaseFilePath );

			if ( g_pFullFileSystem->ReadFile( strQCTemplateFile, NULL, m_tempQC ) )
			{
				m_tempQC.PutString( "\n" );
				m_tempQC.PutString( "$pushd \"../../../<QCI_RELATIVE_DIR>\"\n" );
				m_tempQC.PutString( "$include \"../../../<QCI_RELATIVE_PATH>\"\n" );
				m_tempQC.PutString( "$popd\n" );

				return (char *)m_tempQC.Base();
			}
		}
	}
	else if ( GetItemPrefabValue( GetItemPrefab(), "qc_template", strQCTemplateFile ) )
	{
		m_tempQC.Clear();
		if ( g_pFullFileSystem->ReadFile( strQCTemplateFile, "MOD", m_tempQC ) )
		{
			return (char *)m_tempQC.Base();
		}
	}

	Warning( "Failed to load specified QC template '%s'.\n", strQCTemplateFile.String() );
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetQCTemplate( int nSelectedClass )
{
	const char *pszQCText = GetItemValues()->GetString( CFmtStr( kClassQC, kClassFolders[nSelectedClass] ) );
	if ( V_strlen(pszQCText) == 0 )
	{
		CUtlString strQCTemplateFile;
		if ( GetItemPrefabValue( m_nPrefab == PREFAB_TAUNT ? "misc" : GetItemPrefab(), "qc_template", strQCTemplateFile ) )
		{
			m_tempQC.Clear();
			if ( g_pFullFileSystem->ReadFile( strQCTemplateFile, "MOD", m_tempQC ) )
			{
				return (char *)m_tempQC.Base();
			}
			else
			{
				Warning( "Failed to load specified QC template '%s'.\n", strQCTemplateFile.String() );
			}
		}

		return CAssetTF().GetTargetQC()->GetQCTemplate();
	}
	return pszQCText;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetQCITemplate( int nSelectedClass )
{
	const char *pszQCText = GetItemValues()->GetString( CFmtStr( kClassQCI, kClassFolders[nSelectedClass] ) );
	if ( V_strlen(pszQCText) == 0 )
	{
		CUtlString strQCITemplateFile = CItemUpload::Manifest()->GetQCITemplate();
		CAssetTF asset;
		m_tempQC.Clear();
		if ( g_pFullFileSystem->ReadFile( strQCITemplateFile.String(), "MOD", m_tempQC ) )
		{
			return (char *)m_tempQC.Base();
		}
		else
		{
			Warning( "Failed to load specified QCI template '%s'.\n", strQCITemplateFile.String() );
		}
	}
	return pszQCText;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::ShouldP4AddOrEdit() const
{
	return p4 && m_pPerforceCheckButton && m_pPerforceCheckButton->IsSelected();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsPartnerContent() const
{
	return m_pPartnerCheckButton && m_pPartnerCheckButton->IsVisible() && m_pPartnerCheckButton->IsSelected();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFileImportDialog::GetWorkshopFolder() const
{
	return IsPartnerContent() ? "workshop_partner" : "workshop";
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFileImportDialog::GetCustomBones( int selectedClass, const char* pszFileName, CUtlStringList& strBoneList )
{
	strBoneList.PurgeAndDeleteElements();

	CUtlBuffer hdrBuf;
	const studiohdr_t *pItemStudioHdr = NULL;
	if ( g_pFullFileSystem->ReadFile( pszFileName, NULL, hdrBuf ) )
	{
		pItemStudioHdr = reinterpret_cast< const studiohdr_t * >( hdrBuf.Base() );
	}

	if ( pItemStudioHdr == NULL )
	{
		return false;
	}

	if ( pItemStudioHdr )
	{
		m_pPlayerModelPanel->SetToPlayerClass( selectedClass );
		const studiohdr_t* pClassStudioHdr = m_pPlayerModelPanel->GetStudioHdr();
		for ( int iItemBone=0; iItemBone<pItemStudioHdr->numbones; ++iItemBone )
		{
			const char* pszItemBoneName = pItemStudioHdr->pBone( iItemBone )->pszName();
			bool bCustomBone = true;
			for ( int iClassBone=0; iClassBone<pClassStudioHdr->numbones; ++iClassBone )
			{
				const char* pszClassBoneName = pClassStudioHdr->pBone( iClassBone )->pszName();
				if ( FStrEq( pszItemBoneName, pszClassBoneName ) )
				{
					bCustomBone = false;
					break;
				}
			}

			if ( bCustomBone )
			{
				strBoneList.CopyAndAddToTail( pszItemBoneName );
			}
		}
	}

	return strBoneList.Count();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::ClearLODs()
{
	for ( int iClassIndex=TF_FIRST_NORMAL_CLASS; iClassIndex<TF_LAST_NORMAL_CLASS; ++iClassIndex )
	{
		for ( int iLOD=0; iLOD<NUM_IMPORT_LODS; ++iLOD )
		{
			KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ iClassIndex ], iLOD ) );
			if ( pKey )
			{
				pKey->Clear();
			}
		}

		if ( m_pClassHighlights[ iClassIndex ] )
		{
			m_pClassHighlights[ iClassIndex ]->SetVisible( false );
		}
	}
	
	Assert( !AnyClassHasModels() );
	ClearMaterials();

	SetDirty( true );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::SetLOD( int selectedClass, int nModelIndex, const char *pszFilePath, KeyValues* pKV /*= NULL*/ )
{
	if ( selectedClass == TF_CLASS_UNDEFINED )
	{
		return LOAD_FAILED;
	}

	// Make sure we can load the model first
	if ( *pszFilePath )
	{
		CAssetTF asset;
		if ( asset.AddTargetDMX( pszFilePath ) < 0 )
		{
			return LOAD_FAILED_BADMODEL;
		}

		int nMaxTris = GetModelTriangleBudget( selectedClass, nModelIndex );
		int nTriCount = asset.GetTargetDMX( 0 )->GetTriangleCount();
		if ( nTriCount > nMaxTris )
		{
			if ( pKV )
			{
				pKV->SetInt( "count", nTriCount );
				pKV->SetInt( "limit", nMaxTris );
				SetMessageFileVariable( pKV, pszFilePath );
			}

			return LOAD_FAILED_COMPLEXMODEL;
		}

		CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();
		pQC->SetQCTemplate( GetQCTemplate( m_nSelectedClass ) );

		// need to compile dmx to output mdl file to get studiohdr_t for bone name list
		asset.SetName( "temp" );
		asset.SetClass( kClassFolders[ selectedClass ] );
		if ( asset.CompilePreview() )
		{
			const CUtlVector< CUtlString >& strBuiltFiles = asset.GetBuiltFiles();
			CUtlString strMDLPath;
			for ( int i=0; i<strBuiltFiles.Count(); ++i )
			{
				char pszExtention[16];
				V_ExtractFileExtension( strBuiltFiles[i], pszExtention, ARRAYSIZE( pszExtention ) );
				if ( FStrEq( pszExtention, "mdl" ) )
				{
					strMDLPath = strBuiltFiles[i];
					break;
				}
			}

			CUtlStringList strBoneList;
			int nCustomBones = GetCustomBones( selectedClass, strMDLPath.String(), strBoneList );

			// remove all the temp file
			for ( int i=0; i<strBuiltFiles.Count(); ++i )
			{
				g_pFullFileSystem->RemoveFile( strBuiltFiles[i] );
			}

			const int nCustomBoneLimit = GetModelBoneBudget();
			if ( nCustomBones > nCustomBoneLimit )
			{
				CUtlString strCustomBones = strBoneList[0];
				for ( int i=1; i<nCustomBones; ++i )
				{
					strCustomBones += CFmtStr( ", %s", strBoneList[i] );
				}

				if ( pKV )
				{
					pKV->SetInt( "count", nCustomBones );
					pKV->SetInt( "limit", nCustomBoneLimit );
					pKV->SetString( "custom_bones", strCustomBones.String() );
					SetMessageFileVariable( pKV, pszFilePath );
				}

				return LOAD_FAILED_TOOMANYBONES;
			}
		}

		int nMaterialCount = asset.GetTargetVMTCount();
		if ( nMaterialCount > NUM_IMPORT_MATERIALS_PER_TEAM )
		{
			if ( pKV )
			{
				pKV->SetInt( "count", nMaterialCount );
				pKV->SetInt( "limit", NUM_IMPORT_MATERIALS_PER_TEAM );
				SetMessageFileVariable( pKV, pszFilePath );
			}
			return LOAD_FAILED_TOOMANYMATERIALS;
		}

		// check if material count is not more than the higher LOD
		if ( nModelIndex > 0 )
		{
			KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ selectedClass ], nModelIndex - 1 ) );
			Assert( pKey );
			if ( pKey )
			{
				int nHigherLODMaterialCount = pKey->GetInt( "materialCount" );
				if ( nHigherLODMaterialCount < nMaterialCount )
				{
					if ( pKV )
					{
						SetMessageFileVariable( pKV, pszFilePath );
					}
					return LOAD_FAILED_MATERIALCOUNTMISMATCH;
				}
			}
		}

		for ( int i = 0; i < nMaterialCount; ++i )
		{
			CFmtStr sMaterialName( kMaterialN, i );
			if ( !GetItemValues()->GetString( sMaterialName, NULL ) )
			{
				char pszMaterialId[MAX_PATH];
				V_FileBase( asset.GetTargetVMT( i )->GetMaterialId(), pszMaterialId, sizeof(pszMaterialId) );
				GetItemValues()->SetString( sMaterialName, pszMaterialId );
			}
		}

		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ selectedClass ], nModelIndex ), true );
		pKey->SetString( "file", pszFilePath );
		pKey->SetInt( "polyCount", asset.GetTargetDMX( 0 )->GetPolyCount() );
		pKey->SetInt( "triangleCount", nTriCount );
		pKey->SetInt( "vertexCount", asset.GetTargetDMX( 0 )->GetVertexCount() );
		pKey->SetInt( "materialCount",  nMaterialCount );

		// if we're already under the lowest budget, no need for more LODs
		if ( nTriCount <= GetModelTriangleBudget( selectedClass, NUM_IMPORT_LODS - 1 ) )
		{
			for ( int i=nModelIndex+1; i<NUM_IMPORT_LODS; ++i )
			{
				KeyValues *pKeyLOD = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ selectedClass ], i ) );
				if ( pKeyLOD )
				{
					pKeyLOD->Clear();
				}
			}
		}
	}
	else
	{
		for ( int i=nModelIndex; i<NUM_IMPORT_LODS; ++i )
		{
			KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ selectedClass ], i ) );
			if ( pKey )
			{
				pKey->Clear();
			}
		}

		if ( !AnyClassHasModels() )
		{
			ClearMaterials();
		}
	}

	SetDirty( true );

	if ( selectedClass == m_nSelectedClass )
	{
		UpdateLODDisplay();
	}
	UpdateMaterialDisplay();

	// Show a highlight for classes that have models
	if ( m_pClassHighlights[ selectedClass ] )
	{
		m_pClassHighlights[ selectedClass ]->SetVisible( ClassHasModels( selectedClass ) );
	}

	return LOAD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateLODDisplay()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	int nMinTriRequirement = GetModelTriangleBudget( m_nSelectedClass, NUM_IMPORT_LODS - 1 );
	int nPreviousTriCount = INT_MAX;

	for ( int i=0; i<NUM_IMPORT_LODS; ++i )
	{
		const char *pszFilePath = "";
		int nTriCount = 0;

		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ m_nSelectedClass ], i ) );
		if ( pKey )
		{
			pszFilePath = pKey->GetString( "file" );
			nTriCount = pKey->GetInt( "triangleCount" );
		}
		
		if ( nPreviousTriCount != 0 && nPreviousTriCount > nMinTriRequirement )
		{
			SetLODPanelEnable( true, i );
		}
		else
		{
			SetLODPanelEnable( false, i );
		}

		vgui::Label *pFileLabel = m_pLODFiles[ i ];
		if ( pFileLabel )
		{
			if ( *pszFilePath )
			{
				char file[MAX_PATH];
				V_FileBase( pszFilePath, file, sizeof(file) );
				pFileLabel->SetText( file );
			}
			else
			{
				pFileLabel->SetText( "#TF_PublishFile_NoFileSelected" );
			}
		}

		vgui::Label *pDetailsLabel = m_pLODDetails[ i ];
		if ( pDetailsLabel )
		{
			if ( *pszFilePath )
			{
				wchar_t details[1024];
				g_pVGuiLocalize->ConstructString_safe( details, "#TF_ImportFile_ModelDetails", pKey );
				pDetailsLabel->SetText( details, true );
			}
			else
			{
				pDetailsLabel->SetText( "" );
			}
		}

		nPreviousTriCount = nTriCount;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFileImportDialog::GetModelTriangleBudget( int selectedClass, int nModelIndex )
{
	// Return the budget for this item type
	if ( nModelIndex >= NUM_IMPORT_LODS )
	{
		Warning( "GetModelTriangleBudget: bad model index\n" );
		return 0;
	}

	int nTriangleBudget = kDefaultTriangleBudget[nModelIndex];

	CUtlString strTriangleBudget;
	if ( GetItemPrefabValue( GetItemPrefab(), CFmtStr( "triangle_budget_lod%d", nModelIndex ), strTriangleBudget ) )
	{
		nTriangleBudget = V_atoi( strTriangleBudget.String() );
	}

	return nTriangleBudget;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFFileImportDialog::GetModelBoneBudget()
{
	int nBoneBudget = kDefaultBoneBudget;
	CUtlString strBoneBudget;
	if ( GetItemPrefabValue( GetItemPrefab(), "custom_bone_budget", strBoneBudget ) )
	{
		nBoneBudget = V_atoi( strBoneBudget.String() );
	}
	
	return nBoneBudget;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::SetMaterial( int nMaterialPanelIndex, const char* pszFilePath, MATERIAL_FILE_TYPE fileType )
{
	int selectedSkin = nMaterialPanelIndex / NUM_IMPORT_MATERIALS_PER_TEAM;
	int nMaterialIndex = nMaterialPanelIndex % NUM_IMPORT_MATERIALS_PER_TEAM;

	return SetMaterial( selectedSkin, nMaterialIndex, pszFilePath, fileType );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::SetMaterial( int selectedSkin, int nMaterialIndex, const char *pszFilePath, MATERIAL_FILE_TYPE fileType )
{
	if ( *pszFilePath )
	{
		CAssetTF asset;
		CTargetTGA image( &asset, NULL );
		if ( !image.SetInputFile( pszFilePath ) )
		{
			return false;
		}

		const char *pszMaterialFilePrefix = s_pszMaterialFilePrefixes[fileType];

		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( selectedSkin ), nMaterialIndex ), true );
		pKey->SetString( CFmtStr( "%s_texture_file", pszMaterialFilePrefix ), pszFilePath );
		pKey->SetInt( CFmtStr( "%s_texture_width", pszMaterialFilePrefix ), image.GetWidth() );
		pKey->SetInt( CFmtStr( "%s_texture_height", pszMaterialFilePrefix ), image.GetHeight() );
		pKey->SetInt( CFmtStr( "%s_texture_channels", pszMaterialFilePrefix ), image.GetChannelCount() );
		pKey->SetBool( CFmtStr( "%s_texture_alpha", pszMaterialFilePrefix ), image.HasAlpha() );
	}
	else
	{
		static const char* s_removeKeyNames[] =
		{
			"%s_texture_file",
			"%s_texture_width", 
			"%s_texture_height",
			"%s_texture_channels",
			"%s_texture_alpha"
		};

		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( selectedSkin ), nMaterialIndex ) );
		if ( pKey )
		{
			const char *pszMaterialFilePrefix = s_pszMaterialFilePrefixes[fileType];
			for ( int i=0; i<ARRAYSIZE( s_removeKeyNames ); ++i )
			{
				KeyValues *pSubKey = pKey->FindKey( CFmtStr( s_removeKeyNames[i], pszMaterialFilePrefix ) );
				if ( pSubKey )
				{
					pKey->RemoveSubKey( pSubKey );
				}
			}
		}
	}

	SetDirty( true );

	UpdateMaterialDisplay( selectedSkin, nMaterialIndex );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char* CTFFileImportDialog::GetMaterialTextureFile( int selectedSkin, int nMaterialIndex, MATERIAL_FILE_TYPE fileType )
{
	KeyValues* pMaterialKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( selectedSkin ), nMaterialIndex ) );
	if ( pMaterialKey )
	{
		const char *pszMaterialFilePrefix = s_pszMaterialFilePrefixes[fileType];
		return pMaterialKey->GetString( CFmtStr( "%s_texture_file", pszMaterialFilePrefix ) );
	}
	
	return "";
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CUtlString CTFFileImportDialog::GetMaterialName( int selectedSkin, int nMaterialIndex )
{
	const char* pszFilePath = GetMaterialTextureFile( selectedSkin, nMaterialIndex, MATERIAL_FILE_BASETEXTURE );
	if ( FStrEq( pszFilePath, "" ) )
	{
		return "";
	}

	// Turn the texture name into vmt name
	CUtlString strMaterialName = V_GetFileName( pszFilePath );
	strMaterialName += ".vmt";

	return strMaterialName;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::ClearMaterials()
{
	KeyValues *pKeyParent = GetItemValues()->FindKey( "Materials" );
	if ( pKeyParent )
	{
		for ( int nMaterialIndex = 0; nMaterialIndex < NUM_IMPORT_MATERIALS_PER_TEAM; ++nMaterialIndex )
		{
			KeyValues *pKey = pKeyParent->FindKey( CFmtStr( "Material%d", nMaterialIndex ) );
			if ( pKey )
			{
				pKeyParent->RemoveSubKey( pKey );
				pKey->deleteThis();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::ClearMaterial( int nSkinIndex, int nMaterialIndex )
{
	KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex ), nMaterialIndex ) );
	if ( pKey )
	{
		pKey->Clear();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateMaterialDisplay()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	int nModelMaterials = 0;
	KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ m_nSelectedClass ], 0 ) );
	if ( pKey )
	{
		nModelMaterials = pKey->GetInt( "materialCount" );
	}

	if ( m_pSwapVMTButton )
	{
		m_pSwapVMTButton->SetVisible( nModelMaterials > 1 );
	}

	int nSkinType = GetItemValues()->GetInt( kSkinType );
	for ( int nMaterialPaneIndex = 0; nMaterialPaneIndex < MAX_MATERIAL_COUNT; ++nMaterialPaneIndex )
	{
		vgui::Panel *pMaterialPanel = m_pMaterialPanels[ nMaterialPaneIndex ];
		if (!pMaterialPanel)
		{
			continue;
		}

		int nSkinIndex = nMaterialPaneIndex / NUM_IMPORT_MATERIALS_PER_TEAM;
		int nMaterialIndex = nMaterialPaneIndex % NUM_IMPORT_MATERIALS_PER_TEAM;
		bool bWasVisible = pMaterialPanel->IsVisible();
		bool bVisible = nSkinIndex <= nSkinType && nMaterialIndex < nModelMaterials;
		pMaterialPanel->SetVisible( bVisible );

		if ( bVisible )
		{
			if ( !bWasVisible )
			{
				pMaterialPanel->InvalidateLayout( true, true );
			}
			UpdateMaterialDisplay( nSkinIndex, nMaterialIndex );
		}
	}

	// update paintable buttons
	for ( int nMaterialIndex=0; nMaterialIndex<m_pPaintableCheckButtons.Count(); ++nMaterialIndex )
	{
		bool bVisible = nMaterialIndex < nModelMaterials;
		if ( m_pPaintableCheckButtons[nMaterialIndex] )
		{
			m_pPaintableCheckButtons[nMaterialIndex]->SetVisible( bVisible );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateMaterialDisplay( int nSkinIndex, int nMaterialIndex )
{
	int nMaterialPanelIndex = nSkinIndex * NUM_IMPORT_MATERIALS_PER_TEAM + nMaterialIndex;
	if ( nMaterialPanelIndex >= MAX_MATERIAL_COUNT )
	{
		// nSkinIndex could be 2 for Preview Image
		return;
	}

	vgui::Label *pLabel = m_pMaterialLabels[ nMaterialPanelIndex ];
	if ( pLabel )
	{
		const char* pszInputName = "Input";
		if ( GetItemValues()->GetInt( kSkinType ) )
		{
			pszInputName = ( nSkinIndex == 0 ) ? "Red Input" : "Blue Input";
		}

		pLabel->SetText( CFmtStr( "%s: \"%s\"", pszInputName, GetItemValues()->GetString( CFmtStr( kMaterialN, nMaterialIndex ) ) ) );
	}

	pLabel = m_pMaterialFiles[ nMaterialPanelIndex ];
	if ( pLabel )
	{
		const char *pszFilePath = GetMaterialTextureFile( nSkinIndex, nMaterialIndex, MATERIAL_FILE_BASETEXTURE );
		if ( *pszFilePath )
		{
			char file[MAX_PATH];
			V_FileBase( pszFilePath, file, sizeof(file) );
			pLabel->SetText( file );
			pLabel->SetFgColor( Color( 255, 255, 255, 255 ) );
		}
		else
		{
			pLabel->SetText( "#TF_PublishFile_NoFileSelected" );
			pLabel->SetFgColor( Color( 255, 0, 0, 255 ) );
		}
	}
}

const char *CTFFileImportDialog::GetMaterialText( int nSkinIndex, int nMaterialIndex, CUtlBuffer &sMaterialText )
{
	sMaterialText.Clear();
	sMaterialText.SetBufferType( true, true );

	if ( !sMaterialText.TellPut() )
	{
		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex ), nMaterialIndex ) );
		if ( pKey )
		{
			KeyValuesAD pVMTKey( "VMT" );
			if ( pVMTKey->LoadFromBuffer( "VMT", pKey->GetString( "vmt" ) ) )
			{
				if ( ValidateMaterialValues( pVMTKey, nMaterialIndex ) == BUILD_OKAY )
				{
					sMaterialText.PutString( pKey->GetString( "vmt" ) );
				}
				else
				{
					// HACK! We had proxies bug that will invalidate the material
					// Try to update the proxies from the template to see if this fixes the issue
					KeyValues *pCurrentProxies = pVMTKey->FindKey( "Proxies" );
					if ( pCurrentProxies )
					{
						KeyValuesAD pKVTemplate( "Template" );
						if ( pKVTemplate->LoadFromFile( g_pFullFileSystem, "materials/models/player/items/templates/standard.vmt", "GAME" ) )
						{
							KeyValues *pTemplateProxiesKey = pKVTemplate->FindKey( "Proxies" );
							if ( pTemplateProxiesKey )
							{
								// clear old proxies and copy a new one
								pCurrentProxies->Clear();
								pCurrentProxies->RecursiveMergeKeyValues( pTemplateProxiesKey );

								// try again
								if ( ValidateMaterialValues( pVMTKey, nMaterialIndex ) == BUILD_OKAY )
								{
									sMaterialText.SetBufferType( true, true );
									pVMTKey->RecursiveSaveToFile( sMaterialText, 0, false, true );
									sMaterialText.SeekGet( CUtlBuffer::SEEK_HEAD, 0 );
								}
							}
						}
					}
				}
			}
		}
	}

	if ( !sMaterialText.TellPut() )
	{
		g_pFullFileSystem->ReadFile( "materials/models/player/items/templates/standard.vmt", "GAME", sMaterialText );
	}

	if ( !sMaterialText.TellPut() )
	{
		return NULL;
	}
	return sMaterialText.String();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::SetMaterialText( int nSkinIndex, int nMaterialIndex, const char* pszMaterialText )
{
	KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kMaterialSkinN, CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex ), nMaterialIndex ), true );
	if ( pKey )
	{
		pKey->SetString( "vmt", pszMaterialText );
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::ValidateMaterialValues( KeyValues *pKV, int nMaterialIndex )
{
	static const char *MATERIAL_SHADER = "VertexlitGeneric";
	static const int CONDITION_PAINTABLE = 0x01;
	static const int CONDITION_NOTPAINTABLE = 0x02;
	static const int MAX_MATERIAL_VARIABLES = 8;
	static struct MaterialCheck {
		BUILD_RESULT nErrorResult;
		int nCondition;
		const char *vecVariables[MAX_MATERIAL_VARIABLES];
	} sVecMaterialChecks[] = {
		{
			BUILD_FAILED_MATERIALMISSINGCLOAK, 0,
			{
				"$cloakPassEnabled", "Proxies/invis"
			}
		},
		{
			BUILD_FAILED_MATERIALMISSINGBURNING, 0,
			{
				"$detail", "$detailscale", "$detailblendfactor", "$detailblendmode", "Proxies/AnimatedTexture", "Proxies/BurnLevel"
			}
		},
		{
			BUILD_FAILED_MATERIALMISSINGJARATE, CONDITION_NOTPAINTABLE,
			{
				"$yellow", "Proxies/YellowLevel", "Proxies/Equals"
			}
		},
		{
			BUILD_FAILED_MATERIALMISSINGJARATE, CONDITION_PAINTABLE,
			{
				"$yellow", "Proxies/YellowLevel", "Proxies/Multiply"
			}
		},
		{
			BUILD_FAILED_MATERIALMISSINGPAINTABLE, CONDITION_PAINTABLE,
			{
				"$blendtintbybasealpha", "$blendtintcoloroverbase", "$colortint_base", "$color2", "$colortint_tmp",
					"Proxies/ItemTintColor", "Proxies/SelectFirstIfNonZero"
			}
		},
	};

	if ( V_strcasecmp( pKV->GetName(), MATERIAL_SHADER ) != 0 )
	{
		return BUILD_FAILED_MATERIALMISSINGSHADER;
	}

	for ( int iCheck = 0; iCheck < ARRAYSIZE(sVecMaterialChecks); ++iCheck )
	{
		const MaterialCheck &check = sVecMaterialChecks[ iCheck ];

		bool bIsPaintable = IsPaintable( nMaterialIndex );

		if ( ( check.nCondition & CONDITION_PAINTABLE ) && !bIsPaintable )
		{
			continue;
		}

		if ( ( check.nCondition & CONDITION_NOTPAINTABLE ) && bIsPaintable )
		{
			continue;
		}

		for ( int i = 0; i < ARRAYSIZE(check.vecVariables) && check.vecVariables[i]; ++i )
		{
			if ( !pKV->FindKey( check.vecVariables[i] ) )
			{
				return check.nErrorResult;
			}
		}
	}

	// Ugh, terrible hack...
	// If a KeyValues key doesn't have a value or subkeys, it won't get written out.
	// Unfortunately we need Proxies/invis to be written out as an empty set,
	// so it needs to have _something_ in it to force it to be written to disk.
	const char *kSubkeyHack = "hack_not_written_to_disk";
	KeyValues *pSubKey = pKV->FindKey( "Proxies/invis" );
	Assert(pSubKey);
	if ( !pSubKey->FindKey( kSubkeyHack ) )
	{
		pSubKey->AddSubKey( new KeyValues( kSubkeyHack ) );
	}

	return BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemoveUnnecessaryParametersFromVMT( KeyValues *pKV, int nMaterialIndex )
{
	RemoveLightParameters( pKV, nMaterialIndex );
	RemovePaintParameters( pKV, nMaterialIndex );
	RemoveTranslucentParameters( pKV );
	RemoveCubeMapParameters( pKV );
	RemoveSelfIllumParameters( pKV );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemoveLightParameters( KeyValues *pKV, int nMaterialIndex )
{
	static const char *vecLightingKeys[] =
	{
		"$basemapalphaphongmask", "$halflambert"
	};
	for ( int i=0; i<ARRAYSIZE(vecLightingKeys); ++i )
	{
		const char *pszLightKeyName = vecLightingKeys[i];
		if ( !pKV->GetBool( pszLightKeyName ) )
		{
			KeyValues *pSubKey = pKV->FindKey( pszLightKeyName );
			if ( pSubKey )
			{
				pKV->RemoveSubKey( pSubKey );
				pSubKey->deleteThis();
			}
		}
	}

	if ( FStrEq( GetMaterialTextureFile( 0, nMaterialIndex, MATERIAL_FILE_PHONGEXPONENT ), "" ) )
	{
		KeyValues* pSubKey = pKV->FindKey( "$rimmask" );
		if ( pSubKey )
		{
			pKV->RemoveSubKey( pSubKey );
			pSubKey->deleteThis();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemovePaintParameters( KeyValues *pKV, int nMaterialIndex )
{
	if ( IsPaintable( nMaterialIndex ) )
	{
		static const char *vecNonPaintableProxyKeys[] =
		{
			"Equals"
		};

		// Now remove proxies
		KeyValues *pProxyKV = pKV->FindKey( "Proxies" );
		if ( pProxyKV )
		{
			for ( int iProxyKey = 0; iProxyKey < ARRAYSIZE(vecNonPaintableProxyKeys); ++iProxyKey )
			{
				KeyValues *pKey = pProxyKV->FindKey( vecNonPaintableProxyKeys[ iProxyKey ] );
				if ( pKey )
				{
					pProxyKV->RemoveSubKey( pKey );
					pKey->deleteThis();
				}
			}
		}

		return;
	}

	static const char *vecPaintableKeys[] =
	{
		"$blendtintbybasealpha", "$blendtintcoloroverbase", "$colortint_base", "$colortint_tmp"
	};
	static const char *vecPaintableProxyKeys[] =
	{
		"ItemTintColor", "SelectFirstIfNonZero", "Multiply"
	};

	// First remove all the variables
	for ( int iKey = 0; iKey < ARRAYSIZE(vecPaintableKeys); ++iKey )
	{
		KeyValues *pKey = pKV->FindKey( vecPaintableKeys[ iKey ] );
		if ( pKey )
		{
			pKV->RemoveSubKey( pKey );
			pKey->deleteThis();
		}
	}

	// Now remove proxies
	KeyValues *pProxyKV = pKV->FindKey( "Proxies" );
	if ( pProxyKV )
	{
		for ( int iProxyKey = 0; iProxyKey < ARRAYSIZE(vecPaintableProxyKeys); ++iProxyKey )
		{
			KeyValues *pKey = pProxyKV->FindKey( vecPaintableProxyKeys[ iProxyKey ] );
			if ( pKey )
			{
				pProxyKV->RemoveSubKey( pKey );
				pKey->deleteThis();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemoveTranslucentParameters( KeyValues *pKV )
{
	if ( !pKV->GetBool( "$additive" ) )
	{
		KeyValues *pSubKey = pKV->FindKey( "$additive" );
		if ( pSubKey )
		{
			pKV->RemoveSubKey( pSubKey );
			pSubKey->deleteThis();
		}
	}

	if ( !pKV->GetBool( "$translucent" ) )
	{
		KeyValues *pSubKey = pKV->FindKey( "$translucent" );
		if ( pSubKey )
		{
			pKV->RemoveSubKey( pSubKey );
			pSubKey->deleteThis();
		}
		
		pSubKey = pKV->FindKey( "$alphatest" );
		if ( pSubKey )
		{
			pKV->RemoveSubKey( pSubKey );
			pSubKey->deleteThis();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemoveCubeMapParameters( KeyValues *pKV )
{
	static const char* vecEnvmapKeys[] =
	{
		"$envmap",
		"$basealphaenvmapmask",
		"$normalmapalphaenvmapmask",
		"$envmaptint"
	};

	const char* pszEnvmapName = pKV->GetString( "$envmap" );
	if ( FStrEq( pszEnvmapName, "" ) )
	{
		for ( int i=0; i<ARRAYSIZE(vecEnvmapKeys); ++i )
		{
			const char* pszEnvmapKey = vecEnvmapKeys[i];
			KeyValues *pSubKey = pKV->FindKey( pszEnvmapKey );
			if ( pSubKey )
			{
				pKV->RemoveSubKey( pSubKey );
				pSubKey->deleteThis();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::RemoveSelfIllumParameters( KeyValues *pKV )
{
	static const char* vecSelfIllumKeys[] =
	{
		"$selfillum",
		"$selfillumtint",
		"$selfillummask"
	};

	bool bSelfIllum = false;
	KeyValues *pDX9Key = pKV->FindKey( ">=DX90" );
	if ( pDX9Key )
	{
		bSelfIllum = pDX9Key->GetBool( "$selfillum" );
	}

	if ( !bSelfIllum )
	{
		for ( int i=0; i<ARRAYSIZE( vecSelfIllumKeys ); ++i )
		{
			KeyValues *pSubKey = pKV->FindKey( vecSelfIllumKeys[i] );
			if ( pSubKey )
			{
				pKV->RemoveSubKey( pSubKey );
				pSubKey->deleteThis();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateBodygroupsDisplay()
{
	KeyValues *pKey = GetItemValues()->FindKey( kBodygroup );
	for ( int i=0; i<ARRAYSIZE( kBodygroupArray ); ++i )
	{
		const char* pszBodygroupName = kBodygroupArray[i];
		vgui::CheckButton* pButton = m_pBodygroups[i];
		if ( pButton )
		{
			pButton->SetCheckButtonCheckable( true );
			pButton->SetSelected( !pKey || !pKey->GetBool( pszBodygroupName ) );

			if ( m_nSelectedClass != TF_CLASS_UNDEFINED )
			{
				m_pPlayerModelPanel->SetToPlayerClass( m_nSelectedClass );
				const studiohdr_t* pMDL = m_pPlayerModelPanel->GetStudioHdr();

				bool bEnabled = false;
				for ( int iBodygroup=0; iBodygroup<pMDL->numbodyparts; ++iBodygroup )
				{
					const char* pszValidBodygroup = pMDL->pBodypart( iBodygroup )->pszName();
					if ( FStrEq( pszValidBodygroup, pszBodygroupName ) )
					{
						bEnabled = true;
						break;
					}
				}

				pButton->SetEnabled( bEnabled );
			}
			pButton->SetCheckButtonCheckable( pButton->IsEnabled() );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetBodygroup( KeyValues* pBodygroupKey )
{
	if ( !pBodygroupKey )
	{
		return;
	}

	KeyValues *pKey = GetItemValues()->FindKey( kBodygroup, true );
	if ( pKey )
	{
		pKey->Clear();
		for ( int i=0; i<ARRAYSIZE( kBodygroupArray ); ++i )
		{
			const char* pszBodygroup = kBodygroupArray[i];
			if ( pBodygroupKey->GetBool( pszBodygroup ) )
			{
				pKey->SetBool( pszBodygroup, true );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::SetAnimationSource( int selectedClass, const char *pszFilePath, KeyValues* pKV /*= NULL*/ )
{
	if ( selectedClass == TF_CLASS_UNDEFINED )
	{
		SetMessageFileVariable( pKV, pszFilePath );
		return LOAD_FAILED;
	}

	// Make sure we can load the model first
	if ( pszFilePath && *pszFilePath )
	{
		CAssetTF asset;

		CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();
		pQC->SetQCTemplate( GetUserAnimationQCTemplate( selectedClass ) );
		pQC->SetQCITemplate( GetQCTemplate( selectedClass ) );

		if ( asset.AddTargetDMX( pszFilePath ) < 0 )
		{
			SetMessageFileVariable( pKV, pszFilePath );
			return LOAD_FAILED_BADMODEL;
		}

		CSmartPtr< CTargetDMX > pDMX = pQC->GetTargetDMX( 0 );
		float flFrameRate;
		int nFrameCount;
		float flAnimDuration = 0.f;
		if ( pDMX.IsValid() && pDMX->GetAnimationFrameInfo( flFrameRate, nFrameCount ) )
		{
			flAnimDuration = (float)nFrameCount/flFrameRate;
			if ( flAnimDuration > GetMaxTauntDuration() )
			{
				if ( pKV )
				{
					pKV->SetFloat( "current_anim_duration", flAnimDuration );
					pKV->SetInt( "max_anim_duration", GetMaxTauntDuration() );
				}
				SetMessageFileVariable( pKV, pszFilePath );

				return LOAD_FAIL_ANIMATIONTOOLONG;
			}
		}

		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ selectedClass ] ), true );
		pKey->SetString( "source_file", pszFilePath );

		SetAnimationDuration( selectedClass, flAnimDuration );
	}
	else
	{
		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ selectedClass ] ) );
		if ( pKey )
		{
			KeyValues *pVCDKey = pKey->FindKey( "source_file" );
			if ( pVCDKey )
			{
				pKey->RemoveSubKey( pVCDKey );
				pVCDKey->deleteThis();
			}
		}
	}

	SetDirty( true );

	if ( selectedClass == m_nSelectedClass )
	{
		UpdateAnimationSourceDisplay();
	}

	// Show a highlight for classes that have models
	if ( m_pClassHighlights[ selectedClass ] )
	{
		m_pClassHighlights[ selectedClass ]->SetVisible( ClassHasTauntSources( selectedClass ) );
	}

	return LOAD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::SetAnimationVCD( int selectedClass, const char *pszFilePath, KeyValues* pKV /*= NULL*/ )
{
	if ( selectedClass == TF_CLASS_UNDEFINED )
	{
		SetMessageFileVariable( pKV, pszFilePath );
		return LOAD_FAILED;
	}

	if ( pszFilePath && *pszFilePath )
	{
		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ selectedClass ] ), true );
		pKey->SetString( "vcd_file", pszFilePath );
	}
	else
	{
		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ selectedClass ] ) );
		if ( pKey )
		{
			KeyValues *pVCDKey = pKey->FindKey( "vcd_file" );
			if ( pVCDKey )
			{
				pKey->RemoveSubKey( pVCDKey );
				pVCDKey->deleteThis();
			}
		}
	}

	SetDirty( true );

	if ( selectedClass == m_nSelectedClass )
	{
		UpdateAnimationVCDDisplay();
	}

	// Show a highlight for classes that have models
	if ( m_pClassHighlights[ selectedClass ] )
	{
		m_pClassHighlights[ selectedClass ]->SetVisible( ClassHasTauntSources( selectedClass ) );
	}

	return LOAD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetAnimationDuration( int selectedClass, float flDuration )
{
	if ( selectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	float flClampedDuration = clamp( flDuration, 0.f, GetMaxTauntDuration() );
	KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ selectedClass ] ) );
	if ( pKey )
	{
		pKey->SetFloat( "taunt_duration", flClampedDuration );
	}

	UpdateAnimDurationDisplay();
}


extern CChoreoScene *LoadSceneForModel( const char *filename, IChoreoEventCallback *pCallback, float *flSceneEndTime );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::VerifyVCD( const CAssetTF &asset )
{
	if ( m_nPrefab != PREFAB_TAUNT )
		return BUILD_OKAY;

	// force reload scene cache
	scenefilecache->Reload();
	CUtlVector< int > vcdFileIndices;
	const CUtlVector< CUtlString > &builtFiles = asset.GetBuiltFiles();
	const CUtlVector< CUtlString > &relPathBuiltFiles = asset.GetRelativePathBuiltFiles();
	for ( int iFile=0; iFile<builtFiles.Count(); ++iFile )
	{
		char pszExtention[16];
		V_ExtractFileExtension( builtFiles[iFile].String(), pszExtention, ARRAYSIZE( pszExtention ) );
		if ( !FStrEq( pszExtention, "vcd" ) )
		{
			continue;
		}

		const char *pszFullPath = builtFiles[iFile].String();
		const char *pszRelPath = V_stristr( pszFullPath, "scenes" );
		if ( !pszRelPath )
		{
			return BUILD_FAILED_BAD_VCD_FILE;
		}

		float flSceneEndTime = -1.f;

		CChoreoScene *pScene = LoadSceneForModel( pszRelPath, NULL, &flSceneEndTime );
		if ( pScene )
		{
			CChoreoEvent *pEventSequence = NULL;
			for ( int iEvent=0; iEvent<pScene->GetNumEvents() && !pEventSequence; ++iEvent )
			{
				CChoreoEvent *pEvent = pScene->GetEvent( iEvent );
				if ( pEvent && pEvent->GetType() == CChoreoEvent::EVENTTYPE::SEQUENCE )
				{
					pEventSequence = pEvent;
				}
			}
		
			if ( !pEventSequence )
			{
				delete pScene;
				return BUILD_FAILED_VCD_MISSING_EVENT_SEQUENCE;
			}

			if ( pEventSequence->GetEndTime() > GetMaxTauntDuration() )
			{
				delete pScene;
				return BUILD_FAILED_VCD_EVENT_SEQUENCE_TOO_LONG;
			}

			char szFileName[MAX_PATH];
			V_FileBase( pszRelPath, szFileName, sizeof( szFileName ) );

			pEventSequence->SetParameters( szFileName );
			pScene->SaveToFile( pszFullPath );

			vcdFileIndices.AddToTail( iFile );

			delete pScene;
		}
	}

	const char *pszArchiveName = asset.GetArchivePath();
	if ( pszArchiveName && *pszArchiveName )
	{
		CUtlBuffer buf;
		if ( !g_pFullFileSystem->ReadFile( pszArchiveName, NULL, buf ) )
		{
			// failed to parse zip file
			return BUILD_FAILED_BAD_VCD_FILE;
		}

		IZip *pZip = IZip::CreateZip();
		if ( pZip )
		{
			pZip->ParseFromBuffer( buf.Base(), buf.Size() );
			for ( int i=0; i<vcdFileIndices.Count(); ++i )
			{
				int vcdIndex = vcdFileIndices[i];
				const char *pszFullPath = builtFiles[vcdIndex].String();
				const char *pszRelPath = relPathBuiltFiles[vcdIndex].String();
				pZip->AddFileToZip( pszRelPath, pszFullPath );
			}

			pZip->SaveToBuffer( buf );
			IZip::ReleaseZip( pZip );

			g_pFullFileSystem->WriteFile( pszArchiveName, NULL, buf );
		}
		else
		{
			// failed to parse zip file
			return BUILD_FAILED_BAD_VCD_FILE;
		}
	}

	return BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateAnimationSourceDisplay()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	if ( m_pAnimationSourceFile )
	{
		const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassAnimationSourceFile, kClassFolders[ m_nSelectedClass ] ) );
		if ( pszFilePath && *pszFilePath )
		{
			char file[MAX_PATH];
			V_FileBase( pszFilePath, file, sizeof(file) );
			m_pAnimationSourceFile->SetText( file );
		}
		else
		{
			m_pAnimationSourceFile->SetText( "#TF_PublishFile_NoFileSelected" );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateAnimationVCDDisplay()
{
	if ( m_nSelectedClass == TF_CLASS_UNDEFINED )
	{
		return;
	}

	if ( m_pAnimationVCDFile )
	{
		const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassAnimationVCDFile, kClassFolders[ m_nSelectedClass ] ) );
		if ( pszFilePath && *pszFilePath )
		{
			char file[MAX_PATH];
			V_FileBase( pszFilePath, file, sizeof(file) );
			m_pAnimationVCDFile->SetText( file );
		}
		else
		{
			m_pAnimationVCDFile->SetText( "#TF_PublishFile_NoFileSelected" );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::UpdateAnimDurationDisplay()
{
	if ( m_pAnimationDurationLabel )
	{
		float flDuration = GetItemValues()->GetFloat( CFmtStr( kClassAnimationDuration, kClassFolders[ m_nSelectedClass ] ) );
		wchar_t wszTemp[256];
		wchar_t wszCount[10];
		_snwprintf( wszCount, ARRAYSIZE( wszCount ), L"%.4f", flDuration );
		g_pVGuiLocalize->ConstructString_safe( wszTemp, g_pVGuiLocalize->Find("#TF_ImportFile_AnimationDuration"), 1, wszCount );
		m_pAnimationDurationLabel->SetText( wszTemp );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetDirty( bool bDirty )
{
	if ( m_pBuildButton )
	{
		m_pBuildButton->SetEnabled( !bDirty );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static int FindSuffix( const char *pszString, const char *pszSuffix )
{
	int nStringLen = V_strlen(pszString);
	int nSuffixLen = V_strlen(pszSuffix);
	int nSuffixOffset = nStringLen - nSuffixLen;
	if ( nSuffixOffset >= 0 && V_strcasecmp( (pszString + nSuffixOffset), pszSuffix ) == 0 )
	{
		return nSuffixOffset;
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
static bool FindTexture( const char *pszStrippedPath, const char *pszSuffix, CUtlString &sOutputPath )
{
	static const char *vecTextureExtensions[] = {
		".tga",
		".psd"
	};

	for ( int i = 0; i < ARRAYSIZE(vecTextureExtensions); ++i )
	{
		char pszFullPath[MAX_PATH];
		V_snprintf(pszFullPath, sizeof(pszFullPath), "%s%s%s", pszStrippedPath, pszSuffix, vecTextureExtensions[ i ]);
		if ( CItemUpload::FileExists( pszFullPath ) )
		{
			sOutputPath = pszFullPath;
			return true;
		}
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SetLoopableTaunt( bool bLoopable, float flLoopStartTime )
{
	GetItemValues()->SetFloat( kAnimationLoopable, flLoopStartTime );

	if ( m_pAnimationLoopCheckButton )
	{
		m_pAnimationLoopCheckButton->SetSelected( bLoopable );
	}

	if ( m_pAnimationLoopStartTextEntry )
	{
		m_pAnimationLoopStartTextEntry->SetEnabled( bLoopable );
		m_pAnimationLoopStartTextEntry->SetText( CFmtStr( "%f", bLoopable ? flLoopStartTime : -1.f ) );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsLoopableTaunt() const
{
	return m_pAnimationLoopCheckButton && m_pAnimationLoopCheckButton->IsSelected();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFFileImportDialog::GetAnimationLoopStartTime() const
{
	if ( m_pAnimationLoopStartTextEntry )
	{
		return m_pAnimationLoopStartTextEntry->GetValueAsFloat();
	}

	return 0.f;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::AddTauntToAsset( CAssetTF &asset, int nClassIndex, bool bIsMulticlass, BUILD_STAGE buildStage, KeyValues *pItemData, KeyValues *pBuildMessageVariables )
{
	const uint nPathFlags = (CTargetBase::PATH_FLAG_ALL & ~CTargetBase::PATH_FLAG_ABSOLUTE) | CTargetBase::PATH_FLAG_ZIP;
	const CUtlVector<const char *>&vecUsabilityStrings = ItemSystem()->GetItemSchema()->GetClassUsabilityStrings();

	bool bPreview = buildStage == BUILD_PREVIEW;

	asset.AddModel();

	const char *pszClassFolder = kClassFolders[ nClassIndex ];

	CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();

	asset.ExcludeFileExtension( "qc" );
	asset.ExcludeFileExtension( "mdl" );

	// Are we checking out files to perforce?
	bool bPerforce = !bPreview && ShouldP4AddOrEdit();

	pQC->GetCustomKeyValues()->SetString( "<CLASS_NAME>", pszClassFolder );
	pQC->SetQCTemplate( GetUserAnimationQCTemplate( nClassIndex, bPerforce ) );
	pQC->SetQCITemplate( GetQCITemplate( nClassIndex ) );

	const char *pszVCDFile = GetItemValues()->GetString( CFmtStr( kClassAnimationVCDFile, pszClassFolder ) );
	CSmartPtr< CTargetVCD > pVCD = pQC->GetTargetVCD();
	pVCD->SetInputFile( pszVCDFile );
	const char *pszCustomRelativeDir = CFmtStr( "%s/player/%s/low", GetWorkshopFolder(), vecUsabilityStrings[nClassIndex] );
	pVCD->SetCustomRelativeDir( pszCustomRelativeDir );
	if ( bPreview )
	{
		pVCD->SetCustomModPath( "custom/workshop" );
	}

	// Dir for MDL
	pszCustomRelativeDir = bPerforce ? CFmtStr( "%s/player/animations", GetWorkshopFolder() ) : "player";
	const char *pszAnimMDLMiddleName = bPerforce ? "workshop" : "user"; // use class_user_animations as temp MDL and class_workshop_animations as shipping MDL
	const char *pszCustomMDLOutputName = CFmtStr( "%s_%s_animations", pszClassFolder, pszAnimMDLMiddleName );
	CSmartPtr< CTargetMDL > pMDL = asset.GetTargetMDL();
	pMDL->SetCustomRelativeDir( pszCustomRelativeDir );
	pMDL->SetCustomOutputName( pszCustomMDLOutputName );
	if ( bPreview )
	{
		pMDL->SetCustomModPath( "custom/workshop" );
	}

	// Dir for QC
	pszCustomRelativeDir = bPerforce ? CFmtStr( "%s/player/animations", GetWorkshopFolder() ) : CFmtStr( "player/%s", pszClassFolder );
	const char *pszAnimQCMiddleName = bPerforce ? "workshop" : "preview"; // use class_preview_animations as temp QC and class_workshop_animations as shipping QC
	const char *pszCustomQCOutputName = CFmtStr( "%s_%s_animations", pszClassFolder, pszAnimQCMiddleName );
	pQC->SetCustomRelativeDir( pszCustomRelativeDir );
	pQC->SetCustomOutputName( pszCustomQCOutputName );
	pQC->SetIgnoreP4( !bPerforce );

	const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassAnimationSourceFile, pszClassFolder ) );
	if ( pszFilePath && *pszFilePath )
	{
		int nLOD = asset.AddTargetDMX( pszFilePath );
		if ( nLOD < 0 )
		{
			SetMessageFileVariable( pBuildMessageVariables, pszFilePath );
			return BUILD_FAILED_BADMODEL;
		}

		CSmartPtr< CTargetDMX > pTargetDMX = asset.GetTargetDMX( nLOD );

		if ( bIsMulticlass )
		{
			pTargetDMX->SetNameSuffix( CFmtStr( "_%s", pszClassFolder ) );
		}

		// Update the item data for the path inside the asset archive
		CUtlString sOutputPath;
		pTargetDMX->GetOutputPath( sOutputPath, 0, nPathFlags );
		pItemData->SetString( CFmtStr( kClassAnimationSourceFile, pszClassFolder ), sOutputPath );

		CUtlString sSequenceName;
		pTargetDMX->GetOutputPath( sSequenceName, 0, CTargetBase::PATH_FLAG_FILE );

		float flFrameRate;
		int nFrameCount;
		pTargetDMX->GetAnimationFrameInfo( flFrameRate, nFrameCount );
		float flTotalAnimTime = (float)nFrameCount / flFrameRate;

		// set custom keys for DMX
		pTargetDMX->GetCustomKeyValues()->SetString( "<ITEMTEST_SEQUENCE_NAME>", sSequenceName.String() );
		pTargetDMX->GetCustomKeyValues()->SetString( "<CLASS_NAME>", pszClassFolder );
		pTargetDMX->GetCustomKeyValues()->SetFloat( "<MAX_SEQUENCE_LENGTH>", flTotalAnimTime );

		if ( IsLoopableTaunt() )
		{
			float flStartTime = Clamp( GetAnimationLoopStartTime(), 0.f, flTotalAnimTime );
			pTargetDMX->SetAnimationLoopStartTime( flStartTime );
		}
	
		pTargetDMX->SetSoundScriptFilePath( kWorkshopSoundScriptFile );

		if ( bPreview )
		{
			pTargetDMX->SetCustomModPath( "custom/workshop" );
		}
	}

	// check out files before compiling (INTERNAL ONLY)
	if ( p4 )
	{
		CUtlString sMDLAbsPath;
		pMDL->GetOutputPath( sMDLAbsPath, 0 );
		char szCorrectCaseFilePath[MAX_PATH];
		g_pFullFileSystem->GetCaseCorrectFullPath( sMDLAbsPath.String(), szCorrectCaseFilePath );
		CP4AutoEditAddFile editMDL( szCorrectCaseFilePath, "binary" );

		// check out class_workshop_animation.qc to submit the final workshop
		if ( bPerforce )
		{
			CUtlString sQCAbsPath;
			pQC->GetOutputPath( sQCAbsPath, 0 );
			g_pFullFileSystem->GetCaseCorrectFullPath( sQCAbsPath.String(), szCorrectCaseFilePath );
			CP4AutoEditAddFile editQC( szCorrectCaseFilePath );
		}

		const char *pszSoundScriptFile = GetWorkshopSoundScriptFile();
		if ( pszSoundScriptFile )
		{
			CP4AutoEditAddFile editSoundScript( pszSoundScriptFile );
		}
	}

	return BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::AddModelToAsset( CAssetTF &asset, int nClassIndex, bool bIsMulticlass, BUILD_STAGE buildStage, KeyValues *pItemData, KeyValues *pBuildMessageVariables )
{
	const uint nPathFlags = (CTargetBase::PATH_FLAG_ALL & ~CTargetBase::PATH_FLAG_ABSOLUTE) | CTargetBase::PATH_FLAG_ZIP;
	bool bPreview = buildStage == BUILD_PREVIEW;

	asset.AddModel();

	const char *pszClassFolder = kClassFolders[ nClassIndex ];

	if ( bIsMulticlass )
	{
		asset.GetTargetMDL()->SetNameSuffix( CFmtStr( "_%s", pszClassFolder ) );
	}

	CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();
	pQC->SetQCTemplate( GetQCTemplate( nClassIndex ) );

	int nLastSpecifiedTriCount = 0;
	int nLastSpecifiedLOD = 0;
	for ( int nModelIndex = 0; nModelIndex < NUM_IMPORT_LODS; ++nModelIndex )
	{
		const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassLODNFile, pszClassFolder, nModelIndex ) );
		if ( *pszFilePath )
		{
			int nLOD = asset.AddTargetDMX( pszFilePath );
			if ( nLOD < 0 )
			{
				SetMessageFileVariable( pBuildMessageVariables, pszFilePath );
				return BUILD_FAILED_BADMODEL;
			}

			CSmartPtr< CTargetDMX > pTargetDMX = asset.GetTargetDMX( nLOD );

			const int nMaxTris = GetModelTriangleBudget( nClassIndex, nModelIndex );
			const int nTriCount = pTargetDMX->GetTriangleCount();
			if ( !bPreview && nTriCount > nMaxTris )
			{
				pBuildMessageVariables->SetInt( "count", nTriCount );
				pBuildMessageVariables->SetInt( "limit", nMaxTris );
				SetMessageFileVariable( pBuildMessageVariables, pszFilePath );
				return BUILD_FAILED_COMPLEXMODEL;
			}

			// Update the item data for the path inside the asset archive
			CUtlString sOutputPath;
			pTargetDMX->GetOutputPath( sOutputPath, 0, nPathFlags );
			pItemData->SetString( CFmtStr( kClassLODNFile, pszClassFolder, nModelIndex ), sOutputPath );

			nLastSpecifiedLOD = nModelIndex;
			nLastSpecifiedTriCount = nTriCount;
		}
		else if ( !bPreview && nModelIndex == 0 )
		{
			// We need LOD0...
			pBuildMessageVariables->SetString( "class", pszClassFolder );
			return BUILD_FAILED_MISSINGMODEL;
		}
	} // for each LOD

	if ( !bPreview && nLastSpecifiedTriCount > GetModelTriangleBudget( nClassIndex, NUM_IMPORT_LODS - 1 ) )
	{
		pBuildMessageVariables->SetString( "lod", kLODLevels[ nLastSpecifiedLOD + 1 ] );
		const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassLODNFile, pszClassFolder, nLastSpecifiedLOD ) );
		SetMessageFileVariable( pBuildMessageVariables, pszFilePath );
		return BUILD_FAILED_NEEDMORELOD;
	}

	return BUILD_OKAY;
}

CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::AddMaterialsToAsset( CAssetTF &asset, KeyValues *pItemData, KeyValues *pBuildMessageVariables )
{
	const uint nPathFlags = (CTargetBase::PATH_FLAG_ALL & ~CTargetBase::PATH_FLAG_ABSOLUTE) | CTargetBase::PATH_FLAG_ZIP;
	int nValidVMTIndex = 0;
	CUtlVector< CUtlString > vecMaterialFiles;
	for ( int nVMT = 0; nVMT < asset.GetTargetVMTCount(); nVMT++ )
	{
		CTargetVMT *pVMT = asset.GetTargetVMT( nVMT );
		if ( pVMT )
		{
			if ( pVMT->GetDuplicate() )
			{
				continue;
			}

			if ( pVMT->GetMaterialType() == kInvalidMaterialType )
			{
				SetMessageFileVariable( pBuildMessageVariables, pVMT->GetMaterialId() );
				return BUILD_FAILED_BADMATERIALTYPE;
			}

			for ( int nSkinIndex = 0; nSkinIndex < CItemUpload::Manifest()->GetNumMaterialSkins(); ++nSkinIndex )
			{
				CUtlString strMaterialName = GetMaterialName( nSkinIndex, nValidVMTIndex );
				if ( strMaterialName.IsEmpty() )
				{
					continue;
				}

				KeyValuesAD pKV( "VMT" );
				CUtlBuffer sMaterialText;
				if ( !GetMaterialText( nSkinIndex, nValidVMTIndex, sMaterialText ) ||
					!pKV->LoadFromBuffer( pVMT->GetMaterialId(), sMaterialText ) )
				{
					if ( !strMaterialName.IsEmpty() )
					{
						SetMessageFileVariable( pBuildMessageVariables, strMaterialName );
					}
					else
					{
						SetMessageFileVariable( pBuildMessageVariables, pVMT->GetMaterialId() );
					}
					return BUILD_FAILED_BADMATERIAL;
				}

				BUILD_RESULT nMaterialResult = ValidateMaterialValues( pKV, nValidVMTIndex );
				if ( nMaterialResult != BUILD_OKAY )
				{
					if ( !strMaterialName.IsEmpty() )
					{
						SetMessageFileVariable( pBuildMessageVariables, strMaterialName );
					}
					else
					{
						SetMessageFileVariable( pBuildMessageVariables, pVMT->GetMaterialId() );
					}
					return nMaterialResult;
				}
				
				RemoveUnnecessaryParametersFromVMT( pKV, nValidVMTIndex );
				pVMT->SetVMTKV( pKV, nSkinIndex );

				bool bSelfIllum = pKV->GetBool( ">=DX90/$selfillum" );
				bool bBaseAlphaMask = pKV->GetBool( "$basealphaenvmapmask" );
				Assert( CItemUpload::Manifest()->GetNumTextureTypes() == NUM_MATERIAL_TEXTURE_FILE_TYPE );
				for ( int nTextureType = 0; nTextureType < CItemUpload::Manifest()->GetNumTextureTypes(); ++nTextureType )
				{
					MATERIAL_FILE_TYPE materialFileType = (MATERIAL_FILE_TYPE)nTextureType;
					const char *pszTexturePath = GetMaterialTextureFile( nSkinIndex, nValidVMTIndex, materialFileType );
					if ( V_strlen( pszTexturePath ) == 0 )
					{
						continue;
					}

					BUILD_RESULT checkImageResult = CheckImageSize( pszTexturePath, materialFileType, pBuildMessageVariables );
					if ( checkImageResult != BUILD_OKAY )
					{
						return checkImageResult;
					}

					const char *pszTextureType = CItemUpload::Manifest()->GetTextureType( materialFileType );
					if ( bBaseAlphaMask && FStrEq( pszTextureType, "_normal" ) )
					{
						Assert( materialFileType == MATERIAL_FILE_NORMAL );
						// Base alpha mask will not work in materials using a normal map, use normal map alpha mask instead
						continue;
					}
					else if ( !bSelfIllum && FStrEq( pszTextureType, "_illum" ) )
					{
						Assert( materialFileType == MATERIAL_FILE_SELFILLUM );
						// no need to output illum if self illum is not set
						continue;
					}

					if ( !pVMT->SetTargetVTF( pszTextureType, pszTexturePath, nSkinIndex ) )
					{
						SetMessageFileVariable( pBuildMessageVariables, pszTexturePath );
						return BUILD_FAILED_BADIMAGE;
					}
				}

				for ( int iVTF=0; iVTF<pVMT->GetNumTargetVTFS( nSkinIndex ); ++iVTF )
				{
					// Update the item data for the path inside the asset archive
					CUtlString sOutputPath;
					CTargetVTF *pVTF = pVMT->GetTargetVTF( iVTF, nSkinIndex );
					if ( pVTF )
					{
						pVTF->GetTargetTGA()->GetOutputPath( sOutputPath, 0, nPathFlags );
						sOutputPath = asset.CheckRedundantOutputFilePath( pVTF->GetTargetTGA()->GetInputFile().String(), "", sOutputPath.String() );
						pItemData->SetString( CFmtStr( kMaterialSkinNFile, CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex ), nValidVMTIndex, s_pszMaterialFilePrefixes[iVTF] ), sOutputPath );
					}
				}
			}

			++nValidVMTIndex;
		}
	}

	return BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::CheckSourceSDK()
{
	CUtlString sDummy;
	return CItemUpload::GetContentDir( sDummy ) && CItemUpload::GetBinDirectory( sDummy );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::BUILD_RESULT CTFFileImportDialog::Build( BUILD_STAGE buildStage, KeyValues *pBuildMessageVariables )
{
	const uint nPathFlags = (CTargetBase::PATH_FLAG_ALL & ~CTargetBase::PATH_FLAG_ABSOLUTE) | CTargetBase::PATH_FLAG_ZIP;
	bool bPreview = buildStage == BUILD_PREVIEW;

	if ( !CheckSourceSDK() )
	{
		return BUILD_FAILED_NOSDK;
	}

	// check for build errors
	if ( !bPreview )
	{
		if ( V_strcmp( GetItemName(), "" ) == 0 )
		{
			return BUILD_FAILED_NONAME;
		}

		if ( V_strcmp( GetItemPrefab(), "" ) == 0 )
		{
			return BUILD_FAILED_NOTYPE;
		}

		if ( !IsValidPrefab( GetItemPrefab() ) )
		{
			return BUILD_FAILED_BADTYPE;
		}

		if ( m_nPrefab == PREFAB_TAUNT )
		{
			if ( !AnyClassHasTauntSources() )
			{
				return BUILD_FAILED_NO_TAUNT_SOURCES;
			}
		}
		else
		{
			if ( !AnyClassHasModels() )
			{
				return BUILD_FAILED_NOMODELS;
			}

			if ( !AreClassesLODCountMatch() )
			{
				return BUILD_FAILED_LODCOUNTMISMATCH;
			}

			if ( !AreClassesMaterialCountMatch() )
			{
				return BUILD_FAILED_MATERIALCOUNTMISMATCH;
			}

			if ( !DidSpecifyAllMaterials() )
			{
				return BUILD_FAILED_NOMATERIALS;
			}
		}

		if ( V_strcmp( GetItemIcon(), "" ) == 0 )
		{
			return BUILD_FAILED_NOBACKPACKICON;
		}
	}

	bool bPartnerContent = IsPartnerContent();
	// override the manifest data for partner content
	CItemUpload::Manifest()->SetItemDirectoryOverride( bPartnerContent ? "workshop_partner/player/items/" : "" );
	CItemUpload::Manifest()->SetAnimationDirectoryOverride( bPartnerContent ? "workshop_partner/player/animations/" : "" );
	CItemUpload::Manifest()->SetIconDirectoryOverride( bPartnerContent ? "backpack/workshop_partner/player/items/" : "" );

	CAssetTF asset;
	CBuildLog buildLog;

	asset.SetItemLog( &buildLog );

	CUtlString sName = GetItemName();
	if ( bPreview )
	{
		sName = "item_preview";
	}

	// get rid of leading/trailing white spaces
	char szTempName[MAX_PATH];
	V_strcpy_safe( szTempName, sName.String() );
	Q_StripPrecedingAndTrailingWhitespace( szTempName );
	sName = szTempName;

	if ( !asset.SetName( sName ) )
	{
		return BUILD_FAILED_BADNAME;
	}
	asset.SetClass( GetClassFolder() );

	KeyValuesAD pSessionData( BuildSessionData( sName ) );
	KeyValues *pItemData = pSessionData->FindKey( GetItemValues()->GetName() );
	Assert( pItemData != NULL );

	const char *pszIcon = GetItemIcon();
	if ( V_strlen( pszIcon ) > 0 )
	{
		int nNumIconTypes = CItemUpload::Manifest()->GetNumIconTypes();
		if ( nNumIconTypes > 0 )
		{
			for ( int nIcon = 0; nIcon < nNumIconTypes; ++nIcon )
			{
				if ( !asset.SetTargetIcon( nIcon, pszIcon ) )
				{
					SetMessageFileVariable( pBuildMessageVariables, pszIcon );
					return BUILD_FAILED_BADIMAGE;
				}
			}

			// Get the filename from the last (largest) icon type
			CSmartPtr< CTargetIcon > pTargetIcon = asset.GetTargetIcon( nNumIconTypes - 1 );
			CUtlString sOutputPath;
			pTargetIcon->GetTargetVTF( 0 )->GetTargetTGA()->GetOutputPath( sOutputPath, 0, nPathFlags );
			pItemData->SetString( kItemIcon, sOutputPath );
		}
	}

	// clean up "blue" materials if needed
	if ( GetItemValues()->GetInt( kSkinType ) == 0 )
	{
		for ( int nMaterialIndex = 0; nMaterialIndex<NUM_IMPORT_MATERIALS_PER_TEAM; ++nMaterialIndex )
		{
			ClearMaterial( 1, nMaterialIndex );
		}
	}

	const char *pszSoundScriptFile = GetWorkshopSoundScriptFile();
	if ( pszSoundScriptFile )
	{
		CP4AutoRevertFile revert( pszSoundScriptFile );
	}

	// check and prepare models/taunt sources for compiling
	bool bIsMulticlass = IsMulticlass();
	asset.RemoveModels();
	BUILD_RESULT buildResult;
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( ClassHasTauntSources( nClassIndex ) )
		{
			buildResult = AddTauntToAsset( asset, nClassIndex, bIsMulticlass, buildStage, pItemData, pBuildMessageVariables );
			if ( buildResult != BUILD_OKAY )
				return buildResult;

			// should we add prop?
			if ( ClassHasModels( nClassIndex ) )
			{
				buildResult = AddModelToAsset( asset, nClassIndex, bIsMulticlass, buildStage, pItemData, pBuildMessageVariables );
				if ( buildResult != BUILD_OKAY )
					return buildResult;
			}
		}
		else if ( ClassHasModels( nClassIndex ) )
		{
			buildResult = AddModelToAsset( asset, nClassIndex, bIsMulticlass, buildStage, pItemData, pBuildMessageVariables );
			if ( buildResult != BUILD_OKAY )
				return buildResult;
		}
	} // for each class

	buildResult = AddMaterialsToAsset( asset, pItemData, pBuildMessageVariables );
	if ( buildResult != BUILD_OKAY )
		return buildResult;

	// setworkshop ID
	if ( !bPreview )
	{
		if ( p4 )
		{
			if ( pItemData->GetUint64( kWorkshopID ) == 0 )
			{
				return BUILD_FAILED_NO_WORKSHOP_ID;
			}
		}

		if ( CItemUpload::GetP4() )
		{
			KeyValues *pItemSchema = pSessionData->FindKey( CFmtStr( "%s/%s", GetItemValues()->GetName(), kItemSchema ) );
			if ( pItemSchema )
			{
				item_definition_index_t defIndex = AddKeyValuesToItemWorkshopSchema( pItemSchema );
				if ( !IsTFEnglishNameValid( defIndex ) )
				{
					return BUILD_FAILED_BADTFENGLISHNAME;
				}
			}
		}
	}

	asset.SetAdditionalManifestData( pSessionData );

	if ( m_nPrefab == PREFAB_TAUNT )
	{
		asset.SetBuildScenesImage( true );

		if ( bPreview )
		{
			asset.SetCustomModPath( "custom/workshop" );
		}
	}

	if ( bPreview )
	{
		if ( !asset.CompilePreview() )
		{
			pBuildMessageVariables->SetString( "log", buildLog.Get() );
			return BUILD_FAILED_COMPILE;
		}

		buildResult = VerifyVCD( asset );
		if ( buildResult != BUILD_OKAY )
		{
			return buildResult;
		}

		SavePreviewData( asset );
	}
	else
	{
		CUtlString sAssetName;
		char pszSessionPath[ MAX_PATH ];
		char pszArchivePath[ MAX_PATH ];

		asset.GetName( sAssetName );
		const char *pszWorkshopDir = CFmtStr( kSteamWorkshopDir, GetWorkshopFolder() );
		if ( !g_pFullFileSystem->RelativePathToFullPath_safe( pszWorkshopDir, "GAME", pszSessionPath ) )
		{
			SetMessageFileVariable( pBuildMessageVariables, pszWorkshopDir );
			return BUILD_FAILED_COMPILE;
		}
		V_ComposeFileName( pszSessionPath, CFmtStr( "%s.zip", sAssetName.Get() ), pszArchivePath, sizeof(pszArchivePath) );
		asset.SetArchivePath( pszArchivePath );

		if ( !asset.Compile() )
		{
			pBuildMessageVariables->SetString( "log", buildLog.Get() );
			return BUILD_FAILED_COMPILE;
		}

		buildResult = VerifyVCD( asset );
		if ( buildResult != BUILD_OKAY )
		{
			return buildResult;
		}

		GetItemValues()->SetString( kBuildOutput, pszArchivePath );
	}

	return BUILD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CTFFileImportDialog::BuildSessionData( const char *pszItemName )
{
	KeyValues *pData = new KeyValues( "Data" );
	KeyValues *pItemData = GetItemValues()->MakeCopy();
	pItemData->AddSubKey( BuildItemSchema( pszItemName ) );
	pData->AddSubKey( pItemData );
	return pData;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
KeyValues *CTFFileImportDialog::BuildItemSchema( const char *pszItemName )
{
	const uint nPathFlags = (CTargetBase::PATH_FLAG_ALL & ~CTargetBase::PATH_FLAG_ABSOLUTE);

	KeyValues *pData = new KeyValues( kItemSchema );
	KeyValues *pKey;
	int nClassIndex;

	pData->UsesEscapeSequences( true );

	// Get the sanitized name for item schema tokens
	CUtlString sName;
	CAssetTF asset;
	asset.SetName( pszItemName );
	asset.GetName( sName );
	asset.SetClass( GetClassFolder() );

	const char* pszTFEnglishName = GetItemValues()->GetString( kTFEnglishName );

	pData->SetString( kWorkshopName, GetItemValues()->GetString( kItemName ) );
	pData->SetString( "name", pszTFEnglishName ); // TODO: Change this when we add a ship panel to fill in name from tf_english.txt

	const char *pszForcePrefab = "no_craft";
	pData->SetString( "prefab", CFmtStr( "%s %s", pszForcePrefab, GetItemPrefab() ) );
	pData->SetString( "item_name", CFmtStr( "#TF_%s", sName.Get() ) );
	pData->SetString( "item_description", CFmtStr( "#TF_%s_Desc", sName.Get() ) );

	if ( m_nPrefab != PREFAB_TAUNT )
	{
		pData->SetString( "equip_region", GetItemValues()->GetString( kEquipRegion ) );
	}
	
	const char *kThe = "The ";
	if ( V_strncasecmp( pszTFEnglishName, kThe, V_strlen( kThe ) ) == 0 )
	{
		pData->SetBool( "propername", true );
	}

	if ( m_nPrefab != PREFAB_TAUNT )
	{
		KeyValues *pItemBodygroups = GetItemValues()->FindKey( kBodygroup );
		if ( pItemBodygroups )
		{
			CUtlVector<int> bodygroupIDs;
			for ( int i=0; i<ARRAYSIZE( kBodygroupArray ); ++i )
			{
				if ( pItemBodygroups->GetBool( kBodygroupArray[i] ) )
				{
					bodygroupIDs.AddToTail(i);
				}
			}

			if ( bodygroupIDs.Count() > 0 )
			{
				KeyValues *pSchemaVisualsKey = pData->FindKey( "visuals", true );
				if ( pSchemaVisualsKey )
				{
					KeyValues *pSchemaBodygroupsKey = pSchemaVisualsKey->FindKey( "player_bodygroups", true );
					if ( pSchemaBodygroupsKey )
					{
						for ( int i=0; i<bodygroupIDs.Count(); ++i )
						{
							int nBodygroupIndex = bodygroupIDs[i];
							pSchemaBodygroupsKey->SetBool( kBodygroupArray[nBodygroupIndex], true );
						}
					}
				}
			}
		}
	}
	else if ( IsLoopableTaunt() )
	{
		KeyValues *pAttributesKey = pData->FindKey( "attributes", true );
		if ( pAttributesKey )
		{
			KeyValues *pLoopAttrKey = pAttributesKey->FindKey( "taunt is press and hold", true );
			if ( pLoopAttrKey )
			{
				pLoopAttrKey->SetString( "attribute_class", "enable_misc2_holdtaunt" );
				pLoopAttrKey->SetInt( "value", 1 );
			}
		}
	}

	const char *pszIcon = GetItemIcon();
	if ( V_strlen( pszIcon ) > 0 )
	{
		if ( asset.SetTargetIcon( 0, pszIcon ) )
		{
			CSmartPtr< CTargetIcon > pTargetIcon = asset.GetTargetIcon( 0 );
			CUtlString sOutputPath;
			if ( pTargetIcon->GetOutputPath( sOutputPath, 0, nPathFlags ) )
			{
				const char *pszMaterialsPrefix = "materials/";
				char *pszOutputPath = sOutputPath.GetForModify();
				V_FixSlashes( pszOutputPath, '/' );
				if ( V_strncmp( pszOutputPath, pszMaterialsPrefix, V_strlen( pszMaterialsPrefix ) ) == 0 )
				{
					pszOutputPath += V_strlen( pszMaterialsPrefix );
				}
				V_StripExtension( pszOutputPath, pszOutputPath, V_strlen( pszOutputPath ) + 1 );

				pData->SetString( "image_inventory", pszOutputPath );
			}
			else
			{
				Warning( "Couldn't get output path for icon %s", pszIcon );
			}
		}
		else
		{
			Warning( "Couldn't load icon %s", pszIcon );
		}
	}

	// always add can_craft_count even if the item is not craftable in case we want to make it craftable in the future
	pKey = pData->FindKey( "capabilities", true );
	pKey->SetInt( "can_craft_count", 1 );

	if ( IsAnyVMTPaintable() )
	{
		pKey->SetBool( "paintable", true );
	}

	const CUtlVector<const char *>&vecUsabilityStrings = ItemSystem()->GetItemSchema()->GetClassUsabilityStrings();
	if ( m_nPrefab != PREFAB_TAUNT )
	{
		bool bIsMulticlass = IsMulticlass();
		asset.RemoveModels();
		for ( nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
		{
			if ( !ClassHasModels( nClassIndex ) )
				continue;

			asset.AddModel();

			if ( bIsMulticlass )
			{
				asset.GetTargetMDL()->SetNameSuffix( CFmtStr( "_%s", kClassFolders[ nClassIndex ] ) );
			}

			const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassLODNFile, kClassFolders[ nClassIndex ], 0 ) );
			if ( *pszFilePath )
			{
				int nLOD = asset.AddTargetDMX( pszFilePath );
				if ( nLOD < 0 )
				{
					Warning( "Couldn't load model %s", pszFilePath );
					continue;
				}

				// Update the item data for the path inside the asset archive
				CUtlString sOutputPath;
				if ( asset.GetTargetMDL()->GetOutputPath( sOutputPath, 0, nPathFlags ) )
				{
					sOutputPath.FixSlashes( '/' );

					if ( bIsMulticlass )
					{
						pKey = pData->FindKey( "model_player_per_class", true );
						const char* pszClassName = vecUsabilityStrings[ nClassIndex ];
						pKey->SetString( pszClassName, sOutputPath );
					}
					else
					{
						pData->SetString( "model_player", sOutputPath );
					}
				}
				else
				{
					Warning( "Couldn't get output path for model %s", pszFilePath );
				}
			}
		}
	}

	KeyValues *pUsedByClass = pData->FindKey( "used_by_classes", true );
	for ( nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( !ClassHasModels( nClassIndex ) && !ClassHasTauntSources( nClassIndex ) )
			continue;

		pUsedByClass->SetBool( vecUsabilityStrings[ nClassIndex ], true );
	}

	// add "taunt" block here
	if ( m_nPrefab == PREFAB_TAUNT )
	{
		KeyValues *pTauntKey = pData->FindKey( "taunt", true );
		KeyValues *pCustomTauntPerClass = pTauntKey->FindKey( "custom_taunt_scene_per_class", true );
		
		CSmartPtr< CTargetQC > pQC = asset.GetTargetQC();
		CSmartPtr< CTargetVCD > pVCD = pQC->GetTargetVCD();

		bool bIsMulticlass = IsMulticlass();

		for ( nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
		{
			const char *pszClass = vecUsabilityStrings[nClassIndex];
			if ( pUsedByClass->GetBool( pszClass ) )
			{
				pVCD->SetCustomRelativeDir( CFmtStr( "%s/player/%s/low", GetWorkshopFolder(), pszClass ) );
				CUtlString sTauntOutputPath;
				pVCD->GetOutputPath( sTauntOutputPath, 0, nPathFlags );
				sTauntOutputPath.FixSlashes( '/' );
				pCustomTauntPerClass->SetString( pszClass, sTauntOutputPath.String() );

				// should we add prop?
				if ( !ClassHasModels( nClassIndex ) )
					continue;

				asset.AddModel();

				if ( bIsMulticlass )
				{
					asset.GetTargetMDL()->SetNameSuffix( CFmtStr( "_%s", kClassFolders[ nClassIndex ] ) );
				}

				const char *pszFilePath = GetItemValues()->GetString( CFmtStr( kClassLODNFile, kClassFolders[ nClassIndex ], 0 ) );
				if ( *pszFilePath )
				{
					int nLOD = asset.AddTargetDMX( pszFilePath );
					if ( nLOD < 0 )
					{
						Warning( "Couldn't load model %s", pszFilePath );
						continue;
					}

					// Update the item data for the path inside the asset archive
					CUtlString sPropOutputPath;
					if ( asset.GetTargetMDL()->GetOutputPath( sPropOutputPath, 0, nPathFlags ) )
					{
						sPropOutputPath.FixSlashes( '/' );

						KeyValues *pCustomPropPerClass = pTauntKey->FindKey( "custom_taunt_prop_per_class", true );
						pCustomPropPerClass->SetString( pszClass, sPropOutputPath.String() );
					}
					else
					{
						Warning( "Couldn't get output path for model %s", pszFilePath );
					}
				}
			}
		}
	}

	return pData;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::Load( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath )
{
	const char *pszExtension = V_GetFileExtension( pszFilePath );
	if ( V_strcasecmp( pszExtension, "txt" ) == 0 )
	{
		return LoadTxt( pszFilePath, pathID, sFailedPath );
	}
	if ( V_strcasecmp( pszExtension, "zip" ) == 0 )
	{
		return LoadZip( pszFilePath, pathID, sFailedPath );
	}
	sFailedPath = pszFilePath;
	return LOAD_FAILED;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::LoadTxt( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath )
{
	char pszBasePath[MAX_PATH];
	char pszLoadPath[MAX_PATH];

	V_ExtractFilePath( pszFilePath, pszBasePath, sizeof(pszBasePath) );

	KeyValuesAD pData( "Data" );
	pData->UsesEscapeSequences( true );
	
	if ( !pData->LoadFromFile( g_pFullFileSystem, pszFilePath, pathID ) )
	{
		sFailedPath = pszFilePath;
		return LOAD_FAILED;
	}

	KeyValues *pItemData = pData->FindKey( GetItemValues()->GetName() );
	if ( !pItemData )
	{
		sFailedPath = pszFilePath;
		return LOAD_FAILED;
	}

	SetItemName( pItemData->GetString( kItemName) );
	SetItemPrefab( pItemData->GetString( kItemPrefab ) );
	SetSkinType( pItemData->GetInt( kSkinType ) );
	SetEquipRegion( pItemData->GetString( kEquipRegion ) );
	SetWorkshopID( pItemData->GetString( kWorkshopID ) );
	SetTFEnglishName( pItemData->GetString( kTFEnglishName ) );
	
	KeyValues *pBodygroup = pItemData->FindKey( kBodygroup );
	if ( pBodygroup )
	{
		SetBodygroup( pBodygroup );
	}

	const char *pszIconFile = pItemData->GetString( kItemIcon );
	if ( *pszIconFile && !V_IsAbsolutePath( pszIconFile) )
	{
		V_ComposeFileName( pszBasePath, pszIconFile, pszLoadPath, sizeof(pszLoadPath) );
		pszIconFile = pszLoadPath;
	}
	SetItemIcon( pszIconFile );

	for ( int nMaterialIndex=0; nMaterialIndex<NUM_IMPORT_MATERIALS_PER_TEAM; ++nMaterialIndex )
	{
		const char* pszPaintable = CFmtStr( kItemPaintable, nMaterialIndex );
		SetPaintable( pItemData->GetBool( pszPaintable ), nMaterialIndex );
	}

	bool bSelected = false;
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		const char* pszQCKeyName = CFmtStr( kClassQC, kClassFolders[ nClassIndex ] );
		GetItemValues()->SetString( pszQCKeyName, pItemData->GetString( pszQCKeyName ) );

		for ( int nModelIndex = 0; nModelIndex < NUM_IMPORT_LODS; ++nModelIndex )
		{
			const char *pszModelFile = pItemData->GetString( CFmtStr( kClassLODNFile, kClassFolders[ nClassIndex ], nModelIndex ) );
			if ( *pszModelFile )
			{
				if ( !V_IsAbsolutePath( pszModelFile) )
				{
					V_ComposeFileName( pszBasePath, pszModelFile, pszLoadPath, sizeof(pszLoadPath) );
					pszModelFile = pszLoadPath;
				}

				// set default selected class
				if ( !bSelected )
				{
					m_nSelectedClass = nClassIndex;

					if ( m_pClassRadioButtons[ nClassIndex ] )
					{
						m_pClassRadioButtons[ nClassIndex ]->SetSelected( true );
						bSelected = true;
					}
				}
			}

			LOAD_RESULT result = SetLOD( nClassIndex, nModelIndex, pszModelFile );
			if ( result != LOAD_OKAY )
			{
				sFailedPath = pszModelFile;
				return result;
			}
		}
	}

	const float flAnimationLoopStartTime = pItemData->GetFloat( kAnimationLoopable, -1.f );
	SetLoopableTaunt( flAnimationLoopStartTime >= 0.f, flAnimationLoopStartTime );

	// Load animation files
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		const char* pszQCIKeyName = CFmtStr( kClassQCI, kClassFolders[ nClassIndex ] );
		GetItemValues()->SetString( pszQCIKeyName, pItemData->GetString( pszQCIKeyName ) );

		const char *pszSourceFile = pItemData->GetString( CFmtStr( kClassAnimationSourceFile, kClassFolders[ nClassIndex ] ) );
		if ( *pszSourceFile )
		{
			if ( !V_IsAbsolutePath( pszSourceFile) )
			{
				V_ComposeFileName( pszBasePath, pszSourceFile, pszLoadPath, sizeof(pszLoadPath) );
				pszSourceFile = pszLoadPath;
			}

			// set default selected class
			if ( !bSelected )
			{
				m_nSelectedClass = nClassIndex;

				if ( m_pClassRadioButtons[ nClassIndex ] )
				{
					m_pClassRadioButtons[ nClassIndex ]->SetSelected( true );
					bSelected = true;
				}
			}
		}

		char szFileBase[ MAX_PATH ];
		V_FileBase( pszSourceFile, szFileBase, sizeof( szFileBase ) );

		LOAD_RESULT result = SetAnimationSource( nClassIndex, pszSourceFile );
		if ( result != LOAD_OKAY )
		{
			sFailedPath = pszSourceFile;
			return result;
		}

		const char *pszVCDFile = pItemData->GetString( CFmtStr( kClassAnimationVCDFile, kClassFolders[ nClassIndex ] ) );
		if ( *pszVCDFile )
		{
			if ( !V_IsAbsolutePath( pszVCDFile) )
			{
				V_ComposeFileName( pszBasePath, pszVCDFile, pszLoadPath, sizeof(pszLoadPath) );
				pszVCDFile = pszLoadPath;
			}

			if ( !g_pFullFileSystem->FileExists( pszVCDFile, "MOD" ) )
			{
				const char *pszTempVCDFile = pszVCDFile;

				// If the file isn't found using the path in the manifest, let's look where we think it should be on disk. 
				// We're sometimes seeing absolute paths for the creator's hard drive in the manifest for the vcd_file.
				char szFallbackTest[ MAX_PATH ];
				V_sprintf_safe( szFallbackTest, "%sgame\\scenes\\%s\\player\\%s\\low\\%s.vcd", pszBasePath, GetWorkshopFolder(), kClassFolders[ nClassIndex ], szFileBase );
				pszVCDFile = szFallbackTest;

				if ( !g_pFullFileSystem->FileExists( pszVCDFile, "MOD" ) )
				{
					// Set it back to the original value and let it fail below.
					pszVCDFile = pszTempVCDFile;
				}
			}
		}

		result = SetAnimationVCD( nClassIndex, pszVCDFile );
		if ( result != LOAD_OKAY )
		{
			sFailedPath = pszVCDFile;
			return result;
		}
	}

	Assert( CItemUpload::Manifest()->GetNumTextureTypes() == NUM_MATERIAL_TEXTURE_FILE_TYPE );
	for ( int nSkinIndex = 0; nSkinIndex < CItemUpload::Manifest()->GetNumMaterialSkins(); ++nSkinIndex )
	{
		for ( int nMaterialIndex = 0; nMaterialIndex < NUM_IMPORT_MATERIALS_PER_TEAM; ++nMaterialIndex )
		{
			const char *pszMaterialSkin = CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex );
			const char *pszMaterialVMT = pItemData->GetString( CFmtStr( kMaterialSkinNVMT, pszMaterialSkin, nMaterialIndex ) );
			const char* pszMaterialFile = pItemData->GetString( CFmtStr( kMaterialSkinNFile, pszMaterialSkin, nMaterialIndex, s_pszMaterialFilePrefixes[MATERIAL_FILE_BASETEXTURE] ) );
			if ( *pszMaterialFile && !V_IsAbsolutePath( pszMaterialFile) )
			{
				V_ComposeFileName( pszBasePath, pszMaterialFile, pszLoadPath, sizeof(pszLoadPath) );
				pszMaterialFile = pszLoadPath;
			}

			CUtlBuffer sMaterialText;
			if ( FStrEq( pszMaterialVMT, "" ) )	   	
			{
				sMaterialText.SetBufferType( true, true );
				g_pFullFileSystem->ReadFile( pszMaterialFile, NULL, sMaterialText );
				pszMaterialVMT = sMaterialText.String();
			}

			if ( !SetMaterialText( nSkinIndex, nMaterialIndex, pszMaterialVMT ) )
			{
				sFailedPath = pszMaterialFile;
				return LOAD_FAILED_BADMATERIAL;
			}

			for ( int nTextureType = 0; nTextureType < CItemUpload::Manifest()->GetNumTextureTypes(); ++nTextureType )
			{
				const char *pszMaterialTextureFile = pItemData->GetString( CFmtStr( kMaterialSkinNFile, pszMaterialSkin, nMaterialIndex, s_pszMaterialFilePrefixes[nTextureType] ) );
				if ( *pszMaterialTextureFile && !V_IsAbsolutePath( pszMaterialTextureFile) )
				{
					V_ComposeFileName( pszBasePath, pszMaterialTextureFile, pszLoadPath, sizeof(pszLoadPath) );
					pszMaterialTextureFile = pszLoadPath;
				}

				if ( !SetMaterial( nSkinIndex, nMaterialIndex, pszMaterialTextureFile, (MATERIAL_FILE_TYPE)nTextureType ) )
				{
					sFailedPath = pszMaterialTextureFile;
					return LOAD_FAILED_BADMATERIAL;
				}
			}
		}
	}

	return LOAD_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::LOAD_RESULT CTFFileImportDialog::LoadZip( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath )
{
	// Unzip the file into the temporary directory
	char pszZipName[MAX_PATH];
	V_FileBase( pszFilePath, pszZipName, sizeof(pszZipName) );

	char pszTempPath[MAX_PATH];
	V_StripExtension( pszFilePath, pszTempPath, sizeof(pszTempPath) );

	if ( ShouldP4AddOrEdit() )
	{
		char szCorrectCaseFilePath[MAX_PATH];
		g_pFullFileSystem->GetCaseCorrectFullPath( pszFilePath, szCorrectCaseFilePath );
		CP4AutoEditFile a( szCorrectCaseFilePath );
	}

	if ( !g_pFullFileSystem->UnzipFile( pszFilePath, pathID, pszTempPath ) )
	{
		sFailedPath = pszFilePath;
		return LOAD_FAILED;
	}

	char pszManifest[MAX_PATH];
	V_ComposeFileName( pszTempPath, "manifest.txt", pszManifest, sizeof(pszManifest) );

	return LoadTxt( pszManifest, pathID, sFailedPath );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFileImportDialog::SAVE_RESULT CTFFileImportDialog::Save( const char *pszFilePath, const char *pathID )
{
	KeyValuesAD pData( BuildSessionData( GetItemName() ) );

	CUtlBuffer sNewText;
	sNewText.SetBufferType( true, true );
	pData->RecursiveSaveToFile( sNewText, 0, false, true );

	if ( !pData->SaveToFile( g_pFullFileSystem, pszFilePath, pathID ) )
	{
		return SAVE_FAILED;
	}
	return SAVE_OKAY;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::SavePreviewData( CAssetTF &asset )
{
	KeyValues *pSessionData = asset.GetAdditionalManifestData();

	if ( m_pPreviewSchema )
	{
		m_pPreviewSchema->deleteThis();
		m_pPreviewSchema = NULL;
	}

	KeyValues *pItemSchema = pSessionData->FindKey( CFmtStr( "%s/%s", GetItemValues()->GetName(), kItemSchema ) );
	if ( pItemSchema )
	{
		m_pPreviewSchema = pItemSchema->MakeCopy();
	}
	Assert(m_pPreviewSchema);

	// Save the files that were built so we can remove them later
	m_vecPreviewFiles = asset.GetBuiltFiles();
	m_vecCustomModFiles = asset.GetModFiles();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::SetupPreviewData()
{
	KeyValuesAD pPreviewSchema( m_pPreviewSchema->MakeCopy() );
	pPreviewSchema->SetString( "item_name", pPreviewSchema->GetString( "name" ) );

	if ( !ItemSystem()->GetItemSchema()->SetupPreviewItemDefinition( pPreviewSchema ) )
	{
		return false;
	}

	// must call this to flush old model and force load new model with the same name
	engine->ExecuteClientCmd( "r_flushlod" );
	engine->ExecuteClientCmd( "mat_reloadallmaterials" );
	engine->ExecuteClientCmd( "cl_soundemitter_flush" );
	
	if ( !m_pPreviewDialog )
	{
		m_pPreviewDialog = new CImportPreviewItemPanel( this, GetItemValues(), m_nSelectedClass );
		m_pPreviewDialog->MakePopup( false );
	}

	CEconItemView itemData;
	itemData.Init( PREVIEW_ITEM_DEFINITION_INDEX, AE_UNIQUE, AE_USE_SCRIPT_VALUE, true );
	itemData.SetClientItemFlags( kEconItemFlagClient_Preview );
	m_pPreviewDialog->PreviewItem( 0, &itemData );

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::CleanupPreviewData()
{
	m_pPreviewDialog.Set( INVALID_PANEL );

	FOR_EACH_VEC( m_vecPreviewFiles, i )
	{
		const char *pszFileName = m_vecPreviewFiles[ i ].String();
		if ( p4 && V_strstr( pszFileName, "_user_animations.mdl" ) )
		{
			char szCorrectCaseFilePath[MAX_PATH];
			g_pFullFileSystem->GetCaseCorrectFullPath( pszFileName, szCorrectCaseFilePath );
			CP4AutoRevertFile revert( szCorrectCaseFilePath );
		}
		else
		{
			g_pFullFileSystem->RemoveFile( pszFileName );
		}
	}
	m_vecPreviewFiles.RemoveAll();

	FOR_EACH_VEC( m_vecCustomModFiles, i )
	{
		g_pFullFileSystem->RemoveFile( m_vecCustomModFiles[ i ] );
	}
	m_vecCustomModFiles.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::IsMulticlass()
{
	int nClasses = 0;
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( ClassHasModels( nClassIndex ) || ClassHasTauntSources( nClassIndex ) )
			++nClasses;
	}
	return (nClasses > 1);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFFileImportDialog::GetClassFolder()
{
	const char *pszClassName = NULL;
	
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( ClassHasModels( nClassIndex ) || ClassHasTauntSources( nClassIndex ) )
		{
			if ( pszClassName )
			{
				pszClassName = kClassFolderMulticlass;
			}
			else
			{
				pszClassName = kClassFolders[ nClassIndex ];
			}
		}
	}
	
	return pszClassName;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::AnyClassHasModels()
{
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( ClassHasModels( nClassIndex ) )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::ClassHasModels( int nClassIndex )
{
	for ( int nModelIndex = 0; nModelIndex < NUM_IMPORT_LODS; ++nModelIndex )
	{
		if ( V_stricmp( GetItemValues()->GetString( CFmtStr( kClassLODNFile, kClassFolders[ nClassIndex ], nModelIndex ) ), "" ) != 0 )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::DidSpecifyAllMaterials()
{
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ nClassIndex ], 0 ) );
		if ( pKey )
		{
			int nSkinType = GetItemValues()->GetInt( kSkinType );
			int nMaterialCount = pKey->GetInt( "materialCount" );
			for ( int nSkinIndex=0; nSkinIndex<=nSkinType; ++nSkinIndex )
			{
				for ( int nMaterialIndex=0; nMaterialIndex<nMaterialCount; ++nMaterialIndex )
				{
					const char *pszMaterialFile = GetItemValues()->GetString( CFmtStr( kMaterialSkinNFile, CItemUpload::Manifest()->GetMaterialSkin( nSkinIndex ), nMaterialIndex, s_pszMaterialFilePrefixes[MATERIAL_FILE_BASETEXTURE] ) );
					if ( FStrEq( pszMaterialFile, "" ) )
					{
						return false;
					}
				}
			}

			// only need to check materials for one class because we already check AreClassesMaterialCountMatch
			break;
		}
	} // for each LOD

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::AreClassesLODCountMatch()
{
	int nCurrentLODCount = 0;
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		int nNumClassLOD = 0;
		for ( int nLOD=0; nLOD<NUM_IMPORT_LODS; ++nLOD )
		{
			KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ nClassIndex ], nLOD ) );
			if ( pKey )
			{
				if ( !FStrEq( pKey->GetString( "file" ), "" ) )
				{
					nNumClassLOD++;
				}
			}
		} // for each LOD

		if ( nCurrentLODCount == 0 )
		{
			nCurrentLODCount = nNumClassLOD;
		}
		else if ( nNumClassLOD != 0 && nCurrentLODCount != nNumClassLOD )
		{
			return false;
		}
	} // for each class

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::AreClassesMaterialCountMatch()
{
	for ( int nLOD=0; nLOD<NUM_IMPORT_LODS; ++nLOD )
	{
		int nCurrentMaterialCount = -1;
		for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
		{		
			KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassLODN, kClassFolders[ nClassIndex ], nLOD ) );
			if ( pKey )
			{
				if ( !FStrEq( pKey->GetString( "file" ), "" ) )
				{
					int nMaterialCount = pKey->GetInt( "materialCount" );
					if ( nCurrentMaterialCount == -1 )
					{
						nCurrentMaterialCount = nMaterialCount;
					}
					else if ( nCurrentMaterialCount != nMaterialCount )
					{
						return false;
					}
				}
			}
		} // for each class
	} // for each LOD

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::AnyClassHasTauntSources()
{
	for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; ++nClassIndex )
	{
		if ( ClassHasTauntSources( nClassIndex ) )
			return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFileImportDialog::ClassHasTauntSources( int nClassIndex )
{
	if ( nClassIndex == TF_CLASS_UNDEFINED )
		return false;

	KeyValues *pKey = GetItemValues()->FindKey( CFmtStr( kClassAnimation, kClassFolders[ nClassIndex ] ) );
	if ( pKey )
	{
		const char *pszSource = pKey->GetString( "source_file" );
		if ( pszSource && *pszSource )
		{
			return true;
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFileImportDialog::GetWorkshopData()
{
	CFilePublishDialog *pPublishDialog = dynamic_cast< CFilePublishDialog * >( GetParent() );
	if ( !pPublishDialog )
		return;

	vgui::TextEntry *pTextEntry = pPublishDialog->FindControl<vgui::TextEntry>( "FileTitle" );
	if ( pTextEntry )
	{
		char pszName[1024];
		pTextEntry->GetText( pszName, sizeof(pszName) );
		SetItemName( pszName );
	}
}

void CTFFileImportDialog::SetWorkshopData()
{
	CFilePublishDialog *pPublishDialog = dynamic_cast< CFilePublishDialog * >( GetParent() );
	if ( !pPublishDialog )
		return;

	pPublishDialog->SetFile( GetItemValues()->GetString( kBuildOutput ), true );

	vgui::TextEntry *pTextEntry = pPublishDialog->FindControl<vgui::TextEntry>( "FileTitle" );
	if ( pTextEntry )
	{
		pTextEntry->SetText( GetItemValues()->GetString( kItemName ) );
	}

	vgui::EditablePanel* pClassUsagePanel = pPublishDialog->FindControl<vgui::EditablePanel>( "ClassUsagePanel" );
	if ( pClassUsagePanel )
	{
		for ( int nClassIndex = TF_FIRST_NORMAL_CLASS; nClassIndex < TF_LAST_NORMAL_CLASS; nClassIndex++ )
		{
			SetChildButtonSelected( pClassUsagePanel, VarArgs( "ClassCheckBox%d", nClassIndex ), ClassHasModels( nClassIndex ) );
		}
	}
}


void CTFFileImportDialog::SetLODPanelEnable( bool bEnable, int nModelIndex )
{
	vgui::Panel *pLODPanel = m_pLODPanels[nModelIndex];
	if ( pLODPanel )
	{
		for ( int i=0; i<pLODPanel->GetChildCount(); ++i )
		{
			pLODPanel->GetChild(i)->SetEnabled( bEnable );
		}
	}
}


void CTFFileImportDialog::SetSkinType( int nSkinType )
{
	if ( m_pSkinComboBox && nSkinType < m_pSkinComboBox->GetItemCount() )
	{
		m_pSkinComboBox->ActivateItemByRow( nSkinType );
	}
	GetItemValues()->SetInt( kSkinType, nSkinType );

	UpdateMaterialDisplay();

	SetDirty( true );
}

void CTFFileImportDialog::SetEquipRegion( const char* pszEquipRegion )
{
	if ( m_pEquipRegionComboBox )
	{
		bool bFound = false;
		for ( int i = 0; i < m_pEquipRegionComboBox->GetItemCount(); ++i )
		{
			KeyValues *pKeyValues = m_pEquipRegionComboBox->GetItemUserData( m_pEquipRegionComboBox->GetItemIDFromRow( i ) );
			if ( V_strcasecmp( pszEquipRegion, pKeyValues->GetString( kEquipRegion ) ) == 0 )
			{
				bFound = true;
				m_pEquipRegionComboBox->ActivateItemByRow( i );
				break;
			}
		}
		
		GetItemValues()->SetString( kEquipRegion, bFound ? pszEquipRegion : "" );
	}

	SetDirty( true );
}

void CTFFileImportDialog::SetWorkshopID( const char* pszWorkshopID )
{
	if ( m_pWorkshopIDTextEntry->IsVisible() )
	{
		GetItemValues()->SetString( kWorkshopID, pszWorkshopID );
		m_pWorkshopIDTextEntry->SetText( GetItemValues()->GetString( kWorkshopID ) );
	}
	SetDirty( true );
}

bool CTFFileImportDialog::IsTFEnglishNameValid( item_definition_index_t defIndex )
{
	// check if the name already exist in item schema
	if ( m_pTFEnglishNameTextEntry->IsVisible() )
	{
		if ( defIndex == INVALID_ITEM_DEF_INDEX )
		{
			return false;
		}

		const char* pszTFEnglishName = GetItemValues()->GetString( kTFEnglishName );
		const CEconItemSchema::ItemDefinitionMap_t& mapItemDefs = ItemSystem()->GetItemSchema()->GetItemDefinitionMap();
		FOR_EACH_MAP_FAST( mapItemDefs, i )
		{
			CEconItemDefinition *pData = mapItemDefs[i];
			const char *pszExistingItemName = pData->GetDefinitionName();
			item_definition_index_t existingItemDefIndex = pData->GetDefinitionIndex();
			if ( existingItemDefIndex != 0 && existingItemDefIndex != defIndex && FStrEq( pszExistingItemName, pszTFEnglishName ) )
			{
				return false;
			}
		}
	}

	return true;
}

void CTFFileImportDialog::SetTFEnglishName( const char* pszTFEnglishName )
{
	if ( m_pTFEnglishNameTextEntry->IsVisible() )
	{
		GetItemValues()->SetString( kTFEnglishName, pszTFEnglishName );
		m_pTFEnglishNameTextEntry->SetText( GetItemValues()->GetString( kTFEnglishName ) );
	}
	SetDirty( true );
}

//-----------------------------------------------------------------------------
item_definition_index_t CTFFileImportDialog::AddKeyValuesToItemWorkshopSchema( KeyValues *pKV )
{
	char szItemWorkshopAbsPath[MAX_PATH];
	if ( !GenerateFullPath( kWorkshopSchemaFile, "MOD", szItemWorkshopAbsPath, ARRAYSIZE( szItemWorkshopAbsPath ) ) )
	{
		Warning( "Failed to GenerateFullPath %s\n", kWorkshopSchemaFile );
		return INVALID_ITEM_DEF_INDEX;
	}
	char szCorrectCaseFilePath[MAX_PATH];
	g_pFullFileSystem->GetCaseCorrectFullPath( szItemWorkshopAbsPath, szCorrectCaseFilePath );
	CP4AutoEditAddFile a( szCorrectCaseFilePath );

	KeyValuesAD pWorkshopSchema( "WorkshopSchema" );

	// This file should force a cache refresh of this specific file otherwise we won't write stuff out.
	if ( !pWorkshopSchema->LoadFromFile( g_pFullFileSystem, kWorkshopSchemaFile, "MOD", true ) )
	{
		Warning( "Failed to load %s\n", kWorkshopSchemaFile );
		return INVALID_ITEM_DEF_INDEX;
	}

	int nNewItemIndex = kStartWorkshopItemIndex;
	const char* pszNewItemName = pKV->GetString( kWorkshopName );
	KeyValues *pNewItemKey = NULL;
	KeyValues *pLastKey = NULL;

	KeyValues *pItemKey = pWorkshopSchema;
	while ( pItemKey )
	{
		const char* pszItemName = pItemKey->GetString( kWorkshopName );
		nNewItemIndex = atoi( pItemKey->GetName() );
		if ( FStrEq( pszItemName, pszNewItemName ) )
		{
			pNewItemKey = pItemKey;
			break;
		}

		pLastKey = pItemKey;
		pItemKey = pItemKey->GetNextKey();
	}

	if ( !pNewItemKey )
	{
		pNewItemKey = pKV->MakeCopy();
		pNewItemKey->SetName( CFmtStr( "%d", nNewItemIndex + 1 ) );
		pLastKey->SetNextKey( pNewItemKey );
	}
	else
	{
		KeyValuesAD temp( pNewItemKey->MakeCopy() );
		pKV->CopySubkeys( pNewItemKey );
		pNewItemKey->RecursiveMergeKeyValues( temp );
	}

	KeyValues* pPaymentKey = pNewItemKey->FindKey( "payment_rules/0", true );
	pPaymentKey->SetFloat( "workshop_revenue_share", 0.25f );
	pPaymentKey->SetString( "target", GetItemValues()->GetString( kWorkshopID ) );
	pPaymentKey->SetString( "payment_rule_for_itemdef", pNewItemKey->GetName() );

	pItemKey = pWorkshopSchema;
	CUtlBuffer buffer;
	while ( pItemKey )
	{
		pItemKey->RecursiveSaveToFile( buffer, 2, false, true );
		pItemKey = pItemKey->GetNextKey();
	}

	if ( !g_pFullFileSystem->WriteFile( szItemWorkshopAbsPath, NULL, buffer ) )
	{
		Warning( "Failed to save new item to %s", szItemWorkshopAbsPath );
	}

	item_definition_index_t defIndex = atoi( pNewItemKey->GetName() );
	return defIndex;
}
