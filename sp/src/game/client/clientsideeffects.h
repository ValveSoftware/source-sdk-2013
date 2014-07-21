//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CLIENTSIDEEFFECTS_H
#define CLIENTSIDEEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

class	Vector;
struct	FXQuadData_t;
struct	FXLineData_t;
//-----------------------------------------------------------------------------
// Purpose: Base class for client side effects
//-----------------------------------------------------------------------------
abstract_class CClientSideEffect
{
public:
	// Constructs the named effect
						CClientSideEffect( const char *name );
	virtual				~CClientSideEffect( void );

	// Update/Draw the effect
	// Derived classes must implement this method!
	virtual void		Draw( double frametime ) = 0;
	// Returns name of effect
	virtual const char	*GetName( void );
	// Retuns whether the effect is still active
	virtual bool		IsActive( void );
	// Sets the effect to inactive so it can be destroed
	virtual void		Destroy( void );
	
private:
	// Name of effect ( static data )
	const char			*m_pszName;
	// Is the effect active
	bool				m_bActive;
};

//-----------------------------------------------------------------------------
// Purpose: Base interface to effects list
//-----------------------------------------------------------------------------
abstract_class IEffectsList
{
public:
	virtual			~IEffectsList( void ) {}

	// Add an effect to the list of effects
	virtual void	AddEffect( CClientSideEffect *effect ) = 0;
	// Simulate/Update/Draw effects on list
	virtual void	DrawEffects( double frametime ) = 0;
	// Flush out all effects fbrom the list
	virtual void	Flush( void ) = 0;
};

extern IEffectsList *clienteffects;

class IMaterialSystem;
extern IMaterialSystem *materials;

//Actual function references
void FX_AddCube( const Vector &mins, const Vector &maxs, const Vector &vColor, float life, const char *materialName );
void FX_AddCenteredCube( const Vector &center, float size, const Vector &vColor, float life, const char *materialName );
void FX_AddStaticLine( const Vector& start, const Vector& end, float scale, float life, const char *materialName, unsigned char flags );
void FX_AddDiscreetLine( const Vector& start, const Vector& direction, float velocity, float length, float clipLength, float scale, float life, const char *shader );

void FX_AddLine( const FXLineData_t &data );

void FX_AddQuad( const FXQuadData_t &data );

void FX_AddQuad( const Vector &origin, 
				 const Vector &normal, 
				 float startSize, 
				 float endSize, 
				 float sizeBias,
				 float startAlpha, 
				 float endAlpha,
				 float alphaBias,
				 float yaw,
				 float deltaYaw,
				 const Vector &color, 
				 float lifeTime, 
				 const char *shader, 
				 unsigned int flags );

// For safe addition of client effects
void SetFXCreationAllowed( bool state );
bool FXCreationAllowed( void );

#endif // CLIENTSIDEEFFECTS_H
