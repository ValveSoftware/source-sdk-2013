/******************************************************************************
*   srengobj.h 
*       This file contains the declaration of the CSrEngine class.
*       This implements ISpSREngine and ISpObjectWithToken.
*       This is the main recognition object
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
******************************************************************************/

#pragma once

#include "stdafx.h"
#include "sreng.h"
#include "resource.h"

// A list of reco contexts is stored. Each entry in the list is an instance of this class.
class CContext
{
public:
    CContext *   m_pNext;
    BOOL operator==(SPRECOCONTEXTHANDLE hContext)
    {
        return (m_hSapiContext == hContext);
    }

    CContext(SPRECOCONTEXTHANDLE hSapiContext) : 
        m_hSapiContext(hSapiContext)
    {}

    SPRECOCONTEXTHANDLE m_hSapiContext; // The reco context handle given by SAPI
};

// A list of reco grammars is stored. Each entry in the list is an instance of this class.
class CDrvGrammar
{
public:
    CDrvGrammar *   m_pNext;
    SPGRAMMARHANDLE m_hSapiGrammar; // The grammar handle given by SAPI
    BOOL            m_SLMLoaded;    // Does the grammar have an associated SLM for dictation
    BOOL            m_SLMActive;    // Is the dictation active
    WCHAR* m_pWordSequenceText;     // The text of the word sequence buffer if one is set
    ULONG m_cchText;                // The size of the word sequence buffer
    SPTEXTSELECTIONINFO* m_pInfo; // The text selection of the word sequence buffer

    CDrvGrammar(SPGRAMMARHANDLE hSapiGrammar) : 
        m_hSapiGrammar(hSapiGrammar),
        m_SLMLoaded(FALSE),
        m_SLMActive(FALSE),
        m_pWordSequenceText(NULL),
        m_cchText(0),
        m_pInfo(NULL)
    {
    }

    ~CDrvGrammar()
    {
        // Free up resources
        //For each grammar object going to be released, SAPI would call SetWordSequenceData(NULL, 0, NULL).
        //SetWordSequenceData and SetTextSelection would release the memories.
        //There is no need to release memories referred by m_pWordSequenceText and m_pInfo here.
    }

#ifdef _WIN32_WCE
    CDrvGrammar()
    {
    }

    static LONG Compare(const CDrvGrammar *, const CDrvGrammar *)
    {
        return 0;
    }
#endif
};

// The RecognizeStream thread read audio data in blocks. For each block
// it decides if the data is speech or silence and adds that value to this queue.
// The decoder thread reads these and processes them.
// A critical section is used to make the queue thread-safe, and an event is used to 
// show if the buffer has space or not.
// This very roughtly simulates the idea of doing features extraction on
// one thread and passes the feature stream to the decoder.
class CFrameQueue
{
public:
    BOOL    m_aFrames[100]; // The queue of speech/silence values
    ULONG   m_cFrames;
    ULONG   m_ulHeadIndex;
    HANDLE  m_hSpaceAvailEvent;
    CRITICAL_SECTION m_cs;

    CFrameQueue()
    {
        m_cFrames = 0;
        m_ulHeadIndex = 0;
        m_hSpaceAvailEvent = NULL;
        InitializeCriticalSection(&m_cs);
    }
    ~CFrameQueue()
    {
        DeleteCriticalSection(&m_cs);
    }
    void SetSpaceAvailEvent(HANDLE h)
    {
        m_hSpaceAvailEvent = h;
    }
    void InsertTail(BOOL b)
    {
        EnterCriticalSection(&m_cs);
        ULONG ulTailIndex = (m_ulHeadIndex + m_cFrames) % sp_countof(m_aFrames);
        m_aFrames[ulTailIndex] = b;
        m_cFrames++;
        if (m_cFrames == sp_countof(m_aFrames))
        {
            ResetEvent(m_hSpaceAvailEvent);
        }
        LeaveCriticalSection(&m_cs);
    }
    BOOL IsFull()
    {
        EnterCriticalSection(&m_cs);
        BOOL b = (m_cFrames == sp_countof(m_aFrames));
        LeaveCriticalSection(&m_cs);
        return b;
    }
    BOOL RemoveHead()
    {
        EnterCriticalSection(&m_cs);
        BOOL b = m_aFrames[m_ulHeadIndex];
        m_ulHeadIndex = (m_ulHeadIndex + 1) % sp_countof(m_aFrames);
        m_cFrames--;
        SetEvent(m_hSpaceAvailEvent);
        LeaveCriticalSection(&m_cs);
        return b;
    }
    BOOL HasData()
    {
        EnterCriticalSection(&m_cs);
        ULONG cFrames = m_cFrames;
        LeaveCriticalSection(&m_cs);
        return cFrames;
    }
};


// Class so we can use CSpBasicQueue to store rule information
class CRuleEntry
{
public:
    BOOL operator==(SPRULEHANDLE rh)
    {
        return (m_hRule == rh);
    }
    CRuleEntry   * m_pNext;
    SPRULEHANDLE m_hRule;   // SAPI rule handle
    BOOL m_fTopLevel;       // Shows if rule can be activated
    BOOL m_fActive;         // Shows if rule is currectly active
};



// The main CSrEngine class

class ATL_NO_VTABLE CSrEngine : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSrEngine, &CLSID_SampleSREngine>,
    public ISpSREngine,
    public ISpObjectWithToken,
    public ISpThreadTask
{
public:
    CSrEngine() :
        m_ulNextGrammarIndex(0),
        m_cActive(0),
		m_bPhraseStarted(FALSE),
        m_bSoundStarted(FALSE),
        m_hQueueHasRoom(NULL),
        m_hRequestSync(NULL),
        m_LangID(0)
        {}

DECLARE_REGISTRY_RESOURCEID(IDR_SRENG)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSrEngine)
	COM_INTERFACE_ENTRY(ISpSREngine)
	COM_INTERFACE_ENTRY(ISpObjectWithToken)
END_COM_MAP()

private:
    HANDLE                          m_hRequestSync;
    CFrameQueue                     m_FrameQueue;
    ULONG                           m_cBlahBlah;    
    CSpBasicQueue<CDrvGrammar>      m_GrammarList;
    CSpBasicQueue<CContext>         m_ContextList;
    ULONG                           m_ulNextGrammarIndex;
    ULONG                           m_cActive;
    ULONGLONG                       m_ullStart;
    ULONGLONG                       m_ullEnd;
    BOOL                            m_bSoundStarted:1;
    BOOL							m_bPhraseStarted:1;
    CComPtr<ISpSREngineSite>        m_cpSite;
    CComPtr<ISpThreadControl>       m_cpDecoderThread;
    HANDLE                          m_hQueueHasRoom;
    CSpBasicQueue<CRuleEntry>       m_RuleList;
    CComPtr<ISpLexicon>             m_cpLexicon;
    CComPtr<ISpObjectToken>         m_cpEngineObjectToken;
    CComPtr<ISpObjectToken>         m_cpUserObjectToken;
    LANGID                          m_LangID;

public:

    HRESULT RandomlyWalkRule(SPRECORESULTINFO * pResult, ULONG nWords, ULONGLONG ullAudioPos, ULONG ulAudioSize);
    HRESULT RecurseWalk(SPSTATEHANDLE hState, SPPATHENTRY * pPath, ULONG * pcTrans);
    HRESULT WalkCFGRule(SPRECORESULTINFO * pResult, ULONG cRulesActive, BOOL fHypothesis,
                        ULONG nWords, ULONGLONG ullAudioPos, ULONG ulAudioSize);
    HRESULT WalkSLM(SPRECORESULTINFO * pResult, ULONG cSLMActive,
                    ULONG nWords, ULONGLONG ullAudioPos, ULONG ulAudioSize);
    HRESULT WalkTextBuffer(void* pvGrammarCookie, SPPATHENTRY * pPath, SPTRANSITIONID hId, ULONG * pcTrans);

    HRESULT AddEvent(SPEVENTENUM eEvent, ULONGLONG ullStreamPos, WPARAM wParam = 0, LPARAM lParam = 0);
    HRESULT AddEventString(SPEVENTENUM eEvent, ULONGLONG ulLStreamPos, const WCHAR * psz, WPARAM = 0);

    HRESULT CreatePhraseFromRule( CRuleEntry * pRule, BOOL fHypothesis,
                                  ULONGLONG ullAudioPos, ULONG ulAudioSize,
                                  ISpPhraseBuilder** ppPhrase );

    CRuleEntry* FindRule( ULONG ulRuleIndex );
    CRuleEntry* NextRuleAlt( CRuleEntry * pPriRule, CRuleEntry * pLastRule );

    void _CheckRecognition();
    void _NotifyRecognition(BOOL fHypothesis, ULONG nWords);

    // ATL contstructor / destructor
    HRESULT FinalConstruct();
    HRESULT FinalRelease();

    // Initialization methods
    STDMETHODIMP SetObjectToken(ISpObjectToken * pToken);
    STDMETHODIMP GetObjectToken(ISpObjectToken ** ppToken);

    STDMETHODIMP SetRecoProfile(ISpObjectToken * pProfileToken);
    STDMETHODIMP SetSite(ISpSREngineSite *pSite);
    STDMETHODIMP GetInputAudioFormat(const GUID * pSrcFormatId, const WAVEFORMATEX * pSrcWFEX,
                                     GUID * pDesiredFormatId, WAVEFORMATEX ** ppCoMemDesiredWFEX);

    STDMETHODIMP OnCreateRecoContext(SPRECOCONTEXTHANDLE hSAPIRecoContext, void ** ppvDrvCtxt);
    STDMETHODIMP OnDeleteRecoContext(void * pvDrvCtxt);

	STDMETHODIMP OnCreateGrammar(void * pvEngineRecoContext,
                                 SPGRAMMARHANDLE hSAPIGrammar,
                                 void ** ppvEngineGrammar);

    STDMETHODIMP OnDeleteGrammar(void * pvEngineGrammar);

    // CFG methods
    STDMETHODIMP WordNotify(SPCFGNOTIFY Action, ULONG cWords, const SPWORDENTRY * pWords);
    STDMETHODIMP RuleNotify(SPCFGNOTIFY Action, ULONG cRules, const SPRULEENTRY * pRules);

    // Proprietary grammar methods
    //  - used to implement an engine-specific grammar format
    //  - this sample does not implement these
    STDMETHODIMP LoadProprietaryGrammar(void * pvEngineGrammar,
                                        REFGUID rguidParam,
                                        const WCHAR * pszStringParam,
                                        const void * pvDataParam,
                                        ULONG ulDataSize,
                                        SPLOADOPTIONS Options)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP UnloadProprietaryGrammar(void * pvEngineGrammar)
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP SetProprietaryRuleState(void * pvEngineGrammar, 
                                    const WCHAR * pszName,
                                    void * pvReserved,
                                    SPRULESTATE NewState,
                                    ULONG * pcRulesChanged)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP SetProprietaryRuleIdState(void * pvEngineGrammar, 
                                      DWORD dwRuleId,
                                      SPRULESTATE NewState)
    {
        return E_NOTIMPL;
    }

    // Since this engine does not support proprietary grammars, we do not need to implement
    // this method other than just returning S_OK.  Note to implementors:  Do NOT return
    // E_NOTIMPL.  Just return S_OK and ignore this data if you do not need it to implement
    // proprietary grammars.
    STDMETHODIMP SetGrammarState(void * pvEngineGrammar, SPGRAMMARSTATE eGrammarState)
    {
        return S_OK;
    }
    STDMETHODIMP SetContextState(void * pvEngineContxt, SPCONTEXTSTATE eCtxtState)
    {
        return S_OK;
    }


    // Dictation methods
    STDMETHODIMP LoadSLM(void * pvEngineGrammar, const WCHAR * pszTopicName);
    STDMETHODIMP UnloadSLM(void * pvEngineGrammar);
    STDMETHODIMP SetSLMState(void * pvEngineGrammar, SPRULESTATE NewState);

    STDMETHODIMP IsPronounceable(void *pDrvGrammar, const WCHAR *pszWord, SPWORDPRONOUNCEABLE * pWordPronounceable);
    STDMETHODIMP SetWordSequenceData(void * pvEngineGrammar, const WCHAR * pText, ULONG cchText, const SPTEXTSELECTIONINFO * pInfo);
    STDMETHODIMP SetTextSelection(void * pvEngineGrammar, const SPTEXTSELECTIONINFO * pInfo);
    STDMETHODIMP SetAdaptationData(void * pvEngineCtxtCookie, const WCHAR * pText, const ULONG cch);    

    // Property methods
    STDMETHODIMP SetPropertyNum( SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, LONG lValue );
    STDMETHODIMP GetPropertyNum( SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, LONG * plValue );
    STDMETHODIMP SetPropertyString( SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, const WCHAR* pValue );
    STDMETHODIMP GetPropertyString( SPPROPSRC eSrc, void* pvSrcObj, const WCHAR* pName, WCHAR** ppCoMemValue );


    // The main recognition method
    STDMETHODIMP RecognizeStream(REFGUID rguidFmtId, const WAVEFORMATEX * pWaveFormatEx,
                            HANDLE hRequestSync, HANDLE hDataAvailable,
                            HANDLE hExit, BOOL fNewAudioStream, BOOL fRealTimeAudio,
                            ISpObjectToken * pAudioObjectToken);

    STDMETHODIMP PrivateCall(void * pvEngineContext, void * pCallFrame, ULONG ulCallFrameSize);
    STDMETHODIMP PrivateCallEx(void * pvEngineContext, const void * pInCallFrame, ULONG ulCallFrameSize,
                               void ** ppvCoMemResponse, ULONG * pcbResponse);


    // ISpThreadTask methods
    STDMETHODIMP InitThread( void * pvTaskData, HWND hwnd )
    {
        return S_OK;
    }
    LRESULT STDMETHODCALLTYPE WindowMessage( void *pvTaskData, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
    {
        return E_UNEXPECTED;
    }

    STDMETHODIMP ThreadProc( void *pvTaskData, HANDLE hExitThreadEvent, HANDLE hNotifyEvent, HWND hwndWorker, volatile const BOOL * pfContinueProcessing );
};

