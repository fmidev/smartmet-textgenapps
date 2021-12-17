%define BINNAME textgenapps
%define RPMNAME smartmet-%{BINNAME}
Summary: Weather text generator binary
Name: %{RPMNAME}
Version: 21.12.17
Release: 1%{?dist}.fmi
License: FMI
Group: Development/Tools
URL: https://github.com/fmidev/smartmet-textgenapps
Source0: %{name}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: boost169-devel
BuildRequires: fmt-devel
BuildRequires: gcc-c++
BuildRequires: gdal33-devel
BuildRequires: make
BuildRequires: mysql++-devel
BuildRequires: rpm-build
BuildRequires: smartmet-library-calculator-devel >= 21.10.6
BuildRequires: smartmet-library-newbase-devel >= 21.12.7
BuildRequires: smartmet-library-textgen-devel >= 21.12.17
BuildRequires: smartmet-library-macgyver-devel >= 21.12.7
BuildRequires: zlib-devel
Requires: boost169-iostreams
Requires: boost169-locale
Requires: boost169-system
Requires: fmt
Requires: gdal33-libs
Requires: glibc
Requires: libgcc
Requires: libjpeg
Requires: libpng
Requires: libstdc++
Requires: mysql++
Requires: smartmet-library-calculator >= 21.10.6
Requires: smartmet-library-macgyver >= 21.12.7
Requires: smartmet-library-newbase >= 21.12.7
Requires: smartmet-library-textgen >= 21.12.17
Requires: zlib
%if 0%{rhel} >= 8
BuildRequires: mariadb-devel
%else
BuildRequires: mysql-devel
Requires: mysql
%endif
Provides: qdtext
#TestRequires: smartmet-timezones
#TestRequires: smartmet-library-macgyver-devel >= 21.12.7
#TestRequires: smartmet-library-newbase-devel >= 21.12.7
#TestRequires: gcc-c++

%description
Weather Text Generator

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -n %{RPMNAME}

%build
make %{_smp_mflags}

%install
%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(0775,root,root,0775)
%{_bindir}/qdtext

%changelog
* Fri Dec 17 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.12.17-1.fmi
- Repackaged since textgen library ABI changed

* Tue Dec  7 2021 Andris Pavēnis <andris.pavenis@fmi.fi> 21.12.7-1.fmi
- Update to postgresql 13 and gdal 3.3

* Thu Oct 28 2021 Pertti Kinnia <pertti.kinnia@fmi.fi> - upcoming
- Added test dependency for smartmet-library-newbase-devel

* Wed Oct  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.10.6-1.fmi
- Repackaged due to API changes in calculator & textgen libraries

* Wed Sep 15 2021 Anssi Reponen <anssi.reponen@fmi.fi> - 21.9.15-1.fmi
- Support for PostgreSQL dictionary database added (BRAINSTORM-1707)

* Thu May  6 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.5.6-1.fmi
- Repackaged due to ABI changes in NFmiAzimuthalArea

* Mon Feb 22 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.2.22-1.fmi
- Repackaged due to newbase ABI changes

* Mon Jan 25 2021 Andris Pavenis <andris.pavenis@fmi.fi> - 21.1.25-1.fmi
- Build update:use makefile.inc

* Thu Jan 14 2021 Mika Heiskanen <mika.heiskanen@fmi.fi> - 21.1.14-1.fmi
- Repackaged smartmet to resolve debuginfo issues

* Tue Dec 15 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.12.15-1.fmi
- Upgrade to pgdg12

* Wed Oct 28 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.10.28-1.fmi
- Upgrade to fmt 7.1

* Fri Aug 21 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.8.21-1.fmi
- Upgrade to fmt 6.2

* Sat Apr 18 2020 Mika Heiskanen <mika.heiskanen@fmi.fi> - 20.4.18-1.fmi
- Upgraded to Boost 1.69

* Wed Dec  4 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.12.4-1.fmi
- Fixed dependency to be on gdal-libs instead of gdal
- Use -fno-omit-frame-pointer for a better profiling and debugging experience

* Tue Dec 3 2019 Anssi Reponen <anssi.reponen> - 19.12.3-1.fmi
- Test cases updated (BRAINSTORM-1727)

* Wed Nov 20 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.11.20-1.fmi
- Repackaged due to newbase API changes

* Thu Oct 31 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.10.31-1.fmi
- Rebuilt due to newbase API/ABI changes

* Fri Sep 27 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.27-1.fmi
- Repackaged due to ABI changes in SmartMet libraries

* Mon Sep  2 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.9.2-1.fmi
- Repackaged since TextGen::Dictionary ABI changed

* Thu Aug  1 2019 Mika Heiskanen <mika.heiskanen@fmi.fi> - 19.8.1-1.fmi
- Filenames can now contain time formatting directives

* Sun Sep 16 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.16-1.fmi
- Repackaged since calculator library API changed

* Thu Sep 13 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.9.13-1.fmi
- Updated since MessageLogger changed

* Mon Aug 20 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.20-1.fmi
- Avoid unnecessary string temporaries as instructed by clang-tidy

* Sun Aug 19 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.19-1.fmi
- Silenced more CodeChecker warnings

* Wed Aug  8 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.8.8-1.fmi
- Silenced CodeChecker warnings

* Sat Apr  7 2018 Mika Heiskanen <mika.heiskanen@fmi.fi> - 18.4.7-1.fmi
- Upgrade to boost 1.66

* Tue Sep 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.9.12-1.fmi
- Repackaged due to textgen library ABI change when auto_ptr was replaced by unique_ptr

* Mon Aug 28 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.8.28-1.fmi
- Upgrade to boost 1.65

* Tue Mar 14 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.3.14-1.fmi
- Recompiled with latest newbase, macgyver and textgen

* Mon Feb 13 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.2.13-1.fmi
- Repackaged due to newbase API change

* Thu Jan 12 2017 Mika Heiskanen <mika.heiskanen@fmi.fi> - 17.1.12-1.fmi
- Moved qdarea to qdtools
- Switched to FMI open source naming conventions

* Wed Jan 11 2017 Anssi Reponen <anssi.reponen@fmi.fi> - 17.1.11-1.fmi
- typo corrected

* Wed Sep 14 2016 Anssi Reponen <anssi.reponen@fmi.fi> - 16.9.14-1.fmi
- Version string written in log file

* Sat Jan 23 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.23-1.fmi
- Added optional output encoding

* Sun Jan 17 2016 Mika Heiskanen <mika.heiskanen@fmi.fi> - 16.1.17-1.fmi
- newbase API changed

* Wed Apr 15 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.15-1.fmi
- newbase API changed

* Thu Apr  9 2015 Mika Heiskanen <mika.heiskanen@fmi.fi> - 15.4.9-1.fmi
- newbase API changed

* Mon Mar 30 2015  <mheiskan@centos7.fmi.fi> - 15.3.30-1.fmi
- Use dynamic linking of smartmet libraries

* Fri Oct  3 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.10.3-1.fmi
- textgen::coordinates is now by default '/smartmet/share/coordinates/default.txt'

* Tue Sep 16 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.9.16-1.fmi
- qdarea::timezone is now by default 'local'
- qdarea::coordinates file is now by default '/smartmet/share/coordinates/default.txt'

* Mon Jun  2 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.6.2-1.fmi
- Recompiled to get the dependencies correct

* Thu May  8 2014 Mika Heiskanen <mika.heiskanen@fmi.fi> - 14.5.8-1.fmi
- Removed HDF5 dependency

* Tue Dec 17 2013 Mika Heiskanen <mika.heiskanen@fmi.fi> - 13.12.17-1.fmi
- Fixed a bug in handling aggregation for specific coordinates

* Fri Nov 30 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.11.30-1.fmi
- Latest versions of the weather stories into use

* Wed Nov 14 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.11.14-1.fmi
- PostGIS support
- Improved stores

* Tue Aug 28 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.8.28-1.el6.fmi
- Querydata is now memory mapped for speed

* Mon Aug 27 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.8.27-1.el6.fmi
- Improved output features
- Latest textgen features

* Thu Jul  5 2012 Mika Heiskanen <mika.heiskanen@fmi.fi> - 12.7.5-1.el6.fmi
- Migration to boost 1.50

* Wed Apr  4 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.4.4-1.el6.fmi
- Removed debugging logging from wind_overview

* Thu Mar 29 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.29-1.el6.fmi
- Improved wave story now handles separately cases where range low value is zero
- Added text-story

* Wed Mar 28 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.28-1.el6.fmi
- Removed debugging logging from wind_overview
- Added improved UTF-8 handling by using Boost.Locale and libicu
- Fixed UTF-8 capitalization of sentences
- Fixed UTF-8 capitalization of location names
- Added story wave_range
- Added default initialization of global locale

* Tue Mar 27 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.27-1.el6.fmi
- Improvements to wind_overview
- Improvements to temperature_max36hours
- Improvements to weather_forecast

* Fri Mar  2 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.2-1.el6.fmi
- Debug text formatter now uses newlines to separate things

* Thu Mar  1 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.1-3.el6.fmi
- Fixed a typo in a keyword on the wind turning towards the east

* Thu Mar  1 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.1-2.el6.fmi
- Fixed UTF-8 code of the degree sign

* Thu Mar  1 2012 mheiskan <mika.heiskanen@fmi.fi> - 12.3.1-1.el6.fmi
- Improved wind_overview
- Fixes to temperature and weather stories

* Thu Dec 15 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.15-3.el6.fmi
- A fix to temperature interval clamping

* Thu Dec 15 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.12.15-1.el6.fmi
- Latest UTF-8 version

* Thu Nov  3 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.11.3-1.el6.fmi
- Bug fix to 0-degrees and 1-degreees phrases

* Wed Nov  2 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.11.2-1.el6.fmi
- Bug fixes to smog and frost analysis
- Frost season is now configurable

* Mon Oct 31 2011 mheiskan <mika.heiskanen@fmi.fi> - 11.10.31-1.el6.fmi
- New UTF8 version

* Tue Jul 13 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.13-1.el5.fmi
- -O2 enabled again

* Mon Jul 12 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.12-1.el5.fmi
- -O2 temporarily disabled in textgen due to segmentation faults

* Mon Jul  5 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.7.5-1.el5.fmi
- Upgrade to newbase 10.7.5-2
- New stories by Anssi Reponen into use

* Thu May 20 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.5.20-1.el5.fmi
- Released with fixed textgen lib

* Wed Mar 17 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.17-1.el5.fmi
- Added "-T data"

* Mon Mar 15 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.15-1.el5.fmi
- New textgen library release with shared_ptr memory leaks removed

* Thu Mar 11 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.3.11-1.el5.fmi
- New parameter names

* Fri Jan 15 2010 mheiskan <mika.heiskanen@fmi.fi> - 10.1.15-1.el5.fmi
- Upgrade to boost 1.41

* Wed Oct  7 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.10.7-1.el5.fmi
- Recompiled with latest textgen

* Wed Sep  9 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.9.9-1.el5.fmi
- Linked with new textgen library (preciptation_total_day added)

* Tue May 12 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.5.12-1.el5.fmi
- Improved weather_overview

* Mon Mar 16 2009 mheiskan <mika.heiskanen@fmi.fi> - 9.3.16-1.el5.fmi
- Recompiled with newbase and textgen
- Added wmax, MaximumWind, HourlyMaximumWindSpeed

* Mon Sep 29 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.29-1.el5.fmi
- Newbase header changes forced a recompile

* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-2.el5.fmi
- Newbase bugfix forced a recompile

* Mon Sep 22 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.22-1.el5.fmi
- Linked with static boost 1.36

* Mon Sep 15 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.9.15-1.el5.fmi
- Linked with latest newbase to obtain mmap support

* Tue Mar 11 2008 mheiskan <mika.heiskanen@fmi.fi> - 8.3.11-1.el5.fmi
- Linked with latest libraries including wind interpolation fixes

* Fri Nov 30 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.6-1.el5.fmi
- Linked with bugfixed newbase (wind chill)

* Sat Nov 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.5-2.el5.fmi
- Removed mysql runtime dependency

* Fri Nov 23 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.4-1.el5.fmi
- New textgen release

* Thu Nov 15 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.3-1.el5.fmi
- New newbase and textgen into use

* Thu Oct 18 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.2-1.el5.fmi
- New newbase and textgen into use

* Mon Sep 24 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-3.el5.fmi
- Fixed "make depend".

* Fri Sep 14 2007 mheiskan <mika.heiskanen@fmi.fi> - 1.0.1-2.el5.fmi
- Improved build system

* Thu Jun  7 2007 tervo <tervo@xodin.weatherproof.fi> - 1.0.1-1.el5.fmi
- Initial build.
