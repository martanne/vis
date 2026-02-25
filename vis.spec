#
# spec file for package vis
#
# Copyright (c) 2024 SUSE LLC
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license text is stated at the bottom of this file or alternatively
# at the Open Source Initiative website at www.opensource.org/licenses/ISC.

# This macro extracts the Lua version number from %{flavor}.
# For luajit, hardcode 52 (LuaJIT is compatible with Lua 5.2 API).
# For other flavors (e.g. lua54, lua53), strip the "lua" prefix.
# Note: flavor:gsub() returns two values (string, count); wrapping in
# extra parentheses discards the count and returns only the string.
%lua_value %{lua:
local flavor = rpm.expand("%{flavor}")
if flavor == "luajit" then
    print("52")
else
    print((flavor:gsub("lua", "")))
end
}

%if "%{flavor}" == ""
%define flavor lua54
%endif

Name:           vis
Version:        0.9
Release:        0
Summary:        A vi-like editor combining modal editing with structural regular expressions
License:        ISC
Group:          Productivity/Text/Editors
URL:            https://github.com/martanne/vis
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  ncurses-devel
BuildRequires:  pkgconfig
%if "%{flavor}" == "luajit"
BuildRequires:  luajit-devel
%else
BuildRequires:  %{flavor}-devel
%endif

%description
Vis aims to be a modern, legacy-free, simple yet efficient editor,
combining the strengths of both vi(m) and sam.

It extends vi's modal editing with built-in support for multiple
cursors/selections and combines it with sam's structural regular
expression based command language.

%prep
%autosetup

%build
%configure \
    --enable-lua \
    --enable-lpeg-static
%make_build

%install
%make_install

%files
%license LICENSE
%doc README.md CHANGELOG.md
%{_bindir}/vis
%{_bindir}/vis-clipboard
%{_bindir}/vis-complete
%{_bindir}/vis-digraph
%{_bindir}/vis-menu
%{_bindir}/vis-open
%{_mandir}/man1/vis.1%{?ext_man}
%{_mandir}/man1/vis-clipboard.1%{?ext_man}
%{_mandir}/man1/vis-complete.1%{?ext_man}
%{_mandir}/man1/vis-digraph.1%{?ext_man}
%{_mandir}/man1/vis-menu.1%{?ext_man}
%{_mandir}/man1/vis-open.1%{?ext_man}
%dir %{_datadir}/vis
%{_datadir}/vis/

%changelog
