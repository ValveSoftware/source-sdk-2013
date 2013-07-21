""" Converters between python dictionaries and keyvalues. Can also load a keyvalues file directly into dictionaries. 
    Description of KeyValues copies from KeyValues.h:
    
    Purpose: Simple recursive data access class
            Used in vgui for message parameters and resource files
            Destructor deletes all child KeyValues nodes
            Data is stored in key (string names) - (string/int/float)value pairs called nodes.
    
    About KeyValues Text File Format:
    
    It has 3 control characters '{', '}' and '"'. Names and values may be quoted or
    not. The quote '"' charater must not be used within name or values, only for
    quoting whole tokens. You may use escape sequences wile parsing and add within a
    quoted token a \" to add quotes within your name or token. When using Escape
    Sequence the parser must now that by setting KeyValues::UsesEscapeSequences( true ),
    which it's off by default. Non-quoted tokens ends with a whitespace, '{', '}' and '"'.
    So you may use '{' and '}' within quoted tokens, but not for non-quoted tokens.
    An open bracket '{' after a key name indicates a list of subkeys which is finished
    with a closing bracket '}'. Subkeys use the same definitions recursively.
    Whitespaces are space, return, newline and tabulator. Allowed Escape sequences
    are \n, \t, \\, \n and \". The number character '#' is used for macro purposes 
    (eg #include), don't use it as first charater in key names.    
"""
from srcbuiltins import KeyValues, KeyValuesDumpAsDevMsg

def kv2dict(kv):
    ''' Convert a KeyValues instance to a dictionaries. 
        Subkeys become new dictionaries and the other values become strings,
        since we don't know what values they should be. '''
    d = {}
    
    # First load all sub keys recursively
    subkey = kv.GetFirstTrueSubKey()
    while subkey:
        KeyValuesDumpAsDevMsg(subkey)
        name = subkey.GetName()
        if name not in d:
            d[name] = []
        d[name].append(kv2dict(subkey))
        KeyValuesDumpAsDevMsg(subkey)
        subkey = subkey.GetNextTrueSubKey()
        
    # Then load all values (just convert everything to a string)
    value = kv.GetFirstValue()
    while value:
        KeyValuesDumpAsDevMsg(value)
        name = value.GetName()
        if name not in d:
            d[name] = []
        d[name].append(value.GetString())
        KeyValuesDumpAsDevMsg(value)
        value = value.GetNextValue()
        #print value

    return d
    
def dict2kv(d, name='Data'):
    ''' Converts a Python dictionary to a KeyValues instance. '''
    kv = KeyValues(name)
    
    for k, v in d.iteritems():
        if type(v) == dict:
            subkey = KeyValues(k)
            subkey = dict2kv(v, name=k)
            kv.AddSubKey(subkey)
        else:
            kv.SetString(k, str(v))
    
    return kv
 
# This function might need some more testing, but I think it should be correct.  
def LoadFileIntoDictionaries(path, getfirstkey=True):
    try:
        fp = open(path)
    except IOError:
        return None

    keystack = [{}]
    for line in fp:
        parsed = line.strip().split()
        if len(parsed) == 0:
            continue
        if parsed[0].startswith('{') or parsed[0].startswith('//'):
            continue
        if parsed[0].startswith('}'):
            keystack.pop()
            continue
        if len(parsed) == 1 or parsed[1].startswith('//'):
            newsubkey = {}
            keystack[len(keystack)-1][parsed[0].lstrip('"').strip('"')] = newsubkey
            keystack.append(newsubkey)
            continue
        
        keystack[len(keystack)-1][parsed[0].lstrip('"').strip('"')] = parsed[1].lstrip('"').strip('"')
        
    fp.close()
    
    try:
        if getfirstkey:
            return keystack[0].values()[0]
        return keystack[0]
    except IndexError:
        print('Invalid vmt file %s' % (path))
        return []

# Debug
def StartPrintDictionaryKV(d):
    assert(len(d.keys()) == 1)
    assert(type(d.values()[0]) == dict)
    PrintDictionaryKV(d.keys()[0], d.values()[0])
    
def PrintDictionaryKV(name, d, indent="\t"):
    print(name)
    print('{')
    for k, v in d.iteritems():
        if type(v) == dict:
            PrintDictionaryKV(k, v, indent+'\t')
        else:
            print(indent+k+'\t'+v)
    print('}')
    