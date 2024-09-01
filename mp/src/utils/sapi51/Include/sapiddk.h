
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for sapiddk.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __sapiddk_h__
#define __sapiddk_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ISpTokenUI_FWD_DEFINED__
#define __ISpTokenUI_FWD_DEFINED__
typedef interface ISpTokenUI ISpTokenUI;
#endif 	/* __ISpTokenUI_FWD_DEFINED__ */


#ifndef __ISpObjectTokenEnumBuilder_FWD_DEFINED__
#define __ISpObjectTokenEnumBuilder_FWD_DEFINED__
typedef interface ISpObjectTokenEnumBuilder ISpObjectTokenEnumBuilder;
#endif 	/* __ISpObjectTokenEnumBuilder_FWD_DEFINED__ */


#ifndef __ISpErrorLog_FWD_DEFINED__
#define __ISpErrorLog_FWD_DEFINED__
typedef interface ISpErrorLog ISpErrorLog;
#endif 	/* __ISpErrorLog_FWD_DEFINED__ */


#ifndef __ISpGrammarCompiler_FWD_DEFINED__
#define __ISpGrammarCompiler_FWD_DEFINED__
typedef interface ISpGrammarCompiler ISpGrammarCompiler;
#endif 	/* __ISpGrammarCompiler_FWD_DEFINED__ */


#ifndef __ISpGramCompBackend_FWD_DEFINED__
#define __ISpGramCompBackend_FWD_DEFINED__
typedef interface ISpGramCompBackend ISpGramCompBackend;
#endif 	/* __ISpGramCompBackend_FWD_DEFINED__ */


#ifndef __ISpITNProcessor_FWD_DEFINED__
#define __ISpITNProcessor_FWD_DEFINED__
typedef interface ISpITNProcessor ISpITNProcessor;
#endif 	/* __ISpITNProcessor_FWD_DEFINED__ */


#ifndef __ISpPhraseBuilder_FWD_DEFINED__
#define __ISpPhraseBuilder_FWD_DEFINED__
typedef interface ISpPhraseBuilder ISpPhraseBuilder;
#endif 	/* __ISpPhraseBuilder_FWD_DEFINED__ */


#ifndef __ISpThreadControl_FWD_DEFINED__
#define __ISpThreadControl_FWD_DEFINED__
typedef interface ISpThreadControl ISpThreadControl;
#endif 	/* __ISpThreadControl_FWD_DEFINED__ */


#ifndef __ISpTaskManager_FWD_DEFINED__
#define __ISpTaskManager_FWD_DEFINED__
typedef interface ISpTaskManager ISpTaskManager;
#endif 	/* __ISpTaskManager_FWD_DEFINED__ */


#ifndef __ISpTTSEngineSite_FWD_DEFINED__
#define __ISpTTSEngineSite_FWD_DEFINED__
typedef interface ISpTTSEngineSite ISpTTSEngineSite;
#endif 	/* __ISpTTSEngineSite_FWD_DEFINED__ */


#ifndef __ISpTTSEngine_FWD_DEFINED__
#define __ISpTTSEngine_FWD_DEFINED__
typedef interface ISpTTSEngine ISpTTSEngine;
#endif 	/* __ISpTTSEngine_FWD_DEFINED__ */


#ifndef __ISpCFGInterpreterSite_FWD_DEFINED__
#define __ISpCFGInterpreterSite_FWD_DEFINED__
typedef interface ISpCFGInterpreterSite ISpCFGInterpreterSite;
#endif 	/* __ISpCFGInterpreterSite_FWD_DEFINED__ */


#ifndef __ISpCFGInterpreter_FWD_DEFINED__
#define __ISpCFGInterpreter_FWD_DEFINED__
typedef interface ISpCFGInterpreter ISpCFGInterpreter;
#endif 	/* __ISpCFGInterpreter_FWD_DEFINED__ */


#ifndef __ISpSREngineSite_FWD_DEFINED__
#define __ISpSREngineSite_FWD_DEFINED__
typedef interface ISpSREngineSite ISpSREngineSite;
#endif 	/* __ISpSREngineSite_FWD_DEFINED__ */


#ifndef __ISpSREngine_FWD_DEFINED__
#define __ISpSREngine_FWD_DEFINED__
typedef interface ISpSREngine ISpSREngine;
#endif 	/* __ISpSREngine_FWD_DEFINED__ */


#ifndef __ISpSRAlternates_FWD_DEFINED__
#define __ISpSRAlternates_FWD_DEFINED__
typedef interface ISpSRAlternates ISpSRAlternates;
#endif 	/* __ISpSRAlternates_FWD_DEFINED__ */


#ifndef ___ISpPrivateEngineCall_FWD_DEFINED__
#define ___ISpPrivateEngineCall_FWD_DEFINED__
typedef interface _ISpPrivateEngineCall _ISpPrivateEngineCall;
#endif 	/* ___ISpPrivateEngineCall_FWD_DEFINED__ */


#ifndef __SpDataKey_FWD_DEFINED__
#define __SpDataKey_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpDataKey SpDataKey;
#else
typedef struct SpDataKey SpDataKey;
#endif /* __cplusplus */

#endif 	/* __SpDataKey_FWD_DEFINED__ */


#ifndef __SpObjectTokenEnum_FWD_DEFINED__
#define __SpObjectTokenEnum_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpObjectTokenEnum SpObjectTokenEnum;
#else
typedef struct SpObjectTokenEnum SpObjectTokenEnum;
#endif /* __cplusplus */

#endif 	/* __SpObjectTokenEnum_FWD_DEFINED__ */


#ifndef __SpPhraseBuilder_FWD_DEFINED__
#define __SpPhraseBuilder_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpPhraseBuilder SpPhraseBuilder;
#else
typedef struct SpPhraseBuilder SpPhraseBuilder;
#endif /* __cplusplus */

#endif 	/* __SpPhraseBuilder_FWD_DEFINED__ */


#ifndef __SpITNProcessor_FWD_DEFINED__
#define __SpITNProcessor_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpITNProcessor SpITNProcessor;
#else
typedef struct SpITNProcessor SpITNProcessor;
#endif /* __cplusplus */

#endif 	/* __SpITNProcessor_FWD_DEFINED__ */


#ifndef __SpGrammarCompiler_FWD_DEFINED__
#define __SpGrammarCompiler_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpGrammarCompiler SpGrammarCompiler;
#else
typedef struct SpGrammarCompiler SpGrammarCompiler;
#endif /* __cplusplus */

#endif 	/* __SpGrammarCompiler_FWD_DEFINED__ */


#ifndef __SpGramCompBackend_FWD_DEFINED__
#define __SpGramCompBackend_FWD_DEFINED__

#ifdef __cplusplus
typedef class SpGramCompBackend SpGramCompBackend;
#else
typedef struct SpGramCompBackend SpGramCompBackend;
#endif /* __cplusplus */

#endif 	/* __SpGramCompBackend_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"
#include "sapi.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_sapiddk_0000 */
/* [local] */ 






#define SPRECOEXTENSION   L"RecoExtension"
#define SPALTERNATESCLSID L"AlternatesCLSID"


extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0000_v0_0_s_ifspec;

#ifndef __ISpTokenUI_INTERFACE_DEFINED__
#define __ISpTokenUI_INTERFACE_DEFINED__

/* interface ISpTokenUI */
/* [restricted][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpTokenUI;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F8E690F0-39CB-4843-B8D7-C84696E1119D")
    ISpTokenUI : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE IsUISupported( 
            /* [in] */ const WCHAR *pszTypeOfUI,
            /* [in] */ void *pvExtraData,
            /* [in] */ ULONG cbExtraData,
            /* [in] */ IUnknown *punkObject,
            /* [out] */ BOOL *pfSupported) = 0;
        
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE DisplayUI( 
            /* [in] */ HWND hwndParent,
            /* [in] */ const WCHAR *pszTitle,
            /* [in] */ const WCHAR *pszTypeOfUI,
            /* [in] */ void *pvExtraData,
            /* [in] */ ULONG cbExtraData,
            /* [in] */ ISpObjectToken *pToken,
            /* [in] */ IUnknown *punkObject) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpTokenUIVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpTokenUI * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpTokenUI * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpTokenUI * This);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *IsUISupported )( 
            ISpTokenUI * This,
            /* [in] */ const WCHAR *pszTypeOfUI,
            /* [in] */ void *pvExtraData,
            /* [in] */ ULONG cbExtraData,
            /* [in] */ IUnknown *punkObject,
            /* [out] */ BOOL *pfSupported);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *DisplayUI )( 
            ISpTokenUI * This,
            /* [in] */ HWND hwndParent,
            /* [in] */ const WCHAR *pszTitle,
            /* [in] */ const WCHAR *pszTypeOfUI,
            /* [in] */ void *pvExtraData,
            /* [in] */ ULONG cbExtraData,
            /* [in] */ ISpObjectToken *pToken,
            /* [in] */ IUnknown *punkObject);
        
        END_INTERFACE
    } ISpTokenUIVtbl;

    interface ISpTokenUI
    {
        CONST_VTBL struct ISpTokenUIVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpTokenUI_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpTokenUI_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpTokenUI_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpTokenUI_IsUISupported(This,pszTypeOfUI,pvExtraData,cbExtraData,punkObject,pfSupported)	\
    (This)->lpVtbl -> IsUISupported(This,pszTypeOfUI,pvExtraData,cbExtraData,punkObject,pfSupported)

#define ISpTokenUI_DisplayUI(This,hwndParent,pszTitle,pszTypeOfUI,pvExtraData,cbExtraData,pToken,punkObject)	\
    (This)->lpVtbl -> DisplayUI(This,hwndParent,pszTitle,pszTypeOfUI,pvExtraData,cbExtraData,pToken,punkObject)

#endif /* COBJMACROS */


#endif 	/* C style interface */



/* [local] */ HRESULT STDMETHODCALLTYPE ISpTokenUI_IsUISupported_Proxy( 
    ISpTokenUI * This,
    /* [in] */ const WCHAR *pszTypeOfUI,
    /* [in] */ void *pvExtraData,
    /* [in] */ ULONG cbExtraData,
    /* [in] */ IUnknown *punkObject,
    /* [out] */ BOOL *pfSupported);


void __RPC_STUB ISpTokenUI_IsUISupported_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


/* [local] */ HRESULT STDMETHODCALLTYPE ISpTokenUI_DisplayUI_Proxy( 
    ISpTokenUI * This,
    /* [in] */ HWND hwndParent,
    /* [in] */ const WCHAR *pszTitle,
    /* [in] */ const WCHAR *pszTypeOfUI,
    /* [in] */ void *pvExtraData,
    /* [in] */ ULONG cbExtraData,
    /* [in] */ ISpObjectToken *pToken,
    /* [in] */ IUnknown *punkObject);


void __RPC_STUB ISpTokenUI_DisplayUI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpTokenUI_INTERFACE_DEFINED__ */


#ifndef __ISpObjectTokenEnumBuilder_INTERFACE_DEFINED__
#define __ISpObjectTokenEnumBuilder_INTERFACE_DEFINED__

/* interface ISpObjectTokenEnumBuilder */
/* [restricted][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpObjectTokenEnumBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("06B64F9F-7FDA-11d2-B4F2-00C04F797396")
    ISpObjectTokenEnumBuilder : public IEnumSpObjectTokens
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetAttribs( 
            const WCHAR *pszReqAttribs,
            const WCHAR *pszOptAttribs) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTokens( 
            ULONG cTokens,
            ISpObjectToken **pToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTokensFromDataKey( 
            ISpDataKey *pDataKey,
            const WCHAR *pszSubKey,
            const WCHAR *pszCategoryId) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTokensFromTokenEnum( 
            IEnumSpObjectTokens *pTokenEnum) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Sort( 
            const WCHAR *pszTokenIdToListFirst) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpObjectTokenEnumBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpObjectTokenEnumBuilder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpObjectTokenEnumBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpObjectTokenEnumBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            ISpObjectTokenEnumBuilder * This,
            /* [in] */ ULONG celt,
            /* [length_is][size_is][out] */ ISpObjectToken **pelt,
            /* [out] */ ULONG *pceltFetched);
        
        HRESULT ( STDMETHODCALLTYPE *Skip )( 
            ISpObjectTokenEnumBuilder * This,
            /* [in] */ ULONG celt);
        
        HRESULT ( STDMETHODCALLTYPE *Reset )( 
            ISpObjectTokenEnumBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *Clone )( 
            ISpObjectTokenEnumBuilder * This,
            /* [out] */ IEnumSpObjectTokens **ppEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Item )( 
            ISpObjectTokenEnumBuilder * This,
            /* [in] */ ULONG Index,
            /* [out] */ ISpObjectToken **ppToken);
        
        HRESULT ( STDMETHODCALLTYPE *GetCount )( 
            ISpObjectTokenEnumBuilder * This,
            /* [out] */ ULONG *pCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetAttribs )( 
            ISpObjectTokenEnumBuilder * This,
            const WCHAR *pszReqAttribs,
            const WCHAR *pszOptAttribs);
        
        HRESULT ( STDMETHODCALLTYPE *AddTokens )( 
            ISpObjectTokenEnumBuilder * This,
            ULONG cTokens,
            ISpObjectToken **pToken);
        
        HRESULT ( STDMETHODCALLTYPE *AddTokensFromDataKey )( 
            ISpObjectTokenEnumBuilder * This,
            ISpDataKey *pDataKey,
            const WCHAR *pszSubKey,
            const WCHAR *pszCategoryId);
        
        HRESULT ( STDMETHODCALLTYPE *AddTokensFromTokenEnum )( 
            ISpObjectTokenEnumBuilder * This,
            IEnumSpObjectTokens *pTokenEnum);
        
        HRESULT ( STDMETHODCALLTYPE *Sort )( 
            ISpObjectTokenEnumBuilder * This,
            const WCHAR *pszTokenIdToListFirst);
        
        END_INTERFACE
    } ISpObjectTokenEnumBuilderVtbl;

    interface ISpObjectTokenEnumBuilder
    {
        CONST_VTBL struct ISpObjectTokenEnumBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpObjectTokenEnumBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpObjectTokenEnumBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpObjectTokenEnumBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpObjectTokenEnumBuilder_Next(This,celt,pelt,pceltFetched)	\
    (This)->lpVtbl -> Next(This,celt,pelt,pceltFetched)

#define ISpObjectTokenEnumBuilder_Skip(This,celt)	\
    (This)->lpVtbl -> Skip(This,celt)

#define ISpObjectTokenEnumBuilder_Reset(This)	\
    (This)->lpVtbl -> Reset(This)

#define ISpObjectTokenEnumBuilder_Clone(This,ppEnum)	\
    (This)->lpVtbl -> Clone(This,ppEnum)

#define ISpObjectTokenEnumBuilder_Item(This,Index,ppToken)	\
    (This)->lpVtbl -> Item(This,Index,ppToken)

#define ISpObjectTokenEnumBuilder_GetCount(This,pCount)	\
    (This)->lpVtbl -> GetCount(This,pCount)


#define ISpObjectTokenEnumBuilder_SetAttribs(This,pszReqAttribs,pszOptAttribs)	\
    (This)->lpVtbl -> SetAttribs(This,pszReqAttribs,pszOptAttribs)

#define ISpObjectTokenEnumBuilder_AddTokens(This,cTokens,pToken)	\
    (This)->lpVtbl -> AddTokens(This,cTokens,pToken)

#define ISpObjectTokenEnumBuilder_AddTokensFromDataKey(This,pDataKey,pszSubKey,pszCategoryId)	\
    (This)->lpVtbl -> AddTokensFromDataKey(This,pDataKey,pszSubKey,pszCategoryId)

#define ISpObjectTokenEnumBuilder_AddTokensFromTokenEnum(This,pTokenEnum)	\
    (This)->lpVtbl -> AddTokensFromTokenEnum(This,pTokenEnum)

#define ISpObjectTokenEnumBuilder_Sort(This,pszTokenIdToListFirst)	\
    (This)->lpVtbl -> Sort(This,pszTokenIdToListFirst)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpObjectTokenEnumBuilder_SetAttribs_Proxy( 
    ISpObjectTokenEnumBuilder * This,
    const WCHAR *pszReqAttribs,
    const WCHAR *pszOptAttribs);


void __RPC_STUB ISpObjectTokenEnumBuilder_SetAttribs_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpObjectTokenEnumBuilder_AddTokens_Proxy( 
    ISpObjectTokenEnumBuilder * This,
    ULONG cTokens,
    ISpObjectToken **pToken);


void __RPC_STUB ISpObjectTokenEnumBuilder_AddTokens_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpObjectTokenEnumBuilder_AddTokensFromDataKey_Proxy( 
    ISpObjectTokenEnumBuilder * This,
    ISpDataKey *pDataKey,
    const WCHAR *pszSubKey,
    const WCHAR *pszCategoryId);


void __RPC_STUB ISpObjectTokenEnumBuilder_AddTokensFromDataKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpObjectTokenEnumBuilder_AddTokensFromTokenEnum_Proxy( 
    ISpObjectTokenEnumBuilder * This,
    IEnumSpObjectTokens *pTokenEnum);


void __RPC_STUB ISpObjectTokenEnumBuilder_AddTokensFromTokenEnum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpObjectTokenEnumBuilder_Sort_Proxy( 
    ISpObjectTokenEnumBuilder * This,
    const WCHAR *pszTokenIdToListFirst);


void __RPC_STUB ISpObjectTokenEnumBuilder_Sort_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpObjectTokenEnumBuilder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0339 */
/* [local] */ 

#if 0
typedef void *SPWORDHANDLE;

typedef void *SPRULEHANDLE;

typedef void *SPGRAMMARHANDLE;

typedef void *SPRECOCONTEXTHANDLE;

typedef void *SPPHRASERULEHANDLE;

typedef void *SPPHRASEPROPERTYHANDLE;

typedef void *SPTRANSITIONID;

#else
DECLARE_HANDLE(SPWORDHANDLE);
DECLARE_HANDLE(SPRULEHANDLE);
DECLARE_HANDLE(SPGRAMMARHANDLE);
DECLARE_HANDLE(SPRECOCONTEXTHANDLE);
DECLARE_HANDLE(SPPHRASERULEHANDLE);
DECLARE_HANDLE(SPPHRASEPROPERTYHANDLE);
DECLARE_HANDLE(SPTRANSITIONID);
#endif


extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0339_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0339_v0_0_s_ifspec;

#ifndef __ISpErrorLog_INTERFACE_DEFINED__
#define __ISpErrorLog_INTERFACE_DEFINED__

/* interface ISpErrorLog */
/* [restricted][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpErrorLog;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F4711347-E608-11d2-A086-00C04F8EF9B5")
    ISpErrorLog : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddError( 
            const long lLineNumber,
            HRESULT hr,
            const WCHAR *pszDescription,
            const WCHAR *pszHelpFile,
            DWORD dwHelpContext) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpErrorLogVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpErrorLog * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpErrorLog * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpErrorLog * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddError )( 
            ISpErrorLog * This,
            const long lLineNumber,
            HRESULT hr,
            const WCHAR *pszDescription,
            const WCHAR *pszHelpFile,
            DWORD dwHelpContext);
        
        END_INTERFACE
    } ISpErrorLogVtbl;

    interface ISpErrorLog
    {
        CONST_VTBL struct ISpErrorLogVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpErrorLog_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpErrorLog_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpErrorLog_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpErrorLog_AddError(This,lLineNumber,hr,pszDescription,pszHelpFile,dwHelpContext)	\
    (This)->lpVtbl -> AddError(This,lLineNumber,hr,pszDescription,pszHelpFile,dwHelpContext)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpErrorLog_AddError_Proxy( 
    ISpErrorLog * This,
    const long lLineNumber,
    HRESULT hr,
    const WCHAR *pszDescription,
    const WCHAR *pszHelpFile,
    DWORD dwHelpContext);


void __RPC_STUB ISpErrorLog_AddError_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpErrorLog_INTERFACE_DEFINED__ */


#ifndef __ISpGrammarCompiler_INTERFACE_DEFINED__
#define __ISpGrammarCompiler_INTERFACE_DEFINED__

/* interface ISpGrammarCompiler */
/* [restricted][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpGrammarCompiler;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B1E29D58-A675-11D2-8302-00C04F8EE6C0")
    ISpGrammarCompiler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CompileStream( 
            IStream *pSource,
            IStream *pDest,
            IStream *pHeader,
            IUnknown *pReserved,
            ISpErrorLog *pErrorLog,
            /* [in] */ DWORD dwFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpGrammarCompilerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpGrammarCompiler * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpGrammarCompiler * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpGrammarCompiler * This);
        
        HRESULT ( STDMETHODCALLTYPE *CompileStream )( 
            ISpGrammarCompiler * This,
            IStream *pSource,
            IStream *pDest,
            IStream *pHeader,
            IUnknown *pReserved,
            ISpErrorLog *pErrorLog,
            /* [in] */ DWORD dwFlags);
        
        END_INTERFACE
    } ISpGrammarCompilerVtbl;

    interface ISpGrammarCompiler
    {
        CONST_VTBL struct ISpGrammarCompilerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpGrammarCompiler_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpGrammarCompiler_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpGrammarCompiler_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpGrammarCompiler_CompileStream(This,pSource,pDest,pHeader,pReserved,pErrorLog,dwFlags)	\
    (This)->lpVtbl -> CompileStream(This,pSource,pDest,pHeader,pReserved,pErrorLog,dwFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpGrammarCompiler_CompileStream_Proxy( 
    ISpGrammarCompiler * This,
    IStream *pSource,
    IStream *pDest,
    IStream *pHeader,
    IUnknown *pReserved,
    ISpErrorLog *pErrorLog,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB ISpGrammarCompiler_CompileStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpGrammarCompiler_INTERFACE_DEFINED__ */


#ifndef __ISpGramCompBackend_INTERFACE_DEFINED__
#define __ISpGramCompBackend_INTERFACE_DEFINED__

/* interface ISpGramCompBackend */
/* [restricted][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpGramCompBackend;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3DDCA27C-665C-4786-9F97-8C90C3488B61")
    ISpGramCompBackend : public ISpGrammarBuilder
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSaveObjects( 
            IStream *pStream,
            ISpErrorLog *pErrorLog) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitFromBinaryGrammar( 
            const SPBINARYGRAMMAR *pBinaryData) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpGramCompBackendVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpGramCompBackend * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpGramCompBackend * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpGramCompBackend * This);
        
        HRESULT ( STDMETHODCALLTYPE *ResetGrammar )( 
            ISpGramCompBackend * This,
            /* [in] */ WORD NewLanguage);
        
        HRESULT ( STDMETHODCALLTYPE *GetRule )( 
            ISpGramCompBackend * This,
            /* [in] */ const WCHAR *pszRuleName,
            /* [in] */ DWORD dwRuleId,
            /* [in] */ DWORD dwAttributes,
            /* [in] */ BOOL fCreateIfNotExist,
            /* [out] */ SPSTATEHANDLE *phInitialState);
        
        HRESULT ( STDMETHODCALLTYPE *ClearRule )( 
            ISpGramCompBackend * This,
            SPSTATEHANDLE hState);
        
        HRESULT ( STDMETHODCALLTYPE *CreateNewState )( 
            ISpGramCompBackend * This,
            SPSTATEHANDLE hState,
            SPSTATEHANDLE *phState);
        
        HRESULT ( STDMETHODCALLTYPE *AddWordTransition )( 
            ISpGramCompBackend * This,
            SPSTATEHANDLE hFromState,
            SPSTATEHANDLE hToState,
            const WCHAR *psz,
            const WCHAR *pszSeparators,
            SPGRAMMARWORDTYPE eWordType,
            float Weight,
            const SPPROPERTYINFO *pPropInfo);
        
        HRESULT ( STDMETHODCALLTYPE *AddRuleTransition )( 
            ISpGramCompBackend * This,
            SPSTATEHANDLE hFromState,
            SPSTATEHANDLE hToState,
            SPSTATEHANDLE hRule,
            float Weight,
            const SPPROPERTYINFO *pPropInfo);
        
        HRESULT ( STDMETHODCALLTYPE *AddResource )( 
            ISpGramCompBackend * This,
            /* [in] */ SPSTATEHANDLE hRuleState,
            /* [in] */ const WCHAR *pszResourceName,
            /* [in] */ const WCHAR *pszResourceValue);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            ISpGramCompBackend * This,
            DWORD dwReserved);
        
        HRESULT ( STDMETHODCALLTYPE *SetSaveObjects )( 
            ISpGramCompBackend * This,
            IStream *pStream,
            ISpErrorLog *pErrorLog);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromBinaryGrammar )( 
            ISpGramCompBackend * This,
            const SPBINARYGRAMMAR *pBinaryData);
        
        END_INTERFACE
    } ISpGramCompBackendVtbl;

    interface ISpGramCompBackend
    {
        CONST_VTBL struct ISpGramCompBackendVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpGramCompBackend_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpGramCompBackend_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpGramCompBackend_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpGramCompBackend_ResetGrammar(This,NewLanguage)	\
    (This)->lpVtbl -> ResetGrammar(This,NewLanguage)

#define ISpGramCompBackend_GetRule(This,pszRuleName,dwRuleId,dwAttributes,fCreateIfNotExist,phInitialState)	\
    (This)->lpVtbl -> GetRule(This,pszRuleName,dwRuleId,dwAttributes,fCreateIfNotExist,phInitialState)

#define ISpGramCompBackend_ClearRule(This,hState)	\
    (This)->lpVtbl -> ClearRule(This,hState)

#define ISpGramCompBackend_CreateNewState(This,hState,phState)	\
    (This)->lpVtbl -> CreateNewState(This,hState,phState)

#define ISpGramCompBackend_AddWordTransition(This,hFromState,hToState,psz,pszSeparators,eWordType,Weight,pPropInfo)	\
    (This)->lpVtbl -> AddWordTransition(This,hFromState,hToState,psz,pszSeparators,eWordType,Weight,pPropInfo)

#define ISpGramCompBackend_AddRuleTransition(This,hFromState,hToState,hRule,Weight,pPropInfo)	\
    (This)->lpVtbl -> AddRuleTransition(This,hFromState,hToState,hRule,Weight,pPropInfo)

#define ISpGramCompBackend_AddResource(This,hRuleState,pszResourceName,pszResourceValue)	\
    (This)->lpVtbl -> AddResource(This,hRuleState,pszResourceName,pszResourceValue)

#define ISpGramCompBackend_Commit(This,dwReserved)	\
    (This)->lpVtbl -> Commit(This,dwReserved)


#define ISpGramCompBackend_SetSaveObjects(This,pStream,pErrorLog)	\
    (This)->lpVtbl -> SetSaveObjects(This,pStream,pErrorLog)

#define ISpGramCompBackend_InitFromBinaryGrammar(This,pBinaryData)	\
    (This)->lpVtbl -> InitFromBinaryGrammar(This,pBinaryData)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpGramCompBackend_SetSaveObjects_Proxy( 
    ISpGramCompBackend * This,
    IStream *pStream,
    ISpErrorLog *pErrorLog);


void __RPC_STUB ISpGramCompBackend_SetSaveObjects_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpGramCompBackend_InitFromBinaryGrammar_Proxy( 
    ISpGramCompBackend * This,
    const SPBINARYGRAMMAR *pBinaryData);


void __RPC_STUB ISpGramCompBackend_InitFromBinaryGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpGramCompBackend_INTERFACE_DEFINED__ */


#ifndef __ISpITNProcessor_INTERFACE_DEFINED__
#define __ISpITNProcessor_INTERFACE_DEFINED__

/* interface ISpITNProcessor */
/* [restricted][local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpITNProcessor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("12D7360F-A1C9-11d3-BC90-00C04F72DF9F")
    ISpITNProcessor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadITNGrammar( 
            WCHAR *pszCLSID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ITNPhrase( 
            ISpPhraseBuilder *pPhrase) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpITNProcessorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpITNProcessor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpITNProcessor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpITNProcessor * This);
        
        HRESULT ( STDMETHODCALLTYPE *LoadITNGrammar )( 
            ISpITNProcessor * This,
            WCHAR *pszCLSID);
        
        HRESULT ( STDMETHODCALLTYPE *ITNPhrase )( 
            ISpITNProcessor * This,
            ISpPhraseBuilder *pPhrase);
        
        END_INTERFACE
    } ISpITNProcessorVtbl;

    interface ISpITNProcessor
    {
        CONST_VTBL struct ISpITNProcessorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpITNProcessor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpITNProcessor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpITNProcessor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpITNProcessor_LoadITNGrammar(This,pszCLSID)	\
    (This)->lpVtbl -> LoadITNGrammar(This,pszCLSID)

#define ISpITNProcessor_ITNPhrase(This,pPhrase)	\
    (This)->lpVtbl -> ITNPhrase(This,pPhrase)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpITNProcessor_LoadITNGrammar_Proxy( 
    ISpITNProcessor * This,
    WCHAR *pszCLSID);


void __RPC_STUB ISpITNProcessor_LoadITNGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpITNProcessor_ITNPhrase_Proxy( 
    ISpITNProcessor * This,
    ISpPhraseBuilder *pPhrase);


void __RPC_STUB ISpITNProcessor_ITNPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpITNProcessor_INTERFACE_DEFINED__ */


#ifndef __ISpPhraseBuilder_INTERFACE_DEFINED__
#define __ISpPhraseBuilder_INTERFACE_DEFINED__

/* interface ISpPhraseBuilder */
/* [restricted][unique][helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_ISpPhraseBuilder;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("88A3342A-0BED-4834-922B-88D43173162F")
    ISpPhraseBuilder : public ISpPhrase
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitFromPhrase( 
            const SPPHRASE *pPhrase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InitFromSerializedPhrase( 
            const SPSERIALIZEDPHRASE *pPhrase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddElements( 
            ULONG cElements,
            const SPPHRASEELEMENT *pElement) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddRules( 
            const SPPHRASERULEHANDLE hParent,
            const SPPHRASERULE *pRule,
            SPPHRASERULEHANDLE *phNewRule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddProperties( 
            const SPPHRASEPROPERTYHANDLE hParent,
            const SPPHRASEPROPERTY *pProperty,
            SPPHRASEPROPERTYHANDLE *phNewProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddReplacements( 
            ULONG cReplacements,
            const SPPHRASEREPLACEMENT *pReplacements) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpPhraseBuilderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpPhraseBuilder * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpPhraseBuilder * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpPhraseBuilder * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPhrase )( 
            ISpPhraseBuilder * This,
            /* [out] */ SPPHRASE **ppCoMemPhrase);
        
        HRESULT ( STDMETHODCALLTYPE *GetSerializedPhrase )( 
            ISpPhraseBuilder * This,
            /* [out] */ SPSERIALIZEDPHRASE **ppCoMemPhrase);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            ISpPhraseBuilder * This,
            /* [in] */ ULONG ulStart,
            /* [in] */ ULONG ulCount,
            /* [in] */ BOOL fUseTextReplacements,
            /* [out] */ WCHAR **ppszCoMemText,
            /* [out] */ BYTE *pbDisplayAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *Discard )( 
            ISpPhraseBuilder * This,
            /* [in] */ DWORD dwValueTypes);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromPhrase )( 
            ISpPhraseBuilder * This,
            const SPPHRASE *pPhrase);
        
        HRESULT ( STDMETHODCALLTYPE *InitFromSerializedPhrase )( 
            ISpPhraseBuilder * This,
            const SPSERIALIZEDPHRASE *pPhrase);
        
        HRESULT ( STDMETHODCALLTYPE *AddElements )( 
            ISpPhraseBuilder * This,
            ULONG cElements,
            const SPPHRASEELEMENT *pElement);
        
        HRESULT ( STDMETHODCALLTYPE *AddRules )( 
            ISpPhraseBuilder * This,
            const SPPHRASERULEHANDLE hParent,
            const SPPHRASERULE *pRule,
            SPPHRASERULEHANDLE *phNewRule);
        
        HRESULT ( STDMETHODCALLTYPE *AddProperties )( 
            ISpPhraseBuilder * This,
            const SPPHRASEPROPERTYHANDLE hParent,
            const SPPHRASEPROPERTY *pProperty,
            SPPHRASEPROPERTYHANDLE *phNewProperty);
        
        HRESULT ( STDMETHODCALLTYPE *AddReplacements )( 
            ISpPhraseBuilder * This,
            ULONG cReplacements,
            const SPPHRASEREPLACEMENT *pReplacements);
        
        END_INTERFACE
    } ISpPhraseBuilderVtbl;

    interface ISpPhraseBuilder
    {
        CONST_VTBL struct ISpPhraseBuilderVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpPhraseBuilder_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpPhraseBuilder_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpPhraseBuilder_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpPhraseBuilder_GetPhrase(This,ppCoMemPhrase)	\
    (This)->lpVtbl -> GetPhrase(This,ppCoMemPhrase)

#define ISpPhraseBuilder_GetSerializedPhrase(This,ppCoMemPhrase)	\
    (This)->lpVtbl -> GetSerializedPhrase(This,ppCoMemPhrase)

#define ISpPhraseBuilder_GetText(This,ulStart,ulCount,fUseTextReplacements,ppszCoMemText,pbDisplayAttributes)	\
    (This)->lpVtbl -> GetText(This,ulStart,ulCount,fUseTextReplacements,ppszCoMemText,pbDisplayAttributes)

#define ISpPhraseBuilder_Discard(This,dwValueTypes)	\
    (This)->lpVtbl -> Discard(This,dwValueTypes)


#define ISpPhraseBuilder_InitFromPhrase(This,pPhrase)	\
    (This)->lpVtbl -> InitFromPhrase(This,pPhrase)

#define ISpPhraseBuilder_InitFromSerializedPhrase(This,pPhrase)	\
    (This)->lpVtbl -> InitFromSerializedPhrase(This,pPhrase)

#define ISpPhraseBuilder_AddElements(This,cElements,pElement)	\
    (This)->lpVtbl -> AddElements(This,cElements,pElement)

#define ISpPhraseBuilder_AddRules(This,hParent,pRule,phNewRule)	\
    (This)->lpVtbl -> AddRules(This,hParent,pRule,phNewRule)

#define ISpPhraseBuilder_AddProperties(This,hParent,pProperty,phNewProperty)	\
    (This)->lpVtbl -> AddProperties(This,hParent,pProperty,phNewProperty)

#define ISpPhraseBuilder_AddReplacements(This,cReplacements,pReplacements)	\
    (This)->lpVtbl -> AddReplacements(This,cReplacements,pReplacements)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_InitFromPhrase_Proxy( 
    ISpPhraseBuilder * This,
    const SPPHRASE *pPhrase);


void __RPC_STUB ISpPhraseBuilder_InitFromPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_InitFromSerializedPhrase_Proxy( 
    ISpPhraseBuilder * This,
    const SPSERIALIZEDPHRASE *pPhrase);


void __RPC_STUB ISpPhraseBuilder_InitFromSerializedPhrase_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_AddElements_Proxy( 
    ISpPhraseBuilder * This,
    ULONG cElements,
    const SPPHRASEELEMENT *pElement);


void __RPC_STUB ISpPhraseBuilder_AddElements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_AddRules_Proxy( 
    ISpPhraseBuilder * This,
    const SPPHRASERULEHANDLE hParent,
    const SPPHRASERULE *pRule,
    SPPHRASERULEHANDLE *phNewRule);


void __RPC_STUB ISpPhraseBuilder_AddRules_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_AddProperties_Proxy( 
    ISpPhraseBuilder * This,
    const SPPHRASEPROPERTYHANDLE hParent,
    const SPPHRASEPROPERTY *pProperty,
    SPPHRASEPROPERTYHANDLE *phNewProperty);


void __RPC_STUB ISpPhraseBuilder_AddProperties_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpPhraseBuilder_AddReplacements_Proxy( 
    ISpPhraseBuilder * This,
    ULONG cReplacements,
    const SPPHRASEREPLACEMENT *pReplacements);


void __RPC_STUB ISpPhraseBuilder_AddReplacements_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpPhraseBuilder_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0344 */
/* [local] */ 

#if defined(__cplusplus)
interface ISpTask
{
virtual HRESULT STDMETHODCALLTYPE Execute(
                 void *pvTaskData,
                 volatile const BOOL* pfContinueProcessing) = 0;
};
#else
typedef void *ISpTask;

#endif
#if defined(__cplusplus)
interface ISpThreadTask
{
virtual HRESULT STDMETHODCALLTYPE InitThread(
                 void * pvTaskData,
                 HWND hwnd) = 0;
virtual HRESULT STDMETHODCALLTYPE ThreadProc(
                 void *pvTaskData,
                 HANDLE hExitThreadEvent,
                 HANDLE hNotifyEvent,
                 HWND hwndWorker,
                 volatile const BOOL * pfContinueProcessing) = 0;
virtual LRESULT STDMETHODCALLTYPE WindowMessage(
                 void *pvTaskData,
                 HWND hWnd,
                 UINT Msg,
                 WPARAM wParam,
                 LPARAM lParam) = 0;
};
#else
typedef void *ISpThreadTask;

#endif


extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0344_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0344_v0_0_s_ifspec;

#ifndef __ISpThreadControl_INTERFACE_DEFINED__
#define __ISpThreadControl_INTERFACE_DEFINED__

/* interface ISpThreadControl */
/* [restricted][local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpThreadControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A6BE4D73-4403-4358-B22D-0346E23B1764")
    ISpThreadControl : public ISpNotifySink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE StartThread( 
            DWORD dwFlags,
            HWND *phwnd) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WaitForThreadDone( 
            BOOL fForceStop,
            HRESULT *phrThreadResult,
            ULONG msTimeOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateThread( void) = 0;
        
        virtual HANDLE STDMETHODCALLTYPE ThreadHandle( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE ThreadId( void) = 0;
        
        virtual HANDLE STDMETHODCALLTYPE NotifyEvent( void) = 0;
        
        virtual HWND STDMETHODCALLTYPE WindowHandle( void) = 0;
        
        virtual HANDLE STDMETHODCALLTYPE ThreadCompleteEvent( void) = 0;
        
        virtual HANDLE STDMETHODCALLTYPE ExitThreadEvent( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpThreadControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpThreadControl * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpThreadControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *Notify )( 
            ISpThreadControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *StartThread )( 
            ISpThreadControl * This,
            DWORD dwFlags,
            HWND *phwnd);
        
        HRESULT ( STDMETHODCALLTYPE *WaitForThreadDone )( 
            ISpThreadControl * This,
            BOOL fForceStop,
            HRESULT *phrThreadResult,
            ULONG msTimeOut);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateThread )( 
            ISpThreadControl * This);
        
        HANDLE ( STDMETHODCALLTYPE *ThreadHandle )( 
            ISpThreadControl * This);
        
        DWORD ( STDMETHODCALLTYPE *ThreadId )( 
            ISpThreadControl * This);
        
        HANDLE ( STDMETHODCALLTYPE *NotifyEvent )( 
            ISpThreadControl * This);
        
        HWND ( STDMETHODCALLTYPE *WindowHandle )( 
            ISpThreadControl * This);
        
        HANDLE ( STDMETHODCALLTYPE *ThreadCompleteEvent )( 
            ISpThreadControl * This);
        
        HANDLE ( STDMETHODCALLTYPE *ExitThreadEvent )( 
            ISpThreadControl * This);
        
        END_INTERFACE
    } ISpThreadControlVtbl;

    interface ISpThreadControl
    {
        CONST_VTBL struct ISpThreadControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpThreadControl_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpThreadControl_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpThreadControl_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpThreadControl_Notify(This)	\
    (This)->lpVtbl -> Notify(This)


#define ISpThreadControl_StartThread(This,dwFlags,phwnd)	\
    (This)->lpVtbl -> StartThread(This,dwFlags,phwnd)

#define ISpThreadControl_WaitForThreadDone(This,fForceStop,phrThreadResult,msTimeOut)	\
    (This)->lpVtbl -> WaitForThreadDone(This,fForceStop,phrThreadResult,msTimeOut)

#define ISpThreadControl_TerminateThread(This)	\
    (This)->lpVtbl -> TerminateThread(This)

#define ISpThreadControl_ThreadHandle(This)	\
    (This)->lpVtbl -> ThreadHandle(This)

#define ISpThreadControl_ThreadId(This)	\
    (This)->lpVtbl -> ThreadId(This)

#define ISpThreadControl_NotifyEvent(This)	\
    (This)->lpVtbl -> NotifyEvent(This)

#define ISpThreadControl_WindowHandle(This)	\
    (This)->lpVtbl -> WindowHandle(This)

#define ISpThreadControl_ThreadCompleteEvent(This)	\
    (This)->lpVtbl -> ThreadCompleteEvent(This)

#define ISpThreadControl_ExitThreadEvent(This)	\
    (This)->lpVtbl -> ExitThreadEvent(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpThreadControl_StartThread_Proxy( 
    ISpThreadControl * This,
    DWORD dwFlags,
    HWND *phwnd);


void __RPC_STUB ISpThreadControl_StartThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpThreadControl_WaitForThreadDone_Proxy( 
    ISpThreadControl * This,
    BOOL fForceStop,
    HRESULT *phrThreadResult,
    ULONG msTimeOut);


void __RPC_STUB ISpThreadControl_WaitForThreadDone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpThreadControl_TerminateThread_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_TerminateThread_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HANDLE STDMETHODCALLTYPE ISpThreadControl_ThreadHandle_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_ThreadHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE ISpThreadControl_ThreadId_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_ThreadId_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HANDLE STDMETHODCALLTYPE ISpThreadControl_NotifyEvent_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_NotifyEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HWND STDMETHODCALLTYPE ISpThreadControl_WindowHandle_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_WindowHandle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HANDLE STDMETHODCALLTYPE ISpThreadControl_ThreadCompleteEvent_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_ThreadCompleteEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HANDLE STDMETHODCALLTYPE ISpThreadControl_ExitThreadEvent_Proxy( 
    ISpThreadControl * This);


void __RPC_STUB ISpThreadControl_ExitThreadEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpThreadControl_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0345 */
/* [local] */ 

typedef /* [restricted] */ struct SPTMTHREADINFO
    {
    long lPoolSize;
    long lPriority;
    ULONG ulConcurrencyLimit;
    ULONG ulMaxQuickAllocThreads;
    } 	SPTMTHREADINFO;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0345_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0345_v0_0_s_ifspec;

#ifndef __ISpTaskManager_INTERFACE_DEFINED__
#define __ISpTaskManager_INTERFACE_DEFINED__

/* interface ISpTaskManager */
/* [object][restricted][unique][helpstring][uuid][local] */ 


EXTERN_C const IID IID_ISpTaskManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2BAEEF81-2CA3-4331-98F3-26EC5ABEFB03")
    ISpTaskManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetThreadPoolInfo( 
            /* [in] */ const SPTMTHREADINFO *pPoolInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetThreadPoolInfo( 
            /* [out] */ SPTMTHREADINFO *pPoolInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueueTask( 
            /* [in] */ ISpTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ HANDLE hCompEvent,
            /* [out][in] */ DWORD *pdwGroupId,
            /* [out] */ DWORD *pTaskID) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateReoccurringTask( 
            /* [in] */ ISpTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ HANDLE hCompEvent,
            /* [out] */ ISpNotifySink **ppTaskCtrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateThreadControl( 
            /* [in] */ ISpThreadTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ long nPriority,
            /* [out] */ ISpThreadControl **ppTaskCtrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateTask( 
            /* [in] */ DWORD dwTaskId,
            /* [in] */ ULONG ulWaitPeriod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE TerminateTaskGroup( 
            /* [in] */ DWORD dwGroupId,
            /* [in] */ ULONG ulWaitPeriod) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpTaskManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpTaskManager * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpTaskManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpTaskManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetThreadPoolInfo )( 
            ISpTaskManager * This,
            /* [in] */ const SPTMTHREADINFO *pPoolInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetThreadPoolInfo )( 
            ISpTaskManager * This,
            /* [out] */ SPTMTHREADINFO *pPoolInfo);
        
        HRESULT ( STDMETHODCALLTYPE *QueueTask )( 
            ISpTaskManager * This,
            /* [in] */ ISpTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ HANDLE hCompEvent,
            /* [out][in] */ DWORD *pdwGroupId,
            /* [out] */ DWORD *pTaskID);
        
        HRESULT ( STDMETHODCALLTYPE *CreateReoccurringTask )( 
            ISpTaskManager * This,
            /* [in] */ ISpTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ HANDLE hCompEvent,
            /* [out] */ ISpNotifySink **ppTaskCtrl);
        
        HRESULT ( STDMETHODCALLTYPE *CreateThreadControl )( 
            ISpTaskManager * This,
            /* [in] */ ISpThreadTask *pTask,
            /* [in] */ void *pvTaskData,
            /* [in] */ long nPriority,
            /* [out] */ ISpThreadControl **ppTaskCtrl);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateTask )( 
            ISpTaskManager * This,
            /* [in] */ DWORD dwTaskId,
            /* [in] */ ULONG ulWaitPeriod);
        
        HRESULT ( STDMETHODCALLTYPE *TerminateTaskGroup )( 
            ISpTaskManager * This,
            /* [in] */ DWORD dwGroupId,
            /* [in] */ ULONG ulWaitPeriod);
        
        END_INTERFACE
    } ISpTaskManagerVtbl;

    interface ISpTaskManager
    {
        CONST_VTBL struct ISpTaskManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpTaskManager_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpTaskManager_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpTaskManager_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpTaskManager_SetThreadPoolInfo(This,pPoolInfo)	\
    (This)->lpVtbl -> SetThreadPoolInfo(This,pPoolInfo)

#define ISpTaskManager_GetThreadPoolInfo(This,pPoolInfo)	\
    (This)->lpVtbl -> GetThreadPoolInfo(This,pPoolInfo)

#define ISpTaskManager_QueueTask(This,pTask,pvTaskData,hCompEvent,pdwGroupId,pTaskID)	\
    (This)->lpVtbl -> QueueTask(This,pTask,pvTaskData,hCompEvent,pdwGroupId,pTaskID)

#define ISpTaskManager_CreateReoccurringTask(This,pTask,pvTaskData,hCompEvent,ppTaskCtrl)	\
    (This)->lpVtbl -> CreateReoccurringTask(This,pTask,pvTaskData,hCompEvent,ppTaskCtrl)

#define ISpTaskManager_CreateThreadControl(This,pTask,pvTaskData,nPriority,ppTaskCtrl)	\
    (This)->lpVtbl -> CreateThreadControl(This,pTask,pvTaskData,nPriority,ppTaskCtrl)

#define ISpTaskManager_TerminateTask(This,dwTaskId,ulWaitPeriod)	\
    (This)->lpVtbl -> TerminateTask(This,dwTaskId,ulWaitPeriod)

#define ISpTaskManager_TerminateTaskGroup(This,dwGroupId,ulWaitPeriod)	\
    (This)->lpVtbl -> TerminateTaskGroup(This,dwGroupId,ulWaitPeriod)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpTaskManager_SetThreadPoolInfo_Proxy( 
    ISpTaskManager * This,
    /* [in] */ const SPTMTHREADINFO *pPoolInfo);


void __RPC_STUB ISpTaskManager_SetThreadPoolInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_GetThreadPoolInfo_Proxy( 
    ISpTaskManager * This,
    /* [out] */ SPTMTHREADINFO *pPoolInfo);


void __RPC_STUB ISpTaskManager_GetThreadPoolInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_QueueTask_Proxy( 
    ISpTaskManager * This,
    /* [in] */ ISpTask *pTask,
    /* [in] */ void *pvTaskData,
    /* [in] */ HANDLE hCompEvent,
    /* [out][in] */ DWORD *pdwGroupId,
    /* [out] */ DWORD *pTaskID);


void __RPC_STUB ISpTaskManager_QueueTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_CreateReoccurringTask_Proxy( 
    ISpTaskManager * This,
    /* [in] */ ISpTask *pTask,
    /* [in] */ void *pvTaskData,
    /* [in] */ HANDLE hCompEvent,
    /* [out] */ ISpNotifySink **ppTaskCtrl);


void __RPC_STUB ISpTaskManager_CreateReoccurringTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_CreateThreadControl_Proxy( 
    ISpTaskManager * This,
    /* [in] */ ISpThreadTask *pTask,
    /* [in] */ void *pvTaskData,
    /* [in] */ long nPriority,
    /* [out] */ ISpThreadControl **ppTaskCtrl);


void __RPC_STUB ISpTaskManager_CreateThreadControl_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_TerminateTask_Proxy( 
    ISpTaskManager * This,
    /* [in] */ DWORD dwTaskId,
    /* [in] */ ULONG ulWaitPeriod);


void __RPC_STUB ISpTaskManager_TerminateTask_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTaskManager_TerminateTaskGroup_Proxy( 
    ISpTaskManager * This,
    /* [in] */ DWORD dwGroupId,
    /* [in] */ ULONG ulWaitPeriod);


void __RPC_STUB ISpTaskManager_TerminateTaskGroup_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpTaskManager_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0346 */
/* [local] */ 

typedef 
enum SPVSKIPTYPE
    {	SPVST_SENTENCE	= 1L << 0
    } 	SPVSKIPTYPE;

typedef 
enum SPVESACTIONS
    {	SPVES_CONTINUE	= 0,
	SPVES_ABORT	= 1L << 0,
	SPVES_SKIP	= 1L << 1,
	SPVES_RATE	= 1L << 2,
	SPVES_VOLUME	= 1L << 3
    } 	SPVESACTIONS;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0346_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0346_v0_0_s_ifspec;

#ifndef __ISpTTSEngineSite_INTERFACE_DEFINED__
#define __ISpTTSEngineSite_INTERFACE_DEFINED__

/* interface ISpTTSEngineSite */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_ISpTTSEngineSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9880499B-CCE9-11d2-B503-00C04F797396")
    ISpTTSEngineSite : public ISpEventSink
    {
    public:
        virtual DWORD STDMETHODCALLTYPE GetActions( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ const void *pBuff,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRate( 
            /* [out] */ long *pRateAdjust) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVolume( 
            /* [out] */ USHORT *pusVolume) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSkipInfo( 
            /* [out] */ SPVSKIPTYPE *peType,
            /* [out] */ long *plNumItems) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CompleteSkip( 
            /* [in] */ long ulNumSkipped) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpTTSEngineSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpTTSEngineSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpTTSEngineSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpTTSEngineSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddEvents )( 
            ISpTTSEngineSite * This,
            /* [in] */ const SPEVENT *pEventArray,
            /* [in] */ ULONG ulCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetEventInterest )( 
            ISpTTSEngineSite * This,
            /* [out] */ ULONGLONG *pullEventInterest);
        
        DWORD ( STDMETHODCALLTYPE *GetActions )( 
            ISpTTSEngineSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *Write )( 
            ISpTTSEngineSite * This,
            /* [in] */ const void *pBuff,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbWritten);
        
        HRESULT ( STDMETHODCALLTYPE *GetRate )( 
            ISpTTSEngineSite * This,
            /* [out] */ long *pRateAdjust);
        
        HRESULT ( STDMETHODCALLTYPE *GetVolume )( 
            ISpTTSEngineSite * This,
            /* [out] */ USHORT *pusVolume);
        
        HRESULT ( STDMETHODCALLTYPE *GetSkipInfo )( 
            ISpTTSEngineSite * This,
            /* [out] */ SPVSKIPTYPE *peType,
            /* [out] */ long *plNumItems);
        
        HRESULT ( STDMETHODCALLTYPE *CompleteSkip )( 
            ISpTTSEngineSite * This,
            /* [in] */ long ulNumSkipped);
        
        END_INTERFACE
    } ISpTTSEngineSiteVtbl;

    interface ISpTTSEngineSite
    {
        CONST_VTBL struct ISpTTSEngineSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpTTSEngineSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpTTSEngineSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpTTSEngineSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpTTSEngineSite_AddEvents(This,pEventArray,ulCount)	\
    (This)->lpVtbl -> AddEvents(This,pEventArray,ulCount)

#define ISpTTSEngineSite_GetEventInterest(This,pullEventInterest)	\
    (This)->lpVtbl -> GetEventInterest(This,pullEventInterest)


#define ISpTTSEngineSite_GetActions(This)	\
    (This)->lpVtbl -> GetActions(This)

#define ISpTTSEngineSite_Write(This,pBuff,cb,pcbWritten)	\
    (This)->lpVtbl -> Write(This,pBuff,cb,pcbWritten)

#define ISpTTSEngineSite_GetRate(This,pRateAdjust)	\
    (This)->lpVtbl -> GetRate(This,pRateAdjust)

#define ISpTTSEngineSite_GetVolume(This,pusVolume)	\
    (This)->lpVtbl -> GetVolume(This,pusVolume)

#define ISpTTSEngineSite_GetSkipInfo(This,peType,plNumItems)	\
    (This)->lpVtbl -> GetSkipInfo(This,peType,plNumItems)

#define ISpTTSEngineSite_CompleteSkip(This,ulNumSkipped)	\
    (This)->lpVtbl -> CompleteSkip(This,ulNumSkipped)

#endif /* COBJMACROS */


#endif 	/* C style interface */



DWORD STDMETHODCALLTYPE ISpTTSEngineSite_GetActions_Proxy( 
    ISpTTSEngineSite * This);


void __RPC_STUB ISpTTSEngineSite_GetActions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngineSite_Write_Proxy( 
    ISpTTSEngineSite * This,
    /* [in] */ const void *pBuff,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten);


void __RPC_STUB ISpTTSEngineSite_Write_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngineSite_GetRate_Proxy( 
    ISpTTSEngineSite * This,
    /* [out] */ long *pRateAdjust);


void __RPC_STUB ISpTTSEngineSite_GetRate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngineSite_GetVolume_Proxy( 
    ISpTTSEngineSite * This,
    /* [out] */ USHORT *pusVolume);


void __RPC_STUB ISpTTSEngineSite_GetVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngineSite_GetSkipInfo_Proxy( 
    ISpTTSEngineSite * This,
    /* [out] */ SPVSKIPTYPE *peType,
    /* [out] */ long *plNumItems);


void __RPC_STUB ISpTTSEngineSite_GetSkipInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngineSite_CompleteSkip_Proxy( 
    ISpTTSEngineSite * This,
    /* [in] */ long ulNumSkipped);


void __RPC_STUB ISpTTSEngineSite_CompleteSkip_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpTTSEngineSite_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0347 */
/* [local] */ 

typedef struct SPVTEXTFRAG
    {
    struct SPVTEXTFRAG *pNext;
    SPVSTATE State;
    LPCWSTR pTextStart;
    ULONG ulTextLen;
    ULONG ulTextSrcOffset;
    } 	SPVTEXTFRAG;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0347_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0347_v0_0_s_ifspec;

#ifndef __ISpTTSEngine_INTERFACE_DEFINED__
#define __ISpTTSEngine_INTERFACE_DEFINED__

/* interface ISpTTSEngine */
/* [unique][helpstring][uuid][local][object] */ 


EXTERN_C const IID IID_ISpTTSEngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A74D7C8E-4CC5-4f2f-A6EB-804DEE18500E")
    ISpTTSEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Speak( 
            /* [in] */ DWORD dwSpeakFlags,
            /* [in] */ REFGUID rguidFormatId,
            /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
            /* [in] */ const SPVTEXTFRAG *pTextFragList,
            /* [in] */ ISpTTSEngineSite *pOutputSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOutputFormat( 
            /* [in] */ const GUID *pTargetFmtId,
            /* [in] */ const WAVEFORMATEX *pTargetWaveFormatEx,
            /* [out] */ GUID *pOutputFormatId,
            /* [out] */ WAVEFORMATEX **ppCoMemOutputWaveFormatEx) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpTTSEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpTTSEngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpTTSEngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpTTSEngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *Speak )( 
            ISpTTSEngine * This,
            /* [in] */ DWORD dwSpeakFlags,
            /* [in] */ REFGUID rguidFormatId,
            /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
            /* [in] */ const SPVTEXTFRAG *pTextFragList,
            /* [in] */ ISpTTSEngineSite *pOutputSite);
        
        HRESULT ( STDMETHODCALLTYPE *GetOutputFormat )( 
            ISpTTSEngine * This,
            /* [in] */ const GUID *pTargetFmtId,
            /* [in] */ const WAVEFORMATEX *pTargetWaveFormatEx,
            /* [out] */ GUID *pOutputFormatId,
            /* [out] */ WAVEFORMATEX **ppCoMemOutputWaveFormatEx);
        
        END_INTERFACE
    } ISpTTSEngineVtbl;

    interface ISpTTSEngine
    {
        CONST_VTBL struct ISpTTSEngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpTTSEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpTTSEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpTTSEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpTTSEngine_Speak(This,dwSpeakFlags,rguidFormatId,pWaveFormatEx,pTextFragList,pOutputSite)	\
    (This)->lpVtbl -> Speak(This,dwSpeakFlags,rguidFormatId,pWaveFormatEx,pTextFragList,pOutputSite)

#define ISpTTSEngine_GetOutputFormat(This,pTargetFmtId,pTargetWaveFormatEx,pOutputFormatId,ppCoMemOutputWaveFormatEx)	\
    (This)->lpVtbl -> GetOutputFormat(This,pTargetFmtId,pTargetWaveFormatEx,pOutputFormatId,ppCoMemOutputWaveFormatEx)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpTTSEngine_Speak_Proxy( 
    ISpTTSEngine * This,
    /* [in] */ DWORD dwSpeakFlags,
    /* [in] */ REFGUID rguidFormatId,
    /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
    /* [in] */ const SPVTEXTFRAG *pTextFragList,
    /* [in] */ ISpTTSEngineSite *pOutputSite);


void __RPC_STUB ISpTTSEngine_Speak_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpTTSEngine_GetOutputFormat_Proxy( 
    ISpTTSEngine * This,
    /* [in] */ const GUID *pTargetFmtId,
    /* [in] */ const WAVEFORMATEX *pTargetWaveFormatEx,
    /* [out] */ GUID *pOutputFormatId,
    /* [out] */ WAVEFORMATEX **ppCoMemOutputWaveFormatEx);


void __RPC_STUB ISpTTSEngine_GetOutputFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpTTSEngine_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0348 */
/* [local] */ 

typedef /* [restricted] */ struct SPWORDENTRY
    {
    SPWORDHANDLE hWord;
    WORD LangID;
    WCHAR *pszDisplayText;
    WCHAR *pszLexicalForm;
    SPPHONEID *aPhoneId;
    void *pvClientContext;
    } 	SPWORDENTRY;

typedef /* [restricted] */ struct SPRULEENTRY
    {
    SPRULEHANDLE hRule;
    SPSTATEHANDLE hInitialState;
    DWORD Attributes;
    void *pvClientRuleContext;
    void *pvClientGrammarContext;
    } 	SPRULEENTRY;

typedef 
enum SPTRANSITIONTYPE
    {	SPTRANSEPSILON	= 0,
	SPTRANSWORD	= SPTRANSEPSILON + 1,
	SPTRANSRULE	= SPTRANSWORD + 1,
	SPTRANSTEXTBUF	= SPTRANSRULE + 1,
	SPTRANSWILDCARD	= SPTRANSTEXTBUF + 1,
	SPTRANSDICTATION	= SPTRANSWILDCARD + 1
    } 	SPTRANSITIONTYPE;

typedef /* [restricted] */ struct SPTRANSITIONENTRY
    {
    SPTRANSITIONID ID;
    SPSTATEHANDLE hNextState;
    BYTE Type;
    char RequiredConfidence;
    struct 
        {
        DWORD fHasProperty;
        } 	;
    float Weight;
    union 
        {
        struct 
            {
            SPSTATEHANDLE hRuleInitialState;
            SPRULEHANDLE hRule;
            void *pvClientRuleContext;
            } 	;
        struct 
            {
            SPWORDHANDLE hWord;
            void *pvClientWordContext;
            } 	;
        struct 
            {
            void *pvGrammarCookie;
            } 	;
        } 	;
    } 	SPTRANSITIONENTRY;

typedef /* [restricted] */ struct SPTRANSITIONPROPERTY
    {
    const WCHAR *pszName;
    ULONG ulId;
    const WCHAR *pszValue;
    VARIANT vValue;
    } 	SPTRANSITIONPROPERTY;

typedef /* [restricted] */ struct SPSTATEINFO
    {
    ULONG cAllocatedEntries;
    SPTRANSITIONENTRY *pTransitions;
    ULONG cEpsilons;
    ULONG cRules;
    ULONG cWords;
    ULONG cSpecialTransitions;
    } 	SPSTATEINFO;

typedef /* [restricted] */ struct SPPATHENTRY
    {
    SPTRANSITIONID hTransition;
    SPPHRASEELEMENT elem;
    } 	SPPATHENTRY;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0348_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0348_v0_0_s_ifspec;

#ifndef __ISpCFGInterpreterSite_INTERFACE_DEFINED__
#define __ISpCFGInterpreterSite_INTERFACE_DEFINED__

/* interface ISpCFGInterpreterSite */
/* [restricted][local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpCFGInterpreterSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6A6FFAD8-78B6-473d-B844-98152E4FB16B")
    ISpCFGInterpreterSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddTextReplacement( 
            /* [in] */ SPPHRASEREPLACEMENT *pReplace) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddProperty( 
            /* [in] */ const SPPHRASEPROPERTY *pProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceValue( 
            /* [in] */ const WCHAR *pszResourceName,
            /* [out] */ WCHAR **ppCoMemResource) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpCFGInterpreterSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpCFGInterpreterSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpCFGInterpreterSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpCFGInterpreterSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddTextReplacement )( 
            ISpCFGInterpreterSite * This,
            /* [in] */ SPPHRASEREPLACEMENT *pReplace);
        
        HRESULT ( STDMETHODCALLTYPE *AddProperty )( 
            ISpCFGInterpreterSite * This,
            /* [in] */ const SPPHRASEPROPERTY *pProperty);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceValue )( 
            ISpCFGInterpreterSite * This,
            /* [in] */ const WCHAR *pszResourceName,
            /* [out] */ WCHAR **ppCoMemResource);
        
        END_INTERFACE
    } ISpCFGInterpreterSiteVtbl;

    interface ISpCFGInterpreterSite
    {
        CONST_VTBL struct ISpCFGInterpreterSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpCFGInterpreterSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpCFGInterpreterSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpCFGInterpreterSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpCFGInterpreterSite_AddTextReplacement(This,pReplace)	\
    (This)->lpVtbl -> AddTextReplacement(This,pReplace)

#define ISpCFGInterpreterSite_AddProperty(This,pProperty)	\
    (This)->lpVtbl -> AddProperty(This,pProperty)

#define ISpCFGInterpreterSite_GetResourceValue(This,pszResourceName,ppCoMemResource)	\
    (This)->lpVtbl -> GetResourceValue(This,pszResourceName,ppCoMemResource)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpCFGInterpreterSite_AddTextReplacement_Proxy( 
    ISpCFGInterpreterSite * This,
    /* [in] */ SPPHRASEREPLACEMENT *pReplace);


void __RPC_STUB ISpCFGInterpreterSite_AddTextReplacement_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCFGInterpreterSite_AddProperty_Proxy( 
    ISpCFGInterpreterSite * This,
    /* [in] */ const SPPHRASEPROPERTY *pProperty);


void __RPC_STUB ISpCFGInterpreterSite_AddProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCFGInterpreterSite_GetResourceValue_Proxy( 
    ISpCFGInterpreterSite * This,
    /* [in] */ const WCHAR *pszResourceName,
    /* [out] */ WCHAR **ppCoMemResource);


void __RPC_STUB ISpCFGInterpreterSite_GetResourceValue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpCFGInterpreterSite_INTERFACE_DEFINED__ */


#ifndef __ISpCFGInterpreter_INTERFACE_DEFINED__
#define __ISpCFGInterpreter_INTERFACE_DEFINED__

/* interface ISpCFGInterpreter */
/* [restricted][local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpCFGInterpreter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F3D3F926-11FC-11d3-BB97-00C04F8EE6C0")
    ISpCFGInterpreter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitGrammar( 
            /* [in] */ const WCHAR *pszGrammarName,
            /* [in] */ const void **pvGrammarData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Interpret( 
            /* [in] */ ISpPhraseBuilder *pPhrase,
            /* [in] */ const ULONG ulFirstElement,
            /* [in] */ const ULONG ulCountOfElements,
            /* [in] */ ISpCFGInterpreterSite *pSite) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpCFGInterpreterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpCFGInterpreter * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpCFGInterpreter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpCFGInterpreter * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitGrammar )( 
            ISpCFGInterpreter * This,
            /* [in] */ const WCHAR *pszGrammarName,
            /* [in] */ const void **pvGrammarData);
        
        HRESULT ( STDMETHODCALLTYPE *Interpret )( 
            ISpCFGInterpreter * This,
            /* [in] */ ISpPhraseBuilder *pPhrase,
            /* [in] */ const ULONG ulFirstElement,
            /* [in] */ const ULONG ulCountOfElements,
            /* [in] */ ISpCFGInterpreterSite *pSite);
        
        END_INTERFACE
    } ISpCFGInterpreterVtbl;

    interface ISpCFGInterpreter
    {
        CONST_VTBL struct ISpCFGInterpreterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpCFGInterpreter_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpCFGInterpreter_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpCFGInterpreter_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpCFGInterpreter_InitGrammar(This,pszGrammarName,pvGrammarData)	\
    (This)->lpVtbl -> InitGrammar(This,pszGrammarName,pvGrammarData)

#define ISpCFGInterpreter_Interpret(This,pPhrase,ulFirstElement,ulCountOfElements,pSite)	\
    (This)->lpVtbl -> Interpret(This,pPhrase,ulFirstElement,ulCountOfElements,pSite)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpCFGInterpreter_InitGrammar_Proxy( 
    ISpCFGInterpreter * This,
    /* [in] */ const WCHAR *pszGrammarName,
    /* [in] */ const void **pvGrammarData);


void __RPC_STUB ISpCFGInterpreter_InitGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpCFGInterpreter_Interpret_Proxy( 
    ISpCFGInterpreter * This,
    /* [in] */ ISpPhraseBuilder *pPhrase,
    /* [in] */ const ULONG ulFirstElement,
    /* [in] */ const ULONG ulCountOfElements,
    /* [in] */ ISpCFGInterpreterSite *pSite);


void __RPC_STUB ISpCFGInterpreter_Interpret_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpCFGInterpreter_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0350 */
/* [local] */ 

typedef 
enum SPCFGNOTIFY
    {	SPCFGN_ADD	= 0,
	SPCFGN_REMOVE	= SPCFGN_ADD + 1,
	SPCFGN_INVALIDATE	= SPCFGN_REMOVE + 1,
	SPCFGN_ACTIVATE	= SPCFGN_INVALIDATE + 1,
	SPCFGN_DEACTIVATE	= SPCFGN_ACTIVATE + 1
    } 	SPCFGNOTIFY;

typedef 
enum SPRESULTTYPE
    {	SPRT_CFG	= 0,
	SPRT_SLM	= 1,
	SPRT_PROPRIETARY	= 2,
	SPRT_FALSE_RECOGNITION	= 1L << 2
    } 	SPRESULTTYPE;

typedef struct tagSPPHRASEALT
    {
    ISpPhraseBuilder *pPhrase;
    ULONG ulStartElementInParent;
    ULONG cElementsInParent;
    ULONG cElementsInAlternate;
    void *pvAltExtra;
    ULONG cbAltExtra;
    } 	SPPHRASEALT;

typedef struct SPRECORESULTINFO
    {
    ULONG cbSize;
    SPRESULTTYPE eResultType;
    BOOL fHypothesis;
    BOOL fProprietaryAutoPause;
    ULONGLONG ullStreamPosStart;
    ULONGLONG ullStreamPosEnd;
    SPGRAMMARHANDLE hGrammar;
    ULONG ulSizeEngineData;
    void *pvEngineData;
    ISpPhraseBuilder *pPhrase;
    SPPHRASEALT *aPhraseAlts;
    ULONG ulNumAlts;
    } 	SPRECORESULTINFO;

typedef 
enum SPWORDINFOOPT
    {	SPWIO_NONE	= 0,
	SPWIO_WANT_TEXT	= 1
    } 	SPWORDINFOOPT;

typedef 
enum SPRULEINFOOPT
    {	SPRIO_NONE	= 0
    } 	SPRULEINFOOPT;

typedef struct SPPARSEINFO
    {
    ULONG cbSize;
    SPRULEHANDLE hRule;
    ULONGLONG ullAudioStreamPosition;
    ULONG ulAudioSize;
    ULONG cTransitions;
    SPPATHENTRY *pPath;
    GUID SREngineID;
    ULONG ulSREnginePrivateDataSize;
    const BYTE *pSREnginePrivateData;
    BOOL fHypothesis;
    } 	SPPARSEINFO;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0350_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0350_v0_0_s_ifspec;

#ifndef __ISpSREngineSite_INTERFACE_DEFINED__
#define __ISpSREngineSite_INTERFACE_DEFINED__

/* interface ISpSREngineSite */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpSREngineSite;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3B414AEC-720C-4883-B9EF-178CD394FB3A")
    ISpSREngineSite : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Read( 
            /* [in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DataAvailable( 
            ULONG *pcb) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBufferNotifySize( 
            /* [in] */ ULONG cbSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ParseFromTransitions( 
            /* [in] */ const SPPARSEINFO *pParseInfo,
            /* [out] */ ISpPhraseBuilder **ppNewPhrase) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Recognition( 
            /* [in] */ const SPRECORESULTINFO *pResultInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddEvent( 
            /* [in] */ const SPEVENT *pEvent,
            /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Synchronize( 
            /* [in] */ ULONGLONG ullProcessedThruPos) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWordInfo( 
            /* [out][in] */ SPWORDENTRY *pWordEntry,
            /* [in] */ SPWORDINFOOPT Options) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWordClientContext( 
            SPWORDHANDLE hWord,
            void *pvClientContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRuleInfo( 
            /* [out][in] */ SPRULEENTRY *pRuleEntry,
            /* [in] */ SPRULEINFOOPT Options) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRuleClientContext( 
            SPRULEHANDLE hRule,
            void *pvClientContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetStateInfo( 
            SPSTATEHANDLE hState,
            SPSTATEINFO *pStateInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResource( 
            /* [in] */ SPRULEHANDLE hRule,
            /* [in] */ const WCHAR *pszResourceName,
            /* [out] */ WCHAR **ppCoMemResource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTransitionProperty( 
            /* [in] */ SPTRANSITIONID ID,
            /* [out] */ SPTRANSITIONPROPERTY **ppCoMemProperty) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsAlternate( 
            /* [in] */ SPRULEHANDLE hRule,
            /* [in] */ SPRULEHANDLE hAltRule) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxAlternates( 
            /* [in] */ SPRULEHANDLE hRule,
            /* [out] */ ULONG *pulNumAlts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetContextMaxAlternates( 
            /* [in] */ SPRECOCONTEXTHANDLE hContext,
            /* [out] */ ULONG *pulNumAlts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UpdateRecoPos( 
            /* [in] */ ULONGLONG ullCurrentRecoPos) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpSREngineSiteVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpSREngineSite * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpSREngineSite * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpSREngineSite * This);
        
        HRESULT ( STDMETHODCALLTYPE *Read )( 
            ISpSREngineSite * This,
            /* [in] */ void *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG *pcbRead);
        
        HRESULT ( STDMETHODCALLTYPE *DataAvailable )( 
            ISpSREngineSite * This,
            ULONG *pcb);
        
        HRESULT ( STDMETHODCALLTYPE *SetBufferNotifySize )( 
            ISpSREngineSite * This,
            /* [in] */ ULONG cbSize);
        
        HRESULT ( STDMETHODCALLTYPE *ParseFromTransitions )( 
            ISpSREngineSite * This,
            /* [in] */ const SPPARSEINFO *pParseInfo,
            /* [out] */ ISpPhraseBuilder **ppNewPhrase);
        
        HRESULT ( STDMETHODCALLTYPE *Recognition )( 
            ISpSREngineSite * This,
            /* [in] */ const SPRECORESULTINFO *pResultInfo);
        
        HRESULT ( STDMETHODCALLTYPE *AddEvent )( 
            ISpSREngineSite * This,
            /* [in] */ const SPEVENT *pEvent,
            /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext);
        
        HRESULT ( STDMETHODCALLTYPE *Synchronize )( 
            ISpSREngineSite * This,
            /* [in] */ ULONGLONG ullProcessedThruPos);
        
        HRESULT ( STDMETHODCALLTYPE *GetWordInfo )( 
            ISpSREngineSite * This,
            /* [out][in] */ SPWORDENTRY *pWordEntry,
            /* [in] */ SPWORDINFOOPT Options);
        
        HRESULT ( STDMETHODCALLTYPE *SetWordClientContext )( 
            ISpSREngineSite * This,
            SPWORDHANDLE hWord,
            void *pvClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetRuleInfo )( 
            ISpSREngineSite * This,
            /* [out][in] */ SPRULEENTRY *pRuleEntry,
            /* [in] */ SPRULEINFOOPT Options);
        
        HRESULT ( STDMETHODCALLTYPE *SetRuleClientContext )( 
            ISpSREngineSite * This,
            SPRULEHANDLE hRule,
            void *pvClientContext);
        
        HRESULT ( STDMETHODCALLTYPE *GetStateInfo )( 
            ISpSREngineSite * This,
            SPSTATEHANDLE hState,
            SPSTATEINFO *pStateInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetResource )( 
            ISpSREngineSite * This,
            /* [in] */ SPRULEHANDLE hRule,
            /* [in] */ const WCHAR *pszResourceName,
            /* [out] */ WCHAR **ppCoMemResource);
        
        HRESULT ( STDMETHODCALLTYPE *GetTransitionProperty )( 
            ISpSREngineSite * This,
            /* [in] */ SPTRANSITIONID ID,
            /* [out] */ SPTRANSITIONPROPERTY **ppCoMemProperty);
        
        HRESULT ( STDMETHODCALLTYPE *IsAlternate )( 
            ISpSREngineSite * This,
            /* [in] */ SPRULEHANDLE hRule,
            /* [in] */ SPRULEHANDLE hAltRule);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxAlternates )( 
            ISpSREngineSite * This,
            /* [in] */ SPRULEHANDLE hRule,
            /* [out] */ ULONG *pulNumAlts);
        
        HRESULT ( STDMETHODCALLTYPE *GetContextMaxAlternates )( 
            ISpSREngineSite * This,
            /* [in] */ SPRECOCONTEXTHANDLE hContext,
            /* [out] */ ULONG *pulNumAlts);
        
        HRESULT ( STDMETHODCALLTYPE *UpdateRecoPos )( 
            ISpSREngineSite * This,
            /* [in] */ ULONGLONG ullCurrentRecoPos);
        
        END_INTERFACE
    } ISpSREngineSiteVtbl;

    interface ISpSREngineSite
    {
        CONST_VTBL struct ISpSREngineSiteVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpSREngineSite_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpSREngineSite_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpSREngineSite_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpSREngineSite_Read(This,pv,cb,pcbRead)	\
    (This)->lpVtbl -> Read(This,pv,cb,pcbRead)

#define ISpSREngineSite_DataAvailable(This,pcb)	\
    (This)->lpVtbl -> DataAvailable(This,pcb)

#define ISpSREngineSite_SetBufferNotifySize(This,cbSize)	\
    (This)->lpVtbl -> SetBufferNotifySize(This,cbSize)

#define ISpSREngineSite_ParseFromTransitions(This,pParseInfo,ppNewPhrase)	\
    (This)->lpVtbl -> ParseFromTransitions(This,pParseInfo,ppNewPhrase)

#define ISpSREngineSite_Recognition(This,pResultInfo)	\
    (This)->lpVtbl -> Recognition(This,pResultInfo)

#define ISpSREngineSite_AddEvent(This,pEvent,hSAPIRecoContext)	\
    (This)->lpVtbl -> AddEvent(This,pEvent,hSAPIRecoContext)

#define ISpSREngineSite_Synchronize(This,ullProcessedThruPos)	\
    (This)->lpVtbl -> Synchronize(This,ullProcessedThruPos)

#define ISpSREngineSite_GetWordInfo(This,pWordEntry,Options)	\
    (This)->lpVtbl -> GetWordInfo(This,pWordEntry,Options)

#define ISpSREngineSite_SetWordClientContext(This,hWord,pvClientContext)	\
    (This)->lpVtbl -> SetWordClientContext(This,hWord,pvClientContext)

#define ISpSREngineSite_GetRuleInfo(This,pRuleEntry,Options)	\
    (This)->lpVtbl -> GetRuleInfo(This,pRuleEntry,Options)

#define ISpSREngineSite_SetRuleClientContext(This,hRule,pvClientContext)	\
    (This)->lpVtbl -> SetRuleClientContext(This,hRule,pvClientContext)

#define ISpSREngineSite_GetStateInfo(This,hState,pStateInfo)	\
    (This)->lpVtbl -> GetStateInfo(This,hState,pStateInfo)

#define ISpSREngineSite_GetResource(This,hRule,pszResourceName,ppCoMemResource)	\
    (This)->lpVtbl -> GetResource(This,hRule,pszResourceName,ppCoMemResource)

#define ISpSREngineSite_GetTransitionProperty(This,ID,ppCoMemProperty)	\
    (This)->lpVtbl -> GetTransitionProperty(This,ID,ppCoMemProperty)

#define ISpSREngineSite_IsAlternate(This,hRule,hAltRule)	\
    (This)->lpVtbl -> IsAlternate(This,hRule,hAltRule)

#define ISpSREngineSite_GetMaxAlternates(This,hRule,pulNumAlts)	\
    (This)->lpVtbl -> GetMaxAlternates(This,hRule,pulNumAlts)

#define ISpSREngineSite_GetContextMaxAlternates(This,hContext,pulNumAlts)	\
    (This)->lpVtbl -> GetContextMaxAlternates(This,hContext,pulNumAlts)

#define ISpSREngineSite_UpdateRecoPos(This,ullCurrentRecoPos)	\
    (This)->lpVtbl -> UpdateRecoPos(This,ullCurrentRecoPos)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpSREngineSite_Read_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead);


void __RPC_STUB ISpSREngineSite_Read_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_DataAvailable_Proxy( 
    ISpSREngineSite * This,
    ULONG *pcb);


void __RPC_STUB ISpSREngineSite_DataAvailable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_SetBufferNotifySize_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ ULONG cbSize);


void __RPC_STUB ISpSREngineSite_SetBufferNotifySize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_ParseFromTransitions_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ const SPPARSEINFO *pParseInfo,
    /* [out] */ ISpPhraseBuilder **ppNewPhrase);


void __RPC_STUB ISpSREngineSite_ParseFromTransitions_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_Recognition_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ const SPRECORESULTINFO *pResultInfo);


void __RPC_STUB ISpSREngineSite_Recognition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_AddEvent_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ const SPEVENT *pEvent,
    /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext);


void __RPC_STUB ISpSREngineSite_AddEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_Synchronize_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ ULONGLONG ullProcessedThruPos);


void __RPC_STUB ISpSREngineSite_Synchronize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetWordInfo_Proxy( 
    ISpSREngineSite * This,
    /* [out][in] */ SPWORDENTRY *pWordEntry,
    /* [in] */ SPWORDINFOOPT Options);


void __RPC_STUB ISpSREngineSite_GetWordInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_SetWordClientContext_Proxy( 
    ISpSREngineSite * This,
    SPWORDHANDLE hWord,
    void *pvClientContext);


void __RPC_STUB ISpSREngineSite_SetWordClientContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetRuleInfo_Proxy( 
    ISpSREngineSite * This,
    /* [out][in] */ SPRULEENTRY *pRuleEntry,
    /* [in] */ SPRULEINFOOPT Options);


void __RPC_STUB ISpSREngineSite_GetRuleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_SetRuleClientContext_Proxy( 
    ISpSREngineSite * This,
    SPRULEHANDLE hRule,
    void *pvClientContext);


void __RPC_STUB ISpSREngineSite_SetRuleClientContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetStateInfo_Proxy( 
    ISpSREngineSite * This,
    SPSTATEHANDLE hState,
    SPSTATEINFO *pStateInfo);


void __RPC_STUB ISpSREngineSite_GetStateInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetResource_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ SPRULEHANDLE hRule,
    /* [in] */ const WCHAR *pszResourceName,
    /* [out] */ WCHAR **ppCoMemResource);


void __RPC_STUB ISpSREngineSite_GetResource_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetTransitionProperty_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ SPTRANSITIONID ID,
    /* [out] */ SPTRANSITIONPROPERTY **ppCoMemProperty);


void __RPC_STUB ISpSREngineSite_GetTransitionProperty_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_IsAlternate_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ SPRULEHANDLE hRule,
    /* [in] */ SPRULEHANDLE hAltRule);


void __RPC_STUB ISpSREngineSite_IsAlternate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetMaxAlternates_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ SPRULEHANDLE hRule,
    /* [out] */ ULONG *pulNumAlts);


void __RPC_STUB ISpSREngineSite_GetMaxAlternates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_GetContextMaxAlternates_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ SPRECOCONTEXTHANDLE hContext,
    /* [out] */ ULONG *pulNumAlts);


void __RPC_STUB ISpSREngineSite_GetContextMaxAlternates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngineSite_UpdateRecoPos_Proxy( 
    ISpSREngineSite * This,
    /* [in] */ ULONGLONG ullCurrentRecoPos);


void __RPC_STUB ISpSREngineSite_UpdateRecoPos_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpSREngineSite_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0351 */
/* [local] */ 

typedef 
enum SPPROPSRC
    {	SPPROPSRC_RECO_INST	= 0,
	SPPROPSRC_RECO_CTX	= SPPROPSRC_RECO_INST + 1,
	SPPROPSRC_RECO_GRAMMAR	= SPPROPSRC_RECO_CTX + 1
    } 	SPPROPSRC;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0351_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0351_v0_0_s_ifspec;

#ifndef __ISpSREngine_INTERFACE_DEFINED__
#define __ISpSREngine_INTERFACE_DEFINED__

/* interface ISpSREngine */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpSREngine;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2F472991-854B-4465-B613-FBAFB3AD8ED8")
    ISpSREngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetSite( 
            /* [in] */ ISpSREngineSite *pSite) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInputAudioFormat( 
            /* [in] */ const GUID *pguidSourceFormatId,
            /* [in] */ const WAVEFORMATEX *pSourceWaveFormatEx,
            /* [out] */ GUID *pguidDesiredFormatId,
            /* [out] */ WAVEFORMATEX **ppCoMemDesiredWaveFormatEx) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RecognizeStream( 
            /* [in] */ REFGUID rguidFmtId,
            /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
            /* [in] */ HANDLE hRequestSync,
            /* [in] */ HANDLE hDataAvailable,
            /* [in] */ HANDLE hExit,
            /* [in] */ BOOL fNewAudioStream,
            /* [in] */ BOOL fRealTimeAudio,
            /* [in] */ ISpObjectToken *pAudioObjectToken) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRecoProfile( 
            ISpObjectToken *pProfile) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnCreateGrammar( 
            /* [in] */ void *pvEngineRecoContext,
            /* [in] */ SPGRAMMARHANDLE hSAPIGrammar,
            /* [out] */ void **ppvEngineGrammarContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDeleteGrammar( 
            /* [in] */ void *pvEngineGrammar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadProprietaryGrammar( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ REFGUID rguidParam,
            /* [string][in] */ const WCHAR *pszStringParam,
            /* [in] */ const void *pvDataParam,
            /* [in] */ ULONG ulDataSize,
            /* [in] */ SPLOADOPTIONS Options) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadProprietaryGrammar( 
            /* [in] */ void *pvEngineGrammar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProprietaryRuleState( 
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszName,
            /* [in] */ void *pReserved,
            /* [in] */ SPRULESTATE NewState,
            /* [out] */ ULONG *pcRulesChanged) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetProprietaryRuleIdState( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ DWORD dwRuleId,
            /* [in] */ SPRULESTATE NewState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadSLM( 
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszTopicName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnloadSLM( 
            /* [in] */ void *pvEngineGrammar) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSLMState( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ SPRULESTATE NewState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWordSequenceData( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ const WCHAR *pText,
            /* [in] */ ULONG cchText,
            /* [in] */ const SPTEXTSELECTIONINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTextSelection( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ const SPTEXTSELECTIONINFO *pInfo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPronounceable( 
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszWord,
            /* [out] */ SPWORDPRONOUNCEABLE *pWordPronounceable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnCreateRecoContext( 
            /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext,
            /* [out] */ void **ppvEngineContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnDeleteRecoContext( 
            /* [in] */ void *pvEngineContext) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrivateCall( 
            /* [in] */ void *pvEngineContext,
            /* [out][in] */ PVOID pCallFrame,
            /* [in] */ ULONG ulCallFrameSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAdaptationData( 
            /* [in] */ void *pvEngineContext,
            const WCHAR *pAdaptationData,
            const ULONG cch) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPropertyNum( 
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [in] */ LONG lValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyNum( 
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [out] */ LONG *lValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPropertyString( 
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [in] */ const WCHAR *pValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPropertyString( 
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [out] */ WCHAR **ppCoMemValue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetGrammarState( 
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ SPGRAMMARSTATE eGrammarState) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WordNotify( 
            SPCFGNOTIFY Action,
            ULONG cWords,
            const SPWORDENTRY *pWords) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RuleNotify( 
            SPCFGNOTIFY Action,
            ULONG cRules,
            const SPRULEENTRY *pRules) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PrivateCallEx( 
            /* [in] */ void *pvEngineContext,
            /* [in] */ const void *pInCallFrame,
            /* [in] */ ULONG ulInCallFrameSize,
            /* [out] */ void **ppvCoMemResponse,
            /* [out] */ ULONG *pulResponseSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetContextState( 
            /* [in] */ void *pvEngineContext,
            /* [in] */ SPCONTEXTSTATE eContextState) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpSREngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpSREngine * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpSREngine * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpSREngine * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetSite )( 
            ISpSREngine * This,
            /* [in] */ ISpSREngineSite *pSite);
        
        HRESULT ( STDMETHODCALLTYPE *GetInputAudioFormat )( 
            ISpSREngine * This,
            /* [in] */ const GUID *pguidSourceFormatId,
            /* [in] */ const WAVEFORMATEX *pSourceWaveFormatEx,
            /* [out] */ GUID *pguidDesiredFormatId,
            /* [out] */ WAVEFORMATEX **ppCoMemDesiredWaveFormatEx);
        
        HRESULT ( STDMETHODCALLTYPE *RecognizeStream )( 
            ISpSREngine * This,
            /* [in] */ REFGUID rguidFmtId,
            /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
            /* [in] */ HANDLE hRequestSync,
            /* [in] */ HANDLE hDataAvailable,
            /* [in] */ HANDLE hExit,
            /* [in] */ BOOL fNewAudioStream,
            /* [in] */ BOOL fRealTimeAudio,
            /* [in] */ ISpObjectToken *pAudioObjectToken);
        
        HRESULT ( STDMETHODCALLTYPE *SetRecoProfile )( 
            ISpSREngine * This,
            ISpObjectToken *pProfile);
        
        HRESULT ( STDMETHODCALLTYPE *OnCreateGrammar )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineRecoContext,
            /* [in] */ SPGRAMMARHANDLE hSAPIGrammar,
            /* [out] */ void **ppvEngineGrammarContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnDeleteGrammar )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar);
        
        HRESULT ( STDMETHODCALLTYPE *LoadProprietaryGrammar )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ REFGUID rguidParam,
            /* [string][in] */ const WCHAR *pszStringParam,
            /* [in] */ const void *pvDataParam,
            /* [in] */ ULONG ulDataSize,
            /* [in] */ SPLOADOPTIONS Options);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadProprietaryGrammar )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar);
        
        HRESULT ( STDMETHODCALLTYPE *SetProprietaryRuleState )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszName,
            /* [in] */ void *pReserved,
            /* [in] */ SPRULESTATE NewState,
            /* [out] */ ULONG *pcRulesChanged);
        
        HRESULT ( STDMETHODCALLTYPE *SetProprietaryRuleIdState )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ DWORD dwRuleId,
            /* [in] */ SPRULESTATE NewState);
        
        HRESULT ( STDMETHODCALLTYPE *LoadSLM )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszTopicName);
        
        HRESULT ( STDMETHODCALLTYPE *UnloadSLM )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar);
        
        HRESULT ( STDMETHODCALLTYPE *SetSLMState )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ SPRULESTATE NewState);
        
        HRESULT ( STDMETHODCALLTYPE *SetWordSequenceData )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ const WCHAR *pText,
            /* [in] */ ULONG cchText,
            /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *SetTextSelection )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
        
        HRESULT ( STDMETHODCALLTYPE *IsPronounceable )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [string][in] */ const WCHAR *pszWord,
            /* [out] */ SPWORDPRONOUNCEABLE *pWordPronounceable);
        
        HRESULT ( STDMETHODCALLTYPE *OnCreateRecoContext )( 
            ISpSREngine * This,
            /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext,
            /* [out] */ void **ppvEngineContext);
        
        HRESULT ( STDMETHODCALLTYPE *OnDeleteRecoContext )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineContext);
        
        HRESULT ( STDMETHODCALLTYPE *PrivateCall )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineContext,
            /* [out][in] */ PVOID pCallFrame,
            /* [in] */ ULONG ulCallFrameSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetAdaptationData )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineContext,
            const WCHAR *pAdaptationData,
            const ULONG cch);
        
        HRESULT ( STDMETHODCALLTYPE *SetPropertyNum )( 
            ISpSREngine * This,
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [in] */ LONG lValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyNum )( 
            ISpSREngine * This,
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [out] */ LONG *lValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetPropertyString )( 
            ISpSREngine * This,
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [in] */ const WCHAR *pValue);
        
        HRESULT ( STDMETHODCALLTYPE *GetPropertyString )( 
            ISpSREngine * This,
            /* [in] */ SPPROPSRC eSrc,
            /* [in] */ void *pvSrcObj,
            /* [in] */ const WCHAR *pName,
            /* [out] */ WCHAR **ppCoMemValue);
        
        HRESULT ( STDMETHODCALLTYPE *SetGrammarState )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineGrammar,
            /* [in] */ SPGRAMMARSTATE eGrammarState);
        
        HRESULT ( STDMETHODCALLTYPE *WordNotify )( 
            ISpSREngine * This,
            SPCFGNOTIFY Action,
            ULONG cWords,
            const SPWORDENTRY *pWords);
        
        HRESULT ( STDMETHODCALLTYPE *RuleNotify )( 
            ISpSREngine * This,
            SPCFGNOTIFY Action,
            ULONG cRules,
            const SPRULEENTRY *pRules);
        
        HRESULT ( STDMETHODCALLTYPE *PrivateCallEx )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineContext,
            /* [in] */ const void *pInCallFrame,
            /* [in] */ ULONG ulInCallFrameSize,
            /* [out] */ void **ppvCoMemResponse,
            /* [out] */ ULONG *pulResponseSize);
        
        HRESULT ( STDMETHODCALLTYPE *SetContextState )( 
            ISpSREngine * This,
            /* [in] */ void *pvEngineContext,
            /* [in] */ SPCONTEXTSTATE eContextState);
        
        END_INTERFACE
    } ISpSREngineVtbl;

    interface ISpSREngine
    {
        CONST_VTBL struct ISpSREngineVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpSREngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpSREngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpSREngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpSREngine_SetSite(This,pSite)	\
    (This)->lpVtbl -> SetSite(This,pSite)

#define ISpSREngine_GetInputAudioFormat(This,pguidSourceFormatId,pSourceWaveFormatEx,pguidDesiredFormatId,ppCoMemDesiredWaveFormatEx)	\
    (This)->lpVtbl -> GetInputAudioFormat(This,pguidSourceFormatId,pSourceWaveFormatEx,pguidDesiredFormatId,ppCoMemDesiredWaveFormatEx)

#define ISpSREngine_RecognizeStream(This,rguidFmtId,pWaveFormatEx,hRequestSync,hDataAvailable,hExit,fNewAudioStream,fRealTimeAudio,pAudioObjectToken)	\
    (This)->lpVtbl -> RecognizeStream(This,rguidFmtId,pWaveFormatEx,hRequestSync,hDataAvailable,hExit,fNewAudioStream,fRealTimeAudio,pAudioObjectToken)

#define ISpSREngine_SetRecoProfile(This,pProfile)	\
    (This)->lpVtbl -> SetRecoProfile(This,pProfile)

#define ISpSREngine_OnCreateGrammar(This,pvEngineRecoContext,hSAPIGrammar,ppvEngineGrammarContext)	\
    (This)->lpVtbl -> OnCreateGrammar(This,pvEngineRecoContext,hSAPIGrammar,ppvEngineGrammarContext)

#define ISpSREngine_OnDeleteGrammar(This,pvEngineGrammar)	\
    (This)->lpVtbl -> OnDeleteGrammar(This,pvEngineGrammar)

#define ISpSREngine_LoadProprietaryGrammar(This,pvEngineGrammar,rguidParam,pszStringParam,pvDataParam,ulDataSize,Options)	\
    (This)->lpVtbl -> LoadProprietaryGrammar(This,pvEngineGrammar,rguidParam,pszStringParam,pvDataParam,ulDataSize,Options)

#define ISpSREngine_UnloadProprietaryGrammar(This,pvEngineGrammar)	\
    (This)->lpVtbl -> UnloadProprietaryGrammar(This,pvEngineGrammar)

#define ISpSREngine_SetProprietaryRuleState(This,pvEngineGrammar,pszName,pReserved,NewState,pcRulesChanged)	\
    (This)->lpVtbl -> SetProprietaryRuleState(This,pvEngineGrammar,pszName,pReserved,NewState,pcRulesChanged)

#define ISpSREngine_SetProprietaryRuleIdState(This,pvEngineGrammar,dwRuleId,NewState)	\
    (This)->lpVtbl -> SetProprietaryRuleIdState(This,pvEngineGrammar,dwRuleId,NewState)

#define ISpSREngine_LoadSLM(This,pvEngineGrammar,pszTopicName)	\
    (This)->lpVtbl -> LoadSLM(This,pvEngineGrammar,pszTopicName)

#define ISpSREngine_UnloadSLM(This,pvEngineGrammar)	\
    (This)->lpVtbl -> UnloadSLM(This,pvEngineGrammar)

#define ISpSREngine_SetSLMState(This,pvEngineGrammar,NewState)	\
    (This)->lpVtbl -> SetSLMState(This,pvEngineGrammar,NewState)

#define ISpSREngine_SetWordSequenceData(This,pvEngineGrammar,pText,cchText,pInfo)	\
    (This)->lpVtbl -> SetWordSequenceData(This,pvEngineGrammar,pText,cchText,pInfo)

#define ISpSREngine_SetTextSelection(This,pvEngineGrammar,pInfo)	\
    (This)->lpVtbl -> SetTextSelection(This,pvEngineGrammar,pInfo)

#define ISpSREngine_IsPronounceable(This,pvEngineGrammar,pszWord,pWordPronounceable)	\
    (This)->lpVtbl -> IsPronounceable(This,pvEngineGrammar,pszWord,pWordPronounceable)

#define ISpSREngine_OnCreateRecoContext(This,hSAPIRecoContext,ppvEngineContext)	\
    (This)->lpVtbl -> OnCreateRecoContext(This,hSAPIRecoContext,ppvEngineContext)

#define ISpSREngine_OnDeleteRecoContext(This,pvEngineContext)	\
    (This)->lpVtbl -> OnDeleteRecoContext(This,pvEngineContext)

#define ISpSREngine_PrivateCall(This,pvEngineContext,pCallFrame,ulCallFrameSize)	\
    (This)->lpVtbl -> PrivateCall(This,pvEngineContext,pCallFrame,ulCallFrameSize)

#define ISpSREngine_SetAdaptationData(This,pvEngineContext,pAdaptationData,cch)	\
    (This)->lpVtbl -> SetAdaptationData(This,pvEngineContext,pAdaptationData,cch)

#define ISpSREngine_SetPropertyNum(This,eSrc,pvSrcObj,pName,lValue)	\
    (This)->lpVtbl -> SetPropertyNum(This,eSrc,pvSrcObj,pName,lValue)

#define ISpSREngine_GetPropertyNum(This,eSrc,pvSrcObj,pName,lValue)	\
    (This)->lpVtbl -> GetPropertyNum(This,eSrc,pvSrcObj,pName,lValue)

#define ISpSREngine_SetPropertyString(This,eSrc,pvSrcObj,pName,pValue)	\
    (This)->lpVtbl -> SetPropertyString(This,eSrc,pvSrcObj,pName,pValue)

#define ISpSREngine_GetPropertyString(This,eSrc,pvSrcObj,pName,ppCoMemValue)	\
    (This)->lpVtbl -> GetPropertyString(This,eSrc,pvSrcObj,pName,ppCoMemValue)

#define ISpSREngine_SetGrammarState(This,pvEngineGrammar,eGrammarState)	\
    (This)->lpVtbl -> SetGrammarState(This,pvEngineGrammar,eGrammarState)

#define ISpSREngine_WordNotify(This,Action,cWords,pWords)	\
    (This)->lpVtbl -> WordNotify(This,Action,cWords,pWords)

#define ISpSREngine_RuleNotify(This,Action,cRules,pRules)	\
    (This)->lpVtbl -> RuleNotify(This,Action,cRules,pRules)

#define ISpSREngine_PrivateCallEx(This,pvEngineContext,pInCallFrame,ulInCallFrameSize,ppvCoMemResponse,pulResponseSize)	\
    (This)->lpVtbl -> PrivateCallEx(This,pvEngineContext,pInCallFrame,ulInCallFrameSize,ppvCoMemResponse,pulResponseSize)

#define ISpSREngine_SetContextState(This,pvEngineContext,eContextState)	\
    (This)->lpVtbl -> SetContextState(This,pvEngineContext,eContextState)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpSREngine_SetSite_Proxy( 
    ISpSREngine * This,
    /* [in] */ ISpSREngineSite *pSite);


void __RPC_STUB ISpSREngine_SetSite_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_GetInputAudioFormat_Proxy( 
    ISpSREngine * This,
    /* [in] */ const GUID *pguidSourceFormatId,
    /* [in] */ const WAVEFORMATEX *pSourceWaveFormatEx,
    /* [out] */ GUID *pguidDesiredFormatId,
    /* [out] */ WAVEFORMATEX **ppCoMemDesiredWaveFormatEx);


void __RPC_STUB ISpSREngine_GetInputAudioFormat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_RecognizeStream_Proxy( 
    ISpSREngine * This,
    /* [in] */ REFGUID rguidFmtId,
    /* [in] */ const WAVEFORMATEX *pWaveFormatEx,
    /* [in] */ HANDLE hRequestSync,
    /* [in] */ HANDLE hDataAvailable,
    /* [in] */ HANDLE hExit,
    /* [in] */ BOOL fNewAudioStream,
    /* [in] */ BOOL fRealTimeAudio,
    /* [in] */ ISpObjectToken *pAudioObjectToken);


void __RPC_STUB ISpSREngine_RecognizeStream_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetRecoProfile_Proxy( 
    ISpSREngine * This,
    ISpObjectToken *pProfile);


void __RPC_STUB ISpSREngine_SetRecoProfile_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_OnCreateGrammar_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineRecoContext,
    /* [in] */ SPGRAMMARHANDLE hSAPIGrammar,
    /* [out] */ void **ppvEngineGrammarContext);


void __RPC_STUB ISpSREngine_OnCreateGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_OnDeleteGrammar_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar);


void __RPC_STUB ISpSREngine_OnDeleteGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_LoadProprietaryGrammar_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ REFGUID rguidParam,
    /* [string][in] */ const WCHAR *pszStringParam,
    /* [in] */ const void *pvDataParam,
    /* [in] */ ULONG ulDataSize,
    /* [in] */ SPLOADOPTIONS Options);


void __RPC_STUB ISpSREngine_LoadProprietaryGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_UnloadProprietaryGrammar_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar);


void __RPC_STUB ISpSREngine_UnloadProprietaryGrammar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetProprietaryRuleState_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [string][in] */ const WCHAR *pszName,
    /* [in] */ void *pReserved,
    /* [in] */ SPRULESTATE NewState,
    /* [out] */ ULONG *pcRulesChanged);


void __RPC_STUB ISpSREngine_SetProprietaryRuleState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetProprietaryRuleIdState_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ DWORD dwRuleId,
    /* [in] */ SPRULESTATE NewState);


void __RPC_STUB ISpSREngine_SetProprietaryRuleIdState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_LoadSLM_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [string][in] */ const WCHAR *pszTopicName);


void __RPC_STUB ISpSREngine_LoadSLM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_UnloadSLM_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar);


void __RPC_STUB ISpSREngine_UnloadSLM_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetSLMState_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ SPRULESTATE NewState);


void __RPC_STUB ISpSREngine_SetSLMState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetWordSequenceData_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ const WCHAR *pText,
    /* [in] */ ULONG cchText,
    /* [in] */ const SPTEXTSELECTIONINFO *pInfo);


void __RPC_STUB ISpSREngine_SetWordSequenceData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetTextSelection_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ const SPTEXTSELECTIONINFO *pInfo);


void __RPC_STUB ISpSREngine_SetTextSelection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_IsPronounceable_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [string][in] */ const WCHAR *pszWord,
    /* [out] */ SPWORDPRONOUNCEABLE *pWordPronounceable);


void __RPC_STUB ISpSREngine_IsPronounceable_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_OnCreateRecoContext_Proxy( 
    ISpSREngine * This,
    /* [in] */ SPRECOCONTEXTHANDLE hSAPIRecoContext,
    /* [out] */ void **ppvEngineContext);


void __RPC_STUB ISpSREngine_OnCreateRecoContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_OnDeleteRecoContext_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineContext);


void __RPC_STUB ISpSREngine_OnDeleteRecoContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_PrivateCall_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineContext,
    /* [out][in] */ PVOID pCallFrame,
    /* [in] */ ULONG ulCallFrameSize);


void __RPC_STUB ISpSREngine_PrivateCall_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetAdaptationData_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineContext,
    const WCHAR *pAdaptationData,
    const ULONG cch);


void __RPC_STUB ISpSREngine_SetAdaptationData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetPropertyNum_Proxy( 
    ISpSREngine * This,
    /* [in] */ SPPROPSRC eSrc,
    /* [in] */ void *pvSrcObj,
    /* [in] */ const WCHAR *pName,
    /* [in] */ LONG lValue);


void __RPC_STUB ISpSREngine_SetPropertyNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_GetPropertyNum_Proxy( 
    ISpSREngine * This,
    /* [in] */ SPPROPSRC eSrc,
    /* [in] */ void *pvSrcObj,
    /* [in] */ const WCHAR *pName,
    /* [out] */ LONG *lValue);


void __RPC_STUB ISpSREngine_GetPropertyNum_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetPropertyString_Proxy( 
    ISpSREngine * This,
    /* [in] */ SPPROPSRC eSrc,
    /* [in] */ void *pvSrcObj,
    /* [in] */ const WCHAR *pName,
    /* [in] */ const WCHAR *pValue);


void __RPC_STUB ISpSREngine_SetPropertyString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_GetPropertyString_Proxy( 
    ISpSREngine * This,
    /* [in] */ SPPROPSRC eSrc,
    /* [in] */ void *pvSrcObj,
    /* [in] */ const WCHAR *pName,
    /* [out] */ WCHAR **ppCoMemValue);


void __RPC_STUB ISpSREngine_GetPropertyString_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetGrammarState_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineGrammar,
    /* [in] */ SPGRAMMARSTATE eGrammarState);


void __RPC_STUB ISpSREngine_SetGrammarState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_WordNotify_Proxy( 
    ISpSREngine * This,
    SPCFGNOTIFY Action,
    ULONG cWords,
    const SPWORDENTRY *pWords);


void __RPC_STUB ISpSREngine_WordNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_RuleNotify_Proxy( 
    ISpSREngine * This,
    SPCFGNOTIFY Action,
    ULONG cRules,
    const SPRULEENTRY *pRules);


void __RPC_STUB ISpSREngine_RuleNotify_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_PrivateCallEx_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineContext,
    /* [in] */ const void *pInCallFrame,
    /* [in] */ ULONG ulInCallFrameSize,
    /* [out] */ void **ppvCoMemResponse,
    /* [out] */ ULONG *pulResponseSize);


void __RPC_STUB ISpSREngine_PrivateCallEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSREngine_SetContextState_Proxy( 
    ISpSREngine * This,
    /* [in] */ void *pvEngineContext,
    /* [in] */ SPCONTEXTSTATE eContextState);


void __RPC_STUB ISpSREngine_SetContextState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpSREngine_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_sapiddk_0352 */
/* [local] */ 

typedef struct tagSPPHRASEALTREQUEST
    {
    ULONG ulStartElement;
    ULONG cElements;
    ULONG ulRequestAltCount;
    void *pvResultExtra;
    ULONG cbResultExtra;
    ISpPhrase *pPhrase;
    ISpRecoContext *pRecoContext;
    } 	SPPHRASEALTREQUEST;



extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0352_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_sapiddk_0352_v0_0_s_ifspec;

#ifndef __ISpSRAlternates_INTERFACE_DEFINED__
#define __ISpSRAlternates_INTERFACE_DEFINED__

/* interface ISpSRAlternates */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ISpSRAlternates;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FECE8294-2BE1-408f-8E68-2DE377092F0E")
    ISpSRAlternates : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAlternates( 
            /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
            /* [out] */ SPPHRASEALT **ppAlts,
            /* [out] */ ULONG *pcAlts) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( 
            /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
            /* [in] */ SPPHRASEALT *pAlt,
            /* [out] */ void **ppvResultExtra,
            /* [out] */ ULONG *pcbResultExtra) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ISpSRAlternatesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ISpSRAlternates * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ISpSRAlternates * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ISpSRAlternates * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAlternates )( 
            ISpSRAlternates * This,
            /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
            /* [out] */ SPPHRASEALT **ppAlts,
            /* [out] */ ULONG *pcAlts);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            ISpSRAlternates * This,
            /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
            /* [in] */ SPPHRASEALT *pAlt,
            /* [out] */ void **ppvResultExtra,
            /* [out] */ ULONG *pcbResultExtra);
        
        END_INTERFACE
    } ISpSRAlternatesVtbl;

    interface ISpSRAlternates
    {
        CONST_VTBL struct ISpSRAlternatesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ISpSRAlternates_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ISpSRAlternates_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ISpSRAlternates_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ISpSRAlternates_GetAlternates(This,pAltRequest,ppAlts,pcAlts)	\
    (This)->lpVtbl -> GetAlternates(This,pAltRequest,ppAlts,pcAlts)

#define ISpSRAlternates_Commit(This,pAltRequest,pAlt,ppvResultExtra,pcbResultExtra)	\
    (This)->lpVtbl -> Commit(This,pAltRequest,pAlt,ppvResultExtra,pcbResultExtra)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE ISpSRAlternates_GetAlternates_Proxy( 
    ISpSRAlternates * This,
    /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
    /* [out] */ SPPHRASEALT **ppAlts,
    /* [out] */ ULONG *pcAlts);


void __RPC_STUB ISpSRAlternates_GetAlternates_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE ISpSRAlternates_Commit_Proxy( 
    ISpSRAlternates * This,
    /* [in] */ SPPHRASEALTREQUEST *pAltRequest,
    /* [in] */ SPPHRASEALT *pAlt,
    /* [out] */ void **ppvResultExtra,
    /* [out] */ ULONG *pcbResultExtra);


void __RPC_STUB ISpSRAlternates_Commit_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __ISpSRAlternates_INTERFACE_DEFINED__ */


#ifndef ___ISpPrivateEngineCall_INTERFACE_DEFINED__
#define ___ISpPrivateEngineCall_INTERFACE_DEFINED__

/* interface _ISpPrivateEngineCall */
/* [local][unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID__ISpPrivateEngineCall;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("8E7C791E-4467-11d3-9723-00C04F72DB08")
    _ISpPrivateEngineCall : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CallEngine( 
            /* [out][in] */ void *pCallFrame,
            /* [in] */ ULONG ulCallFrameSize) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CallEngineEx( 
            /* [in] */ const void *pInFrame,
            /* [in] */ ULONG ulInFrameSize,
            /* [out] */ void **ppCoMemOutFrame,
            /* [out] */ ULONG *pulOutFrameSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct _ISpPrivateEngineCallVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _ISpPrivateEngineCall * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _ISpPrivateEngineCall * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _ISpPrivateEngineCall * This);
        
        HRESULT ( STDMETHODCALLTYPE *CallEngine )( 
            _ISpPrivateEngineCall * This,
            /* [out][in] */ void *pCallFrame,
            /* [in] */ ULONG ulCallFrameSize);
        
        HRESULT ( STDMETHODCALLTYPE *CallEngineEx )( 
            _ISpPrivateEngineCall * This,
            /* [in] */ const void *pInFrame,
            /* [in] */ ULONG ulInFrameSize,
            /* [out] */ void **ppCoMemOutFrame,
            /* [out] */ ULONG *pulOutFrameSize);
        
        END_INTERFACE
    } _ISpPrivateEngineCallVtbl;

    interface _ISpPrivateEngineCall
    {
        CONST_VTBL struct _ISpPrivateEngineCallVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _ISpPrivateEngineCall_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define _ISpPrivateEngineCall_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define _ISpPrivateEngineCall_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define _ISpPrivateEngineCall_CallEngine(This,pCallFrame,ulCallFrameSize)	\
    (This)->lpVtbl -> CallEngine(This,pCallFrame,ulCallFrameSize)

#define _ISpPrivateEngineCall_CallEngineEx(This,pInFrame,ulInFrameSize,ppCoMemOutFrame,pulOutFrameSize)	\
    (This)->lpVtbl -> CallEngineEx(This,pInFrame,ulInFrameSize,ppCoMemOutFrame,pulOutFrameSize)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE _ISpPrivateEngineCall_CallEngine_Proxy( 
    _ISpPrivateEngineCall * This,
    /* [out][in] */ void *pCallFrame,
    /* [in] */ ULONG ulCallFrameSize);


void __RPC_STUB _ISpPrivateEngineCall_CallEngine_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE _ISpPrivateEngineCall_CallEngineEx_Proxy( 
    _ISpPrivateEngineCall * This,
    /* [in] */ const void *pInFrame,
    /* [in] */ ULONG ulInFrameSize,
    /* [out] */ void **ppCoMemOutFrame,
    /* [out] */ ULONG *pulOutFrameSize);


void __RPC_STUB _ISpPrivateEngineCall_CallEngineEx_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* ___ISpPrivateEngineCall_INTERFACE_DEFINED__ */



#ifndef __SpeechDDKLib_LIBRARY_DEFINED__
#define __SpeechDDKLib_LIBRARY_DEFINED__

/* library SpeechDDKLib */
/* [version][uuid][helpstring] */ 


EXTERN_C const IID LIBID_SpeechDDKLib;

EXTERN_C const CLSID CLSID_SpDataKey;

#ifdef __cplusplus

class DECLSPEC_UUID("D9F6EE60-58C9-458b-88E1-2F908FD7F87C")
SpDataKey;
#endif

EXTERN_C const CLSID CLSID_SpObjectTokenEnum;

#ifdef __cplusplus

class DECLSPEC_UUID("3918D75F-0ACB-41f2-B733-92AA15BCECF6")
SpObjectTokenEnum;
#endif

EXTERN_C const CLSID CLSID_SpPhraseBuilder;

#ifdef __cplusplus

class DECLSPEC_UUID("777B6BBD-2FF2-11d3-88FE-00C04F8EF9B5")
SpPhraseBuilder;
#endif

EXTERN_C const CLSID CLSID_SpITNProcessor;

#ifdef __cplusplus

class DECLSPEC_UUID("12D73610-A1C9-11d3-BC90-00C04F72DF9F")
SpITNProcessor;
#endif

EXTERN_C const CLSID CLSID_SpGrammarCompiler;

#ifdef __cplusplus

class DECLSPEC_UUID("B1E29D59-A675-11D2-8302-00C04F8EE6C0")
SpGrammarCompiler;
#endif

EXTERN_C const CLSID CLSID_SpGramCompBackend;

#ifdef __cplusplus

class DECLSPEC_UUID("DA93E903-C843-11D2-A084-00C04F8EF9B5")
SpGramCompBackend;
#endif
#endif /* __SpeechDDKLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


