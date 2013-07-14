# Copyright 2004-2008 Roman Yakovenko.
# Distributed under the Boost Software License, Version 1.0. (See
# accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

"""This package defines all user messages( warnings + errors ), which will be
reported to user.
"""

from . import warnings_
from .warnings_ import *

def find_out_message_id( msg ):
    return msg.identifier

DISABLE_MESSAGES = [
    W1000, W1001, W1002, W1011, W1012, W1013, W1015, W1019, W1030, W1034, W1039
]
#Messages kept by DISABLE_MESSAGES list will not be reported

def disable( *args ):
    DISABLE_MESSAGES.extend( args )

def filter_disabled_msgs( msgs, disable_messages=None ):
    report = []

    skip_them = DISABLE_MESSAGES[:]
    if disable_messages:
        skip_them.extend( disable_messages )

    skip_them = [_f for _f in map( find_out_message_id, skip_them ) if _f]

    for msg in msgs:
        msg_id = find_out_message_id( msg )
        if msg_id and msg_id not in skip_them:
            report.append( msg )

    return report





