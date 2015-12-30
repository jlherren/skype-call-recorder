Name:           skype-call-recorder
Version:        0.10
Release:        1%{?dist}
Summary:        Recording tool for Skype Calls
License:        GPL
URL:            http://atdot.ch/scr/
Packager:       Jean-Luc Herren <jlh@gmx.ch>
Group:          Applications/Internet
Source0:        %{name}-%{version}.tar.bz2

BuildRequires:  lame-devel
BuildRequires:  id3lib-devel
BuildRequires:  libvorbis-devel

Requires:  lame
Requires:  id3lib
Requires:  libvorbis

%description
Skype Call recorder allows you to record Skype calls to MP3, Ogg Vorbis or WAV files.

%prep
%setup -q -n %{name}

%build
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} .
make

%install
rm -rf "%{buildroot}"
make DESTDIR="%{buildroot}" install

%files
%defattr(-,root,root)
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png

%clean
rm -rf "%{buildroot}"

%changelog
* Mon Jul 7 2014 Gawain Lynch <gawain.lynch@gmail.com> - 0.10-1
- Use RPM macros
- Added changelog
- Added Requires and BuildRequires
- Install into system defined prefix

* Wed Jul 10 2013 Jean-Luc Herren <jlh@gmx.ch> - 0.10-0
- Release 0.10

* Fri Jul 16 2010 Jean-Luc Herren <jlh@gmx.ch> - 0.9-1
- Release 0.9

* Tue Nov 18 2008 Jean-Luc Herren <jlh@gmx.ch> - 0.8-1
- Fix building RPMS
- Release 0.8

* Tue Oct 21 2008 Jean-Luc Herren <jlh@gmx.ch> - 0.7-1
- Release 0.7

* Fri Oct 17 2008 Jean-Luc Herren <jlh@gmx.ch> - 0.6-1
- Release 0.6

* Tue Jul 29 2008 Jean-Luc Herren <jlh@gmx.ch> - 0.5-1
- Release 0.5
- New spec file

