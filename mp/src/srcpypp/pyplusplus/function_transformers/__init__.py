# Helper classes for wrapper function creation

"""This sub-package implements function transformation functionality"""

from .transformer import transformer_t
from . import transformers
from .function_transformation import function_transformation_t

def output( *args, **keywd ):
    def creator( function ):
        return transformers.output_t( function, *args, **keywd )
    return creator

def input( *args, **keywd ):
    def creator( function ):
        return transformers.input_t( function, *args, **keywd )
    return creator

def inout( *args, **keywd ):
    def creator( function ):
        return transformers.inout_t( function, *args, **keywd )
    return creator

def input_static_array( *args, **keywd ):
    def creator( function ):
        return transformers.input_static_array_t( function, *args, **keywd )
    return creator

def output_static_array( *args, **keywd ):
    def creator( function ):
        return transformers.output_static_array_t( function, *args, **keywd )
    return creator
 
def inout_static_array( *args, **keywd ):
    def creator( function ):
        return transformers.inout_static_array_t( function, *args, **keywd )
    return creator

def input_static_matrix( *args, **keywd ):
    def creator( function ):
        return transformers.input_static_matrix_t( function, *args, **keywd )
    return creator

def output_static_matrix( *args, **keywd ):
    def creator( function ):
        return transformers.output_static_matrix_t( function, *args, **keywd )
    return creator

def inout_static_matrix( *args, **keywd ):
    def creator( function ):
        return transformers.inout_static_matrix_t( function, *args, **keywd )
    return creator

def modify_type( *args, **keywd ):
    def creator( function ):
        return transformers.type_modifier_t( function, *args, **keywd )
    return creator

def input_c_buffer( *args, **keywd ):
    def creator( function ):
        return transformers.input_c_buffer_t( function, *args, **keywd )
    return creator

def transfer_ownership( *args, **keywd ):
    def creator( function ):
        return transformers.transfer_ownership_t( function, *args, **keywd )
    return creator

def from_address( *args, **keywd ):
    def creator( function ):
        return transformers.from_address_t( function, *args, **keywd )
    return creator


