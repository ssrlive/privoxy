# $Id: Makefile.in,v 1.7 2001/06/03 17:09:09 swa Exp $
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
# $Log: Makefile.in,v $
#
#
%define PACKAGE_NAME junkbuster
%define PACKAGE_URL http://ijbswa.sourceforge.net
Summary: The Internet Junkbuster
Vendor: http://ijbswa.sourceforge.net
Name: %PACKAGE_NAME
Version: 2.9
Release: 4
Source0: http://www.waldherr.org/junkbuster/ijbswa.tar.gz
Copyright: GPL
BuildRoot: /tmp/junkbuster
Group: Networking/Utilities
URL: %PACKAGE_URL
Packager: Stefan Waldherr <stefan@waldherr.org>
Distribution: defineme
Obsoletes: junkbuster-raw junkbuster-blank
Prereq: chkconfig

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

Conflicts: junkbuster-raw junkbuster

%description
The Internet Junkbuster (TM) blocks unwanted banner ads and protects
your privacy from cookies and other threats. It's free under the GPL
(no warranty), runs under *NIX and works with almost any browser. You
need to clear you browser's cache and specify the proxy-server,
described in /usr/doc/junkbuster.  This is a modified version which
returns a blank GIF for blocked images by default.  But you can
configure this via /etc/junkbuster/config.

#
# -----------------------------------------------------------------------------
#

%prep

#
# -----------------------------------------------------------------------------
#

# 
%setup -c -n ijbswa

#
# -----------------------------------------------------------------------------
#

%build

#export DISTNAME='\"%PACKAGE_NAME-%PACKAGE_VERSION-%PACKAGE_RELEASE\"'
#export DISTURL='\"%PACKAGE_URL\"'
#make MORE_CFLAGS="$RPM_OPT_FLAGS"' -D_DISTNAME="$(DISTNAME)" -D_DISTURL="$(DISTURL)"'
# adds 486 optimization and stuff => bad
#make MORE_CFLAGS=' -D_DISTNAME="DDD" -D_DISTURL="UUU"'
./configure
make
strip junkbuster

#
# -----------------------------------------------------------------------------
#

%pre
if [ -f /etc/rc.d/init.d/junkbuster.init ]; then
        /etc/rc.d/init.d/junkbuster.init stop
fi
if [ -f /etc/rc.d/init.d/junkbuster ]; then
        /etc/rc.d/init.d/junkbuster stop
fi
rm -f /usr/local/bin/junkbuster
rm -f /usr/local/man/man1/junkbuster.1

if test -d /var/log/junkbuster
then
	mv -f /var/log/junkbuster /var/log/junkbuster.rpmorig
fi

#
# -----------------------------------------------------------------------------
#

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/{var/log/junkbuster,usr/{sbin,man/man8},etc/{junkbuster,junkbuster/templates,logrotate.d,cron.weekly,cron.monthly,rc.d/{init.d,rc{0,1,2,3,5,6}.d}}}
install -s -m 744 junkbuster $RPM_BUILD_ROOT/usr/sbin/junkbuster
cp -f junkbuster.1 $RPM_BUILD_ROOT/usr/man/man8/junkbuster.8
cp -f permissionsfile $RPM_BUILD_ROOT/etc/junkbuster/permissionsfile
cp -f re_filterfile $RPM_BUILD_ROOT/etc/junkbuster/re_filterfile
# cp -f blocklist $RPM_BUILD_ROOT/etc/junkbuster/blocklist
# cp -f imagelist $RPM_BUILD_ROOT/etc/junkbuster/imagelist
# cp -f cookiefile $RPM_BUILD_ROOT/etc/junkbuster/cookiefile
cp -f aclfile $RPM_BUILD_ROOT/etc/junkbuster/aclfile
cp -f config $RPM_BUILD_ROOT/etc/junkbuster/config
cp -f forward $RPM_BUILD_ROOT/etc/junkbuster/forward
cp -f trust $RPM_BUILD_ROOT/etc/junkbuster/trust
# cp -f popup $RPM_BUILD_ROOT/etc/junkbuster/popup
cp -f templates/default $RPM_BUILD_ROOT/etc/junkbuster/templates/
cp -f templates/show-status  $RPM_BUILD_ROOT/etc/junkbuster/templates/
cp -f templates/show-status  $RPM_BUILD_ROOT/etc/junkbuster/templates/

cp -f junkbuster.logrotate $RPM_BUILD_ROOT/etc/logrotate.d/junkbuster

install -m 755 junkbuster.init $RPM_BUILD_ROOT/etc/rc.d/init.d/junkbuster
install -m 744 -d $RPM_BUILD_ROOT/var/log/junkbuster

ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc0.d/K09junkbuster
ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc1.d/K09junkbuster
ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc2.d/K09junkbuster
ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S84junkbuster
ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S84junkbuster
ln -sf ../init.d/junkbuster $RPM_BUILD_ROOT/etc/rc.d/rc6.d/K09junkbuster


#
# -----------------------------------------------------------------------------
#

%preun
if [ -f /etc/rc.d/init.d/junkbuster.init ]; then
        /etc/rc.d/init.d/junkbuster.init stop
fi
if [ -f /etc/rc.d/init.d/junkbuster ]; then
        /etc/rc.d/init.d/junkbuster stop
fi

#
# -----------------------------------------------------------------------------
#

%post
cat << EOT >&2

Now you'll need to start junkbuster with 

	/etc/rc.d/init.d/junkbuster start

or simply reboot; It will be started automatically at boot time. 
Don't forget to add the proxy stuff in Netscape.

EOT

# check configuration of start/stop/ scripts
# /sbin/chkconfig --add junkbuster


#
# -----------------------------------------------------------------------------
#

%clean
rm -rf $RPM_BUILD_ROOT

#
# -----------------------------------------------------------------------------
#

%files
%defattr(-,root,root)
# %doc ijbfaq.html ijbman.html README README.TOO gpl.html 
%doc junkbuster.weekly junkbuster.monthly
%attr (-,nobody,nobody) /var/log/junkbuster
%config /etc/junkbuster/*
%config /etc/logrotate.d/junkbuster
%attr (-,nobody,nobody) /usr/sbin/junkbuster
/usr/man/man8/junkbuster.8
%config /etc/rc.d/init.d/junkbuster
%config(missingok) /etc/rc.d/rc0.d/K09junkbuster
%config(missingok) /etc/rc.d/rc1.d/K09junkbuster
%config(missingok) /etc/rc.d/rc2.d/K09junkbuster
%config(missingok) /etc/rc.d/rc3.d/S84junkbuster
%config(missingok) /etc/rc.d/rc5.d/S84junkbuster
%config(missingok) /etc/rc.d/rc6.d/K09junkbuster
