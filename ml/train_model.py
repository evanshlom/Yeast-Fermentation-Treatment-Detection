"""
train_model.py — Train a RandomForest gas-classification model and
compile it directly into plain C (no runtime needed on the STM32).

Usage (inside Docker, from the project folder):
    docker build -t gas-ml -f ml/Dockerfile ml
    docker run --rm -v "${PWD}:/data" -w /data gas-ml python ml/train_model.py training_csvs/*.csv

Optional: --tail N   only use the last N rows of each class (after
                      excluding any Type not in CLASS_LABELS below).
                      Default: 300.

Input CSVs must have at least these columns (extra columns are ignored):
    CO2_AOUTv, ALC_AOUTv, Type

Any row whose Type isn't in CLASS_LABELS (e.g. "bakingsoda", if you're
excluding it for this run) is dropped, not treated as an error — this
lets you keep bakingsoda in your raw CSVs and just leave it out of
CLASS_LABELS below when you don't want it in a given training run.

Output (written to /data, i.e. your project folder via the volume mount):
    ml_model.c   — drop this into your STM32 project, replacing the
                   placeholder version. Contains the compiled decision
                   trees as plain if/else C — no library, no runtime.
    ml_model.h   — matching header, already referenced by main_test.c.

After generating these, just run:
    make test
    .\\flash_and_monitor.ps1 -BinName gas_monitor_test.bin
"""

import sys
import glob
import argparse
import pandas as pd
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix
import m2cgen as m2c

# Fixed class order — this exact order is what class index 0/1/2 means
# in the generated C code. Do not reorder without regenerating.
# Change this list to add/remove classes for a given training run —
# rows with any other Type value are dropped, not an error.
CLASS_LABELS = ["openair", "regular", "activatedcharcoal"]

# Kept intentionally small: this model gets compiled into literal
# nested if/else C code. More trees / deeper trees = a much larger
# generated file. With only 2-3 real features and 3 classes, this
# should be more than enough to separate them cleanly.
N_ESTIMATORS = 25
MAX_DEPTH = 5


REQUIRED_COLS = ["CO2_AOUTv", "ALC_AOUTv", "Type"]


def load_data(paths):
    frames = []
    for p in paths:
        df = pd.read_csv(p)

        # Match required columns case-insensitively, then rename to
        # the canonical casing used everywhere else in this script —
        # avoids errors from CSVs with e.g. "type" instead of "Type".
        rename_map = {}
        for required in REQUIRED_COLS:
            match = next(
                (c for c in df.columns if c.lower() == required.lower()), None
            )
            if match is not None and match != required:
                rename_map[match] = required
        if rename_map:
            df = df.rename(columns=rename_map)

        missing = set(REQUIRED_COLS) - set(df.columns)
        if missing:
            print(f"ERROR: {p} is missing columns: {missing}")
            sys.exit(1)
        frames.append(df)
    return pd.concat(frames, ignore_index=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csvs", nargs="+", help="CSV file(s) or glob pattern(s)")
    parser.add_argument(
        "--tail", type=int, default=300,
        help="Only use the last N rows of each class, after filtering "
             "to CLASS_LABELS (default: 300)",
    )
    args = parser.parse_args()

    paths = []
    for arg in args.csvs:
        paths.extend(glob.glob(arg))

    if not paths:
        print("No CSV files matched.")
        sys.exit(1)

    print(f"Loading {len(paths)} file(s)...")
    df = load_data(paths)

    print(f"Total rows loaded: {len(df)}")
    print(df["Type"].value_counts())

    # Drop any Type not in CLASS_LABELS (e.g. excluding bakingsoda for
    # this run) rather than erroring — lets one CSV set serve multiple
    # training runs with different class subsets.
    excluded = set(df["Type"].unique()) - set(CLASS_LABELS)
    if excluded:
        dropped = len(df) - len(df[df["Type"].isin(CLASS_LABELS)])
        print(f"Excluding Type value(s) not in CLASS_LABELS: {excluded} "
              f"({dropped} rows dropped)")
    df = df[df["Type"].isin(CLASS_LABELS)]

    if df.empty:
        print(f"ERROR: no rows left after filtering to {CLASS_LABELS}")
        sys.exit(1)

    # Only use the last N rows per class — keeps training data to the
    # settled/steady-state portion of each session rather than the
    # transition period right after switching conditions.
    df = df.groupby("Type", group_keys=False).tail(args.tail)

    print(f"\nAfter filtering to {CLASS_LABELS} and taking last "
          f"{args.tail} rows per class:")
    print(f"Total rows used: {len(df)}")
    print(df["Type"].value_counts())

    # Feature engineering: ratio is the primary signal since it should
    # stay stable across sessions even as absolute voltages drift.
    df["ratio"] = df["ALC_AOUTv"] / df["CO2_AOUTv"].replace(0, np.nan)
    df = df.dropna(subset=["ratio"])

    X = df[["CO2_AOUTv", "ALC_AOUTv", "ratio"]].values
    y = df["Type"].map(CLASS_LABELS.index).values

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.25, stratify=y, random_state=42
    )

    model = RandomForestClassifier(
        n_estimators=N_ESTIMATORS,
        max_depth=MAX_DEPTH,
        random_state=42,
    )
    model.fit(X_train, y_train)

    y_pred = model.predict(X_test)
    print("\n--- Holdout evaluation ---")
    print(classification_report(y_test, y_pred, target_names=CLASS_LABELS))
    print("Confusion matrix (rows=actual, cols=predicted):")
    print(confusion_matrix(y_test, y_pred))
    print(f"Classes, in order (index 0/1/2): {CLASS_LABELS}")

    # ── Compile to C ──────────────────────────────────────────────
    generated = m2c.export_to_c(model)
    generated = generated.replace("void score(", "static void ml_score(")

    labels_array = ", ".join(f'"{c}"' for c in CLASS_LABELS)

    c_file = f"""/**
 * @file    ml_model.c
 * @brief   AUTO-GENERATED by train_model.py — do not hand-edit.
 *          RandomForest ({N_ESTIMATORS} trees, max_depth={MAX_DEPTH})
 *          compiled directly into C via m2cgen. No runtime needed.
 *
 * Class order (index 0/1/2): {CLASS_LABELS}
 */

#include "ml_model.h"
#include <string.h>

{generated}

const char * const ML_CLASS_LABELS[ML_NUM_CLASSES] = {{ {labels_array} }};

int ml_predict_class(float co2_aoutv, float alc_aoutv)
{{
    double ratio = (co2_aoutv != 0.0f) ? (double)(alc_aoutv / co2_aoutv) : 0.0;
    double input[3]  = {{ (double)co2_aoutv, (double)alc_aoutv, ratio }};
    double output[ML_NUM_CLASSES];

    ml_score(input, output);

    int best_idx = 0;
    for (int i = 1; i < ML_NUM_CLASSES; i++)
    {{
        if (output[i] > output[best_idx])
        {{
            best_idx = i;
        }}
    }}

    return best_idx;
}}
"""

    h_file = """/**
 * @file    ml_model.h
 * @brief   AUTO-GENERATED by train_model.py — do not hand-edit.
 */

#ifndef ML_MODEL_H
#define ML_MODEL_H

#define ML_NUM_CLASSES  3

extern const char * const ML_CLASS_LABELS[ML_NUM_CLASSES];

/* Returns the predicted class index (0/1/2) — look up
 * ML_CLASS_LABELS[result] for the string name. */
int ml_predict_class(float co2_aoutv, float alc_aoutv);

#endif /* ML_MODEL_H */
"""

    with open("ml_model.c", "w") as f:
        f.write(c_file)
    with open("ml_model.h", "w") as f:
        f.write(h_file)

    print("\nWrote ml_model.c and ml_model.h")


if __name__ == "__main__":
    main()