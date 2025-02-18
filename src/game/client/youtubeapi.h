//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: API to interface with YouTube via libcurl and OpenSSL
//
// $NoKeywords: $
//=============================================================================

#ifndef YOUTUBEAPI_H
#define YOUTUBEAPI_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"

class CUtlString;

enum eYouTubeLoginStatus
{
	kYouTubeLogin_NotLoggedIn,
	kYouTubeLogin_LoggedIn,
	kYouTubeLogin_Pending,
	kYouTubeLogin_CouldNotConnect,
	kYouTubeLogin_Forbidden,
	kYouTubeLogin_GenericFailure,
	kYouTubeLogin_Cancelled,
};

enum eYouTubeAccessControl
{
	kYouTubeAccessControl_Public,
	kYouTubeAccessControl_Private,
	kYouTubeAccessControl_Unlisted,
};

DECLARE_POINTER_HANDLE(YouTubeUploadHandle_t);
DECLARE_POINTER_HANDLE(YouTubeInfoHandle_t);

/**
 * Interface to general response handler for YouTube requests
 */
class CYouTubeResponseHandler
{
public:

	/**
	 * Invoked in the main thread after a response has been fully received.
	 * @param pResponse
	 */
	virtual void HandleResponse( long responseCode, const char *pResponse ) = 0;
};


/**
 * Set application specific developer settings, which must be set before trying to log in the user or upload a video
 * @param pDeveloperKey
 * @param pDeveloperTag
 */
void YouTube_SetDeveloperSettings( const char *pDeveloperKey, const char *pDeveloperTag );

/**
 * Attempt to log the user in over SSL
 * @param pUserName
 * @param pPassword
 * @param pSource
 */
void YouTube_Login( const char *pUserName, const char *pPassword, const char *pSource );

/**
 * Cancel the login process.
 */
void YouTube_LoginCancel();

/**
 * @return eYouTubeLoginStatus
 */
eYouTubeLoginStatus YouTube_GetLoginStatus();

/**
 * @return YouTube login name
 */
const char *YouTube_GetLoginName();

/**
 * @return the URL to the YouTube profile, if the user is logged in
 */
bool YouTube_GetProfileURL( CUtlString &strProfileURL );

/**
 * Attempt to upload a movie file to YouTube
 * @param pFilePath full path to the file
 * @param pMimeType i.e. "video/mp4" 
 * @param pTitle (must be less than 60 characters)
 * @param pDescription
 * @param pCategory - usually "Games" (see category terms in http://gdata.youtube.com/schemas/2007/categories.cat)
 * @param pKeywords
 * @param access
 * @param pURLToVideo if the upload was successful, this string will be a URL to the video on YouTube
 * @return true if the video was uploaded successfully, false otherwise
 */
YouTubeUploadHandle_t YouTube_Upload( const char* pFilePath, const char *pMimeType, const char *pTitle, const char *pDescription, const char *pCategory, const char *pKeywords, eYouTubeAccessControl access );

/**
 * @param handle
 * @return true if upload is finished, false otherwise
 */
bool YouTube_IsUploadFinished( YouTubeUploadHandle_t handle );

/**
 * Get the progress of the upload
 * @param handle
 * @param flPercentage the parameter to be filled in
 * @return true if the upload is still valid, false otherwise
 */
bool YouTube_GetUploadProgress( YouTubeUploadHandle_t handle, double &ultotal, double &ulnow );

/**
 * @param handle
 * @param bSuccess
 * @param strURLToVideo
 */
bool YouTube_GetUploadResults( YouTubeUploadHandle_t handle, bool &bSuccess, CUtlString &strURLToVideo, CUtlString &strURLToVideoStats );

/**
 * Clear status of the upload
 * @param handle
 */
void YouTube_ClearUploadResults( YouTubeUploadHandle_t handle );

/**
 * Cancel the upload of a video
 * @param handle
 */
void YouTube_CancelUpload( YouTubeUploadHandle_t handle );

/**
 * Asynchronously retrieve information for the given video.
 * @param pURLToVideoStats
 * @param responseHandler
 */
YouTubeInfoHandle_t YouTube_GetVideoInfo( const char *pURLToVideoStats, CYouTubeResponseHandler &responseHandler );

/**
 * @param handle
 */
void YouTube_CancelGetVideoInfo( YouTubeInfoHandle_t handle );

#endif // YOUTUBEAPI_H
