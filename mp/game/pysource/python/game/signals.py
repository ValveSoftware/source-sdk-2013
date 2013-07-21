"""
Game signals.
"""
from . dispatch import Signal
from collections import defaultdict

def FireSignalRobust(s, **kwargs):
    responses = s.send_robust(None, **kwargs)
    for r in responses:
        if isinstance(r[1], Exception):
            PrintWarning('Error in receiver %s (module: %s): %s\n' % (r[0], r[0].__module__, r[1]))

class LevelInitSignal(Signal):
    """ Special signal class for levelinit signals.
    
        Takes an additional argument to immediately call if the level is already initialized.
        Convenience.
    """
    def connect(self, receiver, sender=None, weak=True, dispatch_uid=None, callifinit=False):
        super(LevelInitSignal, self).connect(receiver, sender, weak, dispatch_uid)
        # TODO?
        #if callifinit and srcmgr.levelinit:
        #    receiver()
            
# The level signals are send from src_python.cpp.
# Send at level initialization before entities are spawned
prelevelinit = LevelInitSignal()
map_prelevelinit = defaultdict(lambda : LevelInitSignal())

# Send at level initialization after entities are spawned
postlevelinit = LevelInitSignal()
map_postlevelinit = defaultdict(lambda : LevelInitSignal())

# Send at level shutdown before entities are removed
prelevelshutdown = Signal()
map_prelevelshutdown = defaultdict(lambda : Signal())

# Send at level shutdown after entities are removed
postlevelshutdown = Signal()
map_postlevelshutdown = defaultdict(lambda : Signal())

'''
# Examples of several signals:
from core.dispatch import receiver

@receiver(prelevelinit)
def on_prelevelinit(sender, **kwargs):
    print "Pre Level init callback!"
    
@receiver(map_prelevelinit['wmp_forest'])
def on_prelevelinit(sender, **kwargs):
    print "Pre Level init callback wmp_forest!"
    
@receiver(postlevelinit)
def on_postlevelinit(sender, bla, **kwargs):
    print "Post Level init callback!"
    
@receiver(prelevelshutdown)
def on_prelevelshutdown(sender, **kwargs):
    print "Pre Level shutdown callback!"
    
@receiver(postlevelshutdown)
def on_postlevelshutdown(sender, **kwargs):
    print "Post Level shutdown callback!"
    
'''