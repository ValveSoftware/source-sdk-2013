#pragma once

#include "baseentity.h"

#include "ihandleentity.h"


// Early concept code for entity component system (ECS)
// implementation. CBaseEntity should implement this interface,
// and then slowly be gutted of all other features until it's just
// an implementation for this. Those features can be made into
// components.
class IEntityContainer //: IHandleEntity
{
public:
	virtual ~IEntityContainer();

	// Using CBaseEntity as a component currently.
	virtual CBaseEntity* getComponent(int id);

	template<typename Type>
	virtual Type* getComponent();
};

