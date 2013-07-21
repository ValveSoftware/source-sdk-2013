from _entitiesmisc import *
from _entities import *

# Lists of all entity classes
if isclient:
    list_ents = [ 
        C_BaseEntity, 
    ]
    
    # List of aliases
    CBaseEntity = C_BaseEntity

else:
    list_ents = [ 
        CBaseEntity, 

    ]

@classmethod    
def InitEntityClass(cls):
    """ Entity Class initializer, could be seen as a metaclass.
        It is called when the class is created and when a new map is loaded.
        Used for one time initializations per map.
    """
    #SetupClassFields(cls)
    #SetupInputMethods(cls)
    
# Bind the following methods to each entity class
for cls in list_ents:
    # InitEntityClass: Called on level initialization and on the first time the entity factory is initialized.
    cls.InitEntityClass = InitEntityClass
    
clstoclstype = {
    'CBaseEntity' : ('@PointClass', ['Targetname', 'Origin']),
    'CFuncBrush' : ('@SolidClass', ['Targetname', 'Parentname', 'Origin', 'RenderFields', 'Global', 'Inputfilter', 'EnableDisable', 'Shadow']),
    'CBaseTrigger' : ('@SolidClass', ['Targetname', 'Parentname', 'Origin', 'RenderFields', 'Global', 'Inputfilter', 'EnableDisable', 'Shadow']),
    'CBaseFilter' : ('@FilterClass', ['BaseFilter']),
}
def DetermineClsType(cls):
    if cls.__name__ in clstoclstype:
        return clstoclstype[cls.__name__]
    for basecls in cls.__bases__:
        rs = DetermineClsType(basecls)
        if rs: return rs
    return None

def networked(cls):
    ''' Makes the class networked, which can serve as a base for entities which don't need to be
        networked. '''
    if 'networkinst' not in cls.__dict__:
        networkname = '%s.__%s' % (cls.__module__, cls.__name__)
        cls.networkinst = NetworkedClass(networkname, cls)
    return cls
    
def entity( clsname, 
            networked=False, 
            helpstring='',
            clstype='',
            entityextraproperties='',
            base=[],
            studio='',
            iconsprite='',
            cylinder=[],
            color='',
            size = '',
            cppproperties='',
            nofgdentry=False):
    ''' Decorator for turning a class into an entity.
        The class entity must be derived from CBaseEntity.'''
    def wrapcls(cls):
        # FIXME: This creates a circular reference between the class and factory/network instance
        #        Although new factories will remove the old factories, it does not clean up nicely yet.
        factoryname = 'factory__%s' % (clsname)
        factory = EntityFactory(clsname, cls)
        factory.entityname = clsname
        factory.clstype = clstype
        factory.entityextraproperties = entityextraproperties
        factory.cppproperties = cppproperties
        factory.helpstring = helpstring
        factory.nofgdentry = nofgdentry
        
        factory.fgdbase = base
        factory.fgdstudio = studio
        factory.fgdiconsprite = iconsprite
        factory.fgdcylinder = cylinder
        factory.fgdcolor = color
        factory.fgdsize = size
        
        setattr(cls, factoryname, factory)
        
        if not factory.clstype:
            info = DetermineClsType(cls)
            if info: factory.clstype = info[0]
        else: info = None
        
        if not factory.fgdbase:
            if info: factory.fgdbase = info[1]

        if networked and 'networkinst' not in cls.__dict__:
            networkname = '%s.__%s' % (cls.__module__, cls.__name__)
            cls.networkinst = NetworkedClass(networkname, cls)
            
        # Initialize the class so the fields are setup
        cls.InitEntityClass()
            
        return cls
    return wrapcls
    