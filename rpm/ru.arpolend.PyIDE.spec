%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude ^(libpython3\\.11|libssl\\.so\\.3|libcrypto\\.so\\.3|libffi\\.so|libmpdec\\.so\\.3|libsqlite3\\.so|liblzma\\.so\\.5|lib[A-Za-z0-9_]+-[0-9a-f]{8}).*$

Name:       ru.arpolend.PyIDE
Summary:    Run Python scripts on Aurora OS
Version:    0.1.0
Release:    1
License:    MIT and Python-2.0 and Apache-2.0 and BSD-2-Clause and BSD-3-Clause and Unlicense and 0BSD and Zlib and CC0-1.0 and MIT-CMU and matplotlib and FTL and LGPL-2.1-or-later and (GPL-3.0-or-later WITH GCC-exception-3.1)
Source0:    %{name}-%{version}.tar.bz2

BuildRequires: cmake
BuildRequires: elfutils
BuildRequires: python3
BuildRequires: qt5-qttools-linguist
BuildRequires: pkgconfig(auroraapp)
BuildRequires: pkgconfig(runtime-manager-qt5)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
Requires: sailfishsilica-qt5
Requires: nemo-qml-plugin-notifications-qt5

%description
Run local Python scripts and display matplotlib figures.
Includes Python, NumPy, Matplotlib, and their dependencies.

%prep
%autosetup

%build
%cmake
if [ -f build.ninja ]; then
    %{__ninja}
else
    %{__make} %{?_smp_mflags}
fi

%install
DESTDIR=%{buildroot} cmake --build . --target install

# GNU strip corrupts the LOAD segments in NumPy's OpenBLAS library.
for library in %{buildroot}%{_datadir}/%{name}/lib/site-packages/numpy.libs/libscipy_openblas64_*.so; do
    test -f "$library" || exit 1
    eu-strip "$library"
done

%check
%ifarch aarch64
python3 %{_builddir}/%{name}-%{version}/tools/check_elf_alignment.py %{buildroot}%{_datadir}/%{name}/lib/site-packages 65536
%else
python3 %{_builddir}/%{name}-%{version}/tools/check_elf_alignment.py %{buildroot}%{_datadir}/%{name}/lib/site-packages 4096
%endif

%files
%{_bindir}/%{name}
%{_libexecdir}/%{name}/python3.11
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
%{_datadir}/icons/hicolor/*/apps/%{name}.png
