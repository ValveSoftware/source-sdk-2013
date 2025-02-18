//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Read SMD and create DMX data
//
//=============================================================================


#ifndef DMSMDSERIALIZER_H
#define DMSMDSERIALIZER_H


#if defined( _WIN32 )
#pragma once
#endif


// Valve includes
#include "datamodel/idatamodel.h"
#include "tier1/utlbuffer.h"
#include "tier1/utlstring.h"
#include "tier1/utlvector.h"



//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CDmeDag;
class CDmeMesh;
class CPolygonData;


//-----------------------------------------------------------------------------
// Serialization class for SMD files
//-----------------------------------------------------------------------------
class CDmSmdSerializer : public IDmSerializer
{
public:
	enum Axis_t
	{
		X_AXIS = 0,
		Y_AXIS = 1,
		Z_AXIS = 2
	};

	CDmSmdSerializer()
	: m_bOptAutoStripPrefix( false )
	, m_bOptImportSkeleton( true )
	, m_bOptAnimation( false )
	, m_flFrameRate( 30.0f )
	{
		SetUpAxis( Z_AXIS );
	}

	// Inherited from IDMSerializer
	virtual const char *GetName() const { return "smd"; }
	virtual const char *GetDescription() const { return "VALVe SMD"; }
	virtual bool IsBinaryFormat() const { return false; }
	virtual bool StoresVersionInFile() const { return true; }
	virtual int GetCurrentVersion() const { return 1; }
	virtual bool Serialize( CUtlBuffer &buf, CDmElement *pRoot ) { return false; }	// No DMX -> SMD support

	virtual bool Unserialize(
		CUtlBuffer &utlBuf,
		const char *pszEncodingName,
		int nEncodingVersion,
		const char *pszSourceFormatName,
		int nSourceFormatVersion,
		DmFileId_t nDmFileId,
		DmConflictResolution_t nDmConflictResolution,
		CDmElement **ppDmRoot );

	// Methods used for importing (only should return non-NULL for serializers that return false from StoresVersionInFile)
	virtual const char *GetImportedFormat() const { return NULL; }
	virtual int GetImportedVersion() const { return 1; }

	// CDmSmdSerializer
	CDmElement *ReadSMD( const char *pszFilename, CDmeMesh **ppDmeMeshCreated = NULL );

	void SetUpAxis( Axis_t nUpAxis );
	Axis_t GetUpAxis() const { return m_nUpAxis; }

	void SetIsAnimation( bool bOptAnimation ) { m_bOptAnimation = bOptAnimation; }
	bool IsReadAnimation() const { return m_bOptAnimation; }

	void SetFrameRate( float flFrameRate ) { m_flFrameRate = MAX( 0.1f, flFrameRate ); }	// Don't allow 0 or negative frame rate
	float GetFrameRate() const { return m_flFrameRate; }

	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	struct SmdJoint_t
	{
		int m_nId;				// The id parsed from the SMD file
		int m_nActualId;		// The actual node id which is created after sorting and creating all joints in order with no gaps in numbering, corresponds to joitnIndex in DmeModel
		CUtlString m_sName;
		int m_nParentId;
		int m_nLineNumber;
		CDmeDag *m_pDmeDag;

		SmdJoint_t()
			: m_nId( -1 )
			, m_nActualId( -1 )
			, m_nParentId( -1 )
			, m_nLineNumber( -1 )
			, m_pDmeDag( NULL )
		{}

	};


	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	typedef CUtlMap< int, SmdJoint_t > SmdJointMap_t;

protected:
	void ParserGetNodeName( const char *pszBuf, CUtlString &sName ) const;

	bool ParserHandleSkeletonLine(
		const char *pszBuf,
		CUtlString &sName,
		int &nId,
		int &nParentId ) const;

	CDmElement *CDmSmdSerializer::ReadSMD(
		CUtlBuffer &inUtlBuf,
		DmFileId_t nDmFileId,
		const char *pszFilename,
		CDmeMesh **ppDmeMeshCreated );

	
	//-----------------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------------
	class CNodeData
	{
	public:
		CNodeData()
		: m_nParentIndex( -1 )
		, m_bSkinned( false )
		, m_nInfluenceIndex( 0 )
		, m_pDmeDag( NULL )
		{
		}

		bool Valid() const
		{
			return m_pDmeDag != NULL;
		}

		void Reset()
		{
			m_pDmeDag = NULL;
		}

		int m_nParentIndex;
		bool m_bSkinned;
		int m_nInfluenceIndex;
		CDmeDag *m_pDmeDag;

		CUtlVector< Vector > m_positions;
	};

	void FixNodeName( CUtlString &sName ) const;

	void ParserSetJoint(
		const SmdJointMap_t &smdJointMap,
		int nFrame, int nId,
		const Vector &vPosition, const RadianEuler &eRadianEulerXYZ,
		const char *pszFilename, int nLineNumber );

	Axis_t m_nUpAxis;				// 0 == X, 1 == Y, 2 == Z
	matrix3x4_t m_mAdj;				// Matrix to adjust for SMD source orientation to DMX Y up
	matrix3x4_t m_mAdjNormal;		// Matrix to adjust normals, inverse transpose of m_mAdj

public:
	bool m_bOptImportSkeleton;
	bool m_bOptAutoStripPrefix;
	bool m_bOptAnimation;
	float m_flFrameRate;
	CUtlString m_sNodeDelPrefix;
	CUtlString m_sNodeAddPrefix;

};


#endif // DMSMDSERIALIZER_H
