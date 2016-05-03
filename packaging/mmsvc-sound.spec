Name:       mmsvc-sound
Summary:    A sound module for muse server
Version:    0.0.2
Release:    0
Group:      Multimedia/Libraries
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(mused)
#BuildRequires:  pkgconfig(alure)
#BuildRequires:  pkgconfig(libcanberra)
BuildRequires:  pkgconfig(libpulse)
BuildRequires:  pkgconfig(sndfile)

%description
A sound module for muse servier and native sound API.


%package devel
Summary:  A sound module for muse server (Development)
Requires: %{name} = %{version}-%{release}


%description devel
Development related files of a sound module for muse server.


%prep
%setup -q


%build
MAJORVER=`echo %{version} | awk 'BEGIN {FS="."}{print $1}'`
%cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DFULLVER=%{version} -DMAJORVER=${MAJORVER}
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE.APLv2 %{buildroot}/usr/share/license/%{name}


%make_install


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%files
%manifest mmsvc-sound.manifest
%{_libdir}/libmuse-sound.so*
%{_datadir}/license/%{name}

%files devel
%{_includedir}/media/*.h
%{_libdir}/pkgconfig/*.pc
%{_libdir}/libmuse-sound.so
