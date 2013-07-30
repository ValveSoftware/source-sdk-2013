from pygccxml.declarations import matchers, cpptypes

class calldef_withtypes(matchers.custom_matcher_t):
    def __init__(self, matchtypes=None):
        super(calldef_withtypes, self).__init__(self.testdecl)
        
        self.matchtypes = matchtypes
        
    def __compare_types( self, type_or_str, type ):
        assert type_or_str
        if type is None:
            return False
        if isinstance( type_or_str, cpptypes.type_t ):
            if type_or_str != type:
                return False
        else:
            if type_or_str != type.decl_string:
                return False
        return True
        
    def testdecl( self, decl ):
        if self.matchtypes:
            if isinstance(self.matchtypes, (list, tuple)):
                matchtypes = self.matchtypes
            else:
                matchtypes = [self.matchtypes]
                
            for t in matchtypes:
                if self.__compare_types( t, decl.return_type ):
                    return True
        
                for arg in decl.arguments:
                    if self.__compare_types( t, arg.type ):
                        return True
        
        return False