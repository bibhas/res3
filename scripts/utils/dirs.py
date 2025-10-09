#!/usr/bin/env python3

import os, pathlib

def script_dir(f=__file__):
  return os.path.abspath(os.path.dirname(f))

def script_dir_path(f=__file__):
  return pathlib.Path(os.path.abspath(os.path.dirname(f)))

def project_source_dir_path():
  return script_dir_path(__file__).parents[1]

def project_scripts_dir_path():
  return script_dir_path(__file__).parents[0]
