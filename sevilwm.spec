Summary: Another version of evilwm, a minimalist window manager
Name: sevilwm
Version: 0.6.1
Release: 1
License: AEWM License
Group: User Interface/X
URL: http://user.ecc.u-tokyo.ac.jp/~s31552/wp/sevilwm/
Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description

%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
cp sevilwm $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT


%files
/usr/bin/sevilwm
%defattr(-,root,root,-)
%doc


%changelog
* Thu Jul 22 2004  <s31552@mail.ecc.u-tokyo.ac.jp> - 
- Initial build.

