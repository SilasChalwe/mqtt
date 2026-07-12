import json
import os
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler
from pathlib import Path
from socketserver import ThreadingTCPServer

from combined import load_battery_rows, plot_runs, run_sim

VALIDATE_DIR = Path(__file__).resolve().parent
CSV_DIR = VALIDATE_DIR / 'csv'
PNG_DIR = VALIDATE_DIR / 'png'
HTML_FILE = VALIDATE_DIR / 'dashboard.html'


def safe_path(relative_path: str) -> Path:
    candidate = (VALIDATE_DIR / relative_path).resolve()
    if VALIDATE_DIR not in candidate.parents and candidate != VALIDATE_DIR:
        raise ValueError('Invalid path')
    return candidate


class DashboardRequestHandler(SimpleHTTPRequestHandler):
    def translate_path(self, path):
        if path.startswith('/'):
            path = path[1:]
        if path in ('', 'index.html'):
            return str(HTML_FILE)
        return str(VALIDATE_DIR / path)

    def do_POST(self):
        if self.path == '/api/run-single':
            self.handle_run_single()
        elif self.path == '/api/run-suite':
            self.handle_run_suite()
        else:
            self.send_error(HTTPStatus.NOT_FOUND, 'Unknown endpoint')

    def handle_run_single(self):
        try:
            payload = self.read_json()
            output_csv = payload.get('output_csv', 'csv/bfs_run.csv')
            output_csv_path = safe_path(output_csv)
            samples = int(payload.get('samples', 500))
            start_time = payload.get('start_time')
            end_time = payload.get('end_time', start_time)
            cleanup = bool(payload.get('cleanup', True))
            
            # Calculate hours from time range
            from datetime import datetime
            def parse_iso_time(value):
                if value is None:
                    raise ValueError('Missing start_time or end_time')
                if not isinstance(value, str):
                    raise ValueError('Invalid datetime format')
                normalized = value.strip()
                # Accept ISO strings with Z, with explicit offset, or both.
                if normalized.endswith('Z'):
                    normalized = normalized[:-1]
                if normalized.endswith('+00:00'):
                    normalized = normalized[:-6] + '+00:00'
                try:
                    return datetime.fromisoformat(normalized)
                except ValueError:
                    raise ValueError(f'Unrecognized datetime format: {value}')

            start_dt = parse_iso_time(start_time)
            end_dt = parse_iso_time(end_time)
            hours = (end_dt - start_dt).total_seconds() / 3600.0
            if hours <= 0:
                hours = 24.0
            
            # Target runtime is 80% of simulation time for optimization
            target_runtime_hours = hours * 0.8
            
            loads = self.parse_loads(payload.get('loads', '1.0 1.5 2.0'))

            CSV_DIR.mkdir(exist_ok=True)
            PNG_DIR.mkdir(exist_ok=True)
            
            base_name = output_csv_path.stem
            managed_csv = CSV_DIR / f'{base_name}_managed.csv'
            unmanaged_csv = CSV_DIR / f'{base_name}_unmanaged.csv'
            
            # Skip simulation if CSV files already exist
            if not managed_csv.exists() or not unmanaged_csv.exists():
                run_sim(str(output_csv_path), hours=hours, interval_s=60, capacity_ah=100.0,
                        initial_soc=90.0, solar_peak=1.0, battery_discharge_limit=5.0,
                        target_runtime_hours=target_runtime_hours, start_time=start_time, loads=loads)
                self.split_csv(output_csv_path, managed_csv, unmanaged_csv)
            png_file = PNG_DIR / f'{base_name}_comparison.png'
            run_data = {
                'managed': load_battery_rows(str(managed_csv)),
                'unmanaged': load_battery_rows(str(unmanaged_csv)),
            }
            plot_runs(run_data, output_path=str(png_file), samples=samples)
            chart_data = {
                'managed': self.build_chart_payload(run_data['managed']),
                'unmanaged': self.build_chart_payload(run_data['unmanaged']),
            }
            if cleanup:
                managed_csv.unlink(missing_ok=True)
                unmanaged_csv.unlink(missing_ok=True)

            self.send_json({'success': True, 'message': '', 'plot': str(png_file.name), 'chart_data': chart_data})
        except Exception as exc:
            self.send_error_response(str(exc))

    def handle_run_suite(self):
        try:
            payload = self.read_json()
            hours = float(payload.get('hours', 24))
            samples = int(payload.get('samples', 500))
            start_time = payload.get('start_time')
            loads = self.parse_loads(payload.get('loads', '1.0 1.5 2.0'))
            targets = self.parse_targets(payload.get('targets', '24 5 13 9'))
            cleanup = bool(payload.get('cleanup', True))

            CSV_DIR.mkdir(exist_ok=True)
            PNG_DIR.mkdir(exist_ok=True)
            outputs = []
            for t in targets:
                output_csv_path = CSV_DIR / f'bfs_run_{t}h.csv'
                managed_csv = CSV_DIR / f'bfs_run_{t}h_managed.csv'
                unmanaged_csv = CSV_DIR / f'bfs_run_{t}h_unmanaged.csv'
                
                # Skip simulation if CSV files already exist
                if not managed_csv.exists() or not unmanaged_csv.exists():
                    run_sim(str(output_csv_path), hours=hours, interval_s=60, capacity_ah=100.0,
                            initial_soc=90.0, solar_peak=1.0, battery_discharge_limit=5.0,
                            target_runtime_hours=t, start_time=start_time, loads=loads)
                    self.split_csv(output_csv_path, managed_csv, unmanaged_csv)
                
                png_file = PNG_DIR / f'bfs_run_{t}h_comparison.png'
                run_data = {
                    'managed': load_battery_rows(str(managed_csv)),
                    'unmanaged': load_battery_rows(str(unmanaged_csv)),
                }
                plot_runs(run_data, output_path=str(png_file), samples=samples)
                if cleanup:
                    managed_csv.unlink(missing_ok=True)
                    unmanaged_csv.unlink(missing_ok=True)
                outputs.append({'target': t, 'plot': str(png_file.name)})

            self.send_json({'success': True, 'message': '', 'outputs': outputs})
        except Exception as exc:
            self.send_error_response(str(exc))

    def read_json(self):
        length = int(self.headers.get('Content-Length', 0))
        raw = self.rfile.read(length).decode('utf-8')
        return json.loads(raw)

    def send_json(self, data, status=HTTPStatus.OK):
        encoded = json.dumps(data).encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(encoded)))
        self.end_headers()
        self.wfile.write(encoded)

    def send_error_response(self, message):
        self.send_json({'success': False, 'error': message}, status=HTTPStatus.INTERNAL_SERVER_ERROR)

    def parse_loads(self, value):
        if value is None:
            return [1.0, 1.5, 2.0]
        parts = str(value).replace(',', ' ').split()
        return [float(p) for p in parts if p.strip()]

    def parse_targets(self, value):
        if value is None:
            return [24, 5, 13, 9]
        parts = str(value).replace(',', ' ').split()
        return [int(p) for p in parts if p.strip()]

    def split_csv(self, source_csv, managed_csv, unmanaged_csv):
        import csv
        with open(source_csv, newline='', encoding='utf-8') as fp_in:
            reader = csv.DictReader(fp_in)
            with open(managed_csv, 'w', newline='', encoding='utf-8') as fp_man, open(unmanaged_csv, 'w', newline='', encoding='utf-8') as fp_un:
                man_writer = csv.DictWriter(fp_man, fieldnames=reader.fieldnames)
                un_writer = csv.DictWriter(fp_un, fieldnames=reader.fieldnames)
                man_writer.writeheader()
                un_writer.writeheader()
                for row in reader:
                    if row.get('scenario') == 'managed':
                        man_writer.writerow(row)
                    else:
                        un_writer.writerow(row)

    def build_chart_payload(self, rows):
        return {
            'time': [row['time'].isoformat() for row in rows],
            'voltage': [row['voltage'] for row in rows],
            'soc': [row['soc'] for row in rows],
            'solar_current': [row['solar_current'] for row in rows],
            'solar_power': [row['solar_power'] for row in rows],
        }


if __name__ == '__main__':
    port = 8000
    with ThreadingTCPServer(('0.0.0.0', port), DashboardRequestHandler) as httpd:
        print(f'HTML dashboard running at http://localhost:{port}')
        httpd.serve_forever()
