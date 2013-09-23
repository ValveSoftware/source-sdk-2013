#ifndef VGUI_MENULAYER_H
#define VGUI_MENULAYER_H

#include "cbase.h"
#include <vgui_controls/controls.h>
#include <vgui_controls/panel.h>

class CVGUIParticleContainer;
struct vParticle;
class vParticleOperatorBase;

#define pFnLayerRoutine( x ) static void x( CVGUIMenuLayer *l, bool bInit )

class CVGUIMenuLayer : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CVGUIMenuLayer, vgui::Panel );
public:

	CVGUIMenuLayer( vgui::Panel *parent, bool bAutoScale = false, bool bRefractive = false );
	~CVGUIMenuLayer();

	void SetBackgroundImage( const char *pszFile );
	void SetBackgroundImage( int i );

	void SetBackgroundColor( int r, int g, int b, int a = 255 );
	void SetBackgroundColor( Color col );
	Color GetBackgroundColor();

	void SetNormalizedBackgroundBounds( Vector2D p00, Vector2D p11 );
	void SetIngameRenderingEnabled( bool bEnabled );

	enum PinCornerLayer_t
	{
		PINCL_00 = 0,
		PINCL_10,
		PINCL_11,
		PINCL_01,
		PINCL_0C,
		PINCL_C0,
		PINCL_CC,
	};

	void SetRelativeBounds( int x, int y, int w, int t, PinCornerLayer_t pin = PINCL_00 );
	void SetRoutine( void(* func )( CVGUIMenuLayer *l, bool bInit ) );
	void SetPaintHook( void(* func )( CVGUIMenuLayer *l, bool bInit ) );

	void AddParticle( vParticle * p );
	void AddOperator( vParticleOperatorBase * pOp );

	void AddEmissionLayer( CVGUIMenuLayer *l );
	int GetNumEmissionLayers();
	CVGUIMenuLayer *GetEmissionLayer( int index );

protected:

	virtual void OnThink();
	virtual void Paint();

	virtual void PerformLayout();

private:

	CVGUIParticleContainer *m_pParticleParent;
	void(* m_pRoutine )( CVGUIMenuLayer *l, bool bInit );
	void(* m_pPaintHook )( CVGUIMenuLayer *l, bool bInit );
	bool m_bRequireInit;

	CUtlVector< CVGUIMenuLayer* >m_hEmissionLayers;

	int m_iBackgroundImage;
	Color m_colBackgroundColor;
	Vector2D m_vecBGBounds_min;
	Vector2D m_vecBGBounds_max;

	bool m_bAutoSize;
	bool m_bRefractive;
	bool m_bIngame;
	int rx, ry, rw, rt;
	PinCornerLayer_t m_pin;
};


#endif