'''
Module space used in the "spy" and "cpy" console commands.

Import anything here you want to be available by default in these commands.
'''
import os
import sys
from imp import reload

import entities
from entities import entlist