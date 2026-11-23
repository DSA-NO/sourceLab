import argparse

import numpy as np
import uproot


parser = argparse.ArgumentParser(description="Check a smoke run's ROOT output.")
parser.add_argument("file")
parser.add_argument("tree")
parser.add_argument("events", type=int)
parser.add_argument("--material", choices=("G4_WATER", "G4_TISSUE_SOFT_ICRP"))
args = parser.parse_args()

with uproot.open(args.file) as root_file:
    tree = root_file[args.tree]
    dose = tree["Dose"].array(library="np")
    if len(dose) != args.events or not np.isfinite(dose).all():
        raise ValueError(f"{args.tree}: expected {args.events} finite per-event doses")

    runinfo = root_file["runinfo"]
    events = runinfo["Events"].array(library="np")
    random_states = runinfo["RandomState"].array(library="np")
    if int(events.sum()) != args.events or len(random_states) == 0:
        raise ValueError("runinfo does not describe the expected events")
    if not all(random_states):
        raise ValueError("runinfo contains an empty random state")
    if args.material:
        materials = runinfo["PhantomMaterial"].array(library="np")
        if len(materials) == 0 or any(material != args.material for material in materials):
            raise ValueError(f"runinfo does not identify phantom material {args.material}")

print(f"Verified {args.events} events in {args.file}")
