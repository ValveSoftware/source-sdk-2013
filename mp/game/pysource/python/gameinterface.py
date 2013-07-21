from _gameinterface import *

if isclient:
    # List of aliases
    CRecipientFilter = C_RecipientFilter

# Ensure old ConVars killed before creating a new ConVar
# Do not use __RealConVar directly, otherwise it might result in an invalid ConVar when 
# replacing the convar with a new one!
__RealConVar = ConVar
def ConVar(name, *args, **kwargs):
    PyShutdownConVar(name)
    return __RealConVar(name, *args, **kwargs)
    
__RealConCommand = ConCommand
def ConCommand(name, *args, **kwargs):
    PyShutdownConCommand(name)
    return __RealConCommand(name, *args, **kwargs)
    
# Decorator
def concommand(*args, **kwargs):
    """ Bind the function to a console command.
    
        Args:
    
        Kwargs:
        
           name (str): Name of console command.
           
           
           helpstring (str): Help string for console command.
           
           
           flags (int): Flags for console command (FCVAR_*).
           
           
           completionfunc (method): Auto complete function.
           
           
           useweakref (bool): If True, the concommand stores a weakref to the method.
           
    """
    def createconcommand(method):
        # Add method to the arguments list
        newargs = list(args)
        if not newargs:
            kwargs['method'] = method
            if 'name' not in kwargs.keys():
                kwargs['name'] = method.__name__
        else:
            newargs.insert(1,method)
        kwargs['useweakref'] = True
        # Bind concommand to the method. The concommand itself stores the method as a weakref.
        method.concommand = ConCommand(*newargs, **kwargs)
        return method
    return createconcommand

# Autocompletion class
class AutoCompletion(object):
    """ Simple auto completion class for use with ConCommand.
    
        input:
        fnlist - method that returns the complete list of keywords
    """
    def __init__(self, fnlist):
        super(AutoCompletion, self).__init__()
        self.fnlist = fnlist
        
    def __call__(self, partial):
        # Get the list of keywords
        keywords = self.fnlist()
        
        # Split into a command the remaining part
        try:
            command, partial = partial.split(' ', 1)
        except IndexError:
            command = partial.rstrip()
            partial = ''
        
        # Filter based on partial
        if partial:
            keywords = filter(lambda keyword: keyword.lower().startswith(partial), keywords)
            
        # Return keywords (with the command in front of each keyword)
        keywords = map(lambda keyword: '%s %s' % (command, keyword), keywords)
        return sorted(keywords)
