import math
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

import uproot

PROJECT = Path(__file__).resolve().parents[1]
SCRIPT = PROJECT / "analysis" / "summarize_phantom.py"
LEGACY_SCRIPT = PROJECT / "analysis" / "summarize_water.py"
sys.path.insert(0, str(SCRIPT.parent))
from summarize_phantom import pooled_statistics


def write_fixture(
    path, *, bad_counts=False, bad_geometry=False, include_random_state=True,
    region_count=5, missing_region=None, phantom_material=None,
):
    threads = [0, 1, 2]
    counts = [3, 2, 0]
    info = {
        "Tag": ["fixture"] * 3,
        "Source": ["cs137"] * 3,
        "Geometry": ["water-annulus"] * 3,
        "Region": ["phantom"] * 3,
        "DepthCm": [0.0] * 3,
        "EMModel": ["option4"] * 3,
        "RadioactiveDecay": ["on"] * 3,
        "Geant4Version": ["test-version"] * 3,
        "RandomEngine": ["test-engine"] * 3,
        "RandomState": ["state"] * 3,
        "Events": counts,
        "ThreadId": threads,
    }
    if not include_random_state:
        del info["RandomState"]
    if phantom_material is not None:
        info["PhantomMaterial"] = phantom_material
    info_types = {
        name: "string" for name in info
        if name not in ("DepthCm", "Events", "ThreadId")
    }
    info_types.update({"DepthCm": "float64", "Events": "int32", "ThreadId": "int32"})

    geometry = {name: [] for name in (
        "ThreadId", "Region", "InnerRadiusMm", "OuterRadiusMm", "MassG"
    )}
    stats = {name: [] for name in ("ThreadId", "Region", "Quantity", "Events", "Mean", "M2")}
    for thread, count in zip(threads, counts):
        for region in range(-1, region_count):
            if region == missing_region:
                continue
            stats["ThreadId"].append(thread)
            stats["Region"].append(region)
            stats["Quantity"].append("Dose")
            stats["Events"].append(count + (bad_counts and thread == 1 and region == 4))
            stats["Mean"].append(([2e-12, 12e-12, math.nan])[thread])
            stats["M2"].append(([8e-24, 8e-24, 0.0])[thread])
            if region >= 0:
                geometry["ThreadId"].append(thread)
                geometry["Region"].append(region)
                geometry["InnerRadiusMm"].append(float(region + 1))
                geometry["OuterRadiusMm"].append(float(region + 2))
                geometry["MassG"].append(
                    2.0 if bad_geometry and thread == 1 and region == 4 else 1.0
                )

    with uproot.recreate(path) as root_file:
        root_file.mktree("runinfo", info_types).extend(info)
        root_file.mktree("water_region_run", {
            "ThreadId": "int32", "Region": "int32", "InnerRadiusMm": "float64",
            "OuterRadiusMm": "float64", "MassG": "float64",
        }).extend(geometry)
        root_file.mktree("score_statistics", {
            "ThreadId": "int32", "Region": "int32", "Quantity": "string",
            "Events": "int32", "Mean": "float64", "M2": "float64",
        }).extend(stats)


class PhantomReportTests(unittest.TestCase):
    def test_pooling_and_small_counts(self):
        count, mean, stddev, stderr = pooled_statistics(
            [(3, 2.0, 8.0), (0, math.nan, 0.0), (2, 12.0, 8.0)]
        )
        self.assertEqual(count, 5)
        self.assertAlmostEqual(mean, 6.0)
        self.assertAlmostEqual(stddev, math.sqrt(34.0))
        self.assertAlmostEqual(stderr, math.sqrt(6.8))
        self.assertEqual(pooled_statistics([(0, math.nan, 0.0)]), (0, None, None, None))
        self.assertEqual(pooled_statistics([(1, 4.0, 0.0)]), (1, 4.0, None, None))
        with self.assertRaises(ValueError):
            pooled_statistics([(1, math.nan, 0.0)])

    def test_report_and_existing_output(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "fixture.root"
            report_path = root.with_name("fixture-report.txt")
            write_fixture(root, phantom_material=["G4_WATER"] * 3)
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(root)], capture_output=True, text=True
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            report = report_path.read_text(encoding="utf-8")
            self.assertIn("Tag: fixture", report)
            self.assertIn("Phantom material: G4_WATER", report)
            self.assertIn("Events: 5", report)
            self.assertIn("Scoring streams: 3", report)
            self.assertIn("Water region 4: 5-6 mm; mass 1 g", report)
            self.assertIn("Total dose: 3.000000e+01 Gy", report)
            self.assertIn("Mean dose per event: 6.000000e+00 Gy", report)
            self.assertIn("Sample standard deviation: 5.830952e+00 Gy", report)
            self.assertIn("Standard error of mean: 2.607681e+00 Gy", report)
            self.assertEqual(report.count("Water region "), 5)

            repeated = subprocess.run(
                [sys.executable, str(SCRIPT), str(root)], capture_output=True, text=True
            )
            self.assertNotEqual(repeated.returncode, 0)
            self.assertIn("File exists", repeated.stderr)
            self.assertEqual(report_path.read_text(encoding="utf-8"), report)

    def test_inconsistent_workers_are_rejected(self):
        for problem in ({"bad_counts": True}, {"bad_geometry": True}, {"missing_region": 4}):
            with self.subTest(problem=problem), tempfile.TemporaryDirectory() as directory:
                root = Path(directory) / "fixture.root"
                write_fixture(root, **problem)
                result = subprocess.run(
                    [sys.executable, str(SCRIPT), str(root)], capture_output=True, text=True
                )
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("ERROR:", result.stderr)
                self.assertFalse(root.with_name("fixture-report.txt").exists())

    def test_legacy_entry_point_matches_phantom_report(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "fixture.root"
            write_fixture(root, phantom_material=["G4_TISSUE_SOFT_ICRP"] * 3)
            reports = []
            for index, script in enumerate((SCRIPT, LEGACY_SCRIPT)):
                output = Path(directory) / f"report-{index}.txt"
                result = subprocess.run(
                    [sys.executable, str(script), str(root), "--output", str(output)],
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                reports.append([
                    line for line in output.read_text(encoding="utf-8").splitlines()
                    if not line.startswith("Generated (UTC):")
                ])
            self.assertEqual(reports[0], reports[1])
            self.assertIn("  Phantom material: G4_TISSUE_SOFT_ICRP", reports[1])

    def test_legacy_module_imports(self):
        from summarize_water import pooled_statistics as legacy_pooled_statistics

        self.assertEqual(legacy_pooled_statistics([(1, 4.0, 0.0)]), (1, 4.0, None, None))
        result = subprocess.run(
            [sys.executable, "-c",
             "from analysis.summarize_water import pooled_statistics; "
             "assert pooled_statistics([(1, 4.0, 0.0)]) == (1, 4.0, None, None)"],
            cwd=PROJECT, capture_output=True, text=True,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_tissue_material_and_inconsistent_metadata(self):
        for materials in (
            ["G4_TISSUE_SOFT_ICRP"] * 3,
            ["G4_WATER", "G4_TISSUE_SOFT_ICRP", "G4_WATER"],
        ):
            with self.subTest(materials=materials), tempfile.TemporaryDirectory() as directory:
                root = Path(directory) / "fixture.root"
                write_fixture(root, phantom_material=materials)
                result = subprocess.run(
                    [sys.executable, str(SCRIPT), str(root)], capture_output=True, text=True
                )
                if materials[0] == "G4_TISSUE_SOFT_ICRP":
                    self.assertEqual(result.returncode, 0, result.stderr)
                    report = root.with_name("fixture-report.txt").read_text(encoding="utf-8")
                    self.assertIn("Phantom material: G4_TISSUE_SOFT_ICRP", report)
                    self.assertIn("Whole ICRP soft tissue phantom", report)
                    self.assertIn("ICRP soft tissue region 4:", report)
                else:
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn("PhantomMaterial differs between workers", result.stderr)

    def test_older_output_without_random_states_is_reported(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / "fixture.root"
            write_fixture(root, include_random_state=False, region_count=6)
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(root)], capture_output=True, text=True
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            report = root.with_name("fixture-report.txt").read_text(encoding="utf-8")
            self.assertIn("Random states: not recorded in this file", report)
            self.assertIn("Phantom material: not recorded (legacy water output)", report)
            self.assertIn("Water region 5:", report)


if __name__ == "__main__":
    unittest.main()
