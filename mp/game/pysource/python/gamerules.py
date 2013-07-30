from _gamerules import *
import traceback

# Helpers for ammo definitions
POUNDS_PER_KG = 2.2
KG_PER_POUND = (1.0/POUNDS_PER_KG)
def lbs2kg(x): return ((x)*KG_PER_POUND)
def kg2lbs(x): return ((x)*POUNDS_PER_KG)
def BULLET_MASS_GRAINS_TO_LB(grains): return (0.002285*(grains)/16.0)
def BULLET_MASS_GRAINS_TO_KG(grains): return lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))
BULLET_IMPULSE_EXAGGERATION = 1
def BULLET_IMPULSE(grains, ftpersec): return ((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION) 

# Team Defines
TEAM_ANY = -1 # for some team query methods
TEAM_INVALID = -1
TEAM_UNASSIGNED = 0 # not assigned to a team
TEAM_SPECTATOR = 1 # spectator team
# Start your team numbers after this
LAST_SHARED_TEAM = TEAM_SPECTATOR

# Proxy for safe gamerules method calling
# As alternative for GameRules()
class SafeAttr(object):
    def __call__(self, *args, **kwargs):
        pass
        
    def __getattribute__(self, name):
        return SafeAttr()
        
    def __getattr__(self, attr):
        return SafeAttr()
        
    def __setattr__(self, attr, value):
        pass

class GameRulesProxy(object):
    def __getattr__(self, name):
        if name == 'gamerules': return self()
        try:
            return getattr(object.__getattribute__(self, 'gamerules'), name)
        except AttributeError:
            traceback.print_exc()
            return SafeAttr()
            
    def __getattribute__(self, name):
        if name == 'gamerules': return self()
        try:
            return getattr(object.__getattribute__(self, 'gamerules'), name)
        except AttributeError:
            PrintWarning('Exception occured in calling a gamerules method: \n')
            traceback.print_exc()
            return SafeAttr()
            
    def __setattr__(self, name, value):
        if name == 'gamerules': 
            object.__setattr__(self, 'gamerules', value)
            return
        setattr(self.gamerules, name, value)
        
    def __call__(self):
        return object.__getattribute__(self, 'gamerules')
        
    def __nonzero__(self):
        return bool(self.gamerules)
        
    gamerules = None
    
gamerules = GameRulesProxy()
