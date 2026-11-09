import csv
from http.client import IncompleteRead
from io import BytesIO
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools import record_device_diagnostics as recorder


class DeviceRecorderTests(unittest.TestCase):
    def test_truncated_http_body_does_not_end_the_measurement(self):
        sample = b'{"measured":true,"wifi":{"connected":true},"loxone":{"connected":true}}'
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "measurements.csv"
            now = [0.0]

            def sleep(seconds):
                now[0] += seconds

            with patch("sys.argv", ["recorder", "http://panel", str(output), "--hours", "0.0005"]), \
                    patch.object(recorder.time, "monotonic", side_effect=lambda: now[0]), \
                    patch.object(recorder.time, "sleep", side_effect=sleep), \
                    patch.object(recorder, "urlopen", side_effect=[
                        IncompleteRead(b"{", 13), BytesIO(sample)
                    ]):
                recorder.main()

            with output.open(encoding="utf-8") as stream:
                rows = list(csv.DictReader(stream))
        self.assertEqual([row["status"] for row in rows], ["IncompleteRead", "ok"])
        self.assertEqual(rows[1]["loxoneConnected"], "True")
