/****************************************************************************
* Reco.H *
*--------*
*   Description:
*       Header file for Reco tool.  This file declares the various dialog classes.
*
****************************************************************************/

#if !defined(AFX_RECO_H__BD16E9D0_597E_11D2_960E_00C04F8EE628__INCLUDED_)
#define AFX_RECO_H__BD16E9D0_597E_11D2_960E_00C04F8EE628__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
//  Defines
//
#define MYGRAMMARID     101
#define ITNGRAMMARID    102
#define WM_RECOEVENT    WM_APP      // Window message used for recognition events

#define MAX_LOADSTRING  250

//
//  Constant strings
//
const WCHAR g_szDynRuleName[] = L"DynRule";

//
//  Forward class declarations
//

class CRecoDlgListItem;
class CRecoDlgClass;
class CDynGrammarDlgClass;

//
//   Helper class for CSpBasicQueue
//
class CDynItem
{
public:
    CDynItem(WCHAR *psz) : m_dstr(psz) {} ;
public:
    CSpDynamicString    m_dstr;
    CDynItem          * m_pNext;
};

/****************************************************************************
* CRecoDlgListItem *
*------------------*
*   
*   This class stores the recognition result as well as a text string associated
*   with the recognition.  Note that the string will sometimes be <noise> and
*   the pResult will be NULL.  In other cases the string will be <Unrecognized>
*   and pResult will be valid.
*
****************************************************************************/

class CRecoDlgListItem
{
public:
    CRecoDlgListItem(ISpRecoResult * pResult, const WCHAR * pwsz, BOOL fHypothesis) :
        m_cpRecoResult(pResult),
        m_dstr(pwsz),
        m_fHypothesis(fHypothesis)
    {}

    ISpRecoResult * GetRecoResult() const { return m_cpRecoResult; }
    int GetTextLength() const { return m_dstr.Length(); }
    const WCHAR * GetText() const { return m_dstr; }
    BOOL IsHypothesis() const { return m_fHypothesis; }

private:
    CComPtr<ISpRecoResult>  m_cpRecoResult;
    CSpDynamicString        m_dstr;
    BOOL                    m_fHypothesis;
};


/****************************************************************************
* CRecoDlgClass *
*---------------*
*
*   This class manages the main application dialog.
*
****************************************************************************/

class CRecoDlgClass
{
    friend CDynGrammarDlgClass;
//
//  Public methods
//
public:
    CRecoDlgClass(HINSTANCE hInstance) :
        m_hInstance(hInstance),
        m_bInSound(FALSE),
        m_bGotReco(FALSE),
        m_hfont(NULL)
    {
        m_szGrammarFile[0] = 0;
    }

    static LRESULT CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//
//  Private methods
//
private:
    HRESULT Create(BOOL bCreateChecked);
    BOOL InitDialog(HWND hDlg);
    HRESULT CreateRecoCtxt(LRESULT ItemData);
    HRESULT UpdateRecoCtxtState();
    void Cleanup();
    void Reset();
    void RecoEvent();
    void SpecifyCAndCGrammar();
    void SetWordSequenceData();
    void EmulateRecognition(WCHAR *pszText);
    void UpdateGrammarStatusWindow();
    HRESULT ConstructRuleDisplay(const SPPHRASERULE *pRule, CSpDynamicString & dstr, ULONG ulLevel);
    HRESULT ConstructPropertyDisplay(const SPPHRASEELEMENT *pElem, const SPPHRASEPROPERTY *pProp, 
                                     CSpDynamicString & dstr, ULONG ulLevel);
    BOOL UpdatePropWindow(const CRecoDlgListItem * pli);
    HRESULT UpdateGrammarState(WORD wIdOfChangedControl);

    HRESULT GetTextFile(WCHAR ** ppwszCoMem, ULONG * pcch);

    HRESULT FeedDocumentFromFile();
    HRESULT FeedDocumentFromClipboard();
    HRESULT FeedDocument(const WCHAR * pwszCoMem, ULONG cch);

    HRESULT TrainFromFile();

    int MessageBoxFromResource( UINT uiResource );

//
//  Member data
//  
private:
    const HINSTANCE                 m_hInstance;                    // Instance handle of process
    HWND                            m_hDlg;                         // Window handle of dialog
    CComPtr<ISpRecoGrammar>         m_cpCFGGrammar;                 // Loaded CFG grammar
    CComPtr<ISpVoice>               m_cpCFGVoice;
    CComPtr<ISpRecoGrammar>         m_cpITNGrammar;                 // Loaded ITN cfg grammar
    CComPtr<ISpRecoGrammar>         m_cpDictationGrammar;           // Loaded dictation grammar
    CComPtr<ISpRecoGrammar>         m_cpSpellingGrammar;            // Loaded spelling grammar
    CComPtr<ISpRecoContext>         m_cpRecoCtxt;                   // Recognition context
    CComPtr<ISpRecognizer>          m_cpRecognizer;                 // Recognition instance
    CComPtr<ISpPhoneConverter>      m_cpPhoneConv;                  // Phone converter
    BOOL                            m_bInSound;                    
    BOOL                            m_bGotReco;
    TCHAR                           m_szGrammarFile[MAX_PATH];      // Fully qualified file path
    TCHAR                           m_szGrammarFileTitle[MAX_PATH]; // Name of the file (no path)
    LANGID                          m_langid;
    HFONT                           m_hfont;
    CSpBasicQueue<CDynItem>         m_ItemList;
};

/****************************************************************************
* CDynGrammarDlgClass *
*---------------------*
*
*   This class manages the dynamic rule dialog.
*
****************************************************************************/

class CDynGrammarDlgClass
{
//
//  Public methods
//
public:
    CDynGrammarDlgClass(CRecoDlgClass * pParent, CSpBasicQueue<CDynItem> *pItemList) :
        m_pParent(pParent)
    {
        m_hinstRichEdit = LoadLibrary(_T("riched20.dll"));
        m_hDynRule = NULL;
        m_pItemList = pItemList;
    }

    ~CDynGrammarDlgClass()
    {
        FreeLibrary(m_hinstRichEdit);
    }

    static LRESULT CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
//
//  Private methods
//
private:
    BOOL InitDialog(HWND hDlg, CSpBasicQueue<CDynItem> *pItemList);
    void AddItem();
    void Cleanup();
    void ClearAll();

//
//  Member data
//
private:
    const CRecoDlgClass *   m_pParent;
    HWND                    m_hDlg;
    SPSTATEHANDLE           m_hDynRule;
    HINSTANCE               m_hinstRichEdit;
    CSpBasicQueue<CDynItem> * m_pItemList;
};

//=== This is for the alternates pop window
/****************************************************************************
* CAlternatesDlgClass *
*---------------------*
*
****************************************************************************/
#define NUM_ALTS    5

class CAlternatesDlgClass
{
  public:
    CAlternatesDlgClass()
    {
        m_ulNumAltsReturned = 0;
        memset( m_Alts, 0, NUM_ALTS * sizeof(ISpPhraseAlt*) ); 
    }

    ~CAlternatesDlgClass()
    {
        for( ULONG i = 0; i < m_ulNumAltsReturned; ++i )
        {
            m_Alts[i]->Release();
        }
    }

    static LRESULT CALLBACK AlternatesDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    //--- Member data
    ISpRecoResult*  m_pResult;
    ISpPhraseAlt*   m_Alts[NUM_ALTS];
    ULONG           m_ulNumAltsReturned;
};

//
//  This dialog function is only defined for the preview release of this tool.
//
LRESULT CALLBACK BetaHelpDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

//
// Helper function
//
inline char ConfidenceGroupChar(char Confidence)
{
    switch (Confidence)
    {
    case SP_LOW_CONFIDENCE:
        return '-';

    case SP_NORMAL_CONFIDENCE:
        return ' ';

    case SP_HIGH_CONFIDENCE:
        return '+';

    default:
        _ASSERTE(false);
        return '?';
    }
}



#endif // !defined(AFX_RECO_H__BD16E9D0_597E_11D2_960E_00C04F8EE628__INCLUDED_)
