#ifndef SHEDITMRENDER_H
#define SHEDITMRENDER_H

#include "cbase.h"
#include "ShaderEditor/ISEdit_ModelRender.h"

class C_BaseFlex_OverrideLod;

class SEditModelRender : public ISEditModelRender, public CAutoGameSystemPerFrame
{
public:
	SEditModelRender( char const *name );
	~SEditModelRender();

// autogamesystem
	virtual bool Init();
	virtual void Shutdown();
	virtual void Update( float frametime );

	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

// interface
	virtual bool LoadModel( const char *localPath );
	virtual void DestroyModel();
	virtual void GetModelCenter( float *pFl3_ViewOffset );

	virtual int QuerySequences( char ***list );
	virtual void SetSequence( const char *name );

	virtual void ExecRender();
	virtual void DoPostProc( int x, int y, int w, int h );

	virtual int MaterialPicker( char ***szMat );

	virtual void DestroyCharPtrList( char ***szList );

private:

	bool IsModelReady();
	void ResetModel();

	C_BaseFlex *pModelInstance;
	char m_szModelPath[MAX_PATH];
	int m_iNumPoseParams;
};

#endif