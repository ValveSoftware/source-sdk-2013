#ifndef C_MUZZLEFLASH_EFFECT_H
#define C_MUZZLEFLASH_EFFECT_H


class C_MuzzleflashEffect
{
public:

	C_MuzzleflashEffect();
	~C_MuzzleflashEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flStrength );

	float GetHorizontalFOV() const { return m_flHorizontalFOV; }

protected:

	void UpdateLightNew(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp);

	ClientShadowHandle_t m_FlashlightHandle;
	CTextureReference m_FlashlightTexture;

	float m_flHorizontalFOV;
};

#endif