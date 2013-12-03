//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

//===================================================================
// Useful macros
//
#define CONSTRUCTOR
#define DESTRUCTOR

#define EXPORT_THIS		__declspec(dllexport)

#define DEFAULT_EXT		_T("smd")

#define FStrEq(sz1, sz2) (strcmp((sz1), (sz2)) == 0)


//===================================================================
// Class that implements the scene-export.
//
class SmdExportClass : public SceneExport
{
	friend BOOL CALLBACK ExportOptionsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
	friend class DumpModelTEP;
	friend class DumpDeformsTEP;

public:
	CONSTRUCTOR				SmdExportClass	(void);
	DESTRUCTOR				~SmdExportClass	(void);

	// Required by classes derived from SceneExport
	virtual int				ExtCount		(void)		{ return 1;						}
	virtual const TCHAR*	Ext				(int i)		{ return DEFAULT_EXT;			}
	virtual const TCHAR*	LongDesc		(void)		{ return _T("Valve Skeletal Model Exporter for 3D Studio Max");	}
	virtual const TCHAR*	ShortDesc		(void)		{ return _T("Valve SMD");			}
	virtual const TCHAR*	AuthorName		(void)		{ return _T("Valve, LLC");			}
	virtual const TCHAR*	CopyrightMessage(void)		{ return _T("Copyright (c) 1998, Valve LLC");			}
	virtual const TCHAR*	OtherMessage1	(void)		{ return _T("");				}
	virtual const TCHAR*	OtherMessage2	(void)		{ return _T("");				}
	virtual unsigned int	Version			(void)		{ return 201;					}
	virtual void			ShowAbout		(HWND hWnd)	{ return;						}
	// virtual int				DoExport		(const TCHAR *name, ExpInterface *ei, Interface *i);
	virtual int		DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE,DWORD options=0); // Export	file

	// Integer constants for this class
	enum
		{
		MAX_NAME_CHARS			= 70,
		UNDESIRABLE_NODE_MARKER	= -7777
		};

	// For keeping info about each (non-ignored) 3dsMax node in the tree
	typedef struct
	{
		char		szNodeName[MAX_NAME_CHARS];	// usefull for lookups
		Matrix3		mat3NodeTM;					// node's transformation matrix (at time zero)
		Matrix3		mat3ObjectTM;				// object-offset transformation matrix (at time zero)
		int			imaxnodeParent;				// cached index of parent node
		float		xRotFirstFrame;				// 1st frame's X rotation
		float		yRotFirstFrame;				// 1st frame's Y rotation
		float		zRotFirstFrame;				// 1st frame's Z rotation
		bool		isMirrored;
	} MaxNode;
	MaxNode		*m_rgmaxnode;		// array of nodes
	long		m_imaxnodeMac;		// # of nodes

	// Animation metrics (gleaned from 3dsMax and cached for convenience)
	Interval	m_intervalOfAnimation;
	TimeValue	m_tvStart;
	TimeValue	m_tvEnd;
	int			m_tpf;		// ticks-per-frame

private:
	BOOL					CollectNodes	(ExpInterface *expiface);
	BOOL					DumpBones		(FILE *pFile, ExpInterface *pexpiface);
	BOOL					DumpRotations	(FILE *pFile, ExpInterface *pexpiface);
	BOOL					DumpModel		(FILE *pFile, ExpInterface *pexpiface);
	BOOL					DumpDeforms		(FILE *pFile, ExpInterface *pexpiface);

	// Is this MAX file just the reference frame, or an animation?
	// If TRUE, the "bones" and "mesh" files will be created.
	// If FALSE, the "rots" file will be created.
	BOOL		m_fReferenceFrame;
};


//===================================================================
// Basically just a ClassFactory for communicating with 3DSMAX.
//
class SmdExportClassDesc : public ClassDesc
{
public:
	int				IsPublic		(void)					{ return TRUE;								}
	void *			Create			(BOOL loading=FALSE)	{ return new SmdExportClass;				}
	const TCHAR *	ClassName		(void)					{ return _T("SmdExport");					}
	SClass_ID 		SuperClassID	(void)					{ return SCENE_EXPORT_CLASS_ID;				}
	Class_ID 		ClassID			(void)					{ return Class_ID(0x774a43fd, 0x794d2210);	}
	const TCHAR *	Category		(void)					{ return _T("");							}
};


//===================================================================
// Tree Enumeration Callback
//		Just counts the nodes in the node tree
//
class CountNodesTEP : public ITreeEnumProc
{
public:
	virtual int				callback(INode *node);
	int						m_cNodes;		// running count of nodes
};


//===================================================================
// Tree Enumeration Callback
//		Collects the nodes in the tree into the global array
//
class CollectNodesTEP : public ITreeEnumProc
{
public:
	virtual int				callback(INode *node);
	SmdExportClass			*m_phec;
};


//===================================================================
// Tree Enumeration Callback
//		Dumps the bone offsets to a file.
//
class DumpNodesTEP : public ITreeEnumProc
{
public:
	virtual int				callback(INode *node);
	FILE					*m_pfile;		// write to this file
	SmdExportClass			*m_phec;
};


//===================================================================
// Tree Enumeration Callback
//		Dumps the per-frame bone rotations to a file.
//
class DumpFrameRotationsTEP : public ITreeEnumProc
{
public:
	virtual int				callback(INode *node);
	void					cleanup(void);
	FILE					*m_pfile;		// write to this file
	TimeValue				m_tvToDump;		// dump snapshot at this frame time
	SmdExportClass			*m_phec;
};

//===================================================================
// Tree Enumeration Callback
//		Dumps the triangle meshes to a file.
//
class DumpModelTEP : public ITreeEnumProc
{
public:
	virtual int				callback(INode *node);
	void					cleanup(void);
	FILE					*m_pfile;		// write to this file
	TimeValue				m_tvToDump;		// dump snapshot at this frame time
	SmdExportClass			*m_phec;
	IPhyContextExport		*m_mcExport;
	IPhysiqueExport			*m_phyExport;
    Modifier				*m_phyMod;
	Modifier				*m_bonesProMod;
	BonesPro_WeightArray	*m_wa;
private:
	Point3					Pt3GetRVertexNormal(RVertex *prvertex, DWORD smGroupFace);
	void					DumpWeights( int iVertex );
};

