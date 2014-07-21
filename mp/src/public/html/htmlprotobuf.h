//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//=============================================================================//

#ifndef HTML_PROTOBUF
#define HTML_PROTOBUF

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"
#include "html/htmlmessages.h"

namespace google
{
	namespace protobuf
	{
		class MessageLite;
	}
}

class CHTMLBaseProtoBufMsg
{
public:
	void SerializeCrossProc( CUtlBuffer *pBuffer ) const;
	bool BDeserializeCrossProc( CUtlBuffer *pBuffer );

protected:
	void *m_pMsg;
	bool m_bIsValid;
};


//-----------------------------------------------------------------------------
// Purpose: Base class for protobuf objects
//-----------------------------------------------------------------------------
template< typename PB_OBJECT_TYPE > 
class CHTMLProtoBufMsg : public CHTMLBaseProtoBufMsg
{
public:
	CHTMLProtoBufMsg( EHTMLCommands eMsg )
	{
		m_pMsg = new PB_OBJECT_TYPE;
		m_bIsValid = true;
	}

	// Construct and deserialize in one
	CHTMLProtoBufMsg( CUtlBuffer *pBuffer )
	{
		m_pMsg = NULL;
		m_bIsValid = BDeserializeCrossProc( pBuffer );
	}

	// Destructor
	virtual ~CHTMLProtoBufMsg()
	{
		delete (PB_OBJECT_TYPE *)m_pMsg;
	}

	bool BIsValid() { return m_bIsValid; }

	// Accessors
	PB_OBJECT_TYPE &Body() { return *((PB_OBJECT_TYPE*)( (google::protobuf::MessageLite *)m_pMsg )); }
	const PB_OBJECT_TYPE &BodyConst() const { return *((const PB_OBJECT_TYPE*)( (google::protobuf::MessageLite *)m_pMsg )); }

};


#endif // HTML_PROTOBUF
