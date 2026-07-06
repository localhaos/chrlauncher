Addons cwd loader test.

Requested behavior: no temp copy; use Addons as process working directory before normal launcher startup.

Implementation target: wWinMain should set cwd to Addons when Addons/config.ini exists next to the executable.
