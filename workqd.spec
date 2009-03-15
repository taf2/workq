%define moduledir /var/jobs
%define suexec_caller workqd

Summary: Work Queue
Name: workq
Version: 1.1.1
Release: 1
Vendor: Anerian, LLC
License: LGPL
Group: System Environment/Daemons
Source0: http://anerian.ws/dist/%{name}-%{version}.tar.gz
URL: http://anerian.ws
BuildRoot: %{_tmppath}/%{name}-root
BuildPrereq: pkgconfig, glib2-devel, mysql-devel, json-glib-devel
Prefix: %{_prefix}
Requires: glib2 >= 2.10.0
Requires: mysql-libs
Requires: json-glib

%description
Work Queue for handling long running tasks

%package devel
Summary: Work Queue development libraries
Group: Development/Libraries
Requires: workq

%description devel
Development libraries for extending workq

%prep
%setup

%build
%configure --enable-gtk-doc
CPPFLAGS="$RPM_OPT_FLAGS" make

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog NEWS README
%{_bindir}/%{name}d
%{_libdir}/%{name}/modules/*.so
%{_libdir}/*.so*

%files devel
/usr/share/*
%{_libdir}/*
%{_libdir}/%{name}/modules/*.a
%{_libdir}/%{name}/modules/*.la
%{_libdir}/%{name}-1.0/include/config.h
%{_includedir}/%{name}-1.0/%{name}/*.h
%{_libdir}/pkgconfig/%{name}-1.0.pc

%changelog
* Sat Jan 31 2009 Todd Fisher
- Initial RPM release.
