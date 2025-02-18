//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef HTTP_H
#define HTTP_H

#ifdef _WIN32
#pragma once
#endif

//--------------------------------------------------------------------------------------------------------------
/**
 * Status of the download thread, as set in RequestContext::status.
 */
enum HTTPStatus_t
{
	HTTP_INVALID = -1,
	HTTP_CONNECTING = 0,///< This is set in the main thread before the download thread starts.
	HTTP_FETCH,			///< The download thread sets this when it starts reading data.
	HTTP_DONE,			///< The download thread sets this if it has read all the data successfully.
	HTTP_ABORTED,		///< The download thread sets this if it aborts because it's RequestContext::shouldStop has been set.
	HTTP_ERROR			///< The download thread sets this if there is an error connecting or downloading.  Partial data may be present, so the main thread can check.
};

//--------------------------------------------------------------------------------------------------------------
/**
 * Error encountered in the download thread, as set in RequestContext::error.
 */
enum HTTPError_t
{
	HTTP_ERROR_NONE = 0,
	HTTP_ERROR_ZERO_LENGTH_FILE,
	HTTP_ERROR_CONNECTION_CLOSED,
	HTTP_ERROR_INVALID_URL,			///< InternetCrackUrl failed
	HTTP_ERROR_INVALID_PROTOCOL,	///< URL didn't start with http:// or https://
	HTTP_ERROR_CANT_BIND_SOCKET,
	HTTP_ERROR_CANT_CONNECT,
	HTTP_ERROR_NO_HEADERS,			///< Cannot read HTTP headers
	HTTP_ERROR_FILE_NONEXISTENT,
	HTTP_ERROR_MAX
};

#endif	// HTTP_H
