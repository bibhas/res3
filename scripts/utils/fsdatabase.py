#!/usr/bin/env python3

import os, sys, contextlib, platform
import shutil, pathlib
import time 

class FSDatabaseReader:
  # Util class
  class DatabaseInfo:
    def __init__(self, path, name):
      self.path = path
      self.name = name
      # Factory (static) method
  @classmethod
  def load(cls, db_dirname, start=None):
    def find_dir_in_parent_dirs(dirname, start=None):
      if start is None:
        start = os.getcwd()
      items = os.listdir(path=start)
      candidatepath = os.path.join(start, dirname)
      if dirname in items and os.path.isdir(candidatepath):
        return candidatepath
      if start == "/":
        # We've already search "/" and there's
        # no where else to search
        return None
      return find_dir_in_parent_dirs(dirname, os.path.split(start)[0])
    if start is None:
      start = os.getcwd()
    build_info_path = find_dir_in_parent_dirs(db_dirname, start=start)
    if build_info_path is None:
      print(f"Could not find {db_dirname} dir (started at : {start})")
      return None
    resp = cls()
    for root, _, files in os.walk(build_info_path):
      for f in files:
        filepath = os.path.join(root, f)
        with open(filepath, "r") as file:
          resp.infos[f] = {
            "path" : file.read(),
            "time" : get_formatted_local_time(os.path.getmtime(filepath))
          }
    resp.db = FSDatabaseReader.DatabaseInfo(
      path = pathlib.Path(build_info_path).parents[0],
      name = db_dirname
    )
    return resp
  # Constructor
  def __init__(self):
    self.db = None
    self.infos = {}
  def path_to_str(self, p):
    return str(p)
  
def get_formatted_local_time(t):
  return time.strftime('%c', time.localtime(t))
