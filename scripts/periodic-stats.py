"""
periodic-stats.py

Periodically write out all statistics
1st argument is the interval size in nanoseconds (default is 1e9 = 1 second of simulated time)
2rd argument, if present will limit the number of snapshots and dynamically remove itermediate data
"""

import sim

class PeriodicStats:
  def setup(self, args):
    args = dict(enumerate((args or '').split(':')))
    interval = int(args.get(0, '') or 1000000000)
    self.max_snapshots = int(args.get(1, 0))
    self.num_snapshots = 0
    self.interval = int(interval * sim.util.Time.NS)
    self.next_interval = float('inf')
    self.in_roi = False
    sim.util.Every(self.interval, self.periodic, roi_only = True)

  def hook_roi_begin(self):
    self.in_roi = True
    self.next_interval = sim.stats.time() + self.interval
    sim.stats.write('periodic-0')

  def hook_roi_end(self):
    self.next_interval = float('inf')
    self.in_roi = False

  def periodic(self, time, time_delta):
    if self.max_snapshots and self.num_snapshots > self.max_snapshots:
      # Floor division: this counter names the snapshots, as
      # 'periodic-<num_snapshots * interval>'. Python 3's `/` turned it into a
      # float, so after a halving the names landed on half- and then
      # quarter-multiples of the interval and the grid stopped being uniform --
      # which tools/viz assumes it is.
      self.num_snapshots //= 2
      for t in range(self.interval, time, self.interval * 2):
        sim.util.db_delete('periodic-%d' % t)
      self.interval *= 2

    if time >= self.next_interval:
      self.num_snapshots += 1
      sim.stats.write('periodic-%d' % (self.num_snapshots * self.interval))
      self.next_interval += self.interval

sim.util.register(PeriodicStats())
