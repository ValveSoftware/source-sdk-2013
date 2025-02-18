//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef DYNAMIC_RECIPE_SUBPANEL_H
#define DYNAMIC_RECIPE_SUBPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "backpack_panel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "tf_gcmessages.h"
#include "econ_gcmessages.h"
#include "tf_imagepanel.h"
#include "tf_controls.h"
#include "item_selection_panel.h"
#include "econ_dynamic_recipe.h"

class CImageButton;

#define DYNAMIC_RECIPE_INPUT_ROWS 4
#define DYNAMIC_RECIPE_INPUT_COLS 3
#define DYNAMIC_RECIPE_INPUT_COUNT ( DYNAMIC_RECIPE_INPUT_ROWS * DYNAMIC_RECIPE_INPUT_COLS )
#define DYNAMIC_RECIPE_OUTPUT_ROWS 4
#define DYNAMIC_RECIPE_OUTPUT_COLS 3
#define DYNAMIC_RECIPE_OUTPUT_COUNT ( DYNAMIC_RECIPE_OUTPUT_ROWS * DYNAMIC_RECIPE_OUTPUT_COLS )

#define DYNAMIC_RECIPE_BACKPACK_ROWS 4
#define DYNAMIC_RECIPE_BACKPACK_COLS 4
#define DYNAMIC_RECIPE_PACKPACK_COUNT_PER_PAGE ( DYNAMIC_RECIPE_BACKPACK_ROWS * DYNAMIC_RECIPE_BACKPACK_COLS )

class CRecipeComponentItemModelPanel;

class CRecipeComponentItemModelPanel : public CItemModelPanel
{	
public:
	DECLARE_CLASS_SIMPLE( CRecipeComponentItemModelPanel, CItemModelPanel );
	CRecipeComponentItemModelPanel( vgui::Panel *parent, const char *name );

	void AddRecipe( itemid_t nRecipe );
	virtual void DeleteRecipes();
	virtual void SetItem( const CEconItemView *pItem ) OVERRIDE;
	void SetRecipeItem( itemid_t nRecipeItem, int nPageNumber );
	void AddDefaultItem( CEconItemView *pItem );
	CEconItemView* GetRecipeItem( int nPageNumber ) const;
	itemid_t GetRecipeIndex( int nPageNumber ) const;
	bool IsSlotAvailable( int nPageNumber );
	CEconItemView* GetDefaultItem() const { return m_nPageNumber < m_vecDefaultItems.Count() ? m_vecDefaultItems[ m_nPageNumber ] : NULL; }
	void UpdateDisplayItem();

	void SetPageNumber( int nPageNumber );
	int GetPageNumber() const { return m_nPageNumber; }
protected:
	struct RecipeItem_t
	{
		itemid_t m_nRecipeIndex;
		CEconItemView* m_pRecipeItem;
	};

	void UpdateRecipeItem( RecipeItem_t* pRecipeItem );
	virtual void SetBlankState();

	CUtlVector< CEconItemView* > m_vecDefaultItems;
	CUtlVector< RecipeItem_t > m_vecRecipes;
	int m_nPageNumber;
};

class CInputPanelItemModelPanel : public CRecipeComponentItemModelPanel
{
public:
	CInputPanelItemModelPanel( vgui::Panel *parent, const char *name, const CEconItemView* pDynamicRecipeItem )
		: CRecipeComponentItemModelPanel( parent, name )
		, m_pDynamicRecipeItem( pDynamicRecipeItem )
	{}

	virtual void DeleteRecipes();
	void AddComponentInfo( const CEconItemAttributeDefinition *pComponentAttrib );
	bool MatchesAttribCriteria( itemid_t itemID ) const;
	bool MatchesAttribCriteria( itemid_t itemID, int nPageNumber ) const;
	const CEconItemAttributeDefinition * GetAttrib( int nPageNumber ) const;
	void SetDynamicRecipeItem( const CEconItemView* pDynamicRecipeItem ) { m_pDynamicRecipeItem = pDynamicRecipeItem; }

protected:
	virtual void SetBlankState() OVERRIDE;

private:
	CUtlVector< const CEconItemAttributeDefinition* > m_vecAttrDef;
	const CEconItemView* m_pDynamicRecipeItem;
};


//-----------------------------------------------------------------------------
// An inventory screen that handles displaying the crafting screen
//-----------------------------------------------------------------------------
class CDynamicRecipePanel : public CBackpackPanel
{
	DECLARE_CLASS_SIMPLE( CDynamicRecipePanel, CBackpackPanel );
public:


	CDynamicRecipePanel( vgui::Panel *parent, const char *panelName, CEconItemView* pRecipeItem );
	~CDynamicRecipePanel( void );

	void SetNewRecipe( CEconItemView* pNewRecipeItem );
	void ConsumeItem(  );
	void InitItemPanels();
	virtual const char *GetResFile( void ) { return "Resource/UI/DynamicRecipePanel.res"; }

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void PerformLayout( void ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnKeyCodePressed( vgui::KeyCode code ) OVERRIDE;
	void OnButtonChecked( KeyValues *pData ) OVERRIDE;

	virtual void OpenContextMenu() OVERRIDE {}
	virtual int	 GetNumItemPanels( void ) OVERRIDE;
	virtual void AddNewItemPanel( int iPanelIndex ) OVERRIDE;
	void Craft();
	virtual void OnTick( void ) OVERRIDE;
	virtual void OnShowPanel( bool bVisible, bool bReturningFromArmory ) OVERRIDE;
	void OnCraftResponse( itemid_t nNewToolID, EGCMsgResponse eResponse );
private:

	bool IsInputPanel( int iPanelIndex ) const;
	bool IsOutputPanel( int iPanelIndex) const;
	bool IsBackpackPanel( int iPanelIndex) const;
	bool IsInvPanelOnThisPage( unsigned nIndex ) const;
	int GetNumBackpackPanelsPerPage() const { return DYNAMIC_RECIPE_BACKPACK_ROWS * DYNAMIC_RECIPE_BACKPACK_COLS; }
	virtual int GetNumPages() OVERRIDE;
	virtual void SetCurrentPage( int nNewPage ) OVERRIDE;
	int GetFirstBackpackIndex() const { return DYNAMIC_RECIPE_INPUT_COUNT + DYNAMIC_RECIPE_OUTPUT_COUNT; }

	void SetCurrentInputPage( int nNewPage );
	int GetNumInputPages() const;
	int GetNumInputPanelsPerPage() const { return DYNAMIC_RECIPE_INPUT_COUNT; }

	int GetNumOutputPage() const;
	int GetNumOutputPanelsPerPage() const { return DYNAMIC_RECIPE_OUTPUT_COUNT; }

	class CRecipeComponentAttributeCounter : public CEconItemSpecificAttributeIterator
	{
	public:
		CRecipeComponentAttributeCounter()
			: m_nInputCount( 0 )
		{}
		~CRecipeComponentAttributeCounter() { Reset(); }

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value ) OVERRIDE;
		int GetInputCount() const { return m_nInputCount; }
		int GetOutputCount() const { return m_vecOutputItems.Count(); }
		CEconItemView* GetOutputItem( int i );
		CEconItemView* GetInputItem( int i );
		const CEconItemAttributeDefinition* GetInputAttrib( int i );

		void Reset();

	private:

		struct InputComponent_t
		{
			CEconItemView m_ItemView;
			const CEconItemAttributeDefinition* m_pAttrib;
		};

		typedef CUtlVector< CCopyableUtlVector<InputComponent_t> > InputComponentVec;

		static int LeastCommonInputSortFunc( const CCopyableUtlVector<InputComponent_t> *p1, const CCopyableUtlVector<InputComponent_t> *p2 );
		InputComponent_t* GetInputComponent( int i );

		InputComponentVec m_vecInputItems;
		CUtlVector< CEconItemView > m_vecOutputItems;
		CUtlVector< CEconItem* > m_vecTempEconItems;
		int m_nInputCount;
	};

	class CDynamicRecipeItemMatchFind : public CEconItemSpecificAttributeIterator
	{
	public:
		CDynamicRecipeItemMatchFind(  const CEconItemView* pSourceItem, const CEconItemView* pItemTomatch )
			: m_bMatchesAny( false )
			, m_pSourceItem( pSourceItem )
			, m_pItemToMatch( pItemTomatch )
		{}

		virtual bool OnIterateAttributeValue( const CEconItemAttributeDefinition *pAttrDef, const CAttribute_DynamicRecipeComponent& value ) OVERRIDE;
		bool MatchesAnyAttributes() const { return m_bMatchesAny; }
	private:
		const CEconItemView* m_pSourceItem;
		const CEconItemView* m_pItemToMatch;
		bool m_bMatchesAny;
	};

	CEconItemView* m_pDynamicRecipeItem;
	CRecipeComponentAttributeCounter m_RecipeIterator;

	bool AllRecipePanelsFilled( void );
	bool CheckForUntradableItems( void );
	bool WarnAboutPartialCompletion( void );
	void FindPossibleBackpackItems();
	virtual void PositionItemPanel( CItemModelPanel *pPanel, int iIndex );
	void PopulatePanelsForCurrentPage();
	virtual void UpdateModelPanels( void );
	virtual void SetBorderForItem( CItemModelPanel *pItemPanel, bool bMouseOver );
	void SetRecipeComponentIntoPanel( itemid_t nSrcRecipeIndex, CRecipeComponentItemModelPanel* pSrcPanel, int nSrcPage, CRecipeComponentItemModelPanel* pDstPanel, int nDstPage );
	bool InputPanelCanAcceptItem( CItemModelPanel* pPanel, itemid_t nItemID );

	CTFTextToolTip					*m_pToolTip;
	vgui::EditablePanel				*m_pToolTipEmbeddedPanel;
	CExButton						*m_pRecipeCraftButton;
	CExLabel						*m_pNoMatchesLabel;
	CExLabel						*m_pUntradableOutputsLabel;
	CExLabel						*m_pInputsLabel;
	CExLabel						*m_pOutputsLabel;
	vgui::Label						*m_pCurInputPageLabel;
	CExButton						*m_pNextInputPageButton;
	CExButton						*m_pPrevInputPageButton;
	CItemModelPanel					*m_pMouseOverItemPanel;
	vgui::CheckButton				*m_pShowUntradableItemsCheckbox;

	CUtlVector<CInputPanelItemModelPanel*> m_vecRecipeInputModelPanels;
	CUtlVector<CRecipeComponentItemModelPanel*> m_vecBackpackModelPanels;
	CUtlVector<CItemModelPanel*> m_vecRecipeOutputModelPanels;

	vgui::EditablePanel	*m_pRecipeContainer;
	vgui::EditablePanel *m_pInventoryContainer;

	unsigned m_nNumRecipeItems;
	bool m_bAllRecipePanelsFilled;
	bool m_bInputPanelsDirty;
	bool m_bShowUntradable;

	int m_nInputPage;
	int m_nOutputPage;

	float m_flAbortCraftingAt;

	MESSAGE_FUNC_PTR( OnItemPanelMouseDoublePressed, "ItemPanelMouseDoublePressed", panel );
	MESSAGE_FUNC_PTR( OnItemPanelEntered, "ItemPanelEntered", panel );
	MESSAGE_FUNC_PTR( OnItemPanelExited, "ItemPanelExited", panel );
	MESSAGE_FUNC( OnRecipeCompleted, "RecipeCompleted" );
	
	virtual bool AllowDragging( CItemModelPanel *panel ) OVERRIDE;
	virtual void StartDrag( int x, int y ) OVERRIDE;
	virtual void StopDrag( bool bSucceeded ) OVERRIDE;
	virtual bool CanDragTo( CItemModelPanel *pItemPanel, int iPanelIndex ) OVERRIDE;
	virtual void HandleDragTo( CItemModelPanel *pItemPanel, int iPanelIndex ) OVERRIDE;

	void ReturnRecipeItemToBackpack( itemid_t nItemID, CRecipeComponentItemModelPanel* pSrcPanel, int nSrcPage );

	CPanelAnimationVarAliasType( int, m_iItemCraftingOffcenterX, "item_crafting_offcenter_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iOutputItemYPos, "output_item_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iInventoryXPos, "inventory_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iInventoryYPos, "inventory_ypos", "0", "proportional_int" );

	friend void ConfirmDestroyItems( bool bConfirmed, void* pContext );
};

#endif // DYNAMIC_RECIPE_SUBPANEL_H
