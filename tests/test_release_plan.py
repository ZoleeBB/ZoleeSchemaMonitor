import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from release_plan import merged_pr, next_version


class ReleasePlanTests(unittest.TestCase):
    def test_single_digit_rollover_and_first_release(self):
        self.assertEqual(next_version([]), "0.1.0")
        self.assertEqual(next_version(["v0.1.9", "v0.1.8", "v0.2.0-preview"]), "0.2.0")
        self.assertEqual(next_version(["v0.9.9"]), "1.0.0")
        with self.assertRaises(ValueError):
            next_version(["v9.9.9"])

    def test_only_exact_merged_main_commit_can_publish(self):
        pull = {"number": 1, "merged_at": "2026-11-04", "merge_commit_sha": "exact", "base": {"ref": "main"}}
        self.assertIsNotNone(merged_pr([pull], "exact"))
        self.assertIsNone(merged_pr([pull], "other"))
        self.assertIsNone(merged_pr([{**pull, "merged_at": None}], "exact"))
        self.assertIsNone(merged_pr([{**pull, "base": {"ref": "MS1"}}], "exact"))


if __name__ == "__main__":
    unittest.main()
