#!/usr/bin/env python3

import os, shutil, platform
from pathlib import Path
from distutils import dir_util
import utils

DEBUG: bool = False

# Wrappers for system functions

def __shutil_rmtree(dest_path: str) -> None:
  if not DEBUG:
    shutil.rmtree(dest_path)
  else:
    utils.LOG_DEBUG(f"Invoking shutil.rmtree({dest_path})")
    
def __dir_util_copy_tree(src: str, dest: str) -> None:
  if not DEBUG:
    dir_util.copy_tree(src, dest)
  else:
    utils.LOG_DEBUG(f"Invoking dir_util.copy_tree({src}, {dest})")

def __os_make_dirs(dest: str) -> None:
  if not DEBUG:
    os.makedirs(dest)
  else:
    utils.LOG_DEBUG(f"Invoking os.make_dirs({dest})")
  
def __shutil_copy(src: str, dest: str) -> None:
  if not DEBUG:
    shutil.copy(src, dest)
  else:
    utils.LOG_DEBUG(f"Invoking shutil.copy({src}, {dest})")

def __os_remove(src: str) -> None:
  if not DEBUG:
    os.remove(src)
  else:
    utils.LOG_DEBUG(f"Invoking os.remove({src})")

# Copy functions
    
def copy(src: Path, dest: Path, copy_dir_contents: bool = False, confirm: bool = True) -> bool:
  def __impl():
    if dest is None:
      utils.LOG_ERROR("dest is `None`!")
      return False
    if not src.exists():
      utils.LOG_ERROR(f"src `{src}` does not exist!!")
      return False
    if dest.exists() and not dest.is_dir():
      utils.LOG_ERROR(f"dest `{dest}` exists, and it's not a directory!!")
      return False
    if src.is_dir():
      dest_path = dest / src.name
      if dest_path.exists() and not copy_dir_contents:
        utils.LOG_DEBUG(f"- Deleting {dest_path} [confirm={confirm}]")
        if confirm and not utils.prompt_confirmation():
          utils.LOG_ERROR("x Abandoned operation x")
          return False
        __shutil_rmtree(str(dest_path))
      if copy_dir_contents:
        utils.LOG_DEBUG(f"Copying contents\n  from: {src}\n  to: {dest}")
        __dir_util_copy_tree(str(src), str(dest))
      else:
        utils.LOG_DEBUG(f"Copying dir\n  from: {src}\n  to: {dest}")
        __dir_util_copy_tree(str(src), str(dest_path))
    else:
      if not dest.exists():
        utils.LOG_DEBUG(f"Making directories at {dest}")
        __os_make_dirs(str(dest))
      utils.LOG_DEBUG(f"Copying file\n  from: {src}\n  to: {dest}")
      __shutil_copy(str(src), str(dest))
    return True
  try:
    return __impl()
  except (PermissionError, dir_util.DistutilsFileError) as e:
    utils.LOG_ERROR(f"Permission denied!", bold=True)
    return False

def platform_copy(src: Path, dest: utils.PlatformPath = None, copy_contents: bool = False, confirm: bool = True) -> bool:
  resolved_path = dest.resolve()
  if resolve_path is None:
    return False
  return copy(src=src, dest=resolved_path, copy_dir_contents=copy_contents, confirm=confirm)

def platform_copy(src: Path, dest: utils.PlatformPath = None, copy_contents: bool = False, confirm: bool = True) -> bool:
  return copy(src=src, dest=dest.resolve(), copy_dir_contents=copy_contents, confirm=confirm)

def platform_copy_dir_contents(src: Path, dest: utils.PlatformPath = None, confirm: bool = True) -> bool:
  return copy(src=src, dest=dest.resolve(), copy_dir_contents=True, confirm=confirm)

# Remove functions

def platform_remove_from_dir(item: str, src: utils.PlatformPath = None, confirm=True, complain=True) -> bool:
  def __impl():
    selected_dir: Path = src.resolve()
    if selected_dir is None:
      if complain:
        utils.LOG_ERROR("selected_dir is None!")
      return False
    target_path: Path = selected_dir / item
    utils.LOG_DEBUG(f"Deleting {target_path}")
    if confirm and not utils.prompt_confirmation():
      if complain:
        utils.LOG_ERROR("x Abandoned operation x")
      return False
    if not target_path.exists():
      if complain:
        utils.LOG_ERROR(f"target_path(`{target_path}`) doesn't exist!")
      return False
    if target_path.is_dir():
      __shutil_rmtree(str(target_path))
    else:
      __os_remove(str(target_path))
    return True
  try:
    return __impl()
  except PermissionError:
    if complain:
      utils.LOG_ERROR(f"Permission denied!", bold=True)
    return False

# Module entry point (for testing)
  
if __name__ == "__main__":
  def script_dir(f=__file__) -> Path:
    return Path(f).parents[0].absolute()
  resp = platform_copy(
    src = script_dir(__file__) / ".." / "test" / "dir1",
    dest = utils.PlatformPath(
      mac = script_dir(__file__) / ".." / "test" / "dir2",
      win = script_dir(__file__) / ".." / "test" / "dir2"
    ),
    copy_contents = False
  )
  assert(resp)
  
  
