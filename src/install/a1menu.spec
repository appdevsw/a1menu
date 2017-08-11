#The version should be set from cmake or from the command line. If you see 'versionundefined', something went wrong
%define version     versionundefined

%define name        a1menu
%define release     1
%define buildroot   %{_topdir}/BUILDROOT/%{name}-%{version}
%define builddir    %{_topdir}/BUILD/%{name}-%{version}
%define _unpackaged_files_terminate_build 0

 
BuildRoot:  	%{buildroot}
Summary:        A menu for MATE Desktop
License:        custom
Name:           %{name}
Version:        %{version}
Release:        %{release}
Source:         %{name}-%{version}.tar.gz
Prefix:         /usr
Group:          Development/Tools

BuildRequires:  cmake
BuildRequires:  libxml2-devel
#BuildRequires:  threads
BuildRequires:  gtk3-devel
BuildRequires:  mate-panel-devel
BuildRequires:  libX11-devel
BuildRequires:  gettext



%description
A menu for MATE Desktop
 
%prep
%setup -q
 
%build
cmake .
make -j 4
 
%install
make install DESTDIR=/%{buildroot}

%files -f %{builddir}/install_manifest.txt
%defattr(0755,root,root)

