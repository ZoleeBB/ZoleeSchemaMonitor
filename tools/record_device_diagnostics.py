"""A P4 webdiagnosztikájának mintavétele külső gépről, megszakításokon át."""
import argparse
import csv
from datetime import datetime, timezone
from http.client import HTTPException
import json
import math
from pathlib import Path
import time
from urllib.request import urlopen


FIELDS = ("firmware", "uptime", "sampleAge", "displayFps", "displayMaxRenderMs",
          "cpu0", "cpu1", "internalFree", "internalMinimum", "psramFree", "dmaLargest",
          "mainStackFree", "sdMounted", "sdSpaceValid", "displayWindowEndUs", "displayWindowUs",
          "displayFrames", "displayFramesOver50Ms")


def positive(value):
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError("Pozitív, véges szám szükséges")
    return number


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", help="A kijelző alapcíme, például http://zolee-schema-monitor.local")
    parser.add_argument("output", type=Path, help="Új CSV-fájl")
    parser.add_argument("--hours", type=positive, default=24)
    parser.add_argument("--interval", type=positive, default=1, help="Mintaköz másodpercben")
    args = parser.parse_args()
    url = args.url.rstrip("/") + "/api/diagnostics"
    start = time.monotonic()
    end = start + args.hours * 3600
    with args.output.open("x", encoding="utf-8", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=("utc", "elapsedSeconds", "status", *FIELDS,
                                                   "wifiConnected", "loxoneConnected"))
        writer.writeheader()
        next_sample = start
        while time.monotonic() < end:
            row = {"utc": datetime.now(timezone.utc).isoformat(timespec="milliseconds"),
                   "elapsedSeconds": round(time.monotonic() - start, 3)}
            try:
                with urlopen(url, timeout=4) as response:
                    sample = json.load(response)
                if not isinstance(sample, dict):
                    raise ValueError("Érvénytelen diagnosztikai válasz")
                if sample.get("mode") == "development":
                    parser.exit(2, "A cím a fejlesztői kiszolgálóhoz tartozik. A P4 címe szükséges.\n")
                if sample.get("measured") is True:
                    row.update((key, sample.get(key)) for key in FIELDS)
                    row.update(status="ok", wifiConnected=sample["wifi"]["connected"],
                               loxoneConnected=sample["loxone"]["connected"])
                else:
                    row["status"] = "unmeasured"
            except (OSError, HTTPException, ValueError, KeyError, TypeError) as error:
                row["status"] = type(error).__name__
            writer.writerow(row)
            output.flush()
            next_sample += args.interval
            now = time.monotonic()
            if next_sample < now:
                next_sample += math.ceil((now - next_sample) / args.interval) * args.interval
            time.sleep(max(0, min(next_sample, end) - now))


if __name__ == "__main__":
    main()
