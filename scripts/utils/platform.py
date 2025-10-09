# is_platform.py

import os, platform
from pathlib import Path

def is_platform(plat):
  if plat.lower() == "macos":
    plat = "Darwin"
  return (platform.system().lower() == plat.lower())

def current_platform():
  _system = platform.system()
  match _system.lower():
    case "darwin":
      return "macOS"
    case _:
      return _system

class PlatformPath:
  def __init__(self, mac: Path = None, win: Path = None):
    self.mac_path = mac
    self.win_path = win
  def resolve(self) -> Path:
    match current_platform():
      case "macOS":
        return self.mac_path
      case "Windows":
        return self.win_path
      case _:
        LOG_ERROR("Could not resolve PlatformPath for this platform!")
        return None
  def __truediv__(self, other: str) -> "PlatformPath":
    self.mac_path = self.mac_path.joinpath(other).resolve()
    self.win_path = self.win_path.joinpath(other).resolve()
    return self

class PlatformFlag:
  def __init__(self, default=None):
    self.select_mac = default
    self.select_win = default
  def select(self) -> bool:
    if is_platform("Windows") and self.select_win:
      return True
    if is_platform("macOS") and self.select_mac:
      return True
    return False

class MacOnlyPlatformFlag(PlatformFlag):
  def __init__(self):
    super(PlatformFlag, self).__init__(default=False)
    self.select_mac = True

class WinOnlyPlatformFlag(PlatformFlag):
  def __init__(self):
    super(PlatformFlag, self).__init__(default=False)
    self.select_win = True

class AllPlatformFlag(PlatformFlag):
  def __init__(self):
    super(PlatformFlag, self).__init__(default=True)

