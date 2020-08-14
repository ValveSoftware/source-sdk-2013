//========= Mapbase - https://github.com/mapbase-source/source-sdk-2013 ============//
//
// Purpose: Copies size.
//
//=============================================================================

#include "cbase.h"


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPointCopySize : public CLogicalEntity
{
	DECLARE_CLASS( CPointCopySize, CLogicalEntity );
private:
	// The entity to copy the size from.
	// m_target is the target that receives it.
	string_t m_iszSizeSource;
	EHANDLE m_hSizeSource;
	EHANDLE m_hTarget;

	float m_flScale;

	void CopySize(CBaseEntity *pSource, CBaseEntity *pTarget);

	// Inputs
	void InputCopySize( inputdata_t &inputdata );
	void InputCopySizeToEntity( inputdata_t &inputdata );
	void InputCopySizeFromEntity( inputdata_t &inputdata );

	void InputSetTarget( inputdata_t &inputdata ) { BaseClass::InputSetTarget(inputdata); m_hTarget = NULL; }
	void InputSetSource( inputdata_t &inputdata ) { m_iszSizeSource = inputdata.value.StringID(); m_hSizeSource = NULL; }

	// Outputs
	COutputEvent m_OnCopy;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(point_copy_size, CPointCopySize);


BEGIN_DATADESC( CPointCopySize )

	// Keys
	DEFINE_KEYFIELD(m_iszSizeSource, FIELD_STRING, "source"),
	DEFINE_FIELD(m_hSizeSource, FIELD_EHANDLE),
	DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),

	DEFINE_INPUT(m_flScale, FIELD_FLOAT, "SetScale"),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "CopySize", InputCopySize ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "CopySizeToEntity", InputCopySizeToEntity ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "CopySizeFromEntity", InputCopySizeFromEntity ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetSource", InputSetSource ),

	// Outputs
	DEFINE_OUTPUT(m_OnCopy, "OnCopy"),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCopySize::CopySize(CBaseEntity *pSource, CBaseEntity *pTarget)
{
	const Vector cvecAlignMins = pSource->WorldAlignMins();
	const Vector cvecAlignMaxs = pSource->WorldAlignMaxs();

	Vector vecAlignMins = Vector(cvecAlignMins);
	Vector vecAlignMaxs = Vector(cvecAlignMaxs);

	if (m_flScale != 0.0f && m_flScale != 1.0f)
	{
		vecAlignMins *= m_flScale;
		vecAlignMaxs *= m_flScale;
	}

	pTarget->SetCollisionBounds(vecAlignMins, vecAlignMins);
	m_OnCopy.FireOutput(pTarget, this);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCopySize::InputCopySize( inputdata_t &inputdata )
{
	if (!m_hSizeSource)
		m_hSizeSource = gEntList.FindEntityByName(NULL, STRING(m_iszSizeSource), this, inputdata.pActivator, inputdata.pCaller);
	if (!m_hTarget)
		m_hTarget = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);

	if (!m_hSizeSource || !m_hTarget)
	{
		Warning("%s (%s): Could not find %s\n", GetClassname(), GetDebugName(), !m_hSizeSource ? "size source" : "target to copy size to");
		return;
	}

	CopySize(m_hSizeSource, m_hTarget);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCopySize::InputCopySizeToEntity( inputdata_t &inputdata )
{
	if (!m_hSizeSource)
		m_hSizeSource = gEntList.FindEntityByName(NULL, STRING(m_iszSizeSource), this, inputdata.pActivator, inputdata.pCaller);

	if (!m_hSizeSource)
	{
		Warning("%s (%s): Could not find size source\n", GetClassname(), GetDebugName());
		return;
	}

	if (!inputdata.value.Entity())
		return;

	CopySize(m_hSizeSource, inputdata.value.Entity());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCopySize::InputCopySizeFromEntity( inputdata_t &inputdata )
{
	if (!m_hTarget)
		m_hTarget = gEntList.FindEntityByName(NULL, STRING(m_target), this, inputdata.pActivator, inputdata.pCaller);

	if (!m_hTarget)
	{
		Warning("%s (%s): Could not find target to copy size to\n", GetClassname(), GetDebugName());
		return;
	}

	if (!inputdata.value.Entity())
		return;

	CopySize(inputdata.value.Entity(), m_hTarget);
}
