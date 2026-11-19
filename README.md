# PyIDE for Aurora OS

PyIDE is a Python editor for Aurora OS built with C++, Qt 5/QML and Silica.
It bundles Python 3.11, pip, NumPy and Matplotlib, uses Runtime Manager to open
files, and is built with CMake.

Open `CMakeLists.txt` in Aurora IDE and build and deploy it using an Aurora OS kit.
The interface uses Russian when the system language is Russian, and English otherwise.
The application has its own icon and is not affiliated with the Python Software Foundation.
Pull down the editor menu to open, create, save or run a Python file, or to find
and replace text. The menu also shows the app version and a short description.
Ctrl+S saves; Ctrl+F opens search, Up/Down navigate matches,
Tab replaces the current match, and Escape closes search. Completion suggestions
include Python names and simple statement templates. New files are saved in Downloads without
overwriting an existing file. The editor accepts UTF-8 files; it preserves a
UTF-8 BOM and consistent CRLF line endings, but normalizes mixed line endings.
It refuses to overwrite a file changed externally and asks before discarding
unsaved edits.
The last document and its unsaved edits are restored on startup. If its file
changed externally, the cached edits are retained and cannot overwrite it.
The package ID is `ru.arpolend.PyIDE`. Installing it alongside an older
`ru.auroraos.PythonRunner` build does not migrate that build's editor cache.

PyIDE is registered for Python files and can open them from the file
manager. Opening a file does not run it automatically.

Run executes the current editor contents, including unsaved edits. The results
page appears when output or plots are available. Its menu saves text output as
a new `.txt` file in Downloads without overwriting existing files. Output is
limited to the most recent 1,048,576 characters. **Stop**
terminates the Python process.
The home-screen cover shows the editor or results according to the active page.

The application bundles Python 3.11 and pip from Aurora's experimental Conan
packages; no system Python is required. It also bundles NumPy 2.4.6 and
Matplotlib 3.10.8, with contourpy, cycler, fonttools, kiwisolver, packaging,
Pillow, pyparsing, python-dateutil and tzdata. This minimal scientific set
does not include SciPy or pandas.

The first build downloads about 55 MB of architecture-specific dependencies
from the Aurora Conan server and about 38 MB of Python wheels from PyPI.
Wheels are pinned and checked by SHA-256. The bundled scientific libraries
support aarch64 and x86_64, not 32-bit ARM. They target glibc 2.28 or newer;
compatibility and executable-file signatures must still be verified on the
device. The runtime is experimental.
Packaging uses elfutils to strip NumPy's OpenBLAS without corrupting its ELF
segments; the RPM check verifies the alignment of the packaged native modules.
The bundled runtime omits terminal-oriented `curses`, `readline`, and `crypt`
extensions and the optional `dbm.gnu` and `_uuid` extensions. `uuid` has a
Python fallback.

The libraries and their native extensions are included at build time, before
application signing. A successful pip install does not by itself mean native
extensions can be loaded. A load failure does not by itself identify the cause.
Previously downloaded package directories are left untouched but are no
longer added to the application's Python path.

Matplotlib uses the application's non-interactive Agg backend. Each `plt.show()`
renders the current figures as images in the application. Plots are not
interactive; `show()` does not block. A script that explicitly changes the
backend can bypass this integration. The sample `examples/plot.py` demonstrates
plotting. Scripts run in their own directory with no interactive standard input
and no command-line arguments. Only run scripts you trust: they inherit the
application's sandbox permissions.
