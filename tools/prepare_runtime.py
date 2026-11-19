import os
import pathlib
import stat
import sys
import tarfile
import zipfile


def write_member(archive, name, destination, executable=False):
    member = archive.getmember(name)
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    with archive.extractfile(member) as source, open(destination, "wb") as target:
        while True:
            block = source.read(1024 * 1024)
            if not block:
                break
            target.write(block)
    if executable:
        os.chmod(destination, os.stat(destination).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)


def prepare(python_archive, ssl_archive, ffi_archive, decimal_archive, sqlite_archive, xz_archive, output, wheels):
    os.makedirs(output, exist_ok=True)
    with tarfile.open(python_archive, "r:gz") as archive:
        write_member(archive, "bin/python3.11", os.path.join(output, "bin", "python3.11"), True)
        write_member(archive, "lib/libpython3.11.so.1.0",
                     os.path.join(output, "lib", "libpython3.11.so.1.0"))
        write_member(archive, "licenses/LICENSE", os.path.join(output, "licenses", "Python-LICENSE"))
        write_member(archive, "lib/python3.11/ensurepip/_bundled/pip-24.0-py3-none-any.whl",
                     os.path.join(output, "lib", "pip.whl"))
        prefix = "lib/python3.11/"
        dynamic = prefix + "lib-dynload/"
        with zipfile.ZipFile(os.path.join(output, "lib", "python311.zip"), "w",
                             compression=zipfile.ZIP_DEFLATED) as library:
            for member in archive:
                name = member.name
                if not member.isfile() or not name.startswith(prefix):
                    continue
                relative = name[len(prefix):]
                parts = relative.split("/")
                if "__pycache__" in parts or "test" in parts or "tests" in parts or name.endswith(".pyc"):
                    continue
                if name.startswith(dynamic):
                    excluded = ("_crypt.", "_curses.", "_curses_panel.", "_gdbm.", "_uuid.", "readline.")
                    if name.endswith(".so") and not os.path.basename(name).startswith(excluded):
                        write_member(archive, name, os.path.join(output, name))
                elif not name.startswith(prefix + "site-packages/"):
                    with archive.extractfile(member) as source:
                        library.writestr(relative, source.read())

    for path, mappings in (
        (ssl_archive, [("lib/libssl.so.3", "lib/libssl.so.3"),
                       ("lib/libcrypto.so.3", "lib/libcrypto.so.3"),
                       ("licenses/LICENSE.txt", "licenses/OpenSSL-LICENSE")]),
        (ffi_archive, [("lib/libffi.so", "lib/libffi.so"),
                       ("licenses/LICENSE", "licenses/libffi-LICENSE")]),
        (decimal_archive, [("lib/libmpdec.so.2.5.1", "lib/libmpdec.so.3"),
                           ("licenses/LICENSE.txt", "licenses/mpdecimal-LICENSE")]),
        (sqlite_archive, [("lib/libsqlite3.so", "lib/libsqlite3.so"),
                          ("licenses/LICENSE", "licenses/sqlite-LICENSE")]),
        (xz_archive, [("lib/liblzma.so.5.8.1", "lib/liblzma.so.5"),
                      ("licenses/COPYING", "licenses/xz-LICENSE")]),
    ):
        with tarfile.open(path, "r:gz") as archive:
            for name, destination in mappings:
                write_member(archive, name, os.path.join(output, destination))

    site_packages = os.path.join(output, "lib", "site-packages")
    os.makedirs(site_packages, exist_ok=True)
    for wheel in wheels:
        with zipfile.ZipFile(wheel) as archive:
            for member in archive.infolist():
                path = pathlib.PurePosixPath(member.filename)
                if member.is_dir():
                    continue
                if (path.is_absolute() or "\\" in member.filename
                        or any(part in ("", ".", "..") for part in path.parts)):
                    raise ValueError("Unsafe wheel member: " + member.filename)
                if path.parts[0].endswith(".data"):
                    continue
                destination = os.path.join(site_packages, *path.parts)
                os.makedirs(os.path.dirname(destination), exist_ok=True)
                with archive.open(member) as source, open(destination, "wb") as target:
                    while True:
                        block = source.read(1024 * 1024)
                        if not block:
                            break
                        target.write(block)


if __name__ == "__main__":
    if len(sys.argv) < 9:
        sys.exit("Usage: prepare_runtime.py CPYTHON OPENSSL LIBFFI MPDECIMAL SQLITE XZ OUTPUT WHEEL...")
    prepare(*sys.argv[1:8], sys.argv[8:])
