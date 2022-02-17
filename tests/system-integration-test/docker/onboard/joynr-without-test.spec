# -*- rpm-spec -*-
BuildRoot:      /data/build/joynr/package/RPM/joynr
Summary:        joynr base package including executables, libraries and default settings
Name:           joynr
Version:        1.19.8
Release:        1
License:        Apache License, Version 2.0
Group:          unknown
Vendor:         BMW Car IT GmbH (http://www.bmw-carit.de)
Url: https://github.com/bmwcarit/joynr

Prefix: /usr

%define _rpmdir /data/build/joynr/package/RPM
%define _unpackaged_files_terminate_build 0
%define _topdir /data/build/joynr/package/RPM
%debug_package

%description
joynr is a web-based communication framework for Java, Javascript and C++
applications wanting to interact with other applications, no matter whether
they're deployed on consumer devices, vehicles, or backend infrastructure.

joynr makes writing distributed applications easy, as it:

* takes care of determining the most appropriate communication paradigm to
  talk with the desired end point

* provides a simple application programming interface to the joynr framework

* speeds up integration of new applications

... allowing you to focus on building your distributed application.

Have a peek at our documentation on joynr.io for more information!

%package devel
Summary: joynr extra development files
Requires: %{name}-%{?_isa} = %{version}-%{release}

%description devel
The joynr cmake support files, C++ headers and code generator

%prep
mv $RPM_BUILD_ROOT /data/build/joynr/package/RPM/tmp

%install
if [ -e $RPM_BUILD_ROOT ];
then
  rm -rf $RPM_BUILD_ROOT
fi
mv /data/build/joynr/package/RPM/tmp $RPM_BUILD_ROOT

%clean

%post

%postun

%pre

%preun

%files
%defattr(-,root,root,755)
/usr/bin/cluster-controller
/etc/joynr
/usr/share/doc
/usr/lib64/libJoynr*.so*

%files devel
%defattr(-,root,root,-)
%dir "/usr/lib64/cmake"
/usr/lib64/cmake/joynr-*
/usr/lib64/cmake/JoynrGenerator/*
/usr/include/joynr-*
/usr/bin/joynr-generator
/usr/libexec
