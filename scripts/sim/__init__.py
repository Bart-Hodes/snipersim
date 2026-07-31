import sim_config as config
import sim_stats as stats
import sim_hooks as hooks
import sim_dvfs as dvfs
import sim_control as control
import sim_bbv as bbv
import sim_mem as mem
import sim_thread as thread
import sim.util

import os, sqlite3
# check_same_thread=False: script hooks (sim.util.Every, periodic-stats.py, ...)
# are invoked from whichever simulator thread hit the hook, not the thread that
# imported this module. sqlite3 otherwise refuses the handle with
# "SQLite objects created in a thread can only be used in that same thread",
# which silently broke snapshot pruning in periodic-stats.py. Sniper's hooks do
# not run concurrently with each other, so serialization is not at risk here.
stats.db = sqlite3.connect(os.path.join(config.output_dir, 'sim.stats.sqlite3'),
                           check_same_thread = False)
