import unittest

from tools.summarize_frame_windows import WINDOW_FIELDS, summarize


def sample(uptime, *values):
    return {"status": "ok", "uptime": str(uptime), **dict(zip(WINDOW_FIELDS, map(str, values)))}


class FrameWindowTests(unittest.TestCase):
    def test_weights_actual_windows_skips_duplicates_and_separates_restarts(self):
        first = sample(10, 10000000, 1000000, 20, 1)
        result = summarize([first, first, {"status": "TimeoutError"},
                            sample(13, 13000000, 2000000, 10, 2),
                            sample(1, "", "", "", ""), sample(2, 2000000, 1000000, 0, 0)])
        self.assertEqual(result, dict(windows=3, duplicates=1, missing_rows=2,
                                      frames=30, slow_frames=3, measured_us=4000000,
                                      gap_us=1000000, resets=1))

    def test_invalid_frame_counts_and_overlapping_windows_fail(self):
        with self.assertRaises(ValueError):
            summarize([sample(1, 1000000, 1000000, 1, 2)])
        with self.assertRaises(ValueError):
            summarize([sample(1, 1000000, 1000000, 1, 0), sample(2, 2000000, 1500000, 1, 0)])
