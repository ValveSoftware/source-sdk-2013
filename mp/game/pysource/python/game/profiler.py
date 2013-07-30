import cProfile
from collections import defaultdict

from srcbase import RegisterTickMethod, UnregisterTickMethod
from gameinterface import ConCommand, engine
    
from core.usermessages import usermessage

# Is profiling on?
profiling_on = False

prs = defaultdict(cProfile.Profile)

# Enable/disable
def StartProfiler(name):
    if profiling_on:
        prs[name].enable(True,True)
    
def EndProfiler(name):
    prs[name].disable()
    
# Decorator
def profile(name):
    """ Use this decorator to profile a method
    """
    def fnwrapper(fn):
        def fnwrapper2(*args):
            if profiling_on:
                StartProfiler(name)
                try:
                    rv = fn(*args)
                finally:
                    EndProfiler(name)
                return rv
            else:
                return fn(*args)
        return fnwrapper2
    return fnwrapper

# Print stats
def PrintStats(names):
    if type(names) != list:
        names = [names]
        
    for name in names:
        print('Printing stats for %s' % (name))
        try:
            prs[name].print_stats()
        except:
            print('No stats...')
    
def Reset(name):
    prs[name] = cProfile.Profile()
    
def ResetAll():
    global prs
    prs = defaultdict(cProfile.Profile)
    
def cc_profiling_start(args):
    global profiling_on
    profiling_on = True
    
def cc_profiling_stopandprint(args):
    global profiling_on
    profiling_on = False
    specific = None
    
    if len(args) > 1:
        specific = args[1]
        PrintStats(specific)
    else:
        for key in prs.keys():
            PrintStats(key)
    ResetAll()

profiling_timed = None
def cc_profiling_run(args):
    global profiling_on, profiling_timed
    if profiling_on:
        print('profiling_run already running!')
        return
    profiling_on = True
    profiling_timed = None
    if len(args) > 1:
        profiling_timed = []
        for i in range(2, len(args)):
            profiling_timed.append(args[i])
    print('Profiling started (%f seconds)' % (float(args[1])))
    RegisterTickMethod(profiling_run_end, float(args[1]), looped=False)
    
def profiling_run_end():
    global profiling_on, profiling_timed
    profiling_on = False
    print('profiling done (%s): ' % ('client' if isclient else 'server'))
    for key in prs.keys():
        PrintStats(key)
    ResetAll()
    
if isserver:
    profiling_start = ConCommand('profiling_start', cc_profiling_start, '', 0)
    profiling_stopandprint = ConCommand('profiling_stopandprint', cc_profiling_stopandprint, '', 0)
    profiling_run = ConCommand('profiling_run', cc_profiling_run, '', 0)
else:
    profiling_start = ConCommand('cl_profiling_start', cc_profiling_start, '', 0)
    profiling_stopandprint = ConCommand('cl_profiling_stopandprint', cc_profiling_stopandprint, '', 0)
    profiling_run = ConCommand('cl_profiling_run', cc_profiling_run, '', 0)
    
    
# VProf, used by benchmark map
@usermessage('startvprof')
def StartVProf(*args, **kwargs):
    engine.ExecuteClientCmd('vprof_reset')
    engine.ExecuteClientCmd('vprof_on')
    
@usermessage('endvprof')
def EndVProf(*args, **kwargs):
    engine.ExecuteClientCmd('vprof_off')
    engine.ExecuteClientCmd('vprof_generate_report_hierarchy')
    engine.ExecuteClientCmd('vprof_generate_report')
    engine.ExecuteClientCmd('con_logfile ""')
    