# Validation Report

## Objective
To validate the system's efficiency by comparing battery discharge curves in managed and unmanaged scenarios.

## Method
A simulation was run for a 5-hour horizon using the combined validation script with 500-point smoothing.
The generated output compared managed and unmanaged battery state-of-charge (SoC) trajectories from the simulation CSV.

## Findings
- The managed scenario started at 90.0% SoC and ended at 73.0% SoC.
- The unmanaged scenario also started at 90.0% SoC but ended at 30.5% SoC.
- Total discharge over the run was:
  - Managed: 17.0 percentage points
  - Unmanaged: 59.5 percentage points

## Conclusion
The managed scenario is clearly better for preserving battery life and system efficiency. It retained substantially more charge over the same period, showing that the management logic reduced unnecessary discharge and improved energy use compared with the unmanaged case.

## Usage
Use the combined validation script in `validate/combined.py` to generate simulation CSVs and comparison plots.

Examples:

- Run a single 24-hour simulation with a custom start time and three loads:

```bash
python3 validate/combined.py --hours 24 --samples 500 --cleanup --start-time 2026-07-12T00:00:00 --loads 1.0 1.5 2.0
```

- Run a validation suite for 1-hour, 5-hour, 9-hour, 13-hour, and 24-hour targets:

```bash
python3 validate/combined.py --suite --samples 500 --cleanup --start-time 2026-07-12T00:00:00 --loads 1.0 1.5 2.0
```

- Run a custom suite for specific target runtimes:

```bash
python3 validate/combined.py --suite --targets 24 5 13 9 --samples 500 --cleanup --start-time 2026-07-12T00:00:00 --loads 1.0 1.5 2.0
```

- Run a single simulation and save the output CSV to a custom path:

```bash
python3 validate/combined.py --output validate/bfs_run.csv --hours 24 --samples 500 --start-time 2026-07-12T00:00:00 --loads 1.0 1.5 2.0
```

- Plot existing telemetry CSVs directly:

```bash
python3 validate/combined.py validate/csv/bfs_run_24h_managed.csv validate/csv/bfs_run_24h_unmanaged.csv --output validate/png/bfs_run_24h_comparison.png --samples 500
```

- Launch the graphical dashboard to run commands without typing them manually:

```bash
python3 validate/dashboard.py
```
