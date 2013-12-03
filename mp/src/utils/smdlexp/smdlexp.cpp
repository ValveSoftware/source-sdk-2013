//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "MAX.H"
#include "DECOMP.H"
#include "STDMAT.H"
#include "ANIMTBL.H"
#include "istdplug.h"
#include "phyexp.h"
#include "BonesPro.h"

#include "smexprc.h"
#include "smedefs.h"

//===================================================================
// Prototype declarations
//
int GetIndexOfINode(INode *pnode,BOOL fAssertPropExists = TRUE);
void SetIndexOfINode(INode *pnode, int inode);
BOOL FUndesirableNode(INode *pnode);
BOOL FNodeMarkedToSkip(INode *pnode);
float FlReduceRotation(float fl);


//===================================================================
// Global variable definitions
//

// Save for use with dialogs
static HINSTANCE hInstance;

// We just need one of these to hand off to 3DSMAX.
static SmdExportClassDesc SmdExportCD;

// For OutputDebugString and misc sprintf's
static char st_szDBG[300];

// INode mapping table
static int g_inmMac = 0;

//===================================================================
// Utility functions
//

static int AssertFailedFunc(char *sz)
{
	MessageBox(GetActiveWindow(), sz, "Assert failure", MB_OK);
	int Set_Your_Breakpoint_Here = 1;
	return 1;
}
#define ASSERT_MBOX(f, sz) ((f) ? 1 : AssertFailedFunc(sz))


//===================================================================
// Required plug-in export functions
//
BOOL WINAPI DllMain( HINSTANCE hinstDLL, ULONG fdwReason, LPVOID lpvReserved) 
{	
	static int fFirstTimeHere = TRUE;
	if (fFirstTimeHere)
	{
		fFirstTimeHere = FALSE;
		hInstance = hinstDLL;
	}
	return TRUE;
}

	
EXPORT_THIS int LibNumberClasses(void)
{
	return 1;
}

	
EXPORT_THIS ClassDesc *LibClassDesc(int iWhichClass)
{
	switch(iWhichClass)
	{
		case 0: return &SmdExportCD;
		default: return 0;
	}
}

	
EXPORT_THIS const TCHAR *LibDescription()
{
	return _T("Valve SMD Plug-in.");
}

	
EXPORT_THIS ULONG LibVersion()
{
	return VERSION_3DSMAX;
}


//=====================================================================
// Methods for SmdExportClass
//

CONSTRUCTOR SmdExportClass::SmdExportClass(void)
{
	m_rgmaxnode		= NULL;
}


DESTRUCTOR SmdExportClass::~SmdExportClass(void)
{
	if (m_rgmaxnode)
		delete[] m_rgmaxnode;
}


int SmdExportClass::DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts, DWORD options) 
{
	ExpInterface	*pexpiface = ei;	// Hungarian
	Interface		*piface = i;		// Hungarian
	
	// Reset the name-map property manager
	g_inmMac = 0;

	if (!suppressPrompts)
	{
		// Present the user with the Export Options dialog
		if (DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_EXPORTOPTIONS), GetActiveWindow(),
							ExportOptionsDlgProc, (LPARAM)this) <= 0)
			return 0;		// error or cancel
	}
	else
	{
		m_fReferenceFrame = 0;
	}

	// Break up filename, re-assemble longer versions
	TSTR strPath, strFile, strExt;
	TCHAR szFile[MAX_PATH];
	SplitFilename(TSTR(name), &strPath, &strFile, &strExt);
		sprintf(szFile,  "%s\\%s.%s",  (char*)strPath, (char*)strFile, DEFAULT_EXT);

	/*
	if (m_fReferenceFrame)
		sprintf(szFile,  "%s\\%s_model.%s",  (char*)strPath, (char*)strFile, DEFAULT_EXT);
	*/

	FILE *pFile;
	if ((pFile = fopen(szFile, "w")) == NULL)
		return FALSE/*failure*/;

	fprintf( pFile, "version %d\n", 1 );

	// Get animation metrics
	m_intervalOfAnimation = piface->GetAnimRange();
	m_tvStart = m_intervalOfAnimation.Start();
	m_tvEnd = m_intervalOfAnimation.End();
	m_tpf = ::GetTicksPerFrame();

	// Count nodes, label them, collect into array
	if (!CollectNodes(pexpiface))
		return 0;	/*fail*/
	
	// Output nodes
	if (!DumpBones(pFile, pexpiface))
	{
		fclose( pFile );
		return 0;	/*fail*/
	}

	// Output bone rotations, for each frame. Do only first frame if this is the reference frame MAX file
	DumpRotations(pFile, pexpiface);

	// Output triangle meshes (first frame/all frames), if this is the reference frame MAX file
	if (m_fReferenceFrame)
	{
		DumpModel(pFile, pexpiface);
	}

	if (!suppressPrompts)
	{
		// Tell user that exporting is finished (it can take a while with no feedback)
		char szExportComplete[300];
		sprintf(szExportComplete, "Exported %s.", szFile);
		MessageBox(GetActiveWindow(), szExportComplete, "Status", MB_OK);
	}

	fclose( pFile );

	return 1/*success*/;
}
	
	
BOOL SmdExportClass::CollectNodes( ExpInterface *pexpiface)
{
	// Count total nodes in the model, so I can alloc array
	// Also "brands" each node with node index, or with "skip me" marker.
	CountNodesTEP procCountNodes;
	procCountNodes.m_cNodes = 0;
	(void) pexpiface->theScene->EnumTree(&procCountNodes);
	ASSERT_MBOX(procCountNodes.m_cNodes > 0, "No nodes!");

	// Alloc and fill array
	m_imaxnodeMac = procCountNodes.m_cNodes;
	m_rgmaxnode = new MaxNode[m_imaxnodeMac];
	ASSERT_MBOX(m_rgmaxnode != NULL, "new failed");


	CollectNodesTEP procCollectNodes;
	procCollectNodes.m_phec = this;
	(void) pexpiface->theScene->EnumTree(&procCollectNodes);
	
	return TRUE;
}


BOOL SmdExportClass::DumpBones(FILE *pFile, ExpInterface *pexpiface)
{
	// Dump bone names
	DumpNodesTEP procDumpNodes;
	procDumpNodes.m_pfile = pFile;
	procDumpNodes.m_phec = this;
	fprintf(pFile, "nodes\n" );
	(void) pexpiface->theScene->EnumTree(&procDumpNodes);
	fprintf(pFile, "end\n" );

	return TRUE;
}

	
BOOL SmdExportClass::DumpRotations(FILE *pFile, ExpInterface *pexpiface)
{
	// Dump bone-rotation info, for each frame
	// Also dumps root-node translation info (the model's world-position at each frame)
	DumpFrameRotationsTEP procDumpFrameRotations;
	procDumpFrameRotations.m_pfile	= pFile;
	procDumpFrameRotations.m_phec	= this;

	TimeValue m_tvTill = (m_fReferenceFrame) ? m_tvStart : m_tvEnd;

	fprintf(pFile, "skeleton\n" );
	for (TimeValue tv = m_tvStart; tv <= m_tvTill; tv += m_tpf)
	{
		fprintf(pFile, "time %d\n", tv / GetTicksPerFrame() );
		procDumpFrameRotations.m_tvToDump = tv;
		(void) pexpiface->theScene->EnumTree(&procDumpFrameRotations);
	}
	fprintf(pFile, "end\n" );

	return TRUE;
}
	
	
BOOL SmdExportClass::DumpModel( FILE *pFile, ExpInterface *pexpiface)
{
	// Dump mesh info: vertices, normals, UV texture map coords, bone assignments
	DumpModelTEP procDumpModel;
	procDumpModel.m_pfile	= pFile;
	procDumpModel.m_phec	= this;
	fprintf(pFile, "triangles\n" );
	procDumpModel.m_tvToDump = m_tvStart;
	(void) pexpiface->theScene->EnumTree(&procDumpModel);
	fprintf(pFile, "end\n" );
	return TRUE;
}	




//=============================================================================
//							TREE-ENUMERATION PROCEDURES
//=============================================================================

#define ASSERT_AND_ABORT(f, sz)							\
	if (!(f))											\
	{													\
		ASSERT_MBOX(FALSE, sz);							\
		cleanup( );										\
		return TREE_ABORT;								\
	}


//=================================================================
// Methods for CountNodesTEP
//
int CountNodesTEP::callback( INode *node)
{
	INode *pnode = node; // Hungarian

	ASSERT_MBOX(!(pnode)->IsRootNode(), "Encountered a root node!");

	if (::FUndesirableNode(pnode))
	{
		// Mark as skippable
		::SetIndexOfINode(pnode, SmdExportClass::UNDESIRABLE_NODE_MARKER);
		return TREE_CONTINUE;
	}
	
	// Establish "node index"--just ascending ints
	::SetIndexOfINode(pnode, m_cNodes);

	m_cNodes++;
	
	return TREE_CONTINUE;
}


//=================================================================
// Methods for CollectNodesTEP
//
int CollectNodesTEP::callback(INode *node)
{
	INode *pnode = node; // Hungarian

	ASSERT_MBOX(!(pnode)->IsRootNode(), "Encountered a root node!");

	if (::FNodeMarkedToSkip(pnode))
		return TREE_CONTINUE;

	// Get pre-stored "index"
	int iNode = ::GetIndexOfINode(pnode);
	ASSERT_MBOX(iNode >= 0 && iNode <= m_phec->m_imaxnodeMac-1, "Bogus iNode");
	
	// Get name, store name in array
	TSTR strNodeName(pnode->GetName());
	strcpy(m_phec->m_rgmaxnode[iNode].szNodeName, (char*)strNodeName);

	// Get Node's time-zero Transformation Matrices
	m_phec->m_rgmaxnode[iNode].mat3NodeTM		= pnode->GetNodeTM(0/*TimeValue*/);
	m_phec->m_rgmaxnode[iNode].mat3ObjectTM		= pnode->GetObjectTM(0/*TimeValue*/);

	// I'll calculate this later
	m_phec->m_rgmaxnode[iNode].imaxnodeParent	= SmdExportClass::UNDESIRABLE_NODE_MARKER;

	return TREE_CONTINUE;
}






//=================================================================
// Methods for DumpNodesTEP
//
int DumpNodesTEP::callback(INode *pnode)
{
	ASSERT_MBOX(!(pnode)->IsRootNode(), "Encountered a root node!");

	if (::FNodeMarkedToSkip(pnode))
		return TREE_CONTINUE;

	// Get node's parent
	INode *pnodeParent;
	pnodeParent = pnode->GetParentNode();
	
	// The model's root is a child of the real "scene root"
	TSTR strNodeName(pnode->GetName());
	BOOL fNodeIsRoot = pnodeParent->IsRootNode( );
	
	int iNode = ::GetIndexOfINode(pnode);
	int iNodeParent = ::GetIndexOfINode(pnodeParent, !fNodeIsRoot/*fAssertPropExists*/);

	// Convenient time to cache this
	m_phec->m_rgmaxnode[iNode].imaxnodeParent = fNodeIsRoot ? SmdExportClass::UNDESIRABLE_NODE_MARKER : iNodeParent;

	// Root node has no parent, thus no translation
	if (fNodeIsRoot)
		iNodeParent = -1;
		
	// check to see if the matrix isn't right handed
	m_phec->m_rgmaxnode[iNode].isMirrored = DotProd( CrossProd( m_phec->m_rgmaxnode[iNode].mat3ObjectTM.GetRow(0).Normalize(), m_phec->m_rgmaxnode[iNode].mat3ObjectTM.GetRow(1).Normalize() ).Normalize(), m_phec->m_rgmaxnode[iNode].mat3ObjectTM.GetRow(2).Normalize() ) < 0;

	// Dump node description
	fprintf(m_pfile, "%3d \"%s\" %3d\n", 
		iNode, 
		strNodeName, 
		iNodeParent );

	return TREE_CONTINUE;
}



//=================================================================
// Methods for DumpFrameRotationsTEP
//
int DumpFrameRotationsTEP::callback(INode *pnode)
{
	ASSERT_MBOX(!(pnode)->IsRootNode(), "Encountered a root node!");

	if (::FNodeMarkedToSkip(pnode))
		return TREE_CONTINUE;

	int iNode = ::GetIndexOfINode(pnode);

	TSTR strNodeName(pnode->GetName());

	// The model's root is a child of the real "scene root"
	INode *pnodeParent = pnode->GetParentNode();
	BOOL fNodeIsRoot = pnodeParent->IsRootNode( );

	// Get Node's "Local" Transformation Matrix
	Matrix3 mat3NodeTM		= pnode->GetNodeTM(m_tvToDump);
	Matrix3 mat3ParentTM	= pnodeParent->GetNodeTM(m_tvToDump);
	mat3NodeTM.NoScale();		// Clear these out because they apparently
	mat3ParentTM.NoScale();		// screw up the following calculation.
	Matrix3 mat3NodeLocalTM	= mat3NodeTM * Inverse(mat3ParentTM);
	Point3 rowTrans = mat3NodeLocalTM.GetTrans();

	// check to see if the parent bone was mirrored.  If so, mirror invert this bones position
	if (m_phec->m_rgmaxnode[iNode].imaxnodeParent >= 0 && m_phec->m_rgmaxnode[m_phec->m_rgmaxnode[iNode].imaxnodeParent].isMirrored)
	{
		rowTrans = rowTrans * -1.0f;
	}

	// Get the rotation (via decomposition into "affine parts", then quaternion-to-Euler)
	// Apparently the order of rotations returned by QuatToEuler() is X, then Y, then Z.
	AffineParts affparts;
	float rgflXYZRotations[3];

	decomp_affine(mat3NodeLocalTM, &affparts);
	QuatToEuler(affparts.q, rgflXYZRotations);

	float xRot = rgflXYZRotations[0];		// in radians
	float yRot = rgflXYZRotations[1];		// in radians
	float zRot = rgflXYZRotations[2];		// in radians

	// Get rotations in the -2pi...2pi range
	xRot = ::FlReduceRotation(xRot);
	yRot = ::FlReduceRotation(yRot);
	zRot = ::FlReduceRotation(zRot);
	
	// Print rotations
	fprintf(m_pfile, "%3d %f %f %f %f %f %f\n", 
		// Node:%-15s Rotation (x,y,z)\n",
		iNode, rowTrans.x, rowTrans.y, rowTrans.z, xRot, yRot, zRot);

	return TREE_CONTINUE;
}



//=================================================================
// Methods for DumpModelTEP
//
Modifier *FindPhysiqueModifier (INode *nodePtr)
{
	// Get object from node. Abort if no object.
	Object *ObjectPtr = nodePtr->GetObjectRef();
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject *DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier *ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Physique ?
			if (ModifierPtr->ClassID() == Class_ID(	PHYSIQUE_CLASS_ID_A, PHYSIQUE_CLASS_ID_B) )
			{
				// Yes -> Exit.
				return ModifierPtr;
			}
			// Next modifier stack entry.
			ModStackIndex++;
		}
	}
	// Not found.
	return NULL;
}

Modifier *FindBonesProModifier (INode *nodePtr)
{
	// Get object from node. Abort if no object.
	Object *ObjectPtr = nodePtr->GetObjectRef();
	if (!ObjectPtr) return NULL;

	// Is derived object ?
	if (ObjectPtr->SuperClassID() == GEN_DERIVOB_CLASS_ID)
	{
		// Yes -> Cast.
		IDerivedObject *DerivedObjectPtr = static_cast<IDerivedObject*>(ObjectPtr);

		// Iterate over all entries of the modifier stack.
		int ModStackIndex = 0;
		while (ModStackIndex < DerivedObjectPtr->NumModifiers())
		{
			// Get current modifier.
			Modifier *ModifierPtr = DerivedObjectPtr->GetModifier(ModStackIndex);

			// Is this Bones Pro OSM?
			if (ModifierPtr->ClassID() == BP_CLASS_ID_OSM )
			{
				// Yes -> Exit.
				return ModifierPtr;
			}
			// Is this Bones Pro WSM?
			if (ModifierPtr->ClassID() == BP_CLASS_ID_WSM )
			{
				// Yes -> Exit.
				return ModifierPtr;
			}
			// Next modifier stack entry.
			ModStackIndex++;
		}
	}
	// Not found.
	return NULL;
}

// #define DEBUG_MESH_DUMP

//=================================================================
// Methods for DumpModelTEP
//
int DumpModelTEP::callback(INode *pnode)
{
	Object*	pobj;
	int	fHasMat = TRUE;

	// clear physique export parameters
	m_mcExport = NULL;
	m_phyExport = NULL;
    m_phyMod = NULL;
    m_bonesProMod = NULL;

	ASSERT_MBOX(!(pnode)->IsRootNode(), "Encountered a root node!");

	if (::FNodeMarkedToSkip(pnode))
		return TREE_CONTINUE;

	// Actually, if it's not selected, skip it!
	//if (!pnode->Selected())
	//	return TRUE;
	
	int iNode = ::GetIndexOfINode(pnode);
	TSTR strNodeName(pnode->GetName());
	
	// The Footsteps node apparently MUST have a dummy mesh attached!  Ignore it explicitly.
	if (FStrEq((char*)strNodeName, "Bip01 Footsteps"))
		return TREE_CONTINUE;

	// Helper nodes don't have meshes
	pobj = pnode->GetObjectRef();
	if (pobj->SuperClassID() == HELPER_CLASS_ID)
		return TREE_CONTINUE;

	// The model's root is a child of the real "scene root"
	INode *pnodeParent = pnode->GetParentNode();
	BOOL fNodeIsRoot = pnodeParent->IsRootNode( );

	// Get node's material: should be a multi/sub (if it has a material at all)
	Mtl *pmtlNode = pnode->GetMtl();
	if (pmtlNode == NULL)
	{
		return TREE_CONTINUE;
		fHasMat = FALSE;
	}
	else if (!(pmtlNode->ClassID() == Class_ID(MULTI_CLASS_ID, 0) && pmtlNode->IsMultiMtl()))
	{
		// sprintf(st_szDBG, "ERROR--Material on node %s isn't a Multi/Sub-Object", (char*)strNodeName);
		// ASSERT_AND_ABORT(FALSE, st_szDBG);
		// fHasMat = FALSE;
	}
	
	// Get Node's object, convert to a triangle-mesh object, so I can access the Faces
	ObjectState os = pnode->EvalWorldState(m_tvToDump);
	pobj = os.obj;
	TriObject *ptriobj;
	BOOL fConvertedToTriObject = 
		pobj->CanConvertToType(triObjectClassID) &&
		(ptriobj = (TriObject*)pobj->ConvertToType(m_tvToDump, triObjectClassID)) != NULL;
	if (!fConvertedToTriObject)
		return TREE_CONTINUE;
	Mesh *pmesh = &ptriobj->mesh;

	// Shouldn't have gotten this far if it's a helper object
	if (pobj->SuperClassID() == HELPER_CLASS_ID)
	{
		sprintf(st_szDBG, "ERROR--Helper node %s has an attached mesh, and it shouldn't.", (char*)strNodeName);
		ASSERT_AND_ABORT(FALSE, st_szDBG);
	}

	// Ensure that the vertex normals are up-to-date
	pmesh->buildNormals();

	// We want the vertex coordinates in World-space, not object-space
	Matrix3 mat3ObjectTM = pnode->GetObjectTM(m_tvToDump);


	// initialize physique export parameters
    m_phyMod = FindPhysiqueModifier(pnode);
    if (m_phyMod)
	{
		// Physique Modifier exists for given Node
	    m_phyExport = (IPhysiqueExport *)m_phyMod->GetInterface(I_PHYINTERFACE);

        if (m_phyExport)
        {
            // create a ModContext Export Interface for the specific node of the Physique Modifier
           m_mcExport = (IPhyContextExport *)m_phyExport->GetContextInterface(pnode);

		   if (m_mcExport)
		   {
		       // convert all vertices to Rigid 
                m_mcExport->ConvertToRigid(TRUE);
		   }
		}
	}

	// initialize bones pro export parameters
	m_wa  = NULL;
	m_bonesProMod = FindBonesProModifier(pnode);
	if (m_bonesProMod)
	{
		m_bonesProMod->SetProperty( BP_PROPID_GET_WEIGHTS, &m_wa );
	}

	// Dump the triangle face info
	int cFaces = pmesh->getNumFaces();
	for (int iFace = 0; iFace < cFaces; iFace++)
	{
		Face*	pface		= &pmesh->faces[iFace];
		TVFace*	ptvface		= (pmesh->tvFace) ? &pmesh->tvFace[iFace] : NULL;
		DWORD	smGroupFace	= pface->getSmGroup();

		// Get face's 3 indexes into the Mesh's vertex array(s).
		DWORD iVertex0 = pface->getVert(0);
		DWORD iVertex1 = pface->getVert(1);
		DWORD iVertex2 = pface->getVert(2);
		ASSERT_AND_ABORT((int)iVertex0 < pmesh->getNumVerts(), "Bogus Vertex 0 index");
		ASSERT_AND_ABORT((int)iVertex1 < pmesh->getNumVerts(), "Bogus Vertex 1 index");
		ASSERT_AND_ABORT((int)iVertex2 < pmesh->getNumVerts(), "Bogus Vertex 2 index");
		
		// Get the 3 Vertex's for this face
		Point3 pt3Vertex0 = pmesh->getVert(iVertex0);
		Point3 pt3Vertex1 = pmesh->getVert(iVertex1);
		Point3 pt3Vertex2 = pmesh->getVert(iVertex2);

		// Get the 3 RVertex's for this face
		// NOTE: I'm using getRVertPtr instead of getRVert to work around a 3DSMax bug
		RVertex *prvertex0 = pmesh->getRVertPtr(iVertex0);
		RVertex *prvertex1 = pmesh->getRVertPtr(iVertex1);
		RVertex *prvertex2 = pmesh->getRVertPtr(iVertex2);
		
		// Find appropriate normals for each RVertex
		// A vertex can be part of multiple faces, so the "smoothing group"
		// is used to locate the normal for this face's use of the vertex.
		Point3 pt3Vertex0Normal;
		Point3 pt3Vertex1Normal;
		Point3 pt3Vertex2Normal;
		if (smGroupFace) 
		{
			pt3Vertex0Normal = Pt3GetRVertexNormal(prvertex0, smGroupFace);
			pt3Vertex1Normal = Pt3GetRVertexNormal(prvertex1, smGroupFace);
			pt3Vertex2Normal = Pt3GetRVertexNormal(prvertex2, smGroupFace);
		}
		else 
		{
			pt3Vertex0Normal = pmesh->getFaceNormal( iFace );
			pt3Vertex1Normal = pmesh->getFaceNormal( iFace );
			pt3Vertex2Normal = pmesh->getFaceNormal( iFace );
		}
		ASSERT_AND_ABORT( Length( pt3Vertex0Normal ) <= 1.1, "bogus orig normal 0" );
		ASSERT_AND_ABORT( Length( pt3Vertex1Normal ) <= 1.1, "bogus orig normal 1" );
		ASSERT_AND_ABORT( Length( pt3Vertex2Normal ) <= 1.1, "bogus orig normal 2" );
	
		// Get Face's sub-material from node's material, to get the bitmap name.
		// And no, there isn't a simpler way to get the bitmap name, you have to
		// dig down through all these levels.
		TCHAR szBitmapName[256] = "null.bmp";
		if (fHasMat)
		{
			Texmap *ptexmap = NULL;
			MtlID mtlidFace = pface->getMatID();
			if (pmtlNode->IsMultiMtl())
			{
				if (mtlidFace >= pmtlNode->NumSubMtls())
				{
					sprintf(st_szDBG, "ERROR--Bogus sub-material index %d in node %s; highest valid index is %d",
						mtlidFace, (char*)strNodeName, pmtlNode->NumSubMtls()-1);
					// ASSERT_AND_ABORT(FALSE, st_szDBG);
					mtlidFace = 0;
				}
				Mtl *pmtlFace = pmtlNode->GetSubMtl(mtlidFace);
				ASSERT_AND_ABORT(pmtlFace != NULL, "NULL Sub-material returned");
 
				/*
				if ((pmtlFace->ClassID() == Class_ID(MULTI_CLASS_ID, 0) && pmtlFace->IsMultiMtl()))
				{
					// it's a sub-sub material.  Gads.
					pmtlFace = pmtlFace->GetSubMtl(mtlidFace);			
					ASSERT_AND_ABORT(pmtlFace != NULL, "NULL Sub-material returned");
				}
				*/

				if (!(pmtlFace->ClassID() == Class_ID(DMTL_CLASS_ID, 0)))
				{

					sprintf(st_szDBG,
						"ERROR--Sub-material with index %d (used in node %s) isn't a 'default/standard' material [%x].",
						mtlidFace, (char*)strNodeName, pmtlFace->ClassID());
					ASSERT_AND_ABORT(FALSE, st_szDBG);
				}
				StdMat *pstdmtlFace = (StdMat*)pmtlFace;
				ptexmap = pstdmtlFace->GetSubTexmap(ID_DI);
			}
			else
			{
				ptexmap = pmtlNode->GetActiveTexmap();
			}

			// ASSERT_AND_ABORT(ptexmap != NULL, "NULL diffuse texture")
			if (ptexmap != NULL) 
			{
				if (!(ptexmap->ClassID() == Class_ID(BMTEX_CLASS_ID, 0)))
				{
					sprintf(st_szDBG,
						"ERROR--Sub-material with index %d (used in node %s) doesn't have a bitmap as its diffuse texture.",
						mtlidFace, (char*)strNodeName);
					ASSERT_AND_ABORT(FALSE, st_szDBG);
				}
				BitmapTex *pbmptex = (BitmapTex*)ptexmap;
				strcpy(szBitmapName, pbmptex->GetMapName());
				TSTR strPath, strFile;
				SplitPathFile(TSTR(szBitmapName), &strPath, &strFile);
				strcpy(szBitmapName,strFile);
			}
		}

		UVVert UVvertex0( 0, 0, 0 );
		UVVert UVvertex1( 1, 0, 0 );
		UVVert UVvertex2( 0, 1, 0 );
		
		// All faces must have textures assigned to them
		if (ptvface && (pface->flags & HAS_TVERTS))
		{
			// Get TVface's 3 indexes into the Mesh's TVertex array(s).
			DWORD iTVertex0 = ptvface->getTVert(0);
			DWORD iTVertex1 = ptvface->getTVert(1);
			DWORD iTVertex2 = ptvface->getTVert(2);
			ASSERT_AND_ABORT((int)iTVertex0 < pmesh->getNumTVerts(), "Bogus TVertex 0 index");
			ASSERT_AND_ABORT((int)iTVertex1 < pmesh->getNumTVerts(), "Bogus TVertex 1 index");
			ASSERT_AND_ABORT((int)iTVertex2 < pmesh->getNumTVerts(), "Bogus TVertex 2 index");

			// Get the 3 TVertex's for this TVFace
			// NOTE: I'm using getRVertPtr instead of getRVert to work around a 3DSMax bug
			UVvertex0 = pmesh->getTVert(iTVertex0);
			UVvertex1 = pmesh->getTVert(iTVertex1);
			UVvertex2 = pmesh->getTVert(iTVertex2);
		}
		else 
		{
			//sprintf(st_szDBG, "ERROR--Node %s has a textureless face.  All faces must have an applied texture.", (char*)strNodeName);
			//ASSERT_AND_ABORT(FALSE, st_szDBG);
		}
		
		/*
		const char *szExpectedExtension = ".bmp";
		if (stricmp(szBitmapName+strlen(szBitmapName)-strlen(szExpectedExtension), szExpectedExtension) != 0)
			{
			sprintf(st_szDBG, "Node %s uses %s, which is not a %s file", (char*)strNodeName, szBitmapName, szExpectedExtension);
			ASSERT_AND_ABORT(FALSE, st_szDBG);
			}
		*/

		// Determine owning bones for the vertices.
		int iNodeV0, iNodeV1, iNodeV2;
		// Simple 3dsMax model: the vertices are owned by the object, and hence the node
		iNodeV0 = iNode;
		iNodeV1 = iNode;
		iNodeV2 = iNode;
		
		// Rotate the face vertices out of object-space, and into world-space space
		Point3 v0 = pt3Vertex0 * mat3ObjectTM;
		Point3 v1 = pt3Vertex1 * mat3ObjectTM;
		Point3 v2 = pt3Vertex2 * mat3ObjectTM;


		Matrix3 mat3ObjectNTM = mat3ObjectTM;
		mat3ObjectNTM.NoScale( );
		ASSERT_AND_ABORT( Length( pt3Vertex0Normal ) <= 1.1, "bogus pre normal 0" );
		pt3Vertex0Normal = VectorTransform(mat3ObjectNTM, pt3Vertex0Normal);
		ASSERT_AND_ABORT( Length( pt3Vertex0Normal ) <= 1.1, "bogus post normal 0" );
		ASSERT_AND_ABORT( Length( pt3Vertex1Normal ) <= 1.1, "bogus pre normal 1" );
		pt3Vertex1Normal = VectorTransform(mat3ObjectNTM, pt3Vertex1Normal);
		ASSERT_AND_ABORT( Length( pt3Vertex1Normal ) <= 1.1, "bogus post normal 1" );
		ASSERT_AND_ABORT( Length( pt3Vertex2Normal ) <= 1.1, "bogus pre normal 2" );
		pt3Vertex2Normal = VectorTransform(mat3ObjectNTM, pt3Vertex2Normal);
		ASSERT_AND_ABORT( Length( pt3Vertex2Normal ) <= 1.1, "bogus post normal 2" );

		// Finally dump the bitmap name and 3 lines of face info
		fprintf(m_pfile, "%s\n", szBitmapName);
		fprintf(m_pfile, "%3d %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f",
				iNodeV0, v0.x, v0.y, v0.z,
				pt3Vertex0Normal.x, pt3Vertex0Normal.y, pt3Vertex0Normal.z,
				UVvertex0.x, UVvertex0.y);
		DumpWeights( iVertex0 );
		fprintf(m_pfile, "%3d %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f",
				iNodeV1, v1.x, v1.y, v1.z,
				pt3Vertex1Normal.x, pt3Vertex1Normal.y, pt3Vertex1Normal.z,
				UVvertex1.x, UVvertex1.y);
		DumpWeights( iVertex1 );
		fprintf(m_pfile, "%3d %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f %8.4f",
				iNodeV2, v2.x, v2.y, v2.z,
				pt3Vertex2Normal.x, pt3Vertex2Normal.y, pt3Vertex2Normal.z,
				UVvertex2.x, UVvertex2.y);
		DumpWeights( iVertex2 );
	}	

	cleanup( );	
	return TREE_CONTINUE;
}


#define MAX_BLEND_WEIGHTS 8

static struct {
	int iNode;
	float flWeight;
} aWeights[MAX_BLEND_WEIGHTS+1];

int AddWeight( int iCount, int iNode, float flWeight )
{
	if (flWeight < 0.001)
		return iCount;

	for (int i = 0; i < iCount; i++)
	{
		if (aWeights[i].flWeight < flWeight)
		{
			for (int j = iCount; j > i; j--)
			{
				aWeights[j] = aWeights[j-1];
			}
			break;
		}
	}
	aWeights[i].iNode = iNode;
	aWeights[i].flWeight = flWeight;

	iCount++;
	if (iCount > MAX_BLEND_WEIGHTS)
		iCount = MAX_BLEND_WEIGHTS;

	return iCount;
}


void DumpModelTEP::DumpWeights(int iVertex)
{
	if (m_mcExport)
	{
		IPhyVertexExport *vtxExport = m_mcExport->GetVertexInterface(iVertex);

		if (vtxExport)
		{
			if (vtxExport->GetVertexType() & BLENDED_TYPE)
			{
				IPhyBlendedRigidVertex *pBlend = ((IPhyBlendedRigidVertex *)vtxExport);
				int iCount = 0;

				for (int i = 0; i < pBlend->GetNumberNodes(); i++)
				{
					iCount = AddWeight( iCount, GetIndexOfINode( pBlend->GetNode( i ) ), pBlend->GetWeight( i ) );
				}

				fprintf(m_pfile, " %2d  ", iCount );
				for (i = 0; i < iCount; i++)
				{
					fprintf(m_pfile, " %2d %5.3f ", aWeights[i].iNode, aWeights[i].flWeight );
				}
			}
			else 
			{
				INode *Bone = ((IPhyRigidVertex *)vtxExport)->GetNode();

				fprintf(m_pfile, "  1   %2d 1.000", GetIndexOfINode(Bone) );
			}
			m_mcExport->ReleaseVertexInterface(vtxExport);
		}
	} 
	else if (m_wa != NULL)
	{
		int iCount = 0;

		for ( int iBone = 0; iBone < m_wa->nb; iBone++)
		{
			if (m_wa->w[iVertex * m_wa->nb + iBone] > 0.0)
			{
				BonesPro_Bone bone;
				bone.t = BP_TIME_ATTACHED;
				bone.index = iBone;
				m_bonesProMod->SetProperty( BP_PROPID_GET_BONE_STAT, &bone );
				if (bone.node != NULL)
				{
					iCount = AddWeight( iCount, GetIndexOfINode( bone.node ), m_wa->w[iVertex * m_wa->nb + iBone] );
				}
			}
		}

		fprintf(m_pfile, " %2d  ", iCount );
		for (int i = 0; i < iCount; i++)
		{
			fprintf(m_pfile, " %2d %5.3f ", aWeights[i].iNode, aWeights[i].flWeight );
		}
	}

	fprintf(m_pfile, "\n" );
	fflush( m_pfile );
}

void DumpModelTEP::cleanup(void)
{
	if (m_phyMod && m_phyExport)
	{
		if (m_mcExport)
		{
			m_phyExport->ReleaseContextInterface(m_mcExport);
			m_mcExport = NULL;
        }
        m_phyMod->ReleaseInterface(I_PHYINTERFACE, m_phyExport);
		m_phyExport = NULL;
		m_phyMod = NULL;
	}
}


Point3 DumpModelTEP::Pt3GetRVertexNormal(RVertex *prvertex, DWORD smGroupFace)
{
	// Lookup the appropriate vertex normal, based on smoothing group.
	int cNormals = prvertex->rFlags & NORCT_MASK;

	ASSERT_MBOX((cNormals == 1 && prvertex->ern == NULL) ||
				(cNormals > 1 && prvertex->ern != NULL), "BOGUS RVERTEX");
	
	if (cNormals == 1)
		return prvertex->rn.getNormal();
	else
	{
		for (int irn = 0; irn < cNormals; irn++)
			if (prvertex->ern[irn].getSmGroup() & smGroupFace)
				break;

		if (irn >= cNormals) 
		{
			irn = 0;
			// ASSERT_MBOX(irn < cNormals, "unknown smoothing group\n");
		}
		return prvertex->ern[irn].getNormal();
	}
}





//===========================================================
// Dialog proc for export options
//
static BOOL CALLBACK ExportOptionsDlgProc(
	HWND	hDlg,
	UINT	message,
	WPARAM	wParam,
	LPARAM	lParam)
{
	static SmdExportClass *pexp;
	switch (message)
	{
	case WM_INITDIALOG:
		pexp = (SmdExportClass*) lParam;
		CheckRadioButton(hDlg, IDC_CHECK_SKELETAL, IDC_CHECK_REFFRAME, IDC_CHECK_SKELETAL);
		SetFocus(GetDlgItem(hDlg,IDOK));
		return FALSE;
	case WM_DESTROY:
		return FALSE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			pexp->m_fReferenceFrame	= IsDlgButtonChecked(hDlg, IDC_CHECK_REFFRAME);
			EndDialog(hDlg, 1);		// 1 indicates "ok to export"
			return TRUE;
		case IDCANCEL:				// 0 indicates "cancel export"
			EndDialog(hDlg, 0);
			return TRUE;
		case IDC_CHECK_SKELETAL:
		case IDC_CHECK_REFFRAME:
			CheckRadioButton(hDlg, IDC_CHECK_SKELETAL, IDC_CHECK_REFFRAME, LOWORD(wParam));
			break;
		}
	}
	return FALSE;
}



//========================================================================
// Utility functions for getting/setting the personal "node index" property.
// NOTE: I'm storing a string-property because I hit a 3DSMax bug in v1.2 when I
// NOTE: tried using an integer property.
// FURTHER NOTE: those properties seem to change randomly sometimes, so I'm
// implementing my own.

typedef struct
{
	char	szNodeName[SmdExportClass::MAX_NAME_CHARS];
	int		iNode;
} NAMEMAP;
const int MAX_NAMEMAP = 512;
static NAMEMAP g_rgnm[MAX_NAMEMAP];

int GetIndexOfINode(INode *pnode, BOOL fAssertPropExists)
{
	TSTR strNodeName(pnode->GetName());
	for (int inm = 0; inm < g_inmMac; inm++)
	{
		if (FStrEq(g_rgnm[inm].szNodeName, (char*)strNodeName))
		{
			return g_rgnm[inm].iNode;
		}
	}

	if (fAssertPropExists)
		ASSERT_MBOX(FALSE, "No NODEINDEXSTR property");
	return -7777;
}

	
void SetIndexOfINode(INode *pnode, int inode)
{
	TSTR strNodeName(pnode->GetName());
	NAMEMAP *pnm;
	for (int inm = 0; inm < g_inmMac; inm++)
		if (FStrEq(g_rgnm[inm].szNodeName, (char*)strNodeName))
			break;
	if (inm < g_inmMac)
		pnm = &g_rgnm[inm];
	else
	{
		ASSERT_MBOX(g_inmMac < MAX_NAMEMAP, "NAMEMAP is full");
		pnm = &g_rgnm[g_inmMac++];
		strcpy(pnm->szNodeName, (char*)strNodeName);
	}
	pnm->iNode = inode;
}


//=============================================================
// Returns TRUE if a node should be ignored during tree traversal.
//
BOOL FUndesirableNode(INode *pnode)
{
	// Get Node's underlying object, and object class name
	Object *pobj = pnode->GetObjectRef();

	// Don't care about lights, dummies, and cameras
	if (pobj->SuperClassID() == CAMERA_CLASS_ID)
		return TRUE;
	if (pobj->SuperClassID() == LIGHT_CLASS_ID)
		return TRUE;

	return FALSE;
}


//=============================================================
// Returns TRUE if a node has been marked as skippable
//
BOOL FNodeMarkedToSkip(INode *pnode)
{
	return (::GetIndexOfINode(pnode) == SmdExportClass::UNDESIRABLE_NODE_MARKER);
}

	
//=============================================================
// Reduces a rotation to within the -2PI..2PI range.
//
static float FlReduceRotation(float fl)
{
	while (fl >= TWOPI)
		fl -= TWOPI;
	while (fl <= -TWOPI)
		fl += TWOPI;
	return fl;
}
