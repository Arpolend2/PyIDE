"""Non-interactive matplotlib backend for Aurora Python Runner."""

import os
import tempfile

from matplotlib import _pylab_helpers
from matplotlib.backends.backend_agg import FigureCanvasAgg, FigureManagerBase
from matplotlib.backend_bases import _Backend

FigureCanvas = FigureCanvasAgg
FigureManager = FigureManagerBase
_sequence = 0


def show(*args, **kwargs):
    global _sequence
    directory = os.environ["AURORA_PLOT_DIR"]
    for manager in _pylab_helpers.Gcf.get_all_fig_managers():
        _sequence += 1
        destination = os.path.join(directory, "plot-%08d.png" % _sequence)
        fd, temporary = tempfile.mkstemp(prefix=".plot-", suffix=".png", dir=directory)
        os.close(fd)
        try:
            manager.canvas.figure.savefig(temporary, format="png")
            os.replace(temporary, destination)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)


@_Backend.export
class _AuroraBackend(_Backend):
    FigureCanvas = FigureCanvas
    FigureManager = FigureManager
    show = staticmethod(show)
