# stopwatch.py

import os, time

class Stopwatch:
  def __init__(self):
    self.start = time.time()
  def elapsed(self, seconds):
    return time.time() > self.start + seconds
  
