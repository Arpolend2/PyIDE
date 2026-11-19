import os
import pathlib
import subprocess
import sys
import tempfile
import unittest


class ScientificCheckTest(unittest.TestCase):
    def test_rejects_packages_outside_selected_directory(self):
        script = pathlib.Path(__file__).resolve().parents[1] / "python" / "scientific_check.py"
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            selected = root / "selected"
            selected.mkdir()
            other = root / "other"
            (other / "numpy").mkdir(parents=True)
            (other / "numpy" / "__init__.py").write_text("", encoding="utf-8")
            env = dict(os.environ, PYTHONPATH=str(other))
            result = subprocess.run(
                [sys.executable, "-S", str(script), str(selected)],
                env=env, text=True, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("numpy would be imported outside the test directory", result.stderr)


if __name__ == "__main__":
    unittest.main()
