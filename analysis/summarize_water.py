#!/usr/bin/env python3
import argparse
import math
import sys
from datetime import datetime, timezone
from pathlib import Path

import uproot

# CLHEP uses millimetres and nanoseconds, so gray = metre**2 / second**2.
GRAY_IN_GEANT4_UNITS = 1e-12


def pooled_statistics(rows):
    count = 0
    mean = 0.0
    m2 = 0.0
    for n, worker_mean, worker_m2 in rows:
        if n < 0:
            raise ValueError("A worker has a negative event count")
        if n == 0:
            continue
        if not math.isfinite(worker_mean) or not math.isfinite(worker_m2) or worker_m2 < 0:
            raise ValueError("A worker has invalid dose statistics")
        total = count + n
        delta = worker_mean - mean
        m2 += worker_m2 + delta * delta * count * n / total
        mean += delta * n / total
        count = total
    if count == 0:
        return 0, None, None, None
    stddev = math.sqrt(m2 / (count - 1)) if count > 1 else None
    stderr = stddev / math.sqrt(count) if stddev is not None else None
    return count, mean, stddev, stderr


def summarize(input_path):
    with uproot.open(input_path) as root_file:
        info_tree = root_file["runinfo"]
        fields = (
            "Tag", "Source", "Geometry", "Region", "DepthCm", "EMModel",
            "RadioactiveDecay", "Geant4Version", "RandomEngine",
        )
        info = info_tree.arrays([*fields, "Events", "ThreadId"], library="np")
        if len(info["ThreadId"]) == 0:
            raise ValueError("runinfo has no worker rows")
        metadata = {}
        for field in fields:
            values = info[field]
            if any(value != values[0] for value in values):
                raise ValueError(f"runinfo {field} differs between workers")
            metadata[field] = values[0]
        if "RandomState" not in info_tree:
            metadata["RandomStateStatus"] = "not recorded in this file"
        elif all(info_tree["RandomState"].array(library="np")):
            metadata["RandomStateStatus"] = "stored per stream in the ROOT runinfo tree"
        else:
            metadata["RandomStateStatus"] = "incomplete in the ROOT runinfo tree"
        if not math.isfinite(float(metadata["DepthCm"])):
            raise ValueError("runinfo has an invalid depth")

        worker_events = {}
        for thread, events in zip(info["ThreadId"], info["Events"]):
            thread, events = int(thread), int(events)
            if thread in worker_events or events < 0:
                raise ValueError("runinfo has duplicate workers or negative event counts")
            worker_events[thread] = events

        geometry_rows = root_file["water_region_run"].arrays(
            ["ThreadId", "Region", "InnerRadiusMm", "OuterRadiusMm", "MassG"],
            library="np",
        )
        geometry = {}
        geometry_workers = {}
        for thread, region, inner, outer, mass in zip(
            geometry_rows["ThreadId"], geometry_rows["Region"],
            geometry_rows["InnerRadiusMm"], geometry_rows["OuterRadiusMm"],
            geometry_rows["MassG"],
        ):
            thread, region = int(thread), int(region)
            dimensions = (float(inner), float(outer), float(mass))
            if thread not in worker_events or region < 0:
                raise ValueError("water_region_run has an unknown worker or region")
            if not all(math.isfinite(value) for value in dimensions) or not (
                0 <= inner < outer and mass > 0
            ):
                raise ValueError(f"Water region {region} has invalid geometry")
            workers = geometry_workers.setdefault(region, set())
            if thread in workers:
                raise ValueError(f"Water region {region} has duplicate geometry rows")
            workers.add(thread)
            if region in geometry and any(
                not math.isclose(a, b, rel_tol=1e-12, abs_tol=0.0)
                for a, b in zip(geometry[region], dimensions)
            ):
                raise ValueError(f"Water region {region} geometry differs between workers")
            geometry[region] = dimensions
        if set(geometry) not in (set(range(5)), set(range(6))):
            raise ValueError("water_region_run must contain five current or six legacy regions")
        if any(workers != set(worker_events) for workers in geometry_workers.values()):
            raise ValueError("water_region_run is missing worker or region geometry")

        stats_rows = root_file["score_statistics"].arrays(
            ["ThreadId", "Region", "Quantity", "Events", "Mean", "M2"],
            library="np",
        )
        stats = {}
        for thread, region, quantity, events, mean, m2 in zip(
            stats_rows["ThreadId"], stats_rows["Region"], stats_rows["Quantity"],
            stats_rows["Events"], stats_rows["Mean"], stats_rows["M2"],
        ):
            if quantity != "Dose":
                continue
            thread, region, events = int(thread), int(region), int(events)
            if thread not in worker_events or region not in {-1, *geometry}:
                raise ValueError("Dose statistics have an unknown worker or region")
            if events != worker_events[thread]:
                raise ValueError(f"Dose event count differs for worker {thread}, region {region}")
            workers = stats.setdefault(region, {})
            if thread in workers:
                raise ValueError(f"Duplicate dose statistics for worker {thread}, region {region}")
            workers[thread] = (events, float(mean), float(m2))
        if set(stats) != {-1, *geometry} or any(
            set(workers) != set(worker_events) for workers in stats.values()
        ):
            raise ValueError("score_statistics is missing worker or region dose rows")

        pooled = {
            region: pooled_statistics(workers.values())
            for region, workers in stats.items()
        }

    return metadata, worker_events, geometry, pooled


def format_report(input_path, metadata, worker_events, geometry, pooled):
    lines = [
        "sourceLab water dose report",
        f"Generated (UTC): {datetime.now(timezone.utc).isoformat(timespec='seconds')}",
        f"ROOT file: {input_path.resolve()}",
        "",
        "Run metadata",
        f"  Tag: {metadata['Tag']}",
        f"  Source: {metadata['Source']}",
        f"  Geometry: {metadata['Geometry']}",
        f"  Output region label: {metadata['Region']}",
        f"  Depth: {float(metadata['DepthCm']):g} cm",
        f"  EM model: {metadata['EMModel']}",
        f"  Radioactive decay: {metadata['RadioactiveDecay']}",
        f"  Geant4: {metadata['Geant4Version']}",
        f"  Random engine: {metadata['RandomEngine']}",
        f"  Events: {sum(worker_events.values())}",
        f"  Scoring streams: {len(worker_events)}",
        "  Events per stream (thread ID: events): "
        + ", ".join(f"{thread}: {worker_events[thread]}" for thread in sorted(worker_events)),
        f"  Random states: {metadata['RandomStateStatus']}",
        "",
        "Dose statistics (pooled across scoring streams; zero-score events included)",
    ]

    for region in [-1, *sorted(geometry)]:
        count, mean, stddev, stderr = pooled[region]
        if region == -1:
            lines.append("\nWhole water phantom")
        else:
            inner, outer, mass = geometry[region]
            lines.append(f"\nWater region {region}: {inner:g}-{outer:g} mm; mass {mass:g} g")
        lines.append(f"  Events: {count}")
        if mean is None:
            lines.append("  Dose statistics: undefined (no events)")
            continue
        mean_gy = mean / GRAY_IN_GEANT4_UNITS
        lines.append(f"  Total dose: {mean_gy * count:.6e} Gy")
        lines.append(f"  Mean dose per event: {mean_gy:.6e} Gy")
        if stddev is None or stderr is None:
            lines.append("  Standard deviation and standard error: undefined (fewer than two events)")
            continue
        lines.append(f"  Sample standard deviation: {stddev / GRAY_IN_GEANT4_UNITS:.6e} Gy")
        lines.append(f"  Standard error of mean: {stderr / GRAY_IN_GEANT4_UNITS:.6e} Gy")
        if mean != 0:
            lines.append(f"  Relative standard error: {100 * stderr / abs(mean):.3f}%")
        else:
            lines.append("  Relative standard error: undefined (zero mean)")

    lines.extend([
        "",
        "Uncertainties are statistical only; they do not include systematic uncertainty.",
        "Worker random states provide provenance, not guaranteed exact MT replay.",
    ])
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description="Summarize sourceLab water dose from a ROOT file.")
    parser.add_argument("input", type=Path, help="sourceLab ROOT output file")
    parser.add_argument(
        "--output", type=Path, help="report path (default: <ROOT filename>-report.txt)"
    )
    args = parser.parse_args()
    report_path = args.output or args.input.with_name(f"{args.input.stem}-report.txt")
    try:
        report = format_report(args.input, *summarize(args.input))
        with report_path.open("x", encoding="utf-8") as file:
            file.write(report)
    except (OSError, KeyError, ValueError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(report, end="")
    print(f"\nReport written to: {report_path.resolve()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
