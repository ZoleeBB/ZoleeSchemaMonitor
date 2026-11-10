"""A P4-naplóban megfigyelt LVGL-mérési ablakok összesítése."""
import argparse
import csv
from pathlib import Path


WINDOW_FIELDS = ("displayWindowEndUs", "displayWindowUs", "displayFrames", "displayFramesOver50Ms")


def summarize(rows):
    previous_end = None
    windows = duplicates = missing_rows = frames = slow_frames = 0
    measured_us = gap_us = resets = 0
    previous_uptime = None
    for row in rows:
        if row["status"] != "ok":
            missing_rows += 1
            continue
        uptime = int(row["uptime"])
        restarted = previous_uptime is not None and uptime < previous_uptime
        previous_uptime = uptime
        if restarted:
            resets += 1
            previous_end = None
        if any(row[field] in (None, "") for field in WINDOW_FIELDS):
            missing_rows += 1
            continue
        end, duration, count, slow = (int(row[field]) for field in WINDOW_FIELDS)
        if not (0 < duration <= end and 0 <= slow <= count):
            raise ValueError("Érvénytelen képkockaablak")
        if end == previous_end:
            duplicates += 1
            continue
        if previous_end is not None:
            if end < previous_end:
                resets += 1
            elif end - duration < previous_end:
                raise ValueError("Átfedő képkockaablakok")
            else:
                gap_us += end - duration - previous_end
        previous_end = end
        windows += 1
        measured_us += duration
        frames += count
        slow_frames += slow
    return dict(windows=windows, duplicates=duplicates, missing_rows=missing_rows,
                frames=frames, slow_frames=slow_frames, measured_us=measured_us,
                gap_us=gap_us, resets=resets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="A record_device_diagnostics.py mérési naplója")
    args = parser.parse_args()
    try:
        with args.csv.open(encoding="utf-8-sig", newline="") as stream:
            rows = csv.DictReader(stream)
            if not {"status", "uptime", *WINDOW_FIELDS}.issubset(rows.fieldnames or []):
                parser.error("A CSV nem tartalmazza a képkockaablakok mérési mezőit.")
            result = summarize(rows)
    except (OSError, TypeError, ValueError, csv.Error) as error:
        parser.exit(2, f"A mérési napló nem értékelhető: {error}\n")
    seconds = result["measured_us"] / 1_000_000
    print(f"Mért idő: {seconds:.3f} s ({result['windows']} különböző ablak)")
    print(f"Ismételt ablak: {result['duplicates']}, mérés nélküli sor: {result['missing_rows']}")
    print(f"Azonos induláson belüli adathiány: {result['gap_us'] / 1_000_000:.3f} s")
    print(f"Észlelt üzemidő-visszaesés: {result['resets']}")
    if not seconds:
        parser.exit(1, "Nincs értékelhető képkockaablak.\n")
    print(f"Átlagos FPS a mért időben: {result['frames'] / seconds:.2f}")
    print(f"50 ms-nál hosszabb rajzolás: {result['slow_frames']} / {result['frames']} képkocka")


if __name__ == "__main__":
    main()
