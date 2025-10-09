#!/usr/bin/env python3

import click

def LOG_DEBUG(msg):
	click.secho(msg, dim=True)
	
def LOG_ERROR(msg, bold=True):
	click.secho(msg, bold=bold, fg="red")

def LOG_SUCCESS(msg, bold=True):
  click.secho(msg, bold=bold, fg="green")

def prompt_confirmation():
	resp = input("OK? [Y/N]: ")
	return resp.lower() in ["yes", "y", "a", "ok", "yup"]

def LOG_INFO(msg):
	click.secho(msg, bold=True)
