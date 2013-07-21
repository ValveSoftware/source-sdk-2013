from _physics import *

# VPHYSICS object game-specific flags
FVPHYSICS_DMG_SLICE = 0x0001		    # does slice damage, not just blunt damage
FVPHYSICS_CONSTRAINT_STATIC = 0x0002	# object is constrained to the world, so it should behave like a static
FVPHYSICS_PLAYER_HELD = 0x0004		    # object is held by the player, so have a very inelastic collision response
FVPHYSICS_PART_OF_RAGDOLL = 0x0008		# object is part of a client or server ragdoll
FVPHYSICS_MULTIOBJECT_ENTITY = 0x0010	# object is part of a multi-object entity
FVPHYSICS_HEAVY_OBJECT = 0x0020		    # HULK SMASH! (Do large damage even if the mass is small)
FVPHYSICS_PENETRATING = 0x0040		    # This object is currently stuck inside another object
FVPHYSICS_NO_PLAYER_PICKUP = 0x0080		# Player can't pick this up for some game rule reason
FVPHYSICS_WAS_THROWN = 0x0100		    # Player threw this object
FVPHYSICS_DMG_DISSOLVE = 0x0200		    # does dissolve damage, not just blunt damage
FVPHYSICS_NO_IMPACT_DMG = 0x0400		# don't do impact damage to anything
FVPHYSICS_NO_NPC_IMPACT_DMG = 0x0800	# Don't do impact damage to NPC's. This is temporary for NPC's shooting combine balls (sjb)
FVPHYSICS_NO_SELF_COLLISIONS = 0x8000	# don't collide with other objects that are part of the same entity

# callbackflags
CALLBACK_GLOBAL_COLLISION	= 0x0001
CALLBACK_GLOBAL_FRICTION	= 0x0002
CALLBACK_GLOBAL_TOUCH		= 0x0004
CALLBACK_GLOBAL_TOUCH_STATIC = 0x0008
CALLBACK_SHADOW_COLLISION	= 0x0010
CALLBACK_GLOBAL_COLLIDE_STATIC = 0x0020
CALLBACK_IS_VEHICLE_WHEEL	= 0x0040
CALLBACK_FLUID_TOUCH		= 0x0100
CALLBACK_NEVER_DELETED		= 0x0200	# HACKHACK: This means this object will never be deleted (set on the world)
CALLBACK_MARKED_FOR_DELETE	= 0x0400	# This allows vphysics to skip some work for this object since it will be
                                        # deleted later this frame. (Set automatically by destroy calls)
CALLBACK_ENABLING_COLLISION = 0x0800	# This is active during the time an object is enabling collisions
                                        # allows us to skip collisions between "new" objects and objects marked for delete
CALLBACK_DO_FLUID_SIMULATION = 0x1000   # remove this to opt out of fluid simulations
CALLBACK_IS_PLAYER_CONTROLLER= 0x2000	# HACKHACK: Set this on players until player cotrollers are unified with shadow controllers
CALLBACK_CHECK_COLLISION_DISABLE = 0x4000
CALLBACK_MARKED_FOR_TEST	= 0x8000	# debug -- marked object is being debugged

# Convenience routine
# ORs gameFlags with the physics object's current game flags
def PhysSetGameFlags(phys, gameFlags):
    flags = phys.GetGameFlags()
    flags |= gameFlags
    phys.SetGameFlags(flags)
    
    return flags

# mask off gameFlags
def PhysClearGameFlags(phys, gameFlags):
    flags = phys.GetGameFlags()
    flags &= ~gameFlags
    phys.SetGameFlags(flags)

    return flags