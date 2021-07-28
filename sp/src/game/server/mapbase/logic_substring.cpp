//====================== By Holly Liberatore / MoofEMP ======================//
//
// Purpose: Takes a string parameter and returns a substring defined by keyvalues
//
//===========================================================================//

#include "cbase.h"

#define SF_SUBSTRING_START_DISABLED (1 << 0)

class CLogicSubstring : public CLogicalEntity
{
public: 
	DECLARE_CLASS( CLogicSubstring, CLogicalEntity );
	DECLARE_DATADESC();

	CLogicSubstring( void ) { }
	
	void InputDisable( inputdata_t &inputData );
	void InputEnable( inputdata_t &inputData );
	void InputInValue( inputdata_t &inputData );
	void InputSetLength( inputdata_t &inputData );
	void InputSetStartPos( inputdata_t &inputData );
	
	void Spawn(void);

private:
	int m_nLength;
	int m_nStartPos;

	bool m_bEnabled;

	COutputString	m_OutValue;
};

LINK_ENTITY_TO_CLASS( logic_substring, CLogicSubstring );

BEGIN_DATADESC( CLogicSubstring )
	
	DEFINE_FIELD( m_bEnabled, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD(m_nLength, FIELD_INTEGER, "length" ),
	DEFINE_KEYFIELD(m_nStartPos, FIELD_INTEGER, "startPos" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_STRING, "InValue", InputInValue ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetLength", InputSetLength ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetStartPos", InputSetStartPos ),

	DEFINE_OUTPUT( m_OutValue, "OutValue" ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Disable or enable the entity (disabling prevents any input functions from running)
//-----------------------------------------------------------------------------
void CLogicSubstring::InputDisable( inputdata_t &inputData ) { m_bEnabled = false; }
void CLogicSubstring::InputEnable ( inputdata_t &inputData ) { m_bEnabled = true ; }

//-----------------------------------------------------------------------------
// Purpose: Trim substring from input
// Output: Substring
//-----------------------------------------------------------------------------
void CLogicSubstring::InputInValue( inputdata_t &inputData )
{
	if( !m_bEnabled ) return;

	int inputLength = Q_strlen(inputData.value.String());
	int startPosCheck = m_nStartPos < 0 ? inputLength + m_nStartPos : m_nStartPos;
	if( startPosCheck < 0 )
	{
		startPosCheck = 0;
	}
	int lengthCheck = (m_nLength < 0 || m_nLength > inputLength - startPosCheck ? inputLength - startPosCheck : m_nLength) + 1;
	if( lengthCheck < 1 || startPosCheck > inputLength )
	{
		m_OutValue.Set( MAKE_STRING(""), inputData.pActivator, this );
		return;
	}
	char* strOutValue = (char*)malloc( lengthCheck );
	Q_strncpy( strOutValue, inputData.value.String() + startPosCheck, lengthCheck );
	m_OutValue.Set( AllocPooledString(strOutValue), inputData.pActivator, this );
	free(strOutValue);
}

//-----------------------------------------------------------------------------
// Purpose: Setter methods for keyvalues
//-----------------------------------------------------------------------------
void CLogicSubstring::InputSetLength( inputdata_t &inputData )
{
	if( !m_bEnabled ) return;

	m_nLength = inputData.value.Int();
}

void CLogicSubstring::InputSetStartPos( inputdata_t &inputData )
{
	if( !m_bEnabled ) return;

	m_nStartPos = inputData.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: Respond to spawnflags when entity spawns
//-----------------------------------------------------------------------------
void CLogicSubstring::Spawn( void )
{
	m_bEnabled = !HasSpawnFlags( SF_SUBSTRING_START_DISABLED );
}
