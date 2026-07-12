#!/usr/bin/env python3
"""Rule-based expert system for interpreting house-load and telemetry data.

This script turns the structural tree and simulator CSV into a simple
expert-system diagnosis with facts, triggered rules, and recommendations.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from typing import Any, Dict, List, Tuple


class ExpertSystem:
    def __init__(self) -> None:
        self.facts: Dict[str, Any] = {}
        self.rules: List[Tuple[str, callable, str]] = []

    def add_fact(self, name: str, value: Any) -> None:
        self.facts[name] = value

    def add_rule(self, name: str, predicate: callable, message: str) -> None:
        self.rules.append((name, predicate, message))

    def evaluate(self) -> List[Dict[str, Any]]:
        results: List[Dict[str, Any]] = []
        for name, predicate, message in self.rules:
            try:
                if predicate(self.facts):
                    results.append({"name": name, "message": message})
            except Exception:
                continue
        return results


def load_tree(path: Path) -> Dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def walk_tree(node: Dict[str, Any], visitor: callable) -> None:
    visitor(node)
    for child in node.get("children", []):
        walk_tree(child, visitor)


def parse_csv(path: Path) -> Dict[str, List[Dict[str, Any]]]:
    rows_by_scenario: Dict[str, List[Dict[str, Any]]] = {"managed": [], "unmanaged": []}
    if not path.exists():
        return rows_by_scenario
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        for row in reader:
            scenario = (row.get("scenario") or "").strip().lower()
            if scenario not in rows_by_scenario:
                continue
            rows_by_scenario[scenario].append(row)
    return rows_by_scenario


def summarize_telemetry(rows: List[Dict[str, Any]]) -> Dict[str, Any]:
    if not rows:
        return {
            "start_soc": None,
            "end_soc": None,
            "drop_soc": None,
            "min_soc": None,
            "avg_solar_current": None,
        }
    def parse_float(value: str | None) -> float | None:
        try:
            return float(value) if value not in (None, "") else None
        except ValueError:
            return None
    soc_values = [parse_float(row.get("battery_soc")) for row in rows if parse_float(row.get("battery_soc")) is not None]
    solar_values = [parse_float(row.get("solar_current")) for row in rows if parse_float(row.get("solar_current")) is not None]
    start_soc = soc_values[0] if soc_values else None
    end_soc = soc_values[-1] if soc_values else None
    drop_soc = (start_soc - end_soc) if start_soc is not None and end_soc is not None else None
    min_soc = min(soc_values) if soc_values else None
    avg_solar_current = sum(solar_values) / len(solar_values) if solar_values else None
    return {
        "start_soc": start_soc,
        "end_soc": end_soc,
        "drop_soc": drop_soc,
        "min_soc": min_soc,
        "avg_solar_current": avg_solar_current,
    }


def collect_load_facts(tree: Dict[str, Any]) -> Dict[str, Any]:
    leaf_nodes: List[Dict[str, Any]] = []
    forced_nodes: List[Dict[str, Any]] = []
    non_forced_nodes: List[Dict[str, Any]] = []

    def visit(node: Dict[str, Any]) -> None:
        amps = node.get("amps")
        if amps is not None:
            leaf_nodes.append(node)
            if node.get("forced", False):
                forced_nodes.append(node)
            else:
                non_forced_nodes.append(node)

    walk_tree(tree, visit)

    total_current = sum(float(node.get("amps", 0.0)) for node in leaf_nodes)
    forced_current = sum(float(node.get("amps", 0.0)) for node in forced_nodes)
    non_forced_current = sum(float(node.get("amps", 0.0)) for node in non_forced_nodes)
    top_loads = sorted(leaf_nodes, key=lambda item: float(item.get("amps", 0.0)), reverse=True)[:5]
    return {
        "leaf_count": len(leaf_nodes),
        "total_current": total_current,
        "forced_current": forced_current,
        "non_forced_current": non_forced_current,
        "top_loads": top_loads,
    }


def build_expert_system(load_facts: Dict[str, Any], telemetry: Dict[str, Dict[str, Any]]) -> ExpertSystem:
    system = ExpertSystem()
    system.add_fact("total_current", load_facts["total_current"])
    system.add_fact("forced_current", load_facts["forced_current"])
    system.add_fact("non_forced_current", load_facts["non_forced_current"])
    system.add_fact("leaf_count", load_facts["leaf_count"])
    system.add_fact("top_loads", load_facts["top_loads"])
    system.add_fact("managed", telemetry.get("managed", {}))
    system.add_fact("unmanaged", telemetry.get("unmanaged", {}))
    system.add_fact("inverter_limit", 15.0)

    def overload_rule(facts: Dict[str, Any]) -> bool:
        return facts["forced_current"] > facts["inverter_limit"]

    def high_risk_rule(facts: Dict[str, Any]) -> bool:
        return facts["forced_current"] > 10.0

    def deep_discharge_rule(facts: Dict[str, Any]) -> bool:
        managed = facts["managed"]
        unmanaged = facts["unmanaged"]
        return (managed.get("min_soc") is not None and managed["min_soc"] < 20.0) or (unmanaged.get("min_soc") is not None and unmanaged["min_soc"] < 20.0)

    def management_benefit_rule(facts: Dict[str, Any]) -> bool:
        managed = facts["managed"]
        unmanaged = facts["unmanaged"]
        if managed.get("end_soc") is None or unmanaged.get("end_soc") is None:
            return False
        return (managed["end_soc"] - unmanaged["end_soc"]) > 10.0

    def solar_gap_rule(facts: Dict[str, Any]) -> bool:
        managed = facts["managed"]
        unmanaged = facts["unmanaged"]
        avg_solar = None
        for scenario in (managed, unmanaged):
            if scenario.get("avg_solar_current") is not None:
                avg_solar = scenario["avg_solar_current"]
                break
        return avg_solar is not None and avg_solar < 1.0

    def high_demand_rule(facts: Dict[str, Any]) -> bool:
        if not facts["top_loads"]:
            return False
        top_amp = float(facts["top_loads"][0].get("amps", 0.0))
        return top_amp >= 5.0

    system.add_rule("overload", overload_rule, "Critical overload: the forced-load current exceeds the inverter limit and the system is not sustainable without shedding or reconfiguration.")
    system.add_rule("high_risk", high_risk_rule, "High risk: the forced-load current is already close to the inverter limit and any extra demand may trigger shutdowns.")
    system.add_rule("deep_discharge", deep_discharge_rule, "Battery depletion risk: one or more scenarios end below a safe reserve threshold.")
    system.add_rule("management_benefit", management_benefit_rule, "Management is helping: the managed scenario preserves significantly more battery charge than the unmanaged one.")
    system.add_rule("solar_gap", solar_gap_rule, "Solar contribution is low relative to the load profile, so the battery must carry more of the burden.")
    system.add_rule("high_demand", high_demand_rule, "A dominant load branch is very large and should be staggered or reassigned to keep the system stable.")
    return system


def build_recommendations(results: List[Dict[str, Any]], load_facts: Dict[str, Any], telemetry: Dict[str, Dict[str, Any]]) -> List[str]:
    recommendations: List[str] = []
    if any(rule["name"] == "overload" for rule in results):
        recommendations.append("Reduce or delay the highest-priority forced loads and split them across more time windows.")
    if any(rule["name"] == "high_risk" for rule in results):
        recommendations.append("Keep the inverter margin above 20% by shedding non-essential loads during peaks.")
    if any(rule["name"] == "deep_discharge" for rule in results):
        recommendations.append("Add reserve capacity or lower the minimum discharge threshold to protect battery health.")
    if any(rule["name"] == "management_benefit" for rule in results):
        recommendations.append("Continue using the managed policy, because it materially improves battery preservation.")
    if any(rule["name"] == "solar_gap" for rule in results):
        recommendations.append("Increase solar input or reduce daytime load spikes to reduce battery drain.")
    if any(rule["name"] == "high_demand" for rule in results):
        top_load = load_facts["top_loads"][0] if load_facts["top_loads"] else None
        if top_load is not None:
            recommendations.append(f"Inspect the heaviest load, {top_load.get('name')}, for possible staggering or load-sharing.")
    if not recommendations:
        recommendations.append("The current configuration appears stable; keep monitoring the trend and check again after any load changes.")
    return recommendations


def render_report(load_facts: Dict[str, Any], telemetry: Dict[str, Dict[str, Any]], results: List[Dict[str, Any]], recommendations: List[str]) -> str:
    lines: List[str] = []
    lines.append("Expert System Interpretation")
    lines.append("=" * 32)
    lines.append(f"Leaf loads: {load_facts['leaf_count']}")
    lines.append(f"Total load current: {load_facts['total_current']:.2f} A")
    lines.append(f"Forced current: {load_facts['forced_current']:.2f} A")
    lines.append(f"Non-forced current: {load_facts['non_forced_current']:.2f} A")
    if load_facts["top_loads"]:
        top_names = ", ".join(f"{node.get('name')} ({float(node.get('amps', 0.0)):.1f} A)" for node in load_facts["top_loads"])
        lines.append(f"Top loads: {top_names}")

    lines.append("")
    lines.append("Scenario summary")
    lines.append("-" * 18)
    for name in ("managed", "unmanaged"):
        summary = telemetry[name]
        lines.append(f"{name}: start {summary.get('start_soc') if summary.get('start_soc') is not None else 'n/a'}%, end {summary.get('end_soc') if summary.get('end_soc') is not None else 'n/a'}%, drop {summary.get('drop_soc') if summary.get('drop_soc') is not None else 'n/a'}%, min {summary.get('min_soc') if summary.get('min_soc') is not None else 'n/a'}%")

    lines.append("")
    lines.append("Triggered rules")
    lines.append("-" * 16)
    if results:
        for rule in results:
            lines.append(f"- {rule['name']}: {rule['message']}")
    else:
        lines.append("- No rule triggered. The system appears nominal for the provided data.")

    lines.append("")
    lines.append("Recommendations")
    lines.append("-" * 15)
    for recommendation in recommendations:
        lines.append(f"- {recommendation}")
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Run the expert-system interpreter over a house tree and telemetry CSV")
    parser.add_argument("--tree", default="validate/house_tree.json", help="Path to the house tree JSON file")
    parser.add_argument("--csv", default="validate/csv/bfs_run.csv", help="Path to the telemetry CSV file")
    parser.add_argument("--output", default="validate/csv/expert_system_report.txt", help="Path to write the text report")
    args = parser.parse_args()

    tree_path = Path(args.tree)
    csv_path = Path(args.csv)
    output_path = Path(args.output)

    if not tree_path.exists():
        raise FileNotFoundError(f"Tree file not found: {tree_path}")
    if not csv_path.exists():
        raise FileNotFoundError(f"CSV file not found: {csv_path}")

    tree = load_tree(tree_path)
    raw_rows = parse_csv(csv_path)
    telemetry = {name: summarize_telemetry(rows) for name, rows in raw_rows.items()}
    load_facts = collect_load_facts(tree)
    system = build_expert_system(load_facts, telemetry)
    results = system.evaluate()
    recommendations = build_recommendations(results, load_facts, telemetry)
    report = render_report(load_facts, telemetry, results, recommendations)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(report, encoding="utf-8")
    print(report)
    print(f"\nReport written to {output_path}")


if __name__ == "__main__":
    main()
