# dispatcher.py

import os
import click
from pathlib import Path
from functools import update_wrapper, wraps
from .reglob import reglob_files
from .dirs import script_dir
from .platform import is_platform

def DEBUG_LOG(msg):
  DEBUG = False
  if DEBUG:
    print(msg)
  
class CommandDispatcher(click.MultiCommand):
  def __init__(self, help: str, commands_dir: Path):
    super(CommandDispatcher, self).__init__(help=help)
    if not commands_dir.exists():
      raise RuntimeError(f"Commands dir not found!")
    self.commands_dir: Path = commands_dir
  def list_commands(self, ctx):
    resp = set([])
    for filename in self.__candidate_command_files(prefix="cgc"):
      fname, fext = os.path.splitext(filename)
      fname_parts = fname.split("_")
      if len(fname_parts) <= 1:
        continue
      # cgc_init → ["cgc", "init"]
      command = fname_parts[1]
      resp.add(command)
    return sorted(resp)
  def get_command(self, ctx, name):
    resp = {}
    for filename in self.__candidate_command_files(prefix=f"cgc_{name}"): 
      with open(filename) as f:
        source = f.read()
        ast = compile(source, filename, 'exec')
        eval(ast, resp, resp)
    if name not in resp:
      return None
    return resp[name]
  def __candidate_command_files(self, prefix):
    pattern = f"^{prefix}[_|.][\w|.]*py$"
    acc = reglob_files(self.commands_dir, pattern)
    platform_commands_dir = self.commands_dir.joinpath("mac" if is_platform("macOS") else "win")
    if platform_commands_dir.exists():
      acc = acc + reglob_files(platform_commands_dir, pattern)
    return acc

def declare_group(parent=click, chain=False, description="??", enabled: bool = True):
  def __declare_group(f):
    @parent.group(
      name=f.__name__, 
      help=description, 
      no_args_is_help=False, 
      invoke_without_command=True, 
      chain=chain,
      hidden=(not enabled)
    )
    @click.pass_context
    @wraps(f)
    def wrapper(ctx, *args, **kwargs):
      return ctx.invoke(f, *args, ctx=ctx, **kwargs)
    return update_wrapper(wrapper, f)
  return __declare_group

def declare_command(parent, description="??", enabled: bool = True):
  def __declare_command(f):
    @parent.command(name=f.__name__, help=description, hidden=(not enabled))
    @click.pass_context
    @wraps(f)
    def wrapper(ctx, *args, **kwargs):
      return ctx.invoke(f, *args, ctx=ctx, **kwargs)
    return update_wrapper(wrapper, f)
  return __declare_command

def add_install_option(group=False, default=None):
  flag_identifier = "install?"
  force_identifier = "force?"
  def wrapper(f):
    @click.option('--install/--undo', help="Install or uninstall?", default=default)
    @click.option('--force', is_flag=True, help="Do not ask for confirmation while deleting files.", default=None)
    def __wrapper(ctx, *args, install, force, **kwargs):
      if ctx.obj == None:
        DEBUG_LOG(f"Resetting {f.__name__} ctx.obj[\"{flag_identifier}\"] to {default}")
        ctx.ensure_object(dict)
        ctx.obj[flag_identifier] = default
        ctx.obj[force_identifier] = False
      should_install = False
      should_force = False
      # install?
      if install != None:
        DEBUG_LOG(f"install parameter is : {install} for {f.__name__}")
        should_install = install
        DEBUG_LOG(f"should_install={should_install} for {f.__name__} [own]")
        if group:
          ctx.obj[flag_identifier] = install
      else:
        should_install = ctx.obj[flag_identifier]
        DEBUG_LOG(f"should_install={should_install} for {f.__name__} [inherited]")
      # force?
      if force != None:
        DEBUG_LOG(f"force parameter is : {force} for {f.__name__}")
        should_force = force
        DEBUG_LOG(f"should_force={should_force} for {f.__name__} [own]")
        if group:
          ctx.obj[force_identifier] = force
      else:
        should_force = ctx.obj[force_identifier]
        DEBUG_LOG(f"should_force={should_force} for {f.__name__} [inherited]")
      return ctx.invoke(f, *args, ctx=ctx, install=should_install, force=should_force, **kwargs)
    return update_wrapper(__wrapper, f)
  return wrapper

def add_undo_option(group=False, default=None):
  flag_identifier = "undo?"
  def wrapper(f):
    @click.option('--undo', is_flag=True, help="Stop running targets.", default=default)
    def __wrapper(ctx, *args, undo, **kwargs):
      if ctx.obj == None:
        DEBUG_LOG(f"Resetting {f.__name__} ctx.obj[\"{flag_identifier}\"] to {default}")
        ctx.ensure_object(dict)
        ctx.obj[flag_identifier] = default
      should_undo = False
      # undo?
      if undo != None:
        DEBUG_LOG(f"undo parameter is : {undo} for {f.__name__}")
        should_undo = undo
        DEBUG_LOG(f"should_undo={should_undo} for {f.__name__} [own]")
        if group:
          ctx.obj[flag_identifier] = undo
      else:
        should_undo = ctx.obj[flag_identifier]
        DEBUG_LOG(f"should_undo={should_undo} for {f.__name__} [inherited]")
      return ctx.invoke(f, *args, ctx=ctx, undo=should_undo, **kwargs)
    return update_wrapper(__wrapper, f)
  return wrapper
