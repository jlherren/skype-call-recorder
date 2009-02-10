
BuildRoot: @buildroot@

Summary: Recording tool for Skype Calls
Name: skype-call-recorder
Version: @version@
Release: 1
Source: %{name}-%{version}.tar.gz
License: GPL
URL: http://atdot.ch/scr/
Packager: jlh <jlh@gmx.ch>
Group: Applications/Internet

%description
Skype Call recorder allows you to record Skype calls to MP3, Ogg Vorbis or WAV files.

%prep
%setup -q

%build
cmake .
make

%install
rm -rf "%{buildroot}"
make DESTDIR="%{buildroot}" install

%files
%defattr(-,root,root)
/usr/local/bin/skype-call-recorder
/usr/local/share/applications/skype-call-recorder.desktop
/usr/local/share/icons/hicolor/128x128/apps/skype-call-recorder.png

%clean
rm -rf "%{buildroot}"

