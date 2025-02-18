//========= Copyright © 1996-2010, Valve LLC, All rights reserved. ============
//
// Purpose: Header for CWebAPIResponse objects
//
//=============================================================================

#ifndef WEBAPI_RESPONSE_H
#define WEBAPI_RESPONSE_H
#ifdef _WIN32
#pragma once
#endif

#include "bufferpool.h"

namespace GCSDK
{

enum EWebAPIOutputFormat
{
	k_EWebAPIOutputFormat_JSON = 1,
	k_EWebAPIOutputFormat_XML = 2,
	k_EWebAPIOutputFormat_VDF = 3,
	k_EWebAPIOutputFormat_ParameterEncoding = 4,
};

enum EWebAPIValueType
{
	// Object is the initial value
	k_EWebAPIValueType_Object		= 0,
	k_EWebAPIValueType_Int32		= 1,
	k_EWebAPIValueType_Int64		= 2,
	k_EWebAPIValueType_UInt32		= 3,
	k_EWebAPIValueType_UInt64		= 4,
	k_EWebAPIValueType_Double		= 5,
	k_EWebAPIValueType_String		= 6,
	k_EWebAPIValueType_BinaryBlob	= 7,
	k_EWebAPIValueType_Bool			= 8,
	k_EWebAPIValueType_Null			= 9,
	k_EWebAPIValueType_NumericArray	= 10,
};

class CWebAPIValues;

class CWebAPIResponse
{
public:
	CWebAPIResponse();
	~CWebAPIResponse();

	// Set the HTTP status code for the response
	void SetStatusCode( EHTTPStatusCode eStatusCode ) { m_eStatusCode = eStatusCode; }

	// Set how many seconds until this response expires
	void SetExpirationSeconds( uint32 unExpirationSeconds ) { m_unExpirationSeconds = unExpirationSeconds; }

	// Set when this response was last modified
	void SetLastModified( RTime32 rtLastModified ) { m_rtLastModified = rtLastModified; }

	// Get the status code for the response
	EHTTPStatusCode GetStatusCode() const { return m_eStatusCode; }

	// Get how many seconds until this response expires
	uint32 GetExpirationSeconds() const { return m_unExpirationSeconds; }

	// Get when the response was last modified
	RTime32 GetLastModified() const { return m_rtLastModified; }

	// extended arrays include their element name as an object in JSON and VDF output formats
	void SetExtendedArrays( bool bExtendedArrays ) { m_bExtendedArrays = bExtendedArrays; }
	bool HasExtendedArrays() const { return m_bExtendedArrays; }

	// Outputs formatted data to buffer
	bool BEmitFormattedOutput( EWebAPIOutputFormat eFormat, CUtlBuffer &outputBuffer, size_t unMaxResultSize );

	// Resets the response to be empty
	void Clear();

	// Create the root value element in the response
	CWebAPIValues *CreateRootValue( const char *pchName );
	const CWebAPIValues *GetRootValue() const { return m_pValues; }
	CWebAPIValues *GetRootValue() { return m_pValues; }

	// Anonymous root nodes affect only JSON output and, when enabled, result in 
	// the omission of the extra set of { } braces and the name of the root node.
	//
	// So this JSON:
	// { "root" : { "key1" : "value1", "key2" : "value2" } }
	// becomes:
	// { "key1" : "value1", "key2" : "value2" }
	void SetJSONAnonymousRootNode( bool bAnonymousRootNode )	{ m_bJSONAnonymousRootNode = bAnonymousRootNode; }
	bool HasJSONAnonymousRootNode() const						{ return m_bJSONAnonymousRootNode; }

private:

	// Emits JSON formatted representation of response
	bool BEmitJSON( CUtlBuffer &outputBuffer, size_t unMaxResultSize );

	// Emits KeyValues .vdf style formatted representation of response
	bool BEmitVDF( CUtlBuffer &outputBuffer, size_t unMaxResultSize );

	// Emits XML formatted representation of response
	bool BEmitXML( CUtlBuffer &outputBuffer, size_t unMaxResultSize );

	// parameter encoding, as used in a lot of open standards
	bool BEmitParameterEncoding( CUtlBuffer &outputBuffer );

	CWebAPIValues *m_pValues;
	EHTTPStatusCode m_eStatusCode;
	uint32 m_unExpirationSeconds;
	RTime32 m_rtLastModified;
	bool m_bExtendedArrays;
	bool m_bJSONAnonymousRootNode;
};

class CWebAPIValues 
{
	// !FIXME! DOTAMERGE
	//DECLARE_CLASS_MEMPOOL_MT( CWebAPIValues );

private:
	void InitInternal( CWebAPIValues *pParent, int nNamePos, EWebAPIValueType eValueType, const char *pchArrayElementNames );
	CWebAPIValues( CWebAPIValues *pParent, int nNamePos, EWebAPIValueType eValueType, const char *pchArrayElementNames = NULL );
	CWebAPIValues( CWebAPIValues *pParent, const char *pchName, EWebAPIValueType eValueType, const char *pchArrayElementNames = NULL );

public:
	explicit CWebAPIValues( const char *pchName );
	CWebAPIValues( const char *pchName, const char *pchArrayElementNames );

	~CWebAPIValues();


	//
	// Child node handling
	//

	// Create a child object of this node, all children of the resultant
	// object must be named.
	CWebAPIValues *CreateChildObject( const char *pchName );

	// Return an existing child object - otherwise create one and return that.
	CWebAPIValues *FindOrCreateChildObject( const char *pchName );

	// Add a child object to the array, this should only be called on objects that are of the array type
	CWebAPIValues *AddChildObjectToArray();

	// Add a child array to the  array, this should only be called on objects that are of the array type
	CWebAPIValues *AddChildArrayToArray( const char * pchArrayElementNames );

	// Create a  child array of this node.  Note that array nodes can only
	// have un-named children, in XML the pchArrayElementNames value will be used
	// as the element name for each of the children of the array, in JSON it will simply
	// be a numerically indexed [] array.
	CWebAPIValues *CreateChildArray( const char *pchName, const char *pchArrayElementNames );

	// Find first matching child by name, O(N) on number of children, this class isn't designed for searching
	CWebAPIValues * FindChild( const char *pchName );
	const CWebAPIValues * FindChild( const char *pchName ) const { return const_cast<CWebAPIValues *>(this)->FindChild( pchName ); }

	// Get the first child of this node
	CWebAPIValues * GetFirstChild();
	const CWebAPIValues * GetFirstChild() const { return m_pFirstChild; }

	// Call this on the returned value from GetFirstChild() or a previous GetNextChild() call to
	// proceed to the next child of the parent GetFirstChild() was originally called on.
	CWebAPIValues * GetNextChild();
	const CWebAPIValues * GetNextChild() const { return m_pNextPeer; }

	// Returns the parent of this node or NULL if this is the root of a tree
	CWebAPIValues * GetParent();
	const CWebAPIValues * GetParent() const { return m_pParent; }

	// Deletes the child with the given name - no-op if no child by that name
	void DeleteChild( const char *pchName );

	//
	// Setters
	//

	// Set string value
	void SetStringValue( const char *pchValue );

	// Set int32 value
	void SetInt32Value( int32 nValue );

	// Set uint32 value
	void SetUInt32Value( uint32 unValue );

	// Set int64 value
	void SetInt64Value ( int64 lValue );

	// Set uint64 value
	void SetUInt64Value( uint64 ulValue );

	// Set double value
	void SetDoubleValue( double flValue );

	// Set binary blob value
	void SetBinaryValue( const uint8 *pValue, uint32 unBytes );

	// Set boolean value
	void SetBoolValue( bool bValue );

	// Set boolean value
	void SetNullValue( );

	//
	// Accessors
	//

	// Get the name of the current node
	const char *GetName() const { return m_nNamePos >= 0 ? ( (const char *)m_pStringBuffer->Base() + m_nNamePos ) : NULL; }

	// get the name of the elements of this numeric array (if this is an array)
	const char *GetElementName() const { return m_nArrayChildElementNamePos >= 0 ? ( (const char *)m_pStringBuffer->Base() + m_nArrayChildElementNamePos ) : NULL; }

	// Get the type currently held by the node
	EWebAPIValueType GetType() const;

	// returns true if this is an object
	bool IsObject() const { return GetType() == k_EWebAPIValueType_Object; }

	// returns true if this is an object
	bool IsArray() const { return GetType() == k_EWebAPIValueType_NumericArray; }

	// Get int32 value
	int32 GetInt32Value() const;

	// Get uint32 value
	uint32 GetUInt32Value() const;

	// Get int64 value
	int64 GetInt64Value() const;

	// Get uint64 value
	uint64 GetUInt64Value() const;

	// Get double value
	double GetDoubleValue() const;

	// Get string value
	void GetStringValue( CUtlString &stringOut ) const;

	// Get binary blob value
	void GetBinaryValue( CUtlBuffer &bufferOut ) const;

	// Get bool value
	bool GetBoolValue() const;

	// Get Null value
	bool IsNullValue() const { return GetType() == k_EWebAPIValueType_Null; }

	//
	// Child Setters
	//

	// Set string value
	void SetChildStringValue( const char *pchChildName, const char *pchValue );

	// Set int32 value
	void SetChildInt32Value( const char *pchChildName, int32 nValue );

	// Set uint32 value
	void SetChildUInt32Value( const char *pchChildName, uint32 unValue );

	// Set int64 value
	void SetChildInt64Value ( const char *pchChildName, int64 lValue );

	// Set uint64 value
	void SetChildUInt64Value( const char *pchChildName, uint64 ulValue );

	// Set double value
	void SetChildDoubleValue( const char *pchChildName, double flValue );

	// Set binary blob value
	void SetChildBinaryValue( const char *pchChildName, const uint8 *pValue, uint32 unBytes );

	// Set boolean value
	void SetChildBoolValue( const char *pchChildName, bool bValue );

	// Set null value
	void SetChildNullValue( const char *pchChildName );

	//
	// Accessors
	//

	// Get int32 value
	int32 GetChildInt32Value( const char *pchChildName, int32 nDefault = 0 ) const;

	// Get uint32 value
	uint32 GetChildUInt32Value( const char *pchChildName, uint32 unDefault = 0 ) const;

	// Get int64 value
	int64 GetChildInt64Value( const char *pchChildName, int64 lDefault = 0 ) const;

	// Get uint64 value
	uint64 GetChildUInt64Value( const char *pchChildName, uint64 ulDefault = 0 ) const;

	// Get double value
	double GetChildDoubleValue( const char *pchChildName, double flDefault = 0 ) const;

	// Get string value
	void GetChildStringValue( CUtlString &stringOut, const char *pchChildName, const char *pchDefault ) const;

	// Get binary blob value (returns false if the child wasn't found)
	bool BGetChildBinaryValue( CUtlBuffer &bufferOut, const char *pchChildName ) const;

	// Get bool value
	bool GetChildBoolValue( const char *pchChildName, bool bDefault = false ) const;

	// get null value
	bool IsChildNullValue( const char *pchChildName ) const;

	//
	// Output methods
	//

	// Emits JSON formatted representation of response
	static bool BEmitJSONRecursive( const CWebAPIValues *pCurrent, CUtlBuffer &outputBuffer, int nTabLevel, size_t unMaxResultSize, bool bIncludeArrayElementName = true );

	// Emits KeyValues .vdf style formatted representation of response
	static bool BEmitVDFRecursive( const CWebAPIValues *pCurrent, CUtlBuffer &outputBuffer, int nTabLevel, uint32 nArrayElement, size_t unMaxResultSize, bool bIncludeArrayElementName = true );

	// Emits XML formatted representation of response
	static bool BEmitXMLRecursive( const CWebAPIValues *pCurrent, CUtlBuffer &outputBuffer,int nTabLevel, size_t unMaxResultSize );

	//
	// Parsing methods
	// 

	// parses JSON into a tree of CWebAPIValues nodes. 
	static CWebAPIValues * ParseJSON( CUtlBuffer &inputBuffer );
	static CWebAPIValues * ParseJSON( const char *pchJSONString );

	//
	// Utility methods
	//

	// copies the children and type from the specified node into this node
	void CopyFrom( const CWebAPIValues *pSource );

private:

	// sets the name of the node when constructing or copying
	void SetName( const char * pchName );

	// Clears any existing value, freeing memory if needed
	void ClearValue();

	// Assert that we don't have any child nodes, this is used when setting a native type value.  We don't
	// support having both our own value and children.  You are either an array of more values, or you are a value yourself.
	void AssertNoChildren();

	// Internal helper for creating children 
	CWebAPIValues *CreateChildInternal( const char *pchName, EWebAPIValueType eValueType, const char *pchArrayElementNames = NULL );

	// Name of this node
	int32 m_nNamePos;

	// Data value contained in this node
	EWebAPIValueType m_eValueType;

	struct WebAPIBinaryValue_t
	{
		int32  m_nDataPos;
		uint32 m_unBytes;
	};

	union
	{
		int32 m_nValue;
		int64 m_lValue;
		uint32 m_unValue;
		uint64 m_ulValue;
		double m_flValue;
		int32 m_nStrValuePos;
		bool m_bValue;
		int32 m_nArrayChildElementNamePos;
		WebAPIBinaryValue_t m_BinaryValue;
	};
	
	CWebAPIValues * m_pFirstChild;
	CWebAPIValues * m_pLastChild;
	CWebAPIValues * m_pNextPeer;
	CWebAPIValues * m_pParent;

	CUtlBuffer *m_pStringBuffer;
};

#define FOR_EACH_WEBAPI_CHILD( pParentParam, pChildParam ) \
	for( CWebAPIValues *pChildParam = pParentParam->GetFirstChild(); pChildParam != NULL; pChildParam = pChildParam->GetNextChild() )

}

#include "tier0/memdbgon.h"

namespace GCSDK
{

//-----------------------------------------------------------------------------
// Purpose: KeyValues wrapper that automatically deletes itself on close
//-----------------------------------------------------------------------------
class CWebAPIValuesAD
{
public:
	CWebAPIValuesAD()
	{
		m_pwav = NULL;
	}

	// create a webapivalues object of the object type
	CWebAPIValuesAD( const char *pchName )
	{
		m_pwav = new CWebAPIValues( pchName );
	}

	// create a webapivalues object of the array type
	CWebAPIValuesAD( const char *pchName, const char *pchArrayElementName )
	{
		m_pwav = new CWebAPIValues( pchName, pchArrayElementName );
	}

	CWebAPIValuesAD( const CWebAPIValuesAD &rhs )
	{
		m_pwav = NULL;
		Copy( rhs.m_pwav );
	}

	CWebAPIValuesAD( const CWebAPIValues *pwav )
	{
		m_pwav = NULL;
		Copy( pwav );
	}

	~CWebAPIValuesAD()
	{
		delete m_pwav;
	}

	CWebAPIValues *operator->()	{ if ( !m_pwav ) m_pwav = new CWebAPIValues( "root" ); return m_pwav; }
	operator CWebAPIValues *()	{ if ( !m_pwav ) m_pwav = new CWebAPIValues( "root" ); return m_pwav; }
	operator const CWebAPIValues *() const { return m_pwav;  }

	CWebAPIValuesAD & operator= ( const CWebAPIValuesAD &rhs )
	{
		Copy( rhs.m_pwav );
		return *this;
	}

	void Take( CWebAPIValues *pwav )
	{
		if ( pwav )
		{
			delete m_pwav;
			m_pwav = pwav;
		}
		else if ( m_pwav )
		{
			delete m_pwav;
			m_pwav = NULL;
		}
	}

	void Copy( const CWebAPIValues *pwav )
	{
		if ( m_pwav )
			delete m_pwav;

		if ( pwav )
		{
			if( pwav->IsArray() )
				m_pwav = new CWebAPIValues( pwav->GetName(), pwav->GetElementName() );
			else
				m_pwav = new CWebAPIValues( pwav->GetName() );
			m_pwav->CopyFrom( pwav );
		}
		else
			m_pwav = NULL;

	}

private:
	CWebAPIValues *operator=(CWebAPIValues *);	// use Take() or Copy()
	CWebAPIValues *m_pwav;
};

// use to decode binary values
bool Base64Decode( const char *pchData, uint32 cchDataMax, uint8 *pubDecodedData, uint32 *pcubDecodedData, bool bIgnoreInvalidCharacters );
bool Base64Encode( const uint8 *pubData, uint32 cubData, char *pchEncodedData, uint32 *pcchEncodedData, const char *pszLineBreak = NULL );
uint32 Base64EncodeMaxOutput( const uint32 cubData, const char *pszLineBreak = NULL );

inline bool Base64EncodeIntoUTLMemory( const uint8 *pubData, uint32 cubData, CUtlMemory<char>& out_pBuffer )
{
	// Stomp buffer and pre-allocate space.
	out_pBuffer.EnsureCapacity( Base64EncodeMaxOutput( cubData ) );

	// Perform encode, swallowing the used space output.
	uint32 unBufferCapacity = out_pBuffer.Count();
	return Base64Encode( pubData, cubData, out_pBuffer.Base(), &unBufferCapacity );
}
	
}

namespace ProtoBufHelper
{
	bool RecursiveAddProtoBufToWebAPIValues( GCSDK::CWebAPIValues *pWebAPIRoot, const ::google::protobuf::Message & msg );
	bool ParseWebAPIValues( ::google::protobuf::Message & msg, const GCSDK::CWebAPIValues *pWebAPIRoot, bool bEnforceRequired = false );
}

#include "tier0/memdbgoff.h"

#endif // WEBAPI_RESPONSE_H
