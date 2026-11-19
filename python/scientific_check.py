import importlib.util
import os
import sys


def main(package_dir):
    root = os.path.realpath(package_dir)
    for name in ("numpy", "matplotlib"):
        spec = importlib.util.find_spec(name)
        if spec is None or spec.origin is None:
            raise RuntimeError("{} not found in {}".format(name, root))
        origin = os.path.realpath(spec.origin)
        print("{} candidate: {}".format(name, origin), flush=True)
        if os.path.commonpath((root, origin)) != root:
            raise RuntimeError("{} would be imported outside the test directory".format(name))

    import numpy as np
    import matplotlib
    import matplotlib.pyplot as plt

    print("NumPy {}: {}".format(np.__version__, np.__file__), flush=True)
    print("Matplotlib {}: {}".format(matplotlib.__version__, matplotlib.__file__), flush=True)
    values = np.array([0.0, 1.0, 2.0])
    if np.dot(values, values) != 5.0:
        raise RuntimeError("NumPy calculation returned an unexpected result")
    plt.plot(values, values ** 2)
    plt.title("Scientific library test")
    plt.show()
    if not any(name.startswith("plot-") and name.endswith(".png")
               for name in os.listdir(os.environ["AURORA_PLOT_DIR"])):
        raise RuntimeError("Matplotlib did not produce a plot")
    print("NumPy calculation and Matplotlib plot succeeded.", flush=True)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("Usage: scientific_check.py PACKAGE_DIR")
    main(sys.argv[1])
