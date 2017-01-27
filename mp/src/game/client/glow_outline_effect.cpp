//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a glowing outline around client renderable objects.
//
//===============================================================================

#include "cbase.h"
#include "glow_outline_effect.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"

#define FULL_FRAME_TEXTURE "_rt_FullFrameFB"

#ifdef GLOWS_ENABLE

// If you've added IMatRenderContext::OverrideDepthFunc (see ::DrawGlowOccluded below),
// then you can enable this and have single-pass glows for "glow when occluded" outlines.
// ***PLEASE*** increment MATERIAL_SYSTEM_INTERFACE_VERSION when you add this!
#define ADDED_OVERRIDE_DEPTH_FUNC 0

// If you've fixed IMatRenderContext::CopyTextureToRenderTargetEx
// (see CGlowObjectManager::RenderGlowModels below), then you can enable this and have
// code that's a bit cleaner. Also, then you won't have to ship debug/debugfbtexture1.
#define FIXED_COPY_TEXTURE_TO_RENDER_TARGET 0

ConVar glow_outline_effect_enable( "glow_outline_effect_enable", "1", FCVAR_ARCHIVE, "Enable entity outline glow effects." );

// This doesn't actually do anything.
ConVar glow_outline_effect_width( "glow_outline_width", "10.0f", FCVAR_CHEAT | FCVAR_UNREGISTERED, "Width of glow outline effect in screen space." );

// Not really necessary, but it's two different styles. I prefer style 0, but style 1 "closes off" partially occluded glows.
static ConVar glow_outline_effect_stencil_mode("glow_outline_effect_stencil_mode", "0", 0,
	"\n\t0: Draws partially occluded glows in a more 3d-esque way, making them look more like they're actually surrounding the model."
	"\n\t1: Draws partially occluded glows in a more 2d-esque way, which can make them more visible."
	"\n\tSee https://i.imgur.com/OJQkXei.gif",
	true, 0, true, 1);

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CGlowObjectManager g_GlowObjectManager;

struct ShaderStencilState_t
{
	bool m_bEnable;
	StencilOperation_t m_FailOp;
	StencilOperation_t m_ZFailOp;
	StencilOperation_t m_PassOp;
	StencilComparisonFunction_t m_CompareFunc;
	int m_nReferenceValue;
	uint32 m_nTestMask;
	uint32 m_nWriteMask;

	ShaderStencilState_t()
	{
		m_bEnable = false;
		m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
		m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
		m_nReferenceValue = 0;
		m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
	}

	void SetStencilState( CMatRenderContextPtr &pRenderContext  )
	{
		pRenderContext->SetStencilEnable(m_bEnable);

		if (m_bEnable)
		{
			pRenderContext->SetStencilFailOperation(m_FailOp);
			pRenderContext->SetStencilZFailOperation(m_ZFailOp);
			pRenderContext->SetStencilPassOperation(m_PassOp);
			pRenderContext->SetStencilCompareFunction(m_CompareFunc);
			pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
			pRenderContext->SetStencilTestMask(m_nTestMask);
			pRenderContext->SetStencilWriteMask(m_nWriteMask);
		}
	}
};

void CGlowObjectManager::RenderGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot )
{
	if ( g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0() )
	{
		if ( glow_outline_effect_enable.GetBool() )
		{
			CMatRenderContextPtr pRenderContext( materials );

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport( nX, nY, nWidth, nHeight );

			PIXEvent _pixEvent( pRenderContext, "EntityGlowEffects" );
			ApplyEntityGlowEffects( pSetup, nSplitScreenSlot, pRenderContext, glow_outline_effect_width.GetFloat(), nX, nY, nWidth, nHeight );
		}
	}
}

void CGlowObjectManager::DrawGlowAlways(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 1;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(false, false);
	render->SetBlend(1);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || !current.m_bRenderWhenUnoccluded)
			continue;

		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(vGlowColor.Base()); // This only sets rgb, not alpha

		current.DrawModel();
	}
}

void CGlowObjectManager::DrawGlowOccluded(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
#if ADDED_OVERRIDE_DEPTH_FUNC	// Enable this when the TF2 team has added IMatRenderContext::OverrideDepthFunc or similar.
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 1;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = glow_outline_effect_stencil_mode.GetBool() ? STENCILOPERATION_KEEP : STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(true, false);

	// Not implemented, we need this feature to be able to do this in 1 pass. Otherwise,
	// we'd have to do 2 passes, 1st to mark on the stencil where the depth test failed,
	// 2nd to actually utilize that information and draw color there.
	pRenderContext->OverrideDepthFunc(true, SHADER_DEPTHFUNC_NEARER);

	for (int i = 0; i < glowObjectDefinitions.Count(); i++)
	{
		auto& current = glowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
			continue;

		render->SetBlend(current.m_flGlowAlpha);
		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(&vGlowColor[0]); // This only sets rgb, not alpha

		current.DrawModel();
	}

	pRenderContext->OverrideDepthFunc(false, SHADER_DEPTHFUNC_NEAREROREQUAL)
#else	// 2-pass as a proof of concept so I can take a nice screenshot.	
	pRenderContext->OverrideDepthEnable(true, false);

	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 2;
	stencilState.m_nWriteMask = 2;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
	stencilState.SetStencilState(pRenderContext);

	// Draw depthtest-passing pixels to the stencil buffer
	{
		render->SetBlend(0);
		pRenderContext->OverrideAlphaWriteEnable(true, false);
		pRenderContext->OverrideColorWriteEnable(true, false);

		for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
		{
			auto& current = m_GlowObjectDefinitions[i];
			if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
				continue;

			current.DrawModel();
		}
	}

	pRenderContext->OverrideAlphaWriteEnable(false, true);
	pRenderContext->OverrideColorWriteEnable(false, true);

	pRenderContext->OverrideDepthEnable(false, false);

	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 3;
	stencilState.m_nTestMask = 2;
	stencilState.m_nWriteMask = 1;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_NOTEQUAL;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_ZFailOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = glow_outline_effect_stencil_mode.GetBool() ? STENCILOPERATION_KEEP : STENCILOPERATION_REPLACE;
	stencilState.SetStencilState(pRenderContext);

	// Draw color+alpha, stenciling out pixels from the first pass
	render->SetBlend(1);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || !current.m_bRenderWhenOccluded || current.m_bRenderWhenUnoccluded)
			continue;

		const Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(vGlowColor.Base()); // This only sets rgb, not alpha

		current.DrawModel();
	}
#endif
}

void CGlowObjectManager::DrawGlowVisible(int nSplitScreenSlot, CMatRenderContextPtr& pRenderContext)
{
	ShaderStencilState_t stencilState;
	stencilState.m_bEnable = true;
	stencilState.m_nReferenceValue = 1;
	stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_ALWAYS;
	stencilState.m_PassOp = STENCILOPERATION_REPLACE;
	stencilState.m_FailOp = STENCILOPERATION_KEEP;
	stencilState.m_ZFailOp = glow_outline_effect_stencil_mode.GetBool() ? STENCILOPERATION_KEEP : STENCILOPERATION_REPLACE;

	stencilState.SetStencilState(pRenderContext);

	pRenderContext->OverrideDepthEnable(true, false);
	render->SetBlend(1);
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
	{
		auto& current = m_GlowObjectDefinitions[i];
		if (current.IsUnused() || !current.ShouldDraw(nSplitScreenSlot) || current.m_bRenderWhenOccluded || !current.m_bRenderWhenUnoccluded)
			continue;

		Vector vGlowColor = current.m_vGlowColor * current.m_flGlowAlpha;
		render->SetColorModulation(vGlowColor.Base()); // This only sets rgb, not alpha

		current.DrawModel();
	}
}

void CGlowObjectManager::ApplyEntityGlowEffects( const CViewSetup *pSetup, int nSplitScreenSlot, CMatRenderContextPtr &pRenderContext, float flBloomScale, int x, int y, int w, int h )
{
	const PIXEvent pixEvent(pRenderContext, "ApplyEntityGlowEffects");

	// Optimization: only do all the framebuffer shuffling if there's at least one glow to be drawn
	{
		bool atLeastOneGlow = false;

		for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++)
		{
			if (m_GlowObjectDefinitions[i].IsUnused() || !m_GlowObjectDefinitions[i].ShouldDraw(nSplitScreenSlot))
				continue;

			atLeastOneGlow = true;
			break;
		}

		if (!atLeastOneGlow)
			return;
	}

	ITexture* const pRtFullFrameFB0 = materials->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);
	ITexture* const pRtFullFrameFB1 = materials->FindTexture("_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET);

	pRenderContext->PushRenderTargetAndViewport();

	// Set backbuffer + hardware depth as MRT 0. We CANNOT CHANGE RENDER TARGETS after this point!!!
	// In CShaderAPIDx8::CreateDepthTexture all depth+stencil buffers are created with the "discard"
	// flag set to TRUE. Not sure about OpenGL, but according to
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb174356(v=vs.85).aspx, if you change
	// the depth+stencil buffer away from a buffer that has discard=TRUE, the contents become garbage.
	pRenderContext->SetRenderTargetEx(0, nullptr);

	// Save current backbuffer to _rt_FullFrameFB1
	pRenderContext->CopyRenderTargetToTexture(pRtFullFrameFB1);

	// Clear backbuffer color and stencil, keep depth for testing
	pRenderContext->ClearColor4ub(0, 0, 0, 0);
	pRenderContext->ClearBuffers(true, false, true);

	// Draw glow models
	{
		// Save modulation color and blend
		Vector vOrigColor;
		render->GetColorModulation(vOrigColor.Base());
		const float flOrigBlend = render->GetBlend();

		// Set override material for glow color
		g_pStudioRender->ForcedMaterialOverride(materials->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, true));
		pRenderContext->OverrideColorWriteEnable(true, true);
		pRenderContext->OverrideAlphaWriteEnable(true, true);

		// Draw "glow when visible" objects
		DrawGlowVisible(nSplitScreenSlot, pRenderContext);

		// Draw "glow when occluded" objects
		DrawGlowOccluded(nSplitScreenSlot, pRenderContext);

		// Draw "glow always" objects
		DrawGlowAlways(nSplitScreenSlot, pRenderContext);

		// Unset override material
		g_pStudioRender->ForcedMaterialOverride(NULL);

		// Restore modulation color and blend
		render->SetColorModulation(vOrigColor.Base());
		render->SetBlend(flOrigBlend);
		pRenderContext->OverrideDepthEnable(false, false);
	}

	// Copy MSAA'd glow models to _rt_FullFrameFB0
	pRenderContext->CopyRenderTargetToTexture(pRtFullFrameFB0);

	// Move original contents of the backbuffer from _rt_FullFrameFB1 to the backbuffer
	{
#if FIXED_COPY_TEXTURE_TO_RENDER_TARGET	// Coordinates don't seem to be mapped 1:1 properly, screen becomes slightly blurry
		pRenderContext->CopyTextureToRenderTargetEx(0, pRtFullFrameFB1, nullptr);
#else
		pRenderContext->SetStencilEnable(false);

		IMaterial* const pFullFrameFB1 = materials->FindMaterial("debug/debugfbtexture1", TEXTURE_GROUP_RENDER_TARGET);
		pFullFrameFB1->AddRef();
		pRenderContext->Bind(pFullFrameFB1);

		const int nSrcWidth = pSetup->width;
		const int nSrcHeight = pSetup->height;
		int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
		pRenderContext->GetViewport(nViewportX, nViewportY, nViewportWidth, nViewportHeight);

		pRenderContext->OverrideDepthEnable(true, false);
		{
			pRenderContext->DrawScreenSpaceRectangle(pFullFrameFB1,
				0, 0, nViewportWidth, nViewportHeight,
				0, 0, nSrcWidth - 1, nSrcHeight - 1,
				pRtFullFrameFB1->GetActualWidth(), pRtFullFrameFB1->GetActualHeight());
		}
		pRenderContext->OverrideDepthEnable(false, false);

		pFullFrameFB1->Release();
#endif
	}

	// Bloom glow models from _rt_FullFrameFB0 to backbuffer while stenciling out inside of models
	{
		// Set stencil state
		ShaderStencilState_t stencilState;
		stencilState.m_bEnable = true;
		stencilState.m_nWriteMask = 0; // We're not changing stencil
		stencilState.m_nReferenceValue = 1;
		stencilState.m_nTestMask = 1;
		stencilState.m_CompareFunc = STENCILCOMPARISONFUNCTION_NOTEQUAL;
		stencilState.m_PassOp = STENCILOPERATION_KEEP;
		stencilState.m_FailOp = STENCILOPERATION_KEEP;
		stencilState.m_ZFailOp = STENCILOPERATION_KEEP;
		stencilState.SetStencilState(pRenderContext);

		ITexture* const pRtQuarterSize1 = materials->FindTexture("_rt_SmallFB1", TEXTURE_GROUP_RENDER_TARGET);
		IMaterial* const pMatHaloAddToScreen = materials->FindMaterial("dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true);

		// Write to alpha
		pRenderContext->OverrideAlphaWriteEnable(true, true);

		const int nSrcWidth = pSetup->width;
		const int nSrcHeight = pSetup->height;
		int nViewportX, nViewportY, nViewportWidth, nViewportHeight;
		pRenderContext->GetViewport(nViewportX, nViewportY, nViewportWidth, nViewportHeight);

		// Draw quad
		pRenderContext->DrawScreenSpaceRectangle(pMatHaloAddToScreen,
			0, 0, nViewportWidth, nViewportHeight,
			0, 0, nSrcWidth / 4 - 1, nSrcHeight / 4 - 1,
			pRtQuarterSize1->GetActualWidth(),
			pRtQuarterSize1->GetActualHeight());
	}

	// Done with all of our "advanced" 3D rendering.
	pRenderContext->SetStencilEnable(false);
	pRenderContext->OverrideColorWriteEnable(false, false);
	pRenderContext->OverrideAlphaWriteEnable(false, false);
	pRenderContext->OverrideDepthEnable(false, false);

	pRenderContext->PopRenderTargetAndViewport();
}

void CGlowObjectManager::GlowObjectDefinition_t::DrawModel()
{
	if ( m_hEntity.Get() )
	{
		m_hEntity->DrawModel( STUDIO_RENDER );
		C_BaseEntity *pAttachment = m_hEntity->FirstMoveChild();

		while ( pAttachment != NULL )
		{
			if ( !g_GlowObjectManager.HasGlowEffect( pAttachment ) && pAttachment->ShouldDraw() )
			{
				pAttachment->DrawModel( STUDIO_RENDER );
			}
			pAttachment = pAttachment->NextMovePeer();
		}
	}
}

#endif // GLOWS_ENABLE