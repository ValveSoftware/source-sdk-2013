
//========= Copyright Valve Corporation, All rights reserved. ============//
/*
** The copyright to the contents herein is the property of Valve Corporation.
** The contents may be used and/or copied only with the written permission of
** Valve, or in accordance with the terms and conditions stipulated in
** the agreement/contract under which the contents have been supplied.
**
*******************************************************************************
**
** Contents:
**
**		Common types used in the Steam DLL interface.
**
**		This file is distributed to Steam application developers.
**
**		
**
*******************************************************************************/

#ifndef INCLUDED_STEAM_COMMON_STEAMCOMMON_H
#define INCLUDED_STEAM_COMMON_STEAMCOMMON_H

#if defined(_MSC_VER) && (_MSC_VER > 1000)
#pragma once
#endif


#ifdef __cplusplus
extern "C"
{
#endif

/* Applications should not define STEAM_EXPORTS. */

#if defined ( _WIN32 )

#ifdef STEAM_EXPORTS
#define STEAM_API __declspec(dllexport)
#else
#define STEAM_API __declspec(dllimport)
#endif

#define STEAM_CALL __cdecl

#else

#define STEAM_API /* */
#define STEAM_CALL /* */

#endif

typedef void (STEAM_CALL  *KeyValueIteratorCallback_t )(const char *Key, const char *Val, void *pvParam);


/******************************************************************************
**
** Exported macros and constants
**
******************************************************************************/

/* DEPRECATED -- these are ignored now, all API access is granted on SteamStartup */
#define STEAM_USING_FILESYSTEM							(0x00000001)
#define STEAM_USING_LOGGING								(0x00000002)
#define STEAM_USING_USERID								(0x00000004)
#define STEAM_USING_ACCOUNT								(0x00000008)
#define STEAM_USING_ALL									(0x0000000f)
/* END DEPRECATED */

#define STEAM_MAX_PATH									(255)
#define STEAM_QUESTION_MAXLEN							(255)
#define STEAM_SALT_SIZE									(8)
#define STEAM_PROGRESS_PERCENT_SCALE					(0x00001000)

/* These are maximum significant string lengths, excluding nul-terminator. */
#define STEAM_CARD_NUMBER_SIZE							(17)
#define STEAM_CARD_HOLDERNAME_SIZE						(100)
#define STEAM_CARD_EXPYEAR_SIZE							(4)
#define STEAM_CARD_EXPMONTH_SIZE						(2)
#define STEAM_CARD_CVV2_SIZE							(5)
#define STEAM_BILLING_ADDRESS1_SIZE						(128)
#define STEAM_BILLING_ADDRESS2_SIZE						(128)
#define STEAM_BILLING_CITY_SIZE							(50)
#define STEAM_BILLING_ZIP_SIZE							(16)
#define STEAM_BILLING_STATE_SIZE						(32)
#define STEAM_BILLING_COUNTRY_SIZE						(32)
#define STEAM_BILLING_PHONE_SIZE						(20)
#define STEAM_BILLING_EMAIL_ADDRESS_SIZE				(100)
#define STEAM_TYPE_OF_PROOF_OF_PURCHASE_SIZE			(20)
#define STEAM_PROOF_OF_PURCHASE_TOKEN_SIZE				(200)
#define STEAM_EXTERNAL_ACCOUNTNAME_SIZE					(100)
#define STEAM_EXTERNAL_ACCOUNTPASSWORD_SIZE				(80)
#define STEAM_BILLING_CONFIRMATION_CODE_SIZE			(22)
#define STEAM_BILLING_CARD_APPROVAL_CODE_SIZE			(100)
#define STEAM_BILLING_TRANS_DATE_SIZE					(9) // mm/dd/yy
#define STEAM_BILLING_TRANS_TIME_SIZE					(9) // hh:mm:ss

/******************************************************************************
**
** Scalar type and enumerated type definitions.
**
******************************************************************************/


typedef unsigned int SteamHandle_t;

typedef void * SteamUserIDTicketValidationHandle_t;

typedef unsigned int SteamCallHandle_t;

#if defined(_MSC_VER)
typedef unsigned __int64	SteamUnsigned64_t;
#else
typedef unsigned long long	SteamUnsigned64_t;
#endif

typedef enum
{
	eSteamSeekMethodSet = 0,
	eSteamSeekMethodCur = 1,
	eSteamSeekMethodEnd = 2
} ESteamSeekMethod;

typedef enum
{
	eSteamBufferMethodFBF = 0,
	eSteamBufferMethodNBF = 1
} ESteamBufferMethod;

typedef enum
{
	eSteamErrorNone													= 0,
	eSteamErrorUnknown												= 1,
	eSteamErrorLibraryNotInitialized								= 2,
	eSteamErrorLibraryAlreadyInitialized							= 3,
	eSteamErrorConfig												= 4,
	eSteamErrorContentServerConnect									= 5,
	eSteamErrorBadHandle											= 6,
	eSteamErrorHandlesExhausted										= 7,
	eSteamErrorBadArg												= 8,
	eSteamErrorNotFound												= 9,
	eSteamErrorRead													= 10,
	eSteamErrorEOF													= 11,
	eSteamErrorSeek													= 12,
	eSteamErrorCannotWriteNonUserConfigFile							= 13,
	eSteamErrorCacheOpen											= 14,
	eSteamErrorCacheRead											= 15,
	eSteamErrorCacheCorrupted										= 16,
	eSteamErrorCacheWrite											= 17,
	eSteamErrorCacheSession											= 18,
	eSteamErrorCacheInternal										= 19,
	eSteamErrorCacheBadApp											= 20,
	eSteamErrorCacheVersion											= 21,
	eSteamErrorCacheBadFingerPrint									= 22,

	eSteamErrorNotFinishedProcessing								= 23,
	eSteamErrorNothingToDo											= 24,
	eSteamErrorCorruptEncryptedUserIDTicket							= 25,
	eSteamErrorSocketLibraryNotInitialized							= 26,
	eSteamErrorFailedToConnectToUserIDTicketValidationServer		= 27,
	eSteamErrorBadProtocolVersion									= 28,
	eSteamErrorReplayedUserIDTicketFromClient						= 29,
	eSteamErrorReceiveResultBufferTooSmall							= 30,
	eSteamErrorSendFailed											= 31,
	eSteamErrorReceiveFailed										= 32,
	eSteamErrorReplayedReplyFromUserIDTicketValidationServer		= 33,
	eSteamErrorBadSignatureFromUserIDTicketValidationServer			= 34,
	eSteamErrorValidationStalledSoAborted							= 35,
	eSteamErrorInvalidUserIDTicket									= 36,
	eSteamErrorClientLoginRateTooHigh								= 37,
	eSteamErrorClientWasNeverValidated								= 38,
	eSteamErrorInternalSendBufferTooSmall							= 39,
	eSteamErrorInternalReceiveBufferTooSmall						= 40,
	eSteamErrorUserTicketExpired									= 41,
	eSteamErrorCDKeyAlreadyInUseOnAnotherClient						= 42,

	eSteamErrorNotLoggedIn											= 101,
	eSteamErrorAlreadyExists										= 102,
	eSteamErrorAlreadySubscribed									= 103,
	eSteamErrorNotSubscribed										= 104,
	eSteamErrorAccessDenied											= 105,
	eSteamErrorFailedToCreateCacheFile								= 106,
	eSteamErrorCallStalledSoAborted									= 107,
	eSteamErrorEngineNotRunning										= 108,
	eSteamErrorEngineConnectionLost									= 109,
	eSteamErrorLoginFailed											= 110,
	eSteamErrorAccountPending										= 111,
	eSteamErrorCacheWasMissingRetry									= 112,
	eSteamErrorLocalTimeIncorrect									= 113,
	eSteamErrorCacheNeedsDecryption									= 114,
	eSteamErrorAccountDisabled										= 115,
	eSteamErrorCacheNeedsRepair										= 116,
	eSteamErrorRebootRequired										= 117,

	eSteamErrorNetwork												= 200,
	eSteamErrorOffline												= 201


} ESteamError;


typedef enum
{
	eNoDetailedErrorAvailable,
	eStandardCerrno,
	eWin32LastError,
	eWinSockLastError,
	eDetailedPlatformErrorCount
} EDetailedPlatformErrorType;

typedef enum						/* Filter elements returned by SteamFind{First,Next} */
{
	eSteamFindLocalOnly,			/* limit search to local filesystem */
	eSteamFindRemoteOnly,			/* limit search to remote repository */
	eSteamFindAll					/* do not limit search (duplicates allowed) */
} ESteamFindFilter;


/******************************************************************************
**
** Exported structure and complex type definitions.
**
******************************************************************************/


typedef struct
{
	ESteamError eSteamError;
	EDetailedPlatformErrorType eDetailedErrorType;
	int nDetailedErrorCode;
	char szDesc[STEAM_MAX_PATH];
} TSteamError;



typedef struct
{
	int bIsDir;						/* If non-zero, element is a directory; if zero, element is a file */
	unsigned int uSizeOrCount;		/* If element is a file, this contains size of file in bytes */
	int bIsLocal;					/* If non-zero, reported item is a standalone element on local filesystem */
	char cszName[STEAM_MAX_PATH];	/* Base element name (no path) */
	long lLastAccessTime;			/* Seconds since 1/1/1970 (like time_t) when element was last accessed */
	long lLastModificationTime;		/* Seconds since 1/1/1970 (like time_t) when element was last modified */
	long lCreationTime;				/* Seconds since 1/1/1970 (like time_t) when element was created */
} TSteamElemInfo;


typedef struct
{
	unsigned int uNumSubscriptions;
	unsigned int uMaxNameChars;
	unsigned int uMaxApps;

} TSteamSubscriptionStats;


typedef struct
{
	unsigned int uNumApps;
	unsigned int uMaxNameChars;
	unsigned int uMaxInstallDirNameChars;
	unsigned int uMaxVersionLabelChars;
	unsigned int uMaxLaunchOptions;
	unsigned int uMaxLaunchOptionDescChars;
	unsigned int uMaxLaunchOptionCmdLineChars;
	unsigned int uMaxNumIcons;
	unsigned int uMaxIconSize;
	unsigned int uMaxDependencies;

} TSteamAppStats;

typedef struct
{
	char *szLabel;
	unsigned int uMaxLabelChars;
	unsigned int uVersionId;
	int bIsNotAvailable;
} TSteamAppVersion;

typedef struct
{
	char *szDesc;
	unsigned int uMaxDescChars;
	char *szCmdLine;
	unsigned int uMaxCmdLineChars;
	unsigned int uIndex;
	unsigned int uIconIndex;
	int bNoDesktopShortcut;
	int bNoStartMenuShortcut;
	int bIsLongRunningUnattended;

} TSteamAppLaunchOption;


typedef struct
{
	char *szName;
	unsigned int uMaxNameChars;
	char *szLatestVersionLabel;
	unsigned int uMaxLatestVersionLabelChars;
	char *szCurrentVersionLabel;
	unsigned int uMaxCurrentVersionLabelChars;
	char *szInstallDirName;
	unsigned int uMaxInstallDirNameChars;
	unsigned int uId;
	unsigned int uLatestVersionId;
	unsigned int uCurrentVersionId;
	unsigned int uMinCacheFileSizeMB;
	unsigned int uMaxCacheFileSizeMB;
	unsigned int uNumLaunchOptions;
	unsigned int uNumIcons;
	unsigned int uNumVersions;
	unsigned int uNumDependencies;

} TSteamApp;

typedef enum 
{
	eNoCost				= 0,
	eBillOnceOnly		= 1,
	eBillMonthly		= 2,
	eProofOfPrepurchaseOnly	= 3,
	eGuestPass			= 4,
	eHardwarePromo		= 5,
	eNumBillingTypes,
} EBillingType;

typedef struct
{
	char *szName;
	unsigned int uMaxNameChars;
	unsigned int *puAppIds;
	unsigned int uMaxAppIds;
	unsigned int uId;
	unsigned int uNumApps;
	EBillingType eBillingType;
	unsigned int uCostInCents;
	unsigned int uNumDiscounts;
	int bIsPreorder;
	int bRequiresShippingAddress;
	unsigned int uDomesticShippingCostInCents;
	unsigned int uInternationalShippingCostInCents;
	bool bIsCyberCafeSubscription;
	unsigned int uGameCode;
	char szGameCodeDesc[STEAM_MAX_PATH];
	bool bIsDisabled;
	bool bRequiresCD;
	unsigned int uTerritoryCode;
	bool bIsSteam3Subscription;

} TSteamSubscription;

typedef struct
{
	char szName[STEAM_MAX_PATH];
	unsigned int uDiscountInCents;
	unsigned int uNumQualifiers;

} TSteamSubscriptionDiscount;

typedef struct
{
	char szName[STEAM_MAX_PATH];
	unsigned int uRequiredSubscription;
	int bIsDisqualifier;

} TSteamDiscountQualifier;

typedef struct TSteamProgress
{
	int bValid;                      /* non-zero if call provides progress info */
	unsigned int uPercentDone;       /* 0 to 100 * STEAM_PROGRESS_PERCENT_SCALE if bValid */
	char szProgress[STEAM_MAX_PATH]; /* additional progress info */
} TSteamProgress;

typedef enum
{
	eSteamNotifyTicketsWillExpire,
	eSteamNotifyAccountInfoChanged,
	eSteamNotifyContentDescriptionChanged,
	eSteamNotifyPleaseShutdown,
	eSteamNotifyNewContentServer,
	eSteamNotifySubscriptionStatusChanged,
	eSteamNotifyContentServerConnectionLost,
	eSteamNotifyCacheLoadingCompleted,
	eSteamNotifyCacheNeedsDecryption,
	eSteamNotifyCacheNeedsRepair
} ESteamNotificationCallbackEvent;


typedef void(*SteamNotificationCallback_t)(ESteamNotificationCallbackEvent eEvent, unsigned int nData);


typedef char SteamPersonalQuestion_t[ STEAM_QUESTION_MAXLEN + 1 ];

typedef struct
{
	unsigned char uchSalt[STEAM_SALT_SIZE];
} SteamSalt_t;

typedef enum
{
	eVisa				= 1,
	eMaster				= 2,
	eAmericanExpress	= 3,
	eDiscover			= 4,
	eDinnersClub		= 5,
	eJCB				= 6
} ESteamPaymentCardType;

typedef struct
{
	ESteamPaymentCardType eCardType;
	char szCardNumber[ STEAM_CARD_NUMBER_SIZE +1 ];
	char szCardHolderName[ STEAM_CARD_HOLDERNAME_SIZE + 1];
	char szCardExpYear[ STEAM_CARD_EXPYEAR_SIZE + 1 ];
	char szCardExpMonth[ STEAM_CARD_EXPMONTH_SIZE+ 1 ];
	char szCardCVV2[ STEAM_CARD_CVV2_SIZE + 1 ];
	char szBillingAddress1[ STEAM_BILLING_ADDRESS1_SIZE + 1 ];
	char szBillingAddress2[ STEAM_BILLING_ADDRESS2_SIZE + 1 ];
	char szBillingCity[ STEAM_BILLING_CITY_SIZE + 1 ];
	char szBillingZip[ STEAM_BILLING_ZIP_SIZE + 1 ];
	char szBillingState[ STEAM_BILLING_STATE_SIZE + 1 ];
	char szBillingCountry[ STEAM_BILLING_COUNTRY_SIZE + 1 ];
	char szBillingPhone[ STEAM_BILLING_PHONE_SIZE + 1 ];
	char szBillingEmailAddress[ STEAM_BILLING_EMAIL_ADDRESS_SIZE + 1 ];
	unsigned int uExpectedCostInCents;
	unsigned int uExpectedTaxInCents;
	/* If the TSteamSubscription says that shipping info is required,	*/
	/* then the following fields must be filled out.					*/
	/* If szShippingName is empty, then assumes so are the rest.		*/
	char szShippingName[ STEAM_CARD_HOLDERNAME_SIZE + 1];
	char szShippingAddress1[ STEAM_BILLING_ADDRESS1_SIZE + 1 ];
	char szShippingAddress2[ STEAM_BILLING_ADDRESS2_SIZE + 1 ];
	char szShippingCity[ STEAM_BILLING_CITY_SIZE + 1 ];
	char szShippingZip[ STEAM_BILLING_ZIP_SIZE + 1 ];
	char szShippingState[ STEAM_BILLING_STATE_SIZE + 1 ];
	char szShippingCountry[ STEAM_BILLING_COUNTRY_SIZE + 1 ];
	char szShippingPhone[ STEAM_BILLING_PHONE_SIZE + 1 ];
	unsigned int uExpectedShippingCostInCents;

} TSteamPaymentCardInfo;

typedef struct
{
	char					szTypeOfProofOfPurchase[ STEAM_TYPE_OF_PROOF_OF_PURCHASE_SIZE + 1 ];

	/* A ProofOfPurchase token is not necessarily a nul-terminated string; it may be binary data
	   (perhaps encrypted). Hence we need a length and an array of bytes.							*/
	unsigned int			uLengthOfBinaryProofOfPurchaseToken;	
	char					cBinaryProofOfPurchaseToken[ STEAM_PROOF_OF_PURCHASE_TOKEN_SIZE + 1 ];
} TSteamPrepurchaseInfo;

typedef struct
{
	char szAccountName[ STEAM_EXTERNAL_ACCOUNTNAME_SIZE + 1 ];
	char szPassword[ STEAM_EXTERNAL_ACCOUNTPASSWORD_SIZE + 1 ];
} TSteamExternalBillingInfo;

typedef enum
{
	ePaymentCardInfo		= 1,
	ePrepurchasedInfo		= 2,
	eAccountBillingInfo		= 3,
	eExternalBillingInfo	= 4,		/* indirect billing via ISP etc (not supported yet) */
	ePaymentCardReceipt		= 5,
	ePrepurchaseReceipt		= 6,
	eEmptyReceipt			= 7
} ESteamSubscriptionBillingInfoType;

typedef struct
{
	ESteamSubscriptionBillingInfoType	eBillingInfoType;
	union {
		TSteamPaymentCardInfo			PaymentCardInfo;
		TSteamPrepurchaseInfo			PrepurchaseInfo;
		TSteamExternalBillingInfo		ExternalBillingInfo;
		char							bUseAccountBillingInfo;
	};

} TSteamSubscriptionBillingInfo;

typedef enum
{
	/* Subscribed */
	eSteamSubscriptionOK							= 0,	/* Subscribed */
	eSteamSubscriptionPending						= 1,	/* Awaiting	transaction	completion */
	eSteamSubscriptionPreorder						= 2,	/* Is currently	a pre-order */
	eSteamSubscriptionPrepurchaseTransferred		= 3,	/* hop to this account */
	/* Unusbscribed */
	eSteamSubscriptionPrepurchaseInvalid			= 4,	/* Invalid cd-key */
	eSteamSubscriptionPrepurchaseRejected			= 5,	/* hopped out / banned / etc */
	eSteamSubscriptionPrepurchaseRevoked			= 6,	/* hop away from this account */
	eSteamSubscriptionPaymentCardDeclined			= 7,	/* CC txn declined */
	eSteamSubscriptionCancelledByUser				= 8,	/* Cancelled by	client */
	eSteamSubscriptionCancelledByVendor				= 9,	/* Cancelled by us */
	eSteamSubscriptionPaymentCardUseLimit			= 10,	/* Card used too many times, potential fraud */
	eSteamSubscriptionPaymentCardAlert				= 11,	/* Got a "pick up card" or the like from bank */
	eSteamSubscriptionFailed						= 12,	/* Other Error in subscription data or transaction failed/lost */
	eSteamSubscriptionPaymentCardAVSFailure			= 13,	/* Card failed Address Verification check */
	eSteamSubscriptionPaymentCardInsufficientFunds	= 14,	/* Card failed due to insufficient funds */
	eSteamSubscriptionRestrictedCountry				= 15	/* The subscription is not available in the user's country */

} ESteamSubscriptionStatus;

typedef struct
{
	ESteamPaymentCardType	eCardType;
	char szCardLastFourDigits[ 4 + 1 ];
	char szCardHolderName[ STEAM_CARD_HOLDERNAME_SIZE + 1];
	char szBillingAddress1[ STEAM_BILLING_ADDRESS1_SIZE + 1 ];
	char szBillingAddress2[ STEAM_BILLING_ADDRESS2_SIZE + 1 ];
	char szBillingCity[ STEAM_BILLING_CITY_SIZE + 1 ];
	char szBillingZip[ STEAM_BILLING_ZIP_SIZE + 1 ];
	char szBillingState[ STEAM_BILLING_STATE_SIZE + 1 ];
	char szBillingCountry[ STEAM_BILLING_COUNTRY_SIZE + 1 ];

	// The following are only available after the subscription leaves "pending" status
	char szCardApprovalCode[ STEAM_BILLING_CARD_APPROVAL_CODE_SIZE + 1];
	char szTransDate[ STEAM_BILLING_TRANS_DATE_SIZE	+ 1];	/* mm/dd/yy */
	char szTransTime[ STEAM_BILLING_TRANS_TIME_SIZE	+ 1];	/* hh:mm:ss */
	unsigned int uPriceWithoutTax;
	unsigned int uTaxAmount;
	unsigned int uShippingCost;

} TSteamPaymentCardReceiptInfo;

typedef struct
{
	char szTypeOfProofOfPurchase[ STEAM_TYPE_OF_PROOF_OF_PURCHASE_SIZE + 1 ];
} TSteamPrepurchaseReceiptInfo;

typedef struct
{
	ESteamSubscriptionStatus			eStatus;
	ESteamSubscriptionStatus			ePreviousStatus;
	ESteamSubscriptionBillingInfoType	eReceiptInfoType;
	char szConfirmationCode[ STEAM_BILLING_CONFIRMATION_CODE_SIZE +	1];
	union {
		TSteamPaymentCardReceiptInfo	PaymentCardReceiptInfo;
		TSteamPrepurchaseReceiptInfo	PrepurchaseReceiptInfo;
	};

} TSteamSubscriptionReceipt;

typedef enum
{
	ePhysicalBytesReceivedThisSession = 1,
	eAppReadyToLaunchStatus = 2,
	eAppPreloadStatus = 3,
	eAppEntireDepot = 4,
	eCacheBytesPresent = 5
} ESteamAppUpdateStatsQueryType;

typedef struct
{
	SteamUnsigned64_t uBytesTotal;
	SteamUnsigned64_t uBytesPresent;
} TSteamUpdateStats;

typedef enum
{
	eSteamUserAdministrator	=	0x00000001, /* May subscribe, unsubscribe, etc */
	eSteamUserDeveloper		=	0x00000002, /* Steam or App developer */
	eSteamUserCyberCafe		=	0x00000004  /* CyberCafe, school, etc -- UI should ask for password */
											/* before allowing logout, unsubscribe, etc             */
} ESteamUserTypeFlags;

typedef enum 
{
	eSteamAccountStatusDefault			=	0x00000000,
	eSteamAccountStatusEmailVerified	=	0x00000001,
	/* Note: Mask value 0x2 is reserved for future use. (Some, but not all, public accounts already have this set.) */
	eSteamAccountDisabled				=	0x00000004
} ESteamAccountStatusBitFields ;


typedef enum
{
	eSteamBootstrapperError						= -1,
	eSteamBootstrapperDontCheckForUpdate		= 0,
	eSteamBootstrapperCheckForUpdateAndRerun	= 7

} ESteamBootStrapperClientAppResult;

typedef enum
{
	eSteamOnline = 0,
	eSteamOffline = 1,
	eSteamNoAuthMode = 2,
	eSteamBillingOffline = 3
} eSteamOfflineStatus;

typedef struct
{
	int eOfflineNow;
	int eOfflineNextSession;
} TSteamOfflineStatus;

typedef struct
{
  unsigned int uAppId;
  int bIsSystemDefined;
  char szMountPath[STEAM_MAX_PATH];
} TSteamAppDependencyInfo;

typedef enum
{
	eSteamOpenFileRegular =			0x0,
	eSteamOpenFileIgnoreLocal =		0x1,
	eSteamOpenFileChecksumReads =	0x2
} ESteamOpenFileFlags;

typedef enum
{
	eSteamValveCDKeyValidationServer = 0,
	eSteamHalfLifeMasterServer = 1,
	eSteamFriendsServer = 2,
	eSteamCSERServer = 3,
	eSteamHalfLife2MasterServer = 4,
	eSteamRDKFMasterServer = 5,
	eMaxServerTypes = 6
} ESteamServerType;

/******************************************************************************
**
** More exported constants
**
******************************************************************************/


#ifdef __cplusplus

const SteamHandle_t										STEAM_INVALID_HANDLE = 0;
const SteamCallHandle_t									STEAM_INVALID_CALL_HANDLE = 0;
const SteamUserIDTicketValidationHandle_t				STEAM_INACTIVE_USERIDTICKET_VALIDATION_HANDLE = 0;
const unsigned int										STEAM_USE_LATEST_VERSION = 0xFFFFFFFF;

#else

#define STEAM_INVALID_HANDLE							((SteamHandle_t)(0))
#define STEAM_INVALID_CALL_HANDLE						((SteamCallHandle_t)(0))
#define	STEAM_INACTIVE_USERIDTICKET_VALIDATION_HANDLE	((SteamUserIDTicketValidationHandle_t)(0))
#define STEAM_USE_LATEST_VERSION						(0xFFFFFFFFu);

#endif


/******************************************************************************
** Each Steam instance (licensed Steam Service Provider) has a unique SteamInstanceID_t.
**
** Each Steam instance as its own DB of users.
** Each user in the DB has a unique SteamLocalUserID_t (a serial number, with possible 
** rare gaps in the sequence).
** 
******************************************************************************/

typedef	unsigned short		SteamInstanceID_t;		// MUST be 16 bits


#if defined ( _WIN32 )
typedef	unsigned __int64	SteamLocalUserID_t;		// MUST be 64 bits
#else
typedef	unsigned long long	SteamLocalUserID_t;		// MUST be 64 bits
#endif

/******************************************************************************
**
** Applications need to be able to authenticate Steam users from ANY instance.
** So a SteamIDTicket contains SteamGlobalUserID, which is a unique combination of 
** instance and user id.
**
** SteamLocalUserID is an unsigned 64-bit integer.
** For platforms without 64-bit int support, we provide access via a union that splits it into 
** high and low unsigned 32-bit ints.  Such platforms will only need to compare LocalUserIDs 
** for equivalence anyway - not perform arithmetic with them.
** 
********************************************************************************/
typedef struct	
{
	unsigned int	Low32bits;
	unsigned int	High32bits;
}	TSteamSplitLocalUserID;

typedef struct
{
	SteamInstanceID_t		m_SteamInstanceID;

	union
	{
		SteamLocalUserID_t		As64bits;
		TSteamSplitLocalUserID	Split;
	}						m_SteamLocalUserID;

} TSteamGlobalUserID;


#ifdef __cplusplus
}
#endif


#endif
