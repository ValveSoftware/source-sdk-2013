
#include "cbase.h"
#include "tier1/KeyValues.h"
#include "Gstring/vgui/vUtil.h"
#include "Gstring/vgui/vLensflare.h"
#include "Gstring/vgui/vParticleOperatorsLensflare.h"

vLensflare_Collection *vLensflare_Collection::InitFromScript( vgui::Panel *parent,
		CGlowOverlay *pSource,
		KeyValues *pData )
{
	pData = pData->MakeCopy();

	CVGUIParticleContainer *pContainer = new CVGUIParticleContainer( parent );
	vLensflare_Collection *pRet = new vLensflare_Collection();
	pRet->m_pContainer = pContainer;
	pRet->m_pSource = pSource;
	pRet->ResizePanel();

	pContainer->AddGlobalOperator( new vParticleOperator_SizeMakeRelative() );

	float flGlobalAlpha = 1.0f;
	float flGlobalScale = 1.0f;
	Vector vecBaseColor( 1, 1, 1 );

	KeyValues *pConfig = pData->FindKey( "config" );
	if ( pConfig != NULL )
	{
		pData->RemoveSubKey( pConfig );

		flGlobalAlpha = pConfig->GetFloat( "global_alpha", flGlobalAlpha );
		flGlobalScale = pConfig->GetFloat( "global_scale", flGlobalScale );

		if ( pConfig->GetInt( "global_color_modulate_by_source" ) != 0 )
			vecBaseColor *= pSource->GetGlowColor();

		Color globalColor( 255, 255, 255, 255 );
		if ( pConfig->FindKey( "global_color" ) != NULL )
			globalColor = pConfig->GetColor("global_color" );

		vecBaseColor *= Vector( globalColor[0] / 255.0f,
								globalColor[1] / 255.0f,
								globalColor[2] / 255.0f );

		pConfig->deleteThis();
		pConfig = NULL;
	}

	for ( KeyValues *pSub = pData->GetFirstTrueSubKey(); pSub; pSub = pSub->GetNextTrueSubKey() )
	{
		const char *pszMaterialName = pSub->GetString( "material" );

		if ( !pszMaterialName || !Q_strlen( pszMaterialName ) )
		{
			Warning( "missing lens flare material in: %s\n", pSub->GetName() );
			continue;
		}

		Color spriteColor( 255, 255, 255, 255 );
		if ( pSub->FindKey( "color" ) != NULL )
			spriteColor = pSub->GetColor("color" );
		Vector vecColor = Vector( spriteColor[0] / 255.0f,
								spriteColor[1] / 255.0f,
								spriteColor[2] / 255.0f ) * vecBaseColor;

		float flAlpha = flGlobalAlpha * pSub->GetFloat( "alpha", 1.0f );
		float flSize = flGlobalScale * pSub->GetFloat( "size", 100.0f );
		float flScaleX = pSub->GetFloat( "scale_x", 1.0f );
		float flScaleY = pSub->GetFloat( "scale_y", 1.0f );

		if ( vecColor.IsZero() ||
			flAlpha <= 0.0f ||
			flSize <= 1.0f )
			continue;

		vParticle *p = new vParticle();
		p->CreateRectRenderer( pszMaterialName, flScaleX, flScaleY );
		p->SetStartAlpha( flAlpha );
		p->SetStartSize_Relative( flSize );
		p->SetStartColor( vecColor );

		KeyValues *pOp = pSub->FindKey( "operator_size" );
		if ( pOp != NULL )
			p->AddParticleOperator( pRet->ParseOperator_Size( pOp ) );

		pOp = pSub->FindKey( "operator_alpha" );
		if ( pOp != NULL )
			p->AddParticleOperator( pRet->ParseOperator_Alpha( pOp ) );

		pOp = pSub->FindKey( "operator_transform" );
		if ( pOp != NULL )
			p->AddParticleOperator( pRet->ParseOperator_Transforms( pOp ) );

		pRet->m_pContainer->AddParticle( p );
	}

	pData->deleteThis();
	return pRet;
}

void vLensflare_Collection::Destroy()
{
	Assert( m_pContainer );

	m_pContainer->SetParent( ((vgui::Panel*)NULL) );
	delete m_pContainer;
	m_pContainer = NULL;

	delete this;
}

void vLensflare_Collection::ResizePanel()
{
	Assert( m_pContainer != NULL );

	int w, t;
	GetHudSize( w, t );
	m_pContainer->SetSize( w, t );
}

vParticleOperatorBase *vLensflare_Collection::ParseOperator_Size( KeyValues *pData )
{
	vParticleOperator_Lensflare_Size *pOp = new vParticleOperator_Lensflare_Size( GetSource(),
		pData->GetFloat( "min", 0 ),
		pData->GetFloat( "max", 1 ),
		pData->GetInt( "modulate_by_angle", 1 ) != 0,
		pData->GetFloat( "modulate_dot_min", 0.1f ),
		pData->GetFloat( "modulate_dot_max", 1 ),
		pData->GetFloat( "modulate_dot_bias", 0.5f ) );

	return pOp;
}

vParticleOperatorBase *vLensflare_Collection::ParseOperator_Alpha( KeyValues *pData )
{
	vParticleOperator_Lensflare_Alpha *pOp = new vParticleOperator_Lensflare_Alpha( GetSource(),
		pData->GetFloat( "min", 0 ),
		pData->GetFloat( "max", 1 ),
		pData->GetInt( "modulate_by_angle", 1 ) != 0,
		pData->GetFloat( "modulate_dot_min", 0.707f ),
		pData->GetFloat( "modulate_dot_max", 1 ),
		pData->GetFloat( "modulate_dot_bias", 0.5f ) );

	return pOp;
}

vParticleOperatorBase *vLensflare_Collection::ParseOperator_Transforms( KeyValues *pData )
{
	vParticleOperator_Lensflare_Transformation *pOp = new vParticleOperator_Lensflare_Transformation( GetSource(),
		pData->GetFloat( "position", 1.0f ),
		pData->GetFloat( "position_angular_offset", 0.0f ),
		pData->GetInt( "orientate_by_direction", 1 ) != 0,
		pData->GetFloat( "rotation_offset", 0.0f ),
		pData->GetFloat( "orientate_rotation_multiplier", 1.0f ) );

	return pOp;
}

CGlowOverlay *vLensflare_Collection::GetSource()
{
	return m_pSource;
}