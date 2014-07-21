//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef WCKEYVALUES_H
#define WCKEYVALUES_H
#pragma once


#include <tier0/dbg.h>
#include <utlvector.h>
#include <utldict.h>
#pragma warning(push, 1)
#pragma warning(disable:4701 4702 4530)
#include <fstream>
#pragma warning(pop)


#define KEYVALUE_MAX_KEY_LENGTH			80
#define KEYVALUE_MAX_VALUE_LENGTH		512


class MDkeyvalue 
{
	public:

		//
		// Constructors/Destructor.
		//
		inline MDkeyvalue(void);
		inline MDkeyvalue(const char *pszKey, const char *pszValue);
		~MDkeyvalue(void);

		MDkeyvalue &operator =(const MDkeyvalue &other);
		
		inline void Set(const char *pszKey, const char *pszValue);
		inline const char *Key(void) const;
		inline const char *Value(void) const;

		//
		// Serialization functions.
		//
		int SerializeRMF(std::fstream &f, BOOL bRMF);
		int SerializeMAP(std::fstream &f, BOOL bRMF);

		char szKey[KEYVALUE_MAX_KEY_LENGTH];			// The name of this key.
		char szValue[KEYVALUE_MAX_VALUE_LENGTH];		// The value of this key, stored as a string.
};


//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
MDkeyvalue::MDkeyvalue(void)
{
	szKey[0] = '\0';
	szValue[0] = '\0';
}


//-----------------------------------------------------------------------------
// Purpose: Constructor with assignment.
//-----------------------------------------------------------------------------
MDkeyvalue::MDkeyvalue(const char *pszKey, const char *pszValue)
{
	szKey[0] = '\0';
	szValue[0] = '\0';

	Set(pszKey, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Assigns a key and value.
//-----------------------------------------------------------------------------
void MDkeyvalue::Set(const char *pszKey, const char *pszValue)
{
	Assert(pszKey);
	Assert(pszValue);

	strcpy(szKey, pszKey);
	strcpy(szValue, pszValue);
}


//-----------------------------------------------------------------------------
// Purpose: Returns the string keyname.
//-----------------------------------------------------------------------------
const char *MDkeyvalue::Key(void) const
{
	return szKey;
}


//-----------------------------------------------------------------------------
// Purpose: Returns the string value of this keyvalue.
//-----------------------------------------------------------------------------
const char *MDkeyvalue::Value(void) const
{
	return szValue;
}


typedef CUtlVector<MDkeyvalue> KeyValueArray;


// Used in cases where there can be duplicate key names.
class WCKVBase_Vector
{
public:
	
	// Iteration helpers.
	inline int GetCount() const			{ return m_KeyValues.Count(); }
	inline int GetFirst() const			{ return m_KeyValues.Count() - 1; }
	inline int GetNext( int i ) const	{ return i - 1; }
	static inline int GetInvalidIndex()	{ return -1; }

	void RemoveKeyAt(int nIndex);
	int FindByKeyName( const char *pKeyName ) const; // Returns the same value as GetInvalidIndex if not found.

	// Special function used for non-unique keyvalue lists.
	void AddKeyValue(const char *pszKey, const char *pszValue);

protected:

	void InsertKeyValue( const MDkeyvalue &kv );

protected:
	CUtlVector<MDkeyvalue> m_KeyValues;
};

// Used for most key/value sets because it's fast. Does not allow duplicate key names.
class WCKVBase_Dict
{
public:

	// Iteration helpers. Note that there is no GetCount() because you can't iterate
	// these by incrementing a counter.
	inline int GetFirst() const			{ return m_KeyValues.First(); }
	inline int GetNext( int i ) const	{ return m_KeyValues.Next( i ); }
	static inline int GetInvalidIndex()	{ return CUtlDict<MDkeyvalue,unsigned short>::InvalidIndex(); }

	int FindByKeyName( const char *pKeyName ) const; // Returns the same value as GetInvalidIndex if not found.
	void RemoveKeyAt(int nIndex);

protected:
	void InsertKeyValue( const MDkeyvalue &kv );

protected:
	CUtlDict<MDkeyvalue,unsigned short> m_KeyValues;
};


// See below for typedefs of this class you can use.
template<class Base>
class WCKeyValuesT : public Base
{
public:

	WCKeyValuesT(void);
	~WCKeyValuesT(void);

	void RemoveAll(void);
	void RemoveKey(const char *pszKey);

	void SetValue(const char *pszKey, const char *pszValue);
	void SetValue(const char *pszKey, int iValue);

	const char *GetKey(int nIndex) const;
	MDkeyvalue &GetKeyValue(int nIndex);
	const MDkeyvalue& GetKeyValue(int nIndex) const;
	const char *GetValue(int nIndex) const;
	const char *GetValue(const char *pszKey, int *piIndex = NULL) const;
};


// These have explicit template instantiations so you can use them.
typedef WCKeyValuesT<WCKVBase_Dict> WCKeyValues;
typedef WCKeyValuesT<WCKVBase_Vector> WCKeyValuesVector;


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
template<class Base>
inline const char *WCKeyValuesT<Base>::GetKey(int nIndex) const
{
	return(m_KeyValues.Element(nIndex).szKey);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
// Output : MDKeyValue
//-----------------------------------------------------------------------------
template<class Base>
inline MDkeyvalue &WCKeyValuesT<Base>::GetKeyValue(int nIndex)
{
	return(m_KeyValues.Element(nIndex));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
// Output : MDkeyvalue
//-----------------------------------------------------------------------------
template<class Base>
inline const MDkeyvalue& WCKeyValuesT<Base>::GetKeyValue(int nIndex) const
{
	return(m_KeyValues.Element(nIndex));
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//-----------------------------------------------------------------------------
template<class Base>
inline const char *WCKeyValuesT<Base>::GetValue(int nIndex) const
{
	return(m_KeyValues.Element(nIndex).szValue);
}


void StripEdgeWhiteSpace(char *psz);


#endif // WCKEYVALUES_H
