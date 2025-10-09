# reglob.py

import os, re
from pathlib import Path
from typing import List

def reglob_files(path: Path, pattern: str, sort: bool = True) -> List[Path]:
  acc: Path = []
  for root, _, files in os.walk(path):
    for f in files:
      if re.match(pattern, f):
        acc.append(Path(root) / f)
    break # We only want to walk one level down
  return sorted(acc) if sort else acc

if __name__ == "__main__":
  print(f"Re-Globbing at {Path.cwd()}")
  name = "init"
  resp = reglob_files(Path("/Volumes/Backup/CinemaGrade/Task2/Github/PluginBridge/scripts/commands"), f"^{name}[_|.][\w|.]*py$")
  print(resp)
  
