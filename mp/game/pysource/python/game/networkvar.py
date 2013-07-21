from collections import defaultdict
import types

def SetupNetworkVar(cls, name, networkvarname):
    """ Creates a property with the given name 
        Calls the setter of the network class which actually
        contains the data. """
    # Skip if the property is already setup
    try:
        p = getattr(cls, name)
        if type(p) == property:
            return
    except AttributeError:
        pass
        
    # Define the property
    getter = lambda self: getattr(self, networkvarname).Get()
    def setter(self, value):
        getattr(self, networkvarname).Set(value)
    p = property(getter, setter, None, '%s networkvar property' % (name))
    setattr(cls, name, p)

# SERVER
if isserver:
    from _entities_misc import NetworkVarInternal, NetworkArrayInternal, NetworkDictInternal

    class NetworkVar(NetworkVarInternal):
        def __init__(self, ent, name, data, changedcallback=None, sendproxy=None):
            super(NetworkVar, self).__init__(ent, name, data, changedcallback=bool(changedcallback), sendproxy=sendproxy)
            networkvarname = '_networkvar_%s' % (name)
            setattr(ent, networkvarname, self)
            SetupNetworkVar(ent.__class__, name, networkvarname)
            
    class NetworkArray(NetworkArrayInternal):
        def __init__(self, ent, name, data=None, changedcallback=None, sendproxy=None):
            if not data: data = list()
            super(NetworkArray, self).__init__(ent, name, data, changedcallback=bool(changedcallback), sendproxy=sendproxy)
            setattr(ent, name, self)
            self.data = data
            
        # TODO: Make this nicer
        def append(self, item):
            self.data.append(None)
            self[-1] = item
        def remove(self, item):
            idx = self.data.index(item)
            self[idx] = None
            del self.data[idx]
            
        # List methods
        def __contains__(self, item):
            return self.data.__contains__(item)
        def __len__(self):
            return self.data.__len__()
        def __iter__(self):
            return self.data.__iter__()
        def __concat__(self, other):
            return self.data.__concat__(other)
        def __add__(self, other):
            return self.data.__add__(other)
        def __sub__(self, other):
            return self.data.__sub__(other)
            
    class NetworkDict(NetworkDictInternal):
        def __init__(self, ent, name, data=None, changedcallback=None, sendproxy=None):
            if data == None: data = dict()
            super(NetworkDict, self).__init__(ent, name, data, changedcallback=bool(changedcallback), sendproxy=sendproxy)
            setattr(ent, name, self)
            self.data = data
            
        # TODO: missing dict methods?
        def __contains__(self, item):
            return self.data.__contains__(item)
        def __len__(self):
            return self.data.__len__()
        def __iter__(self):
            return self.data.__iter__()
            
        def keys(self):
            return self.data.keys()
        def values(self):
            return self.data.values()
        def items(self):
            return self.data.items()
        def has_key(self, key):
            return self.data.has_key(key)
        def get(self, key, default=None):
            return self.data.get(key, default)
        def clear(self):
            return self.data.clear()
        def setdefault(self, key, default=None):
            return self.data.setdefault(key, default)
        def iterkeys(self):
            return self.data.iterkeys()
        def itervalues(self):
            return self.data.itervalues()
        def iteritems(self):
            return self.data.iteritems()
        def pop(self, key, default=None):
            return self.data.pop(key, default)
        def popitem(self):
            return self.data.popitem()
        def copy(self):
            return self.data.copy()
        def update(self, other=None):
            return self.data.update(other)
            
    class NetworkDefaultDict(NetworkDict):
        def __init__(self, ent, name, data=None, changedcallback=None, sendproxy=None, default=None):
            if type(default) != types.FunctionType:
                defaultvalue = default # Must rename!
                default = lambda: defaultvalue
            if data == None: data = defaultdict(default)
            elif type(data) != defaultdict:
                origdata = data
                data = defaultdict(default)
                data.update(origdata)
                
            super(NetworkDefaultDict, self).__init__(ent, name, data, changedcallback=changedcallback, sendproxy=sendproxy)

# CLIENT
else:
    # The client version is actually not needed
    # It just creates the default variable, so if the network variable is not yet
    # initialized on the client you will not get an attribute error
    class NetworkVar(object):
        def __init__(self, ent, name, data, changedcallback=None, sendproxy=None):
            super(NetworkVar, self).__init__()
            setattr(ent, name, data)
            
            if changedcallback:
                setattr(ent, '__%s__Changed' % (name), 
                        getattr(ent, changedcallback))
                        
    class NetworkArray(object):
        def __init__(self, ent, name, data=None, changedcallback=None, sendproxy=None):
            if data == None: data = list()
            super(NetworkArray, self).__init__()
            setattr(ent, name, data)
            
            if changedcallback:
                setattr(ent, '__%s__Changed' % (name), 
                        getattr(ent, changedcallback))
                        
    class NetworkDict(object):
        def __init__(self, ent, name, data=None, changedcallback=None, sendproxy=None):
            if data == None: data = dict()
            super(NetworkDict, self).__init__()
            setattr(ent, name, data)
            
            if changedcallback:
                setattr(ent, '__%s__Changed' % (name), 
                        getattr(ent, changedcallback))
                        
    class NetworkDefaultDict(NetworkDict):
        def __init__(self, ent, name, data=None, changedcallback=None, default=None, sendproxy=None):
            if type(default) != types.FunctionType:
                defaultvalue = default # Must rename!
                default = lambda: defaultvalue
            if data == None: data = defaultdict(default)
            if type(data) != defaultdict:
                origdata = data
                data = defaultdict(default)
                data.update(origdata)
            super(NetworkDefaultDict, self).__init__(ent, name, data, changedcallback)
        
        
# Send/recv prop versions (really just wrappers around existing send/recv props)
class NetworkVarProp(object):
    def __init__(self, ent, name, propname):
        super(NetworkVarProp, self).__init__()
        self.propname = propname
        self.ent = ent
        networkvarname = '_networkvar_%s' % (name)
        setattr(ent, networkvarname, self)
        SetupNetworkVar(ent.__class__, name, networkvarname)
        
    def Get(self):
        return getattr(self.ent, self.propname)
    def Set(self, value):
        setattr(self.ent, self.propname, value)
        