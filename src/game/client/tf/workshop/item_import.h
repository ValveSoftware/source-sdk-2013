//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "vgui_controls/frame.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_bitmappanel.h"

#include "tf_shareddefs.h"

class CTFFileImportTextEditDialog;
class CTFImportMaterialEditDialog;
class CImportPreviewItemPanel;
class CTFPlayerModelPanel;

enum MATERIAL_FILE_TYPE
{
	MATERIAL_FILE_BASETEXTURE,
	MATERIAL_FILE_NORMAL,
	MATERIAL_FILE_PHONGEXPONENT,
	MATERIAL_FILE_SELFILLUM,
	NUM_MATERIAL_TEXTURE_FILE_TYPE
};

//-----------------------------------------------------------------------------
// Purpose: Import file dialog
//-----------------------------------------------------------------------------
class CTFFileImportDialog : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE( CTFFileImportDialog, Frame );

public:
	enum BUILD_RESULT {
		BUILD_OKAY,
		BUILD_FAILED_NOSDK,
		BUILD_FAILED_NONAME,
		BUILD_FAILED_NOTYPE,
		BUILD_FAILED_NOMODELS,
		BUILD_FAILED_LODCOUNTMISMATCH,
		BUILD_FAILED_NOMATERIALS,
		BUILD_FAILED_MATERIALCOUNTMISMATCH,
		BUILD_FAILED_NOBACKPACKICON,
		BUILD_FAILED_BADNAME,
		BUILD_FAILED_BADTFENGLISHNAME,
		BUILD_FAILED_BADTYPE,
		BUILD_FAILED_BADMODEL,
		BUILD_FAILED_BADMATERIALTYPE,
		BUILD_FAILED_BADMATERIAL,
		BUILD_FAILED_MATERIALMISSINGSHADER,
		BUILD_FAILED_MATERIALMISSINGCLOAK,
		BUILD_FAILED_MATERIALMISSINGBURNING,
		BUILD_FAILED_MATERIALMISSINGJARATE,
		BUILD_FAILED_MATERIALMISSINGPAINTABLE,
		BUILD_FAILED_MISSINGMODEL,
		BUILD_FAILED_NEEDMORELOD,
		BUILD_FAILED_COMPLEXMODEL,
		BUILD_FAILED_BADIMAGE,
		BUILD_FAILED_COMPILE,
		BUILD_FAILED_NO_WORKSHOP_ID,
		BUILD_FAILED_IMAGEUNSUPPORTEDFILETYPE,
		BUILD_FAILED_IMAGERESOLUTIONNOTPOWEROF2,
		BUILD_FAILED_IMAGERESOLUTIONOVERLIMIT,
		BUILD_FAILED_NO_TAUNT_SOURCES,
		BUILD_FAILED_BAD_VCD_FILE,
		BUILD_FAILED_VCD_MISSING_EVENT_SEQUENCE,
		BUILD_FAILED_VCD_EVENT_SEQUENCE_TOO_LONG,

		NUM_BUILD_RESULTS
	};
	enum LOAD_RESULT {
		LOAD_OKAY,
		LOAD_FAILED,
		LOAD_FAILED_BADMODEL,
		LOAD_FAILED_COMPLEXMODEL,
		LOAD_FAILED_TOOMANYBONES,
		LOAD_FAILED_BADMATERIAL,
		LOAD_FAILED_TOOMANYMATERIALS,
		LOAD_FAILED_MATERIALCOUNTMISMATCH,
		LOAD_FAIL_ANIMATIONTOOLONG,
		NUM_LOAD_RESULTS
	};
	enum SAVE_RESULT {
		SAVE_OKAY,
		SAVE_FAILED,
		NUM_SAVE_RESULTS
	};

	enum BUILD_STAGE
	{
		BUILD_PREVIEW,
		BUILD_VERIFY,
		BUILD_FINAL
	};

	enum WARNING
	{
		WARNING_BASEALPHAMASK,
		NUM_WARNINGS
	};

	enum ImportPrefab_t
	{
		PREFAB_HAT = 0,
		PREFAB_MISC,
		PREFAB_TAUNT,

		PREFAB_COUNT
	};

public:
	CTFFileImportDialog( vgui::Panel *parent );

	virtual ~CTFFileImportDialog();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnCommand( const char *command );
			void OnCommandLoad();
			void OnCommandSave();
			void OnCommandBrowseIcon();
			void OnCommandBrowseLOD( int index );
			void OnCommandBrowseAnimationSource();
			void OnCommandBrowseAnimationVCD();
			void OnCommandSwapVMT();
			void OnCommandEditMaterial( int nSkinIndex, int nMaterialIndex );
			void OnCommandEditMaterialDone( int nSkinIndex, int nMaterialIndex );
			void OnCommandEditQC();
			void OnCommandEditQCI();
			void OnCommandEditQCDone();
			void OnCommandEditQCIDone();
			bool OnCommandBuild( BUILD_STAGE buildStage );
			void OnCommandUpdateBodygroup();

			void OnOpen();

	virtual void OnClose();

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );

	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

	MESSAGE_FUNC_PTR( OnRadioButtonChecked, "RadioButtonChecked", panel );

	KeyValues *GetItemValues() { return m_pItemValues; }

	void SetItemName( const char *pszName );
	const char *GetItemName();
	bool IsValidPrefab( const char *pszPrefab );
	void SetItemPrefab( const char *pszPrefab );
	const char *GetItemPrefab();
	bool GetItemPrefabValue( const char *pszPrefab, const char *pszName, CUtlString& strOutput );
	void SelectClass( int nClassIndex );
	void SetItemIcon( const char *pszFilePath );
	const char *GetItemIcon();
	void SetPaintable( bool bPaintable, int nMaterialIndex );
	bool IsPaintable( int nMaterialIndex );
	bool IsAnyVMTPaintable();
	const char *GetUserAnimationQCTemplate( int nSelectedClass, bool bPerforce = false );
	const char *GetQCTemplate( int nSelectedClass );
	const char *GetQCITemplate( int nSelectedClass );
	void ClearLODs();
	LOAD_RESULT SetLOD( int selectedClass, int nModelIndex, const char *pszFilePath, KeyValues* pKV = NULL );
	void UpdateLODDisplay();
	int GetModelTriangleBudget( int selectedClass, int nModelIndex );
	int GetModelBoneBudget();
	bool SetMaterial( int nMaterialPanelIndex, const char* pszFilePath, MATERIAL_FILE_TYPE fileType );
	bool SetMaterial( int selectedSkin, int nMaterialIndex, const char *pszFilePath, MATERIAL_FILE_TYPE fileType );
	const char* GetMaterialTextureFile( int selectedSkin, int nMaterialIndex, MATERIAL_FILE_TYPE fileType );
	CUtlString GetMaterialName( int selectedSkin, int nMaterialIndex );
	void ClearMaterials();
	void ClearMaterial( int nSkinIndex, int nMaterialIndex );
	void UpdateMaterialDisplay();
	void UpdateMaterialDisplay( int nSkinIndex, int nMaterialIndex );
	const char *GetMaterialText( int nSkinIndex, int nMaterialIndex, CUtlBuffer &sMaterialText );
	bool SetMaterialText( int nSkinIndex, int nMaterialIndex, const char* pszMaterialText );
	BUILD_RESULT ValidateMaterialValues( KeyValues *pKV, int nMaterialIndex );
	void RemoveUnnecessaryParametersFromVMT( KeyValues *pKV, int nMaterialIndex );
	void RemoveLightParameters( KeyValues *pKV, int nMaterialIndex );
	void RemovePaintParameters( KeyValues *pKV, int nMaterialIndex );
	void RemoveTranslucentParameters( KeyValues *pKV );
	void RemoveCubeMapParameters( KeyValues *pKV );
	void RemoveSelfIllumParameters( KeyValues *pKV );

	void UpdateBodygroupsDisplay();
	void SetBodygroup( KeyValues* pBodygroupKey );

	LOAD_RESULT SetAnimationSource( int selectedClass, const char *pszFilePath, KeyValues* pKV = NULL );
	LOAD_RESULT SetAnimationVCD( int selectedClass, const char *pszFilePath, KeyValues* pKV = NULL );
	void SetAnimationDuration( int selectedClass, float flDuration );
	BUILD_RESULT VerifyVCD( const CAssetTF &asset );
	void UpdateAnimationSourceDisplay();
	void UpdateAnimationVCDDisplay();
	void UpdateAnimDurationDisplay();

	void SetDirty( bool bDirty );

	void SetLoopableTaunt( bool bLoopable, float flLoopStartTime );
	bool IsLoopableTaunt() const;
	float GetAnimationLoopStartTime() const;

	BUILD_RESULT AddTauntToAsset( CAssetTF &asset, int nClassIndex, bool bIsMulticlass, BUILD_STAGE buildStage, KeyValues *pItemData, KeyValues *pBuildMessageVariables );
	BUILD_RESULT AddModelToAsset( CAssetTF &asset, int nClassIndex, bool bIsMulticlass, BUILD_STAGE buildStage, KeyValues *pItemData, KeyValues *pBuildMessageVariables );
	BUILD_RESULT AddMaterialsToAsset( CAssetTF &asset, KeyValues *pItemData, KeyValues *pBuildMessageVariables );
	bool CheckSourceSDK();
	BUILD_RESULT Build( BUILD_STAGE buildStage, KeyValues *pBuildMessageVariables );
	KeyValues *BuildSessionData( const char *pszItemName );
	KeyValues *BuildItemSchema( const char *pszItemName );
	LOAD_RESULT Load( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath );
	LOAD_RESULT LoadTxt( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath );
	LOAD_RESULT LoadZip( const char *pszFilePath, const char *pathID, CUtlString &sFailedPath );
	SAVE_RESULT Save( const char *pszFilePath, const char *pathID );

	void SavePreviewData( CAssetTF &asset );
	bool SetupPreviewData();
	void CleanupPreviewData();

protected:
	bool IsMulticlass();
	const char *GetClassFolder();
	bool AnyClassHasModels();
	bool ClassHasModels( int nClassIndex );
	bool DidSpecifyAllMaterials();
	bool AreClassesLODCountMatch();
	bool AreClassesMaterialCountMatch();
	bool AnyClassHasTauntSources();
	bool ClassHasTauntSources( int nClassIndex );

	void GetWorkshopData();
	void SetWorkshopData();

private:
	bool ShouldP4AddOrEdit() const;
	bool IsPartnerContent() const;
	const char* GetWorkshopFolder() const;

	int GetCustomBones( int selectedClass, const char* pszFileName, CUtlStringList& strBoneList );
	void SetLODPanelEnable( bool bEnable, int nModelIndex );
	void SetSkinType( int nSkinType );
	void SetEquipRegion( const char* pszEquipRegion );
	void SetWorkshopID( const char* pszWorkshopID );
	bool IsTFEnglishNameValid( item_definition_index_t defIndex );
	void SetTFEnglishName( const char* pszTFEnglishName );
	void UpdateUIForPrefab( ImportPrefab_t nPrefab );
	item_definition_index_t AddKeyValuesToItemWorkshopSchema( KeyValues *pKV );

	int m_nSelectedClass;
	ImportPrefab_t m_nPrefab;

	enum FileOpenMode {
		FILE_OPEN_NONE,
		FILE_OPEN_LOAD,
		FILE_OPEN_SAVE,
		FILE_OPEN_ICON,
		FILE_OPEN_LOD0,
		FILE_OPEN_LOD1,
		FILE_OPEN_LOD2,
		FILE_OPEN_ANIMATION_SOURCE,
		FILE_OPEN_ANIMATION_VCD,
	} m_nFileOpenMode;

	KeyValues *m_pItemValues;
	KeyValues *m_pPreviewSchema;
	CUtlVector< CUtlString > m_vecPreviewFiles;
	CUtlVector< CUtlString > m_vecCustomModFiles;
	CUtlBuffer m_tempQC;

	vgui::TextEntry *m_pNameTextEntry;
	vgui::ComboBox	*m_pTypeComboBox;
	vgui::Button	*m_pSwapVMTButton;
	vgui::ComboBox	*m_pSkinComboBox;
	vgui::TextEntry *m_pWorkshopIDTextEntry;
	vgui::TextEntry *m_pTFEnglishNameTextEntry;
	vgui::CheckButton *m_pPerforceCheckButton;
	vgui::CheckButton *m_pPartnerCheckButton;
	
	vgui::EditablePanel	*m_pEquipRegionPanel;
	vgui::ComboBox	*m_pEquipRegionComboBox;

	vgui::ImagePanel *m_pIconImagePanel;
	CUtlVector< vgui::CheckButton* > m_pPaintableCheckButtons;

	vgui::RadioButton *m_pClassRadioButtons[TF_LAST_NORMAL_CLASS];
	vgui::Panel *m_pClassHighlights[TF_LAST_NORMAL_CLASS];

	vgui::EditablePanel	*m_pBodygroupsPanel;
	CUtlVector< vgui::CheckButton* > m_pBodygroups;

	vgui::EditablePanel	*m_pLODsPanel;
	CUtlVector< vgui::Panel* > m_pLODPanels;
	CUtlVector< vgui::Label* > m_pLODFiles;
	CUtlVector< vgui::Label* > m_pLODDetails;

	vgui::EditablePanel *m_pSkinsPanel;
	CUtlVector< vgui::Panel* > m_pMaterialPanels;
	CUtlVector< vgui::Label* > m_pMaterialLabels;
	CUtlVector< vgui::Label* > m_pMaterialFiles;

	vgui::EditablePanel	*m_pTauntInputPanel;
	vgui::Label *m_pAnimationSourceFile;
	vgui::Label *m_pAnimationVCDFile;
	vgui::Label *m_pAnimationDurationLabel;
	vgui::Label *m_pAnimationPropLabel;
	vgui::CheckButton *m_pAnimationLoopCheckButton;
	vgui::TextEntry *m_pAnimationLoopStartTextEntry;

	vgui::Button *m_pBuildButton;
	vgui::DHANDLE<CTFFileImportTextEditDialog> m_pTextEditDialog;
	vgui::DHANDLE<CTFImportMaterialEditDialog> m_pMaterialEditDialog;
	vgui::DHANDLE<CImportPreviewItemPanel> m_pPreviewDialog;
	CTFPlayerModelPanel	*m_pPlayerModelPanel;

	bool m_bWasCheatOn;
};
