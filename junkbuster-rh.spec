# $Id: junkbuster-rh.spec,v 1.11 2001/07/03 11:00:25 sarantis Exp $
#
# Written by and Copyright (C) 2001 the SourceForge
# IJBSWA team.  http://ijbswa.sourceforge.net
#
# Based on the Internet Junkbuster originally written
# by and Copyright (C) 1997 Anonymous Coders and 
# Junkbusters Corporation.  http://www.junkbusters.com
#
# This program is free software; you can redistribute it 
# and/or modify it under the terms of the GNU General
# Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at
# your option) any later version.
#
# This program is distributed in the hope that it will
# be useful, but WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public
# License for more details.
#
# The GNU General Public License should be included with
# this file.  If not, you can view it at
# http://www.gnu.org/copyleft/gpl.html
# or write to the Free Software Foundation, Inc., 59
# Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# $Log: junkbuster-rh.spec,v $
# Revision 1.11  2001/07/03 11:00:25  sarantis
# replaced permissionsfile with actionsfile
#
# Revision 1.10  2001/07/03 09:34:44  sarantis
# bumped up version number.
#
# Revision 1.9  2001/06/12 18:15:29  swa
# the % in front of configure (see tag below) confused
# the rpm build process on 7.1.
#
# Revision 1.8  2001/06/12 17:15:56  swa
# fixes, because a clean build on rh6.1 was impossible.
# GZIP confuses make, % configure confuses rpm, etc.
#
# Revision 1.7  2001/06/11 12:17:26  sarantis
# fix typo in %post
#
# Revision 1.6  2001/06/11 11:28:25  sarantis
# Further optimizations and adaptations in the spec file.
#
# Revision 1.5  2001/06/09 09:14:11  swa
# shamelessly adapted RPM stuff from the newest rpm that
# RedHat provided for the JB.
#
# Revision 1.4  2001/06/08 20:54:18  swa
# type with status file. remove forward et. al from file list.
#
# Revision 1.3  2001/06/07 17:28:10  swa
# cosmetics
#
# Revision 1.2  2001/06/04 18:31:58  swa
# files are now prefixed with either `confdir' or `logdir'.
# `make redhat-dist' replaces both entries confdir and logdir
# with redhat values
#
# Revision 1.1  2001/06/04 10:44:57  swa
# `make redhatr-dist' now works. Except for the paths
# in the config file.
#
#
#
Summary: The Internet Junkbuster
Vendor: http://ijbswa.sourceforge.net
Name: junkbuster
Version: 2.9.8
Release: 1
Source0: http://www.waldherr.org/junkbuster/ijbswa.tar.gz
Copyright: GPL
BuildRoot: %{_tmppath}/%{name}-root
Group: Networking/Utilities
URL: http://ijbswa.sourceforge.net/
Packager: Stefan Waldherr <stefan@waldherr.org>
Obsoletes: junkbuster-raw junkbuster-blank
Prereq: /usr/sbin/useradd , /sbin/chkconfig , /sbin/service 
Conflicts: junkbuster-raw junkbuster-blank

%description
The Internet Junkbuster stops your browser from displaying the
advertisement images that pervade many commercial web pages.  Since
your browser has to download fewer images, surfing the web should be
faster.

%define ijbconf %{_sysconfdir}/junkbuster

%prep
%setup -c -n ijbswa

%build
./configure
make
strip junkbuster

%pre
/usr/sbin/useradd -d /etc/junkbuster -u 73 -r junkbust > /dev/null 2>&1 || /bin/true

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p ${RPM_BUILD_ROOT}%{_sbindir} \
         ${RPM_BUILD_ROOT}%{_mandir}/man8 \
         ${RPM_BUILD_ROOT}/var/log/junkbuster \
         ${RPM_BUILD_ROOT}%{ijbconf}/templates \
         ${RPM_BUILD_ROOT}%{_sysconfdir}/logrotate.d \
         ${RPM_BUILD_ROOT}%{_sysconfdir}/rc.d/init.d

install -s -m 744 junkbuster $RPM_BUILD_ROOT%{_sbindir}/junkbuster
cp -f junkbuster.1 $RPM_BUILD_ROOT%{_mandir}/man8/junkbuster.8
cp -f actionsfile $RPM_BUILD_ROOT%{ijbconf}/actionsfile
cp -f re_filterfile $RPM_BUILD_ROOT%{ijbconf}/re_filterfile
cp -f trust $RPM_BUILD_ROOT%{ijbconf}/trust
cp -f templates/*  $RPM_BUILD_ROOT%{ijbconf}/templates/
cp -f junkbuster.logrotate $RPM_BUILD_ROOT%{_sysconfdir}/logrotate.d/junkbuster
install -m 755 junkbuster.init $RPM_BUILD_ROOT%{_sysconfdir}/rc.d/init.d/junkbuster
install -m 744 -d $RPM_BUILD_ROOT/var/log/junkbuster

# verify all file locations, etc. in the config file
# don't start with ^ or commented lines are not replaced
cat config | \
    sed 's/^confdir.*/confdir \/etc\/junkbuster/g' | \
#    sed 's/^permissionsfile.*/permissionsfile \/etc\/junkbuster\/permissionsfile/g' | \
#    sed 's/^re_filterfile.*/re_filterfile \/etc\/junkbuster\/re_filterfile/g' | \
#    sed 's/^logfile.*/logfile \/var\/log\/junkbuster\/logfile/g' | \
#    sed 's/^jarfile.*/jarfile \/var\/log\/junkbuster\/jarfile/g' | \
#    sed 's/^forward.*/forward \/etc\/junkbuster\/forward/g' | \
#    sed 's/^aclfile.*/aclfile \/etc\/junkbuster\/aclfile/g' > \
    sed 's/^logdir.*/logdir \/var\/log\/junkbuster/g' > \
    $RPM_BUILD_ROOT%{ijbconf}/config

%post
if [ "$1" = "1" ]; then
        /sbin/chkconfig --add junkbuster
fi

%preun
if [ "$1" = "0" ]; then
	/sbin/service junkbuster stop > /dev/null 2>&1 ||:
	/sbin/chkconfig --del junkbuster
fi

%postun
if [ "$1" -ge "1" ]; then
	/sbin/service junkbuster condrestart > /dev/null 2>&1
fi

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
# %doc ijbfaq.html ijbman.html README README.TOO gpl.html
%doc junkbuster.weekly junkbuster.monthly
%dir %{ijbconf}
%config %{ijbconf}/*
%attr(0744,junkbust,junkbust) %dir /var/log/junkbuster
%config %{_sysconfdir}/logrotate.d/junkbuster
%attr(0744,junkbust,junkbust)/usr/sbin/junkbuster
%{_mandir}/man8/*
%config %{_sysconfdir}/rc.d/init.d/junkbuster


%changelog

* Sun Jun  3 2001 Stefan Waldherr <stefan@waldherr.org>
- rework of RPM

* Mon Sep 25 2000 Stefan Waldherr <stefan@waldherr.org>
- CLF Logging patch by davep@cyw.uklinux.net
- Hal DeVore <haldevore@earthling.net> fix akamaitech in blocklist

* Sun Sep 17 2000 Stefan Waldherr <stefan@waldherr.org>
- Steve Kemp skx@tardis.ed.ac.uk's javascript popup patch.
- Markus Breitenbach breitenb@rbg.informatik.tu-darmstadt.de supplied
  numerous fixes and enhancements for Steve's patch.
- adamlock@netscape.com (Adam Lock) in the windows version:
  - Taskbar activity spinner always spins even when logging is
  turned off (which is the default) - people who don't
  like the spinner can turn it off from a menu option.
  - Taskbar popup menu has a options submenu - people can now
  open the settings files for cookies, blockers etc.
  without opening the JB window.
  - Logging functionality works again
  - Buffer overflow is fixed - new code uses a bigger buffer
  and snprintf so it shouldn't overflow anymore.
- Fixed userid swa, group learning problem while installing.
  root must build RPM.
- Added patch by Benjamin Low <ben@snrc.uow.edu.au> that prevents JB to
  core dump when there is no log file.
- Tweaked SuSE startup with the help of mohataj@gmx.net and Doc.B@gmx.de.
- Fixed man page to include imagefile and popupfile.
- Sanity check for the statistics function added.
- "Patrick D'Cruze" <pdcruze@orac.iinet.net.au>: It seems Microsoft
 are transitioning Hotmail from FreeBSD/Apache to Windows 2000/IIS.
 With IIS/5, it appears to omit the trailing \r\n from http header
 only messages.  eg, when I visit http://www.hotmail.com, IIS/5
 responds with a HTTP 302 redirect header.  However, this header
 message is missing the trailing \r\n.  IIS/5 then closes the
 connection.  Junkbuster, unfortunately, discards the header becomes
 it thinks it is incomplete - and it is.  MS have transmitted an
 incomplete header!
- Added bug reports and patch submission forms in the docs.

* Mon Mar 20 2000 Stefan Waldherr <stefan@waldherr.org>
       Andrew <anw@tirana.freewire.co.uk> extended the JB:
       Display of statistics of the total number of requests and the number
       of requests filtered by junkbuster, also the percentage of requests
       filtered. Suppression of the listing of files on the proxy-args page.
       All stuff optional and configurable.

* Sun Sep 12 1999 Stefan Waldherr <stefan@waldherr.org>
       Jan Willamowius (jan@janhh.shnet.org) fixed a bug in the 
       code which prevented the JB from handling URLs of the form
       user:password@www.foo.com. Fixed.

* Mon Aug  2 1999 Stefan Waldherr <stefan@waldherr.org>
	Blank images are no longer cached, thanks to a hint from Markus 
        Breitenbach <breitenb@rbg.informatik.tu-darmstadt.de>. The user 
        agent is NO longer set by the Junkbuster. Sadly, many sites depend 
        on the correct browser version nowadays. Incorporated many 
	suggestions from Jan "Yenya" Kasprzak <kas@fi.muni.cz> for the
        spec file. Fixed logging problem and since runlevel 2 does not 
        use networking, I replaced /etc/rc.d/rc2.d/S84junkbuster with
        /etc/rc.d/rc2.d/K09junkbuster thanks to Shaw Walker 
        <walker@netgate.net>. You should now be able to build this RPM as 
        a non-root user (mathias@weidner.sem.lipsia.de).

* Sun Jan 31 1999 Stefan Waldherr <stefan@waldherr.org>
	/var/log/junkbuster set to nobody. Added /etc/junkbuster/imagelist
	to allow more sophisticated matching of blocked images. Logrotate
	logfile. Added files for auto-updating the blocklist et al.

* Wed Dec 16 1998 Stefan Waldherr <stefan@waldherr.org>
	Configure blank version via config file. No separate blank
	version anymore. Added Roland's <roland@spinnaker.rhein.de>
	patch to show a logo instead of a blank area. Added a suggestion
	from Alex <alex@cocoa.demon.co.uk>: /var/lock/subsys/junkbuster.
	More regexps in the blocklist. Prepared the forwardfile for
	squid. Extended image regexp with help from gabriel 
	<somlo@CS.ColoState.EDU>.

* Thu Nov 19 1998 Stefan Waldherr <stefan@waldherr.org>
	All RPMs now identify themselves in the show-proxy-args page.
	Released Windoze version. Run junkbuster as nobody instead of
	root. 

* Fri Oct 30 1998 Stefan Waldherr <stefan@waldherr.org>
	Newest version. First release (hence the little version number
	mixture -- 2.0.2-0 instead of 2.0-7). This version tightens 
	security over 2.0.1; some multi-user sites will need to change 
	the listen-address in the configuration file. The blank version of
        the Internet Junkbuster has a more sophisticated way of replacing
	images. All RPMs identify themselves in the show-proxy-args page.

* Thu Sep 23 1998 Stefan Waldherr <stefan@waldherr.org>
	Modified the blocking feature, so that only GIFs and JPEGs are
	blocked and replaced but not HTML pages. Thanks to 
	"Gerd Flender" <plgerd@informatik.uni-siegen.de> for this nice
	idea. Added numerous stuff to the blocklist. Keep patches in
        seperate files and no longer in diffs (easier to maintain).

* Tue Jun 16 1998 Stefan Waldherr <swa@cs.cmu.edu>
        Moved config files to /etc/junkbuster directory, moved man page,
	added BuildRoot directive (Thanks to Alexey Nogin <ayn2@cornell.edu>)
        Made new version junkbuster-raw (which is only a stripped version of 
        the junkuster rpm, i.e. without my blocklist, etc.)

* Tue Jun 16 1998 (2.0-1)
	Uhm, not that much. Just a new junkbuster version that
	fixes a couple of bugs ... and of course a bigger 
	blocklist with the unique Now-less-ads-than-ever(SM)
	feature.
	Oh, one thing: I changed the default user agent to Linux -- no 
	need anymore to support Apple.

* Tue Jun 16 1998 (2.0-0)
	Now-less-ads-than-ever (SM)
	compiled with gcc instead of cc
	compiled with -O3, thus it should be a little faster
	show-proxy-args now works
	/etc/junkbuster.init wasn't necessary

* Tue Jun 16 1998 (1.4)
	some more config files were put into /etc
	The junkbuster-blank rpm returns a 1x1 pixel image, that gets 
	displayed by Netscape instead of the blocked image.
	Read http://www.waldherr.org/junkbuster/ for
	further info.

* Tue Jun 16 1998 (1.3)
	The program has been moved to /usr/sbin (from /usr/local/bin)
	Init- and stopscripts (/etc/rc.d/rc*) have been added so
	that the junkbuster starts automatically during bootup.
	The /etc/blocklist file is much more sophisticated. Theoretically
	one should e.g. browse all major US and German newspapers without
	seeing one annoying ad.
	junkbuster.init was modified. It now starts junkbuster with an
	additional "-r @" flag.
