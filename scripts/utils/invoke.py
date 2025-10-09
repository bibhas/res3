#!/usr/bin/env python3

import os, subprocess, shlex, platform
from typing import List
import click
from .log import LOG_DEBUG

class InvocationResult:
  def __init__(self):
    self.out: str = None
    self.err: str = None
    self.returncode: int = -1
  def __str__(self):
    return f"Out: {self.out}\nErr: {self.err}ReturnCode: {self.returncode}"

def invoke(cmd: List[str], pipe=False, shell=True) -> InvocationResult:
  _cmdstr = shlex.join(cmd) if shell else " ".join(cmd)
  if pipe:
    LOG_DEBUG(f"Invoking {_cmdstr} [piped]")
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=shell)
    out, err = proc.communicate()
    resp = InvocationResult()
    resp.out = out.decode("utf-8")
    resp.err = err.decode("utf-8")
    resp.returncode = proc.returncode
    click.secho(f"[invoke.py] code = {resp.returncode}", dim=True)
    return resp
  else:
    LOG_DEBUG(f"Invoking {_cmdstr} [system]")
    exitstatus: int = os.system(_cmdstr)
    resp = InvocationResult()
    resp.returncode = exitstatus
    click.secho(f"[invoke.py] code = {resp.returncode}", dim=True)
    return resp
  


