# PyIDE

A native Python editor for Aurora OS, built with C++, Qt and Silica.

- Open and edit Python files with syntax highlighting, completion, search and replace, and Ctrl+S to save.
- Run the current code, including unsaved edits; view text output and Matplotlib plots on a separate page.
- Restore the last document and unsaved edits on startup. Files changed outside the editor are not overwritten.
- Save output to Downloads; output is limited to the last 1,048,576 characters. The cover shows the current editor or results page.
- The interface is available in English and Russian.
- Includes Python 3.11, pip, NumPy and Matplotlib; no system Python is required. Plots are non-interactive.
- The bundled runtime is experimental and supports aarch64 and x86_64 with glibc 2.28 or newer. Native libraries installed later with pip may not load on Aurora OS.
- Run only trusted scripts: they inherit the app's permissions and have no interactive standard input.
- Build: open `CMakeLists.txt` in Aurora IDE and build with an Aurora SDK kit. The first build downloads pinned, checksum-verified dependencies.

PyIDE is not affiliated with the Python Software Foundation.
