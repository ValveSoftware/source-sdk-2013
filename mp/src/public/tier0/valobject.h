//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:	CValObject is used for tracking individual objects that report
//			in to CValidator.  Whenever a new object reports in (via CValidator::Push),
//			we create a new CValObject to aggregate stats for it.
//
// $NoKeywords: $
//=============================================================================//

#ifndef VALOBJECT_H
#define VALOBJECT_H
#ifdef _WIN32
#pragma once
#endif


#ifdef DBGFLAG_VALIDATE
class CValObject
{
public:
	// Constructors & destructors
	CValObject( void ) { };
	~CValObject( void );

	void Init( tchar *pchType, void *pvObj, tchar *pchName, CValObject *pValObjectParent,
		CValObject *pValObjectPrev );

	// Our object has claimed ownership of a memory block
	void ClaimMemoryBlock( void *pvMem );

	// A child of ours has claimed ownership of a memory block
	void ClaimChildMemoryBlock( int cubUser );

	// Accessors
	tchar *PchType( void ) { return m_rgchType; };
	void *PvObj( void ) { return m_pvObj; };
	tchar *PchName( void ) { return m_rgchName; };
	CValObject *PValObjectParent( void ) { return m_pValObjectParent; };
	int NLevel( void ) { return m_nLevel; };
	CValObject *PValObjectNext( void ) { return m_pValObjectNext; };
	int CpubMemSelf( void ) { return m_cpubMemSelf; };
	int CubMemSelf( void ) { return m_cubMemSelf; };
	int CpubMemTree( void ) { return m_cpubMemTree; };
	int CubMemTree( void ) { return m_cubMemTree; };
	int NUser( void ) { return m_nUser; };
	void SetNUser( int nUser ) { m_nUser = nUser; };
	void SetBNewSinceSnapshot( bool bNewSinceSnapshot ) { m_bNewSinceSnapshot = bNewSinceSnapshot; }
	bool BNewSinceSnapshot( void ) { return m_bNewSinceSnapshot; }

private:
	bool  m_bNewSinceSnapshot;				// If this block is new since the snapshot.
	tchar m_rgchType[64];					// Type of the object we represent
	tchar m_rgchName[64];					// Name of this particular object
	void *m_pvObj;							// Pointer to the object we represent

	CValObject *m_pValObjectParent;			// Our parent object in the tree.
	int m_nLevel;							// Our depth in the tree

	CValObject *m_pValObjectNext;			// Next ValObject in the linked list

	int m_cpubMemSelf;						// # of memory blocks we own directly
	int m_cubMemSelf;						// Total size of the memory blocks we own directly

	int m_cpubMemTree;						// # of memory blocks owned by us and our children
	int m_cubMemTree;						// Total size of the memory blocks owned by us and our children

	int m_nUser;							// Field provided for use by our users
};
#endif // DBGFLAG_VALIDATE


#endif // VALOBJECT_H
