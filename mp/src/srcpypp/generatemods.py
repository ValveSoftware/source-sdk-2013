#!/usr/bin/env python3
import sys
import argparse
import traceback

import srcpy

import settings

def Main():
    # Setup arguments
    parser = argparse.ArgumentParser(description='Generates Python modules for Source Engine')
    parser.add_argument('-a', '--appendonly',
                       help='Only generate append file for modules listed in settings.')
    parser.add_argument('-m', '--specificmodule',
                       help='Only parse code for the single specified module')
    parser.add_argument('-s', '--settings', default='settings',
                       help='Python module containing settings')
                       
    args = parser.parse_args()
    
    # Import the settings module
    try:
        settings = __import__(args.settings)
    except ImportError:
        print('Invalid settings module specified.')
        traceback.print_exc()
        return
        
    # Generate modules using srcpy
    srcpy.ParseModules(settings, specificmodule=args.specificmodule, appendfileonly=args.appendonly)

if __name__ == "__main__":
    Main()