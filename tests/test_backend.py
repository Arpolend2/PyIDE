import os
import tempfile
import unittest
from datetime import datetime

import matplotlib

matplotlib.use("module://aurora_plot_backend", force=True)
import matplotlib.pyplot as plt


class PlotBackendTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.previous = os.environ.get("AURORA_PLOT_DIR")
        os.environ["AURORA_PLOT_DIR"] = self.directory.name

    def tearDown(self):
        plt.close("all")
        if self.previous is None:
            del os.environ["AURORA_PLOT_DIR"]
        else:
            os.environ["AURORA_PLOT_DIR"] = self.previous
        self.directory.cleanup()

    def test_show_saves_all_open_figures_each_time(self):
        plt.plot([1, 2])
        self.assertEqual(os.listdir(self.directory.name), [])
        plt.show()
        plt.figure()
        plt.plot([2, 1])
        plt.show()

        files = sorted(os.listdir(self.directory.name))
        self.assertEqual(len(files), 3)
        for name in files:
            self.assertTrue(name.startswith("plot-") and name.endswith(".png"))
            with open(os.path.join(self.directory.name, name), "rb") as image:
                self.assertEqual(image.read(8), b"\x89PNG\r\n\x1a\n")

    def test_plot_dates(self):
        plt.plot([datetime(2025, 1, 1), datetime(2025, 1, 2)], [1, 2])
        plt.show()
        self.assertEqual(len(os.listdir(self.directory.name)), 1)


if __name__ == "__main__":
    unittest.main()
