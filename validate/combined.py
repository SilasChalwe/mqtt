# Auto-generated combined file from validate/*.py

import argparse
import csv
from collections import defaultdict
from datetime import datetime
import math
from datetime import datetime, timedelta
import os
import subprocess
import sys
from heapq import heappush, heappop


def parse_start_time(value=None):
    if value is None:
        return datetime.now().replace(second=0, microsecond=0)
    if isinstance(value, datetime):
        return value.replace(second=0, microsecond=0)
    if value.endswith('Z'):
        value = value[:-1] + '+00:00'
    return datetime.fromisoformat(value).replace(second=0, microsecond=0)

"""
Python port of PowerEstimator.cpp for use by the simulator.

Usage: create a SolarManager-like object with `getCurrent()` and pass to PowerEstimator.
"""
class SolarManagerStub:

    def __init__(self):
        self._current = 0.0

    def setCurrent(self, a):
        self._current = a

    def getCurrent(self):
        return self._current
INVERTER_MAX_AMPS = 15.0
class PowerEstimator:

    def __init__(self, solar_instance, threshold=0.1, filter_alpha=0.2):
        self.solar = solar_instance
        self.safetyThreshold = threshold
        self._filterAlpha = filter_alpha
        self._filteredSoC = 0.0

    def begin(self):
        self._filteredSoC = 0.0

    def set_filtered_soc(self, soc_pct):
        currentSoC = soc_pct / 100.0
        self._filteredSoC = currentSoC * self._filterAlpha + self._filteredSoC * (1.0 - self._filterAlpha)

    def get_CAvailable(self, batteryAhTotal, targetRuntimeHours, forcedLoadsCurrent):
        if targetRuntimeHours <= 0:
            targetRuntimeHours = 1.0
        usableSoC = self._filteredSoC - self.safetyThreshold
        if usableSoC < 0.0:
            usableSoC = 0.0
        battery_Ah_buffer = batteryAhTotal * usableSoC
        capacityRate = (battery_Ah_buffer + self.solar.getCurrent()) / targetRuntimeHours
        budget = capacityRate if capacityRate < INVERTER_MAX_AMPS else INVERTER_MAX_AMPS
        result = budget - forcedLoadsCurrent
        return result if result > 0.0 else 0.0

    def get_EstimatedRuntime(self, batteryAh, totalLoadCurrent):
        net = totalLoadCurrent - self.solar.getCurrent()
        if net <= 0.0:
            return 99.9
        usableAh = batteryAh * (self._filteredSoC - self.safetyThreshold)
        if usableAh <= 0.0:
            return 0.0
        return usableAh / net
"""Plot battery discharge curves from one or more telemetry CSV runs."""
try:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    import matplotlib.dates as mdates
except ImportError:
    print("Missing dependency: matplotlib. Install with 'pip install matplotlib'.")
    raise
DATE_FORMATS = ['%Y-%m-%dT%H:%M:%S.%fZ', '%Y-%m-%dT%H:%M:%SZ']
def parse_datetime(value):
    if not value:
        return None
    for fmt in DATE_FORMATS:
        try:
            return datetime.strptime(value, fmt)
        except ValueError:
            continue
    raise ValueError(f'Unrecognized datetime format: {value}')
def load_battery_rows(path):
    rows = []
    with open(path, newline='', encoding='utf-8') as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            if row.get('topic') != 'esp32/telemetry/battery':
                continue
            time_str = row.get('payload_time') or row.get('recv_time')
            if not time_str:
                continue
            try:
                timestamp = parse_datetime(time_str)
            except ValueError:
                timestamp = parse_datetime(row.get('recv_time', ''))
            try:
                voltage = float(row.get('battery_voltage', ''))
            except ValueError:
                voltage = None
            try:
                soc = float(row.get('battery_soc', ''))
            except ValueError:
                soc = None
            try:
                solar_current = float(row.get('solar_current', ''))
            except ValueError:
                solar_current = None
            try:
                solar_power = float(row.get('solar_power', ''))
            except ValueError:
                solar_power = None
            rows.append({
                'time': timestamp,
                'voltage': voltage,
                'soc': soc,
                'solar_current': solar_current,
                'solar_power': solar_power,
            })
    rows.sort(key=lambda x: x['time'])
    return rows
def _interpolate_series(times, values, target_count):
    # times: list of float seconds, values: list of floats
    if not times or not values or target_count is None or target_count <= 0:
        return [], []
    if len(times) == 1:
        return [times[0]] * target_count, [values[0]] * target_count
    t0, tn = times[0], times[-1]
    step = (tn - t0) / (target_count - 1)
    new_times = [t0 + i * step for i in range(target_count)]
    new_values = []
    j = 0
    for nt in new_times:
        while j + 1 < len(times) and times[j + 1] < nt:
            j += 1
        if j + 1 == len(times):
            new_values.append(values[-1])
        else:
            t_a, v_a = times[j], values[j]
            t_b, v_b = times[j + 1], values[j + 1]
            if t_b == t_a:
                new_values.append(v_a)
            else:
                frac = (nt - t_a) / (t_b - t_a)
                new_values.append(v_a + frac * (v_b - v_a))
    return new_times, new_values


def plot_runs(run_data, output_path=None, samples=None):
    fig, axes = plt.subplots(3, 1, figsize=(12, 10), sharex=True)
    axes[0].set_title('Battery Discharge Comparison')
    axes[0].set_ylabel('Battery Voltage (V)')
    axes[1].set_ylabel('Battery SoC (%)')
    axes[2].set_ylabel('Solar Current (A)')
    axes[2].set_xlabel('Time')
    for label, rows in run_data.items():
        if not rows:
            continue
        full_times = [r['time'] for r in rows if r['time'] is not None]
        volt_times = [r['time'] for r in rows if r['voltage'] is not None]
        volt_values = [r['voltage'] for r in rows if r['voltage'] is not None]
        soc_times = [r['time'] for r in rows if r['soc'] is not None]
        soc_values = [r['soc'] for r in rows if r['soc'] is not None]
        solar_times = [r['time'] for r in rows if r['solar_current'] is not None]
        solar_values = [r['solar_current'] for r in rows if r['solar_current'] is not None]
        solar_power_times = [r['time'] for r in rows if r['solar_power'] is not None]
        solar_power_values = [r['solar_power'] for r in rows if r['solar_power'] is not None]
        if volt_times and volt_values:
            if samples:
                secs = [(t - volt_times[0]).total_seconds() for t in volt_times]
                new_secs, new_volt = _interpolate_series(secs, volt_values, samples)
                new_times = [volt_times[0] + timedelta(seconds=s) for s in new_secs]
                axes[0].plot(new_times, new_volt, '-', linewidth=2.2, label=f'{label} voltage')
            else:
                axes[0].plot(volt_times, volt_values, marker='o', label=f'{label} voltage')
        if soc_times and soc_values:
            if samples:
                secs = [(t - soc_times[0]).total_seconds() for t in soc_times]
                new_secs, new_soc = _interpolate_series(secs, soc_values, samples)
                new_times = [soc_times[0] + timedelta(seconds=s) for s in new_secs]
                axes[1].plot(new_times, new_soc, '-', linewidth=2.2, label=f'{label} SoC')
            else:
                axes[1].plot(soc_times, soc_values, marker='o', label=f'{label} SoC')
        if solar_times and solar_values:
            if samples:
                secs = [(t - solar_times[0]).total_seconds() for t in solar_times]
                new_secs, new_solar = _interpolate_series(secs, solar_values, samples)
                new_times = [solar_times[0] + timedelta(seconds=s) for s in new_secs]
                axes[2].plot(new_times, new_solar, '-', linewidth=2.2, label=f'{label} solar current')
            else:
                axes[2].plot(solar_times, solar_values, marker='o', label=f'{label} solar current')
        if solar_power_times and solar_power_values:
            if samples:
                secs = [(t - solar_power_times[0]).total_seconds() for t in solar_power_times]
                new_secs, new_power = _interpolate_series(secs, solar_power_values, samples)
                new_times = [solar_power_times[0] + timedelta(seconds=s) for s in new_secs]
                axes[2].plot(new_times, new_power, '--', linewidth=1.8, label=f'{label} solar power')
            else:
                axes[2].plot(solar_power_times, solar_power_values, '--', marker='x', label=f'{label} solar power')
    axes[0].legend(loc='best')
    axes[1].legend(loc='best')
    axes[2].legend(loc='best')
    axes[0].grid(True)
    axes[1].grid(True)
    axes[2].grid(True)

    for ax in axes:
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
        ax.xaxis.set_major_locator(mdates.HourLocator(interval=1))
        ax.figure.autofmt_xdate()

    summary_lines = []
    for label, rows in run_data.items():
        soc_values = [r['soc'] for r in rows if r['soc'] is not None]
        if not soc_values:
            continue
        start_soc = soc_values[0]
        end_soc = soc_values[-1]
        drop_soc = start_soc - end_soc
        summary_lines.append(f'{label}: start {start_soc:.1f}%, end {end_soc:.1f}%, drop {drop_soc:.1f}%')

    if summary_lines:
        fig.text(
            0.02,
            0.01,
            '\n'.join(summary_lines),
            fontsize=9,
            ha='left',
            va='bottom',
            bbox=dict(boxstyle='round', facecolor='white', alpha=0.8),
        )

    fig.autofmt_xdate()
    if output_path:
        plt.savefig(output_path, bbox_inches='tight')
        print(f'Saved plot to {output_path}')
    else:
        plt.show()
def main():
    parser = argparse.ArgumentParser(description='Compare battery discharge curves from telemetry CSV files.')
    parser.add_argument('csv_files', nargs='+', help='Telemetry CSV files produced by telemetry_logger.py')
    parser.add_argument('--labels', nargs='*', help='Optional labels for each run. Defaults to file names.')
    parser.add_argument('--output', help='Optional PNG file to save the plot')
    parser.add_argument('--samples', type=int, default=None, help='Resample points for smoothing (e.g., 500)')
    args = parser.parse_args()
    if args.labels and len(args.labels) != len(args.csv_files):
        parser.error('--labels must be provided once per CSV file')
    run_data = {}
    for index, csv_file in enumerate(args.csv_files):
        label = args.labels[index] if args.labels else csv_file
        run_data[label] = load_battery_rows(csv_file)
        print(f'Loaded {len(run_data[label])} battery rows from {csv_file}')
    plot_runs(run_data, output_path=args.output, samples=args.samples)
def main_from_plot_discharge_curves():
    main()

"""
Convert of BestFirstSearch.cpp to Python for simulation use.

Implements Node and BestFirstSearch.execute/scout logic closely matching the C++ code,
then runs a time-series simulation producing a CSV with managed/unmanaged rows.
"""
FIELDNAMES = ['recv_time', 'scenario', 'notes', 'topic', 'payload_time', 'battery_voltage', 'battery_soc', 'battery_capacity_ah', 'solar_voltage', 'solar_current', 'solar_power', 'solar_energy_wh', 'estimator_available_current']
class Node:

    def __init__(self):
        self.id = 0
        self.name = ''
        self.currentDraw = 0.0
        self.voltage = 12.0
        self.power = 0.0
        self.energyWh = 0.0
        self.priority = 0
        self.relayPin = -1
        self.isForced = False
        self.wireFriction = 0.0
        self.isActive = False
        self.children = []
        self.lastActiveUpdateMs = 0
class BestFirstSearch:

    def __init__(self):
        self.root = Node()
        self.root.name = 'Main_DB'
        self.root.isForced = True

    def create_node(self, parent_name, name, amps, priority, pin, friction, forced, voltage):
        parent = self.root if parent_name in (None, '', 'Main_DB') else self.find_by_name(parent_name)
        if parent is None:
            return None
        n = Node()
        n.name = name
        n.currentDraw = amps
        n.voltage = voltage
        n.power = amps * voltage
        n.energyWh = 0.0
        n.lastActiveUpdateMs = 0
        n.priority = priority
        n.relayPin = pin
        n.wireFriction = friction
        n.isForced = forced
        n.isActive = False
        parent.children.append(n)
        return n

    def find_by_name(self, name):

        def dfs(node):
            if node.name == name:
                return node
            for c in node.children:
                r = dfs(c)
                if r:
                    return r
            return None
        return dfs(self.root)

    def scout(self, availableCurrent):
        candidates = []
        pq = []
        counter = 0
        heappush(pq, (-(self.root.priority - self.root.wireFriction), counter, self.root))
        counter += 1
        while pq:
            _, _, cur = heappop(pq)
            if cur.relayPin != -1:
                candidates.append(cur)
            for child in cur.children:
                heappush(pq, (-(child.priority - child.wireFriction), counter, child))
                counter += 1
        return candidates

    def execute(self, candidates, C_available, P_available):
        N = len(candidates)
        if N == 0:
            return
        safeCurrent = C_available if C_available > 0.0 else 0.0
        safePower = P_available if P_available > 0.0 else 0.0
        POWER_SCALE = 10
        W = int(safePower / POWER_SCALE)
        if W < 0:
            W = 0
        forcedCurrent = 0.0
        forcedPower = 0.0
        for n in candidates:
            n.power = n.currentDraw * n.voltage
            if n.isForced:
                forcedCurrent += n.currentDraw
                forcedPower += n.power
        remainingCurrent = safeCurrent - forcedCurrent
        remainingPower = safePower - forcedPower
        remainingCurrent = max(0.0, remainingCurrent)
        remainingPower = max(0.0, remainingPower)
        remainingW = int(remainingPower / POWER_SCALE)
        remainingW = max(0, remainingW)
        selected = [False] * N
        for i in range(N):
            if candidates[i].isForced:
                selected[i] = True
        nonForcedIndices = [i for i in range(N) if not candidates[i].isForced]
        M = len(nonForcedIndices)
        K = [[0] * (remainingW + 1) for _ in range(M + 1)]
        for i in range(1, M + 1):
            node = candidates[nonForcedIndices[i - 1]]
            P_i = int(node.power / POWER_SCALE)
            if P_i < 0:
                P_i = 0
            U_i = node.priority
            for w in range(0, remainingW + 1):
                if P_i <= w and node.currentDraw <= remainingCurrent and (node.power <= remainingPower):
                    K[i][w] = max(U_i + K[i - 1][w - P_i], K[i - 1][w])
                else:
                    K[i][w] = K[i - 1][w]
        res = K[M][remainingW]
        w_limit = remainingW
        for i in range(M, 0, -1):
            node = candidates[nonForcedIndices[i - 1]]
            P_i = int(node.power / POWER_SCALE)
            if P_i < 0:
                P_i = 0
            if res == K[i - 1][w_limit]:
                selected[nonForcedIndices[i - 1]] = False
            else:
                selected[nonForcedIndices[i - 1]] = True
                res -= node.priority
                w_limit -= P_i
        totalCurrent = forcedCurrent
        for i in range(N):
            if selected[i] and (not candidates[i].isForced):
                totalCurrent += candidates[i].currentDraw
        if totalCurrent > safeCurrent:
            order = [i for i in range(N) if selected[i] and (not candidates[i].isForced)]
            order.sort(key=lambda a: candidates[a].priority / max(0.1, candidates[a].currentDraw))
            for idx in order:
                if totalCurrent <= safeCurrent:
                    break
                selected[idx] = False
                totalCurrent -= candidates[idx].currentDraw
        for i in range(N):
            n = candidates[i]
            n.isActive = selected[i]
def run_sim(output_path, hours=4, interval_s=60, capacity_ah=100.0, initial_soc=90.0, solar_peak=2.0, battery_discharge_limit=5.0, target_runtime_hours=1.0, start_time=None, loads=None):
    bfs = BestFirstSearch()
    if loads is None:
        loads = [1.0, 1.5, 2.0]
    # Build a simple dynamic tree with one parent group and one child per load
    group_name = 'dynamic_group'
    bfs.create_node('Main_DB', group_name, 0.0, 0, -1, 0.0, False, 0.0)
    for idx, load in enumerate(loads):
        load_name = f'load_{idx + 1}'
        forced = idx == 0
        bfs.create_node(group_name, load_name, float(load), 10 + idx, 100 + idx, 0.0, forced, 12.0)
    # Use local SolarManagerStub and PowerEstimator defined above
    solar = SolarManagerStub()
    estimator = PowerEstimator(solar, threshold=0.05, filter_alpha=0.2)
    estimator.set_filtered_soc(initial_soc)

    def load_battery_profile(path):
        pts = []
        if not path or not os.path.isfile(path):
            return lambda soc: 3.4 + soc / 100.0 * (4.2 - 3.4)
        with open(path, newline='') as fp:
            r = csv.reader(fp)
            for row in r:
                if not row or row[0].strip().startswith('#'):
                    continue
                pts.append((float(row[0]), float(row[1])))
        pts.sort()

        def f(soc):
            if soc <= pts[0][0]:
                return pts[0][1]
            if soc >= pts[-1][0]:
                return pts[-1][1]
            for i in range(len(pts) - 1):
                a, va = pts[i]
                b, vb = pts[i + 1]
                if a <= soc <= b:
                    t = (soc - a) / (b - a)
                    return va + t * (vb - va)
            return pts[-1][1]
        return f

    def load_solar_profile(path):
        pts = []
        if not path or not os.path.isfile(path):
            return None
        with open(path, newline='') as fp:
            r = csv.reader(fp)
            for row in r:
                if not row or row[0].strip().startswith('#'):
                    continue
                pts.append((float(row[0]), float(row[1])))
        pts.sort()

        def f(frac):
            if frac <= pts[0][0]:
                return pts[0][1]
            if frac >= pts[-1][0]:
                return pts[-1][1]
            for i in range(len(pts) - 1):
                a, va = pts[i]
                b, vb = pts[i + 1]
                if a <= frac <= b:
                    t = (frac - a) / (b - a)
                    return va + t * (vb - va)
            return pts[-1][1]
        return f
    battery_profile_path = os.path.join(os.path.dirname(__file__), 'battery_profile.csv')
    solar_profile_path = os.path.join(os.path.dirname(__file__), 'solar_profile.csv')
    battery_profile_fn = load_battery_profile(battery_profile_path)
    solar_profile_fn = load_solar_profile(solar_profile_path)
    rows = []
    start_time = parse_start_time(start_time)
    total_steps = int(hours * 3600 / interval_s)
    initial_ah = initial_soc / 100.0 * capacity_ah
    cumulative_solar_wh = 0.0
    for step in range(total_steps + 1):
        t = start_time + timedelta(seconds=step * interval_s)
        phase = step / max(1, total_steps) * math.pi
        frac = step / max(1, total_steps)
        if solar_profile_fn:
            solar_current = max(0.0, solar_profile_fn(frac))
        else:
            solar_current = max(0.0, solar_peak * math.sin(phase))
        solar_voltage = 24.0
        solar_power = solar_voltage * solar_current
        cumulative_solar_wh += solar_power * (interval_s / 3600.0)
        solar.setCurrent(solar_current)
        unmanaged_load_current = sum((n.currentDraw for n in bfs.scout(battery_discharge_limit + solar_current)))
        net_unmanaged = max(0.0, unmanaged_load_current - solar_current)
        current_ah_un = max(0.0, initial_ah - net_unmanaged * (step * interval_s / 3600.0))
        soc_un = current_ah_un / capacity_ah * 100.0
        estimator.set_filtered_soc(soc_un)
        candidates_temp = bfs.scout(0.0)
        forcedLoadsCurrent = sum((n.currentDraw for n in candidates_temp if n.isForced))
        C_available = estimator.get_CAvailable(capacity_ah, target_runtime_hours, forcedLoadsCurrent)
        P_available = C_available * 12.0
        candidates = bfs.scout(C_available)
        bfs.execute(candidates, C_available, P_available)
        managed_load_current = sum((n.currentDraw for n in candidates if n.isActive))
        unmanaged_load_current = sum((n.currentDraw for n in bfs.scout(battery_discharge_limit + solar_current)))
        net_managed = max(0.0, managed_load_current - solar_current)
        net_unmanaged = max(0.0, unmanaged_load_current - solar_current)
        current_ah_man = max(0.0, initial_ah - net_managed * (step * interval_s / 3600.0))
        soc_man = current_ah_man / capacity_ah * 100.0
        volt_man = battery_profile_fn(soc_man) if battery_profile_fn else 3.4 + soc_man / 100.0 * (4.2 - 3.4)
        current_ah_un = max(0.0, initial_ah - net_unmanaged * (step * interval_s / 3600.0))
        soc_un = current_ah_un / capacity_ah * 100.0
        volt_un = battery_profile_fn(soc_un) if battery_profile_fn else 3.4 + soc_un / 100.0 * (4.2 - 3.4)
        row_un = {'recv_time': t.isoformat() + 'Z', 'scenario': 'unmanaged', 'notes': 'bfs_sim', 'topic': 'esp32/telemetry/battery', 'payload_time': t.isoformat() + 'Z', 'battery_voltage': f'{volt_un:.3f}', 'battery_soc': f'{soc_un:.3f}', 'battery_capacity_ah': f'{capacity_ah}', 'solar_voltage': f'{solar_voltage:.2f}', 'solar_current': f'{solar_current:.3f}', 'solar_power': f'{solar_power:.3f}', 'solar_energy_wh': f'{cumulative_solar_wh:.3f}', 'estimator_available_current': f'{C_available:.3f}'}
        rows.append(row_un)
        row_man = dict(row_un)
        row_man['scenario'] = 'managed'
        row_man['battery_voltage'] = f'{volt_man:.3f}'
        row_man['battery_soc'] = f'{soc_man:.3f}'
        rows.append(row_man)
    with open(output_path, 'w', newline='', encoding='utf-8') as fp:
        w = csv.DictWriter(fp, fieldnames=FIELDNAMES)
        w.writeheader()
        for r in rows:
            w.writerow(r)
    print(f'Wrote {len(rows)} rows to {output_path}')
def main_from_bfs_simulator():
    parser = argparse.ArgumentParser(description='BFS simulator and validation runner')
    parser.add_argument('--output', '-o', default='validate/bfs_run.csv', help='Output CSV for single run')
    parser.add_argument('--hours', type=float, default=2.0, help='Simulation duration (hours)')
    parser.add_argument('--interval', type=int, default=60, help='Sample interval (seconds)')
    parser.add_argument('--capacity', type=float, default=100.0, help='Battery capacity (Ah)')
    parser.add_argument('--initial-soc', type=float, default=90.0, help='Initial SoC (percent)')
    parser.add_argument('--solar-peak', type=float, default=1.0, help='Peak solar current (A)')
    parser.add_argument('--discharge-limit', type=float, default=5.0, help='Battery discharge limit (A)')
    parser.add_argument('--target-runtime-hours', type=float, default=1.0, help='Estimator target runtime (hours)')
    parser.add_argument('--suite', action='store_true', help='Run validation suite for targets [24,5,13,9]')
    parser.add_argument('--targets', nargs='+', type=int, help='Custom list of target runtimes (hours) for suite')
    parser.add_argument('--samples', type=int, default=None, help='Resample points for smoothing plots (e.g., 500)')
    parser.add_argument('--cleanup', action='store_true', help='Delete intermediate managed/unmanaged CSVs after plotting')
    parser.add_argument('--start-time', default=None, help='Optional ISO8601 start time for the simulation (defaults to the current system time, e.g. 2026-07-12T10:00:00)')
    parser.add_argument('--loads', nargs='+', type=float, default=None, help='Dynamic list of load currents (A) to include in the simulation')
    args = parser.parse_args()
    if args.suite:
        if args.targets:
            targets = args.targets
        elif '--hours' in sys.argv:
            targets = [args.hours]
        else:
            targets = [24, 5, 13, 9]
        for t in targets:
            target_label = str(int(t)) if float(t).is_integer() else str(t)
            out = f'validate/csv/bfs_run_{target_label}h.csv'
            print(f'Running simulation for target {target_label}h -> {out}')
            run_sim(out, hours=args.hours, interval_s=args.interval, capacity_ah=args.capacity, initial_soc=args.initial_soc, solar_peak=args.solar_peak, battery_discharge_limit=args.discharge_limit, target_runtime_hours=t, start_time=args.start_time, loads=args.loads)
            managed = f'validate/csv/bfs_run_{target_label}h_managed.csv'
            unmanaged = f'validate/csv/bfs_run_{target_label}h_unmanaged.csv'
            with open(out, newline='', encoding='utf-8') as fp_in, open(managed, 'w', newline='', encoding='utf-8') as fp_man, open(unmanaged, 'w', newline='', encoding='utf-8') as fp_un:
                r = csv.DictReader(fp_in)
                man_writer = csv.DictWriter(fp_man, fieldnames=r.fieldnames)
                un_writer = csv.DictWriter(fp_un, fieldnames=r.fieldnames)
                man_writer.writeheader()
                un_writer.writeheader()
                for row in r:
                    if row.get('scenario') == 'managed':
                        man_writer.writerow(row)
                    else:
                        un_writer.writerow(row)
            png = f'validate/png/bfs_run_{target_label}h_comparison.png'
            # Plot using the local functions instead of calling an external script
            man_rows = load_battery_rows(managed)
            un_rows = load_battery_rows(unmanaged)
            run_data = {'managed': man_rows, 'unmanaged': un_rows}
            try:
                plot_runs(run_data, output_path=png, samples=args.samples)
                print('Wrote plot', png)
            except Exception as e:
                print('Failed to write plot', png, ':', e)
            if args.cleanup:
                try:
                    os.remove(managed)
                    os.remove(unmanaged)
                    print('Deleted intermediate files', managed, unmanaged)
                except Exception as ex:
                    print('Failed to delete intermediate files:', ex)
        print('Suite complete')
    else:
        run_sim(args.output, hours=args.hours, interval_s=args.interval, capacity_ah=args.capacity, initial_soc=args.initial_soc, solar_peak=args.solar_peak, battery_discharge_limit=args.discharge_limit, target_runtime_hours=args.target_runtime_hours, start_time=args.start_time, loads=args.loads)


def _run_extracted_mains():
    import sys
    args = sys.argv[1:]
    # If CSV filenames are provided on the command line, treat this as a plot request
    is_plot = any(a.endswith('.csv') for a in args)
    # If BFS simulator flags are present (or --suite), treat as simulator request
    bfs_flags = ('--suite', '--hours', '--interval', '--capacity', '--initial-soc', '--solar-peak', '--discharge-limit', '--targets', '--output', '-o')
    is_bfs = any(f in args for f in bfs_flags)
    if is_plot and not is_bfs:
        try:
            main_from_plot_discharge_curves()
        except Exception as e:
            print("Error running main_from_plot_discharge_curves:", e)
    else:
        try:
            main_from_bfs_simulator()
        except Exception as e:
            print("Error running main_from_bfs_simulator:", e)

if __name__ == '__main__':
    _run_extracted_mains()