//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shared object based on a CBaseRecord subclass
//
//=============================================================================

#ifndef PROTOBUFSHAREDOBJECT_H
#define PROTOBUFSHAREDOBJECT_H
#ifdef _WIN32
#pragma once
#endif

#include "google/protobuf/descriptor.h"
#include "tier1/KeyValues.h"



namespace google
{
	namespace protobuf
	{
		class Message;
	}
}

namespace GCSDK
{

bool IsProtoBufFieldLess( const ::google::protobuf::Message & msgLHS,
						  const ::google::protobuf::Message & msgRHS,
						  const ::google::protobuf::FieldDescriptor *pFieldLHS,
						  const ::google::protobuf::FieldDescriptor *pFieldRHS );

//----------------------------------------------------------------------------
// Purpose: Base class for CProtoBufSharedObject. This is where all the actual
//			code lives.
//----------------------------------------------------------------------------
class CProtoBufSharedObjectBase : public CSharedObject
{
public:
	typedef CSharedObject BaseClass;

	virtual bool BParseFromMessage( const CUtlBuffer & buffer ) OVERRIDE;
	virtual bool BParseFromMessage( const std::string &buffer ) OVERRIDE;
	virtual bool BUpdateFromNetwork( const CSharedObject & objUpdate ) OVERRIDE;

	virtual bool BIsKeyLess( const CSharedObject & soRHS ) const ;
	virtual void Copy( const CSharedObject & soRHS );
	virtual void Dump() const OVERRIDE;

#ifdef DBGFLAG_VALIDATE
	virtual void Validate( CValidator &validator, const char *pchName );
#endif


	// Static helpers
	static void Dump( const ::google::protobuf::Message & msg );
	static KeyValues *CreateKVFromProtoBuf( const ::google::protobuf::Message & msg );
	static void RecursiveAddProtoBufToKV( KeyValues *pKVDest, const ::google::protobuf::Message & msg );

protected:
	virtual ::google::protobuf::Message *GetPObject() = 0;
	const ::google::protobuf::Message *GetPObject() const { return const_cast<CProtoBufSharedObjectBase *>(this)->GetPObject(); }

private:

};


//----------------------------------------------------------------------------
// Purpose: Template for making a shared object that uses a specific protobuf
//			message class for its wire protocol and in-memory representation.
//----------------------------------------------------------------------------
template< typename Message_t, int nTypeID, bool bPublicMutable = true >
class CProtoBufSharedObject : public CProtoBufSharedObjectBase
{
public:
	~CProtoBufSharedObject()
	{
	}

	virtual int GetTypeID() const { return nTypeID; }

	// WHERE IS YOUR GOD NOW
	template< typename T >
		using Public_Message_t = typename std::enable_if< bPublicMutable && std::is_same< T, Message_t >::value, Message_t & >::type;
	template< typename T >
		using Protected_Message_t = typename std::enable_if< !bPublicMutable && std::is_same< T, Message_t >::value, Message_t & >::type;

	template< typename T = Message_t >
		Public_Message_t<T> Obj() { return m_msgObject; }

	const Message_t & Obj() const { return m_msgObject; }

	typedef Message_t SchObjectType_t;
	const static int k_nTypeID = nTypeID;
protected:
	template< typename T = Message_t >
		Protected_Message_t<T> MutObj() { return m_msgObject; }

	::google::protobuf::Message *GetPObject() { return &m_msgObject; }

private:
	Message_t m_msgObject;
};



//----------------------------------------------------------------------------
// Purpose: Template for making a shared object that uses a specific protobuf
//          message class for its wire protocol and in-memory representation.
//
//          The wrapper version of this class wraps a message allocated and
//          owned elsewhere. The user of this class is in charge of
//          guaranteeing that lifetime.
//----------------------------------------------------------------------------
template< typename Message_t, int nTypeID >
class CProtoBufSharedObjectWrapper : public CProtoBufSharedObjectBase
{
public:
	CProtoBufSharedObjectWrapper( Message_t *pMsgToWrap )
		: m_pMsgObject( pMsgToWrap )
	{}

	~CProtoBufSharedObjectWrapper()
	{
	}

	virtual int GetTypeID() const { return nTypeID; }

	Message_t & Obj() { return *m_pMsgObject; }
	const Message_t & Obj() const { return *m_pMsgObject; }

	typedef Message_t SchObjectType_t;
public:
	const static int k_nTypeID = nTypeID;

protected:
	::google::protobuf::Message *GetPObject() { return m_pMsgObject; }

private:
	Message_t *m_pMsgObject;
};

} // GCSDK namespace

#endif //PROTOBUFSHAREDOBJECT_H
