Name:		jsrdbg
Version:	0.0.6
Release:	2%{?dist}
Summary:	JavaScript Remote Debugger for SpiderMonkey
Group:		Development/Debuggers
License:	GPLv2+
URL:		https://github.com/swojtasiak/jsrdbg
Source0:	https://github.com/swojtasiak/jsrdbg/archive/%{version}.tar.gz

BuildRequires:	libtool
BuildRequires:	gettext
BuildRequires:	mozjs24-devel
BuildRequires:	readline-devel
BuildRequires:	gettext-devel
BuildRequires:	gcc-c++
Requires:	mozjs24

%description
JSRDBG is an implementation of a high level debugging protocol for SpiderMonkey
engine which is available as a shared library. The library can be used to
integrate debugging facilities into an existing application leveraging
SpiderMonkey engine. There are several integration possibilities including
exposition of the high level debugger API locally directly to the application
and even exposing it to remote clients using full duplex TCP/IP communication.

%package devel
Summary: Header files, libraries and development documentation for %{name}
Group: Development/Libraries
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains the header files, static libraries and development
documentation for %{name}. If you like to develop programs using %{name},
you will need to install %{name}-devel.

%prep
%autosetup

%build
autoreconf -i
%configure
make %{?_smp_mflags}

%install
%make_install DESTDIR=%{buildroot}

%files
%doc
%{_libdir}/*.so
%{_libdir}/libjsrdbg.so.0
%{_libdir}/libjsrdbg.so.0.0.0
%{_bindir}/jrdb

%files devel
%{_libdir}/libjsrdbg.a
%{_libdir}/libjsrdbg.la
%{_libdir}/pkgconfig/*.pc
%{_includedir}/jsrdbg

%changelog

* Thu Mar 03 2016 Benjamin Kircher <kircher@otris.de> 0.0.6-2
- Devel package

* Wed Mar 02 2016 Benjamin Kircher <kircher@otris.de> 0.0.6-1
- Initial spec
