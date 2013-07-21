from srcpy.module_generators import SemiSharedModuleGenerator

class Animation(SemiSharedModuleGenerator):
    module_name = '_animation'
    
    files = [
        'bone_setup.h',
        'eventlist.h',
        'animation.h',
        'ai_activity.h',
        'activitylist.h',
        'npcevent.h',
        'srcpy_animation.h',
    ]
    
    def Parse(self, mb):
        # Exclude everything by default
        mb.decls().exclude()
        
        # Useful bone_setup functions
        mb.free_function('Py_Studio_GetMass').include()
        mb.free_function('Py_Studio_GetMass').rename('Studio_GetMass')
        
        # Event list
        mb.free_function('EventList_RegisterPrivateEvent').include()
        mb.free_function('EventList_IndexForName').include()
        mb.free_function('EventList_NameForIndex').include()
        mb.free_function('ResetEventIndexes').include()
        
        # Animation
        mb.free_function('BuildAllAnimationEventIndexes').include()
        mb.free_function('LookupActivity').include()
        mb.free_function('LookupSequence').include()
        mb.free_function('GetSequenceName').include()
        mb.free_function('GetSequenceActivityName').include()
        mb.free_function('GetSequenceFlags').include()
        mb.free_function('GetAnimationEvent').include()
        mb.free_function('HasAnimationEventOfType').include()
        
        # Activity
        mb.free_function('ActivityList_RegisterPrivateActivity').include()
        mb.free_function('ActivityList_IndexForName').include()
        mb.free_function('ActivityList_NameForIndex').include()
        mb.free_function('IndexModelSequences').include()
        mb.free_function('ResetActivityIndexes').include()
        mb.free_function('VerifySequenceIndex').include()
        
        # Enums
        mb.enum('Animevent').include()